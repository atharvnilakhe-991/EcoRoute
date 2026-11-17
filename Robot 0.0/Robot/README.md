# 🤖 Robot Warehouse Management & Automation System
### Modern C++17 Object-Oriented Simulation Engine & Cognify Intelligence Dashboard

A production-grade, software-based Robot Warehouse Management and Automation System engineered in modern C++ (C++17) utilizing rigorous Object-Oriented Programming (OOP) paradigms, advanced Data Structures and Algorithms (BFS, Dijkstra, A*, Space-Time A*), multi-agent coordination, energy-aware routing (EcoRoute), and perishable inventory optimization (FoodSaver). The backend is paired with a native, zero-dependency C++ HTTP REST server (WinSock2) and an interactive, responsive Cognify-inspired light web dashboard.

---

## 📌 Project Overview

In contemporary automated fulfillment centers, autonomous mobile robots (AMRs) must coordinate in shared physical spaces to fulfill orders, minimize energy expenditure, avoid collisions and deadlocks, and minimize waste of perishable inventory.

This project implements an end-to-end, discrete-time warehouse simulation platform featuring:
- **Scalable 15×15 Grid Warehouse**: Configurable storage shelves, multi-bay charging stations, packaging stations, and dynamic runtime obstacles.
- **Heterogeneous Robot Fleet**: Polymorphic robot classes (`PickerRobot`, `TransportRobot`, `HybridRobot`) with specialized payload capacities, movement kinematics, and differential battery drain profiles.
- **Intelligent Multi-Robot Parallel Scheduling**: Concurrent dispatching of multiple customer orders across available fleet units with dynamic reassignment, payload matching, and round-trip battery validation.
- **Space-Time Multi-Agent Coordination**: Time-expanded reservation table (`ReservationTable`) preventing vertex collisions and head-on edge swaps with cascade revocation and deadlock recovery.
- **DSA Algorithm Laboratory**: Interactive execution and comparison of Breadth-First Search (BFS), Dijkstra's Algorithm, and A* Search with real-time step-by-step exploration visualization and performance telemetry.
- **EcoRoute Energy-Aware Routing**: Physics-grounded energy modeling incorporating kinetic acceleration, payload mass resistance, angular turn penalties, and traffic congestion penalties to discover energy-optimal paths.
- **FoodSaver Perishable Management**: FEFO (First-Expired, First-Out) stock allocation, shelf-life risk scoring, non-profit donation lifecycle state machine, and environmental impact tracking (CO₂ emissions prevented, water conserved).
- **Zero-Dependency Native REST API**: Native WinSock2 HTTP server exposing a complete REST API with JSON serialization and static asset delivery.
- **Dual-Mode Web Dashboard**: Responsive light-theme frontend operating in live C++ backend mode or standalone offline emulation mode.

> [!NOTE]
> **Project Scope Notice**: Phase 5 (SafeMove — Human-Aware Robot Safety Simulation) was intentionally excluded from this release and is slated for future development cycles. All completed components (Phases 1, 2, 3, and 4) are fully implemented, compiled, and verified through a 36-suite regression test harness.

---

## 🏗️ System Architecture

The codebase strictly enforces the **Separation of Concerns (SoC)** principle, modularized into distinct, decoupled subsystems:

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                   WEB FRONTEND / COGNIFY DASHBOARD                       │
│  (HTML5, Modern CSS3 Light Theme, ES6 JavaScript, High-DPI HTML5 Canvas) │
│   - Overview Deck     - Warehouse Grid Canvas   - Fleet Monitor          │
│   - Order Hub         - Inventory Matrix        - DSA Algorithm Lab      │
│   - EcoRoute Studio   - FoodSaver Portal        - Live Telemetry Stream  │
└────────────────────────────────────┬─────────────────────────────────────┘
                                     │ HTTP / REST API (JSON)
                                     ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                  NATIVE C++ REST API SERVER & ROUTER                     │
│               (WinSock2 Native HTTP Server, Port 8080)                   │
│   - Static Asset Traversal Protection   - Endpoint Parameter Parsing     │
│   - JSON Request Validation             - Thread-Safe Mutex Dispatch     │
└────────────────────────────────────┬─────────────────────────────────────┘
                                     │
                                     ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                CENTRAL FACADE ORCHESTRATOR (WarehouseSystem)             │
