#include "../include/MultiRobotCoordinator.h"
#include "../include/WarehouseAnalytics.h"
#include "../include/Inventory.h"
#include "../include/PathFinder.h"
#include <algorithm>
#include <sstream>
#include <iostream>

MultiRobotCoordinator::MultiRobotCoordinator(int deadlockThreshold)
    : collisionsPreventedCount(0), deadlocksResolvedCount(0),
      routesReplannedCount(0), deadlockWaitThreshold(deadlockThreshold) {}

bool MultiRobotCoordinator::isRobotInActiveTeam(const std::string& robotId) const {
    for (const auto& pair : activeTeams) {
        for (const auto& member : pair.second.members) {
            if (member.robotId == robotId) return true;
        }
    }
    return false;
}

void MultiRobotCoordinator::registerTeam(const RobotTeam& team) {
    activeTeams[team.orderId] = team;
}

void MultiRobotCoordinator::clearTeam(int orderId) {
    activeTeams.erase(orderId);
}

RobotTeam* MultiRobotCoordinator::getTeamForOrder(int orderId) {
    auto it = activeTeams.find(orderId);
    if (it != activeTeams.end()) return &it->second;
    return nullptr;
}

const RobotTeam* MultiRobotCoordinator::getTeamForOrder(int orderId) const {
    auto it = activeTeams.find(orderId);
    if (it != activeTeams.end()) return &it->second;
    return nullptr;
}

bool MultiRobotCoordinator::hasActiveTeam() const {
    return !activeTeams.empty();
}

std::vector<std::string> MultiRobotCoordinator::getTeamRobotIds(int orderId) const {
    std::vector<std::string> ids;
    auto it = activeTeams.find(orderId);
    if (it != activeTeams.end()) {
        for (const auto& m : it->second.members) {
            ids.push_back(m.robotId);
        }
    }
    return ids;
}

