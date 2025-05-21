# Weave ECS Library

Weave ECS is a modular, high-performance **Entity Component System** that supports two interchangeable backends:

- **Sparse Set ECS** — Stores each component type independently using sparse sets, offering fast direct access and low memory overhead for systems that operate on a few components at a time. Ideal for random access and write-heavy workloads.
- **Archetype ECS** — Groups entities by shared component types into tightly packed arrays (archetypes), enabling very fast iteration and excellent cache performance when processing many components together. Best suited for iteration-heavy, read-optimized workloads like render or physics systems.

Switching between backends is done at **build time via CMake**, so you can experiment, compare, or tailor the ECS system to your game's architecture without changing your code.

## ⚙️ Getting Started

### 🔨 Requirements

- C++20 or later
- CMake 3.12+

## 🛠️ Building the Library

Weave ECS builds as a **static library**. By default, it uses the **Archetype** backend.

### 🧩 Selecting the ECS Backend

You can switch between the `Archetype` and `SparseSet` backends using the `ECS_BACKEND` CMake option.

```bash
# Default (Archetype)
cmake -S . -B build

# Use SparseSet backend
cmake -S . -B build -DECS_BACKEND=SparseSet

# Then build:
cmake --build build
```

## 🧪 Usage Example
1. Define Components

```c++
struct Position { float x, y; };
struct Velocity { float dx, dy; };
```

2. Create a World and Entity

```c++
#include "World.h"

Weave::ECS::World world;

EntityID e = world.CreateEntity();
```

3. Add Components

You can add components individually or in batches:

```c++
// Default-constructed components
world.AddComponents<Position, Velocity>(e);

// Or initialized:
world.AddComponents(e, Position{0.f, 0.f}, Velocity{1.f, 1.f});
```

4. Iterate with GetView

Use GetView<T...>() to iterate over all entities with specific components:

```c++
for (auto [entity, pos, vel] : world.GetView<Position, Velocity>()) {
    pos.x += vel.dx;
    pos.y += vel.dy;
}
```

5. Remove Components

```c++
world.RemoveComponents<Velocity>(e);
```

## 📚 License

MIT License — free for personal, academic, and commercial use.

Enjoy building with Weave ECS!