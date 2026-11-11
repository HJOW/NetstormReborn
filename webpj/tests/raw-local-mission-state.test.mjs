// 단독 미션 상태 7~10/13/14 의 블록 복사·확정·전송 수신 시점(cpppj `o/RawLocalMissionState`)의 웹 대응 `core/raw-local-mission-state.js` 를 검사한다(4-C, WB04-E320).
//  - 원본 독립 관측 `local-mission-state-x86.tsv` 33행(Request/Poll/Run × 선택 상태 × 모드·연결·Fort·메타 변형 × 호출 도중 변경)의 다섯 상태 블록 720BYTE·현재 전역 15개·두 벽시각 예약 double 비트·Template Core 192BYTE·외부 효과 순서를 재생한다.
//  - 손 시험(cpppj `LocalMissionStateTests.cpp` 이식 + 기준선 뒤 경로): 송신 위치의 phase 9 미확정과 수신 큐, 저장소 별칭·필수 소비자 누락, 큰 부모(1/2/3/6/19~42)의 확정/early return, 미복원 상태 진단, State15·4·12·14·16,
//    일반 전투 State7(밀도·Chat·연결 안내)·simulconnect listener·재접속 State7, 일반 State8/9 연결 안내, 호스트 State7 몸체(runStarting/pollStarting), 캡처·복원 함수.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E320)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { LocalMissionParentResult, LocalMissionProgress, RawLocalMissionState, captureLocalMissionStarting, restoreLocalMissionStarting } from '../www/core/raw-local-mission-state.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E320)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'local-mission-state-x86.tsv'));
// 관찰기의 Zone opaque 주소이며 호스트 포인터로 쓰지 않는다. (2026-11-11 WB04-E320)
const ZONE = 0x12500000;

// 바이트를 소문자 16진 문자열로 바꾼다. (2026-11-11 WB04-E320)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
// double 의 little endian 비트 16진 문자열이다. (2026-11-11 WB04-E320)
function doubleHex(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return hex(new Uint8Array(view.buffer));
}
// float 비트(uint32)를 만든다. (2026-11-11 WB04-E320)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 원본 선택 입력만 준비하며 상태 전환/단계 완료를 기대값에서 역산하지 않는다(cpppj StateScene 대응). (2026-11-11 WB04-E320)
class StateScene {
    // 초기 블록의 모든 stale DWORD 와 현재 외부 입력을 native 와 동일하게 준비한다: kind = Request/Poll/Run, selected = 상태 번호, flags = 선택 경로 비트, change = 호출 도중 변경 종류이다.
    constructor(kind, selected, flags, change) {
        // old/phase/immediate/current/next 의 서로 다른 실제 저장소이다.
        this.blocks = [0, 1, 2, 3, 4].map(() => new Uint32Array(36));
        this.server = false; this.boss = true; this.battle = false; this.post = true; this.saved = true; this.mission = true; this.simul = false; this.meta = false;
        this.player = new PlayerWorldState();
        this.zonePending = 0; this.ticks = 42; this.source = 17; this.seed = 88; this.metaRefresh = 0; this.metaRebuild = 0;
        this.title = 90; this.battleByte = 7;
        this.end = 120.0; this.activity = 77.0; this.seconds = 45.75; this.wall = 100.5;
        this.cores = new Uint8Array(192);
        this.address = new Uint8Array(20);
        this.variant = flags; this.mutation = change;
        this.trace = [];
        // 원문 다섯 블록의 인접 필드를 서로 구별한다.
        for (let block = 0; block < 5; block += 1) {
            for (let index = 0; index < 36; index += 1) this.blocks[block][index] = block * 1000 + index;
        }
        const input = this.blocks[kind === 'Poll' ? 4 : 3];
        input[0] = selected;
        input[32] = ZONE;
        input[33] = (this.variant & 2) !== 0 ? 1 : 0;
        input[34] = 1250;
        if (kind === 'Poll') this.blocks[3][0] = 0;
        if (kind === 'Request') this.blocks[2][0] = (this.variant & 1) ? 7 : 0;
        this.server = (this.variant & 1) !== 0;
        this.zonePending = (this.variant & 4) !== 0 ? 1 : 0;
        this.meta = (this.variant & 256) !== 0;
        this.metaRebuild = this.meta ? 1 : 0;
        this.player.playerKnown = 9;
        this.end = (this.variant & 1024) ? 0.0 : (this.variant & 32) ? 90.0 : 120.0;
        // 관찰기가 입력한 물리 Core 전체를 사용하며 수정 결과를 미리 공급하지 않는다.
        for (let index = 0; index < this.cores.length; index += 1) this.cores[index] = (index * 3 + 5) & 0xff;
        // 실제 주소 기록의 사용하지 않는 인접 BYTE 도 보존한다.
        for (let index = 0; index < this.address.length; index += 1) this.address[index] = index;
        this.address[0] = (this.variant & 128) ? 6 : 2;
        this.address[1] = 0;
        this.address[2] = 0x12; this.address[3] = 0x34; this.address[12] = 0x56; this.address[13] = 0x78;
        this.body = new RawLocalMissionState(this.storage(), this.globals(), this.hooks());
    }

    // 서로 다른 원본 다섯 영역을 같은 순서로 전달한다. (2026-11-11 WB04-E320)
    storage() {
        return { old: this.blocks[0], phase: this.blocks[1], immediate: this.blocks[2], current: this.blocks[3], next: this.blocks[4] };
    }

    // 현재 전역 묶음이다(속성으로 현재값을 읽고 쓴다). 메뉴 창 DWORD 는 immediate+84 와 같은 칸이므로 따로 두지 않는다. cdPresent 는 장면의 seed 칸에 대응한다. (2026-11-11 WB04-E320)
    globals() {
        const scene = this;
        return {
            get server() { return scene.server; }, set server(value) { scene.server = value; },
            get boss() { return scene.boss; }, set boss(value) { scene.boss = value; },
            get inBattle() { return scene.battle; }, set inBattle(value) { scene.battle = value; },
            get missionPost() { return scene.post; }, set missionPost(value) { scene.post = value; },
            get savedGame() { return scene.saved; }, set savedGame(value) { scene.saved = value; },
            get missionWorld() { return scene.mission; },
            get simulconnect() { return scene.simul; },
            get metamap() { return scene.meta; },
            player: scene.player,
            get zonePending() { return scene.zonePending; }, set zonePending(value) { scene.zonePending = value; },
            get gameTicks() { return scene.ticks; }, set gameTicks(value) { scene.ticks = value; },
            get sourceType() { return scene.source; }, set sourceType(value) { scene.source = value; },
            get cdPresent() { return scene.seed; }, set cdPresent(value) { scene.seed = value; },
            get metaRefresh() { return scene.metaRefresh; }, set metaRefresh(value) { scene.metaRefresh = value; },
            get metaRebuild() { return scene.metaRebuild; }, set metaRebuild(value) { scene.metaRebuild = value; },
            get titleInitial() { return scene.title; }, set titleInitial(value) { scene.title = value; },
            get battleByte() { return scene.battleByte; },
            get connectEnd() { return scene.end; }, set connectEnd(value) { scene.end = value; },
            get activityEnd() { return scene.activity; }, set activityEnd(value) { scene.activity = value; },
            get gameSeconds() { return scene.seconds; },
            get wallCached() { return scene.wall; },
        };
    }

