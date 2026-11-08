// AI 네 다리 전략의 공통 관문·목록 이동·현재 목표·확장 순서를 원본 본체/목록/난수에 적용하는 모듈이다(cpppj `o/RawAiBridgeStrategies.cpp/.h`, 10.78: 패치 00417bd0 입구와 네 case 전체).
//  - new RawAiBridgeStrategies(memory, pool, types, random, globals, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE 의 `{ flags2 }` 표, random 은 같은 게임 공용 난수(`next(limit)`),
//    globals = { now()(0055b4d0 현재 QWORD 게임 시각), mapLimit()(00531928 을 float 로 저장해 좌표 유효성 검사), invalidGraph()(00540410 현재 DWORD, BYTE 장부도 같은 getter), checking()(005e4794)·disabled()(0054db88, checking 일 때만 읽음), geyserLog()(0052f3e4 간헐천 로그 종류) } 는
//    각 원본 조회 위치에서 현재 값을 다시 읽고, hooks = { technology(owner)→SID(0048fc90), refresh(token, sid)(0040fcb0), graph(sid)→DWORD(00462bc0), center(sid)→점(004adea0 현재 float 중심), connected(token, sid)→bool(00410d40), delay(token)(00411310),
//    home(token)→점(004112b0), reclassify(token)(00411350), select(token, point, excluded, required)→SID(00410a30), randomStart(token, bounds)→점(00411470), advance(token, point)→bool(004174c0), extend(token, source, target)→점(004173b0),
//    rectangle(left, top, right, bottom)→bool(004119b0 inclusive 빈 spot 사각형), assertion(text, line)(004e0620 진단, 반환하면 원본처럼 계속), log(token, kind, event, sid)(004105b0) } 는 도달한 위치에서만 요구한다.
//  - update(token): 공통 관문(실제 본체의 신전 +0x294·graph +0x298 이 무효 graph 와 다름·+0x248 의 0x1000 비트·checking 이면서 disabled 가 아님)을 통과하면 현재 owner 의 기술 부모를 조회해 재고를 보충한 뒤 현재 목표(+0x2d4)의 1..4 case 를 정확히 한 번만 처리한다.
//    1 신전: 기한(+0x5a8 QWORD)·신전/graph 확인 뒤 살아 있는 다른 graph 의 신전 목표로 진행하거나 대상을 초기 목표(+0x2a8)로 되돌리고 시작점을 −10000 으로 지운다.
//    2 간헐천: 현재 대상 연결 판정→연결 시 미연결(+0x348) 목록에서 모든 일치를 안정 압축 제거하고 연결(+0x330) 목록에 추가, 재시도 기한 도래 시 포기(+0x354) 목록의 첫 항목을 되살려 미연결 목록으로 옮기며, 연결되지 않은 유효 대상은 중심을 거의 올림(+0.99999f 절삭)해 진행하거나 포기/새 대상 선택을 한다.
//    3 확장: 기한(+0x5b0)·초기 목표·잔여 횟수 확인 뒤 난수로 고른 주변 3칸 경계 또는 자기 영역(+0x270..+0x27c)에서 무작위 시작점을 얻어 ±8 의 무작위 목표로 연장하고, 연장 결과가 시작점과 같지 않을 때만 잔여 횟수(+0x3c8)를 줄인다.
//    4 좌표: 명시 좌표(+0xaf0/+0xaf4)가 유효하지 않으면 진단한 뒤 그 칸 주변 inclusive 3×3 사각형이 비어 있고 진행이 성공하면 유지, 아니면 좌표를 −10000 으로 지운다.
//  - 목록은 원본 네이티브 0040e9b0 처럼 count/용량을 먼저 읽어 가득 차면 추가를 생략하고, 중복을 유지하며, 제거는 안정 압축하되 미사용 꼬리를 보존한다. 점/경계는 NaN 비트 보존을 위해 `xBits/yBits`(원문 float 비트)를 함께 가질 수 있다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E178: AI 다리 전략 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 0041d7a0 의 00502310 실제 float 상수이다(중심을 거의 올림). 무효 좌표(−10000.0f) 비트와 간헐천 로그 사건 번호, TYPE 수, 목표 번호이다. (2026-11-09 WB04-E178)
const POINT_BIAS = Math.fround(0.99999);
const UNSET_BITS = 0xc61c4000;
const TYPE_COUNT = 188;
const TWO_63 = 9223372036854775808;
// 간헐천 목록 로그의 세 사건(재등록/포기/시도 중)이다. (2026-11-09 WB04-E178)
export const AI_BRIDGE_STRATEGY_LOG = Object.freeze({ requeued: 0, abandoned: 1, trying: 2 });

