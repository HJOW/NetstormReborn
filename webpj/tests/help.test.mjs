// 일반 도움말(F1) 창 복원을 검사한다: 선 그리기·스크롤바·본문 스크롤·주제 본문 변환(링크·조건)과 창 배치·입력·방문 기록, 그리고 원본 스크린샷(help - NetStorm Instructions.png)과의 픽셀 대조.
// 스크린샷은 창 테두리가 있는 외부 캡처라 원점을 창 밖 타이틀 그림으로 먼저 정한 뒤 비교한다(순환 논리 방지). 창 영역은 평균 채널 차이 1 미만으로 사실상 픽셀 일치하며
// 본문 글자 마스크·화살표 삼각형은 정확히 같다. 위치 기대값은 이 정렬을 확인한 뒤 고정한 값이다.
// (2026-10-30 WB03-G: 일반 도움말 창 복원 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, decodePng, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { drawVfxLine } from '../www/render/vfx-line.js';
import { createTextContext } from '../www/render/glyph-text.js';
import { SLIDER_ARROW_COLOR, SliderStep, VerticalSlider } from '../www/ui/slider.js';
import { ScrollingText } from '../www/ui/scroll-text.js';
import { LinkTable, applyHtmlCondition, createParserGlobals, createParserState, htmlPictureText, parseHtml } from '../www/ui/html-text.js';
import { parseStyleText } from '../www/ui/style-text.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';
import { createHelpWindow } from '../www/ui/help-window.js';
import { MainMenuScreen, titlePosition } from '../www/ui/main-menu.js';

// 메뉴 묶음 파일 전부를 읽어 자원·게임 데이터를 만든다(검사마다 공유). (2026-10-30 WB03-G)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);
const colors = resources.colors;
const SCREENSHOT = path.join(REPO_ROOT, 'screenShots', 'help - NetStorm Instructions.png');

// 새 게임 데이터를 만든다. (2026-10-30 WB03-G)
function newData() {
    return GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
}

// 도움말 창을 만들어 돌려준다. 시계는 clock.now 를 바꿔 진행하고 호출은 events 에 쌓는다. (2026-10-30 WB03-G)
function newHelp({ topic = 'F1Help', width = 1024, height = 768, data = newData() } = {}) {
    const events = { closed: 0, commands: [], externals: [], sounds: [], unsupported: [] };
    const clock = { now: 0 };
    const result = createHelpWindow({
        gameData: data,
        resources,
        topic,
        screen: { width, height },
        now: () => clock.now,
        onClose: () => { events.closed += 1; },
        onCommand: (command) => events.commands.push(command),
        onExternal: (address) => events.externals.push(address),
        onSound: (kind) => events.sounds.push(kind),
        onUnsupported: (what) => events.unsupported.push(what),
    });
    assert.ok(result.window, '도움말 창을 만들지 못했습니다');
    return { window: result.window, events, clock, data };
}

// 8비트 표면의 한 픽셀 RGB 를 돌려준다. (2026-10-30 WB03-G)
function rgbAt(pixels, width, x, y) {
    const index = pixels[y * width + x] * 3;
    return [resources.palette.rgb[index], resources.palette.rgb[index + 1], resources.palette.rgb[index + 2]];
}

// 선 그리기: 수평·수직은 양 끝 포함, 대각선은 0.32 분수 위상(시작 0.5)으로 부축을 옮기며 클립 밖은 찍지 않는다. (2026-10-30 WB03-G)
test('VFX 선: 수평·수직 양 끝 포함 · 대각선 분수 위상 · 클립', () => {
    const surface = new IndexedSurface(16, 16);
    const lit = () => {
        const out = [];
        for (let y = 0; y < 16; y += 1) for (let x = 0; x < 16; x += 1) if (surface.pixels[y * 16 + x]) out.push([x, y]);
        return out;
    };
    drawVfxLine(surface, { x: 2, y: 3 }, { x: 5, y: 3 }, 9);
    assert.deepEqual(lit(), [[2, 3], [3, 3], [4, 3], [5, 3]]);
    surface.fill(0);
    drawVfxLine(surface, { x: 4, y: 7 }, { x: 4, y: 5 }, 9);
    assert.deepEqual(lit(), [[4, 5], [4, 6], [4, 7]]);
    surface.fill(0);
    // 기울기 1/2: 위상이 0.5 에서 시작하므로 첫 걸음에 바로 부축이 한 칸 움직인다.
    drawVfxLine(surface, { x: 0, y: 0 }, { x: 4, y: 2 }, 9);
    assert.deepEqual(lit(), [[0, 0], [1, 1], [2, 1], [3, 2], [4, 2]]);
    surface.fill(0);
    // 두 변이 같으면 매 걸음 대각선이다. 거꾸로 그려도 같은 모양이다(반대 방향 부호).
    drawVfxLine(surface, { x: 6, y: 6 }, { x: 2, y: 2 }, 9);
    assert.deepEqual(lit(), [[2, 2], [3, 3], [4, 4], [5, 5], [6, 6]]);
    surface.fill(0);
    // 클립 밖 픽셀은 찍지 않고, 한 점도 선이다.
    drawVfxLine(surface, { x: 0, y: 0 }, { x: 8, y: 8 }, 9, { left: 3, top: 3, right: 6, bottom: 6 });
    assert.deepEqual(lit(), [[3, 3], [4, 4], [5, 5]]);
    surface.fill(0);
    drawVfxLine(surface, { x: 9, y: 9 }, { x: 9, y: 9 }, 9);
    assert.deepEqual(lit(), [[9, 9]]);
});

