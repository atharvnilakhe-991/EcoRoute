#include "../include/EcoRoute.h"
#include "../include/PathFinder.h"
#include <queue>
#include <map>
#include <unordered_set>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// ============================================================================
// RobotEnergyProfile Implementation
// ============================================================================

RobotEnergyProfile RobotEnergyProfile::getProfile(const std::string& type) {
    RobotEnergyProfile p;
    if (type == "TransportRobot") {
        p.robotType = "TransportRobot";
        p.baseMoveCost = 0.9;
        p.payloadEnergyFactor = 0.08;
        p.idleCostPerTick = 0.08;
        p.pickActionCost = 2.0;
        p.deliverActionCost = 1.0;
        p.minSafetyReserve = 15.0;
        p.maxBatteryCapacity = 100.0;
    } else if (type == "HybridRobot") {
        p.robotType = "HybridRobot";
        p.baseMoveCost = 0.7;
        p.payloadEnergyFactor = 0.06;
        p.idleCostPerTick = 0.06;
        p.pickActionCost = 1.5;
        p.deliverActionCost = 0.7;
        p.minSafetyReserve = 15.0;
        p.maxBatteryCapacity = 100.0;
    } else {
        // Default: PickerRobot
        p.robotType = "PickerRobot";
        p.baseMoveCost = 0.5;
        p.payloadEnergyFactor = 0.05;
        p.idleCostPerTick = 0.05;
        p.pickActionCost = 1.0;
        p.deliverActionCost = 0.5;
        p.minSafetyReserve = 15.0;
        p.maxBatteryCapacity = 100.0;
    }
    return p;
}

std::string RobotEnergyProfile::toJSON() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"robotType\":\"" << robotType << "\","
       << "\"baseMoveCost\":" << baseMoveCost << ","
       << "\"payloadEnergyFactor\":" << payloadEnergyFactor << ","
       << "\"idleCostPerTick\":" << idleCostPerTick << ","
       << "\"pickActionCost\":" << pickActionCost << ","
       << "\"deliverActionCost\":" << deliverActionCost << ","
       << "\"minSafetyReserve\":" << minSafetyReserve << ","
       << "\"maxBatteryCapacity\":" << maxBatteryCapacity
       << "}";
    return ss.str();
}

// ============================================================================
// Serialization Implementations
// ============================================================================

std::string EcoRouteResult::toJSON() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"found\":" << (found ? "true" : "false") << ","
       << "\"strategy\":\"" << strategy << "\","
       << "\"status\":\"" << statusMessage << "\","
       << "\"distance\":" << distance << ","
       << "\"pathLength\":" << pathLength << ","
       << "\"estimatedEnergy\":" << estimatedEnergy << ","
       << "\"nodesExplored\":" << nodesExplored << ","
       << "\"executionTimeMicroseconds\":" << executionTimeMicroseconds << ","
       << "\"path\":[";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << path[i].x << ",\"y\":" << path[i].y << "}";
    }
    ss << "]}";
    return ss.str();
}

std::string RouteComparisonResult::toJSON() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);

    auto escapeJson = [](const std::string& str) {
        std::string out;
        for (char c : str) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    };

    ss << "{"
       << "\"start\":{\"x\":" << start.x << ",\"y\":" << start.y << "},"
       << "\"goal\":{\"x\":" << goal.x << ",\"y\":" << goal.y << "},"
       << "\"robotType\":\"" << robotType << "\","
       << "\"payload\":" << payload << ","
       << "\"energySaved\":" << energySaved << ","
       << "\"energySavingsPercent\":" << energySavingsPercent << ","
       << "\"distancePenalty\":" << distancePenalty << ","
       << "\"summary\":\"" << escapeJson(summary) << "\","
       << "\"shortestRoute\":" << shortestRoute.toJSON() << ","
       << "\"ecoRoute\":" << ecoRoute.toJSON()
       << "}";
    return ss.str();
}

