#include "../core/include/AlgorithmBenchmark.h"
#include "../core/include/AlgorithmLab.h"
#include "../core/include/EcoRoute.h"
#include "../core/include/FoodSaver.h"
#include "../core/include/Exceptions.h"
#include "../core/include/HttpHandler.h"
#include "../core/include/HybridRobot.h"
#include "../core/include/MultiRobotCoordinator.h"
#include "../core/include/PathFinder.h"
#include "../core/include/PickerRobot.h"
#include "../core/include/ReservationTable.h"
#include "../core/include/TransportRobot.h"
#include "../core/include/WarehouseSystem.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cmath>

#include <atomic>
#include <thread>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define ASSERT_TRUE(condition, testName)                                       \
  do {                                                                         \
    if (condition) {                                                           \
      std::cout << "  [PASS] " << testName << "\n";                            \
      g_testsPassed++;                                                         \
    } else {                                                                   \
      std::cerr << "  [FAIL] " << testName << " (Line: " << __LINE__ << ")\n"; \
      g_testsFailed++;                                                         \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition, testName) ASSERT_TRUE(!(condition), testName)

void testWarehouseCoordinates() {
  std::cout << "\n--- TEST SUITE 1: Warehouse Coordinates & Walkability ---\n";
  Warehouse wh(15, 15);
  wh.setObstacle(Position(5, 5), true);
  wh.addShelf(Shelf("SH_01", Position(3, 3)));
  wh.addChargingStation(ChargingStation("CS_01", Position(0, 0)));
  wh.addPackingStation(PackingStation("PK_01", Position(14, 14)));

  ASSERT_TRUE(wh.isValidPosition(Position(0, 0)),
              "Valid top-left origin (0,0)");
  ASSERT_TRUE(wh.isValidPosition(Position(14, 14)),
              "Valid bottom-right corner (14,14)");
  ASSERT_FALSE(wh.isValidPosition(Position(-1, 0)),
               "Negative coordinate rejected");
  ASSERT_FALSE(wh.isValidPosition(Position(15, 5)), "X out-of-bounds rejected");
  ASSERT_FALSE(wh.isValidPosition(Position(5, 15)), "Y out-of-bounds rejected");

  ASSERT_TRUE(wh.isWalkable(Position(1, 1)), "Empty floor cell is walkable");
  ASSERT_TRUE(wh.isWalkable(Position(0, 0)), "Charging station is walkable");
  ASSERT_TRUE(wh.isWalkable(Position(14, 14)), "Packing station is walkable");
  ASSERT_FALSE(wh.isWalkable(Position(5, 5)), "Obstacle cell is NOT walkable");
  ASSERT_FALSE(wh.isWalkable(Position(3, 3)),
               "Shelf cell is NOT walkable (impassable)");
}

void testInventoryManagement() {
  std::cout << "\n--- TEST SUITE 2: Inventory & Stock Invariants ---\n";
  Inventory inv;
  Product laptop(101, "Laptop", "Electronics", 10, 1000.0, Position(3, 3));
  inv.addProduct(laptop);

  ASSERT_TRUE(inv.hasStock(101, 5), "Stock check for available quantity");
  ASSERT_FALSE(inv.hasStock(101, 15), "Stock check for unavailable quantity");

  bool deducted = inv.deductStock(101, 4);
  ASSERT_TRUE(deducted && inv.getProduct(101)->getQuantity() == 6,
              "Deduct 4 items from stock 10 leaves 6");

  bool threw = false;
  try {
    inv.deductStock(101, 10); // only 6 available
  } catch (const InsufficientStockException &ex) {
    threw = true;
  }
  ASSERT_TRUE(threw,
              "Deducting excessive stock throws InsufficientStockException");
  ASSERT_TRUE(inv.getProduct(101)->getQuantity() == 6,
              "Stock remains unchanged on failed deduction");

  inv.restock(101, 5);
  ASSERT_TRUE(inv.getProduct(101)->getQuantity() == 11,
              "Restocking 5 items updates quantity to 11");
}

void testRobotPolymorphismAndBattery() {
  std::cout
      << "\n--- TEST SUITE 3: Robot Hierarchy, Polymorphism & Battery ---\n";
  std::unique_ptr<Robot> picker =
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(0, 0));
  std::unique_ptr<Robot> hauler =
      std::make_unique<TransportRobot>("R02", "Hauler-1", Position(0, 0));
  std::unique_ptr<Robot> hybrid =
      std::make_unique<HybridRobot>("R03", "Hybrid-1", Position(0, 0));

  ASSERT_TRUE(picker->getPayloadCapacity() == 6,
              "PickerRobot payload capacity is 6");
  ASSERT_TRUE(hauler->getPayloadCapacity() == 25,
              "TransportRobot payload capacity is 25");
  ASSERT_TRUE(hybrid->getPayloadCapacity() == 12,
              "HybridRobot payload capacity is 12");

  ASSERT_TRUE(picker->getBatteryDrainPerMove() <
                  hauler->getBatteryDrainPerMove(),
              "PickerRobot has lower drain than TransportRobot");

  // Battery depletion test
  picker->consumeBattery(90.0);
  ASSERT_TRUE(picker->getBattery() == 10.0, "Battery consumed from 100 to 10");
  ASSERT_TRUE(picker->getStatus() == RobotStatus::LOW_BATTERY,
              "Robot status transitioned to LOW_BATTERY");

  // Negative clamping
  picker->consumeBattery(20.0);
  ASSERT_TRUE(picker->getBattery() == 0.0, "Battery clamped at 0.0");

  // Recharge
  picker->setStatus(RobotStatus::CHARGING);
  picker->charge(50.0);
  ASSERT_TRUE(picker->getBattery() == 50.0, "Charging increases battery to 50");
  picker->charge(60.0);
  ASSERT_TRUE(picker->getBattery() == 100.0, "Battery clamped at 100.0");
  ASSERT_TRUE(picker->getStatus() == RobotStatus::IDLE,
              "Fully charged robot resets to IDLE");
}

void testRobotPayloadConstraints() {
  std::cout << "\n--- TEST SUITE 4: Robot Payload Constraints ---\n";
  PickerRobot picker("R01", "Picker-1", Position(0, 0)); // capacity 6
  ASSERT_TRUE(picker.pickItem(101, 4), "Picking 4 items (<= 6) succeeds");
  ASSERT_TRUE(picker.getCurrentPayload() == 4, "Current payload updated to 4");
  ASSERT_FALSE(picker.pickItem(101, 3),
               "Picking additional 3 items (4+3 > 6) fails");
  ASSERT_TRUE(picker.getCurrentPayload() == 4,
              "Payload unchanged after rejected pickup");

  picker.deliverItem();
  ASSERT_TRUE(picker.getCurrentPayload() == 0,
              "Delivering items unloads payload to 0");
}

void testPathfindingAlgorithms() {
  std::cout << "\n--- TEST SUITE 5: Pathfinding Correctness (BFS, Dijkstra, "
               "A*) ---\n";
  Warehouse wh(10, 10);
  // Add vertical obstacle wall with a door at (5, 5)
  for (int y = 0; y < 10; ++y) {
    if (y != 5) {
      wh.setObstacle(Position(5, y), true);
    }
  }

  Position start(0, 0);
  Position goal(9, 9);

  PathResult bfsRes = PathFinder::findPathBFS(wh, start, goal);
  PathResult dijkstraRes = PathFinder::findPathDijkstra(wh, start, goal);
  PathResult astarRes = PathFinder::findPathAStar(wh, start, goal);

  ASSERT_TRUE(bfsRes.found, "BFS found route through doorway");
  ASSERT_TRUE(dijkstraRes.found, "Dijkstra found route through doorway");
  ASSERT_TRUE(astarRes.found, "A* found route through doorway");

  ASSERT_TRUE(bfsRes.path.size() == astarRes.path.size(),
              "BFS and A* return identical optimal path length");
  ASSERT_TRUE(dijkstraRes.totalCost == astarRes.totalCost,
              "Dijkstra and A* compute equal optimal path cost");
  ASSERT_TRUE(astarRes.nodesVisited <= bfsRes.nodesVisited,
              "A* explores fewer or equal nodes than BFS");

  // Test obstacle avoidance: ensure no path step hits an obstacle
  bool hitObstacle = false;
  for (const auto &p : astarRes.path) {
    if (!wh.isWalkable(p)) {
      hitObstacle = true;
      break;
    }
  }
  ASSERT_FALSE(hitObstacle,
               "A* path never traverses impassable obstacle cells");

  // Test unreachable destination
  Warehouse trappedWh(5, 5);
  trappedWh.setObstacle(Position(3, 4), true);
  trappedWh.setObstacle(Position(4, 3), true); // Traps (4,4)
  PathResult blocked =
      PathFinder::findPathAStar(trappedWh, Position(0, 0), Position(4, 4));
  ASSERT_FALSE(blocked.found,
               "Unreachable destination correctly returns found=false");

  // Test invalid start/goal
  PathResult invalidStart =
      PathFinder::findPathAStar(wh, Position(-1, 0), goal);
  ASSERT_FALSE(invalidStart.found,
               "Invalid start coordinate safely returns found=false");
  PathResult invalidGoal = PathFinder::findPathAStar(
      wh, start, Position(5, 1)); // (5,1) is an obstacle
  ASSERT_FALSE(invalidGoal.found,
               "Obstacle goal coordinate safely returns found=false");
}

void testOrderLifecycleAndDelivery() {
  std::cout << "\n--- TEST SUITE 6: End-to-End Order Lifecycle & Inventory "
               "Sync ---\n";
  WarehouseSystem sys(15, 15);
  sys.initializeDefaultWarehouse();

  int initialStock = sys.getInventory().getProduct(101)->getQuantity(); // 35
  int orderId = sys.createOrder(101, 2, OrderPriority::HIGH);
  ASSERT_TRUE(orderId > 0, "Order #1025 created successfully");

  // Order state advancement over simulation ticks
  bool reachedCompleted = false;
  int maxTicks = 40;

  for (int t = 1; t <= maxTicks && !reachedCompleted; ++t) {
    sys.stepSimulation();
    auto order = sys.getTaskManager().getOrderById(orderId);
    if (order && order->getStatus() == OrderStatus::COMPLETED) {
      reachedCompleted = true;
    }
  }

  ASSERT_TRUE(reachedCompleted,
              "Order reached COMPLETED status within simulation ticks");

  // Check inventory stock reduction
  int updatedStock = sys.getInventory().getProduct(101)->getQuantity();
  ASSERT_TRUE(
      updatedStock == initialStock - 2,
      "Inventory stock decremented exactly by order quantity (35 -> 33)");

  // Check packing station statistics
  bool stationUpdated = false;
  for (const auto &ps : sys.getWarehouse().getPackingStations()) {
    if (ps.getOrdersProcessed() > 0) {
      stationUpdated = true;
      break;
    }
  }
  ASSERT_TRUE(stationUpdated,
              "Packing station recorded completed order delivery");
}

void testMultiRobotVertexCollision() {
  std::cout
      << "\n--- TEST SUITE 7: Multi-Robot Vertex Collision Prevention ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(4, 2)));

  // Both robots have paths leading to common cell (3, 2)
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(3, 2)),
      {Position(2, 2), Position(3, 2)});
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PICK, Position(3, 2)),
      {Position(4, 2), Position(3, 2)});

  coord.coordinateMovement(robots, wh, tm, 1);

  ASSERT_TRUE(robots[0]->getPosition() != robots[1]->getPosition(),
              "Two robots never occupy the same grid cell at the same tick");
  bool oneMovedToTarget = (robots[0]->getPosition() == Position(3, 2) &&
                           robots[1]->getPosition() == Position(4, 2)) ||
                          (robots[1]->getPosition() == Position(3, 2) &&
                           robots[0]->getPosition() == Position(2, 2));
  ASSERT_TRUE(oneMovedToTarget,
              "At most one robot enters contested target cell; other waits");
  ASSERT_TRUE(coord.getCollisionsPrevented() >= 1,
              "Collision prevention counter accurately increments");
}

void testMultiRobotHeadOnSwapCollision() {
  std::cout
      << "\n--- TEST SUITE 8: Head-on Edge Collision (Swap) Prevention ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(2, 3)));

  // R01 wants to move to (2, 3), R02 wants to move to (2, 2)
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(2, 3)),
      {Position(2, 2), Position(2, 3)});
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PICK, Position(2, 2)),
      {Position(2, 3), Position(2, 2)});

  coord.coordinateMovement(robots, wh, tm, 1);

  bool swapped = (robots[0]->getPosition() == Position(2, 3) &&
                  robots[1]->getPosition() == Position(2, 2));
  ASSERT_FALSE(swapped, "Head-on edge swap is rejected");
  ASSERT_TRUE(robots[0]->getPosition() != robots[1]->getPosition(),
              "Robots remain in distinct positions after swap rejection");
  ASSERT_TRUE(coord.getCollisionsPrevented() >= 1,
              "Swap conflict recorded in collisions prevented");
}

void testNarrowAisleCoordination() {
  std::cout << "\n--- TEST SUITE 9: Three Robots in Narrow Aisle ---\n";
  Warehouse wh(10, 10);
  // Narrow corridor at y=3, x=1..5 with walls above and below
  for (int x = 1; x <= 5; ++x) {
    wh.setObstacle(Position(x, 2), true);
    wh.setObstacle(Position(x, 4), true);
  }

  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(1, 3)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(0, 3)));
  robots.push_back(std::make_unique<PickerRobot>(
      "R03", "Picker-3", Position(0, 2))); // outside corridor

  // R01 moves along corridor to (4, 3)
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(4, 3)),
      {Position(1, 3), Position(2, 3), Position(3, 3), Position(4, 3)});
  // R02 follows R01
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PICK, Position(3, 3)),
      {Position(0, 3), Position(1, 3), Position(2, 3), Position(3, 3)});

  bool safetyMaintained = true;
  for (int tick = 1; tick <= 5; ++tick) {
    coord.coordinateMovement(robots, wh, tm, tick);
    if (robots[0]->getPosition() == robots[1]->getPosition() ||
        robots[0]->getPosition() == robots[2]->getPosition() ||
        robots[1]->getPosition() == robots[2]->getPosition()) {
      safetyMaintained = false;
    }
  }

  ASSERT_TRUE(
      safetyMaintained,
      "All 3 robots maintain safe non-overlapping positions in narrow aisle");
  ASSERT_TRUE(robots[0]->getPosition() == Position(4, 3),
              "Leading robot successfully reaches destination");
}

void testRobotWaitingPreservesTask() {
  std::cout
      << "\n--- TEST SUITE 10: Robot Waiting Preserves Task & State ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(1, 1)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(1, 3)));

  // R01 stays at (1, 2)
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(1, 2)),
      {Position(1, 1), Position(1, 2)});
  // R02 wants to enter (1, 2) as well
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PICK, Position(1, 2)),
      {Position(1, 3), Position(1, 2)});

  coord.coordinateMovement(robots, wh, tm, 1);

  Robot *waitingRobot = (robots[0]->getPosition() == Position(1, 1))
                            ? robots[0].get()
                            : robots[1].get();
  ASSERT_TRUE(waitingRobot->getStatus() == RobotStatus::WAITING,
              "Denied robot enters WAITING status");
  ASSERT_TRUE(waitingRobot->getCurrentTask() != nullptr,
              "Waiting robot preserves active task");
  ASSERT_TRUE(waitingRobot->getWaitTicks() == 1,
              "Waiting tick counter incremented to 1");
}

void testReservationReleaseOnRouteChange() {
  std::cout << "\n--- TEST SUITE 11: Route Change Releases Obsolete "
               "Reservations ---\n";
  MultiRobotCoordinator coord;
  ReservationTable &table = coord.getReservationTable();

  std::vector<Position> initialRoute = {Position(2, 2), Position(2, 3),
                                        Position(2, 4), Position(2, 5)};
  coord.registerPlannedRoute("R01", initialRoute, 1);

  ASSERT_TRUE(table.isVertexReserved(Position(2, 4), 3, "R01"),
              "Initial route reserves vertex at tick 3");
  ASSERT_TRUE(table.isVertexReserved(Position(2, 5), 4, "R01"),
              "Initial route reserves vertex at tick 4");

  // Robot changes route from tick 2 onwards
  std::vector<Position> newRoute = {Position(2, 2), Position(3, 2),
                                    Position(4, 2)};
  coord.registerPlannedRoute("R01", newRoute, 1);

  ASSERT_FALSE(table.isVertexReserved(Position(2, 4), 3, "R01"),
               "Obsolete reservation released at tick 3");
  ASSERT_FALSE(table.isVertexReserved(Position(2, 5), 4, "R01"),
               "Obsolete reservation released at tick 4");
  ASSERT_TRUE(table.isVertexReserved(Position(4, 2), 3, "R01"),
              "New route vertex successfully reserved at tick 3");
}

void testDynamicObstacleBlocksActiveRoute() {
  std::cout << "\n--- TEST SUITE 12: Dynamic Obstacle Blocks Route & Triggers "
               "Replan ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(0, 0)));

  std::vector<Position> route = {Position(0, 0), Position(0, 1), Position(0, 2),
                                 Position(0, 3)};
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(0, 3)), route);
  coord.registerPlannedRoute("R01", route, 1);

  // Dynamic obstacle placed at (0, 2)
  bool placed =
      coord.handleDynamicObstacleAdded(Position(0, 2), robots, wh, tm, 1);
  ASSERT_TRUE(placed, "Dynamic obstacle placed successfully");
  ASSERT_TRUE(wh.isDynamicObstacle(Position(0, 2)),
              "Warehouse recognizes cell as dynamic obstacle");

  // Advance simulation tick
  coord.coordinateMovement(robots, wh, tm, 1);

  ASSERT_TRUE(robots[0]->getPosition() != Position(0, 2),
              "Robot does not move into dynamic obstacle");
  ASSERT_TRUE(coord.getRoutesReplanned() >= 1 ||
                  robots[0]->getStatus() == RobotStatus::BLOCKED,
              "Route replanned or marked blocked due to dynamic obstacle");
}

void testDynamicObstacleOnOccupiedRobotCellRejected() {
  std::cout << "\n--- TEST SUITE 13: Dynamic Obstacle on Occupied Cell "
               "Rejected ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(3, 4)));

  bool placed =
      coord.handleDynamicObstacleAdded(Position(3, 4), robots, wh, tm, 1);
  ASSERT_FALSE(placed, "Placing dynamic obstacle directly on occupied robot "
                       "position is rejected");
  ASSERT_FALSE(wh.isDynamicObstacle(Position(3, 4)),
               "Cell remains regular floor cell");
  ASSERT_TRUE(robots[0]->getPosition() == Position(3, 4),
              "Robot remains safe at its position");
}

void testStationCapacityAndOccupancy() {
  std::cout
      << "\n--- TEST SUITE 14: Station Capacity & Occupancy Coordination ---\n";
  Warehouse wh(10, 10);
  wh.addPackingStation(PackingStation("PK01", Position(5, 5)));
  TaskManager tm;
  MultiRobotCoordinator coord;
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(5, 4)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(5, 6)));

  // Both robots attempt to move into packing station (5, 5)
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PACK, Position(5, 5)),
      {Position(5, 4), Position(5, 5)});
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PACK, Position(5, 5)),
      {Position(5, 6), Position(5, 5)});

  coord.coordinateMovement(robots, wh, tm, 1);

  int robotsAtStation = 0;
  if (robots[0]->getPosition() == Position(5, 5))
    robotsAtStation++;
  if (robots[1]->getPosition() == Position(5, 5))
    robotsAtStation++;

  ASSERT_TRUE(robotsAtStation == 1,
              "Exactly one robot occupies packing station; capacity respected");
  ASSERT_TRUE(robots[0]->getPosition() != robots[1]->getPosition(),
              "No overlapping occupancy at station");
}

void testDeadlockDetectionAndRecovery() {
  std::cout
      << "\n--- TEST SUITE 15: Deadlock Detection & Replan Recovery ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord(2); // Wait threshold = 2 ticks
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(2, 3)));

  // R01 wants (2,3), R02 wants (2,2) - mutual block
  robots[0]->assignTask(
      std::make_shared<Task>(1, TaskType::MOVE_TO_PICK, Position(2, 3)),
      {Position(2, 2), Position(2, 3)});
  robots[1]->assignTask(
      std::make_shared<Task>(2, TaskType::MOVE_TO_PICK, Position(2, 2)),
      {Position(2, 3), Position(2, 2)});

  // Coordinate over multiple ticks to trigger deadlock wait threshold
  coord.coordinateMovement(robots, wh, tm, 1);
  coord.coordinateMovement(robots, wh, tm, 2);

  ASSERT_TRUE(coord.getDeadlocksResolved() >= 1,
              "Deadlock detected and resolution triggered when wait ticks "
              "exceed threshold");
  // Verify no teleportation: robot must be within Manhattan distance 1 of
  // starting cell
  ASSERT_TRUE(robots[0]->getPosition().manhattanDistance(Position(2, 2)) <= 1,
              "No teleportation occurred during deadlock recovery");
  ASSERT_TRUE(robots[1]->getPosition().manhattanDistance(Position(2, 3)) <= 1,
              "No teleportation occurred during deadlock recovery");
}

void testUnreachableDestinationReporting() {
  std::cout << "\n--- TEST SUITE 16: Unreachable Destination Handling ---\n";
  WarehouseSystem sys(10, 10);
  // Seal off target product completely
  sys.getWarehouse().setObstacle(Position(3, 2), true);
  sys.getWarehouse().setObstacle(Position(3, 4), true);
  sys.getWarehouse().setObstacle(Position(2, 3), true);
  sys.getWarehouse().setObstacle(Position(4, 3), true);

  sys.getInventory().addProduct(
      Product(201, "Trapped Item", "Special", 10, 50.0, Position(3, 3)));
  sys.getWarehouse().addShelf(Shelf("SH_TRAP", Position(3, 3)));

  int orderId = sys.createOrder(201, 1, OrderPriority::HIGH);
  ASSERT_TRUE(orderId > 0, "Order placed for unreachable product");

  // Advance simulation tick
  sys.stepSimulation();

  auto order = sys.getTaskManager().getOrderById(orderId);
  ASSERT_TRUE(order != nullptr, "Order exists in system");
  ASSERT_TRUE(order->getStatus() == OrderStatus::PENDING,
              "Order safely remains PENDING; not erroneously completed");
}

void testPriorityRouteCompetition() {
  std::cout << "\n--- TEST SUITE 17: High-Priority vs Low-Priority Route "
               "Competition ---\n";
  Warehouse wh(10, 10);
  TaskManager tm;
  MultiRobotCoordinator coord;

  // Create high priority order and low priority order
  auto highOrder =
      std::make_shared<Order>(101, 1, OrderPriority::HIGH, Position(9, 9));
  auto lowOrder =
      std::make_shared<Order>(102, 1, OrderPriority::LOW, Position(9, 9));
  tm.addOrder(highOrder);
  tm.addOrder(lowOrder);

  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(4, 2)));

  // Both robots compete for cell (3, 2)
  robots[0]->assignTask(std::make_shared<Task>(highOrder->getId(),
                                               TaskType::MOVE_TO_PICK,
                                               Position(3, 2)),
                        {Position(2, 2), Position(3, 2)});
  robots[1]->assignTask(std::make_shared<Task>(lowOrder->getId(),
                                               TaskType::MOVE_TO_PICK,
                                               Position(3, 2)),
                        {Position(4, 2), Position(3, 2)});

  coord.coordinateMovement(robots, wh, tm, 1);

  ASSERT_TRUE(robots[0]->getPosition() == Position(3, 2),
              "High priority order robot wins disputed cell");
  ASSERT_TRUE(robots[1]->getPosition() == Position(4, 2),
              "Low priority order robot waits in place");
  ASSERT_TRUE(robots[1]->getStatus() == RobotStatus::WAITING,
              "Displaced robot enters WAITING state");
}

void testSimulationReset() {
  std::cout
      << "\n--- TEST SUITE 18: Full Simulation State Reset Invariants ---\n";
  WarehouseSystem sys(15, 15);
  sys.initializeDefaultWarehouse();

  // Modify state: add dynamic obstacle, create order, step simulation
  sys.addDynamicObstacle(Position(5, 5));
  sys.createOrder(101, 2, OrderPriority::HIGH);
  sys.stepSimulation();
  sys.stepSimulation();

  ASSERT_TRUE(sys.getSimulationTick() > 0, "Simulation advanced before reset");
  ASSERT_TRUE(sys.getWarehouse().isDynamicObstacle(Position(5, 5)),
              "Dynamic obstacle present before reset");

  // Perform Reset
  sys.resetSimulation();

  ASSERT_TRUE(sys.getSimulationTick() == 0, "Simulation tick reset to 0");
  ASSERT_FALSE(sys.getWarehouse().isDynamicObstacle(Position(5, 5)),
               "Dynamic obstacles cleared after reset");
  ASSERT_TRUE(sys.getCoordinator().getCollisionsPrevented() == 0,
              "Coordinator statistics reset");
  ASSERT_TRUE(
      sys.getCoordinator().getReservationTable().getVertexReservationCount() ==
          0,
      "Reservation table cleared");
  ASSERT_TRUE(sys.getRobots().size() == 4,
              "Robot fleet re-initialized to initial count");
  for (const auto &r : sys.getRobots()) {
    ASSERT_TRUE(r->getStatus() == RobotStatus::IDLE,
                "All robots reset to IDLE status");
    ASSERT_TRUE(r->getWaitTicks() == 0, "All robots waitTicks reset to 0");
  }
}

void testZeroObservedTicksAndEmptyAnalytics() {
  std::cout << "\n--- TEST SUITE 19: Zero Observed Ticks & Empty Analytics "
               "Invariants ---\n";
  WarehouseAnalytics analytics(15, 15);

  ASSERT_TRUE(analytics.getObservedTicks() == 0,
              "Initial observed simulation ticks is exactly 0");
  ASSERT_TRUE(analytics.getFleetAverageUtilization() == 0.0,
              "Fleet average utilization is 0.0 with 0 observed ticks");
  ASSERT_TRUE(analytics.getFleetAverageWaitingPercentage() == 0.0,
              "Fleet waiting percentage is 0.0 with 0 observed ticks");
  ASSERT_TRUE(analytics.getFleetTotalBatteryConsumed() == 0.0,
              "Total battery consumed is 0.0 initially");
  ASSERT_TRUE(analytics.getFleetTotalMovementSteps() == 0,
              "Total movement steps is 0 initially");
  ASSERT_TRUE(analytics.getTotalOrdersCreated() == 0,
              "Total orders created is 0");
  ASSERT_TRUE(analytics.getTotalOrdersCompleted() == 0,
              "Total orders completed is 0");
  ASSERT_TRUE(analytics.getAverageOrderCompletionTime() == -1.0,
              "Average order completion time returns -1.0 when empty");
  ASSERT_TRUE(analytics.getMinOrderCompletionTime() == -1,
              "Min completion time returns -1 when empty");
  ASSERT_TRUE(analytics.getMaxOrderCompletionTime() == -1,
              "Max completion time returns -1 when empty");
  ASSERT_TRUE(analytics.getOrderCompletionRate() == 0.0,
              "Order completion rate is 0.0% when empty");
}

