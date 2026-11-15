// 실제 캠페인 내용물 수명 연결 raw-campaign-lifetime-bindings.js(RawCampaignLifetimeBindings·bindPlayerContentNoticeGumps·makeCampaignDeletionEffects)를 cpppj 계약으로 대조한다(4-D C 구획, WB04-E521).
// cpppj `CampaignLifetimeTests.cpp` 중 이 단위 소유 계약: 같은 PATCH 풀 거부(등록 전), Form 6 수명의 단일 등록(중복 등록 거부), Player 가상 표만 Player 통지로 전달하고 다른 부모는
// 명시 경계로 둠, 84·103~106·134 의 입증된 (TYPE, 가상 표) 쌍의 Pre/Post 만 공통 삭제 몸체로 보내고 그 밖은 기존 분배 유지, 공통 삭제의 선택/소리 요청 변환(Refund 거부),
// Player 내용물의 Gump 등록 배열/handle 연결. Player 통지 본체(88입력 원본 관측)·내용물 수명 본체·월드 삭제기는 각 단위 시험(E9·E-content-lifetime·E213)이 소유한다.
// 기대값은 cpppj 계약과 같은 관계이며 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E521)
import assert from 'node:assert/strict';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GumpCreateState } from '../www/ui/gump-create.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { ContentLifetimeState } from '../www/core/content-form-lifetime.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawCampaignLifetimeBindings, bindPlayerContentNoticeGumps, makeCampaignDeletionEffects } from '../www/core/raw-campaign-lifetime-bindings.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { SQUID_DELETION_EFFECT } from '../www/core/squid-destroy-lifecycle.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { realPlayerAssets } from './player-scene-support.mjs';
import { put } from './raw-support.mjs';

// Player/기본 자산 가상 표와 TYPE 번호이다. (2026-11-15 WB04-E521)
const PLAYER_VTABLE = 0x50ee08;
const BASE_VTABLE = 0x501dd0;

// 실제 TYPE 188칸 월드와 같은 풀의 Form 호스트·보상·내용물 상태를 구성한다. (2026-11-15 WB04-E521)
function makeScene() {
    const real = realPlayerAssets();
    const types = real.types.map((record) => ({ ...record }));
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    const links = new BridgeConnectState();
    Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true });
    const world = new RawSurfaceWorld(types, frames, shapes, links, 83);
    const core = world.core();
    const forms = new ContentFormHost(core.pool, core.types, () => {});
    const state = new ContentLifetimeState();
    const reward = new SquidReward(core.pool, core.types, core.bookkeeping, new SquidRewardState(), new ScrambledSpStore(new GameRandom(1), () => 1));
    const notices = [];
    const player = { pool: core.pool, notify: (parent, form, reason) => { notices.push([parent, form, reason]); } };
    return { world, core, forms, state, reward, player, notices, bind: (fallbackParent = null) => new RawCampaignLifetimeBindings(world, forms, state, {}, reward, player, fallbackParent) };
}

test('RawCampaignLifetimeBindings: 다른 풀의 Player 통지는 등록 전에 거부하고 같은 풀은 Form 6 수명을 한 번만 등록한다', () => {
    const scene = makeScene();
    const other = makeScene();
    // 다른 풀의 Player 통지는 어떤 등록도 하기 전에 거부되므로 이후 올바른 구성이 계속 성공한다.
    assert.throws(() => new RawCampaignLifetimeBindings(scene.world, scene.forms, scene.state, {}, scene.reward, other.player), TypeError);
    const bindings = scene.bind();
    assert.equal(bindings.pool, scene.core.pool);
    assert.equal(bindings.contents().pool, scene.core.pool);
    // 같은 Form 분배기에 Content 클래스는 한 번만 등록된다.
    assert.throws(() => scene.bind(), /중복 등록/);
});

test('RawCampaignLifetimeBindings: parentNotice 는 Player 가상 표만 Player 통지로 보내고 다른 부모는 명시 경계로 둔다', () => {
    const scene = makeScene();
    const bindings = scene.bind();
    const playerParent = scene.world.create(152);
    put(scene.core.pool.allocated(playerParent), 0, PLAYER_VTABLE);
    bindings.parentNotice(playerParent, 77, 3);
    assert.deepEqual(scene.notices, [[playerParent, 77, 3]]);
    // 다른 가상 표의 부모는 명시 경계가 없으면 성공으로 대신하지 않는다.
    const other = scene.world.create(84);
    assert.throws(() => bindings.parentNotice(other, 78, 1), /다른 캠페인 내용물 실제 부모 통지 미연결/);
    assert.equal(scene.notices.length, 1);
    // 경계가 있으면 그 경계가 같은 인자를 받는다.
    const scene2 = makeScene();
    const calls = [];
    const bindings2 = scene2.bind((parent, form, reason) => { calls.push([parent, form, reason]); });
    const worker = scene2.world.create(84);
    bindings2.parentNotice(worker, 9, 2);
    assert.deepEqual(calls, [[worker, 9, 2]]);
    assert.equal(scene2.notices.length, 0);
});

