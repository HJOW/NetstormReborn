// 외곽 Session의 기술 획득·세계/로컬 간헐천·너겟 배치 대상이다(225553df, WB04-E637).
// 사용: 아래 함수를 설치된 같은 owner에 호출한다. 공개 callback 게시 자체는4D-1d, 실제 Load 검사는4D-2h다.
// typed 분기를 옮기며 PlaceBattleNuggets의 Native Client 장기 분기는4D-8에 남긴다.
import { acquireNewTechnology } from './raw-new-technology.js';
import { RawWorldGeyser, worldGeyserDensity } from './raw-world-geyser.js';
import { RawWorldGeyserBounds, worldGeyserBudget, worldGeyserSpacing } from './raw-world-geyser-bounds.js';
import { RawFortRandomPlacement, makeFortSinglePlacementHook } from './raw-fort-random-placement.js';
import { makeNormalTerrainFarmWorldHooks } from './raw-normal-terrain-world-binding.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';
import { placeBattleNuggets } from './raw-battle-nugget-placement.js';
import { runBattleResourceFrame } from './raw-battle-resource-frame.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';
import { fortSaveCoordinate } from './raw-fort-object-save.js';

// C++ SessionMissing과 같이 선택된 진단 위치에서만 실패한다.
function missing(label) { return () => { throw new Error(label + '의 실제 진단 장치 미연결'); }; }
// 원문 uint16 SID의 같은 Pool 범위만 허용한다. 가짜 부모/잘린 번호로 바꾸지 않는다.
function parentSid(owner, parent) {
    if (!Number.isInteger(parent) || parent < 0 || parent > 65535 || parent >= owner.core.pool.capacity) throw new RangeError('기술 획득의 실제 부모 SID 범위');
    return parent;
}
// 현재 공간/섬/HP/전체 MayPlace를 빌리는 호출 지역 배치 몸체를 구성한다. Player 기록 결합은 호출자가 선택한다.
function placement(owner, invalidType) {
    const c = owner.core, hooks = makeNormalTerrainFarmWorldHooks(owner.world, owner.chunks, owner.islands, owner.pipeline, owner.hp);
    hooks.mayPlace = (query) => owner.constructionModules.mayPlace(query);
    const single = new RawFortRandomPlacement(c.pool, c.hash, c.spots, c.types, c.frames, owner.random, owner.randomPlacement, hooks);
    const finish = { // 지역 사각형은 값 전달 사본이고 시계 장치는 호출 위치에서만 요구한다.
        regionBounds: (region) => { const b = owner.islands.bounds(region); return { left: b.left, top: b.top, right: b.right, bottom: b.bottom }; },
        wallSeconds: owner.services.construction.wallSeconds, invalidType,
    };
    return new RawFortFinishOwner(c.pool, c.types, owner.state.assetState.currentResidences(), owner.finishState, makeFortSinglePlacementHook(c.pool, single, finish));
}

