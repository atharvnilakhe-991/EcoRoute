#include "../include/AlgorithmLab.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

static std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

std::string AlgorithmLabComparison::generateSummary() const {
    std::ostringstream ss;
    if (!bfsResult.found && !dijkstraResult.found && !astarResult.found) {
        ss << "Destination is unreachable from start (" << start.x << "," << start.y 
           << ") to (" << goal.x << "," << goal.y << ").";
        return ss.str();
    }

    ss << "Pathfinding Complete. ";
    if (astarResult.found && bfsResult.found) {
        int bfsExplored = bfsResult.nodesExplored;
        int astarExplored = astarResult.nodesExplored;
        double reduction = 0.0;
        if (bfsExplored > 0) {
            reduction = (1.0 - (static_cast<double>(astarExplored) / bfsExplored)) * 100.0;
        }
        if (reduction > 0.0) {
            ss << "A* pruned " << std::fixed << std::setprecision(1) << reduction 
               << "% of node explorations compared to BFS (" << astarExplored 
               << " vs " << bfsExplored << " nodes). ";
        } else {
            ss << "A* and BFS explored comparable search spaces (" << astarExplored 
               << " vs " << bfsExplored << " nodes). ";
        }
    }

    if (astarResult.found && dijkstraResult.found) {
        if (astarResult.totalCost == dijkstraResult.totalCost) {
            ss << "Dijkstra and A* found mathematically identical optimal paths (Cost: " 
               << astarResult.totalCost << "). ";
        } else {
            ss << "A* Cost: " << astarResult.totalCost << ", Dijkstra Cost: " 
               << dijkstraResult.totalCost << ". ";
        }
    }

    if (bfsResult.found) {
        ss << "BFS Path Length: " << bfsResult.path.size() << " steps. ";
    }
    return ss.str();
}

std::string AlgorithmLabComparison::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"start\":{\"x\":" << start.x << ",\"y\":" << start.y << "},"
       << "\"goal\":{\"x\":" << goal.x << ",\"y\":" << goal.y << "},"
       << "\"gridWidth\":" << gridWidth << ","
       << "\"gridHeight\":" << gridHeight << ","
       << "\"obstacleCount\":" << obstacleCount << ","
       << "\"summary\":\"" << escapeJsonString(generateSummary()) << "\","
       << "\"bfs\":" << bfsResult.toJSON() << ","
       << "\"dijkstra\":" << dijkstraResult.toJSON() << ","
       << "\"astar\":" << astarResult.toJSON() << ","
       << "\"results\":["
       << bfsResult.toJSON() << ","
       << dijkstraResult.toJSON() << ","
       << astarResult.toJSON()
       << "]}";
    return ss.str();
}

std::string AlgorithmLabComparison::toCSV() const {
    std::ostringstream ss;
    ss << "Algorithm,Found,Status,PathLength,StepCount,TotalCost,NodesExplored,NodesVisited,ExecutionTimeUs,Start,Goal\n";
    auto appendRow = [&](const PathResult& res) {
        ss << res.algorithmName << ","
           << (res.found ? "true" : "false") << ","
           << (res.statusMessage.empty() ? (res.found ? "SUCCESS" : "NO_PATH_FOUND") : res.statusMessage) << ","
           << res.path.size() << ","
           << (res.path.empty() ? 0 : static_cast<int>(res.path.size() - 1)) << ","
           << std::fixed << std::setprecision(2) << res.totalCost << ","
           << res.nodesExplored << ","
           << res.nodesVisited << ","
           << std::fixed << std::setprecision(2) << res.executionTimeMicroseconds << ","
           << "(" << start.x << ";" << start.y << "),"
           << "(" << goal.x << ";" << goal.y << ")\n";
    };
    appendRow(bfsResult);
    appendRow(dijkstraResult);
    appendRow(astarResult);
    return ss.str();
}

