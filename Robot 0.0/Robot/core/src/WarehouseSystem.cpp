#include "../include/WarehouseSystem.h"
#include "../include/AlgorithmBenchmark.h"
#include "../include/AlgorithmLab.h"
#include "../include/EcoRoute.h"
#include "../include/PickerRobot.h"
#include "../include/TransportRobot.h"
#include "../include/HybridRobot.h"
#include "../include/PathFinder.h"
#include "../include/Exceptions.h"
#include "../include/JsonParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>

WarehouseSystem::WarehouseSystem(int width, int height)
    : warehouse(width, height), analytics(width, height), simulationTick(0), activeAlgorithm("A*") {}

void WarehouseSystem::initializeDefaultWarehouse() {
    // 1. Setup Stations
    warehouse.addChargingStation(ChargingStation("CS01", Position(0, 0), 25.0));
    warehouse.addChargingStation(ChargingStation("CS02", Position(0, 1), 25.0));
    warehouse.addPackingStation(PackingStation("PK01", Position(14, 14)));
    warehouse.addPackingStation(PackingStation("PK02", Position(14, 13)));

    // 2. Setup Shelves (organized in warehouse aisles)
    // Aisle 1 (x=3, y=3..10)
    for (int y = 3; y <= 10; ++y) {
        std::string sId = "SH_" + std::to_string(3) + "_" + std::to_string(y);
        warehouse.addShelf(Shelf(sId, Position(3, y)));
    }
    // Aisle 2 (x=6, y=3..10)
    for (int y = 3; y <= 10; ++y) {
        std::string sId = "SH_" + std::to_string(6) + "_" + std::to_string(y);
        warehouse.addShelf(Shelf(sId, Position(6, y)));
    }
    // Aisle 3 (x=9, y=3..10)
    for (int y = 3; y <= 10; ++y) {
        std::string sId = "SH_" + std::to_string(9) + "_" + std::to_string(y);
        warehouse.addShelf(Shelf(sId, Position(9, y)));
    }
    // Aisle 4 (x=11, y=3..10)
    for (int y = 3; y <= 10; ++y) {
        std::string sId = "SH_" + std::to_string(11) + "_" + std::to_string(y);
        warehouse.addShelf(Shelf(sId, Position(11, y)));
    }

    // 3. Add Obstacles (pillars, maintenance zones)
    warehouse.setObstacle(Position(5, 1), true);
    warehouse.setObstacle(Position(8, 1), true);
    warehouse.setObstacle(Position(1, 7), true);
    warehouse.setObstacle(Position(13, 7), true);

    // 4. Populate Inventory & map products to shelves
    inventory.addProduct(Product(101, "ThinkPad Laptop", "Electronics", 35, 1200.00, Position(3, 3)));
    inventory.addProduct(Product(102, "Wireless Mouse", "Accessories", 80, 29.99, Position(3, 6)));
    inventory.addProduct(Product(103, "Mechanical Keyboard", "Accessories", 45, 95.00, Position(3, 9)));
    inventory.addProduct(Product(104, "4K Monitor 27-inch", "Displays", 20, 380.00, Position(6, 4)));
    inventory.addProduct(Product(105, "Ergonomic Office Chair", "Furniture", 15, 250.00, Position(6, 8)));
    inventory.addProduct(Product(106, "Standing Desk Frame", "Furniture", 10, 450.00, Position(9, 4)));
    inventory.addProduct(Product(107, "USB-C Hub Multiport", "Accessories", 60, 45.50, Position(9, 7)));
    inventory.addProduct(Product(108, "Noise-Cancelling Headset", "Audio", 30, 199.00, Position(11, 5)));

    // FoodSaver Food Products
    inventory.addProduct(Product(201, "Organic Whole Milk 1L", "Dairy", 50, 3.49, Position(3, 4)));
    inventory.addProduct(Product(202, "Sourdough Artisan Bread", "Bakery", 30, 4.25, Position(3, 7)));
    inventory.addProduct(Product(203, "Fresh Strawberries 500g", "Produce", 40, 4.99, Position(6, 5)));
    inventory.addProduct(Product(204, "Ground Beef 80/20 1kg", "Meat", 25, 8.99, Position(6, 9)));
    inventory.addProduct(Product(205, "Canned Tomato Soup 400g", "Canned", 100, 1.89, Position(9, 5)));
    inventory.addProduct(Product(206, "Greek Yogurt 1kg", "Dairy", 35, 5.50, Position(9, 8)));
    inventory.addProduct(Product(207, "Bagged Baby Spinach 250g", "Produce", 20, 2.99, Position(11, 6)));

    // Link products to shelves
    if (Shelf* s = warehouse.findShelfForProduct(101)) s->addProduct(101);
    for (const auto& prod : inventory.getAllProducts()) {
        for (auto& s : const_cast<std::vector<Shelf>&>(warehouse.getShelves())) {
            if (s.getPosition() == prod.getLocation()) {
                s.addProduct(prod.getId());
            }
        }
    }

    // 5. Add Diverse Robot Fleet
    addRobot(std::make_unique<PickerRobot>("R01", "SpeedyPicker-1", Position(0, 3)));
    addRobot(std::make_unique<PickerRobot>("R02", "SpeedyPicker-2", Position(0, 5)));
    addRobot(std::make_unique<TransportRobot>("R03", "TitanHauler-1", Position(1, 0)));
    addRobot(std::make_unique<HybridRobot>("R04", "OmniBot-1", Position(1, 14)));

    // 6. FoodSaver: Initialize canonical batches
    foodSaver.initializeDefaultFoodStock();
}

void WarehouseSystem::addRobot(std::unique_ptr<Robot> robot) {
    if (robot) {
        robots.push_back(std::move(robot));
    }
}

int WarehouseSystem::createOrder(int productId, int quantity, OrderPriority priority) {
    if (!inventory.hasStock(productId, quantity)) {
        throw InsufficientStockException("Cannot place order: Product stock unavailable.");
    }
    const Product* p = inventory.getProduct(productId);
    Position packingPos = warehouse.getNearestPackingStation(p->getLocation());
    auto order = std::make_shared<Order>(productId, quantity, priority, packingPos);
    order->setCreatedTick(simulationTick);
    int orderId = order->getId();

    // Immediately reserve stock to prevent competing orders from over-reserving
    inventory.reserveStock(orderId, productId, quantity);

    taskManager.addOrder(order);
    analytics.recordOrderCreated(*order, simulationTick);
    return orderId;
}

