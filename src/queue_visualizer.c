#include "queue_visualizer.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int front_address = 0;
static int rear_address = 0;
static VisualNode visualNodes[MAX_MEM_NODES];
static int visualNodeCount = 0;
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

typedef enum { Q_IDLE, Q_INPUT_PARAMS, Q_EXECUTING } QStatus;
typedef enum { Q_FUNC_NONE, Q_FUNC_ENQUEUE, Q_FUNC_DEQUEUE } QFunc;

static struct {
    QFunc type;
    int targetVal;
    int currentLine;
    float lineProgress;
    int newNodeAddress;
    int toDeleteAddress;
} ctx;

static QStatus qStatus = Q_IDLE;
static char valBuf[8] = "10";
static bool valEditMode = false;

static const char* enqueuePseudocode[] = {
    "node *temp = malloc(sizeof(node))", // 0
    "temp->data = val",                  // 1
    "temp->next = NULL",                 // 2
    "if (rear == NULL) {",               // 3
    "    front = rear = temp",           // 4
    "} else {",                          // 5
    "    rear->next = temp",             // 6
    "    rear = temp",                   // 7
    "}"                                  // 8
};

static const char* dequeuePseudocode[] = {
    "if (front == NULL) return",         // 0
    "node *temp = front",                // 1
    "front = front->next",               // 2
    "if (front == NULL) rear = NULL",    // 3
    "free(temp)"                         // 4
};

// --- Helpers (Internal only) ---
static VisualNode* GetVisualNode(int address) {
    if (address == 0) return NULL;
    for (int i = 0; i < visualNodeCount; i++) if (visualNodes[i].address == address) return &visualNodes[i];
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
    const float stepX = 130.0f, stepY = 80.0f;
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
    Vector2 a = Vector2Scale(p0, u * u), b = Vector2Scale(p1, 2.0f * u * t), c = Vector2Scale(p2, t * t);
    return Vector2Add(Vector2Add(a, b), c);
}

static void DrawCurvedArrow(Vector2 start, Vector2 end, Color color, float pct) {
    if (pct <= 0.0f) return;
    Vector2 control = { (start.x + end.x) / 2.0f, fminf(start.y, end.y) - 60.0f };
    Vector2 prev = start, secondToLast = start;
    int totalSteps = 20;
    int visibleSteps = (int)((float)totalSteps * pct);
    if (visibleSteps < 1) visibleSteps = 1;
    for (int i = 1; i <= visibleSteps; i++) {
        float t = (float)i / (float)totalSteps;
        Vector2 point = QuadraticBezier(start, control, end, t);
        DrawLineEx(prev, point, 3.0f, color); secondToLast = prev; prev = point;
    }
    if (pct >= 0.95f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, secondToLast));
        Vector2 side = { -dir.y, dir.x };
        float length = 18.0f, width = 10.0f;
        Vector2 base = Vector2Subtract(end, Vector2Scale(dir, length));
        Vector2 p1 = Vector2Add(base, Vector2Scale(side, width)), p2 = Vector2Subtract(base, Vector2Scale(side, width));
        DrawTriangle(end, p2, p1, color); DrawLineEx(end, p1, 1.0f, color); DrawLineEx(end, p2, 1.0f, color);
    }
}

static void DrawStraightArrow(Vector2 start, Vector2 end, Color color, float pct) {
    if (pct <= 0.0f) return;
    Vector2 currentEnd = Vector2Add(start, Vector2Scale(Vector2Subtract(end, start), pct));
    DrawLineEx(start, currentEnd, 3.0f, color);
    if (pct >= 0.95f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
        Vector2 side = { -dir.y, dir.x };
        float length = 18.0f, width = 10.0f;
        Vector2 base = Vector2Subtract(end, Vector2Scale(dir, length));
        Vector2 p1 = Vector2Add(base, Vector2Scale(side, width)), p2 = Vector2Subtract(base, Vector2Scale(side, width));
        DrawTriangle(end, p2, p1, color); DrawLineEx(end, p1, 1.0f, color); DrawLineEx(end, p2, 1.0f, color);
    }
}

