#include "linked_list_visualizer.h"
#include "memory_manager.h"
#include "rlgl.h"
#include "raymath.h"
#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>

static int head_address = 0;
static int curr_address = 0;
static VisualNode visualNodes[MAX_MEM_NODES];
static int visualNodeCount = 0;

static SimStatus simStatus = SIM_IDLE;
static SimContext context = { FUNC_NONE, 0, 0, 0 };
static char errorMsg[256] = "";
static bool showError = false;

// Buffers for parameters
static char valBuf[8] = "10";
static char posBuf[8] = "1";

void LinkedListVisualizer_Init(void) {
    MemoryManager_Init();
    head_address = 0;
    curr_address = 0;
    visualNodeCount = 0;
    simStatus = SIM_IDLE;
    context.type = FUNC_NONE;
    showError = false;
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

// Logic implementations
static void PerformInsert(void) {
    int new_addr = MemoryManager_Malloc(context.targetVal);
    if (new_addr == -1) return;

    MemoryNode* newNode = MemoryManager_GetNode(new_addr);
    if (context.targetPos == 1 || head_address == 0) {
        newNode->next_address = head_address;
        head_address = new_addr;
    } else {
        MemoryNode* prevNode = MemoryManager_GetNode(curr_address);
        newNode->next_address = prevNode->next_address;
        prevNode->next_address = new_addr;
    }

    // Visual Mapping
    visualNodes[visualNodeCount].address = new_addr;
    visualNodes[visualNodeCount].position = (Vector2){ 200, 0 }; // Default offset, user can drag
    visualNodes[visualNodeCount].data = (char)context.targetVal;
    visualNodeCount++;

    simStatus = SIM_IDLE;
    context.type = FUNC_NONE;
}

void LinkedListVisualizer_TraverseStep(void) {
    if (curr_address == 0) {
        showError = true;
        sprintf(errorMsg, "CRITICAL ERROR: Cannot traverse past NULL.");
        return;
    }
    MemoryNode* n = MemoryManager_GetNode(curr_address);
    if (n) {
        curr_address = n->next_address;
        context.currentPos++;
    }
}

void LinkedListVisualizer_ExecuteStep(void) {
    if (context.type == FUNC_INSERT) {
        if (context.currentPos != context.targetPos - 1) {
            showError = true;
            sprintf(errorMsg, "SIMULATION ERROR: You must traverse to position %d first!", context.targetPos - 1);
            return;
        }
        PerformInsert();
    }
}

static void DrawOrthogonalArrow(Vector2 start, Vector2 end, Color color) {
    Vector2 mid = { (start.x + end.x) / 2, start.y };
    Vector2 mid2 = { (start.x + end.x) / 2, end.y };
    DrawLineEx(start, mid, 2, color);
    DrawLineEx(mid, mid2, 2, color);
    DrawLineEx(mid2, end, 2, color);
    DrawTriangle(end, (Vector2){ end.x - 10, end.y - 5 }, (Vector2){ end.x - 10, end.y + 5 }, color);
}

void LinkedListVisualizer_Draw(void) {
    // 1. Draw Connections
    int current = head_address;
    while (current != 0) {
        MemoryNode* node = MemoryManager_GetNode(current);
        VisualNode* vn = GetVisualNode(current);
        if (vn && node->next_address != 0) {
            VisualNode* nextVn = GetVisualNode(node->next_address);
            if (nextVn) {
                Vector2 start = { vn->position.x + 75, vn->position.y + 25 };
                Vector2 end = { nextVn->position.x, nextVn->position.y + 25 };
                DrawOrthogonalArrow(start, end, BLACK);
            }
        }
        current = node->next_address;
    }

    // 2. Draw Nodes
    current = head_address;
    while (current != 0) {
        VisualNode* vn = GetVisualNode(current);
        if (vn) {
            Rectangle rec = { vn->position.x, vn->position.y, 100, 50 };
            DrawRectangleRec(rec, WHITE);
            DrawRectangleLinesEx(rec, (current == curr_address) ? 3 : 1, BLACK);
            DrawLineEx((Vector2){ vn->position.x + 50, vn->position.y }, (Vector2){ vn->position.x + 50, vn->position.y + 50 }, 1, BLACK);
            char val[4]; sprintf(val, "%d", (int)vn->data);
            DrawText(val, vn->position.x + 15, vn->position.y + 15, 15, BLACK);
            DrawCircle(vn->position.x + 75, vn->position.y + 25, 3, BLACK);
            char addr[16]; sprintf(addr, "0x%X", current);
            DrawText(addr, vn->position.x, vn->position.y - 12, 10, DARKGRAY);
        }
        current = MemoryManager_GetNode(current)->next_address;
    }
}

void LinkedListVisualizer_DrawUI(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. THE STACK (Fixed Left Center)
    Rectangle stackBox = { 20, (float)sh/2 - 120, 240, 220 };
    DrawRectangleRec(stackBox, WHITE);
    DrawRectangleLinesEx(stackBox, 2, BLACK);
    DrawText("Stack Memory", stackBox.x + 10, stackBox.y + 10, 13, BLACK);

    if (simStatus == SIM_EXECUTING) {
        DrawRectangle(stackBox.x, stackBox.y + 30, stackBox.width, 30, LIGHTGRAY);
        char func[64]; sprintf(func, "INSERT(List, %d, %d)", context.targetVal, context.targetPos);
        DrawText(func, stackBox.x + 10, stackBox.y + 38, 12, BLACK);
    }

    char headTxt[32]; sprintf(headTxt, "head: 0x%X", head_address);
    DrawText(headTxt, stackBox.x + 20, stackBox.y + 80, 13, BLACK);
    char currTxt[32]; sprintf(currTxt, "curr: 0x%X", curr_address);
    DrawText(currTxt, stackBox.x + 20, stackBox.y + 110, 13, BLACK);

    // 2. Bottom Function Pills
    DrawText("FUNCTIONS", 50, sh - 100, 11, DARKGRAY);
    Rectangle insBtn = { 50, (float)sh - 80, 120, 45 };
    Rectangle delBtn = { 180, (float)sh - 80, 120, 45 };
    
    if (GuiButton(insBtn, "Insert")) {
        simStatus = SIM_INPUT_PARAMS;
        context.type = FUNC_INSERT;
    }
    if (GuiButton(delBtn, "Delete")) {
        // Implement interactive delete similar to insert
    }

    // 3. Conditional Function Keys (Bottom Right/Center)
    if (simStatus == SIM_EXECUTING) {
        DrawText("INSERT FUNCTION KEYS", 400, sh - 100, 11, DARKGRAY);
        Rectangle travKey = { 400, (float)sh - 80, 100, 45 };
        Rectangle execKey = { 510, (float)sh - 80, 100, 45 };
        
        if (GuiButton(travKey, "Trav")) LinkedListVisualizer_TraverseStep();
        if (GuiButton(execKey, "Insert")) LinkedListVisualizer_ExecuteStep();
    }

    // 4. Integrated Parameter Input (No full screen dimming)
    if (simStatus == SIM_INPUT_PARAMS) {
        Rectangle paramBox = { (float)sw/2 - 150, (float)sh/2 - 100, 300, 200 };
        DrawRectangleRec(paramBox, WHITE);
        DrawRectangleLinesEx(paramBox, 2, BLACK);
        DrawText("INSERT(List, val, pos)", paramBox.x + 40, paramBox.y + 20, 15, BLACK);
        
        DrawText("Value:", paramBox.x + 30, paramBox.y + 65, 12, BLACK);
        GuiTextBox((Rectangle){paramBox.x + 100, paramBox.y + 60, 150, 30}, valBuf, 8, true);
        
        DrawText("Position:", paramBox.x + 30, paramBox.y + 105, 12, BLACK);
        GuiTextBox((Rectangle){paramBox.x + 100, paramBox.y + 100, 150, 30}, posBuf, 8, true);
        
        if (GuiButton((Rectangle){paramBox.x + 100, paramBox.y + 150, 100, 35}, "START")) {
            context.targetVal = atoi(valBuf);
            context.targetPos = atoi(posBuf);
            context.currentPos = 0;
            curr_address = head_address;
            simStatus = SIM_EXECUTING;
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