// 비정렬 DWORD 의 주변 바이트를 읽지 않는다. (2026-11-09 WB04-E178)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 전략 DWORD 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 현재 지정한 DWORD 만 쓴다(주변 바이트 보존). (2026-11-09 WB04-E178)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 전략 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원문 DWORD 의 signed 비교이다. (2026-11-09 WB04-E178)
const signed = (value) => value | 0;

// 실제 __ftol signed64 하위 DWORD 이며 indefinite 하위는 0 이다. (2026-11-09 WB04-E178)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E178)
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

// 두 DWORD 를 이은 QWORD(낮은 DWORD 먼저)를 배정도 시각으로 해석한다. NaN 은 순서 없음(모든 비교 거짓)이다. (2026-11-09 WB04-E178)
function deadline(low, high) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, low >>> 0, true);
    view.setUint32(4, high >>> 0, true);
    return view.getFloat64(0, true);
}

// 선택된 하위 누락을 빈 성공으로 바꾸지 않고 도달한 위치에서만 오류로 알린다. (2026-11-09 WB04-E178)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 다리 전략 실제 하위 미연결');
    return hook;
}

// 점의 한 좌표 원문 비트이다(NaN 비트 보존용 xBits/yBits 가 있으면 그 값). (2026-11-09 WB04-E178)
const pointBits = (point, axis) => (axis === 'x' ? (point.xBits ?? floatBits(point.x)) : (point.yBits ?? floatBits(point.y))) >>> 0;

// 0041d610 은 사각형을 수정하지 않고 고정 256 경계에서 여백 5 를 엄격히 검사한다. (2026-11-09 WB04-E178)
const interior = (bounds) => bounds.left > 5 && bounds.top > 5 && bounds.right < 251 && bounds.bottom < 251;

// 네 다리 전략 처리기이다. 생성은 게임 효과/난수/필드 변경을 실행하지 않는다. (2026-11-09 WB04-E178)
export class RawAiBridgeStrategies {
    #memory;
    #pool;
    #types;
    #random;
    #globals;
    #hooks;

    // 같은 PATCH 188 세계와 현재 전역을 빌린다. (2026-11-09 WB04-E178)
    constructor(memory, pool, types, random, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 다리 전략 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('AI 다리 전략 실제 PATCH188 누락');
        if (typeof random?.next !== 'function') throw new TypeError('AI 다리 전략 공용 난수 누락');
        for (const name of ['now', 'mapLimit', 'geyserLog']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 다리 전략 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#random = random;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 실제 월드 조립에서 다른 월드 혼합을 확인하기 위한 같은 풀이다. (2026-11-09 WB04-E178)
    get pool() { return this.#pool; }

    // 현재 살아 있는 같은 본체를 다시 조회하고 정확한 0xb08 만 허용한다. (2026-11-09 WB04-E178)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 다리 전략 실제0xb08 본체 누락');
        return raw;
    }

    // 외부 효과 뒤의 현재 DWORD 를 읽는다. (2026-11-09 WB04-E178)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 원본이 쓰는 네 BYTE 만 바꾼다. (2026-11-09 WB04-E178)
    #setField(token, offset, value) { write(this.#body(token), offset, value >>> 0); }

    // 두 float 의 원문 비트를 NaN/부호 0 도 그대로 읽는다. (2026-11-09 WB04-E178)
    #point(token, offset) {
        const xBits = this.#field(token, offset);
        const yBits = this.#field(token, offset + 4);
        return { x: bitsFloat(xBits), y: bitsFloat(yBits), xBits, yBits };
    }

    // 같은 실제 본체의 X/Y 순서로 정확한 비트를 쓴다. (2026-11-09 WB04-E178)
    #setPoint(token, offset, point) {
        this.#setField(token, offset, pointBits(point, 'x'));
        this.#setField(token, offset + 4, pointBits(point, 'y'));
    }

