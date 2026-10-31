// 한국어 F1 도움말(WB03-P)을 검사한다: help.korean(번역한 주제만 담은 부분 파일)이 영어 help.english 의 첫 앵커와 같은 구조(태그·그림·링크 주소·조건·글자 모양 표시)인지,
// 도움말 창이 번역하지 않은 주제를 주제 단위로 영어 파일에서 대신 찾는지, 한국어 본문이 유니코드 글꼴 코드인지 확인한다. 번역 문구 자체의 옳고 그름은 검사하지 않는다(개발자 검수 대상).
// (2026-10-31 WB03-P, 2026-11-01 WB03-Q: 번역 주제 15개 → 별칭 앵커 포함 확대, 별칭 앵커 검사 추가, 2026-11-01 WB03-R: 멀티플레이어 설명·주문 주제 추가와 영어로 남긴 <c> 문구 검사)
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

// 본문의 구조 토큰(태그·그림·링크 주소·조건, `~` 글자 모양 표시)을 정렬해 돌려준다. 번역하면서 낱말 순서가 바뀔 수 있어 순서는 보지 않는다.
// 화면에 글자 그대로 찍히는 코드(`~~y`)와 원본 그림 코드(`~[B.3]`·`~~[C.xxx]`)는 뒷부분까지 한 토큰으로 본다. 태그(`<…>`)는 대소문자를 무시하고 글자 모양 표시는 구분한다(`~I` 와 `~i` 는 다르다).
// (2026-10-31 WB03-P, 2026-11-01 WB03-R: `~~글자`·`~[…]` 전체 토큰화와 글자 모양 표시 대소문자 구분 추가 — 변이 검사에서 그림 코드 숫자·찍히는 코드 글자 변경을 놓쳐 보강)
function structureOf(topic) {
    return (topic.match(/<[^>]*>|~~\[[^\]]*\]|~~[^\s]|~\[[^\]]*\]|~[^\s]/g) ?? []).map((token) => (token.startsWith('<') ? token.toLowerCase() : token)).sort();
}

// 번역한 주제 목록(첫 앵커 이름, 파일 순서)이다. 본문 없이 다음 앵커와 묶이는 별칭 앵커(ALIAS_TARGETS 의 키)도 포함한다. (2026-10-31 WB03-P, 2026-11-01 WB03-Q: 별칭 앵커와 새 주제 추가)
const TRANSLATED = Object.freeze([
    'sacrificeOutline', 'immobileHelp', 'pickupHelp', 'moveyourpriestHelp', 'performsacrificeHelp', 'sphereHelp', 'deusphereHelp', 'serenisphereHelp',
    'pyrosphereHelp', 'bountyislandHelp', 'F1Help', 'interfaceHelp', 'vesselPriestHelp', 'themeHelp', 'unitHelp',
    'transportHelp', 'researchHelp', 'technologyHelp', 'statsHelp', 'moneyHelp', 'stormPowerHelp', 'influenceHelp',
    'rainVortexType', 'windVortexType', 'thunderVortexType', 'vortexHelp', 'spVortexHelp', 'golemVortexHelp', 'influenceVortexHelp',
    'factoryHelp', 'sunFactoryType', 'windFactoryType', 'rainFactoryType', 'thunderFactoryType', 'upgradeHelp', 'shooterHelp',
    'tacticHelp', 'tacticsHelp', 'salvageHelp', 'minimapGumpHelp', 'moneyGumpHelp', 'teleportViewHelp', 'campaignHelp', 'multiplayerHelp', 'multiQuickHelp',
    'chatViewHelp', 'formatCommandHelp', 'chathelperHelp', 'outpostType', 'outpostHelp', 'rankHelp', 'multiSafeHelp', 'reliabilityHelp', 'battlemasterHelp',
    'battleIslandType', 'zoneHelp', 'challengeIslandType', 'playerListView', 'playerListViewHelp', 'battleStatusView', 'battleStatusViewHelp',
    'playerOptionsHelp', 'obeliskType', 'buriedType', 'bombHelp', 'battleOptionsHelp',
]);

// 본문이 없는 별칭 앵커 → 그 앵커가 보여 주는 본문 앵커이다(영어 원문에서 별칭 앵커는 본문 앵커 바로 앞에 붙어 있어 같은 본문이 나온다). (2026-11-01 WB03-Q, 2026-11-01 WB03-R: 6개 추가)
const ALIAS_TARGETS = Object.freeze({
    researchHelp: 'technologyHelp', moneyHelp: 'stormPowerHelp', tacticHelp: 'tacticsHelp', campaignHelp: 'multiplayerHelp',
    rainVortexType: 'vortexHelp', windVortexType: 'vortexHelp', thunderVortexType: 'vortexHelp',
    factoryHelp: 'thunderFactoryType', sunFactoryType: 'thunderFactoryType', windFactoryType: 'thunderFactoryType', rainFactoryType: 'thunderFactoryType',
    outpostType: 'outpostHelp', zoneHelp: 'challengeIslandType', playerListView: 'playerListViewHelp', battleStatusView: 'battleStatusViewHelp',
    obeliskType: 'bombHelp', buriedType: 'bombHelp',
});

