#ifndef GRAPH_VISUALIZER_H
#define GRAPH_VISUALIZER_H

#include "raylib.h"

typedef enum {
    GRAPH_MODE_MATRIX,
    GRAPH_MODE_LIST
} GraphType;

void GraphVisualizer_Init(void);
void GraphVisualizer_Update(void);
void GraphVisualizer_Draw(void);
void GraphVisualizer_SetType(GraphType type);
void GraphVisualizer_AddEdge(int u, int v);

#endif
