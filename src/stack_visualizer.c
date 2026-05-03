#include "stack_visualizer.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int top_address = 0;
static VisualNode visualNodes[MAX_MEM_NODES];
static int visualNodeCount = 0;
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

typedef enum { STACK_IDLE, STACK_INPUT_PARAMS, STACK_EXECUTING } StackStatus;
typedef enum { STACK_FUNC_NONE, STACK_FUNC_PUSH, STACK_FUNC_POP } StackFunc;

static struct {
    StackFunc type;
    int targetVal;
    int currentLine;
    int logicalStep;
    float lineProgress;
    int newNodeAddress;
    int toDeleteAddress;
    bool practiceMode;
} ctx;

static StackStatus stackStatus = STACK_IDLE;
static char valBuf[8] = "10";
static bool valEditMode = false;
static char errorMsg[256] = "";
static bool showError = false;

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

static void DrawLogicDiagram(int x, int y, const char* goal, AlgAction iconType) {
    Rectangle bg = { (float)x - 10, (float)y - 10, 320, 150 };
    DrawRectangleRec(bg, (Color){ 255, 255, 240, 255 }); 
    DrawRectangleLinesEx(bg, 2, DARKGRAY);
    DrawText("LOGIC GOAL", x + 10, y, 14, BLACK);
    Rectangle iconArea = { (float)x + 110, (float)y + 40, 80, 50 };
    switch (iconType) {
        case ACT_MALLOC: DrawRectangleLinesEx(iconArea, 2, DARKGREEN); DrawText("+", iconArea.x + 35, iconArea.y + 15, 20, DARKGREEN); break;
        case ACT_ASSIGN: DrawRectangleLinesEx(iconArea, 2, DARKGREEN); DrawText("DATA", iconArea.x + 20, iconArea.y + 15, 15, DARKGREEN); break;
        case ACT_LINK: DrawCircle(iconArea.x + 10, iconArea.y + 25, 5, BLACK); DrawLineEx((Vector2){iconArea.x + 10, iconArea.y + 25}, (Vector2){iconArea.x + 70, iconArea.y + 25}, 2, BLACK); DrawCircle(iconArea.x + 70, iconArea.y + 25, 5, BLACK); break;
        case ACT_FREE: DrawRectangleLinesEx(iconArea, 2, RED); DrawLine(iconArea.x, iconArea.y, iconArea.x + iconArea.width, iconArea.y + iconArea.height, RED); DrawLine(iconArea.x + iconArea.width, iconArea.y, iconArea.x, iconArea.y + iconArea.height, RED); break;
        default: break;
    }
    DrawText(goal, x + 10, y + 110, 13, DARKGRAY);
}

static AlgAction GetRequiredAction(void) {
    if (!ctx.practiceMode) {
        if (ctx.type == STACK_FUNC_PUSH) {
            switch (ctx.currentLine) {
                case 0: return ACT_MALLOC; case 1: return ACT_ASSIGN; case 2: case 3: return ACT_LINK; default: return ACT_NONE;
            }
        } else if (ctx.type == STACK_FUNC_POP) {
            switch (ctx.currentLine) {
                case 0: return ACT_LOGIC; case 1: case 2: return ACT_LINK; case 3: return ACT_FREE; default: return ACT_NONE;
            }
        }
    } else {
        if (ctx.type == STACK_FUNC_PUSH) {
            switch (ctx.logicalStep) {
                case 0: return ACT_MALLOC; case 1: return ACT_ASSIGN; case 2: return ACT_LINK; default: return ACT_NONE;
            }
        } else {
            switch (ctx.logicalStep) {
                case 0: return ACT_LINK; case 1: return ACT_FREE; default: return ACT_NONE;
            }
        }
    }
    return ACT_NONE;
}