Warehouse AlgorithmLab::createLabWarehouse(
    const Warehouse& baseWarehouse,
    const std::vector<Position>& customObstacles,
    const std::vector<Position>& removedObstacles,
    const std::vector<std::pair<Position, double>>& customWeights) 
{
    Warehouse labWarehouse = baseWarehouse;

    // Remove specified obstacles (set to EMPTY floor)
    for (const auto& pos : removedObstacles) {
        if (labWarehouse.isValidPosition(pos)) {
            labWarehouse.setObstacle(pos, false);
        }
    }

    // Add custom obstacles
    for (const auto& pos : customObstacles) {
        if (labWarehouse.isValidPosition(pos)) {
            labWarehouse.setObstacle(pos, true);
        }
    }

    // Apply custom cell weights
    for (const auto& item : customWeights) {
        if (labWarehouse.isValidPosition(item.first)) {
            labWarehouse.setCellCost(item.first, item.second);
        }
    }

    return labWarehouse;
}

PathResult AlgorithmLab::runSingleAlgorithm(
    const Warehouse& warehouse,
    const std::string& algorithm,
    const Position& start,
    const Position& goal) 
{
    if (algorithm == "BFS") {
        return PathFinder::findPathBFS(warehouse, start, goal);
    } else if (algorithm == "Dijkstra") {
        return PathFinder::findPathDijkstra(warehouse, start, goal);
    } else {
        // Default to A*
        return PathFinder::findPathAStar(warehouse, start, goal);
    }
}

AlgorithmLabComparison AlgorithmLab::compareAlgorithms(
    const Warehouse& warehouse,
    const Position& start,
    const Position& goal) 
{
    AlgorithmLabComparison comp;
    comp.start = start;
    comp.goal = goal;
    comp.gridWidth = warehouse.getWidth();
    comp.gridHeight = warehouse.getHeight();

    int obstacles = 0;
    for (int y = 0; y < comp.gridHeight; ++y) {
        for (int x = 0; x < comp.gridWidth; ++x) {
            Position p(x, y);
            if (!warehouse.isWalkable(p)) {
                obstacles++;
            }
        }
    }
    comp.obstacleCount = obstacles;

    // Execute each algorithm independently on identical graph input
    comp.bfsResult = PathFinder::findPathBFS(warehouse, start, goal);
    comp.dijkstraResult = PathFinder::findPathDijkstra(warehouse, start, goal);
    comp.astarResult = PathFinder::findPathAStar(warehouse, start, goal);

    return comp;
}

std::string AlgorithmLab::runLabAlgorithmJSON(
    const Warehouse& baseWarehouse,
    const std::string& algorithm,
    const Position& start,
    const Position& goal,
    const std::vector<Position>& customObstacles,
    const std::vector<Position>& removedObstacles,
    const std::vector<std::pair<Position, double>>& customWeights) 
{
    Warehouse labWarehouse = createLabWarehouse(baseWarehouse, customObstacles, removedObstacles, customWeights);
    PathResult result = runSingleAlgorithm(labWarehouse, algorithm, start, goal);
    return result.toJSON();
}

std::string AlgorithmLab::compareLabAlgorithmsJSON(
    const Warehouse& baseWarehouse,
    const Position& start,
    const Position& goal,
    const std::vector<Position>& customObstacles,
    const std::vector<Position>& removedObstacles,
    const std::vector<std::pair<Position, double>>& customWeights) 
{
    Warehouse labWarehouse = createLabWarehouse(baseWarehouse, customObstacles, removedObstacles, customWeights);
    AlgorithmLabComparison comp = compareAlgorithms(labWarehouse, start, goal);
    return comp.toJSON();
}

std::string AlgorithmLab::generateLabReport(
    const Warehouse& baseWarehouse,
    const Position& start,
    const Position& goal,
    const std::string& format,
    const std::vector<Position>& customObstacles,
    const std::vector<Position>& removedObstacles,
    const std::vector<std::pair<Position, double>>& customWeights) 
{
    Warehouse labWarehouse = createLabWarehouse(baseWarehouse, customObstacles, removedObstacles, customWeights);
    AlgorithmLabComparison comp = compareAlgorithms(labWarehouse, start, goal);
    if (format == "csv") {
        return comp.toCSV();
    }
    return comp.toJSON();
}

