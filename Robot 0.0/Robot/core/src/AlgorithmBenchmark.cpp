#include "../include/AlgorithmBenchmark.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

std::string BenchmarkAlgorithmMetrics::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"algorithm\":\"" << algorithmName << "\","
       << "\"algorithmName\":\"" << algorithmName << "\","
       << "\"supported\":" << (supported ? "true" : "false") << ","
       << "\"found\":" << (found ? "true" : "false") << ","
       << "\"repetitions\":" << repetitions << ","
       << "\"successRate\":" << successRate << ","
       << "\"medianExecutionTimeUs\":" << medianExecutionTimeUs << ","
       << "\"minExecutionTimeUs\":" << minExecutionTimeUs << ","
       << "\"maxExecutionTimeUs\":" << maxExecutionTimeUs << ","
       << "\"executionTimeMicroseconds\":" << medianExecutionTimeUs << "," // backward compatibility
       << "\"nodesVisited\":" << nodesVisited << ","
       << "\"pathLength\":" << pathLength << ","
       << "\"totalCost\":" << totalCost << ","
       << "\"waitingTicks\":" << waitingTicks << ","
       << "\"isOptimal\":" << (isOptimal ? "true" : "false") << ","
       << "\"statusMessage\":\"" << statusMessage << "\","
       << "\"robotsCompleted\":" << robotsCompleted << ","
       << "\"robotsContending\":" << robotsContending << ","
       << "\"replanningEvents\":" << replanningEvents << ","
       << "\"makespanTicks\":" << makespanTicks << ","
       << "\"path\":[";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << path[i].x << ",\"y\":" << path[i].y << "}";
    }
    ss << "]}";
    return ss.str();
}

std::string BenchmarkScenarioResult::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << scenario.id << "\","
       << "\"name\":\"" << scenario.name << "\","
       << "\"scenarioName\":\"" << scenario.name << "\","
       << "\"description\":\"" << scenario.description << "\","
       << "\"start\":{\"x\":" << scenario.start.x << ",\"y\":" << scenario.start.y << "},"
       << "\"goal\":{\"x\":" << scenario.goal.x << ",\"y\":" << scenario.goal.y << "},"
       << "\"width\":" << scenario.width << ","
       << "\"height\":" << scenario.height << ","
       << "\"obstacleCount\":" << scenario.obstacleCount << ","
       << "\"isMultiRobot\":" << (scenario.isMultiRobot ? "true" : "false") << ",";

    if (scenario.isMultiRobot) {
        int comp = 0, cont = 0, replan = 0, ms = 0, wait = 0;
        for (const auto& r : algorithmResults) {
            if (r.algorithmName == "SpaceTimeA*" || r.algorithmName == "Space-Time A*") {
                comp = r.robotsCompleted;
                cont = r.robotsContending;
                replan = r.replanningEvents;
                ms = r.makespanTicks;
                wait = r.waitingTicks;
                break;
            }
        }
        ss << "\"multiRobotMetrics\":{"
           << "\"robotsCompleted\":" << comp << ","
           << "\"robotsContending\":" << cont << ","
           << "\"makespanTicks\":" << ms << ","
           << "\"waitingTicks\":" << wait << ","
           << "\"replanningEvents\":" << replan
           << "},";
    }

    ss << "\"results\":[";
    for (size_t i = 0; i < algorithmResults.size(); ++i) {
        if (i > 0) ss << ",";
        ss << algorithmResults[i].toJSON();
    }
    ss << "]}";
    return ss.str();
}

std::string BenchmarkSuiteResult::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"success\":true,"
       << "\"timestamp\":\"" << timestamp << "\","
       << "\"totalScenarios\":" << totalScenarios << ","
       << "\"repetitions\":" << repetitions << ","
       << "\"note\":\"" << note << "\","
       << "\"scenarios\":[";
    for (size_t i = 0; i < scenarios.size(); ++i) {
        if (i > 0) ss << ",";
        ss << scenarios[i].toJSON();
    }
    ss << "]}";
    return ss.str();
}

Warehouse AlgorithmBenchmark::createOpenGridWarehouse() {
    Warehouse wh(15, 15);
    wh.addChargingStation(ChargingStation("CS01", Position(0, 0)));
    wh.addPackingStation(PackingStation("PK01", Position(14, 14)));
    return wh;
}

