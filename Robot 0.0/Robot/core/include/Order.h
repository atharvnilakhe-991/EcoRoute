#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>
#include "Position.h"

enum class OrderPriority {
    LOW,
    MEDIUM,
    HIGH
};

enum class OrderStatus {
    PENDING,
    ASSIGNED,
    PICKING,
    DELIVERING,
    COMPLETED,
    CANCELLED
};

std::string orderPriorityToString(OrderPriority priority);
std::string orderStatusToString(OrderStatus status);
OrderPriority stringToOrderPriority(const std::string& str);
bool tryStringToOrderPriority(const std::string& str, OrderPriority& outPriority);
bool tryStringToOrderStatus(const std::string& str, OrderStatus& outStatus);

class Order {
private:
    static int nextOrderId;
    int id;
    int productId;
    int quantity;
    OrderPriority priority;
    OrderStatus status;
    std::string assignedRobotId;
    Position packingDestination;
    int createdTick;
    int assignedTick;
    int completedTick;
    bool completedRecorded;
    int deliveredQuantity;
    int pickedQuantity;
    std::vector<std::string> teamRobotIds;

public:
    Order();
    Order(int productId, int quantity, OrderPriority priority, Position packingDest);
    Order(int id, int productId, int quantity, OrderPriority priority, Position packingDest);

    static void setNextOrderId(int nextId);
    static int getNextOrderId();
    void setId(int newId);

    int getId() const;
    int getProductId() const;
    int getQuantity() const;
    OrderPriority getPriority() const;
    OrderStatus getStatus() const;
    std::string getAssignedRobotId() const;
    Position getPackingDestination() const;
    int getCreatedTick() const;
    int getAssignedTick() const;
    int getCompletedTick() const;
    int getCompletionDuration() const;
    bool isCompletedRecorded() const;
    int getDeliveredQuantity() const;
    int getPickedQuantity() const;
    bool isFullyDelivered() const;
    const std::vector<std::string>& getTeamRobotIds() const;

    void setStatus(OrderStatus newStatus);
    void setAssignedRobotId(const std::string& robotId);
    void setPackingDestination(const Position& pos);
    void setCreatedTick(int tick);
    void setAssignedTick(int tick);
    void setCompletedTick(int tick);
    void markCompletedRecorded();
    void recordDelivered(int qty);
    void recordPicked(int qty);
    void setDeliveredQuantity(int qty);
    void setPickedQuantity(int qty);
    void setTeamRobotIds(const std::vector<std::string>& team);
    void addTeamRobotId(const std::string& robotId);

    std::string toJSON() const;
};

#endif // ORDER_H
