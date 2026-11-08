// 패턴 없는 사제 CanonDecoder 의 실제 한 칸 순회/미리보기 범위/충돌 사각형을 연결한 모듈이다(cpppj `o/RawPriestPlacementGeometry.cpp/.h`, 10.78). 사제 배치의 비패턴 계약과 E146 일반 모양 계산기를 잇는 얇은 어댑터이다.
//  - new RawPriestPlacementGeometry(pool, types, frames, state, hooks): types(188칸)·frames(타입별 { frames, defaultFrame })·state({ patternTypes[8], decoderDiagnostics })의 수명은 호출자가 보장한다.
//    hooks = { beginRegions(query), endShape(query, frame, snappedX, snappedY) → bool, finishRegions(query) → bool, beginShape?(query, frame, snappedX, snappedY) } 앞의 셋은 필수이다(미복원 지형 판단을 임의로 허용하지 않도록 필수 경계로 둔다).
//    query = { type, x, y, flags, owner, mode } 이고 현재 사제 genus(0x200000)이며 현재 패턴 타입 전역이 아니어야 한다(비패턴 계약).
//  - bounds(query): 첫 인자는 원본 배치처럼 타입 번호이며 명시 프레임을 켜지 않는다. 실제 비패턴 생성과 현재 발자국으로 미리보기 범위를 구한다(frame −1 도 계산한다).
//  - inspect(query, scan): 요청을 값으로 캡처한 뒤 일반 모양 순회에 위임한다. decoder 생성 뒤 beginRegions → 유효 칸마다 [발자국 양수 확인 → (선택) beginShape → scan(finder 사각형) → endShape] → 모든 칸이 끝나면 finishRegions 이다.
//    scan 이 거짓이거나 endShape 가 거짓이면 즉시 거짓이고 진행/후처리를 호출하지 않는다.
//  - RawPriestPlacementGeometry.collisionArea(x, y, footX, footY): 0049b8c0..0049b95a 의 finder 인자 산술이며 사제의 양수 발자국 계약(1 미만 거부)을 유지한다.
//  - makePriestGeometryPreviewHooks(pool, geometry, hooks): 미리보기 bounds 만 실제 비패턴 decoder 에 연결한다. makePriestGeometryCollisionHooks(pool, geometry, hooks): 충돌 검사기의 모양 순회(inspectRegions)를 실제 decoder 로 연결한다.
// 10.72(CD)의 짝수 회전/좌표 저장 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E156: 사제 배치 기하 이식으로 추가)
import { RawCanonPlacementGeometry, canonCollisionArea } from './raw-canon-placement-geometry.js';

// 사제 genus 비트와 타입 표 크기이다. (2026-11-09 WB04-E156)
const GENUS_PRIEST = 0x200000;
const TYPE_COUNT = 188;

// 사제 배치의 비패턴 기하 계층이다. 각 단계의 지역 효과를 필수 경계로 두어 미복원 지형 판단을 임의로 허용하지 않는다. (2026-11-09 WB04-E156)
export class RawPriestPlacementGeometry {
    #pool;
    #types;
    #frames;
    #state;
    #hooks;

    // 타입/프레임 표 크기와 세 필수 지역 경계를 확인한다. (2026-11-09 WB04-E156)
    constructor(pool, types, frames, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 모양은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== types.length || !state
            || typeof hooks?.beginRegions !== 'function' || typeof hooks.endShape !== 'function' || typeof hooks.finishRegions !== 'function') {
            throw new TypeError('사제 모양 자료/지역 경계 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = { beginRegions: hooks.beginRegions, endShape: hooks.endShape, finishRegions: hooks.finishRegions, beginShape: hooks.beginShape };
    }

    // 어댑터는 같은 풀/판본에만 연결하도록 원래 풀을 제공한다. (2026-11-09 WB04-E156)
    get pool() { return this.#pool; }

    // 현재 사제 genus 와 비패턴 계약은 일반 타입 계산기에 위임하기 전에 검사한다. (2026-11-09 WB04-E156)
    #check(query) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length || (this.#types[query.type].flags2 & GENUS_PRIEST) === 0 || this.#state.patternTypes.includes(query.type)) {
            throw new RangeError('사제 비패턴 CanonDecoder 타입 계약 오류');
        }
    }

