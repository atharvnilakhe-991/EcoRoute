#include "../include/PickerRobot.h"
#include <iostream>

PickerRobot::PickerRobot(const std::string& id, const std::string& name, Position initialPos)
    : Robot(id, name, initialPos, 6), pickSpeedFactor(1.5) {}

bool PickerRobot::moveNextStep() {
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

bool PickerRobot::pickItem(int productId, int quantity) {
    (void)productId;
    if (currentPayload + quantity > payloadCapacity) {
        return false;
    }
    currentPayload += quantity;
    consumeBattery(1.0); // Lightweight picking drain
    status = RobotStatus::PICKING;
    return true;
}

bool PickerRobot::deliverItem() {
    currentPayload = 0;
    consumeBattery(0.5);
    status = RobotStatus::DELIVERING;
    return true;
}

double PickerRobot::getBatteryDrainPerMove() const {
    return 0.5; // Efficient movement
}

std::string PickerRobot::getRobotTypeName() const {
    return "PickerRobot";
}