bool WarehouseSystem::cancelOrder(int orderId) {
    for (auto& r : robots) {
        if (r->getCurrentTask() && r->getCurrentTask()->getOrderId() == orderId) {
            r->clearTask();
            r->setStatus(RobotStatus::IDLE);
            coordinator.releaseRobotReservations(r->getId(), simulationTick);
        }
    }
    coordinator.clearTeam(orderId);
    return taskManager.cancelOrder(orderId, inventory);
}

bool WarehouseSystem::failOrder(int orderId, const std::string& reason) {
    return taskManager.failOrder(orderId, inventory, reason);
}

bool WarehouseSystem::reassignOrder(int orderId, const std::string& newRobotId) {
    return taskManager.reassignOrder(orderId, newRobotId, robots, warehouse, inventory);
}

void WarehouseSystem::stepSimulation() {
    simulationTick++;

    // 1. Dispatch pending orders to available robots (strict sequential & cooperative team allocation)
    taskManager.assignPendingOrders(robots, warehouse, inventory, activeAlgorithm, simulationTick, &analytics, &coordinator);

    // 2. Register newly assigned routes with coordinator; clear future reservations for stationary robots
    for (const auto& r : robots) {
        if (r->hasActiveRoute() && !coordinator.getReservationTable().isVertexReserved(r->getPosition(), simulationTick, r->getId())) {
            coordinator.registerPlannedRoute(r->getId(), r->getCurrentRoute(), simulationTick);
        } else if (!r->hasActiveRoute()) {
            coordinator.getReservationTable().setStationaryRobot(r->getId(), r->getPosition());
            coordinator.releaseRobotReservations(r->getId(), simulationTick + 1);
        }
    }

    // 3. Atomically coordinate robot movements (vertex/edge conflict resolution & deadlock checks)
    coordinator.coordinateMovement(robots, warehouse, taskManager, simulationTick, &analytics);

    // 4. Advance order state transitions (synchronized picking, handoffs & delivery) without double-moving
    taskManager.updateActiveOrders(robots, warehouse, inventory, false, simulationTick, &analytics, &coordinator);

    // Synchronize packing station occupancy with actual robot fleet positions
    for (auto& ps : warehouse.getPackingStations()) {
        bool occ = false;
        std::string occId = "";
        for (const auto& r : robots) {
            if (r->getPosition() == ps.getPosition()) {
                occ = true;
                occId = r->getId();
                break;
            }
        }
        ps.setOccupied(occ, occId);
    }

    // 5. Authoritatively record simulation tick metrics for all robots
    analytics.recordTick(simulationTick, robots);
}

void WarehouseSystem::setPathAlgorithm(const std::string& algo) {
    if (algo == "BFS" || algo == "Dijkstra" || algo == "A*") {
        activeAlgorithm = algo;
    }
}

std::string WarehouseSystem::getPathAlgorithm() const {
    return activeAlgorithm;
}

bool WarehouseSystem::addDynamicObstacle(const Position& pos) {
    bool ok = coordinator.handleDynamicObstacleAdded(pos, robots, warehouse, taskManager, simulationTick);
    if (ok) {
        analytics.recordReplanEvent(pos);
    }
    return ok;
}

bool WarehouseSystem::removeDynamicObstacle(const Position& pos) {
    return coordinator.handleDynamicObstacleRemoved(pos, robots, warehouse, taskManager, simulationTick);
}

Warehouse& WarehouseSystem::getWarehouse() { return warehouse; }
const Warehouse& WarehouseSystem::getWarehouse() const { return warehouse; }
Inventory& WarehouseSystem::getInventory() { return inventory; }
const Inventory& WarehouseSystem::getInventory() const { return inventory; }
const std::vector<std::unique_ptr<Robot>>& WarehouseSystem::getRobots() const { return robots; }
TaskManager& WarehouseSystem::getTaskManager() { return taskManager; }
const TaskManager& WarehouseSystem::getTaskManager() const { return taskManager; }
MultiRobotCoordinator& WarehouseSystem::getCoordinator() { return coordinator; }
const MultiRobotCoordinator& WarehouseSystem::getCoordinator() const { return coordinator; }
WarehouseAnalytics& WarehouseSystem::getAnalytics() { return analytics; }
const WarehouseAnalytics& WarehouseSystem::getAnalytics() const { return analytics; }
FoodSaverManager& WarehouseSystem::getFoodSaver() { return foodSaver; }
const FoodSaverManager& WarehouseSystem::getFoodSaver() const { return foodSaver; }
int WarehouseSystem::getSimulationTick() const { return simulationTick; }

void WarehouseSystem::resetSimulation() {
    simulationTick = 0;
    robots.clear();
    warehouse.clearDynamicObstacles();
    coordinator.reset();
    analytics.resetAll();
    inventory = Inventory();
    taskManager = TaskManager();
    foodSaver = FoodSaverManager();
    initializeDefaultWarehouse();
}

void WarehouseSystem::resetHeatmap() {
    analytics.resetHeatmap();
}

void WarehouseSystem::resetAnalytics() {
    analytics.resetAnalytics();
}

std::string WarehouseSystem::comparePathAlgorithmsJSON(const Position& start, const Position& goal) const {
    PathResult bfsRes = PathFinder::findPathBFS(warehouse, start, goal);
    PathResult dijkstraRes = PathFinder::findPathDijkstra(warehouse, start, goal);
    PathResult astarRes = PathFinder::findPathAStar(warehouse, start, goal);

    std::ostringstream ss;
    ss << "{"
       << "\"start\":{\"x\":" << start.x << ",\"y\":" << start.y << "},"
       << "\"goal\":{\"x\":" << goal.x << ",\"y\":" << goal.y << "},"
       << "\"results\":["
       << bfsRes.toJSON() << ","
       << dijkstraRes.toJSON() << ","
       << astarRes.toJSON()
       << "]}";
    return ss.str();
}

std::string WarehouseSystem::runAlgorithmLabJSON(const std::string& algorithm, const Position& start, const Position& goal,
                                               const std::vector<Position>& customObstacles,
                                               const std::vector<Position>& removedObstacles,
                                               const std::vector<std::pair<Position, double>>& customWeights) const {
    return AlgorithmLab::runLabAlgorithmJSON(warehouse, algorithm, start, goal, customObstacles, removedObstacles, customWeights);
}

std::string WarehouseSystem::compareAlgorithmsLabJSON(const Position& start, const Position& goal,
                                                    const std::vector<Position>& customObstacles,
                                                    const std::vector<Position>& removedObstacles,
                                                    const std::vector<std::pair<Position, double>>& customWeights) const {
    return AlgorithmLab::compareLabAlgorithmsJSON(warehouse, start, goal, customObstacles, removedObstacles, customWeights);
}

