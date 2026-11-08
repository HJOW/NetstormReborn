// AI 다리의 단계별 탐색·조각 소비·연결점 진행을 원본 본체에 적용하는 모듈이다(cpppj `o/RawAiBridgePath.cpp/.h`, 10.78: 패치 0040fde0 초기화·0040fd30 소비·004167a0 Step·004173b0 Extend·004174c0 Advance).
//  - new RawAiBridgePath(memory, pool, finder, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool(내용물 슬롯), finder 는 현재 타입 6/권한을 공유하는 `ContainedFinderState`,
//    globals = { now()(0055b4d0 현재 게임 시각), mapLimit()(00531928 좌표 상한, float 로 변환해 검사), bridgeType()(005411a0 실제 다리 TYPE), logKind()(0052f3cc 가 29 이면 디버그 표식도 배치), startMarker()·endMarker()·targetMarker()(0054121c/00541234/0054122c 표식 TYPE) } 는 각 효과 위치에서 현재 값을 다시 읽는다.
//    hooks = { initialize(token, source, target)(0040fde0 같은 +0x5f0 탐색 상태 초기화), prepare(token)→bool(0040fe90 현재 후보 준비의 원래 완료값, E176), search(token)→bool(004144b0 현재 다리 후보 탐색), start(token, target)→점(00410b10 시작점, E176),
//    deploy(token, type, point, pattern, direction)(004108f0 일반 다리 배치와 통지), marker(token, type, point)(004106e0 argument 0/방향 0 표식), technology(owner)→SID(0048fc90 현재 owner 기술 부모), refresh(token, sid)(0040fcb0 같은 실제 다리 재고 보충),
//    destroy(sid, flags)(첫 일치 조각의 가상 +10 삭제), log(token, kind, text)(004105b0 현재 종류/본체 출력 관문 뒤 실제 로그) } 이며 도달한 위치에서만 요구한다.
//  - initialize(token, source, target): 0040fde0 — 네 caller float 를 순서대로 CRT 절삭해 +0x638/+0x63c/+0x640/+0x644 에 쓰고 후보 집계/나선 시작 칸/단계·길이·최적 비용/무효 결과(−10000 비트)/검색 진행/마지막 후보 번호(+0x628 − 1, 0 개면 0xffffffff)만 초기화하며 현재 단계/주변 바이트를 보존한다.
//  - consume(token, parent, pattern): 사용 가능(flags & 0x140 == 0x40)하고 패턴 WORD 가 전체 DWORD 와 같은 첫 내용물만 삭제하고 삭제 뒤 현재 시각(double) + float rate(+0x600)를 +0x630 QWORD 에 쓴다(소유자/TYPE/kind 필터 없음).
//  - step(token, source, target): 한 호출에 한 탐색 단계만 처리한다(상태 0 대기 → 1 초기화 → 2 후보 준비 → 3 탐색 → 4 대기 → 5 결과 반환). 상태 5 에서만 true 를 반환하며 source(`{ x, y }` 를 제자리에서 바꿈)에 +0x6bc 결과를 쓰고 상태를 0 으로 되돌린다.
//  - extend(token, source, target): 완료된 연결점의 배치 → 소비 → 재고 갱신(현재 owner 로 기술 부모를 각각 다시 조회) → 디버그 표식(종류 29) 순서이며 선택한 끝점은 하위가 본체를 바꾸어도 이번 반환값으로 유지한다. 무효 결과는 배치/기술 조회를 생략한다.
//  - advance(token, target): +0x3d0 연결점을 처음 찾거나(시작점 부재는 timeout 을 바꾸지 않고 true) 연장하고, 무효 결과에서만 signed timeout(+0x3d8)을 감소해 0 이하이면 1 로 되돌린 뒤 +0xb00 이 켜져 있으면 "Bridge timeout\n" 로그와 함께 false 를 반환한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E177: AI 다리 경로 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { RawContainedFinder } from './raw-contained-finder.js';

// 원본 무효 결과(−10000.0f) 비트, 소비한 조각의 대기 사용 판정 마스크/값, 디버그 표식 활성 종류, timeout 로그 문구이다. (2026-11-09 WB04-E177)
const UNSET_BITS = 0xc61c4000;
const AVAILABLE_MASK = 0x140;
const AVAILABLE_VALUE = 0x40;
const DEBUG_KIND = 29;
const TIMEOUT_TEXT = 'Bridge timeout\n';
const TWO_63 = 9223372036854775808;

// 비정렬 little endian DWORD 를 원래 BYTE 순서로 조합한다. (2026-11-09 WB04-E177)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 경로 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 원본 DWORD 의 signed 비교를 보존한다. (2026-11-09 WB04-E177)
const signed = (value) => value | 0;