bool MultiRobotCoordinator::formTeamForOrder(
    const Order& order,
    const std::vector<std::unique_ptr<Robot>>& robots,
    const Warehouse& warehouse,
    const Inventory& inventory,
    RobotTeam& outTeam,
    const std::string& algorithm) {

    const Product* prod = inventory.getProduct(order.getProductId());
    if (!prod) return false;
    Position shelfPos = prod->getLocation();

    std::vector<Robot*> pickers;
    std::vector<Robot*> transporters;
    std::vector<Robot*> others;

    for (const auto& r : robots) {
        if (r->getStatus() != RobotStatus::IDLE || r->hasActiveRoute() || r->getBattery() < 20.0 || r->getCurrentTask() != nullptr || r->isStaging()) {
            continue;
        }
        if (isRobotInActiveTeam(r->getId())) {
            continue;
        }

        // Battery feasibility check: verify candidate robot can complete full round trip safely
        int d1 = r->getPosition().manhattanDistance(shelfPos);
        Position packPos = warehouse.getNearestPackingStation(shelfPos);
        int d2 = shelfPos.manhattanDistance(packPos);
        Position chargePos = warehouse.getNearestChargingStation(packPos);
        int d3 = packPos.manhattanDistance(chargePos);
        int totalDist = d1 + d2 + d3;
        double reqBattery = static_cast<double>(totalDist) * r->getBatteryDrainPerMove() + 10.0;
        if (r->getBattery() < reqBattery) {
            continue;
        }

        std::string type = r->getRobotTypeName();
        if (type == "PickerRobot") {
            pickers.push_back(r.get());
        } else if (type == "TransportRobot" || type == "HybridRobot") {
            transporters.push_back(r.get());
        } else {
            others.push_back(r.get());
        }
    }

    // Sort pickers by distance to shelf (nearest first)
    std::sort(pickers.begin(), pickers.end(), [&](Robot* a, Robot* b) {
        return a->getPosition().manhattanDistance(shelfPos) < b->getPosition().manhattanDistance(shelfPos);
    });

    // Sort transporters by capacity descending (TransportRobot R03 before HybridRobot R04)
    std::sort(transporters.begin(), transporters.end(), [&](Robot* a, Robot* b) {
        return a->getPayloadCapacity() > b->getPayloadCapacity();
    });

    outTeam.orderId = order.getId();
    outTeam.productId = order.getProductId();
    outTeam.totalQuantity = order.getQuantity();
    outTeam.members.clear();
    outTeam.active = true;

    int totalQty = order.getQuantity();

    const auto& pks = warehouse.getPackingStations();
    Position pk01 = pks.empty() ? warehouse.getNearestPackingStation(shelfPos) : pks[0].getPosition();
    Position pk02 = pks.size() > 1 ? pks[1].getPosition() : pk01;

    // Distribute single deliveries across packing stations based on active assignments to avoid bottlenecks
    Position targetPack = pk01;
    if (pks.size() > 1) {
        int count0 = 0, count1 = 0;
        for (const auto& pair : activeTeams) {
            for (const auto& m : pair.second.members) {
                if (m.targetPackingStation == pk01) count0++;
                else if (m.targetPackingStation == pk02) count1++;
            }
        }
        if (count1 < count0) {
            targetPack = pk02;
        } else if (count0 < count1) {
            targetPack = pk01;
        } else {
            targetPack = (shelfPos.manhattanDistance(pk01) <= shelfPos.manhattanDistance(pk02)) ? pk01 : pk02;
        }
    }

    // Case 1: Cooperative Multi-Robot Team (Pickers + Transporters available)
    if (!pickers.empty() && !transporters.empty()) {
        if (totalQty >= 4 && pickers.size() >= 2 && transporters.size() >= 2) {
            int q1 = (totalQty + 1) / 2;
            int q2 = totalQty - q1;

            PathResult pathP1 = PathFinder::findPathToTarget(warehouse, pickers[0]->getPosition(), shelfPos, algorithm);
            PathResult pathP2 = PathFinder::findPathToTarget(warehouse, pickers[1]->getPosition(), shelfPos, algorithm);
            if (!pathP1.found || !pathP2.found) return false;

            Position pickCell1 = pathP1.path.back();
            Position pickCell2 = pathP2.path.back();

            if (pickCell1 == pickCell2) {
                static const int dx[] = {0, 1, 0, -1};
                static const int dy[] = {-1, 0, 1, 0};
                for (int i = 0; i < 4; ++i) {
                    Position adj(shelfPos.x + dx[i], shelfPos.y + dy[i]);
                    if (warehouse.isWalkable(adj) && adj != pickCell1) {
                        PathResult alt = PathFinder::findPathToTarget(warehouse, pickers[1]->getPosition(), adj, algorithm);
                        if (alt.found && !alt.path.empty()) {
                            pickCell2 = alt.path.back();
                            pathP2 = alt;
                            break;
                        }
                    }
                }
            }

            // Both Transporter and Hybrid participate!
            PathResult packRoute1 = PathFinder::findPathToTarget(warehouse, pickCell1, pk01, algorithm);
            Position rdv1 = pickCell1;
            if (packRoute1.found && packRoute1.path.size() >= 2) {
                rdv1 = packRoute1.path[std::min<size_t>(2, packRoute1.path.size() - 1)];
            }

            PathResult packRoute2 = PathFinder::findPathToTarget(warehouse, pickCell2, pk02, algorithm);
            Position rdv2 = pickCell2;
            if (packRoute2.found && packRoute2.path.size() >= 2) {
                rdv2 = packRoute2.path[std::min<size_t>(2, packRoute2.path.size() - 1)];
            }

            TeamMemberInfo mP1{pickers[0]->getId(), TeamRole::PICKER, transporters[0]->getId(), q1, rdv1, pk01, false, pickCell1};
            TeamMemberInfo mT1{transporters[0]->getId(), TeamRole::TRANSPORTER, pickers[0]->getId(), q1, rdv1, pk01, false, Position(-1, -1)};

            TeamMemberInfo mP2{pickers[1]->getId(), TeamRole::PICKER, transporters[1]->getId(), q2, rdv2, pk02, false, pickCell2};
            TeamMemberInfo mT2{transporters[1]->getId(), TeamRole::TRANSPORTER, pickers[1]->getId(), q2, rdv2, pk02, false, Position(-1, -1)};

            outTeam.members.push_back(mP1);
            outTeam.members.push_back(mT1);
            outTeam.members.push_back(mP2);
            outTeam.members.push_back(mT2);
            return true;
        } else if (pickers[0]->getPayloadCapacity() >= totalQty) {
            // Direct delivery for single-robot orders when picker has sufficient capacity
            TeamMemberInfo mP{pickers[0]->getId(), TeamRole::DIRECT, "", totalQty, Position(-1, -1), targetPack, false};
            outTeam.members.push_back(mP);
            return true;
        } else {
            // 1 picker + 1 transporter
            PathResult pathP = PathFinder::findPathToTarget(warehouse, pickers[0]->getPosition(), shelfPos, algorithm);
            if (!pathP.found) return false;
            Position pickCell = pathP.path.back();

            PathResult packRoute = PathFinder::findPathToTarget(warehouse, pickCell, targetPack, algorithm);
            Position rdv = pickCell;
            if (packRoute.found && packRoute.path.size() >= 2) {
                rdv = packRoute.path[std::min<size_t>(2, packRoute.path.size() - 1)];
            }

            TeamMemberInfo mP{pickers[0]->getId(), TeamRole::PICKER, transporters[0]->getId(), totalQty, rdv, targetPack, false};
            TeamMemberInfo mT{transporters[0]->getId(), TeamRole::TRANSPORTER, pickers[0]->getId(), totalQty, rdv, targetPack, false};

            outTeam.members.push_back(mP);
            outTeam.members.push_back(mT);
            return true;
        }
    }

    // Case 2: Only Pickers available (Direct Transportation)
    if (!pickers.empty() && transporters.empty()) {
        if (pickers[0]->getPayloadCapacity() >= totalQty) {
            TeamMemberInfo mP{pickers[0]->getId(), TeamRole::DIRECT, "", totalQty, Position(-1, -1), targetPack, false};
            outTeam.members.push_back(mP);
            return true;
        } else if (totalQty >= 2 && pickers.size() >= 2) {
            int q1 = (totalQty + 1) / 2;
            int q2 = totalQty - q1;
            TeamMemberInfo mP1{pickers[0]->getId(), TeamRole::DIRECT, "", q1, Position(-1, -1), pk01, false};
            TeamMemberInfo mP2{pickers[1]->getId(), TeamRole::DIRECT, "", q2, Position(-1, -1), pk02, false};
            outTeam.members.push_back(mP1);
            outTeam.members.push_back(mP2);
            return true;
        } else {
            TeamMemberInfo mP{pickers[0]->getId(), TeamRole::DIRECT, "", totalQty, Position(-1, -1), targetPack, false};
            outTeam.members.push_back(mP);
            return true;
        }
    }

    // Case 3: Only Transporters / Hybrids available (Direct Transportation)
    if (pickers.empty() && !transporters.empty()) {
        if (transporters[0]->getPayloadCapacity() >= totalQty) {
            TeamMemberInfo mT{transporters[0]->getId(), TeamRole::DIRECT, "", totalQty, Position(-1, -1), targetPack, false};
            outTeam.members.push_back(mT);
            return true;
        }
        return false;
    }

    // Case 4: Any other available robot
    if (!others.empty()) {
        if (others[0]->getPayloadCapacity() >= totalQty) {
            TeamMemberInfo mO{others[0]->getId(), TeamRole::DIRECT, "", totalQty, Position(-1, -1), targetPack, false};
            outTeam.members.push_back(mO);
            return true;
        }
        return false;
    }

    return false;
}