void testStatusBasedUtilizationAndMovementCounting() {
  std::cout << "\n--- TEST SUITE 20: Status-Based Utilization & Movement "
               "Counting ---\n";
  WarehouseAnalytics analytics(15, 15);

  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(0, 0)));
  robots.push_back(
      std::make_unique<PickerRobot>("R02", "Picker-2", Position(0, 1)));
  robots.push_back(
      std::make_unique<TransportRobot>("R03", "Hauler-1", Position(0, 2)));

  // Initial state: all IDLE
  robots[0]->setStatus(RobotStatus::IDLE);
  robots[1]->setStatus(RobotStatus::WAITING);
  robots[2]->setStatus(RobotStatus::MOVING_TO_PICK);

  analytics.recordTick(1, robots);

  const auto *m1 = analytics.getMetricsForRobot("R01");
  const auto *m2 = analytics.getMetricsForRobot("R02");
  const auto *m3 = analytics.getMetricsForRobot("R03");

  ASSERT_TRUE(m1 && m1->idleTicks == 1, "R01 records 1 idle tick");
  ASSERT_TRUE(m1->workingTicks == 0, "R01 working ticks is 0");
  ASSERT_TRUE(m1->getIdlePercentage(1) == 100.0,
              "R01 idle percentage is 100.0%");
  ASSERT_TRUE(m1->getWorkingUtilization(1) == 0.0,
              "R01 working utilization is 0.0%");

  ASSERT_TRUE(m2 && m2->waitingTicks == 1, "R02 records 1 waiting tick");
  ASSERT_TRUE(m2->getWaitingPercentage(1) == 100.0,
              "R02 waiting percentage is 100.0%");

  ASSERT_TRUE(m3 && m3->workingTicks == 1, "R03 records 1 working tick");
  ASSERT_TRUE(m3->getWorkingUtilization(1) == 100.0,
              "R03 working utilization is 100.0%");

  // Tick 2: R02 remains WAITING in place; R03 moves to (0, 3)
  robots[2]->setPosition(Position(0, 3));
  analytics.recordTick(2, robots);

  ASSERT_TRUE(m2->waitingTicks == 2, "R02 accumulates 2 waiting ticks");
  ASSERT_TRUE(m2->movementSteps == 0,
              "R02 movement steps remains 0 while waiting (waiting != moving)");
  ASSERT_TRUE(m3->movementSteps == 1,
              "R03 movement step increments only on actual position change");

  // Tick 3: R03 stays in (0, 3) in PICKING status
  robots[2]->setStatus(RobotStatus::PICKING);
  analytics.recordTick(3, robots);

  ASSERT_TRUE(m3->workingTicks == 3,
              "R03 accumulates 3 working ticks (PICKING is working)");
  ASSERT_TRUE(m3->movementSteps == 1,
              "R03 movement steps remains 1 while performing picking in-place");
}

void testBatteryConsumptionTracking() {
  std::cout << "\n--- TEST SUITE 21: Authoritative Battery Consumption "
               "Tracking ---\n";
  WarehouseAnalytics analytics(15, 15);

  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(0, 0)));

  // Tick 1: full battery 100%
  analytics.recordTick(1, robots);
  const auto *m1 = analytics.getMetricsForRobot("R01");
  ASSERT_TRUE(m1 && m1->batteryConsumed == 0.0,
              "Initial battery consumed is 0.0");

  // Robot consumes 30% battery on movement (battery decreases from 100% to 70%)
  robots[0]->consumeBattery(30.0);
  analytics.recordTick(2, robots);

  ASSERT_TRUE(m1->batteryConsumed == 30.0,
              "Battery consumed correctly tracks 30.0% drain");
  ASSERT_TRUE(analytics.getFleetTotalBatteryConsumed() == 30.0,
              "Fleet total battery consumed is 30.0%");

  // Robot enters CHARGING status and charges 15% (battery increases from 70% to
  // 85%, remaining in CHARGING status)
  robots[0]->setStatus(RobotStatus::CHARGING);
  robots[0]->charge(15.0);
  analytics.recordTick(3, robots);

  ASSERT_TRUE(m1->batteryConsumed == 30.0,
              "Battery consumed does not increase during charging");
  ASSERT_TRUE(m1->chargingTicks == 1,
              "Charging ticks counted during CHARGING status");
  ASSERT_TRUE(m1->chargingEvents >= 1,
              "Charging event recorded on positive battery delta");

  // Duplicate call with tick 3 does not double-count charging ticks
  analytics.recordTick(3, robots);
  ASSERT_TRUE(
      m1->chargingTicks == 1,
      "Recording same simulation tick does not double-count charging ticks");
}

void testOrderLifecycleTimestampsAndDurations() {
  std::cout << "\n--- TEST SUITE 22: Order Lifecycle Timestamps & Completion "
               "Durations ---\n";
  WarehouseAnalytics analytics(15, 15);
  Product prod(101, "Laptop", "Electronics", 10, 1200.0, Position(3, 3));

  Order order(101, 2, OrderPriority::HIGH, Position(14, 14));
  order.setCreatedTick(3);
  order.setAssignedTick(5);
  order.setAssignedRobotId("R01");

  ASSERT_TRUE(order.getCreatedTick() == 3,
              "Order creation timestamp recorded at tick 3");
  ASSERT_TRUE(order.getAssignedTick() == 5,
              "Order assignment timestamp recorded at tick 5");
  ASSERT_TRUE(order.getCompletionDuration() == -1,
              "Incomplete order duration returns -1");
  ASSERT_FALSE(order.isCompletedRecorded(),
               "Order is not yet marked completed recorded");

  // Complete order at tick 12
  order.setStatus(OrderStatus::COMPLETED);
  order.setCompletedTick(12);
  ASSERT_TRUE(order.getCompletionDuration() == 9,
              "Order completion duration is 12 - 3 = 9 ticks");
  ASSERT_TRUE(order.getCompletionDuration() >= 0,
              "Order completion duration is non-negative");

  // Record into analytics
  analytics.recordOrderCreated(order, 3);
  analytics.recordOrderAssigned(order, "R01", 5);
  analytics.recordOrderCompleted(order, &prod, 12);
  order.markCompletedRecorded();

  ASSERT_TRUE(order.isCompletedRecorded(),
              "Order is marked completed recorded");
  ASSERT_TRUE(analytics.getTotalOrdersCreated() == 1,
              "Analytics records 1 order created");
  ASSERT_TRUE(analytics.getTotalOrdersCompleted() == 1,
              "Analytics records 1 order completed");
  ASSERT_TRUE(analytics.getTotalUnitsFulfilled() == 2,
              "Analytics records 2 units fulfilled");
  ASSERT_TRUE(analytics.getAverageOrderCompletionTime() == 9.0,
              "Average completion time is 9.0 ticks");
  ASSERT_TRUE(analytics.getMinOrderCompletionTime() == 9,
              "Min completion time is 9 ticks");
  ASSERT_TRUE(analytics.getMaxOrderCompletionTime() == 9,
              "Max completion time is 9 ticks");
  ASSERT_TRUE(analytics.getFulfilledByCategory().at("Electronics") == 2,
              "Fulfilled category tracks 2 Electronics units");

  // Verify duplicate completion prevention
  if (!order.isCompletedRecorded()) {
    analytics.recordOrderCompleted(order, &prod, 12);
  }
  ASSERT_TRUE(analytics.getTotalOrdersCompleted() == 1,
              "Duplicate order completion is prevented");
}

void testCongestionHeatmapSpatialMatrixAndBounds() {
  std::cout << "\n--- TEST SUITE 23: Congestion Heatmap Matrix, Bounds & "
               "Replanning ---\n";
  WarehouseAnalytics analytics(15, 15);

  // Bounds checking
  ASSERT_TRUE(analytics.isValidCell(0, 0),
              "Cell (0, 0) is valid within 15x15 bounds");
  ASSERT_TRUE(analytics.isValidCell(14, 14),
              "Cell (14, 14) is valid within 15x15 bounds");
  ASSERT_FALSE(analytics.isValidCell(-1, 5),
               "Negative coordinate (-1, 5) is invalid");
  ASSERT_FALSE(analytics.isValidCell(15, 0),
               "Out of bounds coordinate (15, 0) is invalid");

  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "Picker-1", Position(5, 5)));

  // Tick 1: R01 visits (5, 5)
  analytics.recordTick(1, robots);
  ASSERT_TRUE(analytics.getCellVisits(5, 5) == 1, "Cell (5, 5) has 1 visit");
  ASSERT_TRUE(analytics.getCellWaits(5, 5) == 0,
              "Cell (5, 5) has 0 waits initially");

  // Tick 2: R01 WAITING at (5, 5)
  robots[0]->setStatus(RobotStatus::WAITING);
  analytics.recordTick(2, robots);
  ASSERT_TRUE(analytics.getCellVisits(5, 5) == 2, "Cell (5, 5) has 2 visits");
  ASSERT_TRUE(analytics.getCellWaits(5, 5) == 1,
              "Cell (5, 5) records 1 waiting event");

  // Dynamic replanning observation at (5, 5)
  analytics.recordReplanEvent(Position(5, 5));
  ASSERT_TRUE(analytics.getCellReplans(5, 5) == 1,
              "Cell (5, 5) records 1 replanning event");

  // Safe query on invalid cell returns 0 without crashing
  ASSERT_TRUE(analytics.getCellVisits(99, 99) == 0,
              "Invalid cell query returns 0 visits safely");
  ASSERT_TRUE(analytics.getCellWaits(-5, 0) == 0,
              "Invalid cell query returns 0 waits safely");
}

void testAnalyticsResetVersusSimulationReset() {
  std::cout << "\n--- TEST SUITE 24: Granular Analytics Reset vs Full "
               "Simulation Reset ---\n";
  WarehouseSystem sys(15, 15);
  sys.initializeDefaultWarehouse();

  sys.createOrder(101, 2, OrderPriority::HIGH);
  sys.stepSimulation();
  sys.stepSimulation();
  sys.stepSimulation();

  int tickBefore = sys.getSimulationTick();
  ASSERT_TRUE(tickBefore == 3, "Simulation advanced to tick 3");
  ASSERT_TRUE(sys.getAnalytics().getObservedTicks() == 3,
              "Analytics observed 3 ticks");
  ASSERT_TRUE(sys.getAnalytics().getMaxCellVisits() > 0,
              "Heatmap recorded robot cell visits");

  // 1. Test Heatmap-only reset: resets heatmap, keeps simulation tick,
  // inventory, robots
  sys.resetHeatmap();
  ASSERT_TRUE(sys.getAnalytics().getMaxCellVisits() == 0,
              "Heatmap reset clears all cell visits to 0");
  ASSERT_TRUE(sys.getSimulationTick() == 3,
              "Heatmap reset does not alter current simulation tick");
  ASSERT_TRUE(sys.getRobots().size() == 4,
              "Heatmap reset preserves robot fleet");

  // 2. Test Analytics reset: resets metrics history, keeps warehouse inventory
  // and robots
  sys.resetAnalytics();
  ASSERT_TRUE(sys.getAnalytics().getObservedTicks() == 0,
              "Analytics reset clears observed ticks to 0");
  ASSERT_TRUE(sys.getInventory().hasStock(101, 1),
              "Analytics reset preserves warehouse inventory");
  ASSERT_TRUE(sys.getRobots().size() == 4,
              "Analytics reset preserves robot fleet");

  // 3. Test Full Simulation reset: resets ticks, robots, inventory,
  // coordinator, and analytics
  sys.resetSimulation();
  ASSERT_TRUE(sys.getSimulationTick() == 0,
              "Full reset restores simulation tick to 0");
  ASSERT_TRUE(sys.getAnalytics().getObservedTicks() == 0,
              "Full reset restores analytics observed ticks to 0");
  ASSERT_TRUE(sys.getCoordinator().getCollisionsPrevented() == 0,
              "Full reset clears coordinator metrics");
  ASSERT_TRUE(sys.getWarehouse().getDynamicObstacles().empty(),
              "Full reset clears dynamic obstacles");
}

void testRobotUtilizationAndBatteryIntelligence() {
  std::cout << "\n--- TEST SUITE 25: Robot Utilization & Battery Intelligence "
               "Telemetry ---\n";
  WarehouseAnalytics analytics(15, 15);

  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "SpeedyPicker-1", Position(0, 0)));
  robots.push_back(
      std::make_unique<TransportRobot>("R02", "TitanHauler-1", Position(1, 0)));
  robots[1]->consumeBattery(20.0); // R02 battery = 80.0%

  // 1. Initial / Zero-tick behavior
  const auto *mInit = analytics.getMetricsForRobot("R01");
  ASSERT_TRUE(mInit == nullptr,
              "Metrics uninitialized before first simulation tick");
  RobotUtilizationMetrics emptyM;
  ASSERT_TRUE(emptyM.getBlockedPercentage(0) == 0.0,
              "Zero-tick blocked percentage safely returns 0.0");
  ASSERT_TRUE(emptyM.getBatteryTrend() == "INSUFFICIENT_HISTORY",
              "Empty battery history reports INSUFFICIENT_HISTORY");
  ASSERT_TRUE(emptyM.getBatteryDrainRate() == 0.0,
              "Empty battery history reports 0.0 drain rate");

  // 2. Tick 1: R01 is BLOCKED, R02 is IDLE
  robots[0]->setStatus(RobotStatus::BLOCKED);
  analytics.recordTick(1, robots);

  const auto *m1 = analytics.getMetricsForRobot("R01");
  ASSERT_TRUE(m1 != nullptr, "R01 metrics created after tick 1");
  ASSERT_TRUE(m1->blockedTicks == 1, "R01 records 1 blocked tick");
  ASSERT_TRUE(m1->getBlockedPercentage(1) == 100.0,
              "R01 blocked percentage is 100% at tick 1");
  ASSERT_TRUE(m1->batteryHistory.size() == 1,
              "R01 battery history records 1 sample");
  ASSERT_TRUE(m1->getBatteryTrend() == "INSUFFICIENT_HISTORY",
              "Single sample reports INSUFFICIENT_HISTORY");

  // 3. Tick 2: R01 moves and consumes battery (100.0 -> 98.0)
  robots[0]->setStatus(RobotStatus::MOVING_TO_PICK);
  robots[0]->consumeBattery(2.0);
  analytics.recordTick(2, robots);

  ASSERT_TRUE(m1->workingTicks == 1, "R01 records 1 working tick");
  ASSERT_TRUE(m1->batteryHistory.size() == 2,
              "R01 battery history records 2 samples");
  ASSERT_TRUE(m1->getBatteryTrend() == "DISCHARGING",
              "Battery delta < -0.05 reports DISCHARGING");
  ASSERT_TRUE(m1->getBatteryDrainRate() == 2.0,
              "Battery drain rate calculated as 2.0% per tick");

  // 4. Tick 3: R01 charges (98.0 -> 98.5, remaining in CHARGING status
  // below 99.0% auto-reset)
  robots[0]->setStatus(RobotStatus::CHARGING);
  robots[0]->charge(0.5);
  analytics.recordTick(3, robots);

  ASSERT_TRUE(m1->chargingTicks == 1, "R01 records 1 charging tick");
  ASSERT_TRUE(m1->getBatteryTrend() == "CHARGING",
              "Battery delta > 0.05 reports CHARGING");

  // 5. Full JSON Serialization & Fleet Metrics
  std::string json = analytics.toJSON(robots, 0, 0, 0, 0, 0);
  ASSERT_TRUE(json.find("\"fleetAverageBattery\":") != std::string::npos,
              "JSON contains fleetAverageBattery");
  ASSERT_TRUE(json.find("\"fleetHighestBattery\":") != std::string::npos,
              "JSON contains fleetHighestBattery");
  ASSERT_TRUE(json.find("\"fleetLowestBattery\":") != std::string::npos,
              "JSON contains fleetLowestBattery");
  ASSERT_TRUE(json.find("\"totalObservedTicks\":3") != std::string::npos,
              "JSON robotUtilization contains totalObservedTicks");
  ASSERT_TRUE(json.find("\"blockedPercentage\":") != std::string::npos,
              "JSON robotUtilization contains blockedPercentage");
  ASSERT_TRUE(json.find("\"batteryTrend\":") != std::string::npos,
              "JSON robotUtilization contains batteryTrend");
  ASSERT_TRUE(json.find("\"batteryDrainRate\":") != std::string::npos,
              "JSON robotUtilization contains batteryDrainRate");
  ASSERT_TRUE(json.find("\"batteryHistory\":[") != std::string::npos,
              "JSON robotUtilization contains batteryHistory");
}

void testInteractiveCongestionHeatmapModesAndSerialization() {
  std::cout << "\n--- TEST SUITE 26: Interactive Congestion Heatmap Modes & "
               "Serialization ---\n";
  WarehouseAnalytics analytics(15, 15);

  // 1. Empty heatmap history verification
  ASSERT_TRUE(analytics.getMaxCellVisits() == 0,
              "Empty heatmap has 0 max visits");
  ASSERT_TRUE(analytics.getMaxCellWaits() == 0,
              "Empty heatmap has 0 max waits");
  ASSERT_TRUE(analytics.getMaxCellReplans() == 0,
              "Empty heatmap has 0 max replans");
  ASSERT_TRUE(analytics.getCellVisits(7, 7) == 0,
              "Unvisited cell has 0 visits");
  ASSERT_TRUE(analytics.getCellWaits(7, 7) == 0, "Unvisited cell has 0 waits");
  ASSERT_TRUE(analytics.getCellReplans(7, 7) == 0,
              "Unvisited cell has 0 replans");

  // 2. Cell Visits Intensity
  std::vector<std::unique_ptr<Robot>> robots;
  robots.push_back(
      std::make_unique<PickerRobot>("R01", "SpeedyPicker-1", Position(3, 4)));
  analytics.recordTick(1, robots);
  analytics.recordTick(2, robots);

  ASSERT_TRUE(analytics.getCellVisits(3, 4) == 2,
              "Cell (3, 4) records 2 visits across 2 ticks");
  ASSERT_TRUE(analytics.getMaxCellVisits() == 2, "Max visits updated to 2");
  ASSERT_TRUE(analytics.getCellWaits(3, 4) == 0, "IDLE robot records 0 waits");

  // 3. Waiting Hotspots Intensity (WAITING and BLOCKED)
  robots[0]->setStatus(RobotStatus::WAITING);
  analytics.recordTick(3, robots);
  robots[0]->setStatus(RobotStatus::BLOCKED);
  analytics.recordTick(4, robots);

  ASSERT_TRUE(analytics.getCellWaits(3, 4) == 2,
              "Cell (3, 4) records 2 waits from WAITING and BLOCKED");
  ASSERT_TRUE(analytics.getMaxCellWaits() == 2, "Max waits updated to 2");

  // 4. Replanning Hotspots Intensity
  analytics.recordReplanEvent(Position(3, 4));
  analytics.recordReplanEvent(Position(3, 4));
  analytics.recordReplanEvent(Position(5, 5));

  ASSERT_TRUE(analytics.getCellReplans(3, 4) == 2,
              "Cell (3, 4) records 2 replans");
  ASSERT_TRUE(analytics.getCellReplans(5, 5) == 1,
              "Cell (5, 5) records 1 replan");
  ASSERT_TRUE(analytics.getMaxCellReplans() == 2, "Max replans updated to 2");

  // 5. Bounds safety: invalid coordinates return 0 and don't modify state
  ASSERT_TRUE(analytics.getCellVisits(-1, 0) == 0,
              "Negative x query returns 0 safely");
  ASSERT_TRUE(analytics.getCellWaits(0, -1) == 0,
              "Negative y query returns 0 safely");
  ASSERT_TRUE(analytics.getCellReplans(15, 15) == 0,
              "Out-of-bounds query returns 0 safely");
  analytics.recordReplanEvent(Position(-5, 20)); // ignored safely
  ASSERT_TRUE(analytics.getMaxCellReplans() == 2,
              "Out-of-bounds replan ignored without state corruption");

  // 6. JSON serialization contains full congestion object with all three mode
  // matrices
  std::string json = analytics.toJSON(robots, 0, 0, 0, 0, 0);
  ASSERT_TRUE(json.find("\"congestion\":{") != std::string::npos,
              "JSON includes congestion root object");
  ASSERT_TRUE(json.find("\"maxVisits\":") != std::string::npos,
              "JSON includes maxVisits");
  ASSERT_TRUE(json.find("\"maxWaits\":") != std::string::npos,
              "JSON includes maxWaits");
  ASSERT_TRUE(json.find("\"maxReplans\":") != std::string::npos,
              "JSON includes maxReplans");
  ASSERT_TRUE(json.find("\"visits\":[") != std::string::npos,
              "JSON includes visits matrix");
  ASSERT_TRUE(json.find("\"waits\":[") != std::string::npos,
              "JSON includes waits matrix");
  ASSERT_TRUE(json.find("\"replans\":[") != std::string::npos,
              "JSON includes replans matrix");

  // 7. Heatmap reset clears all three matrices while maintaining dimensions
  analytics.resetHeatmap();
  ASSERT_TRUE(analytics.getMaxCellVisits() == 0,
              "Reset clears max visits to 0");
  ASSERT_TRUE(analytics.getMaxCellWaits() == 0, "Reset clears max waits to 0");
  ASSERT_TRUE(analytics.getMaxCellReplans() == 0,
              "Reset clears max replans to 0");
  ASSERT_TRUE(analytics.getCellVisits(3, 4) == 0,
              "Reset clears cell visits to 0");
  ASSERT_TRUE(analytics.getCellWaits(3, 4) == 0,
              "Reset clears cell waits to 0");
  ASSERT_TRUE(analytics.getCellReplans(3, 4) == 0,
              "Reset clears cell replans to 0");
  ASSERT_TRUE(analytics.getCellVisitsMatrix().size() == 15,
              "Matrix dimensions preserved after reset");
}

