#include "../include/Robot.h"
#include <algorithm>
#include <sstream>

std::string robotStatusToString(RobotStatus status) {
    switch (status) {
        case RobotStatus::IDLE: return "IDLE";
        case RobotStatus::MOVING_TO_PICK: return "MOVING_TO_PICK";
        case RobotStatus::PICKING: return "PICKING";
        case RobotStatus::MOVING_TO_PACK: return "MOVING_TO_PACK";
        case RobotStatus::DELIVERING: return "DELIVERING";
        case RobotStatus::RETURNING_TO_CHARGE: return "RETURNING_TO_CHARGE";
        case RobotStatus::CHARGING: return "CHARGING";
        case RobotStatus::LOW_BATTERY: return "LOW_BATTERY";
        case RobotStatus::WAITING: return "WAITING";
        case RobotStatus::BLOCKED: return "BLOCKED";
        default: return "UNKNOWN";
    }
}

bool tryStringToRobotStatus(const std::string& str, RobotStatus& outStatus) {
    if (str == "IDLE") { outStatus = RobotStatus::IDLE; return true; }
    if (str == "MOVING_TO_PICK") { outStatus = RobotStatus::MOVING_TO_PICK; return true; }
    if (str == "PICKING") { outStatus = RobotStatus::PICKING; return true; }
    if (str == "MOVING_TO_PACK") { outStatus = RobotStatus::MOVING_TO_PACK; return true; }
    if (str == "DELIVERING") { outStatus = RobotStatus::DELIVERING; return true; }
    if (str == "RETURNING_TO_CHARGE") { outStatus = RobotStatus::RETURNING_TO_CHARGE; return true; }
    if (str == "CHARGING") { outStatus = RobotStatus::CHARGING; return true; }
    if (str == "LOW_BATTERY") { outStatus = RobotStatus::LOW_BATTERY; return true; }
    if (str == "WAITING") { outStatus = RobotStatus::WAITING; return true; }
    if (str == "BLOCKED") { outStatus = RobotStatus::BLOCKED; return true; }
    return false;
}

RobotStatus stringToRobotStatus(const std::string& str) {
    RobotStatus s = RobotStatus::IDLE;
    tryStringToRobotStatus(str, s);
    return s;
}

Robot::Robot(const std::string& id, const std::string& name, Position initialPos, int capacity)
    : id(id), name(name), battery(100.0), position(initialPos),
      status(RobotStatus::IDLE), currentTask(nullptr), routeIndex(0),
      payloadCapacity(capacity), currentPayload(0), waitTicks(0),
      stagingDestination(Position(-1, -1)) {}

std::string Robot::getId() const { return id; }
std::string Robot::getName() const { return name; }
double Robot::getBattery() const { return battery; }
Position Robot::getPosition() const { return position; }
RobotStatus Robot::getStatus() const { return status; }
int Robot::getPayloadCapacity() const { return payloadCapacity; }
int Robot::getCurrentPayload() const { return currentPayload; }
const std::vector<Position>& Robot::getCurrentRoute() const { return currentRoute; }
size_t Robot::getRouteIndex() const { return routeIndex; }
std::shared_ptr<Task> Robot::getCurrentTask() const { return currentTask; }
int Robot::getWaitTicks() const { return waitTicks; }

bool Robot::isStaging() const {
    return stagingDestination != Position(-1, -1) && hasActiveRoute();
}

Position Robot::getStagingDestination() const {
    return stagingDestination;
}

void Robot::setStagingRoute(const Position& stagingDest, const std::vector<Position>& route) {
    stagingDestination = stagingDest;
    setCurrentRoute(route);
    resetWaitTicks();
}

void Robot::clearStaging() {
    stagingDestination = Position(-1, -1);
}

void Robot::setPosition(const Position& pos) {
    position = pos;
}

void Robot::setStatus(RobotStatus newStatus) {
    status = newStatus;
}

void Robot::setBattery(double bat) {
    battery = std::max(0.0, std::min(100.0, bat));
}

void Robot::setCurrentPayload(int payload) {
    currentPayload = std::max(0, payload);
}