    // 출력 trace 는 경계 진입 시 추가하며 상태 효과를 대신 계산하지 않는다. (2026-11-11 WB04-E320)
    event(value) {
        this.trace.push(value);
    }

    // 큰 UI/파일/전송은 native 와 같은 명시 외부 입력이고 작은 상태 계산은 실제 제품이 수행한다. (2026-11-11 WB04-E320)
    hooks() {
        const scene = this;
        return {
            logState: (state) => { scene.event(`State,${state}`); if (scene.mutation === 4) scene.blocks[3][0] = 8; },
            assertion: () => { scene.event('Assert'); },
            invalidState: () => { throw new Error('선택 native 입력 밖의 상태 진단'); },
            // 동결 trace 의 옛 이름을 유지하며 실제 의미는 4978d0 의 기본 배경 분모 선택이다.
            selectDefaultDenominator: () => { scene.event('Selection'); },
            cleanupBattle: (reason) => {
                scene.event(`Cleanup,${reason}`);
                scene.server = false; scene.boss = false; scene.post = true; scene.zonePending = 0; scene.saved = false; scene.ticks = 0; scene.source = 0;
                if (scene.mutation === 7) scene.blocks[3][33] = 0;
            },
            // 기존 동결 trace 의 Seed 는 CD 호출 경계의 옛 표식이다. 난수 몸체를 실행하지 않는다.
            cdPresent: () => { scene.event('Seed'); return 123; },
            refreshCommands: () => { scene.event('Commands'); },
            stopMusic: () => { scene.event('Music'); },
            refreshMission: () => { scene.event('Mission'); },
            installZone: (zone) => {
                scene.event(`Zone,${zone}`);
                scene.server = true; scene.boss = true;
                if (scene.mutation === 2) scene.blocks[3][32] = ZONE + 16;
            },
            freeZone: (zone) => { scene.event(`Free,${zone}`); },
            installMapHandlers: () => { scene.event('MapHandlers'); },
            // 기존 Cursor 표식도 실제 4993c0 의 명시 경계이며 새 해석은 Renderer 다.
            initializeRenderer: () => { scene.event('Cursor'); },
            uiMode: (mode) => { scene.event(`UI,${mode}`); },
            wallSeconds: () => { scene.event('Wall'); if (scene.mutation === 1) scene.end = 99.0; return scene.wall; },
            loadReconnectFort: () => { scene.event('ReconnectFort'); },
            progress: (progress, remaining) => { scene.event(`Progress,${progress},${remaining}`); },
            battlePalette: () => { scene.event('Palette'); return 77; },
            applyPalette: (value) => { scene.event(`ApplyPalette,${value}`); },
            connectServer: (raw, value) => { scene.event(`Connect,${hex(raw)},${value}`); },
            connectionStatus: (target) => {
                scene.event(`Status,${target}`);
                if (scene.mutation === 5) scene.server = true;
                if (target === -1) return (scene.variant & 512) ? 1 : 0;
                return (scene.variant & 2048) ? -1 : (scene.variant & 8) ? 0 : 1;
            },
            connectionOpen: (target) => { scene.event(`Open,${target}`); return !(scene.variant & 16); },
            closeConnection: (target) => { scene.event(`Close,${target}`); },
            renameConnection: (from, to) => { scene.event(`Rename,${from},${to}`); },
            logConnection: (line, value) => { scene.event(`ConnLog,${line},${value}`); },
            sendLogin: () => { scene.event('SendLogin'); },
            connectedProgress: () => { scene.event('ConnectedProgress'); },
            closeProgress: () => { scene.event('CloseProgress'); },
            serverConnection: () => { scene.event('ServerConnection'); return 55; },
            attachCommander: (token) => { scene.event(`Commander,${token}`); },
            fortAvailable: () => !(scene.variant & 64),
            loadCurrentFort: () => { scene.event('FortName'); scene.event(`LoadFort,${scene.player.templateOwner}`); },
            fortCore: (owner) => { scene.event(`Core,${owner}`); return scene.cores.subarray(owner * 64, owner * 64 + 64); },
            localAddress: () => { scene.event('Address'); if (scene.mutation === 3) scene.player.templateOwner = 2; return scene.address; },
            touchFort: (owner) => { scene.event(`Touch,${owner}`); },
            refreshFort: () => { scene.event('RefreshFort'); },
            setReconnectSp: (money) => { scene.event(`Sp,${floatBits(money)}`); },
            loginTimer: (start) => { scene.event(`Timer,${start ? 1 : 0}`); },
            inputMode: (value) => { scene.event(`Input,${value ? 1 : 0}`); },
            metaFirst: () => { scene.event('MetaFirst'); },
            metaSecond: () => { scene.event('MetaSecond'); },
            metaThird: () => { scene.event('MetaThird'); },
        };
    }

    // 실제 다섯 블록의 모든 720BYTE 를 원문 순서로 직렬화한다. (2026-11-11 WB04-E320)
    blockBytes() {
        return this.blocks.map((block) => hex(new Uint8Array(block.buffer, block.byteOffset, block.byteLength))).join('');
    }

    // 같은 현재 외부 전역을 native 기록 순서대로 읽는다(원본 14DWORD 와 마지막 BYTE). (2026-11-11 WB04-E320)
    globalBytes() {
        const values = [this.server ? 1 : 0, this.boss ? 1 : 0, this.battle ? 1 : 0, this.post ? 1 : 0, this.saved ? 1 : 0, this.player.playerKnown, this.player.battleOwner, this.player.templateOwner,
            this.zonePending, this.ticks, this.source, this.seed, this.metaRefresh, this.metaRebuild, this.title];
        return values.join(',');
    }

