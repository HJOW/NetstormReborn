// 한국어 지원(WB03-N)을 검사한다: 번역표(XlatTable)·한국어 번역 파일, 유니코드 글꼴(UnicodeGlyphFont·UnicodeFontProvider),
// 글꼴 묶음(FontSet)의 글꼴 고르기, 글자 그리기, 한국어 메인 메뉴 버튼·펼침 메뉴, 한국어 자산 묶음 조립을 확인한다.
// 브라우저의 Canvas 글자 그리기는 Node 에 없으므로 가짜 래스터라이저(고정 크기 상자)로 배치·측정·그리기 규칙을 검사하고,
// 실제 D2Coding 글리프는 브라우저 검사(임시 CDP 스크립트)로 확인한다. (2026-10-31 WB03-N)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { XlatTable } from '../www/assets/xlat.js';
import { decodeOriginalText, encodeUtf16Units, encodeWindows1252 } from '../www/assets/text-encoding.js';
import { COVERAGE_THRESHOLD, EM_PER_SLOT_HEIGHT, UnicodeFontProvider, UnicodeGlyphFont } from '../www/assets/unicode-font.js';
import { FontSet } from '../www/assets/font-set.js';
import { BitmapFont } from '../www/assets/bitmap-font.js';
import { KOREAN_ASSET_PATHS, KOREAN_BUNDLE, KOREAN_LANGUAGE_PATHS, buildKoreanSupport } from '../www/assets/korean-assets.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { FONT_INK_INDEX, createTextContext, drawGlyphText } from '../www/render/glyph-text.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { MAIN_MENU_SLOTS, MainMenuScreen } from '../www/ui/main-menu.js';
import { StoneButton } from '../www/ui/stone-button.js';
import { PopupMenu } from '../www/ui/popup-menu.js';
import { buildHelpItems, buildOptionsItems, buildPopupItems } from '../www/ui/option-menus.js';
import { Settings } from '../www/platform/settings.js';
import { MAIN_MENU_WINDOW_TITLE, Netstorm } from '../www/netstorm.js';
import { createCanvasGlyphRasterizer } from '../www/platform/glyph-rasterizer.js';

// 가짜 래스터라이저이다. 한글(0x7f 초과)은 em 폭, 그 밖은 em 의 절반 폭으로 전진하고, 공백은 그림이 없으며, 나머지는 3×2 알파 그림을 낸다.
// 알파 [255,109,110 / 0,200,255] 중 문턱(110) 이상인 칸은 (0,0)·(2,0)·(1,1)·(2,1) 이다. 호출을 calls 에 기록한다. (2026-10-31 WB03-N)
function fakeRasterizer() {
    const calls = [];
    return {
        calls,
        glyph(code, spec) {
            calls.push({ code, spec });
            const advance = code > 0x7f ? spec.emSize : Math.round(spec.emSize / 2);
            if (code === 0x20) return { advance, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) };
            return { advance, left: 1, top: 2, width: 3, height: 2, coverage: Uint8Array.from([255, 109, 110, 0, 200, 255]) };
        },
    };
}

// 글꼴 슬롯 0(높이 14)의 한글 글자 크기이다. 가짜 래스터라이저에서 한글 전진 폭이 이 값이고 영문·공백은 절반(반올림)이다. 슬롯 높이 14 → em 13. (2026-10-31 WB03-N)
const EM = 13;
assert.equal(Math.round(14 * EM_PER_SLOT_HEIGHT), EM);

// 메뉴 묶음 파일(원본 글꼴 포함)이다. (2026-10-31 WB03-N)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));

// 한국어 묶음 파일(실제 woff2·번역표)이다. (2026-10-31 WB03-N)
const koreanFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE).map((entry) => entry.path));

// 가짜 래스터라이저로 만든 한국어 지원과 그 자원 묶음이다. (2026-10-31 WB03-N)
const rasterizer = fakeRasterizer();
const korean = await buildKoreanSupport(koreanFiles, { createRasterizer: () => rasterizer });
const koreanResources = buildMenuResources(menuFiles, korean);
const englishResources = buildMenuResources(menuFiles);

// 번역표 읽기: 원본 xlat 형식(Xlat.cpp)의 구분 규칙, 줄바꿈 처리, 덮어쓰기, 미완성 블록. (2026-10-31 WB03-N)
test('XlatTable: *** / --- / === 블록, CR 제거, 줄바꿈 하나만 떼기, 덮어쓰기, 미완성 블록 버리기', () => {
    const table = new XlatTable([
        '// 주석은 블록 밖이라 무시', '+++ 구분선 +++', '***', 'Campaign', '---', '캠페인', '===',
        '***', '여러 줄', '원문', '---', '첫 줄', '', '셋째 줄', '===',
        '***', 'Dup', '---', 'one', '===', '***', 'Dup', '---', 'two', '===',
        '***', 'Unfinished', '---', 'never closed',
    ].join('\r\n'));
    assert.equal(table.size, 3);
    assert.equal(table.translate('Campaign'), '캠페인');
    // 원문이 여러 줄이어도 키가 된다. 번역문 안의 빈 줄은 보존되고 끝의 줄바꿈 하나만 빠진다.
    assert.equal(table.translate('여러 줄\n원문'), '첫 줄\n\n셋째 줄');
    assert.equal(table.translate('Dup'), 'two');
    // 대소문자까지 같아야 하고, 없거나 미완성이면 원문 그대로다.
    assert.equal(table.translate('campaign'), 'campaign');
    assert.equal(table.translate('Unfinished'), 'Unfinished');
    assert.ok(table.has('Campaign') && !table.has('campaign'));
    // 마지막 === 줄에 줄바꿈이 없어도 블록이 완성된다.
    assert.equal(new XlatTable('***\nA\n---\nB\n===').translate('A'), 'B');
    // 블록 사이의 번역문 구분 문자가 아닌 줄(`-` 로 시작하지 않는 줄)은 원문에 그대로 들어간다.
    assert.equal(new XlatTable('***\nA - b\n---\nC\n===').translate('A - b'), 'C');
    assert.equal(new XlatTable('').size, 0);
});