// 스크롤바 값: 범위·쪽 크기·위치 제한과 손잡이 사각형(홈 길이 비율 환산·절삭). (2026-10-30 WB03-G)
test('스크롤바 값: 생성 초기값 · 범위·쪽·위치 제한 · 손잡이 사각형', () => {
    const slider = new VerticalSlider({ height: 289 });
    slider.attach(713, 59);
    // 생성자 마지막 호출과 같은 초기값(범위 0..100, 쪽 10, 위치 50)이다.
    assert.deepEqual([slider.minimum, slider.maximum, slider.page, slider.position, slider.step], [0, 100, 10, 50, 1]);
    // 홈은 화살표 두 칸(9)을 뺀 271 이고 손잡이는 (위치·쪽)/범위 비율이다: 50*271/100 = 135 → top 136, 쪽 27 → bottom 135+27-1 = 161.
    assert.deepEqual(slider.handle, { left: 1, top: 136, right: 8, bottom: 161 });
    assert.deepEqual(slider.thumbBounds(), { left: 713, top: 68, right: 722, bottom: 339 });
    // 범위를 두 끝 정렬로 받고 같으면 최댓값을 하나 늘린다. 쪽 크기는 범위 길이로 줄고 위치는 최대 - 쪽 이하이다.
    slider.setRange(327, 0);
    assert.deepEqual([slider.minimum, slider.maximum], [0, 327]);
    slider.setRange(5, 5);
    assert.deepEqual([slider.minimum, slider.maximum, slider.page], [5, 6, 1]);
    slider.setRange(0, 327);
    slider.setPage(289);
    slider.setPosition(500);
    assert.equal(slider.position, 38);
    slider.setPosition(-9);
    assert.equal(slider.position, 0);
    assert.deepEqual(slider.handle, { left: 1, top: 1, right: 8, bottom: 238 });
    // 쪽 크기가 범위보다 크면 길이로 줄인다(전체 길이를 덮는 손잡이).
    slider.setPage(5000);
    assert.equal(slider.page, 327);
    // 범위보다 큰 쪽을 직접 위치에 쓰면 원본 assert 이므로 오류이다.
    slider.page = 400;
    assert.throws(() => slider.setPosition(0), RangeError);
});

// 스크롤바 입력: 화살표 한 단계(안쪽에서 뗄 때)·반복·손잡이 위아래 쪽 이동·손잡이 끌기·취소. (2026-10-30 WB03-G)
test('스크롤바 입력: 화살표 단계 · 0.4초 뒤 반복 · 쪽 이동 · 손잡이 끌기', () => {
    const steps = [];
    const sounds = [];
    const clock = { now: 10 };
    let changes = 0;
    const slider = new VerticalSlider({ height: 289, now: () => clock.now, onStep: (code) => { steps.push(code); return true; }, onChange: () => { changes += 1; }, onSound: (kind) => sounds.push(kind) });
    slider.attach(713, 59);
    slider.setRange(0, 327);
    slider.setPage(289);
    slider.setPosition(0);
    // 아래 화살표: 누르면 눌린 모양과 효과음이 나고 안쪽에서 떼야 한 단계가 실행된다.
    assert.equal(slider.pointerDown(717, 343), true);
    assert.deepEqual(sounds, ['button']);
    assert.equal(slider.increment.pressed, 1);
    slider.pointerUp(717, 343);
    assert.deepEqual(steps, [SliderStep.DOWN]);
    // 위 화살표: 바깥에서 떼면 불발이다.
    slider.pointerDown(717, 63);
    slider.pointerMove(600, 63);
    assert.equal(slider.decrement.pressed, 0);
    slider.pointerUp(600, 63);
    assert.deepEqual(steps, [SliderStep.DOWN]);
    // 눌러 두면 0.4초(첫 지연) 전에는 반복하지 않고 그 뒤 0.05초 간격으로 반복한다.
    steps.length = 0;
    slider.pointerDown(717, 63);
    clock.now = 10.39;
    assert.equal(slider.update(), false);
    clock.now = 10.41;
    assert.equal(slider.update(), true);
    clock.now = 10.45;
    assert.equal(slider.update(), false);
    clock.now = 10.47;
    assert.equal(slider.update(), true);
    assert.deepEqual(steps, [SliderStep.UP, SliderStep.UP]);
    slider.cancel();
    // 홈: 손잡이 위쪽은 쪽 위, 아래쪽은 쪽 아래 요청이다(손잡이 bottom 238 보다 아래 y = 68 + 250).
    steps.length = 0;
    slider.pointerDown(717, 68 + 250);
    slider.pointerUp(717, 68 + 250);
    assert.deepEqual(steps, [SliderStep.PAGE_DOWN]);
    // 손잡이가 위에서 떨어져 있으면 손잡이 위쪽 홈은 쪽 위 요청이다.
    slider.setPage(100);
    slider.setPosition(100);
    steps.length = 0;
    slider.pointerDown(717, 68 + 2);
    slider.pointerUp(717, 68 + 2);
    assert.deepEqual(steps, [SliderStep.PAGE_UP]);
    // 손잡이를 끌면 위치가 (범위 × (이동량 + 손잡이 위쪽)) / (높이 - 폭 × 2) 의 절삭값이 되고 변화를 알린다: 327*(10+1)/271 = 13.27 → 13.
    slider.setRange(0, 327);
    slider.setPage(100);
    slider.setPosition(0);
    const handleTop = slider.handle.top;
    assert.equal(slider.pointerDown(717, 68 + handleTop + 5), true);
    assert.equal(slider.dragPreviousY, 68 + handleTop + 5);
    assert.equal(slider.pointerMove(717, 68 + handleTop + 15), true);
    assert.equal(slider.position, Math.trunc((327 * (10 + handleTop)) / 271));
    assert.equal(changes, 1);
    slider.pointerUp(717, 68 + handleTop + 15);
    assert.equal(slider.dragPreviousY, null);
    // 보이지 않으면 입력을 받지 않는다.
    slider.visible = false;
    assert.equal(slider.pointerDown(717, 68 + 250), false);
    // 부모가 단계 요청을 처리하지 않으면 자신의 위치를 한 칸 옮긴다.
    const alone = new VerticalSlider({ height: 100 });
    alone.attach(0, 0);
    alone.setRange(0, 20);
    alone.setPage(5);
    alone.setPosition(3);
    alone.pointerDown(4, 94);
    alone.pointerUp(4, 94);
    assert.equal(alone.position, 4);
});

