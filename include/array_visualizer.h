#ifndef ARRAY_VISUALIZER_H
#define ARRAY_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    ARR_V1, // Standard Static
    ARR_V2, // Heap Structure (Static)
    ARR_V3, // Dynamic Array
    ARR_V4  // Fully Dynamic
} ArrayVersion;

void ArrayVisualizer_Init(ArrayVersion version);
void ArrayVisualizer_Update(Vector2 mouseWorldPos, float zoom);
void ArrayVisualizer_Draw(void);
void ArrayVisualizer_DrawUI(void);

void ArrayVisualizer_SetSpawnCenter(Vector2 center);
void ArrayVisualizer_SetCamera(Camera2D cam);
void ArrayVisualizer_NextStep(void);
bool ArrayVisualizer_IsBusy(void);
void ArrayVisualizer_CancelInteraction(void);

#endif
