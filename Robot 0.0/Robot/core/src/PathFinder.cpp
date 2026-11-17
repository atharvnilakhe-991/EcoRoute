#include "../include/PathFinder.h"
#include <queue>
#include <set>
#include <unordered_set>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <limits>

std::string PathResult::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"found\":" << (found ? "true" : "false") << ","
       << "\"algorithm\":\"" << algorithmName << "\","
       << "\"status\":\"" << (statusMessage.empty() ? (found ? "SUCCESS" : "NO_PATH_FOUND") : statusMessage) << "\","
       << "\"start\":{\"x\":" << start.x << ",\"y\":" << start.y << "},"
       << "\"goal\":{\"x\":" << goal.x << ",\"y\":" << goal.y << "},"
       << "\"totalCost\":" << totalCost << ","
       << "\"nodesVisited\":" << nodesVisited << ","
       << "\"nodesExplored\":" << nodesExplored << ","
       << "\"waitingTicks\":" << waitingTicks << ","
       << "\"executionTimeMicroseconds\":" << executionTimeMicroseconds << ","
       << "\"pathLength\":" << path.size() << ","
       << "\"stepCount\":" << (path.empty() ? 0 : static_cast<int>(path.size() - 1)) << ","
       << "\"path\":[";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << path[i].x << ",\"y\":" << path[i].y << "}";
    }
    ss << "],\"explorationOrder\":[";
    for (size_t i = 0; i < explorationOrder.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << explorationOrder[i].x << ",\"y\":" << explorationOrder[i].y << "}";
    }
    ss << "]}";
    return ss.str();
}

std::vector<Position> PathFinder::reconstructPath(const std::map<Position, Position>& parentMap, Position current) {
    std::vector<Position> totalPath;
    totalPath.push_back(current);
    auto it = parentMap.find(current);
    while (it != parentMap.end()) {
        current = it->second;
        totalPath.push_back(current);
        it = parentMap.find(current);
    }
    std::reverse(totalPath.begin(), totalPath.end());
    return totalPath;
}

static double calculatePathCost(const Warehouse& warehouse, const std::vector<Position>& path) {
    if (path.size() <= 1) return 0.0;
    double cost = 0.0;
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i] == path[i - 1]) {
            cost += 1.0; // Waiting step cost
        } else {
            cost += warehouse.getCellCost(path[i]);
        }
    }
    return cost;
}

