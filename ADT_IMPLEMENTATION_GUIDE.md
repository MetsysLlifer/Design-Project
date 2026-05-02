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

## 4. Information Display Standards
To ensure clarity, information must be displayed using the following patterns:

### A. Node/Element Anatomy
- **Standard Dimensions**: 100x50 pixels for linked nodes, 55x55 for array elements.
- **Field Dividers**: Nodes must have a vertical line separating the `data` and `next` fields.
- **Labels**: Every node must display its physical memory address (e.g., `0x1004`) above the box in `DARKGREEN`.
- **Contrast**: Use `WHITE` text for data inside colored nodes and `DARKGREEN` for labels on light backgrounds.

### B. Pointer & Data Referencing
- **Stack-to-Heap**: Use **Straight Arrows** (Solid Black) to show a stack pointer (e.g., `List`) referencing a heap object.
- **Heap-to-Heap**: Use **Curved Arrows** (Quadratic Bezier) for internal connections (e.g., `node->next`).
- **NULL Representation**: Point to a dedicated dark-grey box labeled "NULL" rather than vanishing.
- **Arrowheads**: Must be large and robust (triangle based) to ensure directionality is unmistakable even during movement.

### C. Communication (Popups)
- **Notifications**: Use for automatic logic adjustments (e.g., index clamping). Requires a "PROCEED" button.
- **Logic Errors**: Use for invalid user input (e.g., "Array Full"). Requires an "OK" button.
- **Styling**: Popups must be centered, have a thick `4px` border, and dim the background elements slightly.

## 5. Animation Principles
- **Smooth Lerping**: Objects should not jump instantly. Use `position += (target - position) * 0.15f`.
- **Line Growth**: Connecting arrows should use `lineProgress` (0.0 to 1.0) to "grow" from the source to the target.
- **Spawn Effect**: New nodes should slide into the view (e.g., spawn at `y - 100` and lerp to target).
- **Physical Shifting**: In array implementations, elements should physically slide left/right during insertion/deletion to emphasize the $O(n)$ cost.

## 6. Interaction Feedback
- **Dragging**: Highlight the node with a slightly larger border or "glow" when picked up.
- **Execution Path**: A blue arrow must point from the CPU "PC" address directly to the currently active line in the algorithm panel.
- **PC Counter**: The Program Counter (PC) should increment by 4 for every line of pseudocode to simulate real instruction fetching.

## 7. Layout Requirements
- **Top Left**: Back button and "THE EXECUTOR" (CPU) panel.
- **Left Center**: "THE STACK" panel containing pointers and local variables.
- **Top Right (Left of Memory)**: "ALGORITHM" (Pseudocode) panel.
- **Far Right**: "THE HEAP" (Memory Manager) panel.
- **Bottom**: Function control buttons (e.g., Insert, Delete, Push, Pop).

## 8. Integration Step
After creating the source files:
1. Add the headers to `src/main.c`.
2. Add the ADT to `ADTType` enum.
3. Update `SCENE_START` in `main.c` to add a menu button.
4. Update `SCENE_VISUALIZER` logic to route `Update`, `Draw`, and `DrawUI` calls to your new module.
5. Add the source file to the `Makefile`.
