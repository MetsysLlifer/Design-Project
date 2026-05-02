#include "stack_visualizer.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h" // Reuse VisualNode and SimStatus concepts if possible, but let's keep it self-contained
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Reuse the same logic engine
static int top_address = 0;
static VisualNode visualNodes[MAX_MEM_NODES];
static int visualNodeCount = 0;
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

typedef enum {
    STACK_IDLE,
    STACK_INPUT_PARAMS,
    STACK_EXECUTING
} StackStatus;

typedef enum {
    STACK_FUNC_NONE,
    STACK_FUNC_PUSH,
    STACK_FUNC_POP
} StackFunc;

static struct {
    StackFunc type;
    int targetVal;
    int currentLine;
    float lineProgress;
    int newNodeAddress;
    int toDeleteAddress;
} ctx;

static StackStatus stackStatus = STACK_IDLE;
static char valBuf[8] = "10";
static bool valEditMode = false;

static const char* pushPseudocode[] = {
    "node *temp = malloc(sizeof(node))", // 0
    "temp->data = val",                  // 1
    "temp->next = top",                  // 2
    "top = temp"                         // 3
};

static const char* popPseudocode[] = {
    "if (top == NULL) return",           // 0
    "node *temp = top",                  // 1
    "top = top->next",                   // 2
    "free(temp)"                         // 3
};

// --- Helper Functions (Shared logic with LL) ---
static VisualNode* GetVisualNode(int address) {
    if (address == 0) return NULL;
    for (int i = 0; i < visualNodeCount; i++) {
        if (visualNodes[i].address == address) return &visualNodes[i];
    }
    return NULL;
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
        float length = 18.0f; float width = 10.0f;
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
        float length = 18.0f; float width = 10.0f;
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
    if (showNull) arrowEnd.x -= 25.0f;
    DrawStraightArrow((Vector2){ box.x + box.width, box.y + box.height / 2.0f }, arrowEnd, color, pct);
    if (showNull && pct >= 0.95f) {
        Rectangle nullBox = { tipPos.x - 25, tipPos.y - 10, 50, 20 };
        DrawRectangleRec(nullBox, (Color){ 30, 30, 30, 255 });
        DrawRectangleLinesEx(nullBox, 1, (Color){ 220, 220, 220, 255 });
        DrawText("NULL", nullBox.x + 8, nullBox.y + 4, 10, (Color){ 220, 220, 220, 255 });
    }
}

static Vector2 GetNodeLeftCenterScreen(VisualNode* vn) {
    Vector2 leftCenter = { vn->position.x, vn->position.y + 25 };
    return GetWorldToScreen2D(leftCenter, viewCamera);
}

static void DrawPseudocode(int x, int y, const char** lines, int count, int currentLine) {
    Rectangle bg = { (float)x - 10, (float)y - 10, 320, (float)count * 20 + 20 };
    DrawRectangleRec(bg, (Color){ 235, 245, 255, 255 });
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

// --- Implementation ---

void StackVisualizer_Init(void) {
    MemoryManager_Init();
    top_address = 0;
    visualNodeCount = 0;
    spawnCenter = (Vector2){ 0, 0 };
    viewCamera = (Camera2D){ 0 };
    stackStatus = STACK_IDLE;
    ctx.type = STACK_FUNC_NONE;
    ctx.currentLine = 0;
    ctx.lineProgress = 1.0f;
}

void StackVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
    for (int i = 0; i < visualNodeCount; i++) {
        if (!visualNodes[i].isDragging) {
            visualNodes[i].position.x += (visualNodes[i].targetPosition.x - visualNodes[i].position.x) * 0.15f;
            visualNodes[i].position.y += (visualNodes[i].targetPosition.y - visualNodes[i].position.y) * 0.15f;
        }
    }
    if (ctx.lineProgress < 1.0f) {
        ctx.lineProgress += GetFrameTime() * 2.5f;
        if (ctx.lineProgress > 1.0f) ctx.lineProgress = 1.0f;
    }

    if (stackStatus != STACK_IDLE && stackStatus != STACK_EXECUTING) return;

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

void StackVisualizer_NextStep(void) {
    ctx.lineProgress = 0.0f;
    if (ctx.type == STACK_FUNC_PUSH) {
        switch (ctx.currentLine) {
            case 0: // node *temp = malloc(sizeof(node))
                ctx.newNodeAddress = MemoryManager_Malloc(0);
                if (ctx.newNodeAddress != -1) {
                    Vector2 targetPos = FindSpawnPosition(spawnCenter);
                    visualNodes[visualNodeCount].address = ctx.newNodeAddress;
                    visualNodes[visualNodeCount].position = (Vector2){ targetPos.x, targetPos.y - 100 };
                    visualNodes[visualNodeCount].targetPosition = targetPos;
                    visualNodes[visualNodeCount].data = 0;
                    visualNodes[visualNodeCount].color = (Color){ (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), 255 };
                    visualNodeCount++;
                    ctx.currentLine = 1;
                }
                break;
            case 1: // temp->data = val
                {
                    MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress);
                    if (n) n->value = ctx.targetVal;
                    VisualNode* vn = GetVisualNode(ctx.newNodeAddress);
                    if (vn) vn->data = (char)ctx.targetVal;
                    ctx.currentLine = 2;
                }
                break;
            case 2: // temp->next = top
                {
                    MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress);
                    if (n) n->next_address = top_address;
                    ctx.currentLine = 3;
                }
                break;
            case 3: // top = temp
                top_address = ctx.newNodeAddress;
                ctx.newNodeAddress = 0;
                stackStatus = STACK_IDLE;
                ctx.type = STACK_FUNC_NONE;
                break;
        }
    } else if (ctx.type == STACK_FUNC_POP) {
        switch (ctx.currentLine) {
            case 0: // if (top == NULL) return
                if (top_address == 0) {
                    stackStatus = STACK_IDLE;
                    ctx.type = STACK_FUNC_NONE;
                } else ctx.currentLine = 1;
                break;
            case 1: // node *temp = top
                ctx.toDeleteAddress = top_address;
                ctx.currentLine = 2;
                break;
            case 2: // top = top->next
                {
                    MemoryNode* n = MemoryManager_GetNode(top_address);
                    if (n) top_address = n->next_address;
                    ctx.currentLine = 3;
                }
                break;
            case 3: // free(temp)
                MemoryManager_Free(ctx.toDeleteAddress);
                RemoveVisualNode(ctx.toDeleteAddress);
                ctx.toDeleteAddress = 0;
                stackStatus = STACK_IDLE;
                ctx.type = STACK_FUNC_NONE;
                break;
        }
    }
}

