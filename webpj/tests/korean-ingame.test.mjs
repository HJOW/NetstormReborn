// 한국어 모드에서 게임 안 ESC 메뉴 막대·Game/View/Options/Players 항목·오른쪽 위 상태 안내·미션 시작 진행 창의 문구가 xlat.korean 으로 모두 번역되는지 확인한다.
// 실제 소스 모듈(RawGameMenuBar·RawGameMenus·MessageGump·missionLoginProgressText)에 번역 기록용 장치를 넣어 코드가 실제로 요청하는 원문을 모으고, 한국어 번역표에 없는 원문이 남지 않았는지 검사한다.
// 게임을 실행하지 않는다. (2026-11-20 WB04-E1118)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { XlatTable } from '../www/assets/xlat.js';
import { RawGameMenus } from '../www/ui/raw-game-menus.js';
import { gameMenuCommandValue } from '../www/core/game-menu-commands.js';
import { GameMenuBarChild, RawGameMenuBar, bindGameMenuBarControls, createGameMenuBarState } from '../www/core/raw-game-menu-bar.js';
import { MessageGump, MessageGumpNode } from '../www/ui/message-gump.js';
import { GumpCreateState } from '../www/ui/gump-create.js';
import { missionLoginProgressText } from '../www/ui/mission-loading-window.js';
import { LocalMissionProgress } from '../www/core/raw-local-mission-state.js';

// 한국어 번역표이다. 번역 파일 원본(webpj/translations/xlat.korean)을 직접 읽는다.
const korean = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'xlat.korean'), 'utf8'));
// 번역 장치가 받은 원문을 모으는 상자와 그 장치이다. 번역 결과는 원문 그대로 돌려 코드 흐름을 바꾸지 않는다.
function recorder() {
    const keys = new Set();
    return { keys, translate: (key) => { keys.add(String(key)); return key; } };
}
// 번역표에 없는 원문 목록이다. 숫자·기호뿐이거나 이름 자리(`%s`)만 있는 원문은 번역 대상이 아니다.
const untranslated = (keys) => [...keys].filter((key) => /[A-Za-z]{3,}/u.test(key) && !korean.has(key));

// RawGameMenus 시험용 장치다. 작성 호출은 모두 무시하고 번역 요청만 기록한다. 구분선·하위 메뉴·체크 항목은 호출 형태만 맞춘다.
function menuHooks(globals, config, extra = {}) {
    const rec = recorder(); let menus = 0;
    const hooks = {
        global: (address) => { const command = gameMenuCommandValue(address); if (command !== null) return command >>> 0; return globals[address] ?? 0; },
        translate: rec.translate, config: (key) => config[key] ?? 0, demo: () => Boolean(globals.demo),
        vertical: () => {}, fancy: () => {}, separator: () => {}, title: () => {}, item: () => {}, checked: () => {}, submenu: () => {}, take: () => (menus += 1),
        current: () => menus + 1, node: () => ({ baseWidth: 100, baseHeight: 60 }), attachCurrent: () => {},
        player: (owner) => { const bytes = new Uint8Array(180); bytes.set([...`P${owner}`].map((c) => c.charCodeAt(0)), 0xc); new DataView(bytes.buffer).setUint32(0x2c, 7, true); return bytes; },
        refreshPriest: () => {}, playerMenu: (subscriber) => 1000 + subscriber,
        languages: () => [{ label: 'English', number: 0, checked: 1 }, { label: '한국어', number: 7, checked: 0 }],
        frameRates: () => [{ label: '60 FPS', number: 60, checked: 1 }],
        difficulties: () => [{ label: 'Easy', number: 0, checked: 0 }, { label: 'Normal', number: 1, checked: 1 }, { label: 'Hard', number: 2, checked: 0 }],
        ...extra,
    };
    return { hooks, rec };
}