static void TryExecuteAction(AlgAction selected) {
    AlgAction required = GetRequiredAction();
    if (required == ACT_LOGIC) { StackVisualizer_NextStep(); required = GetRequiredAction(); }
    if (required == ACT_NONE) return;
    if (selected == required) {
        if (ctx.type == STACK_FUNC_PUSH) {
            switch (ctx.logicalStep) {
                case 0: while(ctx.currentLine != 1) StackVisualizer_NextStep(); ctx.logicalStep = 1; break;
                case 1: while(ctx.currentLine != 2) StackVisualizer_NextStep(); ctx.logicalStep = 2; break;
                case 2: while(stackStatus == STACK_EXECUTING) StackVisualizer_NextStep(); ctx.logicalStep = 3; break;
            }
        } else {
            switch (ctx.logicalStep) {
                case 0: while(ctx.currentLine != 3) StackVisualizer_NextStep(); ctx.logicalStep = 1; break;
                case 1: while(stackStatus == STACK_EXECUTING) StackVisualizer_NextStep(); ctx.logicalStep = 2; break;
            }
        }
    } else {
        showError = true;
        switch (required) {
            case ACT_MALLOC: sprintf(errorMsg, "LOGIC ERROR: New node needed on heap."); break;
            case ACT_ASSIGN: sprintf(errorMsg, "LOGIC ERROR: Set data before linking."); break;
            case ACT_LINK: sprintf(errorMsg, "LOGIC ERROR: Update pointer connections."); break;
            case ACT_FREE: sprintf(errorMsg, "LOGIC ERROR: Free memory of popped node."); break;
            default: sprintf(errorMsg, "INVALID ACTION."); break;
        }
    }
}

static VisualNode* GetVisualNode(int address) {
    if (address == 0) return NULL;
    for (int i = 0; i < visualNodeCount; i++) if (visualNodes[i].address == address) return &visualNodes[i];
    return NULL;
}

static void RemoveVisualNode(int address) {
    for (int i = 0; i < visualNodeCount; i++) if (visualNodes[i].address == address) { visualNodes[i] = visualNodes[visualNodeCount - 1]; visualNodeCount--; return; }
}

static bool IsNodeOverlap(Rectangle candidate) {
    for (int i = 0; i < visualNodeCount; i++) {
        Rectangle existing = { visualNodes[i].position.x - 10, visualNodes[i].position.y - 10, 120, 70 };
        if (CheckCollisionRecs(candidate, existing)) return true;
    }
    return false;
}

static Vector2 FindSpawnPosition(Vector2 basePos) {
    Rectangle cand = { basePos.x - 10, basePos.y - 10, 120, 70 }; if (!IsNodeOverlap(cand)) return basePos;
    float sX = 130.0f, sY = 80.0f;
    for (int r = 1; r < 7; r++) for (int dx = -r; dx <= r; dx++) for (int dy = -r; dy <= r; dy++) {
        if (abs(dx) != r && abs(dy) != r) continue;
        Vector2 p = { basePos.x + dx * sX, basePos.y + dy * sY }; cand = (Rectangle){ p.x - 10, p.y - 10, 120, 70 };
        if (!IsNodeOverlap(cand)) return p;
    }
    return basePos;
}

static void DrawStraightArrow(Vector2 start, Vector2 end, Color color, float pct) {
    if (pct <= 0.0f) return;
    Vector2 currentEnd = Vector2Add(start, Vector2Scale(Vector2Subtract(end, start), pct));
    DrawLineEx(start, currentEnd, 3.0f, color);
    if (pct >= 0.95f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, start)); Vector2 side = { -dir.y, dir.x };
        float len = 18.0f, wid = 10.0f; Vector2 base = Vector2Subtract(end, Vector2Scale(dir, len));
        Vector2 p1 = Vector2Add(base, Vector2Scale(side, wid)), p2 = Vector2Subtract(base, Vector2Scale(side, wid));
        DrawTriangle(end, p2, p1, color); DrawLineEx(end, p1, 1.0f, color); DrawLineEx(end, p2, 1.0f, color);
    }
}