// 스크롤 본문: 위치 제한·스크롤바 동기화·단계 요청·링크 판정·스크롤된 그리기. (2026-10-30 WB03-G)
test('스크롤 본문: setScroll 제한 · 스크롤바 동기화 · 단계 · hitTest · 스크롤된 그리기', () => {
    const context = createTextContext({ flags: 0, font: 5, color: colors.information, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 });
    const body = new ScrollingText(resources.fonts, context);
    const slider = new VerticalSlider({ onStep: (code) => body.handleStep(code), onChange: () => body.setScroll(slider.position) });
    body.attachSlider(slider);
    // 줄 40 개 중 3, 4번째 줄은 링크 태그 7 이다.
    const source = Array.from({ length: 40 }, (_, index) => (index === 3 || index === 4 ? `~[T7]link ${index}~[T0]` : `line ${index}`)).join('\n') + '\n';
    body.appendFragments(parseStyleText(source, context, colors));
    body.setSize(200, 100);
    body.x = 50;
    body.y = 20;
    body.placeSlider();
    body.setScroll(0);
    const total = body.totalHeight();
    assert.equal(body.softLines.length, 40);
    assert.equal(total, body.softLines.reduce((sum, line) => sum + line.height, 0));
    assert.ok(total > 300);
    // 폭은 스크롤바 폭 9 를 뺀 값으로 나뉜다. 위치는 (전체 - 보이는 높이) 이하·0 이상으로 맞춰진다.
    assert.equal(body.layoutWidth, 200);
    body.setScroll(10000);
    assert.equal(body.scroll, total - 100);
    assert.deepEqual([slider.maximum, slider.page, slider.position], [total, 100, total - 100]);
    body.setScroll(-30);
    assert.equal(body.scroll, 0);
    // 단계 요청: 1 아래 5, 2 위 5, 3 한 쪽 아래, 4 한 쪽 위, 그 밖은 처리하지 않는다.
    assert.equal(body.handleStep(1), true);
    assert.equal(body.scroll, 5);
    body.handleStep(1);
    body.handleStep(2);
    assert.equal(body.scroll, 5);
    body.handleStep(3);
    assert.equal(body.scroll, 105);
    body.handleStep(4);
    assert.equal(body.scroll, 5);
    assert.equal(body.handleStep(9), false);
    // 링크 판정: 스크롤 5 에서 3번째 줄(index 3)의 글자 위를 누르면 링크 태그 7 이 나오고, 줄 폭 밖·본문 밖은 null 이다.
    body.setScroll(0);
    const bounds = body.bounds();
    const line3 = body.softLines[3];
    const hit = body.hitTest(bounds.left + 3, bounds.top + line3.y + line3.height - 2);
    assert.equal(hit.tag, 7);
    assert.equal(body.hitTest(bounds.left + 3, bounds.top + body.softLines[1].y + 5).tag, 0);
    assert.equal(body.hitTest(bounds.left + 150, bounds.top + line3.y + 3), null);
    assert.equal(body.hitTest(bounds.left - 1, bounds.top + 3), null);
    body.setScroll(30);
    assert.equal(body.hitTest(bounds.left + 3, bounds.top + line3.y - 30 + line3.height - 2).tag, 7);
    // 스크롤하면 위쪽 줄은 가려지고 아래 줄이 올라온다: 스크롤 30(두 줄)에서 첫 줄 위치에 line 2 의 글자가 그려진다.
    const plain = new IndexedSurface(300, 200);
    const scrolled = new IndexedSurface(300, 200);
    body.setScroll(0);
    body.draw(plain, resources.fonts);
    body.setScroll(30);
    body.draw(scrolled, resources.fonts);
    let same = true;
    // 스크롤 0 의 15..(100) 픽셀 줄이 스크롤 30 의 맨 위 줄들과 같아야 한다(첫 줄이 정확히 두 줄 높이만큼 올라감).
    for (let y = 0; y < 70; y += 1) for (let x = 50; x < 250; x += 1) if (plain.pixels[(y + 20 + 30) * 300 + x] !== scrolled.pixels[(y + 20) * 300 + x]) same = false;
    assert.equal(same, true);
});

