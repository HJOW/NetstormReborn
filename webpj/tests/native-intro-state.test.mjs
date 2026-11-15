// 인트로 상태 native-intro-state.js(installNativeIntroState: State 39/40/41/42)를 원본 10.78 관측 17행과 cpppj `NativeIntroStateTests` 계약으로 대조한다(WB04-E532).
//  - 원본 관측 native-intro-state-x86.tsv 17행: 입력(state·준비 여부·영상 이름 유무·영상 활성·열기 결과·중단 지점)에서 종료 종류(menu/stop/normal)·다섯 144BYTE 상태 블록 전체·fort/server/boss 모드·pending 이름 16BYTE 전체·
//    효과 순서(cleanup·input·hide·core·renderer·hud·path·mark·open·pending·state:N)를 실제 `RawLocalMissionState` 분배기와 함께 재생해 대조한다.
//  - 계약: 39/40/41/42 밖의 상태는 장치 호출 전에 거부하고, 선택하지 않은 장치는 요구하지 않으며, 준비 없이 영상이 없으면 바로 State 41 이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E532)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { RawLocalMissionState } from '../www/core/raw-local-mission-state.js';
import { installNativeIntroState } from '../www/core/native-intro-state.js';

// 원본 관측 17행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E532)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'native-intro-state-x86.tsv'));

// 중단(영상 이름 지점/메뉴 진입)을 나타내는 검사 전용 신호이다. (2026-11-15 WB04-E532)
class IntroStop extends Error {}

// 144BYTE 상태 블록의 소문자 16진이다. (2026-11-15 WB04-E532)
const blockHex = (block) => Buffer.from(block.buffer, block.byteOffset, 144).toString('hex');

// 한 행을 새 상태에서 재생한다(cpppj 시험과 같은 분배기·장치 배선). (2026-11-15 WB04-E532)
function replay(fields) {
    const blocks = Array.from({ length: 5 }, () => new Uint32Array(36));
    // 원문 입력과 같은 전체 구별값을 설치한다.
    for (let index = 0; index < 5; index += 1) for (let word = 0; word < 36; word += 1) blocks[index][word] = index * 1000 + word;
    blocks[3][0] = Number(fields[0]);
    blocks[3][35] = Number(fields[1]);
    const mode = { fort: false, server: false, boss: false, battle: false, post: false, saved: false, mission: false, simul: false, meta: false };
    const scene = { zone: 0, ticks: 0, source: 0, seed: 0, refresh: 0, rebuild: 0, title: 0, battleByte: 0, connect: 0, activity: 0, seconds: 0, wall: 0 };
    const player = new PlayerWorldState();
    const pending = new Uint8Array(16).fill(0xa5);
    pending.set([78, 101, 120, 116, 0]);
    const trace = [];
    let status = 'normal';
    // 도달한 효과 직전의 검사 중단이다.
    const event = (text) => {
        trace.push(text);
        if (fields[5] === text) {
            status = 'stop';
            throw new IntroStop('stop');
        }
    };
    const box = (name) => ({ get value() { return mode[name]; }, set value(next) { mode[name] = Boolean(next); } });
    const intro = {
        cleanup: () => event('cleanup'),
        hideChatInput: () => event('input'),
        hideChat: () => event('hide'),
        core: () => event('core'),
        renderer: () => event('renderer'),
        hud: () => event('hud'),
        moviePath: () => { event('path'); return fields[2] === '1' ? 'MOVIE/missing.smk' : ''; },
        markIntro: () => event('mark'),
        openMovie: (moviePath) => { assert.equal(moviePath, 'MOVIE/missing.smk'); event('open'); },
        movieActive: () => fields[3] !== '0',
    };
    const storage = { old: blocks[0], phase: blocks[1], immediate: blocks[2], current: blocks[3], next: blocks[4] };
    const hooks = {
        logState: (selected) => {
            trace.push(`state:${selected}`);
            if (selected === 3) {
                status = 'menu';
                throw new IntroStop('menu');
            }
        },
        parentState: (selected) => {
            if (selected === 41) event('pending');
            return installNativeIntroState({ state: storage, fort: box('fort'), server: box('server'), boss: box('boss'), pendingName: pending }, intro);
        },
    };
    const globals = {
        get server() { return mode.server; }, set server(value) { mode.server = value; },
        get boss() { return mode.boss; }, set boss(value) { mode.boss = value; },
        get inBattle() { return mode.battle; }, set inBattle(value) { mode.battle = value; },
        get missionPost() { return mode.post; }, set missionPost(value) { mode.post = value; },
        get savedGame() { return mode.saved; }, set savedGame(value) { mode.saved = value; },
        get missionWorld() { return mode.mission; },
        get simulconnect() { return mode.simul; },
        get metamap() { return mode.meta; },
        player,
        get zonePending() { return scene.zone; }, set zonePending(value) { scene.zone = value; },
        get gameTicks() { return scene.ticks; }, set gameTicks(value) { scene.ticks = value; },
        get sourceType() { return scene.source; }, set sourceType(value) { scene.source = value; },
        get cdPresent() { return scene.seed; }, set cdPresent(value) { scene.seed = value; },
        get metaRefresh() { return scene.refresh; }, set metaRefresh(value) { scene.refresh = value; },
        get metaRebuild() { return scene.rebuild; }, set metaRebuild(value) { scene.rebuild = value; },
        get titleInitial() { return scene.title; }, set titleInitial(value) { scene.title = value; },
        get battleByte() { return scene.battleByte; },
        get connectEnd() { return scene.connect; }, set connectEnd(value) { scene.connect = value; },
        get activityEnd() { return scene.activity; }, set activityEnd(value) { scene.activity = value; },
        get gameSeconds() { return scene.seconds; },
        get wallCached() { return scene.wall; },
    };
    const body = new RawLocalMissionState(storage, globals, hooks);
    intro.request = (next) => { body.request(next); };
    try {
        body.run();
    } catch (error) {
        if (!(error instanceof IntroStop)) throw error;
    }
    return {
        status,
        raw: blocks.map(blockHex).join(':'),
        modes: [mode.fort, mode.server, mode.boss].map(Number).join(','),
        pending: Buffer.from(pending).toString('hex'),
        events: trace.join(';'),
    };
}

