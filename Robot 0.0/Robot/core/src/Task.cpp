#include "../include/Task.h"
#include <sstream>

int Task::nextTaskId = 1;

std::string taskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::MOVE_TO_PICK: return "MOVE_TO_PICK";
        case TaskType::PICK_ITEM: return "PICK_ITEM";
        case TaskType::MOVE_TO_PACK: return "MOVE_TO_PACK";
        case TaskType::DELIVER_ITEM: return "DELIVER_ITEM";
        case TaskType::RETURN_TO_CHARGE: return "RETURN_TO_CHARGE";
        case TaskType::CHARGE: return "CHARGE";
        case TaskType::MOVE_TO_HANDOFF: return "MOVE_TO_HANDOFF";
        case TaskType::HANDOFF_ITEM: return "HANDOFF_ITEM";
        case TaskType::NONE: default: return "NONE";
    }
}

bool tryStringToTaskType(const std::string& str, TaskType& outType) {
    if (str == "MOVE_TO_PICK") { outType = TaskType::MOVE_TO_PICK; return true; }
    if (str == "PICK_ITEM") { outType = TaskType::PICK_ITEM; return true; }
    if (str == "MOVE_TO_PACK") { outType = TaskType::MOVE_TO_PACK; return true; }
    if (str == "DELIVER_ITEM") { outType = TaskType::DELIVER_ITEM; return true; }
    if (str == "RETURN_TO_CHARGE") { outType = TaskType::RETURN_TO_CHARGE; return true; }
    if (str == "CHARGE") { outType = TaskType::CHARGE; return true; }
    if (str == "MOVE_TO_HANDOFF") { outType = TaskType::MOVE_TO_HANDOFF; return true; }
    if (str == "HANDOFF_ITEM") { outType = TaskType::HANDOFF_ITEM; return true; }
    if (str == "NONE") { outType = TaskType::NONE; return true; }
    return false;
}

TaskType stringToTaskType(const std::string& str) {
    TaskType t = TaskType::NONE;
    tryStringToTaskType(str, t);
    return t;
}

Task::Task() : id(0), orderId(0), type(TaskType::NONE), targetLocation(Position(0, 0)), productId(0), quantity(0), completed(false), partnerRobotId("") {}

Task::Task(int orderId, TaskType type, Position targetLocation, int productId, int quantity)
    : id(nextTaskId++), orderId(orderId), type(type), targetLocation(targetLocation),
      productId(productId), quantity(quantity), completed(false), partnerRobotId("") {}

Task::Task(int id, int orderId, TaskType type, Position targetLocation, int productId, int quantity, bool completed)
    : id(id), orderId(orderId), type(type), targetLocation(targetLocation),
      productId(productId), quantity(quantity), completed(completed), partnerRobotId("") {
    if (id >= nextTaskId) {
        nextTaskId = id + 1;
    }
}

void Task::setNextTaskId(int nextId) {
    nextTaskId = std::max(1, nextId);
}

int Task::getNextTaskId() {
    return nextTaskId;
}

int Task::getId() const { return id; }
int Task::getOrderId() const { return orderId; }
TaskType Task::getType() const { return type; }
Position Task::getTargetLocation() const { return targetLocation; }
int Task::getProductId() const { return productId; }
int Task::getQuantity() const { return quantity; }
bool Task::isCompleted() const { return completed; }
std::string Task::getPartnerRobotId() const { return partnerRobotId; }

void Task::setTargetLocation(const Position& pos) { targetLocation = pos; }
void Task::setType(TaskType newType) { type = newType; }
void Task::setPartnerRobotId(const std::string& partnerId) { partnerRobotId = partnerId; }
void Task::setCompleted(bool val) {
    completed = val;
}

std::string Task::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"orderId\":" << orderId << ","
       << "\"type\":\"" << taskTypeToString(type) << "\","
       << "\"targetLocation\":{\"x\":" << targetLocation.x << ",\"y\":" << targetLocation.y << "},"
       << "\"productId\":" << productId << ","
       << "\"quantity\":" << quantity << ","
       << "\"partnerRobotId\":\"" << partnerRobotId << "\","
       << "\"completed\":" << (completed ? "true" : "false")
       << "}";
    return ss.str();
}
