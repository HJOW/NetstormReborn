// 한국어 F1 도움말(WB03-P)을 검사한다: help.korean(번역한 주제만 담은 부분 파일)이 영어 help.english 의 첫 앵커와 같은 구조(태그·그림·링크 주소·조건·글자 모양 표시)인지,
// 도움말 창이 번역하지 않은 주제를 주제 단위로 영어 파일에서 대신 찾는지, 한국어 본문이 유니코드 글꼴 코드인지 확인한다. 번역 문구 자체의 옳고 그름은 검사하지 않는다(개발자 검수 대상).
// (2026-10-31 WB03-P)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { GameData } from '../www/assets/game-data.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { KOREAN_BUNDLE, buildKoreanSupport } from '../www/assets/korean-assets.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { createHelpWindow } from '../www/ui/help-window.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';

// 가짜 래스터라이저이다(한글은 em 폭, 그 밖은 절반 폭). (2026-10-31 WB03-P)
function fakeRasterizer() {
    return {
        glyph(code, spec) {
            const advance = code > 0x7f ? spec.emSize : Math.round(spec.emSize / 2);
            if (code === 0x20) return { advance, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) };
            return { advance, left: 1, top: 2, width: 3, height: 2, coverage: Uint8Array.from([255, 109, 110, 0, 200, 255]) };
        },
    };
}

// 메뉴·한국어 묶음 파일과 원본 아카이브, 자원이다(검사마다 공유). (2026-10-31 WB03-P)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const koreanFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE).map((entry) => entry.path));
const archive = new TaffArchive(readData('netstorm.tarc'));
const korean = await buildKoreanSupport(koreanFiles, { createRasterizer: () => fakeRasterizer() });
const koreanResources = buildMenuResources(menuFiles, korean);
const englishResources = buildMenuResources(menuFiles);

// 새 게임 데이터를 만든다. withFiles 가 false 면 한국어 언어 자료를 합치지 않는다. (2026-10-31 WB03-P)
function newData(language, { withFiles = true } = {}) {
    const files = language === 'korean' && withFiles ? new Map([...menuFiles, ...korean.languageFiles]) : menuFiles;
    return GameData.build({ files, language, options: { soundVolume: 3, musicVolume: 2 } });
}

const koreanHelp = decodeOriginalText(koreanFiles.get('d/help.korean'));
const englishHelp = decodeOriginalText(archive.read('\\d\\help.english'));

// 도움말 파일을 `<a name="…">` 앵커별 본문으로 나눈다. 이름(소문자) → 앵커 본문 목록(파일 순서)이다. (2026-10-31 WB03-P)
function parseTopics(text) {
    const starts = [...text.replaceAll('\r\n', '\n').matchAll(/<a name="([^"]*)">/g)];
    const normalized = text.replaceAll('\r\n', '\n');
    const topics = new Map();
    starts.forEach((match, index) => {
        const end = index + 1 < starts.length ? starts[index + 1].index : normalized.length;
        const key = match[1].toLowerCase();
        if (!topics.has(key)) topics.set(key, []);
        topics.get(key).push(normalized.slice(match.index, end));
    });
    return topics;
}

const koreanTopics = parseTopics(koreanHelp);
const englishTopics = parseTopics(englishHelp);

// 본문의 구조 토큰(태그·그림·링크 주소·조건, `~` 글자 모양 표시)을 정렬해 돌려준다. 번역하면서 낱말 순서가 바뀔 수 있어 순서는 보지 않는다. (2026-10-31 WB03-P)
function structureOf(topic) {
    return (topic.match(/<[^>]*>|~[^\s]/g) ?? []).map((token) => token.toLowerCase()).sort();
}

// 번역한 주제 목록(첫 앵커 이름, 파일 순서)이다. (2026-10-31 WB03-P)
const TRANSLATED = Object.freeze([
    'sacrificeOutline', 'immobileHelp', 'pickupHelp', 'moveyourpriestHelp', 'performsacrificeHelp', 'sphereHelp', 'deusphereHelp', 'serenisphereHelp',
    'pyrosphereHelp', 'bountyislandHelp', 'F1Help', 'interfaceHelp', 'vesselPriestHelp', 'themeHelp', 'unitHelp',
]);

// help.korean 형식: UTF-8(BOM 없음)·소스와 복사본 일치·번역 주제 15개·앵커 이름 중복 없음·영어에 있는 주제만. (2026-10-31 WB03-P)
test('help.korean: UTF-8·소스와 복사본 일치·번역 주제 목록', () => {
    const bytes = koreanFiles.get('d/help.korean');
    assert.notDeepEqual([...bytes.subarray(0, 3)], [0xef, 0xbb, 0xbf], 'BOM 없는 UTF-8');
    assert.deepEqual([...bytes], [...new Uint8Array(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'help.korean')))]);
    assert.deepEqual([...koreanTopics.keys()], TRANSLATED.map((name) => name.toLowerCase()));
    for (const [name, bodies] of koreanTopics) {
        assert.equal(bodies.length, 1, `${name} 앵커가 둘 이상`);
        assert.ok(englishTopics.has(name), `${name} 은 영어에 없음`);
        assert.ok(/[가-힣]/.test(bodies[0]), `${name} 에 한글이 없음`);
    }
});

