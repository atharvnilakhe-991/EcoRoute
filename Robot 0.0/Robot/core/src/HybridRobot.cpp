#include "../include/HybridRobot.h"

HybridRobot::HybridRobot(const std::string& id, const std::string& name, Position initialPos)
    : Robot(id, name, initialPos, 12) {}

bool HybridRobot::moveNextStep() {
    if (!hasActiveRoute()) {
        return false;
    }
    if (currentRoute[routeIndex] == position && routeIndex + 1 < currentRoute.size()) {
        routeIndex++;
    }
    position = currentRoute[routeIndex];
    routeIndex++;
    consumeBattery(getBatteryDrainPerMove());
    return true;
}

bool HybridRobot::pickItem(int productId, int quantity) {
    (void)productId;
    if (currentPayload + quantity > payloadCapacity) {
        return false;
    }
    currentPayload += quantity;
    consumeBattery(1.5);
    status = RobotStatus::PICKING;
    return true;
}

bool HybridRobot::deliverItem() {
    currentPayload = 0;
    consumeBattery(0.7);
    status = RobotStatus::DELIVERING;
    return true;
}

double HybridRobot::getBatteryDrainPerMove() const {
    return 0.7;
}

std::string HybridRobot::getRobotTypeName() const {
    return "HybridRobot";
}
