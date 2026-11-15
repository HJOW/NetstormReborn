// 메인 메뉴 State 3 native-main-menu-state.js(installNativeMainMenuState·readNativeFortEntrySource·notifyNativeFortEntryLeave·prepareNativeEditorEntry)를 원본 10.78 관측 82행과 cpppj `NativeMainMenuStateNativeTests` 계약으로 대조한다(WB04-E534).
//  - 원본 관측 native-main-menu-state-x86.tsv 82관측(41 입력 × 두 제어 워드 639/895): 입력 11 DWORD(intro·Welcome·startup·first·updated·tips·이전 phase·출처·결과·battle·출처 배경 존재)와 외부 변경/중단 이름에서
//    종료 종류·다섯 144BYTE 블록 전체·현재 DWORD 여섯·현재/pending/배경 이름 전체·활동 기한 double·외부 호출 순서를 실제 `RawLocalMissionState` 분배기와 공통 tail(기존 Editor checkpoint 뒤의 HUD·주변음 기한·음악·팔레트·next 4)로 대조한다.
//    Edit 기능은 웹 제외라 그 tail 은 이 시험의 장치로만 재현한다(같은 28줄 순서).
//  - 계약: 공통 앞단의 signed phase 판정, 이전 Battle14 통지의 호출 순간 재조회, Editor 진입 앞단의 current 1/2 계약.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E534)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { resetAmbientSoundDeadline } from '../www/core/raw-ambient-sound.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { RawLocalMissionState } from '../www/core/raw-local-mission-state.js';
import { installNativeMainMenuState, notifyNativeFortEntryLeave, prepareNativeEditorEntry, readNativeFortEntrySource } from '../www/core/native-main-menu-state.js';

// 원본 관측 82행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E534)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'native-main-menu-state-x86.tsv'));

// 중단(선택 도달점/중간 실패)과 정상 ret 를 구별하는 검사 전용 신호이다. (2026-11-15 WB04-E534)
class Stop extends Error {
    constructor(cause) { super(cause); this.cause = cause; }
}
const bytesHex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');

// 큰 외부 장치의 명시 입력만 제공하고 State 3 분기/직접 쓰기/공통 ret 는 실제 부모에 맡긴다(cpppj Native3FixtureScene). (2026-11-15 WB04-E534)
class Scene {
    constructor(description) {
        const fields = description.split(',');
        assert.equal(fields.length, 12);
        this.values = fields.slice(0, 11).map((text) => Number(text) >>> 0);
        this.mutation = fields[11];
        this.local = new ClientLocalStateGlobals();
        this.player = new PlayerWorldState();
        this.source = { value: this.values[7] };
        this.outcome = { value: this.values[8] };
        this.first = { value: this.values[3] };
        this.updated = this.values[4];
        this.tips = this.values[5];
        this.editor = { value: 1 };
        this.battle = this.values[9] !== 0;
        this.modes = { server: false, boss: false, post: false, saved: false, mission: false, simul: false, meta: false };
        this.misc = { zone: 0, ticks: 0, seed: 0, refresh: 0, rebuild: 0, battleByte: 0, end: 0, seconds: 0, wall: 0 };
        this.background = new Uint8Array(80).fill(0x3f);
        this.current = new Uint8Array(256).fill(0xa5);
        this.pending = new Uint8Array(256).fill(0xa5);
        this.events = [];
        const blocks = this.local.blocks();
        for (let block = 0; block < blocks.length; block += 1) for (let word = 0; word < blocks[block].length; word += 1) blocks[block][word] = block * 1000 + word;
        blocks[1][0] = this.values[6];
        blocks[3][0] = 3;
        const put = (target, text) => { for (let index = 0; index < text.length; index += 1) target[index] = text.charCodeAt(index); target[text.length] = 0; };
        put(this.current, 'Old');
        put(this.pending, 'Next');
        put(this.background, 'Previous');
    }

    // 도달한 외부 효과만 기록한 뒤 그 위치의 명시 중단을 발생시킨다.
    event(value, boundary = '') {
        const selected = boundary === '' ? value : boundary;
        this.events.push(value);
        if (this.mutation === `stop:${selected}`) throw new Stop(`stop:${selected}`);
    }

    globals() {
        const scene = this;
        return {
            state: this.local.state(),
            sourceType: this.source,
            outcome: this.outcome,
            firstMenu: this.first,
            battle: () => scene.battle,
            editor: this.editor,
            background: this.background,
            currentName: this.current,
            pendingName: this.mutation === 'pending-null' ? new Uint8Array(0) : this.pending,
            userUpdated: () => { scene.events.push('updated'); return scene.updated; },
            tellTips: () => { scene.events.push('tips'); return scene.tips; },
            // 실제 PE 82관측은 구 검증 안내를 포함하는 참조 모드이며 제품 정책과 구별한다.
            referenceValidationNotice: true,
        };
    }

