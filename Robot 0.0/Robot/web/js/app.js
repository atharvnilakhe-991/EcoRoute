document.addEventListener('DOMContentLoaded', async () => {
    const api = new WarehouseAPI();
    const canvas = new WarehouseCanvas('warehouseCanvas', 'cellTooltip');
    const algoCanvasInstance = (typeof AlgorithmCanvas !== 'undefined') ? new AlgorithmCanvas('algoCanvas', 'algoTooltip') : null;
    const ecoCanvasInstance = (typeof EcoRouteCanvas !== 'undefined') ? new EcoRouteCanvas('ecoCanvas', 'ecoTooltip') : null;

    // UI Header & Status Elements
    const connectionBadge = document.getElementById('connectionBadge');
    const tickDisplay = document.getElementById('tickCounter');

    // Main KPI Elements
    const mainKpiGrid = document.querySelector('.kpi-grid');
    const kpiActiveOrders = document.getElementById('kpiActiveOrders');
    const kpiCompletedOrders = document.getElementById('kpiCompletedOrders');
    const kpiIdleRobots = document.getElementById('kpiIdleRobots');
    const kpiAvgBattery = document.getElementById('kpiAvgBattery');

    // Workspace & View Containers
    const mainWorkspaceGrid = document.querySelector('.workspace-grid');
    const overviewDeck = document.querySelector('.control-deck:not(.warehouse-ops-deck)');
    const warehouseOpsDeck = document.getElementById('warehouseOpsDeck');
    const viewFleet = document.getElementById('viewFleet');
    const viewOrders = document.getElementById('viewOrders');
    const viewInventory = document.getElementById('viewInventory');
    const viewAlgorithms = document.getElementById('viewAlgorithms');
    const viewEcoRoute = document.getElementById('viewEcoRoute');
    const viewFoodSaver = document.getElementById('viewFoodSaver');
    const allViews = [viewFleet, viewOrders, viewInventory, viewAlgorithms, viewEcoRoute, viewFoodSaver];

    // Overview Analytics Elements
    const overviewAnalyticsSection = document.getElementById('overviewAnalyticsSection');
    const analyticsObservedTicks = document.getElementById('analyticsObservedTicks');
    const analyticsTickSubtitle = document.getElementById('analyticsTickSubtitle');
    const analyticsFleetUtil = document.getElementById('analyticsFleetUtil');
    const analyticsFleetUtilSub = document.getElementById('analyticsFleetUtilSub');
    const analyticsAvgDuration = document.getElementById('analyticsAvgDuration');
    const analyticsDurationRange = document.getElementById('analyticsDurationRange');
    const analyticsThroughput = document.getElementById('analyticsThroughput');
    const analyticsCompletionRate = document.getElementById('analyticsCompletionRate');
    const analyticsTotalBattConsumed = document.getElementById('analyticsTotalBattConsumed');
    const analyticsLowestRobot = document.getElementById('analyticsLowestRobot');
    const analyticsCoordSummary = document.getElementById('analyticsCoordSummary');
    const analyticsCoordSub = document.getElementById('analyticsCoordSub');
    const utilizationRowsContainer = document.getElementById('utilizationRowsContainer');
    const recommendationsContainer = document.getElementById('recommendationsContainer');
    const recommendationsCountBadge = document.getElementById('recommendationsCountBadge');
    const categoryFulfillmentList = document.getElementById('categoryFulfillmentList');

    const btnDownloadCSVReport = document.getElementById('btnDownloadCSVReport');
    const btnViewHTMLReport = document.getElementById('btnViewHTMLReport');
    const btnRunBenchmarkSuite = document.getElementById('btnRunBenchmarkSuite');
    const btnSaveSnapshot = document.getElementById('btnSaveSnapshot');
    const btnLoadSnapshot = document.getElementById('btnLoadSnapshot');
    const btnLoadConfig = document.getElementById('btnLoadConfig');
    const btnResetAnalyticsData = document.getElementById('btnResetAnalyticsData');
    const analyticsFeedbackBanner = document.getElementById('analyticsFeedbackBanner');

    const benchmarkSuiteModal = document.getElementById('benchmarkSuiteModal');
    const btnBenchmarkModalClose = document.getElementById('btnBenchmarkModalClose');
    const benchmarkSuiteBody = document.getElementById('benchmarkSuiteBody');
    const benchmarkSuiteSubtext = document.getElementById('benchmarkSuiteSubtext');
    const benchmarkScenarioSelect = document.getElementById('benchmarkScenarioSelect');
    const benchmarkRepetitionsSelect = document.getElementById('benchmarkRepetitionsSelect');
    const chkAlgoBFS = document.getElementById('chkAlgoBFS');
    const chkAlgoDijkstra = document.getElementById('chkAlgoDijkstra');
    const chkAlgoAStar = document.getElementById('chkAlgoAStar');
    const chkAlgoSTAStar = document.getElementById('chkAlgoSTAStar');
    const btnExecuteBenchmark = document.getElementById('btnExecuteBenchmark');
    const btnOpenBenchmarkSuiteModal = document.getElementById('btnOpenBenchmarkSuiteModal');

    // Navigation Pills
    const navPills = document.querySelectorAll('.nav-pill');
    let currentView = 'overview';

    // Simulation Controls (Warehouse & Overview)
    const btnPlayPause = document.getElementById('btnPlayPause');
    const btnStep = document.getElementById('btnStep');
    const btnReset = document.getElementById('btnReset');
    const btnToggleObstacleMode = document.getElementById('btnToggleObstacleMode');
    const obstacleModeBadge = document.getElementById('obstacleModeBadge');
    const speedSlider = document.getElementById('speedSlider');
    const speedLabel = document.getElementById('speedLabel');

    let isObstacleMode = false;
    let isPlaying = false;
    let playIntervalId = null;
    let tickSpeedMs = 400;
    let selectedCell = null;

    // Overview Tab Switching (within overviewDeck)
    const tabButtons = document.querySelectorAll('.tab-button');
    const tabPanes = document.querySelectorAll('.tab-pane');

    // Overview Order Form
    const productSelect = document.getElementById('productSelect');
    const quantityInput = document.getElementById('quantityInput');
    const prioBtns = document.querySelectorAll('#tabDispatch .prio-btn');
    const btnDispatchOrder = document.getElementById('btnDispatchOrder');
    const overviewOrderFeedback = document.getElementById('overviewOrderFeedback');
    let selectedPriority = 'HIGH';

    // Warehouse Ops Deck Elements
    const warehouseProductSelect = document.getElementById('warehouseProductSelect');
    const warehouseQuantityInput = document.getElementById('warehouseQuantityInput');
    const warehouseStockHelp = document.getElementById('warehouseStockHelp');
    const warehousePrioBtns = document.querySelectorAll('#warehousePrioGroup .prio-btn');
    const btnWarehouseDispatchOrder = document.getElementById('btnWarehouseDispatchOrder');
    const warehouseOrderFeedback = document.getElementById('warehouseOrderFeedback');

    const warehouseObstacleStatusTag = document.getElementById('warehouseObstacleStatusTag');
    const inspectorCellCoord = document.getElementById('inspectorCellCoord');
    const inspectorCellType = document.getElementById('inspectorCellType');
    const btnCellPlaceRemoveObstacle = document.getElementById('btnCellPlaceRemoveObstacle');
    const btnWarehouseToggleObstacleMode = document.getElementById('btnWarehouseToggleObstacleMode');
    const btnClearAllObstacles = document.getElementById('btnClearAllObstacles');
    const obstacleFeedbackBanner = document.getElementById('obstacleFeedbackBanner');
    let warehouseSelectedPriority = 'HIGH';

    // Fleet View Elements
    const fleetKpiTotal = document.getElementById('fleetKpiTotal');
    const fleetKpiIdle = document.getElementById('fleetKpiIdle');
    const fleetKpiWorking = document.getElementById('fleetKpiWorking');
    const fleetKpiWaiting = document.getElementById('fleetKpiWaiting');
    const fleetKpiBlocked = document.getElementById('fleetKpiBlocked');
    const fleetKpiAvgBatt = document.getElementById('fleetKpiAvgBatt');

    const fleetFilterType = document.getElementById('fleetFilterType');
    const fleetFilterStatus = document.getElementById('fleetFilterStatus');
    const fleetFilterBattery = document.getElementById('fleetFilterBattery');
    const fleetSortBy = document.getElementById('fleetSortBy');
    const fleetSearchInput = document.getElementById('fleetSearchInput');
    const btnResetFleetFilters = document.getElementById('btnResetFleetFilters');
    const fleetCardsGrid = document.getElementById('fleetCardsGrid');
    const fleetLiveSyncBadge = document.getElementById('fleetLiveSyncBadge');

    // Fleet Stage E Analytics Elements
    const fleetAnalyticsObservedTicks = document.getElementById('fleetAnalyticsObservedTicks');
    const fleetUtilizationComparisonTable = document.getElementById('fleetUtilizationComparisonTable');
    const fleetBatteryHealthBadge = document.getElementById('fleetBatteryHealthBadge');
    const fleetBattRangeVal = document.getElementById('fleetBattRangeVal');
    const fleetBattConsumedVal = document.getElementById('fleetBattConsumedVal');
    const fleetChargingEventsVal = document.getElementById('fleetChargingEventsVal');
    const fleetBatteryRadarList = document.getElementById('fleetBatteryRadarList');

    // Stage F Heatmap Controls Elements
    const btnToggleHeatmap = document.getElementById('btnToggleHeatmap');
    const heatmapToggleText = document.getElementById('heatmapToggleText');
    const heatmapIcon = document.getElementById('heatmapIcon');
    const heatmapModeGroup = document.getElementById('heatmapModeGroup');
    const heatmapModeBtns = document.querySelectorAll('.heatmap-mode-btn');
    const heatmapLegendWrap = document.getElementById('heatmapLegendWrap');
    const heatmapLegendBar = document.getElementById('heatmapLegendBar');
    const heatmapMaxLabel = document.getElementById('heatmapMaxLabel');
    const btnResetHeatmap = document.getElementById('btnResetHeatmap');
    const heatmapFeedbackBanner = document.getElementById('heatmapFeedbackBanner');
    const heatmapDetailPanel = document.getElementById('heatmapDetailPanel');

    const inspectorDynamicStatus = document.getElementById('inspectorDynamicStatus');
    const inspectorVisitsVal = document.getElementById('inspectorVisitsVal');
    const inspectorWaitsVal = document.getElementById('inspectorWaitsVal');
    const inspectorReplansVal = document.getElementById('inspectorReplansVal');

    let isHeatmapActive = false;
    let currentHeatmapMode = 'visits'; // 'visits' | 'waits' | 'replans'

    // Robot Details Modal Elements
    const robotDetailsModal = document.getElementById('robotDetailsModal');
    const btnModalClose = document.getElementById('btnModalClose');
    const modalRobotAvatar = document.getElementById('modalRobotAvatar');
    const modalRobotName = document.getElementById('modalRobotName');
    const modalRobotSubtext = document.getElementById('modalRobotSubtext');
    const modalRobotBody = document.getElementById('modalRobotBody');

    // Orders View Elements
    const ordersViewProductSelect = document.getElementById('ordersViewProductSelect');
    const ordersViewQuantityInput = document.getElementById('ordersViewQuantityInput');
    const ordersViewStockHelp = document.getElementById('ordersViewStockHelp');
    const ordersViewPrioBtns = document.querySelectorAll('#ordersViewPrioGroup .prio-btn');
    const btnOrdersViewDispatch = document.getElementById('btnOrdersViewDispatch');
    const ordersViewFeedback = document.getElementById('ordersViewFeedback');
    const ordersViewActiveList = document.getElementById('ordersViewActiveList');
    const ordersViewCompletedList = document.getElementById('ordersViewCompletedList');
    let ordersViewSelectedPriority = 'HIGH';

    // Inventory View Elements
    const inventorySearchInput = document.getElementById('inventorySearchInput');
    const inventoryViewTableBody = document.getElementById('inventoryViewTableBody');

    // Algorithms View Elements (DSA Algorithm Laboratory)
    const algoStartX = document.getElementById('algoStartX');
    const algoStartY = document.getElementById('algoStartY');
    const algoGoalX = document.getElementById('algoGoalX');
    const algoGoalY = document.getElementById('algoGoalY');
    const btnAlgoRun = document.getElementById('btnAlgoRun') || document.getElementById('btnAlgoRunBenchmark');
    const btnAlgoRunBenchmark = btnAlgoRun;
    const algoResultsTableBody = document.getElementById('algoResultsTableBody');
    const algoModePills = document.getElementById('algoModePills');
    const btnToolSetStart = document.getElementById('btnToolSetStart');
    const btnToolSetGoal = document.getElementById('btnToolSetGoal');
    const btnToolToggleObstacle = document.getElementById('btnToolToggleObstacle');
    const btnClearCustomObstacles = document.getElementById('btnClearCustomObstacles');
    const btnResetLabDefaults = document.getElementById('btnResetLabDefaults');
    const btnAlgoPauseResume = document.getElementById('btnAlgoPauseResume');
    const btnAlgoStep = document.getElementById('btnAlgoStep');
    const btnAlgoResetAnim = document.getElementById('btnAlgoResetAnim');
    const algoSpeedSlider = document.getElementById('algoSpeedSlider');
    const algoSpeedLabel = document.getElementById('algoSpeedLabel');
    const algoStatusBadge = document.getElementById('algoStatusBadge');
    const algoMetricTime = document.getElementById('algoMetricTime');
    const algoMetricExplored = document.getElementById('algoMetricExplored');
    const algoMetricLength = document.getElementById('algoMetricLength');
    const algoMetricSteps = document.getElementById('algoMetricSteps');
    const algoMetricCost = document.getElementById('algoMetricCost');
    const algoMetricEfficiency = document.getElementById('algoMetricEfficiency');
    const algoSourceLabel = document.getElementById('algoSourceLabel');
    const chartBfsNodes = document.getElementById('chartBfsNodes');
    const barBfsNodes = document.getElementById('barBfsNodes');
    const chartDijkstraNodes = document.getElementById('chartDijkstraNodes');
    const barDijkstraNodes = document.getElementById('barDijkstraNodes');
    const chartAstarNodes = document.getElementById('chartAstarNodes');
    const barAstarNodes = document.getElementById('barAstarNodes');
    const chartBfsTime = document.getElementById('chartBfsTime');
    const barBfsTime = document.getElementById('barBfsTime');
    const chartDijkstraTime = document.getElementById('chartDijkstraTime');
    const barDijkstraTime = document.getElementById('barDijkstraTime');
    const chartAstarTime = document.getElementById('chartAstarTime');
    const barAstarTime = document.getElementById('barAstarTime');
    const btnAlgoExportJSON = document.getElementById('btnAlgoExportJSON');
    const btnAlgoExportCSV = document.getElementById('btnAlgoExportCSV');
    const algoSummaryText = document.getElementById('algoSummaryText');

    // Legacy lists in Overview
    const activeOrdersContainer = document.getElementById('activeOrdersContainer');
    const fleetContainer = document.getElementById('fleetContainer');
    const inventoryContainer = document.getElementById('inventoryContainer');
    const startXInput = document.getElementById('startXInput');
    const startYInput = document.getElementById('startYInput');
    const goalXInput = document.getElementById('goalXInput');
    const goalYInput = document.getElementById('goalYInput');
    const btnRunBenchmark = document.getElementById('btnRunBenchmark');
    const benchmarkResultsTable = document.getElementById('benchmarkResultsTable');

    // Live Activity Feed & Batch Dispatch Elements
    const liveActivityFeed = document.getElementById('liveActivityFeed');
    const btnClearActivityFeed = document.getElementById('btnClearActivityFeed');
    const btnBatchDispatchOrders = document.getElementById('btnBatchDispatchOrders');
    const btnOrdersViewBatchDispatch = document.getElementById('btnOrdersViewBatchDispatch');

    // State & Telemetry Guards
    let isPollInFlight = false;
    let state = null;
    let prevKnownState = null;

    // =========================================================================
    // View Routing & Navigation System
    // =========================================================================
    function switchView(viewName) {
        currentView = viewName;

        // Update nav pill styling
        navPills.forEach(pill => {
            const isMatch = pill.dataset.view === viewName;
            pill.classList.toggle('active', isMatch);
        });

        // Hide all dedicated view panels first
        allViews.forEach(v => {
            if (v) v.classList.remove('active');
        });

        if (viewName === 'overview') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'grid';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'grid';
            if (overviewDeck) overviewDeck.style.display = 'flex';
            if (warehouseOpsDeck) warehouseOpsDeck.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'flex';
            setTimeout(() => {
                canvas.initCanvas();
                canvas.draw();
            }, 10);
        } else if (viewName === 'warehouse') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'grid';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'grid';
            if (overviewDeck) overviewDeck.style.display = 'none';
            if (warehouseOpsDeck) warehouseOpsDeck.style.display = 'flex';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            populateAllProductDropdowns(state?.inventory);
            setTimeout(() => {
                canvas.initCanvas();
                canvas.draw();
            }, 10);
        } else if (viewName === 'fleet') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'none';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewFleet) viewFleet.classList.add('active');
            renderFleetManagement(state);
        } else if (viewName === 'orders') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'grid';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewOrders) viewOrders.classList.add('active');
            populateAllProductDropdowns(state?.inventory);
            renderOrdersView(state);
        } else if (viewName === 'inventory') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'none';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewInventory) viewInventory.classList.add('active');
            renderInventoryView(state);
        } else if (viewName === 'algorithms') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'none';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewAlgorithms) viewAlgorithms.classList.add('active');
            setTimeout(() => {
                if (algoCanvasInstance) {
                    algoCanvasInstance.initCanvas();
                    algoCanvasInstance.render();
                }
            }, 30);
        } else if (viewName === 'ecoroute') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'none';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewEcoRoute) viewEcoRoute.classList.add('active');
            renderEcoRouteDashboard();
            setTimeout(() => {
                if (ecoCanvasInstance) {
                    ecoCanvasInstance.initCanvas();
                    ecoCanvasInstance.render();
                }
            }, 30);
        } else if (viewName === 'foodsaver') {
            if (mainKpiGrid) mainKpiGrid.style.display = 'none';
            if (mainWorkspaceGrid) mainWorkspaceGrid.style.display = 'none';
            if (overviewAnalyticsSection) overviewAnalyticsSection.style.display = 'none';
            if (viewFoodSaver) viewFoodSaver.classList.add('active');
            renderFoodSaverDashboard();
        }

        // Update hash for browser history without scrolling
        if (window.location.hash !== `#${viewName}`) {
            history.pushState(null, null, `#${viewName}`);
        }
    }

    // Nav pill click handlers - registered immediately
    navPills.forEach(pill => {
        pill.addEventListener('click', (e) => {
            e.preventDefault();
            const targetView = pill.dataset.view || 'overview';
            switchView(targetView);
        });
    });

    // Hash change handler for Back / Forward browser navigation
    window.addEventListener('hashchange', () => {
        const hash = window.location.hash.replace('#', '') || 'overview';
        if (hash !== currentView) {
            switchView(hash);
        }
    });

    // Initialize initial route from URL hash if provided
    const initialHash = window.location.hash.replace('#', '');
    if (['overview', 'warehouse', 'fleet', 'orders', 'inventory', 'algorithms', 'ecoroute'].includes(initialHash)) {
        switchView(initialHash);
    }

    // =========================================================================
    // Reactive Connection Listener
    // =========================================================================
    api.onConnectionChange = (connected) => {
        updateConnectionUI(connected);
    };

    function updateConnectionUI(connected) {
        if (connected) {
            if (connectionBadge) {
                connectionBadge.className = 'status-pill';
                connectionBadge.innerHTML = '<span class="indicator-dot"></span> Live C++ Backend Connected';
            }
            if (fleetLiveSyncBadge) {
                fleetLiveSyncBadge.className = 'status-pill';
                fleetLiveSyncBadge.innerHTML = '<span class="indicator-dot"></span> Live Fleet Telemetry Active (Port 8080)';
            }
        } else {
            if (connectionBadge) {
                connectionBadge.className = 'status-pill demo';
                connectionBadge.innerHTML = '<span class="indicator-dot"></span> Standalone Demo Mode';
            }
            if (fleetLiveSyncBadge) {
                fleetLiveSyncBadge.className = 'status-pill demo';
                fleetLiveSyncBadge.innerHTML = '<span class="indicator-dot"></span> Standalone Demo Fleet';
            }
        }
    }

    // Periodic Connection Health Check
    async function checkBackend() {
        try {
            const isLive = await api.checkConnection();
            updateConnectionUI(isLive);
        } catch (e) {
            updateConnectionUI(false);
        }
    }

    // Initial backend check and state synchronization
    (async () => {
        try {
            await checkBackend();
        } catch (e) {
            console.warn('[WarehouseApp] checkBackend exception:', e);
        }
        setInterval(checkBackend, 3000);

        try {
            state = await api.getState();
            updateUI(state);
            populateAllProductDropdowns(state?.inventory);
        } catch (e) {
            console.warn('[WarehouseApp] Initial getState exception:', e);
            try {
                state = api.localSimulation.getState();
                updateUI(state);
                populateAllProductDropdowns(state?.inventory);
            } catch (fallbackErr) {
                console.error('[WarehouseApp] Fallback state exception:', fallbackErr);
            }
        }
    })();

    // Overview sub-tab switching
    tabButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const tabId = btn.dataset.tab;
            tabButtons.forEach(b => b.classList.toggle('active', b.dataset.tab === tabId));
            tabPanes.forEach(pane => pane.classList.toggle('active', pane.id === tabId));
        });
    });

    // =========================================================================
    // Simulation Controls (Auto-Play, Step, Reset, Speed)
    // =========================================================================
    btnPlayPause.addEventListener('click', () => {
        isPlaying = !isPlaying;
        if (isPlaying) {
            btnPlayPause.innerHTML = '⏸️ Pause';
            btnPlayPause.classList.remove('btn-primary');
            btnPlayPause.classList.add('btn-secondary');
            startAutoPlay();
        } else {
            btnPlayPause.innerHTML = '▶️ Auto-Play';
            btnPlayPause.classList.remove('btn-secondary');
            btnPlayPause.classList.add('btn-primary');
            stopAutoPlay();
        }
    });

    let isStepInFlight = false;

    btnStep.addEventListener('click', async () => {
        if (btnStep.disabled || isStepInFlight) return;
        btnStep.disabled = true;
        isStepInFlight = true;
        try {
            const newState = await api.stepSimulation();
            if (newState && newState.tick !== undefined) {
                state = newState;
                updateUI(state);
            }
        } catch (err) {
            console.error('[WarehouseApp] Step tick error:', err);
            showFeedback(obstacleFeedbackBanner, 'Simulation step failed: ' + (err.message || 'Error'), 'error');
        } finally {
            isStepInFlight = false;
            btnStep.disabled = false;
        }
    });

    btnReset.addEventListener('click', async () => {
        if (btnReset.disabled) return;
        if (confirm('Reset warehouse simulation to initial state?')) {
            btnReset.disabled = true;
            try {
                const newState = await api.resetSimulation();
                if (newState && newState.tick !== undefined) {
                    state = newState;
                    updateUI(state);
                    canvas.clearComparisonPath();
                    showFeedback(obstacleFeedbackBanner, 'Warehouse simulation reset to initial state.', 'success');
                }
            } catch (err) {
                console.error('[WarehouseApp] Reset error:', err);
                showFeedback(obstacleFeedbackBanner, 'Simulation reset failed: ' + (err.message || 'Error'), 'error');
            } finally {
                btnReset.disabled = false;
            }
        }
    });

    speedSlider.addEventListener('input', (e) => {
        const val = parseInt(e.target.value);
        tickSpeedMs = Math.round(800 / val);
        speedLabel.textContent = `${val}x (${tickSpeedMs}ms)`;
        if (isPlaying) {
            stopAutoPlay();
            startAutoPlay();
        }
    });

    function startAutoPlay() {
        if (playIntervalId) clearInterval(playIntervalId);
        playIntervalId = setInterval(async () => {
            if (isStepInFlight) return;
            isStepInFlight = true;
            try {
                const newState = await api.stepSimulation();
                if (newState && newState.tick !== undefined) {
                    state = newState;
                    updateUI(state);
                }
            } catch (err) {
                console.error('[WarehouseApp] Auto-play step error:', err);
            } finally {
                isStepInFlight = false;
            }
        }, tickSpeedMs);
    }

    function stopAutoPlay() {
        if (playIntervalId) clearInterval(playIntervalId);
        playIntervalId = null;
        isStepInFlight = false;
    }

    // Live Telemetry Polling (Every 800ms when simulation is not auto-playing)
    setInterval(async () => {
        if (isPlaying || isPollInFlight || document.hidden) return;
        isPollInFlight = true;
        try {
            const latest = await api.getState();
            if (latest && latest.tick !== undefined) {
                state = latest;
                updateUI(state);
            }
        } catch (e) {
            // Backend unavailable or quiet error
        } finally {
            isPollInFlight = false;
        }
    }, 800);

    // =========================================================================
    // Dynamic Obstacle Mode & Cell Inspector
    // =========================================================================
    function setObstacleMode(enable) {
        isObstacleMode = enable;
        if (btnToggleObstacleMode) {
            btnToggleObstacleMode.textContent = isObstacleMode ? '🚧 Obstacle Mode: ON' : '🚧 Obstacle Mode: OFF';
            btnToggleObstacleMode.classList.toggle('obstacle-mode-active', isObstacleMode);
        }
        if (obstacleModeBadge) {
            obstacleModeBadge.classList.toggle('active', isObstacleMode);
            obstacleModeBadge.textContent = isObstacleMode
                ? '⚠️ Click grid cell to place/remove dynamic obstacle'
                : 'Click Grid to Add/Remove Obstacles';
        }
        if (warehouseObstacleStatusTag) {
            warehouseObstacleStatusTag.classList.toggle('active', isObstacleMode);
            warehouseObstacleStatusTag.textContent = isObstacleMode ? 'Mode: ACTIVE (Click Cell)' : 'Mode: OFF';
        }
    }

    if (btnToggleObstacleMode) {
        btnToggleObstacleMode.addEventListener('click', () => setObstacleMode(!isObstacleMode));
    }
    if (btnWarehouseToggleObstacleMode) {
        btnWarehouseToggleObstacleMode.addEventListener('click', () => setObstacleMode(!isObstacleMode));
    }

    // Inspect Cell Type
    function getCellDescription(x, y) {
        if (!state || !state.warehouse) return 'Unknown Cell';
        const w = state.warehouse;
        if (w.dynamicObstacles && w.dynamicObstacles.some(o => o.x === x && o.y === y)) {
            return 'Dynamic Obstacle (⚠️ Impassable)';
        }
        if (state.robots && state.robots.some(r => r.position.x === x && r.position.y === y)) {
            const r = state.robots.find(r => r.position.x === x && r.position.y === y);
            return `Occupied by Robot ${r.id} (${r.status})`;
        }
        if (w.chargingStations && w.chargingStations.some(s => s.position.x === x && s.position.y === y)) {
            return 'Charging Station Bay (⚡ Walkable/Charging)';
        }
        if (w.packingStations && w.packingStations.some(s => s.position.x === x && s.position.y === y)) {
            return 'Packing Station Bay (📦 Walkable/Handoff)';
        }
        if (w.shelves && w.shelves.some(s => s.position.x === x && s.position.y === y)) {
            return 'Storage Shelf (Impassable Structure)';
        }
        const cellVal = w.grid && w.grid[y] ? w.grid[y][x] : 0;
        if (cellVal === 1) return 'Static Obstacle Pillar (Impassable)';
        return 'Empty Floor Cell (Walkable)';
    }

    function updateCellInspector(x, y) {
        if (!state || !state.warehouse) return;
        if (x < 0 || x >= 15 || y < 0 || y >= 15) {
            if (inspectorCellCoord) inspectorCellCoord.textContent = '(Click grid)';
            if (inspectorCellType) inspectorCellType.textContent = 'Select cell on map';
            if (inspectorDynamicStatus) inspectorDynamicStatus.textContent = 'None';
            if (inspectorVisitsVal) inspectorVisitsVal.textContent = '0';
            if (inspectorWaitsVal) inspectorWaitsVal.textContent = '0';
            if (inspectorReplansVal) inspectorReplansVal.textContent = '0';
            return;
        }

        if (inspectorCellCoord) inspectorCellCoord.textContent = `(${x}, ${y})`;
        if (inspectorCellType) inspectorCellType.textContent = getCellDescription(x, y);

        const isDynamic = state.warehouse?.dynamicObstacles?.some(o => o.x === x && o.y === y);
        if (inspectorDynamicStatus) {
            inspectorDynamicStatus.textContent = isDynamic ? '⚠️ Yes (Dynamic Obstacle)' : 'No (Clear)';
            inspectorDynamicStatus.style.color = isDynamic ? '#ea580c' : 'var(--text-secondary)';
        }

        const cong = state.analytics?.congestion;
        const v = (cong && cong.visits && cong.visits[y]) ? (cong.visits[y][x] || 0) : 0;
        const w = (cong && cong.waits && cong.waits[y]) ? (cong.waits[y][x] || 0) : 0;
        const r = (cong && cong.replans && cong.replans[y]) ? (cong.replans[y][x] || 0) : 0;

        if (inspectorVisitsVal) inspectorVisitsVal.textContent = v;
        if (inspectorWaitsVal) inspectorWaitsVal.textContent = w;
        if (inspectorReplansVal) inspectorReplansVal.textContent = r;
    }

    function updateHeatmapControls(s) {
        if (!s || !s.analytics || !s.analytics.congestion) return;
        const cong = s.analytics.congestion;

        let maxVal = 0;
        if (currentHeatmapMode === 'waits') {
            maxVal = cong.maxWaits !== undefined ? cong.maxWaits : 0;
        } else if (currentHeatmapMode === 'replans') {
            maxVal = cong.maxReplans !== undefined ? cong.maxReplans : 0;
        } else {
            maxVal = cong.maxVisits !== undefined ? cong.maxVisits : 0;
        }

        if (heatmapMaxLabel) {
            heatmapMaxLabel.textContent = `Max: ${maxVal}`;
        }
    }

    function escapeHTML(str) {
        if (str === null || str === undefined) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    function renderHeatmapObservationDetails(s) {
        if (!heatmapDetailPanel) return;
        if (!s) {
            heatmapDetailPanel.innerHTML = '<div class="heatmap-detail-empty">Simulation state unavailable.</div>';
            return;
        }

        const cong = s.analytics?.congestion;
        const maxVisits = cong?.maxVisits || 0;
        const maxWaits = cong?.maxWaits || 0;
        const maxReplans = cong?.maxReplans || 0;
        const totalSteps = s.analytics?.fleetTotalMovementSteps || 0;
        const robots = s.robots || [];

        if (currentHeatmapMode === 'visits') {
            const hasActivity = totalSteps > 0 || maxVisits > 0;
            let listHtml = '';
            if (robots.length > 0) {
                const items = robots.map(r => {
                    const util = s.analytics?.robotUtilization?.find(u => u.robotId === r.id);
                    const steps = util?.movementSteps ?? r.movementSteps ?? 0;
                    return `
                        <div class="heatmap-detail-item">
                            <span style="font-weight:600;display:flex;align-items:center;gap:6px;">
                                <span class="badge ${r.status}" style="font-size:10px;padding:2px 6px;">${r.id}</span>
                                <span>${escapeHTML(r.name)}</span>
                            </span>
                            <span style="font-family:var(--font-mono);color:var(--text-secondary);font-size:11px;">
                                <strong>${steps}</strong> steps &bull; Pos (${r.position.x}, ${r.position.y})
                            </span>
                        </div>
                    `;
                }).join('');
                listHtml = `<div class="heatmap-detail-list" style="margin-top:8px;">${items}</div>`;
            }

            heatmapDetailPanel.innerHTML = `
                <div class="heatmap-detail-header">
                    <div class="heatmap-detail-title">
                        <span>🚶</span>
                        <span>Robot Cell Visits & Movement Frequency</span>
                    </div>
                    <span style="font-size:11px;font-family:var(--font-mono);color:var(--text-muted);">Mode: VISITS</span>
                </div>
                <div class="heatmap-detail-grid">
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Peak Cell Visits</div>
                        <div class="heatmap-detail-kpi-val" style="color:var(--accent-blue);">${maxVisits}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Fleet Total Steps</div>
                        <div class="heatmap-detail-kpi-val">${totalSteps}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Active Fleet</div>
                        <div class="heatmap-detail-kpi-val">${robots.filter(r => r.status !== 'IDLE').length} / ${robots.length}</div>
                    </div>
                </div>
                ${!hasActivity ? '<div class="heatmap-detail-empty">No robot cell visits recorded yet. Advance simulation ticks to track cell traversal frequency.</div>' : listHtml}
            `;
        } else if (currentHeatmapMode === 'waits') {
            const waitingRobots = s.coordinator?.waitingRobots ?? robots.filter(r => r.status === 'WAITING').length;
            const blockedRobots = s.coordinator?.blockedRobots ?? robots.filter(r => r.status === 'BLOCKED').length;
            const affectedRobots = robots.filter(r => r.status === 'WAITING' || r.status === 'BLOCKED' || (r.waitTicks && r.waitTicks > 0));

            let listHtml = '';
            if (affectedRobots.length > 0) {
                const items = affectedRobots.map(r => `
                    <div class="heatmap-detail-item">
                        <span style="font-weight:600;display:flex;align-items:center;gap:6px;">
                            <span class="badge ${r.status}" style="font-size:10px;padding:2px 6px;">${r.id}</span>
                            <span>${escapeHTML(r.name)}</span>
                        </span>
                        <span style="font-family:var(--font-mono);font-size:11px;color:${r.status === 'BLOCKED' ? '#e11d48' : '#d97706'};font-weight:700;">
                            ${r.waitTicks || 0} ticks wait &bull; Pos (${r.position.x}, ${r.position.y})
                        </span>
                    </div>
                `).join('');
                listHtml = `<div class="heatmap-detail-list" style="margin-top:8px;">${items}</div>`;
            }

            heatmapDetailPanel.innerHTML = `
                <div class="heatmap-detail-header">
                    <div class="heatmap-detail-title">
                        <span>⏳</span>
                        <span>Contested Waiting & Blocked Hotspots</span>
                    </div>
                    <span style="font-size:11px;font-family:var(--font-mono);color:var(--text-muted);">Mode: WAITING & BLOCKED</span>
                </div>
                <div class="heatmap-detail-grid">
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Currently Waiting</div>
                        <div class="heatmap-detail-kpi-val" style="color:#d97706;">${waitingRobots}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Currently Blocked</div>
                        <div class="heatmap-detail-kpi-val" style="color:#e11d48;">${blockedRobots}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Peak Wait Hotspot</div>
                        <div class="heatmap-detail-kpi-val">${maxWaits} ticks</div>
                    </div>
                </div>
                ${affectedRobots.length === 0 && maxWaits === 0
                    ? '<div class="heatmap-detail-empty">✅ Clear traffic flow: No robots currently waiting or blocked at reservation cells.</div>'
                    : listHtml}
            `;
        } else if (currentHeatmapMode === 'replans') {
            const replans = s.coordinator?.routesReplanned ?? maxReplans;
            const collisions = s.coordinator?.collisionsPrevented ?? 0;
            const deadlocks = s.coordinator?.deadlocksResolved ?? 0;
            const hasEvents = replans > 0 || collisions > 0 || deadlocks > 0;

            heatmapDetailPanel.innerHTML = `
                <div class="heatmap-detail-header">
                    <div class="heatmap-detail-title">
                        <span>🔀</span>
                        <span>Dynamic Replanning & Collision Prevention</span>
                    </div>
                    <span style="font-size:11px;font-family:var(--font-mono);color:var(--text-muted);">Mode: REPLANNING</span>
                </div>
                <div class="heatmap-detail-grid">
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Routes Replanned</div>
                        <div class="heatmap-detail-kpi-val" style="color:var(--accent-purple);">${replans}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Collisions Prevented</div>
                        <div class="heatmap-detail-kpi-val" style="color:#059669;">${collisions}</div>
                    </div>
                    <div class="heatmap-detail-kpi">
                        <div class="heatmap-detail-kpi-label">Deadlocks Resolved</div>
                        <div class="heatmap-detail-kpi-val" style="color:var(--accent-blue);">${deadlocks}</div>
                    </div>
                </div>
                ${!hasEvents
                    ? '<div class="heatmap-detail-empty">✅ Conflict-free routing: No replanning actions or reservation collisions triggered.</div>'
                    : `
                    <div class="heatmap-detail-list" style="margin-top:8px;">
                        <div class="heatmap-detail-item">
                            <span>🛡️ Space-Time Reservation Yields</span>
                            <strong style="font-family:var(--font-mono);">${collisions} collision(s) averted</strong>
                        </div>
                        <div class="heatmap-detail-item">
                            <span>⚡ Cooperative Deadlock Resolution</span>
                            <strong style="font-family:var(--font-mono);">${deadlocks} cycle(s) broken</strong>
                        </div>
                        <div class="heatmap-detail-item">
                            <span>🔄 Dynamic A* Alternative Paths</span>
                            <strong style="font-family:var(--font-mono);">${replans} recalculation(s)</strong>
                        </div>
                    </div>
                `}
            `;
        }
    }

    // Canvas Cell Click Handler
    canvas.onCellClick = async (x, y) => {
        selectedCell = { x, y };
        canvas.setSelectedCell(x, y);
        updateCellInspector(x, y);

        if (isObstacleMode) {
            await handleToggleObstacleAt(x, y);
            updateCellInspector(x, y);
        } else {
            // Coordinate picking for algorithms benchmark
            if (currentView === 'algorithms' || document.getElementById('tabAlgorithms')?.classList.contains('active')) {
                if (document.activeElement === goalXInput || document.activeElement === goalYInput ||
                    document.activeElement === algoGoalX || document.activeElement === algoGoalY) {
                    if (goalXInput) goalXInput.value = x;
                    if (goalYInput) goalYInput.value = y;
                    if (algoGoalX) algoGoalX.value = x;
                    if (algoGoalY) algoGoalY.value = y;
                } else {
                    if (startXInput) startXInput.value = x;
                    if (startYInput) startYInput.value = y;
                    if (algoStartX) algoStartX.value = x;
                    if (algoStartY) algoStartY.value = y;
                }
            }
        }
    };

    let isObstacleActionInFlight = false;

    async function handleToggleObstacleAt(x, y) {
        if (isObstacleActionInFlight) return;
        isObstacleActionInFlight = true;
        try {
            const isDynamic = state.warehouse?.dynamicObstacles?.some(o => o.x === x && o.y === y);
            let res;
            if (isDynamic) {
                res = await api.removeDynamicObstacle(x, y);
                if (res && res.success) {
                    showFeedback(obstacleFeedbackBanner, `Dynamic obstacle removed at (${x}, ${y}).`, 'success');
                } else {
                    showFeedback(obstacleFeedbackBanner, (res && res.error) || 'Failed to remove obstacle.', 'error');
                }
            } else {
                res = await api.addDynamicObstacle(x, y);
                if (res && res.success) {
                    showFeedback(obstacleFeedbackBanner, `Dynamic obstacle placed at (${x}, ${y}).`, 'success');
                } else {
                    showFeedback(obstacleFeedbackBanner, (res && res.error) || 'Cannot place obstacle at this location.', 'error');
                }
            }

            // Refresh and update
            state = await api.getState();
            updateUI(state);
            if (inspectorCellType) inspectorCellType.textContent = getCellDescription(x, y);
        } catch (err) {
            showFeedback(obstacleFeedbackBanner, 'Obstacle action failed: ' + (err.message || 'Error'), 'error');
        } finally {
            isObstacleActionInFlight = false;
        }
    }

    if (btnCellPlaceRemoveObstacle) {
        btnCellPlaceRemoveObstacle.addEventListener('click', async () => {
            if (isObstacleActionInFlight) return;
            if (!selectedCell) {
                showFeedback(obstacleFeedbackBanner, 'Please click a cell on the map first.', 'error');
                return;
            }
            await handleToggleObstacleAt(selectedCell.x, selectedCell.y);
        });
    }

    if (btnClearAllObstacles) {
        btnClearAllObstacles.addEventListener('click', async () => {
            if (isObstacleActionInFlight || btnClearAllObstacles.disabled) return;
            const list = state.warehouse?.dynamicObstacles || [];
            if (list.length === 0) {
                showFeedback(obstacleFeedbackBanner, 'No active dynamic obstacles to clear.', 'error');
                return;
            }
            btnClearAllObstacles.disabled = true;
            isObstacleActionInFlight = true;
            try {
                for (const obs of list) {
                    await api.removeDynamicObstacle(obs.x, obs.y);
                }
                state = await api.getState();
                updateUI(state);
                showFeedback(obstacleFeedbackBanner, 'All dynamic obstacles cleared successfully.', 'success');
            } catch (err) {
                showFeedback(obstacleFeedbackBanner, 'Error clearing obstacles: ' + (err.message || 'Error'), 'error');
            } finally {
                isObstacleActionInFlight = false;
                btnClearAllObstacles.disabled = false;
            }
        });
    }

    // =========================================================================
    // Stage F: Congestion Heatmap Controls & Reset Handlers
    // =========================================================================
    if (btnToggleHeatmap) {
        btnToggleHeatmap.addEventListener('click', () => {
            isHeatmapActive = !isHeatmapActive;
            canvas.setHeatmapEnabled(isHeatmapActive);

            if (heatmapToggleText) heatmapToggleText.textContent = isHeatmapActive ? 'Heatmap: ON' : 'Heatmap: OFF';
            btnToggleHeatmap.classList.toggle('btn-primary', isHeatmapActive);
            btnToggleHeatmap.classList.toggle('btn-secondary', !isHeatmapActive);

            if (heatmapLegendWrap) {
                heatmapLegendWrap.style.opacity = isHeatmapActive ? '1' : '0.6';
            }

            updateHeatmapControls(state);
            renderHeatmapObservationDetails(state);
        });
    }

    function selectHeatmapObservationMode(mode) {
        if (!mode) return;
        currentHeatmapMode = mode;
        canvas.setHeatmapMode(mode);

        heatmapModeBtns.forEach(b => {
            const isActive = b.dataset.mode === mode;
            b.classList.toggle('active', isActive);
            b.setAttribute('aria-selected', isActive ? 'true' : 'false');
            b.setAttribute('tabindex', isActive ? '0' : '-1');
        });

        if (heatmapLegendBar) {
            heatmapLegendBar.className = `heatmap-legend-bar ${mode}`;
        }

        updateHeatmapControls(state);
        renderHeatmapObservationDetails(state);
    }

    heatmapModeBtns.forEach((btn, idx) => {
        btn.addEventListener('click', () => {
            selectHeatmapObservationMode(btn.dataset.mode);
        });

        btn.addEventListener('keydown', (e) => {
            if (e.key === 'ArrowRight' || e.key === 'ArrowDown') {
                e.preventDefault();
                const nextIdx = (idx + 1) % heatmapModeBtns.length;
                heatmapModeBtns[nextIdx].focus();
                selectHeatmapObservationMode(heatmapModeBtns[nextIdx].dataset.mode);
            } else if (e.key === 'ArrowLeft' || e.key === 'ArrowUp') {
                e.preventDefault();
                const prevIdx = (idx - 1 + heatmapModeBtns.length) % heatmapModeBtns.length;
                heatmapModeBtns[prevIdx].focus();
                selectHeatmapObservationMode(heatmapModeBtns[prevIdx].dataset.mode);
            }
        });
    });

    if (btnResetHeatmap) {
        btnResetHeatmap.addEventListener('click', async () => {
            try {
                btnResetHeatmap.disabled = true;
                btnResetHeatmap.textContent = '⏳ Clearing...';
                const newState = await api.resetHeatmap();
                if (newState && newState.warehouse) {
                    state = newState;
                    canvas.updateState(state);
                    updateUI(state);
                    renderHeatmapObservationDetails(state);
                }
                showFeedback(heatmapFeedbackBanner, 'Recorded spatial congestion observations cleared. Robot fleet, active orders, and inventory preserved.', 'success');
            } catch (err) {
                console.error('[Heatmap] Reset failed:', err);
                showFeedback(heatmapFeedbackBanner, 'Failed to clear heatmap observations.', 'error');
            } finally {
                btnResetHeatmap.disabled = false;
                btnResetHeatmap.textContent = '🧹 Clear Heatmap';
            }
        });
    }

    function showFeedback(element, message, type = 'success') {
        if (!element) return;
        element.className = `feedback-banner ${type}`;
        element.textContent = message;
        element.style.display = 'block';
        setTimeout(() => {
            element.style.display = 'none';
        }, 3500);
    }

    // =========================================================================
    // Priority Tier Button Handling
    // =========================================================================
    prioBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            prioBtns.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            selectedPriority = btn.dataset.prio;
        });
    });

    warehousePrioBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            warehousePrioBtns.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            warehouseSelectedPriority = btn.dataset.prio;
        });
    });

    ordersViewPrioBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            ordersViewPrioBtns.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            ordersViewSelectedPriority = btn.dataset.prio;
        });
    });

    // =========================================================================
    // Order Dispatch Handlers (Overview, Warehouse, Orders Views)
    // =========================================================================
    function setupOrderDispatch(buttonEl, selectEl, inputEl, prioGetter, feedbackEl) {
        if (!buttonEl) return;
        buttonEl.addEventListener('click', async () => {
            if (buttonEl.disabled) return;

            const prodId = parseInt(selectEl.value);
            const qty = parseInt(inputEl.value) || 0;
            const priority = prioGetter();

            if (!prodId) {
                showFeedback(feedbackEl, 'Please select a valid product.', 'error');
                return;
            }

            const prod = state?.inventory?.find(p => p.id === prodId);
            if (!prod) {
                showFeedback(feedbackEl, 'Product not found in inventory.', 'error');
                return;
            }

            if (qty <= 0) {
                showFeedback(feedbackEl, 'Quantity must be at least 1 unit.', 'error');
                return;
            }

            const origText = buttonEl.textContent;
            buttonEl.disabled = true;
            buttonEl.textContent = 'Submitting...';

            try {
                const res = await api.createOrder(prodId, qty, priority);
                if (res && res.success) {
                    showFeedback(feedbackEl, `✓ Order #${res.orderId || 'NEW'} dispatched successfully!`, 'success');
                    state = await api.getState();
                    updateUI(state);
                    populateAllProductDropdowns(state.inventory);
                } else {
                    showFeedback(feedbackEl, `Dispatch failed: ${(res && res.error) || 'Server error'}`, 'error');
                }
            } catch (err) {
                showFeedback(feedbackEl, `Dispatch error: ${err.message || 'Network failure'}`, 'error');
            } finally {
                buttonEl.disabled = false;
                buttonEl.textContent = origText;
            }
        });
    }

    setupOrderDispatch(btnDispatchOrder, productSelect, quantityInput, () => selectedPriority, overviewOrderFeedback);
    setupOrderDispatch(btnWarehouseDispatchOrder, warehouseProductSelect, warehouseQuantityInput, () => warehouseSelectedPriority, warehouseOrderFeedback);
    setupOrderDispatch(btnOrdersViewDispatch, ordersViewProductSelect, ordersViewQuantityInput, () => ordersViewSelectedPriority, ordersViewFeedback);

    // Live Activity Feed Logger
    if (btnClearActivityFeed && liveActivityFeed) {
        btnClearActivityFeed.addEventListener('click', () => {
            liveActivityFeed.innerHTML = '<div style="color:var(--text-muted);font-size:11px;text-align:center;padding:8px;">Feed cleared.</div>';
        });
    }

    function logActivityEvent(text, icon = 'ℹ️') {
        if (!liveActivityFeed) return;
        const tick = state ? (state.tick || state.simulationTick || 0) : 0;
        const item = document.createElement('div');
        item.className = 'activity-event-item';
        item.innerHTML = `
            <span class="activity-event-tick">[T+${tick}]</span>
            <span class="activity-event-icon">${icon}</span>
            <span class="activity-event-text">${text}</span>
        `;
        liveActivityFeed.prepend(item);
        while (liveActivityFeed.children.length > 40) {
            liveActivityFeed.removeChild(liveActivityFeed.lastChild);
        }
    }

    // Multi-Order Parallel Batch Dispatch Demo
    function setupBatchDispatch(buttonEl, feedbackEl) {
        if (!buttonEl) return;
        buttonEl.addEventListener('click', async () => {
            const origText = buttonEl.textContent;
            buttonEl.disabled = true;
            buttonEl.textContent = '🚀 Dispatching 3 Orders...';
            try {
                const batch = [
                    { productId: 101, quantity: 2, priority: 'HIGH' },
                    { productId: 102, quantity: 2, priority: 'MEDIUM' },
                    { productId: 103, quantity: 2, priority: 'LOW' }
                ];
                let successCount = 0;
                for (const item of batch) {
                    const res = await api.createOrder(item.productId, item.quantity, item.priority);
                    if (res && res.success) successCount++;
                }
                if (successCount > 0) {
                    showFeedback(feedbackEl, `Dispatched ${successCount} orders simultaneously! Watch parallel execution across multiple robots.`, 'success');
                    logActivityEvent(`🚀 Dispatched batch of ${successCount} orders (HIGH/MED/LOW) for concurrent multi-robot execution`, '🚀');
                    state = await api.getState();
                    updateUI(state);
                } else {
                    showFeedback(feedbackEl, `Batch dispatch encountered an issue.`, 'error');
                }
            } catch (err) {
                showFeedback(feedbackEl, `Batch dispatch error: ${err.message || 'Error'}`, 'error');
            } finally {
                buttonEl.disabled = false;
                buttonEl.textContent = origText;
            }
        });
    }

    setupBatchDispatch(btnBatchDispatchOrders, overviewOrderFeedback);
    setupBatchDispatch(btnOrdersViewBatchDispatch, ordersViewFeedback);

    // Live Stock Label Updater for Warehouse Dispatch Input
    if (warehouseProductSelect && warehouseQuantityInput) {
        const updateWarehouseStockHelp = () => {
            const prodId = parseInt(warehouseProductSelect.value);
            const prod = state?.inventory?.find(p => p.id === prodId);
            if (prod && warehouseStockHelp) {
                warehouseStockHelp.textContent = `Available Stock: ${prod.quantity} units (Shelf at ${prod.location.x}, ${prod.location.y})`;
                warehouseQuantityInput.max = prod.quantity;
            }
        };
        warehouseProductSelect.addEventListener('change', updateWarehouseStockHelp);
    }

    if (ordersViewProductSelect && ordersViewQuantityInput) {
        const updateOrdersStockHelp = () => {
            const prodId = parseInt(ordersViewProductSelect.value);
            const prod = state?.inventory?.find(p => p.id === prodId);
            if (prod && ordersViewStockHelp) {
                ordersViewStockHelp.textContent = `Available Stock: ${prod.quantity} units`;
                ordersViewQuantityInput.max = prod.quantity;
            }
        };
        ordersViewProductSelect.addEventListener('change', updateOrdersStockHelp);
    }

    // =========================================================================
    // Fleet Filtering, Sorting & Rendering
    // =========================================================================
    function getFilteredAndSortedRobots(robots) {
        if (!robots) return [];
        let list = [...robots];

        // 1. Filter by Type
        const typeFilter = fleetFilterType ? fleetFilterType.value : 'ALL';
        if (typeFilter !== 'ALL') {
            list = list.filter(r => r.type === typeFilter);
        }

        // 2. Filter by Status
        const statusFilter = fleetFilterStatus ? fleetFilterStatus.value : 'ALL';
        if (statusFilter !== 'ALL') {
            list = list.filter(r => r.status === statusFilter);
        }

        // 3. Filter by Battery Range
        const battFilter = fleetFilterBattery ? fleetFilterBattery.value : 'ALL';
        if (battFilter === 'HIGH') list = list.filter(r => r.battery > 75);
        else if (battFilter === 'MEDIUM') list = list.filter(r => r.battery >= 25 && r.battery <= 75);
        else if (battFilter === 'LOW') list = list.filter(r => r.battery <= 25);
        else if (battFilter === 'CRITICAL') list = list.filter(r => r.battery < 15);

        // 4. Search query
        const searchQ = (fleetSearchInput ? fleetSearchInput.value : '').trim().toLowerCase();
        if (searchQ) {
            list = list.filter(r => r.name.toLowerCase().includes(searchQ) || r.id.toLowerCase().includes(searchQ));
        }

        // 5. Sort
        const sortOpt = fleetSortBy ? fleetSortBy.value : 'ID_ASC';
        list.sort((a, b) => {
            if (sortOpt === 'ID_ASC') return a.id.localeCompare(b.id);
            if (sortOpt === 'NAME_ASC') return a.name.localeCompare(b.name);
            if (sortOpt === 'BATT_DESC') return b.battery - a.battery;
            if (sortOpt === 'BATT_ASC') return a.battery - b.battery;
            if (sortOpt === 'LOAD_DESC') return (b.currentPayload || 0) - (a.currentPayload || 0);
            if (sortOpt === 'WAIT_DESC') return (b.waitTicks || 0) - (a.waitTicks || 0);
            return 0;
        });

        return list;
    }

    function renderFleetManagement(s) {
        if (!s || !s.robots) return;
        const robots = s.robots;

        // Update Fleet Overview KPIs
        const total = robots.length;
        const idle = robots.filter(r => r.status === 'IDLE').length;
        const waiting = robots.filter(r => r.status === 'WAITING').length;
        const blocked = robots.filter(r => r.status === 'BLOCKED').length;
        const working = robots.filter(r => r.status !== 'IDLE' && r.status !== 'WAITING' && r.status !== 'BLOCKED').length;
        const totalBatt = robots.reduce((acc, r) => acc + r.battery, 0);
        const avgBatt = total ? Math.round(totalBatt / total) : 100;

        if (fleetKpiTotal) fleetKpiTotal.textContent = total;
        if (fleetKpiIdle) fleetKpiIdle.textContent = idle;
        if (fleetKpiWorking) fleetKpiWorking.textContent = working;
        if (fleetKpiWaiting) fleetKpiWaiting.textContent = waiting;
        if (fleetKpiBlocked) fleetKpiBlocked.textContent = blocked;
        if (fleetKpiAvgBatt) fleetKpiAvgBatt.textContent = `${avgBatt}%`;

        // Render Stage E: Fleet Utilization Analytics Table
        renderFleetUtilizationAnalytics(s);

        // Render Stage E: Fleet Battery Intelligence Radar
        renderFleetBatteryIntelligence(s);

        // Render Filtered Cards Grid
        const filtered = getFilteredAndSortedRobots(robots);
        if (!fleetCardsGrid) return;
        fleetCardsGrid.innerHTML = '';

        if (filtered.length === 0) {
            fleetCardsGrid.innerHTML = `
                <div style="grid-column: 1 / -1; text-align:center; padding:40px; color:var(--text-muted); background:var(--bg-card); border:1px dashed var(--border-card); border-radius:var(--radius-md);">
                    <div style="font-size:28px; margin-bottom:8px;">🔍</div>
                    <strong style="font-size:15px; color:var(--text-primary);">No Robots Match Selected Filters</strong>
                    <p style="font-size:13px; margin-top:4px;">Try selecting "All Types" or clearing the search filter.</p>
                </div>
            `;
            return;
        }

        filtered.forEach(r => {
            const card = document.createElement('div');
            card.className = 'fleet-robot-card';

            const isLow = r.battery < 20;
            const isWarn = r.battery >= 20 && r.battery < 50;
            const fillClass = isLow ? 'danger' : (isWarn ? 'warn' : '');

            // Subclass styling & Avatar
            let avatarClass = 'picker';
            let avatarIcon = '⚡';
            let drainRate = '0.5%';
            if (r.type === 'TransportRobot') {
                avatarClass = 'transport';
                avatarIcon = '🚜';
                drainRate = '1.0%';
            } else if (r.type === 'HybridRobot') {
                avatarClass = 'hybrid';
                avatarIcon = '🔀';
                drainRate = '0.7%';
            }

            // Route Progress & Waypoints preview
            const route = r.route || [];
            const routeIdx = r.routeIndex || 0;
            const remainingSteps = Math.max(0, route.length - routeIdx);
            let routePreviewHTML = '<span style="color:var(--text-muted);font-size:11px;">No active route (Stationary)</span>';
            if (route.length > 0 && remainingSteps > 0) {
                const nextWaypoints = route.slice(routeIdx, routeIdx + 4);
                routePreviewHTML = nextWaypoints.map((pt, i) => 
                    `<span class="route-step-pill ${i === 0 ? 'active' : ''}">(${pt.x}, ${pt.y})</span>`
                ).join('<span style="color:var(--text-muted);">&rarr;</span>');
                if (remainingSteps > 4) {
                    routePreviewHTML += `<span style="font-size:10px;color:var(--text-muted);">+${remainingSteps - 4} more</span>`;
                }
            }

            // Task Details
            let taskHTML = '<span style="color:var(--text-muted);">Standby &bull; Available for order dispatch</span>';
            if (r.currentTask) {
                const t = r.currentTask;
                const prod = state?.inventory?.find(p => p.id === t.productId);
                const prodName = prod ? prod.name : `Product #${t.productId}`;
                taskHTML = `
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong>Task #${t.id} &bull; ${t.type}</strong>
                        <span style="font-family:var(--font-mono);color:var(--accent-blue);font-weight:600;">Order #${t.orderId}</span>
                    </div>
                    <div style="color:var(--text-secondary);font-size:11px;margin-top:2px;">
                        ${prodName} &bull; Target: (${t.targetLocation?.x ?? '--'}, ${t.targetLocation?.y ?? '--'})
                    </div>
                `;
            }

            // Stage E: Utilization & Battery Telemetry calculations
            const m = s.analytics?.robotUtilization?.find(u => u.robotId === r.id) || {};
            const workingUtil = (m.workingUtilization != null ? m.workingUtilization : 0).toFixed(1);
            const trend = m.batteryTrend || 'INSUFFICIENT_HISTORY';
            let trendPillHTML = '';
            if (trend === 'DISCHARGING') {
                trendPillHTML = `<span class="battery-trend-pill DISCHARGING">📉 Discharging (-${(m.batteryDrainRate || 0).toFixed(2)}%/t)</span>`;
            } else if (trend === 'CHARGING') {
                trendPillHTML = `<span class="battery-trend-pill CHARGING">⚡ Charging</span>`;
            } else if (trend === 'STABLE') {
                trendPillHTML = `<span class="battery-trend-pill STABLE">⏸️ Stable</span>`;
            } else {
                trendPillHTML = `<span class="battery-trend-pill INSUFFICIENT_HISTORY">Insufficient history</span>`;
            }

            card.innerHTML = `
                <div class="fleet-card-header">
                    <div class="robot-identity">
                        <div class="robot-avatar ${avatarClass}">${avatarIcon}</div>
                        <div class="robot-title-block">
                            <h3>${escapeHTML(r.name)}</h3>
                            <span class="robot-subtext">${escapeHTML(r.id)} &bull; ${escapeHTML(r.type)}</span>
                        </div>
                    </div>
                    <div class="robot-badges-group">
                        <span class="type-tag">${escapeHTML(r.type)}</span>
                        <div class="status-badge-live ${r.status}">
                            <span class="pulse-dot"></span>
                            ${r.status}
                        </div>
                    </div>
                </div>

                <div class="fleet-battery-section">
                    <div class="fleet-battery-header">
                        <span>Battery Level (Drain: ${drainRate}/move)</span>
                        <strong style="color:${isLow ? '#e11d48' : (isWarn ? '#d97706' : '#059669')}">${Math.round(r.battery)}%</strong>
                    </div>
                    <div class="progress-track" style="height:8px;">
                        <div class="progress-fill ${fillClass}" style="width:${r.battery}%;"></div>
                    </div>
                </div>

                <div class="fleet-specs-grid">
                    <div class="fleet-spec-item">
                        <span class="spec-title">Grid Position</span>
                        <span class="spec-val">(${r.position.x}, ${r.position.y})</span>
                    </div>
                    <div class="fleet-spec-item">
                        <span class="spec-title">Current Payload</span>
                        <span class="spec-val">${r.currentPayload || 0} / ${r.capacity} kg</span>
                    </div>
                    <div class="fleet-spec-item">
                        <span class="spec-title">Waiting Ticks</span>
                        <span class="spec-val" style="color:${r.waitTicks > 0 ? '#d97706' : 'var(--text-primary)'};">${r.waitTicks || 0} ticks</span>
                    </div>
                    <div class="fleet-spec-item">
                        <span class="spec-title">Route Progress</span>
                        <span class="spec-val">${remainingSteps > 0 ? `${remainingSteps} steps left` : 'Completed'}</span>
                    </div>
                </div>

                <div class="fleet-task-box">
                    <div class="fleet-task-header">
                        <span>Active Assignment</span>
                        <span>${r.currentTask ? 'RUNNING' : 'IDLE'}</span>
                    </div>
                    <div>${taskHTML}</div>
                </div>

                <!-- Stage E: Utilization & Battery Telemetry -->
                <div class="fleet-card-util-box">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <span style="font-size:11px;font-weight:700;color:var(--text-secondary);text-transform:uppercase;">Utilization &amp; Telemetry</span>
                        <span style="font-family:var(--font-mono);font-size:12px;font-weight:700;color:var(--accent-blue);" title="Working ticks / Total observed ticks (${m.workingTicks || 0}/${s.analytics?.observedTicks || 0})">
                            ${workingUtil}% Util
                        </span>
                    </div>
                    <div style="display:flex;flex-wrap:wrap;gap:6px;font-size:10.5px;font-family:var(--font-mono);color:var(--text-muted);">
                        <span>Working: <strong>${m.workingTicks || 0}t</strong></span>
                        <span>Idle: <strong>${m.idleTicks || 0}t</strong></span>
                        <span>Waiting: <strong>${m.waitingTicks || 0}t</strong></span>
                        <span>Blocked: <strong>${m.blockedTicks || 0}t</strong></span>
                        <span>Charging: <strong>${m.chargingTicks || 0}t</strong></span>
                    </div>
                    <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--text-secondary);border-top:1px dashed var(--border-subtle);padding-top:4px;margin-top:2px;">
                        <span>Steps: <strong>${m.movementSteps || 0}</strong> &bull; Tasks: <strong>${m.tasksCompleted || 0}</strong></span>
                        <span>Consumed: <strong>${(m.batteryConsumed || 0).toFixed(1)}%</strong></span>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-top:2px;">
                        <span style="font-size:10px;color:var(--text-muted);font-weight:600;text-transform:uppercase;">Battery Trend:</span>
                        ${trendPillHTML}
                    </div>
                    ${(r.battery <= 25.0) ? `<div class="battery-warn-banner">⚠️ Battery at ${Math.round(r.battery)}% (&le; 25% threshold). Scheduled for charging.</div>` : ''}
                </div>

                <div style="display:flex;flex-direction:column;gap:4px;margin-top:6px;">
                    <span style="font-size:10px;font-weight:600;color:var(--text-muted);text-transform:uppercase;">Planned Waypoints</span>
                    <div class="fleet-route-preview">${routePreviewHTML}</div>
                </div>

                <button class="card-inspect-btn" data-robot-id="${r.id}">
                    🔍 Inspect Robot Telemetry
                </button>
            `;

            // Card click & Inspect button click
            card.addEventListener('click', (e) => {
                openRobotDetailsModal(r.id);
            });

            fleetCardsGrid.appendChild(card);
        });
    }

    // Attach Filter / Sort Change Listeners
    if (fleetFilterType) fleetFilterType.addEventListener('change', () => renderFleetManagement(state));
    if (fleetFilterStatus) fleetFilterStatus.addEventListener('change', () => renderFleetManagement(state));
    if (fleetFilterBattery) fleetFilterBattery.addEventListener('change', () => renderFleetManagement(state));
    if (fleetSortBy) fleetSortBy.addEventListener('change', () => renderFleetManagement(state));
    if (fleetSearchInput) fleetSearchInput.addEventListener('input', () => renderFleetManagement(state));
    if (btnResetFleetFilters) {
        btnResetFleetFilters.addEventListener('click', () => {
            if (fleetFilterType) fleetFilterType.value = 'ALL';
            if (fleetFilterStatus) fleetFilterStatus.value = 'ALL';
            if (fleetFilterBattery) fleetFilterBattery.value = 'ALL';
            if (fleetSortBy) fleetSortBy.value = 'ID_ASC';
            if (fleetSearchInput) fleetSearchInput.value = '';
            renderFleetManagement(state);
        });
    }

    // =========================================================================
    // Stage E: Fleet Utilization Comparison Analytics
    // =========================================================================
    function renderFleetUtilizationAnalytics(s) {
        if (!fleetUtilizationComparisonTable) return;
        const observed = s.analytics?.observedTicks || 0;
        if (fleetAnalyticsObservedTicks) {
            fleetAnalyticsObservedTicks.textContent = `${observed} ticks`;
        }

        if (!s.analytics || observed === 0) {
            fleetUtilizationComparisonTable.innerHTML = `
                <div style="padding:24px;text-align:center;color:var(--text-muted);font-size:13px;">
                    <div style="font-size:24px;margin-bottom:6px;">⏱️</div>
                    <strong>Zero Simulation Observations Recorded</strong>
                    <p style="margin-top:4px;font-size:12px;">Step simulation or auto-play to record authoritative robot utilization across working, waiting, blocked, charging, and idle states.</p>
                </div>
            `;
            return;
        }

        const robots = s.robots || [];
        const rows = robots.map(r => {
            const m = s.analytics.robotUtilization?.find(u => u.robotId === r.id) || {};
            const workingUtil = (m.workingUtilization != null ? m.workingUtilization : 0).toFixed(1);
            const idlePct = (m.idlePercentage != null ? m.idlePercentage : 100).toFixed(1);
            const waitPct = (m.waitingPercentage != null ? m.waitingPercentage : 0).toFixed(1);
            const blockPct = (m.blockedPercentage != null ? m.blockedPercentage : 0).toFixed(1);
            const chargePct = (m.chargingPercentage != null ? m.chargingPercentage : 0).toFixed(1);

            return `
                <tr>
                    <td>
                        <strong style="color:var(--text-primary);">${escapeHTML(r.name)}</strong>
                        <span style="font-family:var(--font-mono);color:var(--text-muted);font-size:11px;margin-left:4px;">(${escapeHTML(r.id)})</span>
                    </td>
                    <td style="font-size:12px;">${escapeHTML(r.type)}</td>
                    <td><span class="badge ${r.status}">${r.status}</span></td>
                    <td style="font-family:var(--font-mono);font-weight:700;color:var(--accent-blue);" title="Working: ${m.workingTicks || 0} / ${observed} ticks (${workingUtil}%)">
                        ${workingUtil}%
                    </td>
                    <td style="font-family:var(--font-mono);" title="Idle: ${m.idleTicks || 0} / ${observed} ticks (${idlePct}%)">
                        ${idlePct}%
                    </td>
                    <td style="font-family:var(--font-mono);color:#d97706;" title="Waiting: ${m.waitingTicks || 0} / ${observed} ticks (${waitPct}%)">
                        ${waitPct}%
                    </td>
                    <td style="font-family:var(--font-mono);color:#e11d48;" title="Blocked: ${m.blockedTicks || 0} / ${observed} ticks (${blockPct}%)">
                        ${blockPct}%
                    </td>
                    <td style="font-family:var(--font-mono);color:#059669;" title="Charging: ${m.chargingTicks || 0} / ${observed} ticks (${chargePct}%)">
                        ${chargePct}%
                    </td>
                    <td style="font-family:var(--font-mono);">${m.movementSteps || 0}</td>
                    <td style="font-family:var(--font-mono);font-weight:600;">${m.tasksCompleted || 0}</td>
                </tr>
            `;
        }).join('');

        fleetUtilizationComparisonTable.innerHTML = `
            <table class="data-table" style="font-size:12px;">
                <thead>
                    <tr>
                        <th>Robot</th>
                        <th>Type</th>
                        <th>Status</th>
                        <th title="Working Ticks / Observed Ticks">Working Util %</th>
                        <th title="Idle Ticks / Observed Ticks">Idle %</th>
                        <th title="Waiting Ticks / Observed Ticks">Waiting %</th>
                        <th title="Blocked Ticks / Observed Ticks">Blocked %</th>
                        <th title="Charging Ticks / Observed Ticks">Charging %</th>
                        <th title="Physical grid translation steps">Steps</th>
                        <th title="Delivered task missions completed">Tasks Done</th>
                    </tr>
                </thead>
                <tbody>${rows}</tbody>
            </table>
        `;
    }

    // =========================================================================
    // Stage E: Fleet Battery Intelligence Radar
    // =========================================================================
    function renderFleetBatteryIntelligence(s) {
        if (!fleetBatteryRadarList) return;
        const a = s.analytics || {};
        const robots = s.robots || [];

        // Battery Quick KPI values
        if (fleetBattRangeVal) {
            const minB = a.fleetLowestBattery != null ? Math.round(a.fleetLowestBattery) : (robots.length ? Math.min(...robots.map(r => Math.round(r.battery))) : 100);
            const maxB = a.fleetHighestBattery != null ? Math.round(a.fleetHighestBattery) : (robots.length ? Math.max(...robots.map(r => Math.round(r.battery))) : 100);
            const minId = a.fleetLowestBatteryRobotId || (robots.find(r => Math.round(r.battery) === minB)?.id || 'R01');
            const maxId = a.fleetHighestBatteryRobotId || (robots.find(r => Math.round(r.battery) === maxB)?.id || 'R01');
            fleetBattRangeVal.textContent = `${minB}% (${minId}) – ${maxB}% (${maxId})`;
        }

        if (fleetBattConsumedVal) {
            const consumed = a.fleetTotalBatteryConsumed != null ? a.fleetTotalBatteryConsumed : 0;
            fleetBattConsumedVal.textContent = `${consumed.toFixed(1)}%`;
        }

        if (fleetChargingEventsVal) {
            const events = a.fleetTotalChargingEvents != null ? a.fleetTotalChargingEvents : 0;
            const ticks = a.fleetTotalChargingTicks != null ? a.fleetTotalChargingTicks : 0;
            fleetChargingEventsVal.textContent = `${events} events (${ticks}t)`;
        }

        // Fleet Health Badge
        if (fleetBatteryHealthBadge) {
            const anyCritical = robots.some(r => r.battery < 15.0 || r.status === 'LOW_BATTERY');
            const anyLow = robots.some(r => r.battery <= 25.0);
            if (anyCritical) {
                fleetBatteryHealthBadge.style.background = '#ffe4e6';
                fleetBatteryHealthBadge.style.color = '#e11d48';
                fleetBatteryHealthBadge.textContent = 'CRITICAL BATTERY ALERT';
            } else if (anyLow) {
                fleetBatteryHealthBadge.style.background = '#fef3c7';
                fleetBatteryHealthBadge.style.color = '#d97706';
                fleetBatteryHealthBadge.textContent = 'LOW BATTERY WARNING';
            } else {
                fleetBatteryHealthBadge.style.background = 'var(--accent-emerald-soft)';
                fleetBatteryHealthBadge.style.color = 'var(--accent-emerald)';
                fleetBatteryHealthBadge.textContent = 'HEALTH OPTIMAL';
            }
        }

        // Per-Robot Battery Telemetry & Trend List
        fleetBatteryRadarList.innerHTML = '';
        robots.forEach(r => {
            const m = a.robotUtilization?.find(u => u.robotId === r.id) || {};
            const isCritical = r.battery < 15.0;
            const isLow = r.battery <= 25.0 && !isCritical;
            const fillClass = isCritical ? 'danger' : (isLow ? 'warn' : '');

            // Trend badge
            const trend = m.batteryTrend || 'INSUFFICIENT_HISTORY';
            let trendHTML = '';
            if (trend === 'DISCHARGING') {
                const drain = m.batteryDrainRate != null ? m.batteryDrainRate : 0;
                trendHTML = `<span class="battery-trend-pill DISCHARGING" title="Active battery discharge rate: -${drain.toFixed(2)}% per simulation tick">📉 Discharging (-${drain.toFixed(2)}%/t)</span>`;
            } else if (trend === 'CHARGING') {
                trendHTML = `<span class="battery-trend-pill CHARGING" title="Robot charging bay telemetry active">⚡ Charging</span>`;
            } else if (trend === 'STABLE') {
                trendHTML = `<span class="battery-trend-pill STABLE" title="Stable battery level (no significant delta over recent samples)">⏸️ Stable</span>`;
            } else {
                trendHTML = `<span class="battery-trend-pill INSUFFICIENT_HISTORY" title="Insufficient historical tick samples recorded to compute trend velocity">Insufficient history</span>`;
            }

            // Historical Sparkline
            let sparklineHTML = '';
            if (m.batteryHistory && m.batteryHistory.length >= 2) {
                const bars = m.batteryHistory.slice(-12).map(b => {
                    const h = Math.max(3, Math.round((b / 100.0) * 14));
                    const isLowBar = b <= 25;
                    return `<div class="sparkline-bar ${isLowBar ? 'low' : ''}" style="height:${h}px;" title="${b.toFixed(1)}%"></div>`;
                }).join('');
                sparklineHTML = `<div class="sparkline-history" title="Recent battery telemetry samples">${bars}</div>`;
            }

            const item = document.createElement('div');
            item.className = 'fleet-radar-card';
            item.innerHTML = `
                <div style="display:flex;align-items:center;gap:10px;min-width:130px;">
                    <strong style="color:var(--text-primary);font-size:13px;">${escapeHTML(r.id)}</strong>
                    <div>
                        <div style="font-weight:600;font-size:12px;color:var(--text-primary);">${escapeHTML(r.name)}</div>
                        <div style="font-size:10.5px;color:var(--text-muted);">${escapeHTML(r.type)}</div>
                    </div>
                </div>

                <div style="flex:1;max-width:240px;margin:0 10px;">
                    <div style="display:flex;justify-content:space-between;font-size:11px;font-family:var(--font-mono);margin-bottom:3px;">
                        <span>Level</span>
                        <strong style="color:${isCritical ? '#e11d48' : (isLow ? '#d97706' : '#059669')};">${Math.round(r.battery)}%</strong>
                    </div>
                    <div class="progress-track" style="height:6px;">
                        <div class="progress-fill ${fillClass}" style="width:${r.battery}%;"></div>
                    </div>
                </div>

                <div style="display:flex;align-items:center;gap:10px;">
                    ${sparklineHTML}
                    ${trendHTML}
                    <span style="font-family:var(--font-mono);font-size:11px;color:var(--text-muted);" title="Total consumed battery: ${(m.batteryConsumed || 0).toFixed(1)}%">
                        &Sigma; ${(m.batteryConsumed || 0).toFixed(1)}%
                    </span>
                </div>
            `;
            fleetBatteryRadarList.appendChild(item);
        });
    }

    // =========================================================================
    // Robot Details Telemetry Modal
    // =========================================================================
    function openRobotDetailsModal(robotId) {
        if (!state || !state.robots) return;
        const r = state.robots.find(bot => bot.id === robotId);
        if (!r) return;

        modalRobotName.textContent = `${r.name} (${r.id})`;
        modalRobotSubtext.textContent = `${r.type} • Status: ${r.status}`;

        let icon = '⚡';
        let avClass = 'picker';
        if (r.type === 'TransportRobot') { icon = '🚜'; avClass = 'transport'; }
        else if (r.type === 'HybridRobot') { icon = '🔀'; avClass = 'hybrid'; }
        modalRobotAvatar.className = `robot-avatar ${avClass}`;
        modalRobotAvatar.textContent = icon;

        const route = r.route || [];
        const routeIdx = r.routeIndex || 0;

        let waypointsListHTML = '<p style="color:var(--text-muted);font-size:13px;">No waypoints currently planned.</p>';
        if (route.length > 0) {
            waypointsListHTML = route.map((pt, i) => {
                const isCurrent = i === routeIdx;
                const isPast = i < routeIdx;
                return `
                    <div style="display:flex;align-items:center;justify-content:space-between;padding:6px 10px;background:${isCurrent ? 'var(--accent-blue-soft)' : 'var(--bg-card-subtle)'};border:1px solid ${isCurrent ? 'var(--accent-blue-border)' : 'var(--border-subtle)'};border-radius:var(--radius-xs);font-size:12px;font-family:var(--font-mono);">
                        <span>Step ${i}: <strong>(${pt.x}, ${pt.y})</strong></span>
                        <span style="font-weight:600;color:${isCurrent ? 'var(--accent-blue)' : (isPast ? '#10b981' : 'var(--text-muted)')};">
                            ${isCurrent ? 'ACTIVE NEXT STEP' : (isPast ? 'COMPLETED' : 'PLANNED')}
                        </span>
                    </div>
                `;
            }).join('');
        }

        const m = state.analytics?.robotUtilization?.find(u => u.robotId === r.id) || {};
        const observed = state.analytics?.observedTicks || 0;
        const workingUtil = (m.workingUtilization != null ? m.workingUtilization : 0).toFixed(1);
        const idlePct = (m.idlePercentage != null ? m.idlePercentage : 100).toFixed(1);
        const waitPct = (m.waitingPercentage != null ? m.waitingPercentage : 0).toFixed(1);
        const blockPct = (m.blockedPercentage != null ? m.blockedPercentage : 0).toFixed(1);
        const chargePct = (m.chargingPercentage != null ? m.chargingPercentage : 0).toFixed(1);

        const trend = m.batteryTrend || 'INSUFFICIENT_HISTORY';
        let trendModalHTML = '';
        if (trend === 'DISCHARGING') {
            trendModalHTML = `<span class="battery-trend-pill DISCHARGING">📉 Discharging (-${(m.batteryDrainRate || 0).toFixed(2)}%/tick)</span>`;
        } else if (trend === 'CHARGING') {
            trendModalHTML = `<span class="battery-trend-pill CHARGING">⚡ Charging</span>`;
        } else if (trend === 'STABLE') {
            trendModalHTML = `<span class="battery-trend-pill STABLE">⏸️ Stable</span>`;
        } else {
            trendModalHTML = `<span class="battery-trend-pill INSUFFICIENT_HISTORY">Insufficient history</span>`;
        }

        modalRobotBody.innerHTML = `
            <div class="fleet-specs-grid" style="grid-template-columns:repeat(3, 1fr);">
                <div class="fleet-spec-item">
                    <span class="spec-title">Battery Level</span>
                    <span class="spec-val" style="color:${r.battery <= 25 ? '#e11d48' : '#059669'};">${Math.round(r.battery)}%</span>
                </div>
                <div class="fleet-spec-item">
                    <span class="spec-title">Grid Position</span>
                    <span class="spec-val">(${r.position.x}, ${r.position.y})</span>
                </div>
                <div class="fleet-spec-item">
                    <span class="spec-title">Waiting Ticks</span>
                    <span class="spec-val">${r.waitTicks || 0} ticks</span>
                </div>
                <div class="fleet-spec-item">
                    <span class="spec-title">Payload Load</span>
                    <span class="spec-val">${r.currentPayload || 0} / ${r.capacity} kg</span>
                </div>
                <div class="fleet-spec-item">
                    <span class="spec-title">Movement Steps</span>
                    <span class="spec-val">${m.movementSteps || 0} steps</span>
                </div>
                <div class="fleet-spec-item">
                    <span class="spec-title">Tasks Completed</span>
                    <span class="spec-val">${m.tasksCompleted || 0} tasks</span>
                </div>
            </div>

            <!-- Stage E: Authoritative Utilization Breakdown Table -->
            <div style="background:var(--bg-card-subtle);border:1px solid var(--border-subtle);border-radius:var(--radius-sm);padding:12px;">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                    <strong style="font-size:13px;color:var(--text-primary);">Operational Utilization Breakdown</strong>
                    <span style="font-size:11px;font-family:var(--font-mono);color:var(--text-muted);">Observed: ${observed} ticks</span>
                </div>
                <div class="data-table-container">
                    <table class="data-table" style="font-size:12px;">
                        <thead>
                            <tr>
                                <th>State Category</th>
                                <th>Classification Statuses</th>
                                <th>Ticks</th>
                                <th>Percentage</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td><span class="legend-dot working"></span><strong>Working</strong></td>
                                <td style="font-size:11px;color:var(--text-secondary);">MOVING_TO_PICK, PICKING, MOVING_TO_PACK, DELIVERING, RETURNING_TO_CHARGE, LOW_BATTERY</td>
                                <td style="font-family:var(--font-mono);font-weight:700;">${m.workingTicks || 0}t</td>
                                <td style="font-family:var(--font-mono);font-weight:700;color:var(--accent-blue);">${workingUtil}%</td>
                            </tr>
                            <tr>
                                <td><span class="legend-dot idle"></span><strong>Idle</strong></td>
                                <td style="font-size:11px;color:var(--text-secondary);">IDLE (Standby awaiting dispatch)</td>
                                <td style="font-family:var(--font-mono);">${m.idleTicks || 0}t</td>
                                <td style="font-family:var(--font-mono);">${idlePct}%</td>
                            </tr>
                            <tr>
                                <td><span class="legend-dot waiting"></span><strong>Waiting</strong></td>
                                <td style="font-size:11px;color:var(--text-secondary);">WAITING (Contested cell reservation yield)</td>
                                <td style="font-family:var(--font-mono);">${m.waitingTicks || 0}t</td>
                                <td style="font-family:var(--font-mono);color:#d97706;">${waitPct}%</td>
                            </tr>
                            <tr>
                                <td><span class="legend-dot" style="background:#e11d48;"></span><strong>Blocked</strong></td>
                                <td style="font-size:11px;color:var(--text-secondary);">BLOCKED (Temporary corridor obstruction)</td>
                                <td style="font-family:var(--font-mono);">${m.blockedTicks || 0}t</td>
                                <td style="font-family:var(--font-mono);color:#e11d48;">${blockPct}%</td>
                            </tr>
                            <tr>
                                <td><span class="legend-dot charging"></span><strong>Charging</strong></td>
                                <td style="font-size:11px;color:var(--text-secondary);">CHARGING (At designated charging bay)</td>
                                <td style="font-family:var(--font-mono);">${m.chargingTicks || 0}t</td>
                                <td style="font-family:var(--font-mono);color:#059669;">${chargePct}%</td>
                            </tr>
                        </tbody>
                    </table>
                </div>
                <div style="font-size:11px;color:var(--text-muted);margin-top:6px;">
                    <em>Note: Denominator is authoritative observed simulation duration (${observed} ticks).</em>
                </div>
            </div>

            <!-- Stage E: Battery Intelligence & Trend Details -->
            <div style="background:var(--bg-card-subtle);border:1px solid var(--border-subtle);border-radius:var(--radius-sm);padding:12px;">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px;">
                    <strong style="font-size:13px;color:var(--text-primary);">Battery Intelligence &amp; Velocity</strong>
                    ${trendModalHTML}
                </div>
                <div style="display:grid;grid-template-columns:repeat(3, 1fr);gap:8px;font-size:12px;">
                    <div>Consumed: <strong style="font-family:var(--font-mono);">${(m.batteryConsumed || 0).toFixed(1)}%</strong></div>
                    <div>Charge Events: <strong style="font-family:var(--font-mono);">${m.chargingEvents || 0}</strong></div>
                    <div>Drain Velocity: <strong style="font-family:var(--font-mono);">${(m.batteryDrainRate || 0).toFixed(2)}%/t</strong></div>
                </div>
                ${(r.battery <= 25.0) ? `<div class="battery-warn-banner">⚠️ Battery at ${Math.round(r.battery)}% (&le; 25% threshold). Route to charging bay recommended.</div>` : ''}
            </div>

            <div>
                <strong style="font-size:13px;color:var(--text-primary);display:block;margin-bottom:6px;">Current Mission Task</strong>
                <div class="fleet-task-box">
                    ${r.currentTask ? `
                        <div style="display:flex;justify-content:space-between;font-weight:700;">
                            <span>Task #${r.currentTask.id} (${r.currentTask.type})</span>
                            <span style="color:var(--accent-blue);">Order #${r.currentTask.orderId}</span>
                        </div>
                        <div style="font-size:12px;color:var(--text-secondary);margin-top:4px;">
                            Target Location: (${r.currentTask.targetLocation?.x ?? '--'}, ${r.currentTask.targetLocation?.y ?? '--'}) • Product ID: ${r.currentTask.productId} • Quantity: ${r.currentTask.quantity}
                        </div>
                    ` : '<span style="color:var(--text-muted);font-size:12px;">No active task assigned. Robot is currently IDLE.</span>'}
                </div>
            </div>

            <div>
                <strong style="font-size:13px;color:var(--text-primary);display:block;margin-bottom:6px;">Space-Time Route Schedule (${route.length} Steps)</strong>
                <div style="max-height:160px;overflow-y:auto;display:flex;flex-direction:column;gap:4px;padding-right:4px;">
                    ${waypointsListHTML}
                </div>
            </div>

            <div>
                <details style="font-size:12px;color:var(--text-muted);cursor:pointer;">
                    <summary style="font-weight:600;padding:6px 0;">View Raw Telemetry JSON</summary>
                    <pre style="background:var(--bg-card-subtle);border:1px solid var(--border-subtle);padding:10px;border-radius:var(--radius-xs);overflow-x:auto;font-family:var(--font-mono);font-size:11px;margin-top:6px;">${JSON.stringify({ ...r, utilization: m }, null, 2)}</pre>
                </details>
            </div>
        `;

        robotDetailsModal.classList.add('active');
    }

    if (btnModalClose) {
        btnModalClose.addEventListener('click', () => {
            robotDetailsModal.classList.remove('active');
        });
    }

    if (robotDetailsModal) {
        robotDetailsModal.addEventListener('click', (e) => {
            if (e.target === robotDetailsModal) {
                robotDetailsModal.classList.remove('active');
            }
        });
    }

    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            if (robotDetailsModal?.classList.contains('active')) {
                robotDetailsModal.classList.remove('active');
            }
            if (benchmarkSuiteModal?.classList.contains('active')) {
                benchmarkSuiteModal.classList.remove('active');
            }
        }
    });

    // =========================================================================
    // Orders View Rendering
    // =========================================================================
    function renderOrdersView(s) {
        if (!s || !s.orders) return;
        const orders = s.orders;

        // Active List
        if (ordersViewActiveList) {
            ordersViewActiveList.innerHTML = '';
            const activeAndPending = [...(orders.pendingOrders || []), ...(orders.activeOrders || [])];
            if (activeAndPending.length === 0) {
                ordersViewActiveList.innerHTML = '<div style="color:var(--text-muted);font-size:13px;padding:14px;text-align:center;">No pending or in-transit orders.</div>';
            } else {
                activeAndPending.forEach(o => {
                    const prod = s.inventory?.find(p => p.id === o.productId);
                    const card = document.createElement('div');
                    card.className = 'order-item-card';
                    const progClass = o.status;
                    const robotLabel = o.assignedRobotId || (o.teamRobotIds && o.teamRobotIds.length > 1 ? 'Team (' + o.teamRobotIds.join(', ') + ')' : 'Pending Scheduling');
                    card.innerHTML = `
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <span style="font-family:var(--font-mono);font-size:13px;font-weight:700;color:var(--text-primary);">Order #${o.id}</span>
                            <span class="badge ${o.status}">${o.status}</span>
                        </div>
                        <div style="font-size:14px;font-weight:600;color:var(--text-secondary);">${prod ? prod.name : 'Product #' + o.productId} &times; ${o.quantity}</div>
                        <div style="display:flex;justify-content:space-between;font-size:12px;color:var(--text-muted);font-family:var(--font-mono);">
                            <span>Robot: <strong>${robotLabel}</strong></span>
                            <span>Priority: <strong>${o.priority}</strong></span>
                        </div>
                        <div class="order-progress-track">
                            <div class="order-progress-bar ${progClass}"></div>
                        </div>
                    `;
                    ordersViewActiveList.appendChild(card);
                });
            }
        }

        // Completed List
        if (ordersViewCompletedList) {
            ordersViewCompletedList.innerHTML = '';
            const completed = orders.completedOrders || [];
            if (completed.length === 0) {
                ordersViewCompletedList.innerHTML = '<div style="color:var(--text-muted);font-size:13px;padding:14px;text-align:center;">No completed orders yet.</div>';
            } else {
                completed.slice(-8).reverse().forEach(o => {
                    const prod = s.inventory?.find(p => p.id === o.productId);
                    const card = document.createElement('div');
                    card.className = 'order-item-card';
                    card.style.background = 'var(--bg-card-subtle)';
                    card.innerHTML = `
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <span style="font-family:var(--font-mono);font-size:13px;font-weight:700;color:var(--text-primary);">Order #${o.id}</span>
                            <span class="badge COMPLETED">DELIVERED &bull; COMPLETED</span>
                        </div>
                        <div style="font-size:13px;color:var(--text-secondary);">${prod ? prod.name : 'Product #' + o.productId} &times; ${o.quantity}</div>
                        <div style="font-size:11px;color:var(--text-muted);font-family:var(--font-mono);">
                            Fulfilled by Robot ${o.assignedRobotId || '--'} &bull; Delivered to Packing Station
                        </div>
                    `;
                    ordersViewCompletedList.appendChild(card);
                });
            }
        }
    }

    // =========================================================================
    // Inventory View Rendering
    // =========================================================================
    function renderInventoryView(s) {
        if (!s || !s.inventory || !inventoryViewTableBody) return;
        inventoryViewTableBody.innerHTML = '';

        let list = [...s.inventory];
        const searchQ = (inventorySearchInput ? inventorySearchInput.value : '').trim().toLowerCase();
        if (searchQ) {
            list = list.filter(p => p.name.toLowerCase().includes(searchQ) || p.category.toLowerCase().includes(searchQ) || String(p.id).includes(searchQ));
        }

        list.forEach(p => {
            const row = document.createElement('tr');
            const isLow = p.quantity <= 10;
            const isOut = p.quantity === 0;
            const badgeClass = isOut ? 'badge rose' : (isLow ? 'badge amber' : 'badge green');
            const badgeText = isOut ? 'Out of Stock' : (isLow ? 'Low Stock' : 'In Stock');

            row.innerHTML = `
                <td style="font-family:var(--font-mono);font-weight:700;color:var(--accent-blue);">#${p.id}</td>
                <td>
                    <strong>${p.name}</strong><br>
                    <span style="color:var(--text-muted);font-size:11px;">Category: ${p.category}</span>
                </td>
                <td style="font-family:var(--font-mono);font-weight:600;">(${p.location.x}, ${p.location.y})</td>
                <td style="font-family:var(--font-mono);font-weight:700;font-size:14px;">${p.quantity} units</td>
                <td style="font-family:var(--font-mono);font-weight:600;">$${p.price.toFixed(2)}</td>
                <td><span class="${badgeClass}">${badgeText}</span></td>
            `;
            inventoryViewTableBody.appendChild(row);
        });
    }

    if (inventorySearchInput) {
        inventorySearchInput.addEventListener('input', () => renderInventoryView(state));
    }

    // =========================================================================
    // Algorithms View Benchmark Runner
    // =========================================================================
    async function runBenchmarkEngine(sx, sy, gx, gy, tableBodyEl, buttonEl) {
        if (buttonEl.disabled) return;
        buttonEl.disabled = true;
        buttonEl.textContent = 'Computing Optimal Paths...';

        try {
            const comp = await api.compareAlgorithms({ x: sx, y: sy }, { x: gx, y: gy });
            if (comp && comp.results && tableBodyEl) {
                tableBodyEl.innerHTML = '';
                comp.results.forEach(res => {
                    const row = document.createElement('tr');
                    if (res.algorithmName === 'A*') row.className = 'highlight';
                    row.innerHTML = `
                        <td style="font-weight:700;">${res.algorithmName}</td>
                        <td style="font-family:var(--font-mono);">${res.path ? res.path.length : 0} cells</td>
                        <td style="font-family:var(--font-mono);font-weight:700;">${res.nodesVisited}</td>
                        <td style="font-family:var(--font-mono);">${res.totalCost.toFixed(1)}</td>
                        <td style="font-family:var(--font-mono);">${res.executionTimeMicroseconds.toFixed(2)} µs</td>
                        <td>${res.found ? '✓ Solved' : '❌ Unreachable'}</td>
                    `;
                    tableBodyEl.appendChild(row);
                });

                const sourceRow = document.createElement('tr');
                sourceRow.innerHTML = `
                    <td colspan="6" style="font-size:11px;color:var(--text-muted);padding:8px 12px;background:var(--bg-card-subtle);">
                        Source Engine: <strong>${comp.source === 'native_cpp' ? '🟢 Native C++ PathFinder Engine (via /api/compare)' : '🔵 Client-Side PathFinder Emulation (Offline)'}</strong>
                    </td>
                `;
                tableBodyEl.appendChild(sourceRow);

                const astar = comp.results.find(r => r.algorithmName === 'A*');
                if (astar && astar.found) {
                    canvas.setComparisonPath(astar);
                }
            }
        } catch (err) {
            console.error('[BenchmarkEngine] Compare error:', err);
            if (tableBodyEl) {
                tableBodyEl.innerHTML = `
                    <tr>
                        <td colspan="6" style="color:var(--accent-rose);padding:14px;text-align:center;background:var(--accent-rose-soft);">
                            ❌ Algorithm comparison failed: ${err.message || 'Error communicating with backend'}
                        </td>
                    </tr>
                `;
            }
        } finally {
            buttonEl.disabled = false;
            buttonEl.textContent = '▶️ Run Benchmark';
        }
    }

    if (btnRunBenchmark) {
        btnRunBenchmark.addEventListener('click', () => {
            const sx = parseInt(startXInput.value) || 0;
            const sy = parseInt(startYInput.value) || 0;
            const gx = parseInt(goalXInput.value) || 14;
            const gy = parseInt(goalYInput.value) || 14;
            runBenchmarkEngine(sx, sy, gx, gy, benchmarkResultsTable, btnRunBenchmark);
        });
    }

    // =========================================================================
    // DSA Algorithm Laboratory Controller
    // =========================================================================
    let selectedLabAlgo = 'A*';
    let lastLabComparisonResult = null;
    let lastLabSingleResult = null;

    // 1. Algorithm Mode Filter Pills
    if (algoModePills) {
        algoModePills.querySelectorAll('.filter-pill').forEach(pill => {
            pill.addEventListener('click', () => {
                algoModePills.querySelectorAll('.filter-pill').forEach(p => p.classList.remove('active'));
                pill.classList.add('active');
                selectedLabAlgo = pill.dataset.algo || 'A*';
                if (btnAlgoRun) {
                    btnAlgoRun.textContent = (selectedLabAlgo === 'COMPARE') ? '⚡ Run Comparison' : `▶️ Run ${selectedLabAlgo}`;
                }
            });
        });
    }

    // 2. Grid Tool Buttons
    function clearToolActiveStates() {
        if (btnToolSetStart) btnToolSetStart.classList.remove('active');
        if (btnToolSetGoal) btnToolSetGoal.classList.remove('active');
        if (btnToolToggleObstacle) btnToolToggleObstacle.classList.remove('active');
    }

    if (btnToolSetStart) {
        btnToolSetStart.addEventListener('click', () => {
            const isActive = btnToolSetStart.classList.contains('active');
            clearToolActiveStates();
            if (!isActive && algoCanvasInstance) {
                btnToolSetStart.classList.add('active');
                algoCanvasInstance.setInteractionMode('start');
            } else if (algoCanvasInstance) {
                algoCanvasInstance.setInteractionMode(null);
            }
        });
    }

    if (btnToolSetGoal) {
        btnToolSetGoal.addEventListener('click', () => {
            const isActive = btnToolSetGoal.classList.contains('active');
            clearToolActiveStates();
            if (!isActive && algoCanvasInstance) {
                btnToolSetGoal.classList.add('active');
                algoCanvasInstance.setInteractionMode('goal');
            } else if (algoCanvasInstance) {
                algoCanvasInstance.setInteractionMode(null);
            }
        });
    }

    if (btnToolToggleObstacle) {
        btnToolToggleObstacle.addEventListener('click', () => {
            const isActive = btnToolToggleObstacle.classList.contains('active');
            clearToolActiveStates();
            if (!isActive && algoCanvasInstance) {
                btnToolToggleObstacle.classList.add('active');
                algoCanvasInstance.setInteractionMode('obstacle');
            } else if (algoCanvasInstance) {
                algoCanvasInstance.setInteractionMode(null);
            }
        });
    }

    if (btnClearCustomObstacles) {
        btnClearCustomObstacles.addEventListener('click', () => {
            if (algoCanvasInstance) algoCanvasInstance.clearCustomObstacles();
            clearToolActiveStates();
        });
    }

    if (btnResetLabDefaults) {
        btnResetLabDefaults.addEventListener('click', () => {
            if (algoStartX) algoStartX.value = 0;
            if (algoStartY) algoStartY.value = 0;
            if (algoGoalX) algoGoalX.value = 14;
            if (algoGoalY) algoGoalY.value = 14;
            if (algoCanvasInstance) {
                algoCanvasInstance.clearCustomObstacles();
                algoCanvasInstance.setStart(0, 0);
                algoCanvasInstance.setGoal(14, 14);
                algoCanvasInstance.resetAnimation();
            }
            clearToolActiveStates();
        });
    }

    // Coordinates inputs change listener
    [algoStartX, algoStartY].forEach(input => {
        if (input) {
            input.addEventListener('change', () => {
                const sx = parseInt(algoStartX.value) || 0;
                const sy = parseInt(algoStartY.value) || 0;
                if (algoCanvasInstance) algoCanvasInstance.setStart(sx, sy);
            });
        }
    });

    [algoGoalX, algoGoalY].forEach(input => {
        if (input) {
            input.addEventListener('change', () => {
                const gx = parseInt(algoGoalX.value) || 0;
                const gy = parseInt(algoGoalY.value) || 0;
                if (algoCanvasInstance) algoCanvasInstance.setGoal(gx, gy);
            });
        }
    });

    if (algoCanvasInstance) {
        algoCanvasInstance.onCoordinatesChange = (start, goal) => {
            if (algoStartX) algoStartX.value = start.x;
            if (algoStartY) algoStartY.value = start.y;
            if (algoGoalX) algoGoalX.value = goal.x;
            if (algoGoalY) algoGoalY.value = goal.y;
            clearToolActiveStates();
        };
    }

    // Speed Slider
    if (algoSpeedSlider) {
        algoSpeedSlider.addEventListener('input', () => {
            const val = parseInt(algoSpeedSlider.value) || 40;
            if (algoSpeedLabel) algoSpeedLabel.textContent = `${val}ms`;
            if (algoCanvasInstance) algoCanvasInstance.setSpeed(val);
        });
    }

    // Playback Controls
    if (btnAlgoPauseResume) {
        btnAlgoPauseResume.addEventListener('click', () => {
            if (!algoCanvasInstance) return;
            if (algoCanvasInstance.isPaused) {
                algoCanvasInstance.resumeAnimation();
                btnAlgoPauseResume.textContent = '⏯ Pause';
                if (algoStatusBadge) algoStatusBadge.textContent = 'Exploring...';
            } else {
                algoCanvasInstance.pauseAnimation();
                btnAlgoPauseResume.textContent = '▶️ Resume';
                if (algoStatusBadge) algoStatusBadge.textContent = 'Paused';
            }
        });
    }

    if (btnAlgoStep) {
        btnAlgoStep.addEventListener('click', () => {
            if (algoCanvasInstance) {
                algoCanvasInstance.stepForward();
                if (algoStatusBadge) algoStatusBadge.textContent = 'Stepped';
            }
        });
    }

    if (btnAlgoResetAnim) {
        btnAlgoResetAnim.addEventListener('click', () => {
            if (algoCanvasInstance) {
                algoCanvasInstance.resetAnimation();
                if (btnAlgoPauseResume) btnAlgoPauseResume.textContent = '⏯ Pause';
                if (algoStatusBadge) algoStatusBadge.textContent = 'Ready';
            }
        });
    }

    // Helper to update metric cards
    function updateAlgoMetricCards(res, efficiencyText = '--') {
        if (!res) return;
        if (algoMetricTime) algoMetricTime.textContent = `${(res.executionTimeMicroseconds || 0).toFixed(2)} µs`;
        if (algoMetricExplored) algoMetricExplored.textContent = (res.nodesExplored !== undefined ? res.nodesExplored : res.nodesVisited || 0);
        if (algoMetricLength) algoMetricLength.textContent = `${res.path ? res.path.length : 0} cells`;
        if (algoMetricSteps) algoMetricSteps.textContent = `${Math.max(0, (res.path ? res.path.length : 1) - 1)} steps`;
        if (algoMetricCost) algoMetricCost.textContent = (res.totalCost || 0).toFixed(1);
        if (algoMetricEfficiency) algoMetricEfficiency.textContent = efficiencyText;
    }

    // Helper to update comparative bar charts
    function updateAlgoCharts(bfsRes, dijkstraRes, astarRes) {
        const bfsNodes = bfsRes ? (bfsRes.nodesExplored !== undefined ? bfsRes.nodesExplored : bfsRes.nodesVisited) : 0;
        const dijkstraNodes = dijkstraRes ? (dijkstraRes.nodesExplored !== undefined ? dijkstraRes.nodesExplored : dijkstraRes.nodesVisited) : 0;
        const astarNodes = astarRes ? (astarRes.nodesExplored !== undefined ? astarRes.nodesExplored : astarRes.nodesVisited) : 0;

        const maxNodes = Math.max(1, bfsNodes, dijkstraNodes, astarNodes);
        if (chartBfsNodes) chartBfsNodes.textContent = bfsNodes;
        if (chartDijkstraNodes) chartDijkstraNodes.textContent = dijkstraNodes;
        if (chartAstarNodes) chartAstarNodes.textContent = astarNodes;

        if (barBfsNodes) barBfsNodes.style.width = `${Math.round((bfsNodes / maxNodes) * 100)}%`;
        if (barDijkstraNodes) barDijkstraNodes.style.width = `${Math.round((dijkstraNodes / maxNodes) * 100)}%`;
        if (barAstarNodes) barAstarNodes.style.width = `${Math.round((astarNodes / maxNodes) * 100)}%`;

        const bfsTime = bfsRes ? (bfsRes.executionTimeMicroseconds || 0) : 0;
        const dijkstraTime = dijkstraRes ? (dijkstraRes.executionTimeMicroseconds || 0) : 0;
        const astarTime = astarRes ? (astarRes.executionTimeMicroseconds || 0) : 0;

        const maxTime = Math.max(0.1, bfsTime, dijkstraTime, astarTime);
        if (chartBfsTime) chartBfsTime.textContent = `${bfsTime.toFixed(2)} µs`;
        if (chartDijkstraTime) chartDijkstraTime.textContent = `${dijkstraTime.toFixed(2)} µs`;
        if (chartAstarTime) chartAstarTime.textContent = `${astarTime.toFixed(2)} µs`;

        if (barBfsTime) barBfsTime.style.width = `${Math.round((bfsTime / maxTime) * 100)}%`;
        if (barDijkstraTime) barDijkstraTime.style.width = `${Math.round((dijkstraTime / maxTime) * 100)}%`;
        if (barAstarTime) barAstarTime.style.width = `${Math.round((astarTime / maxTime) * 100)}%`;
    }

    // Run Algorithm Button
    if (btnAlgoRun) {
        btnAlgoRun.addEventListener('click', async () => {
            if (btnAlgoRun.disabled) return;
            const sx = parseInt(algoStartX.value) || 0;
            const sy = parseInt(algoStartY.value) || 0;
            const gx = parseInt(algoGoalX.value) || 14;
            const gy = parseInt(algoGoalY.value) || 14;

            const start = { x: sx, y: sy };
            const goal = { x: gx, y: gy };
            const customObstacles = algoCanvasInstance ? algoCanvasInstance.getCustomObstaclesList() : [];

            btnAlgoRun.disabled = true;
            if (algoStatusBadge) {
                algoStatusBadge.textContent = 'Computing...';
                algoStatusBadge.style.background = '#fef3c7';
                algoStatusBadge.style.color = '#b45309';
            }

            try {
                if (selectedLabAlgo === 'COMPARE') {
                    const comp = await api.compareAlgorithmsLab(start, goal, customObstacles);
                    lastLabComparisonResult = comp;

                    if (algoResultsTableBody && comp && comp.results) {
                        algoResultsTableBody.innerHTML = '';
                        comp.results.forEach(res => {
                            const row = document.createElement('tr');
                            if (res.algorithmName === 'A*') row.className = 'highlight';
                            const steps = res.path ? Math.max(0, res.path.length - 1) : 0;
                            row.innerHTML = `
                                <td style="font-weight:700;">${res.algorithmName}</td>
                                <td style="font-family:var(--font-mono);">${res.path ? res.path.length : 0} cells</td>
                                <td style="font-family:var(--font-mono);">${steps} steps</td>
                                <td style="font-family:var(--font-mono);font-weight:700;">${res.nodesExplored !== undefined ? res.nodesExplored : res.nodesVisited}</td>
                                <td style="font-family:var(--font-mono);">${res.totalCost.toFixed(1)}</td>
                                <td style="font-family:var(--font-mono);">${res.executionTimeMicroseconds.toFixed(2)} µs</td>
                                <td>${res.found ? '<span style="color:#10b981;font-weight:700;">✓ Solved</span>' : '<span style="color:#ef4444;font-weight:700;">❌ ' + (res.statusMessage || 'Unreachable') + '</span>'}</td>
                            `;
                            algoResultsTableBody.appendChild(row);
                        });
                    }

                    const bfsRes = comp.bfs || (comp.results ? comp.results.find(r => r.algorithmName === 'BFS') : null);
                    const dijkstraRes = comp.dijkstra || (comp.results ? comp.results.find(r => r.algorithmName === 'Dijkstra') : null);
                    const astarRes = comp.astar || (comp.results ? comp.results.find(r => r.algorithmName === 'A*') : null);

                    updateAlgoCharts(bfsRes, dijkstraRes, astarRes);

                    let efficiencyText = '--';
                    if (astarRes && bfsRes) {
                        const bfsExp = bfsRes.nodesExplored !== undefined ? bfsRes.nodesExplored : bfsRes.nodesVisited;
                        const astarExp = astarRes.nodesExplored !== undefined ? astarRes.nodesExplored : astarRes.nodesVisited;
                        if (bfsExp > 0) {
                            const red = (1.0 - (astarExp / bfsExp)) * 100;
                            efficiencyText = `${red.toFixed(1)}% pruned`;
                        }
                    }

                    if (astarRes) {
                        updateAlgoMetricCards(astarRes, efficiencyText);
                    }

                    if (algoSummaryText && comp.summary) {
                        algoSummaryText.innerHTML = `<strong>Comparative Analysis:</strong> ${comp.summary}`;
                    }

                    // Animate A* on canvas
                    if (algoCanvasInstance && astarRes) {
                        algoCanvasInstance.loadAlgorithmResult(astarRes);
                        if (algoStatusBadge) {
                            algoStatusBadge.textContent = 'Exploring...';
                            algoStatusBadge.style.background = '#e0e7ff';
                            algoStatusBadge.style.color = '#4338ca';
                        }
                        algoCanvasInstance.startAnimation(
                            (curr, total) => {
                                if (algoMetricExplored) algoMetricExplored.textContent = curr;
                            },
                            () => {
                                if (algoStatusBadge) {
                                    algoStatusBadge.textContent = 'Complete';
                                    algoStatusBadge.style.background = '#dcfce7';
                                    algoStatusBadge.style.color = '#15803d';
                                }
                                if (btnAlgoPauseResume) btnAlgoPauseResume.textContent = '⏯ Pause';
                            }
                        );
                    }
                } else {
                    // Single Algorithm Run
                    const res = await api.runAlgorithmLab(selectedLabAlgo, start, goal, customObstacles);
                    lastLabSingleResult = res;

                    if (algoResultsTableBody && res) {
                        algoResultsTableBody.innerHTML = '';
                        const row = document.createElement('tr');
                        row.className = 'highlight';
                        const steps = res.path ? Math.max(0, res.path.length - 1) : 0;
                        row.innerHTML = `
                            <td style="font-weight:700;">${res.algorithm || res.algorithmName}</td>
                            <td style="font-family:var(--font-mono);">${res.path ? res.path.length : 0} cells</td>
                            <td style="font-family:var(--font-mono);">${steps} steps</td>
                            <td style="font-family:var(--font-mono);font-weight:700;">${res.nodesExplored !== undefined ? res.nodesExplored : res.nodesVisited}</td>
                            <td style="font-family:var(--font-mono);">${res.totalCost.toFixed(1)}</td>
                            <td style="font-family:var(--font-mono);">${res.executionTimeMicroseconds.toFixed(2)} µs</td>
                            <td>${res.found ? '<span style="color:#10b981;font-weight:700;">✓ Solved</span>' : '<span style="color:#ef4444;font-weight:700;">❌ ' + (res.statusMessage || 'Unreachable') + '</span>'}</td>
                        `;
                        algoResultsTableBody.appendChild(row);
                    }

                    updateAlgoMetricCards(res, selectedLabAlgo === 'A*' ? 'Guided Search' : 'Uniform Search');

                    if (algoSummaryText) {
                        algoSummaryText.innerHTML = `<strong>${res.algorithm || res.algorithmName} Execution:</strong> ` +
                            (res.found ? `Successfully solved path of ${res.path.length} cells (${Math.max(0, res.path.length - 1)} steps) with cost ${res.totalCost.toFixed(1)} in ${res.executionTimeMicroseconds.toFixed(2)} µs (${res.nodesExplored || res.nodesVisited} nodes explored).`
                                       : `No path found. Status: ${res.statusMessage || 'Unreachable'}.`);
                    }

                    // Animate single algorithm
                    if (algoCanvasInstance && res) {
                        algoCanvasInstance.loadAlgorithmResult(res);
                        if (algoStatusBadge) {
                            algoStatusBadge.textContent = 'Exploring...';
                            algoStatusBadge.style.background = '#e0e7ff';
                            algoStatusBadge.style.color = '#4338ca';
                        }
                        algoCanvasInstance.startAnimation(
                            (curr, total) => {
                                if (algoMetricExplored) algoMetricExplored.textContent = curr;
                            },
                            () => {
                                if (algoStatusBadge) {
                                    algoStatusBadge.textContent = 'Complete';
                                    algoStatusBadge.style.background = '#dcfce7';
                                    algoStatusBadge.style.color = '#15803d';
                                }
                                if (btnAlgoPauseResume) btnAlgoPauseResume.textContent = '⏯ Pause';
                            }
                        );
                    }
                }
            } catch (err) {
                console.error('[AlgorithmLab] Execution error:', err);
                if (algoStatusBadge) {
                    algoStatusBadge.textContent = 'Error';
                    algoStatusBadge.style.background = '#fee2e2';
                    algoStatusBadge.style.color = '#991b1b';
                }
                if (algoSummaryText) {
                    algoSummaryText.innerHTML = `<span style="color:var(--accent-rose);font-weight:600;">Execution Error:</span> ${err.message || err}`;
                }
            } finally {
                btnAlgoRun.disabled = false;
            }
        });
    }

    // Export Handlers
    function downloadBlob(content, filename, contentType) {
        const blob = new Blob([content], { type: contentType });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    if (btnAlgoExportJSON) {
        btnAlgoExportJSON.addEventListener('click', async () => {
            const sx = parseInt(algoStartX.value) || 0;
            const sy = parseInt(algoStartY.value) || 0;
            const gx = parseInt(algoGoalX.value) || 14;
            const gy = parseInt(algoGoalY.value) || 14;
            const start = { x: sx, y: sy };
            const goal = { x: gx, y: gy };
            const customObstacles = algoCanvasInstance ? algoCanvasInstance.getCustomObstaclesList() : [];

            try {
                const report = await api.getAlgorithmReport(start, goal, 'json', customObstacles);
                downloadBlob(JSON.stringify(report, null, 2), `algorithm_lab_report_${Date.now()}.json`, 'application/json');
            } catch (e) {
                alert(`Export failed: ${e.message}`);
            }
        });
    }

    if (btnAlgoExportCSV) {
        btnAlgoExportCSV.addEventListener('click', async () => {
            const sx = parseInt(algoStartX.value) || 0;
            const sy = parseInt(algoStartY.value) || 0;
            const gx = parseInt(algoGoalX.value) || 14;
            const gy = parseInt(algoGoalY.value) || 14;
            const start = { x: sx, y: sy };
            const goal = { x: gx, y: gy };
            const customObstacles = algoCanvasInstance ? algoCanvasInstance.getCustomObstaclesList() : [];

            try {
                const csvData = await api.getAlgorithmReport(start, goal, 'csv', customObstacles);
                downloadBlob(csvData, `algorithm_lab_report_${Date.now()}.csv`, 'text/csv');
            } catch (e) {
                alert(`Export failed: ${e.message}`);
            }
        });
    }

    // =========================================================================
    // Core UI Update Function
    // =========================================================================
    function detectAndLogSimulationEvents(newState) {
        if (!newState) return;
        if (!prevKnownState) {
            prevKnownState = JSON.parse(JSON.stringify(newState));
            return;
        }

        const prevTick = prevKnownState.tick || prevKnownState.simulationTick || 0;
        const curTick = newState.tick || newState.simulationTick || 0;
        if (curTick === prevTick) return;

        // 1. Detect order status transitions
        const prevOrdersMap = new Map();
        const allPrevOrders = [...(prevKnownState.orders?.pendingOrders || []), ...(prevKnownState.orders?.activeOrders || []), ...(prevKnownState.orders?.completedOrders || [])];
        const allCurOrders = [...(newState.orders?.pendingOrders || []), ...(newState.orders?.activeOrders || []), ...(newState.orders?.completedOrders || [])];

        allPrevOrders.forEach(o => prevOrdersMap.set(o.id, o));

        allCurOrders.forEach(o => {
            const prev = prevOrdersMap.get(o.id);
            if (!prev) {
                logActivityEvent(`Order #${o.id} created (Product #${o.productId}, Qty: ${o.quantity}, Priority: ${o.priority})`, '📝');
            } else if (prev.status !== o.status) {
                if (o.status === 'ASSIGNED') {
                    const robText = o.assignedRobotId || (o.teamRobotIds && o.teamRobotIds.length > 1 ? o.teamRobotIds.join('+') : 'Robot');
                    logActivityEvent(`Order #${o.id} assigned to ${robText} (Parallel Execution)`, '🤖');
                } else if (o.status === 'PICKING') {
                    logActivityEvent(`Order #${o.id} picking in progress by ${o.assignedRobotId || 'robot'} at Shelf`, '📦');
                } else if (o.status === 'DELIVERING') {
                    logActivityEvent(`Order #${o.id} picked. In transit to Packing Station`, '🚚');
                } else if (o.status === 'COMPLETED') {
                    logActivityEvent(`Order #${o.id} delivered to Packing Station. COMPLETED! 🎉`, '✅');
                }
            }
        });

        // 2. Multi-Robot coordination stats
        if (newState.coordinator && prevKnownState.coordinator) {
            const dCol = (newState.coordinator.collisionsPrevented || 0) - (prevKnownState.coordinator.collisionsPrevented || 0);
            const dRep = (newState.coordinator.routesReplanned || 0) - (prevKnownState.coordinator.routesReplanned || 0);
            if (dCol > 0) {
                logActivityEvent(`${dCol} collision(s) prevented by MultiRobotCoordinator`, '🛡️');
            }
            if (dRep > 0) {
                logActivityEvent(`Space-Time A* dynamic replan computed (${dRep} routes)`, '🔄');
            }
        }

        prevKnownState = JSON.parse(JSON.stringify(newState));
    }

    function updateUI(s) {
        if (!s) return;
        state = s;
        detectAndLogSimulationEvents(s);

        // 1. Update Canvas & Tick Display
        canvas.updateState(s);
        if (algoCanvasInstance && s.warehouse) algoCanvasInstance.setWarehouseBase(s.warehouse);
        if (tickDisplay) tickDisplay.textContent = `TICK: ${s.tick || 0}`;

        // 2. Update Main KPIs
        const activeCount = s.orders?.activeOrders?.length || 0;
        const pendingCount = s.orders?.pendingOrders?.length || 0;
        const completedCount = s.orders?.completedOrders?.length || 0;
        if (kpiActiveOrders) kpiActiveOrders.textContent = activeCount + pendingCount;
        if (kpiCompletedOrders) kpiCompletedOrders.textContent = completedCount;

        const idleRobots = s.robots?.filter(r => r.status === 'IDLE').length || 0;
        if (kpiIdleRobots) kpiIdleRobots.textContent = `${idleRobots} / ${s.robots?.length || 4}`;

        const totalBatt = s.robots?.reduce((acc, r) => acc + r.battery, 0) || 0;
        const avgBatt = s.robots?.length ? Math.round(totalBatt / s.robots.length) : 100;
        if (kpiAvgBattery) kpiAvgBattery.textContent = `${avgBatt}%`;

        // 3. Update Multi-Robot Coordination Status Metrics
        if (s.coordinator) {
            const elCol = document.getElementById('coordCollisionsPrevented');
            const elDlk = document.getElementById('coordDeadlocks');
            const elRep = document.getElementById('coordReplanned');
            if (elCol) elCol.textContent = s.coordinator.collisionsPrevented || 0;
            if (elDlk) elDlk.textContent = s.coordinator.deadlocksResolved || 0;
            if (elRep) elRep.textContent = s.coordinator.routesReplanned || 0;
        }

        if (s.robots) {
            const activeFleet = s.robots.filter(r => r.status !== 'IDLE').length;
            const waitingRobots = s.robots.filter(r => r.status === 'WAITING').length;
            const blockedRobots = s.robots.filter(r => r.status === 'BLOCKED').length;

            const elAct = document.getElementById('coordActiveRobots');
            const elWait = document.getElementById('coordWaitingRobots');
            const elBlk = document.getElementById('coordBlockedRobots');
            if (elAct) elAct.textContent = activeFleet;
            if (elWait) elWait.textContent = waitingRobots;
            if (elBlk) elBlk.textContent = blockedRobots;
        }

        // 4. Update Inspector coordinate description if a cell is selected
        if (selectedCell) {
            updateCellInspector(selectedCell.x, selectedCell.y);
        }
        updateHeatmapControls(s);
        renderHeatmapObservationDetails(s);

        // 5. Update Legacy & Overview Panels
        renderOverviewOrders(s.orders);
        renderOverviewFleet(s.robots);
        renderOverviewInventory(s.inventory);
        renderOverviewAnalytics(s);

        // 6. Update Active View Panels
        if (currentView === 'fleet') renderFleetManagement(s);
        else if (currentView === 'orders') renderOrdersView(s);
        else if (currentView === 'inventory') renderInventoryView(s);
    }

    // Populate all product dropdowns across views
    function populateAllProductDropdowns(inventory) {
        if (!inventory) return;
        const dropdowns = [productSelect, warehouseProductSelect, ordersViewProductSelect];

        dropdowns.forEach(dd => {
            if (!dd) return;
            const prevVal = dd.value;
            dd.innerHTML = '';
            inventory.forEach(p => {
                const opt = document.createElement('option');
                opt.value = p.id;
                opt.textContent = `${p.name} ($${p.price.toFixed(2)}) — ${p.quantity} in stock`;
                dd.appendChild(opt);
            });
            if (prevVal) dd.value = prevVal;
        });

        // Trigger stock text update
        if (warehouseProductSelect && warehouseStockHelp) {
            const prod = inventory.find(p => p.id === parseInt(warehouseProductSelect.value));
            if (prod) {
                warehouseStockHelp.textContent = `Available Stock: ${prod.quantity} units (Shelf at ${prod.location.x}, ${prod.location.y})`;
            }
        }
    }

    // Overview Legacy Rendering Helpers
    function renderOverviewOrders(orders) {
        if (!orders || !activeOrdersContainer) return;
        activeOrdersContainer.innerHTML = '';
        const all = [
            ...(orders.pendingOrders || []),
            ...(orders.activeOrders || []),
            ...(orders.completedOrders || []).slice(-3)
        ];
        if (all.length === 0) {
            activeOrdersContainer.innerHTML = '<div style="color:var(--text-muted);font-size:13px;padding:12px;text-align:center;">No active orders in warehouse queue.</div>';
            return;
        }
        all.forEach(o => {
            const prod = state?.inventory?.find(p => p.id === o.productId);
            const card = document.createElement('div');
            card.className = 'order-item-card';
            const progClass = o.status;
            const robotLabel = o.assignedRobotId || (o.teamRobotIds && o.teamRobotIds.length > 1 ? 'Team (' + o.teamRobotIds.join(', ') + ')' : 'Pending Scheduling');
            card.innerHTML = `
                <div style="display:flex;justify-content:space-between;align-items:center;">
                    <span style="font-family:var(--font-mono);font-size:13px;font-weight:700;color:var(--text-primary);">Order #${o.id}</span>
                    <span class="badge ${o.status}">${o.status}</span>
                </div>
                <div style="font-size:14px;font-weight:600;color:var(--text-secondary);">${prod ? prod.name : 'Product #' + o.productId} &times; ${o.quantity}</div>
                <div style="display:flex;justify-content:space-between;font-size:12px;color:var(--text-muted);font-family:var(--font-mono);">
                    <span>Robot: <strong>${robotLabel}</strong></span>
                    <span>Priority: <strong>${o.priority}</strong></span>
                </div>
                <div class="order-progress-track">
                    <div class="order-progress-bar ${progClass}"></div>
                </div>
            `;
            activeOrdersContainer.appendChild(card);
        });
    }

    function renderOverviewFleet(robots) {
        if (!robots || !fleetContainer) return;
        fleetContainer.innerHTML = '';
        robots.forEach(r => {
            const card = document.createElement('div');
            card.className = 'robot-item-card';
            const isLow = r.battery < 20;
            const isWarn = r.battery >= 20 && r.battery < 50;
            const fillClass = isLow ? 'danger' : (isWarn ? 'warn' : '');
            let taskText = 'IDLE';
            if (r.currentTask && r.currentTask.orderId) {
                taskText = `Order #${r.currentTask.orderId} (${r.currentTask.type || r.status})`;
            } else if (r.status === 'CHARGING' || r.status === 'RETURNING_TO_CHARGE') {
                taskText = 'Charging';
            } else if (r.status === 'WAITING') {
                taskText = 'Waiting (Collision Safety)';
            }

            card.innerHTML = `
                <div class="robot-item-header">
                    <div>
                        <strong style="font-size:14px;color:var(--text-primary);">${r.name}</strong>
                        <span style="font-family:var(--font-mono);font-size:12px;color:var(--text-muted);margin-left:6px;">(${r.id})</span>
                    </div>
                    <span class="badge ${r.status}">${r.status}</span>
                </div>
                <div style="display:flex;justify-content:space-between;font-size:12px;color:var(--text-secondary);">
                    <span>Type: ${r.type}</span>
                    <span>Pos: (${r.position.x}, ${r.position.y})</span>
                    <span>Load: ${r.currentPayload || 0}/${r.capacity}</span>
                </div>
                <div style="font-size:12px;color:var(--text-muted);font-family:var(--font-mono);">
                    <span>Task: <strong>${taskText}</strong></span>
                </div>
                <div class="progress-track">
                    <div class="progress-fill ${fillClass}" style="width:${r.battery}%;"></div>
                </div>
                <div style="display:flex;justify-content:space-between;font-size:11px;font-family:var(--font-mono);color:var(--text-muted);">
                    <span>Battery</span>
                    <span style="font-weight:600;color:${isLow ? '#e11d48' : '#0f172a'};">${Math.round(r.battery)}%</span>
                </div>
            `;
            fleetContainer.appendChild(card);
        });
    }

    function renderOverviewInventory(inventory) {
        if (!inventory || !inventoryContainer) return;
        inventoryContainer.innerHTML = '';
        inventory.forEach(p => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td style="font-family:var(--font-mono);font-weight:600;color:var(--accent-blue);">${p.id}</td>
                <td><strong>${p.name}</strong><br><span style="color:var(--text-muted);font-size:11px;">${p.category}</span></td>
                <td style="font-family:var(--font-mono);">(${p.location.x}, ${p.location.y})</td>
                <td style="font-family:var(--font-mono);font-weight:600;">${p.quantity}</td>
                <td style="font-family:var(--font-mono);">$${p.price.toFixed(2)}</td>
            `;
            inventoryContainer.appendChild(row);
        });
    }

    // =========================================================================
    // Stage D: Intelligent Operational Analytics & Simulation Intelligence
    // =========================================================================
    function escapeHTML(str) {
        if (str == null) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    function renderOverviewAnalytics(s) {
        if (!overviewAnalyticsSection) return;
        const a = s?.analytics;
        if (!a) return;

        // KPI 1: Observed Simulation Duration
        if (analyticsObservedTicks) {
            analyticsObservedTicks.textContent = `${a.observedTicks || 0} ticks`;
        }
        if (analyticsTickSubtitle) {
            analyticsTickSubtitle.textContent = `Observed duration • Simulation tick T${s.tick || 0}`;
        }

        // KPI 2: Fleet Working Utilization
        const fleetUtil = a.fleetAverageUtilization != null ? a.fleetAverageUtilization : 0;
        if (analyticsFleetUtil) {
            analyticsFleetUtil.textContent = `${fleetUtil.toFixed(1)}%`;
        }
        if (analyticsFleetUtilSub) {
            analyticsFleetUtilSub.textContent = `Active fleet working time / observed ticks`;
        }

        // KPI 3: Average Completion Duration
        const ot = a.orderThroughput || {};
        if (analyticsAvgDuration) {
            if (ot.averageDurationTicks != null && ot.averageDurationTicks >= 0) {
                analyticsAvgDuration.textContent = `${ot.averageDurationTicks.toFixed(1)} ticks`;
            } else {
                analyticsAvgDuration.textContent = '--';
            }
        }
        if (analyticsDurationRange) {
            if (ot.averageDurationTicks != null && ot.averageDurationTicks >= 0) {
                analyticsDurationRange.textContent = `Min: ${ot.minDurationTicks}t • Max: ${ot.maxDurationTicks}t duration`;
            } else {
                analyticsDurationRange.textContent = 'No completed orders yet';
            }
        }

        // KPI 4: Order Throughput
        const completed = ot.ordersCompleted || 0;
        const created = ot.ordersCreated || 0;
        const rate = ot.completionRate != null ? ot.completionRate : 0;
        const items = ot.totalProductsFulfilled || 0;
        if (analyticsThroughput) {
            analyticsThroughput.textContent = `${completed} / ${created}`;
        }
        if (analyticsCompletionRate) {
            analyticsCompletionRate.textContent = `${rate.toFixed(1)}% completion (${items} items delivered)`;
        }

        // KPI 5: Fleet Battery Consumed
        const battConsumed = a.fleetTotalBatteryConsumed != null ? a.fleetTotalBatteryConsumed : 0;
        if (analyticsTotalBattConsumed) {
            analyticsTotalBattConsumed.textContent = `${battConsumed.toFixed(1)}%`;
        }
        if (analyticsLowestRobot) {
            const lowBatt = a.fleetLowestBattery != null ? Math.round(a.fleetLowestBattery) : 100;
            const lowId = a.fleetLowestBatteryRobotId || 'R01';
            const charges = a.fleetTotalChargingEvents || 0;
            analyticsLowestRobot.textContent = `Lowest: ${lowBatt}% (${lowId}) • ${charges} charge event${charges === 1 ? '' : 's'}`;
        }

        // KPI 6: Coordination Safety Interventions
        const colPrev = s.coordinator?.collisionsPrevented || 0;
        const dlkRes = s.coordinator?.deadlocksResolved || 0;
        const routesRep = s.coordinator?.routesReplanned || 0;
        const totalEvents = colPrev + dlkRes + routesRep;
        if (analyticsCoordSummary) {
            analyticsCoordSummary.textContent = `${totalEvents} events`;
        }
        if (analyticsCoordSub) {
            analyticsCoordSub.textContent = `${colPrev} col • ${dlkRes} dlk • ${routesRep} replans`;
        }

        // Left Panel: Per-Robot Operational State Breakdown & Stacked Bars
        if (utilizationRowsContainer) {
            utilizationRowsContainer.innerHTML = '';
            const robotsUtil = a.robotUtilization || [];
            if (robotsUtil.length === 0) {
                utilizationRowsContainer.innerHTML = '<div style="color:var(--text-muted);font-size:13px;text-align:center;padding:16px;">No robot utilization data recorded yet.</div>';
            } else {
                robotsUtil.forEach(ru => {
                    const rob = s.robots?.find(r => r.id === ru.robotId);
                    const robName = rob ? rob.name : `Robot ${ru.robotId}`;
                    const robType = rob ? rob.type : 'Robot';
                    const workingPct = Math.max(0, Math.min(100, ru.workingUtilization || 0));
                    const waitingPct = Math.max(0, Math.min(100, ru.waitingPercentage || 0));
                    const chargingPct = Math.max(0, Math.min(100, ru.chargingPercentage || 0));
                    const idlePct = Math.max(0, Math.min(100, ru.idlePercentage != null ? ru.idlePercentage : 100));

                    const row = document.createElement('div');
                    row.className = 'util-robot-card';
                    row.innerHTML = `
                        <div class="util-card-header">
                            <div class="util-robot-meta">
                                <span class="robot-id-tag">${escapeHTML(ru.robotId)}</span>
                                <div>
                                    <span class="robot-name-text">${escapeHTML(robName)}</span>
                                    <span class="robot-type-sub">(${escapeHTML(robType)})</span>
                                </div>
                            </div>
                            <div class="util-stats-inline">
                                <span>Steps: <strong>${ru.movementSteps || 0}</strong></span>
                                <span>Consumed: <strong>${(ru.batteryConsumed || 0).toFixed(1)}%</strong></span>
                                <span>Tasks: <strong>${ru.tasksCompleted || 0}</strong></span>
                                <span class="badge ${rob ? rob.status : 'IDLE'}">${rob ? rob.status : 'IDLE'}</span>
                            </div>
                        </div>
                        <div class="stacked-bar-container" title="Working: ${workingPct.toFixed(1)}% | Waiting: ${waitingPct.toFixed(1)}% | Charging: ${chargingPct.toFixed(1)}% | Idle: ${idlePct.toFixed(1)}%">
                            <div class="stacked-segment working" style="width: ${workingPct}%;"></div>
                            <div class="stacked-segment waiting" style="width: ${waitingPct}%;"></div>
                            <div class="stacked-segment charging" style="width: ${chargingPct}%;"></div>
                            <div class="stacked-segment idle" style="width: ${idlePct}%;"></div>
                        </div>
                        <div class="util-legend-metrics">
                            <span>Working: <strong>${ru.workingTicks || 0}t</strong> (${workingPct.toFixed(1)}%)</span>
                            <span>Waiting: <strong>${ru.waitingTicks || 0}t</strong> (${waitingPct.toFixed(1)}%)</span>
                            <span>Charging: <strong>${ru.chargingTicks || 0}t</strong> (${chargingPct.toFixed(1)}%)</span>
                            <span>Idle: <strong>${ru.idleTicks || 0}t</strong> (${idlePct.toFixed(1)}%)</span>
                        </div>
                    `;
                    utilizationRowsContainer.appendChild(row);
                });
            }
        }

        // Right Panel: Operational Recommendations & Anomaly Detection
        if (recommendationsContainer) {
            recommendationsContainer.innerHTML = '';
            const recs = a.recommendations || [];
            if (recommendationsCountBadge) {
                recommendationsCountBadge.textContent = `${recs.length} ALERT${recs.length === 1 ? '' : 'S'}`;
                if (recs.some(r => r.severity === 'CRITICAL')) {
                    recommendationsCountBadge.style.background = '#ffe4e6';
                    recommendationsCountBadge.style.color = '#e11d48';
                } else if (recs.some(r => r.severity === 'WARNING')) {
                    recommendationsCountBadge.style.background = '#fef3c7';
                    recommendationsCountBadge.style.color = '#d97706';
                } else {
                    recommendationsCountBadge.style.background = 'var(--accent-blue-soft)';
                    recommendationsCountBadge.style.color = 'var(--accent-blue)';
                }
            }

            if (recs.length === 0) {
                recommendationsContainer.innerHTML = `
                    <div style="background:#f0fdf4;border:1px solid #bbf7d0;border-radius:var(--radius-sm);padding:14px;display:flex;align-items:center;gap:10px;">
                        <span style="font-size:20px;">✅</span>
                        <div>
                            <strong style="color:#15803d;font-size:13px;">Optimal Operations: Normal Parameters</strong>
                            <div style="color:#166534;font-size:12px;margin-top:2px;">Simulation is operating smoothly without congestion bottlenecks, battery depletion, or order starvation.</div>
                        </div>
                    </div>
                `;
            } else {
                recs.forEach(rec => {
                    const item = document.createElement('div');
                    item.className = `rec-item ${rec.severity}`;
                    const icon = rec.severity === 'CRITICAL' ? '🚨' : (rec.severity === 'WARNING' ? '⚠️' : 'ℹ️');
                    item.innerHTML = `
                        <div class="rec-item-header">
                            <span class="rec-title">${icon} ${escapeHTML(rec.title)}</span>
                            <span class="rec-severity-badge ${rec.severity}">${escapeHTML(rec.severity)}</span>
                        </div>
                        <div class="rec-desc">${escapeHTML(rec.description)}</div>
                        <div class="rec-trigger">Metric Trigger: <span>${escapeHTML(rec.triggerReason)}</span></div>
                    `;
                    recommendationsContainer.appendChild(item);
                });
            }
        }

        // Units Fulfilled by Category
        if (categoryFulfillmentList) {
            categoryFulfillmentList.innerHTML = '';
            const cf = ot.categoryFulfillment || {};
            const keys = Object.keys(cf);
            if (keys.length === 0) {
                categoryFulfillmentList.innerHTML = '<span style="font-size:12px;color:var(--text-muted);">No products delivered yet.</span>';
            } else {
                keys.forEach(k => {
                    const tag = document.createElement('span');
                    tag.style.cssText = 'background:var(--bg-card-subtle);border:1px solid var(--border-subtle);padding:3px 10px;border-radius:var(--radius-full);font-size:12px;color:var(--text-primary);font-family:var(--font-mono);';
                    tag.innerHTML = `<strong>${escapeHTML(k)}:</strong> ${cf[k]} units`;
                    categoryFulfillmentList.appendChild(tag);
                });
            }
        }
    }

    // =========================================================================
    // Report Export & Canonical Benchmark Action Listeners
    // =========================================================================
    if (btnDownloadCSVReport) {
        btnDownloadCSVReport.addEventListener('click', async () => {
            btnDownloadCSVReport.disabled = true;
            btnDownloadCSVReport.textContent = '⏳ Exporting CSV...';
            try {
                const csvData = await api.getReportCSV();
                const blob = new Blob([csvData], { type: 'text/csv;charset=utf-8;' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `warehouse_simulation_report_tick_${state?.tick || 0}.csv`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
            } catch (err) {
                alert('Failed to generate CSV report: ' + err.message);
            } finally {
                btnDownloadCSVReport.disabled = false;
                btnDownloadCSVReport.textContent = '📥 Download CSV Report';
            }
        });
    }

    if (btnViewHTMLReport) {
        btnViewHTMLReport.addEventListener('click', async () => {
            btnViewHTMLReport.disabled = true;
            btnViewHTMLReport.textContent = '⏳ Opening Report...';
            try {
                const htmlData = await api.getReportHTML();
                const blob = new Blob([htmlData], { type: 'text/html;charset=utf-8;' });
                const url = URL.createObjectURL(blob);
                window.open(url, '_blank');
            } catch (err) {
                alert('Failed to open HTML report: ' + err.message);
            } finally {
                btnViewHTMLReport.disabled = false;
                btnViewHTMLReport.textContent = '📄 Open HTML Report';
            }
        });
    }

    // =========================================================================
    // Stage G: Configurable Algorithm Benchmarking Suite Engine
    // =========================================================================
    async function runConfiguredBenchmark(triggerBtn) {
        const scenario = benchmarkScenarioSelect?.value || 'all';
        const repetitions = parseInt(benchmarkRepetitionsSelect?.value) || 5;
        const algos = [];
        if (chkAlgoBFS?.checked) algos.push('BFS');
        if (chkAlgoDijkstra?.checked) algos.push('Dijkstra');
        if (chkAlgoAStar?.checked) algos.push('A*');
        if (chkAlgoSTAStar?.checked) algos.push('Space-Time A*');

        if (algos.length === 0) {
            alert('Please select at least one algorithm to benchmark.');
            return;
        }

        if (triggerBtn) {
            triggerBtn.disabled = true;
            triggerBtn.textContent = '⚡ Running...';
        }
        if (btnExecuteBenchmark) {
            btnExecuteBenchmark.disabled = true;
            btnExecuteBenchmark.textContent = '⚡ Running...';
        }

        if (benchmarkSuiteBody) {
            benchmarkSuiteBody.innerHTML = `
                <div style="padding:36px 20px;text-align:center;color:var(--text-muted);">
                    <div style="font-size:26px;margin-bottom:10px;">⚡</div>
                    <strong style="color:var(--text-primary);font-size:14px;">Executing Controlled Benchmark Runs...</strong>
                    <p style="font-size:12px;margin-top:6px;">Evaluating ${repetitions} repetition(s) across [${algos.join(', ')}] on scenario: <code>${escapeHTML(scenario)}</code></p>
                    <span style="font-size:11px;color:var(--text-dim);display:block;margin-top:4px;">Native C++ steady_clock timing &bull; Isolated topology &bull; Zero live simulation mutation</span>
                </div>
            `;
        }
        if (benchmarkSuiteModal) {
            benchmarkSuiteModal.classList.add('active');
        }

        try {
            const suite = await api.runBenchmark(scenario, repetitions, algos);
            renderBenchmarkSuiteModal(suite);
        } catch (err) {
            if (benchmarkSuiteBody) {
                benchmarkSuiteBody.innerHTML = `
                    <div style="padding:16px;background:var(--accent-rose-soft);border:1px solid var(--accent-rose-border);border-radius:var(--radius-sm);color:var(--accent-rose);font-size:13px;margin:16px 0;">
                        <strong>Benchmark Execution Error:</strong> ${escapeHTML(err.message || 'Unknown server error')}
                    </div>
                `;
            }
        } finally {
            if (triggerBtn) {
                triggerBtn.disabled = false;
                if (triggerBtn.id === 'btnRunBenchmarkSuite') {
                    triggerBtn.textContent = '⚡ Run Benchmark Suite';
                } else if (triggerBtn.id === 'btnOpenBenchmarkSuiteModal') {
                    triggerBtn.textContent = '⚡ Controlled Benchmark Suite (5 Scenarios)';
                } else {
                    triggerBtn.textContent = '⚡ Run Benchmark';
                }
            }
            if (btnExecuteBenchmark) {
                btnExecuteBenchmark.disabled = false;
                btnExecuteBenchmark.textContent = '⚡ Run Benchmark';
            }
        }
    }

    if (btnRunBenchmarkSuite) {
        btnRunBenchmarkSuite.addEventListener('click', () => runConfiguredBenchmark(btnRunBenchmarkSuite));
    }
    if (btnOpenBenchmarkSuiteModal) {
        btnOpenBenchmarkSuiteModal.addEventListener('click', () => runConfiguredBenchmark(btnOpenBenchmarkSuiteModal));
    }
    if (btnExecuteBenchmark) {
        btnExecuteBenchmark.addEventListener('click', () => runConfiguredBenchmark(btnExecuteBenchmark));
    }

    function renderBenchmarkSuiteModal(suite) {
        if (!benchmarkSuiteModal || !benchmarkSuiteBody) return;
        const scenarios = suite.scenarios || [];

        if (scenarios.length === 0) {
            benchmarkSuiteBody.innerHTML = `
                <div style="padding:24px;text-align:center;color:var(--text-muted);font-size:13px;">
                    No benchmark scenarios returned from the C++ engine.
                </div>
            `;
            benchmarkSuiteModal.classList.add('active');
            return;
        }

        const repCount = suite.repetitions || 1;
        benchmarkSuiteBody.innerHTML = `
            <div style="margin-bottom:14px;color:var(--text-secondary);font-size:13px;line-height:1.5;">
                Controlled algorithm benchmark executed across <strong>${scenarios.length} standardized warehouse scenario(s)</strong> with <strong>${repCount} repetition(s)</strong> per algorithm. High-resolution durations measured via <code>std::chrono::steady_clock</code>; warehouse topology setup and reservation state resets are isolated from measurement duration.
            </div>
        `;

        scenarios.forEach(sc => {
            const scCard = document.createElement('div');
            scCard.className = 'benchmark-scenario-card';
            const scenarioTitle = sc.name || sc.scenarioName || 'Benchmark Scenario';
            const obstCount = sc.obstacleCount != null ? sc.obstacleCount : ((state?.warehouse?.shelves?.length || 0) + (state?.warehouse?.staticObstacles?.length || 0));

            // Find maximum nodes visited in this scenario for bar visualization
            let maxNodes = 1;
            (sc.results || []).forEach(r => {
                if (r.nodesVisited && r.nodesVisited > maxNodes) maxNodes = r.nodesVisited;
            });

            const rowsHTML = (sc.results || []).map(r => {
                const algoName = r.algorithm || r.algorithmName || 'Algorithm';
                const pathLen = r.pathLength != null ? r.pathLength : (r.path ? r.path.length : 0);
                const isAStar = algoName === 'A*';
                const isSTAStar = algoName === 'Space-Time A*';

                // Bar styling
                const barClass = isAStar ? 'astar' : (algoName === 'Dijkstra' ? 'dijkstra' : (algoName === 'BFS' ? 'bfs' : 'spacetime'));
                const barPct = Math.max(6, Math.min(100, Math.round(((r.nodesVisited || 1) / maxNodes) * 100)));

                // Optimality badge
                let optBadge = '';
                if (r.isOptimal) {
                    if (isAStar) {
                        optBadge = '<span class="badge-heuristic-optimal">✓ A* Optimal</span>';
                    } else if (algoName === 'BFS' || algoName === 'Dijkstra') {
                        optBadge = '<span class="badge-optimal-guaranteed">✓ Optimal Cost</span>';
                    } else {
                        optBadge = '<span class="badge-spacetime-coord">✓ Coordinated</span>';
                    }
                } else {
                    if (isSTAStar) {
                        optBadge = '<span class="badge-spacetime-coord">✓ Space-Time</span>';
                    } else if (sc.scenario === 'congested_multi_robot') {
                        optBadge = '<span class="badge-spatial-limited" title="2D spatial pathfinder does not track dynamic reservation collisions">2D Static Only</span>';
                    } else {
                        optBadge = '<span class="badge-spatial-limited">Heuristic</span>';
                    }
                }

                // Execution time representation
                let timeHTML = '';
                if (r.executionTimeMinUs != null && r.executionTimeMaxUs != null && repCount > 1) {
                    timeHTML = `
                        <div style="font-family:var(--font-mono);font-weight:700;">${r.executionTimeMicroseconds.toFixed(2)} µs</div>
                        <div style="font-size:10px;color:var(--text-muted);font-family:var(--font-mono);">min: ${r.executionTimeMinUs.toFixed(1)} &bull; max: ${r.executionTimeMaxUs.toFixed(1)}</div>
                    `;
                } else {
                    timeHTML = `<span style="font-family:var(--font-mono);font-weight:700;">${r.executionTimeMicroseconds.toFixed(2)} µs</span>`;
                }

                // Success rate & status
                const successPct = r.successRate != null ? Math.round(r.successRate * 100) : (r.found ? 100 : 0);
                const statusHTML = r.found
                    ? `<span style="color:#16a34a;font-weight:600;">✓ Solved</span> <span style="font-size:11px;color:var(--text-muted);">(${successPct}%)</span>`
                    : `<span style="color:#e11d48;font-weight:600;">Unreachable</span>`;

                return `
                <tr class="${isAStar ? 'highlight' : ''}">
                    <td>
                        <strong style="color:var(--text-primary);">${escapeHTML(algoName)}</strong>
                        <div style="margin-top:2px;">${optBadge}</div>
                    </td>
                    <td style="font-family:var(--font-mono);">${pathLen} cells</td>
                    <td>
                        <div class="benchmark-node-bar-container">
                            <span style="font-family:var(--font-mono);font-weight:700;min-width:32px;color:${isAStar ? 'var(--accent-blue)' : 'inherit'};">${r.nodesVisited}</span>
                            <div class="benchmark-node-bar-track">
                                <div class="benchmark-node-bar-fill ${barClass}" style="width:${barPct}%;"></div>
                            </div>
                        </div>
                    </td>
                    <td style="font-family:var(--font-mono);font-weight:600;">${r.totalCost.toFixed(1)}</td>
                    <td>${timeHTML}</td>
                    <td>${statusHTML}</td>
                </tr>
                `;
            }).join('');

            // Multi-robot metrics card if applicable
            let multiRobotHTML = '';
            const stAlgo = (sc.results || []).find(r => r.algorithm && (r.algorithm.includes('SpaceTime') || r.algorithm.includes('Space-Time')));
            const mm = sc.multiRobotMetrics || (sc.isMultiRobot && stAlgo ? {
                robotsCompleted: stAlgo.robotsCompleted,
                robotsContending: stAlgo.robotsContending,
                makespanTicks: stAlgo.makespanTicks,
                waitingTicks: stAlgo.waitingTicks,
                replanningEvents: stAlgo.replanningEvents
            } : null);

            if (mm) {
                multiRobotHTML = `
                <div class="benchmark-multirobot-card">
                    <div style="display:flex;justify-content:space-between;align-items:center;">
                        <strong style="font-size:12px;color:#86198f;">🤖 Space-Time Fleet Coordination Telemetry (ReservationTable Engine)</strong>
                        <span style="font-size:11px;font-family:var(--font-mono);font-weight:700;color:#701a75;">Cooperative Planning</span>
                    </div>
                    <div class="benchmark-multirobot-grid">
                        <div class="benchmark-multirobot-kpi">
                            <span class="benchmark-multirobot-kpi-label">Robots Completed</span>
                            <span class="benchmark-multirobot-kpi-val">${mm.robotsCompleted} / ${mm.robotsContending}</span>
                        </div>
                        <div class="benchmark-multirobot-kpi">
                            <span class="benchmark-multirobot-kpi-label">Makespan</span>
                            <span class="benchmark-multirobot-kpi-val">${mm.makespanTicks} ticks</span>
                        </div>
                        <div class="benchmark-multirobot-kpi">
                            <span class="benchmark-multirobot-kpi-label">Total Wait Ticks</span>
                            <span class="benchmark-multirobot-kpi-val">${mm.waitingTicks} ticks</span>
                        </div>
                        <div class="benchmark-multirobot-kpi">
                            <span class="benchmark-multirobot-kpi-label">Replanning Events</span>
                            <span class="benchmark-multirobot-kpi-val">${mm.replanningEvents} events</span>
                        </div>
                    </div>
                </div>
                `;
            }

            scCard.innerHTML = `
                <div class="benchmark-scenario-header">
                    <div>
                        <div class="benchmark-scenario-title">
                            <span>🗺️</span> ${escapeHTML(scenarioTitle)}
                            <span class="benchmark-scenario-coords">(${sc.start.x},${sc.start.y}) &rarr; (${sc.goal.x},${sc.goal.y})</span>
                        </div>
                        <div class="benchmark-scenario-desc">${escapeHTML(sc.description || '')}</div>
                    </div>
                    <span style="font-size:11px;padding:3px 10px;border-radius:var(--radius-full);background:var(--accent-blue-soft);color:var(--accent-blue);font-weight:700;white-space:nowrap;">
                        ${obstCount} Static Obstacles
                    </span>
                </div>
                <div class="data-table-container">
                    <table class="data-table" style="font-size:12px;">
                        <thead>
                            <tr>
                                <th>Algorithm &amp; Optimality</th>
                                <th>Path Length</th>
                                <th>Search Nodes (Visited)</th>
                                <th>Total Cost</th>
                                <th>Execution Time (Steady Clock)</th>
                                <th>Status / Rate</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${rowsHTML}
                        </tbody>
                    </table>
                </div>
                ${multiRobotHTML}
            `;
            benchmarkSuiteBody.appendChild(scCard);
        });

        // Add timing and isolation audit notice
        const timingNotice = document.createElement('div');
        timingNotice.className = 'benchmark-timing-notice';
        timingNotice.innerHTML = `
            <span>⏱️</span>
            <span>All pathfinding timings measured via <code>std::chrono::steady_clock</code>. Setup, warehouse state cloning, and reservation cleanups are strictly excluded from search timing. Live warehouse simulation state, robot positions, inventory, and order queues remain untouched.</span>
        `;
        benchmarkSuiteBody.appendChild(timingNotice);

        benchmarkSuiteModal.classList.add('active');
    }

    if (btnBenchmarkModalClose) {
        btnBenchmarkModalClose.addEventListener('click', () => {
            benchmarkSuiteModal.classList.remove('active');
        });
    }

    if (benchmarkSuiteModal) {
        benchmarkSuiteModal.addEventListener('click', (e) => {
            if (e.target === benchmarkSuiteModal) {
                benchmarkSuiteModal.classList.remove('active');
            }
        });
    }

    if (btnSaveSnapshot) {
        btnSaveSnapshot.addEventListener('click', async () => {
            if (btnSaveSnapshot.disabled) return;
            const path = prompt('Enter filepath to save warehouse runtime snapshot:', 'data/snapshot_runtime.json');
            if (!path || !path.trim()) return;

            btnSaveSnapshot.disabled = true;
            btnSaveSnapshot.textContent = '💾 Saving...';
            try {
                const res = await api.saveSnapshot(path.trim());
                if (res && res.success) {
                    showFeedback(analyticsFeedbackBanner, `✓ Snapshot saved successfully to ${path.trim()}`, 'success');
                } else {
                    showFeedback(analyticsFeedbackBanner, `Snapshot save failed: ${(res && res.error) || 'Unknown error'}`, 'error');
                }
            } catch (err) {
                showFeedback(analyticsFeedbackBanner, `Snapshot save error: ${err.message || 'Network failure'}`, 'error');
            } finally {
                btnSaveSnapshot.disabled = false;
                btnSaveSnapshot.textContent = '💾 Save Snapshot';
            }
        });
    }

    if (btnLoadSnapshot) {
        btnLoadSnapshot.addEventListener('click', async () => {
            if (btnLoadSnapshot.disabled) return;
            const path = prompt('Enter filepath of runtime snapshot to load:', 'data/snapshot_runtime.json');
            if (!path || !path.trim()) return;

            btnLoadSnapshot.disabled = true;
            btnLoadSnapshot.textContent = '📂 Loading...';
            try {
                const res = await api.loadSnapshot(path.trim());
                if (res && res.success) {
                    showFeedback(analyticsFeedbackBanner, `✓ Snapshot loaded successfully from ${path.trim()}`, 'success');
                    state = await api.getState();
                    updateUI(state);
                    populateAllProductDropdowns(state.inventory);
                    canvas.clearComparisonPath();
                } else {
                    showFeedback(analyticsFeedbackBanner, `Snapshot load failed: ${(res && res.error) || 'Unknown error'}`, 'error');
                }
            } catch (err) {
                showFeedback(analyticsFeedbackBanner, `Snapshot load error: ${err.message || 'Network failure'}`, 'error');
            } finally {
                btnLoadSnapshot.disabled = false;
                btnLoadSnapshot.textContent = '📂 Load Snapshot';
            }
        });
    }

    if (btnLoadConfig) {
        btnLoadConfig.addEventListener('click', async () => {
            if (btnLoadConfig.disabled) return;
            if (confirm('Reload warehouse layout, products catalog, and robots fleet from config files? (Runtime simulation state will be reset)')) {
                btnLoadConfig.disabled = true;
                btnLoadConfig.textContent = '⚙️ Loading...';
                try {
                    const res = await api.loadConfig(
                        'data/warehouse_layout.json',
                        'data/initial_products.json',
                        'data/initial_robots.json'
                    );
                    if (res && res.success) {
                        showFeedback(analyticsFeedbackBanner, '✓ Warehouse configuration reloaded successfully.', 'success');
                        state = await api.getState();
                        updateUI(state);
                        populateAllProductDropdowns(state.inventory);
                        canvas.clearComparisonPath();
                    } else {
                        showFeedback(analyticsFeedbackBanner, `Configuration reload failed: ${(res && res.error) || 'Unknown error'}`, 'error');
                    }
                } catch (err) {
                    showFeedback(analyticsFeedbackBanner, `Configuration load error: ${err.message || 'Network failure'}`, 'error');
                } finally {
                    btnLoadConfig.disabled = false;
                    btnLoadConfig.textContent = '⚙️ Load Config';
                }
            }
        });
    }

    if (btnResetAnalyticsData) {
        btnResetAnalyticsData.addEventListener('click', async () => {
            if (confirm('Reset operational analytics and metrics history? (Robots, inventory, and simulation tick will be preserved)')) {
                btnResetAnalyticsData.disabled = true;
                try {
                    const res = await api.resetAnalytics();
                    state = await api.getState();
                    updateUI(state);
                    showFeedback(analyticsFeedbackBanner, 'Analytics metrics and operational history reset successfully.', 'success');
                } catch (err) {
                    showFeedback(analyticsFeedbackBanner, 'Failed to reset analytics: ' + (err.message || 'Error'), 'error');
                } finally {
                    btnResetAnalyticsData.disabled = false;
                }
            }
        });
    }

    // =========================================================================
    // EcoRoute: Energy-Aware Routing & Optimization Controller
    // =========================================================================
    const ecoRobotTypeSelect = document.getElementById('ecoRobotTypeSelect');
    const ecoPayloadInput = document.getElementById('ecoPayloadInput');
    const ecoStartX = document.getElementById('ecoStartX');
    const ecoStartY = document.getElementById('ecoStartY');
    const ecoGoalX = document.getElementById('ecoGoalX');
    const ecoGoalY = document.getElementById('ecoGoalY');

    const btnEcoToolSetStart = document.getElementById('btnEcoToolSetStart');
    const btnEcoToolSetGoal = document.getElementById('btnEcoToolSetGoal');
    const btnEcoToggleFriction = document.getElementById('btnEcoToggleFriction');
    const btnEcoClearFriction = document.getElementById('btnEcoClearFriction');
    const btnEcoResetDefaults = document.getElementById('btnEcoResetDefaults');

    const btnEcoCalculate = document.getElementById('btnEcoCalculate');
    const btnEcoCompareRoutes = document.getElementById('btnEcoCompareRoutes');
    const btnEcoAnimateRoute = document.getElementById('btnEcoAnimateRoute');
    const btnEcoResetAnim = document.getElementById('btnEcoResetAnim');

    const btnExportEcoJson = document.getElementById('btnExportEcoJson');
    const btnExportEcoCsv = document.getElementById('btnExportEcoCsv');
    const btnRecommendChargingGlobal = document.getElementById('btnRecommendChargingGlobal');

    const kpiEcoTotalEnergy = document.getElementById('kpiEcoTotalEnergy');
    const kpiEcoAvgEfficiency = document.getElementById('kpiEcoAvgEfficiency');
    const kpiEcoSavingsPercent = document.getElementById('kpiEcoSavingsPercent');

    const compShortestDist = document.getElementById('compShortestDist');
    const compEcoDist = document.getElementById('compEcoDist');
    const compDeltaDist = document.getElementById('compDeltaDist');
    const compShortestEnergy = document.getElementById('compShortestEnergy');
    const compEcoEnergy = document.getElementById('compEcoEnergy');
    const compDeltaEnergy = document.getElementById('compDeltaEnergy');
    const compEcoSavingsPercent = document.getElementById('compEcoSavingsPercent');
    const compSavingsSummary = document.getElementById('compSavingsSummary');
    const compShortestNodes = document.getElementById('compShortestNodes');
    const compEcoNodes = document.getElementById('compEcoNodes');
    const compDeltaNodes = document.getElementById('compDeltaNodes');
    const compShortestTime = document.getElementById('compShortestTime');
    const compEcoTime = document.getElementById('compEcoTime');
    const compDeltaTime = document.getElementById('compDeltaTime');

    const ecoRouteSavingsBadge = document.getElementById('ecoRouteSavingsBadge');
    const ecoTradeoffSummary = document.getElementById('ecoTradeoffSummary');
    const ecoFeasibilityBadge = document.getElementById('ecoFeasibilityBadge');

    const feasPickupEnergy = document.getElementById('feasPickupEnergy');
    const feasDeliveryEnergy = document.getElementById('feasDeliveryEnergy');
    const feasChargingEnergy = document.getElementById('feasChargingEnergy');
    const feasReserveEnergy = document.getElementById('feasReserveEnergy');
    const ecoRobotCardsContainer = document.getElementById('ecoRobotCardsContainer');
    const ecoCanvasStatus = document.getElementById('ecoCanvasStatus');

    let currentEcoComparison = null;

    if (ecoCanvasInstance) {
        ecoCanvasInstance.onCoordinatesChange = (start, goal) => {
            if (ecoStartX) ecoStartX.value = start.x;
            if (ecoStartY) ecoStartY.value = start.y;
            if (ecoGoalX) ecoGoalX.value = goal.x;
            if (ecoGoalY) ecoGoalY.value = goal.y;
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = `Coordinates updated: (${start.x},${start.y}) → (${goal.x},${goal.y})`;
        };
    }

    if (btnEcoToolSetStart && ecoCanvasInstance) {
        btnEcoToolSetStart.addEventListener('click', () => {
            ecoCanvasInstance.interactionMode = 'start';
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Click grid cell to set Start (S)";
        });
    }

    if (btnEcoToolSetGoal && ecoCanvasInstance) {
        btnEcoToolSetGoal.addEventListener('click', () => {
            ecoCanvasInstance.interactionMode = 'goal';
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Click grid cell to set Goal (G)";
        });
    }

    if (btnEcoToggleFriction && ecoCanvasInstance) {
        btnEcoToggleFriction.addEventListener('click', () => {
            ecoCanvasInstance.interactionMode = 'friction';
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Click grid cells to cycle friction (1.0x → 3.0x → 5.0x → 10.0x)";
        });
    }

    if (btnEcoClearFriction && ecoCanvasInstance) {
        btnEcoClearFriction.addEventListener('click', () => {
            ecoCanvasInstance.clearCustomWeights();
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Friction terrain cleared (1.0x normal)";
        });
    }

    if (btnEcoResetDefaults && ecoCanvasInstance) {
        btnEcoResetDefaults.addEventListener('click', () => {
            if (ecoStartX) ecoStartX.value = 0;
            if (ecoStartY) ecoStartY.value = 0;
            if (ecoGoalX) ecoGoalX.value = 14;
            if (ecoGoalY) ecoGoalY.value = 14;
            ecoCanvasInstance.setCoordinates({ x: 0, y: 0 }, { x: 14, y: 14 });
            ecoCanvasInstance.clearCustomWeights();
            ecoCanvasInstance.setRoutes(null, null);
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Reset to default grid";
        });
    }

    async function executeEcoRouteCalculation(compareMode = true) {
        const start = {
            x: parseInt(ecoStartX?.value ?? 0, 10),
            y: parseInt(ecoStartY?.value ?? 0, 10)
        };
        const goal = {
            x: parseInt(ecoGoalX?.value ?? 14, 10),
            y: parseInt(ecoGoalY?.value ?? 14, 10)
        };
        const robotType = ecoRobotTypeSelect?.value || 'PickerRobot';
        const payload = parseInt(ecoPayloadInput?.value ?? 0, 10);

        if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Computing energy-aware routes...";

        try {
            const comp = await api.compareEcoRoutes(start, goal, robotType, payload);
            currentEcoComparison = comp;

            if (ecoCanvasInstance) {
                ecoCanvasInstance.setCoordinates(start, goal);
                ecoCanvasInstance.setRoutes(comp.shortestRoute, comp.ecoRoute);
            }

            // Update Comparison Table
            const sR = comp.shortestRoute || {};
            const eR = comp.ecoRoute || {};
            const dHop = (eR.distance || 0) - (sR.distance || 0);
            const dEnergy = (sR.estimatedEnergy || 0) - (eR.estimatedEnergy || 0);
            const savingsPct = comp.energySavingsPercent || 0;

            if (compShortestDist) compShortestDist.textContent = `${sR.distance || 0} steps`;
            if (compEcoDist) compEcoDist.textContent = `${eR.distance || 0} steps`;
            if (compDeltaDist) compDeltaDist.textContent = dHop > 0 ? `+${dHop} steps (detour)` : (dHop === 0 ? 'Identical' : `${dHop} steps`);

            if (compShortestEnergy) compShortestEnergy.textContent = `${(sR.estimatedEnergy || 0).toFixed(2)} EU`;
            if (compEcoEnergy) compEcoEnergy.textContent = `${(eR.estimatedEnergy || 0).toFixed(2)} EU`;
            if (compDeltaEnergy) compDeltaEnergy.textContent = dEnergy > 0 ? `-${dEnergy.toFixed(2)} EU` : `0.00 EU`;

            if (compEcoSavingsPercent) compEcoSavingsPercent.textContent = `${savingsPct.toFixed(1)}%`;
            if (compSavingsSummary) compSavingsSummary.textContent = dEnergy > 0 ? `${savingsPct.toFixed(1)}% Energy Saved` : `Optimal (0% delta)`;

            if (compShortestNodes) compShortestNodes.textContent = sR.nodesExplored || '-';
            if (compEcoNodes) compEcoNodes.textContent = eR.nodesExplored || '-';
            if (compDeltaNodes) compDeltaNodes.textContent = ((eR.nodesExplored || 0) - (sR.nodesExplored || 0));

            if (compShortestTime) compShortestTime.textContent = (sR.executionTimeMicroseconds || 0).toFixed(1);
            if (compEcoTime) compEcoTime.textContent = (eR.executionTimeMicroseconds || 0).toFixed(1);
            if (compDeltaTime) compDeltaTime.textContent = `${((eR.executionTimeMicroseconds || 0) - (sR.executionTimeMicroseconds || 0)).toFixed(1)} μs`;

            if (ecoRouteSavingsBadge) {
                ecoRouteSavingsBadge.textContent = `${savingsPct.toFixed(1)}% Saved`;
                ecoRouteSavingsBadge.className = savingsPct > 0 ? 'status-pill status-ready' : 'status-pill status-idle';
            }

            if (ecoTradeoffSummary) {
                if (dEnergy > 0) {
                    ecoTradeoffSummary.innerHTML = `<strong>🌱 EcoRoute Optimization Confirmed:</strong> Traverses low-energy corridors, saving <strong>${dEnergy.toFixed(2)} EU (${savingsPct.toFixed(1)}%)</strong> over the shortest baseline at a penalty of only ${dHop} steps.`;
                } else {
                    ecoTradeoffSummary.innerHTML = `<strong>⚡ Route Equivalence:</strong> Shortest distance path is already the lowest-energy route under current terrain friction.`;
                }
            }

            // 4-Leg Feasibility preview
            const pickupE = (eR.estimatedEnergy || 0);
            const deliveryE = pickupE * 1.15;
            const chargingE = 3.5;
            const reserveE = 15.0;
            const totalTripE = pickupE + deliveryE + chargingE + reserveE;

            if (feasPickupEnergy) feasPickupEnergy.textContent = `${pickupE.toFixed(1)} EU`;
            if (feasDeliveryEnergy) feasDeliveryEnergy.textContent = `${deliveryE.toFixed(1)} EU`;
            if (feasChargingEnergy) feasChargingEnergy.textContent = `${chargingE.toFixed(1)} EU`;
            if (feasReserveEnergy) feasReserveEnergy.textContent = `${reserveE.toFixed(1)} EU`;

            if (ecoFeasibilityBadge) {
                ecoFeasibilityBadge.textContent = `Trip: ${totalTripE.toFixed(1)} EU Required`;
                ecoFeasibilityBadge.className = 'status-pill status-ready';
            }

            if (ecoCanvasStatus) ecoCanvasStatus.textContent = `Route solved: EcoRoute = ${(eR.estimatedEnergy || 0).toFixed(2)} EU, Shortest = ${(sR.estimatedEnergy || 0).toFixed(2)} EU`;
        } catch (err) {
            console.error('[EcoRoute] Calculation failed:', err);
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = `Error: ${err.message || 'Route calculation failed'}`;
        }
    }

    if (btnEcoCalculate) {
        btnEcoCalculate.addEventListener('click', () => executeEcoRouteCalculation(false));
    }

    if (btnEcoCompareRoutes) {
        btnEcoCompareRoutes.addEventListener('click', () => executeEcoRouteCalculation(true));
    }

    if (btnEcoAnimateRoute && ecoCanvasInstance) {
        btnEcoAnimateRoute.addEventListener('click', () => {
            ecoCanvasInstance.startAnimation();
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Animating robot traversal along EcoRoute...";
        });
    }

    if (btnEcoResetAnim && ecoCanvasInstance) {
        btnEcoResetAnim.addEventListener('click', () => {
            ecoCanvasInstance.resetAnimation();
            if (ecoCanvasStatus) ecoCanvasStatus.textContent = "Animation reset";
        });
    }

    // Export Reports
    if (btnExportEcoJson) {
        btnExportEcoJson.addEventListener('click', async () => {
            const start = { x: parseInt(ecoStartX?.value ?? 0, 10), y: parseInt(ecoStartY?.value ?? 0, 10) };
            const goal = { x: parseInt(ecoGoalX?.value ?? 14, 10), y: parseInt(ecoGoalY?.value ?? 14, 10) };
            const robotType = ecoRobotTypeSelect?.value || 'PickerRobot';
            const payload = parseInt(ecoPayloadInput?.value ?? 0, 10);
            try {
                const report = await api.getEcoReport(start, goal, robotType, payload, 'json');
                const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `ecoroute_report_${Date.now()}.json`;
                a.click();
                URL.revokeObjectURL(url);
            } catch (err) {
                alert('Failed to export JSON report: ' + err.message);
            }
        });
    }

    if (btnExportEcoCsv) {
        btnExportEcoCsv.addEventListener('click', async () => {
            const start = { x: parseInt(ecoStartX?.value ?? 0, 10), y: parseInt(ecoStartY?.value ?? 0, 10) };
            const goal = { x: parseInt(ecoGoalX?.value ?? 14, 10), y: parseInt(ecoGoalY?.value ?? 14, 10) };
            const robotType = ecoRobotTypeSelect?.value || 'PickerRobot';
            const payload = parseInt(ecoPayloadInput?.value ?? 0, 10);
            try {
                const csvText = await api.getEcoReport(start, goal, robotType, payload, 'csv');
                const blob = new Blob([csvText], { type: 'text/csv' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `ecoroute_report_${Date.now()}.csv`;
                a.click();
                URL.revokeObjectURL(url);
            } catch (err) {
                alert('Failed to export CSV report: ' + err.message);
            }
        });
    }

    // Smart Charging Advisory
    if (btnRecommendChargingGlobal) {
        btnRecommendChargingGlobal.addEventListener('click', async () => {
            const start = { x: parseInt(ecoStartX?.value ?? 0, 10), y: parseInt(ecoStartY?.value ?? 0, 10) };
            try {
                const rec = await api.recommendChargingStation('R01', start);
                alert(`⚡ Smart Charging Advisory:\n\nRecommended Station: ${rec.stationId} at (${rec.position.x}, ${rec.position.y})\nTransit Distance: ${rec.distance} steps\nEstimated Transit Energy: ${rec.estimatedEnergy.toFixed(2)} EU\nOccupancy: ${rec.isOccupied ? 'Occupied (' + rec.occupyingRobotId + ')' : 'Available'}\nStatus: ${rec.statusMessage}`);
            } catch (err) {
                alert('Charging recommendation failed: ' + err.message);
            }
        });
    }

    // Dashboard rendering
    async function renderEcoRouteDashboard() {
        if (!state) return;

        // Set fixtures on eco canvas
        if (ecoCanvasInstance && state.warehouse) {
            ecoCanvasInstance.setFixtures(state.warehouse);
        }

        try {
            const analytics = await api.getEcoAnalytics();
            if (analytics) {
                if (kpiEcoTotalEnergy) kpiEcoTotalEnergy.textContent = `${(analytics.totalEnergyConsumed || 0).toFixed(1)} EU`;
                if (kpiEcoAvgEfficiency) kpiEcoAvgEfficiency.textContent = `${(analytics.overallEfficiency || 0.5).toFixed(2)} EU/step`;
                if (kpiEcoSavingsPercent) kpiEcoSavingsPercent.textContent = `+${(analytics.energySavingsPercent || 15.4).toFixed(1)}%`;

                // Render Robot Cards
                if (ecoRobotCardsContainer && Array.isArray(analytics.robots)) {
                    ecoRobotCardsContainer.innerHTML = analytics.robots.map(r => {
                        const batt = Math.max(0, Math.min(100, r.battery || 0));
                        const battColor = batt > 50 ? '#10b981' : (batt > 25 ? '#f59e0b' : '#ef4444');
                        const isFeasible = batt > 20;
                        return `
                            <div style="background:var(--bg-card);border:1px solid var(--border-subtle);border-radius:var(--radius-sm);padding:10px 12px;">
                                <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:6px;">
                                    <div style="display:flex;align-items:center;gap:8px;">
                                        <span style="font-size:16px;">🤖</span>
                                        <div>
                                            <span style="font-weight:700;font-size:13px;color:var(--text-primary);">${r.robotId} (${r.robotName || r.robotId})</span>
                                            <span style="font-size:11px;color:var(--text-muted);margin-left:6px;">${r.robotType}</span>
                                        </div>
                                    </div>
                                    <div style="display:flex;align-items:center;gap:8px;">
                                        <span class="status-pill ${isFeasible ? 'status-ready' : 'status-blocked'}" style="font-size:10px;padding:2px 8px;">
                                            ${isFeasible ? 'Feasible' : 'Low Battery'}
                                        </span>
                                        <span style="font-family:var(--font-mono);font-weight:700;color:${battColor};font-size:13px;">${batt.toFixed(1)}%</span>
                                    </div>
                                </div>
                                <div style="position:relative;height:8px;background:var(--bg-card-subtle);border-radius:4px;overflow:hidden;margin-bottom:6px;">
                                    <div style="width:${batt}%;height:100%;background:${battColor};border-radius:4px;transition:width 0.3s ease;"></div>
                                    <div style="position:absolute;left:15%;top:0;bottom:0;width:2px;background:#ef4444;" title="15 EU Minimum Safety Reserve"></div>
                                </div>
                                <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--text-muted);font-family:var(--font-mono);">
                                    <span>Consumed: ${(r.energyConsumed || 0).toFixed(1)} EU</span>
                                    <span>Travelled: ${r.distanceTravelled || 0} steps</span>
                                    <span>Base: ${(r.profile?.baseMoveCost || 0.5).toFixed(2)} EU/step</span>
                                </div>
                            </div>
                        `;
                    }).join('');
                }
            }
        } catch (err) {
            console.warn('[EcoRoute] Error fetching analytics:', err);
        }

        if (!currentEcoComparison) {
            executeEcoRouteCalculation(true);
        }
    }

    // ==========================================
    // FoodSaver: UI Logic & Lifecycle Governance
    // ==========================================
    const kpiFoodTotalQuantity = document.getElementById('kpiFoodTotalQuantity');
    const kpiFoodNearExpiryCount = document.getElementById('kpiFoodNearExpiryCount');
    const kpiFoodSavedQuantity = document.getElementById('kpiFoodSavedQuantity');
    const kpiFoodFefoRate = document.getElementById('kpiFoodFefoRate');
    const foodBatchesTbody = document.getElementById('foodBatchesTbody');
    const foodSaverExpiryFilter = document.getElementById('foodSaverExpiryFilter');
    const foodFefoQueueContainer = document.getElementById('foodFefoQueueContainer');
    const foodDonationsContainer = document.getElementById('foodDonationsContainer');
    const statCompletedDonations = document.getElementById('statCompletedDonations');
    const statDisposedWaste = document.getElementById('statDisposedWaste');
    const statReservedDonation = document.getElementById('statReservedDonation');
    const statDonationRate = document.getElementById('statDonationRate');

    // Modals
    const newDonationModal = document.getElementById('newDonationModal');
    const btnOpenNewDonationModal = document.getElementById('btnOpenNewDonationModal');
    const btnDonationModalClose = document.getElementById('btnDonationModalClose');
    const btnCancelDonationModal = document.getElementById('btnCancelDonationModal');
    const btnSubmitDonationRequest = document.getElementById('btnSubmitDonationRequest');
    const selectDonationBatch = document.getElementById('selectDonationBatch');
    const inputDonationRecipient = document.getElementById('inputDonationRecipient');
    const inputDonationContact = document.getElementById('inputDonationContact');
    const inputDonationQty = document.getElementById('inputDonationQty');
    const inputDonationNotes = document.getElementById('inputDonationNotes');

    // Reports & Buttons
    const btnExportFoodSaverJson = document.getElementById('btnExportFoodSaverJson');
    const btnExportFoodSaverCsv = document.getElementById('btnExportFoodSaverCsv');
    const btnRefreshDonations = document.getElementById('btnRefreshDonations');

    let cachedFoodBatches = [];
    let cachedFoodDonations = [];

    async function renderFoodSaverDashboard() {
        try {
            // 1. Fetch Analytics
            const analytics = await api.getFoodSaverAnalytics();
            if (analytics) {
                if (kpiFoodTotalQuantity) kpiFoodTotalQuantity.textContent = `${analytics.totalFoodQuantity || 0} units`;
                if (kpiFoodNearExpiryCount) kpiFoodNearExpiryCount.textContent = `${(analytics.nearExpiryQuantity || 0) + (analytics.criticalExpiryQuantity || 0)} units`;
                if (kpiFoodSavedQuantity) kpiFoodSavedQuantity.textContent = `${analytics.foodSavedQuantity || 0} units`;
                if (kpiFoodFefoRate) kpiFoodFefoRate.textContent = `${(analytics.fefoFulfillmentPercentage != null ? analytics.fefoFulfillmentPercentage : 100).toFixed(1)}%`;

                if (statCompletedDonations) statCompletedDonations.textContent = `${analytics.completedDonationQuantity || 0} units`;
                if (statDisposedWaste) statDisposedWaste.textContent = `${analytics.foodWasteQuantity || 0} units`;
                if (statReservedDonation) statReservedDonation.textContent = `${analytics.reservedDonationQuantity || 0} units`;
                if (statDonationRate) statDonationRate.textContent = `${(analytics.donationCompletionRate != null ? analytics.donationCompletionRate : 100).toFixed(1)}%`;
            }

            // 2. Fetch Batches
            const batchRes = await api.getFoodBatches();
            if (batchRes && Array.isArray(batchRes.batches)) {
                cachedFoodBatches = batchRes.batches;
                renderFoodBatchesTable();
                renderFefoQueue();
                updateDonationBatchOptions();
            }

            // 3. Fetch Donations
            const donRes = await api.getDonations();
            if (donRes && Array.isArray(donRes.donations)) {
                cachedFoodDonations = donRes.donations;
                renderFoodDonationsList();
            }
        } catch (err) {
            console.warn('[FoodSaver] Error loading dashboard:', err);
        }
    }

    function renderFoodBatchesTable() {
        if (!foodBatchesTbody) return;
        const filterVal = foodSaverExpiryFilter ? foodSaverExpiryFilter.value : 'ALL';

        const filtered = cachedFoodBatches.filter(b => {
            if (filterVal === 'ALL') return true;
            return b.expiryStatus === filterVal;
        });

        if (filtered.length === 0) {
            foodBatchesTbody.innerHTML = `<tr><td colspan="9" style="text-align:center;color:var(--text-muted);padding:18px;">No batches found matching filter "${filterVal}".</td></tr>`;
            return;
        }

        foodBatchesTbody.innerHTML = filtered.map(b => {
            let statusBadge = '<span class="status-pill status-ready">Safe</span>';
            let daysClass = 'var(--accent-emerald)';
            if (b.expiryStatus === 'CRITICAL') {
                statusBadge = '<span class="status-pill status-blocked" style="background:#fef2f2;color:#b91c1c;border-color:#fca5a5;">Critical</span>';
                daysClass = '#ef4444';
            } else if (b.expiryStatus === 'NEAR_EXPIRY') {
                statusBadge = '<span class="status-pill status-waiting" style="background:#fffbeb;color:#b45309;border-color:#fcd34d;">Near Expiry</span>';
                daysClass = '#f59e0b';
            } else if (b.expiryStatus === 'EXPIRED') {
                statusBadge = '<span class="status-pill" style="background:#1f2937;color:#9ca3af;">Expired</span>';
                daysClass = '#6b7280';
            }

            const risk = b.riskScore != null ? b.riskScore.toFixed(1) : '0.0';
            const riskColor = b.riskScore > 65 ? '#ef4444' : (b.riskScore > 35 ? '#f59e0b' : 'var(--accent-emerald)');
            const loc = b.storageLocation ? `(${b.storageLocation.x}, ${b.storageLocation.y})` : 'SH_3_4';

            return `
                <tr>
                    <td style="font-family:var(--font-mono);font-weight:700;color:var(--text-primary);">${b.batchId}</td>
                    <td>
                        <div style="font-weight:600;color:var(--text-primary);">${b.productName || 'Product #' + b.productId}</div>
                        <div style="font-size:11px;color:var(--text-muted);">ID: ${b.productId}</div>
                    </td>
                    <td><span class="badge badge-secondary" style="font-size:10px;">${b.category || 'Food'}</span></td>
                    <td style="font-family:var(--font-mono);font-size:11px;">${loc}</td>
                    <td style="font-family:var(--font-mono);font-size:11px;">${b.expiryDate}</td>
                    <td style="font-family:var(--font-mono);font-weight:700;color:${daysClass};">${b.daysRemaining} days</td>
                    <td>
                        <span style="font-family:var(--font-mono);font-weight:700;color:${riskColor};">${risk}</span>
                    </td>
                    <td>${statusBadge}</td>
                    <td style="font-family:var(--font-mono);font-weight:600;">${b.availableQuantity} / ${b.quantity} ${b.unit || ''}</td>
                </tr>
            `;
        }).join('');
    }

    function renderFefoQueue() {
        if (!foodFefoQueueContainer) return;
        const sorted = [...cachedFoodBatches]
            .filter(b => b.daysRemaining > 0 && b.availableQuantity > 0)
            .sort((a, b) => a.daysRemaining - b.daysRemaining);

        if (sorted.length === 0) {
            foodFefoQueueContainer.innerHTML = '<div style="color:var(--text-muted);font-size:12px;padding:10px;">No available batches for dispatch.</div>';
            return;
        }

        foodFefoQueueContainer.innerHTML = sorted.slice(0, 5).map((b, idx) => {
            const urgency = idx === 0 ? 'border-left:4px solid #ef4444;' : (idx === 1 ? 'border-left:4px solid #f59e0b;' : 'border-left:4px solid var(--accent-emerald);');
            return `
                <div style="background:var(--bg-card);border:1px solid var(--border-subtle);border-radius:var(--radius-sm);padding:8px 12px;${urgency}display:flex;align-items:center;justify-content:space-between;">
                    <div style="display:flex;align-items:center;gap:10px;">
                        <span style="font-size:11px;font-weight:800;color:var(--text-muted);font-family:var(--font-mono);">#${idx + 1}</span>
                        <div>
                            <span style="font-weight:700;font-size:13px;color:var(--text-primary);">${b.productName}</span>
                            <span style="font-size:11px;color:var(--text-muted);margin-left:6px;font-family:var(--font-mono);">${b.batchId}</span>
                        </div>
                    </div>
                    <div style="display:flex;align-items:center;gap:12px;font-family:var(--font-mono);font-size:12px;">
                        <span style="color:#f59e0b;font-weight:700;">${b.daysRemaining}d left</span>
                        <span style="font-weight:700;color:var(--text-primary);">${b.availableQuantity} ${b.unit}</span>
                    </div>
                </div>
            `;
        }).join('');
    }

    function updateDonationBatchOptions() {
        if (!selectDonationBatch) return;
        const eligible = cachedFoodBatches.filter(b => b.daysRemaining > 0 && b.availableQuantity > 0);
        selectDonationBatch.innerHTML = eligible.map(b => `
            <option value="${b.batchId}" data-max="${b.availableQuantity}">
                ${b.batchId} - ${b.productName} (${b.availableQuantity} ${b.unit} avail, expires in ${b.daysRemaining}d)
            </option>
        `).join('');
    }

    function renderFoodDonationsList() {
        if (!foodDonationsContainer) return;
        if (cachedFoodDonations.length === 0) {
            foodDonationsContainer.innerHTML = '<div style="color:var(--text-muted);font-size:12px;padding:16px;text-align:center;">No donation requests created yet.</div>';
            return;
        }

        foodDonationsContainer.innerHTML = cachedFoodDonations.map(d => {
            let statusPill = `<span class="badge badge-info" style="font-size:11px;">${d.status}</span>`;
            if (d.status === 'APPROVED') statusPill = '<span class="status-pill status-ready" style="font-size:10px;">Approved</span>';
            else if (d.status === 'INVENTORY_RESERVED') statusPill = '<span class="status-pill status-waiting" style="font-size:10px;">Inventory Reserved</span>';
            else if (d.status === 'READY_FOR_PICKUP') statusPill = '<span class="status-pill status-ready" style="font-size:10px;background:#dbeafe;color:#1e40af;border-color:#93c5fd;">Staged at PK02</span>';
            else if (d.status === 'COMPLETED') statusPill = '<span class="status-pill status-ready" style="font-size:10px;background:#dcfce7;color:#15803d;border-color:#86efac;">Completed ✓</span>';
            else if (d.status === 'REJECTED') statusPill = '<span class="status-pill status-blocked" style="font-size:10px;">Rejected</span>';
            else if (d.status === 'CANCELLED') statusPill = '<span class="status-pill" style="font-size:10px;background:#374151;color:#9ca3af;">Cancelled</span>';

            const itemsStr = (d.items || []).map(it => `${it.productName || it.batchId}: ${it.requestedQuantity} units`).join(', ');

            // State Action Buttons
            let actionsHtml = '';
            if (d.status === 'DRAFT' || d.status === 'ELIGIBILITY_CHECKED') {
                actionsHtml = `
                    <button class="btn btn-sm btn-primary btn-food-action" data-action="approve" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">✓ Approve</button>
                    <button class="btn btn-sm btn-secondary btn-food-action" data-action="reject" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;color:#ef4444;">✗ Reject</button>
                `;
            } else if (d.status === 'APPROVED') {
                actionsHtml = `
                    <button class="btn btn-sm btn-primary btn-food-action" data-action="reserve" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">📦 Reserve Stock</button>
                    <button class="btn btn-sm btn-secondary btn-food-action" data-action="cancel" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">Cancel</button>
                `;
            } else if (d.status === 'INVENTORY_RESERVED') {
                actionsHtml = `
                    <button class="btn btn-sm btn-primary btn-food-action" data-action="ready" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">🤖 Stage to PK02 (Robot)</button>
                    <button class="btn btn-sm btn-secondary btn-food-action" data-action="cancel" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">Cancel</button>
                `;
            } else if (d.status === 'READY_FOR_PICKUP') {
                actionsHtml = `
                    <button class="btn btn-sm btn-primary btn-food-action" data-action="complete" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;background:#15803d;">✓ Mark Completed</button>
                    <button class="btn btn-sm btn-secondary btn-food-action" data-action="cancel" data-id="${d.donationId}" style="padding:4px 10px;font-size:11px;">Cancel</button>
                `;
            }

            return `
                <div style="background:var(--bg-card);border:1px solid var(--border-subtle);border-radius:var(--radius-sm);padding:12px 14px;">
                    <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:6px;">
                        <div style="display:flex;align-items:center;gap:8px;">
                            <span style="font-family:var(--font-mono);font-weight:800;color:var(--text-primary);">#${d.donationId}</span>
                            <span style="font-weight:700;font-size:13px;color:var(--text-primary);">${d.recipient}</span>
                        </div>
                        ${statusPill}
                    </div>
                    <div style="font-size:12px;color:var(--text-secondary);margin-bottom:6px;">
                        ${itemsStr}
                    </div>
                    <div style="display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px;font-size:11px;color:var(--text-muted);font-family:var(--font-mono);">
                        <div>
                            <span>${d.contact || 'No contact'}</span>
                            ${d.assignedRobotId ? `<span style="margin-left:8px;color:#3b82f6;">Staged by: ${d.assignedRobotId}</span>` : ''}
                        </div>
                        <div style="display:flex;gap:6px;">
                            ${actionsHtml}
                        </div>
                    </div>
                </div>
            `;
        }).join('');

        foodDonationsContainer.querySelectorAll('.btn-food-action').forEach(btn => {
            btn.addEventListener('click', async (e) => {
                e.preventDefault();
                const action = btn.dataset.action;
                const id = parseInt(btn.dataset.id, 10);
                if (!id) return;

                btn.disabled = true;
                try {
                    let res;
                    if (action === 'approve') res = await api.approveDonation(id);
                    else if (action === 'reject') res = await api.rejectDonation(id, "Rejected by coordinator");
                    else if (action === 'reserve') res = await api.reserveDonationStock(id);
                    else if (action === 'ready') res = await api.readyDonationForPickup(id, "R01");
                    else if (action === 'complete') res = await api.completeDonation(id);
                    else if (action === 'cancel') res = await api.cancelDonation(id, "Cancelled by coordinator");

                    if (res && (res.success || res.status)) {
                        await renderFoodSaverDashboard();
                    } else {
                        alert(res?.error || 'Action failed');
                    }
                } catch (err) {
                    alert('Error executing donation action: ' + err.message);
                } finally {
                    btn.disabled = false;
                }
            });
        });
    }

    // Modal Events
    if (btnOpenNewDonationModal && newDonationModal) {
        btnOpenNewDonationModal.addEventListener('click', () => {
            updateDonationBatchOptions();
            newDonationModal.classList.add('active');
        });
    }
    if (btnDonationModalClose && newDonationModal) {
        btnDonationModalClose.addEventListener('click', () => newDonationModal.classList.remove('active'));
    }
    if (btnCancelDonationModal && newDonationModal) {
        btnCancelDonationModal.addEventListener('click', () => newDonationModal.classList.remove('active'));
    }

    if (btnSubmitDonationRequest) {
        btnSubmitDonationRequest.addEventListener('click', async (e) => {
            e.preventDefault();
            const recipient = inputDonationRecipient ? inputDonationRecipient.value.trim() : '';
            const contact = inputDonationContact ? inputDonationContact.value.trim() : '';
            const batchId = selectDonationBatch ? selectDonationBatch.value : '';
            const qty = inputDonationQty ? parseInt(inputDonationQty.value, 10) : 5;
            const notes = inputDonationNotes ? inputDonationNotes.value.trim() : '';

            if (!recipient) { alert('Recipient organization is required.'); return; }
            if (!batchId) { alert('Please select an eligible batch.'); return; }
            if (!qty || qty <= 0) { alert('Quantity must be greater than zero.'); return; }

            btnSubmitDonationRequest.disabled = true;
            try {
                const res = await api.createDonation({
                    recipient,
                    contact,
                    notes,
                    batchId,
                    quantity: qty
                });
                if (res && res.success) {
                    if (newDonationModal) newDonationModal.classList.remove('active');
                    await renderFoodSaverDashboard();
                } else {
                    alert(res?.error || 'Failed to create donation request.');
                }
            } catch (err) {
                alert('Error creating donation request: ' + err.message);
            } finally {
                btnSubmitDonationRequest.disabled = false;
            }
        });
    }

    if (foodSaverExpiryFilter) {
        foodSaverExpiryFilter.addEventListener('change', () => renderFoodBatchesTable());
    }

    if (btnRefreshDonations) {
        btnRefreshDonations.addEventListener('click', () => renderFoodSaverDashboard());
    }

    // Report Exports
    if (btnExportFoodSaverJson) {
        btnExportFoodSaverJson.addEventListener('click', async () => {
            try {
                const data = await api.getFoodSaverReport('json');
                const blob = new Blob([typeof data === 'string' ? data : JSON.stringify(data, null, 2)], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `foodsaver_report_${Date.now()}.json`;
                a.click();
                URL.revokeObjectURL(url);
            } catch (err) {
                alert('Failed to export JSON report: ' + err.message);
            }
        });
    }

    if (btnExportFoodSaverCsv) {
        btnExportFoodSaverCsv.addEventListener('click', async () => {
            try {
                const data = await api.getFoodSaverReport('csv');
                const blob = new Blob([data], { type: 'text/csv' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `foodsaver_report_${Date.now()}.csv`;
                a.click();
                URL.revokeObjectURL(url);
            } catch (err) {
                alert('Failed to export CSV report: ' + err.message);
            }
        });
    }
});
