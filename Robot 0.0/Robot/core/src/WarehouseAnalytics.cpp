#include "../include/WarehouseAnalytics.h"
#include "../include/Robot.h"
#include "../include/Order.h"
#include "../include/Product.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

std::string RobotUtilizationMetrics::toJSON(int totalObservedTicks) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"robotId\":\"" << robotId << "\","
       << "\"robotName\":\"" << robotName << "\","
       << "\"robotType\":\"" << robotType << "\","
       << "\"totalObservedTicks\":" << totalObservedTicks << ","
       << "\"idleTicks\":" << idleTicks << ","
       << "\"workingTicks\":" << workingTicks << ","
       << "\"waitingTicks\":" << waitingTicks << ","
       << "\"blockedTicks\":" << blockedTicks << ","
       << "\"chargingTicks\":" << chargingTicks << ","
       << "\"movementSteps\":" << movementSteps << ","
       << "\"tasksAssigned\":" << tasksAssigned << ","
       << "\"tasksCompleted\":" << tasksCompleted << ","
       << "\"batteryConsumed\":" << batteryConsumed << ","
       << "\"chargingEvents\":" << chargingEvents << ","
       << "\"workingUtilization\":" << getWorkingUtilization(totalObservedTicks) << ","
       << "\"waitingPercentage\":" << getWaitingPercentage(totalObservedTicks) << ","
       << "\"blockedPercentage\":" << getBlockedPercentage(totalObservedTicks) << ","
       << "\"chargingPercentage\":" << getChargingPercentage(totalObservedTicks) << ","
       << "\"idlePercentage\":" << getIdlePercentage(totalObservedTicks) << ","
       << "\"averageTaskDuration\":" << getAverageTaskDuration() << ","
       << "\"batteryDrainRate\":" << getBatteryDrainRate() << ","
       << "\"batteryTrend\":\"" << getBatteryTrend() << "\","
       << "\"batteryHistory\":[";
    for (size_t i = 0; i < batteryHistory.size(); ++i) {
        if (i > 0) ss << ",";
        ss << batteryHistory[i];
    }
    ss << "]}";
    return ss.str();
}

std::string OperationalRecommendation::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << id << "\","
       << "\"severity\":\"" << severity << "\","
       << "\"category\":\"" << category << "\","
       << "\"title\":\"" << title << "\","
       << "\"description\":\"" << description << "\","
       << "\"triggerReason\":\"" << triggerReason << "\""
       << "}";
    return ss.str();
}

WarehouseAnalytics::WarehouseAnalytics(int width, int height)
    : totalObservedTicks(0),
      lastRecordedTick(-1),
      warehouseWidth(width > 0 ? width : 15),
      warehouseHeight(height > 0 ? height : 15),
      totalOrdersCreated(0),
      totalOrdersCompleted(0),
      totalOrdersFailed(0),
      totalUnitsFulfilled(0) {
    resetHeatmap();
}

void WarehouseAnalytics::setDimensions(int width, int height) {
    if (width > 0) warehouseWidth = width;
    if (height > 0) warehouseHeight = height;
    resetHeatmap();
}

bool WarehouseAnalytics::isValidCell(int x, int y) const {
    return (x >= 0 && x < warehouseWidth && y >= 0 && y < warehouseHeight);
}

