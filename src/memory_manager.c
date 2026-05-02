#include "memory_manager.h"
#include <stdio.h>
#include <string.h>

static MemoryNode heap[MAX_MEM_NODES];
static const int base_address = 0x1000;

void MemoryManager_Init(void) {
    memset(heap, 0, sizeof(heap));
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        heap[i].address = base_address + (i * 16);
        heap[i].status = MEM_FREE;
        heap[i].highlight = BLANK;
    }
}

int MemoryManager_Malloc(int value) {
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        if (heap[i].status == MEM_FREE || heap[i].status == MEM_GARBAGE) {
            heap[i].status = MEM_ALLOCATED;
            heap[i].value = value;
            heap[i].next_address = 0;
            heap[i].highlight = (Color){ 0, 0, 0, 40 };
            return heap[i].address;
        }
    }
    return -1;
}

void MemoryManager_Free(int address) {
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        if (heap[i].address == address) {
            heap[i].status = MEM_GARBAGE;
            return;
        }
    }
}

void MemoryManager_Update(void) {
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        if (heap[i].highlight.a > 0) heap[i].highlight.a -= 5;
    }
}

void MemoryManager_Draw(Rectangle area) {
    float cellHeight = 25.0f;
    int visibleNodes = (int)(area.height / cellHeight);
    
    DrawRectangleRec(area, RAYWHITE);
    DrawRectangleLinesEx(area, 2, BLACK);

    for (int i = 0; i < visibleNodes && i < MAX_MEM_NODES; i++) {
        Rectangle cell = { area.x, area.y + i * cellHeight, area.width, cellHeight };
        DrawRectangleLinesEx(cell, 1, BLACK);
        
        if (heap[i].status == MEM_ALLOCATED) {
            DrawRectangleRec(cell, (Color){ 0, 0, 0, 10 });
            char text[32];
            sprintf(text, "0x%X: [%c | 0x%X]", heap[i].address, (char)heap[i].value, heap[i].next_address);
            DrawText(text, cell.x + 10, cell.y + 6, 11, BLACK);
        } else {
            char addr[16];
            sprintf(addr, "0x%X", heap[i].address);
            DrawText(addr, cell.x + 10, cell.y + 6, 11, LIGHTGRAY);
        }
    }
}

MemoryNode* MemoryManager_GetNode(int address) {
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        if (heap[i].address == address) return &heap[i];
    }
    return NULL;
}