│   - Discrete Simulation Clock (Ticks)   - Dynamic Obstacle Coordinator   │
│   - Configuration Ingestion (JSON)      - Subsystem Life-Cycle Sync      │
└──────────┬─────────────────┬─────────────────┬─────────────────┬─────────┘
           │                 │                 │                 │
           ▼                 ▼                 ▼                 ▼
┌──────────────────┐┌─────────────────┐┌────────────────┐┌────────────────┐
│   TASK MANAGER   ││ COORDINATOR &   ││   ECOROUTE     ││   FOODSAVER    │
│ & SCHEDULER      ││ RESERVATION     ││ OPTIMIZER      ││ PERISHABLE HUB │
├──────────────────┤├─────────────────┤├────────────────┤├────────────────┤
│ - Parallel Order ││ - Reservation   ││ - Energy Model ││ - FEFO Batch   │
│   Dispatch       ││   Table (X,Y,t) ││   (Motion/Turn/││   Tracking     │
│ - Battery Check  ││ - Conflict &    ││    Mass/Heat)  ││ - Risk Scoring │
│ - Payload Match  ││   Swap Detector ││ - Dynamic Edge ││ - Donation FSM │
│ - Team Dispatch  ││ - Deadlock ST-A*││   Cost Weighting││ - Eco Impact   │
└──────────┬───────┘└────────┬────────┘└────────┬───────┘└────────┬───────┘
           │                 │                  │                 │
           └─────────────────┼──────────────────┴─────────────────┘
                             ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                      ALGORITHM & DOMAIN CORE                             │
│   - PathFinder (BFS, Dijkstra, A*, Space-Time A*)                        │
│   - Polymorphic Fleet: Robot Base -> PickerRobot, TransportRobot, Hybrid │
│   - Physical Warehouse Grid: Obstacles, Walls, Corridors, Cell Weights   │
│   - Facility Stations: ChargingStation, PackingStation, Staging Bays    │
│   - Inventory & Batch Management: Products, Stock Invariants, Batches    │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 Repository Directory Structure

