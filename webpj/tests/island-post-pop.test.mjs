// noIsland 최초 등록 접두와 받침 소유자 전파 raw-island-post-pop.js(RawIslandPostPop)를 원본 10.78 관측 islandpostpop-x86.tsv와 cpppj 계약으로 대조한다.
// 행 종류: SurfacePostPop 402(프레임 글자 선택·F 칸 받침 생성·H 칸 소유자 전파·연결 순회)·SupportOwner 150(3×3 받침과 종유석 소유자). original1037·originalCD 행은 범위 밖이다.
// 장면 준비와 기록 훅은 bridgeconnect 시험과 공유하는 raw-scene-support.mjs 의 Scene 이다. 기대 결과는 계산하지 않고 입력만 구성한다. (2026-11-08 WB04-E47: RawIslandPostPop 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBridgeConnect } from '../www/core/raw-bridge-connect.js';
import { RawIslandPostPop } from '../www/core/raw-island-post-pop.js';
import { REPO_ROOT } from './helpers.mjs';
import { Scene, SOURCE, OWNER_OFFSET } from './raw-scene-support.mjs';
import { get, readRows } from './raw-support.mjs';

// noIsland 의 실제 3×3 글자 순서이다. 각 방향의 첫 프레임만 합성 입력으로 사용한다(번호 93 = noIsland). (2026-11-08 WB04-E47)
const LETTERS = 'FCGBADIEH';
const NO_ISLAND = 93;
const surfaceFrames = () => Array.from(LETTERS, (letter) => ({ side: letter.charCodeAt(0), variant: 80, number: 1, flags: 0 }));

// 저장된 입력/관찰은 한 번만 읽는다. (2026-11-08 WB04-E47)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'islandpostpop-x86.tsv'));
const scenes = new Map(rows.filter((row) => row[0] === 'Scene').map((row) => [row[1], row.slice(2)]));

// float 비트를 수로 읽는다. (2026-11-08 WB04-E47)
const floatView = new DataView(new ArrayBuffer(4));
function floatOf(raw, offset) {
    floatView.setUint32(0, get(raw, offset, 4), true);
    return floatView.getFloat32(0, true);
}

// 장면을 준비하고 noIsland 프레임 표를 심은 뒤 이웃 탐색기를 다시 만든다(탐색기는 생성 때의 표를 복사한다). (2026-11-08 WB04-E47)
function prepared(columns) {
    const scene = new Scene();
    scene.prepare(columns);
    scene.frames[NO_ISLAND] = surfaceFrames();
    scene.refreshNeighbors();
    return scene;
}

// 원본과 같은 효과 경계로 후처리 실행기를 만든다. state = { loadingDepth, savedGame() } 이다. (2026-11-08 WB04-E47)
function surfaceOf(scene, state, overrides = {}) {
    const objects = scene.hooks();
    const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, objects);
    const hooks = {
        objects,
        connect: (sid) => connect.connect(sid),
        findTypeAt: (x, y, type) => connect.findTypeAt(x, y, type),
        updateDisplay: (sid) => scene.events.push(`U:${sid}`),
        incompleteSupport: () => scene.events.push('D'),
        ...overrides,
    };
    return { surface: new RawIslandPostPop(scene.pool, scene.hash.entries(0), scene.types, scene.frames, scene.state, state, hooks), hooks, connect };
}

test('islandpostpop: 원본 10.78 SurfacePostPop 402행·SupportOwner 150행의 사건 순서·새 슬롯·풀 Adler 가 일치한다', () => {
    const counts = { SurfacePostPop: 0, SupportOwner: 0 };
    // 각 행은 앞 행의 변경을 이어받지 않고 입력부터 다시 준비한다.
    for (const row of rows.filter((item) => item[0] !== 'Scene' && item[1] === 'originals')) {
        const scene = prepared(scenes.get(row[2]));
        const state = { loadingDepth: Number(row[4]), savedGame: () => row[5] === '1' };
        const { surface } = surfaceOf(scene, state);
        if (row[0] === 'SurfacePostPop') {
            surface.prefix(SOURCE, Number(row[3]));
            scene.events.push(`T:50:${row[3]}`);
        } else {
            assert.equal(row[0], 'SupportOwner');
            const raw = scene.pool.slot(SOURCE);
            surface.setSupportOwner(floatOf(raw, 14), floatOf(raw, 18), Number(row[3]));
        }
        const label = `${row[0]} 장면 ${row[2]}`;
        assert.equal(scene.eventText(), row[6], `${label} 사건`);
        assert.equal(scene.bornText(), row[7], `${label} 새 슬롯`);
        assert.equal(scene.poolAdler(), Number(row[8]), `${label} 풀`);
        counts[row[0]] += 1;
    }
    assert.deepEqual(counts, { SurfacePostPop: 402, SupportOwner: 150 });
});

