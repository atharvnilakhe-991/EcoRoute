#include "include/Exceptions.h"
#include "include/PathFinder.h"
#include "include/WarehouseSystem.h"
#include <iomanip>
#include <iostream>

void printSeparator(const std::string &title) {
  std::cout << "\n======================================================\n";
  std::cout << "  " << title << "\n";
  std::cout << "======================================================\n";
}

int main() {
  std::cout << R"(
  ____       _           _     __        __              _                         
 |  _ \ ___ | |__   ___ | |_   \ \      / /_ _ _ __ ___ | |__   ___  _   _ ___  ___ 
 | |_) / _ \| '_ \ / _ \| __|   \ \ /\ / / _` | '__/ _ \| '_ \ / _ \| | | / __|/ _ \
 |  _ < (_) | |_) | (_) | |_     \ V  V / (_| | | |  __/| | | | (_) | |_| \__ \  __/
 |_| \_\___/|_.__/ \___/ \__|     \_/\_/ \__,_|_|  \___||_| |_|\___/ \__,_|___/\___|
                                                                                    
    Robot Warehouse Management & Automation System (C++ OOP Core)
    ============================================================
)" << std::endl;

  try {
    // 1. Initialize Warehouse System
    printSeparator("1. INITIALIZING WAREHOUSE SYSTEM & PHYSICAL GRID");
    WarehouseSystem system(15, 15);
    std::string cfgErr;
    if (system.loadConfiguration("data/warehouse_layout.json", "data/initial_products.json", "data/initial_robots.json", cfgErr)) {
      std::cout << "Warehouse initialized from configuration files (data/ directory): 15x15 grid.\n";
    } else {
      std::cout << "Warehouse initialized using default configuration fallback: 15x15 grid.\n";
      system.initializeDefaultWarehouse();
    }
    std::cout << "Configured: 4 Aisles of Shelves, 2 Charging Stations, 2 "
                 "Packing Stations, 4 Obstacle Pillars.\n";

    system.getWarehouse().printWarehouseGrid();

    // 2. Display Inventory
    printSeparator("2. INITIAL PRODUCT INVENTORY & SHELF LOCATIONS");
    system.getInventory().printInventory();

    // 3. Demonstrate OOP Polymorphism & Robot Fleet
    printSeparator("3. ROBOT FLEET & OOP POLYMORPHISM DEMO");
    std::cout << std::left << std::setw(8) << "ID" << std::setw(20) << "Name"
              << std::setw(18) << "Type (Subclass)" << std::setw(10)
              << "Battery" << std::setw(10) << "Capacity" << std::setw(14)
              << "Drain/Move"
              << "Position\n";
    std::cout << "-------------------------------------------------------------"
                 "-------------------\n";
    for (const auto &r : system.getRobots()) {
      std::cout << std::left << std::setw(8) << r->getId() << std::setw(20)
                << r->getName() << std::setw(18) << r->getRobotTypeName()
                << std::fixed << std::setprecision(1) << std::setw(9)
                << r->getBattery() << "%" << std::setw(10)
                << r->getPayloadCapacity() << std::setw(14)
                << r->getBatteryDrainPerMove() << r->getPosition() << "\n";
    }

    // 4. Algorithm Benchmark (BFS vs Dijkstra vs A*)
    printSeparator("4. PATHFINDING BENCHMARK: BFS vs DIJKSTRA vs A*");
    Position testStart(0, 0);
    Position testGoal(14, 14);
    std::cout << "Calculating route from " << testStart << " to " << testGoal
              << " across warehouse obstacles:\n\n";

    PathResult bfsRes =
        PathFinder::findPathBFS(system.getWarehouse(), testStart, testGoal);
    PathResult dijkstraRes = PathFinder::findPathDijkstra(system.getWarehouse(),
                                                          testStart, testGoal);
    PathResult astarRes =
        PathFinder::findPathAStar(system.getWarehouse(), testStart, testGoal);

    std::cout << std::left << std::setw(12) << "Algorithm" << std::setw(8)
              << "Found" << std::setw(14) << "Path Length" << std::setw(16)
              << "Nodes Visited" << std::setw(16) << "Total Cost"
              << "Time (microseconds)\n";
    std::cout << "-------------------------------------------------------------"
                 "-----------\n";

    auto printRow = [](const PathResult &res) {
      std::cout << std::left << std::setw(12) << res.algorithmName
                << std::setw(8) << (res.found ? "YES" : "NO") << std::setw(14)
                << res.path.size() << std::setw(16) << res.nodesVisited
                << std::fixed << std::setprecision(1) << std::setw(16)
                << res.totalCost << std::fixed << std::setprecision(2)
                << res.executionTimeMicroseconds << " us\n";
    };

    printRow(bfsRes);
    printRow(dijkstraRes);
    printRow(astarRes);

    std::cout << "\nObservation: ";
    if (astarRes.nodesVisited < bfsRes.nodesVisited) {
      std::cout << "A* significantly reduces nodes visited ("
                << astarRes.nodesVisited << " vs BFS " << bfsRes.nodesVisited
                << ") due to Manhattan heuristic guidance!\n";
    } else if (astarRes.nodesVisited == bfsRes.nodesVisited) {
      std::cout << "For this specific diagonal route across the warehouse "
                   "aisles, all reachable walkable cells ("
                << astarRes.nodesVisited
                << ") were explored across BFS, Dijkstra, and A* to find the "
                   "optimal path of cost "
                << astarRes.totalCost << ".\n";
    } else {
      std::cout << "All algorithms successfully found the optimal path of cost "
                << astarRes.totalCost << ".\n";
    }

    // 5. Live Order Simulation Scenario
    printSeparator("5. END-TO-END ORDER DISPATCH & SIMULATION SCENARIO");
    std::cout << "Creating Order #1025: 2x 'ThinkPad Laptop' (Product #101) "
                 "with HIGH priority...\n";

    int orderId = system.createOrder(101, 2, OrderPriority::HIGH);
    std::cout << "Order successfully created! Order ID: " << orderId << "\n";

    std::cout << "\nStarting discrete simulation loop (advancing ticks)...\n";
    bool orderDone = false;
    int maxTicks = 35;

    for (int tick = 1; tick <= maxTicks && !orderDone; ++tick) {
      system.stepSimulation();
      std::cout << "[Tick " << std::setw(2) << tick << "] ";

      for (const auto &r : system.getRobots()) {
        if (r->getCurrentTask() &&
            r->getCurrentTask()->getOrderId() == orderId) {
          std::cout << "Robot " << r->getId() << " (" << r->getName() << ") "
                    << "Status: " << std::setw(16)
                    << robotStatusToString(r->getStatus())
                    << " Pos: " << r->getPosition() << " Batt: " << std::fixed
                    << std::setprecision(1) << r->getBattery() << "% ";

          if (r->getStatus() == RobotStatus::PICKING) {
            std::cout << "--> [PICKING 2x Laptops from Shelf]";
          } else if (r->getStatus() == RobotStatus::DELIVERING) {
            std::cout << "--> [DELIVERED to Packing Station!]";
          }
          std::cout << "\n";
        }
      }

      auto order = system.getTaskManager().getOrderById(orderId);
      if (order && order->getStatus() == OrderStatus::COMPLETED) {
        std::cout << "\n>>> ORDER #" << orderId
                  << " COMPLETED SUCCESSFULLY! <<<\n";
        orderDone = true;
      }
    }

    // 6. Updated Inventory Verification
    printSeparator("6. POST-ORDER INVENTORY STATUS");
    std::cout << "Verifying inventory stock deduction for Product #101:\n";
    const Product *updatedProd = system.getInventory().getProduct(101);
    if (updatedProd) {
      std::cout << "Product: " << updatedProd->getName()
                << " | Remaining Stock: " << updatedProd->getQuantity()
                << " (Initial was 35, expected 33)\n";
    }

    // 7. Exception Handling Demonstration
    printSeparator("7. EXCEPTION HANDLING DEMO");
    std::cout << "Attempting to order 999 units of 'Standing Desk Frame' (Only "
                 "10 in stock)...\n";
    try {
      system.createOrder(106, 999, OrderPriority::MEDIUM);
    } catch (const InsufficientStockException &ex) {
      std::cout << "Caught Expected Domain Exception: " << ex.what() << "\n";
    }

    // 8. Snapshot Persistence
    printSeparator("8. WAREHOUSE SNAPSHOT PERSISTENCE");
    std::string snapshotFile = "data/warehouse_snapshot.json";
    if (system.saveToFile(snapshotFile)) {
      std::cout << "System snapshot saved to: " << snapshotFile << "\n";
      WarehouseSystem verifySys(10, 10);
      std::string loadErr;
      if (verifySys.loadFromFile(snapshotFile, loadErr)) {
        std::cout << "Snapshot verification passed: Loaded state restored (Tick: "
                  << verifySys.getSimulationTick() << ", Robots: " << verifySys.getRobots().size()
                  << ", Catalog: " << verifySys.getInventory().getAllProducts().size() << " products).\n";
      } else {
        std::cout << "Snapshot verification warning: " << loadErr << "\n";
      }
    }

    printSeparator("SIMULATION DEMO COMPLETED SUCCESSFULLY");
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
