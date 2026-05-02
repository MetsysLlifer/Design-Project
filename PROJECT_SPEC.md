# Futuristic DSA Visualizer - Comprehensive Project Specification

## 1. Project Overview & Educational Goals
This project is an interactive, deeply educational visualizer for Data Structures and Algorithms (DSA), implemented in **C** using **Raylib** and **Raygui**. It is specifically designed for students who have basic programming knowledge but need hands-on, visceral experience to master **pointers, dynamic memory allocation, and underlying C implementations**.

Unlike standard internet visualizers that only show logical shapes (like circles and arrows), this app bridges the gap between logical representation and physical memory. It focuses heavily on the structural differences between implementations (Arrays vs. Pointers vs. Cursor-based memory simulations).

## 2. Core Reference Material
- **Primary Reference**: `book/DSA.pdf`
- All pseudocode, implementation logic (e.g., ADT behaviors), and structural rules must align strictly with the concepts taught in this reference book.

## 3. Supported ADTs (Abstract Data Types)
The visualizer will comprehensively cover the following ADTs:
1. **List**
2. **Stack**
3. **Queue**
4. **Dictionary**
5. **Computer Word** (Bitwise operations and representations)
6. **Graphs and Trees**
7. *Other ADTs defined in `DSA.pdf`*

## 4. Implementation Methods (The "How")
A core feature of this project is showing that one ADT can be implemented in multiple ways. Each ADT will be visualizable using up to three different underlying memory representations:
1. **Array-Based (Contiguous Memory)**: Shows a standard contiguous block of elements.
2. **Linked-List (Dynamic Pointers)**: Shows nodes scattered in a simulated heap, connected by pointer addresses.
3. **Cursor-Based (Simulated Dynamic Memory within an Array)**: Shows how an array can simulate a heap (using an `avail` list) in environments lacking dynamic allocation.

## 5. Dual-View Visualization: Logical vs. Physical Memory
To teach pointers and memory manipulation effectively, the screen will constantly display two views:
- **Logical View**: The high-level abstraction (e.g., a tree hierarchy, or linked nodes connected by glowing arrows).
- **Physical Memory View**: A literal grid representing the RAM/Heap or the underlying Array. 
  - Instead of abstract dots, the user sees raw memory addresses (e.g., `0x10A4`), values stored at those addresses, and next/prev pointer values explicitly written in the memory cells.
  - When memory is `malloc`'d, a cell in the physical view lights up as "ALLOCATED". When `free`'d, it greys out as "GARBAGE".

## 6. The Three Learning Modes
For every ADT and algorithm, the user selects from three interactive modes:

### Mode 1: Show Simulation (Watch)
- A smooth, animated logical and physical representation of the algorithm running.
- Includes granular playback controls:
  - **Play/Pause**
  - **Speed Slider** (Dynamic control over animation speed)
  - **Step Forward**
  - **Step Backward (Rollback state using Action History)**

### Mode 2: Simulation + Pseudocode (Learn)
- The screen is divided to show the visual simulation (Logical + Memory views) alongside actual **C Code / Pseudocode**.
- As the simulation progresses, the currently executing line of code is highlighted in neon.
- Real-time explanations populate a "Learning Log", explaining *why* a pointer changed value or *why* memory was allocated/freed, directly referencing the highlighted C code.

### Mode 3: Interactive Simulation (Play/Test/Gamified)
- The ultimate learning tool. The user takes control and must manually perform the algorithm step-by-step.
- **Example Scenario: Deleting a node in a Linked-List**
  1. The game prompts: *"Delete Node containing value '42'"*.
  2. The user must click physical memory cells or logical nodes to **"Traverse"** the list, finding the predecessor.
  3. The user must manually click the predecessor node's `next` pointer field and type or drag it to the successor node's address to **"Link"** them.
  4. The user must click the target node and select the **`free()`** command.
- **Dynamic Feedback & Error Handling**:
  - **Screen Shake Effect**: If the user makes an invalid memory move (e.g., calling `free()` on a node before relinking its predecessor, thus causing a memory leak or dangling pointer), the screen will violently shake.
  - A glowing red error message will display exactly what C-level mistake was made (e.g., *"SEGMENTATION FAULT: You lost the reference to the rest of the list!"*).
  - The state then automatically rolls back to the last valid step so the user can try again.

## 7. Architecture & Implementation Guidelines
To achieve this without real memory leaks, the C architecture follows these principles:
- **State Machine Pattern**: The main loop delegates strictly to the active module/ADT.
- **Simulated Memory Manager**: We will write a custom memory simulator (an array acting as a fake heap) so we can deterministically track "addresses" (indices) and detect memory leaks during Mode 3 without actually crashing the Raylib app.
- **Action History (Command Pattern)**: To support the "Rollback" and "Step Backward" features, all data mutations generate an `Action` struct stored in a history stack.
- **Gamified UI & Effects**: 
  - Raygui with a custom neon/futuristic theme.
  - Special visual functions: `ScreenShake()`, `DrawPhysicalMemoryGrid()`, `DrawNeonPointer()`, `HighlightCodeLine()`.

## 8. Usage Instructions for CLI Agent
When a user asks to continue building this project, use this document as the absolute source of truth. Prioritize:
1. Implementing the simulated memory grid (Physical Memory View).
2. Adding specific ADTs (List, Stack, Queue) mapped from `DSA.pdf`.
3. Creating the interactive "drag-and-drop" or "address typing" mechanics for Mode 3.
4. Implementing the screen-shake failure states.