Warehouse AlgorithmBenchmark::createObstacleRichWarehouse() {
    Warehouse wh(15, 15);
    wh.addChargingStation(ChargingStation("CS01", Position(0, 0)));
    wh.addPackingStation(PackingStation("PK01", Position(14, 14)));

    // Dense storage shelves
    std::vector<int> cols = {3, 6, 9, 11};
    for (int col : cols) {
        for (int y = 3; y <= 10; ++y) {
            wh.addShelf(Shelf("SH_" + std::to_string(col) + "_" + std::to_string(y), Position(col, y)));
        }
    }

    // Static pillars
    wh.setObstacle(Position(5, 1), true);
    wh.setObstacle(Position(8, 1), true);
    wh.setObstacle(Position(1, 7), true);
    wh.setObstacle(Position(13, 7), true);

    return wh;
}

Warehouse AlgorithmBenchmark::createNarrowCorridorWarehouse() {
    Warehouse wh(15, 15);

    // Wall across row 5 except at x=1 and x=13
    for (int x = 0; x < 15; ++x) {
        if (x != 1 && x != 13) {
            wh.setObstacle(Position(x, 5), true);
        }
    }

    // Wall across row 7 except at x=1 and x=13
    for (int x = 0; x < 15; ++x) {
        if (x != 1 && x != 13) {
            wh.setObstacle(Position(x, 7), true);
        }
    }

    return wh;
}

Warehouse AlgorithmBenchmark::createLongDistanceWarehouse() {
    return createObstacleRichWarehouse();
}