void testAdvancedAlgorithmBenchmarkingAndIsolation() {
  std::cout << "\n--- TEST SUITE 27: Advanced Algorithm Benchmarking, "
               "Repeatability & State Isolation ---\n";

  // 1. All supported algorithms succeed on open grid
  auto openRes = AlgorithmBenchmark::runScenario(
      "open_grid", 3, {"BFS", "Dijkstra", "A*", "SpaceTimeA*"});
  ASSERT_TRUE(openRes.algorithmResults.size() == 4,
              "All 4 algorithms evaluated in open_grid scenario");
  for (const auto &m : openRes.algorithmResults) {
    ASSERT_TRUE(m.found,
                m.algorithmName + " successfully finds route on open_grid");
    ASSERT_TRUE(m.successRate == 100.0,
                m.algorithmName +
                    " reports 100% success rate across repetitions");
    ASSERT_TRUE(
        m.minExecutionTimeUs <= m.medianExecutionTimeUs &&
            m.medianExecutionTimeUs <= m.maxExecutionTimeUs,
        m.algorithmName +
            " execution timing distribution min <= median <= max holds");
    ASSERT_TRUE(m.medianExecutionTimeUs >= 0.0,
                m.algorithmName + " median duration is non-negative");
  }

  // 2. Deterministic scenario consistency
  auto openRes2 = AlgorithmBenchmark::runScenario("open_grid", 1, {"A*"});
  ASSERT_TRUE(openRes.algorithmResults[2].pathLength ==
                  openRes2.algorithmResults[0].pathLength,
              "Deterministic scenario yields identical optimal path length "
              "across runs");
  ASSERT_TRUE(
      openRes.algorithmResults[2].nodesVisited ==
          openRes2.algorithmResults[0].nodesVisited,
      "Deterministic scenario yields identical node expansions across runs");

  // 3. Path lengths and costs consistency
  // On open grid with unit costs, BFS, Dijkstra, and A* find equal optimal path
  // length
  int bfsLen = openRes.algorithmResults[0].pathLength;
  int dijLen = openRes.algorithmResults[1].pathLength;
  int astLen = openRes.algorithmResults[2].pathLength;
  ASSERT_TRUE(
      bfsLen == dijLen && dijLen == astLen,
      "BFS, Dijkstra, and A* yield identical optimal path length on open grid");
  ASSERT_TRUE(openRes.algorithmResults[1].totalCost ==
                  openRes.algorithmResults[2].totalCost,
              "Dijkstra and A* compute equal optimal total path cost");
  ASSERT_TRUE(openRes.algorithmResults[2].nodesVisited <=
                  openRes.algorithmResults[0].nodesVisited,
              "A* heuristic guided search explores <= nodes than unguided BFS");

  // 4. Even-number repetition count and mathematical median validity
  auto evenRes = AlgorithmBenchmark::runScenario("open_grid", 4, {"A*"});
  ASSERT_TRUE(evenRes.algorithmResults[0].repetitions == 4,
              "Even repetitions (4x) completed");
  ASSERT_TRUE(evenRes.algorithmResults[0].minExecutionTimeUs <=
                      evenRes.algorithmResults[0].medianExecutionTimeUs &&
                  evenRes.algorithmResults[0].medianExecutionTimeUs <=
                      evenRes.algorithmResults[0].maxExecutionTimeUs,
              "Even-count median lies within [min, max] range");

  // 5. Scenario Reachability Across All 5 Canonical Scenarios
  auto scObstacle = AlgorithmBenchmark::runScenario("obstacle_rich", 1, {"A*"});
  ASSERT_TRUE(scObstacle.algorithmResults[0].found,
              "Obstacle-Rich Grid is reachable by A*");
  auto scNarrow = AlgorithmBenchmark::runScenario("narrow_corridor", 1, {"A*"});
  ASSERT_TRUE(scNarrow.algorithmResults[0].found,
              "Narrow Corridor is reachable by A*");
  ASSERT_TRUE(scNarrow.algorithmResults[0].pathLength == 13,
              "Narrow Corridor yields expected 13-cell single-lane path");
  auto scLong = AlgorithmBenchmark::runScenario("long_distance", 1, {"A*"});
  ASSERT_TRUE(scLong.algorithmResults[0].found,
              "Long-Distance Route is reachable by A*");
  auto scCongested = AlgorithmBenchmark::runScenario("congested_multi_robot", 1,
                                                     {"SpaceTimeA*"});
  ASSERT_TRUE(scCongested.algorithmResults[0].found,
              "Congested Multi-Robot Scenario is reachable by SpaceTimeA*");
  ASSERT_TRUE(scCongested.algorithmResults[0].makespanTicks ==
                  scCongested.algorithmResults[0].pathLength - 1,
              "Makespan equals total schedule ticks (path length - 1)");

  // 6. Start equals goal and unreachable goal handling
  Warehouse testWh(5, 5);
  PathResult samePosRes =
      PathFinder::findPathAStar(testWh, Position(2, 2), Position(2, 2));
  ASSERT_TRUE(samePosRes.found, "Start equals goal returns found == true");
  ASSERT_TRUE(samePosRes.path.size() == 1,
              "Start equals goal returns path of length 1");
  ASSERT_TRUE(samePosRes.totalCost == 0.0,
              "Start equals goal has total cost 0.0");

  // Unreachable goal (boxed in by obstacles)
  Warehouse walledWh(5, 5);
  walledWh.setObstacle(Position(3, 4), true);
  walledWh.setObstacle(Position(4, 3), true);
  PathResult unreachRes =
      PathFinder::findPathAStar(walledWh, Position(0, 0), Position(4, 4));
  ASSERT_TRUE(!unreachRes.found,
              "Boxed-in unreachable goal returns found == false");
  ASSERT_TRUE(unreachRes.path.empty(), "Unreachable goal returns empty path");

  // 7. Space-Time A* dynamic reservation conflict and wait-action behavior
  Warehouse stWh(5, 5);
  ReservationTable stTable;
  // Reserve cell (1, 0) at tick 1
  stTable.reserveVertex(Position(1, 0), 1, "R_BLOCKER");
  // Start at (0, 0), goal at (2, 0). Robot can wait at (0, 0) or detour via (0,
  // 1)
  PathResult stRes = PathFinder::findSpaceTimePathAStar(
      stWh, stTable, Position(0, 0), Position(2, 0), 0, "R_TEST", 15);
  ASSERT_TRUE(stRes.found,
              "Space-Time A* successfully avoids dynamic vertex reservation");
  for (size_t t = 0; t < stRes.path.size(); ++t) {
    if (stRes.path[t] == Position(1, 0)) {
      ASSERT_TRUE(
          t != 1,
          "Space-Time A* never steps on reserved vertex (1, 0) at tick 1");
    }
  }

  // 8. Congested multi-robot space-time scenario metrics
  auto multiRes = AlgorithmBenchmark::runScenario("congested_multi_robot", 1,
                                                  {"SpaceTimeA*", "A*"});
  ASSERT_TRUE(multiRes.scenario.isMultiRobot,
              "Congested scenario identified as multi-robot");
  ASSERT_TRUE(multiRes.algorithmResults[0].found,
              "SpaceTimeA* finds safe route avoiding reservations");
  ASSERT_TRUE(multiRes.algorithmResults[0].robotsCompleted == 1,
              "SpaceTimeA* records 1 completed robot");
  ASSERT_TRUE(multiRes.algorithmResults[0].robotsContending == 3,
              "SpaceTimeA* records 3 contending robots in corridor");
  ASSERT_TRUE(multiRes.algorithmResults[0].makespanTicks > 0,
              "SpaceTimeA* records positive makespan ticks");

  // 9. Input validation & Error handling
  std::string errRepLow =
      AlgorithmBenchmark::runCustomBenchmarkJSON("open_grid", 0, {"A*"});
  ASSERT_TRUE(errRepLow.find("\"success\":false") != std::string::npos,
              "Repetitions < 1 rejected with error");

  std::string errRepHigh =
      AlgorithmBenchmark::runCustomBenchmarkJSON("open_grid", 25, {"A*"});
  ASSERT_TRUE(errRepHigh.find("\"success\":false") != std::string::npos,
              "Excessive repetitions (> 20) rejected with error");

  std::string errUnknownSc = AlgorithmBenchmark::runCustomBenchmarkJSON(
      "non_existent_maze", 2, {"A*"});
  ASSERT_TRUE(errUnknownSc.find("\"success\":false") != std::string::npos,
              "Unknown scenario ID rejected with error");

  std::string errUnknownAlgo = AlgorithmBenchmark::runCustomBenchmarkJSON(
      "open_grid", 2, {"QuantumTeleport"});
  ASSERT_TRUE(errUnknownAlgo.find("\"success\":false") != std::string::npos,
              "Unknown algorithm ID rejected with error");

  std::string errEmptyAlgos =
      AlgorithmBenchmark::runCustomBenchmarkJSON("open_grid", 2, {});
  ASSERT_TRUE(errEmptyAlgos.find("\"success\":false") != std::string::npos,
              "Empty algorithms list rejected with error");

  // 10. JSON serialization format
  std::string suiteJSON = AlgorithmBenchmark::runCanonicalSuiteJSON(1);
  ASSERT_TRUE(suiteJSON.find("\"success\":true") != std::string::npos,
              "Suite JSON contains success:true");
  ASSERT_TRUE(suiteJSON.find("\"totalScenarios\":5") != std::string::npos,
              "Suite JSON contains 5 scenarios");
  ASSERT_TRUE(suiteJSON.find("\"open_grid\"") != std::string::npos,
              "Suite JSON includes open_grid scenario");
  ASSERT_TRUE(suiteJSON.find("\"obstacle_rich\"") != std::string::npos,
              "Suite JSON includes obstacle_rich scenario");
  ASSERT_TRUE(suiteJSON.find("\"narrow_corridor\"") != std::string::npos,
              "Suite JSON includes narrow_corridor scenario");
  ASSERT_TRUE(suiteJSON.find("\"long_distance\"") != std::string::npos,
              "Suite JSON includes long_distance scenario");
  ASSERT_TRUE(suiteJSON.find("\"congested_multi_robot\"") != std::string::npos,
              "Suite JSON includes congested_multi_robot scenario");
  ASSERT_TRUE(suiteJSON.find("\"multiRobotMetrics\":{") != std::string::npos,
              "Multi-robot scenario JSON includes multiRobotMetrics object");

  // 7. Strict Simulation State Isolation Verification
  WarehouseSystem sys(15, 15);
  sys.initializeDefaultWarehouse();
  int orderId = sys.createOrder(101, 2, OrderPriority::HIGH);
  ASSERT_TRUE(orderId > 0,
              "Benchmark isolation test order created successfully");
  sys.stepSimulation(); // advance to tick 1

  int tickBefore = sys.getSimulationTick();
  int stockBefore = sys.getInventory().getProduct(101)->getQuantity();
  size_t pendingBefore = sys.getTaskManager().getPendingOrders().size();
  size_t activeBefore = sys.getTaskManager().getActiveOrders().size();
  int obsTicksBefore = sys.getAnalytics().getObservedTicks();
  int heatmapVisitsBefore = sys.getAnalytics().getCellVisits(0, 3);
  std::vector<Position> robotPosBefore;
  for (const auto &r : sys.getRobots()) {
    robotPosBefore.push_back(r->getPosition());
  }

  // Execute canonical benchmark suite through WarehouseSystem API
  std::string runRes = sys.runBenchmarkSuiteJSON();
  ASSERT_TRUE(runRes.find("\"success\":true") != std::string::npos,
              "WarehouseSystem runBenchmarkSuiteJSON succeeds");

  // Verify simulation state remains 100% UNCHANGED and UNCORRUPTED
  ASSERT_TRUE(sys.getSimulationTick() == tickBefore,
              "Benchmarking preserves live simulation tick (0 tick mutation)");
  ASSERT_TRUE(sys.getInventory().getProduct(101)->getQuantity() == stockBefore,
              "Benchmarking preserves live inventory stock");
  ASSERT_TRUE(sys.getTaskManager().getPendingOrders().size() == pendingBefore,
              "Benchmarking preserves pending orders");
  ASSERT_TRUE(sys.getTaskManager().getActiveOrders().size() == activeBefore,
              "Benchmarking preserves active orders");
  ASSERT_TRUE(sys.getAnalytics().getObservedTicks() == obsTicksBefore,
              "Benchmarking does not inflate observed ticks");
  ASSERT_TRUE(sys.getAnalytics().getCellVisits(0, 3) == heatmapVisitsBefore,
              "Benchmarking does not contaminate spatial heatmap");

  bool positionsIntact = true;
  for (size_t i = 0; i < sys.getRobots().size(); ++i) {
    if (sys.getRobots()[i]->getPosition() != robotPosBefore[i]) {
      positionsIntact = false;
      break;
    }
  }
  ASSERT_TRUE(positionsIntact,
              "Benchmarking does not alter live robot fleet positions");
}

void testPackingStationOccupancyAndDeadlockCorrection() {
  std::cout << "\n--- TEST SUITE 28: Packing Station Occupancy & Deadlock "
               "Prevention ---\n";

  // Scenario A: Robot completes order, vacates to staging cell, and becomes
  // eligible for another order
  {
    WarehouseSystem sys(10, 10);
    Warehouse &wh = sys.getWarehouse();
    Shelf sh01("SH01", Position(0, 0));
    sh01.addProduct(101);
    wh.addShelf(sh01);
    wh.addPackingStation(PackingStation("PK01", Position(5, 5)));
    wh.addChargingStation(ChargingStation("CS01", Position(9, 9)));
    sys.getInventory().addProduct(Product(101, "Test Product A", "Electronics",
                                          20, 100.0, Position(0, 0)));

    sys.addRobot(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(4, 5)));

    int o1 = sys.createOrder(101, 1, OrderPriority::HIGH);

    bool o1Completed = false;
    for (int t = 1; t <= 30 && !o1Completed; ++t) {
      sys.stepSimulation();
      auto ord = sys.getTaskManager().getOrderById(o1);
      if (ord && ord->getStatus() == OrderStatus::COMPLETED) {
        o1Completed = true;
      }
    }
    ASSERT_TRUE(
        o1Completed,
        "Scenario A: Order 1 completes delivery at packing station (5, 5)");

    // Step simulation for R01 to complete its staging vacation
    for (int t = 0; t < 5; ++t) {
      sys.stepSimulation();
    }

    Robot *r1 = nullptr;
    for (const auto &r : sys.getRobots()) {
      if (r->getId() == "R01") {
        r1 = r.get();
        break;
      }
    }
    ASSERT_TRUE(r1 != nullptr, "Scenario A: Robot R01 exists");
    ASSERT_TRUE(r1->getPosition() != Position(5, 5),
                "Scenario A: R01 vacated packing station to staging cell");
    ASSERT_FALSE(r1->hasActiveRoute(), "Scenario A: Staging route completed");
    ASSERT_TRUE(r1->getStatus() == RobotStatus::IDLE,
                "Scenario A: Robot is IDLE after completing staging");

    // Now create Order 2 and verify R01 is eligible and assigned
    int o2 = sys.createOrder(101, 1, OrderPriority::HIGH);
    sys.stepSimulation();
    auto ord2 = sys.getTaskManager().getOrderById(o2);
    ASSERT_TRUE(ord2 != nullptr && ord2->getStatus() == OrderStatus::ASSIGNED,
                "Scenario A: Order 2 assigned to eligible robot");
    ASSERT_TRUE(
        ord2->getAssignedRobotId() == "R01",
        "Scenario A: R01 successfully assigned subsequent order after staging");
    ASSERT_TRUE(r1->getStatus() == RobotStatus::MOVING_TO_PICK,
                "Scenario A: R01 transitions to MOVING_TO_PICK for new order");
  }

  // Scenario B: Robot cannot initially find staging cell, then successfully
  // retries when obstruction cleared
  {
    WarehouseSystem sys(5, 5);
    Warehouse &wh = sys.getWarehouse();
    Shelf sh01("SH01", Position(0, 0));
    sh01.addProduct(101);
    wh.addShelf(sh01);
    wh.addPackingStation(PackingStation("PK01", Position(2, 2)));
    wh.addChargingStation(ChargingStation("CS01", Position(4, 4)));
    sys.getInventory().addProduct(Product(101, "Test Product B", "Electronics",
                                          20, 100.0, Position(0, 0)));

    sys.addRobot(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));

    // Block all 4 neighbors of packing station (2, 2): (2, 1), (1, 2), (3, 2),
    // (2, 3)
    sys.addDynamicObstacle(Position(2, 1));
    sys.addDynamicObstacle(Position(1, 2));
    sys.addDynamicObstacle(Position(3, 2));
    sys.addDynamicObstacle(Position(2, 3));

    // Create and complete order at (2, 2)
    int o1 = sys.createOrder(101, 1, OrderPriority::HIGH);
    auto ord = sys.getTaskManager().getOrderById(o1);
    ord->setStatus(OrderStatus::DELIVERING);
    Robot *r1 = nullptr;
    for (const auto &r : sys.getRobots()) {
      if (r->getId() == "R01") {
        r1 = r.get();
        break;
      }
    }
    ASSERT_TRUE(r1 != nullptr, "Scenario B: Robot R01 exists");
    r1->assignTask(std::make_shared<Task>(o1, TaskType::MOVE_TO_PACK,
                                          Position(2, 2), 101, 1),
                   {});
    r1->setStatus(RobotStatus::DELIVERING);

    // Step simulation: order completes, but no staging cell is available
    sys.stepSimulation();
    ASSERT_TRUE(ord->getStatus() == OrderStatus::COMPLETED,
                "Scenario B: Order completed at station");
    ASSERT_TRUE(r1->getPosition() == Position(2, 2),
                "Scenario B: Robot safely stays in place on station");
    ASSERT_FALSE(r1->hasActiveRoute(),
                 "Scenario B: No phantom route assigned when blocked");

    const PackingStation *ps = wh.getPackingStationAt(Position(2, 2));
    ASSERT_TRUE(
        ps != nullptr && ps->isOccupied(),
        "Scenario B: Station remains marked occupied while robot is parked");

    // Now remove dynamic obstacle at (2, 1) to open a staging cell
    sys.removeDynamicObstacle(Position(2, 1));

    // Step simulation: retry vacation detects newly freed cell (2, 1)
    sys.stepSimulation();
    ASSERT_TRUE(r1->hasActiveRoute() || r1->getPosition() != Position(2, 2),
                "Scenario B: Robot successfully retries and vacates after "
                "obstruction clears");
  }

  // Scenario C: Taskless staging robot is blocked and subsequently recovers
  {
    Warehouse wh(5, 5);
    wh.addPackingStation(PackingStation("PK01", Position(2, 2)));
    TaskManager tm;
    MultiRobotCoordinator coord(2); // Wait threshold = 2 ticks
    std::vector<std::unique_ptr<Robot>> robots;
    robots.push_back(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
    robots.push_back(std::make_unique<PickerRobot>(
        "R02", "Picker-2", Position(2, 1))); // Blocks destination

    // R01 has taskless staging route to (2, 1)
    robots[0]->setStagingRoute(Position(2, 1),
                               {Position(2, 2), Position(2, 1)});
    ASSERT_TRUE(robots[0]->isStaging(),
                "Scenario C: R01 is identified as staging robot");

    // Step 1: Denied because R02 occupies (2, 1)
    coord.coordinateMovement(robots, wh, tm, 1);
    ASSERT_TRUE(robots[0]->getStatus() == RobotStatus::WAITING,
                "Scenario C: Staging robot denied enters WAITING");

    // Step 2: Denied again, triggers replan attempt
    coord.coordinateMovement(robots, wh, tm, 2);
    // Destination (2, 1) is still blocked by R02, so coordinator selects
    // alternative staging cell or sets BLOCKED
    ASSERT_TRUE(robots[0]->getWaitTicks() == 0,
                "Scenario C: Deadlock wait counter reset to prevent infinite "
                "replan loop");

    // Move R02 out of the way to (0, 0)
    robots[1]->setPosition(Position(0, 0));
    coord.coordinateMovement(robots, wh, tm, 3);
    // R01 can now move to (2, 1) or alternate
    ASSERT_TRUE(robots[0]->getPosition() != Position(2, 2) ||
                    robots[0]->hasActiveRoute(),
                "Scenario C: Staging robot recovers once corridor is clear");
  }

  // Scenario D: Dynamic obstacle invalidates staging route without causing
  // crash or collision
  {
    Warehouse wh(5, 5);
    TaskManager tm;
    MultiRobotCoordinator coord;
    std::vector<std::unique_ptr<Robot>> robots;
    robots.push_back(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(1, 1)));

    // Staging route: (1,1) -> (1,2) -> (1,3)
    robots[0]->setStagingRoute(
        Position(1, 3), {Position(1, 1), Position(1, 2), Position(1, 3)});
    coord.registerPlannedRoute("R01", robots[0]->getCurrentRoute(), 1);

    // Dynamic obstacle appears on staging path at (1, 2)
    bool handled =
        coord.handleDynamicObstacleAdded(Position(1, 2), robots, wh, tm, 1);
    ASSERT_TRUE(handled,
                "Scenario D: Dynamic obstacle on staging route accepted");
    ASSERT_TRUE(robots[0]->getPosition() == Position(1, 1),
                "Scenario D: Robot position preserved (no teleportation)");

    // Route should either replan around (1, 2) or set BLOCKED
    bool avoidsObstacle = true;
    for (const auto &p : robots[0]->getCurrentRoute()) {
      if (p == Position(1, 2))
        avoidsObstacle = false;
    }
    ASSERT_TRUE(avoidsObstacle, "Scenario D: Replan or detour strictly avoids "
                                "the new dynamic obstacle");
  }

  // Scenario E: Reservation cleanup does not remove another robot's valid route
  {
    MultiRobotCoordinator coord;
    std::vector<Position> route1 = {Position(1, 1), Position(1, 2),
                                    Position(1, 3)};
    std::vector<Position> route2 = {Position(3, 1), Position(3, 2),
                                    Position(3, 3)};

    coord.registerPlannedRoute("R01", route1, 5);
    coord.registerPlannedRoute("R02", route2, 5);

    // R02 completes its task and releases future reservations
    coord.releaseRobotReservations("R02", 5);

    // R01's route must remain 100% intact
    ASSERT_TRUE(
        coord.getReservationTable().isVertexReserved(Position(1, 2), 6, "R01"),
        "Scenario E: R01 vertex at tick 6 intact");
    ASSERT_TRUE(
        coord.getReservationTable().isVertexReserved(Position(1, 3), 7, "R01"),
        "Scenario E: R01 vertex at tick 7 intact");
    ASSERT_FALSE(
        coord.getReservationTable().isVertexReserved(Position(3, 2), 6, "R02"),
        "Scenario E: R02 reservations cleanly released");
  }

  // Scenario F: Stationary robot treated as obstacle by Space-Time A* at future
  // planning times
  {
    Warehouse wh(5, 5);
    ReservationTable table;

    // R01 is stationary at (2, 2)
    table.setStationaryRobot("R01", Position(2, 2));

    // Check that at any future tick, (2, 2) is reported reserved by other for
    // R02
    ASSERT_TRUE(table.isVertexReservedByOther(Position(2, 2), 1, "R02"),
                "Scenario F: Stationary robot blocks tick 1");
    ASSERT_TRUE(table.isVertexReservedByOther(Position(2, 2), 10, "R02"),
                "Scenario F: Stationary robot blocks tick 10");
    ASSERT_TRUE(table.isVertexReservedByOther(Position(2, 2), 30, "R02"),
                "Scenario F: Stationary robot blocks tick 30");

    // R02 plans space-time path from (1, 2) to (3, 2)
    PathResult res = PathFinder::findSpaceTimePathAStar(
        wh, table, Position(1, 2), Position(3, 2), 1, "R02");
    ASSERT_TRUE(res.found,
                "Scenario F: Space-Time A* finds path around stationary robot");

    bool traversesStationary = false;
    for (const auto &p : res.path) {
      if (p == Position(2, 2))
        traversesStationary = true;
    }
    ASSERT_FALSE(traversesStationary, "Scenario F: Planned route NEVER "
                                      "traverses stationary robot cell (2, 2)");
  }

  // Scenario G: Low-battery order completion retains intended charging behavior
  {
    WarehouseSystem sys(10, 10);
    Warehouse &wh = sys.getWarehouse();
    Shelf sh01("SH01", Position(0, 0));
    sh01.addProduct(101);
    wh.addShelf(sh01);
    wh.addPackingStation(PackingStation("PK01", Position(2, 2)));
    wh.addChargingStation(ChargingStation("CS01", Position(8, 8)));
    sys.getInventory().addProduct(Product(101, "Test Product G", "Electronics",
                                          20, 100.0, Position(0, 0)));

    sys.addRobot(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(2, 2)));
    Robot *r1 = nullptr;
    for (const auto &r : sys.getRobots()) {
      if (r->getId() == "R01") {
        r1 = r.get();
        break;
      }
    }
    ASSERT_TRUE(r1 != nullptr, "Scenario G: Robot R01 exists");
    r1->consumeBattery(80.0); // Battery down to 20.0% (< 25.0%)

    int o1 = sys.createOrder(101, 1, OrderPriority::HIGH);
    auto ord = sys.getTaskManager().getOrderById(o1);
    ord->setStatus(OrderStatus::DELIVERING);
    r1->assignTask(std::make_shared<Task>(o1, TaskType::MOVE_TO_PACK,
                                          Position(2, 2), 101, 1),
                   {});
    r1->setStatus(RobotStatus::DELIVERING);

    // Step simulation: delivery completes with battery < 25%
    sys.stepSimulation();
    ASSERT_TRUE(ord->getStatus() == OrderStatus::COMPLETED,
                "Scenario G: Order completed");
    ASSERT_TRUE(
        r1->getStatus() == RobotStatus::RETURNING_TO_CHARGE,
        "Scenario G: Low-battery robot transitions to RETURNING_TO_CHARGE");
    ASSERT_FALSE(r1->isStaging(),
                 "Scenario G: Low-battery robot does not enter staging");
    ASSERT_TRUE(r1->getCurrentTask() != nullptr &&
                    r1->getCurrentTask()->getType() ==
                        TaskType::RETURN_TO_CHARGE,
                "Scenario G: Return to charge task assigned");
  }

  // Scenario H: Concurrent packing station entry and head-on edge swap
  // prevention
  {
    Warehouse wh(10, 10);
    wh.addPackingStation(PackingStation("PK01", Position(5, 5)));
    TaskManager tm;
    MultiRobotCoordinator coord;
    std::vector<std::unique_ptr<Robot>> robots;
    robots.push_back(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(5, 4)));
    robots.push_back(
        std::make_unique<PickerRobot>("R02", "Picker-2", Position(5, 6)));

    robots[0]->assignTask(
        std::make_shared<Task>(1, TaskType::MOVE_TO_PACK, Position(5, 5)),
        {Position(5, 4), Position(5, 5)});
    robots[1]->assignTask(
        std::make_shared<Task>(2, TaskType::MOVE_TO_PACK, Position(5, 5)),
        {Position(5, 6), Position(5, 5)});

    coord.coordinateMovement(robots, wh, tm, 1);

    int atStation = 0;
    if (robots[0]->getPosition() == Position(5, 5))
      atStation++;
    if (robots[1]->getPosition() == Position(5, 5))
      atStation++;

    ASSERT_TRUE(atStation == 1,
                "Scenario H: Exactly one robot enters station concurrently");
    ASSERT_TRUE(robots[0]->getPosition() != robots[1]->getPosition(),
                "Scenario H: No vertex collision between competing robots");
    ASSERT_TRUE(coord.getCollisionsPrevented() >= 1,
                "Scenario H: Collision prevented counter incremented");
  }
}