PathResult PathFinder::findPathBFS(const Warehouse& warehouse, const Position& start, const Position& goal) {
    auto startTime = std::chrono::high_resolution_clock::now();
    PathResult result;
    result.algorithmName = "BFS";
    result.found = false;
    result.start = start;
    result.goal = goal;
    result.nodesVisited = 0;
    result.nodesExplored = 0;
    result.waitingTicks = 0;
    result.totalCost = 0.0;

    if (!warehouse.isValidPosition(start) || !warehouse.isValidPosition(goal)) {
        result.statusMessage = "OUT_OF_BOUNDS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(start)) {
        result.statusMessage = "START_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(goal)) {
        result.statusMessage = "GOAL_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (start == goal) {
        result.found = true;
        result.path = {start};
        result.explorationOrder = {start};
        result.nodesVisited = 1;
        result.nodesExplored = 1;
        result.totalCost = 0.0;
        result.statusMessage = "SUCCESS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    std::queue<Position> frontier;
    std::unordered_set<Position> visited;
    std::map<Position, Position> parent;

    frontier.push(start);
    visited.insert(start);

    while (!frontier.empty()) {
        Position current = frontier.front();
        frontier.pop();
        result.nodesVisited++;
        result.nodesExplored++;
        result.explorationOrder.push_back(current);

        if (current == goal) {
            result.found = true;
            result.path = reconstructPath(parent, current);
            result.totalCost = calculatePathCost(warehouse, result.path);
            result.statusMessage = "SUCCESS";
            break;
        }

        for (const auto& next : warehouse.getWalkableNeighbors(current)) {
            if (visited.find(next) == visited.end()) {
                visited.insert(next);
                parent[next] = current;
                frontier.push(next);
            }
        }
    }

    if (!result.found) {
        result.statusMessage = "NO_PATH_FOUND";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

PathResult PathFinder::findPathDijkstra(const Warehouse& warehouse, const Position& start, const Position& goal) {
    auto startTime = std::chrono::high_resolution_clock::now();
    PathResult result;
    result.algorithmName = "Dijkstra";
    result.found = false;
    result.start = start;
    result.goal = goal;
    result.nodesVisited = 0;
    result.nodesExplored = 0;
    result.waitingTicks = 0;
    result.totalCost = 0.0;

    if (!warehouse.isValidPosition(start) || !warehouse.isValidPosition(goal)) {
        result.statusMessage = "OUT_OF_BOUNDS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(start)) {
        result.statusMessage = "START_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(goal)) {
        result.statusMessage = "GOAL_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (start == goal) {
        result.found = true;
        result.path = {start};
        result.explorationOrder = {start};
        result.nodesVisited = 1;
        result.nodesExplored = 1;
        result.totalCost = 0.0;
        result.statusMessage = "SUCCESS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    using Element = std::pair<double, Position>;
    std::priority_queue<Element, std::vector<Element>, std::greater<Element>> frontier;

    std::map<Position, double> costSoFar;
    std::map<Position, Position> parent;
    std::unordered_set<Position> closedSet;

    frontier.push({0.0, start});
    costSoFar[start] = 0.0;

    while (!frontier.empty()) {
        auto [currentCost, current] = frontier.top();
        frontier.pop();

        if (closedSet.find(current) != closedSet.end()) {
            continue;
        }
        closedSet.insert(current);
        result.nodesVisited++;
        result.nodesExplored++;
        result.explorationOrder.push_back(current);

        if (current == goal) {
            result.found = true;
            result.path = reconstructPath(parent, current);
            result.totalCost = costSoFar[goal];
            result.statusMessage = "SUCCESS";
            break;
        }

        for (const auto& next : warehouse.getWalkableNeighbors(current)) {
            double newCost = costSoFar[current] + warehouse.getCellCost(next);
            if (costSoFar.find(next) == costSoFar.end() || newCost < costSoFar[next]) {
                costSoFar[next] = newCost;
                parent[next] = current;
                frontier.push({newCost, next});
            }
        }
    }

    if (!result.found) {
        result.statusMessage = "NO_PATH_FOUND";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

PathResult PathFinder::findPathAStar(const Warehouse& warehouse, const Position& start, const Position& goal) {
    auto startTime = std::chrono::high_resolution_clock::now();
    PathResult result;
    result.algorithmName = "A*";
    result.found = false;
    result.start = start;
    result.goal = goal;
    result.nodesVisited = 0;
    result.nodesExplored = 0;
    result.waitingTicks = 0;
    result.totalCost = 0.0;

    if (!warehouse.isValidPosition(start) || !warehouse.isValidPosition(goal)) {
        result.statusMessage = "OUT_OF_BOUNDS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(start)) {
        result.statusMessage = "START_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (!warehouse.isWalkable(goal)) {
        result.statusMessage = "GOAL_BLOCKED";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (start == goal) {
        result.found = true;
        result.path = {start};
        result.explorationOrder = {start};
        result.nodesVisited = 1;
        result.nodesExplored = 1;
        result.totalCost = 0.0;
        result.statusMessage = "SUCCESS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    using Element = std::pair<double, Position>;
    std::priority_queue<Element, std::vector<Element>, std::greater<Element>> frontier;

    std::map<Position, double> gScore;
    std::map<Position, Position> parent;
    std::unordered_set<Position> closedSet;

    frontier.push({0.0, start});
    gScore[start] = 0.0;

    const double minMovementCost = warehouse.getMinCellCost();

    while (!frontier.empty()) {
        auto [currentPriority, current] = frontier.top();
        frontier.pop();

        if (closedSet.find(current) != closedSet.end()) {
            continue;
        }
        closedSet.insert(current);
        result.nodesVisited++;
        result.nodesExplored++;
        result.explorationOrder.push_back(current);

        if (current == goal) {
            result.found = true;
            result.path = reconstructPath(parent, current);
            result.totalCost = gScore[goal];
            result.statusMessage = "SUCCESS";
            break;
        }

        for (const auto& next : warehouse.getWalkableNeighbors(current)) {
            double tentativeGScore = gScore[current] + warehouse.getCellCost(next);
            if (gScore.find(next) == gScore.end() || tentativeGScore < gScore[next]) {
                parent[next] = current;
                gScore[next] = tentativeGScore;
                double hScore = static_cast<double>(next.manhattanDistance(goal)) * minMovementCost;
                double fScore = tentativeGScore + hScore;
                frontier.push({fScore, next});
            }
        }
    }

    if (!result.found) {
        result.statusMessage = "NO_PATH_FOUND";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

PathResult PathFinder::findPathToTarget(const Warehouse& warehouse, const Position& start, const Position& target, const std::string& algorithm) {
    if (warehouse.isWalkable(target)) {
        if (algorithm == "BFS") return findPathBFS(warehouse, start, target);
        if (algorithm == "Dijkstra") return findPathDijkstra(warehouse, start, target);
        return findPathAStar(warehouse, start, target);
    }

    std::vector<Position> neighbors;
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    for (int i = 0; i < 4; ++i) {
        Position adj(target.x + dx[i], target.y + dy[i]);
        if (warehouse.isWalkable(adj)) {
            neighbors.push_back(adj);
        }
    }

    if (neighbors.empty()) {
        PathResult empty;
        empty.found = false;
        empty.algorithmName = algorithm;
        empty.nodesVisited = 0;
        empty.waitingTicks = 0;
        empty.totalCost = 0.0;
        empty.executionTimeMicroseconds = 0.0;
        return empty;
    }

    PathResult bestResult;
    bestResult.found = false;
    double minCost = std::numeric_limits<double>::infinity();

    for (const auto& neighbor : neighbors) {
        PathResult res;
        if (algorithm == "BFS") res = findPathBFS(warehouse, start, neighbor);
        else if (algorithm == "Dijkstra") res = findPathDijkstra(warehouse, start, neighbor);
        else res = findPathAStar(warehouse, start, neighbor);

        if (res.found && res.totalCost < minCost) {
            minCost = res.totalCost;
            bestResult = res;
        }
    }

    return bestResult;
}

// ============================================================================
// SPACE-TIME MULTI-ROBOT COOPERATIVE PATH PLANNING
// ============================================================================

struct SpaceTimeKey {
    Position pos;
    int time;

    bool operator<(const SpaceTimeKey& other) const {
        if (time != other.time) return time < other.time;
        return pos < other.pos;
    }
};

PathResult PathFinder::findSpaceTimePathAStar(const Warehouse& warehouse, const ReservationTable& table,
                                             const Position& start, const Position& goal,
                                             int startTick, const std::string& robotId, int maxHorizon) {
    auto startTime = std::chrono::high_resolution_clock::now();
    PathResult result;
    result.algorithmName = "SpaceTime-A*";
    result.found = false;
    result.start = start;
    result.goal = goal;
    result.nodesVisited = 0;
    result.nodesExplored = 0;
    result.waitingTicks = 0;
    result.totalCost = 0.0;

    if (!warehouse.isValidPosition(start) || !warehouse.isValidPosition(goal) ||
        !warehouse.isWalkable(start) || !warehouse.isWalkable(goal)) {
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    if (start == goal) {
        result.found = true;
        result.path = {start};
        result.totalCost = 0.0;
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    // Min-heap priority queue: pair of (fScore, SpaceTimeKey)
    using STElement = std::pair<double, SpaceTimeKey>;
    std::priority_queue<STElement, std::vector<STElement>, std::greater<STElement>> frontier;

    std::map<SpaceTimeKey, double> gScore;
    std::map<SpaceTimeKey, SpaceTimeKey> parent;
    std::set<SpaceTimeKey> closedSet;

    SpaceTimeKey root{start, startTick};
    gScore[root] = 0.0;
    frontier.push({0.0, root});

    const double minMovementCost = warehouse.getMinCellCost();
    SpaceTimeKey goalKey{Position(-1, -1), -1};

    while (!frontier.empty()) {
        auto [currentPriority, current] = frontier.top();
        frontier.pop();

        if (closedSet.find(current) != closedSet.end()) {
            continue;
        }
        closedSet.insert(current);
        result.nodesVisited++;

        if (current.pos == goal) {
            goalKey = current;
            result.found = true;
            break;
        }

        if (current.time - startTick >= maxHorizon) {
            continue; // Bounded planning horizon
        }

        int nextTime = current.time + 1;

        // Transition Option 1: Move to an adjacent walkable neighbor
        for (const auto& nextPos : warehouse.getWalkableNeighbors(current.pos)) {
            // Check vertex reservation
            if (table.isVertexReservedByOther(nextPos, nextTime, robotId)) {
                continue;
            }
            // Check edge reservation (head-on collision prevention)
            if (table.isEdgeReservedByOther(current.pos, nextPos, current.time, robotId)) {
                continue;
            }

            double stepCost = warehouse.getCellCost(nextPos);
            double tentativeG = gScore[current] + stepCost;
            SpaceTimeKey nextKey{nextPos, nextTime};

            if (gScore.find(nextKey) == gScore.end() || tentativeG < gScore[nextKey]) {
                gScore[nextKey] = tentativeG;
                parent[nextKey] = current;
                double h = static_cast<double>(nextPos.manhattanDistance(goal)) * minMovementCost;
                frontier.push({tentativeG + h, nextKey});
            }
        }

        // Transition Option 2: Wait in place at current.pos for 1 tick
        if (!table.isVertexReservedByOther(current.pos, nextTime, robotId)) {
            double waitCost = 1.0;
            double tentativeG = gScore[current] + waitCost;
            SpaceTimeKey waitKey{current.pos, nextTime};

            if (gScore.find(waitKey) == gScore.end() || tentativeG < gScore[waitKey]) {
                gScore[waitKey] = tentativeG;
                parent[waitKey] = current;
                double h = static_cast<double>(current.pos.manhattanDistance(goal)) * minMovementCost;
                frontier.push({tentativeG + h, waitKey});
            }
        }
    }

    if (result.found && goalKey.time != -1) {
        // Reconstruct space-time path
        std::vector<Position> stPath;
        SpaceTimeKey curr = goalKey;
        stPath.push_back(curr.pos);

        while (!(curr.pos == start && curr.time == startTick)) {
            curr = parent[curr];
            stPath.push_back(curr.pos);
        }
        std::reverse(stPath.begin(), stPath.end());
        result.path = stPath;
        result.totalCost = gScore[goalKey];

        // Count waiting steps
        result.waitingTicks = 0;
        for (size_t i = 1; i < result.path.size(); ++i) {
            if (result.path[i] == result.path[i - 1]) {
                result.waitingTicks++;
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

PathResult PathFinder::findSpaceTimePathToTarget(const Warehouse& warehouse, const ReservationTable& table,
                                                const Position& start, const Position& target,
                                                int startTick, const std::string& robotId, int maxHorizon) {
    if (warehouse.isWalkable(target)) {
        return findSpaceTimePathAStar(warehouse, table, start, target, startTick, robotId, maxHorizon);
    }

    std::vector<Position> neighbors;
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    for (int i = 0; i < 4; ++i) {
        Position adj(target.x + dx[i], target.y + dy[i]);
        if (warehouse.isWalkable(adj)) {
            neighbors.push_back(adj);
        }
    }

    if (neighbors.empty()) {
        PathResult empty;
        empty.found = false;
        empty.algorithmName = "SpaceTime-A*";
        empty.nodesVisited = 0;
        empty.waitingTicks = 0;
        empty.totalCost = 0.0;
        empty.executionTimeMicroseconds = 0.0;
        return empty;
    }

    PathResult bestResult;
    bestResult.found = false;
    double minCost = std::numeric_limits<double>::infinity();

    for (const auto& neighbor : neighbors) {
        PathResult res = findSpaceTimePathAStar(warehouse, table, start, neighbor, startTick, robotId, maxHorizon);
        if (res.found && res.totalCost < minCost) {
            minCost = res.totalCost;
            bestResult = res;
        }
    }

    return bestResult;
}