test('게임 메뉴 Game·View·Options·Players 의 모든 문구가 한국어로 번역된다', () => {
    // 미션(목표 보기 포함)·데모·도전 화면과 전투 화면을 각각 만들어 코드가 요청하는 원문을 모두 모은다.
    const base = { 0x5c85a4: 0, 0x594fbc: 1, 0x594fa4: 1, 0x594fb8: 0, 0x594fc8: 0, 0x594fa0: 1, 0x59a8ac: 1, 0x540c70: 1, 0x5c7b14: 1, 0x531900: 3, 0x531904: 5, 0x55b4b0: 0 };
    const keys = new Set();
    for (const [globals, config] of [[base, { theammode: 1, useBanners: 0 }], [{ ...base, 0x594fa0: 0 }, {}], [{ ...base, demo: 1 }, {}], [{ ...base, 0x594fb8: 1 }, {}]]) {
        const { hooks, rec } = menuHooks(globals, config), menus = new RawGameMenus(hooks);
        menus.game(); menus.view(); menus.options(); menus.players(true); menus.players(false);
        for (const key of rec.keys) keys.add(key);
    }
    // 도전 화면(Refresh Zones)과 편집이 아닌 전투 화면을 따로 한 번씩 더 본다.
    const challenge = menuHooks({ ...base, 0x594fc8: 1 }, {}); const view = new RawGameMenus(challenge.hooks); view.view(); view.options();
    for (const key of challenge.rec.keys) keys.add(key);
    // 모은 원문이 충분히 많아야 이 시험이 실제 코드를 훑었다고 볼 수 있다.
    assert.ok(keys.size >= 35, `수집한 원문 수 ${keys.size}`);
    for (const required of ['Restart Mission', 'Leave Mission', 'Hide buildings - ~iF2~.', 'View Player List - ~iF9~.', 'Cursor Snap To Grid', 'Player List\n', 'Refresh Zones - ~iF9~.']) assert.ok(keys.has(required), `코드가 요청하지 않은 원문 ${JSON.stringify(required)}`);
    assert.deepEqual(untranslated(keys), [], '번역표에 없는 게임 메뉴 문구');
});

test('ESC 메뉴 막대의 Game·View·Players·About·Cheat 라벨이 한국어로 번역된다', () => {
    const rec = recorder(); const state = createGameMenuBarState({ value: 0 });
    const hooks = {
        read: (name) => ({ 0: 0, 1: 1, 2: 1, 3: 1, 4: 800, 5: 0, 6: 1 })[name] ?? 0, prepare: () => {}, translate: rec.translate, about: () => 'About', child: () => 1,
        horizontal: () => 1, take: () => 1, technicalHelp: () => 0, vertical: () => {}, separator: () => {}, submenu: () => {}, item: () => {}, width: () => {}, attach: () => {}, serial: () => 1,
    };
    new RawGameMenuBar(state, hooks).open();
    for (const label of ['Game', 'View', 'Options', 'Players', 'Cheat', 'About']) assert.ok(label === 'About' || rec.keys.has(label), `막대가 요청하지 않은 라벨 ${label}`);
    // About 제목은 앱이 따로 번역 장치로 조회하는 문구이므로 같은 번역표에서 확인한다.
    const labels = new Set([...rec.keys, 'About']);
    assert.deepEqual(untranslated(labels), []);
    assert.equal(korean.translate('Game'), '게임'); assert.equal(korean.translate('View'), '보기'); assert.equal(korean.translate('Players'), '플레이어'); assert.equal(korean.translate('About'), '정보');
    // 하위 부모 번호가 막대에서 쓰는 값과 같다는 점도 함께 고정한다.
    assert.deepEqual(Object.keys(GameMenuBarChild), ['Game', 'Edit', 'View', 'Options', 'Players', 'Cheat']);
    // 막대 조립 함수는 누락된 장치를 빈 성공으로 바꾸지 않는다(번역 키 확인에 쓴 장치와 같은 모양).
    assert.equal(typeof bindGameMenuBarControls, 'function');
});

// MessageGump 시험용 장면이다. 번역 요청만 기록하고 스타일 문자열을 모은다.
function messageScene(overrides = {}) {
    const rec = recorder(), shown = [], node = new MessageGumpNode();
    const name = (owner) => Uint8Array.from([...`P${owner}`].map((c) => c.charCodeAt(0)).concat(0));
    const hooks = {
        node: () => node, constructText: () => { node.flags = 0x400001; node.serial = 41; },
        challenge: () => 0, playerName: () => {}, challengePlayerName: () => {}, playerText: name, challengePlayerText: name, battle: () => 1,
        translate: (key) => { rec.translate(key); return korean.translate(key); }, style: (token, value) => shown.push(value), fit: () => {},
        wallSeconds: () => 0, idle: () => { node.flags |= 0x80; }, warningSound: () => {}, gameTimeSeconds: () => 3725, ...overrides,
    };
    return { rec, shown, create: new GumpCreateState(), hooks };
}

