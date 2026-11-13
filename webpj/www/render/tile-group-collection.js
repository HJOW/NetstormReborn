// 지면/다리의 행별 표시 묶음 수집기를 옮긴 모듈이다(cpppj `client/TileGroupCollection.cpp/.h`, 10.78 00433410 ClientBoard 의 행 묶음 수집. CD 004884a0 경로는 범위 밖).
//  - collect(view, rowLimit, covered): 해시 0단계를 고른 뒤 월드 가시 사각형의 네 좌표를 `좌표 + 0.5`(double)의 CRT 64비트 절삭 하위 DWORD(signed)로 칸 번호로 바꾼다
//    (NaN·범위 밖은 FISTP 정수 indefinite 의 하위 DWORD 0, 상단은 255 로 제한). 왼쪽/위는 한 칸 줄이고 오른쪽/아래는 한 칸 넓혀(DWORD 감김) 각각 0~255 로 따로 제한한 뒤
//    y 바깥·x 안쪽 순서로 현재 단계(0) 칸의 머리 SID 하나만 읽는다. SID 가 0 이면 배경이 덮이지 않은 것(covered=false)이고, 아니면 슬롯 +14/+18 의 float x·y 바이트를 그대로 복사하고
//    +35 의 signed BYTE 를 깊이로, kind 1·tail 0 으로 16바이트 항목에 기록한다(토큰 `objectTokenBase + SID × 50`). 항목을 기록한 **뒤** 행 개수가 rowLimit 이상이면 오류이고(strict 한도),
//    개수가 0 이 아닌 행만 헤더(토큰 `groupTokenBase + 헤더 번호 × 8`, 첫 항목부터 count 개의 연속 보기와 `{ value }` 개수 상자)를 만들며 헤더 수가 `screenHeight/11 + 6` 이상이면 오류이다.
//    정상 반환 때만 해시 선택을 2 단계로 바꾸고 covered.value 를 기록한다(실패 전의 부분 출력과 0단계 선택은 남는다). 행 내부는 공통 정렬을 적용하지 않는다.
//  - group(token): 현재 수집의 헤더 토큰을 묶음 보기로 돌려준다(정렬된 kind 1 분배가 쓴다. 지난 프레임의 토큰은 거부). object(token): 객체 토큰을 같은 풀의 SID 로 되돌린다(기준 미만·stride 배수 아님·0·용량 이상은 거부).
//    entries(): 현재 수집의 연속 항목 배열 보기이다.
//  - bindTileGroupCollection(collection, dispatch, collector): 공통 수집기 훅의 `groups` 와 분배 훅의 `group` 을 이 수집기로 연결한다.
// 상태 객체: { screenHeight = 768, objectTokenBase = 0, groupTokenBase = 0x80000000 } — 화면 높이와 두 토큰 영역은 호출자가 겹치지 않게 정한다. 풀/해시는 수집기보다 오래 살아야 한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E394: 5-A 렌더러 프레임의 지면 묶음 수집기로 추가, 기준선 뒤 cpppj 변경 없음)
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';
import { WORLD_CELLS } from '../assets/terrain-mask.js';

// 월드 한 변의 칸 수에서 1 뺀 값(상단 제한)이다. (2026-11-13 WB04-E394)
const LAST_CELL = WORLD_CELLS - 1;
// 풀 슬롯 안의 위치: x float(14)·y float(18)·깊이 BYTE(35)이다. (2026-11-13 WB04-E394)
const SLOT_X = 14;
const SLOT_Y = 18;
const SLOT_DEPTH = 35;
// 16바이트 항목 안의 위치: token(8)·depth WORD(12)·kind(14)·tail(15)이다. (2026-11-13 WB04-E394)
const TOKEN = 8;
const DEPTH = 12;
const KIND = 14;
const TAIL = 15;
// 지면 묶음 항목 종류(공통 표시 목록의 kind 1)이다. (2026-11-13 WB04-E394)
const KIND_GROUP = 1;
// 헤더 토큰 한 칸의 간격과 32비트 토큰 공간이다. (2026-11-13 WB04-E394)
const GROUP_STRIDE = 8;
const TOKEN_SPACE = 0x100000000;
// 화면 높이의 상한이다(원본 `INT_MAX − 66`). (2026-11-13 WB04-E394)
const MAX_SCREEN_HEIGHT = 2147483647 - 66;

