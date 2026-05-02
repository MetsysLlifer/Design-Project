#include "linked_list_visualizer.h"
#include "memory_manager.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int head_address = 0;
static int curr_address = 0;
static VisualNode visualNodes[MAX_MEM_NODES];
static int visualNodeCount = 0;
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

static SimStatus simStatus = SIM_IDLE;
static SimContext context = { FUNC_NONE, 0, 0, 0, INSERT_INDEX, DELETE_INDEX, 0, 0, 0, 0, 0, 1.0f, false };
static char errorMsg[256] = "";
static bool showError = false;

static const char* insertPseudocode[] = {
    "node *temp = malloc(sizeof(node))", // 0
    "temp->data = val",                  // 1
    "if (pos == 1) {",                  // 2
    "    temp->next = head",            // 3
    "    head = temp",                  // 4
    "} else {",                         // 5
    "    node *prev = head",            // 6
    "    for (i=1; i < pos-1; i++)",    // 7
    "        prev = prev->next",        // 8
    "    temp->next = prev->next",      // 9
    "    prev->next = temp",            // 10
    "}"                                 // 11
};

static const char* deletePseudocode[] = {
    "if (head == NULL) return",          // 0
    "if (pos == 1) {",                  // 1
    "    node *temp = head",            // 2
    "    head = head->next",            // 3
    "    free(temp)",                   // 4
    "} else {",                         // 5
    "    node *prev = head",            // 6
    "    for (i=1; i < pos-1; i++)",    // 7
    "        prev = prev->next",        // 8
    "    node *toDel = prev->next",     // 9
    "    prev->next = toDel->next",     // 10
    "    free(toDel)",                  // 11
    "}"                                 // 12
};

static void DrawPseudocode(int x, int y, const char** lines, int count, int currentLine) {
    Rectangle bg = { (float)x - 10, (float)y - 10, 320, (float)count * 20 + 20 };
    DrawRectangleRec(bg, (Color){ 235, 245, 255, 255 }); // Very light blue
    DrawRectangleLinesEx(bg, 1, DARKBLUE);
    DrawText("ALGORITHM (INSTRUCTION SET)", x, y - 25, 12, DARKBLUE);

    for (int i = 0; i < count; i++) {
        Color textColor = (Color){ 30, 60, 90, 255 };
        if (i == currentLine) {
            DrawRectangle(x - 5, y + i * 20, 310, 18, (Color){ 255, 255, 0, 150 });
            textColor = RED;
        }
        DrawText(lines[i], x, y + i * 20, 15, textColor);
    }
}

// Buffers for parameters
static char valBuf[8] = "10";
static char posBuf[8] = "1";
static char delValBuf[8] = "10";
static char delPosBuf[8] = "1";

static bool valEditMode = false;
static bool posEditMode = false;
static bool delValEditMode = false;
static bool delPosEditMode = false;

static Vector2 FindSpawnPosition(Vector2 basePos);
static void RemoveVisualNode(int address);

void LinkedListVisualizer_Init(void) {
    MemoryManager_Init();
    head_address = 0;
    curr_address = 0;
    visualNodeCount = 0;
    spawnCenter = (Vector2){ 0, 0 };
    viewCamera = (Camera2D){ 0 };
    simStatus = SIM_IDLE;
    context.type = FUNC_NONE;
    context.insertMode = INSERT_INDEX;
    context.deleteMode = DELETE_INDEX;
    context.prevAddress = 0;
    context.toDeleteAddress = 0;
    context.newNodeAddress = 0;
    context.currentLine = 0;
    context.totalLines = 0;
    context.lineProgress = 1.0f;
    showError = false;
}

static int GetListLength(void) {
    int length = 0;
    int current = head_address;
    while (current != 0) {
        MemoryNode* node = MemoryManager_GetNode(current);
        if (!node) break;
        length++;
        current = node->next_address;
    }
    return length;
}

static void ResetTraversal(void) {
    if (head_address == 0) {
        curr_address = 0;
        context.currentPos = 0;
    } else {
        curr_address = head_address;
        context.currentPos = 1;
    }
}

static AlgAction GetRequiredAction(void) {
    if (context.type == FUNC_INSERT) {
        switch (context.currentLine) {
            case 0: return ACT_MALLOC;
            case 1: return ACT_ASSIGN;
            case 2:
            case 5:
            case 7: return ACT_LOGIC;
            case 3:
            case 4:
            case 9:
            case 10: return ACT_LINK;
            case 6:
            case 8: return ACT_TRAVERSE;
            default: return ACT_NONE;
        }
    } else if (context.type == FUNC_DELETE) {
        switch (context.currentLine) {
            case 0:
            case 1:
            case 5:
            case 7: return ACT_LOGIC;
            case 2:
            case 3:
            case 9:
            case 10: return ACT_LINK;
            case 4:
            case 11: return ACT_FREE;
            case 6:
            case 8: return ACT_TRAVERSE;
            default: return ACT_NONE;
        }
    }
    return ACT_NONE;
}