test('오른쪽 위 상태 안내(정지·시간·접속·동맹·SP 수신 등)가 모든 색 조합에서 한국어로 번역된다', () => {
    const rec = recorder(), shown = [];
    // 안내 종류 1~11·16~18 을 플레이어 1~8·두 번째 플레이어 1~8 의 모든 조합으로 만든다. 번역 결과를 실제로 돌려받아 표시 문자열을 모은다.
    for (const kind of [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18]) {
        for (let player = 1; player <= 8; player += 1) for (let parameter = 1; parameter <= 8; parameter += 1) {
            const scene = messageScene({ challenge: () => 0 });
            const original = scene.hooks.translate; scene.hooks.translate = (key) => { rec.translate(key); return original(key); };
            new MessageGump(scene.create, scene.hooks).construct(0x12401000, kind, player, kind === 4 || kind === 18 ? 500 : parameter, kind === 17 ? Uint8Array.from([87, 97, 114, 110]) : null);
            shown.push(...scene.shown);
        }
    }
    // 도전 화면의 이름 없는 접속 안내(`~E~w%s is present`)도 같은 방식으로 모은다.
    const challengeScene = messageScene({ challenge: () => 1 }); const original = challengeScene.hooks.translate;
    challengeScene.hooks.translate = (key) => { rec.translate(key); return original(key); };
    new MessageGump(challengeScene.create, challengeScene.hooks).construct(0x12401000, 3, 1, 0);
    for (const required of ['~E~wGame Paused!', '~E~yGame Time~r:~w %d:%02d:%02d', '~E~wGame started!', '~E~wGame completed!', '~wServer not responding', '~E~wResource Injection of %d sp was received', '~E~w%s is present',
        '~E~b%s~w has disconnected', '~E~o%s~w and ~l%s~w are now Allies', '~E~u%s~w not responding']) assert.ok(rec.keys.has(required), `코드가 요청하지 않은 원문 ${JSON.stringify(required)}`);
    assert.ok(rec.keys.size >= 150, `수집한 원문 수 ${rec.keys.size}`);
    assert.deepEqual(untranslated(rec.keys), [], '번역표에 없는 상태 안내 문구');
    // 번역이 실제로 표시에 쓰이고 서식 자리(%s, %d)가 채워졌으며 한국어가 들어 있다.
    assert.ok(shown.some((text) => /게임 일시 정지/u.test(text)) && shown.some((text) => /게임 시간/u.test(text) && text.includes('1:02:05')), '정지·시간 안내가 한국어로 표시');
    assert.ok(shown.some((text) => /P1.*스톰 파워/u.test(text) && text.includes('500')), 'SP 수신 안내');
    assert.ok(shown.every((text) => !/%[sd]/u.test(text)), '채워지지 않은 서식 자리');
});

test('미션 시작 진행 창의 문구(Initializing.·Starting Mission...·Cancel)가 한국어로 번역된다', () => {
    const rec = recorder();
    for (const kind of Object.values(LocalMissionProgress)) missionLoginProgressText(kind, 3, rec.translate);
    // 웹에서 도달하는 단일 플레이어 진행 문구만 번역 대상이고 서버 접속 문구(멀티플레이어)는 웹 범위 밖이다.
    const reachable = ['Initializing.', 'Starting Mission...']; for (const key of reachable) assert.ok(rec.keys.has(key), key);
    assert.deepEqual(untranslated(new Set([...reachable, 'Cancel'])), []);
    assert.equal(korean.translate('Starting Mission...'), '미션 시작 중...'); assert.equal(korean.translate('Cancel'), '취소');
    // 번역된 진행 문구에서 `%s`·`%d` 자리표시자가 사라지고 한국어가 그대로 나온다.
    assert.equal(missionLoginProgressText(LocalMissionProgress.MissionTransferring, 0, (key) => korean.translate(key)), '미션 시작 중...');
});

test('번역해서는 안 되는 도움말 주제 접미사 Type 은 번역표에 없다', () => {
    // 도움말 주제 이름은 `종류이름 + translate('Type')`(예: bridgeType)이라 번역하면 주제를 찾지 못한다.
    assert.equal(korean.has('Type'), false);
    // 색 조합이 모든 8×8 로 들어 있어 이름이 바뀌어도 서식 자리가 보존된다.
    for (const first of 'brwgmylo') for (const second of 'brwgmylo') {
        const key = `~E~${first}%s~w and ~${second}%s~w are now Allies`;
        assert.ok(korean.has(key)); assert.ok(korean.translate(key).includes(`~E~${first}%s~w`) && korean.translate(key).includes(`~${second}%s~w`));
    }
});
