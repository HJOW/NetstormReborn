// Tell 대화상자(HTML 창) 복원을 검사한다: 본문 변환·지시문 추출·스타일 글자·줄 나눔·배치·입력·파일 목록과, 원본 스크린샷(Credits·Demo·Campaign)과의 위치·픽셀 대조.
// 스크린샷은 외부 캡처 도구가 만든 것이라 창 테두리가 있고 일부 영역에 캡처 노이즈가 있어(특히 패널 위쪽 질감), 글자·목록 영역의 위치 일치(정렬 탐색에서 한 칸만 어긋나도 점수가 크게 나빠짐)와
// 평균 채널 차이로 확인한다. 위치 기대값은 이 복원이 원본 스크린샷과 정렬됨을 확인한 뒤 고정한 값이다.
// (2026-10-28 WB03-D: Tell 대화상자 복원 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, decodePng, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { ALL_FONT_PATHS, FontSet, fontCachePath, isFontAvailable } from '../www/assets/font-set.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { createFragment, applyShortCommand, measureChain, parseStyleText } from '../www/ui/style-text.js';
import { ScrollingText } from '../www/ui/scroll-text.js';
import { createParserGlobals, createParserState, extractDirectives, parseHtml, prepareDialogContent } from '../www/ui/html-text.js';
import { expandFileList } from '../www/ui/tell-lists.js';
import { TellWindow, createTellWindow } from '../www/ui/tell-window.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { createTextContext } from '../www/render/glyph-text.js';
import { titlePosition } from '../www/ui/main-menu.js';

// 메뉴 묶음 파일 전부를 읽어 자원·게임 데이터를 만든다(검사마다 공유). (2026-10-28 WB03-D)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);
const colors = resources.colors;
const fonts = resources.fonts;

// 새 게임 데이터를 만든다(Tell 목록이 설정 객체를 등록하므로 검사마다 따로 만든다). (2026-10-28 WB03-D)
function newData() {
    return GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
}

// 기본 글자 문맥(슬롯 0, 흰색, 효과 없음)이다. (2026-10-28 WB03-D)
const BASE_CONTEXT = { flags: 0, font: 0, color: colors.white, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 };

// 바이트 배열을 문자열로 바꾼다(검사 출력용). (2026-10-28 WB03-D)
function text(bytes) {
    return String.fromCharCode(...bytes);
}

// 글꼴 파일 규칙: 파일 경로 표·가용 조합·준비 도구의 복사 목록이 같다. (2026-10-28 WB03-D)
test('글꼴 묶음: 슬롯·스타일 → 캐시 파일 경로와 가용 조합이 원본 규칙과 같다', () => {
    assert.equal(fontCachePath(0, 0), 'd/!Arial.normal.14.700.chfnt');
    assert.equal(fontCachePath(3, 0), 'd/!Arial.normal.20.700.chfnt');
    assert.equal(fontCachePath(5, 2), 'd/!Arial.bold.14.0.chfnt');
    assert.equal(fontCachePath(1, 0), 'd/!Courier New.normal.13.0.chfnt');
    assert.equal(fontCachePath(3, 2), null);
    assert.equal(fontCachePath(2, 0), null);
    assert.equal(isFontAvailable(6, 4), true);
    assert.equal(isFontAvailable(4, 1), false);
    const prepared = manifest.files.map((entry) => entry.path).filter((assetPath) => assetPath.endsWith('.chfnt')).sort();
    assert.deepEqual([...ALL_FONT_PATHS].sort(), prepared);
    // 측정: 슬롯 3(20px) 제목은 그림자(1,1) 때문에 높이 21, 폭은 측정 폭 합 + 1 이다.
    const bytes = new TextEncoder().encode('NetStorm Credits');
    const plain = fonts.measure(bytes, createTextContext({ font: 3 }));
    const shadow = fonts.measure(bytes, createTextContext({ flags: 2, font: 3, shadowX: 1, shadowY: 1 }));
    assert.equal(plain.height, 20);
    assert.deepEqual(shadow, { width: plain.width + 1, height: 21 });
});