// 좌표 하나를 `+0.5` 와 CRT 64비트 절삭을 거쳐 확장 전 signed 칸 번호로 바꾼다. 입력은 float32 로 반올림하고, NaN·무한대·±2^63 밖은 정수 indefinite 의 하위 DWORD 0 이며 큰 유한 수는 하위 DWORD 를 보존한다.
// 결과는 255 이하로 제한한다. (2026-11-13 WB04-E394)
function cell(coordinate) {
    const value = Math.fround(coordinate) + 0.5;
    let low = 0;
    if (Number.isFinite(value) && value >= -(2 ** 63) && value < 2 ** 63) low = Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
    return Math.min(low, LAST_CELL);
}

// 칸 번호에 DWORD 덧셈(확장량)을 하고 0~255 로 제한한다. 32비트 감김을 그대로 재현한다. (2026-11-13 WB04-E394)
function expand(cellNumber, step) {
    return Math.min(Math.max((cellNumber + step) | 0, 0), LAST_CELL);
}

// 지면/다리의 행별 표시 묶음 수집기이다. 사용: const collector = new TileGroupCollection(hash, pool, { screenHeight: 768 }); const groups = collector.collect(view, 80, covered). (2026-11-13 WB04-E394)
export class TileGroupCollection {
    // 해시·풀·상태와 이번 수집의 연속 항목 저장소(바이트)·현재 항목 수·헤더 목록이다.
    #hash;
    #pool;
    #state;
    #storage;
    #count = 0;
    #groups = [];

    // 풀/해시 참조와 상태를 검증하고 모든 월드 칸을 담을 안정된 연속 저장소를 확보한다. 화면 높이와 토큰 영역이 잘못되면 거부한다.
    constructor(hash, pool, state = {}) {
        const resolved = { screenHeight: state.screenHeight ?? 768, objectTokenBase: state.objectTokenBase ?? 0, groupTokenBase: state.groupTokenBase ?? 0x80000000 };
        if (!Number.isInteger(resolved.screenHeight) || resolved.screenHeight < 0 || resolved.screenHeight > MAX_SCREEN_HEIGHT) throw new RangeError('Tile group screen height');
        for (const base of [resolved.objectTokenBase, resolved.groupTokenBase]) {
            if (!Number.isInteger(base) || base < 0 || base >= TOKEN_SPACE) throw new RangeError('Tile group token ranges');
        }
        const end = resolved.objectTokenBase + pool.capacity * pool.layout.stride;
        const groupEnd = resolved.groupTokenBase + WORLD_CELLS * GROUP_STRIDE;
        if (end > TOKEN_SPACE || groupEnd > TOKEN_SPACE || (resolved.objectTokenBase < groupEnd && resolved.groupTokenBase < end)) throw new RangeError('Tile group token ranges');
        this.#hash = hash;
        this.#pool = pool;
        this.#state = Object.freeze(resolved);
        this.#storage = new Uint8Array(WORLD_CELLS * WORLD_CELLS * RENDER_ENTRY_BYTES);
    }