void Robot::setWaitTicks(int ticks) {
    waitTicks = std::max(0, ticks);
}

void Robot::setCurrentTask(std::shared_ptr<Task> task) {
    currentTask = task;
}

void Robot::setRoute(const std::vector<Position>& route, size_t index) {
    currentRoute = route;
    routeIndex = (index <= route.size()) ? index : 0;
}

void Robot::charge(double amount) {
    battery = std::min(100.0, battery + amount);
    if (battery >= 99.0 && status == RobotStatus::CHARGING) {
        status = RobotStatus::IDLE;
        resetWaitTicks();
    }
}

void Robot::consumeBattery(double amount) {
    battery = std::max(0.0, battery - amount);
    if (battery < 15.0 && status != RobotStatus::CHARGING && status != RobotStatus::RETURNING_TO_CHARGE) {
        status = RobotStatus::LOW_BATTERY;
    }
}

void Robot::incrementWaitTicks() {
    waitTicks++;
}

void Robot::resetWaitTicks() {
    waitTicks = 0;
}

void Robot::assignTask(std::shared_ptr<Task> task, const std::vector<Position>& route) {
    stagingDestination = Position(-1, -1);
    currentTask = task;
    currentRoute = route;
    routeIndex = 0;
    if (!currentRoute.empty() && currentRoute[0] == position) {
        routeIndex = 1;
    }
    resetWaitTicks();

    if (task) {
        if (task->getType() == TaskType::MOVE_TO_PICK || task->getType() == TaskType::MOVE_TO_HANDOFF) {
            status = RobotStatus::MOVING_TO_PICK;
        } else if (task->getType() == TaskType::MOVE_TO_PACK) {
            status = RobotStatus::MOVING_TO_PACK;
        } else if (task->getType() == TaskType::RETURN_TO_CHARGE) {
            status = RobotStatus::RETURNING_TO_CHARGE;
        }
    }
}

void Robot::setCurrentRoute(const std::vector<Position>& route) {
    currentRoute = route;
    routeIndex = 0;
    if (!currentRoute.empty() && currentRoute[0] == position) {
        routeIndex = 1;
    }
    resetWaitTicks();
}

void Robot::clearTask() {
    stagingDestination = Position(-1, -1);
    currentTask = nullptr;
    currentRoute.clear();
    routeIndex = 0;
    resetWaitTicks();
    if (battery >= 15.0) {
        status = RobotStatus::IDLE;
    } else {
        status = RobotStatus::LOW_BATTERY;
    }
}

bool Robot::hasActiveRoute() const {
    if (currentRoute.empty() || routeIndex >= currentRoute.size()) {
        return false;
    }
    if (routeIndex == currentRoute.size() - 1 && currentRoute[routeIndex] == position) {
        return false;
    }
    return true;
}

Position Robot::getNextRoutePosition() const {
    if (hasActiveRoute()) {
        if (currentRoute[routeIndex] == position && routeIndex + 1 < currentRoute.size()) {
            return currentRoute[routeIndex + 1];
        }
        return currentRoute[routeIndex];
    }
    return position;
}

std::string Robot::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << id << "\","
       << "\"name\":\"" << name << "\","
       << "\"type\":\"" << getRobotTypeName() << "\","
       << "\"battery\":" << battery << ","
       << "\"position\":{\"x\":" << position.x << ",\"y\":" << position.y << "},"
       << "\"status\":\"" << robotStatusToString(status) << "\","
       << "\"capacity\":" << payloadCapacity << ","
       << "\"currentPayload\":" << currentPayload << ","
       << "\"waitTicks\":" << waitTicks << ","
       << "\"routeIndex\":" << routeIndex << ","
       << "\"route\":[";
    for (size_t i = 0; i < currentRoute.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << currentRoute[i].x << ",\"y\":" << currentRoute[i].y << "}";
    }
    ss << "],";
    if (currentTask) {
        ss << "\"currentTask\":" << currentTask->toJSON();
    } else {
        ss << "\"currentTask\":null";
    }
    ss << "}";
    return ss.str();
}