```text
Robot/
│
├── core/                               // C++17 Core Simulation Engine
│   ├── include/
│   │   ├── Position.h                  // 2D grid coordinates, distance metrics & equality
│   │   ├── Product.h                   // Warehouse stock item definition & metadata
│   │   ├── Inventory.h                 // Stock repository, locking & invariant enforcement
│   │   ├── Robot.h                     // Abstract polymorphic Robot base class
│   │   ├── PickerRobot.h               // High-speed, lightweight picking robot (cap: 6)
│   │   ├── TransportRobot.h            // High-capacity heavy hauler robot (cap: 25)
│   │   ├── HybridRobot.h               // General-purpose balanced robot (cap: 12)
│   │   ├── Shelf.h                     // Storage locations for physical goods
│   │   ├── Station.h                   // Base class for physical warehouse stations
│   │   ├── ChargingStation.h           // Multi-bay automated recharging docks
│   │   ├── PackingStation.h            // Order fulfillment, consolidation & packing bays
│   │   ├── Order.h                     // Customer request model & priority enumerations
│   │   ├── Task.h                      // Atomic subtask lifecycle representation
│   │   ├── TaskManager.h               // Priority scheduler & concurrent robot assigner
│   │   ├── PathFinder.h                // BFS, Dijkstra, Static A*, and Space-Time A*
│   │   ├── Warehouse.h                 // Grid topology, obstacles, cell traversal weights
│   │   ├── EcoRoute.h                  // EcoRoute energy-aware routing engine & models
│   │   ├── FoodSaver.h                 // Perishable inventory, FEFO tracking & donation FSM
│   │   ├── WarehouseSystem.h           // Central orchestrator facade & simulation controller
│   │   └── Exceptions.h                // Domain-specific exception hierarchy
│   │
│   ├── src/                            // C++ class implementations
│   │   ├── Position.cpp                // Coordinate arithmetic & distance implementations
│   │   ├── Product.cpp                 // Product attribute management
│   │   ├── Inventory.cpp               // Stock mutation, validation & safety
│   │   ├── Robot.cpp                   // Robot state transitions, battery & navigation
│   │   ├── PickerRobot.cpp             // Picker specialization kinematics
│   │   ├── TransportRobot.cpp          // Heavy transport specialization kinematics
│   │   ├── HybridRobot.cpp             // Hybrid specialization kinematics
│   │   ├── Shelf.cpp                   // Shelf capacity & inventory association
│   │   ├── Station.cpp                 // Station queue & occupancy management
│   │   ├── ChargingStation.cpp         // Recharging bay rate logic & clamps
│   │   ├── PackingStation.cpp          // Packing bay fulfillment logic
│   │   ├── Order.cpp                   // Order priority comparator & lifecycle
│   │   ├── Task.cpp                    // Task execution progression
│   │   ├── TaskManager.cpp             // Parallel assignment, scoring & battery validation
│   │   ├── PathFinder.cpp              // Graph exploration & space-time algorithms
│   │   ├── Warehouse.cpp               // Grid initialization, bounds & dynamic obstacles
│   │   ├── EcoRoute.cpp                // Physics energy estimator & dynamic route planner
│   │   ├── FoodSaver.cpp               // Expiry risk analysis, FEFO engine & donation logic
│   │   └── WarehouseSystem.cpp         // Orchestration loop, tick step & state serialization
│   │
│   └── main.cpp                        // Standalone C++ terminal simulation runner
│
├── server/
│   ├── server.cpp                      // Native WinSock2 HTTP server & request dispatcher
│   └── HttpHandler.h / .cpp            // REST routing, parameter parsing & JSON builders
│
├── tests/
│   └── test_regression.cpp             // Comprehensive 36-suite automated test harness
│
├── web/                                // Modern Light Web Dashboard
│   ├── index.html                      // Cognify-styled responsive dashboard interface
│   ├── css/
│   │   └── style.css                   // Unified typography, variables, layout & components
│   └── js/
│       ├── api.js                      // Dual-mode HTTP REST client & offline emulator
│       ├── warehouse_canvas.js         // High-DPI canvas renderer for warehouse grid
│       └── app.js                      // View routing, UI bindings, analytics & charts
│
├── data/
│   ├── warehouse_layout.json           // 15x15 grid layout, stations & obstacle coordinates
│   ├── initial_products.json           // Seed catalog of warehouse merchandise
│   └── initial_robots.json             // Initial fleet configuration and docking positions
│
├── CMakeLists.txt                      // Modern CMake build script
├── build.bat                           // One-click compilation script (MinGW / Ninja)
├── build_and_run_tests.bat             // Automated test build and regression runner
├── run_web.bat                         // Automated web dashboard launcher
└── README.md                           // Comprehensive project documentation
```

---

## 🎓 Object-Oriented Programming (OOP) Principles Demonstrated

| OOP Principle | Implementation in Architecture | Concrete Code Reference |
| :--- | :--- | :--- |
| **Classes & Objects** | Real-world entities encapsulated as distinct C++ classes with clear state and behavior | `Product`, `Warehouse`, `Order`, `Shelf`, `Batch` |
| **Encapsulation** | Strict `private`/`protected` member variables accessed via validated getters/setters; immutable invariants | `Inventory::deductStock()`, `Robot::consumeBattery()` |
| **Abstraction** | Pure virtual interfaces defining contractual capabilities without prescribing concrete mechanics | `Robot::moveNextStep()`, `Robot::getBatteryDrainPerMove()` |
| **Inheritance** | Derived specializations inheriting common behaviors and state from base classes | `PickerRobot : public Robot`, `ChargingStation : public Station` |
| **Polymorphism** | Dynamic dispatch at runtime via `virtual` methods invoked on pointer/reference types | `TaskManager` iterating over `std::vector<std::unique_ptr<Robot>>` |
| **Composition** | Strong ownership lifecycle; parent owns and instantiates child components | `Warehouse` owns `std::vector<Shelf>` and stations |
| **Aggregation** | Weak reference relationships; components exist independently of containers | `TaskManager` manages pointers to `Order` and `Robot` |
| **Exception Hierarchy** | Custom domain-specific exceptions derived from `std::exception` | `InsufficientStockException`, `PathNotFoundException` |
| **Static Class Members** | Shared atomic state for sequential identification and global tracking | `Order::nextOrderId`, `Task::nextTaskId`, `Batch::nextBatchId` |

