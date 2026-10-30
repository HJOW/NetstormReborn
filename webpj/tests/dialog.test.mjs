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
import { WEB_EXCLUDED_LIST_OBJECTS, expandFileList } from '../www/ui/tell-lists.js';
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
// original 이 true 면 웹 제외 목록 객체(User Made Campaigns)를 빼지 않고 원본과 같은 목록으로 만든다(원본 스크린샷 대조용). (2026-10-30 WB03-I)
function renderDialog(name, width = 1024, height = 768, { original = false } = {}) {
    const data = newData();
    const surface = new IndexedSurface(width, height);
    surface.tile(resources.clouds);
    const position = titlePosition(width, height, resources.title);
    surface.drawImage(resources.title, position.x, position.y);
    const hooks = original ? { expandMenu: (label, command, argument) => expandFileList(data, label, command, argument, { excluded: [] }) } : {};
    const result = createTellWindow({ gameData: data, resources, name, screen: { width, height }, hooks });
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
    // 원본과 같은 8행(User Made Campaigns 포함, 원본 스크린샷 mainMenu - Campaign.png 와 같은 배치)이다.
    const campaign = renderDialog('UCampaign', 1024, 768, { original: true }).window;
    assert.deepEqual([campaign.baseWidth, campaign.baseHeight], [238, 301]);
    assert.equal(campaign.menu.items.length, 8);
    assert.deepEqual(campaign.menu.items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text', 'separator', 'text']);
    assert.deepEqual(campaign.menu.items.filter((item) => item.kind === 'text').map((item) => item.label), ['Early Missions', 'Priest Training', 'Struggle For Freedom', 'A Nation Rises', 'Complete Victory', 'User Made Campaigns']);
    assert.deepEqual(campaign.menu.items[0].command, { name: 'Tell', argument: 'offical1.overview' });
    // 웹 판본(기본): User Made Campaigns 행과 그 앞 구분선을 뺀 6행이다. 패널은 구분선 16 + 행 17 = 33 만큼 낮아져 화면 가운데에 다시 놓인다.
    const web = renderDialog('UCampaign').window;
    assert.deepEqual([web.baseWidth, web.baseHeight], [238, 268]);
    assert.equal(web.menu.items.length, 6);
    assert.deepEqual(web.menu.items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text']);
    assert.deepEqual(web.menu.items.filter((item) => item.kind === 'text').map((item) => item.label), ['Early Missions', 'Priest Training', 'Struggle For Freedom', 'A Nation Rises', 'Complete Victory']);
    assert.deepEqual(web.bounds(), { left: 393, top: 250, right: 631, bottom: 518 });
    assert.equal(web.bounds().top, Math.floor((768 - 268) / 2));
    assert.deepEqual(web.menu.bounds(), { left: 436, top: 372, right: 589, bottom: 473 });
    assert.deepEqual(web.body.bounds(), { left: 443, top: 270, right: 581, bottom: 362 });
    assert.deepEqual(web.buttons[0].bounds(), { left: 493, top: 483, right: 531, bottom: 503 });
    // 본문→목록→버튼 사이 간격은 원본과 같다(제외한 두 행만큼 목록이 짧아졌을 뿐이다).
    assert.equal(web.menu.bounds().top - web.body.bounds().bottom, campaign.menu.bounds().top - campaign.body.bounds().bottom);
    assert.equal(web.buttons[0].bounds().top - web.menu.bounds().bottom, campaign.buttons[0].bounds().top - campaign.menu.bounds().bottom);
    assert.equal(web.menu.bounds().bottom - web.menu.bounds().top, campaign.menu.bounds().bottom - campaign.menu.bounds().top - 33);
    // 다른 화면 크기에서는 가운데로 다시 놓인다(논리 높이 768, 16:9 폭 1366).
    const wide = renderDialog('Credits', 1366, 768).window;
    assert.equal(wide.bounds().left, 395 + (1366 - 1024) / 2);
    assert.equal(wide.bounds().top, 304);
});

// 파일 목록: 설정 객체 등록·이름순·구분선·`{cur.file}` 치환과 등록된 섹션 조회. 원본(제외 없음)은 offical*.english 8개 → 행 6개 + 구분선 2개이다. (2026-10-28 WB03-D)
test('Tell 파일 목록(제외 없음): offical*.english 8개 → 행 6개 + 구분선 2개', () => {
    const data = newData();
    const items = expandFileList(data, '@' + data.get('guideSpec'), 'Tell', '{cur.file}.overview', { excluded: [] });
    assert.deepEqual(items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text', 'separator', 'text']);
    assert.deepEqual(items.filter((item) => item.kind === 'text').map((item) => item.command.argument), ['offical1.overview', 'offical2.overview', 'offical3.overview', 'offical4.overview', 'offical5.overview', 'offical6.overview']);
    // 등록된 객체의 섹션을 Tell 이 찾을 수 있다. 임시 cur 객체는 내려져 있다.
    assert.ok(data.tellSection('offical1.overview').includes('Early Missions'));
    assert.ok(data.tellSection('offical6.overview').includes('userSpec'));
    assert.equal(data.registry.findByName('cur'), null);
    assert.equal(data.get('offical3.index'), '3');
    // 다시 펼쳐도 객체를 중복 등록하지 않는다.
    const count = data.registry.objects.length;
    expandFileList(data, '@' + data.get('guideSpec'), 'Tell', '{cur.file}.overview', { excluded: [] });
    assert.equal(data.registry.objects.length, count);
});

// 웹 제외 목록(기본값): offical6(User Made Campaigns)과 그 앞 구분선(offical5sep)을 뺀다. 뺀 객체는 설정 객체로도 등록되지 않아 섹션이 열리지 않는다.
// GameData.findFiles 는 여전히 8개 파일을 센다(제외는 목록 층에서만 한다). (2026-10-30 WB03-I)
test('Tell 파일 목록(웹 기본): User Made Campaigns 와 그 앞 구분선을 뺀다', () => {
    const data = newData();
    const label = '@' + data.get('guideSpec');
    assert.deepEqual([...WEB_EXCLUDED_LIST_OBJECTS], ['offical6']);
    assert.equal(data.findFiles(data.get('guideSpec')).length, 8);
    const items = expandFileList(data, label, 'Tell', '{cur.file}.overview');
    assert.deepEqual(items.map((item) => item.kind), ['text', 'text', 'separator', 'text', 'text', 'text']);
    assert.deepEqual(items.filter((item) => item.kind === 'text').map((item) => item.command.argument), ['offical1.overview', 'offical2.overview', 'offical3.overview', 'offical4.overview', 'offical5.overview']);
    assert.ok(!items.some((item) => item.label === 'User Made Campaigns'));
    // 뺀 객체는 등록되지 않고 Tell 섹션도 없다. 남은 객체의 index 는 뺀 객체까지 센 원래 순번 그대로다. 구분선 객체(offical5sep)는 목록 정리 때문에 등록만 되어 있다.
    assert.equal(data.registry.findByName('offical6'), null);
    assert.equal(data.tellSection('offical6.overview'), null);
    assert.ok(data.get('offical6.title') == null);
    assert.ok(data.tellSection('offical5.overview') !== null);
    assert.equal(data.get('offical3.index'), '3');
    assert.equal(data.get('offical5.index'), '5');
    // 대소문자를 가리지 않고 제외한다.
    const upper = expandFileList(newData(), label, 'Tell', '{cur.file}.overview', { excluded: ['OFFICAL6'] });
    assert.equal(upper.length, 6);
    // 앞쪽 객체를 빼도 남은 객체의 index 는 뺀 객체까지 센 원래 순번이다(offical3 는 계속 3).
    const early = newData();
    expandFileList(early, label, 'Tell', '{cur.file}.overview', { excluded: ['offical1'] });
    assert.equal(early.registry.findByName('offical1'), null);
    assert.equal(early.get('offical3.index'), '3');
});

// 객체를 빼서 구분선만 남는 자리 정리: 맨 앞, 맨 뒤, 연달아 놓인 구분선을 지운다(하나도 안 뺀 목록은 원본 그대로 둔다). (2026-10-30 WB03-I)
test('Tell 파일 목록: 뺀 뒤 앞·뒤·연속 구분선을 정리한다', () => {
    const kinds = (excluded) => {
        const data = newData();
        return expandFileList(data, '@' + data.get('guideSpec'), 'Tell', '{cur.file}.overview', { excluded }).map((item) => (item.kind === 'separator' ? '-' : item.command.argument.replace('offical', '').replace('.overview', '')));
    };
    assert.deepEqual(kinds([]), ['1', '2', '-', '3', '4', '5', '-', '6']);
    // 맨 뒤: 5 와 6 을 빼면 마지막 구분선이 남지 않는다.
    assert.deepEqual(kinds(['offical5', 'offical6']), ['1', '2', '-', '3', '4']);
    // 맨 앞: 1 과 2 를 빼면 첫 구분선이 남지 않는다.
    assert.deepEqual(kinds(['offical1', 'offical2']), ['3', '4', '5', '-', '6']);
    // 연속: 3, 4, 5 를 빼면 구분선 둘이 이어지므로 하나만 남는다.
    assert.deepEqual(kinds(['offical3', 'offical4', 'offical5']), ['1', '2', '-', '6']);
    // 전부 빼면 빈 목록이다.
    assert.deepEqual(kinds(['offical1', 'offical2', 'offical3', 'offical4', 'offical5', 'offical6']), []);
    // 아무것도 빼지 않은 목록은 원본 그대로 둔다: 구분선 파일(offical2sep) 하나만 맞는 패턴은 구분선 한 줄이 남는다(웹 기본 제외 목록에 걸리지 않는 패턴).
    const data = newData();
    const only = expandFileList(data, '@' + data.get('guideSpec').replace('*', '2sep'), 'Tell', '{cur.file}.overview');
    assert.deepEqual(only.map((item) => item.kind), ['separator']);
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

// 스크린샷을 찍은 시점의 저장된 진행 값이다(원본 options.cfg 의 Done* 항목, 값 1 = 완료). Early Missions 6개·Struggle For Freedom 6개·A Nation Rises 앞 3개가 완료이고
// Priest Training·Complete Victory 는 하나도 없다. 이 값을 설정에 넣으면 스크린샷의 체크 표시·잠금 상태가 그대로 나온다. (2026-10-29 WB03-E)
const CAMPAIGN_PROGRESS = Object.freeze({
    DoneTutorial1: 1, DoneTutorial2: 1, DoneTutorial3: 1, DoneTutorial4: 1, DoneTutorial5: 1, DoneTutorial6: 1,
    DoneThewarbegins: 1, DoneMasterofwhirligigs: 1, DoneSavetheisland: 1, DoneFragilefortune: 1, DoneThunderingpower: 1, DoneDissolvedalliance: 1,
    DoneThenoose: 1, DoneRainvsrain: 1, DoneRunforit: 1,
});

// 진행 값을 설정에 넣은 새 게임 데이터로 캠페인 목록(UCampaign)을 먼저 펼쳐 묶음 객체를 등록한 뒤, 지정한 묶음의 개요 대화상자를 그려 돌려준다.
// 앱에서도 Campaign 대화상자를 거쳐야 `offical1.overview` 같은 섹션이 등록되므로 같은 순서를 따른다. (2026-10-29 WB03-E)
function renderOverview(name, progress, width = 1024, height = 768) {
    const data = GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2, ...progress } });
    const surface = new IndexedSurface(width, height);
    surface.tile(resources.clouds);
    const position = titlePosition(width, height, resources.title);
    surface.drawImage(resources.title, position.x, position.y);
    createTellWindow({ gameData: data, resources, name: 'UCampaign', screen: { width, height } });
    const result = createTellWindow({ gameData: data, resources, name, screen: { width, height } });
    assert.ok(result.window, `${name} 대화상자를 만들지 못했습니다`);
    result.window.draw(surface);
    const rgb = new Uint8Array(width * height * 3);
    for (let index = 0; index < width * height; index += 1) rgb.set(resources.palette.rgb.subarray(surface.pixels[index] * 3, surface.pixels[index] * 3 + 3), index * 3);
    return { data, window: result.window, rgb, width, height };
}

// 캠페인 하위 5개 목록의 기대값이다: 스크린샷 파일, 패널 크기, 행 상태(c = 활성, k = 체크 표시, x = 잠김). 크기는 스크린샷과 정렬됨을 확인한 값이다. (2026-10-29 WB03-E)
const CAMPAIGN_SHOTS = Object.freeze({
    'offical1.overview': { file: 'mainMenu - Campaign - Early Missions.png', size: [338, 311], rows: ['k', 'k', 'k', 'k', 'k', 'k'] },
    'offical2.overview': { file: 'mainMenu - Campaign - Priest Training.png', size: [418, 404], rows: ['c', 'c', 'c', 'c', 'c', 'c', 'c', 'x', 'x'] },
    'offical3.overview': { file: 'mainMenu - Campaign - Struggle For Freedom.png', size: [458, 353], rows: ['k', 'k', 'k', 'k', 'k', 'k'] },
    'offical4.overview': { file: 'mainMenu - Campaign - A Nation Rises.png', size: [258, 251], rows: ['k', 'k', 'k', 'c', 'x'] },
    'offical5.overview': { file: 'mainMenu - Campaign - Complete Victory.png', size: [258, 247], rows: ['x', 'x', 'x', 'x', 'x'] },
});

// 흰 글자 마스크(채널 최솟값 ≥ 215)를 영역마다 만든다: 키는 'x,y' 로 영역 왼쪽 위 기준 상대 좌표이다. (2026-10-30 WB03-I)
function whiteMask(rendered, rect) {
    const mask = new Set();
    // 영역의 모든 픽셀에서 흰 글자를 모은다.
    for (let y = rect.top; y < rect.bottom; y += 1) {
        for (let x = rect.left; x < rect.right; x += 1) {
            const offset = (y * rendered.width + x) * 3;
            if (Math.min(rendered.rgb[offset], rendered.rgb[offset + 1], rendered.rgb[offset + 2]) >= 215) mask.add(`${x - rect.left},${y - rect.top}`);
        }
    }
    return mask;
}

// 웹 Campaign(6행)은 원본 스크린샷과 대조된 8행 배치에서 두 행(구분선 + User Made Campaigns)만 뺀 것이다: 본문 글자와 앞 6행의 흰 글자가 같은 모양으로 놓이고,
// 위치는 패널이 줄어든 만큼 세로로 옮겨질 뿐이다. 원본 스크린샷에 6행 화면은 없으므로 이 검사는 '제외한 두 행을 뺀 부분만 원본과 같다'는 근거를 확인한다. (2026-10-30 WB03-I)
test('웹 Campaign(6행): 본문과 앞 6행의 글자 모양이 원본 8행 배치와 같다', () => {
    const original = renderDialog('UCampaign', 1024, 768, { original: true });
    const web = renderDialog('UCampaign');
    // 본문(제목 그림·설명): 같은 크기이고 흰 글자 모양이 같다.
    const originalBody = original.window.body.bounds();
    const webBody = web.window.body.bounds();
    assert.equal(webBody.right - webBody.left, originalBody.right - originalBody.left);
    assert.equal(webBody.bottom - webBody.top, originalBody.bottom - originalBody.top);
    const bodyOriginal = whiteMask(original, originalBody);
    assert.ok(bodyOriginal.size > 200, '본문에 흰 글자가 충분히 있어야 비교가 의미 있음');
    assert.deepEqual([...whiteMask(web, webBody)].sort(), [...bodyOriginal].sort());
    // 목록 앞 6행(원본 8행 중 구분선 뒤의 User Made Campaigns 를 뺀 부분)을 목록 왼쪽 위 기준 상대 좌표로 비교한다.
    // 가장 긴 라벨('User Made Campaigns')이 빠져 목록 폭이 2픽셀 줄고 가로 가운데 정렬 때문에 목록이 1픽셀 안쪽으로 옮겨지지만, 글자는 왼쪽 여백(15)에서 시작하므로 상대 좌표는 같다.
    // 목록 테두리의 밝은 경사면(안쪽 2픽셀: 웹은 맨 아래·오른쪽이 6행 끝에 오고 원본은 8행 끝에 옴)은 글자가 아니므로 비교에서 뺀다.
    const six = original.window.menu.offsets[6];
    const originalMenu = original.window.menu.bounds();
    const webMenu = web.window.menu.bounds();
    assert.equal(webMenu.bottom - webMenu.top, six);
    const inset = (rect, amount) => ({ left: rect.left + amount, top: rect.top + amount, right: rect.right - amount, bottom: rect.bottom - amount });
    const originalRegion = { left: originalMenu.left, top: originalMenu.top, right: originalMenu.left + (webMenu.right - webMenu.left), bottom: originalMenu.top + six };
    const maskOriginal = whiteMask(original, inset(originalRegion, 2));
    const maskWeb = whiteMask(web, inset(webMenu, 2));
    assert.ok(maskOriginal.size > 300, '목록에 흰 글자가 충분히 있어야 비교가 의미 있음');
    assert.deepEqual([...maskWeb].sort(), [...maskOriginal].sort());
    // 이 비교가 위치에 민감함을 보인다: 원본 영역을 가로·세로로 한 칸 어긋나게 잡으면 겹침이 약 59~66% 로 떨어진다(획이 2픽셀 굵기라 0 이 되지는 않는다).
    const base = inset(originalRegion, 2);
    for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        const moved = whiteMask(original, { left: base.left + dx, top: base.top + dy, right: base.right + dx, bottom: base.bottom + dy });
        const overlap = [...moved].filter((key) => maskWeb.has(key)).length / maskWeb.size;
        assert.ok(overlap < 0.8, `한 칸(${dx}, ${dy}) 어긋난 영역과의 겹침 ${overlap.toFixed(3)} 이 너무 커 위치에 둔감함`);
    }
});