    // 잘못된 번호를 다른 정상 WORD 로 감지 않고 물리 풀 검사 뒤에만 SID 로 좁힌다. (2026-11-09 WB04-E178)
    #object(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 다리 전략 실제 SID 범위');
        return value;
    }

    // free/owner 필터를 genus 조회에 추가하지 않고 현재 raw 타입의 실제 flags2 이다. (2026-11-09 WB04-E178)
    #genus(value) {
        const kind = this.#pool.slot(this.#object(value))[10];
        if (kind >= this.#types.length) throw new RangeError('AI 다리 전략 TYPE 범위');
        return this.#types[kind].flags2 >>> 0;
    }

    // 현재 목록 포인터의 실제 DWORD 이며 목록 밖 호스트 메모리 접근만 명시 거부한다. (2026-11-09 WB04-E178)
    #item(token, list, index) {
        if (index < 0) throw new RangeError('AI 다리 전략 음수 목록 인덱스');
        return read(this.#memory.bytes(this.#field(token, list)), index * 4);
    }

    // 네이티브 0040e9b0 과 같이 count/용량을 먼저 읽고 가득 차면 포인터를 읽지 않고 생략하며 중복을 유지한다. (2026-11-09 WB04-E178)
    #append(token, list, value) {
        const count = signed(this.#field(token, list + 8));
        if (count >= signed(this.#field(token, list + 4))) return;
        if (count < 0) throw new RangeError('AI 다리 전략 음수 추가 위치');
        write(this.#memory.bytes(this.#field(token, list)), count * 4, value);
        this.#setField(token, list + 8, count + 1);
    }

    // 모든 일치를 제거하며 네이티브 안정 압축과 미사용 꼬리 보존을 유지한다. (2026-11-09 WB04-E178)
    #remove(token, list, value) {
        let removed = 0;
        // 현재 active 전체에서 모든 동일 값을 제거한다(count 는 매 반복 현재 값).
        for (let index = 0; index < signed(this.#field(token, list + 8)); index += 1) {
            const item = this.#item(token, list, index);
            if (item === value) removed += 1;
            else write(this.#memory.bytes(this.#field(token, list)), (index - removed) * 4, item);
        }
        this.#setField(token, list + 8, this.#field(token, list + 8) - removed);
    }

    // 네 case 는 refresh 뒤 현재 신전/graph/기한을 다시 검사하며 순서 없는 기한은 실행하지 않는다. (2026-11-09 WB04-E178)
    #ready(token, offset) {
        if (!this.#field(token, 0x294) || this.#field(token, 0x298) === need(this.#globals.invalidGraph)()) return false;
        const due = deadline(this.#field(token, offset), this.#field(token, offset + 4));
        return this.#globals.now() >= due;
    }

    // 원본 지도 상한은 float 로 저장한 뒤 strict 두 축 조건으로 검사한다. (2026-11-09 WB04-E178)
    #valid(point) {
        const limit = Math.fround(this.#globals.mapLimit());
        return point.x > 0 && point.x < limit && point.y > 0 && point.y < limit;
    }

    // 0040f330 의 실제 중심 → +0.99999f → signed CRT 하위 DWORD → float 이다. (2026-11-09 WB04-E178)
    #roundedCenter(value) {
        const point = need(this.#hooks.center)(this.#object(value));
        return { x: Math.fround(signed(truncate(point.x + POINT_BIAS))), y: Math.fround(signed(truncate(point.y + POINT_BIAS))) };
    }

    // 원본 필드 쓰기 순서(Y → X)로 시작점 +0x3d0/+0x3d4 를 −10000 으로 쓴다. (2026-11-09 WB04-E178)
    #resetPoint(token) {
        this.#setField(token, 0x3d4, UNSET_BITS);
        this.#setField(token, 0x3d0, UNSET_BITS);
    }

    // 목표 1: 현재 살아 있는 다른 graph 신전으로 진행하거나 대상을 재설정한다(유효한 기존 적 신전을 진행한 호출은 추가 상태를 만들지 않음). (2026-11-09 WB04-E178)
    #temple(token) {
        if (!this.#ready(token, 0x5a8)) return;
        const target = this.#field(token, 0x3cc);
        if (target && !(this.#pool.slot(this.#object(target))[11] & 2) && (this.#genus(target) & 0x200) && need(this.#hooks.graph)(this.#object(this.#field(token, 0x3cc))) !== this.#field(token, 0x298)) {
            const point = this.#roundedCenter(this.#field(token, 0x3cc));
            need(this.#hooks.advance)(token, point);
            return;
        }
        this.#setField(token, 0x3cc, this.#field(token, 0x2a8));
        this.#resetPoint(token);
    }

    // 목표 2: 현재 대상 연결 → 포기 목록 재등록 → 진행/실패 또는 새 대상 선택의 원래 순서이다. (2026-11-09 WB04-E178)
    #geysers(token) {
        if (!this.#ready(token, 0x5a8)) return;
        const target = this.#field(token, 0x3cc);
        const valid = Boolean(target && !(this.#pool.slot(this.#object(target))[11] & 2) && (this.#genus(target) & 0x10000000));
        let connected = false;
        if (valid && (connected = Boolean(need(this.#hooks.connected)(token, this.#object(this.#field(token, 0x3cc)))))) {
            this.#remove(token, 0x348, this.#field(token, 0x3cc));
            this.#append(token, 0x330, this.#field(token, 0x3cc));
        }
        const due = deadline(this.#field(token, 0x5d0), this.#field(token, 0x5d4));
        if (signed(this.#field(token, 0x35c)) > 0 && this.#globals.now() >= due) {
            const retry = this.#item(token, 0x354, 0);
            need(this.#hooks.delay)(token);
            this.#remove(token, 0x354, retry);
            this.#append(token, 0x348, retry);
            need(this.#hooks.log)(token, this.#globals.geyserLog(), AI_BRIDGE_STRATEGY_LOG.requeued, retry);
        }
        if (valid && !connected) {
            const point = this.#roundedCenter(this.#field(token, 0x3cc));
            if (need(this.#hooks.advance)(token, point)) return;
            const abandoned = this.#field(token, 0x3cc);
            this.#append(token, 0x354, abandoned);
            need(this.#hooks.delay)(token);
            this.#remove(token, 0x348, abandoned);
            this.#setField(token, 0x3cc, 0);
            this.#resetPoint(token);
            this.#setField(token, 0x3d8, 1);
            need(this.#hooks.log)(token, this.#globals.geyserLog(), AI_BRIDGE_STRATEGY_LOG.abandoned, abandoned);
            return;
        }
        const home = need(this.#hooks.home)(token);
        need(this.#hooks.reclassify)(token);
        const invalid = need(this.#globals.invalidGraph)();
        const selected = need(this.#hooks.select)(token, home, invalid, invalid);
        this.#setField(token, 0x3cc, selected);
        if (selected) need(this.#hooks.log)(token, this.#globals.geyserLog(), AI_BRIDGE_STRATEGY_LOG.trying, selected);
        this.#resetPoint(token);
    }

    // 목표 3: 주변 대상 사각형 검사가 실패해도 오른쪽 아래 좌표를 유지해 같은 유효성 분기로 진행한다. (2026-11-09 WB04-E178)
    #expand(token) {
        if (!this.#ready(token, 0x5b0) || !this.#field(token, 0x2a8) || signed(this.#field(token, 0x3c8)) < 1) return;
        const index = signed(this.#random.next((this.#field(token, 0x2f0) + 1) >>> 0));
        let source;
        if (index < signed(this.#field(token, 0x2f0))) {
            const target = this.#item(token, 0x2e8, index);
            const raw = this.#pool.slot(this.#object(target));
            const x = bitsFloat(read(raw, 14));
            const y = bitsFloat(read(raw, 18));
            const bounds = { left: Math.fround(x - 3), top: Math.fround(y - 3), right: Math.fround(x + 3), bottom: Math.fround(y + 3) };
            source = { x: bounds.right, y: bounds.bottom };
            if (interior(bounds)) source = need(this.#hooks.randomStart)(token, bounds);
            else this.#remove(token, 0x2e8, target);
        } else {
            const bounds = { left: bitsFloat(this.#field(token, 0x270)), top: bitsFloat(this.#field(token, 0x274)), right: bitsFloat(this.#field(token, 0x278)), bottom: bitsFloat(this.#field(token, 0x27c)) };
            source = need(this.#hooks.randomStart)(token, bounds);
        }
        if (!this.#valid(source)) return;
        const dy = signed(this.#random.next(16)) - 8;
        const dx = signed(this.#random.next(16)) - 8;
        const target = { x: Math.fround(source.x + Math.fround(dx)), y: Math.fround(source.y + Math.fround(dy)) };
        const next = need(this.#hooks.extend)(token, source, target);
        this.#setPoint(token, 0x3d0, next);
        const current = this.#point(token, 0x3d0);
        if (!(current.x === source.x && current.y === source.y)) this.#setField(token, 0x3c8, this.#field(token, 0x3c8) - 1);
    }

    // 목표 4: 진단 뒤 현재 좌표를 다시 읽어 inclusive 사각형과 실제 진행을 호출한다. (2026-11-09 WB04-E178)
    #coordinate(token) {
        if (!this.#ready(token, 0x5b0)) return;
        if (!this.#valid(this.#point(token, 0xaf0))) need(this.#hooks.assertion)('sendBridgeHere.isValid()', 0xa3a);
        const x = truncate(bitsFloat(this.#field(token, 0xaf0)));
        const y = truncate(bitsFloat(this.#field(token, 0xaf4)));
        if (!need(this.#hooks.rectangle)(signed(x - 1), signed(y - 1), signed(x + 1), signed(y + 1)) || !need(this.#hooks.advance)(token, this.#point(token, 0xaf0))) {
            this.#setField(token, 0xaf4, UNSET_BITS);
            this.#setField(token, 0xaf0, UNSET_BITS);
        }
    }

    // 00417bd0: 공통 관문 → 현재 재고 갱신 → 현재 목표 1..4 를 원본 순서로 한 번 처리한다(미지원 목표도 선행 재고 효과는 보존). (2026-11-09 WB04-E178)
    update(token) {
        this.#body(token);
        if (!this.#field(token, 0x294) || this.#field(token, 0x298) === need(this.#globals.invalidGraph)() || !(this.#field(token, 0x248) & 0x1000) || (need(this.#globals.checking)() && need(this.#globals.disabled)())) return;
        const parent = need(this.#hooks.technology)(this.#field(token, 0x260));
        need(this.#hooks.refresh)(token, parent);
        switch (this.#field(token, 0x2d4)) {
        case 1: this.#temple(token); break;
        case 2: this.#geysers(token); break;
        case 3: this.#expand(token); break;
        case 4: this.#coordinate(token); break;
        default: break;
        }
    }
}