void StackVisualizer_Draw(void) {
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i];
        Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
        Color bgColor = vn->color;
        if (vn->address == ctx.newNodeAddress) bgColor = (Color){ 200, 255, 200, 255 };
        if (vn->address == ctx.toDeleteAddress) bgColor = (Color){ 255, 200, 200, 255 };
        DrawRectangleRec(rec, bgColor);
        DrawRectangleLinesEx(rec, 2, DARKGREEN);
        DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, DARKGREEN);
        char val[4]; sprintf(val, "%d", (int)vn->data);
        DrawText(val, vn->position.x + 15, vn->position.y + 15, 15, WHITE);
        DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, WHITE);
        char addr[16]; sprintf(addr, "0x%X", vn->address);
        DrawText(addr, vn->position.x, vn->position.y - 12, 10, DARKGREEN);
    }
    int current = top_address;
    while (current != 0) {
        MemoryNode* node = MemoryManager_GetNode(current);
        VisualNode* vn = GetVisualNode(current);
        if (vn && node->next_address != 0) {
            VisualNode* nextVn = GetVisualNode(node->next_address);
            if (nextVn) {
                Rectangle pointerBox = GetPointerFieldBox(vn);
                Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                DrawCurvedArrow(start, end, BLACK, 1.0f);
            }
        }
        current = node->next_address;
    }
    if (ctx.newNodeAddress != 0) {
        MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress);
        VisualNode* vn = GetVisualNode(ctx.newNodeAddress);
        if (vn && n && n->next_address != 0) {
            VisualNode* nextVn = GetVisualNode(n->next_address);
            if (nextVn) {
                float prog = (ctx.currentLine == 2) ? ctx.lineProgress : 1.0f;
                Rectangle pointerBox = GetPointerFieldBox(vn);
                Vector2 start = { pointerBox.x + pointerBox.width / 2.0f, pointerBox.y + pointerBox.height / 2.0f };
                Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                DrawCurvedArrow(start, end, DARKGRAY, prog);
            }
        }
    }
}