test('인트로 상태: 원본 10.78 17입력의 다섯 144BYTE 상태·모드·pending 이름·부분 효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 17);
    let cases = 0;
    // 각 원시 CPU 관측을 별도 상태에 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 11);
        const actual = replay(row);
        const label = `행 ${cases}: ${row.slice(0, 6).join(',')}`;
        assert.equal(actual.status, row[6], label);
        assert.equal(actual.raw, row[7], label);
        assert.equal(actual.modes, row[8], label);
        assert.equal(actual.pending, row[9], label);
        assert.equal(actual.events, row[10], label);
        cases += 1;
    }
    assert.equal(cases, 17);
});

test('인트로 상태 계약: 39/40/41/42 밖은 장치 호출 전에 거부하고 선택하지 않은 장치는 요구하지 않는다', () => {
    const make = (current0, current35 = 0) => {
        const blocks = Array.from({ length: 5 }, () => new Uint32Array(36));
        blocks[3][0] = current0;
        blocks[3][35] = current35;
        const modes = { fort: false, server: false, boss: false };
        const box = (name) => ({ get value() { return modes[name]; }, set value(next) { modes[name] = Boolean(next); } });
        return { blocks, modes, globals: { state: { old: blocks[0], phase: blocks[1], immediate: blocks[2], current: blocks[3], next: blocks[4] }, fort: box('fort'), server: box('server'), boss: box('boss'), pendingName: new Uint8Array(4).fill(7) } };
    };
    assert.throws(() => installNativeIntroState(make(43).globals, {}), /State39\/40\/41\/42 계약/);
    // State 39 준비 없음은 State 41 예약만 요구한다.
    const requests = [];
    const none = make(39, 0);
    installNativeIntroState(none.globals, { request: (next) => { requests.push(next); } });
    assert.deepEqual(requests, [41]);
    assert.deepEqual(none.modes, { fort: false, server: false, boss: false });
    // State 40: 영상이 없으면 State 41, 있으면 next[0] 을 40 으로 유지한다.
    const active = make(40);
    installNativeIntroState(active.globals, { movieActive: () => true });
    assert.equal(active.blocks[4][0], 40);
    // State 41: pending 이름 첫 BYTE 만 지우고 State 3 을 예약하며 이름 물리가 없으면 거부한다.
    const back = make(41);
    installNativeIntroState(back.globals, { request: (next) => { requests.push(next); } });
    assert.deepEqual(Array.from(back.globals.pendingName), [0, 7, 7, 7]);
    assert.equal(requests.at(-1), 3);
    const noName = make(41);
    noName.globals.pendingName = new Uint8Array(0);
    assert.throws(() => installNativeIntroState(noName.globals, { request: () => {} }), RangeError);
    // 준비된 State 39 는 선택된 효과가 없으면 앞선 모드 쓰기 전에 거부한다.
    const prepared = make(39, 1);
    assert.throws(() => installNativeIntroState(prepared.globals, {}), /실제 장치 미연결/);
    assert.deepEqual(prepared.modes, { fort: false, server: false, boss: false });
});
