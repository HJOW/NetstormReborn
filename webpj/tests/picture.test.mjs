// 도움말 그림(아이콘) 지원을 검사한다: `.type` 문법 해석(클러스터·특수 프레임·글자 구간·기준점 비율), 116개 실제 타입 표와 셰이프 블록의 대응,
// 그림 명령 `~[I타입.프레임식]` 의 원본 규칙(타입 이름·번호, `*`·숫자·글자 프레임, 범위 밖 번호, 기준점 보정), 그리고 영어·한국어 도움말의 모든 그림 명령이
// 지원 가능한 그림으로 해석되는지 확인한다. 원본 규칙은 cpppj `StyleTextMarkup.cpp`·`RiftType.cpp`·`Markup.cpp` 를 근거로 한다.
// (2026-11-01 WB03-T: 도움말 그림 지원 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { GameData } from '../www/assets/game-data.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { KOREAN_BUNDLE, buildKoreanSupport } from '../www/assets/korean-assets.js';
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER, TypeTable, parseTypeDefinition } from '../www/assets/type-table.js';
import { EMPTY_PICTURE, createPictureResolver, parseCrtInteger } from '../www/assets/picture-resolver.js';
import { createTextContext } from '../www/render/glyph-text.js';
import { parseStyleText } from '../www/ui/style-text.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';
import { createHelpWindow } from '../www/ui/help-window.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';

// 공용 자료이다: 아카이브·셰이프·타입 표·그림 해석기(검사마다 공유). (2026-11-01 WB03-T)
const archive = new TaffArchive(readData('netstorm.tarc'));
const shapes = new ShapeDatabase(readData('d/_shapes.shp'));
const tableErrors = [];
const table = TypeTable.fromArchive(archive, decodeOriginalText, (name, error) => tableErrors.push(`${name}: ${error.message}`));
const resolve = createPictureResolver({ shapes, getTable: () => table });

// 로딩 순서 116개는 cpppj 의 로딩 순서 표와 같다(cpppj 소스가 없으면 건너뛴다). (2026-11-01 WB03-T)
test('TYPE_LOAD_ORDER: 116개이고 cpppj TypeLoadOrder.cpp 와 같다', (t) => {
    assert.equal(TYPE_LOAD_ORDER.length, 116);
    assert.equal(new Set(TYPE_LOAD_ORDER.map((name) => name.toLowerCase())).size, 116, '이름 중복 없음');
    const source = path.join(REPO_ROOT, 'cpppj', 'src', 'o', 'TypeLoadOrder.cpp');
    if (!fs.existsSync(source)) return t.skip('cpppj 소스가 없음');
    const text = fs.readFileSync(source, 'utf8');
    const block = text.slice(text.indexOf('kAssetTypes{'), text.indexOf('};', text.indexOf('kAssetTypes{')));
    const names = [...block.matchAll(/"([^"]+)"/g)].map((match) => match[1]);
    assert.deepEqual(names, [...TYPE_LOAD_ORDER]);
    return undefined;
});