// HTML 변환: 태그 → StyleText 제어문자, 소스 줄바꿈 처리, 단락 억제, 배경 이름. (2026-10-28 WB03-D)
test('HTML 본문 변환: 제목·단락·줄바꿈·배경 태그', () => {
    const globals = createParserGlobals();
    const run = (source) => parseHtml(source, createParserState(), globals);
    assert.equal(run('<h2>Title</h2>'), '~N~3~E' + 'Title' + '~*~N~.\n' + '\n');
    assert.equal(run('<h1>A</h1><h3>B</h3><h4>C</h4>'), '~N~4~EA~*~N~.\n~N~5~B~EB~*~N~.\n~N~1~EC~*~N~.\n\n');
    // 제목 바로 뒤 단락은 빈 줄을 줄이고, 본문 뒤 단락은 빈 줄을 하나 더 만든다.
    assert.equal(run('<h2>T</h2><p>x<p>y'), '~N~3~ET~*~N~.\n~*\nx~*\n \ny\n');
    // 소스의 줄바꿈은 앞 글자가 공백·LF 가 아니면 공백 하나가 되고 CR 은 버려진다. <br> 는 LF 이다.
    assert.equal(run('a\r\nb\r\n\r\nc<br>d'), 'a b c\nd\n');
    assert.equal(run('<b>x</b><i>y</i><c>z</c>'), '~N~Bx~N~N~jy~.~N~iz~.\n');
    assert.equal(run('<a href="x">l</a>'), '~l~El~N~.\n');
    // 배경 이름은 따옴표 안 문자열이며 전역에 남는다.
    run('<background="nsstart">hi');
    assert.equal(globals.background, 'nsstart');
    assert.equal(run('<background=nsisles>z'), 'z\n');
    assert.equal(globals.background, 'nsisles');
});

// 지시문 추출: 우선순위·제거 범위·기본 확인 버튼·Timeout. (2026-10-28 WB03-D)
test('지시문 추출: 우선순위·제거 범위·OkButton·Timeout', () => {
    const source = 'abc $Menu=Item One,Tell,One $Button=Go,Tell,Next $ButtonInstant=Now,Tell,Fast $Timeout=20,Tell,Blank $OnExit=x,Tell,Bye tail';
    const found = extractDirectives(source);
    assert.deepEqual(found.buttons.map((button) => [button.label, button.command.name, button.command.argument, button.instant]), [['Go', 'Tell', 'Next', false], ['Now', 'Tell', 'Fast', true]]);
    assert.deepEqual(found.menuItems.map((item) => [item.kind, item.label, item.command.argument]), [['text', 'Item One', 'One']]);
    assert.deepEqual(found.timeout, { seconds: 20, command: { name: 'Tell', argument: 'Blank' } });
    assert.deepEqual(found.onExit, { name: 'Tell', argument: 'Bye' });
    // `$` 로 시작하는 일치는 다음 `$` 앞까지 지워 일반 글자만 남는다.
    assert.equal(found.text, 'abc ');
    assert.equal(found.handledAny, true);
    // 버튼·메뉴가 없으면 기본 확인 버튼(OkButton)이 들어간다.
    const content = prepareDialogContent('<h2>Hi</h2>text', { okButton: '$Button=OK,DoNothing,0' });
    assert.deepEqual(content.directives.buttons.map((button) => [button.label, button.command.name]), [['OK', 'DoNothing']]);
    // 영숫자가 없는 본문은 빈 본문이다.
    assert.equal(prepareDialogContent('<p> \r\n').empty, true);
    // 두 번째 변환 결과 끝에는 LF 가 연속으로 붙는다(변환 끝 LF + 공장 끝 LF).
    assert.ok(content.styleText.endsWith('\n\n\n'));
});

