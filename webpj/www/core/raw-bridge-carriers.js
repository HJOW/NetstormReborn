// 위험한 다리 위 사제/rainWalker 의 현재 이동 조회(421c30 이후 4202f0)와 같은 Graph 의 가까운 표면 대피(427e00)를 옮긴 모듈이다(cpppj `o/RawBridgeCarriers.cpp/.h`, 10.78 전용).
// 큰 이동/Graph/진단 효과만 훅으로 받고 후보 순서와 선택은 원문의 루프에서 계산한다. 자체 finder 두 개의 수명을 분리해 대피 중에도 바깥 칸 위 순회를 보존한다.
//  - check(bridge): 그 다리 칸에 놓인 객체를 일반 finder 로 훑어 사제/rainWalker 두 TYPE 만 현재 이동 Form 이 없을 때 대피시킨다.
//  - retreat(parent): 권한(boss)과 Graph 가 있을 때 반경 5 의 후보 중 정상 표면·같은 Graph·거리 1 이상 최소(동률은 앞 후보 유지)로 실제 이동을 시작한다(검증 실패 시 시작하지 않음).
// 훅 { graphFor(sid) → 번호(254=없음), movementType(type) → 종류, movementForm(sid, kind) → sid(0=없음), start(sid, {x,y}) → 시작 여부, valid({x,y}) → 현재 mapLimit 검사, assertion(줄),
//       wontWalkOffEdge(type) → 진단 관문 비교, scan(area) → 선택 관측 }. 모든 현재 전역(boss·priest·rainWalker·avoidLife)은 값 getter 로 받아 조회 시점마다 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E46: RawBridgeCarriers 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯 필드 위치(수명 단어 하위 바이트·타입·좌표)와 타입 수이다. (2026-11-08 WB04-E46)
const WORD_OFFSET = 12;
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const TYPE_COUNT = 188;

// 타입 flags: 표면(0x800), bridge genus(4), 이동체 genus 비트(0x30000)와 수명 비트 마스크이다. (2026-11-08 WB04-E46)
const SURFACE = 0x800;
const GENUS_BRIDGE = 4;
const CARRIER_GENUS = 0x30000;
const LIFE_MASK = 0x78;

// 진단 줄 번호와 대피 후보 반경, 초기 최소 거리, 후보가 없을 때의 선택점이다. (2026-11-08 WB04-E46)
const ASSERT_CARRIER = 0x2d8;
const ASSERT_WALK = 0x893;
const ASSERT_START = 0x8b0;
const RADIUS = 5;
const INITIAL_BEST = 100;
const NO_POINT = Object.freeze({ x: -10000, y: -10000 });

// 정렬되지 않은 DWORD 와 float 읽기, CRT _ftol(하위 DWORD, 범위 밖 0)이다. (2026-11-08 WB04-E46)
const floatView = new DataView(new ArrayBuffer(4));
function floatOf(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}
function ftol(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0;
    return Math.trunc(value) | 0;
}

// 반경 finder 는 먼저 거의 올림(0.99999f)→정수→float 저장을 거친다. 한 칸 finder 의 단순 절삭과 구별한다. (2026-11-08 WB04-E46)
const centerOf = (value) => ftol(Math.fround(ftol(value + Math.fround(0.99999))));

// 도달한 실제 효과가 없으면 앞선 실제 변경을 유지한 채 오류를 전달한다. (2026-11-08 WB04-E46)
function need(device) {
    if (typeof device !== 'function') throw new Error('다리 이동체 대피의 실제 효과 미연결');
    return device;
}

// 위험한 다리 위 이동체 대피 실행기이다. 사용: new RawBridgeCarriers(pool, hash, types, () => boss, () => checking, () => priestType, () => rainWalkerType, () => avoidLife, hooks).
export class RawBridgeCarriers {
    #pool;
    #hash;
    #types;
    #boss;
    #checking;
    #priest;
    #rainWalker;
    #avoidLife;
    #hooks;

    // 판본/타입 크기를 검증하며 구성 자체는 현재 전역/슬롯을 쓰지 않는다.
    constructor(pool, hash, types, boss, checking, priest, rainWalker, avoidLife, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT) throw new TypeError('다리 이동체 대피의 PATCH 풀/TYPE 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#boss = boss;
        this.#checking = checking;
        this.#priest = priest;
        this.#rainWalker = rainWalker;
        this.#avoidLife = avoidLife;
        this.#hooks = hooks;
    }

