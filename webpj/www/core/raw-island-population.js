// 섬 배치 뒤의 거주 구역·초기 객체 생성과 record 후처리를 같은 실제 월드에 연결하는 모듈이다(cpppj `o/RawIslandPopulation.cpp/.h`, 10.78 전용: 0046df80 Residence·0046e000 Populate·0040e2b0 ReplaceInitial·004324e0 WriteRecord).
//  - residence(region)(0046df80→0040e130): 현재 지도에서 지역 사각형을 다시 얻고(부모가 준 record 6 BYTE 는 읽지 않음) 권한이 없으면 진단 뒤 현재 권한을 다시 읽어 없으면 반환한다.
//    world 지도 record 의 BYTE3×103 으로 로컬 난수를 만들어 기존 `RawFortFinishOwner.placeRandom` 으로 현재 거주 구역 TYPE 1개를 임의 배치한다.
//  - replaceInitial(area, region)(0040e2b0): 중앙 10×10 사각형(짧은 변의 원문 음수 대체식 포함)에 현재 primary TYPE 을 최대 10회, 모두 실패하면 fallback TYPE 을 최대 10회 배치한다(Fort 모드는 로컬 난수를 실제 벽시각 seed 로 교체).
//    결과가 없으면 진단, 있으면 위치 snapshot(삭제 전) → 가상 삭제(flags 0) → 교체 TYPE 생성 → 현재 TYPE+118 의 기본 프레임을 프레임 DWORD(+36)에 직접 쓰기 → HP 보유이면 현재 타입 HP getter 로 비가상 HP setter → 현재 지역 owner → 가상 Pop.
//  - populate(region)(0046e000): 입구의 사각형/record snapshot → 권한 진단 → residence → replaceInitial → record 의 bit 0x800 → writeRecord(대상 지도의 같은 지역 청크 전부에 6 BYTE 쓰기, 일치 청크가 없어도 마지막 생산 통지 실행).
// 지도의 실제 record 조회 `fortTerritoryIslandRecord`(00432430: y 바깥/x 안쪽 순서로 같은 섬의 첫 record 6 BYTE, 없으면 0)는 편집/저장 파일(`RawFortTerritorySave`, 웹 제외)에 있어 이 모듈에 지역 도우미로 둔다.
// 모든 자료는 같은 실제 풀이며 외부 하위(진단·벽시각·위치·삭제·생성·프레임·HP·owner·Pop·생산 갱신)는 선택한 위치에서만 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E223: 섬 population 이식으로 추가)

import { GameRandom } from './random.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 원문 c2/139/c3/d1/118 보고 뒤 현재 상태로 계속하는 진단 종류이다. (2026-11-10 WB04-E223)
export const ISLAND_POPULATION_ASSERTION = Object.freeze({
    boundsMissing: 'BoundsMissing',
    recordMissing: 'RecordMissing',
    residenceAuthority: 'ResidenceAuthority',
    populationAuthority: 'PopulationAuthority',
    missingPlacement: 'MissingPlacement',
});

// 프레임 DWORD 위치와 로컬 난수의 record 계수, 사각형 보정 상수, 한 TYPE 의 배치 시도 횟수이다. (2026-11-10 WB04-E223)
const FRAME_OFFSET = 36;
const SEED_FACTOR = 103;
const CENTER_SIZE = 10;
const ATTEMPTS = 10;
const RECORD_BYTES = 6;

// 선택된 효과의 부재만 그 위치에서 거부한다. (2026-11-10 WB04-E223)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('섬 population의 실제 하위 미연결');
    return callback;
}

// 0040d900 의 DWORD 감김 16배를 float32 로 저장한다. (2026-11-10 WB04-E223)
const cell = (value) => Math.fround((value << 4) | 0);