static VisualNode* GetVisualNode(int address) {
    if (address == 0) return NULL;
    for (int i = 0; i < visualNodeCount; i++) {
        if (visualNodes[i].address == address) return &visualNodes[i];
    }
    return NULL;
}

void LinkedListVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
    
    // Smooth Animation
    for (int i = 0; i < visualNodeCount; i++) {
        if (!visualNodes[i].isDragging) {
            visualNodes[i].position.x += (visualNodes[i].targetPosition.x - visualNodes[i].position.x) * 0.15f;
            visualNodes[i].position.y += (visualNodes[i].targetPosition.y - visualNodes[i].position.y) * 0.15f;
        }
    }

    if (simStatus != SIM_IDLE && simStatus != SIM_EXECUTING) return;

    static int draggingIdx = -1;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int i = visualNodeCount - 1; i >= 0; i--) {
            Rectangle bounds = { visualNodes[i].position.x, visualNodes[i].position.y, 100, 50 };
            if (CheckCollisionPointRec(mouseWorldPos, bounds)) {
                draggingIdx = i;
                visualNodes[i].isDragging = true;
                break;
            }
        }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (draggingIdx != -1) {
            visualNodes[draggingIdx].isDragging = false;
            visualNodes[draggingIdx].targetPosition = visualNodes[draggingIdx].position;
        }
        draggingIdx = -1;
    }
    if (draggingIdx != -1) {
        visualNodes[draggingIdx].position = mouseWorldPos;
        visualNodes[draggingIdx].targetPosition = mouseWorldPos;
    }
}

void LinkedListVisualizer_NextStep(void) {
    context.lineProgress = 0.0f;
    if (context.type == FUNC_INSERT) {
        switch (context.currentLine) {
            case 0: // node *temp = malloc(sizeof(node))
                context.newNodeAddress = MemoryManager_Malloc(0);
                if (context.newNodeAddress != -1) {
                    Vector2 targetPos = FindSpawnPosition(spawnCenter);
                    visualNodes[visualNodeCount].address = context.newNodeAddress;
                    visualNodes[visualNodeCount].position = (Vector2){ targetPos.x, targetPos.y - 100 }; // Spawn from above
                    visualNodes[visualNodeCount].targetPosition = targetPos;
                    visualNodes[visualNodeCount].data = 0;
                    visualNodes[visualNodeCount].color = (Color){ (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), 255 };
                    visualNodes[visualNodeCount].isDragging = false;
                    visualNodeCount++;
                    context.currentLine = 1;
                }
                break;
            case 1: // temp->data = val
                {
                    MemoryNode* n = MemoryManager_GetNode(context.newNodeAddress);
                    if (n) n->value = context.targetVal;
                    VisualNode* vn = GetVisualNode(context.newNodeAddress);
                    if (vn) vn->data = (char)context.targetVal;
                    context.currentLine = 2;
                }
                break;
            case 2: // if (pos == 1)
                if (context.targetPos == 1) context.currentLine = 3;
                else context.currentLine = 5;
                break;
            case 3: // temp->next = head
                {
                    MemoryNode* n = MemoryManager_GetNode(context.newNodeAddress);
                    if (n) n->next_address = head_address;
                    context.currentLine = 4;
                }
                break;
            case 4: // head = temp
                head_address = context.newNodeAddress;
                context.newNodeAddress = 0;
                curr_address = 0;
                simStatus = SIM_IDLE;
                context.type = FUNC_NONE;
                break;
            case 5: // else {
                context.currentLine = 6;
                break;
            case 6: // node *prev = head
                curr_address = head_address;
                context.currentPos = 1;
                context.currentLine = 7;
                break;
            case 7: // for (i=1; i < pos-1; i++)
                if (context.currentPos < context.targetPos - 1) context.currentLine = 8;
                else context.currentLine = 9;
                break;
            case 8: // prev = prev->next
                LinkedListVisualizer_TraverseStep();
                context.currentLine = 7; // Loop back
                break;
            case 9: // temp->next = prev->next
                {
                    MemoryNode* n = MemoryManager_GetNode(context.newNodeAddress);
                    MemoryNode* prev = MemoryManager_GetNode(curr_address);
                    if (n && prev) n->next_address = prev->next_address;
                    context.currentLine = 10;
                }
                break;
            case 10: // prev->next = temp
                {
                    MemoryNode* prev = MemoryManager_GetNode(curr_address);
                    if (prev) prev->next_address = context.newNodeAddress;
                    context.currentLine = 11;
                }
                break;
            case 11: // }
                context.newNodeAddress = 0;
                curr_address = 0;
                simStatus = SIM_IDLE;
                context.type = FUNC_NONE;
                break;
        }
    } else if (context.type == FUNC_DELETE) {
        switch (context.currentLine) {
            case 0: // if (head == NULL) return
                if (head_address == 0) {
                    curr_address = 0;
                    simStatus = SIM_IDLE;
                    context.type = FUNC_NONE;
                } else context.currentLine = 1;
                break;
            case 1: // if (pos == 1)
                if (context.targetPos == 1) context.currentLine = 2;
                else context.currentLine = 5;
                break;
            case 2: // node *temp = head
                context.toDeleteAddress = head_address;
                context.currentLine = 3;
                break;
            case 3: // head = head->next
                {
                    MemoryNode* n = MemoryManager_GetNode(head_address);
                    if (n) head_address = n->next_address;
                    context.currentLine = 4;
                }
                break;
            case 4: // free(temp)
                MemoryManager_Free(context.toDeleteAddress);
                RemoveVisualNode(context.toDeleteAddress);
                context.toDeleteAddress = 0;
                curr_address = 0;
                simStatus = SIM_IDLE;
                context.type = FUNC_NONE;
                break;
            case 5: // else {
                context.currentLine = 6;
                break;
            case 6: // node *prev = head
                curr_address = head_address;
                context.currentPos = 1;
                context.currentLine = 7;
                break;
            case 7: // for (i=1; i < pos-1; i++)
                if (context.currentPos < context.targetPos - 1) context.currentLine = 8;
                else context.currentLine = 9;
                break;
            case 8: // prev = prev->next
                LinkedListVisualizer_TraverseStep();
                context.currentLine = 7; // Loop back
                break;
            case 9: // node *toDel = prev->next
                {
                    MemoryNode* prev = MemoryManager_GetNode(curr_address);
                    if (prev) context.toDeleteAddress = prev->next_address;
                    context.currentLine = 10;
                }
                break;
            case 10: // prev->next = toDel->next
                {
                    MemoryNode* prev = MemoryManager_GetNode(curr_address);
                    MemoryNode* toDel = MemoryManager_GetNode(context.toDeleteAddress);
                    if (prev && toDel) prev->next_address = toDel->next_address;
                    context.currentLine = 11;
                }
                break;
            case 11: // free(toDel)
                MemoryManager_Free(context.toDeleteAddress);
                RemoveVisualNode(context.toDeleteAddress);
                context.currentLine = 12;
                break;
            case 12: // }
                context.toDeleteAddress = 0;
                curr_address = 0;
                simStatus = SIM_IDLE;
                context.type = FUNC_NONE;
                break;
        }
    }
}

