#include "raylib.h"
#include "raymath.h"
#include "array_visualizer.h"
#include "linked_list_visualizer.h"
#include "stack_visualizer.h"
#include "queue_visualizer.h"
#include "graph_visualizer.h"
#include "memory_manager.h"
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef enum { SCENE_MAIN_MENU, SCENE_START, SCENE_ARRAY_VERSION_SELECT, SCENE_VISUALIZER } AppScene;
typedef enum { ADT_LIST, ADT_STACK, ADT_QUEUE } ADTType;
typedef enum { IMPL_ARRAY, IMPL_LINKED, IMPL_CURSOR } ImplType;

AppScene currentScene = SCENE_MAIN_MENU;
ADTType selectedADT = ADT_LIST;
ImplType selectedImpl = IMPL_LINKED;
Camera2D camera = { 0 };
bool showMemoryVis = true;

void InitApp(void) {
    ArrayVisualizer_Init(ARR_V1);
    LinkedListVisualizer_Init();
    StackVisualizer_Init();
    QueueVisualizer_Init();
    GraphVisualizer_Init();
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ 640, 360 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void UpdateCanvas(void) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = mouseWorldPos;
        camera.zoom += wheel * 0.1f;
        if (camera.zoom < 0.1f) camera.zoom = 0.1f;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }
    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
    Vector2 screenCenter = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
    Vector2 centerWorldPos = GetScreenToWorld2D(screenCenter, camera);

    if (selectedADT == ADT_LIST) {
        if (selectedImpl == IMPL_LINKED) {
            LinkedListVisualizer_SetSpawnCenter(centerWorldPos);
            LinkedListVisualizer_SetCamera(camera);
            LinkedListVisualizer_Update(mouseWorldPos, camera.zoom);
        } else {
            ArrayVisualizer_SetSpawnCenter(centerWorldPos);
            ArrayVisualizer_SetCamera(camera);
            ArrayVisualizer_Update(mouseWorldPos, camera.zoom);
        }
    } else if (selectedADT == ADT_STACK) {
        StackVisualizer_SetSpawnCenter(centerWorldPos);
        StackVisualizer_SetCamera(camera);
        StackVisualizer_Update(mouseWorldPos, camera.zoom);
    } else if (selectedADT == ADT_QUEUE) {
        QueueVisualizer_SetSpawnCenter(centerWorldPos);
        QueueVisualizer_SetCamera(camera);
        QueueVisualizer_Update(mouseWorldPos, camera.zoom);
    }
}

