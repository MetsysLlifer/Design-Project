#include "graph_visualizer.h"
#include "memory_manager.h"
#include <stdio.h>
#include <math.h>

#define MAX_NODES 6

static int matrix[MAX_NODES][MAX_NODES];
static int list_heads[MAX_NODES]; // Array of pointers (addresses)
static GraphType currentType = GRAPH_MODE_MATRIX;
static Vector2 nodePositions[MAX_NODES];

void GraphVisualizer_Init(void) {
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) matrix[i][j] = 0;
        list_heads[i] = 0;
        
        float angle = i * (2.0f * PI / MAX_NODES);
        nodePositions[i] = (Vector2){ 550 + 150 * cosf(angle), 250 + 150 * sinf(angle) };
    }
    
    // Default edges for demonstration
    GraphVisualizer_AddEdge(0, 1);
    GraphVisualizer_AddEdge(1, 2);
    GraphVisualizer_AddEdge(2, 0);
    GraphVisualizer_AddEdge(3, 4);
}

void GraphVisualizer_AddEdge(int u, int v) {
    if (u >= MAX_NODES || v >= MAX_NODES) return;
    
    // Matrix Update
    matrix[u][v] = 1;
    matrix[v][u] = 1; // Undirected
    
    // List Update (Simulated Memory)
    int new_v = MemoryManager_Malloc(v);
    MemoryManager_GetNode(new_v)->next_address = list_heads[u];
    list_heads[u] = new_v;
    
    int new_u = MemoryManager_Malloc(u);
    MemoryManager_GetNode(new_u)->next_address = list_heads[v];
    list_heads[v] = new_u;
}

void GraphVisualizer_SetType(GraphType type) {
    currentType = type;
}

void GraphVisualizer_Update(void) {
    // Logic for animations if needed
}

static void DrawMatrixMemory(Rectangle area) {
    float cellSize = area.width / MAX_NODES;
    DrawText("PHYSICAL VIEW: ADJACENCY MATRIX (2D ARRAY)", area.x, area.y - 25, 15, (Color){ 255, 0, 110, 255 });
    
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            Rectangle r = { area.x + j * cellSize, area.y + i * cellSize, cellSize - 2, cellSize - 2 };
            Color borderColor = (matrix[i][j]) ? (Color){ 0, 212, 255, 255 } : (Color){ 20, 20, 40, 255 };
            DrawRectangleLinesEx(r, 1, borderColor);
            if (matrix[i][j]) DrawRectangleRec(r, (Color){ 0, 212, 255, 40 });
            
            char val[4];
            sprintf(val, "%d", matrix[i][j]);
            DrawText(val, r.x + cellSize/2 - 5, r.y + cellSize/2 - 5, 10, (matrix[i][j] ? WHITE : GRAY));
        }
    }
}

void GraphVisualizer_Draw(void) {
    // Logical View
    DrawText("LOGICAL VIEW (GRAPH)", 350, 50, 20, (Color){ 0, 212, 255, 255 });
    
    // Draw Edges
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = i + 1; j < MAX_NODES; j++) {
            if (matrix[i][j]) {
                DrawLineEx(nodePositions[i], nodePositions[j], 2.0f, (Color){ 255, 255, 255, 50 });
            }
        }
    }
    
    // Draw Nodes
    for (int i = 0; i < MAX_NODES; i++) {
        DrawCircleV(nodePositions[i], 20, (Color){ 20, 20, 40, 255 });
        DrawCircleLinesV(nodePositions[i], 20, (Color){ 0, 212, 255, 255 });
        char id[4]; sprintf(id, "%d", i);
        DrawText(id, nodePositions[i].x - 5, nodePositions[i].y - 8, 15, WHITE);
    }

    // Physical Views
    if (currentType == GRAPH_MODE_MATRIX) {
        DrawMatrixMemory((Rectangle){ 850, 100, 300, 300 });
    } else {
        MemoryManager_Draw((Rectangle){ 750, 350, 500, 320 });
        DrawText("PHYSICAL VIEW: ADJACENCY LIST (LINKED LISTS)", 750, 325, 15, (Color){ 255, 0, 110, 255 });
    }
}