double MultiRobotCoordinator::calculateRobotPriority(Robot* robot, TaskManager& taskManager) {
    if (!robot) return 0.0;
    double score = 0.0;

    if (robot->getCurrentTask()) {
        auto order = taskManager.getOrderById(robot->getCurrentTask()->getOrderId());
        if (order) {
            switch (order->getPriority()) {
                case OrderPriority::HIGH: score += 300.0; break;
                case OrderPriority::MEDIUM: score += 200.0; break;
                case OrderPriority::LOW: score += 100.0; break;
            }
        }
    }

    // Prioritize robots waiting longer to prevent starvation & break deadlock
    score += static_cast<double>(robot->getWaitTicks() * 50.0);

    // Robots carrying payload have higher priority to clear paths
    if (robot->getCurrentPayload() > 0) {
        score += 80.0;
    }

    // Deterministic tie-breaker: give earlier IDs (R01 > R02) consistent tie-break bonus
    if (!robot->getId().empty()) {
        score += (100.0 - static_cast<double>(robot->getId()[robot->getId().size() - 1]));
    }

    return score;
}

void MultiRobotCoordinator::registerPlannedRoute(const std::string& robotId, const std::vector<Position>& route, int currentTick) {
    reservationTable.clearStationaryRobot(robotId);
    reservationTable.releaseFutureReservations(robotId, currentTick);

    for (size_t i = 0; i < route.size(); ++i) {
        int tick = currentTick + static_cast<int>(i);
        reservationTable.reserveVertex(route[i], tick, robotId);
        if (i > 0 && route[i] != route[i - 1]) {
            reservationTable.reserveEdge(route[i - 1], route[i], tick - 1, robotId);
        }
    }
}

