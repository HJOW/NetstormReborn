// 원본 커서 미리보기 수집기(10.78 00446060)를 옮긴 모듈이다: 커서 조각의 부분 기록·쌍 제출과 패치판 SP 표시 상태(cpppj `client/PreviewCollection.cpp/.h`, 10.78 전용. CD 0043da30 경로는 범위 밖).
// 공통 표시 목록(`render/draw-submission.js`)의 마지막 수집기(kind 4 미리보기)로 클립 전 영역과 남은 작업 칸을 받아, 커서가 들고 있는 조각마다 본체(tail 0)와 보조 레이어(tail 2, 깊이 −1)의 두 항목을 쓴다.
//  - collect(view, output, limit): 잠금 깊이가 0 이 아니면 아무것도 쓰지 않는다. 조각(pieces[i], i < pieceCount — 매 반복에서 현재 개수·목록을 다시 읽음)마다: limit < 1 이면 멈추고(그래서 limit 1 에서도 한 쌍을 쓴다),
//    조각 SID 의 raw 위치(+14/+18)와 토큰(`tokenBase + SID × 50`)을 현재 칸에 먼저 쓰고(건너뛴 조각도 x/y/token/depth 14바이트는 남고 kind/tail 과 반환 수는 그대로), 깊이를 정한다:
//    선택 타입의 zOrder → 부모 Gump 가 있으면 `gumpDepth(부모) − 9` → TYPE flags2 의 표면 비트(4)면 0xffffffff → 섬 비트(0x400000)면 −20. 조각 타입이 받침(specialTypeA/B)이면 배치 불가(legal 0)일 때 건너뛰고,
//    가능하면 배치 칸 spot(`x + (y << 8)`, 좌표는 +0.9999 float 상수를 x87 로 더한 뒤 CRT 64비트 절삭의 하위 DWORD)의 비트 2 가 서 있을 때 건너뛰며 아니면 본체 깊이를 20 으로 둔다. 쌍을 낼 칸이 모자라면 오류이다.
//    쌍을 낼 때마다 패치판 표시 피드백(patchFeedback)을 실행하고 본체 kind 4/tail 0, 보조(같은 위치·현재 SID 토큰·`깊이 − 1`·kind 4·tail 2)를 쓴 뒤 count += 2, limit -= 2 이다.
//  - patchFeedback(owner): 분산 SP offset 과 복사 offset 이 다르면 강조 시각(highlight)을 현재 시각으로, 카운트다운이 있으면 SP 조회가 켜져 있을 때 소유자 SP float 이 7000 보다 클 때(순서 비교, NaN 제외) 강조하고 카운트다운을 1 줄이며,
//    첫 SP 풀이 없거나 첫 순열 WORD 가 −1 이면 강조 시각을 현재 시각으로 쓴다. 각 값은 hooks 가 있으면 매번 현재 값을, 없으면 state 의 독립 값을 읽는다.
//  - object(token): 대리 토큰을 같은 풀의 SID 로 되돌린다(tokenBase 미만·0·stride 배수 아님·용량 밖은 거부). makePreviewSpHooks(store, hooks): 초기화를 끝낸 분산 SP 저장소의 읽기를 `readSp` 로 연결한다.
//    bindPreviewCollection(hooks, collector): 공통 수집기 묶음의 `preview` 를 이 수집기로 연결한다.
// 항목은16바이트 Uint8Array다. bindCursor(source)는 같은 실제 커서 전역을 빌려 출력/SP 재진입 뒤 현재 목록/개수를 읽는다. 표시 피드백은 기존 state에 유지한다. (2026-11-14 WB04-E404)
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E386: 5-A 타입별 Draw·미리보기 첫 단위로 추가, 기준선 뒤 cpppj 변경 없음)
import { captureCursorPreviewSource, cursorPreviewCollectionInput } from './cursor-preview-source.js';
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';

// 원본 배치 칸의 +0.9999 단정도 상수(0x3f7ff972)와 SP 강조 기준, TYPE flags2 의 표면/섬 우선 비트이다. (2026-11-13 WB04-E386)
const SPOT_BIAS = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3f7ff972, true);
    return view.getFloat32(0, true);
})();
const SP_THRESHOLD = 7000.0;
const SURFACE = 4;
const ISLAND = 0x400000;
// 항목 안의 바이트 위치와 종류 번호이다. (2026-11-13 WB04-E386)
const X = 0;
const Y = 4;
const TOKEN = 8;
const DEPTH = 12;
const KIND = 14;
const TAIL = 15;
const KIND_PREVIEW = 4;
// 풀 레이아웃(10.78): SID 슬롯 50바이트, 용량과 곱한 토큰 범위가 32비트를 넘으면 거부한다. (2026-11-13 WB04-E386)
const STRIDE = 50;
const TOKEN_SPACE = 0x100000000;

