#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_MEM_NODES 64

typedef enum {
    MEM_FREE,
    MEM_ALLOCATED,
    MEM_GARBAGE // Deallocated but not yet cleared
} MemoryStatus;

typedef struct {
    int address;        // Simulated hex address (e.g., 0x1000 + index)
    int value;          // Data stored (for simple nodes)
    int next_address;   // Pointer to next node's address
    MemoryStatus status;
    Color highlight;    // For glowing effects
} MemoryNode;

void MemoryManager_Init(void);
int MemoryManager_Malloc(int value);
void MemoryManager_Free(int address);
void MemoryManager_Update(void);
void MemoryManager_Draw(Rectangle area, int travAddress);

MemoryNode* MemoryManager_GetNode(int address);
int MemoryManager_GetAddressByIndex(int index);

#endif