// 스타일 문자열: 명령 처리·문맥·조각 분할·모르는 명령·대괄호 명령. (2026-10-28 WB03-D)
test('StyleText 해석: 색·슬롯·그림자·기본 복귀와 대괄호 명령', () => {
    const context = createTextContext(BASE_CONTEXT);
    const parts = parseStyleText('a~wb~3~Ec~.d~~e~ZQ~[C.9]f~[T7]g', context, colors);
    assert.deepEqual(parts.map((part) => text(part.bytes)), ['a', 'b', 'c', 'd', '~', 'e', 'ZQ', 'f', 'g']);
    const find = (label) => parts.find((part) => text(part.bytes) === label);
    assert.equal(find('b').context.color, colors.white);
    assert.equal(find('c').context.font, 3);
    assert.equal(find('c').context.flags, 2);
    assert.equal(find('c').context.shadowColor, colors.black);
    assert.deepEqual([find('d').context.font, find('d').context.flags, find('d').context.color], [0, 0, colors.white]);
    // ~[C.9] 는 색 설정이 꺼져 있어 소비만 하고, ~[T7] 은 이후 조각에 링크 태그를 남긴다.
    assert.equal(find('f').context.color, colors.white);
    assert.deepEqual([find('f').tag, find('g').tag], [0, 7]);
    // 끝이 `~` 이면 마지막에 빈 조각이 붙는다. 대괄호가 닫히지 않으면 `~[` 만 소비한다.
    assert.equal(text(parseStyleText('x~', context, colors).at(-1).bytes), '');
    // 단일 명령 표.
    const mutable = { ...BASE_CONTEXT };
    assert.equal(applyShortCommand('B', mutable, BASE_CONTEXT, colors), true);
    assert.equal(mutable.style, 2);
    assert.equal(applyShortCommand('N', mutable, BASE_CONTEXT, colors), true);
    assert.equal(mutable.style, 0);
    assert.equal(applyShortCommand('Z', mutable, BASE_CONTEXT, colors), false);
    assert.equal(applyShortCommand('4', mutable, BASE_CONTEXT, colors, true), true);
    assert.equal(mutable.font, 0);
    assert.equal(applyShortCommand('5', mutable, BASE_CONTEXT, colors, true), true);
    assert.equal(mutable.font, 5);
    // 체인 측정: 폭은 합, 높이는 가장 큰 조각(제목 슬롯 3 + 그림자 = 21).
    const chain = parseStyleText('~3~Ea~0b', context, colors);
    assert.equal(measureChain(fonts, chain).height, 21);
});

// 줄 나눔: 단어 경계·긴 단어·빈 줄 높이·FitContent/FitHeight 규칙. (2026-10-28 WB03-D)
test('다중 줄 본문: 줄 나눔과 크기 맞춤', () => {
    const body = new ScrollingText(fonts, createTextContext(BASE_CONTEXT));
    const bytes = new TextEncoder().encode('Please choose which demo you wish to view:\n\n');
    body.append(bytes, createTextContext(BASE_CONTEXT));
    body.fitContent(0, 0);
    // 폭을 아주 넓게 잡으면 한 줄이고, 최대 폭 + 1 이 폭이 된다. 빈 줄도 글꼴 높이(14)를 가진다.
    const natural = fonts.measure(bytes.subarray(0, bytes.length - 2), createTextContext(BASE_CONTEXT)).width;
    assert.equal(body.baseWidth, natural + 1);
    assert.equal(body.softLines.length, 2);
    assert.equal(body.baseHeight, 14 + 14);
    // 좁은 폭에서는 단어 단위로 접힌다. 줄의 폭은 항상 설정 폭보다 작다(긴 단어 단독 줄 제외).
    body.setWidth(129);
    body.fitHeight();
    assert.deepEqual(body.softLines.map((line) => text(line.fragments.flatMap((fragment) => [...fragment.bytes]))), ['Please choose which ', 'demo you wish to ', 'view:', '']);
    assert.ok(body.softLines.slice(0, 3).every((line) => line.width < 129));
    assert.equal(body.baseHeight, 14 * 4 + 1);
    // 아주 좁으면 단어마다 한 줄이 된다. 마지막 단어가 폭을 넘으면 높이 0 의 빈 표시 줄이 하나 더 생기는 것도 원본 줄 나눔(SplitText 끝의 줄 바꿈)과 같다.
    body.setWidth(10);
    body.fitHeight();
    assert.equal(body.softLines.length, 10);
    assert.equal(body.softLines[8].height, 0);
    assert.equal(body.baseHeight, 14 * 9 + 1);
    // 줄은 폭보다 '엄격히 좁아야' 한다: 글자 폭이 정확히 폭과 같으면 접히고, 1 크면 한 줄에 들어간다(원본 비교 `>=`).
    const exact = new ScrollingText(fonts, createTextContext(BASE_CONTEXT));
    const phrase = new TextEncoder().encode('aaaa bbbb');
    exact.append(new TextEncoder().encode('aaaa bbbb\n'), createTextContext(BASE_CONTEXT));
    const phraseWidth = fonts.measure(phrase, createTextContext(BASE_CONTEXT)).width;
    exact.layout(phraseWidth);
    assert.equal(exact.softLines.length, 2);
    exact.layout(phraseWidth + 1);
    assert.equal(exact.softLines.length, 1);
    // 스크롤 줄 수 한도: 128번째 줄이 들어오면 맨 앞 완료 줄을 버린다.
    const many = new ScrollingText(fonts, createTextContext(BASE_CONTEXT));
    for (let index = 0; index < 200; index += 1) many.append(new TextEncoder().encode(`line ${index}\n`), createTextContext(BASE_CONTEXT));
    assert.equal(many.hardLines.length, 127);
    assert.equal(text(many.hardLines[0][0].bytes), 'line 73');
});

