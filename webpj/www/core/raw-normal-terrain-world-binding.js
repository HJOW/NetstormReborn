// 일반 지형 성장과 초기 도장·섬별 edgeFarm 후처리를 같은 raw 표면 월드의 소비자로 조립한다(cpppj `client/RawNormalTerrainWorldBinding.cpp/.h`, 10.78 전용).
// 사용: const binding = new RawNormalTerrainWorldBinding(world, chunks, islands, random, crtBox, contextGlobals, stampState, placementState, farmState, puzzlePieceBox, placementPipeline, reward, carrier, diagnostics);
//   binding.generate({ owner, bounds: {left,top,right,bottom}, selectedRegion, branch: NormalTerrainBranch.Normal }) 는 Context → 성장 → MarkEdges → Stamp → Normalize → context 해제 → 섬별 군집 순서이다.
//   binding.bindGenerationInputs(inputs); binding.generateOriginal(owner, bounds, selectedRegion) 는 실제 상위(004c1510): 후보 캐시 → context → 현재 옵션에 따라 군도 또는 일반 → 도장/정규화 → 해제 → 군집이다.
//   binding.postprocessFarms(region) 은 이미 초기 지면을 등록한 월드에서 섬별 후처리만, binding.patternCount(record) 는 현재 puzzlePiece 프레임과 CanonDecoder 가 통과시킨 패턴 칸만 센다.
// 기존에 검증한 모듈(RawTerrainContext/Growth/Stamp, RawFortRandomPlacement, RawFarmCluster)을 다시 쓰지 않고 같은 저장소·난수·가상 효과로 실행하는 client 소비자이다. 모든 참조는 소비자보다 오래 살아야 하며
// 상자(`{ value }`)로 받는 전역(crt·puzzlePiece·군도 DWORD·span 등)은 매 사용 위치에서 현재 값을 읽는다. 군도·미복원 부모는 일반 지형으로 대신하지 않고 거부한다.
// 기준선 뒤 cpppj 변경(`f7f58f5a`: placementState 필드 접근자 `IslandType()`/`EdgeFarmType()`)은 웹 상태가 이미 같은 현재 값 함수(`currentIslandType`/`currentEdgeFarmType`)로 읽는 것과 같다(3.6절 ①).
// CD(10.72) 분기는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E302: 4-B RawNormalTerrainWorldBinding 이식)
import { TERRITORY_PATTERNS } from '../assets/territory-patterns.js';
import { CanonDecoder } from '../assets/territory.js';
import { makeOwnerDispatch } from './raw-bridge-connect.js';
import { RawFarmCluster, makeFarmClusterPlacementHook } from './raw-farm-cluster.js';
import { RawFortRandomPlacement, makeFortRandomPlacementHooks } from './raw-fort-random-placement.js';
import { RawTerrainContext } from './raw-terrain-context.js';
import { RawTerrainGrowth } from './raw-terrain-growth.js';
import { RawTerrainStamp } from './raw-terrain-stamp.js';
import { makeTerrainStampWorldHooks } from './raw-terrain-world-binding.js';
import { typeConstructorAddress } from './squid-constructors.js';

// 호출자가 실제 상위 분기를 확인한 결과이다. 군도와 미복원 부모는 정상 지형으로 대체하지 않는다. (2026-11-11 WB04-E302)
export const NormalTerrainBranch = Object.freeze({
    // 004c1510 의 일반 성장 분기를 실행한다.
    Normal: 'normal',
    // 섬 확장·플레이어 배치를 포함하므로 이 소비자의 generate 는 거부한다.
    Archipelago: 'archipelago',
    // 전체 Fort/IslandBuilder 로드 완료로 해석할 수 없는 요청을 거부한다.
    UnrestoredParent: 'unrestoredParent',
});

