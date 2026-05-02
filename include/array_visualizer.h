#ifndef ARRAY_VISUALIZER_H
#define ARRAY_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

void ArrayVisualizer_Init(void);
void ArrayVisualizer_Update(void);
void ArrayVisualizer_Draw(Rectangle area);
void ArrayVisualizer_DrawExplanation(Rectangle area);
void ArrayVisualizer_StartSorting(void);

// Playback controls
void ArrayVisualizer_TogglePause(void);
void ArrayVisualizer_SetSpeed(float newSpeed);
float ArrayVisualizer_GetSpeed(void);
bool ArrayVisualizer_IsPaused(void);
void ArrayVisualizer_StepForward(void);
void ArrayVisualizer_StepBackward(void);

#endif