void WarehouseAnalytics::recordTick(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots) {
    if (currentTick > 0 && currentTick == lastRecordedTick) {
        // Prevent duplicate counting of the same simulation tick
        return;
    }
    lastRecordedTick = currentTick;
    totalObservedTicks++;

    for (const auto& r : robots) {
        if (!r) continue;
        const std::string& rId = r->getId();
        auto& metrics = robotMetrics[rId];

        metrics.robotId = rId;
        metrics.robotName = r->getName();
        metrics.robotType = r->getRobotTypeName();

        // Record initial position baseline if not yet captured
        if (initialRobotPositions.find(rId) == initialRobotPositions.end()) {
            initialRobotPositions[rId] = r->getPosition();
        }

        // 1. Authoritative status tick categorization
        RobotStatus status = r->getStatus();
        switch (status) {
            case RobotStatus::IDLE:
                metrics.idleTicks++;
                break;
            case RobotStatus::WAITING:
                metrics.waitingTicks++;
                break;
            case RobotStatus::BLOCKED:
                metrics.blockedTicks++;
                break;
            case RobotStatus::CHARGING:
                metrics.chargingTicks++;
                break;
            case RobotStatus::MOVING_TO_PICK:
            case RobotStatus::PICKING:
            case RobotStatus::MOVING_TO_PACK:
            case RobotStatus::DELIVERING:
            case RobotStatus::RETURNING_TO_CHARGE:
            case RobotStatus::LOW_BATTERY:
                metrics.workingTicks++;
                break;
        }

        // 2. Movement step detection (only when physical position actually changes)
        Position curPos = r->getPosition();
        if (metrics.hasInitializedPosition) {
            if (!(curPos == metrics.lastPosition)) {
                metrics.movementSteps++;
            }
        } else {
            metrics.hasInitializedPosition = true;
        }
        metrics.lastPosition = curPos;

        // 3. Battery consumption accounting & sample history
        double curBatt = r->getBattery();
        metrics.batteryHistory.push_back(curBatt);
        if (metrics.batteryHistory.size() > 50) {
            metrics.batteryHistory.erase(metrics.batteryHistory.begin());
        }
        if (metrics.lastBattery > curBatt) {
            metrics.batteryConsumed += (metrics.lastBattery - curBatt);
        } else if (curBatt > metrics.lastBattery + 5.0) {
            // Distinct charging event increment
            metrics.chargingEvents++;
        }
        metrics.lastBattery = curBatt;

        // 4. Congestion Heatmap spatial observations
        if (isValidCell(curPos.x, curPos.y)) {
            cellVisits[curPos.y][curPos.x]++;
            if (status == RobotStatus::WAITING || status == RobotStatus::BLOCKED) {
                cellWaits[curPos.y][curPos.x]++;
            }
        }
    }
}

void WarehouseAnalytics::recordOrderCreated(const Order& order, int currentTick) {
    (void)order;
    (void)currentTick;
    totalOrdersCreated++;
}

void WarehouseAnalytics::recordOrderAssigned(const Order& order, const std::string& robotId, int currentTick) {
    (void)order;
    (void)currentTick;
    if (robotMetrics.find(robotId) != robotMetrics.end()) {
        robotMetrics[robotId].tasksAssigned++;
    }
}

void WarehouseAnalytics::recordOrderCompleted(const Order& order, const Product* product, int currentTick) {
    (void)currentTick;
    totalOrdersCompleted++;
    totalUnitsFulfilled += order.getQuantity();

    if (product) {
        fulfilledByCategory[product->getCategory()] += order.getQuantity();
    }

    int duration = order.getCompletionDuration();
    if (duration >= 0) {
        completionDurations.push_back(duration);
    }

    const std::string& rId = order.getAssignedRobotId();
    if (!rId.empty() && robotMetrics.find(rId) != robotMetrics.end()) {
        robotMetrics[rId].tasksCompleted++;
    }
}

void WarehouseAnalytics::recordOrderFailed(int orderId, int currentTick) {
    (void)orderId;
    (void)currentTick;
    totalOrdersFailed++;
}

void WarehouseAnalytics::recordReplanEvent(const Position& pos) {
    if (isValidCell(pos.x, pos.y)) {
        cellReplans[pos.y][pos.x]++;
    }
}

int WarehouseAnalytics::getObservedTicks() const {
    return totalObservedTicks;
}

const std::map<std::string, RobotUtilizationMetrics>& WarehouseAnalytics::getRobotMetrics() const {
    return robotMetrics;
}

const RobotUtilizationMetrics* WarehouseAnalytics::getMetricsForRobot(const std::string& robotId) const {
    auto it = robotMetrics.find(robotId);
    if (it != robotMetrics.end()) {
        return &(it->second);
    }
    return nullptr;
}

double WarehouseAnalytics::getFleetAverageUtilization() const {
    if (robotMetrics.empty() || totalObservedTicks <= 0) return 0.0;
    double sum = 0.0;
    for (const auto& pair : robotMetrics) {
        sum += pair.second.getWorkingUtilization(totalObservedTicks);
    }
    return sum / robotMetrics.size();
}

double WarehouseAnalytics::getFleetAverageWaitingPercentage() const {
    if (robotMetrics.empty() || totalObservedTicks <= 0) return 0.0;
    double sum = 0.0;
    for (const auto& pair : robotMetrics) {
        sum += pair.second.getWaitingPercentage(totalObservedTicks);
    }
    return sum / robotMetrics.size();
}

