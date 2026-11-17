// 같은 Session의 건설 사제 이동에 원본 MakeBuilder 3544..3569를 연결한다(225553df).
import { typeNumber } from '../assets/type-table.js';
import { RawBoardApproach } from './raw-board-approach.js';
import { RawConstructionBuilder, makeConstructionBuilderHooks } from './raw-construction-builder.js';
import { RegularProcess } from './squid-process.js';

// 같은 월드·경로·예약을 빌리는 Builder 생성과 반환 수명이다.
export class RawCampaignSessionBuilder {
    // 실제 Session, 반납 상태와 SID별 안정된 경로 뷰다.
    #owner; #disposed = false; #paths = new Map();
    // 같은 미부착 예약과 최근 생성한 함수 표·현재 접근 mask다.
    pendingRegular = null; hooks = null; reads = null; approachMask = 0x50444200;

    // 생성 자체는 객체나 프로세스를 만들지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납 뒤 외부에 빌려준 함수가 이전 월드에 진입하지 못하게 한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 건설 사제'); }

    // 같은 TYPE·기준점·경로와 에너지 공급을 사용하는 Builder를 만든다.
    make(state, supplied = {}) {
        this.#live(); const o = this.#owner, c = o.core;
        if (!o.motion || !o.anchor || !o.energySupply) throw new Error('미설치 Session의 건설 운반 연결');
        const hooks = makeConstructionBuilderHooks(c.pool, o.anchor, c.factory, c.owner, supplied);
        // 공통 어댑터가 빌린 세 함수도 Session 반납 이후에는 접근하지 않는다.
        for (const name of ['anchor', 'create', 'setOwner']) {
            const borrowed = hooks[name]; hooks[name] = (...args) => { this.#live(); return borrowed(...args); };
        }
        hooks.pop = (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); };
        hooks.notifySurface = (sid) => { this.#live(); c.building.notifySurface(sid); };
        hooks.approach = (rectangle, x, y, flags) => {
            this.#live(); const approach = new RawBoardApproach((px, py) => o.anchor.graphAt(px, py), () => {
                if (typeof o.services.ai.assertion !== 'function') throw new Error('Builder Boardcoord.cpp 0x235 실제 진단장치 미연결');
                o.services.ai.assertion('0');
            });
            return approach.find([...rectangle], x, y, flags, () => o.gameplayGlobals.aiDebug().checking().value);
        };
        hooks.move = (sid, x, y) => {
            this.#live(); const path = o.motion.paths.startConstruction({ parent: sid, destination: { x, y }, replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 });
            if (!path) return null;
            const pid = c.kernel.find(path), view = this.#paths.get(sid) ?? {};
            view.readArrival = () => { this.#live(); if (c.kernel.get(pid) !== path) throw new Error('건설 운반의 실제 경로 수명 종료'); if (path.runtime.count < 1) throw new Error('건설 운반의 실제 경로 꼭짓점0'); return path.runtime.time(path.runtime.count - 1); };
            view.prepare = (speed, flags) => {
                this.#live(); if (flags || c.kernel.get(pid) !== path) throw new Error('건설 운반 Prepare의 실제 경로/flags 오류');
                const definition = o.assets.game.types()[c.pool.slot(sid)[10] - 70]?.definition;
                const base = definition && typeNumber(definition, 'speed'), turning = definition && typeNumber(definition, 'turningSpeed');
                if (base == null || turning == null) throw new Error('건설 운반 경로 TYPE 속도 누락');
                path.prepare(speed, Math.fround(base), Math.fround(turning));
            };
            this.#paths.set(sid, view); return view;
        };
        hooks.speed = (path, speed, flags) => { this.#live(); if (typeof path.prepare !== 'function') throw new Error('건설 운반의 실제 Prepare view 미연결'); path.prepare(speed, flags); };
        hooks.reserveTimer = () => { this.#live(); this.pendingRegular = new RegularProcess(0, Math.fround(0.05)); return true; };
        hooks.addTimer = (sid, event, payload) => {
            this.#live(); if (!this.pendingRegular || payload !== Math.fround(0.05)) throw new Error('건설 운반의 실제 예약 객체/payload 오류');
            const pending = this.pendingRegular; this.pendingRegular = null;
            const attached = c.processes.attachRegular(pending, sid); if (!attached) throw new Error('건설 운반의 실제 Regular 부착 실패'); attached.resetTask(event, c.processState.now);
        };
        const reads = {
            builderType: () => { this.#live(); return o.state.start.teleportType; }, carrierType: () => { this.#live(); return o.state.start.manaBoltType; },
            bridgeType: () => { this.#live(); return c.bridgeState.bridgeType; }, approachMask: () => { this.#live(); return this.approachMask; },
            timerPayload: () => { this.#live(); return Math.fround(0.05); }, now: () => { this.#live(); return c.processState.now; },
        };
        const builder = new RawConstructionBuilder(c.pool, c.types, o.builderTypes, state, hooks, o.energySupply, reads);
        this.hooks = hooks; this.reads = reads; return builder;
    }

    // 미부착 예약과 경로 뷰를 버리고 대여 함수를 닫는다.
    dispose() { this.#disposed = true; this.pendingRegular = null; this.#paths.clear(); }
}
