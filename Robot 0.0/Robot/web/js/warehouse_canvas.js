class WarehouseCanvas {
    constructor(canvasId, tooltipId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.tooltip = document.getElementById(tooltipId);
        
        this.gridSize = 15;
        this.state = null;
        this.comparisonPath = null;
        this.selectedCell = null;
        this.onCellClick = null;
        this.heatmapEnabled = false;
        this.heatmapMode = 'visits'; // 'visits' | 'waits' | 'replans'

        this.robotInterpolations = {}; // robotId -> { curX, curY, targetX, targetY }
        this.animationFrameId = null;

        this.initCanvas();
        this.setupEvents();
        this.startRenderLoop();
    }

    initCanvas() {
        if (!this.canvas || !this.canvas.parentElement) return;
        const rect = this.canvas.parentElement.getBoundingClientRect();
        let size = rect.width;
        if (!size || size <= 0) {
            size = this.canvas.parentElement.clientWidth || this.canvas.clientWidth || 660;
        }
        size = Math.max(280, Math.min(size, 660));
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
        window.addEventListener('resize', () => this.initCanvas());

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
            if (this.onCellClick) {
                this.onCellClick(x, y);
            }
        });
    }

    updateState(newState) {
        this.state = newState;
        if (newState && newState.robots) {
            newState.robots.forEach(r => {
                const prev = this.robotInterpolations[r.id];
                if (!prev) {
                    this.robotInterpolations[r.id] = {
                        curX: r.position.x,
                        curY: r.position.y,
                        targetX: r.position.x,
                        targetY: r.position.y
                    };
                } else {
                    prev.curX = prev.targetX;
                    prev.curY = prev.targetY;
                    prev.targetX = r.position.x;
                    prev.targetY = r.position.y;
                }
            });
        }
    }

    setComparisonPath(pathResult) {
        this.comparisonPath = pathResult;
    }

    clearComparisonPath() {
        this.comparisonPath = null;
    }

    startRenderLoop() {
        const render = () => {
            this.draw();
            this.animationFrameId = requestAnimationFrame(render);
        };
        render();
    }

    draw() {
        if (!this.cellSize || this.cellSize <= 0 || !this.displaySize || this.displaySize <= 0) {
            this.initCanvas();
            if (!this.cellSize || this.cellSize <= 0) return;
        }

        const ctx = this.ctx;
        const cs = this.cellSize;
        ctx.clearRect(0, 0, this.displaySize, this.displaySize);

        // 1. Draw Clean White Floor
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(0, 0, this.displaySize, this.displaySize);

        // 2. Draw Subtle Grid Lines
        ctx.strokeStyle = '#e2e8f0';
        ctx.lineWidth = 1;
        for (let i = 0; i <= this.gridSize; ++i) {
            ctx.beginPath();
            ctx.moveTo(i * cs, 0);
            ctx.lineTo(i * cs, this.displaySize);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(0, i * cs);
            ctx.lineTo(this.displaySize, i * cs);
            ctx.stroke();
        }

        if (!this.state || !this.state.warehouse) return;

        const wh = this.state.warehouse;
        const grid = wh.grid;

        // 3. Draw Static Entities (Obstacles, Shelves, Stations)
        if (grid && Array.isArray(grid) && grid.length >= this.gridSize) {
            for (let y = 0; y < this.gridSize; ++y) {
                for (let x = 0; x < this.gridSize; ++x) {
                    const cellType = grid[y] ? grid[y][x] : 0;
                    const px = x * cs;
                    const py = y * cs;

                    if (cellType === 1) {
                        this.drawObstacle(px, py, cs);
                    } else if (cellType === 2) {
                        this.drawShelf(px, py, cs, x, y);
                    } else if (cellType === 3) {
                        this.drawChargingStation(px, py, cs);
                    } else if (cellType === 4) {
                        this.drawPackingStation(px, py, cs);
                    }
                }
            }
        } else {
            // Robust fallback from entities lists
            if (wh.shelves) wh.shelves.forEach(s => this.drawShelf(s.position.x * cs, s.position.y * cs, cs, s.position.x, s.position.y));
            if (wh.chargingStations) wh.chargingStations.forEach(c => this.drawChargingStation(c.position.x * cs, c.position.y * cs, cs));
            if (wh.packingStations) wh.packingStations.forEach(p => this.drawPackingStation(p.position.x * cs, p.position.y * cs, cs));
        }

        // 3b. Draw Dynamic Obstacles
        if (wh.dynamicObstacles && wh.dynamicObstacles.length > 0) {
            wh.dynamicObstacles.forEach(ob => {
                this.drawDynamicObstacle(ob.x * cs, ob.y * cs, cs);
            });
        }

        // 3c. Draw Spatial Congestion Heatmap Overlay
        if (this.heatmapEnabled) {
            this.drawHeatmapOverlay();
        }

        // 4. Draw Comparison Path if active
        if (this.comparisonPath && this.comparisonPath.path) {
            this.drawRoute(this.comparisonPath.path, '#7c3aed', true);
        }

        // 5. Draw Active Robot Routes
        if (this.state.robots) {
            this.state.robots.forEach(r => {
                if (r.route && r.route.length > 0) {
                    const color = r.id === 'R01' ? '#2563eb' : (r.id === 'R02' ? '#3b82f6' : (r.id === 'R03' ? '#d97706' : '#7c3aed'));
                    this.drawRoute(r.route, color, false);
                }
            });
        }

        // 5b. Draw Selected Cell Highlight Ring
        if (this.selectedCell) {
            this.drawSelectedCell();
        }

        // 6. Draw Animated Robots
        if (this.state.robots) {
            this.state.robots.forEach(r => {
                const interp = this.robotInterpolations[r.id];
                const rx = (interp && interp.targetX !== undefined) ? interp.targetX * cs : (r.position ? r.position.x * cs : 0);
                const ry = (interp && interp.targetY !== undefined) ? interp.targetY * cs : (r.position ? r.position.y * cs : 0);
                this.drawRobot(rx, ry, cs, r);
            });
        }
    }

    drawDynamicObstacle(x, y, size) {
        const ctx = this.ctx;
        ctx.fillStyle = '#fff7ed';
        ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
        
        ctx.strokeStyle = '#f97316';
        ctx.lineWidth = 1.8;
        ctx.strokeRect(x + 2, y + 2, size - 4, size - 4);

        // Soft diagonal hazard stripes
        ctx.strokeStyle = '#fdba74';
        ctx.lineWidth = 1.2;
        ctx.beginPath();
        ctx.moveTo(x + 4, y + 4);
        ctx.lineTo(x + size - 4, y + size - 4);
        ctx.moveTo(x + size - 4, y + 4);
        ctx.lineTo(x + 4, y + size - 4);
        ctx.stroke();

        // Icon
        ctx.fillStyle = '#ea580c';
        ctx.font = '11px Outfit';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('⚠️', x + size / 2, y + size / 2);
    }

    drawObstacle(x, y, size) {
        const ctx = this.ctx;
        ctx.fillStyle = '#fee2e2';
        ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
        
        ctx.strokeStyle = '#f87171';
        ctx.lineWidth = 1.5;
        ctx.strokeRect(x + 2, y + 2, size - 4, size - 4);

        // Soft diagonal stripes
        ctx.beginPath();
        ctx.moveTo(x + 5, y + 5);
        ctx.lineTo(x + size - 5, y + size - 5);
        ctx.moveTo(x + size - 5, y + 5);
        ctx.lineTo(x + 5, y + size - 5);
        ctx.stroke();
    }

    drawShelf(x, y, size, gx, gy) {
        const ctx = this.ctx;
        ctx.fillStyle = '#1e293b'; // Slate navy
        ctx.strokeStyle = '#334155';
        ctx.lineWidth = 1;
        ctx.fillRect(x + 3, y + 3, size - 6, size - 6);
        ctx.strokeRect(x + 3, y + 3, size - 6, size - 6);

        // Shelf divider line
        ctx.fillStyle = '#64748b';
        ctx.fillRect(x + 6, y + size / 2 - 1, size - 12, 1.5);

        // Clean label
        ctx.fillStyle = '#f8fafc';
        ctx.font = 'bold 9px JetBrains Mono';
        ctx.textAlign = 'center';
        ctx.fillText('S', x + size / 2, y + size / 2 - 3);
    }

    drawChargingStation(x, y, size) {
        const ctx = this.ctx;
        ctx.fillStyle = '#ecfeff';
        ctx.strokeStyle = '#06b6d4';
        ctx.lineWidth = 1.5;
        ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
        ctx.strokeRect(x + 2, y + 2, size - 4, size - 4);

        ctx.fillStyle = '#0891b2';
        ctx.font = '15px Outfit';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('⚡', x + size / 2, y + size / 2);
    }

    drawPackingStation(x, y, size) {
        const ctx = this.ctx;
        // Highlight packing station if actively occupied by a robot
        let isActive = false;
        if (this.state && this.state.robots) {
            const gridX = Math.floor(x / this.cellSize);
            const gridY = Math.floor(y / this.cellSize);
            for (const r of this.state.robots) {
                if (r.position && r.position.x === gridX && r.position.y === gridY) {
                    isActive = true;
                    break;
                }
            }
        }

        ctx.fillStyle = isActive ? '#d1fae5' : '#ecfdf5';
        ctx.strokeStyle = isActive ? '#059669' : '#10b981';
        ctx.lineWidth = isActive ? 2.5 : 1.5;
        ctx.fillRect(x + 2, y + 2, size - 4, size - 4);
        ctx.strokeRect(x + 2, y + 2, size - 4, size - 4);

        ctx.fillStyle = '#059669';
        ctx.font = '15px Outfit';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('📦', x + size / 2, y + size / 2);
    }

    drawRoute(path, color, isDashed = false) {
        if (!path || path.length < 2) return;
        const ctx = this.ctx;
        const cs = this.cellSize;

        ctx.strokeStyle = color;
        ctx.lineWidth = 2.5;
        if (isDashed) {
            ctx.setLineDash([5, 4]);
        } else {
            ctx.setLineDash([]);
        }

        ctx.beginPath();
        ctx.moveTo(path[0].x * cs + cs / 2, path[0].y * cs + cs / 2);
        for (let i = 1; i < path.length; ++i) {
            ctx.lineTo(path[i].x * cs + cs / 2, path[i].y * cs + cs / 2);
        }
        ctx.stroke();
        ctx.setLineDash([]);

        // Waypoint markers
        ctx.fillStyle = color;
        for (let i = 0; i < path.length; ++i) {
            ctx.beginPath();
            ctx.arc(path[i].x * cs + cs / 2, path[i].y * cs + cs / 2, 2.5, 0, Math.PI * 2);
            ctx.fill();
        }
    }

    drawRobot(x, y, size, robot) {
        if (!size || size <= 4) return;
        const ctx = this.ctx;
        const cx = x + size / 2;
        const cy = y + size / 2;
        const r = size * 0.38;

        // Soft drop shadow
        ctx.fillStyle = 'rgba(15, 23, 42, 0.12)';
        ctx.beginPath();
        ctx.arc(cx, cy + 2, r + 1, 0, Math.PI * 2);
        ctx.fill();

        // Distinguish Robot Types Visually
        const isTransport = robot.type === 'TransportRobot' || robot.id === 'R03';
        const isHybrid = robot.type === 'HybridRobot' || robot.id === 'R04';
        const isPicker = !isTransport && !isHybrid;

        let baseColor = '#2563eb'; // Blue for PickerRobot (SpeedyPicker)
        let typeBadge = 'PICK';
        if (isTransport) {
            baseColor = '#d97706'; // Amber for TransportRobot (TitanHauler)
            typeBadge = 'HAUL';
        } else if (isHybrid) {
            baseColor = '#7c3aed'; // Purple for HybridRobot (OmniBot)
            typeBadge = 'OMNI';
        }

        const rimColor = (robot.battery != null && robot.battery < 20) ? '#e11d48' : baseColor;

        // White circular body
        ctx.fillStyle = '#ffffff';
        ctx.strokeStyle = rimColor;
        ctx.lineWidth = isTransport ? 3.0 : 2.2;
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();

        // Distinct type inner accent
        if (isTransport) {
            ctx.strokeStyle = 'rgba(217, 119, 6, 0.35)';
            ctx.lineWidth = 1.2;
            const sq = r * 0.9;
            ctx.strokeRect(cx - sq / 2, cy - sq / 2, sq, sq);
        } else if (isHybrid) {
            ctx.strokeStyle = 'rgba(124, 58, 237, 0.35)';
            ctx.lineWidth = 1.2;
            ctx.beginPath();
            ctx.arc(cx, cy, r * 0.65, 0, Math.PI * 2);
            ctx.stroke();
        }

        // Battery Ring indicator (arc)
        if (r > 3) {
            const batteryAngle = ((robot.battery != null ? robot.battery : 100) / 100) * (Math.PI * 2);
            ctx.strokeStyle = robot.battery > 50 ? '#10b981' : (robot.battery > 20 ? '#f59e0b' : '#e11d48');
            ctx.lineWidth = 2.2;
            ctx.beginPath();
            ctx.arc(cx, cy, Math.max(1, r - 3), -Math.PI / 2, -Math.PI / 2 + batteryAngle);
            ctx.stroke();
        }

        // Robot ID text label
        ctx.fillStyle = '#0f172a';
        ctx.font = 'bold 10px JetBrains Mono';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(robot.id, cx, cy - 2);

        // Visual robot type badge below ID
        ctx.fillStyle = baseColor;
        ctx.font = 'bold 7px Outfit';
        ctx.fillText(typeBadge, cx, cy + r * 0.52);

        // Coordination Status Indicators (Waiting / Blocked)
        if (robot.status === 'WAITING') {
            // Amber safety halo
            ctx.strokeStyle = 'rgba(245, 158, 11, 0.45)';
            ctx.lineWidth = 4;
            ctx.beginPath();
            ctx.arc(cx, cy, r + 3, 0, Math.PI * 2);
            ctx.stroke();

            // Amber badge
            ctx.fillStyle = '#f59e0b';
            ctx.beginPath();
            ctx.arc(cx + r * 0.75, cy - r * 0.75, 6, 0, Math.PI * 2);
            ctx.fill();
            ctx.fillStyle = '#ffffff';
            ctx.font = 'bold 8px Outfit';
            ctx.fillText('W', cx + r * 0.75, cy - r * 0.75 + 1);
        } else if (robot.status === 'BLOCKED') {
            // Rose blocked alert halo
            ctx.strokeStyle = 'rgba(225, 29, 72, 0.55)';
            ctx.lineWidth = 4;
            ctx.beginPath();
            ctx.arc(cx, cy, r + 3, 0, Math.PI * 2);
            ctx.stroke();

            // Rose alert badge
            ctx.fillStyle = '#e11d48';
            ctx.beginPath();
            ctx.arc(cx + r * 0.75, cy - r * 0.75, 6, 0, Math.PI * 2);
            ctx.fill();
            ctx.fillStyle = '#ffffff';
            ctx.font = 'bold 8px Outfit';
            ctx.fillText('!', cx + r * 0.75, cy - r * 0.75 + 1);
        }

        // Parallel Order & Task Indicator Pill (above robot)
        let taskLabel = '';
        let taskColor = '#64748b';
        let taskBg = '#f1f5f9';

        if (robot.currentTask && robot.currentTask.orderId) {
            const oId = robot.currentTask.orderId;
            const tType = robot.currentTask.type || robot.status || '';
            let shortType = 'ACT';
            if (tType.includes('PICK')) shortType = 'PICK';
            else if (tType.includes('PACK') || tType.includes('DELIVER')) shortType = 'PACK';
            else if (tType.includes('HANDOFF')) shortType = 'TEAM';
            else if (tType.includes('CHARGE')) shortType = 'CHG';
            taskLabel = `O#${oId} ${shortType}`;
            taskColor = '#1d4ed8';
            taskBg = '#eff6ff';
        } else if (robot.status === 'CHARGING' || robot.status === 'RETURNING_TO_CHARGE') {
            taskLabel = 'CHG';
            taskColor = '#047857';
            taskBg = '#ecfdf5';
        } else if (robot.status === 'WAITING') {
            taskLabel = 'WAIT';
            taskColor = '#b45309';
            taskBg = '#fef3c7';
        } else if (robot.status === 'BLOCKED') {
            taskLabel = 'BLOCKED';
            taskColor = '#be123c';
            taskBg = '#ffe4e6';
        }

        if (taskLabel && size >= 20) {
            ctx.font = 'bold 8px JetBrains Mono';
            const badgeW = ctx.measureText(taskLabel).width + 6;
            const badgeH = 11;
            const badgeX = cx - badgeW / 2;
            const badgeY = cy - r - 8;

            ctx.fillStyle = taskBg;
            ctx.strokeStyle = taskColor;
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.rect(badgeX, badgeY, badgeW, badgeH);
            ctx.fill();
            ctx.stroke();

            ctx.fillStyle = taskColor;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(taskLabel, cx, badgeY + badgeH / 2 + 0.5);
        }
    }

    setHeatmapEnabled(enabled) {
        this.heatmapEnabled = !!enabled;
    }

    setHeatmapMode(mode) {
        if (mode === 'visits' || mode === 'waits' || mode === 'replans') {
            this.heatmapMode = mode;
        }
    }

    setSelectedCell(x, y) {
        if (x >= 0 && x < this.gridSize && y >= 0 && y < this.gridSize) {
            this.selectedCell = { x, y };
        } else {
            this.selectedCell = null;
        }
    }

    clearSelectedCell() {
        this.selectedCell = null;
    }

    getHeatmapColorVisits(ratio) {
        const alpha = Math.min(0.70, Math.max(0.18, 0.18 + ratio * 0.52)).toFixed(2);
        if (ratio <= 0.25) return `rgba(59, 130, 246, ${alpha})`; // Blue
        if (ratio <= 0.50) return `rgba(16, 185, 129, ${alpha})`; // Emerald
        if (ratio <= 0.75) return `rgba(245, 158, 11, ${alpha})`; // Amber
        return `rgba(225, 29, 72, ${alpha})`; // Crimson
    }

    getHeatmapColorWaits(ratio) {
        const alpha = Math.min(0.75, Math.max(0.22, 0.22 + ratio * 0.53)).toFixed(2);
        if (ratio <= 0.33) return `rgba(251, 191, 36, ${alpha})`; // Amber
        if (ratio <= 0.66) return `rgba(249, 115, 22, ${alpha})`; // Orange
        return `rgba(220, 38, 38, ${alpha})`; // Crimson red
    }

    getHeatmapColorReplans(ratio) {
        const alpha = Math.min(0.75, Math.max(0.22, 0.22 + ratio * 0.53)).toFixed(2);
        if (ratio <= 0.33) return `rgba(168, 85, 247, ${alpha})`; // Purple
        if (ratio <= 0.66) return `rgba(217, 70, 239, ${alpha})`; // Fuchsia
        return `rgba(190, 24, 93, ${alpha})`; // Deep Magenta
    }

    drawHeatmapOverlay() {
        if (!this.heatmapEnabled || !this.state || !this.state.analytics || !this.state.analytics.congestion) return;
        const ctx = this.ctx;
        const cs = this.cellSize;
        const cong = this.state.analytics.congestion;

        let matrix = null;
        let maxVal = 0;
        let colorFn = null;

        if (this.heatmapMode === 'waits') {
            matrix = cong.waits;
            maxVal = (cong.maxWaits !== undefined) ? cong.maxWaits : 0;
            colorFn = (ratio) => this.getHeatmapColorWaits(ratio);
        } else if (this.heatmapMode === 'replans') {
            matrix = cong.replans;
            maxVal = (cong.maxReplans !== undefined) ? cong.maxReplans : 0;
            colorFn = (ratio) => this.getHeatmapColorReplans(ratio);
        } else {
            matrix = cong.visits;
            maxVal = (cong.maxVisits !== undefined) ? cong.maxVisits : 0;
            colorFn = (ratio) => this.getHeatmapColorVisits(ratio);
        }

        if (!matrix || maxVal <= 0) return;

        for (let y = 0; y < this.gridSize; ++y) {
            if (!matrix[y]) continue;
            for (let x = 0; x < this.gridSize; ++x) {
                const val = matrix[y][x] || 0;
                if (val <= 0) continue;

                const ratio = val / maxVal;
                const px = x * cs;
                const py = y * cs;

                ctx.fillStyle = colorFn(ratio);
                ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);

                // For high intensity hotspots, draw the count badge if cell size permits
                if (ratio >= 0.70 && cs >= 32) {
                    ctx.fillStyle = 'rgba(255, 255, 255, 0.90)';
                    ctx.font = 'bold 9px JetBrains Mono';
                    ctx.textAlign = 'right';
                    ctx.textBaseline = 'bottom';
                    ctx.fillText(`${val}`, px + cs - 3, py + cs - 3);
                }
            }
        }
    }

    drawSelectedCell() {
        if (!this.selectedCell) return;
        const ctx = this.ctx;
        const cs = this.cellSize;
        const sx = this.selectedCell.x * cs;
        const sy = this.selectedCell.y * cs;

        // Solid inner selection ring
        ctx.strokeStyle = '#4f46e5';
        ctx.lineWidth = 2.4;
        ctx.strokeRect(sx + 1, sy + 1, cs - 2, cs - 2);

        // Soft outer halo
        ctx.strokeStyle = 'rgba(99, 102, 241, 0.45)';
        ctx.lineWidth = 4;
        ctx.strokeRect(sx - 1, sy - 1, cs + 2, cs + 2);
    }

    showTooltip(clientX, clientY, gx, gy) {
        if (!this.state) return;
        let content = `<strong>Cell (${gx}, ${gy})</strong>`;

        const robot = this.state.robots?.find(r => r.position.x === gx && r.position.y === gy);
        if (robot) {
            content += `<br><b>🤖 ${robot.name} (${robot.id})</b><br>Type: ${robot.type}<br>Status: <span style="font-weight:600;color:#2563eb;">${robot.status}</span><br>Battery: ${Math.round(robot.battery)}%`;
            if (robot.status === 'WAITING') {
                content += `<br><span style="color:#d97706;font-weight:600;">⏳ Waiting (${robot.waitTicks || 1} ticks)</span>`;
            } else if (robot.status === 'BLOCKED') {
                content += `<br><span style="color:#e11d48;font-weight:600;">⛔ Path Blocked — Replan Pending</span>`;
            }
            if (robot.currentTask) content += `<br>Task: ${robot.currentTask.type}`;
        } else {
            const isDynObstacle = this.state.warehouse?.dynamicObstacles?.some(o => o.x === gx && o.y === gy);
            if (isDynObstacle) {
                content += '<br>⚠️ <strong style="color:#ea580c;">Dynamic Obstacle</strong> (Click to remove)';
            } else {
                const cellType = this.state.warehouse.grid[gy][gx];
                if (cellType === 1) content += '<br>⚠️ <strong>Obstacle Pillar</strong> (Static impassable)';
                else if (cellType === 2) {
                    const shelf = this.state.warehouse.shelves?.find(s => s.position.x === gx && s.position.y === gy);
                    content += `<br>📦 <strong>Storage Shelf (${shelf ? shelf.id : ''})</strong>`;
                    if (this.state.inventory) {
                        const prod = this.state.inventory.find(p => p.location.x === gx && p.location.y === gy);
                        if (prod) {
                            content += `<br>Product: <strong>${prod.name}</strong><br>Stock: ${prod.quantity} units<br>Price: $${prod.price}`;
                        }
                    }
                } else if (cellType === 3) content += '<br>⚡ <strong>Charging Station</strong>';
                else if (cellType === 4) content += '<br>📦 <strong>Packing Station</strong>';
                else content += '<br>Clear Aisle Corridor (Click in Obstacle Mode to place)';
            }
        }

        if (this.heatmapEnabled && this.state.analytics && this.state.analytics.congestion) {
            const cong = this.state.analytics.congestion;
            const v = (cong.visits && cong.visits[gy]) ? (cong.visits[gy][gx] || 0) : 0;
            const w = (cong.waits && cong.waits[gy]) ? (cong.waits[gy][gx] || 0) : 0;
            const r = (cong.replans && cong.replans[gy]) ? (cong.replans[gy][gx] || 0) : 0;
            content += `<div style="margin-top:6px;padding-top:4px;border-top:1px solid #e2e8f0;font-size:11px;line-height:1.4;">
                <strong style="color:#4f46e5;">🔥 Congestion Observations:</strong><br>
                • Cell Visits: <strong>${v}</strong><br>
                • Waits / Blocked: <strong>${w}</strong><br>
                • Replans: <strong>${r}</strong>
            </div>`;
        }

        this.tooltip.innerHTML = content;
        this.tooltip.style.left = `${clientX + 14}px`;
        this.tooltip.style.top = `${clientY + 14}px`;
        this.tooltip.style.display = 'block';
    }

    hideTooltip() {
        this.tooltip.style.display = 'none';
    }
}