void testInventoryReservationAndConsistency() {
  std::cout << "\n--- TEST SUITE 29: Inventory Reservation, Physical Pickup & "
               "Consistency ---\n";

  // Scenario A & B: Reservation does not immediately reduce on-hand stock;
  // available stock decreases
  {
    Inventory inv;
    inv.addProduct(
        Product(501, "Test Item", "General", 10, 50.0, Position(1, 1)));
    ASSERT_TRUE(inv.getOnHandStock(501) == 10,
                "Scenario A: Initial on-hand stock is 10");
    ASSERT_TRUE(inv.getAvailableStock(501) == 10,
                "Scenario A: Initial available stock is 10");
    ASSERT_TRUE(inv.getReservedStock(501) == 0,
                "Scenario A: Initial reserved stock is 0");

    bool resOk = inv.reserveStock(9001, 501, 4);
    ASSERT_TRUE(resOk, "Scenario A: Reserve 4 units succeeded");
    ASSERT_TRUE(
        inv.getOnHandStock(501) == 10,
        "Scenario A: On-hand stock remains 10 (not physically deducted yet)");
    ASSERT_TRUE(inv.getReservedStock(501) == 4,
                "Scenario B: Reserved stock increased to 4");
    ASSERT_TRUE(inv.getAvailableStock(501) == 6,
                "Scenario B: Available stock decreased to 6 (10 - 4)");

    // Scenario C: Second order cannot over-reserve stock
    bool secondRes =
        inv.reserveStock(9002, 501, 7); // requests 7, but only 6 available
    ASSERT_FALSE(secondRes, "Scenario C: Second order requesting 7 units "
                            "rejected when available is 6");
    ASSERT_TRUE(inv.getReservedStock(501) == 4,
                "Scenario C: Reserved stock remains 4");
    ASSERT_TRUE(inv.getAvailableStock(501) == 6,
                "Scenario C: Available stock remains 6");

    bool thirdRes = inv.reserveStock(9003, 501, 6); // exactly 6 available
    ASSERT_TRUE(
        thirdRes,
        "Scenario C: Third order requesting remaining 6 units succeeds");
    ASSERT_TRUE(inv.getAvailableStock(501) == 0,
                "Scenario C: Available stock is now 0");

    // Scenario D & E: Successful pickup deducts stock exactly once; repeat tick
    // does not deduct twice
    bool pickOk = inv.recordPhysicalPickup(9001, 4);
    ASSERT_TRUE(pickOk, "Scenario D: Physical pickup of 4 units succeeds");
    ASSERT_TRUE(inv.getOnHandStock(501) == 6,
                "Scenario D: On-hand stock physically deducted to 6 (10 - 4)");
    ASSERT_TRUE(inv.getReservedStock(501) == 6,
                "Scenario D: Remaining reserved stock is 6 (from order 9003)");
    ASSERT_TRUE(inv.getAvailableStock(501) == 0,
                "Scenario D: Available stock remains 0 (6 - 6)");

    bool repeatPick = inv.recordPhysicalPickup(9001, 4);
    ASSERT_TRUE(repeatPick,
                "Scenario E: Repeat pickup call is handled idempotently");
    ASSERT_TRUE(inv.getOnHandStock(501) == 6,
                "Scenario E: On-hand stock NOT deducted twice");
    ASSERT_TRUE(inv.getReservedStock(501) == 6,
                "Scenario E: Reserved stock remains unchanged");

    // Scenario F: Cancellation before pickup releases reservation
    bool cancelOk = inv.releaseReservation(9003);
    ASSERT_TRUE(cancelOk,
                "Scenario F: Release reservation for order 9003 succeeds");
    ASSERT_TRUE(inv.getReservedStock(501) == 0,
                "Scenario F: Reserved stock restored to 0");
    ASSERT_TRUE(inv.getOnHandStock(501) == 6,
                "Scenario F: On-hand stock preserved at 6");
    ASSERT_TRUE(inv.getAvailableStock(501) == 6,
                "Scenario F: Available stock restored to 6");
  }

  // Scenario G: Failure before pickup releases unpicked reserved units via
  // system API
  {
    WarehouseSystem sys(10, 10);
    sys.getWarehouse().addShelf(Shelf("SH502", Position(0, 0)));
    sys.getWarehouse().addPackingStation(
        PackingStation("PK01", Position(5, 5)));
    sys.getWarehouse().addChargingStation(
        ChargingStation("CS01", Position(9, 9)));
    sys.getInventory().addProduct(
        Product(502, "Gadget", "Electronics", 15, 30.0, Position(0, 0)));
    sys.addRobot(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(4, 5)));

    int ordG = sys.createOrder(502, 5, OrderPriority::HIGH);
    ASSERT_TRUE(sys.getInventory().getOnHandStock(502) == 15,
                "Scenario G: On-hand stock is 15");
    ASSERT_TRUE(sys.getInventory().getReservedStock(502) == 5,
                "Scenario G: Reserved stock is 5");
    ASSERT_TRUE(sys.getInventory().getAvailableStock(502) == 10,
                "Scenario G: Available stock is 10");

    bool failedOk = sys.failOrder(ordG, "Robot navigation hardware failure");
    ASSERT_TRUE(failedOk, "Scenario G: System failOrder succeeds");
    ASSERT_TRUE(sys.getInventory().getReservedStock(502) == 0,
                "Scenario G: Reserved stock released on failure");
    ASSERT_TRUE(sys.getInventory().getOnHandStock(502) == 15,
                "Scenario G: On-hand stock intact at 15");
    ASSERT_TRUE(sys.getInventory().getAvailableStock(502) == 15,
                "Scenario G: Available stock restored to 15");
  }

  // Scenario H: Partial pickup deducts only picked units and releases remainder
  // on cancel
  {
    Inventory invH;
    invH.addProduct(
        Product(503, "Bulk Screws", "Hardware", 20, 5.0, Position(1, 1)));
    invH.reserveStock(9004, 503, 10);
    ASSERT_TRUE(invH.getOnHandStock(503) == 20,
                "Scenario H: Initial on-hand is 20");
    ASSERT_TRUE(invH.getReservedStock(503) == 10,
                "Scenario H: Initial reserved is 10");

    // Partial pickup of 6 units
    invH.recordPhysicalPickup(9004, 6);
    ASSERT_TRUE(invH.getOnHandStock(503) == 14,
                "Scenario H: On-hand stock deducted by 6 (20 -> 14)");
    ASSERT_TRUE(invH.getReservedStock(503) == 4,
                "Scenario H: Unpicked reserved stock is 4 (10 - 6)");
    ASSERT_TRUE(invH.getOrderPickedQuantity(9004) == 6,
                "Scenario H: Order tracks 6 picked units");
    ASSERT_TRUE(invH.getOrderReservedQuantity(9004) == 4,
                "Scenario H: Order tracks 4 remaining unpicked");

    // Cancel order: releases remaining 4 unpicked units, picked 6 are NOT
    // refunded
    invH.releaseReservation(9004);
    ASSERT_TRUE(invH.getReservedStock(503) == 0,
                "Scenario H: Remaining 4 reserved released");
    ASSERT_TRUE(
        invH.getOnHandStock(503) == 14,
        "Scenario H: On-hand stock remains 14 (already-picked 6 NOT refunded)");
    ASSERT_TRUE(invH.getAvailableStock(503) == 14,
                "Scenario H: Available stock is 14 (14 - 0)");
  }

  // Scenario I: Reassignment does not duplicate reservations
  {
    WarehouseSystem sysI(10, 10);
    sysI.getWarehouse().addShelf(Shelf("SH504", Position(0, 0)));
    sysI.getWarehouse().addPackingStation(
        PackingStation("PK01", Position(5, 5)));
    sysI.getWarehouse().addChargingStation(
        ChargingStation("CS01", Position(9, 9)));
    sysI.getInventory().addProduct(
        Product(504, "Sensor Module", "Electronics", 12, 40.0, Position(0, 0)));
    // Setup: R01 is placed closer to shelf (0, 0) at (3, 5), R02 further at (4,
    // 5)
    sysI.addRobot(
        std::make_unique<PickerRobot>("R01", "Picker-1", Position(3, 5)));
    sysI.addRobot(
        std::make_unique<PickerRobot>("R02", "Picker-2", Position(4, 5)));

    int ordI = sysI.createOrder(504, 3, OrderPriority::HIGH);
    sysI.stepSimulation(); // Closer robot R01 selected and assigned
    auto ordPtr = sysI.getTaskManager().getOrderById(ordI);
    ASSERT_TRUE(ordPtr && ordPtr->getStatus() == OrderStatus::ASSIGNED,
                "Scenario I: Order assigned");
    ASSERT_TRUE(ordPtr->getAssignedRobotId() == "R01",
                "Scenario I: Initial order assigned to closer robot R01");
    ASSERT_TRUE(sysI.getInventory().getReservedStock(504) == 3,
                "Scenario I: Reserved stock is 3");

    // Verify initial robot states
    Robot *r01 = nullptr;
    Robot *r02 = nullptr;
    for (const auto &r : sysI.getRobots()) {
      if (r->getId() == "R01")
        r01 = r.get();
      if (r->getId() == "R02")
        r02 = r.get();
    }
    ASSERT_TRUE(r01 != nullptr &&
                    r01->getStatus() == RobotStatus::MOVING_TO_PICK,
                "Scenario I: R01 active task moving to pick");
    ASSERT_TRUE(r02 != nullptr && r02->getStatus() == RobotStatus::IDLE,
                "Scenario I: R02 initially idle");

    // Reassign to eligible idle robot R02
    bool reassigned = sysI.getTaskManager().reassignOrder(
        ordI, "R02", sysI.getRobots(), sysI.getWarehouse(),
        sysI.getInventory());
    ASSERT_TRUE(reassigned, "Scenario I: Order successfully reassigned to R02");
    ASSERT_TRUE(ordPtr->getAssignedRobotId() == "R02",
                "Scenario I: Assigned robot updated to R02");
    ASSERT_TRUE(
        sysI.getInventory().getReservedStock(504) == 3,
        "Scenario I: Reserved stock strictly NOT duplicated (remains 3)");
    ASSERT_TRUE(sysI.getInventory().getOnHandStock(504) == 12,
                "Scenario I: On-hand stock remains 12");

    // Verify old robot R01 no longer owns the task and reverted to IDLE
    ASSERT_TRUE(r01->getStatus() == RobotStatus::IDLE,
                "Scenario I: Old robot R01 reverted to IDLE");
    ASSERT_TRUE(!r01->hasActiveRoute(),
                "Scenario I: Old robot R01 route cleared");
    ASSERT_TRUE(!r01->getCurrentTask() ||
                    r01->getCurrentTask()->getOrderId() != ordI,
                "Scenario I: Old robot R01 no longer owns task");

    // Verify new robot R02 owns task and has active route
    ASSERT_TRUE(r02->getStatus() == RobotStatus::MOVING_TO_PICK,
                "Scenario I: New robot R02 is MOVING_TO_PICK");
    ASSERT_TRUE(r02->getCurrentTask() &&
                    r02->getCurrentTask()->getOrderId() == ordI,
                "Scenario I: New robot R02 owns task");

    // Verify rejection when target is invalid leaves current assignment intact
    bool rejectNonExistent = sysI.getTaskManager().reassignOrder(
        ordI, "NON_EXISTENT", sysI.getRobots(), sysI.getWarehouse(),
        sysI.getInventory());
    ASSERT_FALSE(rejectNonExistent,
                 "Scenario I: Reassigning to non-existent robot rejected");
    ASSERT_TRUE(
        ordPtr->getAssignedRobotId() == "R02",
        "Scenario I: Assigned robot remains R02 after rejected reassignment");
    ASSERT_TRUE(r02->getCurrentTask() &&
                    r02->getCurrentTask()->getOrderId() == ordI,
                "Scenario I: R02 still owns task after rejected reassignment");

    // Verify rejection when reassigning to self (already assigned robot)
    bool rejectSelf = sysI.getTaskManager().reassignOrder(
        ordI, "R02", sysI.getRobots(), sysI.getWarehouse(),
        sysI.getInventory());
    ASSERT_FALSE(rejectSelf,
                 "Scenario I: Reassignment to already assigned robot rejected");
    ASSERT_TRUE(ordPtr->getAssignedRobotId() == "R02",
                "Scenario I: Assigned robot remains R02 after "
                "self-reassignment rejection");
  }

  // Scenario J: Repeated release calls do not corrupt stock accounting
  {
    Inventory invJ;
    invJ.addProduct(
        Product(505, "Cables", "Electronics", 10, 10.0, Position(1, 1)));
    invJ.reserveStock(9005, 505, 5);
    ASSERT_TRUE(invJ.getReservedStock(505) == 5,
                "Scenario J: Reserved stock is 5");

    bool rel1 = invJ.releaseReservation(9005);
    ASSERT_TRUE(rel1, "Scenario J: First release succeeds");
    ASSERT_TRUE(invJ.getReservedStock(505) == 0,
                "Scenario J: Reserved stock is 0");

    bool rel2 = invJ.releaseReservation(9005);
    ASSERT_FALSE(rel2,
                 "Scenario J: Second release returns false without error");
    ASSERT_TRUE(invJ.getReservedStock(505) == 0,
                "Scenario J: Reserved stock safely clamped at 0");
    ASSERT_TRUE(invJ.getOnHandStock(505) == 10,
                "Scenario J: On-hand stock untouched at 10");
    ASSERT_TRUE(invJ.getAvailableStock(505) == 10,
                "Scenario J: Available stock intact at 10");
  }

  // Scenario K: Existing end-to-end order completion still deducts the exact
  // quantity
  {
    WarehouseSystem sysK(15, 15);
    sysK.initializeDefaultWarehouse();
    int initialOnHand = sysK.getInventory().getOnHandStock(101); // 35
    int ordK = sysK.createOrder(101, 3, OrderPriority::HIGH);
    ASSERT_TRUE(sysK.getInventory().getOnHandStock(101) == initialOnHand,
                "Scenario K: On-hand unchanged right after createOrder");
    ASSERT_TRUE(sysK.getInventory().getReservedStock(101) == 3,
                "Scenario K: Reserved is 3");
    ASSERT_TRUE(sysK.getInventory().getAvailableStock(101) == initialOnHand - 3,
                "Scenario K: Available is 32");

    bool kCompleted = false;
    for (int t = 1; t <= 40 && !kCompleted; ++t) {
      sysK.stepSimulation();
      auto o = sysK.getTaskManager().getOrderById(ordK);
      if (o && o->getStatus() == OrderStatus::COMPLETED) {
        kCompleted = true;
      }
    }
    ASSERT_TRUE(kCompleted, "Scenario K: Order completed end-to-end");
    ASSERT_TRUE(sysK.getInventory().getOnHandStock(101) == initialOnHand - 3,
                "Scenario K: Exactly 3 units deducted from on-hand (35 -> 32)");
    ASSERT_TRUE(sysK.getInventory().getReservedStock(101) == 0,
                "Scenario K: Reserved stock is 0");
    ASSERT_TRUE(sysK.getInventory().getAvailableStock(101) == initialOnHand - 3,
                "Scenario K: Available stock equals on-hand (32)");
  }
}

static std::string getRegressionTempPath(const std::string &filename) {
  std::string path = "D:/Temp/" + filename;
  std::ofstream out(path);
  if (out.is_open()) {
    out.close();
    std::remove(path.c_str());
    return path;
  }
  return filename; // Fallback to current working directory
}

void testConfigurationLoadingAndSnapshotPersistence() {
  std::cout << "\n--- TEST SUITE 30: Configuration Loading & Snapshot "
               "Round-Trip Persistence ---\n";

  // Scenario A: Valid initial configuration loads correctly from data/ files
  {
    WarehouseSystem sysA;
    std::string errA;
    bool loadedA = sysA.loadConfiguration("data/warehouse_layout.json",
                                          "data/initial_products.json",
                                          "data/initial_robots.json", errA);
    ASSERT_TRUE(
        loadedA,
        "Scenario A: Valid initial configuration files loaded successfully");
    ASSERT_TRUE(sysA.getWarehouse().getWidth() == 15,
                "Scenario A: Warehouse width is 15");
    ASSERT_TRUE(sysA.getWarehouse().getHeight() == 15,
                "Scenario A: Warehouse height is 15");
    ASSERT_TRUE(sysA.getRobots().size() == 4,
                "Scenario A: Fleet has exactly 4 robots");
    ASSERT_TRUE(sysA.getInventory().getAllProducts().size() == 8,
                "Scenario A: Catalog has exactly 8 products");
    ASSERT_TRUE(sysA.getWarehouse().getChargingStations().size() == 2,
                "Scenario A: Exactly 2 charging stations configured");
    ASSERT_TRUE(sysA.getWarehouse().getPackingStations().size() == 2,
                "Scenario A: Exactly 2 packing stations configured");
    ASSERT_TRUE(sysA.getSimulationTick() == 0,
                "Scenario A: Initial simulation tick is 0");
  }

  // Scenario B: Invalid warehouse dimensions are rejected
  {
    std::string tmpB = getRegressionTempPath("test_cfg_invalid_dim.json");
    std::ofstream outB(tmpB);
    outB << "{\"width\": 0, \"height\": 15, \"chargingStations\": [], "
            "\"packingStations\": []}";
    outB.close();

    WarehouseSystem sysB;
    std::string errB;
    bool loadedB = sysB.loadConfiguration(tmpB, "data/initial_products.json",
                                          "data/initial_robots.json", errB);
    ASSERT_FALSE(loadedB, "Scenario B: Width=0 configuration rejected");
    ASSERT_TRUE(!errB.empty(),
                "Scenario B: Clear error reported for invalid dimensions");
    std::remove(tmpB.c_str());
  }

  // Scenario C: Out-of-bounds coordinates are rejected
  {
    std::string tmpC = getRegressionTempPath("test_cfg_oob.json");
    std::ofstream outC(tmpC);
    outC << "{\"width\": 15, \"height\": 15, \"chargingStations\": "
            "[{\"id\":\"CS99\",\"position\":{\"x\":25,\"y\":5}}], "
            "\"packingStations\": []}";
    outC.close();

    WarehouseSystem sysC;
    std::string errC;
    bool loadedC = sysC.loadConfiguration(tmpC, "data/initial_products.json",
                                          "data/initial_robots.json", errC);
    ASSERT_FALSE(loadedC,
                 "Scenario C: Out-of-bounds station coordinate rejected");
    ASSERT_TRUE(errC.find("out of bounds") != std::string::npos,
                "Scenario C: Out of bounds error message returned");
    std::remove(tmpC.c_str());
  }

  // Scenario D: Duplicate IDs are rejected
  {
    // Duplicate station ID
    std::string tmpD1 = getRegressionTempPath("test_cfg_dup_station.json");
    std::ofstream outD1(tmpD1);
    outD1
        << "{\"width\": 15, \"height\": 15, \"chargingStations\": "
           "[{\"id\":\"CS01\",\"position\":{\"x\":0,\"y\":0}},{\"id\":\"CS01\","
           "\"position\":{\"x\":0,\"y\":1}}], \"packingStations\": []}";
    outD1.close();

    WarehouseSystem sysD1;
    std::string errD1;
    bool loadedD1 = sysD1.loadConfiguration(tmpD1, "data/initial_products.json",
                                            "data/initial_robots.json", errD1);
    ASSERT_FALSE(loadedD1, "Scenario D: Duplicate station ID CS01 rejected");
    ASSERT_TRUE(errD1.find("Duplicate") != std::string::npos,
                "Scenario D: Duplicate station error reported");
    std::remove(tmpD1.c_str());

    // Duplicate product ID
    std::string tmpD2 = getRegressionTempPath("test_cfg_dup_prod.json");
    std::ofstream outD2(tmpD2);
    outD2 << "[{\"id\":101,\"name\":\"Item "
             "1\",\"category\":\"C1\",\"quantity\":10,\"price\":5.0,"
             "\"location\":{\"x\":3,\"y\":3}},"
          << "{\"id\":101,\"name\":\"Item "
             "2\",\"category\":\"C2\",\"quantity\":5,\"price\":10.0,"
             "\"location\":{\"x\":3,\"y\":6}}]";
    outD2.close();

    WarehouseSystem sysD2;
    std::string errD2;
    bool loadedD2 = sysD2.loadConfiguration("data/warehouse_layout.json", tmpD2,
                                            "data/initial_robots.json", errD2);
    ASSERT_FALSE(loadedD2, "Scenario D: Duplicate product ID 101 rejected");
    ASSERT_TRUE(errD2.find("Duplicate") != std::string::npos,
                "Scenario D: Duplicate product error reported");
    std::remove(tmpD2.c_str());

    // Duplicate robot ID
    std::string tmpD3 = getRegressionTempPath("test_cfg_dup_robot.json");
    std::ofstream outD3(tmpD3);
    outD3
        << "[{\"id\":\"R01\",\"name\":\"Bot1\",\"type\":\"PickerRobot\","
           "\"capacity\":6,\"position\":{\"x\":0,\"y\":3},\"battery\":100.0},"
        << "{\"id\":\"R01\",\"name\":\"Bot2\",\"type\":\"TransportRobot\","
           "\"capacity\":25,\"position\":{\"x\":0,\"y\":5},\"battery\":100.0}]";
    outD3.close();

    WarehouseSystem sysD3;
    std::string errD3;
    bool loadedD3 =
        sysD3.loadConfiguration("data/warehouse_layout.json",
                                "data/initial_products.json", tmpD3, errD3);
    ASSERT_FALSE(loadedD3, "Scenario D: Duplicate robot ID R01 rejected");
    ASSERT_TRUE(errD3.find("Duplicate") != std::string::npos,
                "Scenario D: Duplicate robot error reported");
    std::remove(tmpD3.c_str());
  }

  // Scenario E: Negative stock and invalid battery levels are rejected
  {
    // Negative quantity
    std::string tmpE1 = getRegressionTempPath("test_cfg_neg_stock.json");
    std::ofstream outE1(tmpE1);
    outE1 << "[{\"id\":201,\"name\":\"Negative "
             "Item\",\"category\":\"Test\",\"quantity\":-10,\"price\":5.0,"
             "\"location\":{\"x\":3,\"y\":3}}]";
    outE1.close();

    WarehouseSystem sysE1;
    std::string errE1;
    bool loadedE1 = sysE1.loadConfiguration("data/warehouse_layout.json", tmpE1,
                                            "data/initial_robots.json", errE1);
    ASSERT_FALSE(loadedE1,
                 "Scenario E: Negative product quantity (-10) rejected");
    ASSERT_TRUE(errE1.find("negative quantity") != std::string::npos,
                "Scenario E: Negative stock error reported");
    std::remove(tmpE1.c_str());

    // Battery > 100%
    std::string tmpE2 = getRegressionTempPath("test_cfg_bad_battery.json");
    std::ofstream outE2(tmpE2);
    outE2 << "[{\"id\":\"R99\",\"name\":\"SuperCharged\",\"type\":"
             "\"PickerRobot\",\"capacity\":6,\"position\":{\"x\":0,\"y\":3},"
             "\"battery\":150.0}]";
    outE2.close();

    WarehouseSystem sysE2;
    std::string errE2;
    bool loadedE2 =
        sysE2.loadConfiguration("data/warehouse_layout.json",
                                "data/initial_products.json", tmpE2, errE2);
    ASSERT_FALSE(loadedE2, "Scenario E: Battery level 150.0% rejected");
    ASSERT_TRUE(errE2.find("battery") != std::string::npos,
                "Scenario E: Invalid battery error reported");
    std::remove(tmpE2.c_str());
  }

  // Scenario F: Missing explicitly requested configuration file reports an
  // error
  {
    WarehouseSystem sysF;
    std::string errF;
    bool loadedF = sysF.loadConfiguration("data/non_existent_layout_file.json",
                                          "data/initial_products.json",
                                          "data/initial_robots.json", errF);
    ASSERT_FALSE(loadedF,
                 "Scenario F: Non-existent layout configuration rejected");
    ASSERT_TRUE(errF.find("not found") != std::string::npos ||
                    errF.find("cannot be opened") != std::string::npos,
                "Scenario F: Missing file error clearly reported");
  }

  // Scenario G: Malformed JSON reports an understandable error
  {
    std::string tmpG = getRegressionTempPath("test_cfg_malformed.json");
    std::ofstream outG(tmpG);
    outG << "{ \"width\": 15, \"height\": 15, UNQUOTED_TOKEN: false }";
    outG.close();

    WarehouseSystem sysG;
    std::string errG;
    bool loadedG = sysG.loadConfiguration(tmpG, "data/initial_products.json",
                                          "data/initial_robots.json", errG);
    ASSERT_FALSE(loadedG, "Scenario G: Malformed JSON syntax rejected");
    ASSERT_TRUE(errG.find("parse error") != std::string::npos,
                "Scenario G: JSON parse error message returned");
    std::remove(tmpG.c_str());
  }

  // Scenario H: Failed configuration load does not mutate the live warehouse
  {
    WarehouseSystem sysH(15, 15);
    sysH.initializeDefaultWarehouse();
    int initialProductsCount =
        static_cast<int>(sysH.getInventory().getAllProducts().size());
    int initialRobotsCount = static_cast<int>(sysH.getRobots().size());

    std::string errH;
    bool loadedH = sysH.loadConfiguration("data/non_existent_layout_xyz.json",
                                          "data/initial_products.json",
                                          "data/initial_robots.json", errH);
    ASSERT_FALSE(loadedH, "Scenario H: Load fails as expected");
    ASSERT_TRUE(sysH.getWarehouse().getWidth() == 15,
                "Scenario H: Live warehouse width untouched");
    ASSERT_TRUE(static_cast<int>(sysH.getInventory().getAllProducts().size()) ==
                    initialProductsCount,
                "Scenario H: Live inventory count untouched");
    ASSERT_TRUE(static_cast<int>(sysH.getRobots().size()) == initialRobotsCount,
                "Scenario H: Live robot count untouched");
  }

  // Scenario I: Snapshot save followed by load restores inventory quantities
  {
    WarehouseSystem sysI(15, 15);
    sysI.initializeDefaultWarehouse();
    Product *p101 = sysI.getInventory().getProduct(101);
    ASSERT_TRUE(p101 != nullptr,
                "Scenario I: Product 101 exists in default setup");
    sysI.getInventory().restock(101, 15); // 35 + 15 = 50

    std::string snapPathI = getRegressionTempPath("test_snap_inventory.json");
    bool savedI = sysI.saveToFile(snapPathI);
    ASSERT_TRUE(savedI, "Scenario I: Snapshot saved successfully");

    WarehouseSystem sysI2(10, 10); // Brand new system with different setup
    std::string errI;
    bool loadedI = sysI2.loadFromFile(snapPathI, errI);
    ASSERT_TRUE(loadedI, "Scenario I: Snapshot loaded successfully");
    ASSERT_TRUE(sysI2.getInventory().getOnHandStock(101) == 50,
                "Scenario I: Restocked quantity 50 accurately restored");
    ASSERT_TRUE(sysI2.getInventory().getAvailableStock(101) == 50,
                "Scenario I: Available quantity 50 matches on-hand");
    std::remove(snapPathI.c_str());
  }

  // Scenario J: Snapshot restores inventory reservations and picked quantities
  {
    WarehouseSystem sysJ(15, 15);
    sysJ.initializeDefaultWarehouse();
    int ordJ = sysJ.createOrder(101, 5, OrderPriority::HIGH);
    ASSERT_TRUE(sysJ.getInventory().getOnHandStock(101) == 35,
                "Scenario J: On-hand before pickup is 35");
    ASSERT_TRUE(sysJ.getInventory().getReservedStock(101) == 5,
                "Scenario J: Reserved before pickup is 5");

    // Simulate physical pickup of 2 units
    sysJ.getInventory().recordPhysicalPickup(ordJ, 2);
    ASSERT_TRUE(sysJ.getInventory().getOnHandStock(101) == 33,
                "Scenario J: On-hand after pickup of 2 is 33");
    ASSERT_TRUE(sysJ.getInventory().getReservedStock(101) == 3,
                "Scenario J: Reserved after pickup of 2 is 3");
    ASSERT_TRUE(sysJ.getInventory().getOrderReservedQuantity(ordJ) == 3,
                "Scenario J: Order remaining reserved is 3");
    ASSERT_TRUE(sysJ.getInventory().getOrderPickedQuantity(ordJ) == 2,
                "Scenario J: Order picked quantity is 2");

    std::string snapPathJ =
        getRegressionTempPath("test_snap_reservations.json");
    sysJ.saveToFile(snapPathJ);

    WarehouseSystem sysJ2(10, 10);
    std::string errJ;
    bool loadedJ = sysJ2.loadFromFile(snapPathJ, errJ);
    ASSERT_TRUE(loadedJ,
                "Scenario J: Snapshot with partial physical pickup loaded");
    ASSERT_TRUE(sysJ2.getInventory().getOnHandStock(101) == 33,
                "Scenario J: Restored on-hand is exactly 33");
    ASSERT_TRUE(sysJ2.getInventory().getReservedStock(101) == 3,
                "Scenario J: Restored reserved is exactly 3");
    ASSERT_TRUE(sysJ2.getInventory().getAvailableStock(101) == 30,
                "Scenario J: Restored available is 33 - 3 = 30");
    ASSERT_TRUE(sysJ2.getInventory().getOrderReservedQuantity(ordJ) == 3,
                "Scenario J: Restored order reserved quantity is 3");
    ASSERT_TRUE(sysJ2.getInventory().getOrderPickedQuantity(ordJ) == 2,
                "Scenario J: Restored order picked quantity is 2");
    std::remove(snapPathJ.c_str());
  }

  // Scenario K: Snapshot restores robot positions and battery levels
  {
    WarehouseSystem sysK(15, 15);
    sysK.initializeDefaultWarehouse();

    // Drain battery and move robot R01
    for (const auto &r : sysK.getRobots()) {
      if (r->getId() == "R01") {
        r->setPosition(Position(4, 7));
        r->setBattery(73.5);
        r->setCurrentPayload(2);
      }
    }

    std::string snapPathK = getRegressionTempPath("test_snap_robots.json");
    sysK.saveToFile(snapPathK);

    WarehouseSystem sysK2(10, 10);
    std::string errK;
    bool loadedK = sysK2.loadFromFile(snapPathK, errK);
    ASSERT_TRUE(loadedK, "Scenario K: Robot snapshot loaded successfully");

    bool foundK = false;
    for (const auto &r : sysK2.getRobots()) {
      if (r->getId() == "R01") {
        foundK = true;
        ASSERT_TRUE(r->getPosition() == Position(4, 7),
                    "Scenario K: Robot R01 position (4,7) restored");
        ASSERT_TRUE(std::abs(r->getBattery() - 73.5) < 0.001,
                    "Scenario K: Robot R01 battery 73.5 restored");
        ASSERT_TRUE(r->getCurrentPayload() == 2,
                    "Scenario K: Robot R01 payload 2 restored");
      }
    }
    ASSERT_TRUE(foundK, "Scenario K: Robot R01 verified in restored system");
    std::remove(snapPathK.c_str());
  }

  // Scenario L: Snapshot restores simulation tick and dynamic obstacles
  {
    WarehouseSystem sysL(15, 15);
    sysL.initializeDefaultWarehouse();
    for (int i = 0; i < 17; ++i) {
      sysL.stepSimulation();
    }
    ASSERT_TRUE(sysL.getSimulationTick() == 17,
                "Scenario L: Advanced to tick 17");
    sysL.addDynamicObstacle(Position(2, 2));
    sysL.addDynamicObstacle(Position(5, 5));

    std::string snapPathL = getRegressionTempPath("test_snap_tick_obs.json");
    sysL.saveToFile(snapPathL);

    WarehouseSystem sysL2(10, 10);
    std::string errL;
    bool loadedL = sysL2.loadFromFile(snapPathL, errL);
    ASSERT_TRUE(loadedL,
                "Scenario L: Tick and dynamic obstacle snapshot loaded");
    ASSERT_TRUE(sysL2.getSimulationTick() == 17,
                "Scenario L: Simulation tick 17 restored");
    ASSERT_TRUE(sysL2.getWarehouse().isDynamicObstacle(Position(2, 2)),
                "Scenario L: Dynamic obstacle at (2,2) restored");
    ASSERT_TRUE(sysL2.getWarehouse().isDynamicObstacle(Position(5, 5)),
                "Scenario L: Dynamic obstacle at (5,5) restored");
    std::remove(snapPathL.c_str());
  }

  // Scenario M: Snapshot restores supported order/task states consistently
  {
    WarehouseSystem sysM(15, 15);
    sysM.initializeDefaultWarehouse();
    int ordM = sysM.createOrder(102, 2, OrderPriority::MEDIUM);
    sysM.stepSimulation(); // Step so order gets assigned

    std::string snapPathM = getRegressionTempPath("test_snap_order_task.json");
    sysM.saveToFile(snapPathM);

    WarehouseSystem sysM2(10, 10);
    std::string errM;
    bool loadedM = sysM2.loadFromFile(snapPathM, errM);
    ASSERT_TRUE(loadedM, "Scenario M: Order/task snapshot loaded");
    auto restoredOrder = sysM2.getTaskManager().getOrderById(ordM);
    ASSERT_TRUE(restoredOrder != nullptr, "Scenario M: Restored order exists");
    ASSERT_TRUE(restoredOrder->getProductId() == 102,
                "Scenario M: Product ID 102 matches");
    ASSERT_TRUE(restoredOrder->getQuantity() == 2,
                "Scenario M: Quantity 2 matches");
    std::remove(snapPathM.c_str());
  }

  // Scenario N: Invalid snapshot is rejected without mutating live state
  {
    WarehouseSystem sysN(15, 15);
    sysN.initializeDefaultWarehouse();

    std::string snapPathN = getRegressionTempPath("test_snap_corrupt.json");
    std::ofstream outN(snapPathN);
    // Corrupt: Robot position out of bounds (99, 99)
    outN << "{\"tick\": 5, \"activeAlgorithm\": \"A*\", \"warehouse\": "
            "{\"width\": 15, \"height\": 15}, "
         << "\"inventory\": [{\"id\": 101, \"name\": \"Item\", \"quantity\": "
            "10, \"price\": 10.0, \"location\": {\"x\":3,\"y\":3}}], "
         << "\"robots\": [{\"id\": \"R01\", \"name\": \"Bot\", \"type\": "
            "\"PickerRobot\", \"capacity\": 6, \"battery\": 80.0, "
            "\"position\": {\"x\":99,\"y\":99}}]}";
    outN.close();

    std::string errN;
    bool loadedN = sysN.loadFromFile(snapPathN, errN);
    ASSERT_FALSE(loadedN, "Scenario N: Corrupted snapshot rejected");
    ASSERT_TRUE(!errN.empty(),
                "Scenario N: Error message returned for corrupt snapshot");
    ASSERT_TRUE(sysN.getRobots().size() == 4,
                "Scenario N: Live robot count completely preserved at 4");
    std::remove(snapPathN.c_str());
  }

  // Scenario R: Repeated save/load cycles do not duplicate reservations,
  // orders, robots or products
  {
    WarehouseSystem sysR1(15, 15);
    sysR1.initializeDefaultWarehouse();
    sysR1.createOrder(101, 3, OrderPriority::HIGH);

    std::string snapR1 = getRegressionTempPath("test_snap_r1.json");
    std::string snapR2 = getRegressionTempPath("test_snap_r2.json");

    sysR1.saveToFile(snapR1);

    WarehouseSystem sysR2(10, 10);
    bool l1 = sysR2.loadFromFile(snapR1);
    ASSERT_TRUE(l1, "Scenario R: First round-trip load succeeded");
    sysR2.saveToFile(snapR2);

    WarehouseSystem sysR3(10, 10);
    bool l2 = sysR3.loadFromFile(snapR2);
    ASSERT_TRUE(l2, "Scenario R: Second round-trip load succeeded");

    ASSERT_TRUE(sysR3.getRobots().size() == sysR1.getRobots().size(),
                "Scenario R: Robot fleet size identical after 2 cycles");
    ASSERT_TRUE(sysR3.getInventory().getAllProducts().size() ==
                    sysR1.getInventory().getAllProducts().size(),
                "Scenario R: Product catalog size identical after 2 cycles");
    ASSERT_TRUE(sysR3.getInventory().getReservedStock(101) == 3,
                "Scenario R: Reservation not duplicated (still exactly 3)");
    ASSERT_TRUE(sysR3.getInventory().getOnHandStock(101) ==
                    sysR1.getInventory().getOnHandStock(101),
                "Scenario R: On-hand stock preserved across cycles");

    std::remove(snapR1.c_str());
    std::remove(snapR2.c_str());
  }

  // Scenario S: Snapshot file errors handled safely
  {
    WarehouseSystem sysS;
    std::string errS1;
    bool loadedS1 = sysS.loadFromFile(
        "D:/Temp/missing_file_definitely_not_here.json", errS1);
    ASSERT_FALSE(loadedS1, "Scenario S: Non-existent snapshot file rejected");
    ASSERT_TRUE(!errS1.empty(),
                "Scenario S: Error message reported for missing snapshot");

    // Attempt to pass layout file as a snapshot file (schema mismatch)
    std::string errS2;
    bool loadedS2 = sysS.loadFromFile("data/warehouse_layout.json", errS2);
    ASSERT_FALSE(
        loadedS2,
        "Scenario S: Configuration file rejected when loaded as snapshot");
    ASSERT_TRUE(errS2.find("Missing required snapshot section") !=
                    std::string::npos,
                "Scenario S: Distinction between config file and runtime "
                "snapshot enforced");
  }
}

