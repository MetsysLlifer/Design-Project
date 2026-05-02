#ifndef LINKED_LIST_VISUALIZER_H
#define LINKED_LIST_VISUALIZER_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    SIM_IDLE,
    SIM_SELECT_INSERT,
    SIM_SELECT_DELETE,
    SIM_INPUT_PARAMS,
    SIM_EXECUTING,
    SIM_ERROR
} SimStatus;

typedef enum {
    FUNC_NONE,
    FUNC_INSERT,
    FUNC_DELETE
} ActiveFunc;

typedef enum {
    INSERT_FIRST,
    INSERT_LAST,
    INSERT_INDEX
} InsertMode;

typedef enum {
    DELETE_FIRST,
    DELETE_LAST,
    DELETE_INDEX,
    DELETE_ELEMENT
} DeleteMode;

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
    InsertMode insertMode;
    DeleteMode deleteMode;
    int prevAddress;
    int toDeleteAddress;
    int newNodeAddress;
    int currentLine;
    int totalLines;
} SimContext;

void LinkedListVisualizer_Init(void);
void LinkedListVisualizer_Update(Vector2 mouseWorldPos, float zoom);
void LinkedListVisualizer_Draw(void);
void LinkedListVisualizer_DrawUI(void);

void LinkedListVisualizer_SetSpawnCenter(Vector2 center);
void LinkedListVisualizer_SetCamera(Camera2D cam);
void LinkedListVisualizer_StartInsert(void);
void LinkedListVisualizer_TraverseStep(void);
void LinkedListVisualizer_NextStep(void);
void LinkedListVisualizer_CenterCamera(Camera2D *cam);
void LinkedListVisualizer_AddNodeAt(char value, Vector2 worldPos);
void LinkedListVisualizer_DeleteActive(void);
int LinkedListVisualizer_GetTraversalAddress(void);
bool LinkedListVisualizer_IsBusy(void);
void LinkedListVisualizer_CancelInteraction(void);

#endif