// 링크 표·그림 태그·조건 태그: 원본 HtmlLinkText·HtmlPictureText·HtmlConditionEffect 규칙. (2026-10-30 WB03-G)
test('HTML 링크·그림·조건 태그 규칙', () => {
    const links = new LinkTable();
    assert.equal(links.text('a href="#campaignHelp">', false), '~[T1]');
    assert.equal(links.text('a href="cmd:Tell,TechSupport">', false), '~[T2]');
    assert.equal(links.text('a href="multiQuickHelp">', false), '~[T3]');
    assert.equal(links.text('/a>', true), '~[T0]');
    // 따옴표가 태그 끝(`>`) 뒤에만 있으면 링크가 아니다(번호 0). 앞의 `#` 하나만 건너뛴다.
    assert.equal(links.text('a href=foo>x "y"', false), '~[T0]');
    assert.equal(links.text('a href="##x">', false), '~[T4]');
    assert.deepEqual([1, 2, 3, 4, 0, 5].map((number) => links.resolve(number)), ['campaignHelp', 'cmd:Tell,TechSupport', 'multiQuickHelp', '#x', null, null]);
    // 그림 태그는 첫 따옴표 뒤부터 다음 따옴표·`>` 앞까지를 `~[I…]` 로 옮긴다.
    assert.equal(htmlPictureText('!"fortgump.3">'), '~[Ifortgump.3]');
    assert.equal(htmlPictureText('!>'), '~[I]');
    // 조건: 키 값이 0 이 아니면 참이고 거짓이면 짝이 맞는 닫는 태그까지 숨긴다. `!` 는 부정, `??` 는 문자열 같음, g/ge/l/le 는 숫자 비교(오른쪽 < 왼쪽 순서에 주의).
    const values = { a: '1', b: '0', s: 'hello', n: '5', m: '7' };
    const substitute = (text) => text.replace(/\{(\w+)\}/g, (_, key) => values[key] ?? '');
    const run = (condition) => {
        const state = createParserState();
        state.shown = 1;
        applyHtmlCondition(state, `?${condition}>`, false, substitute);
        return state.shown === 1;
    };
    assert.deepEqual([run('{a}'), run('{b}'), run('!{a}'), run('!{b}')], [true, false, false, true]);
    assert.deepEqual([run('?{s}=hello'), run('?{s}=world'), run('!?{s}=world')], [true, false, true]);
    assert.deepEqual([run('g{m}=5'), run('g{n}=7'), run('ge{n}=5'), run('l{n}=7'), run('le{m}=6')], [true, false, true, true, false]);
    // 숨김 중첩: 거짓 조건 안의 조건은 평가하지 않고 깊이만 세며 바깥 닫는 태그에서 표시가 돌아온다.
    const state = createParserState();
    state.shown = 1;
    applyHtmlCondition(state, '?{b}>', false, substitute);
    applyHtmlCondition(state, '?{a}>', false, substitute);
    assert.deepEqual([state.shown, state.hiddenDepth], [0, 1]);
    // 닫는 태그의 원문도 `?` 부터 시작한다(파서가 `/` 를 건너뛰고 넘김).
    applyHtmlCondition(state, '?>', true, substitute);
    applyHtmlCondition(state, '?>', true, substitute);
    assert.deepEqual([state.shown, state.hiddenDepth], [1, 0]);
    // 파서 통과: 조건이 거짓이면 본문이 빠지고 참이면 들어간다.
    const render = (key) => parseHtml(`x<?{${key}}>hidden</?>y`, createParserState(), createParserGlobals(), { condition: (parserState, raw, closing) => applyHtmlCondition(parserState, raw, closing, substitute) });
    assert.equal(render('a').includes('hidden'), true);
    assert.equal(render('b').includes('hidden'), false);
});

// 주제 본문 변환: 앵커 접두 일치·표시 시작 `~5~w`·링크 12개·조건·종료 위치·없는 주제. (2026-10-30 WB03-G)
test('도움말 주제 본문: F1Help 구간 추출 · 링크 · 조건 · 없는 주제', () => {
    const data = newData();
    const helpText = data.readText(data.get('Quick Help Spec'));
    assert.equal(data.get('Quick Help Spec').toLowerCase(), '\\d\\help.english');
    assert.ok(helpText.length > 100000);
    const substitute = (text) => data.substitute(text);
    const result = renderHelpTopic({ helpText, topic: 'F1Help', substitute });
    assert.equal(result.found, true);
    assert.equal(result.styleText.startsWith('~5~w'), true);
    assert.ok(result.styleText.includes('NetStorm Instructions'));
    assert.ok(result.styleText.includes('Titanic were the creators of NetStorm'));
    // 다음 주제(interfaceHelp)의 본문은 섞이지 않는다.
    assert.equal(result.styleText.includes('User Interface'), true);
    assert.equal(result.styleText.includes('Left Click'), false);
    // 링크 12개(도움말 주제 8 + 지원 `cmd:` 1 + 바깥 주소 3).
    assert.equal(result.links.destinations.length, 12);
    assert.deepEqual(result.links.destinations.slice(0, 3), ['campaignHelp', 'interfaceHelp', 'sphereHelp']);
    assert.ok(result.links.destinations.includes('cmd:Tell,TechSupport'));
    assert.deepEqual(result.links.destinations.filter((destination) => destination.startsWith('http')), ['http://www.netstormhq.com', 'http://te.netstormhq.com', 'http://www.activision.com']);
    // 미션 안(`global.inMission`)에서만 F8 안내가 나온다.
    assert.equal(result.styleText.includes('Review Mission Objectives'), false);
    const inMission = renderHelpTopic({ helpText, topic: 'F1Help', substitute: (text) => data.substitute(text.replaceAll('{global.inMission}', '1')) });
    assert.equal(inMission.styleText.includes('Review Mission Objectives'), true);
    // 주제 이름은 앞부분 일치(대소문자 무시)이다.
    const interface1 = renderHelpTopic({ helpText, topic: 'INTERFACEhelp', substitute });
    assert.equal(interface1.styleText.includes('Left Click'), true);
    assert.equal(interface1.styleText.startsWith('~5~w'), true);
    // 없는 주제는 찾지 못했음을 알리고 본문이 비어 있다.
    const missing = renderHelpTopic({ helpText, topic: 'noSuchTopicAnywhere', substitute });
    assert.equal(missing.found, false);
    assert.equal(missing.links.destinations.length, 0);
    assert.equal(/[A-Za-z0-9]/.test(missing.styleText), false);
});

