// 단독 미션(캠페인·데모·스커미시)에서 승리/패배 곡(fanfare.mus·defeat.mus)이 요청되지 않는 원본 흐름을 웹 모듈 묶음으로 고정한다. (2026-11-23 WB04-E1238)
// 원본은 미션을 떠날 때 공통 중단(004b6350, `prepareBattleAbortState`)이 미션 후(missionPost) 경로의 결과 DWORD(005c85ac)를 0 으로 쓰고 다음 상태를 3(메인 메뉴)으로 예약한다.
// State 3(`installNativeMainMenuState`)은 결과가 1·2 이거나 출처(0055a420)가 있을 때만 결과 곡을 요청하므로 단독 미션의 복귀에서는 요청이 한 번도 없다.
// 결과 곡이 요청되는 경로는 미션 후가 아닌 전투(멀티플레이) 종료 프레임(`RawBattleCompletion.frame`, 관문 `!missionPost`)과 Editor 시험 전투 출처뿐이며 둘 다 웹 범위 밖이다.
// 이 시험은 실제 게임·브라우저를 실행하지 않고 웹 모듈만 사용한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { prepareBattleAbortState } from '../www/core/raw-battle-leave.js';
import { installNativeMainMenuState } from '../www/core/native-main-menu-state.js';
import { LocalMissionParentResult } from '../www/core/raw-local-mission-state.js';
import { SceneMusic, SceneMusicState } from '../www/audio/scene-music.js';

// 복귀 직전 상태 블록·결과 상자·출처 상자와 State 3 장치 기록을 한 묶음으로 만든다. (2026-11-23 WB04-E1238)
// 인자: missionPost(미션 후 여부), battle(전투 여부), outcome(복귀 직전 결과 DWORD), source(출처 DWORD).
function leaveScene({ missionPost, battle, outcome, source = 0 }) {
    const local = new ClientLocalStateGlobals(), state = local.state();
    state.phase[0] = 14; state.current[0] = 14;
    const box = { outcome: { value: outcome }, source: { value: source } }, music = [], deleted = [];
    const abort = { state, abortPending: { value: 0 }, outcome: box.outcome, missionPost, editor: 0, metamap: 0, battle };
    prepareBattleAbortState(abort, () => deleted.push('abort'));
    return { local, state, box, music, deleted, abort };
}

// 예약된 다음 상태를 현재로 옮겨 State 3 부모를 실행하고 요청된 곡과 부모 결과를 돌려준다. (2026-11-23 WB04-E1238)
function runMainMenu(scene) {
    const { state, box, music, deleted } = scene;
    assert.equal(state.next[0], 3);
    state.current[0] = 3;
    const globals = {
        state, sourceType: box.source, outcome: box.outcome, firstMenu: { value: 0 }, battle: () => false, editor: { value: 0 },
        background: new Uint8Array(80), currentName: new Uint8Array(256), pendingName: new Uint8Array(256), userUpdated: () => 1, tellTips: () => 0,
    };
    const hooks = {
        configInt: () => 1, request: () => assert.fail('인트로 예약 없음'), deleteSavedState: () => deleted.push('menu'), requestMusic: (name) => music.push(name),
        sourceBackground: () => null, leaveBattle: () => {}, prefix: () => {}, hudPrefix: () => {}, title: () => {}, closeGuide: () => {}, tell: () => {},
        startup: () => 0, editorPlayers: () => assert.fail('편집 아님'), tail: () => {}, changedState: () => assert.fail('상태 변경 없음'),
    };
    const result = installNativeMainMenuState(globals, hooks);
    return { result, background: Buffer.from(globals.background).toString('latin1').split('\0')[0] };
}

test('단독 미션(missionPost) 복귀: 결과가 1·2 였어도 0 으로 지워지고 State 3 은 결과 곡을 요청하지 않는다', () => {
    for (const outcome of [0, 1, 2, 3]) {
        const scene = leaveScene({ missionPost: true, battle: true, outcome });
        // 공통 중단이 결과를 0, 다음 상태를 3 으로 쓰고 중단 표시를 켠다.
        assert.equal(scene.box.outcome.value, 0, `결과 ${outcome}`);
        assert.equal(scene.abort.abortPending.value, 1);
        const { result, background } = runMainMenu(scene);
        assert.equal(result, LocalMissionParentResult.Commit);
        assert.deepEqual(scene.music, [], `결과 ${outcome} 곡 요청`);
        // 승패 배경(victory·defeat)도 쓰지 않고 기본 titleMenu 배경으로 돌아간다.
        assert.equal(background, 'titleMenu');
        // 저장 삭제는 공통 중단의 한 번뿐이다(State 3 의 승패 삭제는 없다).
        assert.deepEqual(scene.deleted, ['abort']);
    }
});

test('단독 미션 데모·스커미시도 같은 미션 후 경로라 전투가 아니어도 결과 0·State 3 이다', () => {
    const scene = leaveScene({ missionPost: true, battle: false, outcome: 0 });
    assert.equal(scene.box.outcome.value, 0);
    runMainMenu(scene);
    assert.deepEqual(scene.music, []);
});

test('대조: 미션 후가 아닌 전투 종료만 State 6 으로 가고 결과가 State 3 의 곡 요청을 만든다(웹 범위 밖 경로)', () => {
    // 미션 후가 아닌 전투의 중단은 결과가 0 이면 3 을 쓰고 다음 상태를 6(전투 복귀)으로 예약한다. 결과 1·2 는 그대로 보존된다.
    const lost = leaveScene({ missionPost: false, battle: true, outcome: 0 });
    assert.equal(lost.box.outcome.value, 3); assert.equal(lost.state.next[0], 6);
    for (const [outcome, name, background] of [[1, 'fanfare.mus', 'victory'], [2, 'defeat.mus', 'defeat']]) {
        const scene = leaveScene({ missionPost: false, battle: true, outcome });
        assert.equal(scene.box.outcome.value, outcome); assert.equal(scene.state.next[0], 6);
        // State 3 부모만 따로 실행해 결과별 곡·배경을 확인한다(State 6 은 같은 요청을 한다).
        scene.state.next[0] = 3;
        const { background: actual } = runMainMenu(scene);
        assert.deepEqual(scene.music, [name]); assert.equal(actual, background); assert.equal(scene.box.outcome.value, 0);
    }
});

test('SceneMusic: 승리 곡은 결과 상태 1 에서만 선택되고 패배 곡은 결과 상태와 상관없이 선택된다', () => {
    // 대역 장치: 현재 곡 이름·선택 기록·곡 길이·시계만 쓰며 나머지는 비어 있다.
    const make = (resultState) => {
        const state = new SceneMusicState(); state.resultState = resultState;
        const picked = []; let current = 'ser22.mus';
        const scene = new SceneMusic(state, { next: () => 0 }, {
            wallSeconds: () => 100, currentName: () => current, select: (name) => { picked.push(name); current = name; }, duration: () => 20,
            sacrificing: () => false, waitingRoom: () => false, refresh: () => {}, ascendancyPalette: () => false, loadPalette: () => {}, thunderFlash: () => {}, weatherSound: () => {},
        });
        return { scene, picked };
    };
    for (const result of [0, 2, 3, 4]) { const f = make(result); f.scene.request('fanfare.mus'); assert.deepEqual(f.picked, [], `결과 ${result} 승리 곡`); }
    const win = make(1); win.scene.request('fanfare.mus'); assert.deepEqual(win.picked, ['fanfare.mus']);
    const lose = make(0); lose.scene.request('defeat.mus'); assert.deepEqual(lose.picked, ['defeat.mus']);
});
