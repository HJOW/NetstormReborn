// 미리보기 전체 몸체와 실제 Query/Prepare를 원본522행으로 대조한다. (2026-11-14 WB04-E403)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { CursorPreviewScene, CHAIN, REPEAT } from './cursor-preview-support.mjs';
import { RawCursorPreviews, bindCursorPreviews, bindCursorPreviewPlacement, bindCursorPreviewWorld } from '../www/core/raw-cursor-previews.js';
import { RawCursorPlacement } from '../www/core/raw-cursor-placement.js';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { Scene, SOURCE } from './raw-scene-support.mjs';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { GameRandom } from '../www/core/random.js';
import { RawBridgeConnect } from '../www/core/raw-bridge-connect.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';

// 관측 파일이 없으면 명시 skip하며 정상 작업 트리에서는522행 모두 필요하다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-previews-x86.tsv'));

test('CursorPreviews_NativeGenerationReuseAndPlacementChain: 원본522행의 생성·재사용·다리 꼬리와32개 실제 위치 사슬이 같다', { skip: rows === null }, () => {
    assert.equal(rows.length, 522); const scene = new CursorPreviewScene(); let chains = 0;
    // 입력마다 새 전역을 주되 repeat은 같은 실제 목록/풀에 이어 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 4); const input = row[0].split(',').map(Number); assert.equal(input.length, 14); scene.prepare(input); const hooks = scene.hooks(); const ph = scene.placementHooks(hooks);
        const placement = new RawCursorPlacement(scene.pool, scene.types, [], scene.drop, scene.life, scene.state, ph);
        if (input[CHAIN]) { bindCursorPreviewPlacement(hooks, scene.pool, placement, ph); chains += 1; }
        const body = new RawCursorPreviews(scene.pool, scene.types, scene.drop, scene.life, scene.surfaces, hooks); const life = {}; bindCursorPreviews(life, body);
        // 두 번째 호출은 첫 호출이 실제로 만든 목록을 재사용한다.
        for (let call = 0; call < input[REPEAT]; call += 1) life.createPreviews();
        assert.equal(scene.trace.join(';') || '-', row[1], row[0]); assert.equal(scene.snapshot(), row[2], row[0]); assert.equal(scene.raws(), row[3], row[0]);
    }
    assert.equal(chains, 32);
});

test('CursorPreviews_RequiresDependenciesAndOneWorld: 누락·판본·다른 풀을 성공으로 대체하지 않는다', () => {
    const s = new CursorPreviewScene().prepare(); const missing = new RawCursorPreviews(s.pool, s.types, s.drop, s.life, s.surfaces);
    assert.throws(() => missing.create(), /의존 동작/); assert.equal(missing.pool, s.pool);
    const cd = { edition: 'cd1072' }; assert.throws(() => new RawCursorPreviews(cd, [], s.drop, s.life, s.surfaces), /판본 계약/);
    const hooks = s.hooks(); const ph = s.placementHooks(hooks); const placement = new RawCursorPlacement(s.pool, s.types, [], s.drop, s.life, s.state, ph);
    assert.throws(() => bindCursorPreviewPlacement(hooks, cd, placement, ph), /단일 풀/); assert.throws(() => bindCursorPreviewPlacement(hooks, s.pool, placement, {}), /Canon/);
    assert.throws(() => bindCursorPreviewWorld({}, s.pool, { pool: cd }, { pool: s.pool }, { pool: s.pool }, [], s.surfaces), /단일 풀/);
});

test('ClientCursorGlobals_CreateOverwritesInactiveDwordsAtActualAppend: 생성 모드의 비활성 높은DWORD를 실제 append에서 덮어쓴다', () => {
    const s = new CursorPreviewScene().prepare([2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0]); const owner = new ClientCursorGlobals(); s.life = owner.lifecycle();
    s.life.abstractType = 100; s.life.previews[1] = 0xffffffff; s.life.previews[29] = 0x87654321; const hooks = s.hooks(); const walk = hooks.walk;
    hooks.walk = (q, visit) => { let count = 0; walk(q, (cell) => ++count <= 2 ? visit(cell) : false); };
    new RawCursorPreviews(s.pool, s.types, s.drop, s.life, s.surfaces, hooks).create();
    assert.equal(s.nextMain, 102); assert.equal(s.life.previewCount, 2); assert.equal(s.life.previews[0], 100); assert.equal(s.life.previews[1], 101); assert.equal(s.life.previews[29], 0x87654321);
    assert.equal(s.trace.at(-1), 'P'); assert.equal(owner.lifecycle(), s.life);
});

