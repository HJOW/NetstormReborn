// 초기 지형 도장의 월드 연결(cpppj `client/RawTerrainWorldBinding`)의 웹 대응 `core/raw-terrain-world-binding.js` 를 검사한다(4-B, WB04-E301).
// cpppj `TerrainWorldBindingTests.cpp` 의 두 장면을 10.78 로 옮겼다: 실제 Factory 생성부터 표면 hash0/PostPop/지역 지정/현재 객체 목록까지 같은 SID 를 유지하는지, 지역 조회/진단이 빠진 연결은 월드를 바꾸기 전에 거부하는지.
// 입력은 합성 지면 타입(97)과 합성 SHP 이며 원본 관측 파일이 없는 단위 계약 검사이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E301)
import assert from 'node:assert/strict';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameRandom } from '../www/core/random.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawTerrainStamp, TerrainStampState } from '../www/core/raw-terrain-stamp.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { makeTerrainStampWorldHooks } from '../www/core/raw-terrain-world-binding.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { realPlayerAssets } from './player-scene-support.mjs';
import { get } from './raw-support.mjs';

// 원본 isle 타입과 이번 연결 검사에서 만들 한 지면의 좌표, 문자 'x' 이다. (2026-11-11 WB04-E301)
const ISLE = 97;
const BOUND_X = 50;
const BOUND_Y = 53;
const CODE_X = 0x78;

// 실제 월드와 초기 도장이 공유하는 입력 표/문자 지도/난수 및 진단 수명이다(cpppj `TerrainWorldScene`). (2026-11-11 WB04-E301)
class TerrainWorldScene {
    // 합성 지면(실제 생성자 주소·플래그·발자국 1)과 패치 quartile 선택에 충분한 8개 P/P 후보, 합성 SHP 로 월드를 만든다.
    constructor() {
        const types = realPlayerAssets().types.map((record) => ({ ...record }));
        Object.assign(types[ISLE], { constructorAddress: typeConstructorAddress(ISLE), flags1: 0x28000803, flags2: 2, footX: 1, footY: 1 });
        const codes = types.map(() => []);
        // 후보 8개는 원본 patch quartile 선택을 모두 지원하며 fringe flags 는 0 이다.
        for (let index = 0; index < 8; index += 1) codes[ISLE].push({ side: 0x50, variant: 0x50, number: index, flags: 0 });
        const frames = codes.map((list) => new FrameCodeTable(list));
        const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
        shapes[ISLE] = { loaded: true, frameCount: 8, frames: Array.from({ length: 8 }, () => ({ width: 20, height: 16, hotspotX: 8, hotspotY: 12, cellWidth: 1, cellHeight: 1 })) };
        const links = new BridgeConnectState();
        Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true });
        this.world = new RawSurfaceWorld(types, frames, shapes, links, ISLE);
        // 원본 X열 우선 문자/지역 BYTE 배열과 실제 초기 도장의 가변 문자 대응 표이다.
        this.codes = new Uint8Array(65536).fill(0x2e);
        this.regions = new Uint8Array(65536).fill(127);
        this.mapping = new Int32Array(256);
        this.aliases = new Int32Array(256);
        const index = BOUND_X * 256 + BOUND_Y;
        this.codes[index] = CODE_X;
        this.regions[index] = 5;
        this.mapping[CODE_X] = ISLE;
        this.aliases[CODE_X] = CODE_X;
        this.map = { codes: this.codes, regions: this.regions, typeMapping: this.mapping, aliases: this.aliases, bounds: [BOUND_X, BOUND_Y, BOUND_X + 1, BOUND_Y + 1], makePredictable: 1, lastSid: { value: 0 }, excludedFlags: 0 };
        this.state = new TerrainStampState();
        this.random = new GameRandom();
        this.crt = { value: 7 };
        this.diagnostics = 0;
    }

    // 호출자에게 남은 원본 지역 조회/진단이다: 실제 지면 정상 분기에서 noRegion 조회를 요청하면 실패한다. (2026-11-11 WB04-E301)
    effects() {
        return {
            chunkRegion: () => { throw new Error('지역5 초기 지면은 noRegion 조회를 요청하지 않는다'); },
            assertion: () => { this.diagnostics += 1; },
        };
    }
}

