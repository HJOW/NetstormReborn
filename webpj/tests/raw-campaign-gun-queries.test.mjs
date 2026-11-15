// cpppj RawCampaignGunQueries의 조회기 일곱 개를 실제 World·Gun·표적/시각 처리기와 연결해 대조한다. 전용 원본 조립 fixture는 없다. (2026-11-16 WB04-E565)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignGunScene } from './campaign-gun-support.mjs';
import { makeCampaignGunQueryHooks } from '../www/core/raw-campaign-gun-queries.js';
import { put, get, floatToBits } from './raw-support.mjs';

// 실제 자산의 썬 캐논을 지면 위에 설치해 GunProcess/공간/현재 프레임을 공유한다. (2026-11-16 WB04-E565)
function cannonScene() {
    const value = new CampaignGunScene(); value.ground(20, 20, 26, 26);
    const cannon = value.pop(128, 23, 23, 3, 1); return { value, cannon };
}

test('CampaignGunQueries: 다른 모드/표적/피해 Pool·발사간격 표를 구성 시 거부하고 다른 훅을 보존한다', () => {
    const value = new CampaignGunScene({ assets: false }); const other = new CampaignGunScene({ assets: false });
    // 세 하위 Pool은 모두 같은 실제 슬롯을 봐야 한다.
    for (const [name, foreign] of [['mode', other.gunMode], ['target', other.gunTarget], ['hit', other.gunHit]]) assert.throws(() => value.makeQueries({ [name]: foreign }));
    assert.throws(() => value.makeQueries({ intervals: new Float32Array(187) })); assert.throws(() => value.makeQueries({ intervals: new Float32Array(189) }));
    const keep = () => 31; const replace = () => { throw new Error('기존 조회 훅'); };
    const input = { startAnimation: keep, checkForms: replace, validateIslandOwners: replace, targetingMode: replace, findTarget: replace, refreshRange: replace, shotContext: replace, firingInterval: replace };
    const result = value.makeQueries({ hooks: input }); assert.equal(result.startAnimation, keep);
    // 생성은 호출자의 훅 객체를 덮지 않으며 현재 조회 일곱 개만 새 함수로 교체한다.
    for (const name of ['checkForms', 'validateIslandOwners', 'targetingMode', 'findTarget', 'refreshRange', 'shotContext', 'firingInterval']) {
        assert.equal(input[name], replace); assert.notEqual(result[name], replace);
    }
});

test('CampaignGunQueries: 실제 TYPE128의 여덟 DWORD 문맥·현재 방향·좌표·owner·프레임', () => {
    const { value, cannon } = cannonScene(); const raw = value.core.pool.physical(cannon);
    assert.deepEqual(value.queries.shotContext(cannon), [floatToBits(23), floatToBits(23), floatToBits(21.5), floatToBits(21.5), 128, 4, 1, 0]);
    assert.equal(value.queries.targetingMode(cannon, 255), 4);
    // 같은 슬롯의 새 좌표/owner/프레임을 다음 조회가 다시 읽는다.
    put(raw, 14, floatToBits(24.25)); put(raw, 18, floatToBits(25.5)); raw[34] = 3;
    const frame = value.frames[128].run('L').first; put(raw, 36, frame);
    assert.deepEqual(value.queries.shotContext(cannon), [floatToBits(24.25), floatToBits(25.5), floatToBits(22.75), floatToBits(24), 128, 0, 3, frame]);
    assert.equal(value.queries.targetingMode(cannon, 0), 0);
});

test('CampaignGunQueries: 실제 모드가 크로스보우 graph BYTE와 astral 상태를 현재 값으로 구분한다', () => {
    const value = new CampaignGunScene({ assets: false }); const sid = value.create(140); const raw = value.core.pool.physical(sid);
    raw[30] = 3; assert.equal(value.queries.targetingMode(sid, 1), 6);
    raw[30] = 2; assert.equal(value.queries.targetingMode(sid, 2), 4);
    // astral이면 graph 대신 원문 프레임 문자를 읽는다.
    raw[40] |= 8; put(raw, 36, value.frames[140].run('L').first); assert.equal(value.queries.targetingMode(sid, 3), 0);
});

test('CampaignGunQueries: 발사간격은 현재 TYPE의 float 표이고 시각 갱신은 실제 WORD setter다', () => {
    const { value, cannon } = cannonScene(); assert.equal(value.queries.firingInterval(cannon), 5);
    value.firingIntervals[128] = 0.1; assert.equal(value.queries.firingInterval(cannon), Math.fround(0.1));
    value.firingIntervals[128] = 7.25; assert.equal(value.queries.firingInterval(cannon), 7.25);
    value.gunState.now = 12.5; value.queries.refreshRange(cannon); assert.equal(value.gunHit.time(cannon), 12.5);
    value.gunState.now = 13.875; value.queries.refreshRange(cannon); assert.equal(value.gunHit.time(cannon), 13.875);
    value.core.pool.physical(cannon)[10] = 255; assert.throws(() => value.queries.firingInterval(cannon), /TYPE 범위/);
});

