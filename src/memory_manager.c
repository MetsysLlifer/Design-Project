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

void MemoryManager_Draw(Rectangle area, int travAddress) {
    float cellHeight = 25.0f;
    const int fontSize = 11;
    int visibleNodes = (int)(area.height / cellHeight);
    
    // HEAP THEME (GREEN)
    DrawRectangleRec(area, (Color){ 245, 255, 245, 255 });
    DrawRectangleLinesEx(area, 2, DARKGREEN);
    DrawText("THE HEAP (Dynamic Memory)", area.x, area.y - 20, 14, DARKGREEN);

    for (int i = 0; i < visibleNodes && i < MAX_MEM_NODES; i++) {
        Rectangle cell = { area.x, area.y + i * cellHeight, area.width, cellHeight };
        DrawRectangleLinesEx(cell, 1, (Color){ 0, 100, 0, 50 });

        float textX = cell.x + 5;
        float textY = cell.y + (cellHeight - (float)fontSize) / 2.0f;
        
        if (travAddress != 0 && heap[i].address == travAddress) {
            DrawText("trav ->", cell.x - 45, textY, fontSize, (Color){ 255, 0, 110, 255 });
        }
        
        if (heap[i].status == MEM_ALLOCATED) {
            DrawRectangleRec(cell, (Color){ 0, 255, 0, 20 }); // Light green tint
            char text[48];
            sprintf(text, "0x%X: [%d | 0x%X]", heap[i].address, heap[i].value, heap[i].next_address);
            DrawText(text, textX, textY, fontSize, DARKGREEN);
        } else {
            char addr[16];
            sprintf(addr, "0x%X", heap[i].address);
            DrawText(addr, textX, textY, fontSize, (Color){ 180, 210, 180, 255 });
        }
    }
}

MemoryNode* MemoryManager_GetNode(int address) {
    for (int i = 0; i < MAX_MEM_NODES; i++) {
        if (heap[i].address == address) return &heap[i];
    }
    return NULL;
}