double WarehouseAnalytics::getFleetTotalBatteryConsumed() const {
    double sum = 0.0;
    for (const auto& pair : robotMetrics) {
        sum += pair.second.batteryConsumed;
    }
    return sum;
}

int WarehouseAnalytics::getFleetTotalMovementSteps() const {
    int total = 0;
    for (const auto& pair : robotMetrics) {
        total += pair.second.movementSteps;
    }
    return total;
}

int WarehouseAnalytics::getTotalOrdersCreated() const { return totalOrdersCreated; }
int WarehouseAnalytics::getTotalOrdersCompleted() const { return totalOrdersCompleted; }
int WarehouseAnalytics::getTotalOrdersFailed() const { return totalOrdersFailed; }
int WarehouseAnalytics::getTotalUnitsFulfilled() const { return totalUnitsFulfilled; }
const std::vector<int>& WarehouseAnalytics::getCompletionDurations() const { return completionDurations; }
const std::map<std::string, int>& WarehouseAnalytics::getFulfilledByCategory() const { return fulfilledByCategory; }

double WarehouseAnalytics::getAverageOrderCompletionTime() const {
    if (completionDurations.empty()) return -1.0;
    double sum = std::accumulate(completionDurations.begin(), completionDurations.end(), 0.0);
    return sum / completionDurations.size();
}

int WarehouseAnalytics::getMinOrderCompletionTime() const {
    if (completionDurations.empty()) return -1;
    return *std::min_element(completionDurations.begin(), completionDurations.end());
}

int WarehouseAnalytics::getMaxOrderCompletionTime() const {
    if (completionDurations.empty()) return -1;
    return *std::max_element(completionDurations.begin(), completionDurations.end());
}

double WarehouseAnalytics::getOrderCompletionRate() const {
    if (totalOrdersCreated <= 0) return 0.0;
    return (static_cast<double>(totalOrdersCompleted) / totalOrdersCreated) * 100.0;
}

int WarehouseAnalytics::getCellVisits(int x, int y) const {
    if (isValidCell(x, y)) return cellVisits[y][x];
    return 0;
}

int WarehouseAnalytics::getCellWaits(int x, int y) const {
    if (isValidCell(x, y)) return cellWaits[y][x];
    return 0;
}

int WarehouseAnalytics::getCellReplans(int x, int y) const {
    if (isValidCell(x, y)) return cellReplans[y][x];
    return 0;
}

int WarehouseAnalytics::getMaxCellVisits() const {
    int maxV = 0;
    for (int y = 0; y < warehouseHeight; ++y) {
        for (int x = 0; x < warehouseWidth; ++x) {
            if (cellVisits[y][x] > maxV) maxV = cellVisits[y][x];
        }
    }
    return maxV;
}

int WarehouseAnalytics::getMaxCellWaits() const {
    int maxW = 0;
    for (int y = 0; y < warehouseHeight; ++y) {
        for (int x = 0; x < warehouseWidth; ++x) {
            if (cellWaits[y][x] > maxW) maxW = cellWaits[y][x];
        }
    }
    return maxW;
}

int WarehouseAnalytics::getMaxCellReplans() const {
    int maxR = 0;
    for (int y = 0; y < warehouseHeight; ++y) {
        for (int x = 0; x < warehouseWidth; ++x) {
            if (cellReplans[y][x] > maxR) maxR = cellReplans[y][x];
        }
    }
    return maxR;
}

const std::vector<std::vector<int>>& WarehouseAnalytics::getCellVisitsMatrix() const { return cellVisits; }
const std::vector<std::vector<int>>& WarehouseAnalytics::getCellWaitsMatrix() const { return cellWaits; }
const std::vector<std::vector<int>>& WarehouseAnalytics::getCellReplansMatrix() const { return cellReplans; }

void WarehouseAnalytics::resetHeatmap() {
    cellVisits.assign(warehouseHeight, std::vector<int>(warehouseWidth, 0));
    cellWaits.assign(warehouseHeight, std::vector<int>(warehouseWidth, 0));
    cellReplans.assign(warehouseHeight, std::vector<int>(warehouseWidth, 0));
}

