// 링크 분배와 Tell 대화상자의 링크·본문 스크롤바를 검사한다: 도움말 링크 분류(원본 x86 기대값 91행), 실제 도움말·Tell 자료의 링크 분류,
// Tell 본문 링크 누름(목적지 전달·링크가 아닌 글자 구분), 긴 본문의 스크롤바(생성·배치·입력·다시 맞춤).
// 기대값 파일(cpppj/tests/fixtures/help-links-x86.tsv)은 cpppj 가 보호된 원본 10.78 의 링크 분배 함수를 격리 실행해 만든 것이며 읽기 전용으로만 쓴다.
// 행 수를 고정해 두어 cpppj 쪽 파일이 바뀌면 이 검사가 실패해 다시 확인하게 한다.
// (2026-10-31 WB03-K: Tell 대화상자 링크 훅과 본문 스크롤바 이식 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { HELP_LINK_BODY_LIMIT, HELP_LINK_PROTOCOL_LIMIT, classifyHelpLink, dispatchHelpLink, parseHelpLinkAddress } from '../www/ui/help-link.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';
import { parseScriptCommand } from '../www/ui/script-command.js';
import { measureFragment } from '../www/ui/style-text.js';
import { createTellWindow } from '../www/ui/tell-window.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';

// cpppj 기대값 파일의 폴더이다. (2026-10-31 WB03-K)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');

// 메뉴 묶음 파일 전부를 읽어 자원을 만든다(검사마다 공유). (2026-10-31 WB03-K)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// 논리 화면 크기이다(4:3). (2026-10-31 WB03-K)
const SCREEN = Object.freeze({ width: 1024, height: 768 });

// 새 게임 데이터를 만든다. extra 는 추가·대체할 느슨한 파일(경로 → 바이트)이다. (2026-10-31 WB03-K)
function newData({ extra = [] } = {}) {
    return GameData.build({ files: new Map([...files, ...extra]), language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
}

// 16진수 칸을 바이트 하나가 문자 하나인 문자열로 바꾼다. '-' 는 빈 문자열이다. (2026-10-31 WB03-K)
function unhex(field) {
    return field === '-' ? '' : Buffer.from(field, 'hex').toString('latin1');
}

// 바이트 문자열을 16진수로 바꾼다(빈 문자열은 빈 문자열). (2026-10-31 WB03-K)
function hex(text) {
    return Buffer.from(text, 'latin1').toString('hex');
}

// 기대값 파일의 데이터 줄(주석 제외)을 칸으로 나눠 읽는다. 파일이 없으면 null 이다. (2026-10-31 WB03-K)
function readRows(name) {
    const filePath = path.join(FIXTURES, name);
    if (!fs.existsSync(filePath)) return null;
    return fs.readFileSync(filePath, 'utf8').split('\n').filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.replace(/\r$/, '').split('\t'));
}

// 원본 기대값 91행: 분류(명령/도움말/바깥 주소)와 넘겨 받은 문자열이 같다. 바깥 주소 분기의 호출 순서는 설정 조회(autoOpen)와 셸 실행·접두별 명령 설정을 포함하는데,
// 웹에는 바깥 프로그램 실행이 없으므로 접두가 넘어가는 지점(autoOpen 조회 → 접두가 autoOpen 에 있으면 전체 원문으로 셸 열기 → 아니면 접두 조회)까지만 재현해 앞부분을 대조한다.
// 중간에 중단시킨 10행(stop 열이 0 이 아님)은 관측한 호출 수가 일부뿐이라 제외한다. (2026-10-31 WB03-K)
test('도움말 링크: 분류·접두·본문이 원본 x86 기대값 91행과 같다', (t) => {
    const rows = readRows('help-links-x86.tsv');
    if (!rows) {
        t.skip('cpppj/tests/fixtures/help-links-x86.tsv 가 없는 환경');
        return;
    }
    assert.equal(rows.length, 91);
    const compared = { script: 0, help: 0, external: 0 };
    let skipped = 0;
    // 기대값 행을 하나씩 분배해 호출된 순서를 기대 문자열의 앞부분과 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const [name, originalField, automaticField, shellField, , stopField, , traceField] = row;
        if (stopField !== '0') {
            skipped += 1;
            continue;
        }
        const automatic = unhex(automaticField);
        const shell = shellField === '1';
        const events = [];
        const kind = dispatchHelpLink(unhex(originalField), {
            script: (body) => events.push(`C,${hex(body)}`),
            help: (body) => events.push(`H,${hex(body)},1,0`),
            external: (original, protocol) => {
                events.push('P,6175746f4f70656e');
                if (automatic.includes(protocol)) {
                    events.push(`O,${hex(original)}`);
                    if (shell) return;
                }
                events.push(`R,${hex(protocol)},`);
            },
        });
        compared[kind] += 1;
        const mine = events.join(';');
        assert.ok(traceField === mine || traceField.startsWith(`${mine};`), `${name}: 기대 ${traceField} / 결과 ${mine}`);
    }
    assert.equal(skipped, 10);
    assert.deepEqual(compared, { script: 6, help: 10, external: 65 });
});