// 한국어 번역 파일: UTF-8(BOM 없음), 중복 없음, 웹이 보여 줄 수 있는 모든 라벨이 번역되어 있다. (2026-10-31 WB03-N)
test('xlat.korean: UTF-8·중복 없음·메뉴 라벨 전부 번역', () => {
    const bytes = koreanFiles.get(KOREAN_ASSET_PATHS.translations);
    assert.notDeepEqual([...bytes.subarray(0, 3)], [0xef, 0xbb, 0xbf], 'BOM 없는 UTF-8');
    const text = decodeOriginalText(bytes);
    assert.equal(new XlatTable(text).size, (text.match(/^\*\*\*$/gm) ?? []).length, '원문 중복 없음');
    assert.ok(/[가-힣]/.test(text));
    // 번역표 원본(소스)과 웹 공개 복사본이 같다.
    assert.deepEqual([...bytes], [...new Uint8Array(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'xlat.korean')))]);
    // 코드가 만들 수 있는 모든 라벨(원본 비교용 항목 포함)을 모은다.
    // 일반 도움말 창의 Back·OK 단추도 번역 키다(2026-10-31 WB03-O).
    const labels = new Set([...MAIN_MENU_SLOTS.map((slot) => slot.label), MAIN_MENU_WINDOW_TITLE, 'Back', 'OK']);
    const collect = (items) => {
        for (const item of items) {
            if (item.kind === 'separator') continue;
            labels.add(item.label);
            if (item.items) collect(item.items);
        }
    };
    const settings = new Settings();
    collect(buildOptionsItems(settings, { includeAll: true }));
    collect(buildHelpItems({ includeAll: true }));
    // 숫자만 있는 해상도 라벨(640 by 480 …)은 번역하지 않는다.
    const untranslatable = new Set(['640 by 480', '800 by 600', '1024 by 768']);
    const missing = [...labels].filter((label) => !untranslatable.has(label) && !korean.translations.has(label));
    assert.deepEqual(missing, [], '번역이 없는 라벨');
    // 번역표에는 코드가 쓰지 않는 원문이 없다(오타로 키가 어긋난 번역을 찾는다).
    const unused = [...korean.translations.entries.keys()].filter((key) => !labels.has(key));
    assert.deepEqual(unused, [], '코드에 없는 원문');
    // 글자 모양 표시(~i)는 번역문에도 남아 있어야 한다.
    assert.equal(korean.translate('General Help - ~iF1~.'), '일반 도움말 - ~iF1~.');
    assert.equal(korean.translate('Pass Server Diagnostic '), '서버 진단 통과');
});

// 글꼴 파일: 프로젝트 fonts/ 의 woff2 와 같은 바이트이고 woff2 서명을 가진다. (2026-10-31 WB03-N)
test('한국어 글꼴 자산: fonts/ 의 woff2 를 그대로 복사했다', () => {
    for (const key of ['regular', 'bold', 'license']) {
        const relative = KOREAN_ASSET_PATHS[key];
        const source = new Uint8Array(fs.readFileSync(path.join(REPO_ROOT, relative)));
        assert.deepEqual([...koreanFiles.get(relative)], [...source], relative);
    }
    for (const key of ['regular', 'bold']) assert.equal(String.fromCharCode(...koreanFiles.get(KOREAN_ASSET_PATHS[key]).subarray(0, 4)), 'wOF2');
    // 매니페스트에는 한국어 묶음의 모든 파일이 있고 원본 경로(source)가 적혀 있다.
    const entries = manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE);
    assert.deepEqual(entries.map((entry) => entry.path).sort(), [...Object.values(KOREAN_ASSET_PATHS), ...KOREAN_LANGUAGE_PATHS].sort());
    for (const entry of entries) assert.ok(entry.source && !entry.source.startsWith('/'), entry.path);
});

// 부호화: UTF-16 코드 단위, BMP 밖 문자는 대체 문자. (2026-10-31 WB03-N)
test('encodeUtf16Units: 한글·라틴은 코드 단위 그대로, BMP 밖은 U+FFFD', () => {
    assert.deepEqual([...encodeUtf16Units('가A')], [0xac00, 0x41]);
    assert.ok(encodeUtf16Units('x') instanceof Uint16Array);
    assert.deepEqual([...encodeUtf16Units('a😀b')], [0x61, 0xfffd, 0x62]);
    assert.deepEqual([...encodeUtf16Units(undefined)], []);
    assert.ok(encodeWindows1252('x') instanceof Uint8Array, '원본 부호화는 Uint8Array 그대로');
});