    // 독립 두 타이머의 double 비트 전체이다. (2026-11-11 WB04-E320)
    timerBytes() {
        return doubleHex(this.end) + doubleHex(this.activity);
    }

    // 실제 발생 순서의 전체 외부 효과 문자열이다(없으면 '-'). (2026-11-11 WB04-E320)
    traceText() {
        return this.trace.length ? this.trace.join(';') : '-';
    }
}

// 33행 재생: 열은 input/blocks/globals/timers/cores/trace 이다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 원본 관측 33행의 블록·전역·타이머·Core·효과 순서', { skip: ROWS === null }, () => {
    assert.equal(ROWS.length, 34);
    assert.deepEqual(ROWS[0], ['input', 'blocks', 'globals', 'timers', 'cores', 'trace']);
    // 정확한 헤더 한 행만 건너뛰고 33개 원본 입력을 모두 비교한다.
    for (let index = 1; index < ROWS.length; index += 1) {
        const row = ROWS[index];
        assert.equal(row.length, 6);
        const input = row[0].split(',');
        assert.equal(input.length, 4);
        const scene = new StateScene(input[0], Number(input[1]), Number(input[2]), Number(input[3]));
        if (input[0] === 'Request') scene.body.request(Number(input[1]));
        else if (input[0] === 'Poll') scene.body.poll();
        else scene.body.run();
        assert.equal(scene.blockBytes(), row[1], `${row[0]} 블록`);
        assert.equal(scene.globalBytes(), row[2], `${row[0]} 전역`);
        assert.equal(scene.timerBytes(), row[3], `${row[0]} 타이머`);
        assert.equal(hex(scene.cores), row[4], `${row[0]} Core`);
        assert.equal(scene.traceText(), row[5], `${row[0]} 효과`);
    }
});

// 실제 송신 위치에서는 phase 9 가 아직 확정되지 않았으며 수신을 공통부 뒤까지 미뤄야 한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 송신 위치의 수신은 phase 10 확정 뒤에 분배된다', () => {
    const scene = new StateScene('Run', 9, 1, 0);
    const hooks = scene.hooks();
    let sent = false;
    let received = false;
    hooks.sendLogin = () => {
        assert.equal(scene.blocks[1][0], 1000);
        assert.equal(scene.blocks[3][0], 9);
        sent = true;
        scene.body.enqueueReceive(() => {
            assert.equal(scene.blocks[1][0], 10);
            assert.equal(scene.blocks[3][0], 0);
            received = true;
            scene.blocks[3][0] = 13;
            scene.body.run();
        });
        // 상태 실행 중의 분배는 명시 거부한다.
        assert.throws(() => scene.body.dispatchQueued());
    };
    scene.body = new RawLocalMissionState(scene.storage(), scene.globals(), hooks);
    scene.body.run();
    assert.ok(sent && !received);
    assert.equal(scene.blocks[1][0], 10);
    scene.body.dispatchQueued();
    assert.ok(received);
    assert.deepEqual([scene.blocks[1][0], scene.blocks[3][0]], [14, 0]);
    // 큐가 비면 다시 분배해도 확정 상태가 바뀌지 않는다.
    const phase = Uint32Array.from(scene.blocks[1]);
    scene.body.dispatchQueued();
    assert.deepEqual(Array.from(scene.blocks[1]), Array.from(phase));
    assert.throws(() => scene.body.enqueueReceive(null), TypeError);
});

// 실제 저장소 별칭과 필수 효과 누락을 상태 확정 성공으로 감추지 않는다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 저장소 구성과 필수 소비자 누락', () => {
    const scene = new StateScene('Run', 7, 4, 0);
    const before = Uint32Array.from(scene.blocks[1]);
    const hooks = scene.hooks();
    hooks.cleanupBattle = undefined;
    scene.body = new RawLocalMissionState(scene.storage(), scene.globals(), hooks);
    // 필수 정리 소비자가 없으면 앞선 원문 쓰기(제목 첫 BYTE 0)만 남기고 phase 를 확정하지 않으며, 실행 중 표식은 해제되어 다시 실행할 수 있다.
    assert.throws(() => scene.body.run(), /현재전투정리미연결/);
    assert.deepEqual(Array.from(scene.blocks[1]), Array.from(before));
    assert.equal(scene.title, 0);
    assert.throws(() => scene.body.run(), /현재전투정리미연결/);
    // 같은 블록을 두 칸에 쓰면 거부한다.
    const same = { ...scene.storage(), current: scene.blocks[1] };
    assert.throws(() => new RawLocalMissionState(same, scene.globals(), scene.hooks()), RangeError);
    // 같은 버퍼에서 겹치는 보기도 거부하고 겹치지 않는 보기는 허용한다.
    const shared = new Uint32Array(36 * 6);
    const view = (index) => shared.subarray(index * 36, index * 36 + 36);
    assert.doesNotThrow(() => new RawLocalMissionState({ old: view(0), phase: view(1), immediate: view(2), current: view(3), next: view(4) }, scene.globals(), scene.hooks()));
    // 한 DWORD 만 앞 블록의 끝과 겹쳐도(current 가 immediate 의 마지막 칸을 포함) 거부한다.
    assert.throws(() => new RawLocalMissionState({ old: view(0), phase: view(1), immediate: view(2), current: shared.subarray(2 * 36 + 35, 3 * 36 + 35), next: view(4) }, scene.globals(), scene.hooks()), RangeError);
    // 뒤 블록의 시작이 앞 블록의 마지막 칸과 겹치는 반대 방향도 거부한다(next 가 current 의 마지막 칸을 포함).
    assert.throws(() => new RawLocalMissionState({ old: view(0), phase: view(1), immediate: view(2), current: view(3), next: shared.subarray(3 * 36 + 35, 4 * 36 + 35) }, scene.globals(), scene.hooks()), RangeError);
    // 블록 크기·종류가 맞지 않거나 전역 묶음이 없으면 거부한다.
    assert.throws(() => new RawLocalMissionState({ ...scene.storage(), next: new Uint32Array(35) }, scene.globals(), scene.hooks()), TypeError);
    assert.throws(() => new RawLocalMissionState({ ...scene.storage(), next: new Uint32Array(37) }, scene.globals(), scene.hooks()), TypeError);
    assert.throws(() => new RawLocalMissionState({ ...scene.storage(), old: new Uint8Array(144) }, scene.globals(), scene.hooks()), TypeError);
    assert.throws(() => new RawLocalMissionState(scene.storage(), null, scene.hooks()), TypeError);
    assert.throws(() => new RawLocalMissionState(scene.storage(), { ...scene.globals(), player: null }, scene.hooks()), TypeError);
});