// 분류 규칙의 경계: cmd: 는 접두 앞 네 글자 비교(대소문자 무시), 콜론이 없거나 본문이 #로 시작하면 도움말 주제, 나머지는 바깥 주소.
// 접두·본문 한도와 NUL 이후 무시도 원본과 같다. (2026-10-31 WB03-K)
test('도움말 링크: 분류 규칙과 한도 경계', () => {
    assert.deepEqual(parseHelpLinkAddress('cmd:Tell,TechSupport'), { protocol: 'cmd:', body: 'Tell,TechSupport' });
    assert.deepEqual(parseHelpLinkAddress('stormPowerHelp'), { protocol: '', body: 'stormPowerHelp' });
    // 연속 콜론은 함께 건너뛰고 접두 앞 공백과 따옴표 글자는 접두에 들어가지 않는다.
    assert.deepEqual(parseHelpLinkAddress('CMD::Go()'), { protocol: 'CMD:', body: 'Go()' });
    assert.deepEqual(parseHelpLinkAddress('  "cmd":Go()'), { protocol: 'cmd:', body: 'Go()' });
    assert.equal(classifyHelpLink('cmd:Tell,TechSupport'), 'script');
    assert.equal(classifyHelpLink('CMD:Tell,TechSupport'), 'script');
    assert.equal(classifyHelpLink('cmdx:y'), 'external');
    assert.equal(classifyHelpLink('stormPowerHelp'), 'help');
    assert.equal(classifyHelpLink('#frag'), 'help');
    assert.equal(classifyHelpLink('topic:#frag'), 'help');
    assert.equal(classifyHelpLink('topic:frag'), 'external');
    assert.equal(classifyHelpLink('mailto:a@b.c'), 'external');
    assert.equal(classifyHelpLink('http://www.netstormhq.com'), 'external');
    assert.equal(classifyHelpLink('file:NetStormPieceList.html'), 'external');
    // 도움말 주제의 본문은 목적지 전체이고, 바깥 주소는 전체 원문·접두·본문을 넘긴다. 첫 NUL 뒤는 읽지 않는다.
    const seen = [];
    dispatchHelpLink('http://a\0hidden', { external: (...values) => seen.push(values) });
    assert.deepEqual(seen, [['http://a', 'http:', '//a']]);
    const topics = [];
    assert.equal(dispatchHelpLink('#frag', { help: (body) => topics.push(body) }), 'help');
    assert.deepEqual(topics, ['#frag']);
    // 훅이 없으면 분류만 하고 부르지 않는다.
    assert.equal(dispatchHelpLink('cmd:Go', {}), 'script');
    // 본문이 256 이상이면 RangeError 이고 훅은 부르지 않는다. 접두는 토큰 읽기가 78글자에서 끊어 79글자를 넘지 않는다.
    assert.equal(HELP_LINK_BODY_LIMIT, 256);
    assert.equal(HELP_LINK_PROTOCOL_LIMIT, 80);
    assert.deepEqual(parseHelpLinkAddress('a'.repeat(255)).body.length, 255);
    let called = false;
    assert.throws(() => dispatchHelpLink('a'.repeat(256), { help: () => { called = true; } }), RangeError);
    assert.equal(called, false);
    assert.equal(parseHelpLinkAddress(`${'p'.repeat(100)}:tail`).protocol.length, 79);
});