static void DrawStackPointerBox(const char* label, Rectangle box, Vector2 tipPos, Color color, bool showNull, float pct) {
    DrawRectangleRec(box, WHITE); DrawRectangleLinesEx(box, 1, color);
    DrawText(label, box.x + 6, box.y + 4, 12, color);
    Vector2 arrowEnd = tipPos; if (showNull) arrowEnd.x -= 25.0f;
    DrawStraightArrow((Vector2){ box.x + box.width, box.y + box.height / 2.0f }, arrowEnd, color, pct);
    if (showNull && pct >= 0.95f) {
        Rectangle nullBox = { tipPos.x - 25, tipPos.y - 10, 50, 20 };
        DrawRectangleRec(nullBox, (Color){ 30, 30, 30, 255 }); DrawRectangleLinesEx(nullBox, 1, (Color){ 220, 220, 220, 255 });
        DrawText("NULL", nullBox.x + 8, nullBox.y + 4, 10, (Color){ 220, 220, 220, 255 });
    }
}

static Vector2 GetNodeLeftCenterScreen(VisualNode* vn) { Vector2 leftCenter = { vn->position.x, vn->position.y + 25 }; return GetWorldToScreen2D(leftCenter, viewCamera); }
static Vector2 GetPointerFieldCenterScreen(VisualNode* vn) { return GetWorldToScreen2D((Vector2){ vn->position.x + 75, vn->position.y + 25 }, viewCamera); }

static void DrawPseudocode(int x, int y, const char** lines, int count, int currentLine) {
    Rectangle bg = { (float)x - 10, (float)y - 10, 320, (float)count * 20 + 20 };
    DrawRectangleRec(bg, (Color){ 235, 245, 255, 255 }); DrawRectangleLinesEx(bg, 1, DARKBLUE);
    DrawText("ALGORITHM (INSTRUCTION SET)", x, y - 25, 12, DARKBLUE);
    for (int i = 0; i < count; i++) {
        Color textColor = (Color){ 30, 60, 90, 255 };
        if (i == currentLine) { DrawRectangle(x - 5, y + i * 20, 310, 18, (Color){ 255, 255, 0, 150 }); textColor = RED; }
        DrawText(lines[i], x, y + i * 20, 15, textColor);
    }
}

// --- Logic ---

void QueueVisualizer_Init(void) {
    MemoryManager_Init(); front_address = rear_address = 0; visualNodeCount = 0; spawnCenter = (Vector2){ 0, 0 }; viewCamera = (Camera2D){ 0 };
    qStatus = Q_IDLE; ctx.type = Q_FUNC_NONE; ctx.currentLine = 0; ctx.lineProgress = 1.0f;
}

void QueueVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
    for (int i = 0; i < visualNodeCount; i++) if (!visualNodes[i].isDragging) {
        visualNodes[i].position.x += (visualNodes[i].targetPosition.x - visualNodes[i].position.x) * 0.15f;
        visualNodes[i].position.y += (visualNodes[i].targetPosition.y - visualNodes[i].position.y) * 0.15f;
    }
    if (ctx.lineProgress < 1.0f) { ctx.lineProgress += GetFrameTime() * 2.5f; if (ctx.lineProgress > 1.0f) ctx.lineProgress = 1.0f; }

    if (qStatus != Q_IDLE && qStatus != Q_EXECUTING) return;

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