    // 현재 위치는 함수 진입 snapshot 으로 고정하지 않고 읽을 때마다 현재 float 두 개를 읽는다.
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return { x: floatOf(raw, X_OFFSET), y: floatOf(raw, Y_OFFSET) };
    }

    // 현재 BYTE TYPE 를 같은 물리 표에서 조회한다. 범위 밖은 해당 스캔 칸의 예외 경계로 전달한다.
    #type(sid) {
        const type = this.#pool.slot(sid)[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('다리 이동체 대피의 현재 TYPE 범위');
        return this.#types[type];
    }

    // 원문 포인터 동일성만 필요하므로 254 는 0(NULL), 나머지 번호는 +1 로 구별한다.
    #graph(sid) {
        const number = need(this.#hooks.graphFor)(sid);
        return number === 254 ? 0 : number + 1;
    }

    // 422510: bridge genus·비영 수명·현재 7 경계를 읽는다. 일반 surface 도 수명이 0 이면 정상이며 다리가 아닌 후보는 이 조건으로 제외하지 않는다.
    #dangerous(sid) {
        if (!sid || !(this.#type(sid).flags2 & GENUS_BRIDGE)) return false;
        const life = (this.#pool.slot(sid)[WORD_OFFSET] & LIFE_MASK) >> 3;
        return life !== 0 && life < this.#avoidLife();
    }

    // 실제 4202f0(flags 0) 순회: 두 TYPE(사제·rainWalker)만 현재 이동이 없을 때 대피한다. 기존 이동 Form 은 종류 선택 뒤 같은 현재 내용물에서 조회한다.
    check(bridge) {
        const hooks = this.#hooks;
        const point = this.#position(bridge);
        const x = ftol(point.x);
        const y = ftol(point.y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const area = { left: x, top: y, right: x, bottom: y };
        if (hooks.scan) hooks.scan(area);
        // 바깥 finder 는 중첩 대피의 별도 finder 와 독립적으로 다음 링크를 유지한다.
        for (let sid = finder.begin(area, 0); sid !== 0; sid = finder.next()) {
            const kind = this.#pool.slot(sid)[TYPE_OFFSET];
            if (kind !== this.#priest() && kind !== this.#rainWalker()) continue;
            if (!(this.#type(sid).flags2 & CARRIER_GENUS)) need(hooks.assertion)(ASSERT_CARRIER);
            const movement = need(hooks.movementType)(this.#pool.slot(sid)[TYPE_OFFSET]);
            if (!need(hooks.movementForm)(sid, movement)) this.retreat(sid);
        }
    }

    // 427e00: 권한/Graph → 반경 5 → 정상 표면/같은 Graph/거리 → 실제 이동 순서이다. 선택한 최소 거리만 float 로 저장하고 동률은 앞서 선택한 현재 후보를 유지한다.
    retreat(parent) {
        const hooks = this.#hooks;
        if (!this.#boss() || !this.#graph(parent)) return;
        const kind = this.#pool.slot(parent)[TYPE_OFFSET];
        if (this.#checking() && !need(hooks.wontWalkOffEdge)(kind)) need(hooks.assertion)(ASSERT_WALK);
        let best = INITIAL_BEST;
        let chosen = NO_POINT;
        const point = this.#position(parent);
        const x = centerOf(point.x);
        const y = centerOf(point.y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const area = { left: (x - RADIUS) | 0, top: (y - RADIUS) | 0, right: (x + RADIUS) | 0, bottom: (y + RADIUS) | 0 };
        if (hooks.scan) hooks.scan(area);
        // 현재 표면·수명·Graph 조건을 통과한 순서대로 거리를 비교한다.
        for (let sid = finder.begin(area, 0); sid !== 0; sid = finder.next()) {
            if (!(this.#type(sid).flags1 & SURFACE) || this.#dangerous(sid)) continue;
            const mine = this.#graph(parent);
            const other = this.#graph(sid);
            if (mine !== other) continue;
            const destination = this.#position(sid);
            const origin = this.#position(parent);
            const dx = destination.x - origin.x;
            const dy = destination.y - origin.y;
            const distance = Math.sqrt(dy * dy + dx * dx);
            // 원문의 비교는 하한과 같아도 채택한다.
            if (distance >= 1 && distance < best) {
                best = Math.fround(distance);
                chosen = this.#position(sid);
            }
        }
        if (need(hooks.valid)(chosen) && !need(hooks.start)(parent, chosen) && this.#checking()) need(hooks.assertion)(ASSERT_START);
    }
}