// 캠페인 하위 목록 5종: 저장된 진행 값에 따라 행이 [체크 표시 | 활성 | 잠김] 으로 나뉜다. 활성 조건은 `$Checked` 5번째 인자(ConfigParseLong ≠ 0)이며 값이 없으면 빈 문자열 → 0 → 잠김이다.
// 처음 설치한 상태(진행 값 없음)에서는 각 묶음의 첫 행(고정 인자 1)만 활성이다. (2026-10-29 WB03-E)
test('캠페인 하위 목록: 진행 값에 따른 체크 표시·잠금 상태', () => {
    const state = (item) => (item.disabled ? 'x' : item.checked ? 'k' : 'c');
    // 스크린샷 시점의 진행 값으로 다섯 묶음의 행 상태를 확인한다.
    for (const [name, info] of Object.entries(CAMPAIGN_SHOTS)) {
        const window = renderOverview(name, CAMPAIGN_PROGRESS).window;
        assert.deepEqual(window.menu.items.map(state), info.rows, `${name} 행 상태`);
        assert.deepEqual([window.baseWidth, window.baseHeight], info.size, `${name} 패널 크기`);
    }
    // 처음 설치한 상태: Early Missions 는 1번만 활성, Struggle For Freedom 도 1번만, Priest Training 은 앞 7개가 활성이고 마지막 2개는 잠긴다.
    assert.deepEqual(renderOverview('offical1.overview', {}).window.menu.items.map(state), ['c', 'x', 'x', 'x', 'x', 'x']);
    assert.deepEqual(renderOverview('offical3.overview', {}).window.menu.items.map(state), ['c', 'x', 'x', 'x', 'x', 'x']);
    assert.deepEqual(renderOverview('offical2.overview', {}).window.menu.items.map(state), ['c', 'c', 'c', 'c', 'c', 'c', 'c', 'x', 'x']);
    // 한 미션만 끝낸 상태: 1번에 체크 표시, 2번이 열린다. 진행 값은 대소문자를 가리지 않는다(저장 파일은 `DoneThewarbegins` 로 적히지만 목록은 `{DoneTheWarBegins}` 로 읽는다).
    assert.deepEqual(renderOverview('offical1.overview', { DoneTutorial1: 1 }).window.menu.items.map(state), ['k', 'c', 'x', 'x', 'x', 'x']);
    assert.deepEqual(renderOverview('offical3.overview', { DoneThewarbegins: 1 }).window.menu.items.map(state), ['k', 'c', 'x', 'x', 'x', 'x']);
    // 잠긴 행은 누를 수 없고 활성 행의 명령은 MissionBegin 이다.
    const early = renderOverview('offical1.overview', { DoneTutorial1: 1 }).window;
    assert.deepEqual(early.menu.items[1].command, { name: 'MissionBegin', argument: 'Tutorial2' });
    const second = early.menu.bounds();
    const lockedRow = second.top + 3 * 17 + 3;
    assert.equal(early.menu.hitIndex(second.left + 20, lockedRow), -1);
    assert.equal(early.menu.hitIndex(second.left + 20, second.top + 17 + 3), 1);
});

