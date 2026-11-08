// AI 의 부족 좌표 점수/시설별 평가 분배와 실제 배치 후보·에너지 승인을 옮긴 모듈이다(cpppj `o/RawAiDropEvaluation.cpp/.h`, 10.78: 패치 00412640·004126c0·00412ab0·00412b90).
//  - new RawAiDropEvaluation(memory, pool, types, targets, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool(targets 와 같아야 한다), types 는 현재 188 TYPE({ flags2, group })의 같은 배열,
//    targets 는 E172 `RawAiDropTargets`(같은 실제 Gun/지상/블로커 점수), globals = { manaRadius()(0052f488 현재 공급 반경, 제곱 반경과 구분), probeType()(00541174 현재 공급 문자열 조회 TYPE, 초기 71) }는 매 평가 위치에서 다시 읽는다.
//    hooks = { collect(box, x, y, type, owner, theme)(004730c0: box 는 초기 빈 문자열 `{ text: '' }`, theme −1), mayPlace(query)(0049b510 전체: argument 0/flags 0/owner BYTE/mode 1), energy(box, x, y, type, owner)→DWORD(004734d0 실제 승인, bool 로 줄이지 않음),
//    dry(token, x, y, type)→bool(0040f1c0 같은 실제 부족 좌표 추가), unsupported(token, type)(004105b0 종류 29 현재 TYPE 이름 진단, +0xb00 이 비영일 때만), dryLog(token, x, y)(004105b0 현재 Dry Spot 종류/절삭 좌표, 실제 추가 뒤만) } 이다.
//  - dryDistance(token, point, box): 00412640 — 현재 +0xae0 개수/+0xae4 포인터의 각 12바이트 기록(매 기록 재조회)에서 0 이 아닌 Manhattan 거리가 가까울수록 큰 `1000 − 거리`의 첫 최고(축소 전 값으로 비교, 저장은 float)를 구하고 box.value 에 index 를 쓴다(미선택은 보존).
//  - supplyScore(token, point): 004126c0 — 거리 점수가 `1000 − 2 × 반경` 보다 커야 하며 현재 공급 C 문자열(첫 NUL 까지)의 길이가 0 이 아니고 현재 +0xad8 보다 작을 때만 `ftol(저장 float 점수) / (길이 + 1)` 이다.
//  - evaluate(token, type, point, direction, target): 00412ab0 — flags2 0x800 공급 → 0x4000000 블로커 → group 3 지상 → group 0/1 Gun → group 6 은 진단 없이 0, 그 밖은 현재 +0xb00 이면 진단 뒤 0 이다(두 번의 TYPE 조회 순서 보존).
//  - consider(token, type, point, direction, bestPoint, bestScore, target) / considerStored(…, bestPoint, target): 00412b90 — 전체 배치 → 평가 → 양수 동률도 허용하는 개선 판정(읽기는 평가 뒤) → 실제 에너지 승인 → 승인되면 최고 Y → X → 점수 순서로 기록, 아니면 실제 부족 좌표를 추가하고 추가 성공 시 현재 +0xb00 이면 로그한다.
//    considerStored 는 실제 유일한 412e10 호출처럼 점수 포인터(본체 +0x3e8)를 매 읽기/쓰기 위치에서 직접 쓴다. bestScore/target 은 `{ value }` 상자, bestPoint 는 `{ x, y }` 객체이다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E173: AI 배치 평가 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 00501480 의 원본 float 부족 좌표 점수 기준, 부족 좌표 기록 폭(x/y float + TYPE DWORD)이다. (2026-11-09 WB04-E173)
const DISTANCE_VALUE = 1000;
const DRY_STRIDE = 12;
const TWO_63 = 9223372036854775808;

// 지정 비정렬 DWORD 를 현재 물리 블록 안에서 읽는다. (2026-11-09 WB04-E173)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 평가 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 원본 DWORD 를 signed 로 읽는다. (2026-11-09 WB04-E173)
const signed = (value) => value | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E173)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E173)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위가 없으면 그 지점 앞 효과를 보존하며 거부한다. (2026-11-09 WB04-E173)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 평가 실제 하위 미연결');
    return hook;
}

// 배치 후보 평가기이다. 별도 점수·목록 사본 없이 같은 실제 본체와 표적 평가기를 사용한다. (2026-11-09 WB04-E173)
export class RawAiDropEvaluation {
    #memory;
    #pool;
    #types;
    #targets;
    #globals;
    #hooks;

    // 생성 중 게임 판단/장치를 실행하지 않는다. 같은 PATCH 풀과 표적 평가기를 확인한다. (2026-11-09 WB04-E173)
    constructor(memory, pool, types, targets, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 평가 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || targets?.pool !== pool) throw new TypeError('AI 배치 평가 같은 PATCH풀 누락');
        if (!Array.isArray(types) || typeof globals?.manaRadius !== 'function' || typeof globals.probeType !== 'function') throw new TypeError('AI 배치 평가 TYPE 표/현재 전역 조회기 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#targets = targets;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E173)
    get pool() { return this.#pool; }

    // 현재 본체의 정확한 크기/수명을 확인한다. (2026-11-09 WB04-E173)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 평가 실제0xb08본체 누락');
        return raw;
    }

    // 하위 뒤 현재 본체를 다시 빌려 DWORD 를 읽는다. (2026-11-09 WB04-E173)
    #field(token, at) { return read(this.#body(token), at); }

    // 지정 DWORD 이외의 원문을 보존하며 쓴다. (2026-11-09 WB04-E173)
    #set(token, at, value) {
        const raw = this.#body(token);
        if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 평가 쓰기 범위');
        // 지정 네 바이트만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[at + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // TYPE 0 도 원본처럼 조회하며 물리 표 밖만 거부한다. (2026-11-09 WB04-E173)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 배치 평가 현재TYPE 범위');
        return this.#types[type];
    }