// 유니코드 글꼴: 전진 폭·단색 글리프·문턱·빈 글자·캐시. (2026-10-31 WB03-N)
test('UnicodeGlyphFont: 알파 문턱으로 색 번호 100 글리프를 만들고 폭을 합산한다', () => {
    const fake = fakeRasterizer();
    const font = new UnicodeGlyphFont({ rasterizer: fake, spec: { emSize: 12, bold: false, italic: false, underline: false, strikeout: false }, height: 14, ascent: 11 });
    assert.equal(font.height, 14);
    assert.equal(font.ascent, 11);
    assert.equal(font.descent, 3);
    const hangul = font.glyph(0xac00);
    assert.equal(hangul.advance, 12);
    assert.equal(hangul.drawAdvance, 12);
    assert.equal(hangul.present, true);
    assert.deepEqual({ width: hangul.image.width, height: hangul.image.height, left: hangul.image.left, top: hangul.image.top }, { width: 3, height: 2, left: 1, top: 2 });
    // 문턱: 109 는 빠지고 110 은 남는다.
    assert.equal(COVERAGE_THRESHOLD, 110);
    assert.deepEqual([...hangul.image.opacity], [255, 0, 255, 0, 255, 255]);
    assert.ok([...hangul.image.indices].every((value, index) => value === (hangul.image.opacity[index] ? FONT_INK_INDEX : 0)));
    // 공백은 그림이 없지만 전진한다. 같은 글자는 한 번만 그린다.
    const space = font.glyph(0x20);
    assert.equal(space.image, null);
    assert.equal(space.advance, 6);
    assert.equal(space.present, true);
    assert.equal(font.glyph(0xac00), hangul);
    assert.equal(fake.calls.filter((call) => call.code === 0xac00).length, 1);
    // 측정은 전진 폭의 합이다.
    assert.equal(font.measure(Uint16Array.from([0xac00, 0x20, 0x41])), 12 + 6 + 6);
    assert.equal(font.measure(new Uint16Array(0)), 0);
    // 래스터라이저에는 기준선(ascent)과 줄 높이가 함께 넘어간다.
    assert.equal(fake.calls[0].spec.ascent, 11);
    assert.equal(fake.calls[0].spec.height, 14);
});

// 밑줄·취소선은 글자 칸 전체에 한 줄을 긋는다. (2026-10-31 WB03-N)
test('UnicodeGlyphFont: 밑줄과 취소선은 글자 칸 전체에 선을 더한다', () => {
    const make = (spec) => new UnicodeGlyphFont({ rasterizer: fakeRasterizer(), spec: { emSize: 12, bold: false, italic: false, underline: false, strikeout: false, ...spec }, height: 14, ascent: 11 });
    const under = make({ underline: true }).glyph(0xac00);
    // 밑줄은 기준선 아래 한 줄(줄 번호 12)이고 폭은 0 ~ 전진 폭(12)을 덮는다.
    assert.equal(under.image.left, 0);
    assert.equal(under.image.width, 12);
    const row = 12 - under.image.top;
    assert.ok(row >= 0 && row < under.image.height);
    for (let x = 0; x < 12; x += 1) assert.equal(under.image.opacity[row * under.image.width + x], 255, `밑줄 x=${x}`);
    // 취소선은 기준선 위 round(12*0.26)=3 줄(줄 번호 8)이다.
    const strike = make({ strikeout: true }).glyph(0xac00);
    const strikeRow = 8 - strike.image.top;
    for (let x = 0; x < 12; x += 1) assert.equal(strike.image.opacity[strikeRow * strike.image.width + x], 255, `취소선 x=${x}`);
    // 그림이 없는 공백도 선은 긋는다.
    const space = make({ underline: true }).glyph(0x20);
    assert.equal(space.image.width, 6);
    assert.equal(space.image.height, 1);
    assert.ok([...space.image.opacity].every((value) => value === 255));
});

// 공급자: 슬롯 높이에서 글자 크기, 굵기·기울임·밑줄·취소선, 기준선은 원본 비트맵 글꼴. (2026-10-31 WB03-N)
test('UnicodeFontProvider: 슬롯·스타일별 글꼴 사양과 원본 글꼴 기준선', () => {
    const provider = new UnicodeFontProvider(fakeRasterizer());
    const reference = BitmapFont.parse(menuFiles.get('d/!Arial.normal.14.700.chfnt'));
    const slot0 = provider.get(0, 0, reference);
    assert.equal(slot0.spec.emSize, Math.round(14 * EM_PER_SLOT_HEIGHT));
    assert.equal(slot0.spec.bold, false, '슬롯 0 은 굵기 700 이지만 한글은 보통체');
    assert.equal(slot0.height, reference.height);
    assert.equal(slot0.ascent, reference.ascent);
    assert.equal(provider.get(0, 0, reference), slot0, '같은 슬롯·스타일은 하나');
    // 슬롯 5(굵기 0)는 보통, 굵게 스타일(2)이면 굵다. 기울임(1)·취소선(3)·밑줄(4)이 사양에 반영된다.
    assert.equal(provider.get(5, 0).spec.bold, false);
    assert.equal(provider.get(5, 2).spec.bold, true);
    assert.equal(provider.get(5, 1).spec.italic, true);
    assert.equal(provider.get(5, 3).spec.strikeout, true);
    assert.equal(provider.get(5, 4).spec.underline, true);
    // 기준 글꼴이 없으면 슬롯 높이와 em 으로 추정한다. 비어 있는 슬롯 2 는 만들지 않는다.
    assert.equal(provider.get(3, 0).height, 20);
    assert.equal(provider.get(3, 0).ascent, Math.round(Math.round(20 * EM_PER_SLOT_HEIGHT) * 0.85));
    assert.throws(() => provider.get(2, 0), RangeError);
    // 기준 글꼴이 있으면 줄 높이·기준선은 그 값이다(슬롯 3: 원본 Arial 20 은 높이 19·기준선 15 라 추정값과 다르다).
    const reference20 = BitmapFont.parse(menuFiles.get('d/!Arial.normal.20.700.chfnt'));
    const referenced = new UnicodeFontProvider(fakeRasterizer()).get(3, 0, reference20);
    assert.deepEqual([referenced.height, referenced.ascent, referenced.descent], [19, 15, 4]);
    assert.ok(provider.get(4, 0).spec.emSize > provider.get(3, 0).spec.emSize);
});

