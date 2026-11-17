#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <vector>
#include <deque>
#include <memory>
#include "Order.h"
#include "Robot.h"
#include "Warehouse.h"
#include "Inventory.h"

class WarehouseAnalytics;
class MultiRobotCoordinator;

class TaskManager {
private:
    std::deque<std::shared_ptr<Order>> pendingOrders;
    std::vector<std::shared_ptr<Order>> activeOrders;
    std::vector<std::shared_ptr<Order>> completedOrders;

public:
    TaskManager();

    void addOrder(std::shared_ptr<Order> order);
    void clearOrders();
    void addPendingOrder(std::shared_ptr<Order> order);
    void addActiveOrder(std::shared_ptr<Order> order);
    void addCompletedOrder(std::shared_ptr<Order> order);
    std::shared_ptr<Order> getOrderById(int orderId);

    const std::deque<std::shared_ptr<Order>>& getPendingOrders() const;
    const std::vector<std::shared_ptr<Order>>& getActiveOrders() const;
    const std::vector<std::shared_ptr<Order>>& getCompletedOrders() const;

    // Evaluates and selects the best robot suited for an order
    Robot* selectBestRobot(const Order& order, const std::vector<std::unique_ptr<Robot>>& robots, const Warehouse& warehouse, const Position& targetShelfPos);

    // Dispatches pending orders to available robots (strict sequential processing & cooperative team allocation)
    void assignPendingOrders(const std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, Inventory& inventory, const std::string& algorithm = "A*", int currentTick = 0, WarehouseAnalytics* analytics = nullptr, MultiRobotCoordinator* coordinator = nullptr);

    // Advances active orders and robot workflows by 1 simulation tick (synchronized picking, handoffs & delivery)
    void updateActiveOrders(const std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, Inventory& inventory, bool advanceMovement = false, int currentTick = 0, WarehouseAnalytics* analytics = nullptr, MultiRobotCoordinator* coordinator = nullptr);

    // Finds nearest unoccupied, walkable, non-station staging cell for a robot vacating a station
    Position findStagingCell(const Position& currentPos, const Warehouse& warehouse, const std::vector<std::unique_ptr<Robot>>& robots) const;

    // Order Lifecycle Management & Failure Recovery
    bool cancelOrder(int orderId, Inventory& inventory);
    bool failOrder(int orderId, Inventory& inventory, const std::string& reason = "");
    bool reassignOrder(int orderId, const std::string& newRobotId, const std::vector<std::unique_ptr<Robot>>& robots, const Warehouse& warehouse, Inventory& inventory);

    std::string toJSON() const;
};

#endif // TASK_MANAGER_H