// 원본 004e49c0 은 signed64 로 0 쪽 절삭한 낮은 DWORD 를 반환한다(비유한/범위 밖의 indefinite 하위는 0). (2026-11-09 WB04-E177)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E177)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 도달한 하위가 없으면 앞선 효과를 유지하고 해당 위치에서 오류를 알린다. (2026-11-09 WB04-E177)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 다리 경로 실제 하위 미연결');
    return hook;
}

// 다리 경로 단계 처리기이다. 생성자에서는 탐색/게임 장치 효과를 실행하지 않는다. (2026-11-09 WB04-E177)
export class RawAiBridgePath {
    #memory;
    #pool;
    #finder;
    #globals;
    #hooks;

    // PATCH 풀과 현재 원본 저장소/전역 조회기를 빌린다. (2026-11-09 WB04-E177)
    constructor(memory, pool, finder, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 다리 경로 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI 다리 경로 PATCH 풀 누락');
        if (!finder) throw new TypeError('AI 다리 경로 내용물 권한 상태 누락');
        for (const name of ['now', 'mapLimit', 'bridgeType', 'logKind', 'startMarker', 'endMarker', 'targetMarker']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 다리 경로 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#finder = finder;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 실제 월드 조립 시 동일한 풀을 확인할 수 있도록 제공한다. (2026-11-09 WB04-E177)
    get pool() { return this.#pool; }

    // 소멸된 토큰이나 작은 가짜 본체를 허용하지 않는다. (2026-11-09 WB04-E177)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 다리 경로 실제0xb08 본체 누락');
        return raw;
    }

    // 하위 효과 뒤 본체를 다시 조회해 DWORD 를 읽는다. (2026-11-09 WB04-E177)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 본체의 지정 필드 폭만 쓴다(상위/하위 필드 보존). (2026-11-09 WB04-E177)
    #set(token, offset, value) {
        const raw = this.#body(token);
        if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 경로 DWORD 쓰기 범위');
        // 상위와 하위 필드를 보존하며 네 BYTE 를 기록한다.
        for (let index = 0; index < 4; index += 1) raw[offset + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // 두 좌표의 signed zero 와 무효 좌표도 그대로 읽는다. (2026-11-09 WB04-E177)
    #point(token, offset) { return { x: bitsFloat(this.#field(token, offset)), y: bitsFloat(this.#field(token, offset + 4)) }; }

    // 원래 XY 순서대로 결과를 쓴다. (2026-11-09 WB04-E177)
    #setPoint(token, offset, value) {
        this.#set(token, offset, floatBits(value.x));
        this.#set(token, offset + 4, floatBits(value.y));
    }

    // 0040e800: mapLimit 은 원본처럼 float 에 저장한 뒤 두 축 strict 0..상한/NaN 거부로 비교한다. (2026-11-09 WB04-E177)
    #valid(point) {
        const limit = Math.fround(this.#globals.mapLimit());
        return point.x > 0 && point.x < limit && point.y > 0 && point.y < limit;
    }

    // this = 본체 + 5f0 이며 네 caller float 를 순서대로 절삭한다(소스·목표·후보 개수 이외의 입력은 그대로 둔다). (2026-11-09 WB04-E177)
    initialize(token, source, target) {
        const x = truncate(source.x);
        this.#set(token, 0x638, x);
        const y = truncate(source.y);
        this.#set(token, 0x63c, y);
        this.#set(token, 0x640, truncate(target.x));
        this.#set(token, 0x644, truncate(target.y));
        // 원본 +a0/a4 후보 집계와 +b0/b4 나선 시작 칸이다.
        this.#set(token, 0x690, 0);
        this.#set(token, 0x694, 0);
        this.#set(token, 0x6a0, x);
        this.#set(token, 0x6a4, y);
        // 원본 +a8/ac/b8 의 나선 단계·남은 길이·현재 최적 비용이다.
        this.#set(token, 0x698, 2);
        this.#set(token, 0x69c, 1);
        this.#set(token, 0x6a8, 0x10000);
        // 원본 +d0/cc 의 무효 결과 XY = −10000 float 비트를 그대로 쓴다.
        this.#set(token, 0x6c0, UNSET_BITS);
        this.#set(token, 0x6bc, UNSET_BITS);
        // 원본 +d4/dc/d8 의 검색 진행·현재 후보/마지막 인덱스이며 0 개는 ffffffff 로 감긴다.
        this.#set(token, 0x6c4, 0);
        this.#set(token, 0x6cc, 0);
        this.#set(token, 0x6c8, (this.#field(token, 0x628) - 1) >>> 0);
        return true;
    }

    // 삭제 이후의 현재 시각과 rate 를 더하며 소유자/TYPE/kind 필터를 추가하지 않는다. (2026-11-09 WB04-E177)
    consume(token, parent, pattern) {
        this.#body(token);
        const cursor = new RawContainedFinder(this.#pool, this.#finder);
        // 원본 contained 순서에서 사용 중인 조각을 건너뛰고 첫 패턴 일치만 소비한다.
        for (let sid = cursor.begin(parent); sid; sid = cursor.next()) {
            const raw = this.#pool.slot(sid);
            const flags = raw[20] | (raw[21] << 8);
            const word = raw[12] | (raw[13] << 8);
            if ((flags & AVAILABLE_MASK) !== AVAILABLE_VALUE || word !== (pattern >>> 0)) continue;
            need(this.#hooks.destroy)(sid, 0);
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, bitsFloat(this.#field(token, 0x600)) + this.#globals.now(), true);
            this.#set(token, 0x630, view.getUint32(0, true));
            this.#set(token, 0x634, view.getUint32(4, true));
            return;
        }
    }

    // 전이 이후 다음 단계를 같은 호출에서 실행하지 않는다(상태 5 에서만 true 와 결과). (2026-11-09 WB04-E177)
    step(token, source, target) {
        const hooks = this.#hooks;
        switch (this.#field(token, 0x5f0)) {
        case 0:
            if (signed(this.#field(token, 0x628)) >= signed((this.#field(token, 0x604) - 1) >>> 0)) this.#set(token, 0x5f0, 1);
            break;
        case 1:
            need(hooks.initialize)(token, source, target);
            this.#set(token, 0x5f0, (this.#field(token, 0x5f0) + 1) >>> 0);
            break;
        case 2:
            if (need(hooks.prepare)(token)) this.#set(token, 0x5f0, (this.#field(token, 0x5f0) + 1) >>> 0);
            break;
        case 3:
            if (need(hooks.search)(token)) this.#set(token, 0x5f0, (this.#field(token, 0x5f0) + 1) >>> 0);
            break;
        case 4:
            if (signed(this.#field(token, 0x628)) >= signed(this.#field(token, 0x604))) this.#set(token, 0x5f0, 5);
            break;
        case 5: {
            const result = this.#point(token, 0x6bc);
            source.x = result.x;
            source.y = result.y;
            this.#set(token, 0x5f0, 0);
            return true;
        }
        default:
            break;
        }
        return false;
    }

    // 선택한 끝점은 배치/삭제가 본체를 바꾸어도 이번 반환값으로 유지한다. (2026-11-09 WB04-E177)
    extend(token, source, target) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        const local = { x: source.x, y: source.y };
        if (!this.step(token, local, target)) return { x: source.x, y: source.y };
        const result = this.#point(token, 0x6bc);
        if (this.#valid(result)) {
            const direction = this.#field(token, 0x6b8);
            const pattern = this.#field(token, 0x6b4);
            const point = this.#point(token, 0x6ac);
            need(hooks.deploy)(token, globals.bridgeType() >>> 0, point, pattern, direction);
            const parent = need(hooks.technology)(this.#field(token, 0x260));
            this.consume(token, parent, this.#field(token, 0x6b4));
            const currentParent = need(hooks.technology)(this.#field(token, 0x260));
            need(hooks.refresh)(token, currentParent);
            if ((globals.logKind() >>> 0) === DEBUG_KIND) {
                need(hooks.marker)(token, globals.endMarker() >>> 0, result);
                need(hooks.marker)(token, globals.targetMarker() >>> 0, target);
            }
        }
        return result;
    }

    // 시작점 부재는 즉시 timeout 실패로 바꾸지 않으며 기존 타이머도 건드리지 않는다. (2026-11-09 WB04-E177)
    advance(token, target) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        const current = this.#point(token, 0x3d0);
        if (!this.#valid(current)) {
            const start = need(hooks.start)(token, target);
            this.#setPoint(token, 0x3d0, start);
            if (this.#valid(start) && (globals.logKind() >>> 0) === DEBUG_KIND) need(hooks.marker)(token, globals.startMarker() >>> 0, start);
            return true;
        }
        const next = this.extend(token, current, target);
        this.#setPoint(token, 0x3d0, next);
        if (this.#valid(next)) {
            this.#set(token, 0x3d8, 1);
            return true;
        }
        const timeout = (this.#field(token, 0x3d8) - 1) >>> 0;
        this.#set(token, 0x3d8, timeout);
        if (signed(timeout) > 0) return true;
        this.#set(token, 0x3d8, 1);
        if (this.#field(token, 0xb00)) need(hooks.log)(token, globals.logKind() >>> 0, TIMEOUT_TEXT);
        return false;
    }
}
