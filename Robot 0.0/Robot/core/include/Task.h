#ifndef TASK_H
#define TASK_H

#include <string>
#include "Position.h"

enum class TaskType {
    NONE,
    MOVE_TO_PICK,
    PICK_ITEM,
    MOVE_TO_PACK,
    DELIVER_ITEM,
    RETURN_TO_CHARGE,
    CHARGE,
    MOVE_TO_HANDOFF,
    HANDOFF_ITEM
};

std::string taskTypeToString(TaskType type);
TaskType stringToTaskType(const std::string& str);
bool tryStringToTaskType(const std::string& str, TaskType& outType);

class Task {
private:
    static int nextTaskId;
    int id;
    int orderId;
    TaskType type;
    Position targetLocation;
    int productId;
    int quantity;
    bool completed;
    std::string partnerRobotId;

public:
    Task();
    Task(int orderId, TaskType type, Position targetLocation, int productId = 0, int quantity = 0);
    Task(int id, int orderId, TaskType type, Position targetLocation, int productId, int quantity, bool completed);

    static void setNextTaskId(int nextId);
    static int getNextTaskId();

    int getId() const;
    int getOrderId() const;
    TaskType getType() const;
    Position getTargetLocation() const;
    int getProductId() const;
    int getQuantity() const;
    bool isCompleted() const;
    std::string getPartnerRobotId() const;

    void setTargetLocation(const Position& pos);
    void setType(TaskType newType);
    void setPartnerRobotId(const std::string& partnerId);
    void setCompleted(bool val);
    std::string toJSON() const;
};

#endif // TASK_H
