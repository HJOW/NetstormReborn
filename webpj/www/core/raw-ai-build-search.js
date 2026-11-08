// 사제 AI 의 실제 574 배치 탐색 상태/행별 예산/가장 먼 합법 후보를 옮긴 모듈이다(cpppj `o/RawAiBuildSearch.cpp/.h`, 10.78: 패치 0040f660 초기화·0040f700 스캔·0040f130 빈 발자국).
//  - new RawAiBuildSearch(memory, types, spots, typeBase, hooks): memory 는 E164 `RawAiBrainMemory`, types 는 현재 188 TYPE({ footX, footY })의 같은 배열(재조회/캐시 핸들이 같은 현재 표를 본다), spots 는 월드의 같은 65536 BYTE spot(256 × 256, 행 우선),
//    typeBase 는 호스트 포인터 절단 없이 500 stride 를 보존하는 opaque32 TYPE 기준(0049a840 의 TYPE 레코드 포인터 산술), hooks = { wall()(00460d70 매 호출 현재 벽시각, 프레임 snapshot 이 아님),
//    mayPlace(query)→bool(0049b510 전체 배치 검사, query = { type, argument, x, y, flags, owner, mode }) } 이며 도달한 위치에서만 요구한다. 상태는 별도 좌표 목록 없이 같은 0xb08 의 +0x574..+0x5a7(34 BYTE)에 쓴다.
//  - initialize(token, type, bounds, avoid, owner): 0040f660 — TYPE(+0) → 캐시 핸들(+0x18, 표 밖 TYPE 이면 이 쓰기 전에 거부) → 회피 좌표 float 비트(+0x10/+0x14) → owner(+0x1c) → 최고 점수 0(+4) → 미정 −10000.0f(+0xc/+8) →
//    발자국을 더한 왼쪽/위(+0x20/+0x24)와 오른쪽/아래(+0x28/+0x2c)를 signed 절삭(CRT ftol)하고 현재 행(+0x30)을 위쪽으로 둔다. avoid 는 `{ x, y, xBits?, yBits? }` 이며 NaN 페이로드를 보존해야 하면 비트를 준다. 별도 clamp 나 inclusive 변환은 없다.
//  - freeFootprint(type, x, y): 현재 발자국의 모든 칸이 `(spot & 0x3f) === 2` 인지 x 바깥/y 안쪽 순서로 검사한다. 0 이하 폭/높이는 칸에 접근하지 않고 성공할 수 있다. 지도 밖 주소는 합법 칸으로 clamp 하지 않고 거부한다.
//  - scan(token, budgeted): 0 은 전체 완료, 비영은 각 행 시작에서 벽시각이 deadline(= 호출 첫 시각 + float 0.03)보다 크거나 NaN 이면 중단하며 한 행은 예산을 넘어도 끝까지 처리한다. 행마다 현재 cursor/최대 Y 를 다시 읽고 중단 시 미처리 행 cursor 를 보존한다.
//    spot 을 통과한 칸에서만 현재 TYPE 발자국을 조회해 빈 발자국이면 거리(x 차이만 float 저장, y 는 배정도)가 현재 점수보다 strict 하게 클 때 전체 배치를 묻고, 허용되면 float 점수와 좌표를 저장한다. 최대 X 만 각 후보 효과 뒤 현재 값으로 다시 읽는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E169: AI 배치 탐색 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 본체 안의 탐색 상태 시작, TYPE 레코드 포인터 산술 stride, 미정 좌표 −10000.0f 비트, PE 00501230 의 정확한 double 작업 예산(float 0.03 을 double 로)이다. (2026-11-09 WB04-E169)
const STATE = 0x574;
const TYPE_STRIDE = 500;
const UNSET = 0xc61c4000;
const WORK_BUDGET = Math.fround(0.03);
const TYPE_COUNT = 188;
const SPOT_BYTES = 65536;
const SIDE = 256;
const TWO_63 = 9223372036854775808;

