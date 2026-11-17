#ifndef MULTI_ROBOT_COORDINATOR_H
#define MULTI_ROBOT_COORDINATOR_H

#include "ReservationTable.h"
#include "Robot.h"
#include "TaskManager.h"
#include "Warehouse.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class WarehouseAnalytics;
class Inventory;

enum class TeamRole {
  PICKER,
  TRANSPORTER,
  DIRECT
};

struct TeamMemberInfo {
  std::string robotId;
  TeamRole role;
  std::string partnerRobotId;
  int assignedQuantity;
  Position rendezvousPos;
  Position targetPackingStation;
  bool handoffAcknowledged;
  Position targetPickCell;

  TeamMemberInfo(std::string rId = "", TeamRole r = TeamRole::DIRECT, std::string pId = "",
                 int qty = 0, Position rdv = Position(-1, -1), Position pack = Position(-1, -1),
                 bool ack = false, Position pickCell = Position(-1, -1))
      : robotId(std::move(rId)), role(r), partnerRobotId(std::move(pId)),
        assignedQuantity(qty), rendezvousPos(rdv), targetPackingStation(pack),
        handoffAcknowledged(ack), targetPickCell(pickCell) {}
};

struct RobotTeam {
  int orderId;
  int productId;
  int totalQuantity;
  std::vector<TeamMemberInfo> members;
  bool active;
};

/**
 * @brief MultiRobotCoordinator
 * Responsible for multi-robot movement coordination, cooperative team formation,
 * collision prevention (vertex & edge), reservation tracking, and deadlock detection & recovery.
 */
class MultiRobotCoordinator {
private:
  ReservationTable reservationTable;
  int collisionsPreventedCount;
  int deadlocksResolvedCount;
  int routesReplannedCount;
  int deadlockWaitThreshold; // Ticks waiting before triggering replan
  std::map<int, RobotTeam> activeTeams;

  double calculateRobotPriority(Robot *robot, TaskManager &taskManager);

public:
  MultiRobotCoordinator(int deadlockThreshold = 3);

  // Cooperative Team Management
  bool formTeamForOrder(const Order &order,
                        const std::vector<std::unique_ptr<Robot>> &robots,
                        const Warehouse &warehouse,
                        const Inventory &inventory,
                        RobotTeam &outTeam,
                        const std::string &algorithm = "A*");
  void registerTeam(const RobotTeam &team);
  void clearTeam(int orderId);
  RobotTeam *getTeamForOrder(int orderId);
  const RobotTeam *getTeamForOrder(int orderId) const;
  bool hasActiveTeam() const;
  std::vector<std::string> getTeamRobotIds(int orderId) const;
  bool isRobotInActiveTeam(const std::string &robotId) const;

  // Reservation Management
  void registerPlannedRoute(const std::string &robotId,
                            const std::vector<Position> &route,
                            int currentTick);
  void releaseRobotReservations(const std::string &robotId, int fromTick);
  ReservationTable &getReservationTable();
  const ReservationTable &getReservationTable() const;

  // Movement Coordination & Collision Resolution
  void coordinateMovement(std::vector<std::unique_ptr<Robot>> &robots,
                          Warehouse &warehouse, TaskManager &taskManager,
                          int currentTick,
                          WarehouseAnalytics *analytics = nullptr);

  // Dynamic Obstacle Integration
  bool handleDynamicObstacleAdded(const Position &pos,
                                  std::vector<std::unique_ptr<Robot>> &robots,
                                  Warehouse &warehouse,
                                  TaskManager &taskManager, int currentTick);
  bool handleDynamicObstacleRemoved(const Position &pos,
                                    std::vector<std::unique_ptr<Robot>> &robots,
                                    Warehouse &warehouse,
                                    TaskManager &taskManager, int currentTick);

  // Statistics
  int getCollisionsPrevented() const;
  int getDeadlocksResolved() const;
  int getRoutesReplanned() const;
  void setMetrics(int collisionsPrevented, int deadlocksResolved,
                  int routesReplanned);
  void reset();

  std::string toJSON() const;
};

#endif // MULTI_ROBOT_COORDINATOR_H