// 시험 도우미: 장면의 hook 일부를 덮어쓴 본체를 만들어 장면에 연결한다. (2026-11-11 WB04-E320)
function withHooks(scene, override) {
    const hooks = { ...scene.hooks(), ...override };
    scene.body = new RawLocalMissionState(scene.storage(), scene.globals(), hooks);
    return scene.body;
}

// 큰 부모 상태(1/2/3/6/19~42)는 공통 확정 전에 실제 부모를 한 번 호출하고, 부모가 Commit 이면 old/phase/current 를 확정하며 ReturnWithoutCommit 이면 아무것도 확정하지 않는다. 그 밖의 번호는 진단한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 큰 부모 상태의 확정/early return 과 미복원 상태 진단', () => {
    for (const state of [1, 2, 3, 6, 19, 30, 42]) {
        for (const result of [LocalMissionParentResult.Commit, LocalMissionParentResult.ReturnWithoutCommit]) {
            const scene = new StateScene('Run', state, 0, 0);
            const calls = [];
            withHooks(scene, { parentState: (selected) => { calls.push(selected); return result; } }).run();
            assert.deepEqual(calls, [state], `${state} 부모 호출`);
            assert.deepEqual(scene.trace, [`State,${state}`]);
            if (result === LocalMissionParentResult.Commit) {
                // 확정: old 는 이전 phase 전체, phase 는 현재 전체, current 는 상태 번호만 0 이다.
                assert.deepEqual([scene.blocks[0][0], scene.blocks[1][0], scene.blocks[3][0], scene.blocks[1][5], scene.blocks[0][5]], [1000, state, 0, 3005, 1005]);
            } else {
                // early return: old/phase 는 그대로이고 current 의 상태 번호도 지우지 않는다.
                assert.deepEqual([scene.blocks[0][0], scene.blocks[1][0], scene.blocks[3][0], scene.blocks[1][5]], [0, 1000, state, 1005]);
            }
        }
    }
    // 부모 번호 밖(0·5·11·17·18·43·44)은 부모를 부르지 않고 진단한 뒤 일반 확정을 한다.
    for (const state of [0, 5, 11, 17, 18, 43, 44]) {
        const scene = new StateScene('Run', state, 0, 0);
        const invalid = [];
        withHooks(scene, { parentState: () => { throw new Error('부모 호출 금지'); }, invalidState: (selected) => { invalid.push(selected); } }).run();
        assert.deepEqual(invalid, [state]);
        assert.equal(scene.blocks[1][0], state);
    }
    // 부모 소비자가 없으면 거부하고 확정하지 않는다.
    const missing = new StateScene('Run', 2, 0, 0);
    withHooks(missing, { parentState: undefined });
    assert.throws(() => missing.body.run(), /큰메뉴부모상태미연결/);
    assert.equal(missing.blocks[1][0], 1000);
    // 진단 소비자가 없으면 default 도 거부한다.
    const unknown = new StateScene('Run', 5, 0, 0);
    withHooks(unknown, { invalidState: undefined });
    assert.throws(() => unknown.body.run(), /미복원상태진단미연결/);
    // 열거 값은 고정된 이름이다.
    assert.deepEqual(Object.keys(LocalMissionParentResult), ['Commit', 'ReturnWithoutCommit']);
    assert.ok(Object.isFrozen(LocalMissionProgress) && Object.isFrozen(LocalMissionParentResult));
});

// State15 는 서버 인수 부모만 부르고(몸체가 즉시 16 을 요청), 4·12·14·16 은 작은 몸체 없이 공통 확정으로 직행한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State15 서버 인수와 4·12·14·16 직행', () => {
    const scene = new StateScene('Run', 15, 0, 0);
    const taken = [];
    withHooks(scene, { takeServer: () => { taken.push(scene.blocks[3][0]); scene.body.request(16); } }).run();
    assert.deepEqual(taken, [15]);
    assert.deepEqual(scene.trace, ['State,15', 'State,16']);
    assert.deepEqual([scene.blocks[0][0], scene.blocks[1][0], scene.blocks[3][0]], [15, 16, 0]);
    // 서버 인수 부모가 없으면 거부하고 확정하지 않는다.
    const missing = new StateScene('Run', 15, 0, 0);
    withHooks(missing, { takeServer: undefined });
    assert.throws(() => missing.body.run(), /State15 실제 서버 인수 부모 미연결/);
    assert.equal(missing.blocks[1][0], 1000);
    for (const state of [4, 12, 14, 16]) {
        const direct = new StateScene('Run', state, 0, 0);
        direct.body.run();
        assert.deepEqual(direct.trace, [`State,${state}`]);
        assert.deepEqual([direct.blocks[0][0], direct.blocks[1][0], direct.blocks[3][0]], [1000, state, 0]);
    }
});

