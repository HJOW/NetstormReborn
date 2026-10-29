// 메인 메뉴 펼침 메뉴(Help·Options)와 사용자 설정, 소리 켜기/끄기를 검사한다: 설정 저장소(기본값·검증·저장·손상 복구), 항목 정의(웹 항목/원본 전체 항목),
// 펼침 메뉴 입력 규칙(하위 메뉴·누르는 즉시 실행·메뉴 밖 누름), 원본 스크린샷(mainMenu - Help/Options/Options - Music Volume)과의 크기·글자·표시 대조,
// 메인 메뉴 화면과의 연결(모달 입력·위치 전달·대화상자와의 관계), AudioSystem 의 소리·음악·음량 전환이다.
// 스크린샷은 외부 캡처 도구로 찍어 창 테두리와 질감 노이즈가 있으므로 글자(흰색)·켜짐 표시(파란 점)의 위치 일치를 마스크 교집합/합집합으로 확인하고,
// 클라이언트 원점 탐색에서 최적 원점이 이웃 원점보다 크게 좋은지로 위치의 유일성을 확인한다. 팝업 위치 자체는 원본 규칙을 알 수 없어 웹 적응이므로 스크린샷의 측정 위치에 직접 열어 비교한다.
// (2026-10-29 WB03-F: 펼침 메뉴·설정 복원 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, decodePng, readDataMap } from './helpers.mjs';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { AudioSystem } from '../www/audio/audio-system.js';
import { SETTINGS_STORAGE_KEY, SETTINGS_VERSION, SETTING_DEFINITIONS, Settings, getLocalStorage, normalizeSetting } from '../www/platform/settings.js';
import { buildHelpItems, buildOptionsItems, buildPopupItems } from '../www/ui/option-menus.js';
import { PopupMenu } from '../www/ui/popup-menu.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { MenuList } from '../www/ui/menu-list.js';

// 메뉴 묶음 파일 전부를 읽어 자원을 만든다(검사마다 공유). (2026-10-29 WB03-F)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// 논리 화면 크기이다(4:3). (2026-10-29 WB03-F)
const SCREEN = Object.freeze({ width: 1024, height: 768 });

// 메모리 저장소 흉내이다. failGet/failSet 이 참이면 읽기·쓰기에서 예외를 던진다(차단된 저장소). (2026-10-29 WB03-F)
function fakeStorage(initial = {}) {
    const map = new Map(Object.entries(initial));
    return {
        failGet: false,
        failSet: false,
        getItem(key) {
            if (this.failGet) throw new Error('blocked');
            return map.has(key) ? map.get(key) : null;
        },
        setItem(key, value) {
            if (this.failSet) throw new Error('quota');
            map.set(key, String(value));
        },
        map,
    };
}

// 설정 저장소: 기본값은 원본 옵션 기본값·스크린샷의 기본 체크 상태와 같고, 잘못된 값은 거부된다. (2026-10-29 WB03-F)
test('설정: 기본값·값 검증·토글·통지', () => {
    const changes = [];
    const settings = new Settings({ onChange: (key, value, previous) => changes.push([key, value, previous]) });
    assert.deepEqual(settings.snapshot(), { sound: 1, music: 1, playWind: 1, swapLeftRightSpeakers: 0, soundVolume: 3, musicVolume: 2, edgeScroll: 1, autoDemo: 1, tellTips: 1 });
    // 토글은 0/1 만 받고 참/거짓·'0'/'1' 도 받는다. 그 밖의 값은 거부한다.
    assert.equal(normalizeSetting('sound', true), 1);
    assert.equal(normalizeSetting('sound', '0'), 0);
    assert.equal(normalizeSetting('sound', 2), null);
    assert.equal(normalizeSetting('sound', 'yes'), null);
    // 음량은 1~5 정수(숫자 문자열 포함)만 받는다.
    assert.equal(normalizeSetting('soundVolume', 5), 5);
    assert.equal(normalizeSetting('soundVolume', '4'), 4);
    assert.equal(normalizeSetting('soundVolume', 0), null);
    assert.equal(normalizeSetting('soundVolume', 6), null);
    assert.equal(normalizeSetting('soundVolume', 2.5), null);
    assert.equal(normalizeSetting('nothing', 1), null);
    // 변경은 값이 달라질 때만 통지한다. 잘못된 값·같은 값·모르는 키는 false 이다.
    assert.equal(settings.set('soundVolume', 4), true);
    assert.equal(settings.set('soundVolume', 4), false);
    assert.equal(settings.set('soundVolume', 9), false);
    assert.equal(settings.set('nothing', 1), false);
    assert.equal(settings.toggle('music'), 0);
    assert.equal(settings.toggle('music'), 1);
    assert.equal(settings.toggle('soundVolume'), null);
    assert.deepEqual(changes, [['soundVolume', 4, 3], ['music', 0, 1], ['music', 1, 0]]);
    // 알려진 모든 키가 정의되어 있다(옵션 메뉴가 쓰는 키 9개).
    assert.equal(Object.keys(SETTING_DEFINITIONS).length, 9);
    // 통지 콜백의 예외는 값 변경을 막지 않는다.
    const noisy = new Settings({ onChange: () => { throw new Error('boom'); } });
    const original = console.error;
    console.error = () => {};
    try {
        assert.equal(noisy.set('sound', 0), true);
    } finally {
        console.error = original;
    }
    assert.equal(noisy.get('sound'), 0);
});