// 기존 Bounds/첫 record helper 가 보고하는 진단 구분이다. 패치 판본은 Bounds 앞 현재 Exists 검사만 보고한다. (2026-11-11 WB04-E302)
export const TerrainFarmPostpassAssertion = Object.freeze({
    // 두 판본의 Bounds 앞 현재 Exists 검사가 실패했다.
    BoundsMissingIsland: 'boundsMissingIsland',
});

// 지형 상위가 쓰는 맵 한 변 청크 수 상한(15)과 첫 청크 가운데 오프셋 등 원문 상수이다. (2026-11-11 WB04-E302)
const MAX_CHUNK = 15;
// 플레이어 수와 군도 root 가 각 플레이어 배치 뒤 쓰는 6BYTE 기록(원문 0x107/WORD 0 의 정확한 6바이트)이다. (2026-11-11 WB04-E302)
const PLAYER_COUNT = 8;
const PLAYER_RECORD = Uint8Array.of(7, 1, 0, 0, 0, 0);

// DWORD 비트를 부호 있는 정수로 해석한다(원문 signed 산술 앞의 해석). (2026-11-11 WB04-E302)
const signedWord = (value) => value | 0;
// 0 방향 정수 나눗셈이다(원문 idiv). (2026-11-11 WB04-E302)
const idiv = (value, divisor) => Math.trunc(value / divisor) | 0;

// 필수 효과가 함수가 아니면 도달한 위치에서 오류를 낸다(빈 성공으로 대신하지 않음). (2026-11-11 WB04-E302)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// 저장 상자 하나를 읽고 쓰는 함수 쌍으로 바꿔 상태 객체의 shared 접근자에 넘긴다. 같은 상자인지 비교할 수 있게 상자 정체성을 함수에 붙인다. (2026-11-11 WB04-E302)
function boxReader(box) {
    const read = () => box.value;
    read.box = box;
    return read;
}

// 기존 전체 배치·Factory·HP getter 를 현재 ChunkMap/섬 owner·가상 공간/owner 에 연결한다. 배치 파이프라인은 이 world.core() 의 types/frames/shapes/spots/hash/장부로 구성해야 한다.
// 섬 런타임이나 HP 장부가 다른 월드이면 효과 전에 거부한다. 반환한 훅은 월드·섬 런타임·파이프라인보다 오래 살아서는 안 된다. (2026-11-11 WB04-E302)
export function makeNormalTerrainFarmWorldHooks(world, chunks, islands, placement, reward) {
    const core = world.core();
    if (core.pool !== islands.pool || core.bookkeeping !== reward.bookkeeping) throw new TypeError('일반 지형 Farm의 실제 섬/HP 장부 불일치');
    const hooks = makeFortRandomPlacementHooks(core.pool, placement, core.factory, reward, {});
    // 청크 지도의 현재 섬 BYTE 를 signed 로 읽는다.
    hooks.chunkRegion = (x, y) => (chunks.at(x, y).island << 24) >> 24;
    // 현재 섬 목록의 원래 owner 이다.
    hooks.regionOwner = (region) => islands.owner(region);
    // 지면/121 의 실제 가상 소유자 규칙을 월드 SquidOwner 에 분배한다.
    hooks.setOwner = makeOwnerDispatch(core.pool, core.bridgeState, (sid, player) => core.owner.set(sid, player));
    // 월드의 실제 Unpop/Repop/Pop 이다.
    hooks.unpop = (sid, flags) => world.unpop(sid, flags);
    hooks.repop = (sid, flags) => world.repop(sid, flags);
    hooks.pop = (sid, x, y, flags) => world.pop(sid, x, y, flags);
    return hooks;
}

// 일반 지형 상위(004c1510)가 쓰는 현재 전역/장치 묶음의 검사이다. 군도 분기가 도달할 때만 필요한 장치는 그 위치에서 요구한다. (2026-11-11 WB04-E302)
function checkInputs(inputs) {
    if (!inputs || typeof inputs.archipelago?.value !== 'number' || typeof inputs.span?.value !== 'number') throw new TypeError('지형 root 의 군도/확장 상자 누락');
    return inputs;
}

