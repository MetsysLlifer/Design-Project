#include "raylib.h"
#include "raymath.h"
#include "array_visualizer.h"
#include "linked_list_visualizer.h"
#include "graph_visualizer.h"
#include "memory_manager.h"
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef enum { SCENE_MAIN_MENU, SCENE_START, SCENE_VISUALIZER } AppScene;
typedef enum { ADT_LIST, ADT_STACK, ADT_QUEUE } ADTType;
typedef enum { IMPL_ARRAY, IMPL_LINKED, IMPL_CURSOR } ImplType;

AppScene currentScene = SCENE_MAIN_MENU;
ADTType selectedADT = ADT_LIST;
ImplType selectedImpl = IMPL_LINKED;
Camera2D camera = { 0 };
bool showMemoryVis = true;

void InitApp(void) {
    ArrayVisualizer_Init();
    LinkedListVisualizer_Init();
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
    if (selectedADT == ADT_LIST && selectedImpl == IMPL_LINKED) {
        Vector2 screenCenter = { (float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f };
        Vector2 centerWorldPos = GetScreenToWorld2D(screenCenter, camera);
        LinkedListVisualizer_SetSpawnCenter(centerWorldPos);
        LinkedListVisualizer_SetCamera(camera);
        LinkedListVisualizer_Update(mouseWorldPos, camera.zoom);
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
                if (selectedADT == ADT_LIST && selectedImpl == IMPL_LINKED && LinkedListVisualizer_IsBusy()) {
                    showCancelConfirm = true;
                }
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
                DrawText("SELECT DATA STRUCTURE", 50, 50, 30, BLACK);
                if (GuiButton((Rectangle){ 50, 120, 250, 45 }, "LIST (Linked-List)")) { selectedADT = ADT_LIST; selectedImpl = IMPL_LINKED; LinkedListVisualizer_Init(); currentScene = SCENE_VISUALIZER; }
                if (GuiButton((Rectangle){ 50, 600, 150, 40 }, "BACK")) currentScene = SCENE_MAIN_MENU;
                break;

            case SCENE_VISUALIZER:
                BeginMode2D(camera);
                    if (selectedADT == ADT_LIST && selectedImpl == IMPL_LINKED) LinkedListVisualizer_Draw();
                EndMode2D();

                if (selectedADT == ADT_LIST && selectedImpl == IMPL_LINKED) {
                    LinkedListVisualizer_DrawUI();
                }

                if (showMemoryVis) {
                    int travAddr = 0;
                    if (selectedADT == ADT_LIST && selectedImpl == IMPL_LINKED) {
                        travAddr = LinkedListVisualizer_GetTraversalAddress();
                    }
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
                LinkedListVisualizer_CancelInteraction();
                showCancelConfirm = false;
            }
            if (GuiButton((Rectangle){ box.x + 220, box.y + 110, 110, 35 }, "STAY")) showCancelConfirm = false;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