// 글꼴 묶음: 부호화 종류로 글꼴을 가른다. (2026-10-31 WB03-N)
test('FontSet: 유니코드 글꼴이 있으면 UI 문구를 UTF-16 으로 부호화하고 그 배열은 유니코드 글꼴로 잰다', () => {
    const context = createTextContext({ font: 0, color: 1 });
    // 영어: 원본 부호화와 원본 글꼴만 쓴다.
    const english = new FontSet(menuFiles);
    assert.ok(english.encode('Campaign') instanceof Uint8Array);
    assert.throws(() => english.unicodeFont(context), /유니코드 글꼴/);
    // 한국어: UI 문구는 Uint16Array 이고 글꼴·폭은 래스터라이저를 따르지만 Uint8Array 는 여전히 원본 글꼴이다.
    const fonts = koreanResources.fonts;
    const text = fonts.encode('캠페인');
    assert.ok(text instanceof Uint16Array);
    assert.equal(fonts.measure(text, context).width, 3 * EM);
    assert.equal(fonts.measure(text, context).height, 14, '높이는 슬롯 논리 높이');
    const original = encodeWindows1252('Campaign');
    assert.equal(fonts.measure(original, context).width, english.measure(original, context).width);
    assert.equal(fonts.drawFont(context, text), fonts.unicodeFont(context));
    assert.ok(fonts.drawFont(context, original) instanceof BitmapFont);
    // 유니코드 글꼴의 줄 높이·기준선은 같은 슬롯의 원본 비트맵 글꼴을 따른다(슬롯 3 은 높이 19·기준선 15).
    const slot3 = fonts.unicodeFont(createTextContext({ font: 3 }));
    assert.deepEqual([slot3.height, slot3.ascent], [19, 15]);
    // 줄 기준선 계산(metrics)은 같은 슬롯의 원본 비트맵 값이라 영어와 한국어가 같다.
    assert.deepEqual(fonts.metrics(context), english.metrics(context));
    // 그림자 오프셋은 유니코드 문구의 크기에도 더해진다.
    const shadowed = createTextContext({ flags: 2, font: 0, shadowX: 1, shadowY: 1 });
    assert.deepEqual(fonts.measure(text, shadowed), { width: 3 * EM + 1, height: 15 });
    // 없는 글꼴 조합(슬롯 1 에 굵게 스타일)은 null 이고 폭은 0 이다.
    const missing = createTextContext({ font: 1, style: 2 });
    assert.equal(fonts.unicodeFont(missing), null);
    assert.equal(fonts.measure(text, missing).width, 0);
});

// 글자 그리기: 유니코드 글리프가 영어 글리프와 같은 패스(그림자·본문)와 전진 규칙으로 그려진다. (2026-10-31 WB03-N)
test('drawGlyphText: 유니코드 글꼴로 그림자 패스 → 본문 패스를 그린다', () => {
    const font = koreanResources.fonts.unicodeFont(createTextContext({ font: 0 }));
    const surface = new IndexedSurface(40, 20);
    const context = createTextContext({ flags: 2, font: 0, color: 7, shadowColor: 3, shadowX: 1, shadowY: 1 });
    drawGlyphText(surface, font, encodeUtf16Units('가나'), 5, 2, context);
    // 첫 글자 그림은 (펜 + left 1, 줄 위 + top 2) 에서 시작한다: 본문 (6,4)·(8,4)·(7,5)·(8,5), 그림자는 한 칸 아래·오른쪽이다.
    const at = (x, y) => surface.pixels[y * 40 + x];
    assert.equal(at(6, 4), 7);
    assert.equal(at(8, 4), 7);
    assert.equal(at(7, 4), 0);
    assert.equal(at(8, 5), 7);
    assert.equal(at(9, 6), 3, '그림자(+1,+1)');
    // 둘째 글자는 전진 폭(em) 만큼 오른쪽이다.
    assert.equal(at(5 + EM + 1, 4), 7);
    assert.equal(at(5, 4), 0);
});

// 돌 버튼: 번역된 라벨은 표시 문구로 쓰고 영어 라벨은 키로 남는다. 한국어 라벨은 유니코드 글꼴로 잰다. (2026-10-31 WB03-N)
test('StoneButton: 표시 문구와 영어 키를 분리하고 한국어 라벨을 유니코드 글꼴로 잰다', () => {
    const fonts = koreanResources.fonts;
    const button = new StoneButton({ label: 'Campaign', command: { type: 'tell' }, display: korean.translate('Campaign'), encode: (text) => fonts.encode(text) });
    assert.equal(button.label, 'Campaign');
    assert.equal(button.text, '캠페인');
    assert.ok(button.labelBytes instanceof Uint16Array);
    assert.equal(button.labelFont(koreanResources.font, fonts), fonts.unicodeFont(createTextContext({ font: 0 })));
    button.fitToText(koreanResources.font, fonts);
    assert.equal(button.textWidth, 3 * EM);
    assert.equal(button.baseWidth, 3 * EM + 12, '글자 폭 + 12');
    assert.equal(button.baseHeight, 20, '높이 14 + 6');
    // 영어 버튼은 같은 자원에서도 원본 글꼴이다.
    const plain = new StoneButton({ label: 'Demo' });
    assert.equal(plain.text, 'Demo');
    assert.ok(plain.labelBytes instanceof Uint8Array);
    assert.equal(plain.labelFont(koreanResources.font, fonts), koreanResources.font);
    // 63개를 넘는 라벨은 자른다(원본 strncpy 63).
    assert.equal(new StoneButton({ label: 'x', display: '가'.repeat(80), encode: (text) => fonts.encode(text) }).labelBytes.length, 63);
});

