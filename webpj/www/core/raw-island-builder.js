// 실제 영역 패턴을 현재 ChunkMap 과 3204 BYTE 섬 목록에 원본 순서로 놓는 모듈이다(cpppj `o/RawIslandBuilder.cpp/.h`, 10.78 전용: 0046dba0 Place·0046dd70 PlaceAll).
//  - place(record, territory, ignorePosition)(0046dba0): 현재 builder 청크 경계 네 필드 중 하나가 -1 이면 진단(InvalidBounds) 뒤 계속한다. 레코드 6 BYTE 의 위치 nibble(하위 x·상위 y)을 경계의 left/top 에 더해(DWORD 감김, float32 저장) 시작 좌표를 만들고,
//    월드 지도일 때만 실제 섬 목록에 `add(owner, territory)` 로 등록해 지역 번호를 얻는다(임시 지도는 전달한 지역을 유지). 현재 puzzlePiece 타입의 프레임 표와 영역 패턴(레코드 하위 6비트)·방향(상위 2비트)으로 `CanonDecoder` 를 만들어
//    유효 칸마다 지도 항목(섬 번호 BYTE·레코드 6 BYTE·순번·방향 글자)을 쓴다. 패치는 칸 좌표가 지도 밖이면 진단(ChunkRange) 뒤 getter 가 가장자리로 당긴다. 월드 지도이면 끝에서 섬 목록의 존재 진단(BoundsMissingIsland) 뒤 청크 경계를 16배 float 로 저장한다.
//  - placeAll(records, start, end)(0046dd70): 물리 20레코드(120 BYTE) 밖 입력은 쓰기 전에 거부하고, start==end 이면 진단(EqualRange) 뒤 빈 순회, 각 레코드의 현재 flag(bit0 켜짐·bit1 꺼짐)를 읽어 [start,end) 순서로 place 를 부른다(정렬 없음).
//  - inWorld(): 대상 지도와 world 지도의 객체 동일성만 비교한다(같은 크기의 사본을 world 로 해석하지 않음).
// frames 는 타입별 프레임 코드 배열(`{ side, variant, number, flags }`) 188칸, puzzlePieceType 은 현재 타입 전역 getter, state = { bounds: { left, top, right, bottom }, owner, decoderDiagnostics } 이다.
// 진단은 `diagnostic(종류)` 로 보고하고 빈 효과로 성공을 대신하지 않는다. CD 10.72 분기(OddDirection·BoundsNotFound)는 웹 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E222: 섬 Builder 이식으로 추가)

import { CanonDecoder } from '../assets/territory.js';
import { TERRITORY_PATTERNS } from '../assets/territory-patterns.js';

// 원본 helper 의 보고 후 계속 진행을 보존하는 진단 종류이다(CD 전용 OddDirection·BoundsNotFound 는 옮기지 않음). (2026-11-10 WB04-E222)
export const ISLAND_BUILDER_ASSERTION = Object.freeze({
    invalidBounds: 'InvalidBounds',
    equalRange: 'EqualRange',
    missingFrame: 'MissingFrame',
    chunkRange: 'ChunkRange',
    boundsMissingIsland: 'BoundsMissingIsland',
});

// 원본 Territory 레코드와 실제 20개 레코드 저장소의 바이트 수, 프레임 표 크기이다. (2026-11-10 WB04-E222)
const RECORD_BYTES = 6;
const TERRITORY_BYTES = 120;
const TERRITORY_COUNT = 20;
const TYPE_COUNT = 188;

// 원본 DWORD 감김 16배를 float 로 저장한다(signed shift 의 호스트 미정의 동작을 피함). (2026-11-10 WB04-E222)
const cellBoundary = (value) => Math.fround((value << 4) | 0);

// 원본 시작 좌표의 DWORD 감김 덧셈과 float32 저장이다. (2026-11-10 WB04-E222)
const origin = (value, offset) => Math.fround(((value >>> 0) + offset) | 0);

