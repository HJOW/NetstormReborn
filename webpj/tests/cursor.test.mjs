// 본문 위 커서 모양을 검사한다: 커서 번호 15(스크롤)·16(링크) 자산과 번호 표(원본 표 README·cpppj 상수와 대조), 본문의 판정 규칙(링크 > 스크롤 가능 > 기본),
// Tell·도움말 창과 메인 메뉴를 거친 전달, 앱(netstorm.js)의 커서 고르기.
// 판정 규칙의 원본은 cpppj ScrollingTextGumpWidgets::Handle 의 커서 사건이다: 링크 조각이면 16, 아니면 스크롤바가 있고 쪽 크기가 범위보다 작으면 15, 아니면 1.
// 커서 사건은 그 점에서 가장 위에 있는 위젯에만 가므로(GumpInput::Overlay) 본문 밖이나 본문 위의 스크롤바 위는 기본(1)이다.
// 기대값 파일이 없고 cpppj 는 호스트 장치로 커서를 바꾸므로 규칙은 소스 상수와 대조하고 나머지는 구조 검사로 한다.
// (2026-10-31 WB03-L: 본문 위 커서 모양 이식 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { MENU_ASSET_PATHS, buildMenuResources } from '../www/assets/menu-assets.js';
import { CURSOR_LINK, CURSOR_NORMAL, CURSOR_PATHS, CURSOR_SCROLL, decodeCursorResource } from '../www/ui/cursor.js';
import { measureFragment } from '../www/ui/style-text.js';
import { createTellWindow } from '../www/ui/tell-window.js';
import { createHelpWindow } from '../www/ui/help-window.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { AppState, Netstorm } from '../www/netstorm.js';

// 메뉴 묶음 파일 전부를 읽어 자원을 만든다(검사마다 공유). (2026-10-31 WB03-L)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// 논리 화면 크기이다(4:3). (2026-10-31 WB03-L)
const SCREEN = Object.freeze({ width: 1024, height: 768 });

