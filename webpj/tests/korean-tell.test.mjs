// 한국어 지원 2단계(WB03-O)를 검사한다: 한국어 언어 자료(tell.korean·offical*.korean·config.korean)가 영어 원문과 같은 구조(명령·조건·치환 키·링크)를 가지는지,
// 게임 데이터가 섹션 단위로 영어로 대체하는지, 한국어 Tell 창·도움말 창이 유니코드 글꼴 코드로 본문과 단추를 만들고 화면에 들어가는지 확인한다.
// 번역 문구 자체의 옳고 그름은 검사하지 않는다(개발자 검수 대상). 브라우저의 실제 D2Coding 글리프는 브라우저 검사(임시 CDP 스크립트)로 확인한다.
// (2026-10-31 WB03-O)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { GameData } from '../www/assets/game-data.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { KOREAN_BUNDLE, KOREAN_LANGUAGE_PATHS, buildKoreanSupport } from '../www/assets/korean-assets.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { createTellWindow } from '../www/ui/tell-window.js';
import { createHelpWindow } from '../www/ui/help-window.js';
import { expandFileList } from '../www/ui/tell-lists.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';

// 가짜 래스터라이저이다. 한글(0x7f 초과)은 em 폭, 그 밖은 em 의 절반 폭으로 전진하고, 공백은 그림이 없으며, 나머지는 3×2 그림을 낸다. (2026-10-31 WB03-O)
function fakeRasterizer() {
    return {
        glyph(code, spec) {
            const advance = code > 0x7f ? spec.emSize : Math.round(spec.emSize / 2);
            if (code === 0x20) return { advance, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) };
            return { advance, left: 1, top: 2, width: 3, height: 2, coverage: Uint8Array.from([255, 109, 110, 0, 200, 255]) };
        },
    };
}

// 메뉴 묶음·한국어 묶음 파일, 원본 아카이브이다(검사마다 공유). (2026-10-31 WB03-O)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const koreanFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === KOREAN_BUNDLE).map((entry) => entry.path));
const archive = new TaffArchive(readData('netstorm.tarc'));
const korean = await buildKoreanSupport(koreanFiles, { createRasterizer: () => fakeRasterizer() });
const koreanResources = buildMenuResources(menuFiles, korean);
const englishResources = buildMenuResources(menuFiles);

// 새 게임 데이터를 만든다. language 가 korean 이면 한국어 언어 자료를 느슨한 파일로 합치고, withFiles 가 false 면 합치지 않는다(한국어 묶음 실패 대체). (2026-10-31 WB03-O)
function newData(language, { withFiles = true, options = {} } = {}) {
    const files = language === 'korean' && withFiles ? new Map([...menuFiles, ...korean.languageFiles]) : menuFiles;
    return GameData.build({ files, language, options: { soundVolume: 3, musicVolume: 2, ...options } });
}

// 영어 tell.english 와 한국어 tell.korean 텍스트이다. (2026-10-31 WB03-O)
const englishTell = decodeOriginalText(archive.read('\\d\\tell.english'));
const koreanTell = decodeOriginalText(koreanFiles.get('d/tell.korean'));

// 줄 맨 앞의 `[이름]` 줄로 섹션을 나눠 소문자 이름 → 본문 줄 배열 Map 으로 만든다. 같은 이름이 또 나오면 첫 섹션을 쓴다(원본 조회와 같음).
// duplicates 에 중복 이름을 모은다. (2026-10-31 WB03-O)
function parseSections(text) {
    const sections = new Map();
    const duplicates = [];
    let current = null;
    // 줄마다 섹션 머리인지 보고 본문을 모은다.
    for (const line of text.split(/\r?\n/)) {
        const header = /^\[([^\]]+)\]\s*$/.exec(line);
        if (header) {
            const key = header[1].trim().toLowerCase();
            if (sections.has(key)) {
                duplicates.push(key);
                current = [];
            } else {
                current = [];
                sections.set(key, current);
            }
            continue;
        }
        if (current) current.push(line);
    }
    return { sections, duplicates };
}