    // 00412640: 각 개선 비교는 축소 전 값으로 하되 선택된 최고값은 원본처럼 float 저장 후 재사용한다. (2026-11-09 WB04-E173)
    dryDistance(token, point, box) {
        let best = 0;
        const x = Math.fround(point.x);
        const y = Math.fround(point.y);
        // 매 기록에서 현재 포인터/count 를 다시 읽고 TYPE/용량/cursor 로 임의 필터하지 않는다.
        for (let current = 0; current < signed(this.#field(token, 0xae0)); current += 1) {
            const raw = this.#memory.bytes(this.#field(token, 0xae4));
            const at = current * DRY_STRIDE;
            const distance = Math.abs(bitsFloat(read(raw, at)) - x) + Math.abs(bitsFloat(read(raw, at + 4)) - y);
            if (distance > 0) {
                const value = DISTANCE_VALUE - distance;
                if (value > best) {
                    box.value = current;
                    best = Math.fround(value);
                }
            }
        }
        return best;
    }

    // 004126c0: 공급 문자열은 C 의 첫 NUL 까지 세고 실제 공급이 선택한 동일 selected 목록 효과를 유지한다. (2026-11-09 WB04-E173)
    supplyScore(token, point) {
        const unused = { value: 0 };
        const value = this.dryDistance(token, point, unused);
        const radius = Math.fround(this.#globals.manaRadius());
        if (!(value > DISTANCE_VALUE - (radius + radius))) return 0;
        const box = { text: '' };
        const owner = this.#field(token, 0x260);
        const probe = this.#globals.probeType() >>> 0;
        need(this.#hooks.collect)(box, point.x, point.y, probe, owner, -1);
        const nul = box.text.indexOf('\0');
        const length = nul < 0 ? box.text.length : nul;
        if (length > 0x7ffffffe) throw new RangeError('AI 배치 공급 C문자열 길이 범위');
        if (!length || length >= signed(this.#field(token, 0xad8))) return 0;
        return Math.trunc(ftol(value) / (length + 1));
    }

    // 00412ab0: 두 TYPE 조회의 flags/group 을 먼저 읽고 원본 우선순위대로 단 하나의 평가만 선택한다. (2026-11-09 WB04-E173)
    evaluate(token, type, point, direction, target) {
        const flags = this.#type(type).flags2 >>> 0;
        const group = this.#type(type).group;
        if (flags & 0x800) return this.supplyScore(token, point);
        if (flags & 0x4000000) return this.#targets.blockers(token, type, point, direction, target);
        if (group === 3) return this.#targets.ground(token, type, point, target);
        if (group === 0 || group === 1) return this.#targets.gun(token, type, point, direction, target);
        if (group !== 6) {
            this.#type(type);
            if (this.#field(token, 0xb00)) need(this.#hooks.unsupported)(token, type);
        }
        return 0;
    }

    // 후보를 채택하지 않아도 대상 SID 의 평가 효과는 보존하고 승인 뒤 목표 Y → X → 점수 순서로만 쓴다. (2026-11-09 WB04-E173)
    #consider(token, type, point, direction, bestPoint, target, readScore, writeScore) {
        const hooks = this.#hooks;
        const query = { type, argument: 0, x: Math.fround(point.x), y: Math.fround(point.y), flags: 0, owner: this.#field(token, 0x260) & 255, mode: 1 };
        this.#type(type);
        if (!need(hooks.mayPlace)(query)) return false;
        const value = this.evaluate(token, type, point, direction, target);
        const bestScore = readScore();
        if (!(bestScore < value || (value > 0 && value === bestScore))) return false;
        const box = { text: '' };
        const owner = this.#field(token, 0x260);
        if (need(hooks.energy)(box, point.x, point.y, type, owner)) {
            bestPoint.y = point.y;
            bestPoint.x = point.x;
            writeScore(value);
            return true;
        }
        if (need(hooks.dry)(token, point.x, point.y, type)) {
            const y = ftol(point.y);
            const x = ftol(point.x);
            if (this.#field(token, 0xb00)) need(hooks.dryLog)(token, x, y);
        }
        return false;
    }

    // 명시 외부 점수 상자도 평가가 끝난 그 시점에 읽는다. (2026-11-09 WB04-E173)
    consider(token, type, point, direction, bestPoint, bestScore, target) {
        return this.#consider(token, type, point, direction, bestPoint, target, () => bestScore.value, (value) => { bestScore.value = value; });
    }

    // 실제 스캔의 +0x3e8 을 임시 사본/호출 뒤 일괄 동기화로 대신하지 않고 읽기/쓰기 위치에서 직접 쓴다. (2026-11-09 WB04-E173)
    considerStored(token, type, point, direction, bestPoint, target) {
        return this.#consider(token, type, point, direction, bestPoint, target, () => signed(this.#field(token, 0x3e8)), (value) => this.#set(token, 0x3e8, value >>> 0));
    }
}