// 화면에 대화상자를 그려 8비트 표면과 창을 돌려준다(원본 구름·타이틀 위, 메인 메뉴 버튼은 숨김). (2026-10-28 WB03-D)
function renderDialog(name, width = 1024, height = 768) {
    const data = newData();
    const surface = new IndexedSurface(width, height);
    surface.tile(resources.clouds);
    const position = titlePosition(width, height, resources.title);
    surface.drawImage(resources.title, position.x, position.y);
    const result = createTellWindow({ gameData: data, resources, name, screen: { width, height } });
    assert.ok(result.window, `${name} 대화상자를 만들지 못했습니다`);
    result.window.draw(surface);
    const rgb = new Uint8Array(width * height * 3);
    for (let index = 0; index < width * height; index += 1) rgb.set(resources.palette.rgb.subarray(surface.pixels[index] * 3, surface.pixels[index] * 3 + 3), index * 3);
    return { data, surface, window: result.window, rgb, width, height };
}

// 대화상자 배치: Credits·Demo·Campaign 의 패널·본문·목록·버튼 좌표와 크기(원본 스크린샷 정렬로 확인한 값). (2026-10-28 WB03-D)
test('대화상자 배치: Credits·Demo·Campaign 크기와 좌표', () => {
    const credits = renderDialog('Credits').window;
    assert.deepEqual(credits.bounds(), { left: 395, top: 304, right: 629, bottom: 463 });
    assert.deepEqual([credits.baseWidth, credits.baseHeight], [234, 159]);
    assert.deepEqual(credits.body.bounds(), { left: 447, top: 324, right: 576, bottom: 374 });
    assert.deepEqual(credits.menu.bounds(), { left: 415, top: 384, right: 609, bottom: 418 });
    assert.deepEqual(credits.buttons[0].bounds(), { left: 487, top: 428, right: 536, bottom: 448 });
    // 문서의 Credits 대화상자 범위 (397,306)~(631,465) = 234x159 와 같은 크기이다.
    const demo = renderDialog('Demoville').window;
    assert.deepEqual([demo.baseWidth, demo.baseHeight], [249, 232]);
    assert.deepEqual([demo.body.baseWidth, demo.body.baseHeight], [149, 106]);
    const campaign = renderDialog('UCampaign').window;
    assert.deepEqual([campaign.baseWidth, campaign.baseHeight], [238, 301]);
    assert.equal(campaign.menu.items.length, 8);
    assert.deepEqual(campaign.menu.items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text', 'separator', 'text']);
    assert.deepEqual(campaign.menu.items.filter((item) => item.kind === 'text').map((item) => item.label), ['Early Missions', 'Priest Training', 'Struggle For Freedom', 'A Nation Rises', 'Complete Victory', 'User Made Campaigns']);
    assert.deepEqual(campaign.menu.items[0].command, { name: 'Tell', argument: 'offical1.overview' });
    // 다른 화면 크기에서는 가운데로 다시 놓인다(논리 높이 768, 16:9 폭 1366).
    const wide = renderDialog('Credits', 1366, 768).window;
    assert.equal(wide.bounds().left, 395 + (1366 - 1024) / 2);
    assert.equal(wide.bounds().top, 304);
});

// 파일 목록: 설정 객체 등록·이름순·구분선·`{cur.file}` 치환과 등록된 섹션 조회. (2026-10-28 WB03-D)
test('Tell 파일 목록: offical*.english 8개 → 행 6개 + 구분선 2개', () => {
    const data = newData();
    const items = expandFileList(data, '@' + data.get('guideSpec'), 'Tell', '{cur.file}.overview');
    assert.deepEqual(items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text', 'separator', 'text']);
    assert.deepEqual(items.filter((item) => item.kind === 'text').map((item) => item.command.argument), ['offical1.overview', 'offical2.overview', 'offical3.overview', 'offical4.overview', 'offical5.overview', 'offical6.overview']);
    // 등록된 객체의 섹션을 Tell 이 찾을 수 있다. 임시 cur 객체는 내려져 있다.
    assert.ok(data.tellSection('offical1.overview').includes('Early Missions'));
    assert.equal(data.registry.findByName('cur'), null);
    assert.equal(data.get('offical3.index'), '3');
    // 다시 펼쳐도 객체를 중복 등록하지 않는다.
    const count = data.registry.objects.length;
    expandFileList(data, '@' + data.get('guideSpec'), 'Tell', '{cur.file}.overview');
    assert.equal(data.registry.objects.length, count);
});

// 입력: 목록 강조·행 누름 즉시 실행·돌 버튼 누름/뗌·Timeout·닫기. (2026-10-28 WB03-D)
test('대화상자 입력: 목록 행 · 돌 버튼 · Timeout · 닫기', () => {
    const data = newData();
    const commands = [];
    const sounds = [];
    const { window } = createTellWindow({ gameData: data, resources, name: 'Credits', screen: { width: 1024, height: 768 }, nowSeconds: 100, onCommand: (command) => commands.push(command), onSound: (kind) => sounds.push(kind) });
    const second = window.menu.bounds();
    // 포인터를 올리면 그 행이 강조되고, 밖으로 나가면 해제된다.
    assert.equal(window.pointerMove(420, second.top + 3), true);
    assert.equal(window.menu.selection, 0);
    assert.equal(window.pointerMove(420, second.top + 20), true);
    assert.equal(window.menu.selection, 1);
    window.pointerMove(5, 5);
    assert.equal(window.menu.selection, -1);
    // 행을 누르는 즉시 명령이 실행되고 목록 효과음이 난다.
    window.pointerDown(420, second.top + 3);
    assert.deepEqual(commands.pop(), { name: 'Tell', argument: 'CreditsNew' });
    assert.deepEqual(sounds, ['menu']);
    // 돌 버튼: 누름은 효과음 + 눌린 모양, 안쪽에서 뗄 때만 실행한다. 바깥에서 떼면 불발이다.
    const cancel = window.buttons[0].bounds();
    window.pointerDown(cancel.left + 3, cancel.top + 3);
    assert.equal(window.buttons[0].pressed, 1);
    window.pointerMove(5, 5);
    assert.equal(window.buttons[0].pressed, 0);
    window.pointerUp(5, 5);
    assert.equal(commands.length, 0);
    window.pointerDown(cancel.left + 3, cancel.top + 3);
    window.pointerUp(cancel.left + 3, cancel.top + 3);
    assert.deepEqual(commands.pop(), { name: 'Tell', argument: 'Blank', instant: false });
    // Credits 에는 Timeout 이 없다. Demo 에는 120초 Timeout 이 있다.
    assert.equal(window.update(10000), false);
    const demo = createTellWindow({ gameData: data, resources, name: 'Demoville', screen: { width: 1024, height: 768 }, nowSeconds: 50, onCommand: (command) => commands.push(command) }).window;
    assert.equal(demo.update(169.9), false);
    assert.equal(demo.update(170), true);
    assert.deepEqual(commands.pop(), { name: 'Tell', argument: 'Blank' });
    assert.equal(demo.update(500), false);
    // 닫힌 창은 입력을 받지 않는다.
    assert.equal(window.close(), null);
    assert.equal(window.pointerDown(420, second.top + 3), false);
    // 없는 섹션은 만들지 않는다.
    assert.equal(createTellWindow({ gameData: data, resources, name: 'Blank', screen: { width: 1024, height: 768 } }).missing, true);
});

// 원본 스크린샷과의 위치·픽셀 대조에 쓰는 정보이다. 스크린샷은 창 테두리를 포함하며 클라이언트 원점이 그림마다 조금 다르다. (2026-10-28 WB03-D)
const SHOTS = Object.freeze({
    Credits: { file: 'mainMenu - Credits.png', menu: [415, 384, 609, 418], button: [487, 428, 536, 448], title: [447, 324, 576, 374] },
    Demoville: { file: 'mainMenu - Demo.png', menu: [415, 404, 608, 455], button: [487, 465, 536, 485], title: [437, 288, 586, 394] },
    UCampaign: { file: 'mainMenu - Campaign.png', menu: [435, 355, 590, 489], button: [493, 499, 531, 519], title: [443, 253, 581, 345] },
});

// 두 이미지의 지정 영역 평균 채널 차이(채널당)이다. oX/oY 는 스크린샷 클라이언트 원점이다. (2026-10-28 WB03-D)
function regionDiff(rendered, shot, rect, originX, originY) {
    const [left, top, right, bottom] = rect;
    let sum = 0;
    let count = 0;
    // 영역의 모든 픽셀을 비교한다.
    for (let y = top; y < bottom; y += 1) {
        for (let x = left; x < right; x += 1) {
            const s = ((y + originY) * shot.width + x + originX) * 3;
            const d = (y * rendered.width + x) * 3;
            sum += Math.abs(shot.rgb[s] - rendered.rgb[d]) + Math.abs(shot.rgb[s + 1] - rendered.rgb[d + 1]) + Math.abs(shot.rgb[s + 2] - rendered.rgb[d + 2]);
            count += 1;
        }
    }
    return sum / count / 3;
}

// 원본 스크린샷 대조: 목록·본문 영역이 한 픽셀도 어긋나지 않게 정렬되고(최적 원점이 이웃 원점보다 5배 이상 좋음), 목록·버튼 영역 평균 차이가 작다.
test('원본 스크린샷 대조: Credits·Demo·Campaign 목록·본문·버튼 위치 일치', (t) => {
    // 비교 대상 스크린샷과 영역별 허용 평균 차이(채널당)이다. 팔레트 표시 오차(약 ±3)와 캡처 노이즈를 감안한 값이다.
    const limits = { Credits: { menu: 2.5, button: 2.5 }, Demoville: { menu: 3, button: 3 }, UCampaign: { menu: 4.5, button: 8 } };
    // 스크린샷마다 정렬·차이를 확인한다.
    for (const [name, info] of Object.entries(SHOTS)) {
        const shotPath = path.join(REPO_ROOT, 'screenShots', info.file);
        if (!fs.existsSync(shotPath)) {
            t.diagnostic(`${info.file} 가 없는 환경이라 건너뜀`);
            continue;
        }
        const shot = decodePng(shotPath);
        const rendered = renderDialog(name);
        const scores = [];
        // 클라이언트 원점 후보를 훑어 본문·목록 영역 차이가 가장 작은 원점을 찾는다.
        for (let originY = 28; originY <= 40; originY += 1) {
            for (let originX = 0; originX <= 10; originX += 1) scores.push({ originX, originY, score: regionDiff(rendered, shot, info.menu, originX, originY) });
        }
        scores.sort((a, b) => a.score - b.score);
        const best = scores[0];
        // 이웃 원점(한 칸 어긋남)은 모두 훨씬 나쁘다. 정렬이 유일해야 위치가 일치한다고 할 수 있다.
        const neighbors = scores.filter((entry) => Math.abs(entry.originX - best.originX) + Math.abs(entry.originY - best.originY) === 1);
        assert.ok(neighbors.every((entry) => entry.score > best.score * 5), `${name}: 정렬이 유일하지 않습니다`);
        assert.ok(best.score <= limits[name].menu, `${name} 목록 평균 차이 ${best.score.toFixed(2)}`);
        assert.ok(regionDiff(rendered, shot, info.button, best.originX, best.originY) <= limits[name].button, `${name} 버튼 평균 차이`);
        assert.ok(regionDiff(rendered, shot, info.title, best.originX, best.originY) <= 6, `${name} 본문 평균 차이`);
    }
});