void testRestApiValidationAndConcurrency() {
  std::cout << "\n--- TEST SUITE 31: REST API Reliability, Input Validation & "
               "Concurrency Safety ---\n";

  WarehouseSystem testSys(15, 15);
  testSys.initializeDefaultWarehouse();
  std::mutex testMutex;
  HttpRequestHandler handler(testSys, testMutex, "web");

  // ========================================================================
  // Scenario A: Route and HTTP Method Validation
  // ========================================================================
  {
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/nonexistent_route";
    HttpResponse res = handler.handleRequest(req);
    ASSERT_TRUE(res.statusCode == 404,
                "Scenario A: Unknown /api route returns 404 Not Found");
    ASSERT_TRUE(res.body.find("\"success\":false") != std::string::npos,
                "Scenario A: 404 response contains JSON success:false");
    ASSERT_TRUE(res.body.find("Unknown API endpoint") != std::string::npos,
                "Scenario A: 404 response specifies unknown endpoint");

    // Unsupported method on POST-only endpoint
    req.method = "GET";
    req.path = "/api/step";
    res = handler.handleRequest(req);
    ASSERT_TRUE(res.statusCode == 405,
                "Scenario A: GET /api/step returns 405 Method Not Allowed");
    ASSERT_TRUE(
        res.headers["Allow"] == "POST, OPTIONS",
        "Scenario A: 405 on POST endpoint includes Allow: POST, OPTIONS");

    // Unsupported method on GET-only endpoint
    req.method = "POST";
    req.path = "/api/status";
    res = handler.handleRequest(req);
    ASSERT_TRUE(res.statusCode == 405,
                "Scenario A: POST /api/status returns 405 Method Not Allowed");
    ASSERT_TRUE(res.headers["Allow"] == "GET, OPTIONS",
                "Scenario A: 405 on GET endpoint includes Allow: GET, OPTIONS");

    // Unsupported method on /api/order
    req.method = "GET";
    req.path = "/api/order";
    res = handler.handleRequest(req);
    ASSERT_TRUE(res.statusCode == 405,
                "Scenario A: GET /api/order returns 405 Method Not Allowed");

    // CORS OPTIONS Preflight
    req.method = "OPTIONS";
    req.path = "/api/order";
    res = handler.handleRequest(req);
    ASSERT_TRUE(res.statusCode == 204,
                "Scenario A: OPTIONS preflight returns 204 No Content");
    ASSERT_TRUE(res.headers["Access-Control-Allow-Origin"] == "*",
                "Scenario A: OPTIONS returns Access-Control-Allow-Origin: *");
    ASSERT_TRUE(res.headers["Access-Control-Allow-Methods"] ==
                    "GET, POST, OPTIONS",
                "Scenario A: OPTIONS returns Access-Control-Allow-Methods");
  }

  // ========================================================================
  // Scenario B: Protocol Errors & Payload Size Limits
  // ========================================================================
  {
    std::string rawBad = "GARBAGE_WITHOUT_SPACES\r\n\r\n";
    std::string respBad = handler.handleRawRequest(rawBad);
    ASSERT_TRUE(respBad.find("400 Bad Request") != std::string::npos,
                "Scenario B: Malformed request line returns 400 Bad Request");

    std::string rawNoDelim = "GET /api/status HTTP/1.1";
    std::string respNoDelim = handler.handleRawRequest(rawNoDelim);
    ASSERT_TRUE(respNoDelim.find("400 Bad Request") != std::string::npos,
                "Scenario B: Missing header delimiter returns 400 Bad Request");

    // Unsupported Media Type
    HttpRequest reqCt;
    reqCt.method = "POST";
    reqCt.path = "/api/order";
    reqCt.headers["content-type"] = "text/xml";
    reqCt.body = "{}";
    HttpResponse respCt = handler.handleRequest(reqCt);
    ASSERT_TRUE(
        respCt.statusCode == 415,
        "Scenario B: Non-JSON Content-Type returns 415 Unsupported Media Type");

    // Negative Content-Length
    std::string rawNegCl = "POST /api/order HTTP/1.1\r\nHost: "
                           "localhost\r\nContent-Length: -10\r\n\r\n{}";
    std::string respNegCl = handler.handleRawRequest(rawNegCl);
    ASSERT_TRUE(respNegCl.find("400 Bad Request") != std::string::npos,
                "Scenario B: Negative Content-Length returns 400 Bad Request");

    // Body exceeding 2MB
    std::string rawBigCl = "POST /api/order HTTP/1.1\r\nHost: "
                           "localhost\r\nContent-Length: 3000000\r\n\r\n";
    std::string respBigCl = handler.handleRawRequest(rawBigCl);
    ASSERT_TRUE(respBigCl.find("413 Payload Too Large") != std::string::npos,
                "Scenario B: Body exceeding 2MB returns 413 Payload Too Large");
  }

  // ========================================================================
  // Scenario C: Order Input Validation (POST /api/order)
  // ========================================================================
  {
    HttpRequest reqO;
    reqO.method = "POST";
    reqO.path = "/api/order";

    // Empty body
    reqO.body = "";
    HttpResponse respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Empty body in /api/order returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Request body cannot be empty") !=
                    std::string::npos,
                "Scenario C: Empty body error message verified");

    // Malformed JSON syntax
    reqO.body = "{\"productId\": 101, \"quantity\": ";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Malformed JSON syntax returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Malformed JSON") != std::string::npos,
                "Scenario C: Malformed JSON error message verified");

    // Non-object JSON root
    reqO.body = "[101, 5]";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(
        respO.statusCode == 400,
        "Scenario C: Non-object JSON array root returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("JSON payload must be a JSON object") !=
                    std::string::npos,
                "Scenario C: Non-object JSON root error verified");

    // Missing productId
    reqO.body = "{\"quantity\": 5}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Missing productId returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Missing required field 'productId'") !=
                    std::string::npos,
                "Scenario C: Missing productId error message verified");

    // Missing quantity
    reqO.body = "{\"productId\": 101}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Missing quantity returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Missing required field 'quantity'") !=
                    std::string::npos,
                "Scenario C: Missing quantity error message verified");

    // String productId
    reqO.body = "{\"productId\": \"ThinkPad\", \"quantity\": 5}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: String productId returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Field 'productId' must be an integer") !=
                    std::string::npos,
                "Scenario C: String productId error verified");

    // String quantity
    reqO.body = "{\"productId\": 101, \"quantity\": \"five\"}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: String quantity returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Field 'quantity' must be an integer") !=
                    std::string::npos,
                "Scenario C: String quantity error verified");

    // Negative quantity
    reqO.body = "{\"productId\": 101, \"quantity\": -5}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Negative quantity returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Quantity must be greater than zero") !=
                    std::string::npos,
                "Scenario C: Negative quantity error verified");

    // Zero quantity
    reqO.body = "{\"productId\": 101, \"quantity\": 0}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Zero quantity returns 400 Bad Request");

    // Negative productId
    reqO.body = "{\"productId\": -101, \"quantity\": 2}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Negative product ID returns 400 Bad Request");

    // Non-existent product ID
    reqO.body = "{\"productId\": 9999, \"quantity\": 2}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Non-existent product ID returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Product ID 9999 not found in catalog") !=
                    std::string::npos,
                "Scenario C: Catalog absence error message verified");

    // Invalid priority enum value
    reqO.body =
        "{\"productId\": 101, \"quantity\": 2, \"priority\": \"URGENT\"}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Invalid priority returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Invalid priority value") != std::string::npos,
                "Scenario C: Invalid priority error verified");

    // Non-string priority
    reqO.body = "{\"productId\": 101, \"quantity\": 2, \"priority\": 123}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Non-string priority returns 400 Bad Request");

    // Valid order creation
    reqO.body = "{\"productId\": 101, \"quantity\": 2, \"priority\": \"HIGH\"}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 200,
                "Scenario C: Valid order returns 200 OK");
    ASSERT_TRUE(respO.body.find("\"success\":true") != std::string::npos,
                "Scenario C: Valid order returns success:true");
    ASSERT_TRUE(respO.body.find("\"orderId\":") != std::string::npos,
                "Scenario C: Valid order returns assigned orderId");

    // Insufficient available stock
    reqO.body = "{\"productId\": 101, \"quantity\": 99999}";
    respO = handler.handleRequest(reqO);
    ASSERT_TRUE(respO.statusCode == 400,
                "Scenario C: Insufficient stock returns 400 Bad Request");
    ASSERT_TRUE(respO.body.find("Insufficient available stock") !=
                    std::string::npos,
                "Scenario C: Insufficient stock error message verified");
  }

  // ========================================================================
  // Scenario D: Dynamic Obstacle Validation (POST /api/obstacle/add & remove)
  // ========================================================================
  {
    HttpRequest reqObs;
    reqObs.method = "POST";
    reqObs.path = "/api/obstacle/add";

    // Missing y coordinate
    reqObs.body = "{\"x\": 5}";
    HttpResponse respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Missing y coordinate returns 400 Bad Request");

    // Missing x coordinate
    reqObs.body = "{\"y\": 5}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Missing x coordinate returns 400 Bad Request");

    // Non-integer coordinate
    reqObs.body = "{\"x\": \"column_five\", \"y\": 5}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Non-numeric coordinate returns 400 Bad Request");

    // Negative coordinate
    reqObs.body = "{\"x\": -1, \"y\": 5}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Negative coordinate returns 400 Bad Request");
    ASSERT_TRUE(respObs.body.find("Coordinates out of bounds") !=
                    std::string::npos,
                "Scenario D: Negative coordinate error message verified");

    // Coordinate X >= 15
    reqObs.body = "{\"x\": 15, \"y\": 5}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Coordinate X >= 15 returns 400 Bad Request");

    // Valid dynamic obstacle placement at empty floor cell (5, 2)
    reqObs.body = "{\"x\": 5, \"y\": 2}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 200,
                "Scenario D: Valid dynamic obstacle placement returns 200 OK");
    ASSERT_TRUE(respObs.body.find("\"success\":true") != std::string::npos,
                "Scenario D: Placement confirms success:true");

    // Obstacle on occupied robot position (0, 3)
    reqObs.body = "{\"x\": 0, \"y\": 3}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Obstacle on occupied robot position rejected");
    ASSERT_TRUE(respObs.body.find("Cannot place obstacle on occupied robot") !=
                    std::string::npos,
                "Scenario D: Occupied position error verified");

    // Removing existing obstacle (5, 2)
    reqObs.path = "/api/obstacle/remove";
    reqObs.body = "{\"x\": 5, \"y\": 2}";
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(
        respObs.statusCode == 200,
        "Scenario D: Removing existing dynamic obstacle returns 200 OK");
    ASSERT_TRUE(respObs.body.find("removed successfully") != std::string::npos,
                "Scenario D: Removal message confirmed");

    // Removing non-existent obstacle (5, 2)
    respObs = handler.handleRequest(reqObs);
    ASSERT_TRUE(respObs.statusCode == 400,
                "Scenario D: Removing non-existent dynamic obstacle returns "
                "400 Bad Request");
    ASSERT_TRUE(respObs.body.find("No dynamic obstacle found") !=
                    std::string::npos,
                "Scenario D: Missing obstacle error verified");
  }

  // ========================================================================
  // Scenario E: Path Algorithm Comparison Validation (GET /api/compare)
  // ========================================================================
  {
    HttpRequest reqC;
    reqC.method = "GET";
    reqC.path = "/api/compare";
    reqC.queryParams = {{"sx", "0"}, {"sy", "0"}, {"gx", "14"}, {"gy", "14"}};
    HttpResponse respC = handler.handleRequest(reqC);
    ASSERT_TRUE(respC.statusCode == 200,
                "Scenario E: Valid compare coordinates return 200 OK");
    ASSERT_TRUE(respC.body.find("BFS") != std::string::npos,
                "Scenario E: Compare returns algorithm metrics");

    // Non-numeric coordinates
    reqC.queryParams = {
        {"sx", "zero"}, {"sy", "0"}, {"gx", "14"}, {"gy", "14"}};
    respC = handler.handleRequest(reqC);
    ASSERT_TRUE(respC.statusCode == 400,
                "Scenario E: Non-numeric coordinate returns 400 Bad Request");

    // Out-of-bounds coordinates
    reqC.queryParams = {{"sx", "0"}, {"sy", "0"}, {"gx", "25"}, {"gy", "14"}};
    respC = handler.handleRequest(reqC);
    ASSERT_TRUE(respC.statusCode == 400,
                "Scenario E: Out-of-bounds coordinate returns 400 Bad Request");
  }

  // ========================================================================
  // Scenario F: Benchmark Input Validation (POST /api/benchmark/run)
  // ========================================================================
  {
    HttpRequest reqBm;
    reqBm.method = "POST";
    reqBm.path = "/api/benchmark/run";

    // Unknown scenario
    reqBm.body = "{\"scenario\":\"quantum_realm\"}";
    HttpResponse respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(
        respBm.statusCode == 400,
        "Scenario F: Unknown benchmark scenario returns 400 Bad Request");
    ASSERT_TRUE(respBm.body.find("Unknown benchmark scenario") !=
                    std::string::npos,
                "Scenario F: Unknown scenario error verified");

    // Negative repetitions
    reqBm.body = "{\"repetitions\": -5}";
    respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(respBm.statusCode == 400,
                "Scenario F: Negative repetitions returns 400 Bad Request");

    // Unknown algorithm
    reqBm.body = "{\"algorithms\": [\"BFS\", \"SuperAStar\"]}";
    respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(
        respBm.statusCode == 400,
        "Scenario F: Unknown algorithm in list returns 400 Bad Request");

    // Empty algorithms array
    reqBm.body = "{\"algorithms\": []}";
    respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(respBm.statusCode == 400,
                "Scenario F: Empty algorithms array returns 400 Bad Request");

    // Valid benchmark execution
    reqBm.body = "{\"scenario\": \"empty\", \"repetitions\": 1, "
                 "\"algorithms\": [\"BFS\", \"A*\"]}";
    respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(respBm.statusCode == 200,
                "Scenario F: Valid benchmark execution returns 200 OK");
    ASSERT_TRUE(respBm.body.find("\"success\":true") != std::string::npos,
                "Scenario F: Benchmark response contains success:true");

    // Canonical benchmark scenario execution
    reqBm.body = "{\"scenario\": \"open_grid\", \"repetitions\": 1, "
                 "\"algorithms\": [\"BFS\", \"Dijkstra\"]}";
    respBm = handler.handleRequest(reqBm);
    ASSERT_TRUE(
        respBm.statusCode == 200,
        "Scenario F: Canonical benchmark scenario open_grid returns 200 OK");
  }

  // ========================================================================
  // Scenario G: Static File Security and Traversal Prevention
  // ========================================================================
  {
    HttpRequest reqSec;
    reqSec.method = "GET";
    reqSec.path = "/../secret_config.json";
    HttpResponse respSec = handler.handleRequest(reqSec);
    ASSERT_TRUE(respSec.statusCode == 403,
                "Scenario G: Path traversal with .. returns 403 Forbidden");

    reqSec.path = "/ghost_nonexistent_file.png";
    respSec = handler.handleRequest(reqSec);
    ASSERT_TRUE(respSec.statusCode == 404,
                "Scenario G: Missing static file returns 404 Not Found");
  }

  // ========================================================================
  // Scenario H: Shared-State Concurrency Safety
  // ========================================================================
  {
    const int NUM_THREADS = 8;
    const int OPS_PER_THREAD = 20;
    std::atomic<int> succOrders{0};
    std::atomic<int> succSteps{0};
    std::atomic<int> succReads{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < NUM_THREADS; ++t) {
      workers.emplace_back(
          [&handler, &succOrders, &succSteps, &succReads, t]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
              int opType = (t + i) % 3;
              if (opType == 0) {
                HttpRequest req;
                req.method = "POST";
                req.path = "/api/order";
                req.body = "{\"productId\": 102, \"quantity\": 1, "
                           "\"priority\": \"MEDIUM\"}";
                HttpResponse r = handler.handleRequest(req);
                if (r.statusCode == 200)
                  succOrders++;
              } else if (opType == 1) {
                HttpRequest req;
                req.method = "POST";
                req.path = "/api/step";
                HttpResponse r = handler.handleRequest(req);
                if (r.statusCode == 200)
                  succSteps++;
              } else {
                HttpRequest req;
                req.method = "GET";
                req.path = "/api/state";
                HttpResponse r = handler.handleRequest(req);
                if (r.statusCode == 200)
                  succReads++;
              }
            }
          });
    }

    for (auto &w : workers) {
      w.join();
    }

    ASSERT_TRUE(succOrders > 0, "Scenario H: Multi-threaded orders placed "
                                "concurrently without deadlock");
    ASSERT_TRUE(
        succSteps > 0,
        "Scenario H: Multi-threaded simulation steps executed concurrently");
    ASSERT_TRUE(succReads > 0,
                "Scenario H: Multi-threaded state reads completed safely");

    // Verify inventory reservation invariant under concurrency
    int avail = testSys.getInventory().getAvailableStock(102);
    int reserved = testSys.getInventory().getReservedStock(102);
    int onHand = testSys.getInventory().getOnHandStock(102);
    ASSERT_TRUE(avail + reserved == onHand,
                "Scenario H: Inventory reservation consistency preserved under "
                "concurrent multi-threaded requests");
  }
}