// 원문5650..5653: 현재 요청 상태를 준비한 뒤 실제 로컬 생성 몸체를 호출한다.
export function generateCampaignLocalGeysers(owner) {
    if (!owner.loaded) throw new Error('실제 Session 설치 전 로컬 간헐천 생성 요청');
    owner.assetModules.prepareConstruction(); owner.construction.generateLocalGeysers();
}
// 원문5655..5680: 같은 장기 전역과 옵션으로 실제 전체 부모의 지역 배치 몸체를 구성한다.
export function generateCampaignWorldGeysers(owner) {
    if (!owner.loaded) throw new Error('실제 Session 설치 전 세계 간헐천 생성 요청');
    const aux = owner.gameplayGlobals.gameAux();
    const bounds = new RawWorldGeyserBounds({ // 같은 현재 모드/세계 크기/여백을 각 원문 읽기 위치에서 조회한다.
        get missionPost() { return owner.globals.missionPost; }, get mapLimit() { return owner.gameplayGlobals.worldCells().value; },
        get spacingMargin() { return aux.worldGeyserMargin; }, set spacingMargin(value) { aux.worldGeyserMargin = value; },
    }, {
        // 실제 IslandList 존재 관문 뒤 현재 청크 범위를 선택한다.
        chunkBounds: (region) => { if (!owner.islands.exists(region)) throw new Error('세계 간헐천 원본 ChunkMap.bounds의 섬 존재 진단'); return owner.chunks.bounds(region); },
        spacing: () => worldGeyserSpacing(owner.battleOptions[6]),
    });
    const finish = placement(owner, missing('세계 간헐천 TYPE')); finish.bindPlayerRecords(owner.players);
    const hooks = { // 실제 local/장식/전체 Populate를 원본 순서로 묶는다. 별도 세계나 난수 사본은 만들지 않는다.
        log: (text) => console.info(text), local: () => owner.generateLocalGeysers(),
        configInt: owner.services.construction.configInt, wallSeconds: owner.services.construction.wallSeconds,
        // NB29 수정과 스커미시 인원은 기준선의 실제 Session이 켜는 선택 입력이다.
        budget: () => worldGeyserBudget(owner.core.ownerMode.battle, owner.players, owner.battleOptions, owner.services.ai.skirmishPlayers ? owner.services.ai.skirmishPlayers() : 0, true),
        density: () => worldGeyserDensity(owner.battleOptions[17]), spacing: () => worldGeyserSpacing(owner.battleOptions[6]),
        demo: () => { if (!owner.services.missionOutcome.demo) throw new Error('세계 간헐천의 실제 현재 Guide demo 조회 미연결'); return owner.services.missionOutcome.demo() !== 0 ? 1 : 0; },
        whole: () => bounds.whole(), partition: (cell, grid) => bounds.partition(cell, grid),
        decorate: (random, rectangle, region, quantity, type, force) => { finish.placeRandom(random, rectangle, region, quantity, type, force); },
        populate: (quantity, random, rectangle, options) => { owner.assetModules.prepareConstruction(); owner.construction.populateGeysers(quantity, random, rectangle, options); },
    };
    new RawWorldGeyser(owner.core.pool.edition, { // NB04의 별도 시간 난수로 Even 구역 순서만 섞는다.
        get battle() { return owner.core.ownerMode.battle; }, options: owner.battleOptions, decorationTypes: aux.worldDecorationTypes, shuffleEvenCells: true,
    }, hooks).generate();
}
// 원문 RawCampaignSession::RunBattleResources(00486bc0 의 현재 전역·장치 연결): 로그인 중이면 자연 반환하고, 스펠 만료 보충·SP 주기 지급·일반 간헐천 보충(단독 미션은 30초마다 전체 한 개 조건, 일반 전투는 3×3 구역)을
// 같은 장기 전역(보조 전역·전투 옵션·Player 기록)과 실제 Pool/MayPlace/난수에 연결한다. send(packet)는 서버(9)로 가는 로컬 송신이다. 설치 전에도 호출할 수 있다(원문은 State8 준비 중 로그인 기한 관문의 자연 반환을 오류로 바꾸지 않는다).
// 앱은 이 호출이 없어 단독 미션·스커미시의 간헐천 보충과 스펠 소비가 한 번도 일어나지 않았다. (2026-11-22 WB04-E1232)
export function runCampaignBattleResources(owner, send) {
    const aux = owner.gameplayGlobals.gameAux();
    const bounds = new RawWorldGeyserBounds({ // 같은 현재 모드/세계 크기/여백을 각 원문 읽기 위치에서 조회한다.
        get missionPost() { return owner.globals.missionPost; }, get mapLimit() { return owner.gameplayGlobals.worldCells().value; },
        get spacingMargin() { return aux.worldGeyserMargin; }, set spacingMargin(value) { aux.worldGeyserMargin = value; },
    }, {
        chunkBounds: (region) => { if (!owner.islands.exists(region)) throw new Error('자원 프레임의 현재 ChunkMap.bounds 섬 존재 진단'); return owner.chunks.bounds(region); },
        spacing: () => worldGeyserSpacing(owner.battleOptions[6]),
    });
    const hooks = {
        replenish: () => { owner.priestModules.replenishNuggets(); },
        // 원문 비정렬 signed 금액의 하위 네 BYTE 를 그대로 싣는 6ch/종류 2 수입 패킷이다.
        income: (target, source, value) => { const packet = new Uint8Array(11); packet.set([1, 0x6c, 11, 0, target & 0xff, source & 0xff, 2]); new DataView(packet.buffer).setUint32(7, value >>> 0, true); send(packet); },
        predictable: () => { if (!owner.services.construction.configInt) throw new Error('자원 프레임의 현재 Config 장치 미연결'); return owner.services.construction.configInt('predictableGeysers', 0) === 1; },
        wallSeconds: owner.services.construction.wallSeconds,
        budget: () => worldGeyserBudget(owner.core.ownerMode.battle, owner.players, owner.battleOptions, owner.services.ai.skirmishPlayers ? owner.services.ai.skirmishPlayers() : 0, true),
        density: () => worldGeyserDensity(owner.battleOptions[17]), spacing: () => worldGeyserSpacing(owner.battleOptions[6]),
        whole: () => bounds.whole(), partition: (cell) => bounds.partition(cell, 3),
        // 구역 안의 현재 간헐천 TYPE 개수이다: 원문 flags1 finder 의 순서·중복을 그대로 센다.
        count: (area) => {
            const lower = RawCursorPosition.truncatePoint([area.left, area.top]), upper = RawCarrierDropPoint.snap([area.right, area.bottom]);
            const finder = new RawSquidFinder(owner.core.pool, owner.core.hash, owner.core.types); let count = 0;
            for (let sid = finder.begin({ left: fortSaveCoordinate(lower[0]), top: fortSaveCoordinate(lower[1]), right: fortSaveCoordinate(upper[0]), bottom: fortSaveCoordinate(upper[1]) }, 1); sid; sid = finder.next()) if (owner.core.pool.slot(sid)[10] === aux.geyserType) count += 1;
            return count;
        },
        geyserCount: () => owner.state.assetState.currentGeysers().readCount(),
        typeCost: () => { if (aux.geyserType >= owner.core.types.length) throw new RangeError('자원 프레임의 현재 TYPE 범위'); return owner.core.types[aux.geyserType].cost; },
        choose: (count) => owner.random.next(count),
        populate: (quantity, random, rectangle, options) => { owner.assetModules.prepareConstruction(); owner.construction.populateGeysers(quantity, random, rectangle, options); },
    };
    runBattleResourceFrame({
        aux, loginEnd: () => owner.loginEnd, now: () => owner.core.processState.now, boss: () => owner.state.assetState.readBoss(), battle: () => owner.core.ownerMode.battle,
        single: () => owner.globals.missionPost, editor: () => owner.globals.editor, owner: () => owner.playerState.battleOwner, options: owner.battleOptions, players: () => owner.players,
    }, hooks);
}
// 원문5761..5771 typed: 단독 진단/지역 seed/현재 TYPE를 실제 같은 공간의 단일 배치에 넘긴다.
export function placeCampaignBattleNuggets(owner, rectangle, quantity) {
    if (!owner.loaded || !owner.construction) throw new Error('설치 전 실제 비미션 보충 배치');
    const finish = placement(owner, () => {
        if (!owner.services.player.log) throw new Error('보충 배치의 실제 TYPE 진단 로그 미연결');
        owner.services.player.log(0x1d, 'AddRandom: Invalid Type\n');
    });
    placeBattleNuggets(rectangle, quantity, { // source의 세 참조는 지역 snapshot이 아닌 같은 현재 전역이다.
        single: () => owner.globals.missionPost, boss: () => owner.state.assetState.readBoss(), buriedType: () => owner.gemRewardState.buriedType,
    }, {
        singleAssertion: () => { owner.world.storage.assertDisplay('!inSingle', '\\Ns\\O\\Addrandom.cpp', 0xb4); },
        wallSeconds: owner.services.construction.wallSeconds, regionZero: () => owner.construction.playerBounds.regionRecord(0),
        place: (random, bounds, region, count, type, force) => { finish.placeRandom(random, bounds, region, count, type, force); },
    });
}
// 원문6080..6098: 실제 종류 WORD 조회·내용물 생성과 두 독립 snapshot 뒤 현재 안내 서비스를 호출한다.
export function learnCampaignNewTechnology(owner, type) {
    if (!owner.loaded) throw new Error('실제 Session 설치 전 기술 획득');
    const hooks = { ...owner.services.newTechnology }; // C++의 값형 함수 표 사본이며 각 함수가 읽는 현재 대상을 복제하지 않는다.
    hooks.technologyParent = () => owner.player.currentTechnology();
    hooks.find = (parent, mask, kind, index) => {
        const found = owner.priests.carrier.findContained(parentSid(owner, parent), mask, kind, index);
        if (!found) return 0; const raw = owner.core.pool.slot(found); return raw[18] | (raw[19] << 8);
    };
    hooks.create = (parent, kind, argument, flags, attach, quality) => { owner.contained.create(parentSid(owner, parent), kind, argument, flags, attach, quality); };
    hooks.snapshot = () => {
        if (owner.core.ownerMode.battle) owner.snapshotBattle(owner.playerState.templateOwner);
        if (owner.core.ownerMode.fort) owner.snapshotFort(owner.playerState.templateOwner);
    };
    hooks.type = (number) => { if (!Number.isInteger(number) || number < 0 || number >= owner.core.types.length) throw new RangeError('기술 획득의 현재 TYPE 물리 범위'); return number; };
    acquireNewTechnology(type, hooks);
}
