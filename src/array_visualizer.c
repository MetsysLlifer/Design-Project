#include "array_visualizer.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define ARRAY_SIZE 30
#define HISTORY_MAX 5000

#define COLOR_NEON_BLUE (Color){ 0, 212, 255, 255 }
#define COLOR_NEON_PINK (Color){ 255, 0, 110, 255 }
#define COLOR_GLOW      (Color){ 0, 212, 255, 50 }

typedef struct {
    int data[ARRAY_SIZE];
    int currentIndex;
    int compareIndex;
    char explanation[128];
} SortState;

static SortState history[HISTORY_MAX];
static int currentStep = 0;
static int maxStep = 0;

static bool sorting = false;
static bool paused = false;
static float timer = 0.0f;
static float speed = 0.1f;

static void SaveState(const char* explanation) {
    if (maxStep >= HISTORY_MAX - 1) return;
    maxStep++;
    currentStep = maxStep;
    
    // Copy data from previous step
    memcpy(history[currentStep].data, history[currentStep-1].data, sizeof(int) * ARRAY_SIZE);
    history[currentStep].currentIndex = history[currentStep-1].currentIndex;
    history[currentStep].compareIndex = history[currentStep-1].compareIndex;
    strncpy(history[currentStep].explanation, explanation, 127);
    history[currentStep].explanation[127] = '\0';
}

void ArrayVisualizer_Init(void) {
    srand(time(NULL));
    currentStep = 0;
    maxStep = 0;
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        history[0].data[i] = GetRandomValue(20, 300);
    }
    history[0].currentIndex = -1;
    history[0].compareIndex = -1;
    strncpy(history[0].explanation, "Click 'Start Sort' to begin the Bubble Sort algorithm.", 127);
    
    sorting = false;
    paused = false;
}

// Logic step, completely separated from time/drawing
static void ComputeNextStep(void) {
    if (history[currentStep].currentIndex >= ARRAY_SIZE - 1) {
        sorting = false;
        SaveState("Sorting complete! The array is now ordered.");
        return;
    }

    if (history[currentStep].currentIndex == -1) {
        history[currentStep].currentIndex = 0;
        history[currentStep].compareIndex = 0;
    }

    int curIdx = history[currentStep].currentIndex;
    int cmpIdx = history[currentStep].compareIndex;
    
    SaveState("Comparing elements...");
    // Modify the new state (which is currently a copy of the previous)
    history[currentStep].currentIndex = curIdx;
    history[currentStep].compareIndex = cmpIdx;
    
    if (history[currentStep].data[curIdx] > history[currentStep].data[cmpIdx]) {
        int temp = history[currentStep].data[curIdx];
        history[currentStep].data[curIdx] = history[currentStep].data[cmpIdx];
        history[currentStep].data[cmpIdx] = temp;
        strncpy(history[currentStep].explanation, "Swapping elements: Current is larger than next.", 127);
    } else {
        strncpy(history[currentStep].explanation, "No swap needed: Elements are in order.", 127);
    }

    // Advance indices for next step
    cmpIdx++;
    if (cmpIdx >= ARRAY_SIZE - curIdx) {
        curIdx++;
        cmpIdx = 0;
    }
    
    // Actually applying the advanced indices to the current state so the NEXT step compares correctly
    // But wait, the state should reflect what was JUST done. 
    // We will advance the indices for the next iteration by saving another small transition state or just updating the next index.
    // To keep it simple, we just update the indices in current step so the next step picks them up.
    history[currentStep].currentIndex = curIdx;
    history[currentStep].compareIndex = cmpIdx;
}

void ArrayVisualizer_Update(void) {
    if (sorting && !paused) {
        timer += GetFrameTime();
        if (timer >= speed) {
            timer = 0.0f;
            
            // If we are playing and not at max step, just move forward
            if (currentStep < maxStep) {
                currentStep++;
            } else {
                ComputeNextStep();
            }
        }
    }
}

void ArrayVisualizer_Draw(Rectangle area) {
    float barWidth = area.width / ARRAY_SIZE;
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        float barHeight = history[currentStep].data[i];
        Rectangle bar = {
            area.x + i * barWidth + 2,
            area.y + area.height - barHeight,
            barWidth - 4,
            barHeight
        };

        Color color = COLOR_NEON_BLUE;
        if (i == history[currentStep].currentIndex || i == history[currentStep].compareIndex) {
            if (i != -1) {
                color = COLOR_NEON_PINK;
                DrawRectangleRec((Rectangle){ bar.x - 2, bar.y - 2, bar.width + 4, bar.height + 4 }, COLOR_GLOW);
            }
        }

        DrawRectangleRec(bar, color);
        DrawRectangle(bar.x, bar.y, bar.width, 2, WHITE);
    }
}

void ArrayVisualizer_DrawExplanation(Rectangle area) {
    DrawText("BUBBLE SORT", area.x + 20, area.y + 15, 15, COLOR_NEON_PINK);
    DrawText(history[currentStep].explanation, area.x + 20, area.y + 50, 20, WHITE);
    
    char stepInfo[64];
    sprintf(stepInfo, "Step: %d / %d", currentStep, maxStep);
    DrawText(stepInfo, area.x + 20, area.y + 80, 15, COLOR_NEON_BLUE);

    DrawText("Complexity: O(n^2)", area.x + 20, area.y + 100, 15, GRAY);
}

void ArrayVisualizer_StartSorting(void) {
    if (maxStep == 0) {
        sorting = true;
        paused = false;
        history[0].currentIndex = 0;
        history[0].compareIndex = 1;
    } else {
        // Resume sorting if it was stopped
        sorting = true;
        paused = false;
    }
}

void ArrayVisualizer_TogglePause(void) {
    paused = !paused;
}

void ArrayVisualizer_SetSpeed(float newSpeed) {
    speed = newSpeed;
    if (speed < 0.01f) speed = 0.01f;
    if (speed > 1.0f) speed = 1.0f;
}

float ArrayVisualizer_GetSpeed(void) {
    return speed;
}

bool ArrayVisualizer_IsPaused(void) {
    return paused;
}

void ArrayVisualizer_StepForward(void) {
    paused = true;
    if (currentStep < maxStep) {
        currentStep++;
    } else if (sorting) {
        ComputeNextStep();
    }
}

void ArrayVisualizer_StepBackward(void) {
    paused = true;
    if (currentStep > 0) {
        currentStep--;
    }
}
