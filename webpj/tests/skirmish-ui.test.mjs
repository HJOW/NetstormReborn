// 12차 스커미시의 앱 연결을 실제 메뉴 자료로 검사한다: Campaign 목록의 Skirmish 항목, 설정 화면 Tell 본문, 설정 변경·저장, 시작 호출. 게임 월드는 만들지 않는다. (2026-11-20 WB04-E1102)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { Netstorm } from '../www/netstorm.js';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { productCreditsText, productTellBody, PRODUCT_SKIRMISH_TARGET } from '../www/assets/product.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { Settings, SETTINGS_STORAGE_KEY } from '../www/platform/settings.js';
import { DATA_ROOT, readDataMap } from './helpers.mjs';

// 메뉴 묶음 파일(한국어 묶음 포함 가능)과 GameData 를 만든다. (2026-11-20 WB04-E1102)
function menuFiles() {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    return readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu' || entry.bundle === 'korean').map((entry) => entry.path));
}
// 메인 메뉴 위에 Tell 을 열 수 있는 최소 앱을 만든다. storage 는 설정 저장소 흉내이다. (2026-11-20 WB04-E1102)
function makeApp({ language = 'english', skirmish = true, startSkirmish = null, storage = null } = {}) {
    const files = menuFiles();
    const app = new Netstorm({ language, skirmish, startSkirmish });
    app._gameData = GameData.build({ files, language, skirmish });
    app._resources = buildMenuResources(files);
    app._settings = new Settings({ storage });
    app._mainMenu = new MainMenuScreen({ resources: app._resources }); app._mainMenu.resize(1024, 768);
    return app;
}
// 메뉴 한 줄(라벨)을 마우스 좌표로 눌러 대화상자의 명령을 실행한다. (2026-11-20 WB04-E1102)
function clickLabel(app, label) {
    const point = app._mainMenu.controlPoint(label); assert.ok(point, `${label} 컨트롤 없음`);
    app._mainMenu.pointerDown(point.x, point.y); app._mainMenu.pointerUp(point.x, point.y); // 목록 항목은 누를 때, 버튼은 뗄 때 실행한다.
}
// 저장소 흉내이다. (2026-11-20 WB04-E1102)
function fakeStorage() { const map = new Map(); return { map, getItem: (key) => map.get(key) ?? null, setItem: (key, value) => { map.set(key, String(value)); } }; }
// 현재 대화상자의 메뉴 항목 라벨 목록이다. (2026-11-20 WB04-E1102)
const menuLabels = (app) => app._mainMenu.dialog.menu.items.map((item) => item.label);
const buttonLabels = (app) => app._mainMenu.dialog.buttons.map((button) => button.label);

test('제품 본문: 스커미시를 켠 판본만 Campaign 목록에 Skirmish 항목을 첫 버튼 앞에 한 번 넣는다', () => {
    const original = '<h2>Campaign</h2>\n$Menu=Early Missions,Tell,x\n$Button=Back,Tell,Blank\n';
    const english = productTellBody('UCampaign', original, 'english', { skirmish: true });
    assert.ok(english.includes('$Menu=Skirmish,Tell,Patch1101.Skirmish\n$Button=Back')); assert.equal(english.indexOf('$Menu=Skirmish'), english.lastIndexOf('$Menu=Skirmish'));
    assert.equal(productTellBody('ucampaign', english, 'english', { skirmish: true }), english); // 두 번 가공해도 한 번만 들어간다.
    assert.ok(productTellBody('UCampaign', original, 'korean', { skirmish: true }).includes('$Menu=스커미시,Tell,Patch1101.Skirmish\n'));
    assert.equal(productTellBody('UCampaign', original, 'english'), original); assert.equal(productTellBody('UCampaign', original, 'english', { skirmish: false }), original); // 켜지 않으면 원문 그대로이다.
    assert.equal(productTellBody('Blank', original, 'english', { skirmish: true }), original); // 다른 섹션은 건드리지 않는다.
    assert.equal(productTellBody('UCampaign', '<h2>x</h2>\n$Menu=a,Tell,b\n', 'english', { skirmish: true }), '<h2>x</h2>\n$Menu=a,Tell,b\n$Menu=Skirmish,Tell,Patch1101.Skirmish\n'); // 버튼이 없으면 끝에 붙는다.
    assert.equal(PRODUCT_SKIRMISH_TARGET, 'Patch1101.Skirmish');
    // Patch1101 객체에는 네 화면의 자리 틀 섹션이 모든 언어에 있다.
    for (const language of ['english', 'korean']) for (const section of ['[Skirmish]', '[SkirmishCount]', '[SkirmishAi]', '[SkirmishBans]']) assert.ok(productCreditsText(language).includes(`\n${section}\n`), `${language} ${section}`);
});

