// 제품 표기(버전 11.01)와 세 번째 Credits 항목(Netstorm 11.01 Patch Credits)을 검사한다.
// 원본 자료는 그대로 두고 설정 층의 버전 줄·등록 설정 객체 Patch1101·메인 Credits 본문의 한 행 추가만으로 제공하는지,
// 원본 대조용(product: false) GameData 는 원본 10.78 표기·두 행 그대로인지, 같은 Tell 창·버튼 수명(Back→Credits, Cancel→Blank)을 쓰는지 확인한다.
// (2026-11-05 WB03-AC: AGENTS.md 의 11.01 표기·Patch Credits 요구 적용과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { KOREAN_BUNDLE, buildKoreanSupport } from '../www/assets/korean-assets.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import {
    ORIGINAL_DATA_EDITION, PRODUCT_CREDITS_LABELS, PRODUCT_CREDITS_OBJECT, PRODUCT_CREDITS_TARGET, PRODUCT_CREDITS_TITLES,
    PRODUCT_MAJOR_VERSION, PRODUCT_MINOR_VERSION, PRODUCT_VERSION, PRODUCT_VERSION_VALUE, productCreditsLabel, productCreditsText, productTellBody,
} from '../www/assets/product.js';
import { createTellWindow } from '../www/ui/tell-window.js';

// 메뉴 묶음·한국어 묶음 파일과 영어 메뉴 자원이다(검사마다 공유). (2026-11-05 WB03-AC)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const koreanFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE).map((entry) => entry.path));
const englishResources = buildMenuResources(menuFiles);

// 가짜 래스터라이저이다. 한글(0x7f 초과)은 em 폭, 그 밖은 em 의 절반 폭으로 전진하고 3×2 그림을 낸다(공백은 그림 없음). korean-tell.test.mjs 와 같은 규칙이다. (2026-11-05 WB03-AC)
function fakeRasterizer() {
    return {
        glyph(code, spec) {
            const advance = code > 0x7f ? spec.emSize : Math.round(spec.emSize / 2);
            if (code === 0x20) return { advance, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) };
            return { advance, left: 1, top: 2, width: 3, height: 2, coverage: Uint8Array.from([255, 109, 110, 0, 200, 255]) };
        },
    };
}
const korean = await buildKoreanSupport(koreanFiles, { createRasterizer: () => fakeRasterizer() });
const koreanResources = buildMenuResources(menuFiles, korean);

// 1024×768 논리 화면이다. (2026-11-05 WB03-AC)
const SCREEN = Object.freeze({ width: 1024, height: 768 });

// 새 게임 데이터를 만든다. language 가 korean 이고 withFiles 면 한국어 언어 자료를 합친다. product 가 false 면 원본 대조용이다. (2026-11-05 WB03-AC)
function newData({ language = 'english', withFiles = true, product = true } = {}) {
    const files = language === 'korean' && withFiles ? new Map([...menuFiles, ...korean.languageFiles]) : menuFiles;
    return GameData.build({ files, language, options: { soundVolume: 3, musicVolume: 2 }, product });
}

// Tell 창을 연다. 명령은 commands 배열에 모인다. (2026-11-05 WB03-AC)
function open(name, { data = newData(), resources = englishResources, screen = SCREEN, commands = [] } = {}) {
    return createTellWindow({ gameData: data, resources, name, screen, onCommand: (command) => commands.push(command) }).window;
}

// 창 본문의 글자를 줄 단위 문자열로 모은다. 영어 조각은 Windows-1252 바이트, 한국어 조각은 유니코드 코드 배열이다(그림 조각은 뺀다). (2026-11-05 WB03-AC)
function bodyText(window) {
    return window.body.softLines.map((line) => line.fragments.filter((fragment) => !fragment.picture).map((fragment) => String.fromCharCode(...fragment.bytes)).join('')).join('\n');
}

