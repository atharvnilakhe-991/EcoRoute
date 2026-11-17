#ifndef ALGORITHM_BENCHMARK_H
#define ALGORITHM_BENCHMARK_H

#include <string>
#include <vector>
#include <chrono>
#include "Position.h"
#include "Warehouse.h"
#include "ReservationTable.h"
#include "PathFinder.h"

/**
 * @brief Telemetry metrics for a specific algorithm executed on a benchmark scenario.
 */
struct BenchmarkAlgorithmMetrics {
    std::string algorithmName;
    bool supported;
    bool found;
    int repetitions;
    double successRate;          // 0.0 to 100.0%
    double medianExecutionTimeUs;
    double minExecutionTimeUs;
    double maxExecutionTimeUs;
    int nodesVisited;
    int pathLength;
    double totalCost;
    int waitingTicks;
    bool isOptimal;
    std::string statusMessage;
    std::vector<Position> path;

    // Multi-robot coordination metrics
    int robotsCompleted;
    int robotsContending;
    int replanningEvents;
    int makespanTicks;

    std::string toJSON() const;
};

/**
 * @brief Definition and metadata of a controlled warehouse benchmark scenario.
 */
struct BenchmarkScenarioDefinition {
    std::string id;
    std::string name;
    std::string description;
    Position start;
    Position goal;
    int width;
    int height;
    int obstacleCount;
    bool isMultiRobot;
};

/**
 * @brief Benchmark results for a single scenario across tested algorithms.
 */
struct BenchmarkScenarioResult {
    BenchmarkScenarioDefinition scenario;
    std::vector<BenchmarkAlgorithmMetrics> algorithmResults;

    std::string toJSON() const;
};

/**
 * @brief Top-level suite benchmark results container.
 */
struct BenchmarkSuiteResult {
    std::string timestamp;
    int totalScenarios;
    int repetitions;
    std::vector<BenchmarkScenarioResult> scenarios;
    std::string note;

    std::string toJSON() const;
};

/**
 * @brief Authoritative Algorithm Benchmarking and Performance Analysis Engine.
 * Evaluates pathfinding algorithms (BFS, Dijkstra, A*, Space-Time A*) in completely
 * isolated scenario environments without mutating live warehouse simulation state.
 */
class AlgorithmBenchmark {
public:
    // Isolated Warehouse Builders
    static Warehouse createOpenGridWarehouse();
    static Warehouse createObstacleRichWarehouse();
    static Warehouse createNarrowCorridorWarehouse();
    static Warehouse createLongDistanceWarehouse();

    // Benchmark Execution
    static BenchmarkScenarioResult runScenario(const std::string& scenarioId,
                                              int repetitions = 1,
                                              const std::vector<std::string>& algorithms = {"BFS", "Dijkstra", "A*", "SpaceTimeA*"});

    static BenchmarkSuiteResult runCanonicalSuite(int repetitions = 1);
    static std::string runCanonicalSuiteJSON(int repetitions = 1);

    static std::string runCustomBenchmarkJSON(const std::string& scenarioId,
                                             int repetitions,
                                             const std::vector<std::string>& algorithms);
};

#endif // ALGORITHM_BENCHMARK_H
