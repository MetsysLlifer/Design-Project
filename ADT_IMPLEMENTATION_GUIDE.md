# ADT Implementation Guide

This document outlines the standard structure and visual conventions for adding new Abstract Data Types (ADTs) to the DSA Visualizer.

## 1. File Structure
For a new ADT (e.g., `BinaryTree`), create:
- `include/binary_tree_visualizer.h`
- `src/binary_tree_visualizer.c`

## 2. Interface Standards
Every visualizer must implement at least the following functions:
- `Init()`: Reset all state, clear memory, and initialize pointers.
- `Update(Vector2 mouseWorldPos, float zoom)`: Handle animations (lerping) and interaction (dragging).
- `Draw()`: Render world-space objects (Nodes on the Heap).
- `DrawUI()`: Render screen-space panels (CPU, Stack, Pseudocode, Controls).
- `NextStep()`: Advance the state machine by one line of pseudocode.
- `CancelInteraction()`: Stop current operation and clean up temporary memory.
- `IsBusy()`: Return true if an operation is currently executing.

## 3. Visual & Color Conventions
We use a standardized "Mental Model" for memory visualization:

| Component | Color Theme | Description |
|-----------|-------------|-------------|
| **CPU / Execution** | **Cool Blue** | The "Executor" box and PC (Program Counter). |
| **Instruction Set** | **Cool Blue** | The Pseudocode panel (High-contrast blue/white). |
| **The Stack** | **Warm Orange** | Pointers (e.g., `head`, `top`, `front`) and static variables. |
| **The Heap** | **Dynamic Green** | Allocated objects (Nodes, Arrays) and Memory Manager. |
| **Traversal** | **Pink/Magenta** | The `curr` or `traverse` pointer. |
| **Highlighting** | **Yellow/Red/Green** | Use Yellow for current focus, Red for deletion, Green for new allocation. |

## 4. Animation Principles
- **Smooth Lerping**: Objects should not jump instantly. Use `position += (target - position) * 0.15f`.
- **Line Growth**: Connecting arrows should use `lineProgress` (0.0 to 1.0) to "grow" from the source to the target.
- **Spawn Effect**: New nodes should slide into the view (e.g., spawn at `y - 100` and lerp to target).

## 5. Pseudocode Integration
- Define an array of strings representing the algorithm steps.
- Use a `currentLine` counter in your `SimContext`.
- In `NextStep()`, use a `switch(currentLine)` to apply logic and advance `currentLine`.
- Always reset `lineProgress = 0.0f` when a pointer changes to trigger the growth animation.

## 6. Layout Requirements
- **Top Left**: Back button and "THE EXECUTOR" (CPU) panel.
- **Left Center**: "THE STACK" panel containing pointers and local variables.
- **Top Right (Left of Memory)**: "ALGORITHM" (Pseudocode) panel.
- **Far Right**: "THE HEAP" (Memory Manager) panel.
- **Bottom**: Function control buttons (e.g., Insert, Delete, Push, Pop).

## 7. Integration Step
After creating the source files:
1. Add the headers to `src/main.c`.
2. Add the ADT to `ADTType` enum.
3. Update `SCENE_START` in `main.c` to add a menu button.
4. Update `SCENE_VISUALIZER` logic to route `Update`, `Draw`, and `DrawUI` calls to your new module.
5. Add the source file to the `Makefile`.