// 한국어 메인 메뉴: 라벨이 번역되고, 모든 버튼이 가장 넓은 글자에 맞춰지며, 화면이 영어와 다르게 그려진다. (2026-10-31 WB03-N)
test('MainMenuScreen: 한국어 버튼 라벨·배치·그리기', () => {
    const menu = new MainMenuScreen({ resources: koreanResources });
    menu.resize(1024, 768);
    assert.deepEqual(menu.buttons.map((button) => button.text), ['캠페인', '데모', '도움말', '크레딧', '옵션']);
    assert.deepEqual(menu.buttons.map((button) => button.label), ['Campaign', 'Demo', 'Help', 'Credits', 'Options'], '명령 통지용 영어 라벨');
    // 가장 긴 라벨은 한글 세 글자이므로 모든 버튼 폭이 (3em + 12) 이다. 두 글자 라벨은 2em 이다.
    assert.deepEqual([...new Set(menu.buttons.map((button) => button.baseWidth))], [3 * EM + 12]);
    assert.deepEqual(menu.buttons.map((button) => button.textWidth), [3 * EM, 2 * EM, 3 * EM, 3 * EM, 2 * EM]);
    // 영어와 같은 줄 구성(윗줄 3개·아랫줄 2개)과 가운데 정렬을 유지한다.
    const englishMenu = new MainMenuScreen({ resources: englishResources });
    englishMenu.resize(1024, 768);
    assert.deepEqual(menu.buttons.map((button) => button.bounds().top), englishMenu.buttons.map((button) => button.bounds().top));
    const rowWidth = (buttons) => buttons[buttons.length - 1].bounds().right - buttons[0].bounds().left;
    assert.equal(menu.buttons[0].bounds().left, Math.trunc((1024 - rowWidth(menu.buttons.slice(0, 3))) / 2));
    // 그리기: 글자 픽셀이 버튼 안쪽에만 생긴다(한국어 글리프를 비운 같은 메뉴와 비교).
    const withText = new IndexedSurface(1024, 768);
    menu.render(withText);
    const blankRasterizer = { glyph: (code, spec) => ({ advance: code > 0x7f ? spec.emSize : Math.round(spec.emSize / 2), left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) }) };
    const blankResources = buildMenuResources(menuFiles, { translate: korean.translate, unicode: new UnicodeFontProvider(blankRasterizer) });
    const blankMenu = new MainMenuScreen({ resources: blankResources });
    blankMenu.resize(1024, 768);
    const withoutText = new IndexedSurface(1024, 768);
    blankMenu.render(withoutText);
    let changed = 0;
    const rects = menu.buttons.map((button) => button.bounds());
    // 두 화면이 다른 픽셀을 세고, 모두 어떤 버튼 안에 있는지 확인한다.
    for (let y = 0; y < 768; y += 1) {
        for (let x = 0; x < 1024; x += 1) {
            if (withText.pixels[y * 1024 + x] === withoutText.pixels[y * 1024 + x]) continue;
            changed += 1;
            assert.ok(rects.some((rect) => x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom), `버튼 밖 변경 (${x},${y})`);
        }
    }
    assert.ok(changed > 5 * 4, `글자 픽셀 ${changed}`);
});

// 펼침 메뉴 번역: 라벨만 바뀌고 명령·구분선·체크·하위 목록 구조는 같다. (2026-10-31 WB03-N)
test('buildPopupItems: translate 로 Options·Help 라벨을 번역하고 구조는 보존한다', () => {
    const settings = new Settings();
    const plain = buildPopupItems('options', settings);
    const translated = buildPopupItems('options', settings, { translate: korean.translate });
    assert.equal(translated.length, plain.length);
    translated.forEach((item, index) => {
        assert.equal(item.kind, plain[index].kind);
        assert.deepEqual(item.command, plain[index].command);
        assert.equal(item.checked, plain[index].checked);
    });
    assert.deepEqual(translated.filter((item) => item.kind !== 'separator').map((item) => item.label), [
        '소리 켜기', '음악 재생', '바람 소리', '스피커 좌우 바꾸기', '효과음 음량', '음악 음량', '전체 화면 가장자리 스크롤', '자동 데모', '시작 시 팁 표시',
    ]);
    // 하위 음량 목록도 번역되고 현재 단계 체크는 그대로다.
    const volume = translated.find((item) => item.kind === 'submenu');
    assert.deepEqual(volume.items.map((item) => item.label), ['음량 1', '음량 2', '음량 3', '음량 4', '음량 5']);
    assert.deepEqual(volume.items.map((item) => item.checked), plain.find((item) => item.kind === 'submenu').items.map((item) => item.checked));
    assert.deepEqual(buildPopupItems('help', settings, { translate: korean.translate }).map((item) => item.label), ['일반 도움말 - ~iF1~.', '버전']);
    // 원문은 바뀌지 않고, 모르는 대상은 null 이며, translate 가 없으면 영어 그대로다.
    assert.equal(plain[0].label, 'Sound On');
    assert.equal(buildPopupItems('nothing', settings, { translate: korean.translate }), null);
    assert.equal(buildPopupItems('help', settings)[0].label, 'General Help - ~iF1~.');
});

