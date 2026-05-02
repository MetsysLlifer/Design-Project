#ifndef LINKED_LIST_VISUALIZER_H
#define LINKED_LIST_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    SIM_IDLE,
    SIM_INPUT_PARAMS,
    SIM_EXECUTING,
    SIM_ERROR
} SimStatus;

typedef enum {
    FUNC_NONE,
    FUNC_INSERT,
    FUNC_DELETE
} ActiveFunc;

typedef struct {
    int address;
    Vector2 position;
    bool isDragging;
    char data;
} VisualNode;

typedef struct {
    ActiveFunc type;
    int targetVal;
    int targetPos;
    int currentPos;
} SimContext;

void LinkedListVisualizer_Init(void);
void LinkedListVisualizer_Update(Vector2 mouseWorldPos, float zoom);
void LinkedListVisualizer_Draw(void);
void LinkedListVisualizer_DrawUI(void);

void LinkedListVisualizer_StartInsert(void);
void LinkedListVisualizer_TraverseStep(void);
void LinkedListVisualizer_ExecuteStep(void);
void LinkedListVisualizer_CenterCamera(Camera2D *cam);
void LinkedListVisualizer_AddNodeAt(char value, Vector2 worldPos);
void LinkedListVisualizer_DeleteActive(void);

#endif
