#include "array_visualizer.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CAPACITY 10

static int array_size = 0;
static int array_capacity = MAX_CAPACITY;
static VisualNode visualElements[MAX_CAPACITY];
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

typedef enum { ARR_IDLE, ARR_INPUT_PARAMS, ARR_EXECUTING } ArrStatus;
typedef enum { ARR_FUNC_NONE, ARR_FUNC_INSERT, ARR_FUNC_DELETE } ArrFunc;

static struct {
    ArrFunc type;
    int targetVal;
    int targetPos;
    int currentLine;
    float lineProgress;
    int i; // Loop index
} ctx;

static ArrStatus arrStatus = ARR_IDLE;
static char valBuf[8] = "10", posBuf[8] = "0";
static bool valEditMode = false, posEditMode = false;

static const char* insertPseudocode[] = {
    "if (size == capacity) return",      // 0
    "for (i = size; i > pos; i--)",      // 1
    "    arr[i] = arr[i-1]",             // 2
    "arr[pos] = val",                    // 3
    "size++"                             // 4
};

static const char* deletePseudocode[] = {
    "if (size == 0) return",             // 0
    "for (i = pos; i < size-1; i++)",    // 1
    "    arr[i] = arr[i+1]",             // 2
    "size--"                             // 3
};

// --- Helpers ---

static Vector2 GetElementPosition(int index) {
    float startX = spawnCenter.x - (MAX_CAPACITY * 60.0f) / 2.0f;
    return (Vector2){ startX + index * 60.0f, spawnCenter.y };
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

void ArrayVisualizer_Init(void) {
    array_size = 0;
    for (int i = 0; i < MAX_CAPACITY; i++) {
        visualElements[i].data = 0;
        visualElements[i].position = GetElementPosition(i);
        visualElements[i].targetPosition = visualElements[i].position;
    }
    arrStatus = ARR_IDLE; 
    ctx.type = ARR_FUNC_NONE; 
    ctx.currentLine = 0; 
    ctx.lineProgress = 1.0f;
    valEditMode = false;
    posEditMode = false;
}

void ArrayVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    for (int i = 0; i < MAX_CAPACITY; i++) {
        visualElements[i].position.x += (visualElements[i].targetPosition.x - visualElements[i].position.x) * 0.15f;
        visualElements[i].position.y += (visualElements[i].targetPosition.y - visualElements[i].position.y) * 0.15f;
    }
    if (ctx.lineProgress < 1.0f) { ctx.lineProgress += GetFrameTime() * 2.5f; if (ctx.lineProgress > 1.0f) ctx.lineProgress = 1.0f; }
}

void ArrayVisualizer_NextStep(void) {
    ctx.lineProgress = 0.0f;
    if (ctx.type == ARR_FUNC_INSERT) {
        switch (ctx.currentLine) {
            case 0: 
                if (array_size >= array_capacity) {
                    arrStatus = ARR_IDLE;
                } else {
                    ctx.currentLine = 1; 
                    ctx.i = array_size;
                }
                break;
            case 1: 
                if (ctx.i > ctx.targetPos) ctx.currentLine = 2; 
                else ctx.currentLine = 3; 
                break;
            case 2: 
                visualElements[ctx.i].data = visualElements[ctx.i-1].data; 
                ctx.i--; 
                ctx.currentLine = 1; 
                break;
            case 3: 
                visualElements[ctx.targetPos].data = (char)ctx.targetVal; 
                ctx.currentLine = 4; 
                break;
            case 4: 
                array_size++; 
                arrStatus = ARR_IDLE; 
                ctx.type = ARR_FUNC_NONE;
                break;
        }
    } else if (ctx.type == ARR_FUNC_DELETE) {
        switch (ctx.currentLine) {
            case 0: 
                if (array_size == 0 || ctx.targetPos >= array_size) arrStatus = ARR_IDLE; 
                else ctx.currentLine = 1, ctx.i = ctx.targetPos; 
                break;
            case 1: 
                if (ctx.i < array_size - 1) ctx.currentLine = 2; 
                else ctx.currentLine = 3; 
                break;
            case 2: 
                visualElements[ctx.i].data = visualElements[ctx.i+1].data; 
                ctx.i++; 
                ctx.currentLine = 1; 
                break;
            case 3: 
                array_size--; 
                arrStatus = ARR_IDLE; 
                ctx.type = ARR_FUNC_NONE;
                break;
        }
    }
}

void ArrayVisualizer_Draw(void) {
    int effectiveSize = array_size;
    if (arrStatus == ARR_EXECUTING && ctx.type == ARR_FUNC_INSERT) {
        // During insertion, we might be shifting into the 'size' index
        effectiveSize = array_size + 1;
    }

    for (int i = 0; i < MAX_CAPACITY; i++) {
        Rectangle rec = { visualElements[i].position.x, visualElements[i].position.y, 55, 55 };
        Color bc = (Color){ 245, 255, 245, 255 }; // Heap green
        
        if (i < effectiveSize) {
            // Highlight the current index being shifted or modified
            if (arrStatus == ARR_EXECUTING && i == ctx.i) bc = (Color){ 255, 255, 200, 255 };
            // Highlight target position specifically during insertion
            if (arrStatus == ARR_EXECUTING && ctx.type == ARR_FUNC_INSERT && i == ctx.targetPos && ctx.currentLine >= 3) bc = (Color){ 200, 255, 200, 255 };

            DrawRectangleRec(rec, bc); 
            DrawRectangleLinesEx(rec, 2, DARKGREEN);
            char val[4]; sprintf(val, "%d", (int)visualElements[i].data);
            DrawText(val, rec.x + (55 - MeasureText(val, 20))/2, rec.y + 17, 20, DARKGREEN);
        } else {
            DrawRectangleLinesEx(rec, 1, (Color){ 0, 100, 0, 40 });
        }
        char idx[4]; sprintf(idx, "[%d]", i); DrawText(idx, rec.x + 15, rec.y + 60, 10, GRAY);
    }
}

