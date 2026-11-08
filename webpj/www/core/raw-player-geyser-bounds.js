// 로컬 간헐천이 쓰는 Player 의 청크 배치 사각형과 칸 단위 확장(0048f5c0 → 00432430 → 00432050 → 0040d900)을 옮긴 모듈이다(cpppj `o/RawPlayerGeyserBounds.cpp/.h`, 10.78 전용).
//  - bounds(player): Player flags(+0x50) 에 everLoggedIn/AI(0x22)가 없으면 진단 → 현재 지역 DWORD(+0x78)의 첫 일치 청크 record 6 BYTE 를 값으로 복사 → metamap 이면 진단 →
//    placement → 청크 좌표 DWORD<<4 를 signed 로 읽어 float 로 바꾼 뒤 양끝에 4.0 을 빼고/더한다.
//  - regionRecord(region): 청크 지도의 y 행/x 열 순서로 signed 섬 BYTE == region 인 첫 항목의 record 6 BYTE(없으면 0 레코드).
//  - originPlacement(record): 원점 (6,6)에 현재 패턴의 실제 폭/높이를 더한다(record 위치 nibble 은 쓰지 않음).
//  - placement(player, record): 전투가 아니면 진단, Player 1~8 밖이면 진단(복귀 뒤 계속) → 미션이면 패턴 폭/높이와 위치 nibble(+1), 아니면 현재 mapSize(0~3 → 간격 3/4/5/6)와
//    현재 방향표의 이웃 칸(북부터 시계 방향, 홀수 방향은 대각선 보정)으로 3×3 배치를 만든다. 진단이 바꾼 모드/방향표/TYPE 은 뒤의 조회가 다시 읽는다.
//  - 패턴 폭/높이: 호출 시점 puzzlePiece TYPE 의 프레임 표로 CanonDecoder(territory.js)를 만들고(누락 프레임 진단 MissingFrame), 그 뒤의 현재 TYPE 발자국으로 범위를 구한다.
// 상태(state)는 { battle, mission, metamap, mapSize, puzzlePieceType, playerDirections(9칸), decoderDiagnostics } 를 매번 다시 읽는 같은 현재 전역 객체이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E57: Player 간헐천 범위 이식으로 추가)
import { CanonDecoder } from '../assets/territory.js';
import { TERRITORY_PATTERNS } from '../assets/territory-patterns.js';
import { NEIGHBOR_CELLS } from '../assets/terrain-mask.js';

// 원본 assert 위치이다(값은 cpppj enum 순서). 진단 뒤 현재 공유 상태로 계속한다. (2026-11-08 WB04-E57)
export const PLAYER_GEYSER_BOUNDS_ASSERTION = Object.freeze({ inactivePlayer: 0, metamap: 1, notBattle: 2, invalidPlayer: 3, missingFrame: 4 });

// Player 원문 폭·flags/지역 필드·활성 비트, 실제 TYPE 수, 칸 확장값(ROM 00502314), 원점·전투 중심·mapSize 간격표이다. (2026-11-08 WB04-E57)
const PLAYER_BYTES = 0xb4;
const FLAGS_OFFSET = 0x50;
const REGION_OFFSET = 0x78;
const ACTIVE_FLAGS = 0x22;
const TYPE_COUNT = 188;
const EXPANSION = 4.0;
const ORIGIN = 6;
const BATTLE_CENTER = 9;
const SPACING = Object.freeze([3, 4, 5, 6]);