    // 일반 모양 계산기에 넘기는 질의이다. 사제의 첫 인자는 타입 번호이며 명시 프레임을 켜지 않는다. (2026-11-09 WB04-E156)
    static #canonQuery(query) {
        return { type: query.type, argument: query.type, direction: query.flags | 0, x: query.x, y: query.y, explicitFrame: false };
    }

    // 실제 비패턴 생성/진행과 현재 타입 발자국으로 미리보기 범위를 계산한다. frame −1 도 계산한다. (2026-11-09 WB04-E156)
    bounds(query) {
        this.#check(query);
        return new RawCanonPlacementGeometry(this.#pool, this.#types, this.#frames, this.#state).bounds(RawPriestPlacementGeometry.#canonQuery(query));
    }

    // 0049b8c0..0049b95a: 사제와 일반 패턴은 같은 실제 finder 사각형 산술을 사용하되 사제의 양수 발자국 계약을 유지한다. (2026-11-09 WB04-E156)
    static collisionArea(x, y, footX, footY) {
        if (footX < 1 || footY < 1) throw new RangeError('사제 모양 발자국 오류');
        return canonCollisionArea(x, y, footX, footY);
    }

    // 실제 한 칸 순회와 모양별 정수 사각형을 scan 에 공급하며 거부 이후 진행/후처리를 호출하지 않는다. 요청은 값으로 캡처한다. (2026-11-09 WB04-E156)
    inspect(query, scan) {
        if (typeof scan !== 'function') throw new TypeError('사제 모양 충돌 탐색 경계 누락');
        this.#check(query);
        query = { ...query };
        const hooks = this.#hooks;
        const types = this.#types;
        const canonHooks = {
            beginRegions: () => { hooks.beginRegions({ ...query }); },
            // 일반 자산의 0 발자국 계산과 구별해 사제의 기존 양수 발자국 계약을 유지한다.
            beginShape: (_canonQuery, cell) => {
                if (types[query.type].footX < 1 || types[query.type].footY < 1) throw new RangeError('사제 모양 발자국 오류');
                if (typeof hooks.beginShape === 'function') hooks.beginShape({ ...query }, cell.frame, cell.snappedX, cell.snappedY);
            },
            endShape: (_canonQuery, cell) => hooks.endShape({ ...query }, cell.frame, cell.snappedX, cell.snappedY),
            finishRegions: () => hooks.finishRegions({ ...query })
        };
        return new RawCanonPlacementGeometry(this.#pool, types, this.#frames, this.#state).inspect(RawPriestPlacementGeometry.#canonQuery(query), (cell) => scan(cell.area), canonHooks);
    }
}

// 이전 후보 충돌 연결을 유지하면서 미리보기 범위(bounds)만 실제 비패턴 decoder 로 대체한다. (2026-11-09 WB04-E156)
export function makePriestGeometryPreviewHooks(pool, geometry, hooks = {}) {
    if (pool !== geometry.pool) throw new TypeError('사제 모양/미리보기의 SID 풀이 다릅니다');
    return { ...hooks, bounds: (query) => geometry.bounds(query) };
}

// 실제 finder 호출은 기존 후보 검사기에 맡기며 모양 사각형의 생성/진행을 연결한다(후보별 지형 효과 경계는 유지). (2026-11-09 WB04-E156)
export function makePriestGeometryCollisionHooks(pool, geometry, hooks = {}) {
    if (pool !== geometry.pool) throw new TypeError('사제 모양/충돌의 SID 풀이 다릅니다');
    return { ...hooks, inspectRegions: (query, scan) => geometry.inspect(query, scan) };
}
