// 12차 스커미시가 미션 Start 직전에 MP 규칙으로 월드를 만드는 Session 연결이다(cpppj `RawCampaignSession.cpp` 의 ReadSkirmishTerritory·BuildBattleIsland·PopulateCentralResidences·BuildCentralIsland·BuildSkirmishWorld, 10.78 전용).
// 사용: const skirmish = new RawCampaignSessionSkirmish(owner); skirmish.buildSkirmishWorld() — 서비스(`o.services.ai.skirmishWorld`)가 계획을 주면 실행하고, 일반 미션(계획 없음)은 아무 일도 하지 않는다.
//  - buildCentralIsland(record): 중앙섬(owner 0 원점 배치)을 RawCentralIsland 로 만든다. 거주 구역·군도 분리는 같은 Session 의 RawIslandPopulation·RawIslandPartition 을 쓴다.
//  - buildBattleIsland(owner, sourceStorage, territoryChoice): 참가자의 지역 Template 슬롯(owner)에서 영역과 객체를 읽어 RawBattleIsland 로 시작 섬을 만든다. 고른 영역만 0x04 전투 플래그가 켜진 첫 영역이 된다.
//  - buildSkirmishWorld(): ① 객체 소유자·링 배치·Chaff 규칙을 MP(비미션)로 잠시 바꾸고(예외가 나도 되돌림) ② 맵 크기·군도 밀도(참가자 5명 이상이면 5~8%)를 정하고 ③ 중앙섬 ④ 참가자 1..N+1 의 시작 섬(지역 Template 에 fort 읽기→섬 만들기→슬롯 반납) 순서이다.
//    간헐천은 이 함수가 아니라 이어지는 randGeysers(전체 세계 간헐천 생성기)가 만든다.
// owner 는 RawCampaignSessionOwners 이며 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1105: 12차 스커미시 월드 생성 연결)
import { RawBattleIsland } from './raw-battle-island.js';
import { RawCentralIsland } from './raw-central-island.js';
import { RawIslandPartition } from './raw-island-partition.js';
import { RawIslandPopulation } from './raw-island-population.js';
import { getSquidRegion, setSquidRegion, SQUID_REGION_ASSERT } from './raw-squid-region.js';

// 필수 하위 장치가 없으면 그 위치에서 거부한다. (2026-11-20 WB04-E1105)
function need(value, message) {
    if (typeof value !== 'function') throw new Error(message);
    return value;
}
// 군도 밀도 옵션(0..3)의 최소/최대 표이다(0041c9b0/0041c9e0). (2026-11-20 WB04-E1105)
const DENSITY_MINIMUM = Object.freeze([10, 15, 20, 25]);
const DENSITY_MAXIMUM = Object.freeze([30, 35, 40, 45]);
// 한 Template 에 들어가는 영역 수와 영역 표 크기, 지역 Template 슬롯 수이다. (2026-11-20 WB04-E1105)
const TERRITORY_COUNT = 20;
const TERRITORY_BYTES = 120;

// 스커미시 월드 생성 연결이다. 생성자는 Session 소유자만 보관하고 아무것도 바꾸지 않는다. (2026-11-20 WB04-E1105)
export class RawCampaignSessionSkirmish {
    #owner;

