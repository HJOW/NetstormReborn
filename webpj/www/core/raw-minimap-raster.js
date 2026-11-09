// 미니맵 셀 그림(004808d0)의 지면/해시 순서와 팔레트 BYTE 쓰기를 같은 월드에 적용하는 모듈이다(cpppj `o/RawMinimapRaster.cpp/.h`, 10.78 전용).
//  - createMinimapRasterState(options): 셀 그림이 매 호출 읽는 현재 전역을 만든다. { isleType 97, noIslandType 157(지면 WORD 에서 색을 표시하는 두 TYPE),
//    battleIslandType 109, playerIslandType 108(Challenge 2×2 표식), challenge·suppressColors·frame(현재 모드/색 억제/억제 시 색으로 쓰는 프레임 DWORD),
//    black·neutralIsle·neutralNoIsland·challengeGreen(원본 검색으로 얻은 색), owners[8](소유자 1..8 의 미니맵 전용 색) } 이다. 호출자가 값을 바꾸면 다음 그림이 그 값을 읽는다.
//  - new RawMinimapRaster(pool, hash, types, state): pool 은 10.78 SidPool, hash 는 같은 월드의 SquidHash(지면 WORD = 0단계 머리), types 는 TYPE 번호로 찾는 레코드 배열({ flags2 })이다.
//    모두 처리기보다 오래 살아야 하며 복사하지 않는다.
//  - drawCell(bitmap, cellX, cellY): bitmap 은 128×128 Uint8Array(RawMetadisplayDirtyState 의 같은 저장소)이다. 셀(0..63)의 2×2 를 검정으로 초기화 → 지면 4×4(Y 바깥/X 안쪽, 0단계 머리만)
//    → 해시 2단계 체인 → 1단계 2×2(X 바깥/Y 안쪽) → 0단계 4×4(X 바깥/Y 안쪽) 체인을 원본 순서로 덮는다. 일반 모드는 flags2&6 이면 좌표를 먼저 절삭하고 &4 일 때 소유자 색 한 점,
//    Challenge 모드는 1·0단계 체인의 두 표식 TYPE 만 짝수 정렬 2×2 를 그린다. 좌표는 float → signed64 하위 DWORD 절삭(비유한/범위 밖 0)이다.
//  - bitmap/셀 범위 밖·픽셀 범위 밖·TYPE 범위 밖은 RangeError, 손상된 next 순환은 Error 다(원본 메모리 손상 대신 명시 거부).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E204: 미니맵 셀 그림 이식으로 추가)

// 원본 미니맵 행 폭과 셀 수이다(월드 256칸을 두 칸당 한 픽셀로 그린다). (2026-11-09 WB04-E204)
const WIDTH = 128;
const CELLS = 64;

// 슬롯의 next·TYPE·좌표·소유자 위치이다. (2026-11-09 WB04-E204)
const NEXT_AT = 4;
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;

// 일반 모드의 표시 플래그(flags2)와 소유자 없음 기본 색이다. (2026-11-09 WB04-E204)
const VISIBLE_FLAGS = 6;
const DOT_FLAG = 4;
const DEFAULT_OWNER_COLOR = 246;

// x87 ftol 의 signed64 범위이다. (2026-11-09 WB04-E204)
const INT64_LIMIT = 2 ** 63;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E204)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 셀 그림이 매 호출 읽는 현재 전역의 기본값(cpppj MinimapRasterState)이다. (2026-11-09 WB04-E204)
export function createMinimapRasterState({
    isleType = 97, noIslandType = 157, battleIslandType = 109, playerIslandType = 108,
    challenge = 0, suppressColors = 0, frame = 0,
    black = 0, neutralIsle = 0, neutralNoIsland = 0, challengeGreen = 0, owners = new Array(8).fill(0),
} = {}) {
    return { isleType, noIslandType, battleIslandType, playerIslandType, challenge, suppressColors, frame, black, neutralIsle, neutralNoIsland, challengeGreen, owners: owners.slice() };
}

// 좌표를 float → signed64 하위 DWORD 로 절삭한다. 원본 integer-indefinite 의 하위 DWORD 는 0 이다. (2026-11-09 WB04-E204)
function coordinate(raw, at) {
    FLOAT_VIEW.setUint32(0, (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0, true);
    const value = FLOAT_VIEW.getFloat32(0, true);
    if (!Number.isFinite(value) || value < -INT64_LIMIT || value >= INT64_LIMIT) return 0;
    return value | 0;
}

// 단일 픽셀은 BYTE 만 쓰고 현재 bitmap 의 실제 폭 128 을 쓴다. 손상 입력이 버퍼 밖을 쓰지 않도록 거부한다. (2026-11-09 WB04-E204)
function pixel(bitmap, x, y, color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= WIDTH) throw new RangeError('미니맵 픽셀 좌표 범위 오류');
    bitmap[y * WIDTH + x] = color & 0xff;
}

// 미니맵 한 셀(2×2 픽셀)을 같은 월드의 지면/해시에서 그리는 처리기이다. 사용: new RawMinimapRaster(pool, hash, types, state).drawCell(bitmap, x, y). (2026-11-09 WB04-E204)
export class RawMinimapRaster {
    // 실제 50바이트 슬롯 풀·공간 해시·TYPE flags2 표·현재 색/모드 전역이다.
    #pool;
    #hash;
    #types;
    #state;

    // 10.78 풀과 비어 있지 않은 TYPE 표를 요구한다. (2026-11-09 WB04-E204)
    constructor(pool, hash, types, state) {
        if (pool?.edition !== 'patch1078' || !hash || !Array.isArray(types) || types.length === 0 || !state) throw new TypeError('미니맵 셀 그림의 실제 10.78 Pool/TYPE 가 필요합니다');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
    }