// `.type` 문법: 클러스터 이름 분해·플래그 비트·특수 프레임·속성·주석·숫자 형식을 합성 자료로 검사한다. (2026-11-01 WB03-T)
test('parseTypeDefinition: 클러스터 코드·특수 프레임·속성 해석', () => {
    const text = [
        'typename Demo constructor',
        'typeflags shadow dontSave;',
        '{',
        '    description = "데모";   // 주석',
        '    foot_x = 02;',
        '    hotFootRatioX = 0.0625;',
        '    hotFootRatioY = 0.5;',
        '    HOTFOOTRATIOX = 0.125;',
        '}',
        '// 클러스터',
        'A00 : : "a.gif" #00 : "b.gif" #01 ;',
        'B01 : default help : "a.gif" #02 ;',
        'B02 : gumpframe lit cracked bogus : "a.gif" #03 ;',
        'C09 : baseframe : "a.gif" #08 ;',
        'AB07 : : "a.gif" #04 ;',
        'D00 : help : "a.gif" #05 ;',
        'E200 : default : "a.gif" #06 ;',
        'F300 : : "a.gif" #07 ;',
    ].join('\r\n');
    const definition = parseTypeDefinition(text);
    assert.equal(definition.name, 'Demo');
    assert.deepEqual(definition.constructorWords, ['constructor']);
    assert.deepEqual(definition.flags, ['shadow', 'dontSave']);
    assert.equal(definition.properties.length, 5);
    assert.equal(definition.clusters.length, 8);
    assert.deepEqual(definition.clusters.map((cluster) => cluster.imageCount), [2, 1, 1, 1, 1, 1, 1, 1]);
    // 프레임 코드: 한 글자 이름의 변형은 'P', 두 글자 이름은 둘째 글자, 번호는 하위 8비트, 플래그는 알려진 비트만.
    assert.deepEqual(definition.clusters[0].code, { side: 0x41, variant: 0x50, number: 0, flags: 0 });
    assert.deepEqual(definition.clusters[4].code, { side: 0x41, variant: 0x42, number: 7, flags: 0 });
    assert.equal(definition.clusters[2].code.flags, 0x08 | 0x20, 'lit | cracked, 모르는 단어는 무시');
    // 번호는 하위 8비트(char 저장)이다: 200 은 그대로, 300 은 44.
    assert.deepEqual([definition.clusters[6].code.number, definition.clusters[7].code.number], [200, 44]);
    // 특수 프레임: default 는 마지막 것이 이기고, help 가 한 번이라도 나오면 그 뒤의 default 는 도움말 프레임을 바꾸지 않는다(마지막 help 가 이김). gumpframe·baseframe.
    assert.equal(definition.defaultFrame, 6);
    assert.equal(definition.helpFrame, 5);
    assert.equal(definition.gumpFrame, 2);
    assert.equal(definition.baseFrame, 3);
    // 표: 글자 구간 첫 번호·글자 수·기준점(같은 속성은 마지막 값, 비율 × 16·11 을 0쪽으로 버림).
    const definitions = TYPE_LOAD_ORDER.map((name, index) => (index === 0 ? definition : null));
    const view = new TypeTable(definitions).view(FIRST_ASSET_TYPE_NUMBER);
    assert.equal(view.frameCount, 8);
    assert.deepEqual(view.first.slice(0, 7), [0, 1, 3, 5, 6, 7, -1]);
    assert.equal(view.letterKinds, 6);
    assert.deepEqual([view.hotspotX, view.hotspotY], [2, 5]);
    // 정의가 없는 타입은 프레임이 없다.
    assert.equal(new TypeTable(definitions).view(FIRST_ASSET_TYPE_NUMBER + 1).frameCount, 0);
});

// 잘못된 문법은 오류이다: typename 없음·소문자로 시작하는 클러스터·닫히지 않은 문자열·속성 값 없음·클러스터 구분자 없음. (2026-11-01 WB03-T)
test('parseTypeDefinition: 잘못된 문법은 오류', () => {
    assert.throws(() => parseTypeDefinition('{ }'));
    assert.throws(() => parseTypeDefinition('typename X { }\r\na00 : : "a.gif" #0 ;'));
    assert.throws(() => parseTypeDefinition('typename X { description = "열림 }'));
    assert.throws(() => parseTypeDefinition('typename X { foot_x = ; }'));
    assert.throws(() => parseTypeDefinition('typename X { }\r\nA00 : : "a.gif" 0 ;'));
    assert.doesNotThrow(() => parseTypeDefinition('typename X { }'), '클러스터가 없는 정의도 읽힌다');
});

// 실제 116개 `.type` 은 모두 읽히고 클러스터 수 × 레이어 수 = 셰이프 블록 프레임 수이다(원본 manabolt 만 예외). (2026-11-01 WB03-T)
test('실제 타입 표: 116개 모두 읽히고 셰이프 블록과 대응한다', () => {
    assert.deepEqual(tableErrors, []);
    assert.equal(shapes.blocks.length, 116);
    const problems = [];
    // 로딩 순서마다 정의를 읽어 블록 프레임 수와 대조한다.
    TYPE_LOAD_ORDER.forEach((name, index) => {
        const definition = parseTypeDefinition(decodeOriginalText(archive.read(`\\d\\${name}.type`)));
        const layers = Math.max(...definition.clusters.map((cluster) => cluster.imageCount));
        const frames = shapes.blocks[index].frames.length;
        if (frames !== definition.clusters.length * layers && name !== 'manabolt') problems.push(`${name}: 클러스터 ${definition.clusters.length} × 레이어 ${layers} ≠ 프레임 ${frames}`);
        assert.equal(table.view(FIRST_ASSET_TYPE_NUMBER + index).frameCount, definition.clusters.length, name);
        assert.equal(table.find(name), FIRST_ASSET_TYPE_NUMBER + index, name);
    });
    assert.deepEqual(problems, []);
});