// 새 게임 데이터를 만든다. extra 는 추가·대체할 느슨한 파일(경로 → 바이트)이다. (2026-10-31 WB03-L)
function newData({ extra = [] } = {}) {
    return GameData.build({ files: new Map([...files, ...extra]), language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
}

// 시험용 Tell 섹션 하나를 가진 게임 데이터를 만든다. lines 는 본문 줄 배열이다. (2026-10-31 WB03-L)
function dataWith(name, lines) {
    const script = [`[${name}]`, '<h2>{H2}Title</h2>', '<p>{Text}', ...lines, '$Button=Close,DoNothing', ''].join('\r\n');
    return newData({ extra: [['d/tell.english', new TextEncoder().encode(script)]] });
}

// Tell 창을 만든다. (2026-10-31 WB03-L)
function newTell(name, data = newData()) {
    const result = createTellWindow({ gameData: data, resources, name, screen: SCREEN, now: () => 0 });
    assert.ok(result.window, `${name} 창을 만들지 못했습니다`);
    return result.window;
}

// 도움말 창을 만든다. (2026-10-31 WB03-L)
function newHelp(topic = 'F1Help', data = newData()) {
    const result = createHelpWindow({ gameData: data, resources, topic, screen: SCREEN, now: () => 0 });
    assert.ok(result.window, `${topic} 도움말 창을 만들지 못했습니다`);
    return result.window;
}

// 창 본문에서 링크 조각(tag 가 0 이 아님)의 화면 위치를 찾는다. 글자 한가운데를 돌려준다(첫 링크). 없으면 null. (2026-10-31 WB03-L)
function findAnyLink(window) {
    const body = window.body;
    body.ensureLayout();
    const bounds = body.bounds();
    for (const line of body.softLines) {
        let width = 0;
        for (const fragment of line.fragments) {
            const size = measureFragment(body.fonts, fragment);
            const x = bounds.left + width + Math.trunc(size.width / 2);
            const y = bounds.top + line.y - body.scroll + line.height - 2;
            if (fragment.tag && body.hitTest(x, y)?.tag === fragment.tag) return { x, y };
            width += size.width;
        }
    }
    return null;
}

// 링크가 없는 본문 글자 위의 점을 찾는다(첫 줄의 첫 글자 조각 안, 링크가 아닌 조각). 없으면 null. (2026-10-31 WB03-L)
function findPlainText(window) {
    const body = window.body;
    body.ensureLayout();
    const bounds = body.bounds();
    for (const line of body.softLines) {
        let width = 0;
        for (const fragment of line.fragments) {
            const size = measureFragment(body.fonts, fragment);
            const x = bounds.left + width + Math.trunc(size.width / 2);
            const y = bounds.top + line.y - body.scroll + line.height - 2;
            if (!fragment.tag && size.width > 2 && body.hitTest(x, y) === fragment) return { x, y };
            width += size.width;
        }
    }
    return null;
}

// 원본 커서 표(README)의 한 행을 읽는다: { number, file, hotspotX, hotspotY }. 표에 없으면 null. (2026-10-31 WB03-L)
function readCursorTable() {
    const text = fs.readFileSync(path.join(REPO_ROOT, 'assets', 'game-data', 'cursors', 'README.md'), 'utf8');
    const rows = new Map();
    // 표의 데이터 행 `| 번호 | 그룹 | `파일` | (x, y) | …` 만 읽는다.
    for (const match of text.matchAll(/^\|\s*(\d+)\s*\|\s*\d+\s*\|\s*`(RT_CURSOR_\d+\.bin)`\s*\|\s*\((\d+),\s*(\d+)\)/gm)) {
        rows.set(Number(match[1]), { file: match[2], hotspotX: Number(match[3]), hotspotY: Number(match[4]) });
    }
    return rows;
}

// 커서 번호 표: 번호는 원본 값(1·15·16)이고, 번호별 파일·핫스폿이 원본 커서 표(README)와 같으며, 모두 메뉴 묶음에 들어 있고 해독한 값이 자원 묶음에 있다. (2026-10-31 WB03-L)
test('커서 번호 표: 원본 번호·파일·핫스폿과 같고 메뉴 묶음에 들어 있다', () => {
    assert.deepEqual([CURSOR_NORMAL, CURSOR_SCROLL, CURSOR_LINK], [1, 15, 16]);
    assert.deepEqual(Object.keys(CURSOR_PATHS).map(Number), [CURSOR_NORMAL, CURSOR_SCROLL, CURSOR_LINK]);
    const table = readCursorTable();
    assert.equal(table.size, 18);
    // 번호마다 파일 이름·핫스폿이 원본 표와 같고, 해독한 크기·핫스폿이 자원 묶음의 값과 같다.
    for (const [number, cursorPath] of Object.entries(CURSOR_PATHS)) {
        const row = table.get(Number(number));
        assert.equal(cursorPath, `cursors/${row.file}`, `번호 ${number}`);
        const cursor = decodeCursorResource(readData(cursorPath));
        assert.deepEqual([cursor.width, cursor.height, cursor.hotspotX, cursor.hotspotY, cursor.inverted], [32, 32, row.hotspotX, row.hotspotY, 0], `번호 ${number}`);
        assert.deepEqual([resources.cursors[number].hotspotX, resources.cursors[number].hotspotY], [row.hotspotX, row.hotspotY], `번호 ${number} 자원`);
        // 매니페스트의 메뉴 묶음에 들어 있어야 화면 표시 전에 내려받는다.
        const entry = manifest.files.find((candidate) => candidate.path === cursorPath);
        assert.equal(entry?.bundle, 'menu', cursorPath);
    }
    assert.equal(resources.cursor, resources.cursors[CURSOR_NORMAL]);
    assert.equal(MENU_ASSET_PATHS.cursorScroll, CURSOR_PATHS[CURSOR_SCROLL]);
    assert.equal(MENU_ASSET_PATHS.cursorLink, CURSOR_PATHS[CURSOR_LINK]);
});

// 커서 그림: 번호 15 는 가로 막대를 지나는 위아래 화살표, 번호 16 은 손가락을 편 손이다. 흰색·검정 픽셀이 있고 핫스폿이 그림 안에 있다. (2026-10-31 WB03-L)
test('커서 그림: 스크롤 화살표와 가리키는 손', () => {
    const opaqueAt = (cursor, x, y) => cursor.rgba[(y * cursor.width + x) * 4 + 3] !== 0;
    const whiteAt = (cursor, x, y) => opaqueAt(cursor, x, y) && cursor.rgba[(y * cursor.width + x) * 4] > 128;
    const scroll = resources.cursors[CURSOR_SCROLL];
    // 위쪽 화살표 끝(15,3)과 아래쪽 화살표 끝(15,24)이 흰색이고, 가로 막대 가운데(핫스폿 15,13)는 속이 검정이다.
    assert.ok(whiteAt(scroll, 15, 3) && whiteAt(scroll, 15, 24));
    assert.ok(opaqueAt(scroll, scroll.hotspotX, scroll.hotspotY) && !whiteAt(scroll, scroll.hotspotX, scroll.hotspotY));
    // 막대는 폭 19 이상(5..25)이고 위아래 화살표 사이가 비어 있지 않다.
    for (let x = 6; x <= 24; x += 1) assert.ok(opaqueAt(scroll, x, 12) && opaqueAt(scroll, x, 15), `막대 ${x}`);
    const hand = resources.cursors[CURSOR_LINK];
    // 손가락 끝: 핫스폿(12,9)이 흰색 손가락 안이고, 손가락 위(12,4)는 투명하다. 손바닥(16,19)은 흰색이다.
    assert.ok(whiteAt(hand, hand.hotspotX, hand.hotspotY));
    assert.ok(!opaqueAt(hand, 12, 4));
    assert.ok(whiteAt(hand, 16, 19));
    // 두 그림은 다르고 기본 화살표와도 다르다.
    assert.notDeepEqual(Array.from(scroll.rgba), Array.from(hand.rgba));
    assert.notDeepEqual(Array.from(scroll.rgba), Array.from(resources.cursors[CURSOR_NORMAL].rgba));
});

// 원본 상수 대조: cpppj ScrollingTextGumpWidgets 의 kNormalCursor·kScrollCursor·kTagCursor 가 이 번호와 같다. cpppj 소스가 없는 환경이면 건너뛴다. (2026-10-31 WB03-L)
test('커서 번호: cpppj 다중 줄 위젯의 상수와 같다', (t) => {
    const source = path.join(REPO_ROOT, 'cpppj', 'src', 'client', 'ScrollingTextGumpWidgets.cpp');
    if (!fs.existsSync(source)) {
        t.skip('cpppj 소스가 없는 환경');
        return;
    }
    const text = fs.readFileSync(source, 'utf8');
    const match = text.match(/kNormalCursor=(\d+),kScrollCursor=(\d+),kTagCursor=(\d+)/);
    assert.ok(match, '상수 선언을 찾지 못했습니다');
    assert.deepEqual([Number(match[1]), Number(match[2]), Number(match[3])], [CURSOR_NORMAL, CURSOR_SCROLL, CURSOR_LINK]);
});

// 링크가 있는 짧은 본문(스크롤바 없음): 링크 위는 손, 링크가 아닌 글자·빈 곳·본문 밖은 기본이다. 닫힌 창은 기본이다. (2026-10-31 WB03-L)
test('Tell 본문 커서: 링크 위 손, 그 밖은 기본', () => {
    const window = newTell('TechSupport');
    assert.equal(window.body.slider, null);
    const link = findAnyLink(window);
    assert.ok(link, '링크 조각을 찾지 못했습니다');
    assert.equal(window.cursorAt(link.x, link.y), CURSOR_LINK);
    // 링크 글자 위를 벗어난 곳: 링크가 아닌 글자, 본문 첫 줄 맨 앞 여백, 링크 줄 맨 오른쪽 빈 곳은 모두 기본이다(스크롤바가 없어 15 도 아니다).
    const plain = findPlainText(window);
    assert.ok(plain);
    assert.equal(window.cursorAt(plain.x, plain.y), CURSOR_NORMAL);
    const bounds = window.body.bounds();
    assert.equal(window.cursorAt(bounds.right - 2, link.y), CURSOR_NORMAL);
    // 본문 밖(패널 가장자리·돌 버튼 위·화면 바깥)과 본문 경계의 바깥 한 칸은 기본이다.
    const rect = window.bounds();
    assert.equal(window.cursorAt(rect.left + 1, rect.top + 1), CURSOR_NORMAL);
    const ok = window.buttons[0].bounds();
    assert.equal(window.cursorAt(ok.left + 3, ok.top + 3), CURSOR_NORMAL);
    assert.equal(window.cursorAt(-5, -5), CURSOR_NORMAL);
    assert.equal(window.cursorAt(bounds.left - 1, link.y), CURSOR_NORMAL);
    // 링크가 하나도 없는 창(Credits)은 어디서나 기본이다.
    const credits = newTell('Credits');
    const creditsBody = credits.body.bounds();
    for (const [x, y] of [[creditsBody.left + 3, creditsBody.top + 3], [creditsBody.left + 40, creditsBody.top + 20], [creditsBody.right - 3, creditsBody.bottom - 3]]) assert.equal(credits.cursorAt(x, y), CURSOR_NORMAL);
    // 닫힌 창은 기본이다.
    window.close();
    assert.equal(window.cursorAt(link.x, link.y), CURSOR_NORMAL);
});

// 화면에 들어가지 않는 긴 본문: 글자 위는 스크롤 모양, 링크 위는 링크 모양(링크가 우선), 스크롤바 위는 기본이다. 스크롤해 링크가 보이지 않으면 링크 모양이 아니다. (2026-10-31 WB03-L)
test('Tell 본문 커서: 긴 본문의 스크롤 모양과 링크 우선', () => {
    const lines = [];
    // 번호가 붙은 시험 줄 90개 중 3번째 줄에 링크를 넣는다.
    for (let number = 1; number <= 90; number += 1) {
        const text = number === 3 ? 'Read <a href="stormPowerHelp">{Link}Storm Power</a>{Text} now' : `Line ${number}: The quick brown fox jumps over the lazy dog, number ${number}.`;
        lines.push(`${text}<br>`);
    }
    const window = newTell('LongLink', dataWith('LongLink', lines));
    const body = window.body;
    const slider = body.slider;
    assert.ok(slider && slider.page < slider.maximum - slider.minimum);
    const plain = findPlainText(window);
    const link = findAnyLink(window);
    assert.ok(plain && link);
    assert.equal(window.cursorAt(plain.x, plain.y), CURSOR_SCROLL);
    assert.equal(window.cursorAt(link.x, link.y), CURSOR_LINK);
    // 본문 안이라도 글자 줄 오른쪽의 빈 곳(스크롤바 왼쪽)은 스크롤 모양이다.
    const bounds = body.bounds();
    assert.equal(window.cursorAt(bounds.right - 12, bounds.top + 30), CURSOR_SCROLL);
    // 본문 경계는 왼쪽·위쪽이 포함이고 오른쪽·아래쪽이 제외이다: 경계 한 칸 안은 스크롤 모양, 바로 밖은 기본이다.
    assert.equal(window.cursorAt(bounds.left, bounds.top + 30), CURSOR_SCROLL);
    assert.equal(window.cursorAt(bounds.left + 20, bounds.top), CURSOR_SCROLL);
    assert.equal(window.cursorAt(bounds.left + 20, bounds.bottom - 1), CURSOR_SCROLL);
    assert.equal(window.cursorAt(bounds.right, bounds.top + 30), CURSOR_NORMAL);
    assert.equal(window.cursorAt(bounds.left + 20, bounds.bottom), CURSOR_NORMAL);
    assert.equal(window.cursorAt(bounds.left + 20, bounds.top - 1), CURSOR_NORMAL);
    assert.equal(window.cursorAt(bounds.left - 1, bounds.top + 30), CURSOR_NORMAL);
    // 스크롤바 위(화살표·홈·손잡이)는 본문이 받지 않으므로 기본이다.
    const thumb = slider.thumbBounds();
    const down = slider.increment.bounds();
    for (const [x, y] of [[thumb.left + 3, thumb.top + 30], [thumb.left + 3, thumb.bottom - 3], [down.left + 3, down.top + 3], [Math.trunc(slider.x) + 3, Math.trunc(slider.y) + 3]]) assert.equal(window.cursorAt(x, y), CURSOR_NORMAL, `${x},${y}`);
    // 링크 줄이 위로 밀려 보이지 않게 스크롤하면 같은 자리는 스크롤 모양으로 바뀐다.
    body.setScroll(900);
    assert.equal(window.cursorAt(link.x, link.y), CURSOR_SCROLL);
    // 쪽 크기가 범위와 같아지면(스크롤할 내용이 없음) 스크롤 모양이 아니다: 경계 `<` 의 검사.
    slider.setRange(0, slider.page);
    assert.equal(window.cursorAt(plain.x, plain.y), CURSOR_NORMAL);
    slider.setRange(0, slider.page + 1);
    assert.equal(window.cursorAt(plain.x, plain.y), CURSOR_SCROLL);
});

// 도움말 창: 주제마다 스크롤바가 항상 있다. 넘치는 주제의 글자 위는 스크롤 모양, 넘치지 않는 주제는 기본이며 링크 위는 어느 쪽이든 손이다. (2026-10-31 WB03-L)
test('도움말 본문 커서: 넘치는 주제와 넘치지 않는 주제', () => {
    const data = newData();
    const helpText = data.readText(data.get('Quick Help Spec'));
    const topics = [...new Set([...helpText.matchAll(/<a\s+name\s*=\s*"([^"]+)"/gi)].map((match) => match[1]))];
    const counts = { overflow: 0, fits: 0, links: 0 };
    // 모든 주제의 창을 열어 글자 위 커서가 넘침 여부와 맞는지, 링크 위는 손인지 본다.
    for (const topic of topics) {
        const window = newHelp(topic, data);
        const slider = window.body.slider;
        const overflow = slider.page < slider.maximum - slider.minimum;
        const plain = findPlainText(window);
        if (plain) {
            assert.equal(window.cursorAt(plain.x, plain.y), overflow ? CURSOR_SCROLL : CURSOR_NORMAL, topic);
            counts[overflow ? 'overflow' : 'fits'] += 1;
        }
        const link = findAnyLink(window);
        if (link) {
            assert.equal(window.cursorAt(link.x, link.y), CURSOR_LINK, topic);
            counts.links += 1;
        }
        // 창 밖(화면 바깥)과 스크롤바 위, Back 단추 위는 기본이다.
        assert.equal(window.cursorAt(-3, -3), CURSOR_NORMAL, topic);
        assert.equal(window.cursorAt(Math.trunc(slider.x) + 3, Math.trunc(slider.y) + 20), CURSOR_NORMAL, topic);
        const back = window.backButton.bounds();
        assert.equal(window.cursorAt(back.left + 3, back.top + 3), CURSOR_NORMAL, topic);
    }
    assert.ok(counts.overflow > 20 && counts.fits > 5 && counts.links > 20, JSON.stringify(counts));
    // 닫힌 도움말 창은 기본이다.
    const closed = newHelp('F1Help', data);
    const link = findAnyLink(closed);
    assert.ok(link);
    closed.close();
    assert.equal(closed.cursorAt(link.x, link.y), CURSOR_NORMAL);
});

// 메인 메뉴: 대화상자가 열려 있으면 대화상자가 고르고, 없거나 펼침 메뉴가 열려 있거나 cursorAt 이 없는 대화상자면 기본이다. 돌 버튼 위는 기본이다. (2026-10-31 WB03-L)
test('메인 메뉴 커서: 대화상자 전달과 기본값', () => {
    const menu = new MainMenuScreen({ resources });
    menu.resize(SCREEN.width, SCREEN.height);
    const button = menu.buttons[0].bounds();
    assert.equal(menu.cursorAt(button.left + 3, button.top + 3), CURSOR_NORMAL);
    const window = newTell('TechSupport');
    menu.setDialog(window);
    const link = findAnyLink(window);
    assert.equal(menu.cursorAt(link.x, link.y), CURSOR_LINK);
    // 펼침 메뉴가 열려 있으면 대화상자 위라도 기본이다.
    menu.popup = { isOpen: true };
    assert.equal(menu.cursorAt(link.x, link.y), CURSOR_NORMAL);
    menu.popup = null;
    // cursorAt 이 없는 대화상자는 기본이다.
    menu.dialog = { cursorAt: undefined };
    assert.equal(menu.cursorAt(link.x, link.y), CURSOR_NORMAL);
    menu.dialog = null;
    assert.equal(menu.cursorAt(link.x, link.y), CURSOR_NORMAL);
});

// 앱의 커서 고르기(_wantedCursor): 메인 메뉴 상태에서 포인터가 화면 안일 때만 메뉴가 고르고, 그 밖(다른 상태·화면 밖·자원 없음)은 기본이며,
// 고른 번호의 자원이 묶음에 없으면 기본으로 대신한다. 실제 CSS 적용은 브라우저에서 확인한다. (2026-10-31 WB03-L)
test('앱 커서 고르기: 상태·화면 밖·자원 없음', () => {
    const wanted = (overrides) => Netstorm.prototype._wantedCursor.call({
        _state: AppState.MAIN_MENU,
        _input: { pointer: { x: 10, y: 20, inside: true } },
        _resources: { cursors: { [CURSOR_NORMAL]: {}, [CURSOR_SCROLL]: {}, [CURSOR_LINK]: {} } },
        _mainMenu: { cursorAt: () => CURSOR_LINK },
        ...overrides,
    });
    assert.equal(wanted({}), CURSOR_LINK);
    // 메인 메뉴가 아닌 상태, 메뉴 없음, 자원 없음, 포인터가 화면 밖이면 기본이다.
    assert.equal(wanted({ _state: AppState.CLICK_TO_START }), CURSOR_NORMAL);
    assert.equal(wanted({ _mainMenu: null }), CURSOR_NORMAL);
    assert.equal(wanted({ _resources: null }), CURSOR_NORMAL);
    assert.equal(wanted({ _input: { pointer: { x: 10, y: 20, inside: false } } }), CURSOR_NORMAL);
    // 그 번호의 자원이 없으면 기본이다.
    assert.equal(wanted({ _resources: { cursors: { [CURSOR_NORMAL]: {} } } }), CURSOR_NORMAL);
    // 메뉴에는 포인터 좌표가 그대로 전달된다.
    const seen = [];
    wanted({ _mainMenu: { cursorAt: (x, y) => { seen.push([x, y]); return CURSOR_SCROLL; } } });
    assert.deepEqual(seen, [[10, 20]]);
});