    // 소유자 색은 원본의 1..8 switch/default 를 유지하며 억제 전역을 매번 읽는다(억제 시 현재 프레임 하위 BYTE). (2026-11-09 WB04-E204)
    #ownerColor(owner, fallback) {
        const state = this.#state;
        if (state.suppressColors) return state.frame & 0xff;
        return (owner >= 1 && owner <= 8 ? state.owners[owner - 1] : fallback) & 0xff;
    }

    // 현재 체인 next 를 각 효과 뒤에 읽으며 원본의 특수 모드/표면 플래그를 따른다. (2026-11-09 WB04-E204)
    #chain(bitmap, head, challengeMarks) {
        const state = this.#state;
        let visited = 0;
        // next 는 같은 슬롯의 +4 WORD 이며 지면/상태/소유자 추가 필터를 넣지 않는다.
        for (let sid = head; sid;) {
            visited += 1;
            if (visited > this.#pool.capacity) throw new Error('미니맵 해시 체인의 순환');
            // 효과 직전 현재 슬롯이다.
            const raw = this.#pool.slot(sid);
            if (!state.challenge) {
                if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('미니맵 해시의 TYPE 범위 오류');
                const flags = this.#types[raw[TYPE_AT]].flags2 >>> 0;
                if ((flags & VISIBLE_FLAGS) !== 0) {
                    // 원본은 island 에서도 먼저 좌표를 절삭한다.
                    const x = coordinate(raw, X_AT);
                    const y = coordinate(raw, Y_AT);
                    if (flags & DOT_FLAG) pixel(bitmap, (x / 2) | 0, (y / 2) | 0, this.#ownerColor(raw[OWNER_AT], DEFAULT_OWNER_COLOR));
                }
            } else if (challengeMarks && (raw[TYPE_AT] === state.battleIslandType || raw[TYPE_AT] === state.playerIslandType)) {
                // signed /2 뒤 각 축 bit0 만 지운다.
                const x = ((coordinate(raw, X_AT) / 2) | 0) & ~1;
                const y = ((coordinate(raw, Y_AT) / 2) | 0) & ~1;
                // playerIsland 는 소유자 5 칸(자홍) 전역을 쓴다.
                const color = state.suppressColors ? state.frame : raw[TYPE_AT] === state.battleIslandType ? state.challengeGreen : state.owners[4];
                pixel(bitmap, x, y, color);
                pixel(bitmap, x + 1, y, color);
                pixel(bitmap, x, y + 1, color);
                pixel(bitmap, x + 1, y + 1, color);
            }
            sid = raw[NEXT_AT] | (raw[NEXT_AT + 1] << 8);
        }
    }

    // 전체 Refresh 는 이 몸체를 행 우선으로 부르며 한 셀 안의 원본 순서는 여기서 보존한다. (2026-11-09 WB04-E204)
    drawCell(bitmap, cellX, cellY) {
        if (!bitmap || bitmap.length !== WIDTH * WIDTH || !Number.isInteger(cellX) || !Number.isInteger(cellY) || cellX < 0 || cellX >= CELLS || cellY < 0 || cellY >= CELLS) {
            throw new RangeError('미니맵 bitmap/셀 범위 오류');
        }
        const state = this.#state;
        const px = cellX * 2;
        const py = cellY * 2;
        // 초기화는 색 억제와 무관하게 검정 전역이다.
        const black = state.black & 0xff;
        pixel(bitmap, px, py, black);
        pixel(bitmap, px + 1, py, black);
        pixel(bitmap, px, py + 1, black);
        pixel(bitmap, px + 1, py + 1, black);
        // 지면은 Y 바깥/X 안쪽으로 4×4 를 읽어 마지막 지면의 색이 같은 픽셀을 덮게 한다.
        for (let y = cellY * 4; y < cellY * 4 + 4; y += 1) {
            // 같은 0단계의 현재 머리만 지면으로 읽으며 next 를 따라가지 않는다.
            for (let x = cellX * 4; x < cellX * 4 + 4; x += 1) {
                const sid = this.#hash.cell(0, x, y);
                if (!sid) continue;
                const raw = this.#pool.slot(sid);
                const type = raw[TYPE_AT];
                if (type === state.isleType || type === state.noIslandType) {
                    pixel(bitmap, (x / 2) | 0, (y / 2) | 0, this.#ownerColor(raw[OWNER_AT], type === state.isleType ? state.neutralIsle : state.neutralNoIsland));
                }
            }
        }
        this.#chain(bitmap, this.#hash.cell(2, cellX, cellY), false);
        // 1단계는 X 바깥/Y 안쪽의 2×2 버킷 순서다.
        for (let x = px; x < px + 2; x += 1) {
            // 현재 버킷의 체인을 끝낸 뒤 다음 Y 로 이동한다.
            for (let y = py; y < py + 2; y += 1) this.#chain(bitmap, this.#hash.cell(1, x, y), true);
        }
        // 0단계는 X 바깥/Y 안쪽의 4×4 버킷 순서다. 지면 전용 순서와 다르다.
        for (let x = cellX * 4; x < cellX * 4 + 4; x += 1) {
            // 이미 그린 지면도 원본과 같은 일반 flags 검사를 거친다.
            for (let y = cellY * 4; y < cellY * 4 + 4; y += 1) this.#chain(bitmap, this.#hash.cell(0, x, y), true);
        }
    }

    // 같은 월드 계약에 쓰는 풀이다. (2026-11-09 WB04-E204)
    get pool() {
        return this.#pool;
    }
}
