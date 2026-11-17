#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include <vector>
#include <string>
#include <map>
#include "Position.h"
#include "Warehouse.h"
#include "ReservationTable.h"

struct PathResult {
    bool found = false;
    std::vector<Position> path;
    double totalCost = 0.0;
    int nodesVisited = 0;
    int nodesExplored = 0;
    int waitingTicks = 0;
    double executionTimeMicroseconds = 0.0;
    std::string algorithmName;
    std::string statusMessage;
    Position start{0, 0};
    Position goal{0, 0};
    std::vector<Position> explorationOrder;

    std::string toJSON() const;
};

class PathFinder {
public:
    static PathResult findPathBFS(const Warehouse& warehouse, const Position& start, const Position& goal);
    static PathResult findPathDijkstra(const Warehouse& warehouse, const Position& start, const Position& goal);
    static PathResult findPathAStar(const Warehouse& warehouse, const Position& start, const Position& goal);

    // Static Routing to Target
    static PathResult findPathToTarget(const Warehouse& warehouse, const Position& start, const Position& target, const std::string& algorithm = "A*");

    // Space-Time Multi-Robot Cooperative Path Planning
    static PathResult findSpaceTimePathAStar(const Warehouse& warehouse, const ReservationTable& table,
                                            const Position& start, const Position& goal,
                                            int startTick, const std::string& robotId, int maxHorizon = 60);

    static PathResult findSpaceTimePathToTarget(const Warehouse& warehouse, const ReservationTable& table,
                                              const Position& start, const Position& target,
                                              int startTick, const std::string& robotId, int maxHorizon = 60);

private:
    static std::vector<Position> reconstructPath(const std::map<Position, Position>& parentMap, Position current);
};

#endif // PATH_FINDER_H