// 설정 저장: 변경하면 저장하고 새로 열면 되살아난다. 호스트 임시 값(persist false)은 저장하지 않는다. (2026-10-29 WB03-F)
test('설정: 저장·복원과 임시 값', () => {
    const storage = fakeStorage();
    const first = new Settings({ storage });
    assert.equal(first.persisted, true);
    first.set('sound', 0);
    first.set('musicVolume', 5);
    first.set('soundVolume', 1, { persist: false });
    assert.deepEqual(JSON.parse(storage.map.get(SETTINGS_STORAGE_KEY)), { version: SETTINGS_VERSION, values: { ...first.snapshot(), soundVolume: 3 } });
    // 임시로 바꾼 음량은 저장되지 않았고, 다시 열면 저장된 값이 나온다.
    const second = new Settings({ storage });
    assert.equal(second.get('sound'), 0);
    assert.equal(second.get('musicVolume'), 5);
    assert.equal(second.get('soundVolume'), 3);
    assert.equal(second.loadProblem, null);
});

// 설정 복구: 손상·다른 버전·잘못된 값·차단된 저장소에서도 기본값으로 시작하고 값 변경은 메모리에서 계속 동작한다. (2026-10-29 WB03-F)
test('설정: 손상·버전·차단된 저장소에서 기본값으로 계속', () => {
    // 손상된 JSON.
    const broken = new Settings({ storage: fakeStorage({ [SETTINGS_STORAGE_KEY]: '{not json' }) });
    assert.equal(broken.get('sound'), 1);
    assert.match(broken.loadProblem, /손상/);
    // 다른 버전과 모양이 다른 값.
    assert.match(new Settings({ storage: fakeStorage({ [SETTINGS_STORAGE_KEY]: JSON.stringify({ version: 99, values: {} }) }) }).loadProblem, /버전/);
    assert.match(new Settings({ storage: fakeStorage({ [SETTINGS_STORAGE_KEY]: '[]' }) }).loadProblem, /버전/);
    // 일부 값만 잘못되면 그 항목만 기본값이고 나머지는 받아들인다.
    const partial = new Settings({ storage: fakeStorage({ [SETTINGS_STORAGE_KEY]: JSON.stringify({ version: SETTINGS_VERSION, values: { sound: 0, soundVolume: 99, bogus: 1 } }) }) });
    assert.equal(partial.get('sound'), 0);
    assert.equal(partial.get('soundVolume'), 3);
    assert.match(partial.loadProblem, /soundVolume/);
    assert.equal(partial.get('bogus'), undefined);
    // 읽기가 막힌 저장소: 기본값으로 시작하고 저장은 시도하지 않아도 값 변경은 된다.
    const blockedRead = fakeStorage();
    blockedRead.failGet = true;
    const readSettings = new Settings({ storage: blockedRead });
    assert.equal(readSettings.persisted, false);
    assert.equal(readSettings.set('sound', 0), true);
    assert.equal(readSettings.get('sound'), 0);
    // 쓰기가 막힌 저장소: 값은 바뀌고 persisted 가 false 가 된다.
    const blockedWrite = fakeStorage();
    blockedWrite.failSet = true;
    const writeSettings = new Settings({ storage: blockedWrite });
    assert.equal(writeSettings.set('music', 0), true);
    assert.equal(writeSettings.persisted, false);
    assert.equal(writeSettings.get('music'), 0);
    // 저장소가 아예 없으면(null) 저장하지 않는다. localStorage 접근 자체가 예외인 문서도 null 이다.
    assert.equal(new Settings().save(), false);
    assert.equal(getLocalStorage({ get localStorage() { throw new Error('denied'); } }), null);
    assert.equal(getLocalStorage(undefined), null);
});