void WarehouseAnalytics::resetAnalytics() {
    totalObservedTicks = 0;
    lastRecordedTick = -1;
    robotMetrics.clear();
    totalOrdersCreated = 0;
    totalOrdersCompleted = 0;
    totalOrdersFailed = 0;
    totalUnitsFulfilled = 0;
    completionDurations.clear();
    fulfilledByCategory.clear();
    resetHeatmap();
}

void WarehouseAnalytics::resetAll() {
    resetAnalytics();
    initialRobotPositions.clear();
}

std::vector<OperationalRecommendation> WarehouseAnalytics::evaluateRecommendations(
    const std::vector<std::unique_ptr<Robot>>& robots,
    int pendingOrderCount,
    int collisionsPrevented,
    int deadlocksResolved,
    int routesReplanned) const {

    std::vector<OperationalRecommendation> recs;

    // 1. Battery Alerts
    for (const auto& r : robots) {
        if (!r) continue;
        if (r->getBattery() <= 20.0 && r->getStatus() != RobotStatus::CHARGING) {
            OperationalRecommendation rec;
            rec.id = "REC-BATT-" + r->getId();
            rec.severity = "CRITICAL";
            rec.category = "BATTERY";
            rec.title = "Critical Battery Level on " + r->getName();
            rec.description = "Robot " + r->getId() + " battery is at " + std::to_string(static_cast<int>(r->getBattery())) + "%. Route to charging station immediately.";
            rec.triggerReason = "Battery <= 20% while in operational status " + robotStatusToString(r->getStatus());
            recs.push_back(rec);
        } else if (r->getBattery() <= 35.0 && r->getStatus() == RobotStatus::IDLE) {
            OperationalRecommendation rec;
            rec.id = "REC-BATT-WARN-" + r->getId();
            rec.severity = "WARNING";
            rec.category = "BATTERY";
            rec.title = "Preventive Charge Advisory: " + r->getName();
            rec.description = "Robot " + r->getId() + " is idle with battery at " + std::to_string(static_cast<int>(r->getBattery())) + "%. Consider opportunity charging before next dispatch.";
            rec.triggerReason = "Battery <= 35% during IDLE standby";
            recs.push_back(rec);
        }
    }

    // 2. Waiting & Contention Alerts
    for (const auto& r : robots) {
        if (!r) continue;
        if (r->getWaitTicks() >= 3) {
            OperationalRecommendation rec;
            rec.id = "REC-WAIT-" + r->getId();
            rec.severity = "WARNING";
            rec.category = "COORDINATION";
            rec.title = "Prolonged Wait at Yield Point: " + r->getName();
            rec.description = "Robot " + r->getId() + " has waited " + std::to_string(r->getWaitTicks()) + " consecutive ticks at position (" + std::to_string(r->getPosition().x) + ", " + std::to_string(r->getPosition().y) + "). Space-time replanning may be triggered.";
            rec.triggerReason = "Consecutive waitTicks >= 3";
            recs.push_back(rec);
        }
    }

    // 3. Hotspot Spatial Congestion
    int maxV = getMaxCellVisits();
    if (maxV >= 10) {
        OperationalRecommendation rec;
        rec.id = "REC-CONGESTION-HIGH";
        rec.severity = "INFO";
        rec.category = "CONGESTION";
        rec.title = "High Traffic Corridor Detected";
        rec.description = "A central aisle has recorded " + std::to_string(maxV) + " robot traversals. Consider load balancing order dispatch across alternating packing stations.";
        rec.triggerReason = "Max cell visits reached " + std::to_string(maxV);
        recs.push_back(rec);
    }

    // 4. Pending Order Backlog
    int idleCount = 0;
    for (const auto& r : robots) {
        if (r && r->getStatus() == RobotStatus::IDLE) idleCount++;
    }
    if (pendingOrderCount > idleCount && pendingOrderCount >= 2) {
        OperationalRecommendation rec;
        rec.id = "REC-BACKLOG";
        rec.severity = "WARNING";
        rec.category = "DISPATCH";
        rec.title = "Pending Order Queue Backlog";
        rec.description = std::to_string(pendingOrderCount) + " orders awaiting assignment with only " + std::to_string(idleCount) + " idle robots available.";
        rec.triggerReason = "Pending orders (" + std::to_string(pendingOrderCount) + ") > Idle robots (" + std::to_string(idleCount) + ")";
        recs.push_back(rec);
    }

    // 5. Replanning Spikes
    if (routesReplanned >= 4) {
        OperationalRecommendation rec;
        rec.id = "REC-REPLAN-RATE";
        rec.severity = "INFO";
        rec.category = "COORDINATION";
        rec.title = "Dynamic Rerouting Activity Active";
        rec.description = std::to_string(routesReplanned) + " dynamic routes replanned to avoid obstacles and deadlocks.";
        rec.triggerReason = "Routes replanned count reached " + std::to_string(routesReplanned);
        recs.push_back(rec);
    }

    (void)collisionsPrevented;
    (void)deadlocksResolved;

    return recs;
}

