#ifndef RESERVATION_TABLE_H
#define RESERVATION_TABLE_H

#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "Position.h"

/**
 * @brief Time-indexed Space-Time Reservation Table for multi-robot collision prevention.
 * Prevents vertex collisions (two robots in same cell at same tick) and edge collisions
 * (two robots traversing the same edge in opposite directions at the same tick).
 */
class ReservationTable {
private:
    // Key: (tick, Position), Value: robotId
    std::map<std::pair<int, Position>, std::string> vertexReservations;

    // Key: (tick, from, to), Value: robotId
    std::map<std::tuple<int, Position, Position>, std::string> edgeReservations;

    // Key: robotId, Value: stationary Position (blocked for all future time steps)
    std::map<std::string, Position> stationaryRobots;

public:
    ReservationTable() = default;

    // Stationary Robot Tracking
    void setStationaryRobot(const std::string& robotId, const Position& pos);
    void clearStationaryRobot(const std::string& robotId);
    bool isStationary(const std::string& robotId) const;
    Position getStationaryPosition(const std::string& robotId) const;
    const std::map<std::string, Position>& getStationaryRobots() const;

    // Vertex Reservation
    bool reserveVertex(const Position& pos, int tick, const std::string& robotId);
    bool isVertexReserved(const Position& pos, int tick, const std::string& robotId = "") const;
    bool isVertexReservedByOther(const Position& pos, int tick, const std::string& robotId) const;
    std::string getVertexReserver(const Position& pos, int tick) const;

    // Edge Reservation (Head-on Collision Prevention)
    bool reserveEdge(const Position& from, const Position& to, int tick, const std::string& robotId);
    bool isEdgeReserved(const Position& from, const Position& to, int tick, const std::string& robotId = "") const;
    bool isEdgeReservedByOther(const Position& from, const Position& to, int tick, const std::string& robotId) const;

    // Lifecycle & Pruning
    void releaseFutureReservations(const std::string& robotId, int fromTick);
    void clearReservationsForRobot(const std::string& robotId);
    void purgePastReservations(int beforeTick);
    void clearReservations();

    size_t getVertexReservationCount() const;
    size_t getEdgeReservationCount() const;
};

#endif // RESERVATION_TABLE_H