// 항목 정의: 웹 판본은 Direct Draw/Resolution/Pause/Pass Server Diagnostic/Technical Help 를 뺀다. 켜짐 표시는 설정 값을 따른다. (2026-10-29 WB03-F)
test('펼침 메뉴 항목: 웹 항목과 원본 전체 항목, 켜짐 표시', () => {
    const settings = new Settings();
    const labels = (items) => items.map((item) => (item.kind === 'separator' ? '---' : item.label));
    assert.deepEqual(labels(buildOptionsItems(settings)), [
        'Sound On', 'Play Music', 'Wind Noise', 'Speaker Swap L/R', 'Sound Effect Volume', 'Music Volume', '---', 'Edge Scroll in Fullscreen', 'Auto-Demo', 'Tell Tips at Startup',
    ]);
    assert.deepEqual(labels(buildOptionsItems(settings, { includeAll: true })), [
        'Direct Draw / Full Screen', 'Resolution', '---', 'Sound On', 'Play Music', 'Wind Noise', 'Speaker Swap L/R', 'Sound Effect Volume', 'Music Volume', '---',
        'Edge Scroll in Fullscreen', 'Auto-Demo', 'Tell Tips at Startup', 'Pause - ~iShift-F9~.', '---', 'Pass Server Diagnostic ',
    ]);
    assert.deepEqual(labels(buildHelpItems()), ['General Help - ~iF1~.', 'Version']);
    assert.deepEqual(labels(buildHelpItems({ includeAll: true })), ['General Help - ~iF1~.', 'Technical Help', 'Version']);
    assert.equal(buildPopupItems('nothing', settings), null);
    // 켜짐 표시: 기본 설정에서 Speaker Swap 만 꺼져 있다.
    const checks = (items) => items.filter((item) => item.kind === 'check').map((item) => [item.label, item.checked]);
    assert.deepEqual(checks(buildOptionsItems(settings)), [['Sound On', true], ['Play Music', true], ['Wind Noise', true], ['Speaker Swap L/R', false], ['Edge Scroll in Fullscreen', true], ['Auto-Demo', true], ['Tell Tips at Startup', true]]);
    // 설정을 바꾸면 표시와 음량 하위 목록의 현재 단계가 따라간다.
    settings.set('sound', 0);
    settings.set('musicVolume', 4);
    const options = buildOptionsItems(settings);
    assert.equal(options[0].checked, false);
    const musicVolume = options.find((item) => item.label === 'Music Volume');
    assert.deepEqual(musicVolume.items.map((item) => [item.label, item.checked]), [['Volume 1', false], ['Volume 2', false], ['Volume 3', false], ['Volume 4', true], ['Volume 5', false]]);
    assert.deepEqual(musicVolume.items[3].command, { name: 'Volume', argument: 'musicVolume=4' });
    assert.deepEqual(options[0].command, { name: 'Toggle', argument: 'sound' });
    assert.deepEqual(buildHelpItems()[1].command, { name: 'Tell', argument: 'About' });
});

// 펼침 메뉴를 열어 돌려준다(명령·효과음 기록 포함). (2026-10-29 WB03-F)
function openOptions(point = { x: 551, y: 294 }, screen = SCREEN) {
    const commands = [];
    const sounds = [];
    const settings = new Settings();
    const popup = new PopupMenu({ resources, screen, onCommand: (command) => commands.push(command), onSound: (kind) => sounds.push(kind) });
    popup.open(buildOptionsItems(settings, { includeAll: true }), point);
    return { popup, commands, sounds, settings };
}

// 목록의 라벨 행 위쪽 y 와 안쪽 가운데 좌표를 구한다. (2026-10-29 WB03-F)
function rowPoint(list, label) {
    const row = list.items.findIndex((item) => item.label === label);
    assert.ok(row >= 0, `${label} 행이 없습니다`);
    const bounds = list.bounds();
    return { x: bounds.left + 30, y: bounds.top + list.offsets[row] + 4, row };
}

// 위치: 누른 위치에 왼쪽 위를 두고, 화면 오른쪽·아래로 나가면 안쪽으로 당기며, 자식은 부모 오른쪽 가장자리에 부모 행의 위쪽을 맞춘다. (2026-10-29 WB03-F)
test('펼침 메뉴 위치: 누른 위치 · 화면 안 보정 · 자식은 부모 오른쪽', () => {
    const { popup } = openOptions({ x: 551, y: 294 });
    assert.deepEqual(popup.levels[0].list.bounds(), { left: 551, top: 294, right: 717, bottom: 563 });
    // 화면 오른쪽 아래 끝에서 열면 목록이 화면 안에 들어오게 당긴다.
    const corner = openOptions({ x: 1020, y: 760 }).popup.levels[0].list.bounds();
    assert.deepEqual([corner.right, corner.bottom], [1024, 768]);
    // 왼쪽·위쪽 밖의 값은 0 으로 당긴다.
    const origin = openOptions({ x: -30, y: -5 }).popup.levels[0].list.bounds();
    assert.deepEqual([origin.left, origin.top], [0, 0]);
    // 16:9 화면(폭 1366)에서도 그 화면 안에 놓인다.
    const wide = openOptions({ x: 1350, y: 100 }, { width: 1366, height: 768 }).popup.levels[0].list.bounds();
    assert.equal(wide.right, 1366);
    // 자식 목록: Music Volume 행의 위쪽(부모 위 + 오프셋)과 부모 오른쪽 가장자리에 붙는다.
    const { popup: parent } = openOptions();
    const list = parent.levels[0].list;
    const point = rowPoint(list, 'Music Volume');
    parent.pointerDown(point.x, point.y);
    assert.equal(parent.levels.length, 2);
    const child = parent.levels[1].list.bounds();
    assert.deepEqual([child.left, child.top], [list.bounds().right, list.bounds().top + list.offsets[point.row]]);
    assert.deepEqual([child.right - child.left, child.bottom - child.top], [83, 85]);
});