const koreanSections = parseSections(koreanTell);
const englishSections = parseSections(englishTell);
// 배포 소스의 리터럴·보간 문자열에서 공통 Tell 이름을 읽는 자바스크립트 파서이다.
const { parse } = createRequire(import.meta.url)('acorn');

// 웹의 제외 기능 또는 같은 철자의 내부 값이다. 표시할 문장이 없는 원본 구역도 명시한다.
const NON_DISPLAY_SECTIONS = new Map([
    ['end', '내부 키/원본 파일 끝 구분'], ['player', '내부 객체 키·웹 제외 참가자 정보'], ['setowner', '편집·개발자 소유자 설정'],
    ['quiteditmode', '편집 모드 제외'], ['quitabandon', '웹 종료 기능 제외'], ['querydecendtochallevel', '멀티플레이 메타맵 제외'],
    ['usernotupdated', '외부 유효성 검사 비활성화'], ['confirmretreat', '멀티플레이 전투 이탈'], ['campaign', '캠페인 모드 이름/사용자 제작 목록 제외'],
    ['clientcheated', '원본 빈 섹션'], ['servercheated', '원본 빈 섹션'], ['serverthinksyoucheated', '원본 빈 섹션'],
    ['chalintro', '멀티플레이 메타맵 제외'], ['setislands', '표시 없는 설정 명령'], ['zsend', '서버 명령 제외'],
    ['setsize', '웹 해상도 설정 제외'], ['directdrawmanager', 'DirectDraw 제외'], ['changefirewall', '서버 진단 제외'],
    ['gameverdiff', '단독 로컬 연결은 제품 버전이 동일'], ['playernomake', '원본 빈 섹션'], ['sacrificemenu', '미션 모드에서 보상 선택 UI를 생략'],
]);

// 새로 연결한 공통 창이 예전의 메뉴 중심 시작점 목록에서 누락되어도 소스 참조로 잡는다.
test('공통 Tell: 배포 소스에서 참조하는 표시 섹션이 한국어 파일에 있다', () => {
    const missing = new Set();
    // AST에서 문자열 리터럴과 보간 문자열의 첫 고정 부분을 검사한다.
    const visit = (node) => {
        const text = node.type === 'Literal' ? node.value : node.type === 'TemplateLiteral' ? node.quasis[0]?.value.cooked : null;
        if (typeof text === 'string') {
            const name = text.split('(', 1)[0].toLowerCase();
            if (englishSections.sections.has(name) && !NON_DISPLAY_SECTIONS.has(name) && !koreanSections.sections.has(name)) missing.add(name);
        }
        // 문법 노드 자식만 순회하며 위치·원문 메타데이터는 건너뛴다.
        for (const value of Object.values(node)) {
            if (Array.isArray(value)) { for (const child of value) if (child?.type) visit(child); }
            else if (value?.type) visit(value);
        }
    };
    const root = path.join(REPO_ROOT, 'webpj', 'www');
    // 실행 소스만 검사하고 생성 번들의 중복 문법 트리는 제외한다.
    for (const entry of fs.readdirSync(root, { recursive: true, withFileTypes: true })) {
        if (!entry.isFile() || !entry.name.endsWith('.js') || path.relative(root, entry.parentPath).split(path.sep).includes('dist')) continue;
        visit(parse(fs.readFileSync(path.join(entry.parentPath, entry.name), 'utf8'), { ecmaVersion: 'latest', sourceType: 'module' }));
    }
    assert.deepEqual([...missing].sort(), []);
});