// 커서 원문 DWORD 를 호스트 SID 로 바꾼다(범위 밖이면 오류). 커서 생명주기 단위(RawCursorLifecycle)가 같은 함수를 쓴다. (2026-11-13 WB04-E386)
export function cursorPreviewSid(number) {
    const value = number >>> 0;
    if (value > 65535) throw new RangeError('커서 원문 DWORD 의 호스트 SID 범위');
    return value;
}

// 슬롯의 비정렬 little endian DWORD/float32 를 읽는다. (2026-11-13 WB04-E386)
const readFloat = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);

// 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태이다. 사용: new PreviewCollectionState({ pieceCount: 1, pieces: [5], type: 1 }). (2026-11-13 WB04-E386)
export class PreviewCollectionState {
    // 잠금 깊이·선택 타입·부모 Gump·받침 타입 A/B·배치 가능, 조각 개수(signed)와 조각 SID 목록, 배치 칸 좌표(float32), 표시 피드백 값들이다.
    lockDepth = 0;
    type = 0;
    parentGump = 0;
    specialTypeA = 0;
    specialTypeB = 0;
    legal = 0;
    pieceCount = 0;
    pieces = [];
    x = 0;
    y = 0;
    scrambledOffset = 0;
    copiedOffset = 0;
    now = 0;
    highlight = 0;
    countdown = 0;
    querySp = 0;
    localPlayer = 0;
    firstSpPoolPresent = true;
    firstSpIndex = 0;

    // 일부 필드를 덮어쓴다.
    constructor(init = {}) { Object.assign(this, init); }
}

