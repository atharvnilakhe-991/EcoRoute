#ifndef ECO_ROUTE_H
#define ECO_ROUTE_H

#include <string>
#include <vector>
#include <memory>
#include <limits>
#include "Position.h"
#include "Warehouse.h"
#include "Robot.h"
#include "Order.h"

/**
 * @brief Robot-specific energy consumption parameters.
 * Defines deterministic energy consumption rates in Normalized Energy Units (EU).
 */
struct RobotEnergyProfile {
    std::string robotType{"PickerRobot"};
    double baseMoveCost{0.5};          // EU per movement step on standard cell (1.0 weight)
    double payloadEnergyFactor{0.05};   // Additional EU per item payload per movement step
    double idleCostPerTick{0.05};       // EU per simulation tick while idle/waiting
    double pickActionCost{1.0};         // EU per pick operation
    double deliverActionCost{0.5};      // EU per delivery operation
    double minSafetyReserve{15.0};      // Minimum battery margin required to avoid critical discharge
    double maxBatteryCapacity{100.0};   // Full battery scale (100.0 EU)

    static RobotEnergyProfile getProfile(const std::string& type);
    std::string toJSON() const;
};

/**
 * @brief Result of an energy-aware or shortest-distance path calculation.
 */
struct EcoRouteResult {
    bool found{false};
    std::vector<Position> path;
    int distance{0};                    // Step count (path.size() - 1)
    int pathLength{0};                  // Cell count (path.size())
    double estimatedEnergy{0.0};        // Total estimated energy consumption (EU)
    int nodesExplored{0};
    double executionTimeMicroseconds{0.0};
    std::string statusMessage{"SUCCESS"};
    std::string strategy{"EcoRoute"};   // "EcoRoute" or "ShortestDistance"

    std::string toJSON() const;
};

/**
 * @brief Comparative evaluation between Shortest-Distance Baseline and Lowest-Energy EcoRoute.
 */
struct RouteComparisonResult {
    Position start{0, 0};
    Position goal{0, 0};
    std::string robotType{"PickerRobot"};
    int payload{0};

    EcoRouteResult shortestRoute;
    EcoRouteResult ecoRoute;

    double energySaved{0.0};            // Baseline Energy - EcoRoute Energy
    double energySavingsPercent{0.0};    // (energySaved / Baseline Energy) * 100.0
    int distancePenalty{0};             // EcoRoute distance - Shortest distance
    std::string summary;

    std::string toJSON() const;
    std::string toCSV() const;
};

/**
 * @brief Comprehensive 4-leg trip battery feasibility assessment for task assignment.
 */
struct BatteryFeasibilityReport {
    std::string robotId;
    std::string robotType;
    double currentBattery{0.0};

    double energyPickupLeg{0.0};        // Energy to transit from robot position to pickup shelf
    double energyDeliveryLeg{0.0};      // Energy to transit from shelf to packing station (with payload)
    double energyChargingLeg{0.0};      // Energy to transit from packing station to charging station
    double energyActions{0.0};          // Energy for picking and delivering actions
    double safetyReserve{15.0};         // Configured safety buffer

    double totalEnergyRequired{0.0};
    double batteryMargin{0.0};          // currentBattery - totalEnergyRequired
    bool isFeasible{false};

    Position recommendedChargingStation{0, 0};
    std::string recommendedStationId;
    std::string statusReason;

    std::string toJSON() const;
};

/**
 * @brief Recommendation and telemetry for an intelligent charging station assignment.
 */
struct ChargingRecommendation {
    std::string stationId;
    Position position{0, 0};
    double estimatedEnergy{0.0};
    std::vector<Position> route;
    int distance{0};
    bool feasible{false};
    bool isOccupied{false};
    std::string occupyingRobotId;
    std::string statusMessage;

    std::string toJSON() const;
};

/**
 * @brief EcoRoute: Energy-Aware Robot Routing, Optimization & Battery Governance Engine.
 */
class EcoRoute {
public:
    // Step and route energy calculation using the configurable energy model
    static double calculateStepEnergy(const RobotEnergyProfile& profile, int payload, double cellCost);
    static double calculateRouteEnergy(const RobotEnergyProfile& profile, const std::vector<Position>& route, const Warehouse& warehouse, int payload = 0);

    // Energy-Aware Pathfinding: Finds the path that minimizes cumulative energy consumption
    static EcoRouteResult findEcoRoute(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload = 0);

    // Shortest-Distance Baseline: Finds minimum hop path and evaluates its energy consumption
    static EcoRouteResult findShortestRoute(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload = 0);

    // Comparative Analysis between Shortest-Distance Baseline and Lowest-Energy EcoRoute
    static RouteComparisonResult compareRoutes(const Warehouse& warehouse, const Position& start, const Position& goal, const RobotEnergyProfile& profile, int payload = 0);

    // Battery Feasibility Validator for multi-leg task assignments
    static BatteryFeasibilityReport validateBatteryFeasibility(
        const Robot& robot,
        const Order& order,
        const Warehouse& warehouse,
        const std::vector<std::unique_ptr<Robot>>& allRobots,
        const Position& targetShelfPos,
        double customReserve = -1.0
    );

    // Intelligent Charging Station Selector avoiding occupancy conflicts and minimizing transit energy
    static ChargingRecommendation selectBestChargingStation(
        const Position& currentPos,
        double currentBattery,
        const RobotEnergyProfile& profile,
        const Warehouse& warehouse,
        const std::vector<std::unique_ptr<Robot>>& allRobots
    );
};

#endif // ECO_ROUTE_H