// 일반 전투 State7(미션 후처리가 아님): 서버면 밀도, 채널 0x40/0x80 을 순서대로 지우고 연결 안내를 띄우며, simulconnect 는 listener 를 미션 갱신 뒤 Zone 설치 앞에서 부른다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 일반 전투 State7 의 밀도·Chat·연결 안내와 listener', () => {
    const effects = (server, flags = 0, simul = false) => {
        const scene = new StateScene('Run', 7, flags, 0);
        scene.simul = simul;
        withHooks(scene, {
            cleanupBattle: (reason) => { scene.event(`Cleanup,${reason}`); scene.server = server; scene.post = false; scene.saved = false; scene.ticks = 0; scene.source = 0; },
            initializeDensity: () => { scene.event('Density'); },
            clearChat: (mask) => { scene.event(`Chat,${mask}`); },
            listenServer: () => { scene.event('Listen'); },
        }).run();
        return scene;
    };
    const server = effects(true);
    assert.equal(server.trace.slice(0, 15).join(';'), 'State,7;Selection;Cleanup,1;Seed;Commands;Music;Cursor;UI,2;Wall;Density;Chat,64;Chat,128;Progress,ConnectingStarting,0;Palette;ApplyPalette,77');
    // 서버가 아니면 밀도 writer 는 건너뛴다.
    assert.equal(effects(false).trace.slice(8, 14).join(';'), 'Wall;Chat,64;Chat,128;Progress,ConnectingStarting,0;Palette;ApplyPalette,77');
    // 재접속 표식(flags 2)이 있으면 일반 안내가 아니라 재접속 안내이고 savedGame 이 켜진다.
    const reconnect = effects(true, 2);
    assert.ok(reconnect.trace.includes('ReconnectFort') && reconnect.trace.includes('Progress,ReconnectStarting,0') && !reconnect.trace.includes('Density'));
    assert.equal(reconnect.saved, true);
    // 미션 후처리 월드면 미션 가상 갱신을 부르고 listener 는 simulconnect 일 때만 그 뒤에 부른다.
    const scene = new StateScene('Run', 7, 4, 0);
    scene.simul = true;
    withHooks(scene, { listenServer: () => { scene.event('Listen'); } }).run();
    const order = scene.trace.filter((entry) => ['Music', 'Mission', 'Listen', 'Zone,307232768', 'Free,307232768', 'MapHandlers', 'Cursor'].includes(entry));
    assert.deepEqual(order, ['Music', 'Mission', 'Listen', 'Zone,307232768', 'Free,307232768', 'MapHandlers', 'Cursor']);
    const noSimul = new StateScene('Run', 7, 4, 0);
    noSimul.body.run();
    assert.ok(!noSimul.trace.includes('Listen'));
    // 미션 월드가 아니면 후처리여도 미션 가상 갱신을 부르지 않는다.
    const noWorld = new StateScene('Run', 7, 0, 0);
    noWorld.mission = false;
    noWorld.body.run();
    assert.ok(!noWorld.trace.includes('Mission'));
    // 주변음 기한은 현재 벽시각 + 10, 요청 인자 칸은 20 이다(Zone 핸들 0 쓰기는 immediate 가 이미 복사본을 가져 최종 current 에는 남지 않는다).
    assert.equal(noWorld.activity, 110.5);
    assert.equal(scene.blocks[2][1], 20);
    assert.deepEqual([scene.blocks[3][32], scene.blocks[4][32]], [ZONE, ZONE]);
    assert.equal(scene.battle, true);
    assert.equal(scene.player.playerKnown, 0);
});

// 일반 연결 안내: 8 은 연결 만료 예약이 없을 때 벽시각 + 30 을 예약하고 일반 전투면 연결/재접속 안내를, 9 는 남은 초(부호 있는 절삭)를 보인다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State8·9 의 연결 안내와 남은 초', () => {
    for (const [saved, name] of [[false, 'Connecting'], [true, 'Reconnecting']]) {
        const scene = new StateScene('Run', 8, 1024, 0);
        scene.post = false;
        scene.saved = saved;
        scene.body.run();
        assert.equal(scene.end, 130.5);
        assert.deepEqual(scene.trace.slice(0, 3), ['State,8', 'Wall', `Progress,${name},0`]);
    }
    // 미션 후처리 월드는 안내를 띄우지 않는다.
    const post = new StateScene('Run', 8, 1024, 0);
    post.body.run();
    assert.ok(!post.trace.some((entry) => entry.startsWith('Progress')));
    // 이미 예약된 만료는 다시 쓰지 않는다.
    const kept = new StateScene('Run', 8, 0, 0);
    kept.body.run();
    assert.equal(kept.end, 120.0);
    // 남은 초는 캐시 시각 기준의 0 쪽 절삭이며 음수도 부호 있는 값이다.
    for (const [cached, saved, expected] of [[100.5, false, 'ConnectingCountdown,19'], [130.9, false, 'ConnectingCountdown,-10'], [100.5, true, 'ReconnectCountdown,19'], [-1e30, true, 'ReconnectCountdown,0']]) {
        const scene = new StateScene('Run', 9, 8, 0);
        scene.wall = cached;
        scene.saved = saved;
        withHooks(scene, { wallSeconds: () => { scene.event('Wall'); return 100.5; } }).run();
        assert.ok(scene.trace.includes(`Progress,${expected}`), `${cached}: ${scene.trace.join(';')}`);
        assert.equal(scene.blocks[1][0], 9);
    }
    // 시간이 만료되면 출처를 0 으로 지우고 공통 확정 없이 반환한다(다음 상태 3 을 예약).
    const expired = new StateScene('Run', 9, 32, 0);
    expired.body.run();
    assert.deepEqual([expired.source, expired.blocks[4][0], expired.blocks[1][0], expired.blocks[3][0]], [0, 3, 1000, 9]);
});

// 호스트 State7 몸체: 최초 복사/로그는 몸체보다 먼저이고 몸체가 즉시 8 을 요청하면 같은 호출에서 8 이 이어진다. 잘못된 계약은 거부한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 호스트 State7 몸체(pollStarting/runStarting)', () => {
    const polled = new StateScene('Poll', 7, 0, 0);
    polled.body.pollStarting(() => {
        polled.event('Host');
        assert.deepEqual([polled.blocks[3][0], polled.blocks[4][0]], [7, 0]);
        polled.body.request(8);
    });
    assert.deepEqual(polled.trace.slice(0, 3), ['State,7', 'Host', 'State,8']);
    assert.deepEqual([polled.blocks[0][0], polled.blocks[1][0], polled.blocks[3][0]], [7, 8, 0]);
    const run = new StateScene('Run', 7, 0, 0);
    run.body.runStarting(() => { run.event('Host'); });
    assert.deepEqual(run.trace, ['State,7', 'Host']);
    assert.deepEqual([run.blocks[1][0], run.blocks[3][0]], [7, 0]);
    // 다음 상태가 7 이 아니거나 몸체가 없으면 거부하고 아무것도 복사하지 않는다.
    const other = new StateScene('Poll', 8, 0, 0);
    assert.throws(() => other.body.pollStarting(() => {}), /next7/);
    assert.throws(() => polled.body.pollStarting(null), /next7/);
    assert.deepEqual([other.blocks[3][0], other.blocks[4][0]], [0, 8]);
    const wrong = new StateScene('Run', 8, 0, 0);
    assert.throws(() => wrong.body.runStarting(() => {}), /State7/);
    assert.throws(() => run.body.runStarting(null), /State7/);
    // 호스트 몸체가 있어도 7 이 아닌 상태는 일반 작은 몸체를 쓴다.
    const hostSeven = new StateScene('Run', 8, 0, 0);
    hostSeven.blocks[3][0] = 8;
    hostSeven.body.run();
    assert.equal(hostSeven.trace[0], 'State,8');
});

