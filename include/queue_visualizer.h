#ifndef QUEUE_VISUALIZER_H
#define QUEUE_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

void QueueVisualizer_Init(void);
void QueueVisualizer_Update(Vector2 mouseWorldPos, float zoom);
void QueueVisualizer_Draw(void);
void QueueVisualizer_DrawUI(void);

void QueueVisualizer_SetSpawnCenter(Vector2 center);
void QueueVisualizer_SetCamera(Camera2D cam);
void QueueVisualizer_NextStep(void);
int QueueVisualizer_GetTraversalAddress(void);
bool QueueVisualizer_IsBusy(void);
void QueueVisualizer_CancelInteraction(void);

#endif