### Key Software Design Patterns
- **Facade Pattern (`WarehouseSystem`)**: Exposes a clean, unified API hiding the complexity of the task manager, inventory, coordinator, EcoRoute engine, and FoodSaver manager.
- **Strategy Pattern (`PathFinder`)**: Enables interchangeable routing algorithms (BFS, Dijkstra, Static A*, Space-Time A*, EcoRoute) sharing common path output schemas.
- **State Pattern (`Order`, `DonationRequest`, `Robot`)**: Manages rigorous finite-state machine transitions (`DRAFT` $\to$ `APPROVED` $\to$ `INVENTORY_RESERVED` $\to$ `READY_FOR_PICKUP` $\to$ `COMPLETED`).
- **Observer Pattern (`EventLog`)**: Decoupled event notification stream broadcasting warehouse events to dashboard telemetry feeds.

---

## 🧮 Data Structures & Algorithms (DSA) Catalog

1. **2D & 3D Grid Graphs**:
   - Discrete 2D coordinate space $(x, y)$ representing physical floor topology, static racks, walls, and walkability.
   - 3D Space-Time graph $(x, y, t)$ where the third dimension represents discrete time ticks, enabling multi-agent collision-free trajectory planning.
2. **Breadth-First Search (BFS)**:
   - Implemented via `std::queue<Position>` with uniform edge weights. Guarantees minimum hop count on unweighted grids. Explores all directions radially.
   - Time Complexity: $\mathcal{O}(V + E)$; Space Complexity: $\mathcal{O}(V)$.
3. **Dijkstra's Algorithm**:
   - Implemented via `std::priority_queue` (min-heap) tracking cumulative path costs ($g(n)$). Supports non-uniform edge costs, narrow aisle speed penalties, and dynamic weights.
   - Time Complexity: $\mathcal{O}((V + E) \log V)$; Space Complexity: $\mathcal{O}(V)$.
4. **A\* Search Algorithm**:
   - Priority-queue guided pathfinding combining exact cost-so-far ($g(n)$) with an admissible Manhattan distance heuristic ($h(n) = (|x_1 - x_2| + |y_1 - y_2|) \times C_{\min}$). Guarantees optimal path while drastically reducing node explorations.
   - Time Complexity: $\mathcal{O}(b^d)$ worst-case, $\mathcal{O}(d)$ best-case; Space Complexity: $\mathcal{O}(V)$.
5. **Space-Time A\* (Multi-Agent Routing)**:
   - Traverses $(x, y, t)$ states. Actions include moving to an unreserved orthogonal neighbor at $t+1$ or waiting in place $(x, y, t+1)$ when blocked, respecting vertex and swap reservations.
6. **Time-Indexed Hash Tables (`ReservationTable`)**:
   - Vertex reservations: `std::unordered_map<int64_t, std::string>` mapping hash of $(x, y, t)$ to `robotId`.
   - Directed edge reservations: `std::unordered_map<int64_t, std::string>` mapping hash of $((x_1, y_1), (x_2, y_2), t)$ to `robotId`.
7. **First-Expired, First-Out (FEFO) Priority Queues**:
   - Batches ordered by `std::tie(expiryDate, batchId)` ensuring inventory approaching expiry is dispatched prior to newer stock, minimizing spoilage.

---

## 🚀 Completed Functional Phases

### 🔹 Phase 1: Intelligent Multi-Robot Parallel Order Execution
- **Concurrent Dispatching**: Eliminated legacy single-order bottleneck (`if (!activeOrders.empty()) return;`), enabling simultaneous multi-robot order scheduling.
- **Round-Trip Battery Feasibility Validation**: Validates that candidate robots possess sufficient energy for the complete delivery cycle plus reserve safety buffer before assignment:
  $$\text{Required Battery} = (d_{\text{robot}\to\text{shelf}} + d_{\text{shelf}\to\text{pack}} + d_{\text{pack}\to\text{charge}}) \times \text{drainRate} + 10\%$$
- **Atomic Movement & Cascade Revocation**: In `MultiRobotCoordinator::resolveMovements`, proposed movements are evaluated atomically. If a lead robot is blocked, trailing robots scheduled to occupy its cell are revoked in real-time, eliminating vertex collisions and "train-trailing" hazards.
- **Smart Staging Buffer Zones**: Directs idle robots waiting near packaging stations to maintain a Manhattan distance $\ge 2$ from station throats, preventing bottleneck choking.