// 입력 규칙: 올리는 것만으로 열리지 않고, 하위 메뉴 행은 눌러야 열리며 다른 하위 메뉴로 교체되고, 일반 행은 누르는 즉시 실행되어 모두 닫는다. (2026-10-29 WB03-F)
test('펼침 메뉴 입력: 하위 메뉴 열기·교체 · 즉시 실행 · 메뉴 밖 누름', () => {
    const { popup, commands, sounds } = openOptions();
    const list = popup.levels[0].list;
    const music = rowPoint(list, 'Music Volume');
    const sound = rowPoint(list, 'Sound Effect Volume');
    // 올리기만 해서는 자식이 열리지 않고 그 행이 강조된다.
    assert.equal(popup.pointerMove(music.x, music.y), true);
    assert.equal(popup.levels.length, 1);
    assert.equal(list.selection, music.row);
    assert.equal(popup.pointerMove(music.x, music.y), false);
    // 하위 메뉴 행을 누르면 효과음과 함께 열리고, 다른 하위 메뉴 행을 누르면 자식이 바뀌며 부모는 유지된다.
    popup.pointerDown(music.x, music.y);
    assert.equal(popup.levels.length, 2);
    assert.equal(popup.levels[1].parentRow, music.row);
    const firstChildTop = popup.levels[1].list.bounds().top;
    popup.pointerDown(sound.x, sound.y);
    assert.equal(popup.levels.length, 2);
    assert.equal(popup.levels[1].parentRow, sound.row);
    assert.notEqual(popup.levels[1].list.bounds().top, firstChildTop);
    assert.deepEqual(sounds, ['menu', 'menu']);
    assert.equal(commands.length, 0);
    // 자식 위에 있는 포인터는 자식 행만 강조하고 부모 행의 강조는 지운다.
    const child = popup.levels[1].list;
    const childBounds = child.bounds();
    popup.pointerMove(childBounds.left + 10, childBounds.top + 3);
    assert.equal(child.selection, 0);
    assert.equal(list.selection, -1);
    // 자식 행(Volume 4)을 누르면 즉시 명령이 실행되고 모든 단계가 닫힌다.
    popup.pointerDown(childBounds.left + 10, childBounds.top + child.offsets[3] + 4);
    assert.deepEqual(commands, [{ name: 'Volume', argument: 'soundVolume=4' }]);
    assert.equal(popup.isOpen, false);
    assert.deepEqual(sounds, ['menu', 'menu', 'menu']);
});

// 일반 행·구분선·메뉴 밖. (2026-10-29 WB03-F)
test('펼침 메뉴 입력: 토글 행 실행 · 구분선 무시 · 메뉴 밖은 닫기만', () => {
    const { popup, commands, sounds } = openOptions();
    const list = popup.levels[0].list;
    // 구분선 행을 누르면 아무 일도 없다(열린 채, 효과음 없음).
    const separatorRow = list.items.findIndex((item) => item.kind === 'separator');
    const bounds = list.bounds();
    assert.equal(popup.pointerDown(bounds.left + 30, bounds.top + list.offsets[separatorRow] + 4), true);
    assert.equal(popup.isOpen, true);
    assert.deepEqual(sounds, []);
    // 토글 행(Sound On)을 누르면 명령이 한 번 실행되고 닫힌다.
    const point = rowPoint(list, 'Sound On');
    popup.pointerDown(point.x, point.y);
    assert.deepEqual(commands, [{ name: 'Toggle', argument: 'sound' }]);
    assert.equal(popup.isOpen, false);
    // 메뉴 밖을 누르면 닫히고 명령은 없다. 입력은 소비된다(true).
    const again = openOptions();
    assert.equal(again.popup.pointerDown(10, 10), true);
    assert.equal(again.popup.isOpen, false);
    assert.equal(again.commands.length, 0);
    // 닫힌 뒤의 이동·누름은 아무 일도 하지 않는다.
    assert.equal(again.popup.pointerMove(600, 400), false);
    // 화면 크기가 바뀌면 닫힌다.
    const resized = openOptions();
    resized.popup.resize({ width: 1366, height: 768 });
    assert.equal(resized.popup.isOpen, false);
});

// 하위 메뉴 표시: 하위 메뉴 행은 ` >` 폭만큼 넓고, 구분선은 맨 앞·연속에서 빠지며, 눌린 모양/튀어나온 모양이 다르다. (2026-10-29 WB03-F)
test('MenuList: 하위 메뉴 행 폭·구분선 생략·튀어나온 모양', () => {
    const list = new MenuList({ fonts: resources.fonts, colors: resources.colors, raised: true });
    list.addSeparator();
    assert.equal(list.items.length, 0);
    list.addText('Volume', null);
    list.addSubmenu('Volume', []);
    assert.equal(list.items[1].kind, 'submenu');
    const plain = list.items[0];
    const widthOf = (item) => item === plain ? list.baseWidth : null;
    assert.ok(list.baseWidth > 0 && widthOf(plain) > 0);
    // 같은 라벨이라도 하위 메뉴 행이 더 넓다: 목록 폭은 가장 넓은 행이므로 하위 메뉴 행의 폭이 표시 글자 폭을 포함한다.
    const without = new MenuList({ fonts: resources.fonts, colors: resources.colors, raised: true });
    without.addText('Volume', null);
    assert.ok(list.baseWidth > without.baseWidth);
    list.addSeparator();
    list.addSeparator();
    assert.equal(list.items.filter((item) => item.kind === 'separator').length, 1);
    // 같은 내용이라도 raised 와 눌린 모양은 다른 픽셀을 만든다.
    const draw = (raised) => {
        const target = new MenuList({ fonts: resources.fonts, colors: resources.colors, raised });
        target.addText('Credits', null);
        target.x = 20;
        target.y = 20;
        const surface = new IndexedSurface(200, 80);
        target.draw(surface, resources);
        return surface.pixels;
    };
    assert.notDeepEqual(draw(true), draw(false));
});