static void DrawStackPointerBox(const char* label, Rectangle box, Vector2 tipPos, Color color, bool showNull, float pct) {
    DrawRectangleRec(box, WHITE); DrawRectangleLinesEx(box, 1, color); DrawText(label, box.x + 6, box.y + 4, 12, color);
    Vector2 arrowEnd = tipPos; if (showNull) arrowEnd.x -= 25.0f;
    DrawStraightArrow((Vector2){ box.x + box.width, box.y + box.height / 2.0f }, arrowEnd, color, pct);
    if (showNull && pct >= 0.95f) {
        Rectangle nullBox = { tipPos.x - 25, tipPos.y - 10, 50, 20 };
        DrawRectangleRec(nullBox, (Color){ 30, 30, 30, 255 }); DrawRectangleLinesEx(nullBox, 1, (Color){ 220, 220, 220, 255 });
        DrawText("NULL", nullBox.x + 8, nullBox.y + 4, 10, (Color){ 220, 220, 220, 255 });
    }
}

static Vector2 GetNodeLeftCenterScreen(VisualNode* vn) { Vector2 leftCenter = { vn->position.x, vn->position.y + 25 }; return GetWorldToScreen2D(leftCenter, viewCamera); }

static void DrawPseudocode(int x, int y, const char** lines, int count, int currentLine) {
    Rectangle bg = { (float)x - 10, (float)y - 10, 320, (float)count * 20 + 20 };
    DrawRectangleRec(bg, (Color){ 235, 245, 255, 255 }); DrawRectangleLinesEx(bg, 1, DARKBLUE);
    DrawText("ALGORITHM", x, y - 25, 12, DARKBLUE);
    for (int i = 0; i < count; i++) {
        Color textColor = (Color){ 30, 60, 90, 255 };
        if (i == currentLine) { DrawRectangle(x - 5, y + i * 20, 310, 18, (Color){ 255, 255, 0, 150 }); textColor = RED; }
        DrawText(lines[i], x, y + i * 20, 15, textColor);
    }
}

void StackVisualizer_Init(void) {
    MemoryManager_Init(); top_address = 0; visualNodeCount = 0; spawnCenter = (Vector2){ 0, 0 }; viewCamera = (Camera2D){ 0 };
    stackStatus = STACK_IDLE; ctx.type = STACK_FUNC_NONE; ctx.currentLine = 0; ctx.logicalStep = 0; ctx.lineProgress = 1.0f; ctx.practiceMode = false; ctx.newNodeAddress = 0; ctx.toDeleteAddress = 0; showError = false;
}

void StackVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
    for (int i = 0; i < visualNodeCount; i++) if (!visualNodes[i].isDragging) {
        visualNodes[i].position.x += (visualNodes[i].targetPosition.x - visualNodes[i].position.x) * 0.15f;
        visualNodes[i].position.y += (visualNodes[i].targetPosition.y - visualNodes[i].position.y) * 0.15f;
    }
    if (ctx.lineProgress < 1.0f) { ctx.lineProgress += GetFrameTime() * 2.5f; if (ctx.lineProgress > 1.0f) ctx.lineProgress = 1.0f; }
    if (stackStatus != STACK_IDLE && stackStatus != STACK_EXECUTING) return;
    static int dragIdx = -1;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) for (int i = visualNodeCount - 1; i >= 0; i--) if (CheckCollisionPointRec(mouseWorldPos, (Rectangle){ visualNodes[i].position.x, visualNodes[i].position.y, 100, 50 })) { dragIdx = i; visualNodes[i].isDragging = true; break; }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) { if (dragIdx != -1) { visualNodes[dragIdx].isDragging = false; visualNodes[dragIdx].targetPosition = visualNodes[dragIdx].position; } dragIdx = -1; }
    if (dragIdx != -1) { visualNodes[dragIdx].position = mouseWorldPos; visualNodes[dragIdx].targetPosition = mouseWorldPos; }
}

