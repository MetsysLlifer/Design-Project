#include "array_visualizer.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CAPACITY 20

typedef enum { 
    ARR_SETUP_MALLOC_STRUCT,
    ARR_SETUP_INPUT_CAPACITY,
    ARR_SETUP_MALLOC_ARRAY,
    ARR_IDLE, 
    ARR_INPUT_PARAMS, 
    ARR_EXECUTING,
    ARR_RESIZING
} ArrStatus;

typedef enum { ARR_FUNC_NONE, ARR_FUNC_INSERT, ARR_FUNC_DELETE } ArrFunc;

static struct {
    ArrayVersion version;
    ArrFunc type;
    int targetVal;
    int targetPos;
    int currentLine;
    float lineProgress;
    int i; // Loop index
    int structAddress;
    int arrayAddress;
} ctx;

static int array_size = 0;
static int array_capacity = 0;
static VisualNode visualElements[MAX_CAPACITY];
static Vector2 spawnCenter = { 0, 0 };
static Camera2D viewCamera = { 0 };

static ArrStatus arrStatus = ARR_SETUP_INPUT_CAPACITY;
static char valBuf[8] = "10", posBuf[8] = "0", capBuf[8] = "5";
static bool valEditMode = false, posEditMode = false, capEditMode = false;
static char errorMsg[256] = "";
static bool showError = false;
static char notifyMsg[256] = "";
static bool showNotify = false;

static const char* insertPseudocode[] = {
    "if (size == capacity) // Resizing...", // 0
    "for (i = size; i > pos; i--)",          // 1
    "    arr[i] = arr[i-1]",                 // 2
    "arr[pos] = val",                        // 3
    "size++"                                 // 4
};

static const char* deletePseudocode[] = {
    "if (size == 0) return",                 // 0
    "for (i = pos; i < size-1; i++)",        // 1
    "    arr[i] = arr[i+1]",                 // 2
    "size--"                                 // 3
};

static const char* resizePseudocode[] = {
    "int *newArr = malloc(cap * 2 * sz)",    // 0
    "for (i = 0; i < size; i++)",            // 1
    "    newArr[i] = arr[i]",                // 2
    "free(arr)",                             // 3
    "arr = newArr",                          // 4
    "capacity *= 2"                          // 5
};

// --- Helpers ---

static Color elementColors[] = {
    (Color){ 0, 121, 241, 255 },   // Blue
    (Color){ 0, 228, 48, 255 },    // Green
    (Color){ 255, 161, 0, 255 },   // Orange
    (Color){ 200, 122, 255, 255 }, // Purple
    (Color){ 255, 203, 0, 255 },   // Yellow
    (Color){ 255, 80, 80, 255 }    // Red
};