std::string WarehouseAnalytics::generateCSVReport(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "# Robot Warehouse Management & Automation System — Operational Audit Report\n";
    ss << "# Generated at Tick: " << currentTick << " | Observed Ticks: " << totalObservedTicks << "\n\n";

    ss << "[FLEET_UTILIZATION_SUMMARY]\n";
    ss << "RobotID,Name,Type,WorkingUtilization(%),Waiting(%),Idle(%),Charging(%),MovementSteps,TasksAssigned,TasksCompleted,BatteryConsumed(%),CurrentBattery(%)\n";
    for (const auto& r : robots) {
        if (!r) continue;
        const auto* m = getMetricsForRobot(r->getId());
        double util = m ? m->getWorkingUtilization(totalObservedTicks) : 0.0;
        double waitP = m ? m->getWaitingPercentage(totalObservedTicks) : 0.0;
        double idleP = m ? m->getIdlePercentage(totalObservedTicks) : 0.0;
        double chgP = m ? m->getChargingPercentage(totalObservedTicks) : 0.0;
        int steps = m ? m->movementSteps : 0;
        int tAssign = m ? m->tasksAssigned : 0;
        int tComp = m ? m->tasksCompleted : 0;
        double bCons = m ? m->batteryConsumed : 0.0;

        ss << r->getId() << ",\""
           << r->getName() << "\",\""
           << r->getRobotTypeName() << "\","
           << util << ","
           << waitP << ","
           << idleP << ","
           << chgP << ","
           << steps << ","
           << tAssign << ","
           << tComp << ","
           << bCons << ","
           << r->getBattery() << "\n";
    }

    ss << "\n[ORDER_THROUGHPUT_METRICS]\n";
    ss << "TotalOrdersCreated,TotalOrdersCompleted,TotalOrdersFailed,TotalUnitsFulfilled,AverageDurationTicks,MinDurationTicks,MaxDurationTicks,CompletionRate(%)\n";
    ss << totalOrdersCreated << ","
       << totalOrdersCompleted << ","
       << totalOrdersFailed << ","
       << totalUnitsFulfilled << ","
       << getAverageOrderCompletionTime() << ","
       << getMinOrderCompletionTime() << ","
       << getMaxOrderCompletionTime() << ","
       << getOrderCompletionRate() << "\n";

    ss << "\n[CATEGORY_FULFILLMENT_DISTRIBUTION]\n";
    ss << "Category,UnitsFulfilled\n";
    for (const auto& pair : fulfilledByCategory) {
        ss << "\"" << pair.first << "\"," << pair.second << "\n";
    }

    return ss.str();
}