void MultiRobotCoordinator::releaseRobotReservations(const std::string& robotId, int fromTick) {
    reservationTable.releaseFutureReservations(robotId, fromTick);
}

ReservationTable& MultiRobotCoordinator::getReservationTable() {
    return reservationTable;
}

const ReservationTable& MultiRobotCoordinator::getReservationTable() const {
    return reservationTable;
}

void MultiRobotCoordinator::coordinateMovement(std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, TaskManager& taskManager, int currentTick, WarehouseAnalytics* analytics) {
    // Synchronize stationary robots and reserve immediate ticks
    for (const auto& r : robots) {
        if (!r->hasActiveRoute()) {
            reservationTable.setStationaryRobot(r->getId(), r->getPosition());
            reservationTable.reserveVertex(r->getPosition(), currentTick, r->getId());
            reservationTable.reserveVertex(r->getPosition(), currentTick + 1, r->getId());
        } else {
            reservationTable.clearStationaryRobot(r->getId());
        }
    }

    // Structure to hold proposed movement for each robot
    struct ProposedMove {
        Robot* robot;
        Position from;
        Position to;
        bool isMovingAction;
        bool approved;
        double priorityScore;
    };

    std::vector<ProposedMove> proposals;
    proposals.reserve(robots.size());

    // Step 1: Collect proposed movements
    for (auto& r : robots) {
        ProposedMove prop;
        prop.robot = r.get();
        prop.from = r->getPosition();
        prop.to = r->getPosition();
        prop.isMovingAction = false;
        prop.approved = true;
        prop.priorityScore = calculateRobotPriority(r.get(), taskManager);

        bool isActiveTransit = (r->getStatus() == RobotStatus::MOVING_TO_PICK ||
                                r->getStatus() == RobotStatus::MOVING_TO_PACK ||
                                r->getStatus() == RobotStatus::RETURNING_TO_CHARGE ||
                                r->getStatus() == RobotStatus::WAITING ||
                                (r->getCurrentTask() && r->getCurrentTask()->getType() == TaskType::MOVE_TO_HANDOFF) ||
                                (r->getStatus() == RobotStatus::IDLE && r->hasActiveRoute()));

        if (isActiveTransit && r->hasActiveRoute()) {
            Position nextP = r->getNextRoutePosition();
            if (nextP != prop.from) {
                prop.to = nextP;
                prop.isMovingAction = true;
            }
        }

        proposals.push_back(prop);
    }

    // Step 2: Detect & Resolve Conflicts
    // Sort proposals by priority (highest priority resolved first)
    std::sort(proposals.begin(), proposals.end(), [](const ProposedMove& a, const ProposedMove& b) {
        return a.priorityScore > b.priorityScore;
    });

    // Stationary robots initially commit their current positions
    std::vector<Position> committedNextPositions;
    for (const auto& prop : proposals) {
        if (!prop.isMovingAction) {
            committedNextPositions.push_back(prop.from);
        }
    }

    // Iteratively validate each move against other robots' committed destinations
    for (size_t i = 0; i < proposals.size(); ++i) {
        if (!proposals[i].isMovingAction) {
            continue;
        }

        if (!proposals[i].approved) {
            // Already denied (e.g. from mutual swap conflict)
            committedNextPositions.push_back(proposals[i].from);
            continue;
        }

        bool conflictDetected = false;

        // Check A: Vertex Collision with another robot's destination
        for (const auto& committed : committedNextPositions) {
            if (proposals[i].to == committed) {
                conflictDetected = true;
                break;
            }
        }

        // Check B: Head-on Edge Collision (Swap) with any other proposal
        if (!conflictDetected) {
            for (size_t j = 0; j < proposals.size(); ++j) {
                if (i == j || !proposals[j].isMovingAction) continue;
                if (proposals[i].from == proposals[j].to && proposals[i].to == proposals[j].from) {
                    conflictDetected = true;
                    // Both participating robots in edge swap cannot traverse through each other
                    proposals[j].approved = false;
                    proposals[j].to = proposals[j].from;
                    break;
                }
            }
        }

        // Check C: Cell is an obstacle or dynamic obstacle
        if (!conflictDetected && !warehouse.isWalkable(proposals[i].to)) {
            conflictDetected = true;
        }

        if (conflictDetected) {
            proposals[i].approved = false;
            proposals[i].to = proposals[i].from; // Force stay in place
            collisionsPreventedCount++;
            committedNextPositions.push_back(proposals[i].from);

            // Cascade revocation: if a robot cannot vacate proposals[i].from,
            // any robot previously approved to move into proposals[i].from must also be revoked
            std::vector<Position> revokedFromPositions = {proposals[i].from};
            while (!revokedFromPositions.empty()) {
                Position blockedPos = revokedFromPositions.back();
                revokedFromPositions.pop_back();

                for (size_t k = 0; k < proposals.size(); ++k) {
                    if (proposals[k].isMovingAction && proposals[k].approved && proposals[k].to == blockedPos) {
                        proposals[k].approved = false;
                        proposals[k].to = proposals[k].from;
                        collisionsPreventedCount++;
                        committedNextPositions.push_back(proposals[k].from);
                        revokedFromPositions.push_back(proposals[k].from);
                    }
                }
            }
        } else {
            committedNextPositions.push_back(proposals[i].to);
        }
    }

    // Step 3: Apply movements & manage waiting / deadlocks
    for (auto& prop : proposals) {
        Robot* r = prop.robot;

        if (prop.isMovingAction && prop.approved) {
            // Safe to move!
            r->moveNextStep();
            r->resetWaitTicks();

            // If robot was previously in WAITING, restore active transit status
            if (r->getStatus() == RobotStatus::WAITING) {
                if (r->getCurrentTask()) {
                    if (r->getCurrentTask()->getType() == TaskType::MOVE_TO_PICK ||
                        r->getCurrentTask()->getType() == TaskType::MOVE_TO_HANDOFF) {
                        r->setStatus(RobotStatus::MOVING_TO_PICK);
                    } else if (r->getCurrentTask()->getType() == TaskType::MOVE_TO_PACK) {
                        r->setStatus(RobotStatus::MOVING_TO_PACK);
                    } else if (r->getCurrentTask()->getType() == TaskType::RETURN_TO_CHARGE) {
                        r->setStatus(RobotStatus::RETURNING_TO_CHARGE);
                    }
                } else {
                    r->setStatus(RobotStatus::IDLE);
                }
            }
        } else if (prop.isMovingAction && !prop.approved) {
            // Move was denied: Robot must wait in place
            r->incrementWaitTicks();
            if (r->getStatus() != RobotStatus::PICKING && r->getStatus() != RobotStatus::DELIVERING) {
                r->setStatus(RobotStatus::WAITING);
            }

            // Deadlock Check: Has the robot waited too long?
            if (r->getWaitTicks() >= deadlockWaitThreshold) {
                if (r->getCurrentTask()) {
                    deadlocksResolvedCount++;

                    // Trigger intelligent space-time replanning
                    Position dest = r->getCurrentTask()->getTargetLocation();
                    PathResult replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), dest, currentTick, r->getId());

                    if (replan.found && !replan.path.empty()) {
                        routesReplannedCount++;
                        r->setCurrentRoute(replan.path);
                        registerPlannedRoute(r->getId(), replan.path, currentTick);
                        if (analytics) {
                            analytics->recordReplanEvent(r->getPosition());
                        }
                    } else {
                        // Try alternative adjacent detour
                        r->setStatus(RobotStatus::BLOCKED);
                    }
                } else if (r->isStaging()) {
                    // Taskless Staging Robot Recovery
                    deadlocksResolvedCount++;
                    Position stagingDest = r->getStagingDestination();

                    // Check if current staging destination is still valid and reachable
                    bool destStillValid = warehouse.isValidPosition(stagingDest) &&
                                          warehouse.isWalkable(stagingDest) &&
                                          warehouse.getCellType(stagingDest) == CellType::EMPTY;

                    PathResult replan;
                    if (destStillValid) {
                        replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), stagingDest, currentTick, r->getId());
                    }

                    if (replan.found && !replan.path.empty()) {
                        routesReplannedCount++;
                        r->setStagingRoute(stagingDest, replan.path);
                        registerPlannedRoute(r->getId(), replan.path, currentTick);
                        if (analytics) {
                            analytics->recordReplanEvent(r->getPosition());
                        }
                    } else {
                        // Destination blocked or unreachable: select an alternative staging cell!
                        Position altStaging = taskManager.findStagingCell(r->getPosition(), warehouse, robots);
                        if (altStaging != Position(-1, -1)) {
                            PathResult altReplan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), altStaging, currentTick, r->getId());
                            if (altReplan.found && !altReplan.path.empty()) {
                                routesReplannedCount++;
                                r->setStagingRoute(altStaging, altReplan.path);
                                registerPlannedRoute(r->getId(), altReplan.path, currentTick);
                                if (analytics) {
                                    analytics->recordReplanEvent(r->getPosition());
                                }
                            } else {
                                r->setStatus(RobotStatus::BLOCKED);
                            }
                        } else {
                            r->setStatus(RobotStatus::BLOCKED);
                        }
                    }
                }
                r->resetWaitTicks();
            }
        }
    }

    // Step 4: Purge expired reservations older than current tick
    reservationTable.purgePastReservations(currentTick);
}