// 앵커 구간에서 태그를 걷어 낸 보이는 글이 비었는지(본문 없는 별칭 앵커인지) 돌려준다. (2026-11-01 WB03-Q)
function isAliasSegment(segment) {
    return segment.replace(/<[^>]*>/g, '').trim() === '';
}

// help.korean 형식: UTF-8(BOM 없음)·소스와 복사본 일치·번역 주제 목록·앵커 이름 중복 없음·영어에 있는 주제만·별칭 앵커는 영어와 같은 자리. (2026-10-31 WB03-P, 2026-11-01 WB03-Q: 별칭 앵커 검사)
test('help.korean: UTF-8·소스와 복사본 일치·번역 주제 목록', () => {
    const bytes = koreanFiles.get('d/help.korean');
    assert.notDeepEqual([...bytes.subarray(0, 3)], [0xef, 0xbb, 0xbf], 'BOM 없는 UTF-8');
    assert.deepEqual([...bytes], [...new Uint8Array(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'help.korean')))]);
    assert.deepEqual([...koreanTopics.keys()], TRANSLATED.map((name) => name.toLowerCase()));
    for (const [name, bodies] of koreanTopics) {
        assert.equal(bodies.length, 1, `${name} 앵커가 둘 이상`);
        assert.ok(englishTopics.has(name), `${name} 은 영어에 없음`);
        // 별칭 앵커(영어 원문에서 보이는 글이 없는 앵커)는 한국어도 비어 있고, 그 밖의 앵커에는 한글이 있다.
        const alias = isAliasSegment(englishTopics.get(name)[0]);
        assert.equal(alias, Object.keys(ALIAS_TARGETS).some((key) => key.toLowerCase() === name), `${name} 별칭 여부가 목록과 다름`);
        if (alias) assert.ok(isAliasSegment(bodies[0]), `${name} 별칭 앵커에 글이 있음`);
        else assert.ok(/[가-힣]/.test(bodies[0]), `${name} 에 한글이 없음`);
    }
});