// 일반 지형 소비자이다. 내부 군집 훅이 이 소비자가 소유한 단일 배치를 빌리므로 한 번 만든 객체를 그대로 쓴다. (2026-11-11 WB04-E302)
export class RawNormalTerrainWorldBinding {
    // 월드·월드 Core 참조·청크 지도·섬 런타임·난수와 CRT 상자이다.
    #world;
    #core;
    #chunks;
    #islands;
    #random;
    #crt;
    // 문자 대응 전역·초기 99 캐시 상태·배치/군집 전역·puzzlePiece 타입 상자·호출자 진단이다.
    #contextGlobals;
    #stampState;
    #placementState;
    #farmState;
    #puzzlePiece;
    #diagnostics;
    // 상위 현재 전역/장치(미연결이면 null)와 완료된 두 배치 몸체 인스턴스이다.
    #generation = null;
    #single;
    #farms;

    // 완료된 단일 배치/군집과 실제 Damageable 후처리를 같은 월드에 명시 연결한다. 다른 월드의 Carrier·누락된 필수 진단은 월드를 바꾸기 전에 거부한다.
    // diagnostics = { context, stamp, region, farm, postpass, decoderDiagnostics? } (함수 다섯 개 필수). (2026-11-11 WB04-E302)
    constructor(world, chunks, islands, random, crt, contextGlobals, stampState, placementState, farmState, puzzlePiece, placement, reward, carrier, diagnostics) {
        const core = world.core();
        if (core.pool !== carrier.pool || !diagnostics?.context || !diagnostics.stamp || !diagnostics.region || !diagnostics.farm || !diagnostics.postpass) {
            throw new TypeError('일반 지형 실제 후처리/필수 진단 누락');
        }
        if (typeof crt?.value !== 'number' || typeof puzzlePiece?.value !== 'number') throw new TypeError('일반 지형의 CRT/puzzlePiece 상자 누락');
        this.#world = world;
        this.#core = core;
        this.#chunks = chunks;
        this.#islands = islands;
        this.#random = random;
        this.#crt = crt;
        this.#contextGlobals = contextGlobals;
        this.#stampState = stampState;
        this.#placementState = placementState;
        this.#farmState = farmState;
        this.#puzzlePiece = puzzlePiece;
        this.#diagnostics = diagnostics;
        this.#requireStorage();
        const hooks = makeNormalTerrainFarmWorldHooks(world, chunks, islands, placement, reward);
        this.#single = new RawFortRandomPlacement(core.pool, core.hash, core.spots, core.types, core.frames, random, placementState, hooks);
        this.#farms = new RawFarmCluster(core.pool, chunks, islands, placementState, farmState, makeFarmClusterPlacementHook(core.pool, this.#single, { assertion: diagnostics.farm }));
        core.postPop.setBody('carrier', carrier);
    }

    // 군도와 서로 다른 원본 전역을 묵시적인 정상 지형/지역 기본값으로 바꾸지 않는다. (2026-11-11 WB04-E302)
    #requireNormal() {
        if (this.#farmState.isArchipelago()) throw new Error('일반 전용 지형 요청에 군도 상태를 사용할 수 없습니다');
        this.#requireStorage();
    }