void ArrayVisualizer_DrawUI(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Rectangle cb = { 20, 60, 240, 70 }; DrawRectangleRec(cb, (Color){ 230, 240, 255, 255 }); DrawRectangleLinesEx(cb, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cb.x + 10, cb.y + 10, 14, DARKBLUE);
    if (arrStatus == ARR_EXECUTING) {
        char fn[32]; if (ctx.type == ARR_FUNC_INSERT) sprintf(fn, "INSERT(Arr, %d, %d)", ctx.targetVal, ctx.targetPos); else sprintf(fn, "DELETE(Arr, %d)", ctx.targetPos);
        DrawText(fn, cb.x + 20, cb.y + 35, 12, DARKBLUE);
        char pc[32]; sprintf(pc, "PC: 0x%X", 0xA00 + (ctx.currentLine * 4)); DrawText(pc, cb.x + 20, cb.y + 50, 11, BLUE);
        DrawStraightArrow((Vector2){ cb.x + cb.width, cb.y + cb.height / 2.0f }, (Vector2){ (float)sw - 580, 60.0f + (ctx.currentLine * 20) }, BLUE, 1.0f);
    } else DrawText("PC: IDLE", cb.x + 20, cb.y + 35, 12, BLUE);

    Rectangle sb = { 20, (float)sh/2 - 80, 240, 190 }; DrawRectangleRec(sb, (Color){ 255, 245, 230, 255 }); DrawRectangleLinesEx(sb, 2, (Color){ 255, 161, 0, 255 });
    DrawText("THE STACK", sb.x + 10, sb.y + 10, 14, (Color){ 200, 100, 0, 255 });
    
    Rectangle ab = { sb.x + 40, sb.y + 100, 160, 35 }; DrawRectangleRec(ab, WHITE); DrawRectangleLinesEx(ab, 1, BLACK);
    DrawText("arr (ptr)", ab.x + 55, ab.y + 10, 13, BLACK);
    DrawStraightArrow((Vector2){ ab.x + ab.width, ab.y + ab.height / 2.0f }, GetWorldToScreen2D(GetElementPosition(0), viewCamera), BLACK, 1.0f);
    
    char sz[32]; sprintf(sz, "size: %d", array_size); DrawText(sz, sb.x + 20, sb.y + 60, 13, (Color){ 150, 80, 0, 255 });
    char cp[32]; sprintf(cp, "capacity: %d", array_capacity); DrawText(cp, sb.x + 20, sb.y + 80, 13, (Color){ 150, 80, 0, 255 });

    if (arrStatus == ARR_EXECUTING) {
        if (ctx.type == ARR_FUNC_INSERT) DrawPseudocode(sw - 580, 60, insertPseudocode, 5, ctx.currentLine);
        else DrawPseudocode(sw - 580, 60, deletePseudocode, 4, ctx.currentLine);
    }
    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    if (GuiButton((Rectangle){ 50, (float)sh - 80, 120, 45 }, "Insert")) arrStatus = ARR_INPUT_PARAMS, ctx.type = ARR_FUNC_INSERT;
    if (GuiButton((Rectangle){ 180, (float)sh - 80, 120, 45 }, "Delete")) arrStatus = ARR_INPUT_PARAMS, ctx.type = ARR_FUNC_DELETE;
    
    if (arrStatus == ARR_EXECUTING) {
        if (GuiButton((Rectangle){ 400, (float)sh - 80, 200, 45 }, "NEXT STEP")) ArrayVisualizer_NextStep();
        if (GuiButton((Rectangle){ 610, (float)sh - 80, 100, 45 }, "CANCEL")) arrStatus = ARR_IDLE;
    }
    if (arrStatus == ARR_INPUT_PARAMS) {
        Rectangle pb = { (float)sw/2 - 170, (float)sh/2 - 110, 340, 220 };
        DrawRectangleRec(pb, WHITE); DrawRectangleLinesEx(pb, 2, BLACK);
        DrawText(ctx.type == ARR_FUNC_INSERT ? "INSERT(Arr, val, pos)" : "DELETE(Arr, pos)", pb.x + 60, pb.y + 20, 15, BLACK);
        if (ctx.type == ARR_FUNC_INSERT) {
            DrawText("Value:", pb.x + 30, pb.y + 70, 12, BLACK);
            if (GuiTextBox((Rectangle){pb.x + 120, pb.y + 65, 160, 30}, valBuf, 8, valEditMode)) { valEditMode = !valEditMode; posEditMode = false; }
        }
        DrawText("Position:", pb.x + 30, pb.y + 110, 12, BLACK);
        if (GuiTextBox((Rectangle){pb.x + 120, pb.y + 105, 160, 30}, posBuf, 8, posEditMode)) { posEditMode = !posEditMode; valEditMode = false; }
        if (GuiButton((Rectangle){pb.x + 120, pb.y + 160, 100, 35}, "START")) {
            ctx.targetVal = atoi(valBuf); ctx.targetPos = atoi(posBuf);
            valEditMode = posEditMode = false; ctx.currentLine = 0; arrStatus = ARR_EXECUTING;
        }
    }
}

void ArrayVisualizer_SetSpawnCenter(Vector2 center) { spawnCenter = center; }
void ArrayVisualizer_SetCamera(Camera2D cam) { viewCamera = cam; }
bool ArrayVisualizer_IsBusy(void) { return arrStatus != ARR_IDLE; }
void ArrayVisualizer_CancelInteraction(void) { arrStatus = ARR_IDLE; }