// 타입 표 조회: 번호 범위·이름 대소문자·내장 타입. (2026-11-01 WB03-T)
test('TypeTable: 번호 범위와 이름 조회', () => {
    assert.equal(PATCH_TYPE_COUNT, 188);
    assert.equal(table.find('FORTGUMP'), 92);
    assert.equal(table.find('fortGump'), 92);
    assert.equal(table.find('nothing'), -1);
    assert.equal(table.find(''), -1);
    // 내장 타입(70 미만)과 뒤쪽 여분(186~187)은 프레임이 없는 빈 보기이고, 표 밖 번호는 null 이다.
    for (const number of [0, 5, 69, 186, 187]) assert.deepEqual([table.view(number).frameCount, table.view(number).block], [0, -1], String(number));
    for (const number of [-1, 188, 1000, 1.5]) assert.equal(table.view(number), null, String(number));
});

// 실제 타입의 특수 프레임·글자 구간·기준점 값이다(원본 `.type` 에서 읽은 값을 고정, 도움말 그림이 기대는 값). (2026-11-01 WB03-T)
test('실제 타입 표: 도움말 그림이 쓰는 타입의 특수 프레임·글자 구간·기준점', () => {
    const view = (name) => table.view(table.find(name));
    // fortGump: 블록 22, 81프레임, gumpframe 20, 글자 12종이며 L 구간은 62 에서 시작한다.
    assert.deepEqual([view('fortGump').block, view('fortGump').frameCount, view('fortGump').gumpFrame, view('fortGump').letterKinds, view('fortGump').first[11]], [22, 81, 20, 12, 62]);
    // windVortex: default 는 B01(13번째), gumpframe 없음(-1 → 기본 프레임으로 돌아간다).
    assert.deepEqual([view('windVortex').defaultFrame, view('windVortex').gumpFrame, view('windVortex').frameCount], [13, -1, 21]);
    // mana: 8프레임뿐이라 번호 8 이상은 그림이 없다.
    assert.deepEqual([view('mana').frameCount, view('mana').block], [8, 33]);
    // icon: 글자 A·B·C·D 구간(첫 번호 0·49·59·73), 118프레임.
    assert.deepEqual([view('icon').frameCount, view('icon').letterKinds, ...view('icon').first.slice(0, 4)], [118, 4, 0, 49, 59, 73]);
    // 기준점 비율: geyser 는 (3, 2), sunCannon 은 (0, 6) 이고 나머지 도움말 타입은 0 이다.
    assert.deepEqual([view('geyser').hotspotX, view('geyser').hotspotY], [3, 2]);
    assert.deepEqual([view('sunCannon').hotspotX, view('sunCannon').hotspotY], [0, 6]);
    assert.deepEqual([view('windVortex').hotspotX, view('windVortex').hotspotY], [0, 0]);
});

// CRT 정수 변환: 공백·부호·숫자 구간·숫자 없음·32비트 감김. (2026-11-01 WB03-T)
test('parseCrtInteger: 원본 정수 변환 규칙', () => {
    assert.equal(parseCrtInteger('12'), 12);
    assert.equal(parseCrtInteger(' \t 12ab'), 12);
    assert.equal(parseCrtInteger('-7'), -7);
    assert.equal(parseCrtInteger('+3'), 3);
    assert.equal(parseCrtInteger('x1'), 0);
    assert.equal(parseCrtInteger(''), 0);
    assert.equal(parseCrtInteger('-'), 0);
    assert.equal(parseCrtInteger('99999999999'), 1215752191, '32비트로 감김');
});