void QueueVisualizer_NextStep(void) {
    ctx.lineProgress = 0.0f;
    if (ctx.type == Q_FUNC_ENQUEUE) {
        switch (ctx.currentLine) {
            case 0: ctx.newNodeAddress = MemoryManager_Malloc(0); if (ctx.newNodeAddress != -1) {
                    Vector2 tp = FindSpawnPosition(spawnCenter);
                    visualNodes[visualNodeCount].address = ctx.newNodeAddress; visualNodes[visualNodeCount].position = (Vector2){ tp.x, tp.y - 100 };
                    visualNodes[visualNodeCount].targetPosition = tp; visualNodes[visualNodeCount].data = 0; 
                    visualNodes[visualNodeCount].color = (Color){ (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), 255 };
                    visualNodeCount++; ctx.currentLine = 1;
                } break;
            case 1: { MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress); if (n) n->value = ctx.targetVal; VisualNode* vn = GetVisualNode(ctx.newNodeAddress); if (vn) vn->data = (char)ctx.targetVal; ctx.currentLine = 2; } break;
            case 2: ctx.currentLine = 3; break; // temp->next = NULL (already handled by malloc)
            case 3: if (rear_address == 0) ctx.currentLine = 4; else ctx.currentLine = 6; break;
            case 4: front_address = rear_address = ctx.newNodeAddress; ctx.newNodeAddress = 0; qStatus = Q_IDLE; break;
            case 5: ctx.currentLine = 6; break;
            case 6: { MemoryNode* rn = MemoryManager_GetNode(rear_address); if (rn) rn->next_address = ctx.newNodeAddress; ctx.currentLine = 7; } break;
            case 7: rear_address = ctx.newNodeAddress; ctx.currentLine = 8; break;
            case 8: ctx.newNodeAddress = 0; qStatus = Q_IDLE; break;
        }
    } else if (ctx.type == Q_FUNC_DEQUEUE) {
        switch (ctx.currentLine) {
            case 0: if (front_address == 0) qStatus = Q_IDLE; else ctx.currentLine = 1; break;
            case 1: ctx.toDeleteAddress = front_address; ctx.currentLine = 2; break;
            case 2: { MemoryNode* fn = MemoryManager_GetNode(front_address); if (fn) front_address = fn->next_address; ctx.currentLine = 3; } break;
            case 3: if (front_address == 0) rear_address = 0; ctx.currentLine = 4; break;
            case 4: MemoryManager_Free(ctx.toDeleteAddress); RemoveVisualNode(ctx.toDeleteAddress); ctx.toDeleteAddress = 0; qStatus = Q_IDLE; break;
        }
    }
}

void QueueVisualizer_Draw(void) {
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i]; Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
        Color bc = vn->color; if (vn->address == ctx.newNodeAddress) bc = (Color){ 200, 255, 200, 255 }; if (vn->address == ctx.toDeleteAddress) bc = (Color){ 255, 200, 200, 255 };
        DrawRectangleRec(rec, bc); DrawRectangleLinesEx(rec, 2, DARKGREEN);
        DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, DARKGREEN);
        char val[4]; sprintf(val, "%d", (int)vn->data); DrawText(val, vn->position.x + 15, vn->position.y + 15, 15, WHITE);
        DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, WHITE);
        char ad[16]; sprintf(ad, "0x%X", vn->address); DrawText(ad, vn->position.x, vn->position.y - 12, 10, DARKGREEN);
    }
    int cur = front_address;
    while (cur != 0) {
        MemoryNode* n = MemoryManager_GetNode(cur); VisualNode* vn = GetVisualNode(cur);
        if (vn && n->next_address != 0) {
            VisualNode* nv = GetVisualNode(n->next_address);
            if (nv) {
                float pg = (qStatus == Q_EXECUTING && cur == rear_address && ctx.currentLine == 6) ? ctx.lineProgress : 1.0f;
                DrawCurvedArrow((Vector2){ vn->position.x + 75, vn->position.y + 25 }, (Vector2){ nv->position.x, nv->position.y + 25 }, BLACK, pg);
            }
        } cur = MemoryManager_GetNode(cur)->next_address;
    }
}

