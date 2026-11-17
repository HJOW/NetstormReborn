// 외곽 callback의 현재 값/순서·설치 관문과 NB04/NB29의 기준선 선택 스위치를 검사한다(WB04-E637).
// 최소 owner 계약은 실제 미션 Load 검사를 대신하지 않는다. 실제 owner의 loaded는 바꾸지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { generateCampaignLocalGeysers, learnCampaignNewTechnology } from '../www/core/raw-campaign-callbacks.js';
import { RawWorldGeyser } from '../www/core/raw-world-geyser.js';
import { worldGeyserBudget } from '../www/core/raw-world-geyser-bounds.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { put } from './raw-support.mjs';

// cpppj KnownBug_NB04의200개 시각과 원문/수정 스위치를 그대로 대조한다.
test('KnownBug_NB04_EvenGeyserCellOrderIsShuffledOnlyWhenEnabled', () => {
    const options = new Uint8Array(87); options[18] = 2; const cells = [], seeds = [], state = { battle: false, options, decorationTypes: [0, 0, 0, 0] };
    let now = 0, times = 0;
    const hooks = { // 원문 외부 효과의 명시 관측 장치이며 별도 게임 월드를 만들지 않는다.
        log: (text) => assert.equal(text, 'Make Local Geysers\n'), local: () => {}, configInt: () => 1,
        wallSeconds: () => { times++; return now; }, budget: () => 32, density: () => 0, spacing: () => 4,
        partition: (cell, grid) => { assert.equal(grid, 3); cells.push(cell); return {}; },
        populate: (quantity, random) => { seeds.push(random.state); },
    };
    new RawWorldGeyser('patch1078', state, hooks).generate(); assert.deepEqual(cells, [1, 2, 3, 4, 5, 6, 7, 8, 9]); assert.equal(times, 0);
    const last = new Set(); let changed = false;
    // 배치 난수는 독립 셔플에 소비되지 않고 아홉 구역이 중복 없이 방문되어야 한다.
    for (let second = 0; second < 200; second++) {
        now = 1000 + second * 7; cells.length = seeds.length = 0;
        new RawWorldGeyser('patch1078', { ...state, shuffleEvenCells: true }, hooks).generate();
        assert.deepEqual([...cells].sort((a, b) => a - b), [1, 2, 3, 4, 5, 6, 7, 8, 9]); assert.deepEqual(seeds, new Array(9).fill(0xfed));
        changed ||= cells.some((cell, index) => cell !== index + 1); last.add(cells.at(-1));
    }
    assert.ok(changed); assert.ok(last.size >= 6); assert.equal(times, 200);
});

// cpppj KnownBug_NB29의 참가자 명시/미명시·상한과 실제 전투 Player/숨김 우선순위다.
test('KnownBug_NB29_GeyserBudgetDoesNotShrinkWithMorePlayers', () => {
    const players = new Uint8Array(1620), options = new Uint8Array(87);
    // 6명 이하는 원문 그대로이며7/8명은 수정 스위치가 있을 때만 증가한다.
    for (const n of [1, 2, 4, 6]) { assert.equal(worldGeyserBudget(false, players, options, n), (12 - n) * n); assert.equal(worldGeyserBudget(false, players, options, n, true), (12 - n) * n); }
    assert.equal(worldGeyserBudget(false, players, options, 7), 35); assert.equal(worldGeyserBudget(false, players, options, 8), 32);
    assert.equal(worldGeyserBudget(false, players, options, 7, true), 42); assert.equal(worldGeyserBudget(false, players, options, 8, true), 48); assert.equal(worldGeyserBudget(false, players, options, 99, true), 48);
    assert.equal(worldGeyserBudget(false, players, options), 0);
    // 전투에서는 명시 인원 대신 실제 Player+2c와 숨김 BYTE를 읽는다.
    for (let owner = 1; owner <= 8; owner++) put(players, owner * 180 + 0x2c, 1);
    assert.equal(worldGeyserBudget(true, players, options, 1, true), 48); options[70] = 1; assert.equal(worldGeyserBudget(true, players, options, 1, true), 42);
});