// 한국어 펼침 메뉴가 실제로 열리고 그려진다: 항목 폭은 유니코드 글꼴 폭을 따른다. (2026-10-31 WB03-N)
test('PopupMenu: 한국어 Options 메뉴 크기·그리기·명령 실행', () => {
    const settings = new Settings();
    const commands = [];
    const open = (resources, items) => {
        const popup = new PopupMenu({ resources, screen: { width: 1024, height: 768 }, onCommand: (command) => commands.push(command) });
        popup.open(items, { x: 100, y: 100 });
        return popup;
    };
    const popup = open(koreanResources, buildPopupItems('options', settings, { translate: korean.translate }));
    const list = popup.levels[0].list;
    assert.equal(list.items.length, buildPopupItems('options', settings).length);
    assert.ok(list.items.filter((item) => item.kind !== 'separator').every((item) => item.fragments.some((fragment) => fragment.bytes instanceof Uint16Array)));
    // 가장 긴 항목 '전체 화면 가장자리 스크롤'의 글자 폭(한글 11자 + 공백 3개)에 좌우 여백 30 을 더한 값 이상이다.
    const longest = koreanResources.fonts.measure(koreanResources.fonts.encode('전체 화면 가장자리 스크롤'), createTextContext({ flags: 2, font: 0, shadowX: 1, shadowY: 0 })).width;
    assert.equal(longest, 11 * EM + 3 * Math.round(EM / 2) + 1);
    assert.ok(list.baseWidth >= longest + 30, `폭 ${list.baseWidth}`);
    const surface = new IndexedSurface(1024, 768);
    popup.draw(surface);
    const before = surface.pixels.slice();
    assert.ok(before.some((value) => value !== 0));
    // 첫 행(소리 켜기) 가운데를 누르면 영어와 같은 명령이 실행된다.
    const bounds = list.bounds();
    popup.pointerDown(bounds.left + 20, bounds.top + 8);
    assert.deepEqual(commands.map((command) => [command.name, command.argument]), [['Toggle', 'sound']]);
});

// 한국어 자산 묶음 조립: 래스터라이저에 woff2 바이트가 그대로 가고, 파일이 없으면 오류다. (2026-10-31 WB03-N)
test('buildKoreanSupport: 래스터라이저에 보통체·굵은체 woff2 를 넘기고 누락을 알린다', async () => {
    let received = null;
    const support = await buildKoreanSupport(koreanFiles, { createRasterizer: async (faces) => (received = faces, fakeRasterizer()) });
    assert.deepEqual([...received.regular], [...koreanFiles.get(KOREAN_ASSET_PATHS.regular)]);
    assert.deepEqual([...received.bold], [...koreanFiles.get(KOREAN_ASSET_PATHS.bold)]);
    assert.notDeepEqual([...received.regular.subarray(0, 64)], [...received.bold.subarray(0, 64)]);
    assert.ok(support.unicode instanceof UnicodeFontProvider);
    assert.ok(support.translations.size >= 30);
    for (const key of ['regular', 'bold', 'translations']) {
        const partial = new Map(koreanFiles);
        partial.delete(KOREAN_ASSET_PATHS[key]);
        await assert.rejects(() => buildKoreanSupport(partial, { createRasterizer: () => fakeRasterizer() }), new RegExp(KOREAN_ASSET_PATHS[key].replace('.', '\\.')));
    }
    // 읽기 쉬운지 확인용: 실제 글꼴 파일은 1MB 이상이다.
    assert.ok(readData(KOREAN_ASSET_PATHS.regular).length > 1_000_000);
});

// 영어 자원은 한국어 지원 없이도 그대로 동작한다(기존 동작 유지). (2026-10-31 WB03-N)
test('영어 자원: 번역은 원문 그대로이고 글꼴 묶음은 유니코드 글꼴이 없다', () => {
    assert.equal(englishResources.translate('Campaign'), 'Campaign');
    assert.equal(englishResources.fonts.unicode, null);
    const menu = new MainMenuScreen({ resources: englishResources });
    menu.resize(1024, 768);
    assert.deepEqual(menu.buttons.map((button) => button.text), ['Campaign', 'Demo', 'Help', 'Credits', 'Options']);
    assert.ok(menu.buttons.every((button) => button.labelBytes instanceof Uint8Array));
});

// ----- 브라우저 연결부(Canvas 래스터라이저·Netstorm 적재 흐름)는 가짜 DOM 으로 검사한다. -----