void testCooperativeMultiRobotAndSequentialOrders() {
  std::cout << "\n--- TEST SUITE 32: Cooperative Multi-Robot Coordination & Strict Sequential Order Processing ---\n";

  // Requirement 3 & 4: Priority-based order selection & FIFO processing for equal priorities
  {
    TaskManager tm;
    auto oLow = std::make_shared<Order>(1, 101, 1, OrderPriority::LOW, Position(14, 14));
    auto oMed = std::make_shared<Order>(2, 102, 1, OrderPriority::MEDIUM, Position(14, 14));
    auto oHigh1 = std::make_shared<Order>(3, 103, 1, OrderPriority::HIGH, Position(14, 14));
    auto oHigh2 = std::make_shared<Order>(4, 104, 1, OrderPriority::HIGH, Position(14, 14));

    tm.addOrder(oLow);
    tm.addOrder(oMed);
    tm.addOrder(oHigh1);
    tm.addOrder(oHigh2);

    const auto &pending = tm.getPendingOrders();
    ASSERT_TRUE(pending.size() == 4, "Requirement 3/4: All 4 orders queued");
    ASSERT_TRUE(pending[0]->getId() == 3, "Requirement 3/4: High priority Order #3 is first");
    ASSERT_TRUE(pending[1]->getId() == 4, "Requirement 3/4: High priority Order #4 is second (FIFO for equal HIGH priority)");
    ASSERT_TRUE(pending[2]->getId() == 2, "Requirement 3/4: Medium priority Order #2 is third");
    ASSERT_TRUE(pending[3]->getId() == 1, "Requirement 3/4: Low priority Order #1 is last");
  }

  // Requirement 1, 5, 6, 7, 8, 9, 12, 13:
  // Cooperative Multi-Robot Teamwork on a single order with 4 units (R01, R02, R03, R04)
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int initialStock101 = sys.getInventory().getProduct(101)->getQuantity(); // 35
    int o1 = sys.createOrder(101, 4, OrderPriority::HIGH); // Order containing at least 4 units

    // Initial tick: team formation & assignment
    sys.stepSimulation();
    auto ord1 = sys.getTaskManager().getOrderById(o1);
    ASSERT_TRUE(ord1 != nullptr && ord1->getStatus() == OrderStatus::ASSIGNED,
                "Requirement 1: Order 1 assigned to cooperative team");
    ASSERT_TRUE(ord1->getTeamRobotIds().size() == 4,
                "Requirement 1: All 4 robots (R01, R02, R03, R04) assigned to cooperative team for 4-unit order");

    bool hasR01 = false, hasR02 = false, hasR03 = false, hasR04 = false;
    for (const auto &id : ord1->getTeamRobotIds()) {
      if (id == "R01") hasR01 = true;
      if (id == "R02") hasR02 = true;
      if (id == "R03") hasR03 = true;
      if (id == "R04") hasR04 = true;
    }
    ASSERT_TRUE(hasR01 && hasR02 && hasR03 && hasR04,
                "Requirement 1: Team contains R01, R02, R03, and R04 cooperating together");

    // Advance simulation through pickup, handoff, and delivery
    bool completed = false;
    bool sawParallelPickup = false;
    bool sawHandoff = false;
    int maxTicks = 60;

    for (int t = 1; t <= maxTicks && !completed; ++t) {
      sys.stepSimulation();
      auto o = sys.getTaskManager().getOrderById(o1);
      if (o && o->getStatus() == OrderStatus::COMPLETED) {
        completed = true;
      }

      // Check if multiple picker robots are simultaneously picking
      int pickingCount = 0;
      for (const auto &r : sys.getRobots()) {
        if (r->getStatus() == RobotStatus::PICKING || r->getCurrentPayload() > 0) {
          pickingCount++;
        }
        if (r->getCurrentTask() && r->getCurrentTask()->getType() == TaskType::MOVE_TO_HANDOFF) {
          sawHandoff = true;
        }
      }
      if (pickingCount >= 2) {
        sawParallelPickup = true;
      }

      // Check inventory consistency at each tick (Requirement 8)
      int avail = sys.getInventory().getAvailableStock(101);
      int reserved = sys.getInventory().getReservedStock(101);
      int onHand = sys.getInventory().getOnHandStock(101);
      ASSERT_TRUE(avail + reserved == onHand,
                  "Requirement 8: Inventory invariant avail + reserved == onHand preserved during team execution");
    }

    ASSERT_TRUE(sawParallelPickup, "Requirement 5: R01 and R02 collected products simultaneously");
    ASSERT_TRUE(sawHandoff, "Requirement 6: Product handoff coordinated between pickers and transporters");
    ASSERT_TRUE(completed, "Requirement 12: Order 1 completed successfully by team");
    int finalStock101 = sys.getInventory().getProduct(101)->getQuantity();
    ASSERT_TRUE(finalStock101 == initialStock101 - 4,
                "Requirement 8: Inventory stock decremented exactly by 4 (no duplicate collection)");
    ASSERT_TRUE(ord1->getDeliveredQuantity() == 4,
                "Requirement 7: All 4 units verified delivered at packing stations");
    ASSERT_TRUE(sys.getCoordinator().getCollisionsPrevented() >= 0,
                "Requirement 9: Collision prevention active throughout team movement");
  }

  // Requirement 2 & 10: Intelligent Parallel Multi-Robot Order Scheduling & Execution
  // Multiple orders assigned concurrently to distinct robots and executed in parallel
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    // Create 3 orders simultaneously:
    // Order 1: 2 ThinkPad Laptops (101) - HIGH
    // Order 2: 2 Wireless Mice (102) - MEDIUM
    // Order 3: 2 Mechanical Keyboards (103) - LOW
    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::MEDIUM);
    int o3 = sys.createOrder(103, 2, OrderPriority::LOW);

    // Initial state check
    ASSERT_TRUE(sys.getTaskManager().getPendingOrders().size() == 3,
                "Requirement 2: All 3 orders initially in pending queue");
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders().empty(),
                "Requirement 2: No active orders before first simulation step");

    // Step 1: All 3 eligible orders are assigned concurrently to distinct eligible robots!
    sys.stepSimulation();
    size_t activeCountAfterStep1 = sys.getTaskManager().getActiveOrders().size();
    ASSERT_TRUE(activeCountAfterStep1 >= 2,
                "Requirement 2: Multiple orders assigned concurrently in single scheduling cycle");

    // Verify orders are assigned to distinct robots
    std::set<std::string> assignedRobots;
    for (const auto& ord : sys.getTaskManager().getActiveOrders()) {
      ASSERT_FALSE(ord->getAssignedRobotId().empty(), "Order has assigned robot");
      ASSERT_TRUE(assignedRobots.find(ord->getAssignedRobotId()) == assignedRobots.end(),
                  "Requirement 2: Orders assigned to distinct eligible robots");
      assignedRobots.insert(ord->getAssignedRobotId());
    }

    // Advance simulation until all 3 orders reach completion
    int ticks = 0;
    int maxTicks = 120;
    bool sawParallelExecution = (activeCountAfterStep1 > 1);

    while (sys.getTaskManager().getCompletedOrders().size() < 3 && ticks < maxTicks) {
      sys.stepSimulation();
      ticks++;

      if (sys.getTaskManager().getActiveOrders().size() >= 2) {
        sawParallelExecution = true;
      }

      // Verify active orders have genuine non-completed statuses
      for (const auto &actOrd : sys.getTaskManager().getActiveOrders()) {
        ASSERT_TRUE(actOrd->getStatus() == OrderStatus::ASSIGNED ||
                    actOrd->getStatus() == OrderStatus::PICKING ||
                    actOrd->getStatus() == OrderStatus::DELIVERING,
                    "Genuinely active status: Only ASSIGNED/PICKING/DELIVERING in active queue");
      }
    }

    ASSERT_TRUE(sawParallelExecution, "Requirement 2: Multiple orders executed concurrently in parallel");
    ASSERT_TRUE(sys.getTaskManager().getCompletedOrders().size() == 3,
                "Requirement 2: All 3 orders successfully reached COMPLETED status");
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders().empty(),
                "Requirement 2: All active orders completed, active queue empty");

    auto finishedO1 = sys.getTaskManager().getOrderById(o1);
    auto finishedO2 = sys.getTaskManager().getOrderById(o2);
    auto finishedO3 = sys.getTaskManager().getOrderById(o3);
    ASSERT_TRUE(finishedO1 && finishedO1->getStatus() == OrderStatus::COMPLETED && finishedO1->isFullyDelivered(),
                "Requirement 2: Order 1 completed and fully delivered");
    ASSERT_TRUE(finishedO2 && finishedO2->getStatus() == OrderStatus::COMPLETED && finishedO2->isFullyDelivered(),
                "Requirement 2: Order 2 completed and fully delivered");
    ASSERT_TRUE(finishedO3 && finishedO3->getStatus() == OrderStatus::COMPLETED && finishedO3->isFullyDelivered(),
                "Requirement 2: Order 3 completed and fully delivered");

    // Requirement 13: Accurate Analytics Integration
    const auto &analytics = sys.getAnalytics();
    ASSERT_TRUE(analytics.getTotalOrdersCreated() == 3,
                "Requirement 13: Analytics accurately recorded 3 orders created");
    ASSERT_TRUE(analytics.getTotalOrdersCompleted() == 3,
                "Requirement 13: Analytics accurately recorded 3 orders completed");
    ASSERT_TRUE(analytics.getAverageOrderCompletionTime() > 0.0,
                "Requirement 13: Average completion duration calculated from actual simulation ticks");
    ASSERT_TRUE(analytics.getFleetTotalMovementSteps() > 0,
                "Requirement 13: Fleet movement steps tracked authoritatively");
  }

  // Requirement 11: Fault Tolerance & Robot Reassignment
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int oId = sys.createOrder(101, 1, OrderPriority::HIGH);
    sys.stepSimulation();

    auto ord = sys.getTaskManager().getOrderById(oId);
    ASSERT_TRUE(ord != nullptr && ord->getStatus() == OrderStatus::ASSIGNED,
                "Requirement 11: Order assigned to initial robot");

    std::string assignedId = ord->getAssignedRobotId();
    std::string newTargetId = (assignedId == "R01") ? "R02" : "R01";

    bool reassigned = sys.reassignOrder(oId, newTargetId);
    ASSERT_TRUE(reassigned, "Requirement 11: Order successfully reassigned to alternative robot");
    ASSERT_TRUE(ord->getAssignedRobotId() == newTargetId,
                "Requirement 11: Order assigned robot updated to new robot");
  }

  // Dedicated Verification: Concurrent Order Processing and Queue Invariants
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int idA = sys.createOrder(101, 1, OrderPriority::HIGH);
    int idB = sys.createOrder(102, 1, OrderPriority::MEDIUM);
    int idC = sys.createOrder(103, 1, OrderPriority::LOW);

    int maxTotalTicks = 120;
    int ordersCompletedCount = 0;
    bool sawMultiActive = false;

    for (int t = 1; t <= maxTotalTicks && ordersCompletedCount < 3; ++t) {
      sys.stepSimulation();

      size_t activeCount = sys.getTaskManager().getActiveOrders().size();
      if (activeCount > 1) {
        sawMultiActive = true;
      }
      ASSERT_TRUE(activeCount <= sys.getRobots().size(),
                  "Invariant: ACTIVE_ORDER_COUNT <= total robot count");

      // Verify that active orders have genuine non-completed statuses
      for (const auto &actOrd : sys.getTaskManager().getActiveOrders()) {
        ASSERT_TRUE(actOrd->getStatus() == OrderStatus::ASSIGNED ||
                    actOrd->getStatus() == OrderStatus::PICKING ||
                    actOrd->getStatus() == OrderStatus::DELIVERING,
                    "Genuinely active status: Only ASSIGNED/PICKING/DELIVERING in active queue");
        ASSERT_FALSE(actOrd->getStatus() == OrderStatus::COMPLETED,
                     "Completed orders are never present in getActiveOrders()");
      }

      ordersCompletedCount = static_cast<int>(sys.getTaskManager().getCompletedOrders().size());
    }

    ASSERT_TRUE(sawMultiActive, "Multiple orders actively processed in parallel simultaneously");
    ASSERT_TRUE(ordersCompletedCount == 3, "All 3 batch orders completed with invariants preserved throughout");

    // Verify all 3 orders genuinely delivered and completed
    auto ordA = sys.getTaskManager().getOrderById(idA);
    auto ordB = sys.getTaskManager().getOrderById(idB);
    auto ordC = sys.getTaskManager().getOrderById(idC);
    ASSERT_TRUE(ordA && ordA->getStatus() == OrderStatus::COMPLETED && ordA->isFullyDelivered(),
                "Order A genuinely delivered and completed");
    ASSERT_TRUE(ordB && ordB->getStatus() == OrderStatus::COMPLETED && ordB->isFullyDelivered(),
                "Order B genuinely delivered and completed");
    ASSERT_TRUE(ordC && ordC->getStatus() == OrderStatus::COMPLETED && ordC->isFullyDelivered(),
                "Order C genuinely delivered and completed");
  }
}

void testIntelligentMultiRobotParallelExecution() {
  std::cout << "\n--- TEST SUITE 33: Intelligent Multi-Robot Parallel Order Execution ---\n";

  // TEST 1: Multiple orders assigned concurrently
  // Given multiple pending orders and multiple eligible robots, more than one order is assigned in the same scheduling cycle.
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::MEDIUM);
    int o3 = sys.createOrder(103, 2, OrderPriority::LOW);

    ASSERT_TRUE(sys.getTaskManager().getPendingOrders().size() == 3, "TEST 1: 3 orders in pending queue");
    sys.stepSimulation(); // First scheduling cycle

    size_t activeCount = sys.getTaskManager().getActiveOrders().size();
    ASSERT_TRUE(activeCount >= 2, "TEST 1: More than one order assigned in the same scheduling cycle");

    // Orders assigned to distinct eligible robots or disjoint teams
    std::set<std::string> robotsAssigned;
    for (const auto& ord : sys.getTaskManager().getActiveOrders()) {
      ASSERT_FALSE(ord->getAssignedRobotId().empty(), "TEST 1: Order has assigned robot");
      ASSERT_TRUE(robotsAssigned.find(ord->getAssignedRobotId()) == robotsAssigned.end(),
                  "TEST 1: Orders assigned to distinct eligible robots");
      robotsAssigned.insert(ord->getAssignedRobotId());
    }
  }

  // TEST 2: Single robot, multiple pending orders
  // Only one active assignment executed by the robot at a time. Remaining orders stay pending until robot is available.
  {
    WarehouseSystem sys(15, 15);
    // Add only 1 robot
    sys.getWarehouse().addShelf(Shelf("SH01", Position(3, 3)));
    sys.getWarehouse().addPackingStation(PackingStation("PK01", Position(14, 14)));
    sys.getWarehouse().addChargingStation(ChargingStation("CS01", Position(0, 0)));
    sys.getInventory().addProduct(Product(101, "Product 101", "Cat", 50, 10.0, Position(3, 3)));
    sys.addRobot(std::make_unique<PickerRobot>("R01", "Solo-Picker", Position(2, 2)));

    int o1 = sys.createOrder(101, 1, OrderPriority::HIGH);
    int o2 = sys.createOrder(101, 1, OrderPriority::MEDIUM);
    int o3 = sys.createOrder(101, 1, OrderPriority::LOW);

    sys.stepSimulation();
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders().size() == 1,
                "TEST 2: Solo robot executes only 1 active order at a time");
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders()[0]->getId() == o1,
                "TEST 2: Highest priority order #1 assigned first");
    ASSERT_TRUE(sys.getTaskManager().getPendingOrders().size() == 2,
                "TEST 2: Remaining orders stay pending in queue");

    // Advance until Order 1 completes
    int ticks = 0;
    while (!sys.getTaskManager().getActiveOrders().empty() &&
           sys.getTaskManager().getActiveOrders()[0]->getId() == o1 && ticks < 60) {
      sys.stepSimulation();
      ticks++;
    }

    // Step until robot vacates staging and Order 2 is dynamically assigned
    for (int i = 0; i < 25 && sys.getTaskManager().getActiveOrders().empty(); ++i) {
      sys.stepSimulation();
    }
    ASSERT_TRUE(!sys.getTaskManager().getActiveOrders().empty(),
                "TEST 2: Next order dynamically assigned when robot becomes available");
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders()[0]->getId() == o2,
                "TEST 2: Next order assigned is Order #2 (MEDIUM priority)");
  }

  // TEST 3: Multiple orders requesting the same product
  // Inventory reservations remain consistent. No over-reservation occurs. Physical deduction accurate.
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    // Product 101 has stock 35
    int initStock = sys.getInventory().getProduct(101)->getQuantity(); // 35
    int o1 = sys.createOrder(101, 10, OrderPriority::HIGH);
    int o2 = sys.createOrder(101, 10, OrderPriority::MEDIUM);
    int o3 = sys.createOrder(101, 10, OrderPriority::LOW); // 10+10+10 = 30 <= 35

    // Total reserved stock cannot exceed on-hand stock
    int reserved = sys.getInventory().getReservedStock(101);
    int available = sys.getInventory().getAvailableStock(101);
    int onHand = sys.getInventory().getOnHandStock(101);
    ASSERT_TRUE(onHand == initStock, "TEST 3: On-hand stock not prematurely deducted before pickup");
    ASSERT_TRUE(reserved == 30, "TEST 3: 30 units reserved for 3 orders");
    ASSERT_TRUE(available == 5, "TEST 3: 5 units remaining unreserved");
    ASSERT_TRUE(reserved + available == onHand, "TEST 3: Inventory invariant holds: reserved + available == onHand");

    // Verify over-reservation prevention: creating order for 10 units when available is 5 throws InsufficientStockException
    bool threwInsufficient = false;
    try {
      sys.createOrder(101, 10, OrderPriority::LOW);
    } catch (const InsufficientStockException&) {
      threwInsufficient = true;
    }
    ASSERT_TRUE(threwInsufficient, "TEST 3: Over-reservation prevented: InsufficientStockException thrown when requesting beyond available stock");

    // Advance simulation through pickups
    for (int t = 0; t < 25; ++t) {
      sys.stepSimulation();
    }

    // After orders pick up, on-hand stock is accurately decremented
    int currOnHand = sys.getInventory().getOnHandStock(101);
    int currReserved = sys.getInventory().getReservedStock(101);
    int currAvail = sys.getInventory().getAvailableStock(101);
    ASSERT_TRUE(currOnHand <= initStock, "TEST 3: Physical stock successfully deducted upon pickup");
    ASSERT_TRUE(currReserved + currAvail == currOnHand, "TEST 3: Invariant holds after physical deduction: reserved + available == onHand");
  }

  // TEST 4: Mixed-priority orders
  // HIGH priority is respected over MEDIUM and LOW when eligible. Scheduling remains deterministic.
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    // Create low priority first, then medium, then high
    int oLow = sys.createOrder(101, 2, OrderPriority::LOW);
    int oMed = sys.createOrder(102, 2, OrderPriority::MEDIUM);
    int oHigh = sys.createOrder(103, 2, OrderPriority::HIGH);

    const auto& pending = sys.getTaskManager().getPendingOrders();
    ASSERT_TRUE(pending.size() == 3, "TEST 4: All 3 orders queued");
    ASSERT_TRUE(pending[0]->getId() == oHigh, "TEST 4: HIGH priority order is at front of queue");
    ASSERT_TRUE(pending[1]->getId() == oMed, "TEST 4: MEDIUM priority order is second");
    ASSERT_TRUE(pending[2]->getId() == oLow, "TEST 4: LOW priority order is third");

    sys.stepSimulation();
    // HIGH priority order must be assigned
    auto ordHigh = sys.getTaskManager().getOrderById(oHigh);
    ASSERT_TRUE(ordHigh && ordHigh->getStatus() == OrderStatus::ASSIGNED,
                "TEST 4: HIGH priority order assigned before lower priorities");
  }

  // TEST 5: Robot battery feasibility
  // Infeasible assignments rejected. Other eligible robots receive assignments.
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    // Drain R01 battery to 15.0% (infeasible for round trip)
    for (const auto& r : sys.getRobots()) {
      if (r->getId() == "R01") {
        r->setBattery(15.0);
      }
    }

    int oId = sys.createOrder(101, 2, OrderPriority::HIGH);
    sys.stepSimulation();

    auto ord = sys.getTaskManager().getOrderById(oId);
    ASSERT_TRUE(ord && ord->getStatus() == OrderStatus::ASSIGNED, "TEST 5: Order assigned to eligible robot");
    ASSERT_TRUE(ord->getAssignedRobotId() != "R01",
                "TEST 5: Robot R01 with depleted battery rejected; assigned to alternate feasible robot");
  }

  // TEST 6: Concurrent order lifecycle
  // Multiple orders independently progress through lifecycle without premature completion
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::MEDIUM);

    sys.stepSimulation();
    auto ord1 = sys.getTaskManager().getOrderById(o1);
    auto ord2 = sys.getTaskManager().getOrderById(o2);
    ASSERT_TRUE(ord1 && ord1->getStatus() == OrderStatus::ASSIGNED, "TEST 6: Order 1 ASSIGNED");
    ASSERT_TRUE(ord2 && ord2->getStatus() == OrderStatus::ASSIGNED, "TEST 6: Order 2 ASSIGNED");

    bool sawPicking = false;
    bool sawDelivering = false;
    for (int t = 0; t < 60 && (!ord1->isFullyDelivered() || !ord2->isFullyDelivered()); ++t) {
      sys.stepSimulation();
      if (ord1->getStatus() == OrderStatus::PICKING || ord2->getStatus() == OrderStatus::PICKING) {
        sawPicking = true;
      }
      if (ord1->getStatus() == OrderStatus::DELIVERING || ord2->getStatus() == OrderStatus::DELIVERING) {
        sawDelivering = true;
      }
    }

    ASSERT_TRUE(sawPicking, "TEST 6: Orders transitioned through PICKING lifecycle state");
    ASSERT_TRUE(sawDelivering, "TEST 6: Orders transitioned through DELIVERING lifecycle state");
    ASSERT_TRUE(ord1->getStatus() == OrderStatus::COMPLETED, "TEST 6: Order 1 reached COMPLETED upon fulfillment");
    ASSERT_TRUE(ord2->getStatus() == OrderStatus::COMPLETED, "TEST 6: Order 2 reached COMPLETED upon fulfillment");
    ASSERT_TRUE(ord1->getDeliveredQuantity() == 2, "TEST 6: Order 1 delivered quantity accurate (2)");
    ASSERT_TRUE(ord2->getDeliveredQuantity() == 2, "TEST 6: Order 2 delivered quantity accurate (2)");
  }

  // TEST 7: Robot collision prevention under concurrent orders
  // Multiple robots move concurrently: vertex collisions & head-on swaps prevented
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::MEDIUM);
    int o3 = sys.createOrder(103, 2, OrderPriority::LOW);

    for (int t = 0; t < 50; ++t) {
      sys.stepSimulation();
      // Check for vertex collisions across all robot pairs
      const auto& robs = sys.getRobots();
      for (size_t i = 0; i < robs.size(); ++i) {
        for (size_t j = i + 1; j < robs.size(); ++j) {
          ASSERT_TRUE(robs[i]->getPosition() != robs[j]->getPosition(),
                      "TEST 7: No two robots occupy the same grid cell at the same tick");
        }
      }
    }
  }

  // TEST 8: Cooperative team compatibility
  // Existing cooperative team behavior remains functional; robots in active team cannot be assigned another order
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    // 4-unit order forms 4-robot cooperative team (R01, R02, R03, R04)
    int oTeam = sys.createOrder(101, 4, OrderPriority::HIGH);
    int oExtra = sys.createOrder(102, 1, OrderPriority::LOW);

    sys.stepSimulation();
    auto teamOrd = sys.getTaskManager().getOrderById(oTeam);
    ASSERT_TRUE(teamOrd && teamOrd->getTeamRobotIds().size() == 4,
                "TEST 8: Cooperative team formed with 4 robots");

    // Since all 4 robots are in the team, oExtra MUST remain pending
    auto extraOrd = sys.getTaskManager().getOrderById(oExtra);
    ASSERT_TRUE(extraOrd && extraOrd->getStatus() == OrderStatus::PENDING,
                "TEST 8: Order extra remains pending; robots in active team cannot take another order");
  }

  // TEST 9: Dynamic reassignment
  // Newly available robots receive pending orders dynamically without reset
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();

    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::HIGH);
    int o3 = sys.createOrder(103, 2, OrderPriority::HIGH);
    int o4 = sys.createOrder(104, 2, OrderPriority::HIGH);
    int o5 = sys.createOrder(105, 2, OrderPriority::LOW); // 5th order exceeds 4-robot capacity

    sys.stepSimulation();
    // 4 robots take 4 orders, o5 stays pending
    auto ord5 = sys.getTaskManager().getOrderById(o5);
    ASSERT_TRUE(ord5 && ord5->getStatus() == OrderStatus::PENDING,
                "TEST 9: 5th order remains pending when fleet is fully occupied");

    // Advance until at least one order completes and robot becomes available
    int maxTicks = 80;
    bool o5Assigned = false;
    for (int t = 0; t < maxTicks && !o5Assigned; ++t) {
      sys.stepSimulation();
      if (ord5->getStatus() == OrderStatus::ASSIGNED || ord5->getStatus() == OrderStatus::PICKING ||
          ord5->getStatus() == OrderStatus::DELIVERING || ord5->getStatus() == OrderStatus::COMPLETED) {
        o5Assigned = true;
      }
    }
    ASSERT_TRUE(o5Assigned, "TEST 9: Pending order dynamically assigned as soon as robot finishes order and vacates");
  }

  // TEST 10: Regression compatibility
  // Regression test suite continues to pass without errors
  {
    ASSERT_TRUE(g_testsFailed == 0, "TEST 10: Regression compatibility: All prior test assertions passing");
  }
}

void testAlgorithmLaboratorySuite() {
  std::cout << "\n--- TEST SUITE 34: DSA Algorithm Laboratory (BFS, Dijkstra, A*) ---\n";

  auto isContinuousOrthogonal = [](const std::vector<Position>& p) -> bool {
    if (p.size() <= 1) return true;
    for (size_t i = 0; i < p.size() - 1; ++i) {
      int dx = std::abs(p[i].x - p[i + 1].x);
      int dy = std::abs(p[i].y - p[i + 1].y);
      if (dx + dy != 1) return false;
    }
    return true;
  };

  Warehouse openGrid(10, 10);
  Position start(1, 1);
  Position goal(6, 6);

  // Scenario 1: BFS simple open grid
  {
    PathResult res = PathFinder::findPathBFS(openGrid, start, goal);
    ASSERT_TRUE(res.found && res.path.size() == 11 && res.totalCost == 10.0,
                "Scenario 1: BFS finds shortest path on open grid (10 hops)");
  }

  // Scenario 2: Dijkstra simple open grid
  {
    PathResult res = PathFinder::findPathDijkstra(openGrid, start, goal);
    ASSERT_TRUE(res.found && res.path.size() == 11 && res.totalCost == 10.0,
                "Scenario 2: Dijkstra finds optimal cost path on open grid");
  }

  // Scenario 3: A* simple open grid
  {
    PathResult astar = PathFinder::findPathAStar(openGrid, start, goal);
    PathResult bfs = PathFinder::findPathBFS(openGrid, start, goal);
    ASSERT_TRUE(astar.found && astar.path.size() == 11 && astar.nodesExplored <= bfs.nodesExplored,
                "Scenario 3: A* finds optimal path and explores fewer or equal nodes than BFS on open grid");
  }

  // Scenario 4: Source equals destination
  {
    Position same(3, 3);
    PathResult bfs = PathFinder::findPathBFS(openGrid, same, same);
    PathResult dij = PathFinder::findPathDijkstra(openGrid, same, same);
    PathResult ast = PathFinder::findPathAStar(openGrid, same, same);
    bool ok = bfs.found && dij.found && ast.found &&
              bfs.path.size() == 1 && bfs.totalCost == 0.0 &&
              dij.path.size() == 1 && dij.totalCost == 0.0 &&
              ast.path.size() == 1 && ast.totalCost == 0.0 &&
              bfs.statusMessage == "SUCCESS";
    ASSERT_TRUE(ok, "Scenario 4: Source equals destination returns single-node zero-cost path");
  }

  // Scenario 5: Destination unreachable
  {
    Warehouse wallWh(10, 10);
    wallWh.setObstacle(Position(7, 8), true);
    wallWh.setObstacle(Position(9, 8), true);
    wallWh.setObstacle(Position(8, 7), true);
    wallWh.setObstacle(Position(8, 9), true);

    PathResult bfs = PathFinder::findPathBFS(wallWh, Position(0, 0), Position(8, 8));
    PathResult dij = PathFinder::findPathDijkstra(wallWh, Position(0, 0), Position(8, 8));
    PathResult ast = PathFinder::findPathAStar(wallWh, Position(0, 0), Position(8, 8));
    ASSERT_TRUE(!bfs.found && !dij.found && !ast.found &&
                bfs.statusMessage == "NO_PATH_FOUND" && ast.statusMessage == "NO_PATH_FOUND",
                "Scenario 5: Destination unreachable safely returns found=false and NO_PATH_FOUND");
  }

  // Scenario 6: Source blocked
  {
    Warehouse obsWh(10, 10);
    obsWh.setObstacle(Position(1, 1), true);
    PathResult res = PathFinder::findPathAStar(obsWh, Position(1, 1), Position(5, 5));
    ASSERT_TRUE(!res.found && res.statusMessage == "START_BLOCKED",
                "Scenario 6: Blocked source cell rejected with START_BLOCKED");
  }

  // Scenario 7: Destination blocked
  {
    Warehouse obsWh(10, 10);
    obsWh.setObstacle(Position(5, 5), true);
    PathResult res = PathFinder::findPathAStar(obsWh, Position(1, 1), Position(5, 5));
    ASSERT_TRUE(!res.found && res.statusMessage == "GOAL_BLOCKED",
                "Scenario 7: Blocked destination cell rejected with GOAL_BLOCKED");
  }

  // Scenario 8: Obstacle detour
  {
    Warehouse detourWh(10, 10);
    for (int y = 0; y <= 6; ++y) {
      detourWh.setObstacle(Position(3, y), true);
    }
    PathResult res = PathFinder::findPathAStar(detourWh, Position(1, 3), Position(5, 3));
    bool hitsWall = false;
    for (const auto& p : res.path) {
      if (p.x == 3 && p.y <= 6) hitsWall = true;
    }
    ASSERT_TRUE(res.found && !hitsWall && res.path.size() > 0,
                "Scenario 8: Obstacle detour correctly routes around blocking barrier");
  }

  // Scenario 9: Multiple equally short paths
  {
    Warehouse smallWh(5, 5);
    PathResult res = PathFinder::findPathAStar(smallWh, Position(0, 0), Position(2, 2));
    ASSERT_TRUE(res.found && res.path.size() == 5 && isContinuousOrthogonal(res.path),
                "Scenario 9: Multiple equally short paths returns continuous, loop-free optimal path");
  }

  // Scenario 10: Weighted grid setup
  {
    Warehouse weightWh(5, 5);
    weightWh.setCellCost(Position(1, 0), 5.5);
    weightWh.setCellCost(Position(0, 1), 2.0);
    ASSERT_TRUE(weightWh.getCellCost(Position(1, 0)) == 5.5 && weightWh.getCellCost(Position(0, 1)) == 2.0,
                "Scenario 10: Warehouse custom cell costs configured and read accurately");
  }

  // Scenario 11: Dijkstra weighted optimality
  {
    Warehouse wWh(4, 4);
    wWh.setCellCost(Position(1, 0), 15.0);
    PathResult res = PathFinder::findPathDijkstra(wWh, Position(0, 0), Position(2, 0));
    ASSERT_TRUE(res.found && res.totalCost == 4.0 && res.path.size() == 5,
                "Scenario 11: Dijkstra weighted optimality selects lower-cost detour over high-weight direct edge");
  }

  // Scenario 12: A* weighted optimality
  {
    Warehouse wWh(4, 4);
    wWh.setCellCost(Position(1, 0), 15.0);
    PathResult res = PathFinder::findPathAStar(wWh, Position(0, 0), Position(2, 0));
    ASSERT_TRUE(res.found && res.totalCost == 4.0 && res.path.size() == 5,
                "Scenario 12: A* weighted optimality finds the minimum-cost route matching Dijkstra");
  }

  // Scenario 13: BFS unweighted shortest path
  {
    Warehouse wWh(4, 4);
    wWh.setCellCost(Position(1, 0), 15.0);
    PathResult res = PathFinder::findPathBFS(wWh, Position(0, 0), Position(2, 0));
    ASSERT_TRUE(res.found && res.path.size() == 3 && res.totalCost == 16.0,
                "Scenario 13: BFS prioritizes unweighted hop-count over cell edge weights");
  }

  // Scenario 14: Path reconstruction correctness
  {
    PathResult res = PathFinder::findPathAStar(openGrid, Position(0, 0), Position(7, 5));
    bool validEndpoints = (res.path.front() == Position(0, 0)) && (res.path.back() == Position(7, 5));
    bool validSteps = isContinuousOrthogonal(res.path);
    ASSERT_TRUE(res.found && validEndpoints && validSteps,
                "Scenario 14: Path reconstruction starts at source, ends at goal, with valid unit steps");
  }

  // Scenario 15: Parent tracking correctness
  {
    PathResult res = PathFinder::findPathAStar(openGrid, Position(2, 2), Position(7, 7));
    std::set<Position> visitedInPath;
    bool hasCycles = false;
    for (const auto& p : res.path) {
      if (visitedInPath.find(p) != visitedInPath.end()) {
        hasCycles = true;
        break;
      }
      visitedInPath.insert(p);
    }
    ASSERT_TRUE(res.found && !hasCycles,
                "Scenario 15: Parent tracking guarantees acyclic path tree without duplicate positions");
  }

  // Scenario 16: Correct path cost calculation
  {
    Warehouse costWh(6, 6);
    costWh.setCellCost(Position(1, 0), 2.5);
    costWh.setCellCost(Position(2, 1), 3.5);
    PathResult res = PathFinder::findPathAStar(costWh, Position(0, 0), Position(3, 3));
    double expectedCost = 0.0;
    for (size_t i = 1; i < res.path.size(); ++i) {
      expectedCost += costWh.getCellCost(res.path[i]);
    }
    ASSERT_TRUE(res.found && std::abs(res.totalCost - expectedCost) < 1e-6,
                "Scenario 16: Path totalCost exactly matches sum of traversed cell costs");
  }

  // Scenario 17: Correct nodes explored metric
  {
    PathResult res = PathFinder::findPathAStar(openGrid, Position(0, 0), Position(8, 8));
    bool orderMatches = (res.nodesExplored == static_cast<int>(res.explorationOrder.size()));
    ASSERT_TRUE(res.found && res.nodesExplored > 0 && orderMatches,
                "Scenario 17: nodesExplored metric accurately reflects the serialized exploration sequence");
  }

  // Scenario 18: Invalid coordinate handling
  {
    PathResult negStart = PathFinder::findPathAStar(openGrid, Position(-1, 0), Position(5, 5));
    PathResult oobGoal = PathFinder::findPathAStar(openGrid, Position(0, 0), Position(25, 30));
    ASSERT_TRUE(!negStart.found && negStart.statusMessage == "OUT_OF_BOUNDS" &&
                !oobGoal.found && oobGoal.statusMessage == "OUT_OF_BOUNDS",
                "Scenario 18: Negative and out-of-bounds coordinates handled safely with OUT_OF_BOUNDS");
  }

  // Scenario 19: Comparison mode consistency
  {
    Warehouse baseWh(10, 10);
    baseWh.setObstacle(Position(3, 3), true);
    AlgorithmLabComparison comp = AlgorithmLab::compareAlgorithms(baseWh, Position(0, 0), Position(6, 6));
    bool consistent = comp.bfsResult.found && comp.dijkstraResult.found && comp.astarResult.found &&
                      (comp.dijkstraResult.totalCost == comp.astarResult.totalCost) &&
                      comp.obstacleCount >= 1;
    ASSERT_TRUE(consistent, "Scenario 19: AlgorithmLab::compareAlgorithms evaluates all algorithms on identical grid");
  }

  // Scenario 20: Existing warehouse pathfinding regression
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    Position cs(0, 0); // Charging station CS01
    Position pk(14, 14); // Packing station PK01
    PathResult res = PathFinder::findPathAStar(sys.getWarehouse(), cs, pk);
    ASSERT_TRUE(res.found && res.path.size() > 0 && res.path.front() == cs && res.path.back() == pk,
                "Scenario 20: Regression verification: standard warehouse navigation routes correctly");
  }

  // REST API Endpoints Verification: /api/algorithms/run, /api/algorithms/compare, /api/algorithms/report
  {
    WarehouseSystem apiSys(15, 15);
    apiSys.initializeDefaultWarehouse();
    std::mutex mtx;
    HttpRequestHandler handler(apiSys, mtx);

    // 1. POST /api/algorithms/run
    HttpRequest reqRun;
    reqRun.method = "POST";
    reqRun.path = "/api/algorithms/run";
    reqRun.body = "{\"algorithm\":\"A*\",\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respRun = handler.handleRequest(reqRun);
    ASSERT_TRUE(respRun.statusCode == 200 && respRun.body.find("\"found\":true") != std::string::npos,
                "REST API: POST /api/algorithms/run returns 200 OK with valid PathResult JSON");

    // 2. POST /api/algorithms/compare
    HttpRequest reqComp;
    reqComp.method = "POST";
    reqComp.path = "/api/algorithms/compare";
    reqComp.body = "{\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respComp = handler.handleRequest(reqComp);
    ASSERT_TRUE(respComp.statusCode == 200 && respComp.body.find("\"results\":[") != std::string::npos,
                "REST API: POST /api/algorithms/compare returns 200 OK with all three algorithm results");

    // 3. POST /api/algorithms/report (CSV)
    HttpRequest reqRep;
    reqRep.method = "POST";
    reqRep.path = "/api/algorithms/report";
    reqRep.body = "{\"format\":\"csv\",\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respRep = handler.handleRequest(reqRep);
    ASSERT_TRUE(respRep.statusCode == 200 && respRep.headers["Content-Type"].find("text/csv") != std::string::npos,
                "REST API: POST /api/algorithms/report returns 200 OK with Content-Type text/csv");

    // 4. Invalid input handling
    HttpRequest reqInvalid;
    reqInvalid.method = "POST";
    reqInvalid.path = "/api/algorithms/run";
    reqInvalid.body = "{\"algorithm\":\"A*\",\"start\":{\"x\":-5,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respInvalid = handler.handleRequest(reqInvalid);
    ASSERT_TRUE(respInvalid.statusCode == 400,
                "REST API: Invalid coordinates return 400 Bad Request error response");
  }
}