test('앱 연결: 실제 Campaign 목록에 Skirmish 가 있고 설정 화면이 열리며 선택이 저장된다', () => {
    const storage = fakeStorage(), started = [], app = makeApp({ storage, startSkirmish: (settings) => { started.push(JSON.parse(JSON.stringify(settings))); return true; } });
    assert.equal(app._openTell('UCampaign'), true);
    assert.ok(menuLabels(app).includes('Skirmish')); assert.ok(menuLabels(app).indexOf('Skirmish') < menuLabels(app).length);
    // Skirmish 항목을 누르면 설정 첫 화면이 열린다.
    clickLabel(app, 'Skirmish');
    assert.ok(menuLabels(app).includes('AI players: 1') && menuLabels(app).includes('AI 1: Normal / Enemy') && menuLabels(app).includes('Banned units: 0/5')); assert.deepEqual(buttonLabels(app), ['Start', 'Cancel']);
    // AI 수 화면에서 3 을 고르면 첫 화면으로 돌아오고 AI 줄이 3개가 된다.
    clickLabel(app, 'AI players: 1'); assert.ok(menuLabels(app).includes('* 1')); clickLabel(app, '3');
    assert.ok(menuLabels(app).includes('AI players: 3') && menuLabels(app).includes('AI 3: Normal / Enemy'));
    // AI 2 의 난이도를 어려움으로, 진영을 동맹으로 바꾼다(같은 AI 화면에서 이어서 고른다).
    clickLabel(app, 'AI 2: Normal / Enemy'); clickLabel(app, 'Hard'); assert.ok(menuLabels(app).includes('* Hard')); clickLabel(app, 'Ally'); assert.ok(menuLabels(app).includes('* Ally')); assert.ok(menuLabels(app).includes('* Hard'));
    clickLabel(app, 'Back'); assert.ok(menuLabels(app).includes('AI 2: Hard / Ally'));
    // 금지 유닛 화면에서 두 항목을 고른다.
    clickLabel(app, 'Banned units: 0/5'); assert.ok(menuLabels(app).some((label) => label.startsWith('[ ] Wind Generator - windBattery')));
    clickLabel(app, '[ ] Wind Generator - windBattery'); clickLabel(app, '[ ] Sun Cannon - sunCannon'); assert.ok(menuLabels(app).includes('[X] Wind Generator - windBattery') && menuLabels(app).includes('[X] Sun Cannon - sunCannon'));
    clickLabel(app, 'Back'); assert.ok(menuLabels(app).includes('Banned units: 2/5'));
    // 설정은 저장소에 쓰이고 새 앱이 읽는다.
    const saved = JSON.parse(storage.map.get(SETTINGS_STORAGE_KEY)).values;
    assert.equal(saved.skirmishAiCount, 3); assert.equal(saved.skirmishAi3Diff, 3); assert.equal(saved.skirmishAi3Ally, 1); assert.equal(saved.skirmishAi2Diff, 2); // 화면의 'AI 2' 는 소유자 3 이다. assert.equal(saved.skirmishBans, 'windBattery,sunCannon');
    assert.deepEqual(app._loadSkirmishSettings().bans, ['windBattery', 'sunCannon']);
    const reopened = makeApp({ storage }); assert.equal(reopened._loadSkirmishSettings().aiCount, 3); assert.equal(reopened._loadSkirmishSettings().ai[1].ally, true);
    // 시작 단추는 현재 설정으로 시작 함수를 부른다. 취소는 Campaign 목록으로 돌아간다.
    clickLabel(app, 'Start'); assert.equal(started.length, 1); assert.equal(started[0].aiCount, 3); assert.deepEqual(started[0].bans, ['windBattery', 'sunCannon']); assert.equal(started[0].ai[1].difficulty, 3);
    app._openTell('Patch1101.Skirmish'); clickLabel(app, 'Cancel'); assert.ok(menuLabels(app).includes('Early Missions') && menuLabels(app).includes('Skirmish'));
});

test('앱 연결: 시작 함수가 없으면 시작하지 않고 설정 화면을 그대로 둔다', () => {
    const app = makeApp(), warnings = [], original = console.warn;
    console.warn = (...args) => warnings.push(args.join(' '));
    try {
        assert.equal(app._openTell('Patch1101.Skirmish'), true); app._mainMenu.setDialog(null); // 시작 단추가 창을 닫은 상태를 흉내 낸다.
        assert.equal(app._openTell('Patch1101.SkirmishStart'), false);
    } finally { console.warn = original; }
    assert.ok(menuLabels(app).includes('AI players: 1')); assert.equal(warnings.length, 1); assert.ok(warnings[0].includes('스커미시'));
});

test('앱 옵션: 스커미시는 기본으로 켜지고 원본 비교 모드에서는 꺼진다', () => {
    assert.equal(new Netstorm().options.skirmish, true); assert.equal(new Netstorm({ originalComparison: true }).options.skirmish, false);
    assert.equal(new Netstorm({ skirmish: false }).options.skirmish, false); assert.equal(new Netstorm({ originalComparison: true, skirmish: true }).options.skirmish, true);
});

test('앱 연결: 스커미시를 켜지 않으면 Campaign 목록에 항목이 없고 한국어는 번역된 화면이 열린다', () => {
    const off = makeApp({ skirmish: false }); off._openTell('UCampaign'); assert.equal(menuLabels(off).includes('Skirmish'), false);
    const korean = makeApp({ language: 'korean' }); korean._openTell('UCampaign'); assert.ok(menuLabels(korean).includes('스커미시'));
    // 금지 유닛 이름은 메뉴 번역 함수를 거친다(번역표 대신 표식 함수로 확인한다).
    const original = korean._resources.translate; korean._resources = { ...korean._resources, translate: (text) => (text === 'Wind Generator' ? '윈드 제네레이터' : original(text)) };
    korean._openTell('Patch1101.SkirmishBans(0)'); assert.ok(menuLabels(korean).some((label) => label.includes('윈드 제네레이터') && label.includes('windBattery')), menuLabels(korean).join('|'));
    korean._openTell('Patch1101.Skirmish'); assert.ok(menuLabels(korean).includes('인공지능 수: 1')); assert.deepEqual(buttonLabels(korean), ['시작', '취소']);
});
