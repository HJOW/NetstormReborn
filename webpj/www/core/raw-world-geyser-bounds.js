// 전체 세계 간헐천 생성의 원본 청크 범위(00485bc0 전체·00485cc0 분할)·옵션 표(간격 0041ca10)·Player 예산(0041c8e0)을 옮긴 모듈이다(cpppj `o/RawWorldGeyserBounds.cpp/.h`, 10.78 전용).
// x 폭의 float 저장과 y 높이의 x87 배정도 유지, 원문의 현재 모드/옵션 재조회 순서를 보존하며 별도 가상 지도/예산을 소유하지 않는다.
//  - worldGeyserSpacing(selection): 옵션 BYTE 0~3 을 간격 3/4/5/6 으로 읽고 표 밖은 호스트 오류이다. worldGeyserBudget(battle, players, options): battle 일 때 Player 1~8 의 +0x2c 가 비영이고 숨김 BYTE(options[63+owner-1])가 0 인 수 n 으로 `(12−n)×n` 을 반환한다.
//  - RawWorldGeyserBounds.whole(): 청크 범위 지역 0 을 16 배(DWORD 좌표를 << 4 한 뒤 signed→float 저장)로 읽고, missionPost 이면 지역 1 을 읽어 1~255 로 보정한 점(현재 지도 한계 안의 사각형 점만 min/max 확장)으로 합친다.
//    아니면 현재 간격을 공유 여백 DWORD 에 `간격−2` 로 쓴 뒤 사각형을 왼쪽·위로 (여백×16−16), 오른쪽·아래로 (여백×16) 넓힌다(각 좌표는 float 저장).
//  - partition(cell, grid): 지역 0 을 한 번 조회한 뒤 missionPost 이거나 cell 이 grid² 를 넘으면 whole()(지역 0 재조회)이다. 아니면 같은 여백으로 넓히되 y 축은 float 저장 없이 배정도로 유지하고, 폭은 `|trunc(오른쪽−왼쪽)| / grid` 를 float 로 저장,
//    높이는 배정도로 유지해 cell 번호(왼쪽→오른쪽, 위→아래, 1 부터)의 열/행에 해당하는 구간 사각형을 돌려준다(0 이하 cell 은 전체 순회를 마친 다음의 열/행).
// state = { missionPost, mapLimit, spacingMargin }(모두 장기 전역의 live 속성; spacingMargin 은 쓰기 대상), hooks = { chunkBounds(region)→{ left, top, right, bottom }(signed 정수), spacing()→간격 }. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E107: 세계 간헐천 청크 범위/예산 이식으로 추가)

// Player 기록 간격과 존재 DWORD·숨김 옵션 위치, 호스트 반복 한도(손상된 입력의 수십억 반복 방지)이다. (2026-11-08 WB04-E107)
const PLAYER_STRIDE = 0xb4;
const PRESENCE_AT = 0x2c;
const HIDDEN_AT = 63;
const PLAYER_BYTES = 9 * PLAYER_STRIDE;
const OPTION_BYTES = 87;
const MAXIMUM_HOST_GRID = 4096;
const INT64_BOUNDARY = 2 ** 63;
const fround = Math.fround;

// 원본 CRT 는 signed64 절삭의 하위 DWORD(signed32)를 반환한다. 불확정값의 하위 DWORD 는 0 이다. (2026-11-08 WB04-E107)
function truncate(value) {
    if (!Number.isFinite(value) || value < -INT64_BOUNDARY || value >= INT64_BOUNDARY) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 실제 NEG 의 INT_MIN 감김도 보존한다. (2026-11-08 WB04-E107)
const absolute = (value) => (value < 0 ? (0 - value) | 0 : value);

// 실제 도달한 하위만 요구하며 앞선 공유 쓰기와 청크 조회를 되돌리지 않는다. (2026-11-08 WB04-E107)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('전체 간헐천 범위의 실제 하위 미연결');
    return hook;
}

// 비정렬 Player 필드의 정확한 네 BYTE 만 읽는다. (2026-11-08 WB04-E107)
const word = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;

// 원본의 스택 표는 BYTE 0~3 일 때만 정의된 네 항목이다. (2026-11-08 WB04-E107)
export function worldGeyserSpacing(selection) {
    const values = [3, 4, 5, 6];
    if (!(selection >= 0 && selection < values.length)) throw new RangeError('세계 간헐천 간격의 원문 표 범위');
    return values[selection];
}

// flags50/지역/AI 를 세는 다른 Player 관문과 합치지 않는다. battle 이 아니면 0 명이다. (2026-11-08 WB04-E107)
export function worldGeyserBudget(battle, players, options) {
    if (players.length !== PLAYER_BYTES || options.length !== OPTION_BYTES) throw new RangeError('세계 간헐천 예산의 PATCH Player/옵션 물리 범위');
    let count = 0;
    // 실제 Player 1~8 의 현재 +2c 를 먼저 읽고 필요한 숨김 BYTE 만 검사한다.
    if (battle) for (let owner = 1; owner <= 8; owner += 1) if (word(players, owner * PLAYER_STRIDE + PRESENCE_AT) && options[HIDDEN_AT + owner - 1] === 0) count += 1;
    return (12 - count) * count;
}

