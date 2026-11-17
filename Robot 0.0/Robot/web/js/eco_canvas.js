/**
 * EcoRoute Energy-Aware Visualization Canvas Engine
 * Specialized canvas for comparing Shortest-Distance Baseline vs Lowest-Energy EcoRoute paths,
 * illustrating high-friction terrain, battery usage gradients, and charging station routes.
 */
class EcoRouteCanvas {
    constructor(canvasId, tooltipId) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            console.error(`[EcoRouteCanvas] Canvas element #${canvasId} not found.`);
            return;
        }
        this.ctx = this.canvas.getContext('2d');
        this.tooltip = document.getElementById(tooltipId);

        this.gridSize = 15;
        this.displaySize = 540;
        this.cellSize = this.displaySize / this.gridSize;

        // Warehouse fixtures
        this.shelves = [];
        this.chargingStations = [];
        this.packingStations = [];
        this.staticObstacles = [];
        this.customWeights = new Map(); // "x,y" -> cost (double)

        // Route state
        this.startPos = { x: 0, y: 0 };
        this.goalPos = { x: 14, y: 14 };
        this.shortestPath = [];
        this.ecoPath = [];
        this.shortestEnergy = 0.0;
        this.ecoEnergy = 0.0;

        // Interaction mode: 'start' | 'goal' | 'friction' | null
        this.interactionMode = null;
        this.onCoordinatesChange = null;

        // Robot step animation along ecoPath
        this.animStep = -1;
        this.animTimer = null;
        this.isPlaying = false;
        this.animSpeed = 150;

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

        this.ctx.setTransform(1, 0, 0, 1, 0, 0);
        this.ctx.scale(dpr, dpr);

        this.displaySize = size;
        this.cellSize = size / this.gridSize;
    }

    setFixtures(warehouse) {
        if (!warehouse) return;
        this.gridSize = warehouse.width || 15;
        this.shelves = (warehouse.shelves || []).map(s => ({ x: s.position?.x ?? s.x, y: s.position?.y ?? s.y }));
        this.chargingStations = (warehouse.chargingStations || []).map(cs => ({ id: cs.id, x: cs.position?.x ?? cs.x, y: cs.position?.y ?? cs.y }));
        this.packingStations = (warehouse.packingStations || []).map(ps => ({ id: ps.id, x: ps.position?.x ?? ps.x, y: ps.position?.y ?? ps.y }));
        this.staticObstacles = (warehouse.staticObstacles || []).map(o => ({ x: o.x, y: o.y }));

        this.initCanvas();
        this.render();
    }

    setCustomWeight(x, y, cost) {
        const key = `${x},${y}`;
        if (cost <= 1.0) {
            this.customWeights.delete(key);
        } else {
            this.customWeights.set(key, cost);
        }
        this.render();
    }

    clearCustomWeights() {
        this.customWeights.clear();
        this.render();
    }

    setCoordinates(start, goal) {
        if (start && start.x !== undefined) this.startPos = { x: start.x, y: start.y };
        if (goal && goal.x !== undefined) this.goalPos = { x: goal.x, y: goal.y };
        this.render();
    }

    setRoutes(shortestRoute, ecoRoute) {
        this.shortestPath = (shortestRoute && shortestRoute.found && Array.isArray(shortestRoute.path)) ? shortestRoute.path : [];
        this.ecoPath = (ecoRoute && ecoRoute.found && Array.isArray(ecoRoute.path)) ? ecoRoute.path : [];
        this.shortestEnergy = shortestRoute?.estimatedEnergy || 0.0;
        this.ecoEnergy = ecoRoute?.estimatedEnergy || 0.0;
        this.animStep = -1;
        this.render();
    }

    startAnimation() {
        if (!this.ecoPath || this.ecoPath.length === 0) return;
        this.stopAnimation();
        this.isPlaying = true;
        this.animStep = 0;
        this.render();

        this.animTimer = setInterval(() => {
            if (this.animStep < this.ecoPath.length - 1) {
                this.animStep++;
                this.render();
            } else {
                this.stopAnimation();
            }
        }, this.animSpeed);
    }

    stopAnimation() {
        if (this.animTimer) {
            clearInterval(this.animTimer);
            this.animTimer = null;
        }
        this.isPlaying = false;
    }

    resetAnimation() {
        this.stopAnimation();
        this.animStep = -1;
        this.render();
    }

    setupEvents() {
        window.addEventListener('resize', () => {
            this.initCanvas();
            this.render();
        });

        this.canvas.addEventListener('click', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const clickX = e.clientX - rect.left;
            const clickY = e.clientY - rect.top;
            const gx = Math.floor(clickX / this.cellSize);
            const gy = Math.floor(clickY / this.cellSize);

            if (gx < 0 || gx >= this.gridSize || gy < 0 || gy >= this.gridSize) return;

            if (this.interactionMode === 'start') {
                this.startPos = { x: gx, y: gy };
                if (this.onCoordinatesChange) this.onCoordinatesChange(this.startPos, this.goalPos);
                this.interactionMode = null;
                this.render();
            } else if (this.interactionMode === 'goal') {
                this.goalPos = { x: gx, y: gy };
                if (this.onCoordinatesChange) this.onCoordinatesChange(this.startPos, this.goalPos);
                this.interactionMode = null;
                this.render();
            } else if (this.interactionMode === 'friction') {
                const key = `${gx},${gy}`;
                const curr = this.customWeights.get(key) || 1.0;
                // Cycle weights: 1.0 -> 3.0 -> 5.0 -> 10.0 -> 1.0
                let next = 1.0;
                if (curr === 1.0) next = 3.0;
                else if (curr === 3.0) next = 5.0;
                else if (curr === 5.0) next = 10.0;
                else next = 1.0;
                this.setCustomWeight(gx, gy, next);
            }
        });

        this.canvas.addEventListener('mousemove', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const mouseX = e.clientX - rect.left;
            const mouseY = e.clientY - rect.top;
            const gx = Math.floor(mouseX / this.cellSize);
            const gy = Math.floor(mouseY / this.cellSize);

            if (gx >= 0 && gx < this.gridSize && gy >= 0 && gy < this.gridSize) {
                this.showTooltip(e.clientX, e.clientY, gx, gy);
            } else {
                this.hideTooltip();
            }
        });

        this.canvas.addEventListener('mouseleave', () => {
            this.hideTooltip();
        });
    }

    showTooltip(clientX, clientY, gx, gy) {
        if (!this.tooltip) return;
        const key = `${gx},${gy}`;
        const cost = this.customWeights.get(key) || 1.0;
        let cellType = "Floor Cell";
        if (this.shelves.some(s => s.x === gx && s.y === gy)) cellType = "Storage Shelf";
        else if (this.chargingStations.some(cs => cs.x === gx && cs.y === gy)) cellType = "Charging Station";
        else if (this.packingStations.some(ps => ps.x === gx && ps.y === gy)) cellType = "Packing Station";
        else if (this.staticObstacles.some(o => o.x === gx && o.y === gy)) cellType = "Wall / Obstacle";

        const onShortest = this.shortestPath.some(p => p.x === gx && p.y === gy);
        const onEco = this.ecoPath.some(p => p.x === gx && p.y === gy);

        let pathBadge = "";
        if (onShortest && onEco) pathBadge = "<span style='color:#10b981;font-weight:700;'>[Shared Segment]</span>";
        else if (onShortest) pathBadge = "<span style='color:#06b6d4;font-weight:700;'>[Shortest Route]</span>";
        else if (onEco) pathBadge = "<span style='color:#10b981;font-weight:700;'>[EcoRoute Segment]</span>";

        this.tooltip.innerHTML = `
            <div style="font-weight:700;margin-bottom:4px;color:var(--text-primary);">(${gx}, ${gy}) ${cellType}</div>
            <div style="font-size:11px;color:var(--text-secondary);">Terrain Friction: <b style="color:${cost > 1 ? '#f59e0b' : 'inherit'};">${cost.toFixed(1)}x</b></div>
            ${pathBadge ? `<div style="font-size:11px;margin-top:2px;">${pathBadge}</div>` : ''}
        `;
        this.tooltip.style.display = 'block';
        this.tooltip.style.left = `${clientX + 14}px`;
        this.tooltip.style.top = `${clientY + 14}px`;
    }

    hideTooltip() {
        if (this.tooltip) {
            this.tooltip.style.display = 'none';
        }
    }

    render() {
        if (!this.ctx) return;
        const ctx = this.ctx;
        const cs = this.cellSize;
        const total = this.gridSize * cs;

        // Background
        ctx.fillStyle = '#0f172a';
        ctx.fillRect(0, 0, total, total);

        // 1. Grid Cells & Custom Friction Terrain
        for (let x = 0; x < this.gridSize; ++x) {
            for (let y = 0; y < this.gridSize; ++y) {
                const px = x * cs;
                const py = y * cs;
                const key = `${x},${y}`;
                const weight = this.customWeights.get(key) || 1.0;

                if (weight > 1.0) {
                    // High friction terrain
                    ctx.fillStyle = `rgba(245, 158, 11, ${Math.min(0.65, 0.15 + weight * 0.05)})`;
                    ctx.fillRect(px, py, cs, cs);
                } else {
                    ctx.fillStyle = '#1e293b';
                    ctx.fillRect(px, py, cs, cs);
                }

                // Grid lines
                ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
                ctx.lineWidth = 1;
                ctx.strokeRect(px, py, cs, cs);

                if (weight > 1.0) {
                    ctx.fillStyle = '#f59e0b';
                    ctx.font = `bold ${Math.max(9, Math.floor(cs * 0.28))}px monospace`;
                    ctx.textAlign = 'center';
                    ctx.textBaseline = 'middle';
                    ctx.fillText(`${weight.toFixed(0)}x`, px + cs / 2, py + cs / 2);
                }
            }
        }

        // 2. Fixtures: Shelves
        this.shelves.forEach(s => {
            const px = s.x * cs;
            const py = s.y * cs;
            ctx.fillStyle = '#334155';
            ctx.fillRect(px + 2, py + 2, cs - 4, cs - 4);
            ctx.strokeStyle = '#475569';
            ctx.lineWidth = 1.5;
            ctx.strokeRect(px + 2, py + 2, cs - 4, cs - 4);
            ctx.fillStyle = '#94a3b8';
            ctx.font = `${Math.floor(cs * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('📦', px + cs / 2, py + cs / 2);
        });

        // 3. Fixtures: Packing Stations
        this.packingStations.forEach(ps => {
            const px = ps.x * cs;
            const py = ps.y * cs;
            ctx.fillStyle = 'rgba(168, 85, 247, 0.25)';
            ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.strokeStyle = '#a855f7';
            ctx.lineWidth = 2;
            ctx.strokeRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.fillStyle = '#c084fc';
            ctx.font = `${Math.floor(cs * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('📥', px + cs / 2, py + cs / 2);
        });

        // 4. Fixtures: Charging Stations
        this.chargingStations.forEach(csStat => {
            const px = csStat.x * cs;
            const py = csStat.y * cs;
            ctx.fillStyle = 'rgba(16, 185, 129, 0.25)';
            ctx.fillRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.strokeStyle = '#10b981';
            ctx.lineWidth = 2;
            ctx.strokeRect(px + 1, py + 1, cs - 2, cs - 2);
            ctx.fillStyle = '#34d399';
            ctx.font = `${Math.floor(cs * 0.45)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('⚡', px + cs / 2, py + cs / 2);
        });

        // 5. Shortest Distance Route (Cyan Dashed Line)
        if (this.shortestPath && this.shortestPath.length > 1) {
            ctx.beginPath();
            ctx.setLineDash([6, 5]);
            ctx.strokeStyle = '#06b6d4';
            ctx.lineWidth = 3;
            for (let i = 0; i < this.shortestPath.length; ++i) {
                const pt = this.shortestPath[i];
                const cx = pt.x * cs + cs / 2;
                const cy = pt.y * cs + cs / 2;
                if (i === 0) ctx.moveTo(cx, cy);
                else ctx.lineTo(cx, cy);
            }
            ctx.stroke();
            ctx.setLineDash([]);

            // Shortest route node markers
            ctx.fillStyle = '#0891b2';
            for (let i = 1; i < this.shortestPath.length - 1; ++i) {
                const pt = this.shortestPath[i];
                ctx.beginPath();
                ctx.arc(pt.x * cs + cs / 2, pt.y * cs + cs / 2, 3, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        // 6. Lowest-Energy EcoRoute (Solid Emerald Glowing Line)
        if (this.ecoPath && this.ecoPath.length > 1) {
            ctx.save();
            ctx.shadowColor = '#10b981';
            ctx.shadowBlur = 10;
            ctx.beginPath();
            ctx.strokeStyle = '#10b981';
            ctx.lineWidth = 4;
            for (let i = 0; i < this.ecoPath.length; ++i) {
                const pt = this.ecoPath[i];
                const cx = pt.x * cs + cs / 2;
                const cy = pt.y * cs + cs / 2;
                if (i === 0) ctx.moveTo(cx, cy);
                else ctx.lineTo(cx, cy);
            }
            ctx.stroke();
            ctx.restore();

            // EcoRoute step node circles
            ctx.fillStyle = '#34d399';
            for (let i = 1; i < this.ecoPath.length - 1; ++i) {
                const pt = this.ecoPath[i];
                ctx.beginPath();
                ctx.arc(pt.x * cs + cs / 2, pt.y * cs + cs / 2, 3.5, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        // 7. Animated Robot Marker along EcoRoute
        if (this.animStep >= 0 && this.animStep < this.ecoPath.length) {
            const cur = this.ecoPath[this.animStep];
            const cx = cur.x * cs + cs / 2;
            const cy = cur.y * cs + cs / 2;

            ctx.save();
            ctx.shadowColor = '#38bdf8';
            ctx.shadowBlur = 12;
            ctx.fillStyle = '#0284c7';
            ctx.beginPath();
            ctx.arc(cx, cy, cs * 0.35, 0, Math.PI * 2);
            ctx.fill();
            ctx.strokeStyle = '#38bdf8';
            ctx.lineWidth = 2.5;
            ctx.stroke();

            ctx.fillStyle = '#ffffff';
            ctx.font = `${Math.floor(cs * 0.4)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('🤖', cx, cy);
            ctx.restore();
        }

        // 8. Start Indicator (Green Ring)
        if (this.startPos) {
            const sx = this.startPos.x * cs + cs / 2;
            const sy = this.startPos.y * cs + cs / 2;
            ctx.save();
            ctx.shadowColor = '#10b981';
            ctx.shadowBlur = 10;
            ctx.beginPath();
            ctx.arc(sx, sy, cs * 0.42, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(16, 185, 129, 0.3)';
            ctx.fill();
            ctx.lineWidth = 3;
            ctx.strokeStyle = '#10b981';
            ctx.stroke();

            ctx.fillStyle = '#ffffff';
            ctx.font = `bold ${Math.floor(cs * 0.4)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('S', sx, sy);
            ctx.restore();
        }

        // 9. Goal Indicator (Amber/Red Ring)
        if (this.goalPos) {
            const gx = this.goalPos.x * cs + cs / 2;
            const gy = this.goalPos.y * cs + cs / 2;
            ctx.save();
            ctx.shadowColor = '#ef4444';
            ctx.shadowBlur = 10;
            ctx.beginPath();
            ctx.arc(gx, gy, cs * 0.42, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(239, 68, 68, 0.3)';
            ctx.fill();
            ctx.lineWidth = 3;
            ctx.strokeStyle = '#ef4444';
            ctx.stroke();

            ctx.fillStyle = '#ffffff';
            ctx.font = `bold ${Math.floor(cs * 0.4)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('G', gx, gy);
            ctx.restore();
        }
    }
}
window.EcoRouteCanvas = EcoRouteCanvas;