// 도움말 창 배치: 창 450×350 가운데·y 40, 본문 칸·스크롤바·버튼의 좌표(원본 스크린샷 정렬로 확인). 와이드 화면에서는 가운데로 다시 놓인다. (2026-10-30 WB03-G)
test('도움말 창 배치: 창 · 본문 칸 · 스크롤바 · 버튼 좌표', () => {
    const { window } = newHelp();
    assert.deepEqual(window.bounds(), { left: 287, top: 40, right: 737, bottom: 390 });
    assert.deepEqual([window.width, window.height], [450, 350]);
    assert.deepEqual(window.frame, { x: 304, y: 57, width: 420, height: 293 });
    assert.deepEqual(window.inner, { x: 306, y: 59, width: 416, height: 289 });
    assert.deepEqual(window.body.bounds(), { left: 306, top: 59, right: 722, bottom: 348 });
    assert.deepEqual(window.body.slider.thumbBounds(), { left: 713, top: 68, right: 722, bottom: 339 });
    assert.deepEqual(window.body.slider.decrement.bounds(), { left: 713, top: 59, right: 722, bottom: 68 });
    assert.deepEqual(window.body.slider.increment.bounds(), { left: 713, top: 339, right: 722, bottom: 348 });
    assert.deepEqual(window.body.slider.handle, { left: 1, top: 1, right: 8, bottom: 238 });
    assert.deepEqual([window.body.slider.maximum, window.body.slider.page, window.body.slider.position], [327, 289, 0]);
    // Back·OK 는 폭 38(OK 글자 폭 + 12 의 1.2배 → 정수)로 같고 간격 10, 아래에서 10 띄운다.
    assert.deepEqual(window.backButton.bounds(), { left: 464, top: 360, right: 502, bottom: 380 });
    assert.deepEqual(window.okButton.bounds(), { left: 512, top: 360, right: 550, bottom: 380 });
    const wide = newHelp({ width: 1366, height: 768 }).window;
    assert.equal(wide.bounds().left, 287 + (1366 - 1024) / 2);
    assert.equal(wide.bounds().top, 40);
    // 화면 크기가 바뀌어 다시 놓아도 스크롤 위치는 유지된다.
    window.body.setScroll(20);
    window.screen = { width: 1366, height: 768 };
    window.fit();
    window.place();
    assert.equal(window.bounds().left, 287 + 171);
    assert.equal(window.body.scroll, 20);
});

// 링크 중 destination 을 가진 첫 글자 조각의 화면 위치를 찾는다. 도움말 본문에서 링크를 누를 좌표를 만든다. (2026-10-30 WB03-G)
function findLink(window, destination) {
    const body = window.body;
    const bounds = body.bounds();
    for (const line of body.softLines) {
        let width = 0;
        for (const fragment of line.fragments) {
            const size = body.fonts.measure(fragment.bytes, fragment.context);
            if (fragment.tag && window.links.resolve(fragment.tag) === destination) {
                const x = bounds.left + width + 2;
                const y = bounds.top + line.y - body.scroll + line.height - 2;
                if (body.hitTest(x, y)?.tag === fragment.tag) return { x, y };
            }
            width += size.width;
        }
    }
    throw new Error(`링크를 찾지 못했습니다: ${destination}`);
}

// 도움말 창 입력: 링크 이동·방문 기록·Back·OK·창 밖 누름·`cmd:`/바깥 주소 링크·끌기·화살표·쪽·키·휠. (2026-10-30 WB03-G)
test('도움말 창 입력: 링크 · 방문 기록 · Back · OK · 바깥 누름 · 명령/바깥 주소', () => {
    const { window, events } = newHelp();
    assert.deepEqual(window.history, ['F1Help']);
    // 링크는 누르는 즉시 따라간다(효과음 없음). 주제가 바뀌고 방문 기록이 쌓이며 스크롤은 맨 위로 돌아간다.
    const campaign = findLink(window, 'campaignHelp');
    window.pointerDown(campaign.x, campaign.y);
    assert.equal(window.topic, 'campaignHelp');
    assert.deepEqual(window.history, ['F1Help', 'campaignHelp']);
    assert.equal(window.body.scroll, 0);
    assert.deepEqual(events.sounds, []);
    window.pointerUp(campaign.x, campaign.y);
    // 같은 주제 요청(대소문자만 다름)은 기록에 쌓지 않는다. Back 은 이전 주제로, 첫 주제의 Back 은 창을 닫는다.
    window.open('CAMPAIGNhelp');
    assert.equal(window.history.length, 2);
    const back = window.backButton.bounds();
    window.pointerDown(back.left + 5, back.top + 5);
    assert.deepEqual(events.sounds, ['button']);
    assert.equal(window.backButton.pressed, 1);
    window.pointerUp(back.left + 5, back.top + 5);
    assert.equal(window.topic, 'F1Help');
    assert.deepEqual(window.history, ['F1Help']);
    assert.equal(events.closed, 0);
    // `cmd:` 링크는 명령으로, 바깥 주소는 열지 않고 알림으로 넘긴다(창은 그대로 열려 있다).
    const support = findLink(window, 'cmd:Tell,TechSupport');
    window.pointerDown(support.x, support.y);
    assert.deepEqual(events.commands, [{ name: 'Tell', argument: 'TechSupport' }]);
    const site = findLink(window, 'http://www.netstormhq.com');
    window.pointerDown(site.x, site.y);
    assert.deepEqual(events.externals, ['http://www.netstormhq.com']);
    assert.equal(window.topic, 'F1Help');
    assert.equal(events.closed, 0);
    // 첫 주제의 Back 은 창을 닫는다. 닫힌 창은 입력을 받지 않는다.
    window.pointerDown(back.left + 5, back.top + 5);
    window.pointerUp(back.left + 5, back.top + 5);
    assert.equal(events.closed, 1);
    assert.equal(window.pointerDown(300, 100), false);
    // OK 는 안쪽에서 뗄 때만 닫는다(바깥에서 떼면 불발).
    const second = newHelp();
    const ok = second.window.okButton.bounds();
    second.window.pointerDown(ok.left + 5, ok.top + 5);
    second.window.pointerMove(5, 5);
    second.window.pointerUp(5, 5);
    assert.equal(second.events.closed, 0);
    second.window.pointerDown(ok.left + 5, ok.top + 5);
    second.window.pointerUp(ok.left + 5, ok.top + 5);
    assert.equal(second.events.closed, 1);
    // 창 밖을 누르면 바로 닫힌다. 창 안의 빈 가장자리 누름은 아무 일도 없다(모달이라 입력은 소비한다).
    const third = newHelp();
    assert.equal(third.window.pointerDown(286, 100), true);
    assert.equal(third.events.closed, 1);
    const fourth = newHelp();
    assert.equal(fourth.window.pointerDown(290, 100), true);
    assert.equal(fourth.events.closed, 0);
});