// 팝업을 그려 RGB 로 돌려준다. (2026-10-29 WB03-F)
function renderPopup(popup, width = SCREEN.width, height = SCREEN.height) {
    const surface = new IndexedSurface(width, height);
    surface.tile(resources.clouds);
    popup.draw(surface);
    const rgb = new Uint8Array(width * height * 3);
    for (let index = 0; index < width * height; index += 1) rgb.set(resources.palette.rgb.subarray(surface.pixels[index] * 3, surface.pixels[index] * 3 + 3), index * 3);
    return { rgb, width, height };
}

// 흰 글자·파란 켜짐 표시 판정이다. 흰색은 채널 최솟값 ≥ 215, 켜짐 표시는 파란색이 빨강보다 크게 우세한 픽셀이다. (2026-10-29 WB03-F)
const isWhite = (buffer, offset) => Math.min(buffer[offset], buffer[offset + 1], buffer[offset + 2]) >= 215;
const isBlue = (buffer, offset) => buffer[offset + 2] > buffer[offset] + 35 && buffer[offset + 2] > buffer[offset + 1] + 10;

// 스크린샷과의 대조: 크기·위치(원점 탐색의 유일성)·흰 글자 마스크·켜짐 표시 마스크이다. 팝업 영역만 비교하며 가장 좋은 클라이언트 원점을 찾는다. (2026-10-29 WB03-F)
function compareWithScreenshot(rendered, bounds, file, t) {
    const shotPath = path.join(REPO_ROOT, 'screenShots', file);
    if (!fs.existsSync(shotPath)) {
        t.diagnostic(`${file} 가 없는 환경이라 건너뜀`);
        return null;
    }
    const shot = decodePng(shotPath);
    const diff = (originX, originY) => {
        let sum = 0;
        let count = 0;
        // 팝업 영역의 모든 픽셀을 비교한다.
        for (let y = bounds.top; y < bounds.bottom; y += 1) {
            for (let x = bounds.left; x < bounds.right; x += 1) {
                const s = ((y + originY) * shot.width + x + originX) * 3;
                const d = (y * rendered.width + x) * 3;
                sum += Math.abs(shot.rgb[s] - rendered.rgb[d]) + Math.abs(shot.rgb[s + 1] - rendered.rgb[d + 1]) + Math.abs(shot.rgb[s + 2] - rendered.rgb[d + 2]);
                count += 1;
            }
        }
        return sum / count / 3;
    };
    const scores = [];
    // 클라이언트 원점 후보를 훑는다.
    for (let originY = 28; originY <= 40; originY += 1) for (let originX = 0; originX <= 10; originX += 1) scores.push({ originX, originY, score: diff(originX, originY) });
    scores.sort((a, b) => a.score - b.score);
    const best = scores[0];
    const neighbors = scores.filter((entry) => Math.abs(entry.originX - best.originX) + Math.abs(entry.originY - best.originY) === 1);
    assert.ok(neighbors.every((entry) => entry.score > best.score * 2), `${file}: 정렬이 유일하지 않습니다`);
    // 스크린샷의 클라이언트 원점은 모든 캡처에서 x 0~8, y 30~36 안이다.
    assert.ok(best.originX <= 8 && best.originY >= 30 && best.originY <= 36, `${file}: 원점 (${best.originX}, ${best.originY})`);
    // 팝업 위치가 원본과 같으면 질감 위상까지 맞아 평균 차이가 작다(위치가 1픽셀만 어긋나도 20 이상).
    assert.ok(best.score <= 7, `${file}: 평균 차이 ${best.score.toFixed(2)}`);
    // 지정한 영역(기본: 팝업 전체)의 마스크 일치율이다.
    const ratio = (isMask, area = bounds) => {
        let both = 0;
        let either = 0;
        for (let y = area.top; y < area.bottom; y += 1) {
            for (let x = area.left; x < area.right; x += 1) {
                const inShot = isMask(shot.rgb, ((y + best.originY) * shot.width + x + best.originX) * 3);
                const inRendered = isMask(rendered.rgb, (y * rendered.width + x) * 3);
                if (inShot && inRendered) both += 1;
                if (inShot || inRendered) either += 1;
            }
        }
        return { both, either, value: either ? both / either : 1 };
    };
    const result = { best, white: ratio(isWhite), blue: ratio(isBlue), ratio };
    t.diagnostic(`${file}: 원점 (${best.originX}, ${best.originY}) 평균 차이 ${best.score.toFixed(2)} (다음 후보 ${Math.min(...neighbors.map((entry) => entry.score)).toFixed(2)}), 흰 글자 ${result.white.both}/${result.white.either}=${result.white.value.toFixed(3)}, 켜짐 표시 ${result.blue.both}/${result.blue.either}=${result.blue.value.toFixed(3)}`);
    return result;
}