void LinkedListVisualizer_SetSpawnCenter(Vector2 center) {
    spawnCenter = center;
}

void LinkedListVisualizer_SetCamera(Camera2D cam) {
    viewCamera = cam;
}

void LinkedListVisualizer_TraverseStep(void) {
    if (curr_address == 0) {
        showError = true;
        sprintf(errorMsg, "CRITICAL ERROR: Cannot traverse past NULL.");
        return;
    }
    MemoryNode* n = MemoryManager_GetNode(curr_address);
    if (!n) return;
    if (context.type == FUNC_DELETE) {
        context.prevAddress = curr_address;
    }
    curr_address = n->next_address;
    context.currentPos++;
}

static void RemoveVisualNode(int address) {
    for (int i = 0; i < visualNodeCount; i++) {
        if (visualNodes[i].address == address) {
            visualNodes[i] = visualNodes[visualNodeCount - 1];
            visualNodeCount--;
            return;
        }
    }
}

static bool ResolveDeleteTarget(int* outTargetPos) {
    int length = GetListLength();
    int targetPos = 0;
    int targetAddr = 0;
    int prevAddr = 0;

    if (length == 0) {
        showError = true;
        sprintf(errorMsg, "LOGIC ERROR: List is empty.");
        return false;
    }

    if (context.deleteMode == DELETE_FIRST) {
        targetPos = 1;
    } else if (context.deleteMode == DELETE_LAST) {
        targetPos = length;
    } else if (context.deleteMode == DELETE_INDEX) {
        if (context.targetPos < 1 || context.targetPos > length) {
            showError = true;
            sprintf(errorMsg, "LOGIC ERROR: Index out of range (1-%d).", length);
            return false;
        }
        targetPos = context.targetPos;
    } else if (context.deleteMode == DELETE_ELEMENT) {
        int current = head_address;
        int pos = 1;
        while (current != 0) {
            MemoryNode* node = MemoryManager_GetNode(current);
            if (!node) break;
            if (node->value == context.targetVal) {
                targetPos = pos;
                break;
            }
            prevAddr = current;
            current = node->next_address;
            pos++;
        }
        if (targetPos == 0) {
            showError = true;
            sprintf(errorMsg, "LOGIC ERROR: Value %d not found.", context.targetVal);
            return false;
        }
        targetAddr = current;
    }

    if (targetAddr == 0) {
        int current = head_address;
        int pos = 1;
        prevAddr = 0;
        while (current != 0 && pos < targetPos) {
            MemoryNode* node = MemoryManager_GetNode(current);
            if (!node) break;
            prevAddr = current;
            current = node->next_address;
            pos++;
        }
        targetAddr = current;
    }

    if (targetAddr == 0) {
        showError = true;
        sprintf(errorMsg, "LOGIC ERROR: Target node not found.");
        return false;
    }

    *outTargetPos = targetPos;
    return true;
}

