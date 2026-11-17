#ifndef ALGORITHM_LAB_H
#define ALGORITHM_LAB_H

#include <string>
#include <vector>
#include <utility>
#include "Position.h"
#include "Warehouse.h"
#include "PathFinder.h"

/**
 * @brief Results structure holding comparative metrics and paths for all three algorithms.
 */
struct AlgorithmLabComparison {
    Position start{0, 0};
    Position goal{0, 0};
    int gridWidth{15};
    int gridHeight{15};
    int obstacleCount{0};

    PathResult bfsResult;
    PathResult dijkstraResult;
    PathResult astarResult;

    std::string toJSON() const;
    std::string toCSV() const;
    std::string generateSummary() const;
};

/**
 * @brief DSA Algorithm Laboratory Engine
 * Provides an isolated environment for executing, benchmarking, visualizing,
 * and comparing pathfinding algorithms (BFS, Dijkstra, A*) without mutating
 * the active live warehouse simulation state.
 */
class AlgorithmLab {
public:
    // Create an isolated warehouse instance cloned from base warehouse, applying custom obstacles/weights
    static Warehouse createLabWarehouse(
        const Warehouse& baseWarehouse,
        const std::vector<Position>& customObstacles = {},
        const std::vector<Position>& removedObstacles = {},
        const std::vector<std::pair<Position, double>>& customWeights = {}
    );

    // Run a single algorithm on a warehouse instance
    static PathResult runSingleAlgorithm(
        const Warehouse& warehouse,
        const std::string& algorithm,
        const Position& start,
        const Position& goal
    );

    // Compare BFS, Dijkstra, and A* independently on a warehouse instance
    static AlgorithmLabComparison compareAlgorithms(
        const Warehouse& warehouse,
        const Position& start,
        const Position& goal
    );

    // Complete pipeline helper: builds isolated lab warehouse and runs single algorithm
    static std::string runLabAlgorithmJSON(
        const Warehouse& baseWarehouse,
        const std::string& algorithm,
        const Position& start,
        const Position& goal,
        const std::vector<Position>& customObstacles = {},
        const std::vector<Position>& removedObstacles = {},
        const std::vector<std::pair<Position, double>>& customWeights = {}
    );

    // Complete pipeline helper: builds isolated lab warehouse and runs all three algorithms
    static std::string compareLabAlgorithmsJSON(
        const Warehouse& baseWarehouse,
        const Position& start,
        const Position& goal,
        const std::vector<Position>& customObstacles = {},
        const std::vector<Position>& removedObstacles = {},
        const std::vector<std::pair<Position, double>>& customWeights = {}
    );

    // Report generation (JSON or CSV format)
    static std::string generateLabReport(
        const Warehouse& baseWarehouse,
        const Position& start,
        const Position& goal,
        const std::string& format = "json",
        const std::vector<Position>& customObstacles = {},
        const std::vector<Position>& removedObstacles = {},
        const std::vector<std::pair<Position, double>>& customWeights = {}
    );
};

#endif // ALGORITHM_LAB_H