test('RawCampaignLifetimeBindings: 입증된 (TYPE, 가상 표) 쌍의 Pre/Post 만 공통 삭제 몸체로 보내고 그 밖은 기존 분배를 유지한다', () => {
    const scene = makeScene();
    const bindings = scene.bind();
    const routed = [];
    // 공통 삭제 몸체와 기존 분배의 호출을 관찰한다(실제 표면 수명은 E248 시험이 소유한다).
    scene.core.surfaces.hooks = () => ({ emit: (event) => { routed.push(['common', event.effect, event.sid]); } });
    const fallbackCalls = [];
    const hooks = bindings.destroyHooks({ emit: (event) => { fallbackCalls.push([event.effect, event.sid]); }, selected: () => 0 });
    assert.equal(typeof hooks.selected, 'function');
    // 빈 간헐천(84)은 기본 가상 표일 때만 공통 몸체이다.
    const geyser = scene.world.create(84);
    put(scene.core.pool.allocated(geyser), 0, BASE_VTABLE);
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: geyser, parent: 0, flags: 0 });
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: geyser, parent: 0, flags: 0 });
    // Pre/Post 가 아닌 효과는 같은 자산이라도 기존 분배에 남는다.
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.transmit, sid: geyser, parent: 0, flags: 0 });
    assert.deepEqual(routed, [['common', SQUID_DESTROY_EFFECT.preDestroy, geyser], ['common', SQUID_DESTROY_EFFECT.postDestroy, geyser]]);
    assert.deepEqual(fallbackCalls, [[SQUID_DESTROY_EFFECT.transmit, geyser]]);
    // 가상 표가 다르면 같은 TYPE 84 라도 공통 몸체가 아니다.
    put(scene.core.pool.allocated(geyser), 0, 0x12345678);
    hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: geyser, parent: 0, flags: 0 });
    assert.equal(routed.length, 2);
    assert.equal(fallbackCalls.length, 2);
    // 입증된 쌍 다섯 개와 104(range, base 표)도 공통 몸체이다.
    for (const [type, table] of [[103, 0x50c3b0], [104, BASE_VTABLE], [105, 0x509810], [106, 0x5096a0], [134, 0x5070c0]]) {
        const sid = scene.world.create(type);
        put(scene.core.pool.allocated(sid), 0, table);
        hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid, parent: 0, flags: 0 });
        assert.deepEqual(routed.at(-1), ['common', SQUID_DESTROY_EFFECT.preDestroy, sid], `TYPE ${type}`);
    }
    // fallback 이 없으면 공통이 아닌 효과는 성공으로 대신하지 않는다.
    const bare = bindings.destroyHooks({});
    assert.throws(() => bare.emit({ effect: SQUID_DESTROY_EFFECT.transmit, sid: geyser, parent: 0, flags: 0 }), /다른 캠페인 실제 삭제 효과 미연결/);
});

test('RawCampaignLifetimeBindings: 가상 삭제는 공통 쌍만 공통 삭제기로 보내고 다른 파생은 fallback 을 쓰며 없으면 거부한다', () => {
    const scene = makeScene();
    const bindings = scene.bind();
    const calls = [];
    scene.core.virtualDestroy.destroy = (sid, flags, hooks) => { calls.push(['common', sid, flags, hooks]); return true; };
    const geyser = scene.world.create(84);
    put(scene.core.pool.allocated(geyser), 0, BASE_VTABLE);
    const other = scene.world.create(85);
    const marker = { marker: true };
    const destroy = bindings.destroy((sid, flags, hooks) => { calls.push(['fallback', sid, flags, hooks]); return 'fb'; });
    assert.equal(destroy(geyser, 3, marker), true);
    assert.equal(destroy(other, 4, marker), 'fb');
    assert.deepEqual(calls.map((call) => call.slice(0, 3)), [['common', geyser, 3], ['fallback', other, 4]]);
    assert.equal(calls[0][3], marker);
    assert.throws(() => bindings.destroy()(other, 0, marker), /다른 캠페인 실제 파생 삭제 미연결/);
});

test('bindPlayerContentNoticeGumps: 같은 등록 배열과 현재 handle 을 연결하고 범위 밖 등록 ID 를 거부한다', () => {
    const registry = new GumpCreateState();
    registry.entries[5] = 0x1234;
    const handled = [];
    const hooks = bindPlayerContentNoticeGumps(registry, { handle: (token, event, form, reason) => { handled.push([token, event, form, reason]); return 99; } }, { money: () => 0 });
    assert.equal(hooks.gump(5), 0x1234);
    assert.throws(() => hooks.gump(registry.entries.length), RangeError);
    // 처리 결과는 사용하지 않고 입력 훅은 보존한다.
    assert.equal(hooks.handle(1, 2, 3, 4), undefined);
    assert.deepEqual(handled, [[1, 2, 3, 4]]);
    assert.equal(typeof hooks.money, 'function');
    assert.throws(() => bindPlayerContentNoticeGumps(registry, {}, {}), TypeError);
    assert.throws(() => bindPlayerContentNoticeGumps({}, { handle: () => 0 }, {}), TypeError);
});

test('makeCampaignDeletionEffects: 선택 해제와 유닛 손실 소리를 변환하고 Refund 는 실제 reward 에 맡기며 필수 효과 누락을 거부한다', () => {
    const emitted = [];
    const lost = [];
    const effects = { emit: (event) => { emitted.push(event); } };
    const hooks = makeCampaignDeletionEffects(effects, (sid) => { lost.push(sid); });
    assert.equal(hooks.forward, effects.emit);
    hooks.emit({ effect: SQUID_DELETION_EFFECT.clearSelection, sid: 12, recipient: 0 });
    assert.deepEqual(emitted, [{ effect: SQUID_DESTROY_EFFECT.clearSelection, sid: 12, parent: 0, flags: 0 }]);
    hooks.emit({ effect: SQUID_DELETION_EFFECT.unitLostSound, sid: 13, recipient: 0 });
    assert.deepEqual(lost, [13]);
    assert.throws(() => hooks.emit({ effect: SQUID_DELETION_EFFECT.refund, sid: 14, recipient: 0 }), /Refund는 같은 실제 SquidReward/);
    assert.throws(() => makeCampaignDeletionEffects({}, () => {}), TypeError);
    assert.throws(() => makeCampaignDeletionEffects(effects, null), TypeError);
});