static void StartDeleteExecution(void) {
    int targetPos = 0;
    if (!ResolveDeleteTarget(&targetPos)) return;

    context.targetPos = targetPos;
    context.prevAddress = 0;
    context.toDeleteAddress = 0;
    ResetTraversal();
}

static bool IsNodeOverlap(Rectangle candidate) {
    for (int i = 0; i < visualNodeCount; i++) {
        Rectangle existing = { visualNodes[i].position.x - 10, visualNodes[i].position.y - 10, 120, 70 };
        if (CheckCollisionRecs(candidate, existing)) return true;
    }
    return false;
}

static Vector2 FindSpawnPosition(Vector2 basePos) {
    Rectangle candidate = { basePos.x - 10, basePos.y - 10, 120, 70 };
    if (!IsNodeOverlap(candidate)) return basePos;

    const float stepX = 130.0f;
    const float stepY = 80.0f;
    for (int ring = 1; ring < 7; ring++) {
        for (int dx = -ring; dx <= ring; dx++) {
            for (int dy = -ring; dy <= ring; dy++) {
                if (abs(dx) != ring && abs(dy) != ring) continue;
                Vector2 pos = { basePos.x + dx * stepX, basePos.y + dy * stepY };
                candidate = (Rectangle){ pos.x - 10, pos.y - 10, 120, 70 };
                if (!IsNodeOverlap(candidate)) return pos;
            }
        }
    }
    return basePos;
}

static Vector2 QuadraticBezier(Vector2 p0, Vector2 p1, Vector2 p2, float t) {
    float u = 1.0f - t;
    Vector2 a = Vector2Scale(p0, u * u);
    Vector2 b = Vector2Scale(p1, 2.0f * u * t);
    Vector2 c = Vector2Scale(p2, t * t);
    return Vector2Add(Vector2Add(a, b), c);
}

static void DrawCurvedArrow(Vector2 start, Vector2 end, Color color, float pct) {
    if (pct <= 0.0f) return;
    Vector2 control = { (start.x + end.x) / 2.0f, fminf(start.y, end.y) - 60.0f };
    Vector2 prev = start;
    Vector2 secondToLast = start;
    
    int totalSteps = 20;
    int visibleSteps = (int)((float)totalSteps * pct);
    if (visibleSteps < 1) visibleSteps = 1;

    for (int i = 1; i <= visibleSteps; i++) {
        float t = (float)i / (float)totalSteps;
        Vector2 point = QuadraticBezier(start, control, end, t);
        DrawLineEx(prev, point, 3.0f, color);
        secondToLast = prev;
        prev = point;
    }

    if (pct >= 0.95f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, secondToLast));
        Vector2 side = { -dir.y, dir.x };
        float length = 18.0f;
        float width = 10.0f;
        Vector2 base = Vector2Subtract(end, Vector2Scale(dir, length));
        Vector2 p1 = Vector2Add(base, Vector2Scale(side, width));
        Vector2 p2 = Vector2Subtract(base, Vector2Scale(side, width));
        
        DrawTriangle(end, p2, p1, color);
        DrawLineEx(end, p1, 1.0f, color);
        DrawLineEx(end, p2, 1.0f, color);
    }
}

static void DrawStraightArrow(Vector2 start, Vector2 end, Color color, float pct) {
    if (pct <= 0.0f) return;
    Vector2 currentEnd = Vector2Add(start, Vector2Scale(Vector2Subtract(end, start), pct));
    
    DrawLineEx(start, currentEnd, 3.0f, color);
    
    if (pct >= 0.95f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
        Vector2 side = { -dir.y, dir.x };
        float length = 18.0f;
        float width = 10.0f;
        Vector2 base = Vector2Subtract(end, Vector2Scale(dir, length));
        Vector2 p1 = Vector2Add(base, Vector2Scale(side, width));
        Vector2 p2 = Vector2Subtract(base, Vector2Scale(side, width));
        
        DrawTriangle(end, p2, p1, color);
        DrawLineEx(end, p1, 1.0f, color);
        DrawLineEx(end, p2, 1.0f, color);
    }
}

