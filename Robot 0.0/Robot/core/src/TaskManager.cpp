#include "../include/TaskManager.h"
#include "../include/WarehouseAnalytics.h"
#include "../include/PathFinder.h"
#include "../include/MultiRobotCoordinator.h"
#include "../include/EcoRoute.h"
#include <algorithm>
#include <sstream>
#include <limits>
#include <queue>
#include <unordered_set>

TaskManager::TaskManager() {}

void TaskManager::addOrder(std::shared_ptr<Order> order) {
    if (!order) return;

    // Insert sorted by Priority (HIGH > MEDIUM > LOW), preserving FIFO for equal priorities
    auto it = pendingOrders.begin();
    while (it != pendingOrders.end()) {
        if (static_cast<int>(order->getPriority()) > static_cast<int>((*it)->getPriority())) {
            break;
        }
        ++it;
    }
    pendingOrders.insert(it, order);
}

void TaskManager::clearOrders() {
    pendingOrders.clear();
    activeOrders.clear();
    completedOrders.clear();
}

void TaskManager::addPendingOrder(std::shared_ptr<Order> order) {
    if (order) pendingOrders.push_back(order);
}

void TaskManager::addActiveOrder(std::shared_ptr<Order> order) {
    if (order) activeOrders.push_back(order);
}

void TaskManager::addCompletedOrder(std::shared_ptr<Order> order) {
    if (order) completedOrders.push_back(order);
}

std::shared_ptr<Order> TaskManager::getOrderById(int orderId) {
    for (auto& o : pendingOrders) {
        if (o->getId() == orderId) return o;
    }
    for (auto& o : activeOrders) {
        if (o->getId() == orderId) return o;
    }
    for (auto& o : completedOrders) {
        if (o->getId() == orderId) return o;
    }
    return nullptr;
}

const std::deque<std::shared_ptr<Order>>& TaskManager::getPendingOrders() const {
    return pendingOrders;
}

const std::vector<std::shared_ptr<Order>>& TaskManager::getActiveOrders() const {
    return activeOrders;
}

const std::vector<std::shared_ptr<Order>>& TaskManager::getCompletedOrders() const {
    return completedOrders;
}

Robot* TaskManager::selectBestRobot(const Order& order, const std::vector<std::unique_ptr<Robot>>& robots, const Warehouse& warehouse, const Position& targetShelfPos) {
    Robot* bestRobot = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (const auto& r : robots) {
        // Robot must be idle, not currently executing a transit/staging route, have no active task, and have sufficient battery (> 20%)
        if (r->getStatus() != RobotStatus::IDLE || r->hasActiveRoute() || r->getBattery() < 20.0 || r->getCurrentTask() != nullptr || r->isStaging()) {
            continue;
        }

        // Must have capacity for the order quantity
        if (r->getPayloadCapacity() < order.getQuantity()) {
            continue;
        }

        // Battery feasibility validation using EcoRoute 4-leg trip assessment
        BatteryFeasibilityReport feasReport = EcoRoute::validateBatteryFeasibility(*r, order, warehouse, robots, targetShelfPos);
        if (!feasReport.isFeasible) {
            continue;
        }

        // Estimated Congestion: Count other robots within Manhattan radius of 3 from shelf
        int nearbyRobots = 0;
        for (const auto& other : robots) {
            if (other->getId() != r->getId() && other->getPosition().manhattanDistance(targetShelfPos) <= 3) {
                nearbyRobots++;
            }
        }

        // Scoring heuristic:
        // + Battery margin (0 to 100)
        // - Pickup leg energy penalty
        // - Congestion & expected waiting penalty
        // + Type matching specialization bonus
        double score = (feasReport.batteryMargin * 0.5) - (feasReport.energyPickupLeg * 2.0) - (nearbyRobots * 15.0);

        if (order.getQuantity() <= 5 && r->getRobotTypeName() == "PickerRobot") {
            score += 25.0; // Bonus for choosing fast picker for small order
        } else if (order.getQuantity() > 10 && r->getRobotTypeName() == "TransportRobot") {
            score += 25.0; // Bonus for choosing heavy transport
        } else if (order.getQuantity() > 5 && order.getQuantity() <= 10 && r->getRobotTypeName() == "HybridRobot") {
            score += 20.0; // Bonus for versatile hybrid
        }

        if (score > bestScore) {
            bestScore = score;
            bestRobot = r.get();
        }
    }

    return bestRobot;
}