// 가짜 문서이다. FontFace·document.fonts·canvas 2D 문맥을 흉내 낸다. fillText 는 펜 위치 (x+1, y-4) 에 3×4 불투명 상자를 그리고,
// measureText 는 글자 크기(px)에 비례한 폭(한글은 em, 그 밖은 em/2)을 돌려준다. 공백은 아무것도 그리지 않는다. (2026-10-31 WB03-N)
function fakeDocument() {
    const faces = [];
    const added = [];
    class FakeFontFace {
        constructor(family, data, descriptors) {
            this.family = family;
            this.data = data;
            this.descriptors = descriptors;
            faces.push(this);
        }

        load() {
            this.loaded = true;
            return Promise.resolve(this);
        }
    }
    const emOf = (font) => Number(/(\d+)px/.exec(font)[1]);
    const canvas = { width: 300, height: 300 };
    let pixels = new Uint8ClampedArray(canvas.width * canvas.height * 4);
    const calls = [];
    const context = {
        font: '',
        measureText: (character) => ({ width: character.codePointAt(0) > 0x7f ? emOf(context.font) : emOf(context.font) / 2 }),
        clearRect: () => { pixels = new Uint8ClampedArray(canvas.width * canvas.height * 4); },
        fillText: (character, x, y) => {
            calls.push({ character, x, y, font: context.font });
            if (character === ' ') return;
            // 펜 위치 기준 (1, -4) 에서 3×4 상자를 불투명하게 칠한다.
            for (let row = 0; row < 4; row += 1) for (let column = 0; column < 3; column += 1) pixels[((y - 4 + row) * canvas.width + (x + 1 + column)) * 4 + 3] = 255;
        },
        getImageData: (x, y, width, height) => {
            const data = new Uint8ClampedArray(width * height * 4);
            for (let row = 0; row < height; row += 1) data.set(pixels.subarray(((y + row) * canvas.width + x) * 4, ((y + row) * canvas.width + x + width) * 4), row * width * 4);
            return { data };
        },
    };
    canvas.getContext = () => context;
    const document = { defaultView: { FontFace: FakeFontFace }, fonts: { add: (face) => added.push(face), delete: (face) => added.splice(added.indexOf(face), 1) }, createElement: () => canvas };
    return { document, faces, added, calls };
}

// 래스터라이저: woff2 두 파일을 보통체(400)·굵은체(700)로 등록하고, 글자 그림을 잘라 펜 기준 위치로 돌려준다. (2026-10-31 WB03-N)
test('createCanvasGlyphRasterizer: 글꼴 등록·글자 그림 자르기·위치·전진 폭·정리', async () => {
    const env = fakeDocument();
    const rasterizer = await createCanvasGlyphRasterizer({ document: env.document, regular: Uint8Array.of(1), bold: Uint8Array.of(2) });
    assert.deepEqual(env.faces.map((face) => [face.family, face.descriptors.weight, face.data[0], face.loaded]), [['D2Coding', '400', 1, true], ['D2Coding', '700', 2, true]]);
    assert.equal(env.added.length, 2);
    const spec = { emSize: 13, bold: false, italic: false, ascent: 11, height: 14 };
    const hangul = rasterizer.glyph(0xac00, spec);
    // 한글은 em 만큼 전진한다. 상자는 펜 기준 (1, -4) 에서 3×4 이고, 기준선은 줄 맨 위에서 ascent(11) 아래이므로 줄 맨 위 기준 상자 위치는 y = 11 - 4 = 7 이다.
    assert.deepEqual({ advance: hangul.advance, left: hangul.left, top: hangul.top, width: hangul.width, height: hangul.height }, { advance: 13, left: 1, top: 7, width: 3, height: 4 });
    assert.ok([...hangul.coverage].every((value) => value === 255) && hangul.coverage.length === 12);
    // 영문은 em 의 절반(6.5 → 7)으로 전진한다. 굵게·기울임은 글꼴 문자열에 반영된다.
    const latin = rasterizer.glyph(0x41, { ...spec, bold: true, italic: true });
    assert.equal(latin.advance, 7);
    assert.equal(env.calls[env.calls.length - 1].font, 'italic 700 13px "D2Coding"');
    assert.equal(env.calls[0].font, '400 13px "D2Coding"');
    // 공백은 그림이 없고 전진 폭만 있다. 이전 글자의 그림이 남아 있으면 안 된다.
    const space = rasterizer.glyph(0x20, spec);
    assert.deepEqual({ advance: space.advance, width: space.width, height: space.height, length: space.coverage.length }, { advance: 7, width: 0, height: 0, length: 0 });
    // 정리하면 등록한 글꼴이 문서에서 빠진다.
    rasterizer.dispose();
    assert.equal(env.added.length, 0);
    // FontFace 를 쓸 수 없는 환경은 분명한 오류다.
    await assert.rejects(() => createCanvasGlyphRasterizer({ document: { defaultView: {} }, regular: new Uint8Array(1), bold: new Uint8Array(1) }), /FontFace/);
});

// Netstorm 연결: 한국어 묶음 크기 계산·적재 성공/실패 대체, 한국어 펼침 메뉴. (2026-10-31 WB03-N)
test('Netstorm 연결: 한국어 묶음 적재와 실패 시 영어로 계속, 펼침 메뉴 번역', async () => {
    const app = new Netstorm();
    app._root = { ownerDocument: fakeDocument().document };
    assert.equal(app._bundleBytes({ entries: () => [{ size: 5 }, { size: 7 }] }, KOREAN_BUNDLE), 12);
    assert.equal(app._bundleBytes({ entries: () => [] }, KOREAN_BUNDLE), 0);
    // 성공: 진행률은 메뉴 묶음 크기 뒤에 이어지고 번역·유니코드 글꼴이 만들어진다.
    const loader = { loadBundle: async (name, onProgress) => { assert.equal(name, KOREAN_BUNDLE); onProgress(1000, 4000); return koreanFiles; } };
    const support = await app._loadKorean(loader, 500, 4000);
    assert.equal(support.translate('Options'), '옵션');
    assert.ok(support.unicode instanceof UnicodeFontProvider);
    assert.deepEqual(app._loadProgress, { loaded: 1500, total: 4500 });
    // 실패(받기 오류·글꼴 등록 오류)는 경고만 남기고 null 이라 영어로 계속한다.
    const warnings = [];
    const originalWarn = console.warn;
    console.warn = (...args) => warnings.push(args);
    try {
        assert.equal(await app._loadKorean({ loadBundle: async () => { throw new Error('network'); } }, 0, 0), null);
        const noFont = new Netstorm();
        noFont._root = { ownerDocument: { defaultView: {} } };
        assert.equal(await noFont._loadKorean(loader, 0, 0), null);
    } finally {
        console.warn = originalWarn;
    }
    assert.equal(warnings.length, 2);
    // 한국어 펼침 메뉴: Options 를 열면 라벨이 한국어이고 영어 자원에서는 영어다.
    for (const [resources, first] of [[koreanResources, '소리 켜기'], [englishResources, 'Sound On']]) {
        const screen = new MainMenuScreen({ resources });
        screen.resize(1024, 768);
        const menuApp = new Netstorm();
        Object.assign(menuApp, { _mainMenu: screen, _resources: resources, _settings: new Settings(), _input: { pointer: { x: 0, y: 0 } } });
        assert.equal(menuApp._openPopup('options', { x: 100, y: 100 }), true);
        assert.equal(screen.popup.levels[0].list.items[0].label, first);
        assert.equal(menuApp._openPopup('help', { x: 100, y: 100 }), true);
        assert.equal(screen.popup.levels[0].list.items[0].label, resources.translate('General Help - ~iF1~.'));
    }
});