int main(void) {
    const int sw = 1280;
    const int sh = 720;
    InitWindow(sw, sh, "DSA Visualizer");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    InitApp();

    bool shouldQuit = false;
    bool showExitConfirm = false;
    bool showCancelConfirm = false;

    while (!shouldQuit) {
        if (WindowShouldClose()) showExitConfirm = true;
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (currentScene == SCENE_MAIN_MENU) {
                showExitConfirm = true;
            } else if (currentScene == SCENE_VISUALIZER) {
                bool isBusy = false;
                if (selectedADT == ADT_LIST) {
                    if (selectedImpl == IMPL_LINKED) isBusy = LinkedListVisualizer_IsBusy();
                    else if (selectedImpl == IMPL_ARRAY) isBusy = ArrayVisualizer_IsBusy();
                }
                else if (selectedADT == ADT_STACK) isBusy = StackVisualizer_IsBusy();
                else if (selectedADT == ADT_QUEUE) isBusy = QueueVisualizer_IsBusy();
                
                if (isBusy) showCancelConfirm = true;
            }
        }

        if (currentScene == SCENE_VISUALIZER) UpdateCanvas();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScene) {
            case SCENE_MAIN_MENU:
                DrawText("DSA VISUALIZER", sw/2 - MeasureText("DSA VISUALIZER", 40)/2, 200, 40, BLACK);
                if (GuiButton((Rectangle){ (float)sw/2 - 100, 300, 200, 50 }, "START")) currentScene = SCENE_START;
                break;

            case SCENE_START:
                DrawText("SELECT DATA STRUCTURE & IMPLEMENTATION", sw/2 - MeasureText("SELECT DATA STRUCTURE & IMPLEMENTATION", 25)/2, 50, 25, BLACK);
                
                float colWidth = 350;
                float startX = (sw - (colWidth * 3)) / 2.0f;
                float baseY = 150;

                // Column 1: Linked-List (Orange Stack Theme)
                DrawRectangle(startX, baseY - 40, colWidth - 20, 35, (Color){ 255, 245, 230, 255 });
                DrawRectangleLinesEx((Rectangle){ startX, baseY - 40, colWidth - 20, 35 }, 2, (Color){ 255, 161, 0, 255 });
                DrawText("LINKED-LIST BASED", startX + 60, baseY - 30, 18, (Color){ 200, 100, 0, 255 });
                
                if (GuiButton((Rectangle){ startX, baseY, colWidth - 20, 45 }, "LIST (Linked-List)")) { 
                    selectedADT = ADT_LIST; selectedImpl = IMPL_LINKED; LinkedListVisualizer_Init(); currentScene = SCENE_VISUALIZER; 
                }
                if (GuiButton((Rectangle){ startX, baseY + 55, colWidth - 20, 45 }, "STACK (Linked-List)")) { 
                    selectedADT = ADT_STACK; selectedImpl = IMPL_LINKED; StackVisualizer_Init(); currentScene = SCENE_VISUALIZER; 
                }
                if (GuiButton((Rectangle){ startX, baseY + 110, colWidth - 20, 45 }, "QUEUE (Linked-List)")) { 
                    selectedADT = ADT_QUEUE; selectedImpl = IMPL_LINKED; QueueVisualizer_Init(); currentScene = SCENE_VISUALIZER; 
                }

                // Column 2: Array-Based (Green Heap Theme)
                DrawRectangle(startX + colWidth, baseY - 40, colWidth - 20, 35, (Color){ 245, 255, 245, 255 });
                DrawRectangleLinesEx((Rectangle){ startX + colWidth, baseY - 40, colWidth - 20, 35 }, 2, DARKGREEN);
                DrawText("ARRAY-BASED", startX + colWidth + 85, baseY - 30, 18, DARKGREEN);
                
                if (GuiButton((Rectangle){ startX + colWidth, baseY, colWidth - 20, 45 }, "LIST (Array-based)")) { 
                    selectedADT = ADT_LIST; selectedImpl = IMPL_ARRAY; currentScene = SCENE_ARRAY_VERSION_SELECT; 
                }
                // Placeholder buttons for future array-based structures
                GuiSetState(STATE_DISABLED);
                GuiButton((Rectangle){ startX + colWidth, baseY + 55, colWidth - 20, 45 }, "STACK (Array-based)");
                GuiButton((Rectangle){ startX + colWidth, baseY + 110, colWidth - 20, 45 }, "QUEUE (Array-based)");
                GuiSetState(STATE_NORMAL);

                // Column 3: Cursor-Based (Cool Blue CPU Theme)
                DrawRectangle(startX + colWidth * 2, baseY - 40, colWidth - 20, 35, (Color){ 230, 240, 255, 255 });
                DrawRectangleLinesEx((Rectangle){ startX + colWidth * 2, baseY - 40, colWidth - 20, 35 }, 2, DARKBLUE);
                DrawText("CURSOR-BASED", startX + colWidth * 2 + 80, baseY - 30, 18, DARKBLUE);
                
                Rectangle cursorBox = { startX + colWidth * 2, baseY, colWidth - 20, 155 };
                DrawRectangleLinesEx(cursorBox, 1, GRAY);
                DrawText("Coming Soon!", cursorBox.x + 85, cursorBox.y + 50, 20, DARKGRAY);
                DrawText("This implementation style will", cursorBox.x + 35, cursorBox.y + 85, 12, GRAY);
                DrawText("be added in future development.", cursorBox.x + 32, cursorBox.y + 105, 12, GRAY);

                if (GuiButton((Rectangle){ 50, sh - 70, 150, 40 }, "BACK")) currentScene = SCENE_MAIN_MENU;
                break;

            case SCENE_ARRAY_VERSION_SELECT:
                DrawText("SELECT ARRAY IMPLEMENTATION VERSION", sw/2 - MeasureText("SELECT ARRAY IMPLEMENTATION VERSION", 25)/2, 100, 25, BLACK);
                
                float vStartX = sw/2 - 200;
                float vStartY = 200;
                float vWidth = 400;
                float vHeight = 60;
                float vGap = 15;

                if (GuiButton((Rectangle){ vStartX, vStartY, vWidth, vHeight }, "V1: Standard Static\n(Array in Static Struct)")) {
                    ArrayVisualizer_Init(ARR_V1);
                    currentScene = SCENE_VISUALIZER;
                }
                if (GuiButton((Rectangle){ vStartX, vStartY + vHeight + vGap, vWidth, vHeight }, "V2: Heap Structure (Static)\n(Struct on Heap, Fixed Array)")) {
                    ArrayVisualizer_Init(ARR_V2); 
                    currentScene = SCENE_VISUALIZER;
                }
                if (GuiButton((Rectangle){ vStartX, vStartY + (vHeight + vGap) * 2, vWidth, vHeight }, "V3: Dynamic Array\n(Static Struct, Malloc'd Array)")) {
                    ArrayVisualizer_Init(ARR_V3);
                    currentScene = SCENE_VISUALIZER;
                }
                if (GuiButton((Rectangle){ vStartX, vStartY + (vHeight + vGap) * 3, vWidth, vHeight }, "V4: Fully Dynamic\n(Malloc'd Struct & Array)")) {
                    ArrayVisualizer_Init(ARR_V4);
                    currentScene = SCENE_VISUALIZER;
                }

                if (GuiButton((Rectangle){ sw/2 - 75, sh - 100, 150, 40 }, "BACK")) currentScene = SCENE_START;
                break;

            case SCENE_VISUALIZER:
                BeginMode2D(camera);
                    if (selectedADT == ADT_LIST) {
                        if (selectedImpl == IMPL_LINKED) LinkedListVisualizer_Draw();
                        else if (selectedImpl == IMPL_ARRAY) ArrayVisualizer_Draw();
                    }
                    else if (selectedADT == ADT_STACK) StackVisualizer_Draw();
                    else if (selectedADT == ADT_QUEUE) QueueVisualizer_Draw();
                EndMode2D();

                if (selectedADT == ADT_LIST) {
                    if (selectedImpl == IMPL_LINKED) LinkedListVisualizer_DrawUI();
                    else if (selectedImpl == IMPL_ARRAY) ArrayVisualizer_DrawUI();
                }
                else if (selectedADT == ADT_STACK) StackVisualizer_DrawUI();
                else if (selectedADT == ADT_QUEUE) QueueVisualizer_DrawUI();

                if (showMemoryVis) {
                    int travAddr = 0;
                    if (selectedADT == ADT_LIST) {
                        if (selectedImpl == IMPL_LINKED) travAddr = LinkedListVisualizer_GetTraversalAddress();
                        // Array implementation usually uses direct indexing, but we can pass 0
                    }
                    else if (selectedADT == ADT_STACK) travAddr = StackVisualizer_GetTraversalAddress();
                    else if (selectedADT == ADT_QUEUE) travAddr = QueueVisualizer_GetTraversalAddress();
                    
                    MemoryManager_Draw((Rectangle){ (float)sw - 250, 50, 200, 600 }, travAddr);
                }

                if (GuiButton((Rectangle){ 20, 20, 80, 30 }, "BACK")) currentScene = SCENE_START;
                GuiCheckBox((Rectangle){ 110, 25, 20, 20 }, "MEMORY", &showMemoryVis);
                break;
        }

        if (showExitConfirm) {
            Rectangle box = { (float)sw/2 - 180, (float)sh/2 - 90, 360, 180 };
            DrawRectangleRec(box, RAYWHITE);
            DrawRectangleLinesEx(box, 2, BLACK);
            DrawText("EXIT APP?", box.x + 120, box.y + 25, 18, BLACK);
            DrawText("Do you want to quit?", box.x + 90, box.y + 70, 12, DARKGRAY);
            if (GuiButton((Rectangle){ box.x + 60, box.y + 110, 100, 35 }, "YES")) shouldQuit = true;
            if (GuiButton((Rectangle){ box.x + 200, box.y + 110, 100, 35 }, "NO")) showExitConfirm = false;
        } else if (showCancelConfirm) {
            Rectangle box = { (float)sw/2 - 200, (float)sh/2 - 90, 400, 180 };
            DrawRectangleRec(box, RAYWHITE);
            DrawRectangleLinesEx(box, 2, BLACK);
            DrawText("CANCEL CURRENT STEP?", box.x + 70, box.y + 25, 16, BLACK);
            DrawText("This will exit the ongoing insertion/deletion.", box.x + 40, box.y + 70, 12, DARKGRAY);
            if (GuiButton((Rectangle){ box.x + 70, box.y + 110, 110, 35 }, "CANCEL")) {
                if (selectedADT == ADT_LIST) {
                    if (selectedImpl == IMPL_LINKED) LinkedListVisualizer_CancelInteraction();
                    else if (selectedImpl == IMPL_ARRAY) ArrayVisualizer_CancelInteraction();
                }
                else if (selectedADT == ADT_STACK) StackVisualizer_CancelInteraction();
                else if (selectedADT == ADT_QUEUE) QueueVisualizer_CancelInteraction();
                showCancelConfirm = false;
            }
            if (GuiButton((Rectangle){ box.x + 220, box.y + 110, 110, 35 }, "STAY")) showCancelConfirm = false;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
