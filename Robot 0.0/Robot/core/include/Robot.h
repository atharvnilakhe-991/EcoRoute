#ifndef ROBOT_H
#define ROBOT_H

#include <string>
#include <vector>
#include <memory>
#include "Position.h"
#include "Task.h"

enum class RobotStatus {
    IDLE,
    MOVING_TO_PICK,
    PICKING,
    MOVING_TO_PACK,
    DELIVERING,
    RETURNING_TO_CHARGE,
    CHARGING,
    LOW_BATTERY,
    WAITING,
    BLOCKED
};

std::string robotStatusToString(RobotStatus status);
RobotStatus stringToRobotStatus(const std::string& str);
bool tryStringToRobotStatus(const std::string& str, RobotStatus& outStatus);

class Robot {
protected:
    std::string id;
    std::string name;
    double battery; // 0.0 to 100.0
    Position position;
    RobotStatus status;
    std::shared_ptr<Task> currentTask;
    std::vector<Position> currentRoute;
    size_t routeIndex;
    int payloadCapacity;
    int currentPayload;
    int waitTicks; // consecutive ticks waiting for clearance

    Position stagingDestination; // Explicit staging cell destination (-1, -1 when not staging)

public:
    Robot(const std::string& id, const std::string& name, Position initialPos, int capacity);
    virtual ~Robot() = default;

    // Getters
    std::string getId() const;
    std::string getName() const;
    double getBattery() const;
    Position getPosition() const;
    RobotStatus getStatus() const;
    int getPayloadCapacity() const;
    int getCurrentPayload() const;
    const std::vector<Position>& getCurrentRoute() const;
    size_t getRouteIndex() const;
    std::shared_ptr<Task> getCurrentTask() const;
    int getWaitTicks() const;

    // Staging Lifecycle
    bool isStaging() const;
    Position getStagingDestination() const;
    void setStagingRoute(const Position& stagingDest, const std::vector<Position>& route);
    void clearStaging();

    // Setters / Modifiers
    void setPosition(const Position& pos);
    void setStatus(RobotStatus newStatus);
    void setBattery(double bat);
    void setCurrentPayload(int payload);
    void setWaitTicks(int ticks);
    void setCurrentTask(std::shared_ptr<Task> task);
    void setRoute(const std::vector<Position>& route, size_t index = 0);
    void charge(double amount);
    void consumeBattery(double amount);
    void incrementWaitTicks();
    void resetWaitTicks();

    // Navigation and Task assignment
    void assignTask(std::shared_ptr<Task> task, const std::vector<Position>& route);
    void setCurrentRoute(const std::vector<Position>& route);
    void clearTask();
    bool hasActiveRoute() const;
    Position getNextRoutePosition() const;

    // Polymorphic operations
    virtual bool moveNextStep() = 0;
    virtual bool pickItem(int productId, int quantity) = 0;
    virtual bool deliverItem() = 0;
    virtual double getBatteryDrainPerMove() const = 0;
    virtual std::string getRobotTypeName() const = 0;

    virtual std::string toJSON() const;
};

#endif // ROBOT_H
