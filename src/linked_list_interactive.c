#include "linked_list_interactive.h"
#include "memory_manager.h"
#include "linked_list_visualizer.h"
#include <stdio.h>

static InteractiveState currentState = INTERACTIVE_NONE;
static int targetVal = 0;
static char instructions[128] = "Select a node to begin interaction.";
static bool isDragging = false;
static int dragSourceAddr = 0;

void LLInteractive_Init(void) {
    currentState = INTERACTIVE_NONE;
    isDragging = false;
}

void LLInteractive_StartDeleteTask(int value) {
    targetVal = value;
    currentState = INTERACTIVE_DELETE_TASK;
    sprintf(instructions, "TASK: Delete Node %d. First, link Predecessor to Successor.", targetVal);
}

void LLInteractive_Update(void) {
    if (currentState == INTERACTIVE_DELETE_TASK) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Check if clicking on a memory cell's "next" field
            // (Simplification for now: clicking the address area in Physical View)
            Vector2 m = GetMousePosition();
            // Logic to detect which cell is clicked...
            // For now, let's simulate the error state if user tries to free prematurely
        }
    }
}

void LLInteractive_Draw(void) {
    DrawRectangleRec((Rectangle){ 320, 550, 940, 150 }, (Color){ 10, 10, 30, 200 });
    DrawRectangleLinesEx((Rectangle){ 320, 550, 940, 150 }, 2, (Color){ 255, 0, 110, 255 });
    
    DrawText("INTERACTIVE TUTORIAL", 340, 565, 15, (Color){ 255, 0, 110, 255 });
    DrawText(instructions, 340, 600, 20, WHITE);
    
    if (isDragging) {
        DrawLineEx(GetMousePosition(), (Vector2){500, 500}, 2.0f, YELLOW);
    }
}