bool MultiRobotCoordinator::handleDynamicObstacleAdded(const Position& pos, std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, TaskManager& taskManager, int currentTick) {
    if (!warehouse.isValidPosition(pos)) return false;

    // Check if an existing robot is currently in this cell
    for (const auto& r : robots) {
        if (r->getPosition() == pos) {
            return false; // Cannot place obstacle directly on a robot
        }
    }

    // Add obstacle to physical warehouse
    if (!warehouse.addDynamicObstacle(pos)) {
        return false;
    }

    // Replan any active robot whose route passes through the new obstacle
    for (auto& r : robots) {
        if (!r->hasActiveRoute()) continue;

        bool routeAffected = false;
        const auto& route = r->getCurrentRoute();
        for (size_t i = r->getRouteIndex(); i < route.size(); ++i) {
            if (route[i] == pos) {
                routeAffected = true;
                break;
            }
        }

        if (routeAffected) {
            if (r->getCurrentTask()) {
                reservationTable.releaseFutureReservations(r->getId(), currentTick);
                Position target = r->getCurrentTask()->getTargetLocation();
                PathResult replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), target, currentTick, r->getId());

                if (replan.found && !replan.path.empty()) {
                    routesReplannedCount++;
                    r->setCurrentRoute(replan.path);
                    registerPlannedRoute(r->getId(), replan.path, currentTick);
                } else {
                    r->setStatus(RobotStatus::BLOCKED);
                }
            } else if (r->isStaging()) {
                reservationTable.releaseFutureReservations(r->getId(), currentTick);
                Position stagingDest = r->getStagingDestination();
                PathResult replan;
                if (warehouse.isValidPosition(stagingDest) && warehouse.isWalkable(stagingDest) && warehouse.getCellType(stagingDest) == CellType::EMPTY) {
                    replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), stagingDest, currentTick, r->getId());
                }
                if (replan.found && !replan.path.empty()) {
                    routesReplannedCount++;
                    r->setStagingRoute(stagingDest, replan.path);
                    registerPlannedRoute(r->getId(), replan.path, currentTick);
                } else {
                    Position altStaging = taskManager.findStagingCell(r->getPosition(), warehouse, robots);
                    if (altStaging != Position(-1, -1)) {
                        PathResult altReplan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), altStaging, currentTick, r->getId());
                        if (altReplan.found && !altReplan.path.empty()) {
                            routesReplannedCount++;
                            r->setStagingRoute(altStaging, altReplan.path);
                            registerPlannedRoute(r->getId(), altReplan.path, currentTick);
                        } else {
                            r->setStatus(RobotStatus::BLOCKED);
                        }
                    } else {
                        r->setStatus(RobotStatus::BLOCKED);
                    }
                }
            }
        }
    }

    return true;
}

