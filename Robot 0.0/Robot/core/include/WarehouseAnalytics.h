#ifndef WAREHOUSE_ANALYTICS_H
#define WAREHOUSE_ANALYTICS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Position.h"

// Forward declarations to avoid circular dependencies
class Robot;
class Order;
class Product;

/**
 * @brief Categorization of robot operational states for utilization analysis.
 * Working statuses include all states where the robot is actively performing a mission task:
 * MOVING_TO_PICK, PICKING, MOVING_TO_PACK, DELIVERING, RETURNING_TO_CHARGE.
 */
struct RobotUtilizationMetrics {
    std::string robotId;
    std::string robotName;
    std::string robotType;
    int idleTicks = 0;
    int workingTicks = 0;
    int waitingTicks = 0;
    int blockedTicks = 0;
    int chargingTicks = 0;
    int movementSteps = 0;
    int tasksAssigned = 0;
    int tasksCompleted = 0;
    double batteryConsumed = 0.0;
    int chargingEvents = 0;

    // Tracking anchors
    double lastBattery = 100.0;
    Position lastPosition = Position(-1, -1);
    bool hasInitializedPosition = false;

    // Battery sample history for trend analysis
    std::vector<double> batteryHistory;

    double getWorkingUtilization(int totalObservedTicks) const {
        if (totalObservedTicks <= 0) return 0.0;
        return (static_cast<double>(workingTicks) / totalObservedTicks) * 100.0;
    }

    double getWaitingPercentage(int totalObservedTicks) const {
        if (totalObservedTicks <= 0) return 0.0;
        return (static_cast<double>(waitingTicks) / totalObservedTicks) * 100.0;
    }

    double getBlockedPercentage(int totalObservedTicks) const {
        if (totalObservedTicks <= 0) return 0.0;
        return (static_cast<double>(blockedTicks) / totalObservedTicks) * 100.0;
    }

    double getChargingPercentage(int totalObservedTicks) const {
        if (totalObservedTicks <= 0) return 0.0;
        return (static_cast<double>(chargingTicks) / totalObservedTicks) * 100.0;
    }

    double getIdlePercentage(int totalObservedTicks) const {
        if (totalObservedTicks <= 0) return 0.0;
        return (static_cast<double>(idleTicks) / totalObservedTicks) * 100.0;
    }

    double getAverageTaskDuration() const {
        if (tasksCompleted <= 0) return 0.0;
        return static_cast<double>(workingTicks) / tasksCompleted;
    }

    std::string getBatteryTrend() const {
        if (batteryHistory.size() < 2) return "INSUFFICIENT_HISTORY";
        double delta = batteryHistory.back() - batteryHistory[batteryHistory.size() - 2];
        if (delta < -0.05) return "DISCHARGING";
        if (delta > 0.05) return "CHARGING";
        return "STABLE";
    }

    double getBatteryDrainRate() const {
        if (batteryHistory.size() < 2) return 0.0;
        double delta = batteryHistory.front() - batteryHistory.back();
        return std::max(0.0, delta / static_cast<double>(batteryHistory.size() - 1));
    }

    std::string toJSON(int totalObservedTicks) const;
};

struct OperationalRecommendation {
    std::string id;
    std::string severity; // "CRITICAL", "WARNING", "INFO"
    std::string category; // "BATTERY", "CONGESTION", "INVENTORY", "DISPATCH", "COORDINATION"
    std::string title;
    std::string description;
    std::string triggerReason;

    std::string toJSON() const;
};

/**
 * @brief Authoritative simulation and operational analytics engine for the warehouse.
 * Collects actual lifecycle events, computes deterministic utilization and throughput
 * statistics, maintains a spatial congestion observation matrix, and generates audit reports.
 */
class WarehouseAnalytics {
private:
    int totalObservedTicks;
    int lastRecordedTick;
    int warehouseWidth;
    int warehouseHeight;

    // Robot Utilization Metrics (keyed by robotId)
    std::map<std::string, RobotUtilizationMetrics> robotMetrics;

    // Initial Robot Positions (recorded at registration or reset)
    std::map<std::string, Position> initialRobotPositions;

    // Order Lifecycle History
    int totalOrdersCreated;
    int totalOrdersCompleted;
    int totalOrdersFailed;
    int totalUnitsFulfilled;
    std::vector<int> completionDurations; // durations in simulation ticks
    std::map<std::string, int> fulfilledByCategory; // Category -> units

    // Spatial Congestion Heatmap Matrices
    std::vector<std::vector<int>> cellVisits;
    std::vector<std::vector<int>> cellWaits;
    std::vector<std::vector<int>> cellReplans;

public:
    WarehouseAnalytics(int width = 15, int height = 15);

    void setDimensions(int width, int height);

    // Event hooks called by simulation lifecycle
    void recordTick(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots);
    void recordOrderCreated(const Order& order, int currentTick);
    void recordOrderAssigned(const Order& order, const std::string& robotId, int currentTick);
    void recordOrderCompleted(const Order& order, const Product* product, int currentTick);
    void recordOrderFailed(int orderId, int currentTick);
    void recordReplanEvent(const Position& pos);

    // Queries & Calculations
    int getObservedTicks() const;
    const std::map<std::string, RobotUtilizationMetrics>& getRobotMetrics() const;
    const RobotUtilizationMetrics* getMetricsForRobot(const std::string& robotId) const;

    double getFleetAverageUtilization() const;
    double getFleetAverageWaitingPercentage() const;
    double getFleetTotalBatteryConsumed() const;
    int getFleetTotalMovementSteps() const;

    // Order Throughput
    int getTotalOrdersCreated() const;
    int getTotalOrdersCompleted() const;
    int getTotalOrdersFailed() const;
    int getTotalUnitsFulfilled() const;
    const std::vector<int>& getCompletionDurations() const;
    const std::map<std::string, int>& getFulfilledByCategory() const;
    double getAverageOrderCompletionTime() const;
    int getMinOrderCompletionTime() const;
    int getMaxOrderCompletionTime() const;
    double getOrderCompletionRate() const;

    // Congestion Heatmap
    bool isValidCell(int x, int y) const;
    int getCellVisits(int x, int y) const;
    int getCellWaits(int x, int y) const;
    int getCellReplans(int x, int y) const;
    int getMaxCellVisits() const;
    int getMaxCellWaits() const;
    int getMaxCellReplans() const;
    const std::vector<std::vector<int>>& getCellVisitsMatrix() const;
    const std::vector<std::vector<int>>& getCellWaitsMatrix() const;
    const std::vector<std::vector<int>>& getCellReplansMatrix() const;

    // Independent Reset Operations
    void resetHeatmap();
    void resetAnalytics();
    void resetAll();

    // Recommendations Engine
    std::vector<OperationalRecommendation> evaluateRecommendations(
        const std::vector<std::unique_ptr<Robot>>& robots,
        int pendingOrderCount,
        int collisionsPrevented,
        int deadlocksResolved,
        int routesReplanned) const;

    // Reporting
    std::string generateCSVReport(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots) const;
    std::string generateHTMLReport(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots) const;

    // JSON Serialization
    std::string toJSON(const std::vector<std::unique_ptr<Robot>>& robots,
                       int pendingOrderCount,
                       int activeOrderCount,
                       int collisionsPrevented,
                       int deadlocksResolved,
                       int routesReplanned) const;
};

#endif // WAREHOUSE_ANALYTICS_H