    // 같은 실제 tail 이 HUD/벽시각/음악/팔레트/next 4 를 실행한다(기존 Editor checkpoint 뒤 28줄 순서의 시험용 재현).
    tail() {
        const state = this.local.state();
        const command = this.local.hudModeCommand().value;
        assert.equal(command, 0x0200003d);
        this.event('hud2');
        resetAmbientSoundDeadline(this.local.ambientSound(), () => { this.event('wall'); return 100.5; });
        this.event('music:ser22.mus');
        assert.notEqual(state.current[0], 6, 'State 3 의 tail 에서 battleOver 는 선택되면 안 된다');
        this.event('palette');
        const current = state.current[0];
        state.next[0] = 4;
        if (current === 3) return;
        throw new Error('State3에서 선택되면 안 되는 섬 조회');
    }

    hooks() {
        const scene = this;
        const values = this.values;
        return {
            configInt: (key) => { scene.event(key, key === 'intro' ? 'intro' : 'welcome'); assert.ok(key === 'intro' || key === 'WelcomeGiven'); return values[key === 'intro' ? 0 : 1] | 0; },
            request: (state) => { if (!scene.running) throw new Error('State3 실제 분배기 수명 없음'); scene.running.request(state); },
            deleteSavedState: () => scene.event('delete'),
            requestMusic: (name) => {
                scene.event(`music:${name}`);
                if (name === 'fanfare.mus' && scene.mutation === 'source2') scene.source.value = 2;
                if (name === 'fanfare.mus' && scene.mutation === 'outcome2') scene.outcome.value = 2;
            },
            sourceBackground: (type) => {
                scene.event(`source:${type}`);
                assert.ok(type === 1 || type === 2);
                if (!values[10]) return null;
                return type === 1 ? 'SourceOne' : 'SourceTwo';
            },
            leaveBattle: () => scene.event('battleLeave'),
            // PE 에서 같은 명시 cleanup child 가 제공한 모드 효과이다. 자식 알고리즘을 재구현하지 않는다.
            prefix: () => { scene.event('prefix'); scene.editor.value = 0; scene.battle = false; if (scene.mutation === 'prefix-outcome4') scene.outcome.value = 4; },
            hudPrefix: () => scene.event('hud1'),
            title: () => { scene.event('title'); if (scene.mutation === 'state2') scene.local.state().current[0] = 2; },
            closeGuide: () => scene.event('close'),
            tell: (name) => {
                scene.event(`tell:${name}`);
                if (name === 'UserNotUpdated' && scene.mutation === 'tips0') scene.tips = 0;
                if (name === 'Blank' && scene.mutation === 'updated1') scene.updated = 1;
            },
            startup: () => {
                scene.event('startup');
                if (scene.mutation === 'first0') scene.first.value = 0;
                if (scene.mutation === 'tips1') scene.tips = 1;
                if (scene.mutation === 'editor1' || scene.mutation === 'editor1-stop') scene.editor.value = 1;
                return values[2];
            },
            editorPlayers: () => { scene.event('editorPlayers'); if (scene.mutation === 'editor1-stop') throw new Stop('stop:editorPlayers'); },
            changedState: () => { scene.event('changedState'); throw new Stop('changedState'); },
            tail: () => scene.tail(),
        };
    }

    // 기존 실제 공통 Run 으로 intro 39/정상 ret/외부 예외의 다섯 블록을 관측한다.
    run() {
        const scene = this;
        const hooks = {
            logState: (state) => { scene.events.push(`state:${state}`); if (state === 39) throw new Stop('intro39'); },
            parentState: (selected) => { assert.equal(selected, 3); return installNativeMainMenuState(scene.globals(), scene.hooks()); },
        };
        const m = this.modes;
        const x = this.misc;
        const globals = {
            get server() { return m.server; }, set server(value) { m.server = value; },
            get boss() { return m.boss; }, set boss(value) { m.boss = value; },
            get inBattle() { return scene.battle; }, set inBattle(value) { scene.battle = value; },
            get missionPost() { return m.post; }, set missionPost(value) { m.post = value; },
            get savedGame() { return m.saved; }, set savedGame(value) { m.saved = value; },
            get missionWorld() { return m.mission; },
            get simulconnect() { return m.simul; },
            get metamap() { return m.meta; },
            player: this.player,
            get zonePending() { return x.zone; }, set zonePending(value) { x.zone = value; },
            get gameTicks() { return x.ticks; }, set gameTicks(value) { x.ticks = value; },
            get sourceType() { return scene.source.value; }, set sourceType(value) { scene.source.value = value >>> 0; },
            get cdPresent() { return x.seed; }, set cdPresent(value) { x.seed = value; },
            get metaRefresh() { return x.refresh; }, set metaRefresh(value) { x.refresh = value; },
            get metaRebuild() { return x.rebuild; }, set metaRebuild(value) { x.rebuild = value; },
            get titleInitial() { return scene.background[0]; }, set titleInitial(value) { scene.background[0] = value & 0xff; },
            get battleByte() { return x.battleByte; },
            get connectEnd() { return x.end; }, set connectEnd(value) { x.end = value; },
            get activityEnd() { return scene.local.activityEnd().value; }, set activityEnd(value) { scene.local.activityEnd().value = value; },
            get gameSeconds() { return x.seconds; },
            get wallCached() { return x.wall; },
        };
        const body = new RawLocalMissionState(this.local.state(), globals, hooks);
        this.running = body;
        try {
            body.run();
            this.running = null;
            return 'normal';
        } catch (stop) {
            this.running = null;
            if (!(stop instanceof Stop)) throw stop;
            return stop.cause;
        }
    }