// 메타맵 선행 효과: 재구성 요청(메타맵·갱신 없음)은 첫째 효과, 갱신 요청은 둘째/셋째 효과이며 각각 표식을 지운다. 효과가 바꾼 next 는 같은 Poll 에서 소비한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: Poll 의 메타맵 선행 효과', () => {
    const both = new StateScene('Poll', 0, 256, 0);
    both.metaRefresh = 1;
    both.body.poll();
    assert.deepEqual([both.trace.join(';'), both.metaRefresh, both.metaRebuild], ['MetaSecond;MetaThird', 0, 1]);
    both.body.poll();
    assert.deepEqual([both.trace.join(';'), both.metaRebuild], ['MetaSecond;MetaThird;MetaFirst', 0]);
    // 메타맵이 아니면 어떤 효과도 부르지 않고 표식도 지우지 않는다.
    const off = new StateScene('Poll', 0, 0, 0);
    off.metaRefresh = 1;
    off.metaRebuild = 1;
    off.body.poll();
    assert.deepEqual([off.trace.length, off.metaRefresh, off.metaRebuild], [0, 1, 1]);
    // 첫째 효과가 next 를 바꾸면 같은 Poll 에서 그 상태를 실행한다.
    const chained = new StateScene('Poll', 0, 256, 0);
    withHooks(chained, { metaFirst: () => { chained.event('MetaFirst'); chained.blocks[4][0] = 4; } }).poll();
    assert.deepEqual(chained.trace, ['MetaFirst', 'State,4']);
    assert.equal(chained.blocks[1][0], 4);
    // 필수 효과가 없으면 거부하고 표식을 지우지 않는다.
    const missingFirst = new StateScene('Poll', 0, 256, 0);
    withHooks(missingFirst, { metaFirst: undefined });
    assert.throws(() => missingFirst.body.poll(), /메타맵첫효과미연결/);
    assert.equal(missingFirst.metaRebuild, 1);
    const missingSecond = new StateScene('Poll', 0, 256, 0);
    missingSecond.metaRefresh = 1;
    withHooks(missingSecond, { metaThird: undefined });
    assert.throws(() => missingSecond.body.poll(), /메타맵셋째효과미연결/);
    assert.equal(missingSecond.metaRefresh, 1);
    const missingMiddle = new StateScene('Poll', 0, 256, 0);
    missingMiddle.metaRefresh = 1;
    withHooks(missingMiddle, { metaSecond: undefined });
    assert.throws(() => missingMiddle.body.poll(), /메타맵둘째효과미연결/);
});

// 요청 중복 진단과 재진입 거부, 호출 도중 예외 뒤의 재실행. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 요청 덮어쓰기 진단과 Run 재진입', () => {
    const scene = new StateScene('Request', 8, 1, 0);
    scene.body.request(14);
    assert.deepEqual([scene.trace.join(';'), scene.blocks[2][0], scene.blocks[1][0]], ['Assert', 14, 1000]);
    const none = new StateScene('Request', 8, 0, 0);
    none.body.request(9);
    assert.deepEqual([none.trace.length, none.blocks[2][0]], [0, 9]);
    // 진단 소비자가 없으면 거부하고 요청을 덮어쓰지 않는다.
    const missing = new StateScene('Request', 8, 1, 0);
    withHooks(missing, { assertion: undefined });
    assert.throws(() => missing.body.request(14), /즉시상태중복진단미연결/);
    assert.equal(missing.blocks[2][0], 7);
    // 상태 실행 중에 Run 을 다시 부르면 거부하고, 예외 뒤에는 다시 실행할 수 있다.
    const nested = new StateScene('Run', 2, 0, 0);
    withHooks(nested, { parentState: () => { nested.body.run(); return LocalMissionParentResult.Commit; } });
    assert.throws(() => nested.body.run(), /재진입/);
    nested.body = new RawLocalMissionState(nested.storage(), nested.globals(), { ...nested.hooks(), parentState: () => LocalMissionParentResult.Commit });
    nested.blocks[3][0] = 2;
    nested.body.run();
    assert.equal(nested.blocks[1][0], 2);
});

// State13 Template 작은 필드: 주소 종류 6 이면 12 번에서 포트를 읽고, 게임 시각은 0 쪽 절삭 하위 DWORD(NaN·범위 밖은 0), 갱신 횟수는 16비트에서 감기며 Core/주소 크기가 틀리면 거부한다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State13 의 Core 작은 필드와 입력 크기', () => {
    const run = (flags, setup = () => {}) => {
        const scene = new StateScene('Run', 13, flags, 0);
        setup(scene);
        scene.body.run();
        return scene;
    };
    const core = (scene, from = 64) => Array.from(scene.cores.subarray(from, from + 24));
    const base = run(1);
    const original = Array.from({ length: 24 }, (_, index) => ((64 + index) * 3 + 5) & 0xff);
    // 모드 비트(0xfb 보존 + 2), 주소 4바이트, 포트 2바이트(바이트 교환), 게임 시각, 전투 BYTE, 갱신 횟수 +1 만 바뀐다.
    const expected = original.slice();
    expected[0] = (original[0] & 0xfb) | 2;
    expected.splice(4, 4, 4, 5, 6, 7);
    expected.splice(8, 2, 0x34, 0x12);
    expected.splice(12, 4, 45, 0, 0, 0);
    expected[0x10] = 7;
    expected.splice(0x16, 2, (original[0x16] + 1) & 0xff, original[0x17] + ((original[0x16] + 1) >> 8));
    assert.deepEqual(core(base), expected);
    // 주소 종류 6 이면 포트는 12 번 바이트에서 읽는다.
    assert.deepEqual(core(run(1 | 128)).slice(8, 10), [0x78, 0x56]);
    // 갱신 횟수는 16비트에서 감긴다.
    const wrap = run(1, (scene) => { scene.cores[64 + 0x16] = 0xff; scene.cores[64 + 0x17] = 0xff; });
    assert.deepEqual(core(wrap).slice(0x16, 0x18), [0, 0]);
    // 게임 시각: 음수는 0 쪽 절삭의 하위 DWORD, NaN·무한·범위 밖은 0.
    for (const [seconds, bytes] of [[-3.7, [0xfd, 0xff, 0xff, 0xff]], [Number.NaN, [0, 0, 0, 0]], [Infinity, [0, 0, 0, 0]], [1e30, [0, 0, 0, 0]], [-1e30, [0, 0, 0, 0]], [4294967297.9, [1, 0, 0, 0]], [9223372036854775807, [0, 0, 0, 0]], [-9223372036854775808, [0, 0, 0, 0]], [9223372036854779904, [0, 0, 0, 0]], [-9223372036854779904, [0, 0, 0, 0]], [4294967296 * 3 + 7, [7, 0, 0, 0]]]) {
        assert.deepEqual(core(run(1, (scene) => { scene.seconds = seconds; })).slice(12, 16), bytes, String(seconds));
    }
    // 재접속 SP 는 부호 있는 DWORD 를 float 로 바꾼다.
    const sp = run(2 | 1, (scene) => { scene.blocks[3][34] = 0xfffffff6; });
    assert.ok(sp.trace.includes(`Sp,${floatBits(-10)}`));
    // 입력 크기가 틀리면 거부한다(Core 24바이트 미만, 주소 20바이트가 아님).
    const shortCore = new StateScene('Run', 13, 1, 0);
    withHooks(shortCore, { fortCore: () => new Uint8Array(23) });
    assert.throws(() => shortCore.body.run(), /24BYTE/);
    const exactCore = new StateScene('Run', 13, 1, 0);
    withHooks(exactCore, { fortCore: () => exactCore.cores.subarray(0, 24) }).run();
    assert.equal(exactCore.blocks[1][0], 14);
    const badAddress = new StateScene('Run', 13, 1, 0);
    withHooks(badAddress, { localAddress: () => new Uint8Array(19) });
    assert.throws(() => badAddress.body.run(), /주소20BYTE/);
    const longAddress = new StateScene('Run', 13, 1, 0);
    withHooks(longAddress, { localAddress: () => new Uint8Array(21) });
    assert.throws(() => longAddress.body.run(), /주소20BYTE/);
});