// 도움말 창 스크롤: 글자 위 끌기(반대 방향)·화살표(한 단계 5)·반복·쪽 이동·손잡이 끌기·키·휠(웹 적응)·한계. (2026-10-30 WB03-G)
test('도움말 창 스크롤: 끌기 · 화살표 · 반복 · 쪽 · 손잡이 · 키 · 휠', () => {
    const { window, clock, events } = newHelp();
    const body = window.body;
    const slider = body.slider;
    // 최대 스크롤은 전체 327 - 보이는 289 = 38 이다. 링크가 아닌 글자 밖 빈곳(오른쪽 빈 영역)을 눌러 끌면 위로 끄는 만큼 아래로 스크롤된다.
    assert.equal(body.scroll, 0);
    window.pointerDown(600, 120);
    window.pointerMove(600, 100);
    assert.equal(body.scroll, 20);
    window.pointerMove(600, 20);
    assert.equal(body.scroll, 38);
    window.pointerMove(600, 60);
    assert.equal(body.scroll, 0);
    window.pointerUp(600, 60);
    assert.equal(window.dragY, null);
    // 아래 화살표: 안쪽에서 떼면 5픽셀, 눌러 두면 0.4초 뒤부터 0.05초 간격으로 5픽셀씩 간다.
    const down = slider.increment.bounds();
    window.pointerDown(down.left + 3, down.top + 3);
    clock.now = 0;
    window.pointerUp(down.left + 3, down.top + 3);
    assert.equal(body.scroll, 5);
    clock.now = 100;
    window.pointerDown(down.left + 3, down.top + 3);
    clock.now = 100.41;
    assert.equal(window.update(), true);
    clock.now = 100.5;
    assert.equal(window.update(), true);
    assert.equal(body.scroll, 15);
    window.cancel();
    // 손잡이 아래 홈을 누르면 한 쪽(289) 아래로(한계 38), 위 홈은 한 쪽 위로 간다.
    const thumb = slider.thumbBounds();
    window.pointerDown(thumb.left + 3, thumb.bottom - 3);
    window.pointerUp(thumb.left + 3, thumb.bottom - 3);
    assert.equal(body.scroll, 38);
    // 손잡이는 홈의 위쪽 부분이므로 누르면 끌기가 시작된다: 위로 끌면 위치가 줄어든다.
    window.pointerDown(thumb.left + 3, thumb.top + slider.handle.top + 120);
    window.pointerMove(thumb.left + 3, thumb.top + slider.handle.top + 90);
    assert.ok(body.scroll < 38);
    window.pointerUp(thumb.left + 3, thumb.top + slider.handle.top + 90);
    // 키: 위·아래 화살표 10, PageUp·PageDown 은 본문 높이, 한계에서 멈춘다. 처리하지 않는 키는 false 이다.
    body.setScroll(0);
    assert.equal(window.keyDown('ArrowDown'), true);
    assert.equal(body.scroll, 10);
    window.keyDown('ArrowDown');
    window.keyDown('ArrowUp');
    assert.equal(body.scroll, 10);
    window.keyDown('PageDown');
    assert.equal(body.scroll, 38);
    window.keyDown('PageUp');
    assert.equal(body.scroll, 0);
    assert.equal(window.keyDown('KeyA'), false);
    // 휠(웹 적응): 아래로 굴리면 세 줄(45), 한계에서 멈춘다.
    assert.equal(window.wheel(100), true);
    assert.equal(body.scroll, 38);
    assert.equal(window.wheel(-100), true);
    assert.equal(body.scroll, 0);
    assert.equal(window.wheel(0), false);
    // 효과음은 화살표를 누른 두 번뿐이다(홈·손잡이·본문 누름은 소리가 없다).
    assert.deepEqual(events.sounds, ['button', 'button']);
});