void StackVisualizer_NextStep(void) {
    ctx.lineProgress = 0.0f;
    if (ctx.type == STACK_FUNC_PUSH) {
        switch (ctx.currentLine) {
            case 0: ctx.newNodeAddress = MemoryManager_Malloc(0); if (ctx.newNodeAddress != -1) {
                    Vector2 tp = FindSpawnPosition(spawnCenter); visualNodes[visualNodeCount].address = ctx.newNodeAddress; visualNodes[visualNodeCount].position = (Vector2){ tp.x, tp.y - 100 };
                    visualNodes[visualNodeCount].targetPosition = tp; visualNodes[visualNodeCount].data = 0; visualNodes[visualNodeCount].color = (Color){ (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), (unsigned char)GetRandomValue(50, 200), 255 }; visualNodeCount++; ctx.currentLine = 1;
                } break;
            case 1: { MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress); if (n) n->value = ctx.targetVal; VisualNode* vn = GetVisualNode(ctx.newNodeAddress); if (vn) vn->data = (char)ctx.targetVal; ctx.currentLine = 2; } break;
            case 2: { MemoryNode* n = MemoryManager_GetNode(ctx.newNodeAddress); if (n) n->next_address = top_address; ctx.currentLine = 3; } break;
            case 3: top_address = ctx.newNodeAddress; ctx.newNodeAddress = 0; stackStatus = STACK_IDLE; ctx.type = STACK_FUNC_NONE; break;
        }
    } else if (ctx.type == STACK_FUNC_POP) {
        switch (ctx.currentLine) {
            case 0: if (top_address == 0) stackStatus = STACK_IDLE; else ctx.currentLine = 1; break;
            case 1: ctx.toDeleteAddress = top_address; ctx.currentLine = 2; break;
            case 2: { MemoryNode* n = MemoryManager_GetNode(top_address); if (n) top_address = n->next_address; ctx.currentLine = 3; } break;
            case 3: MemoryManager_Free(ctx.toDeleteAddress); RemoveVisualNode(ctx.toDeleteAddress); ctx.toDeleteAddress = 0; stackStatus = STACK_IDLE; ctx.type = STACK_FUNC_NONE; break;
        }
    }
}

void StackVisualizer_Draw(void) {
    for (int i = 0; i < visualNodeCount; i++) {
        VisualNode* vn = &visualNodes[i]; Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
        Color bg = vn->color; if (vn->address == ctx.newNodeAddress) bg = (Color){ 200, 255, 200, 255 }; if (vn->address == ctx.toDeleteAddress) bg = (Color){ 255, 200, 200, 255 };
        DrawRectangleRec(rec, bg); DrawRectangleLinesEx(rec, 2, DARKGREEN);
        DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, DARKGREEN);
        char v[4]; sprintf(v, "%d", (int)vn->data); DrawText(v, vn->position.x + 15, vn->position.y + 15, 15, WHITE);
        DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, WHITE);
        char ad[16]; sprintf(ad, "0x%X", vn->address); DrawText(ad, vn->position.x, vn->position.y - 12, 10, DARKGREEN);
    }
    int cur = top_address;
    while (cur != 0) {
        MemoryNode* n = MemoryManager_GetNode(cur); VisualNode* vn = GetVisualNode(cur);
        if (vn && n->next_address != 0) {
            VisualNode* nv = GetVisualNode(n->next_address);
            if (nv) {
                float pg = (stackStatus == STACK_EXECUTING && ctx.type == STACK_FUNC_PUSH && ctx.currentLine == 2) ? ctx.lineProgress : 1.0f;
                Vector2 start = { vn->position.x + 75, vn->position.y + 25 };
                Vector2 end = { nv->position.x, nv->position.y + 25 };
                DrawStraightArrow(start, end, BLACK, pg);
            }
        } cur = MemoryManager_GetNode(cur)->next_address;
    }
}

