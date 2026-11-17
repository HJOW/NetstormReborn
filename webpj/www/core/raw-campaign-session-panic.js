// 같은 Session의 지면·신전·난수와 실제 이동 경로로 사제 공황을 연결한다(225553df, WB04-E644).
import { RawPriestPanic } from './raw-priest-panic.js';

// 원본 CRT 변환이 정의된 signed 좌표만 0 방향으로 절삭한다.
function coordinate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('사제 panic 현재 좌표 CRT 범위');
    return Math.trunc(value) | 0;
}

// 실제 이동 설치 뒤 공황의 순환 참조를 연결하고 부모가 Host를 비운 뒤 반납한다.
export class RawCampaignSessionPanic {
    // 동일 owner와 설치 및 대여 수명이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 실제 하위 효과 표를 유지한다.
    hooks = null;
    // 구성만으로 월드 또는 난수를 수정하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납된 효과가 이전 월드에 접근하지 못하도록 한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 사제 panic'); }
    // 기존 Pool·이동·지형·속도표를 빌려 원문의 공황 본체를 한 번 설치한다.
    bind() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || !o.motionRuntimeModules?.ready || !o.motion || !o.carrierTarget || o.priestPanic) throw new Error('Session 사제 panic 설치 순서 오류');
        this.#started = true;
        const hooks = this.hooks = {
            temple: (x, y) => { this.#live(); return o.motion.priestArrival.templeAt(x, y); },
            maximum: (sid) => { this.#live(); return o.hp.maxHitPoints(sid); },
            land: (x, y) => { this.#live(); const cx = coordinate(x), cy = coordinate(y); if (cx < 0 || cy < 0 || cx >= 256 || cy >= 256) return false; const sid = c.hash.cell(0, cx, cy); return !!sid && c.pool.slot(sid)[10] === o.terrainType; },
            region: (x, y) => { this.#live(); const snap = (value) => coordinate(Math.fround(coordinate(value + Math.fround(0.99999)))); return o.motion.terrain.regionAt(snap(x), snap(y)); },
            random: (low, high) => { this.#live(); return o.random.next(65536) / 65536 * Math.abs(high - low) + low; },
            target: (sid, temple) => { this.#live(); return o.carrierTarget.startTarget(sid, temple, 0, 0); },
            point: (sid, x, y) => { this.#live(); return o.carrierRoutingModules.startMotion({ parent: sid, destination: { x, y }, replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 }); },
        };
        o.priestPanic = new RawPriestPanic(c.pool, o.dudeSpeeds, hooks);
        this.#ready = true;
    }
    // 실제 Host 정리 후 함수 대여만 닫는다.
    dispose() { this.#disposed = true; }
    // 이 부분의 설치 상태이며 전체 Load 완료는 아니다.
    get ready() { return this.#ready; }
}