// 본문 줄 배열의 구조 토큰을 뽑는다. directives 는 지시문 줄(`$Button=…` 등)을 순서대로 담되 사람이 읽는 라벨은 뺀다(`@`·`#`·`%`·`*` 로 시작하는 목록 패턴 라벨은 남김).
// inline 은 본문 줄 안의 HTML 태그·`{치환 키}`·`~` 글자 모양 표시를 정렬해 담는다(번역하면서 낱말 순서가 바뀔 수 있어 순서는 보지 않는다).
// 팁 줄(`tipN="…"`)은 번호를 앞에 붙여 각 팁의 토큰을 따로 비교한다. 기본값 문구(`{@tell.tip{tipNumber}|문구}`)는 문구를 지운다. (2026-10-31 WB03-O)
function structureOf(lines) {
    const directives = [];
    const inline = [];
    // 줄마다 지시문이면 라벨을 뺀 모양을, 아니면 본문 토큰을 모은다.
    for (const line of lines) {
        const directive = /^\s*\$(Button|ButtonInstant|Menu|Checked|Timeout|OnExit|Key|Input|Zsend)\s*=\s*(.*)$/i.exec(line);
        if (directive) {
            const [kind, rest] = [directive[1].toLowerCase(), directive[2]];
            const parts = rest.split(',');
            if (kind === 'timeout') directives.push(`timeout|${rest}`);
            else if (kind === 'menu') directives.push(`menu|${/^[@#%*]/.test(parts[0]) ? parts[0] : '(text)'}|${parts.slice(1).join(',')}`);
            else directives.push(`${kind}|${parts.slice(1).join(',')}`);
            continue;
        }
        const tip = /^(tip\d+)="(.*)"\s*$/.exec(line);
        const prefix = tip ? `${tip[1]}:` : '';
        // Warning 인자는 화면에 보이는 문장이다. 명령과 치환 키·글자 서식만 원문과 비교한다.
        const text = (tip ? tip[2] : line).replace(/\|[^{}]*\}/g, '|}').replace(/<\$Warning,([^>]*)>/gi, (_, body) => `<$Warning>${(body.match(/\{[^{}]*\}|~[^\s]/g) ?? []).join('')}`);
        for (const token of text.match(/<[^>]*>|\{[^{}]*\}|~[^\s]/g) ?? []) inline.push(prefix + token.toLowerCase());
    }
    return { directives, inline: inline.sort() };
}

// 웹 판본의 현재 메뉴에서 닿는 Tell 섹션의 시작점이다(메인 메뉴 버튼·Help → Version·시작 창). 여기서 `Tell,이름` 참조를 따라 닿는 모든 섹션이 한국어에 있어야 한다. (2026-10-31 WB03-O)
const ENTRY_SECTIONS = ['UCampaign', 'Credits', 'Demoville', 'Demo', 'Welcome', 'Welcome2', 'Tips', 'TipList', 'TechSupport', 'About',
    // 메뉴 뒤 공통 창과 HUD 경고도 번역 완전성의 시작점으로 검사한다.
    'ABORT', 'TryAgain', 'DenySalvage', 'BuildNoBucks', 'NoTechTheme', 'UpgradeNoBucks', 'SalvageVortex', 'SalvageInf', 'NewTech', 'AltarUpgrade', 'AlreadyUpgrading', 'AskSend',
    'SacrificeForStormpower', 'WarnCantUpgradeAltarOffHomeIsland', 'PriestImmobile', 'islandUnderAttack', 'WarnYourPriestTaken', 'WarnNeutralPriestTaken', 'WarnPriestTaken',
    'EarlySacrifice', 'DrawNotAccepted', 'AllianceRequested', 'AllianceSpam', 'AllianceComplete', 'AllianceBroken', 'MultiplayerPriestDead', 'MultiplayerOtherPriestDead',
    'TellGifWritten', 'NewRank', 'ProdPending', 'BridgesKilled', 'NotImplemented'];

// 시작점에서 `Tell,이름`·`<$tell,이름>` 참조를 따라 닿는 섹션 이름(소문자) 집합을 구한다. 목록 객체 섹션(점이 있는 이름)과 BLANK 는 뺀다. (2026-10-31 WB03-O)
function reachableFrom(sections, entries) {
    const seen = new Set();
    const queue = entries.map((name) => name.toLowerCase());
    // 아직 보지 않은 섹션의 참조를 큐에 더한다.
    while (queue.length > 0) {
        const name = queue.pop();
        if (seen.has(name)) continue;
        seen.add(name);
        const body = (sections.get(name) ?? []).join('\n');
        for (const match of body.matchAll(/\btell\s*,\s*([A-Za-z0-9_]+)/gi)) if (!seen.has(match[1].toLowerCase())) queue.push(match[1].toLowerCase());
    }
    seen.delete('blank');
    return seen;
}