// 메인 메뉴 연결: dialog 자리에 넣어 그리고 OK 로 닫는다. 화면 크기가 바뀌면 다시 놓인다. (2026-10-30 WB03-G)
test('메인 메뉴 연결: 대화상자 자리의 도움말 창 · 크기 변경 · 닫기', () => {
    const data = newData();
    const screen = new MainMenuScreen({ resources });
    screen.resize(1024, 768);
    const surface = new IndexedSurface(1024, 768);
    let help = null;
    const result = createHelpWindow({ gameData: data, resources, topic: 'F1Help', screen: { width: 1024, height: 768 }, onClose: () => { screen.setDialog(null); } });
    help = result.window;
    screen.setDialog(help);
    assert.equal(screen.dialog, help);
    screen.render(surface);
    // 도움말이 열려 있으면 메인 메뉴 버튼이 사라지고 입력을 도움말이 받는다(메뉴 버튼 위치를 눌러도 버튼은 눌리지 않는다).
    const campaign = screen.buttons[0].bounds();
    assert.equal(screen.pointerDown(campaign.left + 5, campaign.top + 5), true);
    assert.equal(screen.buttons[0].pressed, 0);
    screen.pointerUp(campaign.left + 5, campaign.top + 5);
    // 크기가 바뀌면 가운데로 다시 놓인다.
    screen.resize(1366, 768);
    assert.equal(help.bounds().left, 287 + 171);
    // OK 로 닫으면 메인 메뉴 버튼이 돌아온다.
    const ok = help.okButton.bounds();
    screen.pointerDown(ok.left + 5, ok.top + 5);
    screen.pointerUp(ok.left + 5, ok.top + 5);
    assert.equal(screen.dialog, null);
    assert.equal(help.closed, true);
});

// 원본 스크린샷 대조: 원점을 창 밖 타이틀 그림으로 정한 뒤 창 영역 평균 차이·본문 글자 마스크·화살표 삼각형을 비교하고 이웃 위치보다 훨씬 낫다는 것을 확인한다. (2026-10-30 WB03-G)
test('원본 스크린샷 대조: F1 도움말 창 · 본문 글자 · 스크롤바 · 버튼', (context) => {
    const width = 1024;
    const height = 768;
    const surface = new IndexedSurface(width, height);
    surface.tile(resources.clouds);
    const position = titlePosition(width, height, resources.title);
    surface.drawImage(resources.title, position.x, position.y);
    const bare = Uint8Array.from(surface.pixels);
    const { window } = newHelp({ width, height });
    window.draw(surface);
    const shot = decodePng(SCREENSHOT);
    const shotRgb = (x, y) => {
        const index = (y * shot.width + x) * 3;
        return [shot.rgb[index], shot.rgb[index + 1], shot.rgb[index + 2]];
    };
    const diff = (a, b) => (Math.abs(a[0] - b[0]) + Math.abs(a[1] - b[1]) + Math.abs(a[2] - b[2])) / 3;
    // 원점: 창과 겹치지 않는 아래쪽 타이틀 그림으로 정한다(다음 후보와 크게 차이 나야 한다).
    const candidates = [];
    for (let oy = 25; oy <= 40; oy += 1) for (let ox = 0; ox <= 12; ox += 1) {
        let sum = 0;
        let count = 0;
        for (let y = 420; y < 560; y += 2) for (let x = 250; x < 600; x += 2) {
            sum += diff(rgbAt(bare, width, x, y), shotRgb(x + ox, y + oy));
            count += 1;
        }
        candidates.push({ ox, oy, mean: sum / count });
    }
    candidates.sort((a, b) => a.mean - b.mean);
    const { ox, oy } = candidates[0];
    assert.deepEqual([ox, oy], [4, 34]);
    // 영역 평균 차이(이웃으로 어긋나면 크게 나빠진다).
    const region = (rect, dx = 0, dy = 0) => {
        let sum = 0;
        let count = 0;
        for (let y = rect.top; y < rect.bottom; y += 1) for (let x = rect.left; x < rect.right; x += 1) {
            sum += diff(rgbAt(surface.pixels, width, x, y), shotRgb(x + ox + dx, y + oy + dy));
            count += 1;
        }
        return sum / count;
    };
    const bounds = window.bounds();
    const body = window.body.bounds();
    const slider = window.body.slider.thumbBounds();
    const textRect = { left: body.left, top: body.top, right: body.right - 9, bottom: body.bottom };
    const sliderRect = { left: slider.left, top: body.top, right: slider.right, bottom: body.bottom };
    const windowMean = region(bounds);
    const bodyMean = region(textRect);
    const sliderMean = region(sliderRect);
    const buttonMean = region({ left: 460, top: 356, right: 556, bottom: 384 });
    context.diagnostic(`원점 (${ox}, ${oy}) 창 ${windowMean.toFixed(2)} 본문 ${bodyMean.toFixed(2)} 스크롤바 ${sliderMean.toFixed(2)} 버튼 ${buttonMean.toFixed(2)}`);
    assert.ok(windowMean < 1, `창 영역 평균 차이 ${windowMean}`);
    assert.ok(bodyMean < 1, `본문 평균 차이 ${bodyMean}`);
    assert.ok(sliderMean < 3, `스크롤바 평균 차이 ${sliderMean}`);
    assert.ok(buttonMean < 1, `버튼 평균 차이 ${buttonMean}`);
    for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        assert.ok(region(textRect, dx, dy) > bodyMean * 20 + 5, `본문 이웃 위치(${dx},${dy}) 차이가 충분히 크지 않습니다`);
        assert.ok(region(sliderRect, dx, dy) > sliderMean * 4, `스크롤바 이웃 위치(${dx},${dy}) 차이가 충분히 크지 않습니다`);
    }
    // 본문의 밝은 글자(제목·본문·링크) 마스크가 같다.
    let both = 0;
    let either = 0;
    for (let y = textRect.top; y < textRect.bottom; y += 1) for (let x = textRect.left; x < textRect.right; x += 1) {
        const mine = rgbAt(surface.pixels, width, x, y);
        const original = shotRgb(x + ox, y + oy);
        const a = mine[0] + mine[1] + mine[2] > 450;
        const b = original[0] + original[1] + original[2] > 450;
        if (a && b) both += 1;
        if (a || b) either += 1;
    }
    assert.ok(either > 5000 && both / either > 0.999, `밝은 글자 마스크 일치율 ${both}/${either}`);
    // 화살표 삼각형(검정 15픽셀)이 위·아래 모두 같다.
    for (const rect of [{ left: slider.left, top: slider.top - 9, right: slider.right, bottom: slider.top }, { left: slider.left, top: slider.bottom, right: slider.right, bottom: slider.bottom + 9 }]) {
        let same = 0;
        let union = 0;
        for (let y = rect.top; y < rect.bottom; y += 1) for (let x = rect.left; x < rect.right; x += 1) {
            const mine = rgbAt(surface.pixels, width, x, y);
            const original = shotRgb(x + ox, y + oy);
            const a = mine[0] + mine[1] + mine[2] < 60;
            const b = original[0] + original[1] + original[2] < 60;
            if (a && b) same += 1;
            if (a || b) union += 1;
        }
        assert.deepEqual([same, union], [15, 15]);
    }
    assert.equal(SLIDER_ARROW_COLOR, 0);
});

