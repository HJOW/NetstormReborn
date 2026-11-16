// 생성자1670/ConnectFence1911..1965의 같은 현재 펜스/공간/예약 결선이다(225553df, WB04-E612).
// 사용: Flyer 조립 뒤 bind. 실제 World 분배/게시·첫 저장 객체는 상위 Session에서 이어간다.
import { RawFenceNeighbors } from './raw-fence-neighbors.js';
import { RawFenceLine } from './raw-fence-line.js';
import { RawFencePostPop } from './raw-fence-post-pop.js';
import { RawFenceMarkerCreate, RawFenceShieldCreate } from './raw-fence-create.js';
import { RawFenceRebuild } from './raw-fence-rebuild.js';
import { RawFenceRegular } from './raw-fence-regular.js';
import { RawFenceMarkerClear } from './raw-fence-marker-clear.js';
import { RawFenceLifetime } from './raw-fence-lifetime.js';
import { RegularProcess } from './squid-process.js';

// 같은 World/현재 TYPE·동맹과 첫 Regular 예약을 기존 아홉 펜스 몸체에 대여한다.
export class RawCampaignSessionFence {
    // 부분 실패/반복/반납과 생성 구간의 정상 복귀다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 std::function 값형 훅과 매 조회 현재 TYPE를 읽는188칸 사거리 뷰다.
    ranges = null; postHooks = null; createHooks = null; rebuildHooks = null; regularHooks = null; lifetimeHooks = null;
    // 여러 소비자가 공유하는 같은 예약/부착 함수와 현재 연결선·표식 TYPE 조회기다.
    reserveRegular = null; addRegular = null; shieldType = null; markerType = null;