    // 현재 수집의 가시 월드 사각형 `{ left, top, right, bottom }`(float)·행별 strict 한도·배경 결과 상자 `{ value }` 로 헤더 목록을 만든다. 반환 목록은 다음 수집까지 유효하며 재진입은 허용하지 않는다.
    collect(view, rowLimit, covered) {
        this.#count = 0;
        this.#groups = [];
        this.#hash.select(0);
        let allCovered = true;
        const left = expand(cell(view.left), 0xffffffff);
        const top = expand(cell(view.top), 0xffffffff);
        const right = expand(cell(view.right), 1);
        const bottom = expand(cell(view.bottom), 1);
        const maxRows = Math.floor(this.#state.screenHeight / 11) + 6;
        const stride = this.#pool.layout.stride;
        const data = new DataView(this.#storage.buffer, this.#storage.byteOffset, this.#storage.length);
        // 빈 행은 헤더를 만들지 않으며 뒤 행은 같은 연속 배열의 다음 위치에 기록한다.
        for (let y = top; y <= bottom; y += 1) {
            const start = this.#count;
            let count = 0;
            // 원본은 한 칸의 머리만 읽는다. next/type/state/실제 프레임 크기는 이 수집기의 필터가 아니다.
            for (let x = left; x <= right; x += 1) {
                const sid = this.#hash.at(x, y);
                if (!sid) {
                    allCovered = false;
                    continue;
                }
                const raw = this.#pool.slot(sid);
                const base = this.#count * RENDER_ENTRY_BYTES;
                this.#storage.set(raw.subarray(SLOT_X, SLOT_X + 4), base);
                this.#storage.set(raw.subarray(SLOT_Y, SLOT_Y + 4), base + 4);
                data.setUint32(base + TOKEN, (this.#state.objectTokenBase + sid * stride) >>> 0, true);
                data.setInt16(base + DEPTH, (raw[SLOT_DEPTH] << 24) >> 24, true);
                this.#storage[base + KIND] = KIND_GROUP;
                this.#storage[base + TAIL] = 0;
                this.#count += 1;
                count += 1;
                if (count >= rowLimit) throw new RangeError('Tile group count < rowLimit');
            }
            if (count > 0) {
                this.#groups.push({
                    token: (this.#state.groupTokenBase + this.#groups.length * GROUP_STRIDE) >>> 0,
                    entries: this.#storage.subarray(start * RENDER_ENTRY_BYTES, (start + count) * RENDER_ENTRY_BYTES),
                    count: { value: count },
                });
                if (this.#groups.length >= maxRows) throw new RangeError('Tile group listCount < MAX_TILE_ROWS');
            }
        }
        this.#hash.select(2);
        covered.value = allCovered;
        return this.#groups;
    }

    // 헤더 토큰을 현재 수집의 묶음 보기로 해석한다. 기준 미만·8 의 배수가 아님·현재 헤더 수 밖(지난 프레임)은 거부한다.
    group(token) {
        const base = this.#state.groupTokenBase;
        if (!Number.isInteger(token) || token < base || (token - base) % GROUP_STRIDE !== 0) throw new RangeError('Tile group token');
        const index = (token - base) / GROUP_STRIDE;
        if (index >= this.#groups.length) throw new RangeError('Tile group token');
        return this.#groups[index];
    }

    // 객체 토큰을 실제 SID 번호로 되돌린다. 기준 미만·stride 배수 아님·SID 0·풀 용량 이상은 거부한다.
    object(token) {
        const base = this.#state.objectTokenBase;
        if (!Number.isInteger(token) || token < base) throw new RangeError('Tile object token');
        const offset = token - base;
        const stride = this.#pool.layout.stride;
        if (offset % stride !== 0 || offset / stride === 0 || offset / stride >= this.#pool.capacity) throw new RangeError('Tile object SID');
        return offset / stride;
    }

    // 현재 수집의 연속 항목 배열(16바이트×개수) 보기이다. 다음 수집 때는 이전 내용을 쓸 수 없다.
    entries() {
        return this.#storage.subarray(0, this.#count * RENDER_ENTRY_BYTES);
    }
}

// 공통 수집과 종류 분배의 묶음 슬롯을 같은 수집기에 연결한다. 사용: bindTileGroupCollection(collectionHooks, dispatchHooks, collector). (2026-11-13 WB04-E394)
export function bindTileGroupCollection(collection, dispatch, collector) {
    // 가시 월드 범위·행별 제한·배경 관찰을 해시와 풀로 전달한다.
    collection.groups = (view, rowLimit, covered) => collector.collect(view, rowLimit, covered);
    // 정렬된 kind 1 은 같은 수집기의 현재 헤더와 내부 멤버를 쓴다.
    dispatch.group = (token) => collector.group(token);
}