std::string WarehouseAnalytics::generateHTMLReport(int currentTick, const std::vector<std::unique_ptr<Robot>>& robots) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n"
       << "<title>Warehouse Simulation Analytics Report — Tick " << currentTick << "</title>\n"
       << "<style>\n"
       << "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background:#f8fafc; color:#0f172a; margin:0; padding:32px; }\n"
       << ".report-header { margin-bottom:24px; border-bottom:1px solid #e2e8f0; padding-bottom:16px; }\n"
       << "h1 { font-size:22px; margin:0 0 6px 0; color:#0f172a; }\n"
       << "table { width:100%; border-collapse:collapse; background:#ffffff; border-radius:8px; overflow:hidden; box-shadow:0 1px 3px rgba(0,0,0,0.05); margin-bottom:24px; }\n"
       << "th, td { padding:12px 14px; text-align:left; border-bottom:1px solid #f1f5f9; font-size:13px; }\n"
       << "th { background:#f8fafc; font-weight:600; color:#475569; }\n"
       << ".kpi-grid { display:grid; grid-template-columns:repeat(4, 1fr); gap:16px; margin-bottom:24px; }\n"
       << ".kpi-card { background:#ffffff; border:1px solid #e2e8f0; padding:16px; border-radius:8px; }\n"
       << ".kpi-title { font-size:12px; color:#64748b; margin-bottom:4px; text-transform:uppercase; font-weight:600; }\n"
       << ".kpi-val { font-size:24px; font-weight:700; color:#0f172a; }\n"
       << "</style>\n</head>\n<body>\n"
       << "<div class=\"report-header\">\n"
       << "  <h1>Warehouse Simulation Intelligence & Operations Report</h1>\n"
       << "  <div style=\"color:#64748b;font-size:13px;\">Simulation Tick: <strong>" << currentTick << "</strong> | Observed Duration: <strong>" << totalObservedTicks << " ticks</strong></div>\n"
       << "</div>\n";

    ss << "<div class=\"kpi-grid\">\n"
       << "  <div class=\"kpi-card\"><div class=\"kpi-title\">Fleet Avg Utilization</div><div class=\"kpi-val\">" << getFleetAverageUtilization() << "%</div></div>\n"
       << "  <div class=\"kpi-card\"><div class=\"kpi-title\">Orders Completed</div><div class=\"kpi-val\">" << totalOrdersCompleted << " / " << totalOrdersCreated << "</div></div>\n"
       << "  <div class=\"kpi-card\"><div class=\"kpi-title\">Avg Completion Time</div><div class=\"kpi-val\">" << (getAverageOrderCompletionTime() >= 0 ? std::to_string(static_cast<int>(getAverageOrderCompletionTime())) + " ticks" : "N/A") << "</div></div>\n"
       << "  <div class=\"kpi-card\"><div class=\"kpi-title\">Total Units Fulfilled</div><div class=\"kpi-val\">" << totalUnitsFulfilled << " units</div></div>\n"
       << "</div>\n";

    ss << "<h2>Fleet Utilization Breakdown</h2>\n<table>\n"
       << "<tr><th>Robot</th><th>Type</th><th>Working</th><th>Waiting</th><th>Idle</th><th>Charging</th><th>Steps</th><th>Tasks Done</th><th>Battery</th></tr>\n";
    for (const auto& r : robots) {
        if (!r) continue;
        const auto* m = getMetricsForRobot(r->getId());
        ss << "<tr>"
           << "<td><strong>" << r->getName() << "</strong> (" << r->getId() << ")</td>"
           << "<td>" << r->getRobotTypeName() << "</td>"
           << "<td>" << (m ? m->getWorkingUtilization(totalObservedTicks) : 0.0) << "%</td>"
           << "<td>" << (m ? m->getWaitingPercentage(totalObservedTicks) : 0.0) << "%</td>"
           << "<td>" << (m ? m->getIdlePercentage(totalObservedTicks) : 0.0) << "%</td>"
           << "<td>" << (m ? m->getChargingPercentage(totalObservedTicks) : 0.0) << "%</td>"
           << "<td>" << (m ? m->movementSteps : 0) << "</td>"
           << "<td>" << (m ? m->tasksCompleted : 0) << "</td>"
           << "<td>" << static_cast<int>(r->getBattery()) << "%</td>"
           << "</tr>\n";
    }
    ss << "</table>\n</body>\n</html>";
    return ss.str();
}