// 그림 해석: 타입 이름·번호, 프레임 식(`*`·숫자·글자), 범위 밖 번호, 기준점 보정, 지원하지 않는 형식. (2026-11-01 WB03-T)
test('그림 해석기: 원본 StyleText 그림 규칙', () => {
    const shape = (picture) => [picture.width, picture.height, picture.hotspotX, picture.hotspotY];
    const metricsOf = (block, frame) => {
        const metrics = shapes.squidMetrics(block, frame);
        return [metrics.width, metrics.height, metrics.hotspotX, metrics.hotspotY];
    };
    // 숫자 프레임: fortGump 프레임 3(블록 22).
    assert.deepEqual(shape(resolve('fortgump', '3')), metricsOf(22, 3));
    // 타입 번호로도 같다(92 = fortGump, 93 = icon 이므로 `93.113` 은 icon 의 113번 프레임).
    assert.deepEqual(shape(resolve('92', '3')), metricsOf(22, 3));
    assert.deepEqual(shape(resolve('93', '113')), metricsOf(23, 113));
    // 글자 프레임: 글자 구간 첫 번호 + 숫자(셰이프 해독 캐시의 같은 그림 객체인지로 인접 프레임과 구별한다).
    assert.deepEqual(shape(resolve('fortgump', 'L6')), metricsOf(22, 62 + 6));
    assert.equal(resolve('fortgump', 'L6').image, shapes.decode(22, 62 + 6));
    assert.equal(resolve('fortgump', 'L0').image, shapes.decode(22, 62));
    assert.equal(resolve('fortgump', 'A3').image, shapes.decode(22, 3));
    assert.equal(resolve('fortgump', 'B2').image, shapes.decode(22, 5 + 2));
    assert.deepEqual(shape(resolve('icon', 'B0')), metricsOf(23, 49));
    assert.equal(resolve('icon', 'B1').image, shapes.decode(23, 50));
    // 글자 구간 수(letterKinds)를 넘는 글자·소문자·부호 있는 수는 글자로 보지 않고 숫자로 읽는다(숫자가 없으면 0 → 프레임 0).
    assert.deepEqual(shape(resolve('fortgump', 'Z6')), metricsOf(22, 0));
    assert.deepEqual(shape(resolve('fortgump', 'l6')), metricsOf(22, 0));
    assert.deepEqual(shape(resolve('fortgump', 'M2')), metricsOf(22, 0), 'M 은 12번째 글자(letterKinds 12) 밖');
    assert.equal(resolve('fortgump', 'M5').image, shapes.decode(22, 0), '글자 구간 밖 글자는 숫자 0 + 프레임 0');
    assert.equal(resolve('fortgump', 'Z6').image, shapes.decode(22, 0));
    // `*`: gumpFrame(없으면 기본 프레임). 기준점 비율이 있는 타입은 타입 기준점을 하위 16비트로 뺀다(geyser (3,2)).
    assert.deepEqual(shape(resolve('fortgump', '*')), metricsOf(22, 20));
    assert.deepEqual(shape(resolve('windvortex', '*')), metricsOf(53, 13));
    const geyser = metricsOf(52, 49);
    assert.deepEqual(shape(resolve('geyser', '*')), [geyser[0], geyser[1], geyser[2] - 3, geyser[3] - 2]);
    // 음수·범위 밖 번호: 0 미만은 프레임 0, 프레임 수 이상은 그림이 없는 크기 0 조각이다.
    assert.deepEqual(shape(resolve('fortgump', '-5')), metricsOf(22, 0));
    assert.deepEqual(shape(resolve('mana', '7')), metricsOf(33, 7));
    assert.equal(resolve('mana', '8'), EMPTY_PICTURE);
    assert.equal(resolve('fortgump', '81'), EMPTY_PICTURE);
    assert.equal(resolve('fortgump', '99999'), EMPTY_PICTURE);
    // 내장 타입 번호는 무자산이라 빈 그림이고, 타입 표 밖 번호·모르는 이름은 지원하지 않는다.
    assert.equal(resolve('5', '0'), EMPTY_PICTURE);
    assert.equal(resolve('300', '0'), null);
    assert.equal(resolve('nothing', '1'), null);
    // 상자 맞춤(`#`)·색 치환(`&`)·너무 긴 식은 지원하지 않는다.
    assert.equal(resolve('fortgump', '#'), null);
    assert.equal(resolve('fortgump', '&'), null);
    assert.equal(resolve('fortgump', '3'.repeat(80)), null);
    // 그림 픽셀: 실제 이미지이며 기준점에 맞춘 상대 위치를 가진다.
    const picture = resolve('windvortex', '*');
    assert.ok(picture.image.width > 100 && picture.image.opacity.some((value) => value === 255));
    assert.deepEqual([picture.image.left, picture.image.top], [-130, -148]);
});

// 타입 표를 만들 수 없는 자원(아카이브 없음)에서는 fortGump 숫자 프레임만 해석하는 대체 경로가 동작한다. (2026-11-01 WB03-T)
test('그림 해석기: 아카이브가 없으면 fortGump 숫자 프레임만 해석', () => {
    const menuPaths = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8')).files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path);
    const files = readDataMap(menuPaths.filter((entry) => entry !== 'netstorm.tarc'));
    const resources = buildMenuResources(files);
    assert.deepEqual([resources.resolvePicture('fortgump', '3').width, resources.resolvePicture('fortgump', '3').height], [12, 15]);
    assert.equal(resources.resolvePicture('fortgump', 'L1'), null);
    assert.equal(resources.resolvePicture('mana', '2'), null);
});