// 비정렬 little endian DWORD 를 읽는다. (2026-11-08 WB04-E57)
function dword(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// 정수 청크 좌표의 DWORD<<4 를 signed 로 해석한 float 이다. (2026-11-08 WB04-E57)
const cell = (value) => Math.fround((value << 4) | 0);

// Player 간헐천 범위 처리기이다. 같은 raw 풀·청크 지도·TYPE/프레임 표·Player 9기록과 현재 상태를 빌린다(임의 섬 범위를 받지 않는다).
// 사용: const bounds = new RawPlayerGeyserBounds(pool, chunks, types, frames, players, state, assertion); bounds.bounds(1) → { left, top, right, bottom }. (2026-11-08 WB04-E57)
export class RawPlayerGeyserBounds {
    // 판본과 상위 연결의 같은 풀 정체성이다.
    #pool;
    // 현재 지역 record 를 직접 읽는 실제 청크 지도(ChunkMap)이다.
    #chunks;
    // 현재 puzzlePiece 발자국을 읽는 실제 TYPE 표(188칸)이다.
    #types;
    // 같은 번호의 실제 Canon 프레임 코드 표(188칸)이다.
    #frames;
    // 현재 flags 와 지역 DWORD 를 읽는 원문 Player 9기록이다.
    #players;
    // 현재 모드/옵션/타입/방향표의 단일 저장소이다.
    #state;
    // 원본 진단의 실제 소비자이다(누락 불가).
    #assertion;

    // 10.78 풀과 물리 표 크기, 진단 장치를 확인한다.
    constructor(pool, chunks, types, frames, players, state, assertion) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Player 간헐천 범위는 10.78 SID 풀만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== TYPE_COUNT) throw new RangeError('Player 간헐천 범위의 현재 원본 TYPE/프레임 표 크기 오류');
        if (!(players instanceof Uint8Array) || players.length !== 9 * PLAYER_BYTES) throw new RangeError('Player 간헐천 범위의 원문 9기록 크기 오류');
        if (typeof chunks?.at !== 'function' || !Number.isInteger(chunks.xLength) || !Number.isInteger(chunks.yLength)) throw new TypeError('Player 간헐천 범위에는 실제 청크 지도가 필요합니다.');
        if (typeof state !== 'object' || state === null || typeof assertion !== 'function') throw new TypeError('Player 간헐천 범위의 현재 상태/진단 누락');
        this.#pool = pool;
        this.#chunks = chunks;
        this.#types = types;
        this.#frames = frames;
        this.#players = players;
        this.#state = state;
        this.#assertion = assertion;
    }

    // 상위 바인딩이 같은 풀/청크 지도를 연결했는지 확인할 때 쓴다(사본이 아니다).
    get pool() { return this.#pool; }
    get map() { return this.#chunks; }

    // 00432430: 지도 전체를 원본 순서로 보고 첫 일치에서만 record 6 BYTE 를 복사한다.
    regionRecord(region) {
        const chunks = this.#chunks;
        const wanted = region | 0;
        // 원본은 먼저 (0,0)의 유효 getter 를 부르지만 실제 ChunkMap 은 생성부터 유효하며 최소 1×1 이다. y 행을 바깥으로 돈다.
        for (let y = 0; y < chunks.yLength; y += 1) {
            // 이 행의 x 를 먼저 늘리며 signed BYTE 를 지역 DWORD 와 비교한다.
            for (let x = 0; x < chunks.xLength; x += 1) {
                const entry = chunks.at(x, y);
                if (((entry.island << 24) >> 24) === wanted) return Uint8Array.from(entry.record.subarray(0, 6));
            }
        }
        return new Uint8Array(6);
    }

    // 같은 record 의 실제 CanonDecoder 와 현재 TYPE 발자국에서 배치 폭/높이를 얻는다. 최초 프레임 검색/진단도 생략하지 않는다.
    #dimensions(record) {
        const type = this.#state.puzzlePieceType >>> 0;
        const shape = record[0] & 63;
        if (type >= this.#types.length || shape >= TERRITORY_PATTERNS.length) throw new RangeError('Player 범위의 현재 영역 타입/패턴 물리 범위');
        const decoder = new CanonDecoder(this.#frames[type], TERRITORY_PATTERNS[shape], (record[0] >> 6) * 2, 0, 0,
            () => this.#assertion(PLAYER_GEYSER_BOUNDS_ASSERTION.missingFrame), this.#state.decoderDiagnostics ?? null);
        // 원본 004bfd70 → 00425b90 의 같은 발자국 기반 결과이며 진단 뒤의 현재 발자국을 읽는다.
        const bounds = decoder.bounds(this.#types[type].footX, this.#types[type].footY);
        return [(bounds[2] - bounds[0]) | 0, (bounds[3] - bounds[1]) | 0];
    }

    // 00431ff0: 원점 (6,6)에 현재 패턴의 실제 폭/높이를 더한다.
    originPlacement(record) {
        const size = this.#dimensions(record);
        return { left: ORIGIN, top: ORIGIN, right: (ORIGIN + size[0]) | 0, bottom: (ORIGIN + size[1]) | 0 };
    }

    // 0041ca10: 매 호출 현재 mapSize BYTE 로 간격 [3,4,5,6] 을 고른다. 표 밖은 오류이다.
    #spacing() {
        const size = this.#state.mapSize;
        if (!(size >= 0 && size < SPACING.length)) throw new RangeError('Player 배치의 현재 mapSize 물리 범위');
        return SPACING[size];
    }

    // 00432050: 미션의 실제 패턴/위치 또는 전투의 현재 옵션/방향 배치이다. 진단이 현재 모드/방향표를 바꿔도 뒤의 조회는 다시 읽는다.
    placement(player, record) {
        const state = this.#state;
        const number = player | 0;
        if (!state.battle) this.#assertion(PLAYER_GEYSER_BOUNDS_ASSERTION.notBattle);
        if (number < 1 || number > 8) this.#assertion(PLAYER_GEYSER_BOUNDS_ASSERTION.invalidPlayer);
        if (state.mission) {
            const size = this.#dimensions(record);
            // 현재 위치 nibble 은 원본 기본 left/top 1 에 더한다.
            const x = (record[2] & 15) + 1;
            const y = (record[2] >> 4) + 1;
            return { left: x, top: y, right: (x + size[0]) | 0, bottom: (y + size[1]) | 0 };
        }
        const directions = state.playerDirections;
        if (number < 0 || number >= directions.length) throw new RangeError('Player 배치의 방향표 물리 범위');
        const direction = directions[number] >>> 0;
        const even = (direction & ~1) >>> 0;
        if (even >= NEIGHBOR_CELLS.length) throw new RangeError('Player 배치의 현재 방향 물리 범위');
        // ROM 0052f83c/52f85c 의 북부터 시계 방향 이웃이다.
        const [dx, dy] = NEIGHBOR_CELLS[even];
        let x = (BATTLE_CENTER + Math.imul(this.#spacing(), dx)) | 0;
        let y = (BATTLE_CENTER + Math.imul(this.#spacing(), dy)) | 0;
        if (direction & 1) {
            x = (x - Math.imul(this.#spacing(), dy)) | 0;
            y = (y + Math.imul(this.#spacing(), dx)) | 0;
        }
        return { left: (x - 2) | 0, top: (y - 2) | 0, right: x, bottom: y };
    }

    // 0048f5c0: flags 진단 → 현재 지역 record 복사 → PlayerId 진단 → 배치 → 좌표 저장/확장 순서이다. 반환은 float32 사각형이다.
    bounds(player) {
        const index = player >>> 0;
        if (index >= 9) throw new RangeError('Player 간헐천 범위의 원문 9기록 경계');
        const offset = index * PLAYER_BYTES;
        if (!(this.#players[offset + FLAGS_OFFSET] & ACTIVE_FLAGS)) this.#assertion(PLAYER_GEYSER_BOUNDS_ASSERTION.inactivePlayer);
        // PlayerId 진단 전에 원본처럼 record 를 값으로 보존한다.
        const record = this.regionRecord(dword(this.#players, offset + REGION_OFFSET) | 0);
        if (this.#state.metamap) this.#assertion(PLAYER_GEYSER_BOUNDS_ASSERTION.metamap);
        const rectangle = this.placement(index, record);
        return {
            left: Math.fround(cell(rectangle.left) - EXPANSION),
            top: Math.fround(cell(rectangle.top) - EXPANSION),
            right: Math.fround(cell(rectangle.right) + EXPANSION),
            bottom: Math.fround(cell(rectangle.bottom) + EXPANSION),
        };
    }
}