// decoder float 의 실제 _ftol 하위 DWORD 를 사용하여 INT_MAX 부근의 float 반올림도 보존한다. (2026-11-10 WB04-E222)
function toChunkCoordinate(value) {
    if (!Number.isFinite(value)) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 섬 Builder 이다. target 은 놓을 지도(ChunkMap), world 는 실제 월드 지도, islands 는 같은 풀의 RawIslandRuntime, diagnostic(종류) 은 필수이다.
// 사용: new RawIslandBuilder(world, world, islands, frames, () => piece, state, diagnostic).placeAll(territoryBytes). (2026-11-10 WB04-E222)
export class RawIslandBuilder {
    // 원본 this+0 의 배치 지도, 실제 world 전역 지도, count/exists/owner/원래 owner/경계를 실제 3204 BYTE 에 기록하는 섬 목록, 타입 프레임 표, 현재 puzzlePiece 타입 getter, 현재 청크 범위/owner 상태, 진단 장치이다.
    #target;
    #world;
    #islands;
    #frames;
    #puzzlePieceType;
    #state;
    #diagnostic;

    // 판본별 타입 수와 필수 진단만 확인한다. (2026-11-10 WB04-E222)
    constructor(target, world, islands, frames, puzzlePieceType, state, diagnostic) {
        if (islands?.pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || typeof puzzlePieceType !== 'function' || typeof diagnostic !== 'function') {
            throw new TypeError('실제 IslandBuilder 판본/진단 연결 오류');
        }
        this.#target = target;
        this.#world = world;
        this.#islands = islands;
        this.#frames = frames;
        this.#puzzlePieceType = puzzlePieceType;
        this.#state = state;
        this.#diagnostic = diagnostic;
    }

    // 원본 inWorld 는 대상 지도 주소와 world 전역 지도 주소만 비교한다. (2026-11-10 WB04-E222)
    inWorld() { return this.#target === this.#world; }

    // 패치의 한 combined 좌표 진단과 기존 getter 끝 clamp 를 구분한다. (2026-11-10 WB04-E222)
    #chunkAt(x, y) {
        if (x < 0 || y < 0 || x >= this.#target.xLength || y >= this.#target.yLength) this.#diagnostic(ISLAND_BUILDER_ASSERTION.chunkRange);
        return this.#target.at(x, y);
    }

    // 0046dba0: 원본 Place 는 record.high2 를 그대로 전달한다(패턴 Count 의 high2×2 를 이 몸체에 적용하지 않음). record 는 6 BYTE 이상, 지역 번호를 돌려준다. (2026-11-10 WB04-E222)
    place(record, territory, ignorePosition = false) {
        const state = this.#state;
        const bounds = state.bounds;
        if (bounds.left === -1 || bounds.top === -1 || bounds.right === -1 || bounds.bottom === -1) this.#diagnostic(ISLAND_BUILDER_ASSERTION.invalidBounds);
        const offsetX = ignorePosition ? 0 : record[2] & 15;
        const offsetY = ignorePosition ? 0 : record[2] >> 4;
        let region = territory >>> 0;
        if (this.inWorld()) region = this.#islands.add(state.owner, territory & 0xff);
        const x = origin(bounds.left, offsetX);
        const y = origin(bounds.top, offsetY);
        const type = this.#puzzlePieceType() >>> 0;
        const pattern = record[0] & 63;
        const direction = record[0] >> 6;
        if (type >= this.#frames.length) throw new RangeError('실제 IslandBuilder 현재 puzzlePiece 타입 범위 오류');
        if (pattern >= TERRITORY_PATTERNS.length) throw new RangeError('실제 IslandBuilder 영역 패턴 범위 오류');
        const decoder = new CanonDecoder(this.#frames[type], TERRITORY_PATTERNS[pattern], direction, x, y,
            () => this.#diagnostic(ISLAND_BUILDER_ASSERTION.missingFrame), state.decoderDiagnostics ?? null);
        let ordinal = 0;
        // 실제 decoder 의 최초 유효 칸을 포함하고 프레임이 없는 칸은 완료된 advance 가 건너뛴다.
        for (; decoder.valid; decoder.advance(), ordinal += 1) {
            const entry = this.#chunkAt(toChunkCoordinate(decoder.x), toChunkCoordinate(decoder.y));
            entry.island = region & 0xff;
            // 원본 record 6 BYTE 만 복사하며 방향/ordinal 은 별도 필드에 기록한다.
            for (let byte = 0; byte < RECORD_BYTES; byte += 1) entry.record[byte] = record[byte];
            entry.ordinal = ordinal;
            entry.side = decoder.side;
        }
        if (this.inWorld()) {
            const number = region | 0;
            if (!this.#islands.exists(number)) this.#diagnostic(ISLAND_BUILDER_ASSERTION.boundsMissingIsland);
            const chunks = this.#target.bounds(number);
            this.#islands.setBounds(number, { left: cellBoundary(chunks.left), top: cellBoundary(chunks.top), right: cellBoundary(chunks.right), bottom: cellBoundary(chunks.bottom) });
        }
        return region >>> 0;
    }

    // 0046dd70: 물리 20레코드 밖의 손상 메모리는 읽지 않고 그 안의 원본 빈/역순 범위는 그대로 처리한다. 원본은 각 레코드의 현재 flag 를 다시 읽고 이름/지역 순서로 정렬하지 않는다. (2026-11-10 WB04-E222)
    placeAll(records, start = 0, end = TERRITORY_COUNT) {
        if (records.length < TERRITORY_BYTES || start < 0 || end < 0 || start > TERRITORY_COUNT || end > TERRITORY_COUNT) throw new RangeError('실제 IslandBuilder Territory 물리 20레코드 범위 오류');
        if (start === end) this.#diagnostic(ISLAND_BUILDER_ASSERTION.equalRange);
        for (let territory = start; territory < end; territory += 1) {
            const record = records.subarray(territory * RECORD_BYTES, (territory + 1) * RECORD_BYTES);
            if ((record[1] & 1) && !(record[1] & 2)) this.place(record, territory, false);
        }
    }

    // 같은 pool 의 실제 Runtime 소비자임을 월드 바인딩에서 확인하는 풀이다. (2026-11-10 WB04-E222)
    get pool() { return this.#islands.pool; }
}