// 캠페인 하위 목록의 원본 스크린샷 대조: 정렬(최적 원점이 이웃보다 3배 이상 좋음)과 클라이언트 원점 범위, 목록·본문·버튼 평균 차이, 목록의 흰 글자 마스크 일치율이다.
// 평균 차이는 캡처의 디더링 노이즈(화면마다 다름)를 포함하므로 한계를 넉넉히 두고, 노이즈와 무관한 글자 위치 일치는 흰 글자(채널 최솟값 ≥ 215) 마스크의 교집합/합집합으로 확인한다. (2026-10-29 WB03-E)
test('캠페인 하위 목록: 원본 스크린샷 5장과 위치·글자 일치', (t) => {
    const isWhite = (buffer, offset) => Math.min(buffer[offset], buffer[offset + 1], buffer[offset + 2]) >= 215;
    // 다섯 화면을 차례로 대조한다.
    for (const [name, info] of Object.entries(CAMPAIGN_SHOTS)) {
        const shotPath = path.join(REPO_ROOT, 'screenShots', info.file);
        if (!fs.existsSync(shotPath)) {
            t.diagnostic(`${info.file} 가 없는 환경이라 건너뜀`);
            continue;
        }
        const shot = decodePng(shotPath);
        const rendered = renderOverview(name, CAMPAIGN_PROGRESS);
        const { window } = rendered;
        const menu = window.menu.bounds();
        const area = (rect) => [rect.left, rect.top, rect.right, rect.bottom];
        const scores = [];
        // 클라이언트 원점 후보를 훑어 목록 영역 차이가 가장 작은 원점을 찾는다.
        for (let originY = 28; originY <= 40; originY += 1) {
            for (let originX = 0; originX <= 10; originX += 1) scores.push({ originX, originY, score: regionDiff(rendered, shot, area(menu), originX, originY) });
        }
        scores.sort((a, b) => a.score - b.score);
        const best = scores[0];
        const neighbors = scores.filter((entry) => Math.abs(entry.originX - best.originX) + Math.abs(entry.originY - best.originY) === 1);
        assert.ok(neighbors.every((entry) => entry.score > best.score * 3), `${name}: 정렬이 유일하지 않습니다`);
        // 스크린샷의 창 테두리·제목 표시줄을 뺀 클라이언트 원점은 모든 캡처에서 x 2~7, y 33~35 안이다.
        assert.ok(best.originX >= 2 && best.originX <= 7 && best.originY >= 33 && best.originY <= 35, `${name}: 원점 (${best.originX}, ${best.originY})`);
        assert.ok(best.score <= 8, `${name} 목록 평균 차이 ${best.score.toFixed(2)}`);
        assert.ok(regionDiff(rendered, shot, area(window.body.bounds()), best.originX, best.originY) <= 6.5, `${name} 본문 평균 차이`);
        assert.ok(regionDiff(rendered, shot, area(window.buttons[0].bounds()), best.originX, best.originY) <= 8, `${name} 버튼 평균 차이`);
        assert.ok(regionDiff(rendered, shot, area(window.bounds()), best.originX, best.originY) <= 6, `${name} 패널 평균 차이`);
        // 목록 영역의 흰 글자 마스크가 같다(체크 표시·잠금과 무관하게 활성 행 글자 위치·모양 일치). 모든 행이 잠긴 화면은 흰 글자가 거의 없어 건너뛴다.
        let both = 0;
        let either = 0;
        for (let y = menu.top; y < menu.bottom; y += 1) {
            for (let x = menu.left; x < menu.right; x += 1) {
                const inShot = isWhite(shot.rgb, ((y + best.originY) * shot.width + x + best.originX) * 3);
                const inRendered = isWhite(rendered.rgb, (y * rendered.width + x) * 3);
                if (inShot && inRendered) both += 1;
                if (inShot || inRendered) either += 1;
            }
        }
        if (either >= 200) assert.ok(both / either >= 0.98, `${name} 흰 글자 마스크 일치율 ${(both / either).toFixed(3)}`);
        else t.diagnostic(`${name}: 흰 글자가 ${either}픽셀뿐이라 마스크 비교를 건너뜀(모든 행 잠김)`);
    }
});

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
        // 원본 스크린샷은 User Made Campaigns 행이 있는 8행 Campaign 이므로 웹 제외 없이(original) 그려 대조한다. (2026-10-30 WB03-I)
        const rendered = renderDialog(name, 1024, 768, { original: true });
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