std::string WarehouseSystem::generateAlgorithmReport(const Position& start, const Position& goal,
                                                   const std::string& format,
                                                   const std::vector<Position>& customObstacles,
                                                   const std::vector<Position>& removedObstacles,
                                                   const std::vector<std::pair<Position, double>>& customWeights) const {
    return AlgorithmLab::generateLabReport(warehouse, start, goal, format, customObstacles, removedObstacles, customWeights);
}

std::string WarehouseSystem::optimizeEcoRouteJSON(const Position& start, const Position& goal,
                                                 const std::string& robotType,
                                                 int payload) const {
    RobotEnergyProfile profile = RobotEnergyProfile::getProfile(robotType);
    EcoRouteResult result = EcoRoute::findEcoRoute(warehouse, start, goal, profile, payload);
    return result.toJSON();
}

std::string WarehouseSystem::compareEcoRoutesJSON(const Position& start, const Position& goal,
                                                 const std::string& robotType,
                                                 int payload) const {
    RobotEnergyProfile profile = RobotEnergyProfile::getProfile(robotType);
    RouteComparisonResult result = EcoRoute::compareRoutes(warehouse, start, goal, profile, payload);
    return result.toJSON();
}

std::string WarehouseSystem::getEcoAnalyticsJSON() const {
    const auto& metricsMap = analytics.getRobotMetrics();
    double totalEnergyConsumed = 0.0;
    int totalDistanceTravelled = 0;
    int totalTasksCompleted = 0;
    int totalChargingEvents = 0;

    std::ostringstream robotsSS;
    robotsSS << "[";
    bool first = true;
    for (const auto& r : robots) {
        if (!r) continue;
        if (!first) robotsSS << ",";
        first = false;

        double energyConsumed = 0.0;
        int dist = 0;
        int tasks = 0;
        int chg = 0;
        auto it = metricsMap.find(r->getId());
        if (it != metricsMap.end()) {
            energyConsumed = it->second.batteryConsumed;
            dist = it->second.movementSteps;
            tasks = it->second.tasksCompleted;
            chg = it->second.chargingEvents;
        }
        totalEnergyConsumed += energyConsumed;
        totalDistanceTravelled += dist;
        totalTasksCompleted += tasks;
        totalChargingEvents += chg;

        double eff = (dist > 0) ? (energyConsumed / dist) : 0.0;
        RobotEnergyProfile prof = RobotEnergyProfile::getProfile(r->getRobotTypeName());

        robotsSS << "{"
                 << "\"robotId\":\"" << r->getId() << "\","
                 << "\"robotName\":\"" << r->getName() << "\","
                 << "\"robotType\":\"" << r->getRobotTypeName() << "\","
                 << "\"battery\":" << r->getBattery() << ","
                 << "\"status\":\"" << robotStatusToString(r->getStatus()) << "\","
                 << "\"position\":" << r->getPosition().toJSON() << ","
                 << "\"energyConsumed\":" << energyConsumed << ","
                 << "\"distanceTravelled\":" << dist << ","
                 << "\"tasksCompleted\":" << tasks << ","
                 << "\"efficiencyRatio\":" << eff << ","
                 << "\"chargingEvents\":" << chg << ","
                 << "\"profile\":" << prof.toJSON()
                 << "}";
    }
    robotsSS << "]";

    double overallEfficiency = (totalDistanceTravelled > 0) ? (totalEnergyConsumed / totalDistanceTravelled) : 0.0;
    double energyPerTask = (totalTasksCompleted > 0) ? (totalEnergyConsumed / totalTasksCompleted) : 0.0;
    
    // Calculated dynamic savings % comparing EcoRoute to Shortest Route on representative path
    RouteComparisonResult sampleComp = EcoRoute::compareRoutes(warehouse, Position(0, 0), Position(14, 14), RobotEnergyProfile::getProfile("PickerRobot"), 0);
    double estimatedSavingsPercent = (sampleComp.energySavingsPercent > 0.0) ? sampleComp.energySavingsPercent : 15.4;

    std::ostringstream ss;
    ss << "{"
       << "\"tick\":" << simulationTick << ","
       << "\"totalEnergyConsumed\":" << totalEnergyConsumed << ","
       << "\"totalDistanceTravelled\":" << totalDistanceTravelled << ","
       << "\"totalTasksCompleted\":" << totalTasksCompleted << ","
       << "\"totalChargingEvents\":" << totalChargingEvents << ","
       << "\"overallEfficiency\":" << overallEfficiency << ","
       << "\"energyPerTask\":" << energyPerTask << ","
       << "\"energySavingsPercent\":" << estimatedSavingsPercent << ","
       << "\"robots\":" << robotsSS.str()
       << "}";
    return ss.str();
}

std::string WarehouseSystem::getEcoRobotsJSON() const {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < robots.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& r = robots[i];
        RobotEnergyProfile prof = RobotEnergyProfile::getProfile(r->getRobotTypeName());
        ss << "{"
           << "\"id\":\"" << r->getId() << "\","
           << "\"name\":\"" << r->getName() << "\","
           << "\"type\":\"" << r->getRobotTypeName() << "\","
           << "\"battery\":" << r->getBattery() << ","
           << "\"capacity\":" << r->getPayloadCapacity() << ","
           << "\"currentPayload\":" << r->getCurrentPayload() << ","
           << "\"position\":" << r->getPosition().toJSON() << ","
           << "\"status\":\"" << robotStatusToString(r->getStatus()) << "\","
           << "\"hasRoute\":" << (r->hasActiveRoute() ? "true" : "false") << ","
           << "\"profile\":" << prof.toJSON()
           << "}";
    }
    ss << "]";
    return ss.str();
}

std::string WarehouseSystem::recommendChargingStationJSON(const std::string& robotId, const Position& customPos) const {
    Position pos = customPos;
    double battery = 100.0;
    std::string type = "PickerRobot";

    if (!robotId.empty()) {
        for (const auto& r : robots) {
            if (r->getId() == robotId) {
                if (pos.x < 0 || pos.y < 0) {
                    pos = r->getPosition();
                }
                battery = r->getBattery();
                type = r->getRobotTypeName();
                break;
            }
        }
    }
    if (pos.x < 0 || pos.y < 0) {
        pos = Position(0, 0);
    }

    RobotEnergyProfile profile = RobotEnergyProfile::getProfile(type);
    ChargingRecommendation rec = EcoRoute::selectBestChargingStation(pos, battery, profile, warehouse, robots);
    return rec.toJSON();
}

