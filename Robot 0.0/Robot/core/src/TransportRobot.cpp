#include "../include/TransportRobot.h"

TransportRobot::TransportRobot(const std::string& id, const std::string& name, Position initialPos)
    : Robot(id, name, initialPos, 25), heavyPayloadLimit(25) {}

bool TransportRobot::moveNextStep() {
    if (!hasActiveRoute()) {
        return false;
    }
    if (currentRoute[routeIndex] == position && routeIndex + 1 < currentRoute.size()) {
        routeIndex++;
    }
    position = currentRoute[routeIndex];
    routeIndex++;
    // Heavier load increases battery drain slightly
    double loadFactor = 1.0 + (static_cast<double>(currentPayload) / payloadCapacity) * 0.4;
    consumeBattery(getBatteryDrainPerMove() * loadFactor);
    return true;
}

bool TransportRobot::pickItem(int productId, int quantity) {
    (void)productId;
    if (currentPayload + quantity > payloadCapacity) {
        return false;
    }
    currentPayload += quantity;
    consumeBattery(2.0); // Heavier lifting machinery
    status = RobotStatus::PICKING;
    return true;
}

bool TransportRobot::deliverItem() {
    currentPayload = 0;
    consumeBattery(1.0);
    status = RobotStatus::DELIVERING;
    return true;
}

double TransportRobot::getBatteryDrainPerMove() const {
    return 0.9;
}

std::string TransportRobot::getRobotTypeName() const {
    return "TransportRobot";
}