bool MultiRobotCoordinator::handleDynamicObstacleRemoved(const Position& pos, std::vector<std::unique_ptr<Robot>>& robots, Warehouse& warehouse, TaskManager& taskManager, int currentTick) {
    if (!warehouse.removeDynamicObstacle(pos)) {
        return false;
    }

    // Unblock any blocked robots that can now reach their destination
    for (auto& r : robots) {
        if (r->getStatus() == RobotStatus::BLOCKED) {
            if (r->getCurrentTask()) {
                Position target = r->getCurrentTask()->getTargetLocation();
                PathResult replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), target, currentTick, r->getId());
                if (replan.found && !replan.path.empty()) {
                    r->setCurrentRoute(replan.path);
                    registerPlannedRoute(r->getId(), replan.path, currentTick);
                    if (r->getCurrentTask()->getType() == TaskType::MOVE_TO_PICK) {
                        r->setStatus(RobotStatus::MOVING_TO_PICK);
                    } else {
                        r->setStatus(RobotStatus::MOVING_TO_PACK);
                    }
                }
            } else if (r->isStaging()) {
                Position stagingDest = r->getStagingDestination();
                PathResult replan;
                if (warehouse.isValidPosition(stagingDest) && warehouse.isWalkable(stagingDest) && warehouse.getCellType(stagingDest) == CellType::EMPTY) {
                    replan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), stagingDest, currentTick, r->getId());
                }
                if (replan.found && !replan.path.empty()) {
                    r->setStagingRoute(stagingDest, replan.path);
                    registerPlannedRoute(r->getId(), replan.path, currentTick);
                    r->setStatus(RobotStatus::IDLE);
                } else {
                    Position altStaging = taskManager.findStagingCell(r->getPosition(), warehouse, robots);
                    if (altStaging != Position(-1, -1)) {
                        PathResult altReplan = PathFinder::findSpaceTimePathToTarget(warehouse, reservationTable, r->getPosition(), altStaging, currentTick, r->getId());
                        if (altReplan.found && !altReplan.path.empty()) {
                            r->setStagingRoute(altStaging, altReplan.path);
                            registerPlannedRoute(r->getId(), altReplan.path, currentTick);
                            r->setStatus(RobotStatus::IDLE);
                        }
                    }
                }
            }
        }
    }

    return true;
}