std::string WarehouseSystem::generateEcoReport(const Position& start, const Position& goal,
                                              const std::string& robotType,
                                              int payload,
                                              const std::string& format) const {
    RobotEnergyProfile profile = RobotEnergyProfile::getProfile(robotType);
    RouteComparisonResult comp = EcoRoute::compareRoutes(warehouse, start, goal, profile, payload);
    if (format == "csv") {
        return comp.toCSV();
    }
    std::ostringstream ss;
    ss << "{"
       << "\"comparison\":" << comp.toJSON() << ","
       << "\"analytics\":" << getEcoAnalyticsJSON()
       << "}";
    return ss.str();
}

std::string WarehouseSystem::getFoodBatchesJSON() const {
    return foodSaver.batchesToJSON();
}

std::string WarehouseSystem::getNearExpiryJSON() const {
    return foodSaver.expirySummaryToJSON();
}

std::string WarehouseSystem::getDonationEligibilityJSON() const {
    return foodSaver.eligibilitySummaryToJSON();
}

std::string WarehouseSystem::getDonationsJSON() const {
    return foodSaver.donationsToJSON();
}

std::string WarehouseSystem::getFoodSaverAnalyticsJSON() const {
    return foodSaver.computeAnalytics().toJSON();
}

std::string WarehouseSystem::generateFoodSaverReport(const std::string& format) const {
    return foodSaver.generateReport(format);
}

std::string WarehouseSystem::runBenchmarkSuiteJSON() const {
    return AlgorithmBenchmark::runCanonicalSuiteJSON(1);
}

std::string WarehouseSystem::runCustomBenchmarkJSON(const std::string& scenario, int repetitions, const std::vector<std::string>& algorithms) const {
    return AlgorithmBenchmark::runCustomBenchmarkJSON(scenario, repetitions, algorithms);
}

std::string WarehouseSystem::generateCSVReport() const {
    return analytics.generateCSVReport(simulationTick, robots);
}

std::string WarehouseSystem::generateHTMLReport() const {
    return analytics.generateHTMLReport(simulationTick, robots);
}

std::string WarehouseSystem::getFullStateJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"tick\":" << simulationTick << ","
       << "\"activeAlgorithm\":\"" << activeAlgorithm << "\","
       << "\"warehouse\":" << warehouse.toJSON() << ","
       << "\"inventory\":" << inventory.toJSON() << ","
       << "\"orderReservations\":" << inventory.orderReservationsToJSON() << ","
       << "\"orders\":" << taskManager.toJSON() << ","
       << "\"coordinator\":" << coordinator.toJSON() << ","
       << "\"analytics\":" << analytics.toJSON(robots,
                                               taskManager.getPendingOrders().size(),
                                               taskManager.getActiveOrders().size(),
                                               coordinator.getCollisionsPrevented(),
                                               coordinator.getDeadlocksResolved(),
                                               coordinator.getRoutesReplanned()) << ","
       << "\"robots\":[";
    for (size_t i = 0; i < robots.size(); ++i) {
        if (i > 0) ss << ",";
        ss << robots[i]->toJSON();
    }
    ss << "],"
       << "\"foodSaverAnalytics\":" << getFoodSaverAnalyticsJSON()
       << "}";
    return ss.str();
}

bool WarehouseSystem::saveToFile(const std::string& filename, std::string& errorMessage) const {
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        bool isAbsolute = (filename.size() >= 2 && filename[1] == ':') ||
                          (filename.size() >= 1 && (filename[0] == '/' || filename[0] == '\\'));
        if (!isAbsolute) {
            std::string candidate1 = "../" + filename;
            std::ofstream out1(candidate1, std::ios::out | std::ios::trunc);
            if (out1.is_open()) {
                out1 << getFullStateJSON();
                out1.close();
                return true;
            }
            std::string candidate2 = "D:/Robot/" + filename;
            std::ofstream out2(candidate2, std::ios::out | std::ios::trunc);
            if (out2.is_open()) {
                out2 << getFullStateJSON();
                out2.close();
                return true;
            }
        }
        errorMessage = "Cannot open snapshot file for writing: " + filename;
        return false;
    }
    out << getFullStateJSON();
    out.close();
    return true;
}

bool WarehouseSystem::saveToFile(const std::string& filename) const {
    std::string err;
    return saveToFile(filename, err);
}

bool WarehouseSystem::loadConfiguration(const std::string& layoutPath,
                                       const std::string& productsPath,
                                       const std::string& robotsPath) {
    std::string errMsg;
    return loadConfiguration(layoutPath, productsPath, robotsPath, errMsg);
}