// 디스크의 www/data 를 서빙하는 가짜 fetch 이다. 요청 주소를 urls 에 기록하고, failPattern 에 맞는 주소는 404 로 답한다. (2026-10-31 WB03-N)
function diskFetch(urls, failPattern = null) {
    return async (url) => {
        urls.push(String(url));
        const relative = decodeURIComponent(new URL(url).pathname.replace(/^\/data\//, ''));
        if (failPattern && failPattern.test(relative)) return { ok: false, status: 404 };
        const bytes = fs.readFileSync(path.join(DATA_ROOT, ...relative.split('/')));
        return {
            ok: true,
            status: 200,
            json: async () => JSON.parse(bytes.toString('utf8')),
            arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
        };
    };
}

// 앱의 적재 흐름(_load): 한국어면 메뉴 묶음 뒤에 한국어 묶음을 받아 라벨을 번역하고, 영어면 한국어 묶음을 받지 않으며,
// 한국어 묶음을 받지 못해도 영어 메뉴로 클릭 대기까지 간다. 진행률은 두 묶음의 합계를 가리킨다. (2026-10-31 WB03-N)
test('Netstorm 적재: 언어별 묶음 선택과 한국어 실패 대체', async () => {
    const sizeOf = (bundle) => manifest.files.filter((entry) => entry.bundle === bundle).reduce((sum, entry) => sum + entry.size, 0);
    const build = (language, fetch) => {
        const app = new Netstorm({ language, fetch, audio: false, assetBase: 'http://localhost/data/' });
        app._root = { ownerDocument: { ...fakeDocument().document, baseURI: 'http://localhost/' } };
        app._settings = new Settings();
        app._progress = { snapshot: () => ({}) };
        return app;
    };
    // 한국어: 메뉴·한국어 묶음을 모두 받고 번역·유니코드 글꼴이 갖춰진 자원이 만들어진다.
    const koreanUrls = [];
    const korean = build('korean', diskFetch(koreanUrls));
    await korean._load();
    assert.equal(korean._state, 'clickToStart');
    assert.equal(korean._resources.translate('Options'), '옵션');
    assert.ok(korean._resources.fonts.unicode instanceof UnicodeFontProvider);
    assert.ok(koreanUrls.some((url) => url.endsWith('/fonts/D2Coding.woff2')) && koreanUrls.some((url) => url.endsWith('/d/xlat.korean')));
    assert.deepEqual(korean._loadProgress, { loaded: sizeOf('menu') + sizeOf(KOREAN_BUNDLE), total: sizeOf('menu') + sizeOf(KOREAN_BUNDLE) });
    // 한국어 언어 자료(tell·offical*·config)가 게임 데이터에 합쳐져 한국어 Tell 본문과 용어가 조회된다(2026-10-31 WB03-O).
    assert.ok(korean._gameData.tellSection('Welcome').includes('님버스'));
    assert.equal(korean._gameData.get('OkButton'), '$Button=확인,DoNothing,0');
    assert.equal(korean._gameData.findFiles('\\D\\offical*.korean').length, 8);
    // 영어: 한국어 묶음을 받지 않는다.
    const englishUrls = [];
    const english = build('english', diskFetch(englishUrls));
    await english._load();
    assert.equal(english._state, 'clickToStart');
    assert.equal(english._resources.translate('Options'), 'Options');
    assert.equal(english._resources.fonts.unicode, null);
    assert.ok(!englishUrls.some((url) => /D2Coding|xlat\.korean/.test(url)));
    assert.ok(english._gameData.tellSection('Welcome').includes('Nimbus'));
    assert.deepEqual(english._loadProgress, { loaded: sizeOf('menu'), total: sizeOf('menu') });
    // 한국어 글꼴을 받지 못하면 경고만 남기고 영어 메뉴로 클릭 대기에 간다(게임이 멈추지 않는다).
    const originalWarn = console.warn;
    const warnings = [];
    console.warn = (...args) => warnings.push(args);
    try {
        const failed = build('korean', diskFetch([], /D2Coding\.woff2$/));
        await failed._load();
        assert.equal(failed._state, 'clickToStart');
        assert.equal(failed._resources.translate('Options'), 'Options');
        assert.equal(failed._resources.fonts.unicode, null);
        // 한국어 묶음이 없으면 언어 자료도 없어 Tell 본문은 영어 파일로 대체된다.
        assert.ok(failed._gameData.tellSection('Welcome').includes('Nimbus'));
    } finally {
        console.warn = originalWarn;
    }
    assert.equal(warnings.length, 1);
});