std::string RouteComparisonResult::toCSV() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Strategy,Found,DistanceSteps,PathCells,EstimatedEnergyEU,NodesExplored,ExecutionTimeUs,Start,Goal\n";
    ss << "ShortestDistance,"
       << (shortestRoute.found ? "true" : "false") << ","
       << shortestRoute.distance << ","
       << shortestRoute.pathLength << ","
       << shortestRoute.estimatedEnergy << ","
       << shortestRoute.nodesExplored << ","
       << shortestRoute.executionTimeMicroseconds << ","
       << "(" << start.x << ";" << start.y << "),"
       << "(" << goal.x << ";" << goal.y << ")\n";
    ss << "EcoRoute,"
       << (ecoRoute.found ? "true" : "false") << ","
       << ecoRoute.distance << ","
       << ecoRoute.pathLength << ","
       << ecoRoute.estimatedEnergy << ","
       << ecoRoute.nodesExplored << ","
       << ecoRoute.executionTimeMicroseconds << ","
       << "(" << start.x << ";" << start.y << "),"
       << "(" << goal.x << ";" << goal.y << ")\n";
    return ss.str();
}

std::string BatteryFeasibilityReport::toJSON() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"robotId\":\"" << robotId << "\","
       << "\"robotType\":\"" << robotType << "\","
       << "\"currentBattery\":" << currentBattery << ","
       << "\"isFeasible\":" << (isFeasible ? "true" : "false") << ","
       << "\"energyPickupLeg\":" << energyPickupLeg << ","
       << "\"energyDeliveryLeg\":" << energyDeliveryLeg << ","
       << "\"energyChargingLeg\":" << energyChargingLeg << ","
       << "\"energyActions\":" << energyActions << ","
       << "\"safetyReserve\":" << safetyReserve << ","
       << "\"totalEnergyRequired\":" << totalEnergyRequired << ","
       << "\"batteryMargin\":" << batteryMargin << ","
       << "\"recommendedStationId\":\"" << recommendedStationId << "\","
       << "\"recommendedChargingStation\":{\"x\":" << recommendedChargingStation.x << ",\"y\":" << recommendedChargingStation.y << "},"
       << "\"statusReason\":\"" << statusReason << "\""
       << "}";
    return ss.str();
}

std::string ChargingRecommendation::toJSON() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"stationId\":\"" << stationId << "\","
       << "\"position\":{\"x\":" << position.x << ",\"y\":" << position.y << "},"
       << "\"estimatedEnergy\":" << estimatedEnergy << ","
       << "\"distance\":" << distance << ","
       << "\"feasible\":" << (feasible ? "true" : "false") << ","
       << "\"isOccupied\":" << (isOccupied ? "true" : "false") << ","
       << "\"occupyingRobotId\":\"" << occupyingRobotId << "\","
       << "\"statusMessage\":\"" << statusMessage << "\","
       << "\"route\":[";
    for (size_t i = 0; i < route.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << route[i].x << ",\"y\":" << route[i].y << "}";
    }
    ss << "]}";
    return ss.str();
}

// ============================================================================
// EcoRoute Engine Implementation
// ============================================================================

double EcoRoute::calculateStepEnergy(const RobotEnergyProfile& profile, int payload, double cellCost) {
    double base = profile.baseMoveCost + (profile.payloadEnergyFactor * static_cast<double>(payload));
    return base * std::max(1.0, cellCost);
}

double EcoRoute::calculateRouteEnergy(const RobotEnergyProfile& profile, const std::vector<Position>& route, const Warehouse& warehouse, int payload) {
    if (route.size() <= 1) return 0.0;
    double totalEnergy = 0.0;
    for (size_t i = 1; i < route.size(); ++i) {
        if (route[i] == route[i - 1]) {
            totalEnergy += profile.idleCostPerTick; // Waiting tick
        } else {
            double cellCost = warehouse.getCellCost(route[i]);
            totalEnergy += calculateStepEnergy(profile, payload, cellCost);
        }
    }
    return totalEnergy;
}