    constructor(owner) { this.#owner = owner; }

    // 지역 Template 의 slot 영역 표(20×6바이트)를 읽고 고른 영역만 전투(4) 플래그가 켜진 첫 영역이 되게 한다. 영역 번호가 범위 밖이면 표를 그대로 둔다. 원문 tm 진단과 같은 조건으로 미설치 슬롯을 거부한다. (2026-11-20 WB04-E1105)
    readSkirmishTerritory(source, slot, territoryChoice, output) {
        const headers = source.headers();
        if (slot >= 9 || new DataView(headers.buffer, headers.byteOffset, headers.byteLength).getUint32(slot * 20, true) === 0) throw new RangeError('스커미시 시작 섬 Template 슬롯 미설치');
        const stream = source.selectTerritory(slot), position = stream.position; // RawFortWorldLoad.readTerritory 와 같은 현재 위치의 120바이트를 읽는다.
        if (position > stream.bytes.length || stream.bytes.length - position < output.length) throw new RangeError('스커미시 시작 섬 Territory 현재 물리 120BYTE 누락');
        // 사용하지 않는 꼬리는 그대로 두고 120바이트만 복사한다.
        for (let index = 0; index < output.length; index += 1) output[index] = stream.bytes[position + index];
        stream.position = position + TERRITORY_BYTES;
        if (territoryChoice < 0 || territoryChoice >= TERRITORY_COUNT) return;
        // 모든 영역의 전투 표지를 끄고 고른 영역만 켠다.
        for (let index = 0; index < TERRITORY_COUNT; index += 1) output[index * 6 + 1] &= ~4 & 0xff;
        output[territoryChoice * 6 + 1] |= 4;
    }

    // 참가자 owner 의 시작 섬을 만든다. sourceStorage 가 있으면 현재 Template 대신 그 지역 Template 의 owner 슬롯에서 영역/객체를 읽고 territoryChoice 영역을 전투섬으로 고른다. (2026-11-20 WB04-E1105)
    buildBattleIsland(owner, sourceStorage = null, territoryChoice = -1) {
        const o = this.#owner, c = o.core, assertion = o.services.ai.assertion;
        const hooks = {
            assertion: () => need(assertion, '전투섬 owner 진단 미연결')('playerId != INVALID_PLAYER_ID'),
            territory: (slot, output) => {
                if (!sourceStorage) { if (!o.runtime) throw new Error('전투섬의 현재 Territory 부모 미설치'); o.runtime.readTerritory(slot, output); return; }
                this.readSkirmishTerritory(sourceStorage, slot, territoryChoice, output);
            },
            placement: (player, record) => { if (!o.construction) throw new Error('전투섬의 현재 Player 배치 부모 미설치'); return o.construction.playerBounds.placement(player | 0, record); },
            region: (player, island) => {
                if (player >= 9) throw new RangeError('전투섬의 실제 Player9 범위');
                new DataView(o.players.buffer, o.players.byteOffset, o.players.byteLength).setUint32(player * 180 + 0x78, island >>> 0, true);
            },
            generate: (state, island) => { if (!o.normal) throw new Error('전투섬의 실제 지형 부모 미설치'); o.normal.generateOriginal(state.owner, state.bounds, island); },
            boss: () => Boolean(o.state.assetState.readBoss()),
            objects: (slot, start, end) => { if (!o.runtime) throw new Error('전투섬의 실제 Objects 부모 미설치'); o.readFortObjectStreams(o.runtime.objects, slot, start, end, sourceStorage); },
            graph: (reset) => { c.graph.rebuild(reset); },
            builderAssertion: () => need(assertion, '전투섬의 builder 진단 미연결')('전투섬 IslandBuilder 진단'),
            decoderDiagnostics: o.metadata.geometry.decoderDiagnostics,
        };
        new RawBattleIsland(c.pool, o.chunks, o.islands, c.frames.map((table) => table.codes), () => o.piece, hooks).build(owner); // 프레임 표는 타입별 코드 배열만 넘긴다(월드 런타임과 같다).
    }

    // 중앙섬의 거주 구역(0046df80)은 기존 실제 거주 구역 배치·seed·게임 객체 생성 부모를 쓴다. (2026-11-20 WB04-E1105)
    populateCentralResidences(region) {
        const o = this.#owner, c = o.core, assertion = o.services.ai.assertion;
        if (!o.finish) throw new Error('중앙 거주 구역의 실제 랜덤 배치 부모 미설치');
        const inputs = { boss: () => o.state.assetState.readBoss(), fort: () => c.ownerMode.fort, residence: () => o.finishState.residenceType, primary: () => o.tutorialArcherType, fallback: () => o.templeProduct, replacement: () => o.rewardState.types.priest };
        const hooks = {
            assertion: () => need(assertion, '중앙 거주 구역의 실제 진단 미연결')('중앙섬 IslandPopulation 진단'), wallSeconds: () => need(o.services.sound.wallSeconds, '중앙 거주 구역의 실제 벽시각 미연결')(),
            position: (sid) => c.neighbors.center(sid), destroy: (sid, flags) => o.world.destroy(sid, flags), create: (type, flags) => o.world.create(type, flags),
            defaultFrame: (type) => { if (type < 70 || type - 70 >= o.assets.game.types().length) throw new RangeError('중앙 거주 구역의 실제 TYPE 기본 프레임 범위'); return o.assets.game.types()[type - 70].definition.defaultFrame; },
            typeHitPoints: (type) => o.hp.typeHitPoints(type), hasHitPoints: (sid) => o.hp.hasHitPoints(sid), setOwner: (sid, owner) => o.ownership.setOwner(sid, owner),
            pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags), refreshProduction: () => need(o.services.ownership.refreshProduction, '중앙 거주 구역의 실제 생산 통지 미연결')(),
        };
        new RawIslandPopulation(c.pool, o.chunks, o.chunks, o.islands, o.finish, inputs, hooks).residence(region);
    }