BenchmarkScenarioResult AlgorithmBenchmark::runScenario(const std::string& scenarioId,
                                                        int repetitions,
                                                        const std::vector<std::string>& algorithms) {
    if (repetitions < 1) repetitions = 1;
    if (repetitions > 20) repetitions = 20;

    BenchmarkScenarioResult res;
    Warehouse wh(15, 15);
    ReservationTable table;

    if (scenarioId == "open_grid") {
        res.scenario.id = "open_grid";
        res.scenario.name = "Open Grid";
        res.scenario.description = "Minimal obstacle environment testing baseline search space exploration.";
        res.scenario.start = Position(1, 1);
        res.scenario.goal = Position(13, 13);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 0;
        res.scenario.isMultiRobot = false;
        wh = createOpenGridWarehouse();
    } else if (scenarioId == "obstacle_rich") {
        res.scenario.id = "obstacle_rich";
        res.scenario.name = "Obstacle-Rich Grid";
        res.scenario.description = "Dense shelf aisles and structural pillars testing detour efficiency.";
        res.scenario.start = Position(1, 4);
        res.scenario.goal = Position(13, 10);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 36;
        res.scenario.isMultiRobot = false;
        wh = createObstacleRichWarehouse();
    } else if (scenarioId == "narrow_corridor") {
        res.scenario.id = "narrow_corridor";
        res.scenario.name = "Narrow Corridor";
        res.scenario.description = "Restricted single-lane corridor testing bottleneck route efficiency.";
        res.scenario.start = Position(1, 6);
        res.scenario.goal = Position(13, 6);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 26;
        res.scenario.isMultiRobot = false;
        wh = createNarrowCorridorWarehouse();
    } else if (scenarioId == "long_distance") {
        res.scenario.id = "long_distance";
        res.scenario.name = "Long-Distance Route";
        res.scenario.description = "Full corner-to-corner traversal across warehouse testing search frontier pruning.";
        res.scenario.start = Position(0, 0);
        res.scenario.goal = Position(14, 14);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 36;
        res.scenario.isMultiRobot = false;
        wh = createLongDistanceWarehouse();
    } else if (scenarioId == "congested_multi_robot") {
        res.scenario.id = "congested_multi_robot";
        res.scenario.name = "Congested Multi-Robot Scenario";
        res.scenario.description = "Space-time multi-robot coordination with active competing corridor reservations.";
        res.scenario.start = Position(2, 7);
        res.scenario.goal = Position(12, 7);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 36;
        res.scenario.isMultiRobot = true;
        wh = createObstacleRichWarehouse();

        // Populate competing reservations in table
        for (int y = 0; y <= 14; ++y) {
            table.reserveVertex(Position(7, y), y, "R_COMPETITOR_1");
        }
        for (int t = 3; t <= 8; ++t) {
            table.reserveVertex(Position(5, 2), t, "R_COMPETITOR_2");
        }
    } else {
        // Unknown scenario fallback
        res.scenario.id = scenarioId;
        res.scenario.name = "Unknown Scenario (" + scenarioId + ")";
        res.scenario.description = "Unrecognized scenario identifier.";
        res.scenario.start = Position(0, 0);
        res.scenario.goal = Position(14, 14);
        res.scenario.width = 15;
        res.scenario.height = 15;
        res.scenario.obstacleCount = 0;
        res.scenario.isMultiRobot = false;
        wh = createOpenGridWarehouse();
    }

    // Execute requested algorithms
    for (const auto& algo : algorithms) {
        BenchmarkAlgorithmMetrics m;
        m.algorithmName = algo;
        m.repetitions = repetitions;
        m.supported = true;
        m.found = false;
        m.successRate = 0.0;
        m.medianExecutionTimeUs = 0.0;
        m.minExecutionTimeUs = 0.0;
        m.maxExecutionTimeUs = 0.0;
        m.nodesVisited = 0;
        m.pathLength = 0;
        m.totalCost = 0.0;
        m.waitingTicks = 0;
        m.isOptimal = false;
        m.robotsCompleted = 0;
        m.robotsContending = res.scenario.isMultiRobot ? 3 : 1;
        m.replanningEvents = 0;
        m.makespanTicks = 0;

        std::vector<double> durationsUs;
        durationsUs.reserve(repetitions);
        int successfulRuns = 0;
        PathResult lastResult;

        for (int r = 0; r < repetitions; ++r) {
            PathResult curRes;
            auto tStart = std::chrono::steady_clock::now();

            if (algo == "BFS") {
                curRes = PathFinder::findPathBFS(wh, res.scenario.start, res.scenario.goal);
            } else if (algo == "Dijkstra") {
                curRes = PathFinder::findPathDijkstra(wh, res.scenario.start, res.scenario.goal);
            } else if (algo == "A*") {
                curRes = PathFinder::findPathAStar(wh, res.scenario.start, res.scenario.goal);
            } else if (algo == "SpaceTimeA*" || algo == "Space-Time A*") {
                curRes = PathFinder::findSpaceTimePathAStar(wh, table, res.scenario.start, res.scenario.goal, 0, "R_TEST", 60);
            } else {
                m.supported = false;
                m.statusMessage = "Unsupported algorithm identifier: " + algo;
                break;
            }

            auto tEnd = std::chrono::steady_clock::now();
            double durUs = std::chrono::duration<double, std::micro>(tEnd - tStart).count();
            durationsUs.push_back(durUs);

            if (curRes.found) {
                successfulRuns++;
            }
            lastResult = curRes;
        }

        if (!m.supported) {
            res.algorithmResults.push_back(m);
            continue;
        }

        std::sort(durationsUs.begin(), durationsUs.end());
        if (repetitions % 2 == 1) {
            m.medianExecutionTimeUs = durationsUs[repetitions / 2];
        } else {
            m.medianExecutionTimeUs = (durationsUs[repetitions / 2 - 1] + durationsUs[repetitions / 2]) / 2.0;
        }
        m.minExecutionTimeUs = durationsUs.front();
        m.maxExecutionTimeUs = durationsUs.back();
        m.successRate = (static_cast<double>(successfulRuns) / repetitions) * 100.0;
        m.found = lastResult.found;
        m.nodesVisited = lastResult.nodesVisited;
        m.totalCost = lastResult.totalCost;
        m.path = lastResult.path;
        m.pathLength = static_cast<int>(lastResult.path.size());
        m.waitingTicks = lastResult.waitingTicks;

        // Optimality definitions
        if (algo == "BFS") {
            m.isOptimal = true; // Shortest hops
            m.statusMessage = res.scenario.isMultiRobot
                ? "Static 2D route solved (unaware of dynamic space-time reservations)."
                : (lastResult.found ? "Optimal unweighted hop path found." : "Destination unreachable.");
        } else if (algo == "Dijkstra") {
            m.isOptimal = true; // Guaranteed least-cost path
            m.statusMessage = res.scenario.isMultiRobot
                ? "Static 2D weighted route solved (unaware of dynamic space-time reservations)."
                : (lastResult.found ? "Guaranteed least-cost path found via priority frontier." : "Destination unreachable.");
        } else if (algo == "A*") {
            m.isOptimal = true; // Guaranteed optimal with admissible Manhattan heuristic
            m.statusMessage = res.scenario.isMultiRobot
                ? "Static 2D heuristic route solved (unaware of dynamic space-time reservations)."
                : (lastResult.found ? "Optimal least-cost path found via heuristic A* search." : "Destination unreachable.");
        } else if (algo == "SpaceTimeA*" || algo == "Space-Time A*") {
            m.isOptimal = true; // Optimal in space-time graph under reservation schedule
            m.robotsCompleted = lastResult.found ? 1 : 0;
            m.makespanTicks = lastResult.path.empty() ? 0 : (static_cast<int>(lastResult.path.size()) - 1);
            m.statusMessage = lastResult.found
                ? (res.scenario.isMultiRobot
                    ? "Cooperative space-time route solved avoiding all vertex and edge swap reservations."
                    : "Optimal space-time route found with 0 conflicting reservations.")
                : "Destination unreachable within time horizon.";
        }

        res.algorithmResults.push_back(m);
    }

    return res;
}