// 영어·한국어 도움말의 모든 그림 명령이 지원 가능한 그림으로 해석된다(지원하지 않는 그림 경고 0). (2026-11-01 WB03-T)
test('도움말 그림: 영어·한국어 모든 주제의 그림 명령이 해석된다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const resources = buildMenuResources(files);
    const data = GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
    const englishHelp = decodeOriginalText(archive.read('\\d\\help.english'));
    const koreanHelp = decodeOriginalText(readData('d/help.korean'));
    const context = createTextContext({ flags: 0, font: 5, color: resources.colors.information, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 });
    const unsupported = [];
    let pictures = 0;
    let withArt = 0;
    // 두 언어 파일의 모든 앵커(주제)를 렌더해 그림 조각을 센다.
    for (const helpText of [englishHelp, koreanHelp]) {
        const names = [...helpText.matchAll(/<a name="([^"]*)">/g)].map((match) => match[1]);
        for (const topic of names) {
            const rendered = renderHelpTopic({ helpText, topic, substitute: (text) => data.substitute(text) });
            const fragments = parseStyleText(rendered.styleText, context, resources.colors, { resolvePicture: resources.resolvePicture, onUnsupported: (what, text) => unsupported.push(`${topic}: ${text}`) });
            for (const fragment of fragments) {
                if (!fragment.picture) continue;
                pictures += 1;
                if (fragment.picture.width > 0) withArt += 1;
            }
        }
    }
    assert.deepEqual(unsupported, []);
    assert.ok(pictures > 200, `그림 조각 수 ${pictures}`);
    assert.ok(withArt > 100 && withArt < pictures, `실제 그림이 있는 조각 ${withArt} / ${pictures}(mana.8~11 처럼 프레임 수 밖은 크기 0)`);
});

// 도움말 창: 그림이 많은 주제도 열리고 그림 조각이 줄 높이에 반영되며 그려진다(예외 없음). (2026-11-01 WB03-T)
test('도움말 창: 건물 그림이 있는 주제가 열려 그려진다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const resources = buildMenuResources(files);
    const data = GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
    const unsupported = [];
    const { window } = createHelpWindow({ gameData: data, resources, topic: 'themeHelp', screen: { width: 1024, height: 768 }, onUnsupported: (what) => unsupported.push(what) });
    assert.deepEqual(unsupported, []);
    const pictureFragments = window.body.softLines.flatMap((line) => line.fragments).filter((fragment) => fragment.picture);
    assert.ok(pictureFragments.length >= 10, `그림 조각 ${pictureFragments.length}`);
    assert.ok(pictureFragments.some((fragment) => fragment.picture.width > 100), '건물 그림(폭 100 이상)');
    // 큰 그림이 있는 줄은 그림 높이만큼 높다.
    const tallLine = window.body.softLines.find((line) => line.fragments.some((fragment) => fragment.picture && fragment.picture.height > 100));
    assert.ok(tallLine.height >= 100);
    const surface = new IndexedSurface(1024, 768);
    assert.doesNotThrow(() => window.draw(surface));
    assert.ok(surface.pixels.some((value) => value !== 0));
});

// 한국어 도움말 창에서도 그림이 같은 방식으로 나온다(글꼴 묶음·한국어 번역 자료와 함께). (2026-11-01 WB03-T)
test('도움말 창: 한국어에서도 에너지 기호·건물 그림 주제가 열린다', async () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const koreanFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE).map((entry) => entry.path));
    const rasterizer = { glyph: () => ({ advance: 6, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) }) };
    const korean = await buildKoreanSupport(koreanFiles, { createRasterizer: () => rasterizer });
    const resources = buildMenuResources(menuFiles, korean);
    const data = GameData.build({ files: new Map([...menuFiles, ...korean.languageFiles]), language: 'korean', options: { soundVolume: 3, musicVolume: 2 } });
    const unsupported = [];
    // 그림이 있는 한국어 주제 몇 개를 연다.
    for (const topic of ['influenceHelp', 'themeHelp', 'stormPowerHelp', 'moneyGumpHelp', 'bombHelp', 'playerOptionsHelp', 'challengeIslandType']) {
        const { window } = createHelpWindow({ gameData: data, resources, topic, screen: { width: 1024, height: 768 }, onUnsupported: (what) => unsupported.push(`${topic}: ${what}`) });
        const pictures = window.body.softLines.flatMap((line) => line.fragments).filter((fragment) => fragment.picture);
        assert.ok(pictures.length > 0, topic);
    }
    assert.deepEqual(unsupported, []);
});