    // 현재 원본 모드의 선택과 분리하여 같은 실제 TYPE/지역/생성자 연결을 확인한다: 지면 TYPE·edge TYPE·noRegion 전역과 두 실제 생성자 주소가 일치해야 한다. (2026-11-11 WB04-E302)
    #requireStorage() {
        const globals = this.#contextGlobals;
        if (globals.islandType !== this.#placementState.currentIslandType() || globals.islandType !== this.#stampState.islandType
            || globals.edgeType !== this.#stampState.edgeType || globals.noRegion !== this.#stampState.noRegion) {
            throw new TypeError('일반 지형 같은 타입/지역 전역 연결 오류');
        }
        // 지면 97 의 실제 생성자와 121 의 실제 Damageable 생성자는 임의 0 슬롯으로 대신하지 않는다.
        for (const type of [globals.islandType, this.#placementState.currentEdgeFarmType()]) {
            if (type >= this.#core.types.length || this.#core.types[type].constructorAddress !== typeConstructorAddress(type)) throw new TypeError('일반 지형 실제 타입 생성자 연결 누락');
        }
    }

    // 원본 일반 context 의 수명은 초기 도장과 정규화까지이며 군집 postpass 전에 끝난다. 마지막 초기 지면 SID 는 관찰 편의용이다. (2026-11-11 WB04-E302)
    generate(request) {
        if (request.branch !== NormalTerrainBranch.Normal) throw new Error('군도/전체 부모를 일반 지형으로 실행할 수 없습니다');
        this.#requireNormal();
        const { bounds } = request;
        if (bounds.left < 1 || bounds.top < 1 || bounds.right > MAX_CHUNK || bounds.bottom > MAX_CHUNK || bounds.left >= bounds.right || bounds.top >= bounds.bottom) {
            throw new RangeError('일반 지형 소비자의 유효 내부 청크 범위가 필요합니다');
        }
        const context = new RawTerrainContext('patch1078', request.owner, request.selectedRegion, bounds, this.#contextGlobals, this.#diagnostics.context);
        new RawTerrainGrowth(context, this.#chunks, this.#contextGlobals).runNormal();
        const last = this.#stampContext(context);
        this.#postprocessFarms(request.selectedRegion);
        return last;
    }

    // context 의 수명을 넘겨 보관하지 않고 원문 세 후처리(MarkEdges → Stamp → Normalize)를 같은 실제 월드에 실행하고 마지막 SID 를 돌려준다. (2026-11-11 WB04-E302)
    #stampContext(context) {
        context.markEdges(0x78, 0x3d);
        const effects = {
            chunkRegion: (x, y) => (this.#chunks.at(x, y).island << 24) >> 24,
            assertion: this.#diagnostics.stamp,
        };
        const stamp = new RawTerrainStamp(this.#core.pool, this.#core.types, this.#core.frames, context.map, this.#random, this.#crt, this.#stampState,
            makeTerrainStampWorldHooks(this.#world, this.#stampState, this.#diagnostics.region, effects));
        stamp.stamp(0x78);
        context.normalizeAfterStamp();
        return context.map.lastSid.value;
    }

    // 군집이 빌린 현재 두 전역과 root 가 쓰는 owner 가 다르면 연결 시점에 거부한다. inputs 의 archipelago/span 은 `{ value }` 상자이고,
    // enabled/battle/mission 은 현재 DWORD 를 읽는 함수, layout()/spacing()/subscriber(p)/placement(p, record)/logPercent(n) 은 군도 분기가 도달할 때만 필요하다.
    // minimum/maximum/percent 는 `{ value }` 상자, growth 는 첫 시드 getter 의 `{ checking?, assertion? }` 이다. (2026-11-11 WB04-E302)
    bindGenerationInputs(inputs) {
        checkInputs(inputs);
        const farm = this.#farmState;
        if ((farm.sharedArchipelago && farm.sharedArchipelago.box !== inputs.archipelago) || (farm.sharedArchipelagoSpan && farm.sharedArchipelagoSpan.box !== inputs.span)) {
            throw new TypeError('지형 root와 군집의 같은 군도/확장 owner 불일치');
        }
        farm.sharedArchipelago = boxReader(inputs.archipelago);
        farm.sharedArchipelagoSpan = boxReader(inputs.span);
        this.#generation = inputs;
    }

    // 후보는 constructor 보다 먼저 캐시하고 PATCH layout/두 spacing 은 constructor 뒤 현재값을 읽는다. 군도 후보이면 확장 범위·흩뿌리기·플레이어 배치·gap 정리를 거치고, 아니면 일반 성장이다. (2026-11-11 WB04-E302)
    generateOriginal(owner, bounds, selectedRegion) {
        if (!this.#generation) throw new Error('실제 지형 생성 상위의 현재 전역/장치 미연결');
        const input = this.#generation;
        // 후보 조건은 현재 세 DWORD 와 섬 수를 이 순서로 읽는다(앞이 거짓이면 뒤를 읽지 않는다).
        const candidate = Boolean(input.enabled() && input.battle() && !input.mission() && this.#islands.count === 1);
        this.#requireStorage();
        const context = new RawTerrainContext('patch1078', owner, selectedRegion, bounds, this.#contextGlobals, this.#diagnostics.context);
        const growth = new RawTerrainGrowth(context, this.#chunks, this.#contextGlobals, input.growth ?? {});
        const layout = need(input.layout, '지형 root의 현재 layout 미연결');
        const archipelago = layout() === 0 && candidate;
        if (archipelago) {
            const spacing = need(input.spacing, '지형 root의 두 현재 spacing 조회 미연결');
            // 첫 조회로 확장폭, 두 번째 조회로 span 을 따로 읽는다(첫 결과를 재사용하지 않음).
            const extension = signedWord(spacing() - 2);
            const second = spacing();
            input.span.value = second === 3 ? 1 : second === 4 ? 2 : 3;
            context.sharedArchipelagoMode().value = 1;
            const before = context.chunks();
            context.setBounds({ left: signedWord(before.left - extension + 1), top: signedWord(before.top - extension + 1), right: signedWord(before.right + extension), bottom: signedWord(before.bottom + extension) });
            const scatterWidth = (Math.imul(input.span.value >>> 0, 2) + 2) >>> 0;
            growth.scatter(idiv(signedWord(Math.imul(Math.imul(scatterWidth, scatterWidth), 0x55)), 0x19));
            const seed = { value: Math.imul(growth.firstChunkSeed(), 0x1bb) >>> 0 };
            const chunks = context.chunks();
            const area = Math.imul(Math.imul(signedWord(chunks.bottom - chunks.top), signedWord(chunks.right - chunks.left)), 0x100) >>> 0;
            let target;
            if (input.percent.value === 0) {
                const high = idiv(signedWord(Math.imul(input.maximum.value >>> 0, area)), 200);
                const low = idiv(signedWord(Math.imul(input.minimum.value >>> 0, area)), 200);
                const first = growth.nextRange(seed, low, high);
                const secondRange = growth.nextRange(seed, low, high);
                target = ((first >>> 0) + (secondRange >>> 0)) >>> 0;
            } else target = idiv(signedWord(Math.imul(input.percent.value >>> 0, area)), 100) >>> 0;
            const logPercent = need(input.logPercent, '지형 root의 실제 백분율 로그 장치 미연결');
            if (!area) throw new RangeError('지형 root의 원문 백분율0 나눗셈');
            logPercent(idiv(signedWord(Math.imul(target, 100)), signedWord(area)));
            growth.growRectangle(0, context.chunks(), signedWord(target), seed);
            const subscriber = need(input.subscriber, '지형 root의 현재 Player 장치 미연결');
            // 각 반복에서 현재 Player base 의 subscriber 를 다시 읽는다.
            for (let player = 1; player <= PLAYER_COUNT; player += 1) {
                if (!subscriber(player)) continue;
                const placement = need(input.placement, '지형 root의 선택된 실제 배치 장치 미연결');
                growth.clearPlacement(placement(player, PLAYER_RECORD));
            }
            growth.fixGaps(1);
            growth.removeSparse();
        } else growth.runNormal();
        const last = this.#stampContext(context);
        if (archipelago) input.archipelago.value = 1;
        this.#postprocessCurrentFarms(selectedRegion);
        return last;
    }

    // 첫 record 는 Y 행/X 청크 순서로 읽는다. 패치는 못 찾으면 원본의 영 레코드를 돌려준다. (2026-11-11 WB04-E302)
    #firstRecord(region) {
        // Y 청크가 바깥이며 각 행의 X 청크를 처음부터 찾는다.
        for (let y = 0; y < this.#chunks.yLength; y += 1) {
            // 원본은 처음 일치한 record 6BYTE 만 반환한다.
            for (let x = 0; x < this.#chunks.xLength; x += 1) {
                const entry = this.#chunks.at(x, y);
                if (((entry.island << 24) >> 24) === region) return entry.record;
            }
        }
        return new Uint8Array(6);
    }

    // 004bfde0: 현재 puzzlePiece 프레임과 기존 CanonDecoder 가 통과한 패턴 칸만 센다. 번호가 표 밖이면 거부한다. (2026-11-11 WB04-E302)
    patternCount(record) {
        const type = this.#puzzlePiece.value >>> 0;
        if (type >= this.#core.frames.length) throw new RangeError('일반 지형 현재 puzzlePiece 타입 범위 오류');
        const pattern = record[0] & 0x3f;
        if (pattern >= TERRITORY_PATTERNS.length) throw new RangeError('일반 지형 패턴 번호 범위 오류');
        const decoder = new CanonDecoder(this.#core.frames[type].codes, TERRITORY_PATTERNS[pattern], (record[0] >> 6) * 2, 0, 0, () => {}, this.#diagnostics.decoderDiagnostics ?? null);
        let count = 0;
        // 원본 constructor 의 최초 유효 칸을 포함하고 advance 가 끝날 때까지 센다.
        for (; decoder.valid; decoder.advance()) count += 1;
        return count;
    }

    // 이미 초기 지면을 등록한 실제 월드에서 상위의 섬별 후처리만 실행한다. 군도 상태는 거부한다. (2026-11-11 WB04-E302)
    postprocessFarms(selectedRegion) {
        this.#requireNormal();
        this.#postprocessCurrentFarms(selectedRegion);
    }

    // generate 가 쓰는 같은 섬별 후처리의 진입점이다(일반 검사는 이미 마쳤다). (2026-11-11 WB04-E302)
    #postprocessFarms(selectedRegion) {
        this.#postprocessCurrentFarms(selectedRegion);
    }

    // root 의 도장/context 해제 뒤 현재 군도 전역을 읽는 같은 군집 몸체를 실행한다. 현재 섬 수와 noRegion 을 매 반복 읽으며 삭제된 항목을 별도로 거르지 않는다. (2026-11-11 WB04-E302)
    #postprocessCurrentFarms(selectedRegion) {
        this.#requireStorage();
        // 원본 상위처럼 count 는 unsigned 저장값을 signed DWORD 로 해석하며 회마다 다시 읽는다.
        for (let region = 0; region < signedWord(this.#islands.count); region += 1) {
            if (selectedRegion !== this.#contextGlobals.noRegion && selectedRegion !== region) continue;
            const record = this.#firstRecord(region);
            if (!this.#islands.exists(region)) this.#diagnostics.postpass(TerrainFarmPostpassAssertion.BoundsMissingIsland);
            const bounds = this.#chunks.bounds(region);
            const cells = { left: Math.fround(bounds.left * 16), top: Math.fround(bounds.top * 16), right: Math.fround(bounds.right * 16), bottom: Math.fround(bounds.bottom * 16) };
            this.#farms.place(cells, region, this.patternCount(record));
        }
    }

    // 같은 월드에 연결했는지 검사할 실제 풀이다(모듈 내부의 복사/모형 풀이 아니라 월드의 현재 풀). (2026-11-11 WB04-E302)
    get pool() {
        return this.#core.pool;
    }
}