std::string WarehouseAnalytics::toJSON(const std::vector<std::unique_ptr<Robot>>& robots,
                                      int pendingOrderCount,
                                      int activeOrderCount,
                                      int collisionsPrevented,
                                      int deadlocksResolved,
                                      int routesReplanned) const {
    double highestBatt = 0.0;
    std::string highestBattRobotId = "";
    double lowestBatt = 100.0;
    std::string lowestBattRobotId = "";
    double sumBattery = 0.0;
    int totalChargingEvents = 0;
    int totalChargingTicks = 0;

    for (const auto& r : robots) {
        if (!r) continue;
        double b = r->getBattery();
        sumBattery += b;
        if (b >= highestBatt) {
            highestBatt = b;
            highestBattRobotId = r->getId();
        }
        if (b <= lowestBatt) {
            lowestBatt = b;
            lowestBattRobotId = r->getId();
        }
        const auto* m = getMetricsForRobot(r->getId());
        if (m) {
            totalChargingEvents += m->chargingEvents;
            totalChargingTicks += m->chargingTicks;
        }
    }
    double avgBattery = robots.empty() ? 100.0 : (sumBattery / robots.size());

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{"
       << "\"observedTicks\":" << totalObservedTicks << ","
       << "\"fleetAverageUtilization\":" << getFleetAverageUtilization() << ","
       << "\"fleetAverageWaitingPercentage\":" << getFleetAverageWaitingPercentage() << ","
       << "\"fleetTotalBatteryConsumed\":" << getFleetTotalBatteryConsumed() << ","
       << "\"fleetTotalMovementSteps\":" << getFleetTotalMovementSteps() << ","
       << "\"fleetAverageBattery\":" << avgBattery << ","
       << "\"fleetHighestBattery\":" << highestBatt << ","
       << "\"fleetHighestBatteryRobotId\":\"" << highestBattRobotId << "\","
       << "\"fleetLowestBattery\":" << lowestBatt << ","
       << "\"fleetLowestBatteryRobotId\":\"" << lowestBattRobotId << "\","
       << "\"fleetTotalChargingEvents\":" << totalChargingEvents << ","
       << "\"fleetTotalChargingTicks\":" << totalChargingTicks << ","
       << "\"orderThroughput\":{"
       << "\"created\":" << totalOrdersCreated << ","
       << "\"completed\":" << totalOrdersCompleted << ","
       << "\"pending\":" << pendingOrderCount << ","
       << "\"active\":" << activeOrderCount << ","
       << "\"failed\":" << totalOrdersFailed << ","
       << "\"totalUnitsFulfilled\":" << totalUnitsFulfilled << ","
       << "\"averageDurationTicks\":" << getAverageOrderCompletionTime() << ","
       << "\"minDurationTicks\":" << getMinOrderCompletionTime() << ","
       << "\"maxDurationTicks\":" << getMaxOrderCompletionTime() << ","
       << "\"completionRate\":" << getOrderCompletionRate() << ","
       << "\"categories\":{";
    size_t catIdx = 0;
    for (const auto& pair : fulfilledByCategory) {
        if (catIdx > 0) ss << ",";
        ss << "\"" << pair.first << "\":" << pair.second;
        catIdx++;
    }
    ss << "}},"
       << "\"robotUtilization\":[";
    size_t rIdx = 0;
    for (const auto& r : robots) {
        if (!r) continue;
        if (rIdx > 0) ss << ",";
        const auto* m = getMetricsForRobot(r->getId());
        if (m) {
            ss << m->toJSON(totalObservedTicks);
        } else {
            RobotUtilizationMetrics defM;
            defM.robotId = r->getId();
            defM.robotName = r->getName();
            defM.robotType = r->getRobotTypeName();
            ss << defM.toJSON(totalObservedTicks);
        }
        rIdx++;
    }
    ss << "],"
       << "\"congestion\":{"
       << "\"width\":" << warehouseWidth << ","
       << "\"height\":" << warehouseHeight << ","
       << "\"maxVisits\":" << getMaxCellVisits() << ","
       << "\"maxWaits\":" << getMaxCellWaits() << ","
       << "\"maxReplans\":" << getMaxCellReplans() << ","
       << "\"visits\":[";
    for (int y = 0; y < warehouseHeight; ++y) {
        if (y > 0) ss << ",";
        ss << "[";
        for (int x = 0; x < warehouseWidth; ++x) {
            if (x > 0) ss << ",";
            ss << cellVisits[y][x];
        }
        ss << "]";
    }
    ss << "],"
       << "\"waits\":[";
    for (int y = 0; y < warehouseHeight; ++y) {
        if (y > 0) ss << ",";
        ss << "[";
        for (int x = 0; x < warehouseWidth; ++x) {
            if (x > 0) ss << ",";
            ss << cellWaits[y][x];
        }
        ss << "]";
    }
    ss << "],"
       << "\"replans\":[";
    for (int y = 0; y < warehouseHeight; ++y) {
        if (y > 0) ss << ",";
        ss << "[";
        for (int x = 0; x < warehouseWidth; ++x) {
            if (x > 0) ss << ",";
            ss << cellReplans[y][x];
        }
        ss << "]";
    }
    ss << "]},"
       << "\"recommendations\":[";
    auto recs = evaluateRecommendations(robots, pendingOrderCount, collisionsPrevented, deadlocksResolved, routesReplanned);
    for (size_t i = 0; i < recs.size(); ++i) {
        if (i > 0) ss << ",";
        ss << recs[i].toJSON();
    }
    ss << "]}";

    return ss.str();
}