// 한국어 tell.korean 형식: UTF-8(BOM 없음)·섹션 이름 중복 없음·영어에 있는 섹션만·모든 섹션에 한글이 있다. 웹 공개 복사본이 소스와 같다. (2026-10-31 WB03-O)
test('tell.korean: UTF-8·중복 없음·영어에 있는 섹션만·소스와 복사본 일치', () => {
    const bytes = koreanFiles.get('d/tell.korean');
    assert.notDeepEqual([...bytes.subarray(0, 3)], [0xef, 0xbb, 0xbf], 'BOM 없는 UTF-8');
    assert.deepEqual([...bytes], [...new Uint8Array(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'tell.korean')))]);
    assert.deepEqual(koreanSections.duplicates, []);
    assert.ok(koreanSections.sections.size >= 25, `섹션 수 ${koreanSections.sections.size}`);
    const notInEnglish = [...koreanSections.sections.keys()].filter((name) => !englishSections.sections.has(name));
    assert.deepEqual(notInEnglish, [], '영어에 없는 섹션');
    // 번역한 섹션에는 모두 한글이 있다(영어 그대로 복사된 섹션을 찾는다).
    for (const [name, lines] of koreanSections.sections) assert.ok(/[가-힣]/.test(lines.join('\n')), `${name} 에 한글이 없습니다`);
});

// 한국어 섹션마다 영어 원문과 지시문 순서·명령·인자, 본문 안의 태그·치환 키·글자 모양 표시가 같아야 한다(번역하다 명령을 바꾸거나 빠뜨리지 않았는지). (2026-10-31 WB03-O)
test('tell.korean: 모든 섹션이 영어 원문과 같은 구조(명령·조건·치환 키·링크)다', () => {
    const problems = [];
    // 번역한 섹션마다 영어와 구조를 비교한다.
    for (const [name, lines] of koreanSections.sections) {
        const english = structureOf(englishSections.sections.get(name));
        const translated = structureOf(lines);
        if (JSON.stringify(english.directives) !== JSON.stringify(translated.directives)) problems.push(`${name}: 지시문 ${JSON.stringify(english.directives)} ≠ ${JSON.stringify(translated.directives)}`);
        // 한국어의 좁은 본문 폭에 맞춘 추가 <br>는 허용하되 원문 줄바꿈과 다른 모든 토큰은 보존한다.
        const isBreak = (token) => /^<br\s*\/?\s*>$/iu.test(token);
        if (translated.inline.filter(isBreak).length < english.inline.filter(isBreak).length) problems.push(`${name}: 원문 <br> 누락`);
        if (JSON.stringify(english.inline.filter((token) => !isBreak(token))) !== JSON.stringify(translated.inline.filter((token) => !isBreak(token)))) problems.push(`${name}: 본문 토큰 ${JSON.stringify(english.inline)} ≠ ${JSON.stringify(translated.inline)}`);
    }
    assert.deepEqual(problems, []);
});

// 웹에서 닿는 Tell 섹션이 모두 한국어로 번역되어 있다(영어 문구가 한국어 화면에 새지 않는다). 팁은 0~39번이 모두 있다. (2026-10-31 WB03-O)
test('tell.korean: 웹에서 닿는 섹션 전부와 팁 40개가 번역되어 있다', () => {
    const reachable = reachableFrom(koreanSections.sections, ENTRY_SECTIONS);
    const missing = [...reachable].filter((name) => !koreanSections.sections.has(name));
    assert.deepEqual(missing, [], '번역이 없는 섹션');
    assert.ok(reachable.size >= 25, `닿는 섹션 ${reachable.size}`);
    // 영어에서 닿는 집합과 같은 크기여야 한다(한국어 쪽 참조가 영어와 같은 길을 가는지).
    assert.deepEqual([...reachableFrom(englishSections.sections, ENTRY_SECTIONS)].sort(), [...reachable].sort());
    const tips = (koreanSections.sections.get('tiplist') ?? []).filter((line) => /^tip\d+=/.test(line));
    assert.equal(tips.length, 40);
    assert.deepEqual(tips.map((line) => Number(/^tip(\d+)=/.exec(line)[1])), Array.from({ length: 40 }, (_, index) => index));
});

