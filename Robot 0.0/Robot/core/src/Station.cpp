#include "../include/Station.h"
#include <sstream>

Station::Station(const std::string& id, Position pos)
    : id(id), position(pos), occupied(false), occupyingRobotId("") {}

std::string Station::getId() const { return id; }
Position Station::getPosition() const { return position; }
bool Station::isOccupied() const { return occupied; }
std::string Station::getOccupyingRobotId() const { return occupyingRobotId; }
void Station::setOccupied(bool occ, const std::string& robotId) {
    occupied = occ;
    occupyingRobotId = occ ? robotId : "";
}

std::string Station::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << id << "\","
       << "\"type\":\"" << getStationType() << "\","
       << "\"position\":{\"x\":" << position.x << ",\"y\":" << position.y << "},"
       << "\"occupied\":" << (occupied ? "true" : "false") << ","
       << "\"occupyingRobotId\":\"" << occupyingRobotId << "\""
       << "}";
    return ss.str();
}