// 실제 도움말 파일의 모든 주제에서 링크를 모아 분류한다. 이전 판정(cmd: 접두 / 스킴:// 또는 mailto / 그 밖은 주제)과 모두 같아야 도움말 창의 동작이 달라지지 않는다. (2026-10-31 WB03-K)
test('실제 도움말 링크: 모든 주제의 분류가 이전 판정과 같다', () => {
    const data = newData();
    const helpText = data.readText(data.get('Quick Help Spec'));
    // 앵커 138개 중 대소문자만 다른 중복 10개를 뺀 128개 주제를 연다.
    const topics = [...new Set([...helpText.matchAll(/<a\s+name\s*=\s*"([^"]+)"/gi)].map((match) => match[1].toLowerCase()))];
    assert.equal(topics.length, 128);
    const previous = (destination) => (/^cmd:/i.test(destination) ? 'script' : /^[a-z][a-z0-9+.-]*:\/\//i.test(destination) || /^mailto:/i.test(destination) ? 'external' : 'help');
    const counts = { script: 0, help: 0, external: 0 };
    // 주제마다 링크 목적지를 모아 새 분류와 이전 분류를 비교한다.
    for (const topic of topics) {
        const result = renderHelpTopic({ helpText, topic, substitute: (text) => data.substitute(text) });
        for (const destination of result.links.destinations) {
            const kind = classifyHelpLink(destination);
            assert.equal(kind, previous(destination), `${topic}: ${destination}`);
            counts[kind] += 1;
        }
    }
    assert.ok(counts.help > 100 && counts.external > 0 && counts.script >= 1);
});

// Tell 창을 만들어 링크·명령·소리 호출을 events 에 쌓는다. 시계는 clock.now 를 바꿔 진행한다. (2026-10-31 WB03-K)
function newTell(name, { screen = SCREEN, data = newData() } = {}) {
    const events = { links: [], commands: [], sounds: [], unsupported: [] };
    const clock = { now: 0 };
    const result = createTellWindow({
        gameData: data,
        resources,
        name,
        screen,
        now: () => clock.now,
        onCommand: (command) => events.commands.push(command),
        onLink: (destination) => events.links.push(destination),
        onSound: (kind) => events.sounds.push(kind),
        onUnsupported: (what) => events.unsupported.push(what),
    });
    assert.ok(result.window, `${name} 창을 만들지 못했습니다`);
    return { window: result.window, events, clock, data };
}

// 링크 중 destination 을 가진 첫 글자 조각의 화면 위치를 찾는다. Tell 본문에서 링크를 누를 좌표를 만든다. (2026-10-31 WB03-K)
function findLink(window, destination) {
    const body = window.body;
    // 줄 나눔이 오래됐으면(맞춤 뒤 폭만 바뀐 경우) 다시 나눈 표시 줄로 찾는다.
    body.ensureLayout();
    const bounds = body.bounds();
    for (const line of body.softLines) {
        let width = 0;
        for (const fragment of line.fragments) {
            if (fragment.tag && window.links.resolve(fragment.tag) === destination) {
                const x = bounds.left + width + 2;
                const y = bounds.top + line.y - body.scroll + line.height - 2;
                if (body.hitTest(x, y)?.tag === fragment.tag) return { x, y };
            }
            width += measureFragment(body.fonts, fragment).width;
        }
    }
    throw new Error(`링크를 찾지 못했습니다: ${destination}`);
}

// Tell 본문의 링크: 섹션마다 목적지를 모아 두고, 누르면 목적지가 호스트로 넘어간다(효과음 없음). 링크가 아닌 글자·링크 뒤 글자는 아무 일도 없다. (2026-10-31 WB03-K)
test('Tell 링크: 실제 섹션의 링크 목적지와 누름', () => {
    const cases = [
        ['TechSupport', ['mailto:www.netstormhq.com'], ['external']],
        ['BuildNoBucks', ['stormPowerHelp'], ['help']],
        ['UpgradeNoBucks', ['stormPowerHelp'], ['help']],
        ['NoTechTheme', ['factoryHelp', 'technologyHelp', 'unitHelp'], ['help', 'help', 'help']],
        ['ChalIntro', ['multiQuickHelp', 'multiQuickHelp', 'battleIslandType', 'challengeIslandType'], ['help', 'help', 'help', 'help']],
        ['PlayerInfo(a,b,c,d)', ['cmd:Tell,CraftWarning(a,b,c,d)'], ['script']],
        ['Tips', ['http://www.netstormhq.com'], ['external']],
        ['PieceDump', ['file:NetStormPieceList.html'], ['external']],
    ];
    // 섹션마다 목적지 표와 분류가 기대와 같고, 각 목적지를 누르면 그 목적지가 전달된다.
    for (const [name, destinations, kinds] of cases) {
        const { window, events } = newTell(name);
        assert.deepEqual(window.links.destinations, destinations, name);
        assert.deepEqual(destinations.map((destination) => classifyHelpLink(destination)), kinds, name);
        for (const destination of new Set(destinations)) {
            const point = findLink(window, destination);
            assert.equal(window.pointerDown(point.x, point.y), true, name);
            assert.equal(events.links.at(-1), destination, name);
            window.pointerUp(point.x, point.y);
        }
        assert.equal(events.links.length, new Set(destinations).size, name);
        assert.deepEqual(events.sounds, [], `${name}: 링크는 소리가 없다`);
        assert.deepEqual(events.commands, [], `${name}: 링크는 버튼 명령이 아니다`);
    }
});

// 링크 목적지가 연결하는 곳이 실제로 있다: 도움말 링크는 도움말 주제가 열리고, 명령 링크는 이름과 인자로 나뉜다. (2026-10-31 WB03-K)
test('Tell 링크: 도움말 주제가 열리고 명령 링크가 이름과 인자로 나뉜다', () => {
    const data = newData();
    const helpText = data.readText(data.get('Quick Help Spec'));
    // Tell 섹션의 모든 도움말 링크 주제가 도움말 파일에 있다.
    for (const topic of ['stormPowerHelp', 'factoryHelp', 'technologyHelp', 'unitHelp', 'multiQuickHelp', 'battleIslandType', 'challengeIslandType']) {
        const found = renderHelpTopic({ helpText, topic, substitute: (text) => data.substitute(text) }).found;
        assert.equal(found, true, topic);
    }
    const { window, events } = newTell('PlayerInfo(a,b,c,d)', { data });
    const point = findLink(window, 'cmd:Tell,CraftWarning(a,b,c,d)');
    window.pointerDown(point.x, point.y);
    const commands = [];
    dispatchHelpLink(events.links[0], { script: (body) => commands.push(parseScriptCommand(body)) });
    assert.deepEqual(commands, [{ name: 'Tell', argument: 'CraftWarning(a,b,c,d)', number: null }]);
});

// 링크가 아닌 곳: 링크가 없는 섹션·링크 앞뒤 글자·창 안 빈 곳을 눌러도 목적지가 전달되지 않는다. 닫힌 창은 입력을 받지 않는다. 링크와 돌 버튼은 서로 방해하지 않는다. (2026-10-31 WB03-K)
test('Tell 링크: 링크가 아닌 곳은 전달하지 않고 버튼과 섞이지 않는다', () => {
    const credits = newTell('Credits');
    assert.equal(credits.window.links.destinations.length, 0);
    const creditsBody = credits.window.body.bounds();
    credits.window.pointerDown(creditsBody.left + 3, creditsBody.top + 3);
    credits.window.pointerUp(creditsBody.left + 3, creditsBody.top + 3);
    assert.deepEqual(credits.events.links, []);
    // 링크 글자 위가 아닌 같은 줄 앞부분(You can Email ...)과 패널 가장자리를 누른다.
    const support = newTell('TechSupport');
    const body = support.window.body.bounds();
    support.window.pointerDown(body.left + 3, body.top + 3);
    support.window.pointerUp(body.left + 3, body.top + 3);
    assert.equal(support.window.pointerDown(support.window.bounds().left + 2, support.window.bounds().top + 2), true);
    assert.deepEqual(support.events.links, []);
    // 링크를 누른 뒤에도 돌 버튼(OK)은 평소처럼 소리와 함께 명령을 낸다.
    const point = findLink(support.window, 'mailto:www.netstormhq.com');
    support.window.pointerDown(point.x, point.y);
    assert.deepEqual(support.events.links, ['mailto:www.netstormhq.com']);
    const ok = support.window.buttons[0].bounds();
    support.window.pointerDown(ok.left + 5, ok.top + 5);
    support.window.pointerUp(ok.left + 5, ok.top + 5);
    assert.deepEqual(support.events.sounds, ['button']);
    assert.equal(support.events.commands.length, 1);
    // 닫힌 창은 입력과 링크를 받지 않는다.
    support.window.close();
    assert.equal(support.window.pointerDown(point.x, point.y), false);
    assert.equal(support.events.links.length, 1);
});

// 긴 본문(화면에 들어가지 않는) 시험용 Tell 섹션을 가진 게임 데이터를 만든다. 줄마다 줄바꿈이 있는 90줄이다. (2026-10-31 WB03-K)
function longData() {
    const lines = [];
    // 번호가 붙은 시험 줄을 90개 만든다.
    for (let number = 1; number <= 90; number += 1) lines.push(`Line ${number}: The quick brown fox jumps over the lazy dog, number ${number}.<br>`);
    const script = ['[LongText]', '<h2>{H2}Long</h2>', '<p>{Text}', ...lines, '$Button=Close,DoNothing', ''].join('\r\n');
    return newData({ extra: [['d/tell.english', new TextEncoder().encode(script)]] });
}

// 본문이 화면에 들어가지 않으면 스크롤바가 달린다: 폭은 화면 안에 맞고 높이는 폭의 절반이며, 스크롤바는 본문 오른쪽 가장자리에 붙고 범위·쪽 크기가 본문과 맞는다. (2026-10-31 WB03-K)
test('Tell 스크롤바: 긴 본문에서 생성되고 본문 오른쪽에 붙는다', () => {
    const { window } = newTell('LongText', { data: longData() });
    const body = window.body;
    const slider = body.slider;
    assert.ok(slider, '스크롤바가 없다');
    assert.equal(body.useSlider, true);
    // 본문 높이는 폭의 절반이고 패널은 화면 안에 있다. 전체 본문은 훨씬 길다.
    assert.equal(body.baseHeight, body.baseWidth * 0.5);
    assert.ok(body.totalHeight() > 10 * body.baseHeight);
    const rect = window.bounds();
    assert.ok(rect.left >= 0 && rect.top >= 0 && rect.right <= SCREEN.width && rect.bottom <= SCREEN.height);
    // 스크롤바는 본문 오른쪽 가장자리(본문 폭 - 9)에 붙고 높이가 본문 높이와 같다. 범위는 전체 높이, 쪽 크기는 본문 높이(절삭)이며 위치는 맨 위다.
    assert.equal(slider.x, Math.fround(body.x + (body.baseWidth - 9)));
    assert.equal(slider.y, body.y);
    assert.equal(slider.baseHeight, body.baseHeight);
    assert.deepEqual([slider.minimum, slider.maximum, slider.page, slider.position], [0, body.totalHeight(), Math.trunc(body.baseHeight), 0]);
    assert.equal(body.scroll, 0);
    // 줄 나눔 폭은 스크롤바 폭을 뺀다: 가장 넓은 표시 줄이 본문 폭 - 9 보다 좁다.
    assert.ok(body.maximumWidth() < body.baseWidth - 9);
    // 화면 크기에 맞는 짧은 섹션에는 스크롤바가 없다.
    const credits = newTell('Credits');
    assert.equal(credits.window.body.slider, null);
    assert.equal(credits.window.body.useSlider, false);
    assert.equal(credits.window.wheel(100), false);
    assert.deepEqual(credits.events.unsupported, []);
    assert.deepEqual(newTell('LongText', { data: longData() }).events.unsupported, []);
});

// 스크롤바 입력: 휠(웹 적응)·화살표(한 단계 5)·누르고 있기 반복·홈(쪽 이동)·손잡이 끌기·본문 글자 끌기, 한계에서 멈춤, 효과음은 화살표를 누른 것뿐이다. (2026-10-31 WB03-K)
test('Tell 스크롤바: 휠 · 화살표 · 반복 · 쪽 · 손잡이 · 본문 끌기', () => {
    const { window, events, clock } = newTell('LongText', { data: longData() });
    const body = window.body;
    const slider = body.slider;
    const maximumScroll = body.totalHeight() - Math.trunc(body.baseHeight);
    // 휠: 아래로 굴리면 세 줄(45), 위로 굴리면 되돌아오고 0 아래로는 가지 않는다. 0 은 처리하지 않는다.
    assert.equal(window.wheel(100), true);
    assert.equal(body.scroll, 45);
    assert.equal(slider.position, 45);
    assert.equal(window.wheel(-100), true);
    assert.equal(window.wheel(-100), true);
    assert.equal(body.scroll, 0);
    assert.equal(window.wheel(0), false);
    // 아래 화살표: 안쪽에서 떼면 5픽셀, 눌러 두면 0.4초 뒤부터 0.05초 간격으로 5픽셀씩 간다.
    const down = slider.increment.bounds();
    clock.now = 10;
    window.pointerDown(down.left + 3, down.top + 3);
    window.pointerUp(down.left + 3, down.top + 3);
    assert.equal(body.scroll, 5);
    clock.now = 20;
    window.pointerDown(down.left + 3, down.top + 3);
    clock.now = 20.3;
    assert.equal(window.update(21), false);
    clock.now = 20.41;
    assert.equal(window.update(21), true);
    clock.now = 20.5;
    assert.equal(window.update(21), true);
    assert.equal(body.scroll, 15);
    window.pointerUp(5, 5);
    assert.equal(body.scroll, 15);
    // 손잡이 아래 홈을 누르면 한 쪽(본문 높이)씩 아래로, 위 홈은 한 쪽 위로 간다.
    const page = Math.trunc(body.baseHeight);
    const thumb = slider.thumbBounds();
    body.setScroll(500);
    window.pointerDown(thumb.left + 3, thumb.bottom - 3);
    window.pointerUp(thumb.left + 3, thumb.bottom - 3);
    assert.equal(body.scroll, 500 + page);
    window.pointerDown(thumb.left + 3, thumb.top + 3);
    window.pointerUp(thumb.left + 3, thumb.top + 3);
    assert.equal(body.scroll, 500);
    // 손잡이를 끌면 위치가 따라온다: 위로 끌면 줄고 아래로 끌면 는다.
    const grip = slider.handle.top + Math.trunc((slider.handle.bottom - slider.handle.top) / 2);
    window.pointerDown(thumb.left + 3, thumb.top + grip);
    window.pointerMove(thumb.left + 3, thumb.top + grip + 5);
    assert.ok(body.scroll > 500);
    window.pointerMove(thumb.left + 3, thumb.top + grip - 20);
    assert.ok(body.scroll < 500);
    window.pointerUp(thumb.left + 3, thumb.top + grip - 20);
    // 링크가 아닌 본문 글자를 누른 채 끌면 끄는 반대 방향으로 스크롤되고, 위쪽·아래쪽 한계에서 멈춘다.
    body.setScroll(100);
    const bounds = body.bounds();
    const startY = bounds.top + 10;
    window.pointerDown(bounds.left + 20, startY);
    assert.equal(window.dragY, startY);
    window.pointerMove(bounds.left + 20, startY - 30);
    assert.equal(body.scroll, 130);
    window.pointerMove(bounds.left + 20, startY + 50);
    assert.equal(body.scroll, 50);
    window.pointerMove(bounds.left + 20, startY + 500);
    assert.equal(body.scroll, 0);
    window.pointerUp(bounds.left + 20, startY + 500);
    assert.equal(window.dragY, null);
    // 아래쪽 한계: 전체 높이 - 본문 높이에서 멈춘다.
    body.setScroll(1e9);
    assert.equal(body.scroll, maximumScroll);
    assert.equal(slider.position, maximumScroll);
    window.wheel(100);
    assert.equal(body.scroll, maximumScroll);
    // 진행 중인 누름·끌기는 취소하면 풀린다.
    window.pointerDown(bounds.left + 20, startY);
    assert.equal(window.cancel(), true);
    assert.equal(window.dragY, null);
    // 효과음은 화살표를 누른 두 번뿐이다(휠·홈·손잡이·본문 끌기는 소리가 없다).
    assert.deepEqual(events.sounds, ['button', 'button']);
});

// 스크롤된 본문의 그리기와 링크 판정: 스크롤하면 본문 영역 그림이 달라지고 스크롤바 영역이 그려진다. 스크롤바를 달지 않은 짧은 본문은 끌어도 움직이지 않는다. (2026-10-31 WB03-K)
test('Tell 스크롤바: 스크롤된 그리기 · 짧은 본문은 끌기가 없다', () => {
    const { window } = newTell('LongText', { data: longData() });
    const body = window.body;
    const surface = new IndexedSurface(SCREEN.width, SCREEN.height);
    window.draw(surface);
    const bounds = body.bounds();
    // 본문 영역과 스크롤바 영역의 색 번호를 복사한다.
    const snapshot = () => {
        const copy = [];
        for (let y = bounds.top; y < bounds.bottom; y += 1) copy.push(Array.from(surface.pixels.subarray(y * surface.width + bounds.left, y * surface.width + bounds.right)));
        return copy;
    };
    const first = snapshot();
    // 스크롤바를 보이지 않게 하고 다시 그리면 달라지는 픽셀이 있고, 그 픽셀은 모두 스크롤바 사각형(폭 9, 본문 높이) 안에 있다.
    const slider = body.slider;
    const before = Uint8Array.from(surface.pixels);
    slider.visible = false;
    window.draw(surface);
    slider.visible = true;
    const sliderLeft = Math.trunc(slider.x);
    const sliderTop = Math.trunc(slider.y);
    let differing = 0;
    // 두 그림을 픽셀마다 비교해 다른 곳이 스크롤바 사각형 밖에 있으면 실패시킨다.
    for (let index = 0; index < before.length; index += 1) {
        if (before[index] === surface.pixels[index]) continue;
        const x = index % surface.width;
        const y = Math.trunc(index / surface.width);
        assert.ok(x >= sliderLeft && x < sliderLeft + 9 && y >= sliderTop && y < sliderTop + Math.trunc(slider.baseHeight) + 1, `스크롤바 밖 픽셀이 달라졌다: ${x},${y}`);
        differing += 1;
    }
    assert.ok(differing > 100, `스크롤바가 그려지지 않았다(차이 ${differing}픽셀)`);
    window.draw(surface);
    body.setScroll(300);
    window.draw(surface);
    assert.notDeepEqual(snapshot(), first);
    body.setScroll(0);
    window.draw(surface);
    assert.deepEqual(snapshot(), first);
    // 스크롤바 없는 짧은 본문은 글자를 눌러 끌어도 움직이지 않는다.
    const credits = newTell('Credits');
    const creditsBody = credits.window.body.bounds();
    credits.window.pointerDown(creditsBody.left + 5, creditsBody.top + 5);
    assert.equal(credits.window.dragY, null);
    assert.equal(credits.window.pointerMove(creditsBody.left + 5, creditsBody.top - 30), false);
    assert.equal(credits.window.body.scroll, 0);
});

// 화면 크기가 바뀌면 다시 맞춘다: 화면이 커서 다 들어가면 스크롤바와 스크롤 위치·끌기가 사라지고, 다시 작아지면 스크롤바가 처음부터 다시 생긴다. 메인 메뉴 대화상자 자리에서도 같다. (2026-10-31 WB03-K)
test('Tell 스크롤바: 화면 크기 변경 때 스크롤바를 다시 맞춘다', () => {
    const { window } = newTell('LongText', { data: longData() });
    const body = window.body;
    body.setScroll(400);
    window.pointerDown(body.bounds().left + 20, body.bounds().top + 10);
    assert.notEqual(window.dragY, null);
    // 아주 큰 화면에서는 모두 들어가므로 스크롤바가 사라진다.
    window.screen = { width: 1024, height: 4000 };
    window.fit();
    window.place();
    assert.equal(body.slider, null);
    assert.equal(body.useSlider, false);
    assert.equal(body.scroll, 0);
    assert.equal(window.dragY, null);
    assert.equal(window.wheel(100), false);
    assert.equal(body.baseHeight, body.totalHeight() + 1);
    // 다시 작은 화면이 되면 스크롤바가 처음 상태(맨 위)로 다시 생긴다.
    window.screen = { width: 1366, height: 768 };
    window.fit();
    window.place();
    assert.ok(body.slider);
    assert.equal(body.scroll, 0);
    assert.equal(body.slider.position, 0);
    // 메인 메뉴 대화상자 자리: 크기가 바뀌면 다시 맞추고 가운데에 놓으며 휠이 전달된다.
    const screen = new MainMenuScreen({ resources });
    screen.resize(1024, 768);
    const opened = newTell('LongText', { data: longData() });
    screen.setDialog(opened.window);
    assert.ok(opened.window.body.slider);
    const surface = new IndexedSurface(1024, 768);
    screen.render(surface);
    assert.equal(screen.dialog.wheel(100), true);
    assert.equal(opened.window.body.scroll, 45);
    screen.resize(1366, 768);
    assert.ok(opened.window.body.slider);
    assert.equal(opened.window.body.scroll, 0);
    assert.equal(opened.window.bounds().left, Math.trunc((1366 - opened.window.baseWidth) / 2));
});