// 원본 스크린샷 대조(Options): 원본 항목 전체를 원본 팝업의 실제 위치(551,294)에 열어 크기·글자·켜짐 표시·질감 위상을 비교한다.
// 이 위치는 스크린샷의 클라이언트 원점을 타이틀 그림으로 먼저 정한 뒤 팝업 위치를 훑어 구한 값이다(평균 차이 2.5, 이웃 위치는 20 이상). (2026-10-29 WB03-F)
test('원본 스크린샷 대조: Options 펼침 메뉴 크기·글자·켜짐 표시', (t) => {
    const { popup } = openOptions({ x: 551, y: 294 });
    const bounds = popup.levels[0].list.bounds();
    // 원본 팝업의 실제 위치 (551,294) 에서 문서의 대략 범위 165×268 과 ±5 이내인 166×269 이다.
    assert.deepEqual([bounds.right - bounds.left, bounds.bottom - bounds.top], [166, 269]);
    const result = compareWithScreenshot(renderPopup(popup), bounds, 'mainMenu - Options.png', t);
    if (!result) return;
    assert.ok(result.white.value >= 0.95, `Options 흰 글자 마스크 일치율 ${result.white.value.toFixed(3)}`);
    assert.ok(result.blue.value >= 0.9, `Options 켜짐 표시 마스크 일치율 ${result.blue.value.toFixed(3)} (${result.blue.both}/${result.blue.either})`);
    assert.ok(result.blue.either >= 150, '켜짐 표시 6개가 모두 비교에 들어갔는지 확인');
    // 하위 메뉴 행(Sound Effect Volume · Music Volume)만 따로 비교한다: 라벨 뒤 ` >` 표시의 위치·모양이 맞아야 한다(전체 마스크에서는 작은 부분이라 놓치기 쉽다).
    const list = popup.levels[0].list;
    // 하위 메뉴 행마다 그 행 사각형의 흰 글자 마스크를 비교한다.
    for (const label of ['Sound Effect Volume', 'Music Volume']) {
        const row = list.items.findIndex((item) => item.label === label);
        const area = { left: bounds.left, right: bounds.right, top: bounds.top + list.offsets[row], bottom: bounds.top + list.offsets[row] + list.heights[row] };
        const rowResult = result.ratio(isWhite, area);
        assert.ok(rowResult.value >= 0.985, `${label} 행 흰 글자 일치율 ${rowResult.value.toFixed(3)} (${rowResult.both}/${rowResult.either})`);
    }
});

// Help 와 Music Volume 하위 목록. (2026-10-29 WB03-F)
test('원본 스크린샷 대조: Help 펼침 메뉴와 Music Volume 하위 목록', (t) => {
    const help = new PopupMenu({ resources, screen: SCREEN });
    help.open(buildHelpItems({ includeAll: true }), { x: 630, y: 311 });
    const helpBounds = help.levels[0].list.bounds();
    assert.deepEqual([helpBounds.right - helpBounds.left, helpBounds.bottom - helpBounds.top], [125, 51]);
    const helpResult = compareWithScreenshot(renderPopup(help), helpBounds, 'mainMenu - Help.png', t);
    if (helpResult) assert.ok(helpResult.white.value >= 0.95, `Help 흰 글자 마스크 일치율 ${helpResult.white.value.toFixed(3)}`);
    // Music Volume 행을 눌러 연 하위 목록(현재 단계 Volume 2 에 표시).
    const { popup } = openOptions({ x: 551, y: 294 });
    const point = rowPoint(popup.levels[0].list, 'Music Volume');
    popup.pointerDown(point.x, point.y);
    const childBounds = popup.levels[1].list.bounds();
    assert.deepEqual([childBounds.right - childBounds.left, childBounds.bottom - childBounds.top], [83, 85]);
    const childResult = compareWithScreenshot(renderPopup(popup), childBounds, 'mainMenu - Options - Music Volume.png', t);
    if (!childResult) return;
    assert.ok(childResult.white.value >= 0.95, `Volume 목록 흰 글자 마스크 일치율 ${childResult.white.value.toFixed(3)}`);
    assert.ok(childResult.blue.value >= 0.9, `Volume 목록 켜짐 표시 일치율 ${childResult.blue.value.toFixed(3)}`);
    // 자식은 부모 오른쪽 가장자리에서 부모 행 위쪽 높이에 붙는다. 스크린샷에서 Music Volume 행(부모 위 + 135)과 같은 높이이다.
    assert.equal(childBounds.top - popup.levels[0].list.bounds().top, 135);
});