void StackVisualizer_DrawUI(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Rectangle cb = { 20, 60, 240, 70 }; DrawRectangleRec(cb, (Color){ 230, 240, 255, 255 }); DrawRectangleLinesEx(cb, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cb.x + 10, cb.y + 10, 14, DARKBLUE);
    if (stackStatus == STACK_EXECUTING) {
        char fn[32]; sprintf(fn, ctx.type == STACK_FUNC_PUSH ? "PUSH(S, %d)" : "POP(S)", ctx.targetVal);
        DrawText(fn, cb.x + 20, cb.y + 35, 12, DARKBLUE);
        char pc[32]; sprintf(pc, "PC: 0x%X", 0xC00 + (ctx.currentLine * 4)); DrawText(pc, cb.x + 20, cb.y + 50, 11, BLUE);
        DrawStraightArrow((Vector2){ cb.x + cb.width, cb.y + cb.height / 2.0f }, (Vector2){ (float)sw - 580, 60.0f + (ctx.currentLine * 20) }, BLUE, 1.0f);
    } else DrawText("PC: IDLE", cb.x + 20, cb.y + 35, 12, BLUE);

    Rectangle sb = { 20, (float)sh/2 - 80, 240, 190 }; DrawRectangleRec(sb, (Color){ 255, 245, 230, 255 }); DrawRectangleLinesEx(sb, 2, (Color){ 255, 161, 0, 255 });
    DrawText("THE STACK", sb.x + 10, sb.y + 10, 14, (Color){ 200, 100, 0, 255 });
    Rectangle topBox = { sb.x + 40, sb.y + 100, 160, 35 }; DrawRectangleRec(topBox, WHITE); DrawRectangleLinesEx(topBox, 1, BLACK); DrawText("top (ptr)", topBox.x + 55, topBox.y + 10, 13, BLACK);
    VisualNode* topNode = GetVisualNode(top_address);
    if (topNode) {
        float prog = (stackStatus == STACK_EXECUTING && ((ctx.type == STACK_FUNC_PUSH && ctx.currentLine == 3) || (ctx.type == STACK_FUNC_POP && ctx.currentLine == 2))) ? ctx.lineProgress : 1.0f;
        DrawStraightArrow((Vector2){ topBox.x + topBox.width, topBox.y + topBox.height / 2.0f }, GetNodeLeftCenterScreen(topNode), BLACK, prog);
    } else DrawStackPointerBox("top", topBox, (Vector2){ topBox.x + topBox.width + 100, topBox.y + topBox.height / 2.0f }, BLACK, true, 1.0f);
    
    if (stackStatus == STACK_EXECUTING) {
        if (!ctx.practiceMode) {
            if (ctx.type == STACK_FUNC_PUSH) DrawPseudocode(sw - 580, 60, pushPseudocode, 4, ctx.currentLine);
            else DrawPseudocode(sw - 580, 60, popPseudocode, 4, ctx.currentLine);
        } else {
            const char* goal = ""; AlgAction req = GetRequiredAction();
            if (ctx.type == STACK_FUNC_PUSH) {
                switch(ctx.logicalStep) { case 0: goal="Step 1: Allocate node."; break; case 1: goal="Step 2: Set data."; break; case 2: goal="Step 3: Update pointers."; break; default: goal="Done!"; break; }
            } else {
                switch(ctx.logicalStep) { case 0: goal="Step 1: Update pointers."; break; case 1: goal="Step 2: Free memory."; break; default: goal="Done!"; break; }
            }
            DrawLogicDiagram(sw - 580, 60, goal, req);
        }
    }

    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    GuiCheckBox((Rectangle){ 130, (float)sh - 100, 20, 20 }, "PRACTICE", &ctx.practiceMode);
    if (GuiButton((Rectangle){ 50, (float)sh - 80, 120, 45 }, "Push")) stackStatus = STACK_INPUT_PARAMS, ctx.type = STACK_FUNC_PUSH;
    if (GuiButton((Rectangle){ 180, (float)sh - 80, 120, 45 }, "Pop")) { ctx.type = STACK_FUNC_POP; ctx.currentLine = 0; ctx.logicalStep = 0; ctx.lineProgress = 0.0f; stackStatus = STACK_EXECUTING; }
    
    if (stackStatus == STACK_EXECUTING) {
        if (!ctx.practiceMode) { if (GuiButton((Rectangle){ 400, (float)sh - 80, 200, 45 }, "NEXT STEP")) StackVisualizer_NextStep(); }
        else {
            float bx = 400;
            if (ctx.type == STACK_FUNC_PUSH) {
                if (GuiButton((Rectangle){ bx, (float)sh - 80, 90, 45 }, "Malloc")) TryExecuteAction(ACT_MALLOC);
                if (GuiButton((Rectangle){ bx + 95, (float)sh - 80, 90, 45 }, "Assign")) TryExecuteAction(ACT_ASSIGN);
                if (GuiButton((Rectangle){ bx + 190, (float)sh - 80, 90, 45 }, "Link")) TryExecuteAction(ACT_LINK);
            } else {
                if (GuiButton((Rectangle){ bx, (float)sh - 80, 90, 45 }, "Link")) TryExecuteAction(ACT_LINK);
                if (GuiButton((Rectangle){ bx + 95, (float)sh - 80, 90, 45 }, "Free")) TryExecuteAction(ACT_FREE);
            }
        }
        if (GuiButton((Rectangle){ sw - 150, (float)sh - 80, 100, 45 }, "CANCEL")) StackVisualizer_CancelInteraction();
    }
    if (stackStatus == STACK_INPUT_PARAMS) {
        Rectangle pb = { (float)sw/2 - 170, (float)sh/2 - 110, 340, 180 }; DrawRectangleRec(pb, WHITE); DrawRectangleLinesEx(pb, 2, BLACK);
        DrawText("PUSH", pb.x + 140, pb.y + 20, 15, BLACK); DrawText("Value:", pb.x + 30, pb.y + 70, 12, BLACK);
        if (GuiTextBox((Rectangle){pb.x + 120, pb.y + 65, 160, 30}, valBuf, 8, valEditMode)) valEditMode = !valEditMode;
        if (GuiButton((Rectangle){pb.x + 120, pb.y + 120, 100, 35}, "START")) { ctx.targetVal = atoi(valBuf); ctx.currentLine = 0; ctx.logicalStep = 0; stackStatus = STACK_EXECUTING; }
    }
    if (showError) {
        Rectangle eb = { (float)sw/2 - 200, (float)sh/2 - 50, 400, 120 }; DrawRectangleRec(eb, WHITE); DrawRectangleLinesEx(eb, 4, BLACK);
        DrawText("LOGIC ERROR", eb.x + 130, eb.y + 20, 18, BLACK); DrawText(errorMsg, eb.x + 20, eb.y + 55, 12, BLACK);
        if (GuiButton((Rectangle){eb.x + 150, eb.y + 80, 100, 30}, "OK")) showError = false;
    }
}
void StackVisualizer_SetSpawnCenter(Vector2 center) { spawnCenter = center; }
void StackVisualizer_SetCamera(Camera2D cam) { viewCamera = cam; }
int StackVisualizer_GetTraversalAddress(void) { return 0; }
bool StackVisualizer_IsBusy(void) { return stackStatus != STACK_IDLE; }
void StackVisualizer_CancelInteraction(void) {
    if (ctx.type == STACK_FUNC_PUSH && ctx.newNodeAddress != 0) { MemoryManager_Free(ctx.newNodeAddress); RemoveVisualNode(ctx.newNodeAddress); }
    stackStatus = STACK_IDLE; ctx.type = STACK_FUNC_NONE; ctx.newNodeAddress = 0; ctx.toDeleteAddress = 0; ctx.logicalStep = 0;
}