    blocks() { return this.local.blocks().map((block) => bytesHex(new Uint8Array(block.buffer, block.byteOffset, 144))).join(':'); }

    words() { return [this.source.value, this.outcome.value, this.first.value, this.updated, this.tips, this.editor.value].join(','); }

    names() { return `${bytesHex(this.current)}:${bytesHex(this.pending)}:${bytesHex(this.background)}`; }

    trace() { return this.events.join(';'); }
}

test('메인 메뉴 State 3: 원본 10.78 82관측의 상태·블록·이름·활동 기한·외부 호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const data = rows.filter((row) => row.length === 9);
    assert.equal(data.length, 82);
    const inputs = new Set();
    let observations = 0;
    for (const row of data) {
        assert.ok(row[1] === '639' || row[1] === '895');
        const scene = new Scene(row[2]);
        const status = scene.run();
        const label = `${row[0]}/${row[1]}`;
        assert.equal(status, row[3], `${label} status`);
        assert.equal(scene.blocks(), row[4], `${label} 5x144BYTE`);
        assert.equal(scene.words(), row[5], `${label} current DWORDs`);
        assert.equal(scene.names(), row[6], `${label} names/background`);
        const activity = new Float64Array([scene.local.activityEnd().value]);
        assert.equal(bytesHex(activity), row[7], `${label} activity double`);
        assert.equal(scene.trace(), row[8], `${label} external trace`);
        observations += 1;
        inputs.add(row[0]);
    }
    assert.equal(observations, 82);
    assert.equal(inputs.size, 41);
});

test('메인 메뉴 State 3 계약: 공통 앞단은 signed phase 로 출처를 정규화하고 이전 Battle14 통지는 호출 순간의 현재 값을 읽는다', () => {
    const source = { value: 7 };
    // signed 이전 phase <= 6 또는 11 이면 출처 0, 그 밖은 보존.
    assert.equal(readNativeFortEntrySource(6, source), 0);
    source.value = 7;
    assert.equal(readNativeFortEntrySource(0xffffffff, source), 0);
    source.value = 7;
    assert.equal(readNativeFortEntrySource(11, source), 0);
    source.value = 7;
    assert.equal(readNativeFortEntrySource(7, source), 7);
    assert.equal(readNativeFortEntrySource(12, source), 7);
    // 전체 32비트 출처를 보존한다.
    source.value = 0x80000001;
    assert.equal(readNativeFortEntrySource(8, source), 0x80000001);
    // 통지는 현재 battle 과 이전 phase 14 일 때만, 장치가 없으면 도달 시 거부한다.
    const state = { phase: new Uint32Array(36) };
    let battle = true;
    let leaves = 0;
    state.phase[0] = 13;
    notifyNativeFortEntryLeave(state, () => battle, () => { leaves += 1; });
    assert.equal(leaves, 0);
    state.phase[0] = 14;
    notifyNativeFortEntryLeave(state, () => battle, () => { leaves += 1; });
    assert.equal(leaves, 1);
    battle = false;
    notifyNativeFortEntryLeave(state, () => battle, () => { leaves += 1; });
    assert.equal(leaves, 1);
    battle = true;
    assert.throws(() => notifyNativeFortEntryLeave(state, () => battle, undefined), /선택 장치 미연결/);
});

test('메인 메뉴 State 3 계약: Editor 진입 앞단은 current 1/2 에서만 출처·첫 BYTE·통지를 실행하고 current 가 3 이 아니면 State 3 부모를 거부한다', () => {
    const block = () => new Uint32Array(36);
    const state = { old: block(), phase: block(), immediate: block(), current: block(), next: block() };
    const source = { value: 9 };
    const background = Uint8Array.of(0x41, 0x42);
    state.current[0] = 3;
    assert.throws(() => prepareNativeEditorEntry(state, source, background, () => false, () => {}), /current1\/2 계약/);
    state.current[0] = 2;
    state.phase[0] = 14;
    let left = 0;
    prepareNativeEditorEntry(state, source, background, () => true, () => { left += 1; });
    assert.deepEqual([source.value, background[0], background[1], left], [9, 0, 0x42, 1]);
    assert.throws(() => prepareNativeEditorEntry(state, source, new Uint8Array(0), () => false, () => {}), RangeError);
    state.current[0] = 4;
    assert.throws(() => installNativeMainMenuState({ state, sourceType: source }, {}), /current 계약/);
});
