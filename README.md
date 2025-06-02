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

2. Create an Engine and an Entity

```c++
#include "Engine.h"

Weave::ECS::Engine engine;

Weave::ECS::EntityID e = engine.GetWorld().CreateEntity();
```

3. Add Components

You can add components individually or in batches:

```c++
// Default-constructed components
world.AddComponents<Position, Velocity>(e);

// Or initialized:
world.AddComponents(e, Position{0.f, 0.f}, Velocity{1.f, 1.f});
```

4. Register Systems

First, create the groups these systems will run in. For example, Update, FixedUpdate, or Render.

```c++
Weave::ECS::SystemGroupID updateGroup = engine.CreateSystemGroup();
```

Then register your systems. These can either take component arguments to be ran on each entity, or a World reference to handle iteration manually.

```c++
engine.RegisterSystem<Position, Velocity>(
    updateGroup,
    [](EntityID entity, Position pos, Velocity vel) {
        pos.x += vel.dx;
        pos.y += vel.dy;
    },
    1.0f // A priority float that determines the order of systems in the group can also be entered optionally.
);

engine.RegisterSystem(
    updateGroup,
    [](World& world) {
        for (auto [entity, pos, vel] : world.GetView<Position, Velocity>()) {
            pos.x += vel.dx;
            pos.y += vel.dy;
        }
    }
);
```

Per entity systems can also be easily multithreaded.

```c++
engine.RegisterSystemThreaded<Position, Velocity>(
    updateGroup,
    [](EntityID entity, Position pos, Velocity vel) {
        pos.x += vel.dx;
        pos.y += vel.dy;
    }
);
```

One important thing to note is that changes to entity composition such as adding or removing components should never occur directly within systems. When registering a system, a command buffer can be requested that will delay operations until the system is complete.

```c++
engine.RegisterSystem<Position, Velocity>(
    updateGroup,
    [](EntityID entity, KillTag tag, CommandBuffer& cmd) {
        cmd.AddCommand([](World& world) {
            world.DeleteEntity(entity);
        });
    }
);
```

## 📚 License

MIT License — free for personal, academic, and commercial use.

Enjoy building with Weave ECS!