// 기술 획득의 최소 owner 계약이다. 내용물 조회 SID와 원문 종류 WORD의 의미를 구별한다.
function technologyContract(kind = 0) {
    const calls = [], slot = new Uint8Array(50); put(slot, 18, kind, 2);
    const owner = {
        loaded: true, core: { ownerMode: { battle: true, fort: false }, types: new Array(188), pool: { capacity: 64, slot: (sid) => { assert.equal(sid, 12); return slot; } } },
        playerState: { templateOwner: 1 }, player: { currentTechnology: () => { calls.push('parent'); return 4; } },
        priests: { carrier: { findContained: (...args) => { assert.deepEqual(args, [4, 4, 77, 0]); calls.push('find'); return 12; } } },
        contained: { create: (...args) => { assert.deepEqual(args, [4, 77, 0, 4, 16, 0]); calls.push('create'); } },
        // Battle 뒤 변경한 두 번째 모드/owner를 Fort 진입이 다시 읽어야 한다.
        snapshotBattle: (ownerSlot) => { assert.equal(ownerSlot, 1); calls.push('battle'); owner.core.ownerMode.fort = true; owner.playerState.templateOwner = 2; },
        snapshotFort: (ownerSlot) => { assert.equal(ownerSlot, 2); calls.push('fort'); },
        services: { newTechnology: {
            name: (type) => { assert.equal(type, 77); calls.push('name'); return 'Current\0ignored'; },
            configure: (key, text) => { calls.push([key, text]); },
            format: (pattern, type) => { assert.equal(pattern, '<!`"%d.*`">'); assert.equal(type, 77); return 'Picture\0ignored'; },
            tell: (name) => { calls.push(name); },
        } },
    };
    return { owner, calls };
}

// 비영 SID가 종류0을 의미할 때도 생성을 생략하지 않으며 중복 기술도 snapshot/안내를 실행한다.
test('CampaignCallbacks 기술 결선: 종류 WORD·두 독립 snapshot·현재 owner·이름/그림 안내 순서', () => {
    // 원문 종류0과 이미 존재하는77을 각각 독립 입력으로 준다.
    for (const kind of [0, 77]) {
        const { owner, calls } = technologyContract(kind); learnCampaignNewTechnology(owner, 77);
        const prefix = kind ? ['parent', 'parent', 'find'] : ['parent', 'parent', 'find', 'parent', 'create'];
        assert.deepEqual(calls, [...prefix, 'battle', 'fort', 'name', ['global.newTechName', 'Current'], ['global.newTechPic', 'Picture'], 'NewTech']);
    }
});

// 원문 부모 범위·미연결 안내가 앞선 기술/저장 부분 효과를 되돌리지 않는다.
test('CampaignCallbacks 기술 결선: 실제 SID 범위와 늦은 안내 누락', () => {
    const invalid = technologyContract(); invalid.owner.player.currentTechnology = () => 64;
    assert.throws(() => learnCampaignNewTechnology(invalid.owner, 77), /부모 SID 범위/); assert.deepEqual(invalid.calls, []);
    const late = technologyContract(); delete late.owner.services.newTechnology.name;
    assert.throws(() => learnCampaignNewTechnology(late.owner, 77), /선택 장치 미연결/); assert.deepEqual(late.calls.slice(-3), ['create', 'battle', 'fort']);
});

// local callback은 준비 상태를 먼저 갱신하고 현재 생성 처리기를 그 다음 선택한다.
test('CampaignCallbacks 로컬 결선: PrepareConstruction 뒤 같은 현재 생성 처리기를 선택한다', () => {
    const calls = [], owner = { loaded: true, construction: null, assetModules: { prepareConstruction: () => { calls.push('prepare'); owner.construction = { generateLocalGeysers: () => calls.push('generate') }; } } };
    generateCampaignLocalGeysers(owner); assert.deepEqual(calls, ['prepare', 'generate']);
});

// 실제 owner의 서비스 사본과 Load 전/반납 뒤 관문이다. 정상 미션/실제 AI 간헐천 검사는4D-2h 뒤 재대조한다.
test('CampaignCallbacks 실제 owner: 값형 서비스·설치 전 거부와 반납 뒤 원시 상태 보존', () => {
    const name = () => 'before', input = { name }, owner = campaignOwners({ newTechnology: input });
    try {
        input.name = () => 'after'; assert.notEqual(owner.services.newTechnology, input); assert.equal(owner.services.newTechnology.name, name);
        const pool = owner.core.pool.bytes(), seed = owner.random.state, players = owner.players.slice();
        assert.throws(() => owner.learnNewTechnology(77), /설치 전 기술/); assert.throws(() => owner.generateLocalGeysers(), /설치 전 로컬/);
        assert.throws(() => owner.generateWorldGeysers(), /설치 전 세계/); assert.throws(() => owner.placeBattleNuggetsBody({}, 1), /설치 전 실제/);
        assert.deepEqual(owner.core.pool.bytes(), pool); assert.deepEqual(owner.players, players); assert.equal(owner.random.state, seed); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
    assert.throws(() => owner.learnNewTechnology(77), /반납한 Session/); assert.throws(() => owner.generateLocalGeysers(), /반납한 Session/);
    assert.throws(() => owner.generateWorldGeysers(), /반납한 Session/); assert.throws(() => owner.placeBattleNuggetsBody({}, 1), /반납한 Session/);
});