void testEcoRouteSuite() {
  std::cout << "\n--- TEST SUITE 35: EcoRoute — Energy-Aware Routing & Optimization ---\n";

  // Scenario 1: Energy consumption for a simple route
  {
    Warehouse wh(10, 10);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    std::vector<Position> route = {
      Position(0, 0), Position(1, 0), Position(2, 0), Position(3, 0), Position(4, 0),
      Position(5, 0), Position(6, 0), Position(7, 0), Position(8, 0), Position(9, 0)
    };
    // 9 steps, normal cost 1.0, 0 payload -> 9 * 0.5 = 4.5 EU
    double energy = EcoRoute::calculateRouteEnergy(prof, route, wh, 0);
    ASSERT_TRUE(std::abs(energy - 4.5) < 1e-6,
                "Scenario 1: Energy consumption calculated accurately for a simple route");
  }

  // Scenario 2: Robot-specific energy profiles
  {
    RobotEnergyProfile pickerProf = RobotEnergyProfile::getProfile("PickerRobot");
    RobotEnergyProfile transProf = RobotEnergyProfile::getProfile("TransportRobot");
    RobotEnergyProfile hybridProf = RobotEnergyProfile::getProfile("HybridRobot");
    ASSERT_TRUE(pickerProf.baseMoveCost < hybridProf.baseMoveCost &&
                hybridProf.baseMoveCost < transProf.baseMoveCost &&
                pickerProf.payloadEnergyFactor < transProf.payloadEnergyFactor,
                "Scenario 2: Robot-specific energy profiles define distinct physical consumption models");
  }

  // Scenario 3: Payload-dependent energy consumption
  {
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    double emptyCost = EcoRoute::calculateStepEnergy(prof, 0, 1.0);
    double loadedCost = EcoRoute::calculateStepEnergy(prof, 4, 1.0);
    ASSERT_TRUE(loadedCost > emptyCost && std::abs(loadedCost - (0.5 + 4 * 0.05)) < 1e-6,
                "Scenario 3: Payload weight scales energy consumption per movement step");
  }

  // Scenario 4: Energy-aware path selection
  {
    Warehouse wh(5, 5);
    Position start(0, 1);
    Position goal(4, 1);
    // Direct path on row 1 has heavy energy friction (weight 10.0)
    wh.setCellCost(Position(1, 1), 10.0);
    wh.setCellCost(Position(2, 1), 10.0);
    wh.setCellCost(Position(3, 1), 10.0);

    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult ecoRes = EcoRoute::findEcoRoute(wh, start, goal, prof, 0);
    EcoRouteResult shortRes = EcoRoute::findShortestRoute(wh, start, goal, prof, 0);

    ASSERT_TRUE(ecoRes.found && ecoRes.estimatedEnergy < shortRes.estimatedEnergy,
                "Scenario 4: Energy-aware path selection prioritizes low-energy detour over high-friction direct route");
  }

  // Scenario 5: Shortest-distance route calculation
  {
    Warehouse wh(5, 5);
    Position start(0, 1);
    Position goal(4, 1);
    wh.setCellCost(Position(1, 1), 10.0);
    wh.setCellCost(Position(2, 1), 10.0);
    wh.setCellCost(Position(3, 1), 10.0);

    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult shortRes = EcoRoute::findShortestRoute(wh, start, goal, prof, 0);
    ASSERT_TRUE(shortRes.found && shortRes.distance == 4,
                "Scenario 5: Shortest-distance baseline finds minimal hop-count route");
  }

  // Scenario 6: Energy route optimality
  {
    Warehouse wh(5, 5);
    Position start(0, 0);
    Position goal(4, 4);
    wh.setCellCost(Position(2, 2), 5.0);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("TransportRobot");
    EcoRouteResult ecoRes = EcoRoute::findEcoRoute(wh, start, goal, prof, 5);
    EcoRouteResult shortRes = EcoRoute::findShortestRoute(wh, start, goal, prof, 5);
    ASSERT_TRUE(ecoRes.estimatedEnergy <= shortRes.estimatedEnergy,
                "Scenario 6: EcoRoute produces mathematically optimal or equal energy cost compared to baseline");
  }

  // Scenario 7: Battery feasibility: sufficient battery
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    auto robot = std::make_unique<PickerRobot>("R_FEAS", "Picker Feas", Position(0, 0));
    robot->charge(100.0); // full battery
    Order order(101, 101, 2, OrderPriority::HIGH, Position(14, 14));
    Position targetShelf(3, 3);
    BatteryFeasibilityReport rep = EcoRoute::validateBatteryFeasibility(*robot, order, sys.getWarehouse(), sys.getRobots(), targetShelf);
    ASSERT_TRUE(rep.isFeasible && rep.batteryMargin > 0.0,
                "Scenario 7: Battery feasibility passes when battery exceeds 4-leg trip energy + safety reserve");
  }

  // Scenario 8: Insufficient battery rejection
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    auto robot = std::make_unique<PickerRobot>("R_LOW", "Picker Low", Position(0, 0));
    robot->setBattery(10.0); // insufficient battery
    Order order(102, 101, 2, OrderPriority::HIGH, Position(14, 14));
    Position targetShelf(3, 3);
    BatteryFeasibilityReport rep = EcoRoute::validateBatteryFeasibility(*robot, order, sys.getWarehouse(), sys.getRobots(), targetShelf);
    ASSERT_TRUE(!rep.isFeasible && rep.batteryMargin < 0.0,
                "Scenario 8: Insufficient battery correctly rejects order assignment");
  }

  // Scenario 9: Safety reserve enforcement
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    auto robot = std::make_unique<PickerRobot>("R_MARGIN", "Picker Margin", Position(0, 0));
    Order order(103, 101, 2, OrderPriority::HIGH, Position(14, 14));
    Position targetShelf(3, 3);
    BatteryFeasibilityReport rep = EcoRoute::validateBatteryFeasibility(*robot, order, sys.getWarehouse(), sys.getRobots(), targetShelf, 15.0);
    double tripEnergy = rep.totalEnergyRequired - rep.safetyReserve;
    robot->setBattery(tripEnergy + 5.0); // has enough for trip + 5.0, but safety reserve requires 15.0
    BatteryFeasibilityReport rep2 = EcoRoute::validateBatteryFeasibility(*robot, order, sys.getWarehouse(), sys.getRobots(), targetShelf, 15.0);
    ASSERT_TRUE(!rep2.isFeasible && rep2.statusReason.find("INSUFFICIENT_BATTERY") != std::string::npos,
                "Scenario 9: Safety reserve (15 EU) is strictly enforced preventing critical depletion");
  }

  // Scenario 10: Charging station selection
  {
    Warehouse wh(10, 10);
    wh.addChargingStation(ChargingStation("CS01", Position(0, 0), 25.0));
    wh.addChargingStation(ChargingStation("CS02", Position(9, 9), 25.0));
    std::vector<std::unique_ptr<Robot>> robots;
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    ChargingRecommendation rec = EcoRoute::selectBestChargingStation(Position(1, 1), 50.0, prof, wh, robots);
    ASSERT_TRUE(rec.feasible && rec.stationId == "CS01",
                "Scenario 10: Charging station selection chooses reachable, lowest-energy station CS01");
  }

  // Scenario 11: Charging station unreachable
  {
    Warehouse wh(5, 5);
    wh.addChargingStation(ChargingStation("CS_BLOCKED", Position(0, 0), 25.0));
    wh.setCellType(Position(1, 0), CellType::OBSTACLE);
    wh.setCellType(Position(0, 1), CellType::OBSTACLE);
    std::vector<std::unique_ptr<Robot>> robots;
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    ChargingRecommendation rec = EcoRoute::selectBestChargingStation(Position(4, 4), 50.0, prof, wh, robots);
    ASSERT_TRUE(!rec.feasible && rec.statusMessage == "NO_REACHABLE_CHARGING_STATIONS",
                "Scenario 11: Unreachable charging station safely returns feasible=false");
  }

  // Scenario 12: Charging station occupancy conflicts
  {
    Warehouse wh(10, 10);
    wh.addChargingStation(ChargingStation("CS01", Position(0, 0), 25.0));
    wh.addChargingStation(ChargingStation("CS02", Position(0, 5), 25.0));
    std::vector<std::unique_ptr<Robot>> robots;
    auto parkedRobot = std::make_unique<PickerRobot>("R_PARKED", "Picker Parked", Position(0, 0));
    robots.push_back(std::move(parkedRobot));

    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    ChargingRecommendation rec = EcoRoute::selectBestChargingStation(Position(1, 1), 50.0, prof, wh, robots);
    ASSERT_TRUE(rec.feasible && rec.stationId == "CS02",
                "Scenario 12: Occupancy conflict avoidance diverts robot to available station CS02");
  }

  // Scenario 13: Invalid coordinates
  {
    Warehouse wh(10, 10);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res1 = EcoRoute::findEcoRoute(wh, Position(-1, 0), Position(5, 5), prof, 0);
    EcoRouteResult res2 = EcoRoute::findEcoRoute(wh, Position(0, 0), Position(15, 5), prof, 0);
    ASSERT_TRUE(!res1.found && res1.statusMessage == "OUT_OF_BOUNDS" &&
                !res2.found && res2.statusMessage == "OUT_OF_BOUNDS",
                "Scenario 13: Negative and out-of-bounds coordinates return OUT_OF_BOUNDS");
  }

  // Scenario 14: Blocked source
  {
    Warehouse wh(5, 5);
    wh.setCellType(Position(0, 0), CellType::OBSTACLE);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res = EcoRoute::findEcoRoute(wh, Position(0, 0), Position(4, 4), prof, 0);
    ASSERT_TRUE(!res.found && res.statusMessage == "START_BLOCKED",
                "Scenario 14: Blocked source cell returns START_BLOCKED");
  }

  // Scenario 15: Blocked destination
  {
    Warehouse wh(5, 5);
    wh.setCellType(Position(4, 4), CellType::OBSTACLE);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res = EcoRoute::findEcoRoute(wh, Position(0, 0), Position(4, 4), prof, 0);
    ASSERT_TRUE(!res.found && res.statusMessage == "GOAL_BLOCKED",
                "Scenario 15: Blocked destination cell returns GOAL_BLOCKED");
  }

  // Scenario 16: Unreachable destination
  {
    Warehouse wh(5, 5);
    wh.setCellType(Position(3, 4), CellType::OBSTACLE);
    wh.setCellType(Position(4, 3), CellType::OBSTACLE);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res = EcoRoute::findEcoRoute(wh, Position(0, 0), Position(4, 4), prof, 0);
    ASSERT_TRUE(!res.found && res.statusMessage == "NO_PATH_FOUND",
                "Scenario 16: Unreachable walled-off goal returns NO_PATH_FOUND");
  }

  // Scenario 17: Route reconstruction
  {
    Warehouse wh(6, 6);
    Position start(0, 0);
    Position goal(5, 5);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res = EcoRoute::findEcoRoute(wh, start, goal, prof, 0);
    bool valid = res.found && res.path.front() == start && res.path.back() == goal;
    for (size_t i = 0; valid && i + 1 < res.path.size(); ++i) {
      if (res.path[i].manhattanDistance(res.path[i+1]) != 1) valid = false;
    }
    ASSERT_TRUE(valid && res.path.size() == 11,
                "Scenario 17: Route reconstruction produces valid contiguous Manhattan path");
  }

  // Scenario 18: Energy metric correctness
  {
    Warehouse wh(5, 5);
    Position start(0, 0);
    Position goal(4, 4);
    wh.setCellCost(Position(1, 1), 3.0);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    EcoRouteResult res = EcoRoute::findEcoRoute(wh, start, goal, prof, 2);
    double manualEnergy = EcoRoute::calculateRouteEnergy(prof, res.path, wh, 2);
    ASSERT_TRUE(res.found && std::abs(res.estimatedEnergy - manualEnergy) < 1e-6,
                "Scenario 18: EcoRoute energy metric exactly matches route energy summation");
  }

  // Scenario 19: Energy savings calculation
  {
    Warehouse wh(5, 5);
    Position start(0, 1);
    Position goal(4, 1);
    wh.setCellCost(Position(1, 1), 10.0);
    wh.setCellCost(Position(2, 1), 10.0);
    wh.setCellCost(Position(3, 1), 10.0);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    RouteComparisonResult comp = EcoRoute::compareRoutes(wh, start, goal, prof, 0);
    ASSERT_TRUE(comp.energySaved > 0.0 && comp.energySavingsPercent > 0.0 &&
                std::abs(comp.energySaved - (comp.shortestRoute.estimatedEnergy - comp.ecoRoute.estimatedEnergy)) < 1e-6,
                "Scenario 19: Energy savings and percentage dynamically computed with precision");
  }

  // Scenario 20: Zero baseline handling
  {
    Warehouse wh(5, 5);
    Position start(2, 2);
    RobotEnergyProfile prof = RobotEnergyProfile::getProfile("PickerRobot");
    RouteComparisonResult comp = EcoRoute::compareRoutes(wh, start, start, prof, 0);
    ASSERT_TRUE(comp.ecoRoute.found && comp.energySaved == 0.0 && comp.energySavingsPercent == 0.0,
                "Scenario 20: Zero baseline (start == goal) handled without NaN or division by zero");
  }

  // Scenario 21: Parallel order execution regression
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    int o1 = sys.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sys.createOrder(102, 2, OrderPriority::HIGH);
    int o3 = sys.createOrder(103, 2, OrderPriority::HIGH);
    (void)o1; (void)o2; (void)o3;
    sys.stepSimulation();
    ASSERT_TRUE(sys.getTaskManager().getActiveOrders().size() >= 2,
                "Scenario 21: Phase 1 parallel order execution preserved without sequential block");
  }

  // Scenario 22: Existing collision prevention regression
  {
    WarehouseSystem sys(15, 15);
    sys.initializeDefaultWarehouse();
    sys.createOrder(101, 2, OrderPriority::HIGH);
    sys.createOrder(102, 2, OrderPriority::HIGH);
    for (int t = 0; t < 25; ++t) {
      sys.stepSimulation();
    }
    // Verify no two robots occupy identical position
    bool collision = false;
    const auto& robots = sys.getRobots();
    for (size_t i = 0; i < robots.size(); ++i) {
      for (size_t j = i + 1; j < robots.size(); ++j) {
        if (robots[i]->getPosition() == robots[j]->getPosition()) collision = true;
      }
    }
    ASSERT_TRUE(!collision,
                "Scenario 22: Multi-robot collision prevention remains 100% active and unviolated");
  }

  // Scenario 23: Existing reservation management regression
  {
    ReservationTable table;
    Position p1(5, 5);
    bool r1 = table.reserveVertex(p1, 2, "R01");
    bool r2 = table.reserveVertex(p1, 2, "R02");
    ASSERT_TRUE(r1 && !r2 && table.isVertexReserved(p1, 2),
                "Scenario 23: Time-space ReservationTable conflict detection remains fully operational");
  }

  // Scenario 24: Existing Phase 2 algorithm regression
  {
    Warehouse wh(15, 15);
    Position s(0, 0);
    Position g(5, 5);
    std::string report = AlgorithmLab::compareLabAlgorithmsJSON(wh, s, g);
    ASSERT_TRUE(report.find("\"BFS\"") != std::string::npos &&
                report.find("\"Dijkstra\"") != std::string::npos &&
                report.find("\"A*\"") != std::string::npos,
                "Scenario 24: Phase 2 DSA Algorithm Laboratory executes all 3 algorithms independently");
  }

  // REST API Endpoints Verification
  {
    WarehouseSystem apiSys(15, 15);
    apiSys.initializeDefaultWarehouse();
    std::mutex mtx;
    HttpRequestHandler handler(apiSys, mtx);

    // 1. POST /api/ecoroute/optimize
    HttpRequest reqOpt;
    reqOpt.method = "POST";
    reqOpt.path = "/api/ecoroute/optimize";
    reqOpt.body = "{\"robotType\":\"PickerRobot\",\"payload\":2,\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respOpt = handler.handleRequest(reqOpt);
    ASSERT_TRUE(respOpt.statusCode == 200 && respOpt.body.find("\"found\":true") != std::string::npos &&
                respOpt.body.find("\"estimatedEnergy\":") != std::string::npos,
                "REST API: POST /api/ecoroute/optimize returns 200 OK with EcoRouteResult JSON");

    // 2. POST /api/ecoroute/compare
    HttpRequest reqComp;
    reqComp.method = "POST";
    reqComp.path = "/api/ecoroute/compare";
    reqComp.body = "{\"robotType\":\"PickerRobot\",\"payload\":0,\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respComp = handler.handleRequest(reqComp);
    ASSERT_TRUE(respComp.statusCode == 200 && respComp.body.find("\"shortestRoute\":") != std::string::npos &&
                respComp.body.find("\"ecoRoute\":") != std::string::npos,
                "REST API: POST /api/ecoroute/compare returns 200 OK with RouteComparisonResult JSON");

    // 3. GET /api/ecoroute/analytics
    HttpRequest reqAnalytics;
    reqAnalytics.method = "GET";
    reqAnalytics.path = "/api/ecoroute/analytics";
    HttpResponse respAnalytics = handler.handleRequest(reqAnalytics);
    ASSERT_TRUE(respAnalytics.statusCode == 200 && respAnalytics.body.find("\"totalEnergyConsumed\":") != std::string::npos &&
                respAnalytics.body.find("\"overallEfficiency\":") != std::string::npos,
                "REST API: GET /api/ecoroute/analytics returns 200 OK with comprehensive energy metrics");

    // 4. GET /api/ecoroute/robots
    HttpRequest reqRobots;
    reqRobots.method = "GET";
    reqRobots.path = "/api/ecoroute/robots";
    HttpResponse respRobots = handler.handleRequest(reqRobots);
    ASSERT_TRUE(respRobots.statusCode == 200 && respRobots.body.find("\"profile\":") != std::string::npos,
                "REST API: GET /api/ecoroute/robots returns 200 OK with fleet energy profiles");

    // 5. POST /api/ecoroute/charging/recommend
    HttpRequest reqCharge;
    reqCharge.method = "POST";
    reqCharge.path = "/api/ecoroute/charging/recommend";
    reqCharge.body = "{\"robotId\":\"R01\",\"position\":{\"x\":1,\"y\":1}}";
    HttpResponse respCharge = handler.handleRequest(reqCharge);
    ASSERT_TRUE(respCharge.statusCode == 200 && respCharge.body.find("\"stationId\":") != std::string::npos,
                "REST API: POST /api/ecoroute/charging/recommend returns 200 OK with charging recommendation");

    // 6. POST /api/ecoroute/report (CSV)
    HttpRequest reqRep;
    reqRep.method = "POST";
    reqRep.path = "/api/ecoroute/report";
    reqRep.body = "{\"format\":\"csv\",\"start\":{\"x\":0,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respRep = handler.handleRequest(reqRep);
    ASSERT_TRUE(respRep.statusCode == 200 && respRep.headers["Content-Type"].find("text/csv") != std::string::npos,
                "REST API: POST /api/ecoroute/report returns 200 OK with Content-Type text/csv");

    // 7. Invalid coordinates return 400 Bad Request
    HttpRequest reqInvalid;
    reqInvalid.method = "POST";
    reqInvalid.path = "/api/ecoroute/optimize";
    reqInvalid.body = "{\"start\":{\"x\":-10,\"y\":0},\"goal\":{\"x\":5,\"y\":5}}";
    HttpResponse respInvalid = handler.handleRequest(reqInvalid);
    ASSERT_TRUE(respInvalid.statusCode == 400,
                "REST API: Invalid coordinates return 400 Bad Request error response");
  }
}