// 비정렬 원문 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E169)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 탐색 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 만 바꾸고 주변/미초기화 상태를 보존한다. (2026-11-09 WB04-E169)
function put(raw, at, value) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 탐색 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 signed DWORD 의 비트 해석이다. (2026-11-09 WB04-E169)
const signed = (value) => value | 0;

// 원본 DWORD 덧셈/뺄셈의 감김을 signed 값으로 보존한다. (2026-11-09 WB04-E169)
const add = (first, second) => (first + second) | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD 이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E169)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트(uint32)의 상호 변환이다. (2026-11-09 WB04-E169)
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

// 선택되지 않은 하위는 요구하지 않고 선택된 누락만 명시 오류로 처리한다. (2026-11-09 WB04-E169)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 탐색 실제 하위 미연결');
    return hook;
}

// 배치 탐색 처리기이다. 별도 좌표 목록 없이 같은 0xb08 의 574..5a7 과 실제 65536 spot/현재 188 TYPE 를 사용한다. (2026-11-09 WB04-E169)
export class RawAiBuildSearch {
    #memory;
    #types;
    #spots;
    #typeBase;
    #hooks;

    // 생성은 시계나 배치 검사를 실행하지 않는다. TYPE 핸들 기준은 비제로이고 187 번 TYPE 까지 32비트 안이어야 한다. (2026-11-09 WB04-E169)
    constructor(memory, types, spots, typeBase, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 탐색 본체 소유자 누락');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !(spots instanceof Uint8Array) || spots.length !== SPOT_BYTES || !typeBase || typeBase > 0xffffffff - 187 * TYPE_STRIDE) {
            throw new RangeError('AI 배치 탐색 실제188TYPE/65536spot/TYPE핸들 누락');
        }
        this.#memory = memory;
        this.#types = types;
        this.#spots = spots;
        this.#typeBase = typeBase >>> 0;
        this.#hooks = hooks ?? {};
    }

    // 해제/다른 크기 본체를 거부한다. (2026-11-09 WB04-E169)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 탐색 실제0xb08본체 누락');
        return raw;
    }

    // 원본 상대 필드를 현재 본체에서 읽는다. (2026-11-09 WB04-E169)
    #field(token, at) { return read(this.#body(token), STATE + at); }

    // 해당 상대 필드만 바꾼다. (2026-11-09 WB04-E169)
    #set(token, at, value) { put(this.#body(token), STATE + at, value >>> 0); }

    // TYPE 를 별도 사본으로 캐시하지 않고 현재 188 TYPE 를 물리 범위 검사 뒤 읽는다. (2026-11-09 WB04-E169)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 배치 탐색 현재TYPE 범위');
        return this.#types[type];
    }

    // 실제 표와 500 stride 의 안정된 opaque32 대응이며 원본 가상 주소와 같음을 주장하지 않는다. (2026-11-09 WB04-E169)
    #handle(type) {
        this.#type(type);
        return (this.#typeBase + type * TYPE_STRIDE) >>> 0;
    }

    // 손상된 캐시를 원래 TYPE 나 다른 기본값으로 보정하지 않는다. (2026-11-09 WB04-E169)
    #typeFromHandle(pointer) {
        if (pointer < this.#typeBase || (pointer - this.#typeBase) % TYPE_STRIDE) throw new RangeError('AI 배치 탐색 cached TYPE핸들 범위');
        const type = (pointer - this.#typeBase) / TYPE_STRIDE;
        this.#type(type);
        return type;
    }

    // 실제 행 우선 spot 을 읽고 손상된 지도 밖 주소는 clamp 하지 않고 거부한다. (2026-11-09 WB04-E169)
    #spot(x, y) {
        if (x < 0 || y < 0 || x >= SIDE || y >= SIDE) throw new RangeError('AI 배치 탐색 실제spot 주소 범위');
        return this.#spots[y * SIDE + x];
    }

    // 0040f660: 원본 여덟 인자와 두 TYPE 조회/상태 쓰기 순서이다(표 밖 TYPE 이면 먼저 쓴 TYPE 필드는 유지). (2026-11-09 WB04-E169)
    initialize(token, type, bounds, avoid, owner) {
        this.#set(token, 0, type);
        this.#set(token, 0x18, this.#handle(type));
        this.#set(token, 0x10, avoid.xBits ?? floatBits(avoid.x));
        this.#set(token, 0x14, avoid.yBits ?? floatBits(avoid.y));
        this.#set(token, 0x1c, owner);
        this.#set(token, 4, 0);
        this.#set(token, 0xc, UNSET);
        this.#set(token, 8, UNSET);
        const current = this.#type(this.#field(token, 0));
        const width = current.footX;
        const height = current.footY;
        this.#set(token, 0x20, ftol(width + Math.fround(bounds.left)));
        const top = ftol(height + Math.fround(bounds.top));
        this.#set(token, 0x24, top);
        this.#set(token, 0x28, ftol(Math.fround(bounds.right)));
        const bottom = ftol(Math.fround(bounds.bottom));
        this.#set(token, 0x30, top);
        this.#set(token, 0x2c, bottom);
    }

    // 0040f130: 원본은 x 를 바깥쪽에 두며 0 이하 footprint 는 칸에 접근하지 않고 성공할 수 있다. (2026-11-09 WB04-E169)
    freeFootprint(type, x, y) {
        const current = this.#type(type);
        let column = add(x, -(current.footX | 0));
        const firstRow = add(add(y, -(current.footY | 0)), 1);
        // 각 x 열에서 y 시작을 다시 사용하며 최초 실패 칸에서 즉시 종료한다.
        for (;;) {
            column = add(column, 1);
            if (column > x) return true;
            if (firstRow > y) continue;
            let row = firstRow;
            // 안쪽 반복문은 원본 inclusive y footprint 를 순회한다.
            do {
                if ((this.#spot(column, row) & 0x3f) !== 2) return false;
                row = add(row, 1);
            } while (row <= y);
        }
    }

    // 0040f700: 각 호출의 deadline 과 행 시작 clock 만 사용하며 한 행은 예산을 넘어도 끝까지 처리한다. (2026-11-09 WB04-E169)
    scan(token, budgeted) {
        const hooks = this.#hooks;
        if (!this.#field(token, 0)) return;
        const deadline = need(hooks.wall)() + WORK_BUDGET;
        if (signed(this.#field(token, 0x30)) >= signed(this.#field(token, 0x2c))) return;
        // 원본은 행마다 현재 cursor/maxY 를 재조회하고 중단 시 미처리 행 cursor 를 보존한다.
        do {
            if (budgeted && !(need(hooks.wall)() <= deadline)) return;
            let x = signed(this.#field(token, 0x20));
            const y = signed(this.#field(token, 0x30));
            // x 시작과 y 는 이 행 snapshot 이며 maxX 만 각 후보 효과 뒤 현재 값으로 다시 읽는다.
            while (x < signed(this.#field(token, 0x28))) {
                if ((this.#spot(x, y) & 0x3f) === 2 && this.freeFootprint(this.#field(token, 0), x, y)) {
                    const pointX = Math.fround(x);
                    const pointY = Math.fround(y);
                    const avoidX = bitsFloat(this.#field(token, 0x10));
                    const avoidY = bitsFloat(this.#field(token, 0x14));
                    const dx = Math.fround(x - avoidX);
                    const dy = y - avoidY;
                    const distance = dx * dx + dy * dy;
                    const saved = Math.fround(distance);
                    if (distance > bitsFloat(this.#field(token, 4))) {
                        const query = { type: this.#typeFromHandle(this.#field(token, 0x18)), argument: 0, x: pointX, y: pointY, flags: 0, owner: this.#field(token, 0x1c) & 0xff, mode: 1 };
                        if (need(hooks.mayPlace)(query)) {
                            this.#set(token, 4, floatBits(saved));
                            this.#set(token, 8, floatBits(pointX));
                            this.#set(token, 0xc, floatBits(pointY));
                        }
                    }
                }
                x = add(x, 1);
            }
            this.#set(token, 0x30, (this.#field(token, 0x30) + 1) >>> 0);
        } while (signed(this.#field(token, 0x30)) < signed(this.#field(token, 0x2c)));
    }
}
