/**
 * DSA Algorithm Laboratory Canvas Engine
 * Interactive grid visualization for BFS, Dijkstra, and A* pathfinding.
 * Features:
 * - Real-time step-by-step exploration animation
 * - Chronological expansion order gradient
 * - Frontier nodes rendering
 * - Optimal reconstructed path highlighting
 * - Interactive Start, Goal, and Obstacle placement
 * - High-DPI crisp rendering and interactive hover tooltips
 */
class AlgorithmCanvas {
    constructor(canvasId, tooltipId) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            console.error(`[AlgorithmCanvas] Canvas element #${canvasId} not found.`);
            return;
        }
        this.ctx = this.canvas.getContext('2d');
        this.tooltip = document.getElementById(tooltipId);

        this.gridSize = 15;
        this.displaySize = 540;
        this.cellSize = this.displaySize / this.gridSize;

        // Base warehouse fixtures
        this.baseShelves = [];
        this.baseChargingStations = [];
        this.basePackingStations = [];
        this.baseStaticObstacles = [];

        // Lab state
        this.startPos = { x: 0, y: 0 };
        this.goalPos = { x: 14, y: 14 };
        this.customObstacles = new Set(); // Set of "x,y" strings

        // Interaction mode: 'start' | 'goal' | 'obstacle' | null
        this.interactionMode = null;
        this.onCoordinatesChange = null;

        // Animation state
        this.currentAlgorithm = 'A*';
        this.explorationOrder = [];
        this.optimalPath = [];
        this.currentExplorationIndex = 0;
        this.animationTimer = null;
        this.isPaused = false;
        this.isPlaying = false;
        this.animationSpeedMs = 40; // milliseconds per step
        this.onStepCallback = null;
        this.onCompleteCallback = null;

        this.initCanvas();
        this.setupEvents();
        this.render();
    }

    initCanvas() {
        if (!this.canvas || !this.canvas.parentElement) return;
        const rect = this.canvas.parentElement.getBoundingClientRect();
        let size = rect.width;
        if (!size || size <= 0) {
            size = this.canvas.parentElement.clientWidth || 540;
        }
        size = Math.max(280, Math.min(size, 620));
        const dpr = window.devicePixelRatio || 1;

        this.canvas.width = Math.round(size * dpr);
        this.canvas.height = Math.round(size * dpr);
        this.canvas.style.width = `${size}px`;
        this.canvas.style.height = `${size}px`;

        if (this.ctx.resetTransform) {
            this.ctx.resetTransform();
        } else {
            this.ctx.setTransform(1, 0, 0, 1, 0, 0);
        }
        this.ctx.scale(dpr, dpr);
        this.displaySize = size;
        this.cellSize = size / this.gridSize;
    }

    setupEvents() {
        window.addEventListener('resize', () => {
            this.initCanvas();
            this.render();
        });

        this.canvas.addEventListener('mousemove', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const x = Math.floor((e.clientX - rect.left) / this.cellSize);
            const y = Math.floor((e.clientY - rect.top) / this.cellSize);

            if (x >= 0 && x < this.gridSize && y >= 0 && y < this.gridSize) {
                this.showTooltip(e.clientX, e.clientY, x, y);
            } else {
                this.hideTooltip();
            }
        });

        this.canvas.addEventListener('mouseleave', () => this.hideTooltip());

        this.canvas.addEventListener('click', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const x = Math.floor((e.clientX - rect.left) / this.cellSize);
            const y = Math.floor((e.clientY - rect.top) / this.cellSize);

            if (x < 0 || x >= this.gridSize || y < 0 || y >= this.gridSize) return;

            this.handleCellClick(x, y);
        });
    }

    setWarehouseBase(warehouseData) {
        if (!warehouseData) return;
        this.baseShelves = warehouseData.shelves || [];
        this.baseChargingStations = warehouseData.chargingStations || [];
        this.basePackingStations = warehouseData.packingStations || [];

        // Parse grid static obstacles if grid array present
        this.baseStaticObstacles = [];
        if (warehouseData.grid && Array.isArray(warehouseData.grid)) {
            for (let y = 0; y < warehouseData.grid.length; ++y) {
                for (let x = 0; x < warehouseData.grid[y].length; ++x) {
                    if (warehouseData.grid[y][x] === 1) { // 1 = OBSTACLE
                        this.baseStaticObstacles.push({ x, y });
                    }
                }
            }
        }
        this.render();
    }

    setInteractionMode(mode) {
        this.interactionMode = mode;
        this.render();
    }

    setStart(x, y) {
        if (x >= 0 && x < this.gridSize && y >= 0 && y < this.gridSize) {
            this.startPos = { x, y };
            this.customObstacles.delete(`${x},${y}`);
            this.render();
            if (this.onCoordinatesChange) this.onCoordinatesChange(this.startPos, this.goalPos);
        }
    }

    setGoal(x, y) {
        if (x >= 0 && x < this.gridSize && y >= 0 && y < this.gridSize) {
            this.goalPos = { x, y };
            this.customObstacles.delete(`${x},${y}`);
            this.render();
            if (this.onCoordinatesChange) this.onCoordinatesChange(this.startPos, this.goalPos);
        }
    }

    toggleCustomObstacle(x, y) {
        if ((x === this.startPos.x && y === this.startPos.y) || 
            (x === this.goalPos.x && y === this.goalPos.y)) {
            return;
        }
        const key = `${x},${y}`;
        if (this.customObstacles.has(key)) {
            this.customObstacles.delete(key);
        } else {
            this.customObstacles.add(key);
        }
        this.render();
    }

    clearCustomObstacles() {
        this.customObstacles.clear();
        this.resetAnimation();
        this.render();
    }

    getCustomObstaclesList() {
        const list = [];
        this.customObstacles.forEach(key => {
            const [x, y] = key.split(',').map(Number);
            list.push({ x, y });
        });
        return list;
    }

    handleCellClick(x, y) {
        if (this.interactionMode === 'start') {
            this.setStart(x, y);
            this.interactionMode = null;
        } else if (this.interactionMode === 'goal') {
            this.setGoal(x, y);
            this.interactionMode = null;
        } else if (this.interactionMode === 'obstacle') {
            this.toggleCustomObstacle(x, y);
        } else {
            // Default click behavior: cycle through setting start -> goal -> toggle obstacle
            if (this.customObstacles.has(`${x},${y}`)) {
                this.customObstacles.delete(`${x},${y}`);
            } else if (x === this.startPos.x && y === this.startPos.y) {
                // Already start
            } else if (x === this.goalPos.x && y === this.goalPos.y) {
                // Already goal
            } else {
                this.toggleCustomObstacle(x, y);
            }
        }
        this.render();
    }

    loadAlgorithmResult(pathResult) {
        this.resetAnimation();
        if (!pathResult) return;
        this.currentAlgorithm = pathResult.algorithm || pathResult.algorithmName || 'A*';
        this.explorationOrder = pathResult.explorationOrder || [];
        this.optimalPath = pathResult.path || [];
        if (pathResult.start) this.startPos = pathResult.start;
        if (pathResult.goal) this.goalPos = pathResult.goal;
        this.currentExplorationIndex = 0;
        this.render();
    }

    startAnimation(onStep, onComplete) {
        this.onStepCallback = onStep;
        this.onCompleteCallback = onComplete;
        this.resetAnimation();

        if (this.explorationOrder.length === 0) {
            if (this.onCompleteCallback) this.onCompleteCallback();
            return;
        }

        this.isPlaying = true;
        this.isPaused = false;
        this.runAnimationStep();
    }

    runAnimationStep() {
        if (!this.isPlaying || this.isPaused) return;

        if (this.currentExplorationIndex < this.explorationOrder.length) {
            this.currentExplorationIndex++;
            this.render();

            if (this.onStepCallback) {
                this.onStepCallback(this.currentExplorationIndex, this.explorationOrder.length);
            }

            this.animationTimer = setTimeout(() => {
                this.runAnimationStep();
            }, this.animationSpeedMs);
        } else {
            // Exploration completed; render final path
            this.isPlaying = false;
            this.render();
            if (this.onCompleteCallback) {
                this.onCompleteCallback();
            }
        }
    }

    pauseAnimation() {
        this.isPaused = true;
        if (this.animationTimer) {
            clearTimeout(this.animationTimer);
            this.animationTimer = null;
        }
    }

    resumeAnimation() {
        if (!this.isPlaying) {
            this.startAnimation(this.onStepCallback, this.onCompleteCallback);
            return;
        }
        if (this.isPaused) {
            this.isPaused = false;
            this.runAnimationStep();
        }
    }

    stepForward() {
        this.pauseAnimation();
        if (this.currentExplorationIndex < this.explorationOrder.length) {
            this.currentExplorationIndex++;
            this.render();
            if (this.onStepCallback) {
                this.onStepCallback(this.currentExplorationIndex, this.explorationOrder.length);
            }
        }
    }

    resetAnimation() {
        if (this.animationTimer) {
            clearTimeout(this.animationTimer);
            this.animationTimer = null;
        }
        this.isPlaying = false;
        this.isPaused = false;
        this.currentExplorationIndex = 0;
        this.render();
    }

    setSpeed(speedMs) {
        this.animationSpeedMs = Math.max(5, Math.min(speedMs, 300));
    }

    isBaseObstacle(x, y) {
        if (this.baseStaticObstacles.some(o => o.x === x && o.y === y)) return true;
        if (this.baseShelves.some(s => s.position && s.position.x === x && s.position.y === y)) return true;
        return false;
    }

    render() {
        const ctx = this.ctx;
        const size = this.displaySize;
        const cs = this.cellSize;

        // Clear canvas
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(0, 0, size, size);

        // 1. Draw Grid Background & Alternating Subtle Grid Floor
        for (let y = 0; y < this.gridSize; ++y) {
            for (let x = 0; x < this.gridSize; ++x) {
                ctx.fillStyle = (x + y) % 2 === 0 ? '#f8fafc' : '#ffffff';
                ctx.fillRect(x * cs, y * cs, cs, cs);
            }
        }

        // 2. Draw Warehouse Fixtures
        // Shelves
        this.baseShelves.forEach(s => {
            if (!s.position) return;
            const px = s.position.x * cs;
            const py = s.position.y * cs;
            ctx.fillStyle = '#334155'; // Dark slate shelf
            ctx.fillRect(px + 2, py + 2, cs - 4, cs - 4);
            ctx.fillStyle = '#94a3b8';
            ctx.fillRect(px + 4, py + cs / 2 - 1, cs - 8, 2);
        });

        // Charging Stations
        this.baseChargingStations.forEach(st => {
            if (!st.position) return;
            const px = st.position.x * cs;
            const py = st.position.y * cs;
            ctx.fillStyle = '#ecfdf5';
            ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.strokeStyle = '#10b981';
            ctx.lineWidth = 1.5;
            ctx.strokeRect(px + 2, py + 2, cs - 4, cs - 4);
            // Draw bolt
            ctx.fillStyle = '#10b981';
            ctx.font = `${Math.floor(cs * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('⚡', px + cs / 2, py + cs / 2);
        });

        // Packing Stations
        this.basePackingStations.forEach(pk => {
            if (!pk.position) return;
            const px = pk.position.x * cs;
            const py = pk.position.y * cs;
            ctx.fillStyle = '#fffbeb';
            ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.strokeStyle = '#f59e0b';
            ctx.lineWidth = 1.5;
            ctx.strokeRect(px + 2, py + 2, cs - 4, cs - 4);
            ctx.fillStyle = '#d97706';
            ctx.font = `${Math.floor(cs * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('📦', px + cs / 2, py + cs / 2);
        });

        // Base Static Obstacles
        this.baseStaticObstacles.forEach(o => {
            const px = o.x * cs;
            const py = o.y * cs;
            ctx.fillStyle = '#475569';
            ctx.fillRect(px + 2, py + 2, cs - 4, cs - 4);
        });

        // Custom Lab Obstacles
        this.customObstacles.forEach(key => {
            const [ox, oy] = key.split(',').map(Number);
            const px = ox * cs;
            const py = oy * cs;
            ctx.fillStyle = '#ef4444';
            ctx.fillRect(px + 2, py + 2, cs - 4, cs - 4);
            ctx.strokeStyle = '#b91c1c';
            ctx.lineWidth = 1.5;
            ctx.strokeRect(px + 2, py + 2, cs - 4, cs - 4);

            ctx.fillStyle = '#ffffff';
            ctx.font = `bold ${Math.floor(cs * 0.38)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('✕', px + cs / 2, py + cs / 2);
        });

        // 3. Draw Explored Nodes (Search Space Exploration History)
        if (this.currentExplorationIndex > 0 && this.explorationOrder.length > 0) {
            const totalSteps = this.explorationOrder.length;
            const count = Math.min(this.currentExplorationIndex, totalSteps);

            for (let i = 0; i < count; ++i) {
                const node = this.explorationOrder[i];
                // Don't overwrite start/goal badges completely
                const isStart = (node.x === this.startPos.x && node.y === this.startPos.y);
                const isGoal = (node.x === this.goalPos.x && node.y === this.goalPos.y);

                const px = node.x * cs;
                const py = node.y * cs;

                // Color gradient based on order index:
                // BFS: Soft blue gradient
                // Dijkstra: Emerald / Teal gradient
                // A*: Indigo / Purple gradient
                const t = count > 1 ? i / (count - 1) : 0;
                let fillStyle;
                if (this.currentAlgorithm === 'BFS') {
                    fillStyle = `rgba(59, 130, 246, ${0.25 + 0.45 * t})`; // blue
                } else if (this.currentAlgorithm === 'Dijkstra') {
                    fillStyle = `rgba(16, 185, 129, ${0.25 + 0.45 * t})`; // emerald
                } else {
                    fillStyle = `rgba(124, 58, 237, ${0.25 + 0.50 * t})`; // purple/indigo
                }

                ctx.fillStyle = fillStyle;
                ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);

                if (!isStart && !isGoal && cs >= 24) {
                    ctx.fillStyle = 'rgba(15, 23, 42, 0.45)';
                    ctx.font = `${Math.max(8, Math.floor(cs * 0.26))}px monospace`;
                    ctx.textAlign = 'center';
                    ctx.textBaseline = 'middle';
                    ctx.fillText(`${i + 1}`, px + cs / 2, py + cs / 2);
                }
            }

            // Draw Frontier Marker on latest expanded node
            if (count > 0 && count < totalSteps) {
                const head = this.explorationOrder[count - 1];
                const px = head.x * cs;
                const py = head.y * cs;
                ctx.strokeStyle = '#f59e0b';
                ctx.lineWidth = 2.5;
                ctx.strokeRect(px + 2, py + 2, cs - 4, cs - 4);
            }
        }

        // 4. Draw Optimal Reconstructed Path (When Exploration is Complete or Static View)
        if (this.optimalPath && this.optimalPath.length > 0) {
            const showPath = (this.currentExplorationIndex >= this.explorationOrder.length || !this.isPlaying);

            if (showPath) {
                // Highlight path cells
                for (let i = 0; i < this.optimalPath.length; ++i) {
                    const p = this.optimalPath[i];
                    const px = p.x * cs;
                    const py = p.y * cs;
                    ctx.fillStyle = 'rgba(251, 191, 36, 0.45)'; // Amber highlight
                    ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);
                }

                // Draw connecting route line
                ctx.beginPath();
                ctx.strokeStyle = '#d97706';
                ctx.lineWidth = Math.max(3, cs * 0.12);
                ctx.lineCap = 'round';
                ctx.lineJoin = 'round';

                for (let i = 0; i < this.optimalPath.length; ++i) {
                    const p = this.optimalPath[i];
                    const cx = p.x * cs + cs / 2;
                    const cy = p.y * cs + cs / 2;
                    if (i === 0) {
                        ctx.moveTo(cx, cy);
                    } else {
                        ctx.lineTo(cx, cy);
                    }
                }
                ctx.stroke();

                // Draw path waypoints
                for (let i = 0; i < this.optimalPath.length; ++i) {
                    const p = this.optimalPath[i];
                    const cx = p.x * cs + cs / 2;
                    const cy = p.y * cs + cs / 2;
                    ctx.beginPath();
                    ctx.arc(cx, cy, Math.max(2.5, cs * 0.1), 0, 2 * Math.PI);
                    ctx.fillStyle = '#b45309';
                    ctx.fill();
                }
            }
        }

        // 5. Draw Start Badge (S)
        const sx = this.startPos.x * cs;
        const sy = this.startPos.y * cs;
        ctx.fillStyle = '#10b981'; // Emerald
        ctx.beginPath();
        ctx.roundRect ? ctx.roundRect(sx + 3, sy + 3, cs - 6, cs - 6, 6) : ctx.fillRect(sx + 3, sy + 3, cs - 6, cs - 6);
        ctx.fill();
        ctx.fillStyle = '#ffffff';
        ctx.font = `bold ${Math.floor(cs * 0.42)}px sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('S', sx + cs / 2, sy + cs / 2);

        // 6. Draw Goal Badge (G)
        const gx = this.goalPos.x * cs;
        const gy = this.goalPos.y * cs;
        ctx.fillStyle = '#ef4444'; // Rose / Red
        ctx.beginPath();
        ctx.roundRect ? ctx.roundRect(gx + 3, gy + 3, cs - 6, cs - 6, 6) : ctx.fillRect(gx + 3, gy + 3, cs - 6, cs - 6);
        ctx.fill();
        ctx.fillStyle = '#ffffff';
        ctx.font = `bold ${Math.floor(cs * 0.42)}px sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('G', gx + cs / 2, gy + cs / 2);

        // 7. Draw Grid Lines
        ctx.strokeStyle = '#e2e8f0';
        ctx.lineWidth = 1;
        for (let i = 0; i <= this.gridSize; ++i) {
            ctx.beginPath();
            ctx.moveTo(i * cs, 0);
            ctx.lineTo(i * cs, size);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(0, i * cs);
            ctx.lineTo(size, i * cs);
            ctx.stroke();
        }

        // 8. Outer Border
        ctx.strokeStyle = '#cbd5e1';
        ctx.lineWidth = 2;
        ctx.strokeRect(0, 0, size, size);
    }

    showTooltip(clientX, clientY, x, y) {
        if (!this.tooltip) return;

        let cellType = 'Empty Floor';
        if (x === this.startPos.x && y === this.startPos.y) cellType = 'Start Position (S)';
        else if (x === this.goalPos.x && y === this.goalPos.y) cellType = 'Goal Position (G)';
        else if (this.customObstacles.has(`${x},${y}`)) cellType = 'Custom Obstacle (Blocked)';
        else if (this.baseStaticObstacles.some(o => o.x === x && o.y === y)) cellType = 'Static Warehouse Obstacle';
        else if (this.baseShelves.some(s => s.position && s.position.x === x && s.position.y === y)) cellType = 'Storage Shelf (Impassable)';
        else if (this.baseChargingStations.some(s => s.position && s.position.x === x && s.position.y === y)) cellType = 'Charging Station (Walkable)';
        else if (this.basePackingStations.some(s => s.position && s.position.x === x && s.position.y === y)) cellType = 'Packing Station (Walkable)';

        // Find step in exploration
        let stepInfo = 'Not Explored';
        const expIdx = this.explorationOrder.findIndex(p => p.x === x && p.y === y);
        if (expIdx !== -1) {
            stepInfo = `Step #${expIdx + 1} of ${this.explorationOrder.length}`;
        }

        // Manhattan distance to goal
        const manhattan = Math.abs(x - this.goalPos.x) + Math.abs(y - this.goalPos.y);

        this.tooltip.innerHTML = `
            <div style="font-weight:700;margin-bottom:4px;color:var(--text-primary);border-bottom:1px solid var(--border-subtle);padding-bottom:3px;">
                Cell (${x}, ${y})
            </div>
            <div style="margin-bottom:2px;"><strong>Type:</strong> ${cellType}</div>
            <div style="margin-bottom:2px;"><strong>Exploration:</strong> ${stepInfo}</div>
            <div><strong>Heuristic h(n):</strong> ${manhattan} units</div>
        `;
        this.tooltip.style.display = 'block';

        const offset = 14;
        let left = clientX + offset;
        let top = clientY + offset;
        if (left + 190 > window.innerWidth) left = clientX - 200;
        if (top + 100 > window.innerHeight) top = clientY - 110;

        this.tooltip.style.left = `${left}px`;
        this.tooltip.style.top = `${top}px`;
    }

    hideTooltip() {
        if (this.tooltip) {
            this.tooltip.style.display = 'none';
        }
    }
}