// 원문 _ftol64 절삭의 하위 DWORD(signed)이다. 비유한 값은 0 이다. (2026-11-10 WB04-E223)
function truncate(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 00432430: 같은 지도의 y 바깥/x 안쪽 순서 조회이며 축약 IslandList 나 Fort 파일의 과거 record 를 쓰지 않는다. 같은 섬의 첫 record 6 BYTE 사본을 돌려주고 없으면 0 이다. (2026-11-10 WB04-E223)
export function fortTerritoryIslandRecord(chunks, island) {
    for (let y = 0; y < chunks.yLength; y += 1) {
        for (let x = 0; x < chunks.xLength; x += 1) {
            const entry = chunks.at(x, y);
            if (((entry.island << 24) >> 24) === island) return Uint8Array.from(entry.record);
        }
    }
    return new Uint8Array(RECORD_BYTES);
}

// 0040e2b0 의 중앙 10×10 두 종류 사각형이다. 원본 00500fc8 은 10.0 이며 x 는 실제 fst32, y 는 x87 레지스터에 유지된다. 짧은 변의 원문 음수 대체식을 그대로 보존하고 네 _ftol 출력은 각 signed 정수를 다시 float32 로 저장한다. (2026-11-10 WB04-E223)
function center(area) {
    const horizontal = Math.fround(area.right - area.left - CENTER_SIZE);
    const vertical = area.bottom - area.top - CENTER_SIZE;
    const x = horizontal >= 0 ? horizontal : -area.right - area.left - CENTER_SIZE;
    const y = vertical >= 0 ? vertical : -area.bottom - area.top - CENTER_SIZE;
    return {
        left: Math.fround(truncate(area.left + x * 0.5)),
        top: Math.fround(truncate(area.top + y * 0.5)),
        right: Math.fround(truncate(area.right - x * 0.5)),
        bottom: Math.fround(truncate(area.bottom - y * 0.5)),
    };
}

// 섬 population 이다. pool 은 10.78 SidPool, target 은 배치/기록 대상 지도, world 는 실제 world 전역 지도(record 조회), islands 는 같은 풀의 RawIslandRuntime, placement 는 RawFortFinishOwner(기존 완료 단일 배치 몸체),
// globals = { boss(), fort(), residence(), primary(), fallback(), replacement() } 현재 전역 getter(권한·Fort 모드·거주 구역/primary/fallback/교체 TYPE),
// hooks = { assertion(종류), wallSeconds(), position(sid) → [x, y], destroy(sid, flags), create(type, flags) → sid, defaultFrame(type), typeHitPoints(type), hasHitPoints(sid), setOwner(sid, owner), pop(sid, x, y, flags), refreshProduction() } 이다.
// 사용: new RawIslandPopulation(pool, map, map, islands, placement, globals, hooks).populate(region). (2026-11-10 WB04-E223)
export class RawIslandPopulation {
    // 실제 풀, 배치/기록 대상 지도, world 지도, 섬 목록, 단일 배치 몸체, 현재 전역, 선택된 실제 장치이다.
    #pool;
    #target;
    #world;
    #islands;
    #placement;
    #globals;
    #hooks;

    // 모든 자료가 같은 실제 풀이어야 하며 더 오래 살아야 한다. (2026-11-10 WB04-E223)
    constructor(pool, target, world, islands, randomPlacement, globals, hooks = {}) {
        if (pool?.edition !== 'patch1078' || islands?.pool !== pool || randomPlacement?.pool !== pool || !(randomPlacement instanceof RawFortFinishOwner)) {
            throw new TypeError('섬 population의 실제 PATCH/Pool 불일치');
        }
        this.#pool = pool;
        this.#target = target;
        this.#world = world;
        this.#islands = islands;
        this.#placement = randomPlacement;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 현재 target 의 실제 bounds 와 원문 16배 DWORD 감김이다. 미존재 섬은 진단 뒤 현재 target 의 실제 청크를 읽으며 16/16/1/1 결과를 임의 사각형으로 바꾸지 않는다. (2026-11-10 WB04-E223)
    #bounds(region) {
        if (!this.#islands.exists(region)) need(this.#hooks.assertion)(ISLAND_POPULATION_ASSERTION.boundsMissing);
        const rect = this.#target.bounds(region);
        return { left: cell(rect.left), top: cell(rect.top), right: cell(rect.right), bottom: cell(rect.bottom) };
    }

    // 0040e130: 실제 world record 의 BYTE3×103 로컬 난수를 기존 완료 배치에 전달한다. 현재 TYPE 시점을 읽는다. (2026-11-10 WB04-E223)
    #residenceIn(area, region, quantity) {
        const record = fortTerritoryIslandRecord(this.#world, region);
        const random = new GameRandom((record[3] * SEED_FACTOR) >>> 0);
        this.#placement.placeRandom(random, area, region, quantity, this.#globals.residence() >>> 0, 0);
    }

    // 0046df80: 부모가 준 record 6 BYTE 는 읽지 않고 현재 지도 사각형을 다시 얻는다. 권한이 없으면 진단 뒤 현재 권한을 다시 읽는다. (2026-11-10 WB04-E223)
    residence(region) {
        const area = this.#bounds(region);
        if (!this.#globals.boss()) {
            need(this.#hooks.assertion)(ISLAND_POPULATION_ASSERTION.residenceAuthority);
            if (!this.#globals.boss()) return;
        }
        this.#residenceIn(area, region, 1);
    }

    // 원문 반환값을 조용히 성공 SID 로 바꾸지 않고 실제 WORD/Pool 물리 경계만 확인한다. 0 반환의 원문 진단 뒤에는 실제 SID 0 위치를 읽을 수 있다. (2026-11-10 WB04-E223)
    #candidate(value) {
        if (value > 0xffff || value >= this.#pool.capacity) throw new RangeError('섬 population의 실제 배치 반환 SID 범위');
        return value;
    }

    // 0040e2b0: 위치 snapshot → 삭제 → 새 생성 → frame/HP → 현재 owner → Pop 순서이다. (2026-11-10 WB04-E223)
    replaceInitial(area, region) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        const rectangle = center(area);
        const record = fortTerritoryIslandRecord(this.#world, region);
        const random = new GameRandom((record[3] * SEED_FACTOR) >>> 0);
        // 현재 Fort 에서만 실제 벽시각 seed 로 교체한다.
        if (globals.fort()) random.setState(RawFortFinishOwner.timeSeed(need(hooks.wallSeconds)()));
        let result = 0;
        // 타입은 각 시도에 현재 값을 재조회하고, 기본 10회가 모두 0 일 때만 대체 TYPE 을 읽는다.
        for (let attempt = 0; attempt < ATTEMPTS && !result; attempt += 1) result = this.#placement.placeRandom(random, rectangle, region, 1, globals.primary() >>> 0, 0);
        if (!result) for (let attempt = 0; attempt < ATTEMPTS && !result; attempt += 1) result = this.#placement.placeRandom(random, rectangle, region, 1, globals.fallback() >>> 0, 0);
        if (!result) need(hooks.assertion)(ISLAND_POPULATION_ASSERTION.missingPlacement);
        const position = need(hooks.position)(this.#candidate(result));
        need(hooks.destroy)(this.#candidate(result), 0);
        const created = need(hooks.create)(globals.replacement() >>> 0, 0);
        const raw = this.#pool.allocated(created);
        const type = raw[10];
        // 같은 새 객체의 현재 TYPE+118 을 직접 frame DWORD 에 쓴다(setter/표시 효과 없음).
        const frame = need(hooks.defaultFrame)(type) >>> 0;
        for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (frame >>> (index * 8)) & 0xff;
        // 판정 뒤 현재 타입 getter 를 다시 읽고 비가상 HP setter 를 쓴다.
        if (need(hooks.hasHitPoints)(created)) setRawSquidHitPoints(this.#pool, created, need(hooks.typeHitPoints)(this.#pool.slot(created)[10]) | 0);
        need(hooks.setOwner)(created, this.#islands.owner(region));
        need(hooks.pop)(created, position[0], position[1], 0);
    }

    // 004324e0: 현재 실제 폭/높이를 y/x 순서로 읽고 일치한 6 BYTE 만 쓴다. 마지막 생산 통지는 실제 일치 청크가 없어도 실행한다. (2026-11-10 WB04-E223)
    #writeRecord(region, record) {
        if (!this.#islands.exists(region)) need(this.#hooks.assertion)(ISLAND_POPULATION_ASSERTION.recordMissing);
        for (let y = 0; y < this.#target.yLength; y += 1) {
            for (let x = 0; x < this.#target.xLength; x += 1) {
                const entry = this.#target.at(x, y);
                if (((entry.island << 24) >> 24) === region) entry.record.set(record.subarray(0, RECORD_BYTES));
            }
        }
        need(this.#hooks.refreshProduction)();
    }

    // 0046e000: 외부 효과가 current record 를 바꿔도 원문 입구의 record snapshot 에 bit 0x800(두 번째 BYTE 의 8)만 켜서 쓴다. (2026-11-10 WB04-E223)
    populate(region) {
        const area = this.#bounds(region);
        const record = fortTerritoryIslandRecord(this.#target, region);
        if (!this.#globals.boss()) {
            need(this.#hooks.assertion)(ISLAND_POPULATION_ASSERTION.populationAuthority);
            if (!this.#globals.boss()) return;
        }
        this.residence(region);
        this.replaceInitial(area, region);
        record[1] |= 8;
        this.#writeRecord(region, record);
    }

    // 상위 실제 Builder/World 와 Pool 정체성을 검사한다. (2026-11-10 WB04-E223)
    get pool() { return this.#pool; }
}