// 캠페인 목록 파일: 영어 파일마다 한국어 파일이 있고(8개) 키·섹션·지시문 구조가 같으며 제목이 번역되어 있다. config.korean 은 영어와 같은 키를 가진다. (2026-10-31 WB03-O)
test('offical*.korean·config.korean: 영어 파일과 같은 이름·키·구조', () => {
    const englishNames = archive.find('\\d\\offical*.english').map((entry) => entry.split('\\').pop().replace(/\.english$/, ''));
    assert.equal(englishNames.length, 8);
    assert.deepEqual(KOREAN_LANGUAGE_PATHS.filter((entry) => /offical/.test(entry)).sort(), englishNames.map((name) => `d/${name}.korean`).sort());
    for (const name of englishNames) {
        const english = decodeOriginalText(archive.read(`\\d\\${name}.english`));
        const translated = decodeOriginalText(koreanFiles.get(`d/${name}.korean`));
        const [englishHead, ...englishBody] = english.split(/\r?\n/);
        const [koreanHead, ...koreanBody] = translated.split(/\r?\n/);
        // 첫 줄 `title = "…"` 의 모양(따옴표 형식)이 같고 한국어 제목은 구분선이거나 한글이다.
        assert.match(englishHead, /^title\s*=\s*"/, name);
        assert.match(koreanHead, /^title\s*=\s*"/, name);
        assert.ok(/^title\s*=\s*"-+"/.test(koreanHead) ? /^title\s*=\s*"-+"/.test(englishHead) : /[가-힣]/.test(koreanHead), `${name} 제목`);
        const englishStructure = structureOf(englishBody);
        const koreanStructure = structureOf(koreanBody);
        assert.deepEqual(koreanStructure.directives, englishStructure.directives, `${name} 지시문`);
        assert.deepEqual(koreanStructure.inline, englishStructure.inline, `${name} 본문 토큰`);
        assert.equal(/\[Overview\]/i.test(translated), /\[Overview\]/i.test(english), `${name} Overview 섹션`);
    }
    // 용어표: 영어와 같은 키 목록(주석·빈 줄 제외)이고 OkButton 이 한국어 확인 단추다.
    const keysOf = (text) => text.split(/\r?\n/).map((line) => /^([^/=\s][^=]*?)\s*=/.exec(line)?.[1]).filter(Boolean);
    const configEnglish = decodeOriginalText(archive.read('\\d\\config.english'));
    const configKorean = decodeOriginalText(koreanFiles.get('d/config.korean'));
    assert.deepEqual(keysOf(configKorean), keysOf(configEnglish));
    assert.match(configKorean, /^OkButton = "\$Button=확인,DoNothing,0"$/m);
});

// 게임 데이터: 한국어 언어 자료를 읽고, 한국어 파일에 없는 Tell 섹션은 영어로 대체하며, 영어는 그대로다. (2026-10-31 WB03-O)
test('GameData: 한국어 Tell 섹션 조회와 섹션 단위 영어 대체', () => {
    const data = newData('korean');
    assert.ok(data.tellSection('Welcome').includes('님버스'));
    assert.ok(data.tellSection('welcome').includes('님버스'), '섹션 이름은 대소문자를 가리지 않는다');
    // 한국어 파일에 없는 섹션은 영어 tell.english 에서 찾는다(번역이 없는 섹션이 열리지 않는 일이 없다).
    assert.ok(!koreanSections.sections.has('playerinfo'));
    assert.ok(data.tellSection('PlayerInfo').includes('Level Manager'));
    assert.equal(data.tellSection('NoSuchSectionAnywhere'), null);
    // 용어표와 기본 확인 단추도 한국어다.
    assert.equal(data.get('OkButton'), '$Button=확인,DoNothing,0');
    assert.equal(data.substitute('{factory}/{vortex}/{priest}'), '워크샾/신전/사제');
    // 팁 조회(`{@tell.tipN}`)는 한국어 TipList 에서 온다.
    assert.ok(data.substitute('{@tell.tip4}').includes('사제'));
    // 영어는 한국어 파일을 쓰지 않고 영어 대체본도 따로 읽지 않는다.
    const english = newData('english');
    assert.ok(english.tellSection('Welcome').includes('Nimbus'));
    assert.equal(english.fallbackTellText, null);
    assert.equal(english.get('OkButton'), '$Button=OK,DoNothing,0');
    // 한국어 묶음을 받지 못한 한국어 실행은 전부 영어로 대체된다.
    const lost = newData('korean', { withFiles: false });
    assert.ok(lost.tellSection('Welcome').includes('Nimbus'));
    assert.equal(lost.get('OkButton'), '$Button=OK,DoNothing,0');
    // 한국어 용어표가 영어 용어표를 가리지 않는 키(번역하지 않은 설치 문구)도 읽힌다.
    assert.ok(data.get('InetShortcut0').startsWith('http://www.activision.com'));
});

// 캠페인 목록(파일 목록 메뉴): 한국어 offical*.korean 의 제목이 행 라벨이 되고 명령은 영어 목록과 같다. (2026-10-31 WB03-O)
test('캠페인 목록: 한국어 행 라벨과 영어와 같은 명령', () => {
    const rows = (data) => expandFileList(data, `@\\D\\offical*.${data.language}`, 'Tell', '{cur.file}.overview');
    // 목록을 펼치면 묶음 파일마다 설정 객체가 등록되어 `이름.overview` 섹션이 열린다(UCampaign 창이 하는 일).
    const koreanData = newData('korean');
    const englishData = newData('english');
    const koreanRows = rows(koreanData);
    const englishRows = rows(englishData);
    assert.deepEqual(koreanRows.map((row) => row.label), ['초기 미션', '사제 훈련', '', '자유를 위한 투쟁', '국가의 탄생', '완전한 승리']);
    assert.deepEqual(koreanRows.map((row) => row.kind), englishRows.map((row) => row.kind));
    assert.deepEqual(koreanRows.map((row) => row.command), englishRows.map((row) => row.command));
    // 각 묶음의 Overview 섹션이 열리고 미션 행이 영어와 같은 수·명령이다.
    for (const target of ['offical1', 'offical2', 'offical3', 'offical4', 'offical5']) {
        const [koreanResult, englishResult] = [koreanData, englishData].map((data, index) => createTellWindow({
            gameData: data,
            resources: index === 0 ? koreanResources : englishResources,
            name: `${target}.overview`,
            screen: { width: 1024, height: 768 },
        }));
        assert.ok(koreanResult.window && englishResult.window, target);
        const items = (window) => window.menu.items.map((item) => item.command);
        assert.deepEqual(items(koreanResult.window), items(englishResult.window), `${target} 미션 행 명령`);
        assert.equal(koreanResult.window.menu.items.length, englishResult.window.menu.items.length);
    }
});

// 화면 크기 목록이다(4:3 최소·기본, 16:9, 16:10). 한국어 대화상자는 어느 크기에서도 화면 안에 들어가야 한다. (2026-10-31 WB03-O)
const SCREENS = Object.freeze([{ width: 800, height: 600 }, { width: 1024, height: 768 }, { width: 1366, height: 768 }, { width: 1280, height: 800 }]);

// 조각이 글자 조각이면 그 코드 배열(Uint16Array 여야 함)을 문자열로 바꾼다. (2026-10-31 WB03-O)
function fragmentText(fragment) {
    return fragment.bytes instanceof Uint16Array ? String.fromCharCode(...fragment.bytes) : null;
}

// 한국어 Tell 창: 웹에서 닿는 모든 섹션이 열리고, 본문 조각·단추 라벨이 유니코드 코드 배열이며, 명령이 영어 창과 같고, 화면 안에 들어가며, 그려진다. (2026-10-31 WB03-O)
test('한국어 Tell 창: 닿는 모든 섹션이 유니코드 글꼴로 열리고 명령이 영어와 같다', () => {
    const reachable = [...reachableFrom(koreanSections.sections, ENTRY_SECTIONS)].filter((name) => name !== 'tiplist');
    const sections = [...new Set([...ENTRY_SECTIONS.map((entry) => entry.toLowerCase()), ...reachable])].filter((name) => name !== 'tiplist');
    const failures = [];
    // 섹션마다 한국어·영어 창을 만들어 비교한다. Tips 는 앞 팁 단추가 보이도록 tipNumber 를 4 로 둔다.
    for (const name of sections) {
        const options = { tipNumber: 4 };
        const koreanData = newData('korean', { options });
        const englishData = newData('english', { options });
        for (const screen of SCREENS) {
            const koreanResult = createTellWindow({ gameData: koreanData, resources: koreanResources, name, screen });
            const englishResult = createTellWindow({ gameData: englishData, resources: englishResources, name, screen });
            // Warning 전용 섹션은 창 대신 같은 명령을 HUD로 보낸다.
            if (!koreanResult.window && !englishResult.window && /<\$Warning,/i.test(koreanSections.sections.get(name).join('\n'))) {
                assert.ok(koreanResult.inlineCommands.some((command) => command.name.toLowerCase() === 'warning' && /[가-힣]/u.test(command.argument)), name);
                continue;
            }
            if (!koreanResult.window || !englishResult.window) {
                failures.push(`${name} ${screen.width}x${screen.height}: 창 없음`);
                continue;
            }
            const window = koreanResult.window;
            const buttonCommands = (target) => target.buttons.map((button) => `${button.command.name}|${button.command.argument}|${button.command.instant ?? ''}`);
            if (JSON.stringify(buttonCommands(window)) !== JSON.stringify(buttonCommands(englishResult.window))) failures.push(`${name}: 단추 명령이 영어와 다름`);
            if (window.buttons.some((button) => !(button.labelBytes instanceof Uint16Array))) failures.push(`${name}: 단추 라벨이 유니코드가 아님`);
            const textFragments = window.body.softLines.flatMap((line) => line.fragments).filter((fragment) => !fragment.picture);
            if (textFragments.length === 0 || textFragments.some((fragment) => !(fragment.bytes instanceof Uint16Array))) failures.push(`${name}: 본문 조각이 유니코드가 아님`);
            const bounds = window.bounds();
            if (bounds.left < 0 || bounds.top < 0 || bounds.right > screen.width || bounds.bottom > screen.height) failures.push(`${name} ${screen.width}x${screen.height}: 화면 밖 ${JSON.stringify(bounds)}`);
            // 그려도 예외가 없고 대화상자 영역에 그림이 생긴다.
            const surface = new IndexedSurface(screen.width, screen.height);
            window.draw(surface);
            const inside = surface.pixels.some((value) => value !== 0);
            if (!inside) failures.push(`${name}: 그려진 픽셀이 없음`);
        }
    }
    assert.deepEqual(failures, []);
    assert.ok(sections.length >= 25, `검사한 섹션 ${sections.length}`);
});

// 시작 창과 목록: Welcome 단추·Tips 문구와 단추·캠페인 목록 행이 실제 한국어 문구이다. (2026-10-31 WB03-O)
test('한국어 Tell 창: 환영·팁·캠페인 문구와 단추 라벨', () => {
    const open = (name, options = {}) => createTellWindow({ gameData: newData('korean', { options }), resources: koreanResources, name, screen: { width: 1024, height: 768 } }).window;
    const bodyText = (window) => window.body.softLines.map((line) => line.fragments.map(fragmentText).join('')).join('\n');
    const labels = (window) => window.buttons.map((button) => button.text);
    assert.deepEqual(labels(open('Welcome')), ['확인']);
    assert.deepEqual(labels(open('Welcome2')), ['초기 미션 플레이', '메인 메뉴']);
    assert.deepEqual(labels(open('Tips', { tipNumber: 4 })), ['이전 팁', '다음 팁', '그만 보기', '확인']);
    assert.deepEqual(labels(open('Tips', { tipNumber: 0 })), ['다음 팁', '그만 보기', '확인']);
    assert.ok(bodyText(open('Tips', { tipNumber: 4 })).includes('F5'), '팁 4 본문');
    assert.ok(bodyText(open('Tips', { tipNumber: 40 })).includes('더 이상 팁이 없습니다!'), '없는 팁 번호의 기본 문구');
    assert.deepEqual(labels(open('Credits')), ['취소']);
    // 세 번째 행은 제품 11.01 Patch Credits 의 한국어 행이다(원본 두 행 뒤). (2026-11-05 WB03-AC)
    assert.deepEqual(open('Credits').menu.items.map((item) => item.label), ['NetStorm 10.72 패치 크레딧', 'NetStorm 오리지널 크레딧', 'NetStorm 11.01 패치 크레딧']);
    assert.deepEqual(labels(open('Patch1101.Credits')), ['뒤로', '취소']);
    assert.deepEqual(labels(open('CreditsNew')), ['더 보기', '뒤로', '취소']);
    assert.deepEqual(labels(open('About')), ['확인']);
    // 기본 확인 단추(OkButton)는 한국어 용어표에서 온다: 단추가 없는 섹션(영어에 `$Button` 이 없는 `Blank` 가 아닌 본문만 있는 경우)에 쓰인다.
    const plain = createTellWindow({ gameData: newData('korean'), resources: koreanResources, name: 'About', screen: { width: 1024, height: 768 }, hooks: {} }).window;
    assert.equal(plain.buttons[0].text, '확인');
    assert.deepEqual(open('UCampaign').menu.items.map((item) => item.label), ['초기 미션', '사제 훈련', '', '자유를 위한 투쟁', '국가의 탄생', '완전한 승리']);
    assert.deepEqual(labels(open('UCampaign')), ['뒤로']);
    assert.deepEqual(open('Demoville').menu.items.map((item) => item.label), ['폭풍은 몰아친다', '데모 "내 등을 지켜라"', '데모 "해체된 동맹"']);
});

// 영어 Tell 창은 이전과 같다: 본문 조각·단추 라벨이 Windows-1252 바이트(원본 글꼴)이다. (2026-10-31 WB03-O)
test('영어 Tell 창: 본문 조각과 단추는 원본 글꼴 바이트 그대로다', () => {
    const { window } = createTellWindow({ gameData: newData('english'), resources: englishResources, name: 'Credits', screen: { width: 1024, height: 768 } });
    assert.ok(window.buttons.every((button) => button.labelBytes instanceof Uint8Array && !(button.labelBytes instanceof Uint16Array)));
    const fragments = window.body.softLines.flatMap((line) => line.fragments);
    assert.ok(fragments.length > 0 && fragments.every((fragment) => fragment.bytes instanceof Uint8Array && !(fragment.bytes instanceof Uint16Array)));
    assert.deepEqual(window.buttons.map((button) => button.text), ['Cancel']);
});

// 한국어 도움말 창: Back·OK 단추가 번역되고 유니코드 글꼴이며, 한국어 도움말 파일이 아직 없어 영어 본문이 유니코드 글꼴로 나온다. (2026-10-31 WB03-O)
test('한국어 도움말 창: 단추 번역·유니코드 글꼴·영어 도움말 파일 대체', () => {
    const open = (data, resources, topic = 'F1Help') => createHelpWindow({ gameData: data, resources, topic, screen: { width: 1024, height: 768 } }).window;
    const window = open(newData('korean'), koreanResources);
    assert.deepEqual([window.backButton.text, window.okButton.text], ['뒤로', '확인']);
    assert.deepEqual([window.backButton.label, window.okButton.label], ['Back', 'OK'], '명령에는 영어 키를 쓴다');
    assert.ok(window.backButton.labelBytes instanceof Uint16Array && window.okButton.labelBytes instanceof Uint16Array);
    const fragments = window.body.softLines.flatMap((line) => line.fragments).filter((fragment) => !fragment.picture);
    assert.ok(fragments.length > 0 && fragments.every((fragment) => fragment.bytes instanceof Uint16Array));
    // 영어 도움말 창은 그대로다.
    const english = open(newData('english'), englishResources);
    assert.deepEqual([english.backButton.text, english.okButton.text], ['Back', 'OK']);
    assert.ok(english.backButton.labelBytes instanceof Uint8Array && !(english.backButton.labelBytes instanceof Uint16Array));
    // 없는 주제는 첫 주제라 뒤로 가기가 창을 닫는다(번역과 무관하게 이전처럼 동작).
    const missing = open(newData('korean'), koreanResources, 'NoSuchTopicAnywhere');
    assert.equal(missing.closed, false);
    missing.back();
    assert.equal(missing.closed, true);
});