    // 생성 자체는 슬롯/예약/난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문 순서로 최근접·연결선→후처리→생성/재생성→Regular→수명을 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.flyerModules?.ready || this.#started || o.fencePostPop) throw new Error('Session Fence의 초기화 전/반복 결선'); this.#started = true;
        this.reserveRegular = () => { this.#live(); o.fencePendingRegular = new RegularProcess(0, 0); return true; }; // 원문 unique_ptr 대입처럼 앞선 미부착 예약을 교체한다.
        this.addRegular = (sid, event, payload) => { // 같은 확보에서 생성/부착한 뒤 현재 시각의 event/count0으로 재설정한다.
            this.#live(); const reserved = o.fencePendingRegular; if (!reserved) throw new Error('펜스 실제 예약 객체 누락'); o.fencePendingRegular = null; reserved.initializeDetached(event, payload);
            const process = c.processes.attachRegular(reserved, sid); if (!process) throw new Error('펜스 Regular 부모 부착 실패'); process.resetTask(event, c.processState.now);
        };
        this.ranges = new Array(c.types.length);
        // C++ range getter와 같이 같은 현재 TYPE의 값을 소비 위치마다 읽는다. 수치 사본을 만들지 않는다.
        for (let type = 0; type < c.types.length; type += 1) Object.defineProperty(this.ranges, type, { enumerable: true, get: () => { this.#live(); return c.types[type].range; } });
        this.shieldType = () => { this.#live(); return o.fenceShieldType; }; this.markerType = () => { this.#live(); return o.fenceNodeType; };
        o.fenceNeighbors = new RawFenceNeighbors(c.pool, c.hash, c.types, this.ranges, o.alliances);
        o.fenceLine = new RawFenceLine(c.pool, c.hash, c.types, c.frames, this.shieldType, (sid, flags) => { this.#live(); o.world.destroy(sid, flags); });
        const hooks = {
            theme: (sid) => { this.#live(); const type = c.pool.slot(sid)[10]; if (type >= o.themes.length) throw new RangeError('펜스 현재 theme TYPE 범위'); return o.themes[type]; }, // 원문 themes.at의 같은 번호다.
            neighbor: (sid, direction) => { this.#live(); return o.fenceNeighbors.neighbor(sid, direction); }, // 실제 현재 공간이다.
            reserveRegular: this.reserveRegular, addRegular: this.addRegular, // 같은 두 함수 값이다.
            clearLine: (points) => { this.#live(); o.fenceLine.clear(points); }, connections: (sid) => { this.#live(); return o.fenceNeighbors.connections(sid); }, // 같은 동맹/선 정리다.
            validateBase: (sid, flags, projected) => { this.#live(); c.postPop.validateBase(sid, flags, projected); }, damageable: (sid, flags) => { this.#live(); o.carrier.damageablePostPop(sid, flags); }, // 실제 공통 사전/후처리다.
        };
        this.postHooks = hooks; o.fencePostPop = new RawFencePostPop(c.pool, hooks);
        const create = {
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, notifySurface: (sid) => { this.#live(); c.building.notifySurface(sid); }, // 같은 가상 생성과 현재 표면 통지다.
            setOwner: (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner); }, pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, // 현재 역할의 실제 소유권 가상74·Pop이다.
        };
        this.createHooks = create; o.fenceMarkerCreate = new RawFenceMarkerCreate(c.pool, o.gunTarget, this.markerType, { ...create }); o.fenceShieldCreate = new RawFenceShieldCreate(c.pool, c.frames, this.shieldType, { ...create });
        const rebuild = {
            neighbor: (sid, direction) => { this.#live(); return o.fenceNeighbors.neighbor(sid, direction, true); }, withinRange: (sid, points) => { this.#live(); return o.fenceNeighbors.withinRange(sid, points); }, // 재생성은 동맹 최근접이다.
            markers: (sid, other) => { this.#live(); o.fenceMarkerCreate.create(sid, other); }, shields: (points, a, b, first, second) => { this.#live(); o.fenceShieldCreate.create(points, a, b, first, second); }, // 같은 실제 두 생성기다.
        };
        this.rebuildHooks = rebuild; o.fenceRebuild = new RawFenceRebuild(c.pool, rebuild);
        const regular = {
            neighbor: (sid, direction) => { this.#live(); return o.fenceNeighbors.neighbor(sid, direction); }, // Regular 정리는 비동맹 제한 없는 첫 이웃이다.
        };
        o.fenceMarkerClear = new RawFenceMarkerClear(c.pool, c.hash, c.types, o.gunTarget, this.markerType, (sid, flags) => { this.#live(); o.world.destroy(sid, flags); });
        regular.clearMarkers = (sid, other) => { this.#live(); o.fenceMarkerClear.clear(sid, other); }; // 실제 표식 삭제다.
        regular.clearLine = (points) => { this.#live(); o.fenceLine.clear(points); }; regular.connections = (sid) => { this.#live(); return o.fenceNeighbors.connections(sid); }; // 같은 연결선/현재 연결 수다.
        regular.reserveRegular = this.reserveRegular; regular.addRegular = this.addRegular; // 후처리·수명과 같은 미부착 예약을 공유한다.
        regular.refreshNeighbors = (sid) => { this.#live(); o.fenceLifetime.refresh(sid); }; regular.rebuild = (sid) => { this.#live(); o.fenceRebuild.rebuild(sid); }; // 뒤에 설치하는 같은 수명도 호출 시 선택한다.
        regular.damageable = (sid, event, count, payload) => { this.#live(); return o.assetBindings.regular.damageableHandle(sid, event, count, payload); }; // 같은 공통 Regular 반환이다.
        this.regularHooks = regular; o.fenceRegular = new RawFenceRegular(c.pool, c.frames, c.frame, regular);
        const lifetime = {
            neighbor: (sid, direction, allied) => { this.#live(); return o.fenceNeighbors.neighbor(sid, direction, allied); }, clearMarkers: regular.clearMarkers, // 현재 요청한 동맹 관문이다.
            clearLine: (points, a, b, first, second) => { this.#live(); o.fenceLine.clear(points); }, // 원문 mode0은 owner/SID 인수를 쓰지 않는다.
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, repop: (sid, flags) => { this.#live(); o.world.repop(sid, flags); }, rebuild: regular.rebuild, // 같은 가상 공간/재생성이다.
            reserveRegular: this.reserveRegular, addRegular: this.addRegular, damageable: (sid, flags) => { this.#live(); o.damageablePre.preDestroy(sid, flags); }, // 현재 실제 피해 전처리다.
        };
        this.lifetimeHooks = lifetime; o.fenceLifetime = new RawFenceLifetime(c.pool, lifetime); this.#ready = true;
    }

    // 반납한 Session의 효과 closure가 이전 World를 재사용하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Fence'); }
    // 실제 Host를 비운 상위가 같은 미부착 예약과 대여 수명을 닫는다.
    dispose() { this.#disposed = true; this.#owner.fencePendingRegular = null; }
    // 내부 조립의 정상 복귀이며 World 게시/Load의 완료는 아니다.
    get ready() { return this.#ready; }
}