// 제품 상수: 표기 11.01(선행 0 보존)·설정 값 11/1/"v11.01"·대상 이름이 cpppj 와 같고, 원본 판본 상수는 10.78 이다.
test('제품 상수: 11.01 표기와 Patch1101 대상', () => {
    assert.equal(PRODUCT_VERSION, '11.01');
    assert.equal(PRODUCT_VERSION_VALUE, 'v11.01');
    assert.deepEqual([PRODUCT_MAJOR_VERSION, PRODUCT_MINOR_VERSION], [11, 1]);
    assert.equal(ORIGINAL_DATA_EDITION, '10.78');
    assert.equal(PRODUCT_CREDITS_OBJECT, 'Patch1101');
    assert.equal(PRODUCT_CREDITS_TARGET, 'Patch1101.Credits');
    // AGENTS.md 가 정한 영어 행 이름·창 제목(대소문자 그대로)이다.
    assert.equal(PRODUCT_CREDITS_LABELS.english, 'Netstorm 11.01 Patch Credits');
    assert.equal(PRODUCT_CREDITS_TITLES.english, 'NetStorm 11.01 Patch Credits');
    assert.equal(productCreditsLabel('german'), PRODUCT_CREDITS_LABELS.english, '없는 언어는 영어');
    assert.equal(productCreditsText('german'), productCreditsText('english'));
    assert.ok(productCreditsText('english').startsWith('[Credits]'));
});

// 본문 확장: 메인 Credits(대소문자 무시)에만 한 행을 덧붙이고, 이미 대상이 있으면 다시 붙이지 않으며, 다른 섹션은 그대로다.
test('제품 Credits 행: Credits 에만 한 번 덧붙인다', () => {
    const body = '<h2>{H2}NetStorm Credits</h2>\r\n$Menu=A,Tell,CreditsNew\r\n$Button=Cancel,Tell,Blank\r\n';
    const extended = productTellBody('Credits', body, 'english');
    assert.equal(extended, `${body}\n$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits\n`);
    assert.equal(productTellBody('CREDITS', body, 'english'), extended);
    assert.equal(productTellBody('Credits', extended, 'english'), extended, '중복 확장 방지');
    assert.equal(productTellBody('CreditsNew', body, 'english'), body);
    assert.equal(productTellBody('About', body, 'english'), body);
    assert.ok(productTellBody('Credits', body, 'korean').endsWith('$Menu=NetStorm 11.01 패치 크레딧,Tell,Patch1101.Credits\n'));
});

// 설정 층: 제품 버전 줄과 Patch1101 객체가 있고, 원문 Tell 조회(tellSection)는 바뀌지 않는다. 원본 대조용은 10.78·객체 없음이다.
test('게임 데이터: 제품 버전 줄·Patch1101 객체와 원본 대조용 분리', () => {
    const data = newData();
    assert.equal(data.get('version'), 'v11.01');
    assert.equal(data.get('MAJOR_VERSION'), '11');
    assert.equal(data.get('MINOR_VERSION'), '1');
    // About 창(Help → Version)과 용어표 copyright 가 제품 표기를 쓴다.
    assert.ok(data.substitute(data.tellSection('About')).includes('Version v11.01'));
    assert.ok(data.substitute('{copyright}').endsWith('Activision Inc. v11.01'));
    assert.ok(data.registry.findByName('Patch1101'));
    assert.ok(data.tellSection('Patch1101.Credits').includes('NetStorm 11.01 Patch Credits'));
    // 원문 Credits 섹션 자체는 그대로 두 행이며, 창에 보일 본문에만 세 번째 행이 붙는다.
    const raw = data.tellSection('Credits');
    assert.equal(raw.includes('Patch1101'), false);
    assert.equal(data.presentTellBody('Credits', raw).split('$Menu=').length - 1, 3);
    // 원본 대조용: 원본 10.78 표기·객체 없음·본문 확장 없음.
    const original = newData({ product: false });
    assert.equal(original.get('version'), 'v10.78');
    assert.equal(original.substitute('{MAJOR_VERSION}.{MINOR_VERSION}'), '10.78');
    assert.equal(original.registry.findByName('Patch1101'), null);
    assert.equal(original.tellSection('Patch1101.Credits'), null);
    assert.equal(original.presentTellBody('Credits', raw), raw);
    assert.equal(original.tellSection('Credits'), raw);
});

