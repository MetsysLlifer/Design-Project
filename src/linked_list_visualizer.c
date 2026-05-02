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
static SimContext context = { FUNC_NONE, 0, 0, 0, INSERT_INDEX, DELETE_INDEX, 0, 0, 0, 0, 0 };
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
    Rectangle bg = { (float)x - 10, (float)y - 10, 300, (float)count * 20 + 20 };
    DrawRectangleRec(bg, (Color){ 240, 240, 240, 255 });
    DrawRectangleLinesEx(bg, 1, DARKGRAY);
    DrawText("ALGORITHM PSEUDOCODE", x, y - 25, 12, DARKGRAY);

    for (int i = 0; i < count; i++) {
        Color textColor = BLACK;
        if (i == currentLine) {
            DrawRectangle(x - 5, y + i * 20, 290, 18, (Color){ 255, 255, 0, 150 });
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

static VisualNode* GetVisualNode(int address) {
    if (address == 0) return NULL;
    for (int i = 0; i < visualNodeCount; i++) {
        if (visualNodes[i].address == address) return &visualNodes[i];
    }
    return NULL;
}

void LinkedListVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
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
        if (draggingIdx != -1) visualNodes[draggingIdx].isDragging = false;
        draggingIdx = -1;
    }
    if (draggingIdx != -1) visualNodes[draggingIdx].position = mouseWorldPos;
}