static Rectangle GetPointerFieldBox(VisualNode* vn) {
    return (Rectangle){ vn->position.x + 58, vn->position.y + 15, 34, 20 };
}

static void DrawStackPointerBox(const char* label, Rectangle box, Vector2 tipPos, Color color, bool showNull, float pct) {
    DrawRectangleRec(box, WHITE);
    DrawRectangleLinesEx(box, 1, color);
    DrawText(label, box.x + 6, box.y + 4, 12, color);
    
    Vector2 arrowEnd = tipPos;
    if (showNull) {
        arrowEnd.x -= 25.0f;
    }
    
    DrawStraightArrow((Vector2){ box.x + box.width, box.y + box.height / 2.0f }, arrowEnd, color, pct);

    if (showNull && pct >= 0.95f) {
        Rectangle nullBox = { tipPos.x - 25, tipPos.y - 10, 50, 20 };
        DrawRectangleRec(nullBox, (Color){ 30, 30, 30, 255 });
        DrawRectangleLinesEx(nullBox, 1, (Color){ 220, 220, 220, 255 });
        DrawText("NULL", nullBox.x + 8, nullBox.y + 4, 10, (Color){ 220, 220, 220, 255 });
    }
}

static Vector2 GetPointerFieldCenterScreen(VisualNode* vn) {
    Rectangle box = GetPointerFieldBox(vn);
    Vector2 center = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };
    return GetWorldToScreen2D(center, viewCamera);
}

static Vector2 GetNodeLeftCenterScreen(VisualNode* vn) {
    Vector2 leftCenter = { vn->position.x, vn->position.y + 25 };
    return GetWorldToScreen2D(leftCenter, viewCamera);
}

void LinkedListVisualizer_Draw(void) {
    // 1. Draw Nodes (including potential temp node)
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i];
        Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
        
        Color bgColor = vn->color; 
        if (vn->address == context.newNodeAddress) bgColor = (Color){ 200, 255, 200, 255 };
        if (vn->address == context.toDeleteAddress) bgColor = (Color){ 255, 200, 200, 255 };
        if (vn->address == curr_address) bgColor = (Color){ 255, 255, 200, 255 };

        DrawRectangleRec(rec, bgColor);
        DrawRectangleLinesEx(rec, 2, DARKGREEN); // Heap object border
        DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, DARKGREEN);
        char val[4]; sprintf(val, "%d", (int)vn->data);
        DrawText(val, vn->position.x + 15, vn->position.y + 15, 15, WHITE);
        DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, WHITE);
        char addr[16]; sprintf(addr, "0x%X", vn->address);
        DrawText(addr, vn->position.x, vn->position.y - 12, 10, DARKGREEN);
    }

    // 2. Draw Connections on top
    int current = head_address;
    while (current != 0) {
        MemoryNode* node = MemoryManager_GetNode(current);
        VisualNode* vn = GetVisualNode(current);
        if (vn && node->next_address != 0) {
            VisualNode* nextVn = GetVisualNode(node->next_address);
            if (nextVn) {
                float progress = 1.0f;
                // Animate if this is the 'prev->next' change in Insert (line 10) or Delete (line 10)
                if (simStatus == SIM_EXECUTING && current == curr_address && context.currentLine == 10) {
                    progress = context.lineProgress;
                }
                
                Rectangle pointerBox = GetPointerFieldBox(vn);
                Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                DrawCurvedArrow(start, end, BLACK, progress);
            }
        }
        current = node->next_address;
    }

    // Draw temp node connections if any
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i];
        MemoryNode* node = MemoryManager_GetNode(vn->address);
        if (node && node->next_address != 0) {
            bool inMainList = false;
            int c = head_address;
            while(c != 0) { if(c == vn->address) { inMainList = true; break; } c = MemoryManager_GetNode(c)->next_address; }
            
            if (!inMainList) {
                VisualNode* nextVn = GetVisualNode(node->next_address);
                if (nextVn) {
                    float progress = 1.0f;
                    // Animate if this is the 'temp->next' change in Insert (line 3 or 9)
                    if (simStatus == SIM_EXECUTING && vn->address == context.newNodeAddress && 
                       (context.currentLine == 3 || context.currentLine == 9)) {
                        progress = context.lineProgress;
                    }

                    Rectangle pointerBox = GetPointerFieldBox(vn);
                    Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                    Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                    DrawCurvedArrow(start, end, DARKGRAY, progress);
                }
            }
        }
    }
}