// 큰 지도 생성 부모 00486440 이 사용하는 실제 00485bc0/00485cc0 범위 계산기이다. (2026-11-08 WB04-E107)
export class RawWorldGeyserBounds {
    // 실제 모드/세계 크기/여백의 같은 장기 전역(live 속성)이다. (2026-11-08 WB04-E107)
    #state;
    // 현재 청크 지도와 옵션 getter 만 호출하는 외부 경계이다. (2026-11-08 WB04-E107)
    #hooks;

    // 현재 전역은 이 객체보다 오래 살아야 하며 값형 모드/지도 사본을 만들지 않는다. (2026-11-08 WB04-E107)
    constructor(state, hooks = {}) {
        this.#state = state;
        this.#hooks = hooks;
    }

    // ChunkMap 의 실제 결과를 clamp 하지 않고 DWORD 16배/단정도 저장한다(0040d900 의 각 좌표 변환). (2026-11-08 WB04-E107)
    #cells(region) {
        const raw = need(this.#hooks.chunkBounds)(region);
        const convert = (value) => fround((value << 4) | 0);
        return { left: convert(raw.left), top: convert(raw.top), right: convert(raw.right), bottom: convert(raw.bottom) };
    }

    // 선택된 현재 간격을 공유 DWORD 에 쓴 뒤 원본 signed 칸 여백을 반환한다(좌표 수정보다 먼저 쓴다). (2026-11-08 WB04-E107)
    #padding() {
        this.#state.spacingMargin = (need(this.#hooks.spacing)() - 2) | 0;
        return (this.#state.spacingMargin << 4) | 0;
    }

    // 0040e880/0040e8f0 의 입력 1~255 clamp·현재 점 유효성·min/max 순서이다(NaN 비교 결과도 그대로). (2026-11-08 WB04-E107)
    #extend(rectangle, x, y) {
        x = x < 1 ? 1 : 255 < x ? 255 : x;
        y = y < 1 ? 1 : 255 < y ? 255 : y;
        // 0040e800 의 두 축 열린 범위(상한은 현재 지도 한계)이다.
        const valid = (px, py) => {
            const bound = fround(this.#state.mapLimit | 0);
            return px > 0 && px < bound && py > 0 && py < bound;
        };
        if (!valid(rectangle.left, rectangle.top)) { rectangle.left = x; rectangle.top = y; }
        else { rectangle.left = x < rectangle.left ? x : rectangle.left; rectangle.top = y < rectangle.top ? y : rectangle.top; }
        if (!valid(rectangle.right, rectangle.bottom)) { rectangle.right = x; rectangle.bottom = y; }
        else { rectangle.right = rectangle.right < x ? x : rectangle.right; rectangle.bottom = rectangle.bottom < y ? y : rectangle.bottom; }
    }

    // 첫 청크 조회가 바꾼 현재 missionPost/옵션도 이 분기에 그대로 반영한다. (2026-11-08 WB04-E107)
    whole() {
        const result = this.#cells(0);
        if (this.#state.missionPost) {
            const other = this.#cells(1);
            this.#extend(result, other.left, other.top);
            this.#extend(result, other.right, other.bottom);
        } else {
            const padding = this.#padding();
            const leading = (padding - 16) | 0;
            result.left = fround(result.left - leading);
            result.top = fround(result.top - leading);
            result.right = fround(result.right + padding);
            result.bottom = fround(result.bottom + padding);
        }
        return result;
    }

    // 원본은 분할 fallback 에서도 이미 지역 0 을 한 번 조회했으며 whole 에서 다시 조회한다. (2026-11-08 WB04-E107)
    partition(cell, grid) {
        const rectangle = this.#cells(0);
        const count = Math.imul(grid, grid);
        if (this.#state.missionPost || cell > count) return this.whole();
        if (grid < 1 || grid > MAXIMUM_HOST_GRID) throw new RangeError('세계 간헐천 분할의 호스트 반복 범위');
        const padding = this.#padding();
        const leading = (padding - 16) | 0;
        rectangle.left = fround(rectangle.left - leading);
        // 원문 y 축은 이 시점에 float 로 저장하지 않는다.
        const top = rectangle.top - leading;
        const bottom = rectangle.bottom + padding;
        rectangle.right = fround(rectangle.right + padding);
        const inverse = 1 / grid;
        const width = fround(absolute(truncate(rectangle.right - rectangle.left)) * inverse);
        const height = absolute(truncate(bottom - top)) * inverse;
        let column = 1;
        let row = 1;
        // 0 이하 cell 은 원문처럼 전체 순회를 마친 다음의 열/행을 사용한다.
        for (let current = 1; current <= count; current += 1) {
            if (column > grid) { column = 1; row += 1; }
            if (current === cell) break;
            column += 1;
        }
        return {
            left: fround((column - 1) * width + rectangle.left),
            top: fround((row - 1) * height + top),
            right: fround(rectangle.right - (grid - column) * width),
            bottom: fround(bottom - (grid - row) * height)
        };
    }
}