void LinkedListVisualizer_NextStep(void) {
    if (context.type == FUNC_INSERT) {
        switch (context.currentLine) {
            case 0: // node *temp = malloc(sizeof(node))
                context.newNodeAddress = MemoryManager_Malloc(0);
                if (context.newNodeAddress != -1) {
                    visualNodes[visualNodeCount].address = context.newNodeAddress;
                    visualNodes[visualNodeCount].position = FindSpawnPosition(spawnCenter);
                    visualNodes[visualNodeCount].data = 0;
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
                simStatus = SIM_IDLE;
                context.type = FUNC_NONE;
                break;
        }
    } else if (context.type == FUNC_DELETE) {
        switch (context.currentLine) {
            case 0: // if (head == NULL) return
                if (head_address == 0) {
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

static void DrawCurvedArrow(Vector2 start, Vector2 end, Color color) {
    Vector2 control = { (start.x + end.x) / 2.0f, fminf(start.y, end.y) - 30.0f };
    Vector2 prev = start;
    for (int i = 1; i <= 20; i++) {
        float t = (float)i / 20.0f;
        Vector2 point = QuadraticBezier(start, control, end, t);
        DrawLineEx(prev, point, 2.0f, color);
        prev = point;
    }

    Vector2 dir = Vector2Normalize(Vector2Subtract(end, prev));
    Vector2 left = Vector2Rotate(dir, 0.6f);
    Vector2 right = Vector2Rotate(dir, -0.6f);
    DrawTriangle(end, Vector2Subtract(end, Vector2Scale(left, 10.0f)), Vector2Subtract(end, Vector2Scale(right, 10.0f)), color);
}

static void DrawStraightArrow(Vector2 start, Vector2 end, Color color) {
    DrawLineEx(start, end, 2.0f, color);
    Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
    Vector2 left = Vector2Rotate(dir, 0.6f);
    Vector2 right = Vector2Rotate(dir, -0.6f);
    DrawTriangle(end, Vector2Subtract(end, Vector2Scale(left, 10.0f)), Vector2Subtract(end, Vector2Scale(right, 10.0f)), color);
}

static Rectangle GetPointerFieldBox(VisualNode* vn) {
    return (Rectangle){ vn->position.x + 58, vn->position.y + 15, 34, 20 };
}

static void DrawStackPointerBox(const char* label, Rectangle box, Vector2 tipPos, Color color, bool showNull) {
    DrawRectangleRec(box, WHITE);
    DrawRectangleLinesEx(box, 1, color);
    DrawText(label, box.x + 6, box.y + 4, 12, color);
    DrawStraightArrow((Vector2){ box.x + box.width, box.y + box.height / 2.0f }, tipPos, color);

    if (showNull) {
        Rectangle nullBox = { tipPos.x - 25, tipPos.y - 10, 50, 20 };
        DrawRectangleRec(nullBox, (Color){ 10, 10, 10, 255 });
        DrawRectangleLinesEx(nullBox, 1, (Color){ 200, 200, 200, 255 });
        DrawText("NULL", nullBox.x + 8, nullBox.y + 4, 10, (Color){ 200, 200, 200, 255 });
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

static void DrawWorldTraverseBox(VisualNode* vn, bool pointToPointerField) {
    if (!vn) return;
    Rectangle box = { vn->position.x + 10, vn->position.y - 40, 90, 24 };
    DrawRectangleRec(box, WHITE);
    DrawRectangleLinesEx(box, 1, BLACK);
    DrawText("Traverse", box.x + 8, box.y + 4, 12, BLACK);

    Vector2 tipPos = pointToPointerField
        ? (Vector2){ vn->position.x + 75, vn->position.y + 25 }
        : (Vector2){ vn->position.x + 10, vn->position.y + 25 };
    DrawStraightArrow((Vector2){ box.x + box.width / 2.0f, box.y + box.height }, tipPos, BLACK);

    if (pointToPointerField) {
        Rectangle pointerBox = GetPointerFieldBox(vn);
        DrawRectangleLinesEx(pointerBox, 1, BLACK);
        DrawText("NULL", pointerBox.x + 2, pointerBox.y + 4, 10, DARKGRAY);
    }
}

void LinkedListVisualizer_Draw(void) {
    // 1. Draw Nodes (including potential temp node)
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i];
        Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
        
        Color bgColor = WHITE;
        if (vn->address == context.newNodeAddress) bgColor = (Color){ 200, 255, 200, 255 };
        if (vn->address == context.toDeleteAddress) bgColor = (Color){ 255, 200, 200, 255 };
        if (vn->address == curr_address) bgColor = (Color){ 255, 255, 200, 255 };

        DrawRectangleRec(rec, bgColor);
        DrawRectangleLinesEx(rec, 1, BLACK);
        DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, BLACK);
        char val[4]; sprintf(val, "%d", (int)vn->data);
        DrawText(val, vn->position.x + 15, vn->position.y + 15, 15, BLACK);
        DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, BLACK);
        char addr[16]; sprintf(addr, "0x%X", vn->address);
        DrawText(addr, vn->position.x, vn->position.y - 12, 10, DARKGRAY);
    }

    // 2. Draw Connections on top
    int current = head_address;
    while (current != 0) {
        MemoryNode* node = MemoryManager_GetNode(current);
        VisualNode* vn = GetVisualNode(current);
        if (vn && node->next_address != 0) {
            VisualNode* nextVn = GetVisualNode(node->next_address);
            if (nextVn) {
                Rectangle pointerBox = GetPointerFieldBox(vn);
                Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                DrawCurvedArrow(start, end, BLACK);
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
                    Rectangle pointerBox = GetPointerFieldBox(vn);
                    Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                    Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                    DrawCurvedArrow(start, end, DARKGRAY);
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
    simStatus = SIM_IDLE;
    context.type = FUNC_NONE;
    context.prevAddress = 0;
    context.toDeleteAddress = 0;
    showError = false;
    ResetTraversal();
}

void LinkedListVisualizer_DrawUI(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. THE STACK (Fixed Left Center)
    Rectangle stackBox = { 20, (float)sh/2 - 120, 240, 220 };
    DrawRectangleRec(stackBox, WHITE);
    DrawRectangleLinesEx(stackBox, 2, BLACK);
    DrawText("Stack Memory", stackBox.x + 10, stackBox.y + 10, 13, BLACK);

    VisualNode* headNode = GetVisualNode(head_address);
    
    if (simStatus == SIM_EXECUTING) {
        Rectangle travBox = { stackBox.x + 20, stackBox.y + 40, 85, 24 };
        if (curr_address != 0) {
            VisualNode* travNode = GetVisualNode(curr_address);
            if (travNode) {
                MemoryNode* travMem = MemoryManager_GetNode(curr_address);
                bool isNull = (travMem && travMem->next_address == 0);
                Vector2 travTip = isNull ? GetPointerFieldCenterScreen(travNode) : GetNodeLeftCenterScreen(travNode);
                DrawStackPointerBox("traverse", travBox, travTip, (Color){ 255, 0, 110, 255 }, isNull);
            }
        }
    }

    if (simStatus == SIM_EXECUTING) {
        Rectangle execBox = { stackBox.x + 20, stackBox.y + 10, stackBox.width - 40, 25 };
        DrawRectangleRec(execBox, LIGHTGRAY);
        DrawRectangleLinesEx(execBox, 1, BLACK);
        char func[64];
        if (context.type == FUNC_INSERT) {
            sprintf(func, "INSERT(List, %d, %d)", context.targetVal, context.targetPos);
        } else if (context.type == FUNC_DELETE) {
            sprintf(func, "DELETE(List, %d)", context.targetPos);
        } else {
            sprintf(func, "FUNC(List)");
        }
        DrawText(func, execBox.x + 10, execBox.y + 6, 12, BLACK);
    }

    Rectangle listBox = { stackBox.x + 40, stackBox.y + 140, 160, 35 };
    DrawRectangleRec(listBox, WHITE);
    DrawRectangleLinesEx(listBox, 1, BLACK);
    DrawText("List", listBox.x + 55, listBox.y + 10, 13, BLACK);

    if (headNode) {
        Vector2 listTip = GetNodeLeftCenterScreen(headNode);
        DrawStraightArrow((Vector2){ listBox.x + listBox.width, listBox.y + listBox.height / 2.0f }, listTip, BLACK);
    } else {
        Vector2 listTip = (Vector2){ listBox.x + listBox.width + 100, listBox.y + listBox.height / 2.0f };
        DrawStackPointerBox("List", listBox, listTip, BLACK, true);
    }

    if (simStatus == SIM_EXECUTING) {
        if (context.currentPos <= 1) {
            Rectangle travLabelBox = { stackBox.x + stackBox.width + 30, listBox.y + 5, 90, 24 };
            Vector2 travTip = (Vector2){ listBox.x + listBox.width / 2.0f, listBox.y + listBox.height / 2.0f };
            DrawStackPointerBox("Traverse", travLabelBox, travTip, BLACK, false);
        } else {
            MemoryNode* travMem = MemoryManager_GetNode(curr_address);
            bool isNull = (travMem && travMem->next_address == 0);
            DrawWorldTraverseBox(GetVisualNode(curr_address), isNull);
        }
    }

    char listTxt[32]; sprintf(listTxt, "List: 0x%X", head_address);
    DrawText(listTxt, stackBox.x + 20, stackBox.y + 115, 13, BLACK);
    char currTxt[32]; sprintf(currTxt, "curr: 0x%X", curr_address);
    DrawText(currTxt, stackBox.x + 20, stackBox.y + 185, 13, BLACK);

    // Pseudocode Display
    if (simStatus == SIM_EXECUTING) {
        if (context.type == FUNC_INSERT) {
            DrawPseudocode(sw - 320, 100, insertPseudocode, 12, context.currentLine);
        } else if (context.type == FUNC_DELETE) {
            DrawPseudocode(sw - 320, 100, deletePseudocode, 13, context.currentLine);
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
            GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 65, 160, 30}, valBuf, 8, true);

            if (context.insertMode == INSERT_INDEX) {
                DrawText("Position:", paramBox.x + 30, paramBox.y + 110, 12, BLACK);
                GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 105, 160, 30}, posBuf, 8, true);
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
                ResetTraversal();
                context.currentLine = 0;
                context.totalLines = 12;
                simStatus = SIM_EXECUTING;
            }
        } else if (context.type == FUNC_DELETE) {
            DrawText("DELETE(List)", paramBox.x + 110, paramBox.y + 20, 15, BLACK);

            if (context.deleteMode == DELETE_INDEX) {
                DrawText("Index:", paramBox.x + 30, paramBox.y + 80, 12, BLACK);
                GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 75, 160, 30}, delPosBuf, 8, true);
            } else if (context.deleteMode == DELETE_ELEMENT) {
                DrawText("Value:", paramBox.x + 30, paramBox.y + 80, 12, BLACK);
                GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 75, 160, 30}, delValBuf, 8, true);
            } else {
                DrawText("No extra input required.", paramBox.x + 70, paramBox.y + 90, 12, DARKGRAY);
            }

            if (GuiButton((Rectangle){paramBox.x + 120, paramBox.y + 160, 100, 35}, "START")) {
                if (context.deleteMode == DELETE_INDEX) {
                    context.targetPos = atoi(delPosBuf);
                } else if (context.deleteMode == DELETE_ELEMENT) {
                    context.targetVal = atoi(delValBuf);
                }
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
