// 실제 메뉴 자원으로 로그인 진행 창의 본문·버튼·교체 수명을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { Netstorm } from '../www/netstorm.js';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { LocalMissionProgress } from '../www/core/raw-local-mission-state.js';
import { createMissionLoadingWindow, missionLoginProgressText } from '../www/ui/mission-loading-window.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { DATA_ROOT, readDataMap } from './helpers.mjs';

// 같은 출하 메뉴 자산과 설정을 여러 검사에서 읽기 전용으로 사용한다.
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// 원본 상태별 번역 뒤 첫 주소와 남은 시간만 치환한다.
test('미션 진행 문구: 현재 번역과 원본 서식', () => {
    assert.equal(missionLoginProgressText(LocalMissionProgress.MissionStarting, 9, (text) => text), 'Initializing.');
    assert.equal(missionLoginProgressText(LocalMissionProgress.ConnectingCountdown, 12, (text) => text), 'Connecting to Game Server  - Countdown 12.');
    assert.equal(missionLoginProgressText(LocalMissionProgress.MissionTransferring, 0, () => '미션 시작 중'), '미션 시작 중');
    assert.throws(() => missionLoginProgressText('unknown', 0, (text) => text), /종류 범위/);
});

// 확장 표시 문자도 원본 바이트 한도를 통과해 본문과 Cancel 지시문으로 돌아온다.
test('미션 진행 창: 한국어 본문·Cancel과 원본 바이트 한도', () => {
    const data = GameData.build({ files, language: 'english' });
    const window = createMissionLoadingWindow({ message: '미션 시작 중', cancelText: '취소', resources, gameData: data, screen: { width: 1024, height: 768 } });
    assert.ok(window.content.styleText.includes('미션 시작 중'));
    assert.equal(window.buttons.length, 1); assert.equal(window.buttons[0].label, '취소');
    assert.deepEqual(window.buttons[0].command, { name: 'LeaveBattle', argument: '3\n', instant: true });
    assert.throws(() => createMissionLoadingWindow({ message: '한'.repeat(256), cancelText: '취소', resources, gameData: data, screen: { width: 1024, height: 768 } }), /1024BYTE/);
});

// 같은 앱 메뉴에서 시작·진행을 실제 돌 창으로 표시하고 다른 창은 종료하지 않는다.
test('미션 진행 창: 앱 로그인 장치의 실제 메뉴 표시·교체·닫기', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    app._resources = resources; app._gameData = GameData.build({ files, language: 'english' });
    app._mainMenu = new MainMenuScreen({ resources }); app._mainMenu.resize(1024, 768);
    const login = app._campaignLoginServices();
    login.startingProgress(); const first = app._mainMenu.dialog;
    assert.ok(first.content.styleText.includes('Starting Mission...')); assert.equal(first.buttons[0].command.name, 'LeaveBattle');
    // 시작 안내는 원본 State7 처럼 왼쪽 85px 를 뺀 영역 가운데에 놓이고, 화면 크기가 바뀌어도 영역의 왼쪽 위 모서리를 지킨다. (2026-11-21 WB04-E1131)
    assert.deepEqual(first.screen, { left: 85, top: 0, width: 1024 - 85, height: 768 });
    assert.equal(first.x, 85 + ((1024 - 85) - first.baseWidth) * 0.5);
    app._mainMenu.resize(1280, 720);
    assert.deepEqual(first.screen, { left: 85, top: 0, width: 1280 - 85, height: 720 }); assert.equal(first.x, 85 + ((1280 - 85) - first.baseWidth) * 0.5);
    app._mainMenu.resize(1024, 768);
    app._mainMenu.render(new IndexedSurface(1024, 768));
    login.progress(LocalMissionProgress.MissionStarting, 0); const second = app._mainMenu.dialog;
    assert.notEqual(second, first); assert.equal(first.closed, true); assert.ok(second.content.styleText.includes('Initializing.'));
    login.closeProgress(); assert.equal(second.closed, true); assert.equal(app._mainMenu.dialog, null);
    login.startingProgress(); const third = app._mainMenu.dialog;
    const other = { cancel: () => false, fit: () => {}, place: () => {}, screen: {} }; app._mainMenu.setDialog(other);
    login.closeProgress(); assert.equal(third.closed, true); assert.equal(app._mainMenu.dialog, other);
});