EcoRouteResult EcoRoute::findEcoRoute(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload) {
    auto startTime = std::chrono::high_resolution_clock::now();
    EcoRouteResult result;
    result.strategy = "EcoRoute";
    result.found = false;

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
        result.pathLength = 1;
        result.distance = 0;
        result.estimatedEnergy = 0.0;
        result.nodesExplored = 1;
        result.statusMessage = "SUCCESS";
        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
        return result;
    }

    // A* guided by strictly admissible energy heuristic:
    // h(n) = manhattanDistance(n, goal) * minStepEnergy
    const double minStepEnergy = calculateStepEnergy(profile, payload, warehouse.getMinCellCost());

    struct Node {
        double fCost;
        double gCost;
        Position pos;
        bool operator>(const Node& other) const {
            return fCost > other.fCost;
        }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::map<Position, double> gScore;
    std::map<Position, Position> parent;
    std::unordered_set<Position> closedSet;

    gScore[start] = 0.0;
    double initialH = static_cast<double>(start.manhattanDistance(goal)) * minStepEnergy;
    openSet.push({initialH, 0.0, start});

    int explored = 0;

    while (!openSet.empty()) {
        Node curr = openSet.top();
        openSet.pop();

        if (closedSet.find(curr.pos) != closedSet.end()) {
            continue;
        }
        closedSet.insert(curr.pos);
        explored++;

        if (curr.pos == goal) {
            result.found = true;
            result.statusMessage = "SUCCESS";
            break;
        }

        for (const auto& next : warehouse.getWalkableNeighbors(curr.pos)) {
            if (closedSet.find(next) != closedSet.end()) continue;

            double stepEnergy = calculateStepEnergy(profile, payload, warehouse.getCellCost(next));
            double tentativeG = curr.gCost + stepEnergy;

            auto it = gScore.find(next);
            if (it == gScore.end() || tentativeG < it->second) {
                gScore[next] = tentativeG;
                parent[next] = curr.pos;
                double h = static_cast<double>(next.manhattanDistance(goal)) * minStepEnergy;
                openSet.push({tentativeG + h, tentativeG, next});
            }
        }
    }

    result.nodesExplored = explored;

    if (result.found) {
        // Reconstruct path
        std::vector<Position> totalPath;
        Position cur = goal;
        totalPath.push_back(cur);
        while (parent.find(cur) != parent.end()) {
            cur = parent[cur];
            totalPath.push_back(cur);
        }
        std::reverse(totalPath.begin(), totalPath.end());
        result.path = std::move(totalPath);
        result.pathLength = static_cast<int>(result.path.size());
        result.distance = std::max(0, result.pathLength - 1);
        result.estimatedEnergy = gScore[goal];
    } else {
        result.statusMessage = "NO_PATH_FOUND";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

EcoRouteResult EcoRoute::findShortestRoute(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload) {
    auto startTime = std::chrono::high_resolution_clock::now();
    EcoRouteResult result;
    result.strategy = "ShortestDistance";

    // Use standard BFS to guarantee strictly minimal hop count
    PathResult bfsRes = PathFinder::findPathBFS(warehouse, start, goal);
    result.found = bfsRes.found;
    result.statusMessage = bfsRes.statusMessage;
    result.path = bfsRes.path;
    result.pathLength = static_cast<int>(bfsRes.path.size());
    result.distance = std::max(0, result.pathLength - 1);
    result.nodesExplored = bfsRes.nodesExplored;

    if (result.found) {
        result.estimatedEnergy = calculateRouteEnergy(profile, result.path, warehouse, payload);
    } else {
        result.estimatedEnergy = 0.0;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTimeMicroseconds = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    return result;
}

RouteComparisonResult EcoRoute::compareRoutes(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload) {
    RouteComparisonResult comp;
    comp.start = start;
    comp.goal = goal;
    comp.robotType = profile.robotType;
    comp.payload = payload;

    comp.shortestRoute = findShortestRoute(warehouse, start, goal, profile, payload);
    comp.ecoRoute = findEcoRoute(warehouse, start, goal, profile, payload);

    if (comp.shortestRoute.found && comp.ecoRoute.found) {
        comp.energySaved = std::max(0.0, comp.shortestRoute.estimatedEnergy - comp.ecoRoute.estimatedEnergy);
        if (comp.shortestRoute.estimatedEnergy > 0.0001) {
            comp.energySavingsPercent = (comp.energySaved / comp.shortestRoute.estimatedEnergy) * 100.0;
        } else {
            comp.energySavingsPercent = 0.0;
        }
        comp.distancePenalty = std::max(0, comp.ecoRoute.distance - comp.shortestRoute.distance);

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        if (comp.energySaved > 0.05) {
            ss << "EcoRoute saves " << comp.energySaved << " EU (" << comp.energySavingsPercent 
               << "%) by bypassing high-cost transit cells (Distance penalty: +" << comp.distancePenalty << " steps).";
        } else {
            ss << "Shortest route is already energy-optimal (Distance: " << comp.shortestRoute.distance 
               << " steps, Energy: " << comp.ecoRoute.estimatedEnergy << " EU).";
        }
        comp.summary = ss.str();
    } else {
        comp.energySaved = 0.0;
        comp.energySavingsPercent = 0.0;
        comp.distancePenalty = 0;
        comp.summary = "Route comparison unavailable: one or both routes could not be completed.";
    }

    return comp;
}

BatteryFeasibilityReport EcoRoute::validateBatteryFeasibility(
    const Robot& robot,
    const Order& order,
    const Warehouse& warehouse,
    const std::vector<std::unique_ptr<Robot>>& allRobots,
    const Position& targetShelfPos,
    double customReserve)
{
    BatteryFeasibilityReport report;
    report.robotId = robot.getId();
    report.robotType = robot.getRobotTypeName();
    report.currentBattery = robot.getBattery();

    RobotEnergyProfile profile = RobotEnergyProfile::getProfile(robot.getRobotTypeName());
    report.safetyReserve = (customReserve >= 0.0) ? customReserve : profile.minSafetyReserve;

    // Check payload capacity
    if (robot.getPayloadCapacity() < order.getQuantity()) {
        report.isFeasible = false;
        report.statusReason = "Payload capacity exceeded (" + std::to_string(robot.getPayloadCapacity()) + " < " + std::to_string(order.getQuantity()) + ")";
        return report;
    }

    // Determine walkable access cell for shelf (if target shelf cell itself is impassable)
    Position pickupPos = targetShelfPos;
    if (!warehouse.isWalkable(targetShelfPos)) {
        static const int dx[] = {0, 1, 0, -1};
        static const int dy[] = {-1, 0, 1, 0};
        int bestDist = 1000000;
        Position bestAdj(-1, -1);
        for (int i = 0; i < 4; ++i) {
            Position adj(targetShelfPos.x + dx[i], targetShelfPos.y + dy[i]);
            if (warehouse.isValidPosition(adj) && warehouse.isWalkable(adj)) {
                int d = robot.getPosition().manhattanDistance(adj);
                if (d < bestDist) {
                    bestDist = d;
                    bestAdj = adj;
                }
            }
        }
        if (bestAdj.x != -1) {
            pickupPos = bestAdj;
        }
    }

    // 1. Leg 1: Current Pos -> Target Shelf access location (empty payload)
    EcoRouteResult leg1 = findEcoRoute(warehouse, robot.getPosition(), pickupPos, profile, 0);
    if (!leg1.found) {
        report.isFeasible = false;
        report.statusReason = "Pickup shelf unreachable from robot location";
        return report;
    }
    report.energyPickupLeg = leg1.estimatedEnergy;

    // 2. Action 1: Picking at shelf
    double pickCost = profile.pickActionCost;

    // 3. Leg 2: Shelf access location -> Packing Station (loaded with order.quantity)
    Position packPos = warehouse.getNearestPackingStation(pickupPos);
    EcoRouteResult leg2 = findEcoRoute(warehouse, pickupPos, packPos, profile, order.getQuantity());
    if (!leg2.found) {
        report.isFeasible = false;
        report.statusReason = "Packing station unreachable from shelf location";
        return report;
    }
    report.energyDeliveryLeg = leg2.estimatedEnergy;

    // 4. Action 2: Delivering at packing station
    double deliverCost = profile.deliverActionCost;
    report.energyActions = pickCost + deliverCost;

    // 5. Leg 3: Packing Station -> Optimal Charging Station (empty payload)
    double postDeliveryBattery = report.currentBattery - (report.energyPickupLeg + report.energyDeliveryLeg + report.energyActions);
    ChargingRecommendation chargeRec = selectBestChargingStation(packPos, postDeliveryBattery, profile, warehouse, allRobots);

    if (!chargeRec.feasible) {
        report.isFeasible = false;
        report.statusReason = "INSUFFICIENT_BATTERY: No feasible charging station reachable post-delivery: " + chargeRec.statusMessage;
        report.energyChargingLeg = chargeRec.estimatedEnergy;
        report.totalEnergyRequired = report.energyPickupLeg + report.energyDeliveryLeg + report.energyActions + report.energyChargingLeg + report.safetyReserve;
        report.batteryMargin = report.currentBattery - report.totalEnergyRequired;
        return report;
    }

    report.recommendedChargingStation = chargeRec.position;
    report.recommendedStationId = chargeRec.stationId;
    report.energyChargingLeg = chargeRec.estimatedEnergy;

    report.totalEnergyRequired = report.energyPickupLeg + report.energyDeliveryLeg + report.energyActions + report.energyChargingLeg + report.safetyReserve;
    report.batteryMargin = report.currentBattery - report.totalEnergyRequired;

    if (report.batteryMargin >= 0.0) {
        report.isFeasible = true;
        report.statusReason = "FEASIBLE";
    } else {
        report.isFeasible = false;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "INSUFFICIENT_BATTERY: Requires " << report.totalEnergyRequired << " EU (Deficit: " << (-report.batteryMargin) << " EU)";
        report.statusReason = ss.str();
    }

    return report;
}

ChargingRecommendation EcoRoute::selectBestChargingStation(
    const Position& currentPos,
    double currentBattery,
    const RobotEnergyProfile& profile,
    const Warehouse& warehouse,
    const std::vector<std::unique_ptr<Robot>>& allRobots)
{
    ChargingRecommendation bestRec;
    bestRec.feasible = false;
    bestRec.estimatedEnergy = std::numeric_limits<double>::infinity();
    bestRec.statusMessage = "NO_CHARGING_STATIONS_CONFIGURED";

    const auto& stations = warehouse.getChargingStations();
    if (stations.empty()) {
        return bestRec;
    }

    // Build set of occupied stations or stations targeted by other robots
    std::unordered_set<Position> occupiedPositions;
    std::map<Position, std::string> occupantIds;

    for (const auto& r : allRobots) {
        if (!r) continue;
        // If robot is currently on a charging station
        occupiedPositions.insert(r->getPosition());
        occupantIds[r->getPosition()] = r->getId();

        // If robot has active task to charge
        if (r->getCurrentTask() && r->getCurrentTask()->getType() == TaskType::RETURN_TO_CHARGE) {
            occupiedPositions.insert(r->getCurrentTask()->getTargetLocation());
            occupantIds[r->getCurrentTask()->getTargetLocation()] = r->getId();
        }
    }

    ChargingRecommendation fallbackOccupiedBest;
    fallbackOccupiedBest.estimatedEnergy = std::numeric_limits<double>::infinity();
    bool anyReachable = false;

    for (const auto& st : stations) {
        Position stPos = st.getPosition();
        EcoRouteResult route = findEcoRoute(warehouse, currentPos, stPos, profile, 0);

        if (!route.found) continue;
        anyReachable = true;

        bool isOccupied = (occupiedPositions.find(stPos) != occupiedPositions.end() && stPos != currentPos);
        std::string occId = isOccupied ? occupantIds[stPos] : "";

        ChargingRecommendation candidate;
        candidate.stationId = st.getId();
        candidate.position = stPos;
        candidate.estimatedEnergy = route.estimatedEnergy;
        candidate.route = route.path;
        candidate.distance = route.distance;
        candidate.isOccupied = isOccupied;
        candidate.occupyingRobotId = occId;
        candidate.feasible = (currentBattery >= route.estimatedEnergy);

        if (!isOccupied) {
            if (candidate.feasible && candidate.estimatedEnergy < bestRec.estimatedEnergy) {
                bestRec = candidate;
                bestRec.statusMessage = "AVAILABLE";
            }
        } else {
            if (candidate.feasible && candidate.estimatedEnergy < fallbackOccupiedBest.estimatedEnergy) {
                fallbackOccupiedBest = candidate;
                fallbackOccupiedBest.statusMessage = "OCCUPIED_BY_" + occId;
            }
        }
    }

    if (bestRec.feasible && bestRec.estimatedEnergy < std::numeric_limits<double>::infinity()) {
        return bestRec;
    }

    // If all stations are occupied but reachable, return the lowest-energy occupied station with warning
    if (fallbackOccupiedBest.feasible && fallbackOccupiedBest.estimatedEnergy < std::numeric_limits<double>::infinity()) {
        fallbackOccupiedBest.statusMessage = "WARNING: Station " + fallbackOccupiedBest.stationId + " currently occupied by " + fallbackOccupiedBest.occupyingRobotId;
        return fallbackOccupiedBest;
    }

    // Completely unfeasible / unreachable
    bestRec.feasible = false;
    if (!anyReachable) {
        bestRec.statusMessage = "NO_REACHABLE_CHARGING_STATIONS";
    } else {
        bestRec.statusMessage = "INSUFFICIENT_BATTERY_TO_REACH_ANY_STATION";
    }
    return bestRec;
}