    // metadata 의 일반 $Random 중앙섬 요청을 같은 지형/현재 중심/군도 분리/Graph 에 연결한다. record 는 Zone 6바이트 영역 기록이다. (2026-11-20 WB04-E1105)
    buildCentralIsland(record) {
        const o = this.#owner, c = o.core, assertion = o.services.ai.assertion, aux = o.gameplayGlobals.gameAux();
        const diagnostic = (reason) => need(assertion, '군도 지역의 실제 진단 미연결')(reason === SQUID_REGION_ASSERT.unsupportedGenus ? 'genus & (gISLAND|gBRIDGE|gISLAND3x3)' : 'regionNum == noRegion');
        const hooks = {
            assertion: (expression) => need(assertion, '중앙섬의 실제 진단 미연결')(expression),
            placement: (territory) => { if (!o.construction) throw new Error('중앙섬의 실제 원점 배치 부모 미설치'); return o.construction.playerBounds.originPlacement(territory); },
            generate: (state, region) => { if (!o.normal) throw new Error('중앙섬의 실제 지형 부모 미설치'); o.normal.generateOriginal(state.owner, state.bounds, region); },
            residence: (region) => this.populateCentralResidences(region),
            partition: () => new RawIslandPartition(c.pool, c.hash.view(0), c.frames.map((table) => table.codes), () => aux.terrainType, o.islands, () => { o.worldRedraw.value = (o.worldRedraw.value + 1) >>> 0; },
                (sid) => getSquidRegion(c.pool, c.types, sid, o.noRegion >>> 0, diagnostic), (sid, region) => setSquidRegion(c.pool, c.types, sid, region, o.noRegion >>> 0, diagnostic)).split(),
            graph: (reset) => { c.graph.rebuild(reset); },
            builderAssertion: () => need(assertion, '중앙섬의 builder 진단 미연결')('중앙섬 IslandBuilder 진단'),
            decoderDiagnostics: o.metadata.geometry.decoderDiagnostics,
        };
        const globals = {
            battle: () => o.state.assetState.inBattle, boss: () => o.state.assetState.readBoss(), terrainEnabled: () => aux.terrainEnabled, archipelago: () => aux.archipelago,
            position: aux.centralPosition(), unassignedTerritory: () => o.finishState.invalidRegion,
        };
        new RawCentralIsland(c.pool, o.chunks, o.islands, c.frames.map((table) => table.codes), () => o.piece, globals, hooks).build(record);
    }