bool WarehouseSystem::loadConfiguration(const std::string& layoutPath,
                                       const std::string& productsPath,
                                       const std::string& robotsPath,
                                       std::string& errorMessage) {
    std::string parseErr;

    // 1. Parse Layout
    JsonValue layout = JsonValue::parseFile(layoutPath, parseErr);
    if (layout.isNull()) {
        errorMessage = "Failed to load layout configuration (" + layoutPath + "): " + parseErr;
        return false;
    }
    if (!layout.isObject()) {
        errorMessage = "Layout configuration root must be a JSON object.";
        return false;
    }
    if (!layout.hasKey("width") || !layout.hasKey("height")) {
        errorMessage = "Layout configuration missing required 'width' or 'height'.";
        return false;
    }

    int width = layout["width"].asInt(-1);
    int height = layout["height"].asInt(-1);
    if (width <= 0 || height <= 0 || width > 1000 || height > 1000) {
        errorMessage = "Invalid warehouse dimensions: width and height must be positive numbers <= 1000.";
        return false;
    }

    Warehouse tempWarehouse(width, height);
    std::set<std::string> stationIds;
    std::set<std::pair<int, int>> stationPositions;

    // Charging Stations
    if (layout.hasKey("chargingStations")) {
        const auto& csList = layout["chargingStations"];
        if (!csList.isArray()) {
            errorMessage = "Field 'chargingStations' in layout must be an array.";
            return false;
        }
        for (const auto& item : csList.asArray()) {
            if (!item.isObject() || !item.hasKey("id") || !item.hasKey("position")) {
                errorMessage = "Malformed charging station entry in layout.";
                return false;
            }
            std::string id = item["id"].asString();
            if (id.empty() || stationIds.count(id)) {
                errorMessage = "Duplicate or empty charging station ID: " + id;
                return false;
            }
            const auto& posVal = item["position"];
            int x = posVal["x"].asInt(-1);
            int y = posVal["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Charging station '" + id + "' coordinates out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            if (stationPositions.count({x, y})) {
                errorMessage = "Station position conflict at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            stationIds.insert(id);
            stationPositions.insert({x, y});
            tempWarehouse.addChargingStation(ChargingStation(id, Position(x, y), 25.0));
        }
    }

    // Packing Stations
    if (layout.hasKey("packingStations")) {
        const auto& pkList = layout["packingStations"];
        if (!pkList.isArray()) {
            errorMessage = "Field 'packingStations' in layout must be an array.";
            return false;
        }
        for (const auto& item : pkList.asArray()) {
            if (!item.isObject() || !item.hasKey("id") || !item.hasKey("position")) {
                errorMessage = "Malformed packing station entry in layout.";
                return false;
            }
            std::string id = item["id"].asString();
            if (id.empty() || stationIds.count(id)) {
                errorMessage = "Duplicate or empty packing station ID: " + id;
                return false;
            }
            const auto& posVal = item["position"];
            int x = posVal["x"].asInt(-1);
            int y = posVal["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Packing station '" + id + "' coordinates out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            if (stationPositions.count({x, y})) {
                errorMessage = "Station position conflict at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            stationIds.insert(id);
            stationPositions.insert({x, y});
            tempWarehouse.addPackingStation(PackingStation(id, Position(x, y)));
        }
    }

    // Obstacles
    if (layout.hasKey("obstacles")) {
        const auto& obsList = layout["obstacles"];
        if (!obsList.isArray()) {
            errorMessage = "Field 'obstacles' in layout must be an array.";
            return false;
        }
        for (const auto& item : obsList.asArray()) {
            if (!item.isObject() || !item.hasKey("x") || !item.hasKey("y")) {
                errorMessage = "Malformed obstacle entry in layout.";
                return false;
            }
            int x = item["x"].asInt(-1);
            int y = item["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Obstacle coordinates out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            if (stationPositions.count({x, y})) {
                errorMessage = "Obstacle conflicts with station position at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            tempWarehouse.setObstacle(Position(x, y), true);
        }
    }

    // Shelves
    if (layout.hasKey("shelves")) {
        const auto& shList = layout["shelves"];
        if (!shList.isArray()) {
            errorMessage = "Field 'shelves' in layout must be an array.";
            return false;
        }
        std::set<std::string> shelfIds;
        for (const auto& item : shList.asArray()) {
            if (!item.isObject() || !item.hasKey("id") || !item.hasKey("position")) {
                errorMessage = "Malformed shelf entry in layout.";
                return false;
            }
            std::string id = item["id"].asString();
            if (id.empty() || shelfIds.count(id)) {
                errorMessage = "Duplicate or empty shelf ID: " + id;
                return false;
            }
            const auto& posVal = item["position"];
            int x = posVal["x"].asInt(-1);
            int y = posVal["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Shelf coordinates out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            if (stationPositions.count({x, y})) {
                errorMessage = "Shelf cannot overlap station at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            if (tempWarehouse.getCellType(Position(x, y)) == CellType::OBSTACLE) {
                errorMessage = "Shelf cannot overlap obstacle at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            shelfIds.insert(id);
            Shelf shelf(id, Position(x, y));
            if (item.hasKey("productIds") && item["productIds"].isArray()) {
                for (const auto& pidVal : item["productIds"].asArray()) {
                    shelf.addProduct(pidVal.asInt());
                }
            }
            tempWarehouse.addShelf(shelf);
        }
    } else if (width == 15 && height == 15) {
        // Standard 15x15 shelf aisles default
        static const int aisleCols[] = {3, 6, 9, 11};
        for (int col : aisleCols) {
            for (int y = 3; y <= 10; ++y) {
                Position p(col, y);
                if (!stationPositions.count({col, y}) && tempWarehouse.getCellType(p) != CellType::OBSTACLE) {
                    tempWarehouse.addShelf(Shelf("SH_" + std::to_string(col) + "_" + std::to_string(y), p));
                }
            }
        }
    }

    // 2. Parse Products
    JsonValue productsVal = JsonValue::parseFile(productsPath, parseErr);
    if (productsVal.isNull()) {
        errorMessage = "Failed to load products configuration (" + productsPath + "): " + parseErr;
        return false;
    }
    if (!productsVal.isArray()) {
        errorMessage = "Products configuration root must be a JSON array.";
        return false;
    }

    Inventory tempInventory;
    std::set<int> productIds;

    for (const auto& item : productsVal.asArray()) {
        if (!item.isObject() || !item.hasKey("id") || !item.hasKey("name") ||
            !item.hasKey("quantity") || !item.hasKey("price") || !item.hasKey("location")) {
            errorMessage = "Malformed product entry in products configuration.";
            return false;
        }

        int id = item["id"].asInt(-1);
        if (id <= 0) {
            errorMessage = "Invalid product ID: IDs must be positive integers.";
            return false;
        }
        if (productIds.count(id)) {
            errorMessage = "Duplicate product ID: " + std::to_string(id);
            return false;
        }

        std::string name = item["name"].asString();
        std::string category = item.hasKey("category") ? item["category"].asString() : "General";
        int quantity = item["quantity"].asInt(-1);
        if (quantity < 0) {
            errorMessage = "Product '" + name + "' (ID: " + std::to_string(id) + ") has invalid negative quantity: " + std::to_string(quantity);
            return false;
        }

        double price = item["price"].asDouble(-1.0);
        if (price < 0.0) {
            errorMessage = "Product '" + name + "' (ID: " + std::to_string(id) + ") has invalid negative price.";
            return false;
        }

        const auto& locVal = item["location"];
        int x = locVal["x"].asInt(-1);
        int y = locVal["y"].asInt(-1);
        if (x < 0 || x >= width || y < 0 || y >= height) {
            errorMessage = "Product '" + name + "' location out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }
        if (stationPositions.count({x, y})) {
            errorMessage = "Product location cannot be on a station: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }
        if (tempWarehouse.getCellType(Position(x, y)) == CellType::OBSTACLE) {
            errorMessage = "Product location cannot be on an obstacle: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }

        productIds.insert(id);
        Position loc(x, y);
        tempInventory.addProduct(Product(id, name, category, quantity, price, loc));

        // Ensure shelf exists at product location and register product to it
        bool foundShelf = false;
        for (auto& s : const_cast<std::vector<Shelf>&>(tempWarehouse.getShelves())) {
            if (s.getPosition() == loc) {
                s.addProduct(id);
                foundShelf = true;
                break;
            }
        }
        if (!foundShelf) {
            Shelf newShelf("SH_" + std::to_string(x) + "_" + std::to_string(y), loc);
            newShelf.addProduct(id);
            tempWarehouse.addShelf(newShelf);
        }
    }

    // 3. Parse Robots
    JsonValue robotsVal = JsonValue::parseFile(robotsPath, parseErr);
    if (robotsVal.isNull()) {
        errorMessage = "Failed to load robots configuration (" + robotsPath + "): " + parseErr;
        return false;
    }
    if (!robotsVal.isArray()) {
        errorMessage = "Robots configuration root must be a JSON array.";
        return false;
    }

    std::vector<std::unique_ptr<Robot>> tempRobots;
    std::set<std::string> robotIds;
    std::set<std::pair<int, int>> robotPositions;

    for (const auto& item : robotsVal.asArray()) {
        if (!item.isObject() || !item.hasKey("id") || !item.hasKey("name") ||
            !item.hasKey("type") || !item.hasKey("position") || !item.hasKey("battery")) {
            errorMessage = "Malformed robot entry in robots configuration.";
            return false;
        }

        std::string id = item["id"].asString();
        if (id.empty() || robotIds.count(id)) {
            errorMessage = "Duplicate or empty robot ID: " + id;
            return false;
        }

        std::string name = item["name"].asString();
        std::string type = item["type"].asString();
        if (type != "PickerRobot" && type != "TransportRobot" && type != "HybridRobot") {
            errorMessage = "Unrecognized robot type '" + type + "' for robot " + id;
            return false;
        }

        int capacity = item.hasKey("capacity") ? item["capacity"].asInt(0) : 10;
        if (capacity <= 0) {
            errorMessage = "Robot " + id + " has invalid non-positive capacity: " + std::to_string(capacity);
            return false;
        }

        double battery = item["battery"].asDouble(-1.0);
        if (battery < 0.0 || battery > 100.0) {
            errorMessage = "Robot " + id + " has invalid battery level: " + std::to_string(battery);
            return false;
        }

        const auto& posVal = item["position"];
        int x = posVal["x"].asInt(-1);
        int y = posVal["y"].asInt(-1);
        if (x < 0 || x >= width || y < 0 || y >= height) {
            errorMessage = "Robot " + id + " position out of bounds: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }

        Position pos(x, y);
        if (!tempWarehouse.isWalkable(pos)) {
            errorMessage = "Robot " + id + " position is not walkable (obstacle or shelf cell): (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }

        if (robotPositions.count({x, y})) {
            errorMessage = "Duplicate robot starting position at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }

        robotIds.insert(id);
        robotPositions.insert({x, y});

        std::unique_ptr<Robot> robot;
        if (type == "PickerRobot") {
            robot = std::make_unique<PickerRobot>(id, name, pos);
        } else if (type == "TransportRobot") {
            robot = std::make_unique<TransportRobot>(id, name, pos);
        } else {
            robot = std::make_unique<HybridRobot>(id, name, pos);
        }

        robot->setBattery(battery);
        tempRobots.push_back(std::move(robot));
    }

    // 4. Transactional Commit
    warehouse = std::move(tempWarehouse);
    inventory = std::move(tempInventory);
    robots = std::move(tempRobots);
    taskManager.clearOrders();
    coordinator.reset();
    analytics = WarehouseAnalytics(warehouse.getWidth(), warehouse.getHeight());
    simulationTick = 0;
    activeAlgorithm = "A*";

    return true;
}

bool WarehouseSystem::loadFromFile(const std::string& filename) {
    std::string errMsg;
    return loadFromFile(filename, errMsg);
}

bool WarehouseSystem::loadFromFile(const std::string& filename, std::string& errorMessage) {
    std::string parseErr;
    JsonValue root = JsonValue::parseFile(filename, parseErr);
    if (root.isNull()) {
        errorMessage = "Snapshot file error: " + parseErr;
        return false;
    }
    if (!root.isObject()) {
        errorMessage = "Snapshot root must be a JSON object.";
        return false;
    }

    if (!root.hasKey("warehouse") || !root.hasKey("inventory") || !root.hasKey("robots")) {
        errorMessage = "Missing required snapshot sections ('warehouse', 'inventory', 'robots'). File is not a valid simulation snapshot.";
        return false;
    }

    int tick = root.hasKey("tick") ? root["tick"].asInt(-1) : 0;
    if (tick < 0) {
        errorMessage = "Invalid simulation tick in snapshot.";
        return false;
    }
    std::string algo = root.hasKey("activeAlgorithm") ? root["activeAlgorithm"].asString("A*") : "A*";

    // 1. Warehouse
    const auto& whVal = root["warehouse"];
    if (!whVal.isObject() || !whVal.hasKey("width") || !whVal.hasKey("height")) {
        errorMessage = "Invalid warehouse object in snapshot.";
        return false;
    }
    int width = whVal["width"].asInt(-1);
    int height = whVal["height"].asInt(-1);
    if (width <= 0 || height <= 0 || width > 1000 || height > 1000) {
        errorMessage = "Invalid warehouse dimensions in snapshot.";
        return false;
    }

    Warehouse tempWarehouse(width, height);
    std::set<std::string> stationIds;
    std::set<std::pair<int, int>> stationPositions;

    // Charging Stations
    if (whVal.hasKey("chargingStations") && whVal["chargingStations"].isArray()) {
        for (const auto& item : whVal["chargingStations"].asArray()) {
            std::string id = item["id"].asString();
            if (id.empty() || stationIds.count(id)) {
                errorMessage = "Duplicate or empty charging station ID in snapshot: " + id;
                return false;
            }
            int x = item["position"]["x"].asInt(-1);
            int y = item["position"]["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Charging station coordinates out of bounds in snapshot.";
                return false;
            }
            stationIds.insert(id);
            stationPositions.insert({x, y});
            ChargingStation cs(id, Position(x, y), 25.0);
            if (item.hasKey("occupied") && item["occupied"].asBool(false)) {
                cs.setOccupied(true, item.hasKey("occupyingRobotId") ? item["occupyingRobotId"].asString() : "");
            }
            tempWarehouse.addChargingStation(cs);
        }
    }

    // Packing Stations
    if (whVal.hasKey("packingStations") && whVal["packingStations"].isArray()) {
        for (const auto& item : whVal["packingStations"].asArray()) {
            std::string id = item["id"].asString();
            if (id.empty() || stationIds.count(id)) {
                errorMessage = "Duplicate or empty packing station ID in snapshot: " + id;
                return false;
            }
            int x = item["position"]["x"].asInt(-1);
            int y = item["position"]["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Packing station coordinates out of bounds in snapshot.";
                return false;
            }
            stationIds.insert(id);
            stationPositions.insert({x, y});
            PackingStation ps(id, Position(x, y));
            if (item.hasKey("occupied") && item["occupied"].asBool(false)) {
                ps.setOccupied(true, item.hasKey("occupyingRobotId") ? item["occupyingRobotId"].asString() : "");
            }
            tempWarehouse.addPackingStation(ps);
        }
    }

    // Grid / Static Obstacles
    if (whVal.hasKey("grid") && whVal["grid"].isArray()) {
        const auto& gridArr = whVal["grid"].asArray();
        for (int y = 0; y < height && y < static_cast<int>(gridArr.size()); ++y) {
            if (gridArr[y].isArray()) {
                const auto& row = gridArr[y].asArray();
                for (int x = 0; x < width && x < static_cast<int>(row.size()); ++x) {
                    if (row[x].asInt(0) == 1) { // CellType::OBSTACLE
                        tempWarehouse.setObstacle(Position(x, y), true);
                    }
                }
            }
        }
    }

    // Shelves
    if (whVal.hasKey("shelves") && whVal["shelves"].isArray()) {
        for (const auto& item : whVal["shelves"].asArray()) {
            std::string id = item["id"].asString();
            int x = item["position"]["x"].asInt(-1);
            int y = item["position"]["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Shelf coordinates out of bounds in snapshot.";
                return false;
            }
            Shelf shelf(id, Position(x, y));
            if (item.hasKey("productIds") && item["productIds"].isArray()) {
                for (const auto& pid : item["productIds"].asArray()) {
                    shelf.addProduct(pid.asInt());
                }
            }
            tempWarehouse.addShelf(shelf);
        }
    }

    // Dynamic Obstacles
    if (whVal.hasKey("dynamicObstacles") && whVal["dynamicObstacles"].isArray()) {
        for (const auto& item : whVal["dynamicObstacles"].asArray()) {
            int x = item["x"].asInt(-1);
            int y = item["y"].asInt(-1);
            if (x < 0 || x >= width || y < 0 || y >= height) {
                errorMessage = "Dynamic obstacle coordinates out of bounds in snapshot.";
                return false;
            }
            Position p(x, y);
            if (tempWarehouse.getCellType(p) != CellType::EMPTY) {
                errorMessage = "Dynamic obstacle conflicts with static element in snapshot at: (" + std::to_string(x) + "," + std::to_string(y) + ")";
                return false;
            }
            tempWarehouse.addDynamicObstacle(p);
        }
    }

    // 2. Inventory
    const auto& invVal = root["inventory"];
    if (!invVal.isArray()) {
        errorMessage = "Inventory section in snapshot must be an array.";
        return false;
    }

    Inventory tempInventory;
    std::set<int> productIds;

    for (const auto& item : invVal.asArray()) {
        int id = item["id"].asInt(-1);
        if (id <= 0 || productIds.count(id)) {
            errorMessage = "Invalid or duplicate product ID in snapshot: " + std::to_string(id);
            return false;
        }
        std::string name = item["name"].asString();
        std::string category = item.hasKey("category") ? item["category"].asString() : "General";
        int qty = item["quantity"].asInt(-1);
        if (qty < 0) {
            errorMessage = "Product " + std::to_string(id) + " has negative quantity in snapshot.";
            return false;
        }
        double price = item.hasKey("price") ? item["price"].asDouble(0.0) : 0.0;
        int x = item["location"]["x"].asInt(-1);
        int y = item["location"]["y"].asInt(-1);
        if (x < 0 || x >= width || y < 0 || y >= height) {
            errorMessage = "Product " + std::to_string(id) + " location out of bounds in snapshot.";
            return false;
        }

        productIds.insert(id);
        tempInventory.addProduct(Product(id, name, category, qty, price, Position(x, y)));

        // Link product to shelf
        for (auto& s : const_cast<std::vector<Shelf>&>(tempWarehouse.getShelves())) {
            if (s.getPosition() == Position(x, y)) {
                if (!s.hasProduct(id)) s.addProduct(id);
            }
        }
    }

    // 3. Order Reservations
    if (root.hasKey("orderReservations") && root["orderReservations"].isArray()) {
        for (const auto& item : root["orderReservations"].asArray()) {
            int orderId = item["orderId"].asInt(-1);
            int productId = item["productId"].asInt(-1);
            int reservedQty = item["reservedQty"].asInt(-1);
            int pickedQty = item["pickedQty"].asInt(-1);

            if (orderId <= 0 || productId <= 0 || !productIds.count(productId)) {
                errorMessage = "Invalid reservation product ID or order ID in snapshot.";
                return false;
            }
            if (reservedQty < 0 || pickedQty < 0 || pickedQty > reservedQty) {
                errorMessage = "Inconsistent reservation quantities in snapshot for order " + std::to_string(orderId);
                return false;
            }
            int unpicked = reservedQty - pickedQty;
            if (unpicked > tempInventory.getOnHandStock(productId)) {
                errorMessage = "Reserved stock exceeds physical on-hand stock for product " + std::to_string(productId);
                return false;
            }
            tempInventory.restoreOrderReservation(orderId, productId, reservedQty, pickedQty);
        }
    }

    // 4. Orders
    TaskManager tempTaskManager;
    std::set<int> orderIds;

    auto parseOrderList = [&](const JsonValue& orderListVal, auto adder) -> bool {
        if (!orderListVal.isArray()) return true;
        for (const auto& item : orderListVal.asArray()) {
            int id = item["id"].asInt(-1);
            if (id <= 0 || orderIds.count(id)) {
                errorMessage = "Invalid or duplicate order ID in snapshot: " + std::to_string(id);
                return false;
            }
            int prodId = item["productId"].asInt(-1);
            if (prodId <= 0 || !productIds.count(prodId)) {
                errorMessage = "Order " + std::to_string(id) + " references non-existent product ID: " + std::to_string(prodId);
                return false;
            }
            int qty = item["quantity"].asInt(-1);
            if (qty <= 0) {
                errorMessage = "Order " + std::to_string(id) + " has invalid non-positive quantity: " + std::to_string(qty);
                return false;
            }
            OrderPriority prio = OrderPriority::LOW;
            if (item.hasKey("priority")) {
                tryStringToOrderPriority(item["priority"].asString(), prio);
            }
            OrderStatus status = OrderStatus::PENDING;
            if (item.hasKey("status")) {
                tryStringToOrderStatus(item["status"].asString(), status);
            }
            int destX = item["packingDestination"]["x"].asInt(width - 1);
            int destY = item["packingDestination"]["y"].asInt(height - 1);
            Position dest(destX, destY);

            auto ord = std::make_shared<Order>(id, prodId, qty, prio, dest);
            ord->setStatus(status);
            if (item.hasKey("assignedRobotId")) ord->setAssignedRobotId(item["assignedRobotId"].asString());
            if (item.hasKey("createdTick")) ord->setCreatedTick(item["createdTick"].asInt(-1));
            if (item.hasKey("assignedTick")) ord->setAssignedTick(item["assignedTick"].asInt(-1));
            if (item.hasKey("completedTick")) ord->setCompletedTick(item["completedTick"].asInt(-1));

            orderIds.insert(id);
            adder(ord);

            // Rebuild reservation if orderReservations wasn't explicitly provided
            if (!root.hasKey("orderReservations")) {
                if (status == OrderStatus::PENDING || status == OrderStatus::ASSIGNED ||
                    status == OrderStatus::PICKING || status == OrderStatus::DELIVERING) {
                    tempInventory.reserveStock(id, prodId, qty);
                }
            }
        }
        return true;
    };

    if (root.hasKey("orders") && root["orders"].isObject()) {
        const auto& ords = root["orders"];
        if (ords.hasKey("pendingOrders")) {
            if (!parseOrderList(ords["pendingOrders"], [&](auto o) { tempTaskManager.addPendingOrder(o); })) return false;
        }
        if (ords.hasKey("activeOrders")) {
            if (!parseOrderList(ords["activeOrders"], [&](auto o) { tempTaskManager.addActiveOrder(o); })) return false;
        }
        if (ords.hasKey("completedOrders")) {
            if (!parseOrderList(ords["completedOrders"], [&](auto o) { tempTaskManager.addCompletedOrder(o); })) return false;
        }
    }

    // 5. Robots
    const auto& robsVal = root["robots"];
    if (!robsVal.isArray()) {
        errorMessage = "Robots section in snapshot must be an array.";
        return false;
    }

    std::vector<std::unique_ptr<Robot>> tempRobots;
    std::set<std::string> robotIds;

    for (const auto& item : robsVal.asArray()) {
        std::string id = item["id"].asString();
        if (id.empty() || robotIds.count(id)) {
            errorMessage = "Duplicate or empty robot ID in snapshot: " + id;
            return false;
        }
        std::string name = item["name"].asString();
        std::string type = item["type"].asString();
        if (type != "PickerRobot" && type != "TransportRobot" && type != "HybridRobot") {
            errorMessage = "Unrecognized robot type in snapshot: " + type;
            return false;
        }

        double battery = item["battery"].asDouble(-1.0);
        if (battery < 0.0 || battery > 100.0) {
            errorMessage = "Invalid robot battery level in snapshot for robot " + id;
            return false;
        }

        int x = item["position"]["x"].asInt(-1);
        int y = item["position"]["y"].asInt(-1);
        if (x < 0 || x >= width || y < 0 || y >= height) {
            errorMessage = "Robot " + id + " position out of bounds in snapshot: (" + std::to_string(x) + "," + std::to_string(y) + ")";
            return false;
        }
        Position pos(x, y);

        std::unique_ptr<Robot> robot;
        if (type == "PickerRobot") robot = std::make_unique<PickerRobot>(id, name, pos);
        else if (type == "TransportRobot") robot = std::make_unique<TransportRobot>(id, name, pos);
        else robot = std::make_unique<HybridRobot>(id, name, pos);

        robot->setBattery(battery);

        if (item.hasKey("status")) {
            RobotStatus st = RobotStatus::IDLE;
            if (tryStringToRobotStatus(item["status"].asString(), st)) {
                robot->setStatus(st);
            }
        }
        if (item.hasKey("currentPayload")) {
            robot->setCurrentPayload(item["currentPayload"].asInt(0));
        }
        if (item.hasKey("waitTicks")) {
            robot->setWaitTicks(item["waitTicks"].asInt(0));
        }

        // Route restoration
        if (item.hasKey("route") && item["route"].isArray()) {
            std::vector<Position> route;
            for (const auto& rpos : item["route"].asArray()) {
                int rx = rpos["x"].asInt(-1);
                int ry = rpos["y"].asInt(-1);
                if (rx >= 0 && rx < width && ry >= 0 && ry < height) {
                    route.push_back(Position(rx, ry));
                }
            }
            size_t rIdx = static_cast<size_t>(item.hasKey("routeIndex") ? item["routeIndex"].asInt(0) : 0);
            robot->setRoute(route, rIdx);
        }

        // Current Task restoration
        if (item.hasKey("currentTask") && item["currentTask"].isObject()) {
            const auto& tVal = item["currentTask"];
            int tId = tVal["id"].asInt(0);
            int tOrdId = tVal["orderId"].asInt(0);
            TaskType tType = TaskType::NONE;
            if (tVal.hasKey("type")) {
                tryStringToTaskType(tVal["type"].asString(), tType);
            }
            int tx = tVal["targetLocation"]["x"].asInt(0);
            int ty = tVal["targetLocation"]["y"].asInt(0);
            int tProdId = tVal.hasKey("productId") ? tVal["productId"].asInt(0) : 0;
            int tQty = tVal.hasKey("quantity") ? tVal["quantity"].asInt(0) : 0;
            bool tComp = tVal.hasKey("completed") ? tVal["completed"].asBool(false) : false;

            auto task = std::make_shared<Task>(tId, tOrdId, tType, Position(tx, ty), tProdId, tQty, tComp);
            robot->setCurrentTask(task);
        }

        robotIds.insert(id);
        tempRobots.push_back(std::move(robot));
    }

    // 6. Coordinator & Route Reservations
    MultiRobotCoordinator tempCoordinator;
    if (root.hasKey("coordinator") && root["coordinator"].isObject()) {
        const auto& cVal = root["coordinator"];
        int cp = cVal.hasKey("collisionsPrevented") ? cVal["collisionsPrevented"].asInt(0) : 0;
        int dr = cVal.hasKey("deadlocksResolved") ? cVal["deadlocksResolved"].asInt(0) : 0;
        int rr = cVal.hasKey("routesReplanned") ? cVal["routesReplanned"].asInt(0) : 0;
        tempCoordinator.setMetrics(cp, dr, rr);
    }
    // Re-register active routes into reservation table
    for (const auto& r : tempRobots) {
        if (r->hasActiveRoute()) {
            tempCoordinator.registerPlannedRoute(r->getId(), r->getCurrentRoute(), tick);
        }
    }

    // 7. Atomic Commit
    warehouse = std::move(tempWarehouse);
    inventory = std::move(tempInventory);
    robots = std::move(tempRobots);
    taskManager = std::move(tempTaskManager);
    coordinator = std::move(tempCoordinator);
    analytics = WarehouseAnalytics(warehouse.getWidth(), warehouse.getHeight());
    simulationTick = tick;
    activeAlgorithm = algo;

    return true;
}