test('CursorPreviews 진단 prefix: 연결7개를 보정하고 본체 위치를 되돌린 다음 진단하며 현재 count를 유지한다', () => {
    const s = new CursorPreviewScene().prepare([1, 1, 0, 4, 0, 7, 1, 0, floatToBits(-1.25), floatToBits(2.25), 0, 1, 0, 0]);
    assert.throws(() => new RawCursorPreviews(s.pool, s.types, s.drop, s.life, s.surfaces, s.hooks()).create(), /NUM_CONNECTORS/);
    assert.equal(s.life.previewCount, 1); assert.ok(!s.trace.includes('P')); const main = s.pool.slot(100); const last = s.pool.slot(206);
    assert.equal(bitsToFloat(get(main, 14)), -1.25); assert.equal(bitsToFloat(get(main, 18)), 2.25);
    assert.equal(bitsToFloat(get(last, 14)), -2.25); assert.equal(bitsToFloat(get(last, 18)), 1.25);
});

test('CursorPreviewWorld 실제 연결: Factory·현재 HP·프레임 표·다리 연결기가 같은 풀과DWORD 꼬리를 사용한다', () => {
    const bridgeRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/bridgeconnect-x86.tsv'));
    const row = bridgeRows.find((r) => r[0] === 'Connect' && r[1] === 'originals' && r[3] === '1' && r[7].includes(','));
    assert.ok(row); const input = bridgeRows.find((r) => r[0] === 'Scene' && r[1] === row[2]); const s = new Scene(); s.prepare(input.slice(2));
    Object.assign(s.types[100], { flags1: 0x10, maxHitPoints: 125 });
    const factory = new SquidFactory(s.pool, s.types); const reward = new SquidReward(s.pool, s.types, { localOwner: 1, aiAttached: Array(9).fill(false) }, new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    reward.bindCurrentTypes(s.types); const bridgeHooks = s.hooks(); bridgeHooks.create = (type, flags) => factory.create(type, flags);
    bridgeHooks.pop = () => assert.fail('미리보기의 공간 Pop 없음'); const connect = new RawBridgeConnect(s.pool, s.neighbors, s.state, bridgeHooks);
    const frames = s.frames.map((codes) => ({ frames: new FrameCodeTable(codes) })); const notify = []; const owner = () => 17; const hooks = { setOwner: owner };
    bindCursorPreviewWorld(hooks, s.pool, factory, reward, connect, frames, s.state, (sid) => notify.push([sid, get(s.pool.slot(sid), 12, 2)]));
    assert.equal(hooks.setOwner, owner); const born = hooks.create(100, 2); assert.equal(born, 128); assert.equal(s.pool.slot(born)[10], 100); assert.equal(hooks.hasHitPoints(born), true);
    hooks.setHitPoints(born, 0); assert.equal(get(s.pool.slot(born), 26), 0); s.types[100].flags1 = 0; assert.equal(hooks.hasHitPoints(born), false);
    assert.equal(hooks.frames(s.state.bridgeType), frames[s.state.bridgeType].frames); assert.throws(() => hooks.frames(188), /프레임 표 범위/);
    hooks.setWord(SOURCE, 0xface1234); assert.deepEqual(notify, [[SOURCE, 0x1234]]);
    const output = new Uint32Array(8); const count = hooks.connect(SOURCE, output); assert.equal(count, row[7].split(',').length);
    assert.ok(output.subarray(0, count).every((sid) => sid > born && s.pool.slot(sid)[10] === s.state.connectorType)); assert.ok(output.subarray(count).every((sid) => sid === 0));
    const incomplete = {}; bindCursorPreviewWorld(incomplete, s.pool, factory, reward, connect, frames, s.state);
    assert.throws(() => incomplete.setWord(SOURCE, 0x8765), /의존 동작/); assert.equal(get(s.pool.slot(SOURCE), 12, 2), 0x8765);
    assert.throws(() => hooks.connect(SOURCE, new Uint32Array(0)), / 꼬리 넘침/);
});