test('islandpostpop: 지도 범위 밖 조회는 0, 훅/지도 오류와 H 의 패치 assert 경계, 비최초·저장 게임은 아무 효과도 없다', () => {
    const scene = prepared(['50,93,0,0,1106247680,1106247680,1,1,134219779,16777218,8,0', '-', '0', '-', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    const state = { loadingDepth: 0, savedGame: () => false };
    // PATCH 비최초 경로는 도달하지 않은 표시 장치를 요구하지 않는다.
    surfaceOf(scene, state, { updateDisplay: undefined }).surface.prefix(SOURCE, 0);
    assert.equal(scene.events.length, 0);
    assert.throws(() => new RawIslandPostPop(scene.pool, new Uint16Array(0), scene.types, scene.frames, scene.state, state, {}), RangeError);
    const { surface } = surfaceOf(scene, state);
    assert.equal(surface.surfaceLetter(-1, 30), 0);
    assert.equal(surface.surfaceLetter(30, 256), 0);
    assert.equal(surface.surfaceLetter(30, 30), 0x48);
    // 왼쪽 위 칸에 받침이 없으면 PATCH 의 assert 장치가 필요하고, 없으면 도달 지점에서 거부한다.
    assert.throws(() => surface.prefix(SOURCE, 1));
    assert.equal(scene.events.length, 0);
    // flag 비트가 없으면 현재 H 여도 아무 파생 효과를 수행하지 않는다.
    surface.prefix(SOURCE, 4);
    assert.equal(scene.events.length, 0);
    // 저장 게임 관문이 켜지면 H 전파를 건너뛰고 연결 순회만 한다.
    const gated = surfaceOf(scene, { loadingDepth: 0, savedGame: () => true }).surface;
    gated.prefix(SOURCE, 1);
    assert.equal(scene.events.length, 0);
});

test('islandpostpop: 저장 게임의 같은 주소를 매 H 칸에서 다시 읽어 받침·종유석 소유자 전파를 차단하거나 실행한다', () => {
    const scene = prepared(scenes.get('0'));
    scene.putFrame(SOURCE, 8);
    // Client 가 소유한 현재 저장 게임 값이며 지역 호환 필드와 반대로 시작한다.
    let savedGame = true;
    let updates = 0;
    const { surface } = surfaceOf(scene, { loadingDepth: 0, savedGame: () => savedGame }, { updateDisplay: () => { updates += 1; } });
    const before = scene.poolAdler();
    surface.prefix(SOURCE, 1);
    assert.equal(scene.poolAdler(), before);
    assert.equal(scene.events.length, 0);
    assert.equal(updates, 0);
    savedGame = false;
    surface.prefix(SOURCE, 1);
    assert.equal(scene.pool.slot(SOURCE)[OWNER_OFFSET], 5);
    assert.equal(scene.pool.slot(80)[OWNER_OFFSET], 5);
    assert.equal(scene.pool.slot(81)[OWNER_OFFSET], 5);
    assert.ok(updates > 0 && scene.events.length > 0);
    // 같은 모듈에서 다시 true 로 바꾸면 이전 전파 결과와 무관하게 다음 H 전파를 막는다.
    savedGame = true;
    scene.pool.physical(SOURCE)[OWNER_OFFSET] = 3;
    scene.events = [];
    const previous = updates;
    surface.prefix(SOURCE, 1);
    assert.equal(scene.pool.slot(SOURCE)[OWNER_OFFSET], 3);
    assert.equal(scene.events.length, 0);
    assert.equal(updates, previous);
});

test('islandpostpop: 3×3 소유자 전파의 누락 진단은 모든 정상 칸 효과 뒤에 보고하고, 상세/assert 장치가 있으면 원문 진단으로 중단한다', () => {
    // 받침/종유석이 없는 장면: 3×3 칸 일부와 받침이 비어 있다.
    const scene = prepared(['50,93,0,0,1106247680,1106247680,1,1,134219779,16777218,8,0', '-', '0', '-', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    const state = { loadingDepth: 0, savedGame: () => false };
    // 독립 입력(상세/assert 없음): 모든 정상 칸의 소유자/표시 호출 뒤 incompleteSupport 한 번.
    surfaceOf(scene, state).surface.setSupportOwner(30, 30, 4);
    assert.equal(scene.events.at(-1), 'D');
    assert.ok(scene.events.slice(0, -1).every((event) => event.startsWith('O:') || event.startsWith('U:')));
    // 상세 문자열과 assert 장치가 연결되면 원문 진단(Damageable.cpp 0x18b)으로 끝난다. 상세에는 칸별 표시와 누락 문구가 들어간다.
    scene.events = [];
    const details = [];
    const assertions = [];
    surfaceOf(scene, state, { detail: (text) => details.push(text), assertion: (...args) => assertions.push(args), typeWas: () => 7 }).surface.setSupportOwner(30, 30, 4);
    assert.equal(assertions.length, 1);
    assert.deepEqual(assertions[0], ['0', '\\Ns\\O\\Damageable.cpp', 0x18b]);
    assert.match(details[0], /\(No islandSid\)/);
    assert.match(details[0], /TypeWas 7$/);
    // noIsland 가 아닌 칸에서는 아무것도 하지 않는다.
    scene.events = [];
    surfaceOf(scene, state).surface.setSupportOwner(100, 100, 4);
    assert.equal(scene.events.length, 0);
});