// 메인 메뉴 화면과의 연결: 버튼을 뗀 위치가 명령과 함께 전달되고, 펼침 메뉴가 열려 있는 동안 입력은 모두 메뉴가 받으며, 메뉴 밖 누름은 메뉴만 닫는다. (2026-10-29 WB03-F)
test('메인 메뉴 연결: 위치 전달 · 모달 입력 · 대화상자·크기 변경과의 관계', () => {
    const received = [];
    const screen = new MainMenuScreen({ resources, onCommand: (command, label, point) => received.push({ command, label, point }) });
    screen.resize(1024, 768);
    const help = screen.buttons.find((button) => button.label === 'Help');
    const bounds = help.bounds();
    const x = bounds.left + 20;
    const y = bounds.top + 8;
    assert.equal(screen.pointerDown(x, y), true);
    assert.equal(help.pressed, 1);
    screen.pointerUp(x, y);
    // 펼침 메뉴 기준점은 버튼 가운데 x(폭 67 → 33)와 버튼 위쪽 y 이다(뗀 위치가 아니다).
    assert.deepEqual(received.map((entry) => [entry.command.type, entry.command.target, entry.label, entry.point]), [['menu', 'help', 'Help', { x: bounds.left + 33, y: bounds.top }]]);
    // 펼침 메뉴를 연다. 열려 있는 동안 메인 메뉴 버튼은 그대로 그려진다(펼침 메뉴가 위에 겹쳐 보임).
    const items = buildHelpItems();
    const popup = new PopupMenu({ resources, screen: { width: 1024, height: 768 } });
    popup.open(items, received[0].point);
    const before = new IndexedSurface(1024, 768);
    screen.render(before);
    screen.setPopup(popup);
    const after = new IndexedSurface(1024, 768);
    screen.render(after);
    const popupBounds = popup.levels[0].list.bounds();
    // 팝업 영역 안에서 달라진 픽셀이 대부분이다(한 픽셀은 우연히 같을 수 있어 개수로 확인한다).
    let changed = 0;
    for (let y = popupBounds.top; y < popupBounds.bottom; y += 1) for (let x = popupBounds.left; x < popupBounds.right; x += 1) if (before.pixels[y * 1024 + x] !== after.pixels[y * 1024 + x]) changed += 1;
    assert.ok(changed > 1000, `펼침 메뉴가 그려지지 않았습니다(달라진 픽셀 ${changed})`);
    // 팝업 밖의 다른 버튼을 눌러도 버튼은 눌리지 않고 메뉴만 닫힌다.
    const credits = screen.buttons.find((button) => button.label === 'Credits').bounds();
    assert.equal(screen.pointerDown(credits.left + 5, credits.top + 5), true);
    assert.ok(screen.popup === null, '펼침 메뉴가 닫혀 있어야 합니다');
    assert.equal(screen.buttons.find((button) => button.label === 'Credits').pressed, 0);
    // 닫힌 뒤에는 다시 버튼이 눌린다.
    assert.equal(screen.pointerDown(credits.left + 5, credits.top + 5), true);
    assert.equal(screen.buttons.find((button) => button.label === 'Credits').pressed, 1);
    screen.cancel();
    // 크기가 바뀌면 열린 펼침 메뉴가 닫히고, 대화상자가 열려도 닫히며, Esc 용 closePopup 은 열려 있었는지 알려 준다.
    screen.setPopup(popup);
    popup.open(items, { x, y });
    screen.resize(1366, 768);
    assert.ok(screen.popup === null, '펼침 메뉴가 닫혀 있어야 합니다');
    assert.equal(popup.isOpen, false);
    const second = new PopupMenu({ resources, screen: { width: 1366, height: 768 } });
    second.open(items, { x, y });
    screen.setPopup(second);
    assert.equal(screen.closePopup(), true);
    assert.equal(screen.closePopup(), false);
    screen.setPopup(second);
    screen.setDialog({ screen: {}, fit() {}, place() {}, cancel() { return false; } });
    assert.ok(screen.popup === null, '펼침 메뉴가 닫혀 있어야 합니다');
    // 메뉴 안의 행을 눌러 명령이 실행되면 메뉴 객체가 비워진다.
    const commands = [];
    const live = new PopupMenu({ resources, screen: { width: 1366, height: 768 }, onCommand: (command) => commands.push(command) });
    live.open(items, { x: 100, y: 100 });
    screen.setDialog(null);
    screen.setPopup(live);
    screen.pointerDown(110, 105);
    assert.equal(commands.length, 1);
    assert.ok(screen.popup === null, '펼침 메뉴가 닫혀 있어야 합니다');
});

