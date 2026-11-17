#include "../include/ReservationTable.h"

void ReservationTable::setStationaryRobot(const std::string& robotId, const Position& pos) {
    stationaryRobots[robotId] = pos;
}

void ReservationTable::clearStationaryRobot(const std::string& robotId) {
    stationaryRobots.erase(robotId);
}

bool ReservationTable::isStationary(const std::string& robotId) const {
    return stationaryRobots.find(robotId) != stationaryRobots.end();
}

Position ReservationTable::getStationaryPosition(const std::string& robotId) const {
    auto it = stationaryRobots.find(robotId);
    if (it != stationaryRobots.end()) return it->second;
    return Position(-1, -1);
}

const std::map<std::string, Position>& ReservationTable::getStationaryRobots() const {
    return stationaryRobots;
}

bool ReservationTable::reserveVertex(const Position& pos, int tick, const std::string& robotId) {
    auto key = std::make_pair(tick, pos);
    auto it = vertexReservations.find(key);
    if (it != vertexReservations.end() && it->second != robotId) {
        return false; // Already reserved by another robot
    }
    vertexReservations[key] = robotId;
    return true;
}

bool ReservationTable::isVertexReserved(const Position& pos, int tick, const std::string& robotId) const {
    auto key = std::make_pair(tick, pos);
    auto it = vertexReservations.find(key);
    if (it != vertexReservations.end()) {
        if (!robotId.empty()) {
            return it->second == robotId;
        }
        return true;
    }
    if (!robotId.empty()) {
        auto sit = stationaryRobots.find(robotId);
        if (sit != stationaryRobots.end() && sit->second == pos) {
            return true;
        }
    } else {
        for (const auto& entry : stationaryRobots) {
            if (entry.second == pos) return true;
        }
    }
    return false;
}

bool ReservationTable::isVertexReservedByOther(const Position& pos, int tick, const std::string& robotId) const {
    auto key = std::make_pair(tick, pos);
    auto it = vertexReservations.find(key);
    if (it != vertexReservations.end()) {
        return it->second != robotId;
    }
    for (const auto& entry : stationaryRobots) {
        if (entry.first != robotId && entry.second == pos) {
            return true;
        }
    }
    return false;
}

std::string ReservationTable::getVertexReserver(const Position& pos, int tick) const {
    auto key = std::make_pair(tick, pos);
    auto it = vertexReservations.find(key);
    if (it != vertexReservations.end()) {
        return it->second;
    }
    for (const auto& entry : stationaryRobots) {
        if (entry.second == pos) {
            return entry.first;
        }
    }
    return "";
}

bool ReservationTable::reserveEdge(const Position& from, const Position& to, int tick, const std::string& robotId) {
    if (isEdgeReservedByOther(from, to, tick, robotId)) {
        return false;
    }
    edgeReservations[std::make_tuple(tick, from, to)] = robotId;
    return true;
}

bool ReservationTable::isEdgeReserved(const Position& from, const Position& to, int tick, const std::string& robotId) const {
    auto itDirect = edgeReservations.find(std::make_tuple(tick, from, to));
    if (itDirect != edgeReservations.end()) {
        if (!robotId.empty()) return itDirect->second == robotId;
        return true;
    }
    return false;
}

bool ReservationTable::isEdgeReservedByOther(const Position& from, const Position& to, int tick, const std::string& robotId) const {
    // Check if another robot is moving from -> to at this tick
    auto itDirect = edgeReservations.find(std::make_tuple(tick, from, to));
    if (itDirect != edgeReservations.end() && itDirect->second != robotId) {
        return true;
    }
    // Check if another robot is traversing the opposite direction to -> from at this tick (head-on edge collision)
    auto itReverse = edgeReservations.find(std::make_tuple(tick, to, from));
    if (itReverse != edgeReservations.end() && itReverse->second != robotId) {
        return true;
    }
    return false;
}

void ReservationTable::releaseFutureReservations(const std::string& robotId, int fromTick) {
    for (auto it = vertexReservations.begin(); it != vertexReservations.end(); ) {
        if (it->first.first >= fromTick && it->second == robotId) {
            it = vertexReservations.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = edgeReservations.begin(); it != edgeReservations.end(); ) {
        if (std::get<0>(it->first) >= fromTick && it->second == robotId) {
            it = edgeReservations.erase(it);
        } else {
            ++it;
        }
    }
}

void ReservationTable::clearReservationsForRobot(const std::string& robotId) {
    releaseFutureReservations(robotId, 0);
    stationaryRobots.erase(robotId);
}

void ReservationTable::purgePastReservations(int beforeTick) {
    for (auto it = vertexReservations.begin(); it != vertexReservations.end(); ) {
        if (it->first.first < beforeTick) {
            it = vertexReservations.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = edgeReservations.begin(); it != edgeReservations.end(); ) {
        if (std::get<0>(it->first) < beforeTick) {
            it = edgeReservations.erase(it);
        } else {
            ++it;
        }
    }
}

void ReservationTable::clearReservations() {
    vertexReservations.clear();
    edgeReservations.clear();
    stationaryRobots.clear();
}

size_t ReservationTable::getVertexReservationCount() const {
    return vertexReservations.size();
}

size_t ReservationTable::getEdgeReservationCount() const {
    return edgeReservations.size();
}