// 실제 Factory 생성부터 표면 hash0/PostPop/지역 지정/현재 렌더 입력까지 같은 SID 를 유지한다. (2026-11-11 WB04-E301)
test('지형 월드 연결: 실제 Factory·소유자·Pop·지역', () => {
    const scene = new TerrainWorldScene();
    const core = scene.world.core();
    scene.world.beginLoad();
    const stamp = new RawTerrainStamp(core.pool, core.types, core.frames, scene.map, scene.random, scene.crt, scene.state,
        makeTerrainStampWorldHooks(scene.world, scene.state, () => { scene.diagnostics += 1; }, scene.effects()));
    stamp.stamp(CODE_X);
    const sid = scene.map.lastSid.value;
    assert.ok(sid !== 0 && scene.diagnostics === 0);
    assert.equal(scene.world.find(BOUND_X, BOUND_Y, ISLE), sid);
    const raw = core.pool.slot(sid);
    // 섬 번호 WORD 는 지역 5, 타입은 isle, 소유자는 0 이다.
    assert.deepEqual([get(raw, 8, 2), raw[10], raw[34]], [5, ISLE, 0]);
    assert.equal(core.hash.view(0)[BOUND_Y * 256 + BOUND_X], sid);
    assert.deepEqual([core.bookkeeping.globalCounts()[ISLE], core.bookkeeping.depth], [1, 0]);
    const objects = scene.world.objects();
    assert.equal(objects.length, 1);
    assert.equal(objects[0].sid, sid);
    assert.deepEqual([objects[0].object.x, objects[0].object.y, objects[0].object.owner], [BOUND_X, BOUND_Y, 0]);
    assert.notEqual(scene.state.variants, null);
    scene.world.finishLoad();
    scene.world.dispose();
});

// 현재 지역 없음 값과 필수 진단/ChunkMap callback 을 묵시 기본값으로 바꾸지 않는다: 거부는 월드를 바꾸지 않는다. (2026-11-11 WB04-E301)
test('지형 월드 연결: 누락된 외부 효과는 거부', () => {
    const scene = new TerrainWorldScene();
    const before = scene.world.objects().length;
    assert.throws(() => makeTerrainStampWorldHooks(scene.world, scene.state, null, scene.effects()), TypeError);
    assert.throws(() => makeTerrainStampWorldHooks(scene.world, scene.state, () => {}, {}), TypeError);
    assert.throws(() => makeTerrainStampWorldHooks(scene.world, scene.state, () => {}, { chunkRegion: () => 127 }), TypeError);
    assert.throws(() => makeTerrainStampWorldHooks(scene.world, scene.state, () => {}, { assertion: () => {} }), TypeError);
    assert.equal(scene.world.objects().length, before);
    // 호출자의 효과 객체는 바뀌지 않는다.
    const effects = scene.effects();
    const hooks = makeTerrainStampWorldHooks(scene.world, scene.state, () => {}, effects);
    assert.notEqual(hooks, effects);
    assert.equal(effects.create, undefined);
    assert.deepEqual(['create', 'setOwner', 'pop', 'setRegion', 'chunkRegion', 'assertion'].filter((name) => typeof hooks[name] !== 'function'), []);
    scene.world.dispose();
});

// 네 월드 훅을 직접 호출해 각각 실제 몸체로 가는지 본다: 생성 flags, 가상 소유자 분배, Pop flags(송신기가 받는 값), 지역 setter 의 진단 조건. (2026-11-11 WB04-E301)
test('지형 월드 연결: 네 훅은 실제 월드 몸체로 간다', () => {
    const scene = new TerrainWorldScene();
    const core = scene.world.core();
    const diagnostics = [];
    const hooks = makeTerrainStampWorldHooks(scene.world, scene.state, (kind) => { diagnostics.push(kind); }, scene.effects());
    const requests = [];
    scene.world.bindSquidTransmission((sid, flags) => { requests.push([sid, flags]); });
    scene.world.beginLoad();
    // 생성 flags 2 는 예측 가능한 지역 번호 범위(15000 미만), 0 은 서버 번호이다.
    const local = hooks.create(ISLE, 2);
    const remote = hooks.create(ISLE, 0);
    assert.ok(local < 15000 && remote >= 15000);
    // 소유자는 월드의 실제 SquidOwner 로 가며 원본 raw 소유자 바이트에 쓰인다.
    hooks.setOwner(remote, 3);
    assert.equal(core.pool.slot(remote)[34], 3);
    // Pop 은 호출자의 flags 를 그대로 월드 송신기까지 전한다(0x40 이 없어야 송신하며 0x20 은 호출자 flags 로 보존된다).
    hooks.pop(remote, BOUND_X, BOUND_Y, 0x21);
    assert.equal(requests.length, 1);
    assert.deepEqual([requests[0][0], requests[0][1] & 0x21], [remote, 0x21]);
    assert.equal(scene.world.find(BOUND_X, BOUND_Y, ISLE), remote);
    // 지역 setter: 섬 genus 지면은 지역 번호를 저장하고 진단이 없다.
    hooks.setRegion(remote, 9);
    assert.deepEqual([get(core.pool.slot(remote), 8, 2), diagnostics], [9, []]);
    // 다리는 지역 번호를 가질 수 없다: 현재 noRegion(127)이면 진단 없이, 다른 값이면 원본 진단 뒤 반환한다.
    const bridge = scene.world.create(82);
    hooks.setRegion(bridge, 127);
    assert.deepEqual(diagnostics, []);
    hooks.setRegion(bridge, 5);
    assert.equal(diagnostics.length, 1);
    // noRegion 은 호출 때의 현재 상태 값이다(상태를 바꾸면 5 가 지역 없음이 된다).
    scene.state.noRegion = 5;
    hooks.setRegion(bridge, 5);
    assert.equal(diagnostics.length, 1);
    scene.world.dispose();
});
