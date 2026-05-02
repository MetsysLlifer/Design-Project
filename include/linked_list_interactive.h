#ifndef LINKED_LIST_INTERACTIVE_H
#define LINKED_LIST_INTERACTIVE_H

#include "raylib.h"

typedef enum {
    INTERACTIVE_NONE,
    INTERACTIVE_DELETE_TASK
} InteractiveState;

void LLInteractive_Init(void);
void LLInteractive_Update(void);
void LLInteractive_Draw(void);
void LLInteractive_StartDeleteTask(int targetValue);

#endif