void TaskManager::assignPendingOrders(const std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, Inventory& inventory, const std::string& algorithm, int currentTick, WarehouseAnalytics* analytics, MultiRobotCoordinator* coordinator) {
    if (pendingOrders.empty()) {
        return;
    }

    // Intelligent Multi-Robot Parallel Order Scheduling:
    // Iteratively assigns as many eligible pending orders as possible to available robots / teams
    for (auto it = pendingOrders.begin(); it != pendingOrders.end(); ) {
        auto order = *it;
        if (!order) {
            it = pendingOrders.erase(it);
            continue;
        }

        // Safety check: skip completed or cancelled orders
        if (order->getStatus() == OrderStatus::COMPLETED || order->getStatus() == OrderStatus::CANCELLED) {
            it = pendingOrders.erase(it);
            continue;
        }

        // Check if inventory has stock for this order
        if (!inventory.hasStockForOrder(order->getId(), order->getProductId(), order->getQuantity())) {
            ++it;
            continue;
        }

        const Product* prod = inventory.getProduct(order->getProductId());
        if (!prod) {
            ++it;
            continue;
        }

        Position shelfPos = prod->getLocation();

        if (coordinator) {
            RobotTeam team;
            bool formed = coordinator->formTeamForOrder(*order, robots, warehouse, inventory, team, algorithm);
            if (!formed || team.members.empty()) {
                // No suitable robot team available right now, continue to evaluate remaining pending orders
                ++it;
                continue;
            }

            // Reserve stock
            if (!inventory.reserveStock(order->getId(), order->getProductId(), order->getQuantity())) {
                ++it;
                continue;
            }

            order->setStatus(OrderStatus::ASSIGNED);
            order->setAssignedTick(currentTick);
            order->setAssignedRobotId(team.members[0].robotId);
            order->setPackingDestination(team.members[0].targetPackingStation);
            std::vector<std::string> teamIds;
            for (const auto& m : team.members) teamIds.push_back(m.robotId);
            order->setTeamRobotIds(teamIds);

            // Assign individual tasks and routes to each team member
            for (const auto& m : team.members) {
                for (auto& r : robots) {
                    if (r->getId() == m.robotId) {
                        if (m.role == TeamRole::PICKER || m.role == TeamRole::DIRECT) {
                            Position destPos = (m.targetPickCell != Position(-1, -1)) ? m.targetPickCell : shelfPos;
                            PathResult pPath = PathFinder::findPathToTarget(warehouse, r->getPosition(), destPos, algorithm);
                            Position targetPos = pPath.path.empty() ? destPos : pPath.path.back();
                            auto task = std::make_shared<Task>(order->getId(), TaskType::MOVE_TO_PICK, targetPos, order->getProductId(), m.assignedQuantity);
                            task->setPartnerRobotId(m.partnerRobotId);
                            r->assignTask(task, pPath.path);
                            coordinator->registerPlannedRoute(r->getId(), pPath.path, currentTick);
                        } else if (m.role == TeamRole::TRANSPORTER) {
                            PathResult tPath = PathFinder::findPathToTarget(warehouse, r->getPosition(), m.rendezvousPos, algorithm);
                            Position targetPos = tPath.path.empty() ? m.rendezvousPos : tPath.path.back();
                            auto task = std::make_shared<Task>(order->getId(), TaskType::MOVE_TO_HANDOFF, targetPos, order->getProductId(), m.assignedQuantity);
                            task->setPartnerRobotId(m.partnerRobotId);
                            r->assignTask(task, tPath.path);
                            coordinator->registerPlannedRoute(r->getId(), tPath.path, currentTick);
                        }
                        break;
                    }
                }
            }

            coordinator->registerTeam(team);
            activeOrders.push_back(order);
            it = pendingOrders.erase(it);

            if (analytics) {
                analytics->recordOrderAssigned(*order, order->getAssignedRobotId(), currentTick);
            }
            continue;
        }

        // Fallback if coordinator is null (for standalone TaskManager tests)
        Robot* candidate = selectBestRobot(*order, robots, warehouse, shelfPos);
        if (!candidate) {
            ++it;
            continue;
        }

        PathResult pathRes = PathFinder::findPathToTarget(warehouse, candidate->getPosition(), shelfPos, algorithm);
        if (!pathRes.found || pathRes.path.empty()) {
            ++it;
            continue;
        }

        if (!inventory.reserveStock(order->getId(), order->getProductId(), order->getQuantity())) {
            ++it;
            continue;
        }

        auto task = std::make_shared<Task>(order->getId(), TaskType::MOVE_TO_PICK, shelfPos, order->getProductId(), order->getQuantity());
        candidate->assignTask(task, pathRes.path);

        order->setStatus(OrderStatus::ASSIGNED);
        order->setAssignedRobotId(candidate->getId());
        order->setPackingDestination(warehouse.getNearestPackingStation(shelfPos));
        if (order->getAssignedTick() < 0) {
            order->setAssignedTick(currentTick);
        }
        if (analytics) {
            analytics->recordOrderAssigned(*order, candidate->getId(), currentTick);
        }

        activeOrders.push_back(order);
        it = pendingOrders.erase(it);
    }
}