test('CampaignGunQueries: 실제 같은 finder가 적 자산을 찾고 owner/동맹 변경을 다음 조회에 반영한다', () => {
    const value = new CampaignGunScene(); value.ground(18, 18, 38, 28);
    const source = value.pop(128, 23, 23, 3, 1); const enemy = value.pop(75, 33, 23, 3, 2);
    assert.equal(value.queries.findTarget(23, 23, 128, value.queries.targetingMode(source), 1), enemy);
    value.core.owner.set(enemy, 1); assert.equal(value.queries.findTarget(23, 23, 128, 4, 1), 0);
    value.core.owner.set(enemy, 2); value.targetState.allyMasks[1] |= 1 << 2;
    assert.equal(value.queries.findTarget(23, 23, 128, 4, 1), 0);
    value.targetState.allyMasks[1] &= ~(1 << 2); assert.equal(value.queries.findTarget(23, 23, 128, 4, 1), enemy);
});

test('CampaignGunQueries: 실제 Gun Form 체인과 부모 상태 손상을 원문 진단으로 거부한다', () => {
    const { value, cannon } = cannonScene(); value.queries.checkForms(cannon);
    const raw = value.core.pool.physical(cannon); const form = get(raw, 6, 2); assert.ok(form);
    const child = value.core.pool.physical(form); const saved = child[10]; child[10] = 70;
    assert.throws(() => value.queries.checkForms(cannon), /parent.checkForms/); child[10] = saved; value.queries.checkForms(cannon);
    raw[11] |= 4; assert.throws(() => value.queries.checkForms(cannon), /parent.checkForms/); raw[11] &= ~4; value.queries.checkForms(cannon);
});

test('CampaignGunQueries: 현재 footprint·checking·noIsland TYPE·owner로 지면 소유자 진단을 분배한다', () => {
    const value = new CampaignGunScene(); value.world.load([]); const support = value.pop(157, 23, 23, 3, 2);
    const cannon = value.create(128, 1); const raw = value.core.pool.physical(cannon); put(raw, 14, floatToBits(23)); put(raw, 18, floatToBits(23));
    let checking = 0; value.gunState.checking = () => checking; value.queries.validateIslandOwners(cannon);
    checking = 1; assert.throws(() => value.queries.validateIslandOwners(cannon), /줄=173/);
    value.core.bridgeState.noIslandType = 156; value.queries.validateIslandOwners(cannon);
    value.core.bridgeState.noIslandType = 157; value.core.pool.physical(support)[34] = 1; value.queries.validateIslandOwners(cannon);
    value.core.pool.physical(support)[34] = 2; assert.throws(() => value.queries.validateIslandOwners(cannon), /줄=173/);
    checking = 0; value.queries.validateIslandOwners(cannon);
});

test('CampaignGunQueries: float/DWORD/BYTE ABI·문맥 읽기 순서와 Core 참조의 값 사본', () => {
    const value = new CampaignGunScene({ assets: false }); const cannon = value.create(128, 1); const raw = value.core.pool.physical(cannon); const calls = [];
    put(raw, 14, 0x7fa12345); put(raw, 18, 0x80000000); put(raw, 36, 2);
    // 외부 장치의 명시 변경으로 원문 owner 캡처와 마지막 프레임 조회 위치를 구분한다.
    const target = { pool: value.core.pool, center: (sid) => { calls.push(['center', sid]); raw[34] = 6; return [1.25, -0]; }, findTarget: (...args) => { calls.push(args); return 91; } };
    const mode = { pool: value.core.pool, get: (sid) => { calls.push(['mode', sid]); raw[34] = 7; put(raw, 36, 0xffffffff); return 4; } };
    const core = { ...value.core }; const intervals = Array.from(value.firingIntervals); intervals[128] = 0.1;
    const hooks = makeCampaignGunQueryHooks(core, mode, target, value.gunHit, value.gunState, intervals);
    core.pool = null; core.types = [];
    assert.deepEqual(hooks.shotContext(cannon), [0x7fa12345, 0x80000000, 0x3fa00000, 0x80000000, 128, 4, 6, 0xffffffff]);
    assert.deepEqual(calls, [['center', cannon], ['mode', cannon]]);
    calls.length = 0; assert.equal(hooks.findTarget(0.1, 0.2, 0x100000080, 0xffffffff, 258), 91);
    assert.deepEqual(calls[0], [Math.fround(0.1), Math.fround(0.2), 128, -1, 2]);
    assert.equal(hooks.firingInterval(cannon), Math.fround(0.1)); intervals[128] = 0.3; assert.equal(hooks.firingInterval(cannon), Math.fround(0.3));
});