static Color GetRandomElementColor(void) {
    return elementColors[GetRandomValue(0, 5)];
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

void ArrayVisualizer_Init(ArrayVersion version) {
    ctx.version = version;
    array_size = 0;
    array_capacity = 0;
    ctx.structAddress = 0;
    ctx.arrayAddress = 0;
    for (int i = 0; i < MAX_CAPACITY; i++) {
        visualElements[i].data = 0; visualElements[i].color = WHITE;
        visualElements[i].position = (Vector2){ 0, 0 }; visualElements[i].targetPosition = (Vector2){ 0, 0 };
    }
    ctx.type = ARR_FUNC_NONE; ctx.currentLine = 0; ctx.lineProgress = 1.0f;
    valEditMode = posEditMode = capEditMode = showError = showNotify = false;
    sprintf(capBuf, "5");

    if (version == ARR_V2 || version == ARR_V4) {
        arrStatus = ARR_SETUP_MALLOC_STRUCT;
        viewCamera.target = (Vector2){ 0, 0 };
        viewCamera.offset = (Vector2){ (float)GetScreenWidth()/2.0f + 100, (float)GetScreenHeight()/2.0f };
        viewCamera.zoom = 1.0f;
    } else {
        arrStatus = ARR_SETUP_INPUT_CAPACITY;
        viewCamera.target = (Vector2){ 0, 0 };
        viewCamera.offset = (Vector2){ (float)GetScreenWidth()/2.0f, (float)GetScreenHeight()/2.0f };
        viewCamera.zoom = 1.0f;
    }
}

void ArrayVisualizer_Update(Vector2 mouseWorldPos, float zoom) {
    MemoryManager_Update();
    for (int i = 0; i < MAX_CAPACITY; i++) {
        visualElements[i].position.x += (visualElements[i].targetPosition.x - visualElements[i].position.x) * 0.15f;
        visualElements[i].position.y += (visualElements[i].targetPosition.y - visualElements[i].position.y) * 0.15f;
    }
    if (ctx.lineProgress < 1.0f) { ctx.lineProgress += GetFrameTime() * 2.5f; if (ctx.lineProgress > 1.0f) ctx.lineProgress = 1.0f; }
}

void ArrayVisualizer_NextStep(void) {
    ctx.lineProgress = 0.0f;

    if (arrStatus == ARR_RESIZING) {
        switch (ctx.currentLine) {
            case 0: ctx.currentLine = 1; ctx.i = 0; break;
            case 1: if (ctx.i < array_size) ctx.currentLine = 2; else ctx.currentLine = 3; break;
            case 2: ctx.i++; ctx.currentLine = 1; break; // Simulating copy
            case 3: ctx.currentLine = 4; break;
            case 4: 
                ctx.arrayAddress = 0x8000; // Simulate new address
                ctx.currentLine = 5; break;
            case 5: 
                array_capacity *= 2; 
                if (array_capacity > MAX_CAPACITY) array_capacity = MAX_CAPACITY;
                arrStatus = ARR_EXECUTING; 
                ctx.currentLine = 0; // Back to Insert logic
                break;
        }
        return;
    }

    if (ctx.type == ARR_FUNC_INSERT) {
        if (ctx.currentLine == 0 && array_size == array_capacity) {
            if (ctx.version == ARR_V3 || ctx.version == ARR_V4) {
                arrStatus = ARR_RESIZING;
                ctx.currentLine = 0;
                sprintf(notifyMsg, "Capacity reached (%d). Resizing array to %d...", array_capacity, array_capacity * 2);
                showNotify = true;
                return;
            } else {
                showError = true;
                sprintf(errorMsg, "LOGIC ERROR: Array is at full capacity (%d).", array_capacity);
                arrStatus = ARR_IDLE;
                return;
            }
        }

        switch (ctx.currentLine) {
            case 0: ctx.currentLine = 1; ctx.i = array_size; break;
            case 1: if (ctx.i > ctx.targetPos) ctx.currentLine = 2; else ctx.currentLine = 3; break;
            case 2: 
                visualElements[ctx.i].data = visualElements[ctx.i-1].data; 
                visualElements[ctx.i].color = visualElements[ctx.i-1].color;
                visualElements[ctx.i].position.x -= 20.0f; 
                ctx.i--; ctx.currentLine = 1; 
                break;
            case 3: 
                visualElements[ctx.targetPos].data = (char)ctx.targetVal; 
                visualElements[ctx.targetPos].color = GetRandomElementColor();
                visualElements[ctx.targetPos].position.y -= 40.0f;
                ctx.currentLine = 4; 
                break;
            case 4: array_size++; arrStatus = ARR_IDLE; ctx.type = ARR_FUNC_NONE; break;
        }
    } else if (ctx.type == ARR_FUNC_DELETE) {
        switch (ctx.currentLine) {
            case 0: if (array_size == 0 || ctx.targetPos >= array_size) arrStatus = ARR_IDLE; else ctx.currentLine = 1, ctx.i = ctx.targetPos; break;
            case 1: if (ctx.i < array_size - 1) ctx.currentLine = 2; else ctx.currentLine = 3; break;
            case 2: 
                visualElements[ctx.i].data = visualElements[ctx.i+1].data; 
                visualElements[ctx.i].color = visualElements[ctx.i+1].color; 
                visualElements[ctx.i].position.x += 20.0f; 
                ctx.i++; ctx.currentLine = 1; 
                break;
            case 3: array_size--; arrStatus = ARR_IDLE; ctx.type = ARR_FUNC_NONE; break;
        }
    }
}

static void DrawArrayObject(Rectangle cont, Color border, bool isDynamic) {
    DrawRectangleRec(cont, (Color){ 245, 255, 245, 255 });
    DrawRectangleLinesEx(cont, 3, border);
    DrawText(isDynamic ? "Heap Array (malloc'd)" : "int data[MAX]", cont.x + 5, cont.y - 18, 12, border);

    int effS = (arrStatus == ARR_EXECUTING && ctx.type == ARR_FUNC_INSERT) ? array_size + 1 : array_size;
    float sX = cont.x + 10, sY = cont.y + (cont.height - 50) / 2.0f;

    for (int i = 0; i < array_capacity; i++) {
        Vector2 target = { sX + i * 55, sY };
        visualElements[i].targetPosition = target;
        if (visualElements[i].position.x == 0) visualElements[i].position = target;

        Rectangle rec = { visualElements[i].position.x, visualElements[i].position.y, 50, 50 };
        if (i < effS) {
            if ((arrStatus == ARR_EXECUTING || arrStatus == ARR_RESIZING) && i == ctx.i) {
                DrawRectangleLinesEx((Rectangle){ rec.x - 4, rec.y - 4, rec.width + 8, rec.height + 8 }, 2, YELLOW);
            }
            DrawRectangleRec(rec, visualElements[i].color); 
            DrawRectangleLinesEx(rec, 1, border);
            char val[4]; sprintf(val, "%d", (int)visualElements[i].data);
            DrawText(val, rec.x + (rec.width - MeasureText(val, 20))/2, rec.y + 15, 20, WHITE);
        } else {
            DrawRectangleLinesEx(rec, 1, (Color){ border.r, border.g, border.b, 60 });
        }
        char idx[8]; sprintf(idx, "[%d]", i);
        DrawText(idx, rec.x + (rec.width - MeasureText(idx, 10))/2, rec.y + 52, 10, GRAY);
    }
}

void ArrayVisualizer_Draw(void) {
    if (arrStatus < ARR_IDLE && arrStatus != ARR_SETUP_INPUT_CAPACITY) return;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float boxW = 200, boxH = 40;

    if (ctx.version == ARR_V1) {
        float listH = (array_capacity + 1.5f) * boxH + 40;
        float padding = 40;
        float containerW = boxW + padding * 2, containerH = listH + padding * 2;
        Rectangle outerRect = { -containerW/2.0f, -containerH/2.0f, containerW, containerH };
        DrawRectangleRec(outerRect, (Color){ 255, 245, 230, 255 });
        DrawRectangleLinesEx(outerRect, 2, (Color){ 255, 161, 0, 100 });
        DrawText("THE STACK (STATIC MEMORY)", outerRect.x + 10, outerRect.y + 10, 12, (Color){ 255, 161, 0, 255 });
        Rectangle listCont = { -boxW/2.0f, -listH/2.0f + 20, boxW, listH }; Color sBorder = (Color){ 255, 161, 0, 255 };
        DrawRectangleRec(listCont, WHITE); DrawRectangleLinesEx(listCont, 3, sBorder);
        DrawText("struct List L", listCont.x + 5, listCont.y - 22, 16, sBorder);
        float curY = listCont.y + 15;
        Rectangle szRec = { listCont.x + 15, curY, boxW - 30, 32 }; DrawRectangleRec(szRec, (Color){ 255, 245, 230, 255 }); DrawRectangleLinesEx(szRec, 1, sBorder);
        char szT[32]; sprintf(szT, "int size: %d", array_size); DrawText(szT, szRec.x + 10, szRec.y + 10, 13, (Color){ 150, 80, 0, 255 });
        curY += 35; int effS = (arrStatus == ARR_EXECUTING && ctx.type == ARR_FUNC_INSERT) ? array_size + 1 : array_size;
        for (int i = 0; i < array_capacity; i++) {
            Vector2 target = { listCont.x + 15, curY }; visualElements[i].targetPosition = target;
            if (visualElements[i].position.x == 0) visualElements[i].position = target;
            Rectangle rec = { visualElements[i].position.x, visualElements[i].position.y, boxW - 30, 35 };
            if (i < effS) {
                if (arrStatus == ARR_EXECUTING && i == ctx.i) DrawRectangleLinesEx((Rectangle){ rec.x - 4, rec.y - 4, rec.width + 8, rec.height + 8 }, 2, YELLOW);
                DrawRectangleRec(rec, visualElements[i].color); DrawRectangleLinesEx(rec, 1, sBorder);
                char v[4]; sprintf(v, "%d", (int)visualElements[i].data); DrawText(v, rec.x + (rec.width - MeasureText(v, 20))/2, rec.y + 8, 20, WHITE);
            } else DrawRectangleLinesEx(rec, 1, (Color){ 255, 161, 0, 60 });
            char id[8]; sprintf(id, "[%d]", i); DrawText(id, rec.x - 35, rec.y + 10, 12, GRAY); curY += 40;
        }
    } else if (ctx.version == ARR_V2) {
        float listH = (array_capacity + 1.5f) * boxH + 40;
        Rectangle listCont = { -boxW/2.0f, -listH/2.0f, boxW, listH }; Color border = DARKGREEN;
        if (ctx.structAddress != 0) {
            DrawRectangleRec(listCont, (Color){ 245, 255, 245, 255 }); DrawRectangleLinesEx(listCont, 3, border);
            DrawText("struct List (Heap Object)", listCont.x + 5, listCont.y - 22, 16, border);
            float curY = listCont.y + 15;
            Rectangle szRec = { listCont.x + 15, curY, boxW - 30, 32 }; DrawRectangleRec(szRec, WHITE); DrawRectangleLinesEx(szRec, 1, border);
            char szT[32]; sprintf(szT, "int size: %d", array_size); DrawText(szT, szRec.x + 10, szRec.y + 10, 13, border);
            curY += 35; int effS = (arrStatus == ARR_EXECUTING && ctx.type == ARR_FUNC_INSERT) ? array_size + 1 : array_size;
            for (int i = 0; i < array_capacity; i++) {
                Vector2 target = { listCont.x + 15, curY }; visualElements[i].targetPosition = target;
                if (visualElements[i].position.x == 0) visualElements[i].position = target;
                Rectangle rec = { visualElements[i].position.x, visualElements[i].position.y, boxW - 30, 35 };
                if (i < effS) {
                    if (arrStatus == ARR_EXECUTING && i == ctx.i) DrawRectangleLinesEx((Rectangle){ rec.x - 4, rec.y - 4, rec.width + 8, rec.height + 8 }, 2, YELLOW);
                    DrawRectangleRec(rec, visualElements[i].color); DrawRectangleLinesEx(rec, 1, border);
                    char v[4]; sprintf(v, "%d", (int)visualElements[i].data); DrawText(v, rec.x + (rec.width - MeasureText(v, 20))/2, rec.y + 8, 20, WHITE);
                } else DrawRectangleLinesEx(rec, 1, (Color){ 0, 100, 0, 60 });
                char id[8]; sprintf(id, "[%d]", i); DrawText(id, rec.x - 35, rec.y + 10, 12, GRAY); curY += 40;
            }
        }
    } else if (ctx.version == ARR_V3 || ctx.version == ARR_V4) {
        Rectangle structCont = { -450, -60, 180, 120 }; Color sBorder = (ctx.version == ARR_V3) ? (Color){ 255, 161, 0, 255 } : DARKGREEN;
        DrawRectangleRec(structCont, WHITE); DrawRectangleLinesEx(structCont, 3, sBorder);
        DrawText(ctx.version == ARR_V3 ? "struct List L (Stack)" : "struct List (Heap)", structCont.x, structCont.y - 20, 12, sBorder);
        char szT[32]; sprintf(szT, "size: %d", array_size); DrawText(szT, structCont.x + 10, structCont.y + 15, 13, sBorder);
        char cpT[32]; sprintf(cpT, "capacity: %d", array_capacity); DrawText(cpT, structCont.x + 10, structCont.y + 40, 13, sBorder);
        Rectangle ptrBox = { structCont.x + 10, structCont.y + 65, 160, 35 };
        DrawRectangleRec(ptrBox, (Color){ 245, 255, 245, 255 }); DrawRectangleLinesEx(ptrBox, 1, DARKGREEN);
        DrawText("int *data", ptrBox.x + 10, ptrBox.y + 10, 12, DARKGREEN);
        if (ctx.arrayAddress != 0) {
            float arrW = array_capacity * 55 + 20; Rectangle arrCont = { -200, -40, arrW, 80 };
            DrawArrayObject(arrCont, DARKGREEN, true);
            Vector2 ptrOrigin = { ptrBox.x + ptrBox.width, ptrBox.y + ptrBox.height / 2.0f };
            Vector2 arrayTarget = { arrCont.x, arrCont.y + arrCont.height / 2.0f };
            float prog = (arrStatus == ARR_RESIZING && ctx.currentLine == 4) ? ctx.lineProgress : 1.0f;
            DrawStraightArrow(ptrOrigin, arrayTarget, BLACK, prog);
        }
    }
    DrawText("Arrays offer fast O(1) random access but slow O(n) insertions/deletions due to physical shifting.", sw/2 - 350, sh - 140, 13, DARKGRAY);
}

void ArrayVisualizer_DrawUI(void) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Rectangle cb = { 20, 60, 240, 70 }; DrawRectangleRec(cb, (Color){ 230, 240, 255, 255 }); DrawRectangleLinesEx(cb, 2, DARKBLUE);
    DrawText("THE EXECUTOR", cb.x + 10, cb.y + 10, 14, DARKBLUE);
    if (arrStatus == ARR_EXECUTING || arrStatus == ARR_RESIZING) {
        char fn[64]; const char* vName = (ctx.version == ARR_V1 || ctx.version == ARR_V3) ? "L" : "(*L)";
        if (arrStatus == ARR_RESIZING) sprintf(fn, "RESIZE(%s, %d)", vName, array_capacity * 2);
        else sprintf(fn, (ctx.type == ARR_FUNC_INSERT) ? "INSERT(%s, %d, %d)" : "DELETE(%s, %d)", vName, ctx.targetVal, ctx.targetPos);
        DrawText(fn, cb.x + 20, cb.y + 35, 12, DARKBLUE);
        char pc[32]; sprintf(pc, "PC: 0x%X", 0xA00 + (ctx.currentLine * 4)); DrawText(pc, cb.x + 20, cb.y + 50, 11, BLUE);
        DrawStraightArrow((Vector2){ cb.x + cb.width, cb.y + cb.height / 2.0f }, (Vector2){ (float)sw - 580, 60.0f + (ctx.currentLine * 20) }, BLUE, 1.0f);
    } else DrawText("PC: IDLE", cb.x + 20, cb.y + 35, 12, BLUE);

    if (ctx.version != ARR_V1 && ctx.version != ARR_V3) {
        Rectangle sb = { 20, (float)sh/2 - 80, 240, 190 }; DrawRectangleRec(sb, (Color){ 255, 245, 230, 255 }); DrawRectangleLinesEx(sb, 2, (Color){ 255, 161, 0, 255 });
        DrawText("THE STACK", sb.x + 10, sb.y + 10, 14, (Color){ 200, 100, 0, 255 });
        if (ctx.version == ARR_V2 || ctx.version == ARR_V4) {
            Rectangle stB = { sb.x + 40, sb.y + 80, 160, 35 }; DrawRectangleRec(stB, WHITE); DrawRectangleLinesEx(stB, 1, (Color){ 200, 100, 0, 255 });
            DrawText("struct List *L", stB.x + 45, stB.y + 10, 13, (Color){ 200, 100, 0, 255 });
            if (ctx.structAddress != 0) {
                Vector2 target = (ctx.version == ARR_V2) ? (Vector2){ -100, 0 } : (Vector2){ -450, 0 };
                DrawStraightArrow((Vector2){ stB.x + stB.width, stB.y + stB.height / 2.0f }, GetWorldToScreen2D(target, viewCamera), BLACK, 1.0f);
            }
        }
    }
    
    if (arrStatus == ARR_SETUP_MALLOC_STRUCT) {
        Rectangle b = { (float)sw/2 - 180, (float)sh/2 - 60, 360, 120 }; DrawRectangleRec(b, WHITE); DrawRectangleLinesEx(b, 2, BLACK);
        DrawText("PROCEDURE: ALLOCATE STRUCT", b.x + 40, b.y + 20, 16, DARKBLUE);
        if (GuiButton((Rectangle){b.x + 80, b.y + 60, 200, 40}, "L = malloc(sizeof(List))")) { ctx.structAddress = 0x5000; arrStatus = ARR_SETUP_INPUT_CAPACITY; }
    } else if (arrStatus == ARR_SETUP_INPUT_CAPACITY) {
        Rectangle b = { (float)sw/2 - 180, (float)sh/2 - 80, 360, 160 }; DrawRectangleRec(b, WHITE); DrawRectangleLinesEx(b, 2, BLACK);
        DrawText(ctx.version >= ARR_V3 ? "SET INITIAL CAPACITY" : "SET MAX_SIZE (FIXED)", b.x + 60, b.y + 20, 16, DARKBLUE);
        if (GuiTextBox((Rectangle){b.x + 100, b.y + 60, 160, 35}, capBuf, 8, capEditMode)) capEditMode = !capEditMode;
        if (GuiButton((Rectangle){b.x + 130, b.y + 110, 100, 35}, "CONTINUE")) {
            array_capacity = atoi(capBuf); if (array_capacity <= 0 || array_capacity > MAX_CAPACITY) array_capacity = MAX_CAPACITY;
            capEditMode = false; if (ctx.version >= ARR_V3) arrStatus = ARR_SETUP_MALLOC_ARRAY; else arrStatus = ARR_IDLE;
        }
    } else if (arrStatus == ARR_SETUP_MALLOC_ARRAY) {
        Rectangle b = { (float)sw/2 - 180, (float)sh/2 - 60, 360, 120 }; DrawRectangleRec(b, WHITE); DrawRectangleLinesEx(b, 2, BLACK);
        DrawText("PROCEDURE: ALLOCATE ARRAY", b.x + 45, b.y + 20, 16, DARKBLUE);
        char bt[64]; sprintf(bt, "L->data = malloc(%d * sizeof(int))", array_capacity);
        if (GuiButton((Rectangle){b.x + 40, b.y + 60, 280, 40}, bt)) { ctx.arrayAddress = 0x7000; arrStatus = ARR_IDLE; }
    }

    if (arrStatus == ARR_IDLE || arrStatus == ARR_INPUT_PARAMS) {
        DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
        if (GuiButton((Rectangle){ 50, (float)sh - 80, 120, 45 }, "Insert")) arrStatus = ARR_INPUT_PARAMS, ctx.type = ARR_FUNC_INSERT;
        if (GuiButton((Rectangle){ 180, (float)sh - 80, 120, 45 }, "Delete")) arrStatus = ARR_INPUT_PARAMS, ctx.type = ARR_FUNC_DELETE;
    }

    if (arrStatus == ARR_EXECUTING || arrStatus == ARR_RESIZING) {
        if (arrStatus == ARR_RESIZING) DrawPseudocode(sw - 580, 60, resizePseudocode, 6, ctx.currentLine);
        else if (ctx.type == ARR_FUNC_INSERT) DrawPseudocode(sw - 580, 60, insertPseudocode, 5, ctx.currentLine);
        else DrawPseudocode(sw - 580, 60, deletePseudocode, 4, ctx.currentLine);
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
            int v = atoi(valBuf), p = atoi(posBuf);
            if (ctx.type == ARR_FUNC_INSERT) {
                if (p < 0) p = 0; if (p > array_size) { sprintf(notifyMsg, "Pos %d too far. Clamping to %d.", p, array_size); p = array_size; showNotify = true; }
            } else {
                if (array_size == 0) { showError = true; sprintf(errorMsg, "LOGIC ERROR: Array is empty."); return; }
                if (p < 0 || p >= array_size) { showError = true; sprintf(errorMsg, "LOGIC ERROR: Position %d invalid.", p); return; }
            }
            ctx.targetVal = v; ctx.targetPos = p; valEditMode = posEditMode = false; ctx.currentLine = 0; arrStatus = ARR_EXECUTING;
        }
    }
    if (showNotify) {
        Rectangle b = { (float)sw/2 - 180, (float)sh/2 - 50, 360, 100 }; DrawRectangleRec(b, WHITE); DrawRectangleLinesEx(b, 2, DARKBLUE);
        DrawText("NOTIFICATION", b.x + 110, b.y + 15, 16, DARKBLUE); DrawText(notifyMsg, b.x + 20, b.y + 45, 12, BLACK);
        if (GuiButton((Rectangle){b.x + 130, b.y + 65, 100, 25}, "PROCEED")) showNotify = false;
    }
    if (showError) {
        Rectangle eb = { (float)sw/2 - 200, (float)sh/2 - 50, 400, 120 }; DrawRectangleRec(eb, WHITE); DrawRectangleLinesEx(eb, 4, BLACK);
        DrawText("LOGIC ERROR", eb.x + 130, eb.y + 20, 18, BLACK); DrawText(errorMsg, eb.x + 20, eb.y + 55, 12, BLACK);
        if (GuiButton((Rectangle){eb.x + 150, eb.y + 80, 100, 30}, "OK")) showError = false;
    }
}
void ArrayVisualizer_SetSpawnCenter(Vector2 center) { spawnCenter = center; }
void ArrayVisualizer_SetCamera(Camera2D cam) { viewCamera = cam; }
bool ArrayVisualizer_IsBusy(void) { return arrStatus != ARR_IDLE && arrStatus != ARR_SETUP_MALLOC_STRUCT && arrStatus != ARR_SETUP_INPUT_CAPACITY && arrStatus != ARR_SETUP_MALLOC_ARRAY; }
void ArrayVisualizer_CancelInteraction(void) { arrStatus = ARR_IDLE; }