### 🔹 Phase 2: DSA Algorithm Laboratory
- **Interactive Algorithm Sandbox**: Dedicated dashboard laboratory for executing BFS, Dijkstra, and A* on arbitrary start and goal coordinates.
- **Step-by-Step Exploration Visualization**: Records and streams the exact sequential order of visited nodes, rendering open and closed sets dynamically on the canvas.
- **Honest Performance Telemetry**: Computes true microsecond execution durations (`std::chrono::high_resolution_clock`), path length, path cost, and explored node counts directly in C++.
- **Comparative Benchmarking & Export**: Side-by-side metric comparison cards with integrated CSV and formatted PDF summary report export.

### 🔹 Phase 3: EcoRoute — Energy-Aware Robot Routing & Optimization
- **Physics-Grounded Energy Estimation**:
  $$E_{\text{route}} = E_{\text{motion}} + E_{\text{turn}} + E_{\text{payload}} + E_{\text{congestion}} + E_{\text{idle}}$$
  - Kinetic Motion ($E_{\text{motion}}$): Base traction energy scaled by distance and cell surface friction.
  - Turn Penalty ($E_{\text{turn}}$): Angular momentum costs for 90° and 180° orientation adjustments ($0.15\text{ mAh}$ per turn).
  - Payload Resistance ($E_{\text{payload}}$): Mass acceleration multiplier ($0.05\text{ mAh} / \text{unit} / \text{cell}$).
  - Traffic Congestion ($E_{\text{congestion}}$): Dynamic penalty derived from active robot proximity within radius $R=3$.
- **Dynamic Edge Weighting**: Re-weights warehouse graph edges dynamically based on physical corridors, speed restrictions, and active heatmaps.
- **Pareto Route Comparison**: Computes both "Fastest (Shortest Distance)" and "EcoRoute (Energy-Optimal)" paths simultaneously, displaying trade-offs in length, energy consumed, and battery feasibility.

### 🔹 Phase 4: FoodSaver — Near-Expiry Inventory & Donation Optimization
- **Batch-Level Perishable Tracking**: Tracks inventory batches with unique batch IDs, receipt dates, manufacturing dates, and ISO expiry dates.
- **Shelf-Life Risk Scoring Model**:
  $$\text{Risk Score} = \max\left(0.0, \min\left(100.0, \left(1.0 - \frac{\text{Days Remaining}}{\text{Total Shelf Life}}\right) \times 100.0\right)\right)$$
  Classified into: `FRESH` ($<25$), `GOOD` ($25\text{--}50$), `FAIR` ($50\text{--}75$), `CRITICAL` ($75\text{--}99$), `EXPIRED` ($100$).
- **FEFO Order Allocation**: Automatically allocates stock for customer orders from earliest-expiring batches, with multi-batch split support.
- **Non-Profit Donation Lifecycle FSM**: Strict 5-state management (`DRAFT` $\to$ `APPROVED` $\to$ `INVENTORY_RESERVED` $\to$ `READY_FOR_PICKUP` $\to$ `COMPLETED`) preventing stock leakage or double reservation.
- **Environmental Impact Metrics**: Calculates food saved (kg), greenhouse gas emissions avoided ($2.5\text{ kg CO}_2\text{e} / \text{kg}$), and potable water conserved ($1,000\text{ L} / \text{kg}$).

---

## 🌐 Complete REST API Reference

The native C++ HTTP server exposes the following REST endpoints on `http://localhost:8080`:

