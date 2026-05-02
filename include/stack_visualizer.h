#ifndef STACK_VISUALIZER_H
#define STACK_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

void StackVisualizer_Init(void);
void StackVisualizer_Update(Vector2 mouseWorldPos, float zoom);
void StackVisualizer_Draw(void);
void StackVisualizer_DrawUI(void);

void StackVisualizer_SetSpawnCenter(Vector2 center);
void StackVisualizer_SetCamera(Camera2D cam);
void StackVisualizer_NextStep(void);
int StackVisualizer_GetTraversalAddress(void);
bool StackVisualizer_IsBusy(void);
void StackVisualizer_CancelInteraction(void);

#endif