// 배치 칸 좌표를 spot 지표로 바꾼다: float 좌표 + 상수를 double 로 더해 64비트 절삭의 하위 32비트(비유한/범위 밖은 0). (2026-11-13 WB04-E386)
function spotCoordinate(coordinate) {
    const value = Math.fround(coordinate) + SPOT_BIAS;
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 커서 조각 미리보기 수집기이다. 풀/타입/spot/state 는 이 객체보다 오래 살아야 한다. 사용: new PreviewCollection(pool, types, spots, state, hooks, tokenBase). (2026-11-13 WB04-E386)
export class PreviewCollection {
    // 객체 슬롯 풀, 타입 표(zOrder·flags2), 256×256 원본 spot 표, 현재 상태, 외부 조회 훅, 토큰 기준이다.
    #pool;
    #types;
    #spots;
    #state;
    #hooks;
    #tokenBase;
    // 현재 실제 커서 참조 묶음이며 독립 피드백 state와 분리한다. (2026-11-14 WB04-E404)
    #cursor = null;

    // spot 표 크기(65536)와 토큰 범위를 생성 때 검증한다. (2026-11-13 WB04-E386)
    constructor(pool, types, spots, state, hooks = {}, tokenBase = 0) {
        if (types.length === 0 || spots.length !== 65536) throw new TypeError('Preview collection tables');
        if (tokenBase + pool.capacity * STRIDE > TOKEN_SPACE) throw new RangeError('Preview collection token range');
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
        this.#tokenBase = tokenBase >>> 0;
    }

    // 같은 PATCH 풀의 실제 전역만 빌린다. 연결 뒤 외부 묶음의 참조 교체는 따르지 않는다. (2026-11-14 WB04-E404)
    bindCursor(source) { this.#cursor = captureCursorPreviewSource(this.#pool, source, 'Preview collection'); }

    // 패치판의 offset/카운터/SP float/첫 포인터·순열 검사 순서로 표시 시각(highlight)을 갱신한다. (2026-11-13 WB04-E386)
    #patchFeedback(owner) {
        const state = this.#state;
        const hooks = this.#hooks;
        const now = () => (hooks.now ? hooks.now() : state.now) >>> 0;
        const offset = (hooks.scrambledOffset ? hooks.scrambledOffset() : state.scrambledOffset) >>> 0;
        const copied = (hooks.copiedOffset ? hooks.copiedOffset() : state.copiedOffset) >>> 0;
        if (offset !== copied) state.highlight = now();
        if (state.countdown >>> 0) {
            if ((hooks.querySp ? hooks.querySp() : state.querySp) >>> 0) {
                if (!hooks.readSp) throw new TypeError('Preview collection SP hook');
                const bits = new DataView(new ArrayBuffer(4));
                bits.setUint32(0, hooks.readSp(owner) >>> 0, true);
                if (bits.getFloat32(0, true) > SP_THRESHOLD) state.highlight = now();
            }
            state.countdown = (state.countdown - 1) >>> 0;
        }
        if (!(hooks.firstPool ? hooks.firstPool() : state.firstSpPoolPresent)) state.highlight = now();
        if (((hooks.firstIndex ? hooks.firstIndex() : state.firstSpIndex) << 16 >> 16) === -1) state.highlight = now();
    }

    // 원본은 view 인수를 읽지 않는다. 배치 판정 실패의 부분 쓰기와 성공 쌍의 원래 깊이를 구분한다. 반환은 쓴 항목 수이다. (2026-11-13 WB04-E386)
    collect(view, output, limit) {
        const state = this.#cursor ? cursorPreviewCollectionInput(this.#cursor) : this.#state;
        const pool = this.#pool;
        const capacity = Math.floor(output.length / RENDER_ENTRY_BYTES);
        const data = new DataView(output.buffer, output.byteOffset, output.byteLength);
        if (state.lockDepth >>> 0) return 0;
        let count = 0;
        let remaining = limit | 0;
        // count 와 조각 SID 는 각 반복에서 현재 상태를 읽으며 실패 후보는 output 위치를 전진하지 않는다.
        for (let index = 0; index < (state.pieceCount | 0); index += 1) {
            if (remaining < 1) return count;
            if (index >= state.pieces.length) throw new RangeError('Preview collection list');
            const sid = cursorPreviewSid(state.pieces[index]);
            if (!sid) throw new RangeError('Preview collection zero SID');
            const raw = pool.slot(sid);
            if (count >= capacity) throw new RangeError('Preview collection output');
            const x = readFloat(raw, 14);
            const y = readFloat(raw, 18);
            const first = count * RENDER_ENTRY_BYTES;
            data.setFloat32(first + X, x, true);
            data.setFloat32(first + Y, y, true);
            data.setUint32(first + TOKEN, (this.#tokenBase + sid * STRIDE) >>> 0, true);
            if ((state.type >>> 0) >= this.#types.length) throw new RangeError('Preview collection type');
            const type = this.#types[state.type >>> 0];
            let depth = type.zOrder >>> 0;
            if (state.parentGump >>> 0) {
                if (!this.#hooks.gumpDepth) throw new TypeError('Preview collection parent depth');
                depth = (this.#hooks.gumpDepth(state.parentGump >>> 0) - 9) >>> 0;
            }
            if (type.flags2 & SURFACE) depth = 0xffffffff;
            if (type.flags2 & ISLAND) depth = (-20) >>> 0;
            data.setInt16(first + DEPTH, (depth & 0xffff) << 16 >> 16, true);
            if (raw[10] === (state.specialTypeA >>> 0) || raw[10] === (state.specialTypeB >>> 0)) {
                if (!(state.legal >>> 0)) continue;
                const spotX = spotCoordinate(state.x) >>> 0;
                const spotY = spotCoordinate(state.y) >>> 0;
                const spotIndex = (spotX + ((spotY << 8) >>> 0)) >>> 0;
                if (spotIndex >= this.#spots.length) throw new RangeError('Preview collection spot address');
                if (this.#spots[spotIndex] & 2) continue;
                data.setInt16(first + DEPTH, 20, true);
            }
            if (count + 2 > capacity) throw new RangeError('Preview collection pair capacity');
            this.#patchFeedback(state.localPlayer >>> 0);
            output[first + KIND] = KIND_PREVIEW;
            output[first + TAIL] = 0;
            if (index >= state.pieces.length) throw new RangeError('Preview collection list');
            const current = cursorPreviewSid(state.pieces[index]);
            if (!current || current >= pool.capacity) throw new RangeError('Preview collection current SID');
            const second = (count + 1) * RENDER_ENTRY_BYTES;
            data.setFloat32(second + X, x, true);
            data.setFloat32(second + Y, y, true);
            data.setUint32(second + TOKEN, (this.#tokenBase + current * STRIDE) >>> 0, true);
            data.setInt16(second + DEPTH, ((depth - 1) & 0xffff) << 16 >> 16, true);
            output[second + KIND] = KIND_PREVIEW;
            output[second + TAIL] = 2;
            count += 2;
            remaining -= 2;
        }
        return count;
    }

    // 대리 토큰을 같은 풀의 SID 로 되돌린다. 호스트 포인터로 해석하지 않는다. (2026-11-13 WB04-E386)
    object(token) {
        token >>>= 0;
        if (token < this.#tokenBase) throw new RangeError('Preview collection token');
        const offset = token - this.#tokenBase;
        if (!offset || offset % STRIDE || offset / STRIDE >= this.#pool.capacity) throw new RangeError('Preview collection token SID');
        return offset / STRIDE;
    }
}

// 부모 깊이 경계는 유지하고 SP 읽기만 초기화를 끝낸 실제 분산 저장소에 연결한 훅 사본을 만든다. (2026-11-13 WB04-E386)
export function makePreviewSpHooks(store, hooks = {}) {
    if (!store.initialized) throw new TypeError('Preview collection prepared SP store required');
    return { ...hooks, readSp: (owner) => store.get(owner) };
}

// 미리보기를 공통 수집기 묶음의 마지막 수집기(클립 전 영역을 그대로 받음)로 연결한다. (2026-11-13 WB04-E386)
export function bindPreviewCollection(hooks, collector) {
    hooks.preview = (view, output, limit) => collector.collect(view, output, limit);
}