| Category | Method | Endpoint | Description |
| :--- | :--- | :--- | :--- |
| **System** | `GET` | `/api/status` | Returns complete live simulation state JSON (grid, robots, orders, stations) |
| **System** | `POST` | `/api/step` | Advances discrete simulation by 1 tick with atomic movement resolution |
| **System** | `POST` | `/api/reset` | Resets simulation tick, robots, reservations, and dynamic obstacles |
| **Orders** | `POST` | `/api/order` | Creates customer order: `{"productId": 101, "quantity": 2, "priority": "HIGH"}` |
| **Orders** | `GET` | `/api/orders` | Retrieves all active and historical order records |
| **Fleet** | `GET` | `/api/robots` | Retrieves live telemetry, status, battery, and coordinates for all robots |
| **Obstacles**| `POST` | `/api/obstacle/add` | Adds dynamic obstacle at `{"x": 4, "y": 4}` and triggers route replanning |
| **Obstacles**| `POST` | `/api/obstacle/remove` | Removes dynamic obstacle at `{"x": 4, "y": 4}` and unblocks affected routes |
| **DSA Lab** | `GET` | `/api/dsa/explore?algo=astar&sx=0&sy=0&gx=14&gy=14` | Returns visited node sequence and computed optimal path |
| **DSA Lab** | `GET` | `/api/dsa/compare?sx=0&sy=0&gx=14&gy=14` | Benchmarks BFS, Dijkstra, and A* execution time, nodes, and path cost |
| **EcoRoute** | `GET` | `/api/ecoroute/compare?sx=0&sy=0&gx=14&gy=14&payload=5` | Compares Fastest vs. EcoRoute paths with energy consumption breakdown |
| **EcoRoute** | `POST` | `/api/ecoroute/plan` | Computes energy-optimal route for specific robot with payload parameters |
| **FoodSaver**| `GET` | `/api/foodsaver/batches` | Retrieves all inventory batch records with shelf-life risk scores |
| **FoodSaver**| `POST` | `/api/foodsaver/batches` | Registers new perishable batch with production and expiry dates |
| **FoodSaver**| `GET` | `/api/foodsaver/expiry` | Returns prioritized list of near-expiry inventory batches |
| **FoodSaver**| `GET` | `/api/foodsaver/eligibility` | Returns batches eligible for food donation |
| **FoodSaver**| `POST` | `/api/foodsaver/donations` | Creates new donation request for near-expiry stock |
| **FoodSaver**| `POST` | `/api/foodsaver/donations/approve` | Advances donation request through approval FSM |
| **FoodSaver**| `GET` | `/api/foodsaver/analytics` | Returns food waste reduction and environmental impact metrics |
| **FoodSaver**| `GET` | `/api/foodsaver/report` | Exports complete FoodSaver audit log in CSV format |
| **Static** | `GET` | `/*` | Serves dashboard frontend assets (`index.html`, CSS, JS) with path traversal checks |

---

## 🧪 Regression Testing & Validation Harness

The regression test suite (`tests/test_regression.cpp`) provides exhaustive coverage across **36 distinct test suites** totaling **1,348 passed assertions with 0 failures**:

```text
========================================================
   Robot Warehouse System — C++ Regression Test Suite   
========================================================
  Suite  1: Warehouse Coordinates & Walkability                [PASS]
  Suite  2: Inventory & Stock Invariants                       [PASS]
  Suite  3: Robot Hierarchy, Polymorphism & Battery            [PASS]
  Suite  4: Robot Payload Constraints                          [PASS]
  Suite  5: Pathfinding Correctness (BFS, Dijkstra, A*)        [PASS]
  Suite  6: End-to-End Order Lifecycle & Inventory Sync        [PASS]
  Suite  7: Multi-Robot Vertex Collision Prevention            [PASS]
  Suite  8: Head-on Edge Collision (Swap) Prevention           [PASS]
  Suite  9: Three Robots in Narrow Aisle                       [PASS]
  Suite 10: Robot Waiting Preserves Task & State               [PASS]
  Suite 11: Route Change Releases Obsolete Reservations        [PASS]
  Suite 12: Dynamic Obstacle Blocks Route & Triggers Replan    [PASS]
  Suite 13: Dynamic Obstacle on Occupied Cell Rejected         [PASS]
  Suite 14: Station Capacity Limits & Concurrent Access        [PASS]
  Suite 15: Deadlock Detection & Space-Time A* Recovery        [PASS]
  Suite 16: Unreachable Destination Handling & Reporting       [PASS]
  Suite 17: Priority-Based Route Contention Resolution         [PASS]
  Suite 18: Full Simulation State Reset Integrity              [PASS]
  Suite 19: Zero Observed Ticks & Zero-Division Safety         [PASS]
  Suite 20: Operational Utilization & Movement Counters        [PASS]
  Suite 21: Battery Consumption Tracking & Charging Events     [PASS]
  Suite 22: Order Lifecycle Timestamps & Durations             [PASS]
  Suite 23: Congestion Heatmap Spatial Matrix & Bounds         [PASS]
  Suite 24: Analytics Telemetry Reset vs State Reset           [PASS]
  Suite 25: Multi-Robot Battery Trend Analysis                 [PASS]
  Suite 26: Heatmap Normalization (Linear, Log, Quantile)      [PASS]
  Suite 27: Isolated Algorithm Benchmarking                    [PASS]
  Suite 28: Packing Station Buffer Staging Coordination        [PASS]
  Suite 29: Two-Phase Inventory Reservation & Deduction        [PASS]
  Suite 30: JSON Configuration Ingestion & Persistence         [PASS]
  Suite 31: REST API Validation, Routing & Concurrency         [PASS]
  Suite 32: Cooperative Robot Teams & Parallel Order Pipeline  [PASS]
  Suite 33: Intelligent Multi-Robot Scheduling (Phase 1)       [PASS]
  Suite 34: DSA Algorithm Laboratory Engine (Phase 2)          [PASS]
  Suite 35: EcoRoute Energy Estimation & Planning (Phase 3)    [PASS]
  Suite 36: FoodSaver Perishable & Donation Engine (Phase 4)   [PASS]
========================================================
   TEST SUMMARY: 1348 PASSED, 0 FAILED (100% SUCCESS)
========================================================
```