// 본문 그림: `~[Ifortgump.N]` 은 저장 에너지 아이콘 조각이 되어 글자 줄 안에 폭·높이만큼 자리를 차지하고 기준점을 기준선에 맞춰 그려진다.
// 타입 표에 없는 이름은 그리지 않고 알리며, 프레임 수 밖 번호는 크기 0 인 빈 그림이 된다. (2026-10-30 WB03-G, 2026-11-01 WB03-T: 전체 타입 표 해석으로 확장)
test('본문 그림: fortGump 아이콘 해석 · 줄 안 배치 · 그리기 · 지원하지 않는 그림', () => {
    // 해석기: fortGump 숫자 프레임(프레임 3 = 12×15, 기준점 (10,13))과 글자 프레임(L1 = 11×11)을 해석하고, 타입 이름이 없으면 null, 프레임 수 밖은 크기 0 이다.
    const icon = resources.resolvePicture('FortGump', '3');
    assert.deepEqual([icon.width, icon.height, icon.hotspotX, icon.hotspotY], [12, 15, 10, 13]);
    assert.deepEqual([icon.image.left, icon.image.top], [-10, -13]);
    assert.equal(resources.resolvePicture('nothing', '1'), null);
    assert.deepEqual([resources.resolvePicture('mana', '8').width, resources.resolvePicture('mana', '8').height], [0, 0]);
    assert.deepEqual([resources.resolvePicture('fortgump', 'L1').width, resources.resolvePicture('fortgump', 'L1').height], [11, 11]);
    assert.deepEqual([resources.resolvePicture('fortgump', '99999').width, resources.resolvePicture('fortgump', '99999').height], [0, 0]);
    // 스타일 문자열: 그림은 글자 조각 사이의 별도 조각이고 그 앞뒤 글자 문맥을 따른다. 지원하지 않는 그림은 알리고 조각을 만들지 않는다.
    const context = createTextContext({ flags: 0, font: 5, color: colors.information, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 });
    const unsupported = [];
    const fragments = parseStyleText('100~[Ifortgump.3] left ~[Inothing.1]!', context, colors, { resolvePicture: resources.resolvePicture, onUnsupported: (what) => unsupported.push(what) });
    assert.deepEqual(fragments.map((fragment) => (fragment.picture ? 'icon' : String.fromCharCode(...fragment.bytes))), ['100', 'icon', ' left ', '!']);
    assert.equal(unsupported.length, 1);
    // 줄 배치: 아이콘은 폭 12·높이 15 로 줄에 들어가고 줄 높이는 글자와 아이콘 높이의 최댓값이다. 폭이 모자라면 새 줄에서 시작한다.
    const body = new ScrollingText(resources.fonts, context);
    body.appendFragments(fragments.slice(0, 3));
    body.append(new TextEncoder().encode('\n'), context);
    body.setSize(200, 60);
    body.x = 50;
    body.y = 20;
    body.layout(200);
    const line = body.softLines[0];
    const textWidth = resources.fonts.measure(new TextEncoder().encode('100'), context).width;
    assert.deepEqual(line.fragments.map((fragment) => (fragment.picture ? 'icon' : 'text')), ['text', 'icon', 'text', 'text']);
    assert.equal(line.height >= 15, true);
    // 그리기: 아이콘의 불투명 픽셀이 (글자 폭, 줄 위쪽 + 최대 기준선 - 기준점 y) 에 그대로 찍힌다.
    const surface = new IndexedSurface(300, 100);
    body.draw(surface, resources.fonts);
    const ascent = Math.max(resources.fonts.metrics(context).ascent, icon.hotspotY);
    const left = 50 + textWidth + icon.hotspotX + icon.image.left;
    const top = 20 + ascent - icon.hotspotY + icon.hotspotY + icon.image.top;
    let opaque = 0;
    let wrong = 0;
    for (let y = 0; y < icon.image.height; y += 1) for (let x = 0; x < icon.image.width; x += 1) {
        if (!icon.image.opacity[y * icon.image.width + x]) continue;
        opaque += 1;
        if (surface.pixels[(top + y) * 300 + left + x] !== icon.image.indices[y * icon.image.width + x]) wrong += 1;
    }
    assert.ok(opaque > 60);
    assert.equal(wrong, 0);
    // 폭이 모자라면 그림은 새 줄에서 시작한다.
    const narrow = new ScrollingText(resources.fonts, context);
    narrow.appendFragments(fragments.slice(0, 2));
    narrow.append(new TextEncoder().encode('\n'), context);
    narrow.layout(textWidth + 8);
    assert.equal(narrow.softLines.length, 2);
    assert.equal(narrow.softLines[1].fragments[0].picture !== undefined, true);
});