// 번역하지 않고 남겨 둔 영어 낱말(네 글자 이상)은 화면에 보이는 고유 이름·키 이름·메뉴 이름뿐이다(문장이 영어로 남은 주제를 찾는다). (2026-10-31 WB03-P, 2026-11-01 WB03-Q: 메뉴 항목 Drop·명령 이름 format·시험용 이름 devo12 허용,
// 2026-11-01 WB03-R: 게임 안 메뉴 이름을 그대로 둔 `<c>` 문구는 영어 원문에 같은 `<c>` 문구가 있을 때만 허용하고 검사에서 뺀다. 원본 그림 코드 `~[I…]` 도 뺀다.)
test('help.korean: 영어 문장이 남지 않았다(고유 이름·키·메뉴 이름만 허용)', () => {
    const allowed = new Set(['Start', "Battle'", 'NetStorm', 'Netstorm', 'Ticonderoga', 'Entertainment', 'Activision', 'Titanic', 'MicroSoft', 'IntellimouseTM', 'Escape', 'Shift', 'Pause', 'Ctrl', 'About', 'Salvage', 'Drop', 'devo', 'format', 'ENTER', 'back', 'front',
        // 채팅 예시 이름과 게임 안 메뉴·옵션 이름(게임 안 메뉴가 아직 한국어가 아니라 영어 이름을 그대로 둔다).
        'OzTheGreatAndPowerful', 'Game', 'Empty', 'Mimicing', 'Sounds', 'Icons', 'Emboss', 'Offset', 'Allow', 'Foreground', 'Colors', 'Chat', 'Effects', 'Controls', 'Channel', 'Player', 'Other', 'Insert', 'Copy', 'Paste', 'Format']);
    const problems = [];
    let visible = '';
    // 주제마다 영어 원문에 있는 `<c>` 문구를 그대로 둔 것은 빼고, 영어 원문에 없는 영어 `<c>` 문구는 문제로 모은다.
    for (const [name, bodies] of koreanTopics) {
        const englishSpans = new Set([...englishTopics.get(name)[0].matchAll(/<c>([^<]*)<\/c>/g)].map((match) => match[1]));
        visible += bodies[0].replace(/<c>([^<]*)<\/c>/g, (whole, inner) => {
            if (/[가-힣]/.test(inner)) return whole;
            if (!englishSpans.has(inner)) problems.push(`${name}: 영어 원문에 없는 영어 <c> 문구 ${JSON.stringify(inner)}`);
            return ' ';
        });
    }
    // 태그·원본 그림 코드·글자 모양 표시를 걷어 내고 남은 영어 낱말을 모은다(첫 앵커 앞의 설명은 화면에 나오지 않는다).
    visible = visible.replace(/<[^>]*>/g, ' ').replace(/~\[[^\]]*\]/g, ' ').replace(/~[^\s]/g, ' ');
    const leftovers = [...new Set([...visible.matchAll(/[A-Za-z][A-Za-z']{3,}/g)].map((match) => match[0]))].filter((word) => !allowed.has(word));
    assert.deepEqual(problems, []);
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
    // 같은 이름의 앵커가 둘인 신전·워크샾 계열 주제도 한국어는 한 번만 나온다(영어 파일에도 옛 본문이 뒤에 하나 더 있다). (2026-11-01 WB03-Q)
    for (const [topic, phrase] of [['vortexHelp', /신전은 각 섬에서/g], ['spVortexHelp', /전투에서 신전은/g], ['golemVortexHelp', /신전에서는 새 섬에서/g], ['influenceVortexHelp', /님버스에서 무엇이든 지으려면/g]]) {
        assert.equal((renderHelpTopic({ helpText: koreanHelp, topic, substitute: (text) => text }).styleText.match(phrase) ?? []).length, 1, topic);
    }
    // 번역하지 않은 주제는 한국어 파일에서 찾지 못한다(영어 대체는 도움말 창이 한다).
    assert.equal(renderHelpTopic({ helpText: koreanHelp, topic: 'altarType', substitute: (text) => text }).found, false);
});

// 별칭 앵커는 영어와 같은 본문 앵커를 가리킨다: 한국어에서도 별칭과 본문 앵커가 같은 본문(같은 링크 목록)을 보여 준다. (2026-11-01 WB03-Q)
test('renderHelpTopic: 별칭 앵커가 영어와 같은 본문 앵커의 본문을 보여 준다', () => {
    const data = newData('korean');
    // 별칭 앵커로 열면 본문 뒤에 링크 색 복원 코드(`~[T0]~N~.`)가 앵커 수만큼 더 붙는다(원본 파서의 동작이며 영어도 같다).
    // 또 별칭 이름이 본문 앵커 이름의 앞부분이면(playerListView ⊂ playerListViewHelp) 두 앵커가 모두 주제로 잡혀 본문 머리 표시(`~5~w `)도 겹친다.
    // 그 머리와 꼬리를 뺀 본문이 같아야 하고, 머리와 꼬리 자체는 영어와 같아야 한다.
    const tailOf = (styleText) => styleText.match(/(?:\s|~\[T0\]~N~\.)+$/)?.[0] ?? '';
    const headOf = (styleText) => styleText.match(/^(?:~5~w )+/)?.[0] ?? '';
    const bodyOf = (styleText) => styleText.slice(headOf(styleText).length, styleText.length - tailOf(styleText).length);
    const render = (helpText, topic) => renderHelpTopic({ helpText, topic, substitute: (text) => data.substitute(text) });
    // 별칭 앵커마다 본문 앵커와 같은 본문·링크가 나오는지를 한국어와 영어 모두에서 비교한다.
    for (const [alias, target] of Object.entries(ALIAS_TARGETS)) {
        const edges = [];
        for (const helpText of [koreanHelp, englishHelp]) {
            const aliasResult = render(helpText, alias);
            const targetResult = render(helpText, target);
            assert.equal(aliasResult.found, true, alias);
            assert.equal(bodyOf(aliasResult.styleText), bodyOf(targetResult.styleText), `${alias} → ${target}`);
            assert.deepEqual(aliasResult.links.destinations, targetResult.links.destinations, `${alias} → ${target} 링크`);
            edges.push([headOf(aliasResult.styleText), tailOf(aliasResult.styleText)]);
        }
        assert.deepEqual(edges[0], edges[1], `${alias} 머리·꼬리가 영어와 다름`);
    }
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
    // 번역하지 않은 주제(altarType)는 영어 파일에서 오고 창은 정상으로 열린다. (2026-11-01 WB03-Q: statsHelp 를 번역해 altarType 으로 바꿈)
    window.open('altarType');
    window.fit();
    window.place();
    assert.equal(window.topic, 'altarType');
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

// 번역한 주제는 모두 한국어 창에서 열리고(본문이 유니코드 코드, 줄이 있음) 화면 안에 들어간다. 4개 화면 크기에서 확인한다. (2026-10-31 WB03-P, 2026-11-01 WB03-Q: 별칭 앵커 포함 45개, 2026-11-01 WB03-R: 66개)
test('한국어 도움말 창: 번역한 주제(별칭 앵커 포함)가 모두 열린다', () => {
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