---

## 🛠️ Build and Execution Guide

### Prerequisites
- Modern C++17 compliant compiler (`g++` via MinGW64 or MSVC).
- Modern CMake (version $\ge 3.15$) and Ninja or Make.
- Modern web browser (Chrome, Edge, Firefox).

### 1. Build and Run Regression Tests
To compile and execute the complete 36-suite test suite in one step:
```cmd
cd Robot
build_and_run_tests.bat
```

### 2. Manual Compilation via MinGW64 Command Line
```cmd
cd Robot

REM Compile Core CLI Simulator
g++ -std=c++17 -O2 -Icore/include core/main.cpp core/src/*.cpp -o build/WarehouseCore.exe

REM Compile Native WinSock2 HTTP Server
g++ -std=c++17 -O2 -Icore/include server/server.cpp server/HttpHandler.cpp core/src/*.cpp -lws2_32 -o build/WarehouseServer.exe

REM Compile Complete Regression Test Suite
g++ -std=c++17 -O2 -Icore/include tests/test_regression.cpp server/HttpHandler.cpp core/src/*.cpp -lws2_32 -o build/WarehouseTests.exe
```

### 3. Launching the Application
1. **Start Backend Server**:
   ```cmd
   cd Robot/build
   WarehouseServer.exe
   ```
   *The server binds to port 8080 and begins serving REST endpoints and static assets.*
2. **Access Web Dashboard**:
   Open `http://localhost:8080` in your browser, or double-click `run_web.bat`.

---

## ⚠️ Known Technical Limitations

1. **Discrete Grid Topology**: Movement and time are modeled discretely ($1\times1$ grid cells and integer ticks). Continuous acceleration, braking distances, and non-orthogonal orientations are simplified into discrete turn costs and cell transitions.
2. **Synchronous Single-Threaded Event Loop**: The native HTTP server employs a synchronous non-blocking socket select loop. While sufficient for dashboard interaction, high-concurrency load testing would benefit from an asynchronous `epoll`/`IOCP` thread pool.
3. **Fixed Map Dimensions**: The default grid layout is configured for $15\times15$. Larger warehouse floor plans require updating `warehouse_layout.json` and adjusting canvas viewport scaling.
4. **Offline Emulation Differences**: When running in offline browser emulation mode (without `WarehouseServer.exe`), algorithm execution times reflect JavaScript client runtime rather than high-performance compiled C++ microsecond metrics.

---

## 🔮 Future Roadmap & Potential Enhancements

- **Phase 5: SafeMove (Human-Aware Robot Safety Simulation)**: Introduce dynamic human worker agents with stochastic movement vectors, establishing dynamic safety speed buffer zones, audible warning signals, and emergency stop halos around personnel.
- **Continuous Kinematic Controller**: Transition from discrete grid transitions to continuous Spline/Dubins curve trajectory planning.
- **Multi-Warehouse Federation**: Enable horizontal scaling across multiple distributed fulfillment centers with centralized order routing.
- **Physical Hardware Integration**: Connect through ROS 2 / Micro-ROS bridge nodes to drive physical TurtleBot or differential drive mobile robots.