int MultiRobotCoordinator::getCollisionsPrevented() const {
    return collisionsPreventedCount;
}

int MultiRobotCoordinator::getDeadlocksResolved() const {
    return deadlocksResolvedCount;
}

int MultiRobotCoordinator::getRoutesReplanned() const {
    return routesReplannedCount;
}

void MultiRobotCoordinator::setMetrics(int collisionsPrevented, int deadlocksResolved, int routesReplanned) {
    collisionsPreventedCount = std::max(0, collisionsPrevented);
    deadlocksResolvedCount = std::max(0, deadlocksResolved);
    routesReplannedCount = std::max(0, routesReplanned);
}

void MultiRobotCoordinator::reset() {
    reservationTable.clearReservations();
    collisionsPreventedCount = 0;
    deadlocksResolvedCount = 0;
    routesReplannedCount = 0;
    activeTeams.clear();
}

std::string MultiRobotCoordinator::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"collisionsPrevented\":" << collisionsPreventedCount << ","
       << "\"deadlocksResolved\":" << deadlocksResolvedCount << ","
       << "\"routesReplanned\":" << routesReplannedCount << ","
       << "\"vertexReservationsCount\":" << reservationTable.getVertexReservationCount() << ","
       << "\"edgeReservationsCount\":" << reservationTable.getEdgeReservationCount()
       << "}";
    return ss.str();
}