// 펼침 메뉴 기준점: 원본 8버튼 배치(1024×768)에서 Help 는 (630,311), Options 는 (551,294) 이다. 두 값은 원본 스크린샷에서 타이틀 그림으로 구한 클라이언트 원점 기준 측정값이며,
// 규칙은 x = 버튼 가운데(폭 75 → 37), y = 버튼 위쪽 + popupDy(Help 0, Options -40)이다. 웹 5버튼 배치에서는 같은 규칙이 각 버튼 기준으로 적용된다. (2026-10-29 WB03-F)
test('펼침 메뉴 기준점: 원본 8버튼 배치에서 Help (630,311) · Options (551,294)', () => {
    const original = new MainMenuScreen({ resources, includeAllButtons: true });
    original.resize(1024, 768);
    const anchor = (screen, label) => screen.popupAnchor(screen.buttons.find((button) => button.label === label));
    assert.deepEqual(anchor(original, 'Help'), { x: 630, y: 311 });
    assert.deepEqual(anchor(original, 'Options'), { x: 551, y: 294 });
    // 웹 5버튼 배치(폭 67)의 같은 규칙: 버튼 가운데 x, Help 는 버튼 위쪽, Options 는 40 위. 16:9 화면에서는 버튼이 가운데로 옮겨가므로 기준점도 따라간다.
    const web = new MainMenuScreen({ resources });
    web.resize(1024, 768);
    assert.deepEqual(anchor(web, 'Help'), { x: 549 + 33, y: 311 });
    assert.deepEqual(anchor(web, 'Options'), { x: 514 + 33, y: 334 - 40 });
    web.resize(1366, 768);
    assert.equal(anchor(web, 'Help').x - web.buttons.find((button) => button.label === 'Help').bounds().left, 33);
    // 어느 위치에서 열어도 화면 안으로 보정된다(좁은 화면의 오른쪽 끝 버튼).
    const popup = new PopupMenu({ resources, screen: { width: 1024, height: 768 } });
    popup.open(buildOptionsItems(new Settings()), { x: 1000, y: 294 });
    assert.equal(popup.levels[0].list.bounds().right, 1024);
});

// 가짜 Web Audio 문맥이다. 재생·정지 호출과 음량 값을 기록한다. (2026-10-29 WB03-F)
function fakeAudioContext() {
    const log = { started: [], stopped: 0 };
    const gain = () => ({ gain: { value: 1 }, connect() {} });
    return {
        log,
        destination: {},
        createGain: gain,
        createBufferSource() {
            const source = {
                loop: false,
                connect() {},
                disconnect() {},
                start() { log.started.push(source); },
                stop() { log.stopped += 1; },
            };
            return source;
        },
        decodeAudioData: async () => ({}),
    };
}

// AudioSystem: 소리 끄기는 이후 효과음을 막고, 음악 끄기는 멈추되 곡을 기억해 다시 켜면 이어 재생하며, 음량 단계는 바로 증폭률이 된다. (2026-10-29 WB03-F)
test('AudioSystem: 소리·음악 켜기/끄기와 음량 단계', async () => {
    const context = fakeAudioContext();
    const audio = new AudioSystem(context, { soundVolumeStep: 3, musicVolumeStep: 2 });
    await audio.loadSound('button.wav', new Uint8Array(4));
    await audio.loadMusic('ser22.mus', new Uint8Array(4));
    // 효과음: 켜짐 → 재생, 끔 → 재생하지 않음(false), 다시 켬 → 재생.
    assert.equal(audio.playSound('button.wav'), true);
    audio.setSoundEnabled(false);
    assert.equal(audio.playSound('button.wav'), false);
    audio.setSoundEnabled(true);
    assert.equal(audio.playSound('button.wav'), true);
    assert.equal(context.log.started.length, 2);
    // 음악: 재생 → 끔(멈춤) → 끈 채 재생 요청(기억만) → 켬(기억한 곡 재생).
    assert.equal(audio.playMusic('ser22.mus'), true);
    assert.equal(context.log.started.length, 3);
    audio.setMusicEnabled(false);
    assert.equal(context.log.stopped, 1);
    assert.equal(audio.currentMusic, null);
    assert.equal(audio.playMusic('ser22.mus'), false);
    assert.equal(context.log.started.length, 3);
    audio.setMusicEnabled(true);
    assert.equal(context.log.started.length, 4);
    assert.equal(audio.currentMusic, 'ser22.mus');
    // 이미 재생 중인 같은 곡은 다시 시작하지 않는다. stopMusic 은 요청한 곡도 잊어 다시 켜도 재생하지 않는다.
    audio.playMusic('ser22.mus');
    assert.equal(context.log.started.length, 4);
    audio.stopMusic();
    audio.setMusicEnabled(false);
    audio.setMusicEnabled(true);
    assert.equal(context.log.started.length, 4);
    // 음량 단계: 3 → -10dB, 5 → 0dB, 1 → -40dB 의 진폭 배율이다.
    assert.ok(Math.abs(audio.soundGain.gain.value - Math.pow(10, -1000 / 2000)) < 1e-12);
    audio.setSoundVolumeStep(5);
    assert.equal(audio.soundGain.gain.value, 1);
    audio.setMusicVolumeStep(1);
    assert.ok(Math.abs(audio.musicGain.gain.value - 0.01) < 1e-12);
    // 닫은 뒤에는 요청을 무시한다.
    audio.close();
    assert.equal(audio.playMusic('ser22.mus'), false);
    assert.equal(audio.playSound('button.wav'), false);
});
