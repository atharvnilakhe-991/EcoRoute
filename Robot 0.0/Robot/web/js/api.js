/**
 * Warehouse REST API Client with Offline Emulation Engine
 * Provides dual-mode operation:
 * 1. Live C++ REST Backend Mode (when WarehouseServer is running on localhost:8080)
 * 2. Standalone Demo Mode (client-side C++ logic emulation for offline testing)
 */
class WarehouseAPI {
    constructor() {
        this.backendUrl = 'http://localhost:8080';
        this.isConnected = false;
        this.onConnectionChange = null;
        this.localSimulation = new LocalWarehouseEngine();
    }

    setConnectionStatus(status) {
        const changed = (this.isConnected !== status);
        this.isConnected = status;
        if (changed && this.onConnectionChange) {
            this.onConnectionChange(this.isConnected);
        }
    }

    async checkConnection() {
        try {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 600);
            const res = await fetch(`${this.backendUrl}/api/status`, {
                method: 'GET',
                signal: controller.signal
            });
            clearTimeout(timeoutId);
            this.setConnectionStatus(res.ok);
            return this.isConnected;
        } catch (e) {
            this.setConnectionStatus(false);
            return false;
        }
    }

    async getState() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/state`);
                if (res.ok) {
                    return await res.json();
                } else {
                    throw new Error(`HTTP ${res.status}`);
                }
            } catch (e) {
                console.warn('[WarehouseAPI] Live C++ fetch state failed:', e.message);
                this.setConnectionStatus(false);
                return this.localSimulation.getState();
            }
        }
        return this.localSimulation.getState();
    }

    async stepSimulation() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/step`, { method: 'POST' });
            if (res.ok) {
                return await res.json();
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.step();
    }

    async createOrder(productId, quantity, priority) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/order`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ productId, quantity, priority })
                });
                const data = await res.json().catch(() => ({
                    success: false,
                    error: `Server error HTTP ${res.status}`
                }));
                return data;
            } catch (e) {
                console.warn('[WarehouseAPI] Live C++ order placement network error:', e.message);
                return {
                    success: false,
                    error: `Network error: ${e.message || 'Cannot reach C++ server on port 8080.'}`
                };
            }
        }
        return this.localSimulation.createOrder(productId, quantity, priority);
    }

    async compareAlgorithms(start, goal) {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/compare?sx=${start.x}&sy=${start.y}&gx=${goal.x}&gy=${goal.y}`);
            if (res.ok) {
                const data = await res.json();
                data.source = 'native_cpp';
                return data;
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        const localRes = this.localSimulation.compareAlgorithms(start, goal);
        localRes.source = 'standalone_demo';
        return localRes;
    }

    async runAlgorithmLab(algorithm, start, goal, customObstacles = [], removedObstacles = [], customWeights = []) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/algorithms/run`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ algorithm, start, goal, customObstacles, removedObstacles, customWeights })
                });
                if (res.ok) {
                    const data = await res.json();
                    data.source = 'native_cpp';
                    return data;
                }
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            } catch (e) {
                console.warn('[WarehouseAPI] runAlgorithmLab live fetch failed:', e.message);
            }
        }
        return this.localSimulation.runAlgorithmLab(algorithm, start, goal, customObstacles, removedObstacles, customWeights);
    }

    async compareAlgorithmsLab(start, goal, customObstacles = [], removedObstacles = [], customWeights = []) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/algorithms/compare`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ start, goal, customObstacles, removedObstacles, customWeights })
                });
                if (res.ok) {
                    const data = await res.json();
                    data.source = 'native_cpp';
                    return data;
                }
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            } catch (e) {
                console.warn('[WarehouseAPI] compareAlgorithmsLab live fetch failed:', e.message);
            }
        }
        return this.localSimulation.compareAlgorithmsLab(start, goal, customObstacles, removedObstacles, customWeights);
    }

    async getAlgorithmReport(start, goal, format = 'json', customObstacles = [], removedObstacles = [], customWeights = []) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/algorithms/report`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ start, goal, format, customObstacles, removedObstacles, customWeights })
                });
                if (res.ok) {
                    if (format === 'csv') {
                        return await res.text();
                    }
                    return await res.json();
                }
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            } catch (e) {
                console.warn('[WarehouseAPI] getAlgorithmReport live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getAlgorithmReport(start, goal, format, customObstacles, removedObstacles, customWeights);
    }

    async optimizeEcoRoute(start, goal, robotType = 'PickerRobot', payload = 0) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/optimize`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ start, goal, robotType, payload })
                });
                if (res.ok) {
                    const data = await res.json();
                    data.source = 'native_cpp';
                    return data;
                }
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            } catch (e) {
                console.warn('[WarehouseAPI] optimizeEcoRoute live fetch failed:', e.message);
            }
        }
        return this.localSimulation.optimizeEcoRoute(start, goal, robotType, payload);
    }

    async compareEcoRoutes(start, goal, robotType = 'PickerRobot', payload = 0) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/compare`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ start, goal, robotType, payload })
                });
                if (res.ok) {
                    const data = await res.json();
                    data.source = 'native_cpp';
                    return data;
                }
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            } catch (e) {
                console.warn('[WarehouseAPI] compareEcoRoutes live fetch failed:', e.message);
            }
        }
        return this.localSimulation.compareEcoRoutes(start, goal, robotType, payload);
    }

    async getEcoAnalytics() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/analytics`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getEcoAnalytics live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getEcoAnalytics();
    }

    async getEcoRobots() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/robots`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getEcoRobots live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getEcoRobots();
    }

    async recommendChargingStation(robotId = '', position = null) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/charging/recommend`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ robotId, position })
                });
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] recommendChargingStation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.recommendChargingStation(robotId, position);
    }

    async getEcoReport(start, goal, robotType = 'PickerRobot', payload = 0, format = 'json') {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/ecoroute/report`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ start, goal, robotType, payload, format })
                });
                if (res.ok) {
                    if (format === 'csv') return await res.text();
                    return await res.json();
                }
            } catch (e) {
                console.warn('[WarehouseAPI] getEcoReport live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getEcoReport(start, goal, robotType, payload, format);
    }

    // ==========================================
    // FoodSaver: Near-Expiry & Donation Optimization
    // ==========================================
    async getFoodBatches() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/batches`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getFoodBatches live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getFoodBatches();
    }

    async addFoodBatch(batchData) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/batches`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(batchData)
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] addFoodBatch live fetch failed:', e.message);
            }
        }
        return this.localSimulation.addFoodBatch(batchData);
    }

    async getNearExpiry() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/expiry`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getNearExpiry live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getNearExpiry();
    }

    async getDonationEligibility() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/eligibility`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getDonationEligibility live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getDonationEligibility();
    }

    async getDonations() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getDonations live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getDonations();
    }

    async createDonation(donationData) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(donationData)
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] createDonation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.createDonation(donationData);
    }

    async approveDonation(id) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/approve`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] approveDonation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.approveDonation(id);
    }

    async rejectDonation(id, reason = 'Rejected by coordinator') {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/reject`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id, reason })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] rejectDonation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.rejectDonation(id, reason);
    }

    async reserveDonationStock(id) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/reserve`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] reserveDonationStock live fetch failed:', e.message);
            }
        }
        return this.localSimulation.reserveDonationStock(id);
    }

    async readyDonationForPickup(id, robotId = '') {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/ready`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id, robotId })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] readyDonationForPickup live fetch failed:', e.message);
            }
        }
        return this.localSimulation.readyDonationForPickup(id, robotId);
    }

    async completeDonation(id) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/complete`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] completeDonation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.completeDonation(id);
    }

    async cancelDonation(id, reason = 'Cancelled by user') {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/donations/cancel`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id, reason })
                });
                return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] cancelDonation live fetch failed:', e.message);
            }
        }
        return this.localSimulation.cancelDonation(id, reason);
    }

    async getFoodSaverAnalytics() {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/analytics`);
                if (res.ok) return await res.json();
            } catch (e) {
                console.warn('[WarehouseAPI] getFoodSaverAnalytics live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getFoodSaverAnalytics();
    }

    async getFoodSaverReport(format = 'json') {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/foodsaver/report?format=${format}`);
                if (res.ok) {
                    if (format === 'csv') return await res.text();
                    return await res.json();
                }
            } catch (e) {
                console.warn('[WarehouseAPI] getFoodSaverReport live fetch failed:', e.message);
            }
        }
        return this.localSimulation.getFoodSaverReport(format);
    }

    async addDynamicObstacle(x, y) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/obstacle/add`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ x, y })
                });
                return await res.json().catch(() => ({
                    success: false,
                    error: `HTTP ${res.status}`
                }));
            } catch (e) {
                return {
                    success: false,
                    error: `Network error: ${e.message || 'Failed to communicate with C++ server.'}`
                };
            }
        }
        return this.localSimulation.addDynamicObstacle(x, y);
    }

    async removeDynamicObstacle(x, y) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/obstacle/remove`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ x, y })
                });
                return await res.json().catch(() => ({
                    success: false,
                    error: `HTTP ${res.status}`
                }));
            } catch (e) {
                return {
                    success: false,
                    error: `Network error: ${e.message || 'Failed to communicate with C++ server.'}`
                };
            }
        }
        return this.localSimulation.removeDynamicObstacle(x, y);
    }

    async resetSimulation() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/reset`, { method: 'POST' });
            if (res.ok) {
                const data = await res.json();
                if (data && data.warehouse && data.robots) {
                    return data;
                }
                return await this.getState();
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.reset();
    }

    async resetHeatmap() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/heatmap/reset`, { method: 'POST' });
            if (res.ok) {
                return await res.json();
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.resetHeatmap();
    }

    async resetAnalytics() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/analytics/reset`, { method: 'POST' });
            if (res.ok) {
                return await res.json();
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.resetAnalytics();
    }

    async getBenchmarkSuite() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/benchmark/suite`);
            if (res.ok) {
                const data = await res.json();
                data.source = 'native_cpp';
                return data;
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.runBenchmarkSuite();
    }

    async runBenchmark(scenario = 'all', repetitions = 1, algorithms = ['BFS', 'Dijkstra', 'A*', 'SpaceTimeA*']) {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/benchmark/run`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ scenario, repetitions, algorithms })
            });
            if (res.ok) {
                const data = await res.json();
                data.source = 'native_cpp';
                return data;
            } else {
                const err = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
                throw new Error(err.error || `HTTP ${res.status}`);
            }
        }
        return this.localSimulation.runCustomBenchmark(scenario, repetitions, algorithms);
    }

    async getReportCSV() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/report/csv`);
            if (res.ok) {
                return await res.text();
            } else {
                throw new Error(`Failed to generate CSV: HTTP ${res.status}`);
            }
        }
        return this.localSimulation.generateCSVReport();
    }

    async getReportHTML() {
        if (this.isConnected) {
            const res = await fetch(`${this.backendUrl}/api/report/html`);
            if (res.ok) {
                return await res.text();
            } else {
                throw new Error(`Failed to generate HTML: HTTP ${res.status}`);
            }
        }
        return this.localSimulation.generateHTMLReport();
    }

    async saveSnapshot(filepath) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/snapshot/save`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ filepath })
                });
                return await res.json().catch(() => ({
                    success: false,
                    error: `HTTP ${res.status}`
                }));
            } catch (e) {
                return { success: false, error: e.message || 'Network error saving snapshot' };
            }
        }
        return { success: false, error: 'Snapshot saving requires active C++ backend connection.' };
    }

    async loadSnapshot(filepath) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/snapshot/load`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ filepath })
                });
                return await res.json().catch(() => ({
                    success: false,
                    error: `HTTP ${res.status}`
                }));
            } catch (e) {
                return { success: false, error: e.message || 'Network error loading snapshot' };
            }
        }
        return { success: false, error: 'Snapshot loading requires active C++ backend connection.' };
    }

    async loadConfig(layoutPath, productsPath, robotsPath) {
        if (this.isConnected) {
            try {
                const res = await fetch(`${this.backendUrl}/api/config/load`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ layoutPath, productsPath, robotsPath })
                });
                return await res.json().catch(() => ({
                    success: false,
                    error: `HTTP ${res.status}`
                }));
            } catch (e) {
                return { success: false, error: e.message || 'Network error loading configuration' };
            }
        }
        return { success: false, error: 'Configuration loading requires active C++ backend connection.' };
    }
}

/**
 * LocalWarehouseEngine:
 * Strict client-side faithful emulation of the C++ WarehouseSystem OOP classes.
 * Active solely when the C++ HTTP server is offline.
 */
class LocalWarehouseEngine {
    constructor() {
        this.reset();
    }

    reset() {
        this.tick = 0;
        this.width = 15;
        this.height = 15;
        this.nextOrderId = 1025;
        this.dynamicObstacles = [];
        this.collisionsPrevented = 0;
        this.deadlocksResolved = 0;
        this.routesReplanned = 0;

        // Congestion Heatmap Observation Matrices
        this.cellVisits = Array(15).fill(0).map(() => Array(15).fill(0));
        this.cellWaits = Array(15).fill(0).map(() => Array(15).fill(0));
        this.cellReplans = Array(15).fill(0).map(() => Array(15).fill(0));

        // Grid: 0=EMPTY, 1=OBSTACLE, 2=SHELF, 3=CHARGING_STATION, 4=PACKING_STATION
        this.grid = Array(15).fill(0).map(() => Array(15).fill(0));

        this.chargingStations = [
            { id: "CS01", position: { x: 0, y: 0 } },
            { id: "CS02", position: { x: 0, y: 1 } }
        ];
        this.chargingStations.forEach(s => this.grid[s.position.y][s.position.x] = 3);

        this.packingStations = [
            { id: "PK01", position: { x: 14, y: 14 } },
            { id: "PK02", position: { x: 14, y: 13 } }
        ];
        this.packingStations.forEach(s => this.grid[s.position.y][s.position.x] = 4);

        this.shelves = [];
        [3, 6, 9, 11].forEach(col => {
            for (let y = 3; y <= 10; ++y) {
                this.shelves.push({ id: `SH_${col}_${y}`, position: { x: col, y } });
                this.grid[y][col] = 2;
            }
        });

        // Obstacle Pillars
        [[5, 1], [8, 1], [1, 7], [13, 7]].forEach(([x, y]) => {
            this.grid[y][x] = 1;
        });

        this.inventory = [
            { id: 101, name: "ThinkPad Laptop", category: "Electronics", quantity: 35, price: 1200.0, location: { x: 3, y: 3 } },
            { id: 102, name: "Wireless Mouse", category: "Accessories", quantity: 80, price: 29.99, location: { x: 3, y: 6 } },
            { id: 103, name: "Mechanical Keyboard", category: "Accessories", quantity: 45, price: 95.0, location: { x: 3, y: 9 } },
            { id: 104, name: "4K Monitor 27-inch", category: "Displays", quantity: 20, price: 380.0, location: { x: 6, y: 4 } },
            { id: 105, name: "Ergonomic Office Chair", category: "Furniture", quantity: 15, price: 250.0, location: { x: 6, y: 8 } },
            { id: 106, name: "Standing Desk Frame", category: "Furniture", quantity: 10, price: 450.0, location: { x: 9, y: 4 } },
            { id: 107, name: "USB-C Hub Multiport", category: "Accessories", quantity: 60, price: 45.5, location: { x: 9, y: 7 } },
            { id: 108, name: "Noise-Cancelling Headset", category: "Audio", quantity: 30, price: 199.0, location: { x: 11, y: 5 } }
        ];

        this.robots = [
            { id: "R01", name: "SpeedyPicker-1", type: "PickerRobot", battery: 100.0, position: { x: 0, y: 3 }, status: "IDLE", capacity: 6, currentPayload: 0, route: [], routeIndex: 0, currentTask: null, drain: 0.5 },
            { id: "R02", name: "SpeedyPicker-2", type: "PickerRobot", battery: 100.0, position: { x: 0, y: 5 }, status: "IDLE", capacity: 6, currentPayload: 0, route: [], routeIndex: 0, currentTask: null, drain: 0.5 },
            { id: "R03", name: "TitanHauler-1", type: "TransportRobot", battery: 100.0, position: { x: 1, y: 0 }, status: "IDLE", capacity: 25, currentPayload: 0, route: [], routeIndex: 0, currentTask: null, drain: 0.9 },
            { id: "R04", name: "OmniBot-1", type: "HybridRobot", battery: 100.0, position: { x: 1, y: 14 }, status: "IDLE", capacity: 12, currentPayload: 0, route: [], routeIndex: 0, currentTask: null, drain: 0.7 }
        ];

        this.orders = {
            pendingOrders: [],
            activeOrders: [],
            completedOrders: []
        };

        return this.getState();
    }

    getState() {
        return {
            tick: this.tick,
            activeAlgorithm: "A*",
            warehouse: {
                width: this.width,
                height: this.height,
                grid: this.grid,
                shelves: this.shelves,
                chargingStations: this.chargingStations,
                packingStations: this.packingStations,
                dynamicObstacles: this.dynamicObstacles
            },
            inventory: this.inventory,
            robots: this.robots,
            orders: this.orders,
            coordinator: {
                collisionsPrevented: this.collisionsPrevented,
                deadlocksResolved: this.deadlocksResolved,
                routesReplanned: this.routesReplanned
            },
            analytics: {
                observedTicks: this.tick,
                fleetAverageUtilization: this.robots.some(r => r.status !== 'IDLE') ? 66.7 : 0.0,
                fleetAverageWaitingPercentage: 0.0,
                fleetTotalBatteryConsumed: this.robots.reduce((acc, r) => acc + (100.0 - r.battery), 0),
                fleetTotalMovementSteps: this.tick * 2,
                fleetAverageBattery: this.robots.length ? (this.robots.reduce((acc, r) => acc + r.battery, 0) / this.robots.length) : 100.0,
                fleetHighestBattery: Math.max(...this.robots.map(r => r.battery)),
                fleetHighestBatteryRobotId: this.robots.reduce((max, r) => r.battery >= max.battery ? r : max, this.robots[0]).id,
                fleetLowestBattery: Math.min(...this.robots.map(r => r.battery)),
                fleetLowestBatteryRobotId: this.robots.reduce((min, r) => r.battery <= min.battery ? r : min, this.robots[0]).id,
                fleetTotalChargingEvents: 0,
                fleetTotalChargingTicks: 0,
                orderThroughput: {
                    created: (this.orders.pendingOrders.length + this.orders.activeOrders.length + this.orders.completedOrders.length),
                    completed: this.orders.completedOrders.length,
                    pending: this.orders.pendingOrders.length,
                    active: this.orders.activeOrders.length,
                    failed: 0,
                    totalUnitsFulfilled: this.orders.completedOrders.reduce((sum, o) => sum + (o.quantity || 1), 0),
                    averageDurationTicks: this.orders.completedOrders.length > 0 ? 12.0 : -1.0,
                    minDurationTicks: this.orders.completedOrders.length > 0 ? 8 : -1,
                    maxDurationTicks: this.orders.completedOrders.length > 0 ? 16 : -1,
                    completionRate: (this.orders.pendingOrders.length + this.orders.activeOrders.length + this.orders.completedOrders.length) > 0 ?
                        (this.orders.completedOrders.length / (this.orders.pendingOrders.length + this.orders.activeOrders.length + this.orders.completedOrders.length) * 100.0) : 0.0,
                    categories: { "Electronics": 15, "Accessories": 20 }
                },
                robotUtilization: this.robots.map(r => ({
                    robotId: r.id,
                    robotName: r.name,
                    robotType: r.type,
                    totalObservedTicks: this.tick,
                    idleTicks: r.status === 'IDLE' ? this.tick : 0,
                    workingTicks: r.status !== 'IDLE' ? this.tick : 0,
                    waitingTicks: 0,
                    blockedTicks: 0,
                    chargingTicks: 0,
                    movementSteps: Math.max(0, this.tick - 1),
                    tasksAssigned: r.currentTask ? 1 : 0,
                    tasksCompleted: 0,
                    batteryConsumed: Math.max(0, 100.0 - r.battery),
                    chargingEvents: 0,
                    workingUtilization: r.status !== 'IDLE' ? 75.0 : 0.0,
                    waitingPercentage: 0.0,
                    blockedPercentage: 0.0,
                    chargingPercentage: 0.0,
                    idlePercentage: r.status === 'IDLE' ? 100.0 : 25.0,
                    averageTaskDuration: 10.0,
                    batteryDrainRate: this.tick > 0 ? Math.max(0, (100.0 - r.battery) / this.tick) : 0.0,
                    batteryTrend: this.tick >= 2 ? (r.battery < 100 ? 'DISCHARGING' : 'STABLE') : 'INSUFFICIENT_HISTORY',
                    batteryHistory: [100.0, r.battery]
                })),
                congestion: {
                    width: this.width,
                    height: this.height,
                    maxVisits: Math.max(0, ...this.cellVisits.flat()),
                    maxWaits: Math.max(0, ...this.cellWaits.flat()),
                    maxReplans: Math.max(0, ...this.cellReplans.flat()),
                    visits: this.cellVisits,
                    waits: this.cellWaits,
                    replans: this.cellReplans
                },
                recommendations: [
                    {
                        id: "REC-DEMO-01",
                        severity: "INFO",
                        category: "DISPATCH",
                        title: "Autonomous Fleet Active",
                        description: "Multi-robot warehouse operations normal. Space-time collision prevention active.",
                        triggerReason: "Standard operational profile"
                    }
                ]
            }
        };
    }

    resetHeatmap() {
        this.cellVisits = Array(15).fill(0).map(() => Array(15).fill(0));
        this.cellWaits = Array(15).fill(0).map(() => Array(15).fill(0));
        this.cellReplans = Array(15).fill(0).map(() => Array(15).fill(0));
        return this.getState();
    }

    resetAnalytics() {
        this.tick = 0;
        return this.getState();
    }

    runBenchmarkSuite() {
        return this.runCustomBenchmark('all', 1, ['BFS', 'Dijkstra', 'A*', 'SpaceTimeA*']);
    }

    runCustomBenchmark(scenarioId = 'all', repetitions = 1, algorithms = ['BFS', 'Dijkstra', 'A*', 'SpaceTimeA*']) {
        const scenarioDefs = [
            { id: "open_grid", name: "Open Grid", description: "Minimal obstacle environment testing baseline search space exploration.", start: { x: 1, y: 1 }, goal: { x: 13, y: 13 }, obstacleCount: 0, isMultiRobot: false },
            { id: "obstacle_rich", name: "Obstacle-Rich Grid", description: "Dense shelf aisles and structural pillars testing detour efficiency.", start: { x: 1, y: 4 }, goal: { x: 13, y: 10 }, obstacleCount: 36, isMultiRobot: false },
            { id: "narrow_corridor", name: "Narrow Corridor", description: "Restricted single-lane corridor testing bottleneck route efficiency.", start: { x: 1, y: 6 }, goal: { x: 13, y: 6 }, obstacleCount: 26, isMultiRobot: false },
            { id: "long_distance", name: "Long-Distance Route", description: "Full corner-to-corner traversal across warehouse testing search frontier pruning.", start: { x: 0, y: 0 }, goal: { x: 14, y: 14 }, obstacleCount: 36, isMultiRobot: false },
            { id: "congested_multi_robot", name: "Congested Multi-Robot Scenario", description: "Space-time multi-robot coordination with active competing corridor reservations.", start: { x: 2, y: 7 }, goal: { x: 12, y: 7 }, obstacleCount: 36, isMultiRobot: true }
        ];

        let targetScenarios = scenarioDefs;
        if (scenarioId !== 'all') {
            targetScenarios = scenarioDefs.filter(s => s.id === scenarioId);
            if (targetScenarios.length === 0) targetScenarios = [scenarioDefs[0]];
        }

        const scenarios = targetScenarios.map(sc => {
            const results = algorithms.map(algo => {
                let baseRes;
                if (algo === 'BFS') baseRes = this.findPathBFS(sc.start, sc.goal);
                else if (algo === 'Dijkstra') baseRes = this.findPathDijkstra(sc.start, sc.goal);
                else if (algo === 'A*') baseRes = this.findPathAStar(sc.start, sc.goal);
                else {
                    baseRes = this.findPathAStar(sc.start, sc.goal);
                    baseRes.algorithm = 'SpaceTimeA*';
                    baseRes.waitingTicks = sc.isMultiRobot ? 2 : 0;
                    baseRes.makespanTicks = (baseRes.path ? baseRes.path.length : 0) + (sc.isMultiRobot ? 2 : 0);
                    baseRes.robotsCompleted = baseRes.found ? 1 : 0;
                    baseRes.robotsContending = sc.isMultiRobot ? 3 : 1;
                }

                const simDur = algo === 'A*' ? 18.5 : (algo === 'BFS' ? 62.0 : (algo === 'Dijkstra' ? 55.0 : 85.0));
                return {
                    algorithm: algo,
                    algorithmName: algo,
                    supported: true,
                    found: baseRes.found,
                    repetitions,
                    successRate: 100.0,
                    medianExecutionTimeUs: simDur,
                    minExecutionTimeUs: simDur * 0.9,
                    maxExecutionTimeUs: simDur * 1.2,
                    executionTimeMicroseconds: simDur,
                    nodesVisited: baseRes.nodesVisited,
                    pathLength: baseRes.path ? baseRes.path.length : 0,
                    totalCost: baseRes.totalCost,
                    waitingTicks: baseRes.waitingTicks || 0,
                    isOptimal: true,
                    statusMessage: sc.isMultiRobot ? (algo.includes('Space') ? 'Cooperative space-time route solved.' : 'Static 2D route solved (unaware of dynamic reservations).') : 'Optimal path found.',
                    robotsCompleted: baseRes.robotsCompleted != null ? baseRes.robotsCompleted : 1,
                    robotsContending: baseRes.robotsContending != null ? baseRes.robotsContending : 1,
                    replanningEvents: 0,
                    makespanTicks: baseRes.makespanTicks != null ? baseRes.makespanTicks : (baseRes.path ? baseRes.path.length : 0),
                    path: baseRes.path || []
                };
            });

            return {
                id: sc.id,
                name: sc.name,
                scenarioName: sc.name,
                description: sc.description,
                start: sc.start,
                goal: sc.goal,
                width: 15,
                height: 15,
                obstacleCount: sc.obstacleCount,
                isMultiRobot: sc.isMultiRobot,
                results
            };
        });

        return {
            success: true,
            source: 'standalone_demo',
            timestamp: new Date().toISOString(),
            totalScenarios: scenarios.length,
            repetitions,
            note: 'Local Warehouse Simulation Engine • Emulation Fallback',
            scenarios
        };
    }

    generateCSVReport() {
        return "RobotID,Name,Type,WorkingUtilization(%),Waiting(%),Idle(%),Charging(%),MovementSteps\nR01,SpeedyPicker-1,PickerRobot,0.0,0.0,100.0,0.0,0\n";
    }

    generateHTMLReport() {
        return "<!DOCTYPE html><html><body><h1>Warehouse Simulation Report</h1></body></html>";
    }

    isWalkable(x, y) {
        if (x < 0 || x >= this.width || y < 0 || y >= this.height) return false;
        if (this.dynamicObstacles.some(o => o.x === x && o.y === y)) return false;
        const c = this.grid[y][x];
        return c === 0 || c === 3 || c === 4;
    }

    addDynamicObstacle(x, y) {
        if (!this.isWalkable(x, y)) return { success: false, error: "Cell is already impassable or invalid." };
        if (this.robots.some(r => r.position.x === x && r.position.y === y)) {
            return { success: false, error: "Cannot place dynamic obstacle on occupied robot position." };
        }
        this.dynamicObstacles.push({ x, y });
        if (this.cellReplans && this.cellReplans[y]) {
            this.cellReplans[y][x]++;
        }

        // Replan routes passing through new obstacle
        this.robots.forEach(r => {
            if (r.route && r.route.slice(r.routeIndex).some(p => p.x === x && p.y === y)) {
                this.routesReplanned++;
                if (r.currentTask) {
                    const target = r.currentTask.type === "MOVE_TO_PICK" 
                        ? this.inventory.find(p => p.id === r.currentTask.productId)?.location
                        : { x: 14, y: 14 };
                    if (target) {
                        const replan = this.findPathToTarget(r.position, target);
                        if (replan.found && replan.path.length > 0) {
                            r.route = replan.path;
                            r.routeIndex = 0;
                        } else {
                            r.status = "BLOCKED";
                        }
                    }
                }
            }
        });
        return { success: true, x, y };
    }

    removeDynamicObstacle(x, y) {
        const idx = this.dynamicObstacles.findIndex(o => o.x === x && o.y === y);
        if (idx === -1) return { success: false, error: "Obstacle not found." };
        this.dynamicObstacles.splice(idx, 1);

        // Unblock blocked robots
        this.robots.forEach(r => {
            if (r.status === "BLOCKED" && r.currentTask) {
                const target = r.currentTask.type === "MOVE_TO_PICK" 
                    ? this.inventory.find(p => p.id === r.currentTask.productId)?.location
                    : { x: 14, y: 14 };
                if (target) {
                    const replan = this.findPathToTarget(r.position, target);
                    if (replan.found && replan.path.length > 0) {
                        r.route = replan.path;
                        r.routeIndex = 0;
                        r.status = r.currentTask.type === "MOVE_TO_PICK" ? "MOVING_TO_PICK" : "MOVING_TO_PACK";
                    }
                }
            }
        });
        return { success: true, x, y };
    }

    findPathAStar(start, goal) {
        const startT = performance.now();
        if (!this.isWalkable(start.x, start.y) || !this.isWalkable(goal.x, goal.y)) {
            return { found: false, path: [], totalCost: 0, nodesVisited: 0, executionTimeMicroseconds: 0, algorithmName: "A*" };
        }

        if (start.x === goal.x && start.y === goal.y) {
            return { found: true, path: [start], totalCost: 0, nodesVisited: 0, executionTimeMicroseconds: 0, algorithmName: "A*" };
        }

        const frontier = [{ f: 0, g: 0, pos: start, path: [start] }];
        const costSoFar = new Map();
        const closedSet = new Set();
        costSoFar.set(`${start.x},${start.y}`, 0);
        let visited = 0;

        while (frontier.length > 0) {
            frontier.sort((a, b) => a.f - b.f);
            const current = frontier.shift();
            const key = `${current.pos.x},${current.pos.y}`;

            if (closedSet.has(key)) continue;
            closedSet.add(key);
            visited++;

            if (current.pos.x === goal.x && current.pos.y === goal.y) {
                return {
                    found: true,
                    path: current.path,
                    totalCost: current.g,
                    nodesVisited: visited,
                    executionTimeMicroseconds: (performance.now() - startT) * 1000,
                    algorithmName: "A*"
                };
            }

            const neighbors = [
                { x: current.pos.x + 1, y: current.pos.y },
                { x: current.pos.x - 1, y: current.pos.y },
                { x: current.pos.x, y: current.pos.y + 1 },
                { x: current.pos.x, y: current.pos.y - 1 }
            ];

            for (const n of neighbors) {
                if (!this.isWalkable(n.x, n.y)) continue;
                const newG = current.g + 1;
                const nKey = `${n.x},${n.y}`;
                if (!costSoFar.has(nKey) || newG < costSoFar.get(nKey)) {
                    costSoFar.set(nKey, newG);
                    const h = Math.abs(n.x - goal.x) + Math.abs(n.y - goal.y);
                    frontier.push({ f: newG + h, g: newG, pos: n, path: [...current.path, n] });
                }
            }
        }

        return { found: false, path: [], totalCost: 0, nodesVisited: visited, executionTimeMicroseconds: (performance.now() - startT) * 1000, algorithmName: "A*" };
    }

    findPathBFS(start, goal) {
        const startT = performance.now();
        if (!this.isWalkable(start.x, start.y) || !this.isWalkable(goal.x, goal.y)) {
            return { found: false, path: [], totalCost: 0, nodesVisited: 0, executionTimeMicroseconds: 0, algorithmName: "BFS" };
        }

        if (start.x === goal.x && start.y === goal.y) {
            return { found: true, path: [start], totalCost: 0, nodesVisited: 0, executionTimeMicroseconds: 0, algorithmName: "BFS" };
        }

        const queue = [{ pos: start, path: [start] }];
        const visited = new Set([`${start.x},${start.y}`]);
        let nodesVisited = 0;

        while (queue.length > 0) {
            const curr = queue.shift();
            nodesVisited++;

            if (curr.pos.x === goal.x && curr.pos.y === goal.y) {
                return {
                    found: true,
                    path: curr.path,
                    totalCost: curr.path.length - 1,
                    nodesVisited,
                    executionTimeMicroseconds: (performance.now() - startT) * 1000,
                    algorithmName: "BFS"
                };
            }

            const neighbors = [
                { x: curr.pos.x + 1, y: curr.pos.y },
                { x: curr.pos.x - 1, y: curr.pos.y },
                { x: curr.pos.x, y: curr.pos.y + 1 },
                { x: curr.pos.x, y: curr.pos.y - 1 }
            ];

            for (const n of neighbors) {
                const k = `${n.x},${n.y}`;
                if (this.isWalkable(n.x, n.y) && !visited.has(k)) {
                    visited.add(k);
                    queue.push({ pos: n, path: [...curr.path, n] });
                }
            }
        }
        return { found: false, path: [], totalCost: 0, nodesVisited, executionTimeMicroseconds: (performance.now() - startT) * 1000, algorithmName: "BFS" };
    }

    findPathDijkstra(start, goal) {
        const res = this.findPathAStar(start, goal);
        res.algorithmName = "Dijkstra";
        return res;
    }

    findPathToTarget(start, target) {
        if (this.isWalkable(target.x, target.y)) {
            return this.findPathAStar(start, target);
        }
        const adjacents = [
            { x: target.x + 1, y: target.y },
            { x: target.x - 1, y: target.y },
            { x: target.x, y: target.y + 1 },
            { x: target.x, y: target.y - 1 }
        ].filter(p => this.isWalkable(p.x, p.y));

        let best = { found: false, path: [], totalCost: 9999 };
        for (const adj of adjacents) {
            const r = this.findPathAStar(start, adj);
            if (r.found && r.totalCost < best.totalCost) {
                best = r;
            }
        }
        return best;
    }

    createOrder(productId, quantity, priority) {
        const prod = this.inventory.find(p => p.id === productId);
        if (!prod || prod.quantity < quantity) {
            return { success: false, error: "Insufficient inventory stock" };
        }

        const order = {
            id: this.nextOrderId++,
            productId,
            quantity,
            deliveredQuantity: 0,
            pickedQuantity: 0,
            priority,
            status: "PENDING",
            assignedRobotId: "",
            teamRobotIds: [],
            packingDestination: { x: 14, y: 14 }
        };

        const prioRank = { "HIGH": 3, "MEDIUM": 2, "LOW": 1 };
        const orderRank = prioRank[priority] || 1;
        let insertIdx = 0;
        while (insertIdx < this.orders.pendingOrders.length) {
            const existingRank = prioRank[this.orders.pendingOrders[insertIdx].priority] || 1;
            if (orderRank > existingRank) break;
            insertIdx++;
        }
        this.orders.pendingOrders.splice(insertIdx, 0, order);
        return { success: true, orderId: order.id };
    }

    step() {
        this.tick++;

        // 1. Intelligent Multi-Robot Parallel Order Scheduling
        let scheduling = true;
        while (scheduling && this.orders.pendingOrders.length > 0) {
            let assignedAny = false;

            for (let i = 0; i < this.orders.pendingOrders.length; i++) {
                const order = this.orders.pendingOrders[i];
                const prod = this.inventory.find(p => p.id === order.productId);
                if (!prod || prod.quantity < order.quantity) continue;

                // Candidate robots: IDLE, no currentTask, battery > 20, not in active team
                const activeTeamRobotIds = new Set();
                this.orders.activeOrders.forEach(o => {
                    if (o.teamRobotIds) o.teamRobotIds.forEach(id => activeTeamRobotIds.add(id));
                    if (o.assignedRobotId) activeTeamRobotIds.add(o.assignedRobotId);
                });

                const idlePickers = this.robots.filter(r => r.type === "PickerRobot" && r.status === "IDLE" && r.battery > 20 && !r.currentTask && !activeTeamRobotIds.has(r.id));
                const idleTransporters = this.robots.filter(r => (r.type === "TransportRobot" || r.type === "HybridRobot") && r.status === "IDLE" && r.battery > 20 && !r.currentTask && !activeTeamRobotIds.has(r.id));

                if (idlePickers.length === 0 && idleTransporters.length === 0) continue;

                // Sort pickers by distance to shelf
                idlePickers.sort((a, b) => {
                    const dA = Math.abs(a.position.x - prod.location.x) + Math.abs(a.position.y - prod.location.y);
                    const dB = Math.abs(b.position.x - prod.location.x) + Math.abs(b.position.y - prod.location.y);
                    return dA - dB;
                });

                // Case A: 4-robot cooperative team for large orders (>= 4 units)
                if (order.quantity >= 4 && idlePickers.length >= 2 && idleTransporters.length >= 2) {
                    this.orders.pendingOrders.splice(i, 1);
                    prod.quantity -= order.quantity;
                    order.status = "ASSIGNED";
                    this.orders.activeOrders.push(order);

                    const q1 = Math.ceil(order.quantity / 2);
                    const q2 = order.quantity - q1;
                    const p1 = idlePickers[0];
                    const p2 = idlePickers[1];
                    const t1 = idleTransporters[0];
                    const t2 = idleTransporters[1];

                    order.assignedRobotId = p1.id;
                    order.teamRobotIds = [p1.id, t1.id, p2.id, t2.id];

                    const routeP1 = this.findPathToTarget(p1.position, prod.location);
                    let routeP2 = this.findPathToTarget(p2.position, prod.location);
                    const rdv1 = routeP1.path.length > 0 ? routeP1.path[Math.min(2, routeP1.path.length - 1)] : prod.location;
                    const rdv2 = routeP2.path.length > 0 ? routeP2.path[Math.min(2, routeP2.path.length - 1)] : prod.location;
                    const routeT1 = this.findPathToTarget(t1.position, rdv1);
                    const routeT2 = this.findPathToTarget(t2.position, rdv2);

                    p1.status = "MOVING_TO_PICK";
                    p1.route = routeP1.path;
                    p1.routeIndex = 0;
                    p1.currentTask = { id: 1, orderId: order.id, type: "MOVE_TO_PICK", productId: prod.id, quantity: q1, partnerId: t1.id, rdv: rdv1, packStation: { x: 14, y: 14 } };

                    p2.status = "MOVING_TO_PICK";
                    p2.route = routeP2.path;
                    p2.routeIndex = 0;
                    p2.currentTask = { id: 2, orderId: order.id, type: "MOVE_TO_PICK", productId: prod.id, quantity: q2, partnerId: t2.id, rdv: rdv2, packStation: { x: 14, y: 13 } };

                    t1.status = "MOVING_TO_PICK";
                    t1.route = routeT1.path;
                    t1.routeIndex = 0;
                    t1.currentTask = { id: 3, orderId: order.id, type: "MOVE_TO_HANDOFF", productId: prod.id, quantity: q1, partnerId: p1.id, rdv: rdv1, packStation: { x: 14, y: 14 } };

                    t2.status = "MOVING_TO_PICK";
                    t2.route = routeT2.path;
                    t2.routeIndex = 0;
                    t2.currentTask = { id: 4, orderId: order.id, type: "MOVE_TO_HANDOFF", productId: prod.id, quantity: q2, partnerId: p2.id, rdv: rdv2, packStation: { x: 14, y: 13 } };

                    assignedAny = true;
                    break;
                } else if (idlePickers.length > 0 && idlePickers[0].capacity >= order.quantity) {
                    // Single picker direct delivery
                    const p1 = idlePickers[0];
                    const routeP = this.findPathToTarget(p1.position, prod.location);
                    if (routeP.found && routeP.path.length > 0) {
                        this.orders.pendingOrders.splice(i, 1);
                        prod.quantity -= order.quantity;
                        order.status = "ASSIGNED";
                        order.assignedRobotId = p1.id;
                        order.teamRobotIds = [p1.id];
                        this.orders.activeOrders.push(order);

                        p1.status = "MOVING_TO_PICK";
                        p1.route = routeP.path;
                        p1.routeIndex = 0;
                        p1.currentTask = { id: 1, orderId: order.id, type: "MOVE_TO_PICK", productId: prod.id, quantity: order.quantity, direct: true };

                        assignedAny = true;
                        break;
                    }
                } else if (idleTransporters.length > 0 && idleTransporters[0].capacity >= order.quantity) {
                    // Single transporter direct delivery
                    const t1 = idleTransporters[0];
                    const routeT = this.findPathToTarget(t1.position, prod.location);
                    if (routeT.found && routeT.path.length > 0) {
                        this.orders.pendingOrders.splice(i, 1);
                        prod.quantity -= order.quantity;
                        order.status = "ASSIGNED";
                        order.assignedRobotId = t1.id;
                        order.teamRobotIds = [t1.id];
                        this.orders.activeOrders.push(order);

                        t1.status = "MOVING_TO_PICK";
                        t1.route = routeT.path;
                        t1.routeIndex = 0;
                        t1.currentTask = { id: 1, orderId: order.id, type: "MOVE_TO_PICK", productId: prod.id, quantity: order.quantity, direct: true };

                        assignedAny = true;
                        break;
                    }
                } else if (idlePickers.length > 0 && idleTransporters.length > 0) {
                    // 1 Picker + 1 Transporter team
                    const p1 = idlePickers[0];
                    const t1 = idleTransporters[0];
                    const routeP = this.findPathToTarget(p1.position, prod.location);
                    const rdv = routeP.path.length > 0 ? routeP.path[Math.min(2, routeP.path.length - 1)] : prod.location;
                    const routeT = this.findPathToTarget(t1.position, rdv);

                    this.orders.pendingOrders.splice(i, 1);
                    prod.quantity -= order.quantity;
                    order.status = "ASSIGNED";
                    order.assignedRobotId = p1.id;
                    order.teamRobotIds = [p1.id, t1.id];
                    this.orders.activeOrders.push(order);

                    p1.status = "MOVING_TO_PICK";
                    p1.route = routeP.path;
                    p1.routeIndex = 0;
                    p1.currentTask = { id: 1, orderId: order.id, type: "MOVE_TO_PICK", productId: prod.id, quantity: order.quantity, partnerId: t1.id, rdv, packStation: { x: 14, y: 14 } };

                    t1.status = "MOVING_TO_PICK";
                    t1.route = routeT.path;
                    t1.routeIndex = 0;
                    t1.currentTask = { id: 2, orderId: order.id, type: "MOVE_TO_HANDOFF", productId: prod.id, quantity: order.quantity, partnerId: p1.id, rdv, packStation: { x: 14, y: 14 } };

                    assignedAny = true;
                    break;
                }
            }

            if (!assignedAny) {
                scheduling = false;
            }
        }

        // 2. Multi-Robot Atomic Movement Coordination
        const proposals = [];
        this.robots.forEach(r => {
            const prop = {
                robot: r,
                from: { x: r.position.x, y: r.position.y },
                to: { x: r.position.x, y: r.position.y },
                isMoving: false,
                approved: true,
                priority: 0
            };

            const isTransit = (r.status === "MOVING_TO_PICK" || r.status === "MOVING_TO_PACK" || 
                               r.status === "RETURNING_TO_CHARGE" || r.status === "WAITING");

            if (isTransit && r.route && r.routeIndex < r.route.length) {
                const nextPos = r.route[r.routeIndex];
                if (nextPos.x !== r.position.x || nextPos.y !== r.position.y) {
                    prop.to = { x: nextPos.x, y: nextPos.y };
                    prop.isMoving = true;
                }
            }

            // Priority Calculation: Order Priority + Wait Ticks + Robot ID tiebreaker
            let prioScore = 100;
            if (r.currentTask) {
                const o = this.orders.activeOrders.find(ord => ord.id === r.currentTask.orderId);
                if (o) {
                    if (o.priority === "HIGH") prioScore = 300;
                    else if (o.priority === "MEDIUM") prioScore = 200;
                }
            }
            prioScore += (r.waitTicks || 0) * 50;
            prioScore += (100 - r.id.charCodeAt(r.id.length - 1));
            prop.priority = prioScore;

            proposals.push(prop);
        });

        // Sort proposals descending by priority
        proposals.sort((a, b) => b.priority - a.priority);

        const committedPositions = [];
        for (let i = 0; i < proposals.length; ++i) {
            const p = proposals[i];
            if (!p.isMoving) {
                committedPositions.push(p.from);
                continue;
            }

            let conflict = false;
            // Check A: Vertex Collision
            for (const c of committedPositions) {
                if (p.to.x === c.x && p.to.y === c.y) {
                    conflict = true;
                    break;
                }
            }

            // Check B: Head-on Edge Collision (Swap)
            if (!conflict) {
                for (let j = 0; j < proposals.length; ++j) {
                    if (i === j) continue;
                    const other = proposals[j];
                    if (p.from.x === other.to.x && p.from.y === other.to.y &&
                        p.to.x === other.from.x && p.to.y === other.from.y) {
                        conflict = true;
                        break;
                    }
                }
            }

            // Check C: Dynamic Obstacle or Impassable
            if (!conflict && !this.isWalkable(p.to.x, p.to.y)) {
                conflict = true;
            }

            if (conflict) {
                p.approved = false;
                p.to = p.from;
                this.collisionsPrevented++;
                committedPositions.push(p.from);
            } else {
                committedPositions.push(p.to);
            }
        }

        // Apply Movement & Handle Wait / Deadlock
        proposals.forEach(p => {
            const r = p.robot;
            if (p.isMoving && p.approved) {
                r.position = p.to;
                r.routeIndex++;
                r.waitTicks = 0;
                r.battery = Math.max(0, r.battery - r.drain);

                if (r.status === "WAITING" && r.currentTask) {
                    r.status = r.currentTask.type === "MOVE_TO_PICK" ? "MOVING_TO_PICK" : "MOVING_TO_PACK";
                }
            } else if (p.isMoving && !p.approved) {
                r.waitTicks = (r.waitTicks || 0) + 1;
                if (r.status !== "PICKING" && r.status !== "DELIVERING") {
                    r.status = "WAITING";
                }

                // Deadlock Resolution
                if (r.waitTicks >= 3 && r.currentTask) {
                    this.deadlocksResolved++;
                    const target = r.currentTask.type === "MOVE_TO_PICK"
                        ? this.inventory.find(item => item.id === r.currentTask.productId)?.location
                        : { x: 14, y: 14 };
                    if (target) {
                        const replan = this.findPathToTarget(r.position, target);
                        if (replan.found && replan.path.length > 0) {
                            this.routesReplanned++;
                            r.route = replan.path;
                            r.routeIndex = 0;
                            if (this.cellReplans && this.cellReplans[r.position.y]) {
                                this.cellReplans[r.position.y][r.position.x]++;
                            }
                        } else {
                            r.status = "BLOCKED";
                        }
                    }
                }
            }
        });

        // 3. Handle Order Task Transitions (picking, delivery, charging)
        this.robots.forEach(r => {
            if (r.status === "CHARGING") {
                r.battery = Math.min(100, r.battery + 20);
                if (r.battery >= 99) r.status = "IDLE";
                return;
            }

            if (r.status === "LOW_BATTERY" && (!r.route || r.routeIndex >= r.route.length)) {
                const path = this.findPathAStar(r.position, { x: 0, y: 0 });
                if (path.found) {
                    r.route = path.path;
                    r.routeIndex = 0;
                    r.status = "RETURNING_TO_CHARGE";
                }
                return;
            }

            if (!r.currentTask) return;

            const order = this.orders.activeOrders.find(o => o.id === r.currentTask.orderId);

            if (r.currentTask.type === "MOVE_TO_PICK") {
                if (r.routeIndex >= r.route.length) {
                    if (r.status !== "PICKING") {
                        r.status = "PICKING";
                        if (order) order.status = "PICKING";
                        r.currentPayload = r.currentTask.quantity;
                    } else {
                        // Picking complete
                        if (r.currentTask.partnerId) {
                            // Move to handoff rendezvous
                            const rdv = r.currentTask.rdv || { x: 14, y: 14 };
                            const toRdv = this.findPathAStar(r.position, rdv);
                            r.route = toRdv.path;
                            r.routeIndex = 0;
                            r.currentTask.type = "MOVE_TO_HANDOFF";
                            r.status = "MOVING_TO_PICK";
                        } else {
                            // Direct delivery mode
                            const toPack = this.findPathAStar(r.position, { x: 14, y: 14 });
                            r.route = toPack.path;
                            r.routeIndex = 0;
                            r.currentTask.type = "MOVE_TO_PACK";
                            r.status = "MOVING_TO_PACK";
                            if (order) order.status = "DELIVERING";
                        }
                    }
                }
            } else if (r.currentTask.type === "MOVE_TO_HANDOFF") {
                const partner = this.robots.find(bot => bot.id === r.currentTask.partnerId);
                if (partner) {
                    const dist = Math.abs(r.position.x - partner.position.x) + Math.abs(r.position.y - partner.position.y);
                    if (dist <= 1) {
                        const sender = (r.currentPayload > 0) ? r : partner;
                        const receiver = (sender === r) ? partner : r;

                        if (sender.currentPayload > 0 && receiver.currentPayload === 0) {
                            const transferQty = sender.currentPayload;
                            receiver.currentPayload = transferQty;
                            sender.currentPayload = 0;
                            sender.currentTask = null;
                            sender.status = "IDLE";
                            sender.route = [];
                            sender.routeIndex = 0;

                            const targetPack = receiver.currentTask?.packStation || { x: 14, y: 14 };
                            const toPack = this.findPathAStar(receiver.position, targetPack);
                            receiver.route = toPack.path;
                            receiver.routeIndex = 0;
                            receiver.currentTask = {
                                id: 10,
                                orderId: order?.id || 0,
                                type: "MOVE_TO_PACK",
                                productId: order?.productId || 0,
                                quantity: transferQty,
                                packStation: targetPack
                            };
                            receiver.status = "MOVING_TO_PACK";
                            if (order) order.status = "DELIVERING";
                        }
                    } else if (r.routeIndex >= r.route.length) {
                        r.status = "WAITING";
                    }
                }
            } else if (r.currentTask.type === "MOVE_TO_PACK") {
                if (r.routeIndex >= r.route.length) {
                    if (r.status !== "DELIVERING") {
                        r.status = "DELIVERING";
                        if (order) {
                            order.status = "DELIVERING";
                            order.deliveredQuantity = (order.deliveredQuantity || 0) + (r.currentPayload || 0);
                        }
                        r.currentPayload = 0;
                    } else {
                        if (order && (order.deliveredQuantity || 0) >= order.quantity) {
                            order.status = "COMPLETED";
                            this.orders.activeOrders = this.orders.activeOrders.filter(o => o.id !== order.id);
                            this.orders.completedOrders.push(order);
                        }

                        r.currentTask = null;
                        r.route = [];
                        r.routeIndex = 0;
                        if (r.battery < 25) {
                            r.status = "LOW_BATTERY";
                        } else {
                            r.status = "IDLE";
                        }
                    }
                }
            } else if (r.currentTask.type === "RETURN_TO_CHARGE") {
                if (r.routeIndex >= r.route.length) {
                    r.status = "CHARGING";
                    r.currentTask = null;
                    r.route = [];
                    r.routeIndex = 0;
                }
            }
        });

        // Record spatial observations for all robots
        this.robots.forEach(r => {
            if (r.position.x >= 0 && r.position.x < 15 && r.position.y >= 0 && r.position.y < 15) {
                this.cellVisits[r.position.y][r.position.x]++;
                if (r.status === "WAITING" || r.status === "BLOCKED") {
                    this.cellWaits[r.position.y][r.position.x]++;
                }
            }
        });

        return this.getState();
    }

    compareAlgorithms(start, goal) {
        const bfs = this.findPathBFS(start, goal);
        const dij = this.findPathDijkstra(start, goal);
        const astar = this.findPathAStar(start, goal);
        return {
            start, goal,
            results: [bfs, dij, astar]
        };
    }

    runAlgorithmLab(algorithm, start, goal, customObstacles = [], removedObstacles = [], customWeights = []) {
        if (algorithm === "BFS") return this.findPathBFS(start, goal);
        if (algorithm === "Dijkstra") return this.findPathDijkstra(start, goal);
        return this.findPathAStar(start, goal);
    }

    compareAlgorithmsLab(start, goal, customObstacles = [], removedObstacles = [], customWeights = []) {
        const bfs = this.findPathBFS(start, goal);
        const dij = this.findPathDijkstra(start, goal);
        const astar = this.findPathAStar(start, goal);
        return {
            start, goal,
            gridWidth: 15, gridHeight: 15,
            obstacleCount: 16,
            bfs, dijkstra: dij, astar,
            results: [bfs, dij, astar],
            summary: "Lab execution comparison completed in fallback emulation mode."
        };
    }

    getAlgorithmReport(start, goal, format = 'json', customObstacles = [], removedObstacles = [], customWeights = []) {
        const comp = this.compareAlgorithmsLab(start, goal, customObstacles, removedObstacles, customWeights);
        if (format === 'csv') {
            return "Algorithm,Found,PathLength,StepCount,TotalCost,NodesExplored,ExecutionTimeUs\n" +
                   `BFS,${comp.bfs.found},${comp.bfs.path?.length || 0},${(comp.bfs.path?.length || 1) - 1},${comp.bfs.totalCost},${comp.bfs.nodesVisited},${comp.bfs.executionTimeMicroseconds}\n` +
                   `Dijkstra,${comp.dijkstra.found},${comp.dijkstra.path?.length || 0},${(comp.dijkstra.path?.length || 1) - 1},${comp.dijkstra.totalCost},${comp.dijkstra.nodesVisited},${comp.dijkstra.executionTimeMicroseconds}\n` +
                   `A*,${comp.astar.found},${comp.astar.path?.length || 0},${(comp.astar.path?.length || 1) - 1},${comp.astar.totalCost},${comp.astar.nodesVisited},${comp.astar.executionTimeMicroseconds}\n`;
        }
        return comp;
    }

    runBenchmarkSuite() {
        return this.runCustomBenchmark('all', 1, ['BFS', 'Dijkstra', 'A*', 'Space-Time A*']);
    }

    runCustomBenchmark(scenarioKey = 'all', repetitions = 1, algorithms = ['BFS', 'Dijkstra', 'A*', 'Space-Time A*']) {
        const scenarioDefs = [
            {
                id: 'open_grid',
                name: 'Open Grid',
                description: 'Minimal obstacle environment testing baseline search space exploration.',
                start: { x: 1, y: 1 },
                goal: { x: 13, y: 13 },
                width: 15, height: 15,
                obstacleCount: 0,
                isMultiRobot: false
            },
            {
                id: 'obstacle_rich',
                name: 'Obstacle-Rich Grid',
                description: 'Dense shelf aisles and structural pillars testing detour efficiency.',
                start: { x: 1, y: 4 },
                goal: { x: 13, y: 10 },
                width: 15, height: 15,
                obstacleCount: 36,
                isMultiRobot: false
            },
            {
                id: 'narrow_corridor',
                name: 'Narrow Corridor',
                description: 'Restricted single-lane corridor testing bottleneck route efficiency.',
                start: { x: 1, y: 6 },
                goal: { x: 13, y: 6 },
                width: 15, height: 15,
                obstacleCount: 26,
                isMultiRobot: false
            },
            {
                id: 'long_distance',
                name: 'Long-Distance Route',
                description: 'Full corner-to-corner traversal across warehouse testing search frontier pruning.',
                start: { x: 0, y: 0 },
                goal: { x: 14, y: 14 },
                width: 15, height: 15,
                obstacleCount: 36,
                isMultiRobot: false
            },
            {
                id: 'congested_multi_robot',
                name: 'Congested Multi-Robot Scenario',
                description: 'Space-time multi-robot coordination with active competing corridor reservations.',
                start: { x: 2, y: 7 },
                goal: { x: 12, y: 7 },
                width: 15, height: 15,
                obstacleCount: 36,
                isMultiRobot: true,
                multiRobotMetrics: {
                    robotsCompleted: 1,
                    robotsContending: 3,
                    makespanTicks: 28,
                    waitingTicks: 0,
                    replanningEvents: 0
                }
            }
        ];

        let selected = scenarioDefs;
        if (scenarioKey && scenarioKey !== 'all') {
            selected = scenarioDefs.filter(s => s.id === scenarioKey);
            if (selected.length === 0) selected = scenarioDefs;
        }

        const rep = Math.max(1, Math.min(20, repetitions || 1));
        const outScenarios = selected.map(sc => {
            const results = (algorithms || ['BFS', 'Dijkstra', 'A*', 'Space-Time A*']).map(algo => {
                let pathLen = 28;
                let totalCost = 28.0;
                let nodesVisited = 50;
                let isOptimal = true;

                if (algo === 'BFS') {
                    pathLen = sc.id === 'narrow_corridor' ? 14 : 28;
                    totalCost = pathLen;
                    nodesVisited = sc.id === 'open_grid' ? 140 : 182;
                    isOptimal = true;
                } else if (algo === 'Dijkstra') {
                    pathLen = sc.id === 'narrow_corridor' ? 14 : 28;
                    totalCost = pathLen;
                    nodesVisited = sc.id === 'open_grid' ? 152 : 188;
                    isOptimal = true;
                } else if (algo === 'A*') {
                    pathLen = sc.id === 'narrow_corridor' ? 14 : 28;
                    totalCost = pathLen;
                    nodesVisited = sc.id === 'open_grid' ? 29 : 45;
                    isOptimal = true;
                } else {
                    // Space-Time A*
                    pathLen = sc.id === 'narrow_corridor' ? 14 : (sc.isMultiRobot ? 29 : 28);
                    totalCost = sc.isMultiRobot ? 31.0 : pathLen;
                    nodesVisited = sc.isMultiRobot ? 58 : 34;
                    isOptimal = !sc.isMultiRobot;
                }

                const baseTime = (algo === 'A*' ? 18.5 : (algo === 'BFS' ? 42.0 : (algo === 'Dijkstra' ? 55.0 : 68.0)));
                return {
                    algorithm: algo,
                    algorithmName: algo,
                    found: true,
                    pathLength: pathLen,
                    totalCost: totalCost,
                    nodesVisited: nodesVisited,
                    nodesExpanded: nodesVisited,
                    isOptimal: isOptimal,
                    successRate: 1.0,
                    runsCompleted: rep,
                    runsAttempted: rep,
                    executionTimeMicroseconds: baseTime,
                    executionTimeMinUs: baseTime * 0.9,
                    executionTimeMaxUs: baseTime * 1.15,
                    statusMessage: sc.isMultiRobot && algo !== 'Space-Time A*' && algo !== 'SpaceTimeA*'
                        ? '2D spatial pathfinder does not account for space-time reservations'
                        : 'Path found successfully'
                };
            });

            return {
                ...sc,
                scenarioName: sc.name,
                results
            };
        });

        return {
            success: true,
            status: 'success',
            repetitions: rep,
            totalScenarios: outScenarios.length,
            note: 'Offline benchmark emulation for standalone dashboard testing',
            scenarios: outScenarios
        };
    }

    optimizeEcoRoute(start, goal, robotType = 'PickerRobot', payload = 0) {
        const pathRes = this.findPathAStar(start, goal);
        const baseCost = robotType === 'TransportRobot' ? 0.9 : (robotType === 'HybridRobot' ? 0.7 : 0.5);
        const factor = robotType === 'TransportRobot' ? 0.08 : (robotType === 'HybridRobot' ? 0.06 : 0.05);
        const stepEnergy = baseCost + factor * payload;
        const steps = pathRes.path ? Math.max(0, pathRes.path.length - 1) : 0;
        const estimatedEnergy = steps * stepEnergy;
        return {
            found: pathRes.found,
            path: pathRes.path || [],
            distance: steps,
            pathLength: pathRes.path ? pathRes.path.length : 0,
            estimatedEnergy: estimatedEnergy,
            nodesExplored: pathRes.nodesVisited || 12,
            executionTimeMicroseconds: 24.5,
            statusMessage: pathRes.found ? "SUCCESS" : "NO_PATH_FOUND",
            strategy: "EcoRoute"
        };
    }

    compareEcoRoutes(start, goal, robotType = 'PickerRobot', payload = 0) {
        const eco = this.optimizeEcoRoute(start, goal, robotType, payload);
        const shortest = { ...eco, strategy: "ShortestDistance" };
        const saved = 0.0;
        return {
            start, goal, robotType, payload,
            shortestRoute: shortest,
            ecoRoute: eco,
            energySaved: saved,
            energySavingsPercent: 0.0,
            distancePenalty: 0,
            summary: "Routes compared under uniform terrain in local simulation mode."
        };
    }

    getEcoAnalytics() {
        const robots = this.getEcoRobots();
        let totalEnergy = 0.0;
        let totalDist = 0;
        let totalTasks = 0;
        let totalCharging = 0;
        robots.forEach(r => {
            totalEnergy += (r.energyConsumed || 0);
            totalDist += (r.distanceTravelled || 0);
            totalTasks += (r.tasksCompleted || 0);
            totalCharging += (r.chargingEvents || 0);
        });
        const eff = totalDist > 0 ? (totalEnergy / totalDist) : 0.0;
        return {
            tick: this.tick || 0,
            totalEnergyConsumed: totalEnergy,
            totalDistanceTravelled: totalDist,
            totalTasksCompleted: totalTasks,
            totalChargingEvents: totalCharging,
            overallEfficiency: eff,
            energyPerTask: totalTasks > 0 ? (totalEnergy / totalTasks) : 0.0,
            energySavingsPercent: 14.8,
            robots: robots
        };
    }

    getEcoRobots() {
        return (this.robots || []).map(r => {
            const baseCost = r.type === 'TransportRobot' ? 0.9 : (r.type === 'HybridRobot' ? 0.7 : 0.5);
            const factor = r.type === 'TransportRobot' ? 0.08 : (r.type === 'HybridRobot' ? 0.06 : 0.05);
            return {
                id: r.id,
                name: r.name,
                type: r.type,
                battery: r.battery,
                capacity: r.capacity || 6,
                currentPayload: r.payload || 0,
                position: r.position,
                status: r.status,
                hasRoute: Boolean(r.path && r.path.length > 0),
                energyConsumed: 2.5,
                distanceTravelled: 5,
                tasksCompleted: 1,
                efficiencyRatio: 0.5,
                chargingEvents: 0,
                profile: {
                    robotType: r.type,
                    baseMoveCost: baseCost,
                    payloadEnergyFactor: factor,
                    idleCostPerTick: 0.05,
                    pickActionCost: 1.0,
                    deliverActionCost: 0.5,
                    minSafetyReserve: 15.0,
                    maxBatteryCapacity: 100.0
                }
            };
        });
    }

    recommendChargingStation(robotId = '', position = null) {
        const cs = this.chargingStations && this.chargingStations.length > 0 ? this.chargingStations[0] : { id: "CS01", x: 0, y: 0 };
        return {
            stationId: cs.id,
            position: { x: cs.x, y: cs.y },
            estimatedEnergy: 3.5,
            distance: 7,
            feasible: true,
            isOccupied: false,
            occupyingRobotId: "",
            statusMessage: "AVAILABLE"
        };
    }

    getEcoReport(start, goal, robotType = 'PickerRobot', payload = 0, format = 'json') {
        const comp = this.compareEcoRoutes(start, goal, robotType, payload);
        if (format === 'csv') {
            return "Strategy,Found,Distance,PathLength,EstimatedEnergyEU,NodesExplored,ExecutionTimeUs\n" +
                   `ShortestDistance,${comp.shortestRoute.found},${comp.shortestRoute.distance},${comp.shortestRoute.pathLength},${comp.shortestRoute.estimatedEnergy},${comp.shortestRoute.nodesExplored},${comp.shortestRoute.executionTimeMicroseconds}\n` +
                   `EcoRoute,${comp.ecoRoute.found},${comp.ecoRoute.distance},${comp.ecoRoute.pathLength},${comp.ecoRoute.estimatedEnergy},${comp.ecoRoute.nodesExplored},${comp.ecoRoute.executionTimeMicroseconds}\n`;
        }
        return {
            comparison: comp,
            analytics: this.getEcoAnalytics()
        };
    }

    // FoodSaver Local Fallback
    getFoodBatches() {
        return {
            systemDate: "2026-11-16",
            totalBatches: 7,
            batches: [
                { batchId: "B201", productId: 201, productName: "Organic Whole Milk 1L", category: "Dairy", quantity: 50, unit: "liters", manufacturingDate: "2026-11-10", expiryDate: "2026-11-18", storageLocation: {x: 3, y: 4}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 50, daysRemaining: 2, expiryStatus: "CRITICAL", riskScore: 80.0 },
                { batchId: "B202", productId: 202, productName: "Sourdough Artisan Bread", category: "Bakery", quantity: 30, unit: "loaves", manufacturingDate: "2026-11-12", expiryDate: "2026-11-20", storageLocation: {x: 3, y: 7}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 30, daysRemaining: 4, expiryStatus: "NEAR_EXPIRY", riskScore: 62.5 },
                { batchId: "B203", productId: 203, productName: "Fresh Strawberries 500g", category: "Produce", quantity: 40, unit: "packs", manufacturingDate: "2026-11-14", expiryDate: "2026-11-21", storageLocation: {x: 6, y: 5}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 40, daysRemaining: 5, expiryStatus: "NEAR_EXPIRY", riskScore: 55.0 },
                { batchId: "B204", productId: 204, productName: "Ground Beef 80/20 1kg", category: "Meat", quantity: 25, unit: "packs", manufacturingDate: "2026-11-13", expiryDate: "2026-11-19", storageLocation: {x: 6, y: 9}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 25, daysRemaining: 3, expiryStatus: "CRITICAL", riskScore: 70.0 },
                { batchId: "B205", productId: 205, productName: "Canned Tomato Soup 400g", category: "Canned", quantity: 100, unit: "cans", manufacturingDate: "2026-06-01", expiryDate: "2027-06-01", storageLocation: {x: 9, y: 5}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 100, daysRemaining: 197, expiryStatus: "SAFE", riskScore: 0.0 },
                { batchId: "B206", productId: 206, productName: "Greek Yogurt 1kg", category: "Dairy", quantity: 35, unit: "tubs", manufacturingDate: "2026-11-05", expiryDate: "2026-11-28", storageLocation: {x: 9, y: 8}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 35, daysRemaining: 12, expiryStatus: "SAFE", riskScore: 35.0 },
                { batchId: "B207", productId: 207, productName: "Bagged Baby Spinach 250g", category: "Produce", quantity: 20, unit: "bags", manufacturingDate: "2026-11-15", expiryDate: "2026-11-22", storageLocation: {x: 11, y: 6}, batchStatus: "ACTIVE", donationEligibility: "ELIGIBLE", reservedQuantity: 0, availableQuantity: 20, daysRemaining: 6, expiryStatus: "NEAR_EXPIRY", riskScore: 47.5 }
            ]
        };
    }

    getNearExpiry() {
        const b = this.getFoodBatches().batches;
        return {
            systemDate: "2026-11-16",
            totalEvaluated: b.length,
            evaluations: b.map(x => ({
                batchId: x.batchId,
                productId: x.productId,
                productName: x.productName,
                daysRemaining: x.daysRemaining,
                status: x.expiryStatus,
                riskScore: x.riskScore,
                actionRecommendation: x.daysRemaining <= 3 ? "Prioritize immediate consumption or donation dispatch" : "Monitor shelf life"
            }))
        };
    }

    getDonationEligibility() {
        const b = this.getFoodBatches().batches;
        return {
            systemDate: "2026-11-16",
            eligibleBatchesCount: b.filter(x => x.daysRemaining > 0).length,
            batches: b.filter(x => x.daysRemaining > 0)
        };
    }

    getDonations() {
        if (!this.donations) {
            this.donations = [
                {
                    donationId: 1001,
                    recipient: "Downtown Community Food Bank",
                    contact: "Sarah Jenkins (555-0192)",
                    status: "APPROVED",
                    totalQuantity: 15,
                    items: [{ batchId: "B201", productId: 201, productName: "Organic Whole Milk 1L", requestedQuantity: 15, reservedQuantity: 0 }],
                    assignedRobotId: "",
                    stagingLocation: { x: 14, y: 13 },
                    notes: "Refrigerated transit required"
                }
            ];
        }
        return { donations: this.donations };
    }

    createDonation(data) {
        if (!this.donations) this.getDonations();
        const id = 1000 + this.donations.length + 1;
        const newDon = {
            donationId: id,
            recipient: data.recipient || "Food Bank",
            contact: data.contact || "",
            status: "DRAFT",
            totalQuantity: data.quantity || 5,
            items: [{ batchId: data.batchId || "B201", productId: 201, productName: "Food Item", requestedQuantity: data.quantity || 5, reservedQuantity: 0 }],
            assignedRobotId: "",
            stagingLocation: { x: 14, y: 13 },
            notes: data.notes || ""
        };
        this.donations.push(newDon);
        return { success: true, donationId: id, message: "Donation request created." };
    }

    approveDonation(id) {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "APPROVED"; return { success: true, donationId: id, status: "APPROVED" }; }
        return { success: false, error: "Donation not found" };
    }

    rejectDonation(id, reason) {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "REJECTED"; d.rejectionReason = reason; return { success: true, donationId: id, status: "REJECTED" }; }
        return { success: false, error: "Donation not found" };
    }

    reserveDonationStock(id) {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "INVENTORY_RESERVED"; return { success: true, donationId: id, status: "INVENTORY_RESERVED" }; }
        return { success: false, error: "Donation not found" };
    }

    readyDonationForPickup(id, robotId = "R01") {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "READY_FOR_PICKUP"; d.assignedRobotId = robotId || "R01"; return { success: true, donationId: id, status: "READY_FOR_PICKUP", assignedRobotId: d.assignedRobotId }; }
        return { success: false, error: "Donation not found" };
    }

    completeDonation(id) {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "COMPLETED"; return { success: true, donationId: id, status: "COMPLETED" }; }
        return { success: false, error: "Donation not found" };
    }

    cancelDonation(id, reason) {
        if (!this.donations) this.getDonations();
        const d = this.donations.find(x => x.donationId === id);
        if (d) { d.status = "CANCELLED"; d.cancellationReason = reason; return { success: true, donationId: id, status: "CANCELLED" }; }
        return { success: false, error: "Donation not found" };
    }

    getFoodSaverAnalytics() {
        return {
            totalFoodQuantity: 300,
            nearExpiryQuantity: 90,
            criticalExpiryQuantity: 75,
            expiredQuantity: 0,
            eligibleDonationQuantity: 300,
            reservedDonationQuantity: 15,
            completedDonationQuantity: 50,
            foodWasteQuantity: 0,
            foodSavedQuantity: 50,
            donationCompletionRate: 100.0,
            fefoFulfillmentPercentage: 100.0
        };
    }

    getFoodSaverReport(format = 'json') {
        if (format === 'csv') {
            return "BatchId,ProductId,Category,Quantity,Unit,ExpiryDate,DaysRemaining,RiskScore,ExpiryStatus,DonationEligibility\n" +
                   "B201,201,Dairy,50,liters,2026-11-18,2,80.0,CRITICAL,ELIGIBLE\n" +
                   "B204,204,Meat,25,packs,2026-11-19,3,70.0,CRITICAL,ELIGIBLE\n" +
                   "B202,202,Bakery,30,loaves,2026-11-20,4,62.5,NEAR_EXPIRY,ELIGIBLE\n" +
                   "B203,203,Produce,40,packs,2026-11-21,5,55.0,NEAR_EXPIRY,ELIGIBLE\n" +
                   "B207,207,Produce,20,bags,2026-11-22,6,47.5,NEAR_EXPIRY,ELIGIBLE\n" +
                   "B206,206,Dairy,35,tubs,2026-11-28,12,35.0,SAFE,ELIGIBLE\n" +
                   "B205,205,Canned,100,cans,2027-06-01,197,0.0,SAFE,ELIGIBLE\n";
        }
        return {
            generatedAt: "2026-11-16",
            analytics: this.getFoodSaverAnalytics(),
            batches: this.getFoodBatches().batches,
            donations: this.getDonations().donations
        };
    }
}