void StackVisualizer_DrawUI(void) {
    int sw = GetScreenWidth(); int sh = GetScreenHeight();
    Rectangle cpuBox = { 20, 60, 240, 70 };
    DrawRectangleRec(cpuBox, (Color){ 230, 240, 255, 255 });
    DrawRectangleLinesEx(cpuBox, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cpuBox.x + 10, cpuBox.y + 10, 14, DARKBLUE);
    if (stackStatus == STACK_EXECUTING) {
        char func[32]; sprintf(func, ctx.type == STACK_FUNC_PUSH ? "PUSH(Stack, %d)" : "POP(Stack)", ctx.targetVal);
        DrawText(func, cpuBox.x + 20, cpuBox.y + 35, 12, DARKBLUE);
        char pcTxt[32]; sprintf(pcTxt, "PC: 0x%X", 0xC00 + (ctx.currentLine * 4));
        DrawText(pcTxt, cpuBox.x + 20, cpuBox.y + 50, 11, BLUE);
        Vector2 cpuRight = { cpuBox.x + cpuBox.width, cpuBox.y + cpuBox.height / 2.0f };
        Vector2 codeLeft = { (float)sw - 580, 60.0f + (ctx.currentLine * 20) };
        DrawStraightArrow(cpuRight, codeLeft, BLUE, 1.0f);
    } else DrawText("PC: IDLE", cpuBox.x + 20, cpuBox.y + 35, 12, BLUE);

    Rectangle stackBox = { 20, (float)sh/2 - 80, 240, 190 };
    DrawRectangleRec(stackBox, (Color){ 255, 245, 230, 255 });
    DrawRectangleLinesEx(stackBox, 2, (Color){ 255, 161, 0, 255 });
    DrawText("THE STACK", stackBox.x + 10, stackBox.y + 10, 14, (Color){ 200, 100, 0, 255 });
    Rectangle topBox = { stackBox.x + 40, stackBox.y + 100, 160, 35 };
    DrawRectangleRec(topBox, WHITE); DrawRectangleLinesEx(topBox, 1, BLACK);
    DrawText("top (ptr)", topBox.x + 55, topBox.y + 10, 13, BLACK);
    VisualNode* topNode = GetVisualNode(top_address);
    if (topNode) {
        float prog = (stackStatus == STACK_EXECUTING && ((ctx.type == STACK_FUNC_PUSH && ctx.currentLine == 3) || (ctx.type == STACK_FUNC_POP && ctx.currentLine == 2))) ? ctx.lineProgress : 1.0f;
        DrawStraightArrow((Vector2){ topBox.x + topBox.width, topBox.y + topBox.height / 2.0f }, GetNodeLeftCenterScreen(topNode), BLACK, prog);
    } else DrawStackPointerBox("top", topBox, (Vector2){ topBox.x + topBox.width + 100, topBox.y + topBox.height / 2.0f }, BLACK, true, 1.0f);
    char topTxt[32]; sprintf(topTxt, "top: 0x%X", top_address);
    DrawText(topTxt, stackBox.x + 20, stackBox.y + 75, 13, (Color){ 150, 80, 0, 255 });

    if (stackStatus == STACK_EXECUTING) {
        if (ctx.type == STACK_FUNC_PUSH) DrawPseudocode(sw - 580, 60, pushPseudocode, 4, ctx.currentLine);
        else DrawPseudocode(sw - 580, 60, popPseudocode, 4, ctx.currentLine);
    }
    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    if (GuiButton((Rectangle){ 50, (float)sh - 80, 120, 45 }, "Push")) stackStatus = STACK_INPUT_PARAMS, ctx.type = STACK_FUNC_PUSH;
    if (GuiButton((Rectangle){ 180, (float)sh - 80, 120, 45 }, "Pop")) {
        ctx.type = STACK_FUNC_POP; ctx.currentLine = 0; ctx.lineProgress = 0.0f; stackStatus = STACK_EXECUTING;
    }
    if (stackStatus == STACK_EXECUTING) {
        if (GuiButton((Rectangle){ 400, (float)sh - 80, 200, 45 }, "NEXT STEP")) StackVisualizer_NextStep();
        if (GuiButton((Rectangle){ 610, (float)sh - 80, 100, 45 }, "CANCEL")) StackVisualizer_CancelInteraction();
    }
    if (stackStatus == STACK_INPUT_PARAMS) {
        Rectangle paramBox = { (float)sw/2 - 170, (float)sh/2 - 110, 340, 180 };
        DrawRectangleRec(paramBox, WHITE); DrawRectangleLinesEx(paramBox, 2, BLACK);
        DrawText("PUSH(Stack, val)", paramBox.x + 80, paramBox.y + 20, 15, BLACK);
        DrawText("Value:", paramBox.x + 30, paramBox.y + 70, 12, BLACK);
        if (GuiTextBox((Rectangle){paramBox.x + 120, paramBox.y + 65, 160, 30}, valBuf, 8, valEditMode)) valEditMode = !valEditMode;
        if (GuiButton((Rectangle){paramBox.x + 120, paramBox.y + 120, 100, 35}, "START")) {
            ctx.targetVal = atoi(valBuf); valEditMode = false; ctx.currentLine = 0; ctx.lineProgress = 0.0f; stackStatus = STACK_EXECUTING;
        }
    }
}

void StackVisualizer_SetSpawnCenter(Vector2 center) { spawnCenter = center; }
void StackVisualizer_SetCamera(Camera2D cam) { viewCamera = cam; }
int StackVisualizer_GetTraversalAddress(void) { return 0; }
bool StackVisualizer_IsBusy(void) { return stackStatus != STACK_IDLE; }
void StackVisualizer_CancelInteraction(void) {
    if (ctx.type == STACK_FUNC_PUSH && ctx.newNodeAddress != 0) { MemoryManager_Free(ctx.newNodeAddress); RemoveVisualNode(ctx.newNodeAddress); }
    stackStatus = STACK_IDLE; ctx.type = STACK_FUNC_NONE; ctx.newNodeAddress = 0; ctx.toDeleteAddress = 0;
}
