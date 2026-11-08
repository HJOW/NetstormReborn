// 썬 바리케이드 연결 표식(fencemark, TYPE 90)의 기존 연결 삭제(004566d0)를 옮긴 모듈이다(cpppj `o/RawFenceMarkerClear.cpp/.h`, 10.78 전용). 화면의 포스 필드(TYPE 170)와 별도이다.
//  - bounds(parent, other): 두 끝점의 실제 중심(`RawGunTarget.center`)에서 이웃 중심을 먼저 읽고 [1, 255] 로 clamp 한 corner 와 부모 중심의 min/max 를 구해(부모가 보드 안 (0,256) 일 때만 min/max, 아니면 clamp 한 이웃 중심)
//    왼쪽/위는 절삭, 오른쪽/아래는 0.9999f 를 더한 뒤 절삭한 정수 검색 영역을 만든다(비유한 중심은 거부).
//  - clear(parent, other): 그 영역을 일반 finder(flags 1: 표면 건너뜀)로 현재 순서대로 순회하며 현재 TYPE 이 fencemark 타입(getter)인 후보 중 QA WORD(+12)가 부모이거나 packed QB(`(+26 DWORD << 8) | +30 BYTE`)가 부모인 표식만
//    가상 삭제(destroy, flags 0x40)한다. 후보를 미리 모으지 않고 삭제 효과 뒤의 실제 finder next 와 현재 태그를 읽는다. 삭제 소비자가 없으면 삭제에 도달했을 때 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E83: 썬 바리케이드 기존 표식 삭제 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 0041d7a0 이 x87 에서 더한 뒤 절삭하는 원본 float 상수이다. (2026-11-08 WB04-E83)
const CEIL_BIAS = Math.fround(0.9999);
// 슬롯의 TYPE(+10)·QA WORD(+12)·QB 상위 DWORD(+26)·QB 하위 BYTE(+30) 위치와 삭제 flags 이다. (2026-11-08 WB04-E83)
const TYPE = 10;
const QA = 12;
const QB_HIGH = 26;
const QB_LOW = 30;
const DELETE_FLAGS = 0x40;

// 지정한 폭만 little endian 으로 읽는다(이웃 필드에 걸치지 않음). (2026-11-08 WB04-E83)
function read(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 보드 중심의 유효 float 만 원본 CRT 절삭으로 바꾼다. (2026-11-08 WB04-E83)
function toInt(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('fencemark 검색 CRT 범위 오류');
    return Math.trunc(value);
}

const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);

export class RawFenceMarkerClear {
    // 같은 현재 풀·삭제 도중 next 가 현재 체인을 읽는 해시·finder 의 발자국 표·같은 풀의 중심 계산기·현재 fencemark 타입 getter·선택 위치의 가상 삭제 소비자이다.
    #pool;
    #hash;
    #types;
    #target;
    #markerType;
    #destroy;

    // 다른 판본/월드의 가상 공간을 하나의 원본 처리기로 혼합하지 않는다. (2026-11-08 WB04-E83)
    constructor(pool, hash, types, target, markerType, destroy) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || target?.pool !== pool || typeof markerType !== 'function') throw new TypeError('fencemark 기존 표식 삭제 판본/TYPE/풀 불일치');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#target = target;
        this.#markerType = markerType;
        this.#destroy = destroy;
    }

    // 원본은 다른 끝점 중심을 먼저 읽고 Rect 의 들어오는 corner 만 1..255 로 clamp 한다. (2026-11-08 WB04-E83)
    bounds(parent, other) {
        const second = this.#target.center(other);
        const first = this.#target.center(parent);
        if (![first[0], first[1], second[0], second[1]].every(Number.isFinite)) throw new RangeError('fencemark 비유한 중심 좌표');
        const x = clamp(second[0]);
        const y = clamp(second[1]);
        const valid = first[0] > 0 && first[1] > 0 && first[0] < 256 && first[1] < 256;
        const left = valid ? Math.min(first[0], x) : x;
        const top = valid ? Math.min(first[1], y) : y;
        const right = valid ? Math.max(first[0], x) : x;
        const bottom = valid ? Math.max(first[1], y) : y;
        return { left: toInt(left), top: toInt(top), right: toInt(right + CEIL_BIAS), bottom: toInt(bottom + CEIL_BIAS) };
    }

    // 실제 flags 1 은 표면을 건너뛰되 삭제되는 항목의 next 는 같은 finder 가 미리 보관한다. (2026-11-08 WB04-E83)
    clear(parent, other) {
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 모든 후보를 현재 순서대로 읽고 효과 뒤의 체인/태그 변경을 반영한다.
        for (let sid = finder.begin(this.bounds(parent, other), 1); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if (raw[TYPE] !== this.#markerType()) continue;
            if (read(raw, QA, 2) !== parent && (((read(raw, QB_HIGH, 4) << 8) | raw[QB_LOW]) >>> 0) !== parent) continue;
            if (typeof this.#destroy !== 'function') throw new Error('fencemark 기존 표식의 실제 가상 삭제 미연결');
            this.#destroy(sid, DELETE_FLAGS);
        }
    }
}