// 영어 Credits 목록: 원본 두 행 뒤에 세 번째 행이 붙고, 패널이 한 행(17) 만큼 커져 화면 가운데에 다시 놓인다. 본문·목록·버튼 사이 간격은 원본과 같다.
test('영어 Credits 목록: 세 행 순서·명령·배치', () => {
    const commands = [];
    const window = open('Credits', { commands });
    const original = open('Credits', { data: newData({ product: false }) });
    assert.deepEqual(window.menu.items.map((item) => item.label), ['Netstorm 10.72 Patch Credits', 'Netstorm Original Credits', 'Netstorm 11.01 Patch Credits']);
    assert.deepEqual(window.menu.items.map((item) => item.command), [
        { name: 'Tell', argument: 'CreditsNew' }, { name: 'Tell', argument: 'Creditsold' }, { name: 'Tell', argument: 'Patch1101.Credits' },
    ]);
    // 앞 두 행은 원본과 같은 라벨·명령이다.
    assert.deepEqual(window.menu.items.slice(0, 2).map((item) => [item.label, item.command]), original.menu.items.map((item) => [item.label, item.command]));
    assert.deepEqual(window.buttons.map((button) => [button.text, button.command]), original.buttons.map((button) => [button.text, button.command]));
    // 배치: 원본 234×159 → 234×176, 위쪽 304 → 296(가운데 정렬), 목록 높이 34 → 51.
    assert.deepEqual([window.baseWidth, window.baseHeight], [234, 176]);
    assert.deepEqual(window.bounds(), { left: 395, top: 296, right: 629, bottom: 472 });
    assert.deepEqual(window.menu.bounds(), { left: 415, top: 376, right: 609, bottom: 427 });
    assert.deepEqual(window.buttons[0].bounds(), { left: 487, top: 437, right: 536, bottom: 457 });
    const gap = (w) => [w.menu.bounds().top - w.body.bounds().bottom, w.buttons[0].bounds().top - w.menu.bounds().bottom];
    assert.deepEqual(gap(window), gap(original));
    assert.equal((window.menu.bounds().bottom - window.menu.bounds().top) - (original.menu.bounds().bottom - original.menu.bounds().top), 17);
    // 세 번째 행을 누르면 즉시 Tell Patch1101.Credits 명령이 나간다(원본 목록 행과 같은 즉시 실행).
    const menu = window.menu.bounds();
    window.pointerDown(menu.left + 5, menu.top + 2 * 17 + 3);
    assert.deepEqual(commands.pop(), { name: 'Tell', argument: 'Patch1101.Credits' });
});