int LinkedListVisualizer_GetTraversalAddress(void) {
    if (simStatus != SIM_EXECUTING) return 0;
    return curr_address;
}

bool LinkedListVisualizer_IsBusy(void) {
    return simStatus != SIM_IDLE;
}

void LinkedListVisualizer_CancelInteraction(void) {
    if (context.type == FUNC_INSERT && context.newNodeAddress != 0) {
        MemoryManager_Free(context.newNodeAddress);
        RemoveVisualNode(context.newNodeAddress);
    }
    
    simStatus = SIM_IDLE;
    context.type = FUNC_NONE;
    context.prevAddress = 0;
    context.toDeleteAddress = 0;
    context.newNodeAddress = 0;
    curr_address = 0;
    showError = false;
}

void LinkedListVisualizer_DrawUI(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 0. THE EXECUTOR (CPU) - Moved below back button
    Rectangle cpuBox = { 20, 60, 240, 70 };
    DrawRectangleRec(cpuBox, (Color){ 230, 240, 255, 255 }); // Cool Blue
    DrawRectangleLinesEx(cpuBox, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cpuBox.x + 10, cpuBox.y + 10, 14, DARKBLUE);
    
    if (simStatus == SIM_EXECUTING) {
        char func[64];
        if (context.type == FUNC_INSERT) {
            sprintf(func, "INSERT(List, %d, %d)", context.targetVal, context.targetPos);
        } else if (context.type == FUNC_DELETE) {
            sprintf(func, "DELETE(List, %d)", context.targetPos);
        } else {
            sprintf(func, "FUNC(List)");
        }
        DrawText(func, cpuBox.x + 20, cpuBox.y + 35, 12, DARKBLUE);
        
        char pcTxt[32]; sprintf(pcTxt, "PC: 0x%X", 0x800 + (context.currentLine * 4));
        DrawText(pcTxt, cpuBox.x + 20, cpuBox.y + 50, 11, BLUE);

        // Execution Path Arrow (CPU -> Pseudocode)
        Vector2 cpuRight = { cpuBox.x + cpuBox.width, cpuBox.y + cpuBox.height / 2.0f };
        Vector2 codeLeft = { (float)sw - 580, 60.0f + (context.currentLine * 20) };
        DrawStraightArrow(cpuRight, codeLeft, BLUE, 1.0f);
    } else {
        DrawText("PC: IDLE", cpuBox.x + 20, cpuBox.y + 35, 12, BLUE);
    }

    // 1. THE STACK (Warm Orange) - Left Center
    Rectangle stackBox = { 20, (float)sh/2 - 80, 240, 190 };
    DrawRectangleRec(stackBox, (Color){ 255, 245, 230, 255 });
    DrawRectangleLinesEx(stackBox, 2, (Color){ 255, 161, 0, 255 });
    DrawText("THE STACK", stackBox.x + 10, stackBox.y + 10, 14, (Color){ 200, 100, 0, 255 });

    Rectangle listBox = { stackBox.x + 40, stackBox.y + 100, 160, 35 };
    DrawRectangleRec(listBox, WHITE);
    DrawRectangleLinesEx(listBox, 1, BLACK);
    DrawText("List (ptr)", listBox.x + 55, listBox.y + 10, 13, BLACK);

    VisualNode* headNode = GetVisualNode(head_address);
    if (headNode) {
        float listProg = 1.0f;
        if (simStatus == SIM_EXECUTING) {
            if (context.type == FUNC_INSERT && context.currentLine == 4) listProg = context.lineProgress;
            if (context.type == FUNC_DELETE && context.currentLine == 3) listProg = context.lineProgress;
        }
        
        Vector2 listTip = GetNodeLeftCenterScreen(headNode);
        DrawStraightArrow((Vector2){ listBox.x + listBox.width, listBox.y + listBox.height / 2.0f }, listTip, BLACK, listProg);
    } else {
        Vector2 listTip = (Vector2){ listBox.x + listBox.width + 100, listBox.y + listBox.height / 2.0f };
        DrawStackPointerBox("List", listBox, listTip, BLACK, true, 1.0f);
    }

    if (simStatus == SIM_EXECUTING) {
        Rectangle travBox = { stackBox.x + 20, stackBox.y + 40, 85, 24 };
        bool showTrav = false;
        if (context.type == FUNC_INSERT && context.currentLine >= 6) showTrav = true;
        if (context.type == FUNC_DELETE && context.currentLine >= 6) showTrav = true;

        if (showTrav) {
            float travProg = 1.0f;
            if (context.currentLine == 6 || context.currentLine == 8) travProg = context.lineProgress;

            if (curr_address != 0) {
                VisualNode* travNode = GetVisualNode(curr_address);
                if (travNode) {
                    MemoryNode* travMem = MemoryManager_GetNode(curr_address);
                    bool isNull = (travMem && travMem->next_address == 0);
                    Vector2 travTip = isNull ? GetPointerFieldCenterScreen(travNode) : GetNodeLeftCenterScreen(travNode);
                    DrawStackPointerBox("traverse", travBox, travTip, (Color){ 255, 0, 110, 255 }, isNull, travProg);
                }
            } else {
                Vector2 listCenter = { listBox.x + listBox.width / 2.0f, listBox.y + listBox.height / 2.0f };
                DrawStackPointerBox("traverse", travBox, listCenter, (Color){ 255, 0, 110, 255 }, false, travProg);
            }
        }
    }

    char listTxt[32]; sprintf(listTxt, "List: 0x%X", head_address);
    DrawText(listTxt, stackBox.x + 20, stackBox.y + 75, 13, (Color){ 150, 80, 0, 255 });
    char currTxt[32]; sprintf(currTxt, "curr: 0x%X", curr_address);
    DrawText(currTxt, stackBox.x + 20, stackBox.y + 145, 13, (Color){ 150, 80, 0, 255 });

    // Pseudocode Display - Upper left of memory
    if (simStatus == SIM_EXECUTING) {
        if (context.type == FUNC_INSERT) {
            DrawPseudocode(sw - 580, 60, insertPseudocode, 12, context.currentLine);
        } else if (context.type == FUNC_DELETE) {
            DrawPseudocode(sw - 580, 60, deletePseudocode, 13, context.currentLine);
        }
    }

    // 2. Bottom Function Pills
    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    Rectangle insBtn = { 50, (float)sh - 80, 120, 45 };
    Rectangle delBtn = { 180, (float)sh - 80, 120, 45 };
    
    if (GuiButton(insBtn, "Insert")) {
        simStatus = SIM_SELECT_INSERT;
        context.type = FUNC_INSERT;
    }
    if (GuiButton(delBtn, "Delete")) {
        simStatus = SIM_SELECT_DELETE;
        context.type = FUNC_DELETE;
    }

    // 3. Conditional Function Keys (Bottom Right/Center)
    if (simStatus == SIM_EXECUTING) {
        DrawText("ALGORITHM STEPS", 400, sh - 100, 11, DARKGRAY);
        Rectangle nextKey = { 400, (float)sh - 80, 200, 45 };
        if (GuiButton(nextKey, "NEXT STEP")) {
            LinkedListVisualizer_NextStep();
        }
        
        Rectangle cancelKey = { 610, (float)sh - 80, 100, 45 };
        if (GuiButton(cancelKey, "CANCEL")) {
            LinkedListVisualizer_CancelInteraction();
        }
    }

    // 4. Insert Mode Selection
    if (simStatus == SIM_SELECT_INSERT) {
        Rectangle box = { (float)sw/2 - 180, (float)sh/2 - 90, 360, 180 };
        DrawRectangleRec(box, WHITE);
        DrawRectangleLinesEx(box, 2, BLACK);
        DrawText("INSERT OPTIONS", box.x + 90, box.y + 20, 16, BLACK);

        if (GuiButton((Rectangle){box.x + 30, box.y + 60, 300, 30}, "Insert First")) {
            context.insertMode = INSERT_FIRST;
            simStatus = SIM_INPUT_PARAMS;
        }
        if (GuiButton((Rectangle){box.x + 30, box.y + 95, 300, 30}, "Insert Last")) {
            context.insertMode = INSERT_LAST;
            simStatus = SIM_INPUT_PARAMS;
        }
        if (GuiButton((Rectangle){box.x + 30, box.y + 130, 300, 30}, "Insert Index")) {
            context.insertMode = INSERT_INDEX;
            simStatus = SIM_INPUT_PARAMS;
        }
    }

    // 5. Delete Mode Selection
    if (simStatus == SIM_SELECT_DELETE) {
        Rectangle box = { (float)sw/2 - 180, (float)sh/2 - 110, 360, 220 };
        DrawRectangleRec(box, WHITE);
        DrawRectangleLinesEx(box, 2, BLACK);
        DrawText("DELETE OPTIONS", box.x + 90, box.y + 20, 16, BLACK);

        if (GuiButton((Rectangle){box.x + 30, box.y + 60, 300, 30}, "Delete First")) {
            context.deleteMode = DELETE_FIRST;
            simStatus = SIM_INPUT_PARAMS;
        }
        if (GuiButton((Rectangle){box.x + 30, box.y + 95, 300, 30}, "Delete Last")) {
            context.deleteMode = DELETE_LAST;
            simStatus = SIM_INPUT_PARAMS;
        }
        if (GuiButton((Rectangle){box.x + 30, box.y + 130, 300, 30}, "Delete Index")) {
            context.deleteMode = DELETE_INDEX;
            simStatus = SIM_INPUT_PARAMS;
        }
        if (GuiButton((Rectangle){box.x + 30, box.y + 165, 300, 30}, "Delete Element")) {
            context.deleteMode = DELETE_ELEMENT;
            simStatus = SIM_INPUT_PARAMS;
        }
    }

    // 6. Integrated Parameter Input (No full screen dimming)
    if (simStatus == SIM_INPUT_PARAMS) {
        Rectangle paramBox = { (float)sw/2 - 170, (float)sh/2 - 110, 340, 220 };
        DrawRectangleRec(paramBox, WHITE);
        DrawRectangleLinesEx(paramBox, 2, BLACK);

        if (context.type == FUNC_INSERT) {
            DrawText("INSERT(List, val, pos)", paramBox.x + 60, paramBox.y + 20, 15, BLACK);
            DrawText("Value:", paramBox.x + 30, paramBox.y + 70, 12, BLACK);
            if (GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 65, 160, 30}, valBuf, 8, valEditMode)) {
                valEditMode = !valEditMode;
                posEditMode = false;
            }

            if (context.insertMode == INSERT_INDEX) {
                DrawText("Position:", paramBox.x + 30, paramBox.y + 110, 12, BLACK);
                if (GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 105, 160, 30}, posBuf, 8, posEditMode)) {
                    posEditMode = !posEditMode;
                    valEditMode = false;
                }
            }

            if (GuiButton((Rectangle){paramBox.x + 120, paramBox.y + 160, 100, 35}, "START")) {
                int length = GetListLength();
                context.targetVal = atoi(valBuf);
                if (context.insertMode == INSERT_FIRST) {
                    context.targetPos = 1;
                } else if (context.insertMode == INSERT_LAST) {
                    context.targetPos = length + 1;
                } else {
                    context.targetPos = atoi(posBuf);
                    if (context.targetPos < 1 || context.targetPos > length + 1) {
                        showError = true;
                        sprintf(errorMsg, "LOGIC ERROR: Position out of range (1-%d).", length + 1);
                        return;
                    }
                }
                valEditMode = false;
                posEditMode = false;
                ResetTraversal();
                context.currentLine = 0;
                context.totalLines = 12;
                simStatus = SIM_EXECUTING;
            }
        } else if (context.type == FUNC_DELETE) {
            DrawText("DELETE(List)", paramBox.x + 110, paramBox.y + 20, 15, BLACK);

            if (context.deleteMode == DELETE_INDEX) {
                DrawText("Index:", paramBox.x + 30, paramBox.y + 80, 12, BLACK);
                if (GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 75, 160, 30}, delPosBuf, 8, delPosEditMode)) {
                    delPosEditMode = !delPosEditMode;
                    delValEditMode = false;
                }
            } else if (context.deleteMode == DELETE_ELEMENT) {
                DrawText("Value:", paramBox.x + 30, paramBox.y + 80, 12, BLACK);
                if (GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 75, 160, 30}, delValBuf, 8, delValEditMode)) {
                    delValEditMode = !delValEditMode;
                    delPosEditMode = false;
                }
            } else {
                DrawText("No extra input required.", paramBox.x + 70, paramBox.y + 90, 12, DARKGRAY);
            }

            if (GuiButton((Rectangle){paramBox.x + 120, paramBox.y + 160, 100, 35}, "START")) {
                if (context.deleteMode == DELETE_INDEX) {
                    context.targetPos = atoi(delPosBuf);
                } else if (context.deleteMode == DELETE_ELEMENT) {
                    context.targetVal = atoi(delValBuf);
                }
                delValEditMode = false;
                delPosEditMode = false;
                StartDeleteExecution();
                if (!showError) {
                    context.currentLine = 0;
                    context.totalLines = 13;
                    simStatus = SIM_EXECUTING;
                }
            }
        }
    }

    // 5. Error Popup
    if (showError) {
        Rectangle errBox = { (float)sw/2 - 200, (float)sh/2 - 50, 400, 120 };
        DrawRectangleRec(errBox, WHITE);
        DrawRectangleLinesEx(errBox, 4, BLACK);
        DrawText("LOGIC ERROR", errBox.x + 130, errBox.y + 20, 18, BLACK);
        DrawText(errorMsg, errBox.x + 20, errBox.y + 55, 12, BLACK);
        if (GuiButton((Rectangle){errBox.x + 150, errBox.y + 80, 100, 30}, "OK")) showError = false;
    }

    DrawText("List: Linked-List", sw - 250, sh - 60, 25, BLACK);
}