// 재접속 State7 의 정리 전 보관 값과 복원 순서(시각 → 출처), 재접속이 아니면 보관하지 않는다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State7 의 캡처·복원 함수', () => {
    const current = new Uint32Array(36);
    const globals = { zonePending: 5, titleInitial: 9, gameTicks: 11, sourceType: 13 };
    // 재접속 표식이 없으면 Zone 대기만 보관하고 제목 첫 BYTE 를 0 으로 쓴다.
    assert.deepEqual(captureLocalMissionStarting(current, globals), { zonePending: 5, gameTicks: 0, sourceType: 0 });
    assert.equal(globals.titleInitial, 0);
    current[33] = 1;
    globals.titleInitial = 9;
    assert.deepEqual(captureLocalMissionStarting(current, globals), { zonePending: 5, gameTicks: 11, sourceType: 13 });
    // 복원은 현재 표식을 다시 읽어 시각 → 출처 순서로 쓴다.
    const order = [];
    const target = {
        set gameTicks(value) { order.push(['gameTicks', value]); },
        set sourceType(value) { order.push(['sourceType', value]); },
    };
    restoreLocalMissionStarting(current, { gameTicks: 21, sourceType: 22 }, target);
    assert.deepEqual(order, [['gameTicks', 21], ['sourceType', 22]]);
    current[33] = 0;
    restoreLocalMissionStarting(current, { gameTicks: 31, sourceType: 32 }, target);
    assert.equal(order.length, 2);
    // 정리가 표식을 바꾸면 보관 당시가 아니라 복원 당시의 표식을 따른다.
    const scene = new StateScene('Run', 7, 2, 7);
    scene.body.run();
    assert.deepEqual([scene.ticks, scene.source], [0, 0]);
});

// 저장소 구성의 경계: 종류(Uint32Array)·메모리 배치 방향·한 DWORD 겹침·맨 앞 블록과의 겹침·전역 묶음의 player. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 저장소 종류와 겹침의 세부 경계', () => {
    const scene = new StateScene('Run', 7, 0, 0);
    const make = (state, globals = scene.globals()) => new RawLocalMissionState(state, globals, scene.hooks());
    const shared = new Uint32Array(36 * 8);
    const view = (from, count = 36) => shared.subarray(from * 36, from * 36 + count);
    // 부호 있는 정수 배열이나 일반 배열은 길이가 36 이어도 거부한다.
    assert.throws(() => make({ ...scene.storage(), next: new Int32Array(36) }), TypeError);
    assert.throws(() => make({ ...scene.storage(), next: Array.from({ length: 36 }, () => 0) }), TypeError);
    // 목록에서 앞선 블록이 메모리에서는 뒤에 있어도(인접) 겹치지 않으면 허용한다.
    assert.doesNotThrow(() => make({ old: view(4), phase: view(3), immediate: view(2), current: view(1), next: view(0) }));
    // 한 DWORD 만 겹쳐도 반대 방향(목록에서 앞선 블록이 메모리 뒤쪽)에서 거부한다.
    assert.throws(() => make({ old: shared.subarray(35, 71), phase: view(0), immediate: view(3), current: view(4), next: view(5) }), RangeError);
    // 맨 앞 블록(old)이 phase 와만 겹쳐도 거부한다.
    assert.throws(() => make({ old: view(0), phase: shared.subarray(35, 71), immediate: view(3), current: view(4), next: view(5) }), RangeError);
    // 전역 묶음의 player 가 없거나 객체가 아니면 거부한다.
    assert.throws(() => make(scene.storage(), { ...scene.globals(), player: undefined }), TypeError);
    assert.throws(() => make(scene.storage(), { ...scene.globals(), player: 5 }), TypeError);
});

// 필수 소비자와 계약 입력: 로그 hook 누락, 호스트 몸체 없는 State7 호출, 실행 중 Poll 재진입. (2026-11-11 WB04-E320)
test('로컬 미션 상태: 로그 소비자 누락과 호스트 몸체 계약', () => {
    const noLog = new StateScene('Run', 4, 0, 0);
    withHooks(noLog, { logState: undefined });
    assert.throws(() => noLog.body.run(), /현재상태로그미연결/);
    assert.equal(noLog.blocks[1][0], 1000);
    // 몸체 없이 next 7 / current 7 을 소비하면 거부하고 복사도 하지 않는다.
    const poll = new StateScene('Poll', 7, 0, 0);
    assert.throws(() => poll.body.pollStarting(null), /next7/);
    assert.deepEqual([poll.blocks[3][0], poll.blocks[4][0]], [0, 7]);
    const run = new StateScene('Run', 7, 0, 0);
    assert.throws(() => run.body.runStarting(undefined), /State7/);
    assert.equal(run.trace.length, 0);
    // 메타맵이 아니면 재구성 표식이 있어도 첫째 효과를 부르지 않고 표식을 지우지 않는다.
    const plain = new StateScene('Poll', 0, 0, 0);
    plain.metaRebuild = 1;
    plain.body.poll();
    assert.deepEqual([plain.trace.length, plain.metaRebuild], [0, 1]);
    // 상태 실행 중 Poll 은 next 를 소비한 뒤에야 재진입으로 거부된다(next 번호는 지워진다).
    const nested = new StateScene('Run', 2, 0, 0);
    withHooks(nested, { parentState: () => { nested.blocks[4][0] = 5; nested.body.poll(); return LocalMissionParentResult.Commit; } });
    assert.throws(() => nested.body.run(), /재진입/);
    assert.equal(nested.blocks[4][0], 0);
});

