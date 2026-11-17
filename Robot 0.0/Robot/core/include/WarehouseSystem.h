#ifndef WAREHOUSE_SYSTEM_H
#define WAREHOUSE_SYSTEM_H

#include <vector>
#include <memory>
#include <string>
#include "Warehouse.h"
#include "Inventory.h"
#include "Robot.h"
#include "TaskManager.h"
#include "MultiRobotCoordinator.h"
#include "WarehouseAnalytics.h"
#include "AlgorithmLab.h"
#include "FoodSaver.h"

class WarehouseSystem {
private:
    Warehouse warehouse;
    Inventory inventory;
    std::vector<std::unique_ptr<Robot>> robots;
    TaskManager taskManager;
    MultiRobotCoordinator coordinator;
    WarehouseAnalytics analytics;
    FoodSaverManager foodSaver;
    int simulationTick;
    std::string activeAlgorithm;

public:
    WarehouseSystem(int width = 15, int height = 15);

    void initializeDefaultWarehouse();

    void addRobot(std::unique_ptr<Robot> robot);
    int createOrder(int productId, int quantity, OrderPriority priority);
    bool cancelOrder(int orderId);
    bool failOrder(int orderId, const std::string& reason = "");
    bool reassignOrder(int orderId, const std::string& newRobotId);

    void stepSimulation();
    void setPathAlgorithm(const std::string& algo);
    std::string getPathAlgorithm() const;

    // Dynamic Obstacle Coordination
    bool addDynamicObstacle(const Position& pos);
    bool removeDynamicObstacle(const Position& pos);

    Warehouse& getWarehouse();
    const Warehouse& getWarehouse() const;
    Inventory& getInventory();
    const Inventory& getInventory() const;
    const std::vector<std::unique_ptr<Robot>>& getRobots() const;
    TaskManager& getTaskManager();
    const TaskManager& getTaskManager() const;
    MultiRobotCoordinator& getCoordinator();
    const MultiRobotCoordinator& getCoordinator() const;
    WarehouseAnalytics& getAnalytics();
    const WarehouseAnalytics& getAnalytics() const;
    FoodSaverManager& getFoodSaver();
    const FoodSaverManager& getFoodSaver() const;
    int getSimulationTick() const;

    // Reset mechanisms
    void resetSimulation();
    void resetHeatmap();
    void resetAnalytics();

    std::string comparePathAlgorithmsJSON(const Position& start, const Position& goal) const;

    // DSA Algorithm Laboratory
    std::string runAlgorithmLabJSON(const std::string& algorithm, const Position& start, const Position& goal,
                                   const std::vector<Position>& customObstacles = {},
                                   const std::vector<Position>& removedObstacles = {},
                                   const std::vector<std::pair<Position, double>>& customWeights = {}) const;

    std::string compareAlgorithmsLabJSON(const Position& start, const Position& goal,
                                        const std::vector<Position>& customObstacles = {},
                                        const std::vector<Position>& removedObstacles = {},
                                        const std::vector<std::pair<Position, double>>& customWeights = {}) const;

    std::string generateAlgorithmReport(const Position& start, const Position& goal,
                                       const std::string& format = "json",
                                       const std::vector<Position>& customObstacles = {},
                                       const std::vector<Position>& removedObstacles = {},
                                       const std::vector<std::pair<Position, double>>& customWeights = {}) const;

    // EcoRoute: Energy-Aware Optimization & Analytics
    std::string optimizeEcoRouteJSON(const Position& start, const Position& goal,
                                     const std::string& robotType = "PickerRobot",
                                     int payload = 0) const;

    std::string compareEcoRoutesJSON(const Position& start, const Position& goal,
                                     const std::string& robotType = "PickerRobot",
                                     int payload = 0) const;

    std::string getEcoAnalyticsJSON() const;

    std::string getEcoRobotsJSON() const;

    std::string recommendChargingStationJSON(const std::string& robotId = "",
                                             const Position& customPos = Position(-1, -1)) const;

    std::string generateEcoReport(const Position& start, const Position& goal,
                                  const std::string& robotType = "PickerRobot",
                                  int payload = 0,
                                  const std::string& format = "json") const;

    // FoodSaver: Near-Expiry Inventory & Donation Optimization
    std::string getFoodBatchesJSON() const;
    std::string getNearExpiryJSON() const;
    std::string getDonationEligibilityJSON() const;
    std::string getDonationsJSON() const;
    std::string getFoodSaverAnalyticsJSON() const;
    std::string generateFoodSaverReport(const std::string& format = "json") const;

    std::string runBenchmarkSuiteJSON() const;
    std::string runCustomBenchmarkJSON(const std::string& scenario, int repetitions, const std::vector<std::string>& algorithms) const;
    std::string generateCSVReport() const;
    std::string generateHTMLReport() const;
    std::string getFullStateJSON() const;

    bool saveToFile(const std::string& filename) const;
    bool saveToFile(const std::string& filename, std::string& errorMessage) const;
    bool loadFromFile(const std::string& filename, std::string& errorMessage);
    bool loadFromFile(const std::string& filename);

    bool loadConfiguration(const std::string& layoutPath,
                           const std::string& productsPath,
                           const std::string& robotsPath,
                           std::string& errorMessage);
    bool loadConfiguration(const std::string& layoutPath,
                           const std::string& productsPath,
                           const std::string& robotsPath);
};

#endif // WAREHOUSE_SYSTEM_H