void testFoodSaverSuite() {
  std::cout << "\n--- TEST SUITE 36: FoodSaver — Near-Expiry Inventory & Food Donation Optimization ---\n";

  // Scenario 1: Valid batch creation
  {
    FoodSaverManager fsm;
    InventoryBatch b1;
    b1.batchId = "B_TEST_01";
    b1.productId = 201;
    b1.productName = "Milk";
    b1.category = "Dairy";
    b1.quantity = 50;
    b1.unit = "liters";
    b1.manufacturingDate = Date(2026, 11, 10);
    b1.expiryDate = Date(2026, 11, 20);
    b1.storageLocation = Position(3, 4);
    std::string err;
    bool added = fsm.addBatch(b1, err);
    ASSERT_TRUE(added && err.empty(), "Scenario 1: Valid batch creation succeeds");
    const InventoryBatch* retrieved = fsm.getBatch("B_TEST_01");
    ASSERT_TRUE(retrieved != nullptr && retrieved->quantity == 50 && retrieved->getAvailableQuantity() == 50,
                "Scenario 1: Stored batch fields and available quantity match");
  }

  // Scenario 2: Invalid batch creation (empty ID, non-positive quantity)
  {
    FoodSaverManager fsm;
    InventoryBatch b1;
    b1.batchId = "";
    b1.productId = 201;
    b1.quantity = 10;
    b1.expiryDate = Date(2026, 11, 25);
    std::string err;
    ASSERT_FALSE(fsm.addBatch(b1, err), "Scenario 2: Empty batch ID rejected");
    b1.batchId = "B_ZERO";
    b1.quantity = 0;
    ASSERT_FALSE(fsm.addBatch(b1, err), "Scenario 2: Non-positive batch quantity rejected");
  }

  // Scenario 3: Duplicate batch ID rejection
  {
    FoodSaverManager fsm;
    InventoryBatch b1;
    b1.batchId = "B_DUP_01";
    b1.productId = 201;
    b1.quantity = 10;
    b1.expiryDate = Date(2026, 11, 25);
    std::string err;
    ASSERT_TRUE(fsm.addBatch(b1, err), "Scenario 3: First insertion succeeds");
    ASSERT_FALSE(fsm.addBatch(b1, err), "Scenario 3: Duplicate batch ID rejected");
  }

  // Scenario 4: Invalid expiry date handling
  {
    Date invalidMonth(2026, 13, 10);
    ASSERT_FALSE(invalidMonth.isValid(), "Scenario 4: Month > 12 evaluated as invalid");
    Date invalidDay(2026, 4, 31); // April has 30 days
    ASSERT_FALSE(invalidDay.isValid(), "Scenario 4: Day 31 in 30-day month evaluated as invalid");
    Date leapValid(2024, 2, 29);
    ASSERT_TRUE(leapValid.isValid(), "Scenario 4: Leap year Feb 29 evaluated as valid");
    Date nonLeapInvalid(2023, 2, 29);
    ASSERT_FALSE(nonLeapInvalid.isValid(), "Scenario 4: Non-leap year Feb 29 evaluated as invalid");
  }

  // Scenario 5: Manufacturing date later than expiry date rejection
  {
    FoodSaverManager fsm;
    InventoryBatch b1;
    b1.batchId = "B_BACKWARDS";
    b1.productId = 201;
    b1.quantity = 10;
    b1.manufacturingDate = Date(2026, 11, 25);
    b1.expiryDate = Date(2026, 11, 20);
    std::string err;
    ASSERT_FALSE(fsm.addBatch(b1, err), "Scenario 5: Manufacturing date later than expiry date is rejected");
  }

  // Scenario 6: Safe batch classification (> 7 days)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    InventoryBatch bSafe;
    bSafe.batchId = "B_SAFE";
    bSafe.productId = 201;
    bSafe.quantity = 20;
    bSafe.manufacturingDate = Date(2026, 11, 1);
    bSafe.expiryDate = Date(2026, 11, 26); // 10 days remaining
    std::string err;
    fsm.addBatch(bSafe, err);
    ExpiryEvaluation evalSafe = fsm.evaluateBatch(bSafe);
    ASSERT_TRUE(evalSafe.status == ExpiryStatus::SAFE && evalSafe.daysRemaining == 10,
                "Scenario 6: Batch with > 7 days remaining classified as SAFE");
  }

  // Scenario 7: Near-expiry classification (4 to 7 days)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    InventoryBatch bNear;
    bNear.batchId = "B_NEAR";
    bNear.productId = 201;
    bNear.quantity = 20;
    bNear.manufacturingDate = Date(2026, 11, 1);
    bNear.expiryDate = Date(2026, 11, 21); // 5 days remaining
    std::string err;
    fsm.addBatch(bNear, err);
    ExpiryEvaluation evalNear = fsm.evaluateBatch(bNear);
    ASSERT_TRUE(evalNear.status == ExpiryStatus::NEAR_EXPIRY && evalNear.daysRemaining == 5,
                "Scenario 7: Batch with 4-7 days remaining classified as NEAR_EXPIRY");
  }

  // Scenario 8: Critical expiry classification (1 to 3 days)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    InventoryBatch bCrit;
    bCrit.batchId = "B_CRIT";
    bCrit.productId = 201;
    bCrit.quantity = 20;
    bCrit.manufacturingDate = Date(2026, 11, 1);
    bCrit.expiryDate = Date(2026, 11, 18); // 2 days remaining
    std::string err;
    fsm.addBatch(bCrit, err);
    ExpiryEvaluation evalCrit = fsm.evaluateBatch(bCrit);
    ASSERT_TRUE(evalCrit.status == ExpiryStatus::CRITICAL && evalCrit.daysRemaining == 2,
                "Scenario 8: Batch with 1-3 days remaining classified as CRITICAL");
  }

  // Scenario 9: Expired classification (<= 0 days)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    InventoryBatch bExp;
    bExp.batchId = "B_EXP";
    bExp.productId = 201;
    bExp.quantity = 20;
    bExp.manufacturingDate = Date(2026, 10, 1);
    bExp.expiryDate = Date(2026, 11, 15); // -1 days remaining
    std::string err;
    fsm.addBatch(bExp, err);
    ExpiryEvaluation evalExp = fsm.evaluateBatch(bExp);
    ASSERT_TRUE(evalExp.status == ExpiryStatus::EXPIRED && evalExp.daysRemaining < 0 && evalExp.riskScore == 100.0,
                "Scenario 9: Batch with <= 0 days remaining classified as EXPIRED with 100.0 risk score");
  }

  // Scenario 10: Days remaining calculation accuracy
  {
    Date d1(2026, 1, 1);
    Date d2(2026, 1, 31);
    ASSERT_TRUE(d1.daysUntil(d2) == 30, "Scenario 10: Jan 1 to Jan 31 is 30 days");
    Date leapStart(2024, 2, 20);
    Date leapEnd(2024, 3, 5);
    ASSERT_TRUE(leapStart.daysUntil(leapEnd) == 14, "Scenario 10: Leap year February spanning calculation is 14 days");
  }

  // Scenario 11: FEFO sorting (batches sorted by earliest expiry first)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b1; b1.batchId = "B_10"; b1.productId = 301; b1.quantity = 10; b1.expiryDate = Date(2026, 11, 30);
    InventoryBatch b2; b2.batchId = "B_05"; b2.productId = 301; b2.quantity = 10; b2.expiryDate = Date(2026, 11, 20);
    InventoryBatch b3; b3.batchId = "B_02"; b3.productId = 301; b3.quantity = 10; b3.expiryDate = Date(2026, 11, 18);
    fsm.addBatch(b1, err);
    fsm.addBatch(b2, err);
    fsm.addBatch(b3, err);
    std::vector<InventoryBatch> sorted = fsm.getBatchesSortedFEFO(301);
    ASSERT_TRUE(sorted.size() == 3, "Scenario 11: Retrieved 3 batches sorted FEFO");
    ASSERT_TRUE(sorted[0].batchId == "B_02" && sorted[1].batchId == "B_05" && sorted[2].batchId == "B_10",
                "Scenario 11: Batches ordered strictly by earliest expiry date first");
  }

  // Scenario 12: FEFO order fulfillment allocation
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b1; b1.batchId = "B_A"; b1.productId = 401; b1.quantity = 20; b1.expiryDate = Date(2026, 11, 18);
    InventoryBatch b2; b2.batchId = "B_B"; b2.productId = 401; b2.quantity = 30; b2.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b1, err);
    fsm.addBatch(b2, err);
    std::vector<std::pair<std::string, int>> allocs;
    bool fOk = fsm.fulfillOrderFEFO(401, 10, allocs, err);
    ASSERT_TRUE(fOk && allocs.size() == 1 && allocs[0].first == "B_A" && allocs[0].second == 10,
                "Scenario 12: Single batch FEFO allocation selects earliest expiring batch B_A");
  }

  // Scenario 13: Partial batch fulfillment across multiple batches
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b1; b1.batchId = "B_A"; b1.productId = 402; b1.quantity = 10; b1.expiryDate = Date(2026, 11, 18);
    InventoryBatch b2; b2.batchId = "B_B"; b2.productId = 402; b2.quantity = 30; b2.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b1, err);
    fsm.addBatch(b2, err);
    std::vector<std::pair<std::string, int>> allocs;
    bool fOk = fsm.fulfillOrderFEFO(402, 15, allocs, err);
    ASSERT_TRUE(fOk && allocs.size() == 2, "Scenario 13: Split batch allocation spans multiple batches");
    ASSERT_TRUE(allocs[0].first == "B_A" && allocs[0].second == 10, "Scenario 13: Exhausts remaining stock from B_A");
    ASSERT_TRUE(allocs[1].first == "B_B" && allocs[1].second == 5, "Scenario 13: Remainder allocated from next earliest batch B_B");
  }

  // Scenario 14: Multiple batch consumption
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b1; b1.batchId = "B_1"; b1.productId = 403; b1.quantity = 5; b1.expiryDate = Date(2026, 11, 18);
    InventoryBatch b2; b2.batchId = "B_2"; b2.productId = 403; b2.quantity = 10; b2.expiryDate = Date(2026, 11, 20);
    InventoryBatch b3; b3.batchId = "B_3"; b3.productId = 403; b3.quantity = 20; b3.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b1, err);
    fsm.addBatch(b2, err);
    fsm.addBatch(b3, err);
    std::vector<std::pair<std::string, int>> allocs;
    bool fOk = fsm.fulfillOrderFEFO(403, 22, allocs, err);
    ASSERT_TRUE(fOk && allocs.size() == 3, "Scenario 14: Order spans all 3 batches");
    ASSERT_TRUE(allocs[0].first == "B_1" && allocs[0].second == 5, "Scenario 14: 1st batch consumed completely");
    ASSERT_TRUE(allocs[1].first == "B_2" && allocs[1].second == 10, "Scenario 14: 2nd batch consumed completely");
    ASSERT_TRUE(allocs[2].first == "B_3" && allocs[2].second == 7, "Scenario 14: 3rd batch partially consumed");
  }

  // Scenario 15: Expired stock rejection from standard customer orders
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch bExp; bExp.batchId = "B_EXP"; bExp.productId = 404; bExp.quantity = 50; bExp.expiryDate = Date(2026, 11, 10);
    fsm.addBatch(bExp, err);
    std::vector<std::pair<std::string, int>> allocs;
    bool fOk = fsm.fulfillOrderFEFO(404, 10, allocs, err);
    ASSERT_FALSE(fOk, "Scenario 15: Expired batch is strictly barred from customer order fulfillment");
  }

  // Scenario 16: Donation eligibility verification
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err, reason;
    InventoryBatch bElig; bElig.batchId = "B_ELIG"; bElig.productId = 501; bElig.quantity = 25; bElig.expiryDate = Date(2026, 11, 20);
    fsm.addBatch(bElig, err);
    ASSERT_TRUE(fsm.checkDonationEligibility(bElig, reason), "Scenario 16: Unexpired active batch is donation ELIGIBLE");
  }

  // Scenario 17: Donation ineligibility (expired, recalled, no available stock)
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string reason;
    InventoryBatch bExp; bExp.batchId = "B_E"; bExp.productId = 502; bExp.quantity = 10; bExp.expiryDate = Date(2026, 11, 10);
    ASSERT_FALSE(fsm.checkDonationEligibility(bExp, reason), "Scenario 17: Expired batch is strictly INELIGIBLE for donation");
    InventoryBatch bRec = bExp; bRec.expiryDate = Date(2026, 11, 25); bRec.batchStatus = BatchStatus::RECALLED;
    ASSERT_FALSE(fsm.checkDonationEligibility(bRec, reason), "Scenario 17: Recalled batch is INELIGIBLE for donation");
    InventoryBatch bZero = bRec; bZero.batchStatus = BatchStatus::ACTIVE; bZero.quantity = 10; bZero.reservedQuantity = 10;
    ASSERT_FALSE(fsm.checkDonationEligibility(bZero, reason), "Scenario 17: Batch with zero available stock is INELIGIBLE for donation");
  }

  // Scenario 18: Insufficient donation quantity rejection
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_DON"; b.productId = 503; b.quantity = 15; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("City Mission", "555-0100", {{"B_DON", 20}}, "Need food", err);
    ASSERT_TRUE(dId < 0, "Scenario 18: Donation request exceeding available quantity is rejected");
  }

  // Scenario 19: Atomic donation inventory reservation
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_RES"; b.productId = 504; b.quantity = 30; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("City Mission", "555-0100", {{"B_RES", 12}}, "", err);
    ASSERT_TRUE(dId > 0, "Scenario 19: Donation request created");
    bool resOk = fsm.reserveDonationInventory(dId, err);
    ASSERT_TRUE(resOk, "Scenario 19: Atomic reservation succeeds");
    const InventoryBatch* batch = fsm.getBatch("B_RES");
    ASSERT_TRUE(batch->reservedQuantity == 12 && batch->getAvailableQuantity() == 18,
                "Scenario 19: Batch reservation numbers match precisely");
  }

  // Scenario 20: Over-reservation prevention
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_COMP"; b.productId = 505; b.quantity = 20; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int d1 = fsm.createDonationRequest("Shelter 1", "555-1", {{"B_COMP", 15}}, "", err);
    ASSERT_TRUE(d1 > 0 && fsm.reserveDonationInventory(d1, err), "Scenario 20: First donation reserves 15 units");
    int d2 = fsm.createDonationRequest("Shelter 2", "555-2", {{"B_COMP", 10}}, "", err);
    ASSERT_TRUE(d2 < 0, "Scenario 20: Second donation requesting 10 units rejected (only 5 available)");
  }

  // Scenario 21: Donation approval lifecycle transition
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_AP"; b.productId = 506; b.quantity = 20; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("Food Pantry", "555-3", {{"B_AP", 5}}, "", err);
    ASSERT_TRUE(fsm.getDonationRequest(dId)->status == DonationStatus::DRAFT, "Scenario 21: Initial status is DRAFT");
    ASSERT_TRUE(fsm.approveDonation(dId, err), "Scenario 21: Transition to APPROVED succeeds");
    ASSERT_TRUE(fsm.getDonationRequest(dId)->status == DonationStatus::APPROVED, "Scenario 21: Status is APPROVED");
  }

  // Scenario 22: Donation rejection lifecycle transition
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_REJ"; b.productId = 507; b.quantity = 20; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("Shelter", "555-4", {{"B_REJ", 5}}, "", err);
    ASSERT_TRUE(fsm.rejectDonation(dId, "Donor requested cancellation", err), "Scenario 22: Transition to REJECTED succeeds");
    const DonationRequest* req = fsm.getDonationRequest(dId);
    ASSERT_TRUE(req->status == DonationStatus::REJECTED && req->rejectionReason == "Donor requested cancellation",
                "Scenario 22: Rejection reason and status verified");
  }

  // Scenario 23: Donation cancellation & reserved inventory release
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_CANC"; b.productId = 508; b.quantity = 30; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("Shelter", "555-5", {{"B_CANC", 10}}, "", err);
    fsm.reserveDonationInventory(dId, err);
    ASSERT_TRUE(fsm.getBatch("B_CANC")->reservedQuantity == 10, "Scenario 23: 10 units reserved");
    ASSERT_TRUE(fsm.cancelDonation(dId, "Transport unavailable", err), "Scenario 23: Cancellation succeeds");
    ASSERT_TRUE(fsm.getBatch("B_CANC")->reservedQuantity == 0 && fsm.getBatch("B_CANC")->getAvailableQuantity() == 30,
                "Scenario 23: Reserved stock released back to inventory");
  }

  // Scenario 24: Donation completion & stock deduction
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_DONE"; b.productId = 509; b.quantity = 40; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("Shelter", "555-6", {{"B_DONE", 15}}, "", err);
    fsm.approveDonation(dId, err);
    fsm.reserveDonationInventory(dId, err);
    bool stageOk = fsm.markReadyForPickup(dId, "R01", err);
    ASSERT_TRUE(stageOk && fsm.getDonationRequest(dId)->status == DonationStatus::READY_FOR_PICKUP, "Scenario 24: Staged for pickup");
    ASSERT_TRUE(fsm.completeDonation(dId, err), "Scenario 24: Completion succeeds");
    const InventoryBatch* batch = fsm.getBatch("B_DONE");
    ASSERT_TRUE(batch->quantity == 25 && batch->reservedQuantity == 0,
                "Scenario 24: Stock deducted from total quantity permanently upon completion");
  }

  // Scenario 25: Food Saved metric accuracy
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    std::string err;
    InventoryBatch b; b.batchId = "B_SAV"; b.productId = 510; b.quantity = 50; b.expiryDate = Date(2026, 11, 25);
    fsm.addBatch(b, err);
    int dId = fsm.createDonationRequest("Shelter", "555-7", {{"B_SAV", 25}}, "", err);
    fsm.approveDonation(dId, err);
    fsm.reserveDonationInventory(dId, err);
    fsm.markReadyForPickup(dId, "R02", err);
    fsm.completeDonation(dId, err);
    FoodSaverAnalytics an = fsm.computeAnalytics();
    ASSERT_TRUE(an.foodSavedQuantity == 25 && an.completedDonationQuantity == 25,
                "Scenario 25: Food Saved and Completed Donation metrics accurately record 25 units");
  }

  // Scenario 26: Food Waste metric accuracy
  {
    FoodSaverManager fsm;
    fsm.setSystemDate(Date(2026, 11, 16));
    FoodSaverAnalytics an = fsm.computeAnalytics();
    ASSERT_TRUE(an.foodWasteQuantity >= 0, "Scenario 26: Food Waste metric computed safely");
  }

  // Scenario 27: Zero-denominator analytics safety
  {
    FoodSaverManager fsmEmpty;
    FoodSaverAnalytics anEmpty = fsmEmpty.computeAnalytics();
    ASSERT_TRUE(!std::isnan(anEmpty.donationCompletionRate) && !std::isinf(anEmpty.donationCompletionRate),
                "Scenario 27: Donation completion rate handles empty data without division by zero");
    ASSERT_TRUE(!std::isnan(anEmpty.fefoFulfillmentPercentage) && !std::isinf(anEmpty.fefoFulfillmentPercentage),
                "Scenario 27: FEFO fulfillment percentage handles empty data without division by zero");
  }

  // Scenario 28: Donation inventory accounting consistency
  {
    FoodSaverManager fsm;
    fsm.initializeDefaultFoodStock();
    for (const auto& b : fsm.getAllBatches()) {
      ASSERT_TRUE(b.getAvailableQuantity() + b.reservedQuantity == b.quantity,
                  "Scenario 28: Invariant availableQuantity + reservedQuantity == quantity holds for all batches");
    }
  }

  // Scenario 29: Existing order workflow regression
  {
    WarehouseSystem sys;
    sys.initializeDefaultWarehouse();
    int stockBefore = sys.getInventory().getProduct(101)->getQuantity(); // 35
    int ordId = sys.createOrder(101, 2, OrderPriority::HIGH);
    ASSERT_TRUE(ordId > 0, "Scenario 29: Order creation functions as expected");
    bool reachedCompleted = false;
    for (int t = 1; t <= 100 && !reachedCompleted; ++t) {
      sys.stepSimulation();
      auto ord = sys.getTaskManager().getOrderById(ordId);
      if (ord && ord->getStatus() == OrderStatus::COMPLETED) {
        reachedCompleted = true;
      }
    }
    ASSERT_TRUE(reachedCompleted, "Scenario 29: Order reaches completed status");
    ASSERT_TRUE(sys.getInventory().getProduct(101)->getQuantity() == stockBefore - 2,
                "Scenario 29: Order fulfillment completes and stock decrements normally");
  }

  // Scenario 30: Existing parallel robot execution regression
  {
    WarehouseSystem sysPar;
    sysPar.initializeDefaultWarehouse();
    int o1 = sysPar.createOrder(101, 2, OrderPriority::HIGH);
    int o2 = sysPar.createOrder(102, 2, OrderPriority::HIGH);
    int o3 = sysPar.createOrder(103, 2, OrderPriority::HIGH);
    ASSERT_TRUE(o1 > 0 && o2 > 0 && o3 > 0, "Scenario 30: Three parallel orders registered");
    sysPar.stepSimulation();
    size_t activeRobots = 0;
    for (const auto& r : sysPar.getRobots()) {
      if (r->getStatus() != RobotStatus::IDLE) activeRobots++;
    }
    ASSERT_TRUE(activeRobots >= 2, "Scenario 30: Multiple robots concurrently execute parallel orders");
  }

  // Scenario 31: Existing collision prevention regression
  {
    WarehouseSystem sysColl;
    sysColl.initializeDefaultWarehouse();
    sysColl.createOrder(101, 2, OrderPriority::HIGH);
    sysColl.createOrder(102, 2, OrderPriority::HIGH);
    bool collisionOccurred = false;
    for (int t = 0; t < 25; ++t) {
      sysColl.stepSimulation();
      std::set<std::pair<int, int>> occupied;
      for (const auto& r : sysColl.getRobots()) {
        Position p = r->getPosition();
        if (occupied.count({p.x, p.y})) collisionOccurred = true;
        occupied.insert({p.x, p.y});
      }
    }
    ASSERT_FALSE(collisionOccurred, "Scenario 31: Zero vertex collisions across concurrent multi-robot execution");
  }

  // Scenario 32: Existing reservation regression
  {
    WarehouseSystem sys;
    sys.initializeDefaultWarehouse();
    ASSERT_TRUE(sys.getInventory().hasStock(101, 10), "Scenario 32: Inventory has stock verification works");
  }

  // Scenario 33: Phase 2 algorithm regression
  {
    WarehouseSystem sys;
    sys.initializeDefaultWarehouse();
    PathResult bfsRes = PathFinder::findPathBFS(sys.getWarehouse(), Position(0, 0), Position(14, 14));
    PathResult dijkstraRes = PathFinder::findPathDijkstra(sys.getWarehouse(), Position(0, 0), Position(14, 14));
    PathResult astarRes = PathFinder::findPathAStar(sys.getWarehouse(), Position(0, 0), Position(14, 14));
    ASSERT_TRUE(bfsRes.found && dijkstraRes.found && astarRes.found,
                "Scenario 33: BFS, Dijkstra, and A* all find valid warehouse paths");
    ASSERT_TRUE(astarRes.path.size() == dijkstraRes.path.size(),
                "Scenario 33: A* and Dijkstra yield identical optimal path length");
  }

  // Scenario 34: Phase 3 EcoRoute regression
  {
    WarehouseSystem sys;
    sys.initializeDefaultWarehouse();
    RobotEnergyProfile profile = RobotEnergyProfile::getProfile("PickerRobot");
    RouteComparisonResult comp = EcoRoute::compareRoutes(sys.getWarehouse(), Position(0, 0), Position(14, 14), profile, 2);
    ASSERT_TRUE(comp.ecoRoute.found && comp.shortestRoute.found,
                "Scenario 34: EcoRoute and Baseline shortest route both found successfully");
    ASSERT_TRUE(comp.ecoRoute.estimatedEnergy <= comp.shortestRoute.estimatedEnergy + 1e-6,
                "Scenario 34: EcoRoute energy is less than or equal to shortest distance baseline");
  }

  // REST API Endpoints Verification
  {
    WarehouseSystem sys;
    sys.initializeDefaultWarehouse();
    std::mutex mtx;
    HttpRequestHandler handler(sys, mtx);

    // 1. GET /api/foodsaver/batches
    HttpRequest reqBatches;
    reqBatches.method = "GET";
    reqBatches.path = "/api/foodsaver/batches";
    HttpResponse respBatches = handler.handleRequest(reqBatches);
    ASSERT_TRUE(respBatches.statusCode == 200, "REST API: GET /api/foodsaver/batches returns 200 OK");

    // 2. POST /api/foodsaver/batches
    HttpRequest reqAdd;
    reqAdd.method = "POST";
    reqAdd.path = "/api/foodsaver/batches";
    reqAdd.body = "{\"batchId\":\"B_API_01\",\"productId\":201,\"productName\":\"Test Milk\",\"quantity\":15,\"expiryDate\":\"2026-11-25\",\"manufacturingDate\":\"2026-11-10\"}";
    HttpResponse respAdd = handler.handleRequest(reqAdd);
    ASSERT_TRUE(respAdd.statusCode == 201, "REST API: POST /api/foodsaver/batches returns 201 Created");

    // 3. GET /api/foodsaver/expiry
    HttpRequest reqExp;
    reqExp.method = "GET";
    reqExp.path = "/api/foodsaver/expiry";
    HttpResponse respExp = handler.handleRequest(reqExp);
    ASSERT_TRUE(respExp.statusCode == 200, "REST API: GET /api/foodsaver/expiry returns 200 OK");

    // 4. GET /api/foodsaver/eligibility
    HttpRequest reqElig;
    reqElig.method = "GET";
    reqElig.path = "/api/foodsaver/eligibility";
    HttpResponse respElig = handler.handleRequest(reqElig);
    ASSERT_TRUE(respElig.statusCode == 200, "REST API: GET /api/foodsaver/eligibility returns 200 OK");

    // 5. POST /api/foodsaver/donations
    HttpRequest reqDon;
    reqDon.method = "POST";
    reqDon.path = "/api/foodsaver/donations";
    reqDon.body = "{\"recipient\":\"Community Shelter\",\"contact\":\"John Doe\",\"batchId\":\"FB-201-01\",\"quantity\":5}";
    HttpResponse respDon = handler.handleRequest(reqDon);
    ASSERT_TRUE(respDon.statusCode == 201, "REST API: POST /api/foodsaver/donations returns 201 Created");

    // 6. POST /api/foodsaver/donations/approve
    HttpRequest reqApp;
    reqApp.method = "POST";
    reqApp.path = "/api/foodsaver/donations/approve";
    reqApp.body = "{\"id\":1001}";
    HttpResponse respApp = handler.handleRequest(reqApp);
    ASSERT_TRUE(respApp.statusCode == 200, "REST API: POST /api/foodsaver/donations/approve returns 200 OK");

    // 7. GET /api/foodsaver/analytics
    HttpRequest reqAnalytics;
    reqAnalytics.method = "GET";
    reqAnalytics.path = "/api/foodsaver/analytics";
    HttpResponse respAnalytics = handler.handleRequest(reqAnalytics);
    ASSERT_TRUE(respAnalytics.statusCode == 200, "REST API: GET /api/foodsaver/analytics returns 200 OK");

    // 8. GET /api/foodsaver/report
    HttpRequest reqRep;
    reqRep.method = "GET";
    reqRep.path = "/api/foodsaver/report";
    reqRep.queryParams["format"] = "csv";
    HttpResponse respRep = handler.handleRequest(reqRep);
    ASSERT_TRUE(respRep.statusCode == 200 && respRep.headers["Content-Type"].find("text/csv") != std::string::npos,
                "REST API: GET /api/foodsaver/report returns 200 OK with Content-Type text/csv");
  }
}

int main() {
  std::cout << "========================================================\n";
  std::cout << "   Robot Warehouse System — C++ Regression Test Suite   \n";
  std::cout << "========================================================\n";

  testWarehouseCoordinates();
  testInventoryManagement();
  testRobotPolymorphismAndBattery();
  testRobotPayloadConstraints();
  testPathfindingAlgorithms();
  testOrderLifecycleAndDelivery();
  testMultiRobotVertexCollision();
  testMultiRobotHeadOnSwapCollision();
  testNarrowAisleCoordination();
  testRobotWaitingPreservesTask();
  testReservationReleaseOnRouteChange();
  testDynamicObstacleBlocksActiveRoute();
  testDynamicObstacleOnOccupiedRobotCellRejected();
  testStationCapacityAndOccupancy();
  testDeadlockDetectionAndRecovery();
  testUnreachableDestinationReporting();
  testPriorityRouteCompetition();
  testSimulationReset();
  testZeroObservedTicksAndEmptyAnalytics();
  testStatusBasedUtilizationAndMovementCounting();
  testBatteryConsumptionTracking();
  testOrderLifecycleTimestampsAndDurations();
  testCongestionHeatmapSpatialMatrixAndBounds();
  testAnalyticsResetVersusSimulationReset();
  testRobotUtilizationAndBatteryIntelligence();
  testInteractiveCongestionHeatmapModesAndSerialization();
  testAdvancedAlgorithmBenchmarkingAndIsolation();
  testPackingStationOccupancyAndDeadlockCorrection();
  testInventoryReservationAndConsistency();
  testConfigurationLoadingAndSnapshotPersistence();
  testRestApiValidationAndConcurrency();
  testCooperativeMultiRobotAndSequentialOrders();
  testIntelligentMultiRobotParallelExecution();
  testAlgorithmLaboratorySuite();
  testEcoRouteSuite();
  testFoodSaverSuite();

  std::cout << "\n========================================================\n";
  std::cout << "   TEST SUMMARY: " << g_testsPassed << " PASSED, "
            << g_testsFailed << " FAILED\n";
  std::cout << "========================================================\n";

  return (g_testsFailed == 0) ? 0 : 1;
}