// 번역하지 않고 남겨 둔 영어 낱말(네 글자 이상)은 화면에 보이는 고유 이름·키 이름·메뉴 이름뿐이다(문장이 영어로 남은 주제를 찾는다). (2026-10-31 WB03-P)
test('help.korean: 영어 문장이 남지 않았다(고유 이름·키·메뉴 이름만 허용)', () => {
    const allowed = new Set(['Start', "Battle'", 'NetStorm', 'Netstorm', 'Ticonderoga', 'Entertainment', 'Activision', 'Titanic', 'MicroSoft', 'IntellimouseTM', 'Escape', 'Shift', 'Pause', 'Ctrl', 'About', 'Salvage']);
    // 태그와 글자 모양 표시를 걷어 내고 남은 영어 낱말을 모은다.
    const visible = koreanHelp.slice(koreanHelp.indexOf('<a name=')).replace(/<[^>]*>/g, ' ').replace(/~[^\s]/g, ' ');
    const leftovers = [...new Set([...visible.matchAll(/[A-Za-z][A-Za-z']{3,}/g)].map((match) => match[0]))].filter((word) => !allowed.has(word));
    assert.deepEqual(leftovers, []);
});

// 번역한 주제마다 영어 첫 앵커와 태그·그림·링크 주소·조건·글자 모양 표시가 같다(번역하다 링크나 조건을 바꾸지 않았는지). (2026-10-31 WB03-P)
test('help.korean: 모든 주제가 영어 첫 앵커와 같은 구조(태그·링크·그림·조건)다', () => {
    const problems = [];
    // 번역한 주제마다 영어 첫 앵커와 구조 토큰을 비교한다.
    for (const [name, bodies] of koreanTopics) {
        const english = structureOf(englishTopics.get(name)[0]);
        const translated = structureOf(bodies[0]);
        if (JSON.stringify(english) !== JSON.stringify(translated)) {
            const missing = english.filter((token) => !translated.includes(token));
            const extra = translated.filter((token) => !english.includes(token));
            problems.push(`${name}: 영어에만 ${JSON.stringify(missing)} / 한국어에만 ${JSON.stringify(extra)} (수 ${english.length} ≠ ${translated.length})`);
        }
    }
    assert.deepEqual(problems, []);
});

// 렌더 결과: 번역한 주제는 영어와 같은 링크 목적지 목록(순서까지)을 가지고 한글 본문이며, 첫 앵커만 보인다(중복 앵커가 있는 주제도 한 번만). (2026-10-31 WB03-P)
test('renderHelpTopic: 한국어 주제의 링크 목적지가 영어와 같고 한 번만 나온다', () => {
    const data = newData('korean');
    for (const name of TRANSLATED) {
        const koreanResult = renderHelpTopic({ helpText: koreanHelp, topic: name, substitute: (text) => data.substitute(text) });
        const englishResult = renderHelpTopic({ helpText: englishHelp, topic: name, substitute: (text) => data.substitute(text) });
        assert.equal(koreanResult.found, true, name);
        assert.ok(/[가-힣]/.test(koreanResult.styleText), `${name} 본문에 한글이 없음`);
        // 링크 목적지는 영어와 같은 집합이다(번역하며 문장 순서가 바뀌면 순서는 다를 수 있다).
        assert.deepEqual([...koreanResult.links.destinations].sort(), [...englishResult.links.destinations].sort(), `${name} 링크 목적지`);
    }
    // 같은 이름의 앵커가 둘인 주제(themeHelp·unitHelp)는 한국어도 한 번만 나온다.
    assert.equal((renderHelpTopic({ helpText: koreanHelp, topic: 'themeHelp', substitute: (text) => text }).styleText.match(/님버스의 세 명의 퓨리/g) ?? []).length, 1);
    assert.equal((renderHelpTopic({ helpText: koreanHelp, topic: 'unitHelp', substitute: (text) => text }).styleText.match(/전투 유닛은 썬, 윈드/g) ?? []).length, 1);
    // 번역하지 않은 주제는 한국어 파일에서 찾지 못한다(영어 대체는 도움말 창이 한다).
    assert.equal(renderHelpTopic({ helpText: koreanHelp, topic: 'statsHelp', substitute: (text) => text }).found, false);
});

// 게임 데이터: 영어 대응 파일 읽기. (2026-10-31 WB03-P)
test('GameData.readEnglishText: 한국어 경로의 영어 파일, 영어·무관한 경로는 null', () => {
    const data = newData('korean');
    assert.ok(data.readEnglishText('\\D\\help.korean').includes('How To Capture and Sacrifice'));
    assert.equal(data.readEnglishText('\\D\\help.english'), null, '이미 영어인 경로');
    assert.equal(data.readEnglishText('\\D\\nothing.korean'), null, '영어 파일이 없음');
    assert.equal(newData('english').readEnglishText('\\D\\help.english'), null, '영어 언어');
    assert.equal(data.get('Quick Help Spec'), '\\D\\help.korean');
});

// 도움말 창: 번역한 주제는 한국어, 번역하지 않은 주제는 영어(같은 창 안에서 주제 단위로 대체), 뒤로 가기·링크가 섞여도 동작한다. (2026-10-31 WB03-P)
test('한국어 도움말 창: 번역한 주제는 한국어이고 번역하지 않은 주제는 영어로 대체된다', () => {
    const lines = (window) => window.body.softLines.map((line) => line.fragments.filter((fragment) => !fragment.picture).map((fragment) => (fragment.bytes instanceof Uint16Array ? String.fromCharCode(...fragment.bytes) : '')).join('')).join('\n');
    const open = (topic) => createHelpWindow({ gameData: newData('korean'), resources: koreanResources, topic, screen: { width: 1024, height: 768 } }).window;
    const window = open('F1Help');
    assert.ok(lines(window).includes('NetStorm 도움말'));
    assert.ok(window.fallbackHelpText !== null && window.fallbackHelpText.includes('How To Capture and Sacrifice'));
    // 번역한 주제로 링크를 따라가도 한국어다.
    window.open('interfaceHelp');
    window.fit();
    window.place();
    assert.ok(lines(window).includes('NetStorm 사용자 인터페이스'));
    assert.ok(!/[A-Za-z]{4}.*[A-Za-z]{4}.*Left Click/.test(lines(window)), '영어 원문이 섞이지 않음');
    // 번역하지 않은 주제(statsHelp)는 영어 파일에서 오고 창은 정상으로 열린다.
    window.open('statsHelp');
    window.fit();
    window.place();
    assert.equal(window.topic, 'statsHelp');
    const english = lines(window);
    assert.ok(english.length > 50 && !/[가-힣]/.test(english.replace(/\s/g, '')), `영어 본문이어야 함: ${english.slice(0, 60)}`);
    assert.ok(window.body.softLines.length > 3);
    // 뒤로 가기는 방문 기록대로 한국어 주제로 돌아온다.
    window.back();
    assert.equal(window.topic, 'interfaceHelp');
    assert.ok(lines(window).includes('NetStorm 사용자 인터페이스'));
    // 영어 도움말 창은 영어 파일 하나만 쓴다.
    const englishWindow = createHelpWindow({ gameData: newData('english'), resources: englishResources, topic: 'interfaceHelp', screen: { width: 1024, height: 768 } }).window;
    assert.equal(englishWindow.fallbackHelpText, null);
    // 한국어 묶음을 받지 못한 한국어 실행은 영어 도움말 파일 하나로 대체되어 영어 파일을 두 번 쓰지 않는다.
    const lost = createHelpWindow({ gameData: newData('korean', { withFiles: false }), resources: englishResources, topic: 'F1Help', screen: { width: 1024, height: 768 } }).window;
    assert.equal(lost.fallbackHelpText, null);
});

// 번역한 주제는 모두 한국어 창에서 열리고(본문이 유니코드 코드, 줄이 있음) 화면 안에 들어간다. 4개 화면 크기에서 확인한다. (2026-10-31 WB03-P)
test('한국어 도움말 창: 번역한 15개 주제가 모두 열린다', () => {
    const failures = [];
    // 화면 크기마다 번역한 주제를 모두 열어 본다.
    for (const screen of [{ width: 800, height: 600 }, { width: 1024, height: 768 }, { width: 1366, height: 768 }, { width: 1280, height: 800 }]) {
        for (const topic of TRANSLATED) {
            const window = createHelpWindow({ gameData: newData('korean'), resources: koreanResources, topic, screen }).window;
            const fragments = window.body.softLines.flatMap((line) => line.fragments).filter((fragment) => !fragment.picture);
            if (fragments.length === 0 || fragments.some((fragment) => !(fragment.bytes instanceof Uint16Array))) failures.push(`${topic} ${screen.width}x${screen.height}: 본문 조각`);
            const bounds = window.bounds();
            if (bounds.left < 0 || bounds.top < 0 || bounds.right > screen.width || bounds.bottom > screen.height) failures.push(`${topic} ${screen.width}x${screen.height}: 화면 밖`);
        }
    }
    assert.deepEqual(failures, []);
});