    // 미션 Start 직전에 MP 시작 규칙으로 월드를 만든다. 계획이 없으면(일반 미션) 아무것도 하지 않는다. 임시로 바꾼 미션 모드 네 곳은 예외가 나도 되돌린다. (2026-11-20 WB04-E1105)
    buildSkirmishWorld() {
        const o = this.#owner;
        const plan = typeof o.services.ai.skirmishWorld === 'function' ? o.services.ai.skirmishWorld() : null;
        if (!plan) return;
        if (!o.loaded) throw new Error('실제 Session 설치 전 스커미시 월드 생성 요청');
        if (!plan.islands.length || plan.islands.length > 8) throw new RangeError('스커미시 월드 계획의 참가자 수 범위');
        o.assetModules.prepareConstruction();
        o.battleOptions[6] = Math.min(Math.max(plan.mapSize | 0, 0), 3); // MP 맵 크기 옵션(링 간격 3/4/5/6칸)이다.
        // 객체 소유자=참가자(objectState.mission), 링 배치(links.mission), 원본 Chaff 를 참가자마다 다시 읽지 않기(objectState.chaffMode), 영역 일치 판정이 소유자까지 비교하기(finishState.chaffMode)를 MP 전투 규칙으로 맞춘다.
        const targets = [[o.objectState, 'mission'], [o.world.storage.links, 'mission'], [o.objectState, 'chaffMode'], [o.finishState, 'chaffMode']];
        const saved = targets.map(([target, key]) => target[key]);
        for (const [target, key] of targets) target[key] = false;
        try { this.#build(plan); } finally { targets.forEach(([target, key], index) => { target[key] = saved[index]; }); }
    }

    // 군도 밀도·중앙섬·시작 섬을 순서대로 만든다. (2026-11-20 WB04-E1105)
    #build(plan) {
        const o = this.#owner, c = o.core, aux = o.gameplayGlobals.gameAux();
        // MP 로그인(InitializeDensity)과 같은 규칙으로 군도 밀도를 정한다: 밀도 옵션(0..3)의 최소/최대 표에서 반값 범위를 두 번 뽑아 더한다. 참가자가 많으면 SID 가 모자라지 않게 낮은 밀도를 쓴다.
        const density = plan.islands.length > 4 ? 0 : 1;
        o.battleOptions[7] = density; aux.archipelagoMinimum = DENSITY_MINIMUM[density]; aux.archipelagoMaximum = DENSITY_MAXIMUM[density];
        const sample = () => { const low = DENSITY_MINIMUM[density] >> 1, high = DENSITY_MAXIMUM[density] >> 1; return o.random.next(high - low) + low; };
        const first = sample(), second = sample(); aux.archipelagoPercent = first + second;
        // 섬 기록은 100개뿐이라 중앙섬+군도가 약 90개를 넘으면 5~8번째 참가자의 섬을 만들 수 없다. 참가자가 5명 이상이면 군도 밀도를 5~8%로 낮춘다.
        if (plan.islands.length > 4) aux.archipelagoPercent = 5 + o.random.next(4);
        const forced = typeof o.services.construction?.configInt === 'function' ? o.services.construction.configInt('skirmishDensity', -1) : -1;
        if (forced >= 0) aux.archipelagoPercent = forced >>> 0; // 검사용: 설정 skirmishDensity(0 이상)가 있으면 군도 밀도 비율을 그 값으로 고정한다.
        // 1) 중앙섬은 시작 섬보다 먼저 만든다(원문 진단: 섬 목록이 비어 있어야 한다).
        try { this.buildCentralIsland(plan.central); } catch (error) {
            const record = Array.from(plan.central, (byte) => byte.toString(16).padStart(2, '0')).join('');
            error.message = `${error.message} [스커미시 중앙섬 기록=${record}]`; throw error;
        }
        const temporary = o.templateStorage.createSibling();
        const clearAll = () => { for (let slot = 1; slot <= 8; slot += 1) temporary.clear(slot); }; // 지역 Template 슬롯을 반납한다.
        try {
            // 2) 참가자마다 자기 시작 섬 fort 를 읽고 전투섬을 만든다.
            for (let owner = 1; owner <= plan.islands.length; owner += 1) {
                const island = plan.islands[owner - 1];
                const hooks = {
                    ...o.services.missionFortLoad,
                    read: () => island.bytes, sameName: (first, second) => first === second, wrongName: () => {}, readOnly: () => true, // 원본 fort 는 읽기만 하고 다시 저장하지 않는다.
                    typeCount: () => c.types.length, currentSlot: () => 9, refreshTitle: undefined, // 지역 Template 는 현재 슬롯과 제목을 바꾸지 않는다.
                };
                hooks.tooManyTypes ??= () => { throw new Error('스커미시 시작 섬 fort의 numTypes<256 진단'); };
                hooks.rejectFile ??= (path) => { throw new Error(`스커미시 시작 섬 fort 서명 거부: ${path}`); };
                temporary.loadFile(owner, {}, `\\D\\${island.fort}.FORT`, hooks);
                try { this.buildBattleIsland(owner, temporary, island.territory); } catch (error) {
                    error.message = `${error.message} [스커미시 시작 섬 owner=${owner} fort=${island.fort} territory=${island.territory}]`; throw error;
                }
                temporary.clear(owner);
            }
        } catch (error) { clearAll(); throw error; }
        clearAll();
    }
}