// 영어 Patch Credits 창: 제목·HJOW·AI 사용·제품 버전이 보이고, Back→Credits 목록·Cancel→메인 메뉴(Blank)이며 화면 안에 들어간다.
test('영어 Patch Credits 창: 제목·본문·Back/Cancel 수명', () => {
    const commands = [];
    const data = newData();
    const window = open('Patch1101.Credits', { data, commands });
    assert.ok(window, 'Patch1101.Credits 창');
    const text = bodyText(window);
    assert.ok(text.startsWith('NetStorm 11.01 Patch Credits'), text);
    // 11.01 카피라이트 줄(AGENTS.md 사용자 커밋 60226ea9): 개발 줄 다음에 `Copyright 2026 HJOW` 가 한 줄로 있어야 한다. (2026-11-11 WB)
    assert.ok(/HJOW, NetStorm 11\.01 patch development\.\s+Copyright 2026 HJOW\s/.test(text.replace(/\n/g, ' ')), `카피라이트 줄 위치: ${text}`);
    for (const fragment of ['Version v11.01', 'HJOW', 'Copyright 2026 HJOW', 'AI assistance', 'OpenAI Codex', 'ChatGPT', 'Anthropic Claude Code', 'analysis, programming,', 'original Credits entries']) {
        assert.ok(text.replace(/\n/g, ' ').includes(fragment), `본문에 '${fragment}' 가 없음`);
    }
    // 현재 Tell 이름 기록: 창을 연 이름 그대로이다.
    assert.equal(data.currentTell, 'Patch1101.Credits');
    assert.deepEqual(window.buttons.map((button) => [button.text, button.command]), [
        ['Back', { name: 'Tell', argument: 'Credits', instant: false }], ['Cancel', { name: 'Tell', argument: 'Blank', instant: false }],
    ]);
    // 돌 버튼은 누른 뒤 안쪽에서 뗄 때 실행한다.
    const back = window.buttons[0].bounds();
    window.pointerDown(back.left + 3, back.top + 3);
    window.pointerUp(back.left + 3, back.top + 3);
    assert.deepEqual(commands.pop(), { name: 'Tell', argument: 'Credits', instant: false });
    // 4:3·16:9 화면 모두 안에 들어간다.
    for (const screen of [SCREEN, { width: 1366, height: 768 }]) {
        const bounds = open('Patch1101.Credits', { screen }).bounds();
        assert.ok(bounds.left >= 0 && bounds.top >= 0 && bounds.right <= screen.width && bounds.bottom <= screen.height, `${screen.width}: ${JSON.stringify(bounds)}`);
    }
});

// 한국어: 세 번째 행·창 제목·단추가 한국어이고 명령은 영어와 같다. 한국어 자료를 받지 못해 영어로 대체된 화면은 영어 행을 쓴다.
test('한국어 Patch Credits: 번역 행·창과 영어 대체', () => {
    const data = newData({ language: 'korean' });
    assert.equal(data.productLanguage, 'korean');
    const list = open('Credits', { data, resources: koreanResources });
    assert.deepEqual(list.menu.items.map((item) => item.label), ['NetStorm 10.72 패치 크레딧', 'NetStorm 오리지널 크레딧', 'NetStorm 11.01 패치 크레딧']);
    assert.deepEqual(list.menu.items[2].command, { name: 'Tell', argument: 'Patch1101.Credits' });
    const window = open('Patch1101.Credits', { data, resources: koreanResources });
    const text = bodyText(window);
    assert.ok(text.startsWith('NetStorm 11.01 패치 크레딧'), text);
    for (const fragment of ['버전 v11.01', 'HJOW', 'Copyright 2026 HJOW', 'AI', 'Anthropic Claude Code']) assert.ok(text.replace(/\n/g, ' ').includes(fragment), `본문에 '${fragment}' 가 없음`);
    assert.deepEqual(window.buttons.map((button) => [button.text, button.command]), [
        ['뒤로', { name: 'Tell', argument: 'Credits', instant: false }], ['취소', { name: 'Tell', argument: 'Blank', instant: false }],
    ]);
    assert.ok(window.buttons.every((button) => button.labelBytes instanceof Uint16Array), '한국어 단추는 유니코드 글꼴');
    // 한국어 묶음 실패(언어는 korean 이지만 한국어 tell 파일 없음): 영어 Tell 과 같은 영어 행·창이다.
    const fallback = newData({ language: 'korean', withFiles: false });
    assert.equal(fallback.productLanguage, 'english');
    assert.equal(open('Credits', { data: fallback }).menu.items[2].label, 'Netstorm 11.01 Patch Credits');
    assert.ok(bodyText(open('Patch1101.Credits', { data: fallback })).startsWith('NetStorm 11.01 Patch Credits'));
});