// State7 의 세부 경계: CD 확인 값은 부호 없는 DWORD, Zone 설치는 현재 블록 값, 맵 처리기 등록 시점에 Zone 핸들은 이미 0. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State7 의 CD 값·Zone 핸들 조회 시점', () => {
    for (const [returned, expected] of [[0xfffffffe, 0xfffffffe], [-1, 0xffffffff], [123, 123]]) {
        const scene = new StateScene('Run', 7, 4, 0);
        withHooks(scene, { cdPresent: () => returned }).run();
        assert.equal(scene.seed, expected);
    }
    // 앞선 효과(명령 숨김)가 현재 Zone 핸들을 바꾸면 설치/해제는 그 현재 값을 쓴다(복사해 둔 next 값이 아님).
    const scene = new StateScene('Run', 7, 4, 0);
    const seen = [];
    withHooks(scene, {
        refreshCommands: () => { scene.blocks[3][32] = 0x777; },
        installZone: (zone) => { seen.push(['install', zone]); },
        freeZone: (zone) => { seen.push(['free', zone]); },
        installMapHandlers: () => { seen.push(['handlers', scene.blocks[3][32], scene.blocks[4][32]]); },
    }).run();
    assert.deepEqual(seen, [['install', 0x777], ['free', 0x777], ['handlers', 0, 0]]);
});

// State13 세부 경계: 템플릿 owner 는 battleOwner 와 구별, Core/주소는 실제 Uint8Array, 주소 종류는 2바이트, 포트 바이트 교환, 큰 진행창 칸 지우기 시점. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State13 의 owner·입력 종류·포트·표식 지우기', () => {
    const owners = new StateScene('Run', 13, 1 | 64, 0);
    owners.player.battleOwner = 2;
    owners.player.templateOwner = 1;
    const asked = [];
    withHooks(owners, {
        fortAvailable: (owner) => { asked.push(['available', owner]); return false; },
        fortCore: (owner) => { asked.push(['core', owner]); return owners.cores.subarray(owner * 64, owner * 64 + 64); },
        touchFort: (owner) => { asked.push(['touch', owner]); },
    }).run();
    assert.deepEqual(asked, [['available', 1], ['core', 1], ['touch', 1]]);
    // Core/주소가 일반 배열이면 길이가 맞아도 거부한다.
    const arrayCore = new StateScene('Run', 13, 1, 0);
    withHooks(arrayCore, { fortCore: () => Array.from({ length: 64 }, () => 0) });
    assert.throws(() => arrayCore.body.run(), RangeError);
    const arrayAddress = new StateScene('Run', 13, 1, 0);
    withHooks(arrayAddress, { localAddress: () => Array.from({ length: 20 }, () => 0) });
    assert.throws(() => arrayAddress.body.run(), RangeError);
    // 주소 종류는 2바이트이다: 6 이어도 상위 바이트가 있으면 6 이 아니다.
    const kind = new StateScene('Run', 13, 1 | 128, 0);
    kind.address[1] = 1;
    kind.body.run();
    assert.deepEqual(Array.from(kind.cores.subarray(64 + 8, 64 + 10)), [0x34, 0x12]);
    // 포트 상위/하위 바이트는 각각 8비트 전체가 교환된다.
    const port = new StateScene('Run', 13, 1, 0);
    port.address[2] = 0xfe;
    port.address[3] = 0xdc;
    port.body.run();
    assert.deepEqual(Array.from(port.cores.subarray(64 + 8, 64 + 10)), [0xdc, 0xfe]);
    // 재접속 표식(+84)은 immediate 와 next 모두 SP 지정과 서버 연결 조회보다 먼저 지워진다.
    const mark = new StateScene('Run', 13, 2 | 1, 0);
    const marks = [];
    withHooks(mark, {
        setReconnectSp: () => { marks.push(['sp', mark.blocks[2][33], mark.blocks[4][33], mark.blocks[3][33]]); },
        connectionOpen: () => { marks.push(['open', mark.blocks[2][33], mark.blocks[4][33], mark.blocks[3][33]]); return true; },
    }).run();
    assert.deepEqual(marks, [['sp', 0, 0, 1], ['open', 0, 0, 1]]);
});

// 연결 상태의 세부 경계: 8 은 호출 도중 바뀐 현재 연결 값을 쓰고, 9 의 만료는 같은 시각에서 즉시 만료이며, 수신 큐는 선입선출이다. (2026-11-11 WB04-E320)
test('로컬 미션 상태: State8 연결 값 시점·State9 만료 경계·수신 큐 순서', () => {
    const value = new StateScene('Run', 8, 1024, 0);
    withHooks(value, { wallSeconds: () => { value.event('Wall'); value.blocks[3][31] = 0x999; return value.wall; } }).run();
    assert.ok(value.trace.includes('Connect,cd0b0000ce0b0000cf0b0000d00b0000d10b0000,2457'));
    // 만료 시각과 현재 벽시각이 같으면 이미 만료이다.
    const same = new StateScene('Run', 9, 0, 0);
    same.end = 100.5;
    same.body.run();
    assert.deepEqual([same.trace.join(';'), same.source, same.blocks[4][0], same.blocks[1][0]], ['State,9;Status,9;Wall', 0, 3, 1000]);
    // 수신 큐는 예약한 순서대로 실행하고 실행 중 추가된 예약도 같은 호출에서 뒤에 이어 실행한다.
    const queue = new StateScene('Run', 4, 0, 0);
    const order = [];
    queue.body.enqueueReceive(() => { order.push(1); queue.body.enqueueReceive(() => { order.push(4); }); });
    queue.body.enqueueReceive(() => { order.push(2); });
    queue.body.enqueueReceive(() => { order.push(3); });
    queue.body.dispatchQueued();
    assert.deepEqual(order, [1, 2, 3, 4]);
});
