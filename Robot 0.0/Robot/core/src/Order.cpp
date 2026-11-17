#include "../include/Order.h"
#include <sstream>

int Order::nextOrderId = 1001;

std::string orderPriorityToString(OrderPriority priority) {
    switch (priority) {
        case OrderPriority::HIGH: return "HIGH";
        case OrderPriority::MEDIUM: return "MEDIUM";
        case OrderPriority::LOW: default: return "LOW";
    }
}

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::ASSIGNED: return "ASSIGNED";
        case OrderStatus::PICKING: return "PICKING";
        case OrderStatus::DELIVERING: return "DELIVERING";
        case OrderStatus::COMPLETED: return "COMPLETED";
        case OrderStatus::CANCELLED: default: return "CANCELLED";
    }
}

bool tryStringToOrderPriority(const std::string& str, OrderPriority& outPriority) {
    if (str == "HIGH") { outPriority = OrderPriority::HIGH; return true; }
    if (str == "MEDIUM") { outPriority = OrderPriority::MEDIUM; return true; }
    if (str == "LOW") { outPriority = OrderPriority::LOW; return true; }
    return false;
}

bool tryStringToOrderStatus(const std::string& str, OrderStatus& outStatus) {
    if (str == "PENDING") { outStatus = OrderStatus::PENDING; return true; }
    if (str == "ASSIGNED") { outStatus = OrderStatus::ASSIGNED; return true; }
    if (str == "PICKING") { outStatus = OrderStatus::PICKING; return true; }
    if (str == "DELIVERING") { outStatus = OrderStatus::DELIVERING; return true; }
    if (str == "COMPLETED") { outStatus = OrderStatus::COMPLETED; return true; }
    if (str == "CANCELLED") { outStatus = OrderStatus::CANCELLED; return true; }
    return false;
}

OrderPriority stringToOrderPriority(const std::string& str) {
    OrderPriority p = OrderPriority::LOW;
    tryStringToOrderPriority(str, p);
    return p;
}

Order::Order() : id(0), productId(0), quantity(0), priority(OrderPriority::LOW),
                 status(OrderStatus::PENDING), assignedRobotId(""), packingDestination(Position(0, 0)),
                 createdTick(-1), assignedTick(-1), completedTick(-1), completedRecorded(false),
                 deliveredQuantity(0), pickedQuantity(0) {}

Order::Order(int productId, int quantity, OrderPriority priority, Position packingDest)
    : id(nextOrderId++), productId(productId), quantity(quantity), priority(priority),
      status(OrderStatus::PENDING), assignedRobotId(""), packingDestination(packingDest),
      createdTick(-1), assignedTick(-1), completedTick(-1), completedRecorded(false),
      deliveredQuantity(0), pickedQuantity(0) {}

Order::Order(int id, int productId, int quantity, OrderPriority priority, Position packingDest)
    : id(id), productId(productId), quantity(quantity), priority(priority),
      status(OrderStatus::PENDING), assignedRobotId(""), packingDestination(packingDest),
      createdTick(-1), assignedTick(-1), completedTick(-1), completedRecorded(false),
      deliveredQuantity(0), pickedQuantity(0) {
    if (id >= nextOrderId) {
        nextOrderId = id + 1;
    }
}

void Order::setNextOrderId(int nextId) {
    nextOrderId = std::max(1, nextId);
}

int Order::getNextOrderId() {
    return nextOrderId;
}

void Order::setId(int newId) {
    id = newId;
    if (id >= nextOrderId) {
        nextOrderId = id + 1;
    }
}

int Order::getId() const { return id; }
int Order::getProductId() const { return productId; }
int Order::getQuantity() const { return quantity; }
OrderPriority Order::getPriority() const { return priority; }
OrderStatus Order::getStatus() const { return status; }
std::string Order::getAssignedRobotId() const { return assignedRobotId; }
Position Order::getPackingDestination() const { return packingDestination; }
int Order::getCreatedTick() const { return createdTick; }
int Order::getAssignedTick() const { return assignedTick; }
int Order::getCompletedTick() const { return completedTick; }
bool Order::isCompletedRecorded() const { return completedRecorded; }

int Order::getCompletionDuration() const {
    if (completedTick >= 0 && createdTick >= 0 && completedTick >= createdTick) {
        return completedTick - createdTick;
    }
    return -1;
}

void Order::setStatus(OrderStatus newStatus) {
    status = newStatus;
}

void Order::setAssignedRobotId(const std::string& robotId) {
    assignedRobotId = robotId;
}

void Order::setPackingDestination(const Position& pos) {
    packingDestination = pos;
}

void Order::setCreatedTick(int tick) {
    createdTick = tick;
}

void Order::setAssignedTick(int tick) {
    assignedTick = tick;
}

void Order::setCompletedTick(int tick) {
    completedTick = tick;
}

void Order::markCompletedRecorded() {
    completedRecorded = true;
}

int Order::getDeliveredQuantity() const { return deliveredQuantity; }
int Order::getPickedQuantity() const { return pickedQuantity; }
bool Order::isFullyDelivered() const { return deliveredQuantity >= quantity; }
const std::vector<std::string>& Order::getTeamRobotIds() const { return teamRobotIds; }

void Order::recordDelivered(int qty) {
    deliveredQuantity += qty;
}

void Order::recordPicked(int qty) {
    pickedQuantity += qty;
}

void Order::setDeliveredQuantity(int qty) {
    deliveredQuantity = qty;
}

void Order::setPickedQuantity(int qty) {
    pickedQuantity = qty;
}

void Order::setTeamRobotIds(const std::vector<std::string>& team) {
    teamRobotIds = team;
}

void Order::addTeamRobotId(const std::string& robotId) {
    for (const auto& existing : teamRobotIds) {
        if (existing == robotId) return;
    }
    teamRobotIds.push_back(robotId);
}

std::string Order::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"productId\":" << productId << ","
       << "\"quantity\":" << quantity << ","
       << "\"deliveredQuantity\":" << deliveredQuantity << ","
       << "\"pickedQuantity\":" << pickedQuantity << ","
       << "\"priority\":\"" << orderPriorityToString(priority) << "\","
       << "\"status\":\"" << orderStatusToString(status) << "\","
       << "\"assignedRobotId\":\"" << assignedRobotId << "\","
       << "\"teamRobotIds\":[";
    for (size_t i = 0; i < teamRobotIds.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << teamRobotIds[i] << "\"";
    }
    ss << "],"
       << "\"packingDestination\":{\"x\":" << packingDestination.x << ",\"y\":" << packingDestination.y << "},"
       << "\"createdTick\":" << createdTick << ","
       << "\"assignedTick\":" << assignedTick << ","
       << "\"completedTick\":" << completedTick << ","
       << "\"duration\":" << getCompletionDuration()
       << "}";
    return ss.str();
}