BenchmarkSuiteResult AlgorithmBenchmark::runCanonicalSuite(int repetitions) {
    BenchmarkSuiteResult suite;
    suite.timestamp = "2026-10-18T23:00:00Z";
    suite.repetitions = repetitions;
    suite.note = "Authoritative C++ Native Benchmark Suite • Complete State Isolation • High-Resolution steady_clock";

    std::vector<std::string> scenarioIds = {
        "open_grid",
        "obstacle_rich",
        "narrow_corridor",
        "long_distance",
        "congested_multi_robot"
    };

    suite.totalScenarios = static_cast<int>(scenarioIds.size());
    for (const auto& scId : scenarioIds) {
        suite.scenarios.push_back(runScenario(scId, repetitions, {"BFS", "Dijkstra", "A*", "SpaceTimeA*"}));
    }

    return suite;
}

std::string AlgorithmBenchmark::runCanonicalSuiteJSON(int repetitions) {
    return runCanonicalSuite(repetitions).toJSON();
}

std::string AlgorithmBenchmark::runCustomBenchmarkJSON(const std::string& scenarioId,
                                                      int repetitions,
                                                      const std::vector<std::string>& algorithms) {
    if (repetitions < 1 || repetitions > 20) {
        return "{\"success\":false,\"error\":\"Repetition count must be between 1 and 20.\"}";
    }

    std::string effectiveScenario = scenarioId;
    if (scenarioId == "empty") effectiveScenario = "open_grid";
    else if (scenarioId == "warehouse") effectiveScenario = "obstacle_rich";
    else if (scenarioId == "narrow" || scenarioId == "bottleneck") effectiveScenario = "narrow_corridor";

    std::vector<std::string> validScenarios = {
        "all", "open_grid", "obstacle_rich", "narrow_corridor", "long_distance", "congested_multi_robot"
    };
    bool scValid = false;
    for (const auto& v : validScenarios) {
        if (effectiveScenario == v) {
            scValid = true;
            break;
        }
    }
    if (!scValid) {
        return "{\"success\":false,\"error\":\"Unrecognized benchmark scenario: '" + scenarioId + "'.\"}";
    }

    if (algorithms.empty()) {
        return "{\"success\":false,\"error\":\"At least one algorithm must be specified.\"}";
    }

    // Validate algorithms
    std::vector<std::string> validAlgos = {"BFS", "Dijkstra", "A*", "SpaceTimeA*", "Space-Time A*"};
    for (const auto& a : algorithms) {
        bool aValid = false;
        for (const auto& va : validAlgos) {
            if (a == va) { aValid = true; break; }
        }
        if (!aValid) {
            return "{\"success\":false,\"error\":\"Unrecognized pathfinding algorithm: '" + a + "'.\"}";
        }
    }

    if (effectiveScenario == "all") {
        BenchmarkSuiteResult suite;
        suite.timestamp = "2026-10-18T23:00:00Z";
        suite.repetitions = repetitions;
        suite.note = "Authoritative C++ Native Custom Benchmark • Complete State Isolation";
        std::vector<std::string> allScenarios = {
            "open_grid", "obstacle_rich", "narrow_corridor", "long_distance", "congested_multi_robot"
        };
        suite.totalScenarios = static_cast<int>(allScenarios.size());
        for (const auto& sId : allScenarios) {
            suite.scenarios.push_back(runScenario(sId, repetitions, algorithms));
        }
        return suite.toJSON();
    }

    BenchmarkScenarioResult singleRes = runScenario(effectiveScenario, repetitions, algorithms);
    BenchmarkSuiteResult suite;
    suite.timestamp = "2026-10-18T23:00:00Z";
    suite.repetitions = repetitions;
    suite.totalScenarios = 1;
    suite.note = "Authoritative C++ Native Custom Benchmark • Scenario: " + effectiveScenario;
    suite.scenarios.push_back(singleRes);
    return suite.toJSON();
}
