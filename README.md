# KernelMem — Qt GUI for the memory management backend

A Qt Widgets front end for the algorithms in `backend/`. The backend classes are used exactly as written; the GUI only calls their public functions and reads their public data.

## Tabs

| Tab | Backend class | What you can do |
|---|---|---|
| Allocation | `MemoryManager` | Allocate with First / Best / Worst Fit (the bar shows which hole the selected algorithm will pick before you click), click a process to release it (holes coalesce), compact, see free / largest hole / fragmentation from `getMetrics`. |
| Page replacement | `PageReplacementSim` (FIFO, LRU, Clock) | Step or run a reference string, see each frame over time, victims, hits, Clock reference bits, and a fault comparison of all three algorithms on the same input. |
| Garbage collector | `memoryManger` | Objects with reference arrows and roots. Run **Mark** (shows reachable vs garbage), then **Sweep**, or run the full `garbageCollection()`. Link/unlink references, add/remove roots, free, allocate, compact. Backend `cout` messages appear in the log. |
| Processes | `processManagement` + its own `MemoryManager` | Start processes at a random free spot in memory, record activity, terminate (memory is released), idle cleanup via `inactivityCleanup(minutes)`, compact memory, reset. A terminated process's name can be started again. |

## Build

**Qt Creator:** open `CMakeLists.txt` (or `KernelMem.pro`) and press Run.

**Command line (Qt 6 or Qt 5.15):**

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<compiler>
cmake --build build
./build/KernelMem        # Windows: build\KernelMem.exe
```

On Windows, run `windeployqt build\KernelMem.exe` if you want to launch the exe outside Qt Creator.

## Note on the backend

`MemoryManagementAlgorithm.h` was missing the closing `#endif` of its include guard, so it did not compile. One line was added at the end of that file:

```cpp
#endif // MEMORY_MANAGEMENT_ALGORITHM_H
```

No other backend line was changed.

## Random placement (Processes tab)

The backend only offers First / Best / Worst Fit, so random placement is built from its public API: the GUI picks a random hole that fits and a random offset inside it, fills the earlier holes and the offset with temporary First Fit blocks, allocates the process, then releases the temporary blocks (`release()` coalesces them back). The temporary blocks use up backend PIDs, so the table shows its own PID numbers.

## Layout

```
backend/     your algorithm headers
src/         GUI (Theme, MemoryBar widget, one file per tab, MainWindow, main)
resources/   spin box / combo box arrow icons
```