void TaskManager::updateActiveOrders(const std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, Inventory& inventory, bool advanceMovement, int currentTick, WarehouseAnalytics* analytics, MultiRobotCoordinator* coordinator) {
    (void)inventory;
    for (auto& r : robots) {
        if (r->getStatus() == RobotStatus::CHARGING) {
            r->charge(20.0);
            continue;
        }

        if (r->getStatus() == RobotStatus::LOW_BATTERY && !r->hasActiveRoute()) {
            RobotEnergyProfile profile = RobotEnergyProfile::getProfile(r->getRobotTypeName());
            ChargingRecommendation rec = EcoRoute::selectBestChargingStation(r->getPosition(), r->getBattery(), profile, warehouse, robots);
            if (rec.feasible && !rec.route.empty()) {
                auto chargeTask = std::make_shared<Task>(0, TaskType::RETURN_TO_CHARGE, rec.position);
                r->assignTask(chargeTask, rec.route);
            } else {
                Position cs = warehouse.getNearestChargingStation(r->getPosition());
                PathResult path = PathFinder::findPathAStar(warehouse, r->getPosition(), cs);
                if (path.found) {
                    auto chargeTask = std::make_shared<Task>(0, TaskType::RETURN_TO_CHARGE, cs);
                    r->assignTask(chargeTask, path.path);
                }
            }
            continue;
        }

        if (!r->getCurrentTask()) {
            if (advanceMovement && r->hasActiveRoute()) {
                r->moveNextStep();
            }

            if (!r->hasActiveRoute()) {
                if (r->isStaging()) {
                    r->clearStaging();
                }
                if (r->getStatus() != RobotStatus::CHARGING && r->getStatus() != RobotStatus::LOW_BATTERY) {
                    r->setStatus(RobotStatus::IDLE);
                }
            }

            if (!r->hasActiveRoute() && r->getStatus() == RobotStatus::IDLE &&
                warehouse.getCellType(r->getPosition()) == CellType::PACKING_STATION) {
                Position stagingPos = findStagingCell(r->getPosition(), warehouse, robots);
                if (stagingPos != Position(-1, -1)) {
                    PathResult stagingPath = PathFinder::findPathAStar(warehouse, r->getPosition(), stagingPos);
                    if (stagingPath.found && !stagingPath.path.empty()) {
                        r->setStagingRoute(stagingPos, stagingPath.path);
                    }
                }
            }
            continue;
        }

        auto task = r->getCurrentTask();
        auto order = getOrderById(task->getOrderId());

        switch (task->getType()) {
            case TaskType::MOVE_TO_PICK: {
                if (advanceMovement && r->hasActiveRoute()) {
                    r->moveNextStep();
                }

                if (!r->hasActiveRoute()) {
                    if (r->getStatus() != RobotStatus::PICKING) {
                        if (order) order->setStatus(OrderStatus::PICKING);
                        r->setStatus(RobotStatus::PICKING);
                        r->pickItem(task->getProductId(), task->getQuantity());
                        inventory.recordPhysicalPickup(task->getOrderId(), task->getQuantity());
                        if (order) order->recordPicked(task->getQuantity());
                    } else {
                        // Picking complete!
                        std::string partnerId = task->getPartnerRobotId();
                        Position rendezvousPos = task->getTargetLocation();
                        if (coordinator) {
                            auto team = coordinator->getTeamForOrder(task->getOrderId());
                            if (team) {
                                for (const auto& m : team->members) {
                                    if (m.robotId == r->getId()) {
                                        partnerId = m.partnerRobotId;
                                        rendezvousPos = m.rendezvousPos;
                                        break;
                                    }
                                }
                            }
                        }

                        if (!partnerId.empty()) {
                            PathResult hPath = PathFinder::findPathToTarget(warehouse, r->getPosition(), rendezvousPos, "A*");
                            auto handoffTask = std::make_shared<Task>(task->getOrderId(), TaskType::MOVE_TO_HANDOFF, rendezvousPos, task->getProductId(), task->getQuantity());
                            handoffTask->setPartnerRobotId(partnerId);
                            r->assignTask(handoffTask, hPath.path);
                            if (coordinator) {
                                coordinator->registerPlannedRoute(r->getId(), hPath.path, currentTick);
                            }
                        } else {
                            Position packPos = warehouse.getNearestPackingStation(r->getPosition());
                            PathResult packPath = PathFinder::findPathToTarget(warehouse, r->getPosition(), packPos, "A*");
                            auto deliverTask = std::make_shared<Task>(task->getOrderId(), TaskType::MOVE_TO_PACK, packPos, task->getProductId(), task->getQuantity());
                            r->assignTask(deliverTask, packPath.path);
                            if (coordinator) {
                                coordinator->registerPlannedRoute(r->getId(), packPath.path, currentTick);
                            }
                            if (order) order->setStatus(OrderStatus::DELIVERING);
                        }
                    }
                }
                break;
            }

            case TaskType::MOVE_TO_HANDOFF: {
                if (advanceMovement && r->hasActiveRoute()) {
                    r->moveNextStep();
                }

                std::string partnerId = task->getPartnerRobotId();
                Robot* partner = nullptr;
                for (const auto& other : robots) {
                    if (other->getId() == partnerId) {
                        partner = other.get();
                        break;
                    }
                }

                if (partner) {
                    int dist = r->getPosition().manhattanDistance(partner->getPosition());
                    // When adjacent (dist <= 1), execute handoff
                    if (dist <= 1) {
                        Robot* sender = (r->getCurrentPayload() > 0) ? r.get() : partner;
                        Robot* receiver = (sender == r.get()) ? partner : r.get();

                        if (sender->getCurrentPayload() > 0 && receiver->getCurrentPayload() == 0) {
                            int transferQty = sender->getCurrentPayload();
                            bool accepted = receiver->pickItem(task->getProductId(), transferQty);
                            if (accepted) {
                                sender->deliverItem(); // Releases load
                                sender->clearTask();
                                sender->setStatus(RobotStatus::IDLE);

                                if (coordinator) {
                                    coordinator->getReservationTable().setStationaryRobot(sender->getId(), sender->getPosition());
                                    coordinator->getReservationTable().clearStationaryRobot(receiver->getId());
                                }

                                Position packPos = warehouse.getNearestPackingStation(receiver->getPosition());
                                if (coordinator) {
                                    auto team = coordinator->getTeamForOrder(task->getOrderId());
                                    if (team) {
                                        for (auto& m : team->members) {
                                            if (m.robotId == sender->getId()) {
                                                m.handoffAcknowledged = true;
                                            }
                                            if (m.robotId == receiver->getId()) {
                                                packPos = m.targetPackingStation;
                                            }
                                        }
                                    }
                                }

                                PathResult packPath = PathFinder::findPathToTarget(warehouse, receiver->getPosition(), packPos, "A*");
                                auto deliverTask = std::make_shared<Task>(task->getOrderId(), TaskType::MOVE_TO_PACK, packPos, task->getProductId(), transferQty);
                                receiver->assignTask(deliverTask, packPath.path);
                                if (coordinator) {
                                    coordinator->registerPlannedRoute(receiver->getId(), packPath.path, currentTick);
                                }
                                if (order) order->setStatus(OrderStatus::DELIVERING);
                            }
                        }
                    } else if (!r->hasActiveRoute()) {
                        if (r->getStatus() != RobotStatus::WAITING) {
                            r->setStatus(RobotStatus::WAITING);
                        }
                    }
                }
                break;
            }

            case TaskType::MOVE_TO_PACK: {
                if (advanceMovement && r->hasActiveRoute()) {
                    r->moveNextStep();
                }

                if (!r->hasActiveRoute()) {
                    if (r->getStatus() != RobotStatus::DELIVERING) {
                        if (order) order->setStatus(OrderStatus::DELIVERING);
                        r->setStatus(RobotStatus::DELIVERING);
                        int deliveredNow = (r->getCurrentPayload() > 0) ? r->getCurrentPayload() : task->getQuantity();
                        r->deliverItem();
                        warehouse.recordOrderDelivered(r->getPosition());
                        auto ps = warehouse.getPackingStationAt(r->getPosition());
                        if (ps) {
                            ps->setOccupied(true, r->getId());
                        }
                        if (order) {
                            order->recordDelivered(deliveredNow);
                        }
                    } else {
                        // Check order completion
                        bool isComplete = false;
                        if (order) {
                            if (order->getTeamRobotIds().size() > 1) {
                                isComplete = order->isFullyDelivered();
                            } else {
                                isComplete = true; // Single robot order is complete upon reaching delivery
                            }
                        }

                        if (isComplete) {
                            order->setStatus(OrderStatus::COMPLETED);
                            if (!order->isCompletedRecorded()) {
                                order->setCompletedTick(currentTick);
                                order->markCompletedRecorded();
                                if (analytics) {
                                    const Product* p = inventory.getProduct(order->getProductId());
                                    analytics->recordOrderCompleted(*order, p, currentTick);
                                }
                            }
                            auto it = std::find(activeOrders.begin(), activeOrders.end(), order);
                            if (it != activeOrders.end()) {
                                activeOrders.erase(it);
                                completedOrders.push_back(order);
                            } else {
                                auto pit = std::find(pendingOrders.begin(), pendingOrders.end(), order);
                                if (pit != pendingOrders.end()) {
                                    pendingOrders.erase(pit);
                                    completedOrders.push_back(order);
                                }
                            }
                            if (coordinator) {
                                coordinator->clearTeam(order->getId());
                            }
                        }

                        if (r->getBattery() < 25.0) {
                            RobotEnergyProfile profile = RobotEnergyProfile::getProfile(r->getRobotTypeName());
                            ChargingRecommendation rec = EcoRoute::selectBestChargingStation(r->getPosition(), r->getBattery(), profile, warehouse, robots);
                            if (rec.feasible && !rec.route.empty()) {
                                auto chargeTask = std::make_shared<Task>(0, TaskType::RETURN_TO_CHARGE, rec.position);
                                r->assignTask(chargeTask, rec.route);
                            } else {
                                Position cs = warehouse.getNearestChargingStation(r->getPosition());
                                PathResult chargePath = PathFinder::findPathAStar(warehouse, r->getPosition(), cs);
                                auto chargeTask = std::make_shared<Task>(0, TaskType::RETURN_TO_CHARGE, cs);
                                r->assignTask(chargeTask, chargePath.path);
                            }
                        } else {
                            r->clearTask();
                            if (warehouse.getCellType(r->getPosition()) == CellType::PACKING_STATION) {
                                Position stagingPos = findStagingCell(r->getPosition(), warehouse, robots);
                                if (stagingPos != Position(-1, -1)) {
                                    PathResult stagingPath = PathFinder::findPathAStar(warehouse, r->getPosition(), stagingPos);
                                    if (stagingPath.found && !stagingPath.path.empty()) {
                                        r->setStagingRoute(stagingPos, stagingPath.path);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }

            case TaskType::RETURN_TO_CHARGE: {
                if (advanceMovement && r->hasActiveRoute()) {
                    r->moveNextStep();
                }

                if (!r->hasActiveRoute()) {
                    r->setStatus(RobotStatus::CHARGING);
                    r->clearTask();
                    r->setStatus(RobotStatus::CHARGING);
                }
                break;
            }

            default:
                break;
        }
    }

    // Finalize any active orders that have reached full delivery
    for (auto it = activeOrders.begin(); it != activeOrders.end(); ) {
        auto ord = *it;
        if (ord && ord->isFullyDelivered() && ord->getStatus() != OrderStatus::COMPLETED) {
            ord->setStatus(OrderStatus::COMPLETED);
            if (!ord->isCompletedRecorded()) {
                ord->setCompletedTick(currentTick);
                ord->markCompletedRecorded();
                if (analytics) {
                    const Product* p = inventory.getProduct(ord->getProductId());
                    analytics->recordOrderCompleted(*ord, p, currentTick);
                }
            }
            if (coordinator) {
                coordinator->clearTeam(ord->getId());
            }
            it = activeOrders.erase(it);
            completedOrders.push_back(ord);
        } else {
            ++it;
        }
    }
}

std::string TaskManager::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"pendingOrders\":[";
    for (size_t i = 0; i < pendingOrders.size(); ++i) {
        if (i > 0) ss << ",";
        ss << pendingOrders[i]->toJSON();
    }
    ss << "],\"activeOrders\":[";
    for (size_t i = 0; i < activeOrders.size(); ++i) {
        if (i > 0) ss << ",";
        ss << activeOrders[i]->toJSON();
    }
    ss << "],\"completedOrders\":[";
    for (size_t i = 0; i < completedOrders.size(); ++i) {
        if (i > 0) ss << ",";
        ss << completedOrders[i]->toJSON();
    }
    ss << "]}";
    return ss.str();
}

Position TaskManager::findStagingCell(const Position& currentPos, const Warehouse& warehouse, const std::vector<std::unique_ptr<Robot>>& robots) const {
    std::queue<Position> q;
    std::unordered_set<Position> visited;

    q.push(currentPos);
    visited.insert(currentPos);

    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    while (!q.empty()) {
        Position curr = q.front();
        q.pop();

        if (curr != currentPos) {
            if (warehouse.isValidPosition(curr) && warehouse.isWalkable(curr) && warehouse.getCellType(curr) == CellType::EMPTY) {
                // Ensure staging cell does not choke packing stations (keep at least 2 cells away from packing stations)
                bool nearStation = false;
                for (const auto& ps : warehouse.getPackingStations()) {
                    if (curr.manhattanDistance(ps.getPosition()) < 2) {
                        nearStation = true;
                        break;
                    }
                }

                // Check if any robot is currently located at curr or targeting it for staging
                bool robotOccupied = false;
                for (const auto& r : robots) {
                    if (r->getPosition() == curr || r->getStagingDestination() == curr) {
                        robotOccupied = true;
                        break;
                    }
                }

                // Check if any robot is currently en route to curr as its final destination
                bool routeTargeted = false;
                if (!robotOccupied) {
                    for (const auto& r : robots) {
                        if (r->hasActiveRoute() && !r->getCurrentRoute().empty()) {
                            if (r->getCurrentRoute().back() == curr) {
                                routeTargeted = true;
                                break;
                            }
                        }
                    }
                }

                if (!nearStation && !robotOccupied && !routeTargeted) {
                    PathResult testPath = PathFinder::findPathAStar(warehouse, currentPos, curr);
                    if (testPath.found && !testPath.path.empty()) {
                        return curr;
                    }
                }
            }
        }

        for (int i = 0; i < 4; ++i) {
            Position next(curr.x + dx[i], curr.y + dy[i]);
            if (warehouse.isValidPosition(next) && warehouse.isWalkable(next)) {
                if (visited.find(next) == visited.end()) {
                    visited.insert(next);
                    q.push(next);
                }
            }
        }
    }

    return Position(-1, -1);
}

bool TaskManager::cancelOrder(int orderId, Inventory& inventory) {
    auto order = getOrderById(orderId);
    if (!order) return false;
    if (order->getStatus() == OrderStatus::COMPLETED || order->getStatus() == OrderStatus::CANCELLED) {
        return false;
    }
    order->setStatus(OrderStatus::CANCELLED);
    inventory.releaseReservation(orderId);

    // If order was in pendingOrders, remove it
    for (auto it = pendingOrders.begin(); it != pendingOrders.end(); ++it) {
        if ((*it)->getId() == orderId) {
            pendingOrders.erase(it);
            break;
        }
    }
    // If order was in activeOrders, remove it
    for (auto it = activeOrders.begin(); it != activeOrders.end(); ++it) {
        if ((*it)->getId() == orderId) {
            activeOrders.erase(it);
            break;
        }
    }
    return true;
}

bool TaskManager::failOrder(int orderId, Inventory& inventory, const std::string& reason) {
    (void)reason;
    return cancelOrder(orderId, inventory);
}

bool TaskManager::reassignOrder(int orderId, const std::string& newRobotId, const std::vector<std::unique_ptr<Robot>>& robots, const Warehouse& warehouse, Inventory& inventory) {
    (void)inventory;
    auto order = getOrderById(orderId);
    if (!order) return false;

    // Cannot reassign completed or cancelled orders
    if (order->getStatus() == OrderStatus::COMPLETED || order->getStatus() == OrderStatus::CANCELLED) {
        return false;
    }

    // Identify old robot and ensure physical pickup has not already occurred
    std::string oldRobotId = order->getAssignedRobotId();
    Robot* oldRobot = nullptr;
    for (const auto& r : robots) {
        if (r->getId() == oldRobotId) {
            oldRobot = r.get();
            break;
        }
    }
    if (oldRobot && oldRobot->getCurrentPayload() > 0) {
        // Physical items already picked by old robot; cannot reassign without explicit return
        return false;
    }

    // Cannot reassign to the robot that is already assigned to this order
    if (!oldRobotId.empty() && oldRobotId == newRobotId) {
        return false;
    }

    // Locate target robot and verify eligibility
    Robot* targetRobot = nullptr;
    for (const auto& r : robots) {
        if (r->getId() == newRobotId) {
            targetRobot = r.get();
            break;
        }
    }
    if (!targetRobot || targetRobot->getStatus() != RobotStatus::IDLE || targetRobot->hasActiveRoute()) {
        return false;
    }

    // Verify target robot payload capacity and battery
    if (targetRobot->getPayloadCapacity() < order->getQuantity() || targetRobot->getBattery() < 20.0) {
        return false;
    }

    // Locate shelf destination
    const Product* prod = inventory.getProduct(order->getProductId());
    if (!prod) return false;
    Position shelfPos = prod->getLocation();

    // Verify route feasibility BEFORE mutating any state
    PathResult pathRes = PathFinder::findPathToTarget(warehouse, targetRobot->getPosition(), shelfPos, "A*");
    if (!pathRes.found || pathRes.path.empty()) {
        return false; // Route planning failed; keep existing assignment intact
    }

    // All pre-checks passed: atomically execute reassignment
    // 1. Release old robot task and route if assigned
    if (oldRobot && oldRobot->getCurrentTask() && oldRobot->getCurrentTask()->getOrderId() == orderId) {
        oldRobot->clearTask();
    }
    for (const auto& r : robots) {
        if (r->getId() != newRobotId && r->getCurrentTask() && r->getCurrentTask()->getOrderId() == orderId) {
            r->clearTask();
        }
    }

    // 2. Assign task and planned route to target robot
    auto task = std::make_shared<Task>(order->getId(), TaskType::MOVE_TO_PICK, shelfPos, order->getProductId(), order->getQuantity());
    targetRobot->assignTask(task, pathRes.path);

    // 3. Update order assignment
    order->setAssignedRobotId(newRobotId);
    order->setTeamRobotIds({newRobotId});
    order->setStatus(OrderStatus::ASSIGNED);

    return true;
}