void QueueVisualizer_DrawUI(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Rectangle cb = { 20, 60, 240, 70 }; DrawRectangleRec(cb, (Color){ 230, 240, 255, 255 }); DrawRectangleLinesEx(cb, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cb.x + 10, cb.y + 10, 14, DARKBLUE);
    if (qStatus == Q_EXECUTING) {
        char fn[32]; sprintf(fn, ctx.type == Q_FUNC_ENQUEUE ? "ENQUEUE(Q, %d)" : "DEQUEUE(Q)", ctx.targetVal);
        DrawText(fn, cb.x + 20, cb.y + 35, 12, DARKBLUE);
        char pc[32]; sprintf(pc, "PC: 0x%X", 0xE00 + (ctx.currentLine * 4)); DrawText(pc, cb.x + 20, cb.y + 50, 11, BLUE);
        DrawStraightArrow((Vector2){ cb.x + cb.width, cb.y + cb.height / 2.0f }, (Vector2){ (float)sw - 580, 60.0f + (ctx.currentLine * 20) }, BLUE, 1.0f);
    } else DrawText("PC: IDLE", cb.x + 20, cb.y + 35, 12, BLUE);

    Rectangle sb = { 20, (float)sh/2 - 110, 240, 220 }; DrawRectangleRec(sb, (Color){ 255, 245, 230, 255 }); DrawRectangleLinesEx(sb, 2, (Color){ 255, 161, 0, 255 });
    DrawText("THE STACK", sb.x + 10, sb.y + 10, 14, (Color){ 200, 100, 0, 255 });
    
    Rectangle fb = { sb.x + 40, sb.y + 60, 160, 35 }, rb = { sb.x + 40, sb.y + 140, 160, 35 };
    DrawRectangleRec(fb, WHITE); DrawRectangleLinesEx(fb, 1, BLACK); DrawText("front (ptr)", fb.x + 55, fb.y + 10, 13, BLACK);
    DrawRectangleRec(rb, WHITE); DrawRectangleLinesEx(rb, 1, BLACK); DrawText("rear (ptr)", rb.x + 55, rb.y + 10, 13, BLACK);
    
    VisualNode* fn = GetVisualNode(front_address); VisualNode* rn = GetVisualNode(rear_address);
    if (fn) DrawStraightArrow((Vector2){ fb.x + fb.width, fb.y + fb.height / 2.0f }, GetNodeLeftCenterScreen(fn), BLACK, 1.0f);
    else DrawStackPointerBox("front", fb, (Vector2){ fb.x + fb.width + 100, fb.y + fb.height / 2.0f }, BLACK, true, 1.0f);
    if (rn) DrawStraightArrow((Vector2){ rb.x + rb.width, rb.y + rb.height / 2.0f }, GetNodeLeftCenterScreen(rn), BLACK, 1.0f);
    else DrawStackPointerBox("rear", rb, (Vector2){ rb.x + rb.width + 100, rb.y + rb.height / 2.0f }, BLACK, true, 1.0f);

    if (qStatus == Q_EXECUTING) {
        if (ctx.type == Q_FUNC_ENQUEUE) DrawPseudocode(sw - 580, 60, enqueuePseudocode, 9, ctx.currentLine);
        else DrawPseudocode(sw - 580, 60, dequeuePseudocode, 5, ctx.currentLine);
    }
    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    if (GuiButton((Rectangle){ 50, (float)sh - 80, 120, 45 }, "Enqueue")) qStatus = Q_INPUT_PARAMS, ctx.type = Q_FUNC_ENQUEUE;
    if (GuiButton((Rectangle){ 180, (float)sh - 80, 120, 45 }, "Dequeue")) { ctx.type = Q_FUNC_DEQUEUE; ctx.currentLine = 0; qStatus = Q_EXECUTING; }
    if (qStatus == Q_EXECUTING) {
        if (GuiButton((Rectangle){ 400, (float)sh - 80, 200, 45 }, "NEXT STEP")) QueueVisualizer_NextStep();
        if (GuiButton((Rectangle){ 610, (float)sh - 80, 100, 45 }, "CANCEL")) QueueVisualizer_CancelInteraction();
    }
    if (qStatus == Q_INPUT_PARAMS) {
        Rectangle pb = { (float)sw/2 - 170, (float)sh/2 - 110, 340, 180 };
        DrawRectangleRec(pb, WHITE); DrawRectangleLinesEx(pb, 2, BLACK);
        DrawText("ENQUEUE(Q, val)", pb.x + 80, pb.y + 20, 15, BLACK);
        DrawText("Value:", pb.x + 30, pb.y + 70, 12, BLACK);
        if (GuiTextBox((Rectangle){pb.x + 120, pb.y + 65, 160, 30}, valBuf, 8, valEditMode)) valEditMode = !valEditMode;
        if (GuiButton((Rectangle){pb.x + 120, pb.y + 120, 100, 35}, "START")) { ctx.targetVal = atoi(valBuf); valEditMode = false; ctx.currentLine = 0; qStatus = Q_EXECUTING; }
    }
}

void QueueVisualizer_SetSpawnCenter(Vector2 center) { spawnCenter = center; }
void QueueVisualizer_SetCamera(Camera2D cam) { viewCamera = cam; }
int QueueVisualizer_GetTraversalAddress(void) { return 0; }
bool QueueVisualizer_IsBusy(void) { return qStatus != Q_IDLE; }
void QueueVisualizer_CancelInteraction(void) {
    if (ctx.type == Q_FUNC_ENQUEUE && ctx.newNodeAddress != 0) { MemoryManager_Free(ctx.newNodeAddress); RemoveVisualNode(ctx.newNodeAddress); }
    qStatus = Q_IDLE; ctx.type = Q_FUNC_NONE; ctx.newNodeAddress = 0; ctx.toDeleteAddress = 0;
}
