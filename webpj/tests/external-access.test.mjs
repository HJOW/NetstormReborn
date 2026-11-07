// 웹 런타임의 외부 접속 경계를 검사한다(docs/webpj-external-access-20261105.md 의 목록과 같은 범위).
//  1) www 소스에 같은 출처 자산 요청(AssetLoader 의 fetch) 밖의 네트워크·창 열기·주소 이동 API 와 절대 주소 문자열이 없다.
//  2) 자산 목록(manifest)의 모든 경로와 앱 적재(_load)의 실제 요청이 데이터 폴더(문서 기준 상대 URL) 아래에 머문다.
//  3) Tell/도움말 링크의 바깥 주소는 치환이 끝난 http/https/mailto 만 브라우저 새 탭/창으로 열고(열기 장치는 한 파일뿐),
//     file:·미치환 자리표시·호스트 없는 주소·다른 접두는 열지 않고 콘솔 안내만 하며, cmd: 명령과 도움말 주제만 내부에서 처리한다.
//  4) 구 검증 안내(UserNotUpdated)·Technical Help·서버 진단·$Zsend/$Input 등 외부 서비스 관련 기능은 웹에서 열리거나 실행되지 않는다.
// 이 검사는 파일 읽기와 가짜 fetch 만 쓰며 실제 네트워크·DNS·브라우저를 쓰지 않는다.
// (2026-11-05 WB02-E: AGENTS.md 의 외부 접속 기능 문서화 요구에 맞춘 웹 경계 검사로 추가)
// (2026-11-07 WB03-AD: 바깥 링크를 새 탭/창으로 열도록 3)번과 소스 감사의 열기 장치 한정을 갱신)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, WEBPJ_ROOT } from './helpers.mjs';
import { AssetLoader, MANIFEST_FILE } from '../www/assets/asset-loader.js';
import { KOREAN_BUNDLE } from '../www/assets/korean-assets.js';
import { Settings } from '../www/platform/settings.js';
import { prepareDialogContent } from '../www/ui/html-text.js';
import { buildHelpItems, buildOptionsItems } from '../www/ui/option-menus.js';
import { Netstorm } from '../www/netstorm.js';

// 웹 공개 루트(www)의 절대 경로이다. (2026-11-05 WB02-E)
const WWW_ROOT = path.join(WEBPJ_ROOT, 'www');

// 자산 목록이다. (2026-11-05 WB02-E)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, MANIFEST_FILE), 'utf8'));

// 검사용 가상 문서 주소이다. 데이터 폴더는 이 주소 기준 './data/' 로 풀려야 한다. (2026-11-05 WB02-E)
const DOCUMENT_URL = 'https://netstorm.example/game/';

// www 아래(데이터 폴더 제외)의 .js 파일 상대 경로 목록을 모은다. (2026-11-05 WB02-E)
function sourceFiles(directory = WWW_ROOT) {
    const files = [];
    // 하위 폴더를 차례로 훑는다. data 폴더(원본 자산 복사본)는 소스가 아니다.
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
        const full = path.join(directory, entry.name);
        if (entry.isDirectory()) {
            if (path.relative(WWW_ROOT, full) !== 'data') files.push(...sourceFiles(full));
        } else if (entry.name.endsWith('.js')) {
            files.push(path.relative(WWW_ROOT, full).split(path.sep).join('/'));
        }
    }
    return files.sort();
}

// 소스의 주석 줄과 줄 끝 주석(` // ` 관례)을 지운 코드 줄 목록이다. 주석의 과거 주소 설명은 감사 대상이 아니다. (2026-11-05 WB02-E)
function codeLines(relativePath) {
    const text = fs.readFileSync(path.join(WWW_ROOT, ...relativePath.split('/')), 'utf8');
    return text.split(/\r?\n/).map((line) => (line.trimStart().startsWith('//') ? '' : line.split(' // ')[0]));
}

// 금지하는 외부 접속·주소 이동 API 와 절대 주소 문자열 패턴이다. (2026-11-05 WB02-E)
const FORBIDDEN = Object.freeze([
    /\bXMLHttpRequest\b/, /\bWebSocket\b/, /\bEventSource\b/, /\bsendBeacon\b/, /\bRTCPeerConnection\b/, /\bimportScripts\b/,
    /\bserviceWorker\b/, /\bnew\s+(?:Shared)?Worker\b/, /\blocation\s*\.\s*(?:href|assign|replace)\b/, /\blocation\s*=[^=]/,
    /\b(?:globalThis|self|top|parent|opener|view)\s*\.\s*open\s*\(/, /\bdocument\s*\.\s*(?:write|domain)\b/,
    /['"`](?:https?|wss?|ftp|mailto|telnet|file):/i, /['"`]\/\/[a-z0-9]/i,
]);

// 소스 정적 감사: 금지 API 가 없고, fetch 는 앱 조립(netstorm.js)과 자산 적재기만 쓰며, `window.open(` 은 도움말 창 지역 변수의 주제 열기뿐이다.
test('외부 접속 소스 감사: 네트워크 API 는 같은 출처 자산 요청뿐이다', () => {
    // 음성 대조: 패턴이 실제 위반 예시를 잡고 같은 출처 적재 코드는 잡지 않는다.
    const caught = (line) => FORBIDDEN.some((pattern) => pattern.test(line));
    for (const bad of ["fetch('https://a.b/c')", 'const s = new WebSocket(url);', "globalThis.open('x')", 'view.open(address)', 'location.href = next;', "const u = '//cdn.example/x.js';", 'navigator.sendBeacon(u, d);']) assert.ok(caught(bad), bad);
    for (const good of ['const response = await this.fetchFunction(this.url(entry.path));', 'window.open(topic);', "if (a === b) return 'mailto';"]) assert.equal(caught(good), false, good);
    const files = sourceFiles();
    assert.ok(files.length > 50, `소스 ${files.length}개`);
    const violations = [];
    const fetchUsers = new Set();
    const windowOpen = [];
    const anchorUsers = [];
    // 파일마다 코드 줄을 검사한다.
    for (const file of files) {
        codeLines(file).forEach((line, index) => {
            for (const pattern of FORBIDDEN) if (pattern.test(line)) violations.push(`${file}:${index + 1}: ${line.trim()}`);
            if (/\bfetch\b/i.test(line)) fetchUsers.add(file);
            if (/\bwindow\s*\.\s*open\s*\(/.test(line)) windowOpen.push(file);
            if (/createElement\(\s*['"`]a['"`]\s*\)/.test(line)) anchorUsers.push(file);
        });
    }
    assert.deepEqual(violations, []);
    assert.deepEqual([...fetchUsers].sort(), ['assets/asset-loader.js', 'netstorm.js']);
    // 도움말 창 공장의 `window` 는 HelpWindow 지역 변수이며 window.open(topic) 은 첫 주제 열기이다(브라우저 창 열기 아님).
    assert.deepEqual(windowOpen, ['ui/help-window.js']);
    // 바깥 주소를 여는 앵커 요소(createElement('a'))는 열기 장치 한 파일에서만 만든다(WB03-AD). 다른 곳의 링크 열기·이동은 금지 패턴이 막는다.
    assert.deepEqual(anchorUsers, ['platform/external-opener.js']);
    assert.match(fs.readFileSync(path.join(WWW_ROOT, 'ui', 'help-window.js'), 'utf8'), /const window = new HelpWindow\(/);
    // 시작 문서는 바깥 스크립트·스타일·글꼴을 부르지 않는다.
    const html = fs.readFileSync(path.join(WWW_ROOT, 'index.html'), 'utf8');
    assert.equal(/\b(?:src|href)\s*=\s*["']?(?:[a-z][a-z0-9+.-]*:|\/\/)/i.test(html), false, 'index.html 의 바깥 주소');
});

// 자산 목록: 모든 경로가 상대 경로이고, 데이터 폴더 URL 아래로 풀린다(다른 출처·상위 폴더로 나가지 않음).
test('자산 목록: 모든 묶음 경로가 데이터 폴더 아래 같은 출처로 풀린다', () => {
    const base = new URL('./data/', DOCUMENT_URL).href;
    const loader = new AssetLoader({ baseUrl: base, fetchFunction: () => { throw new Error('검사에서 요청하지 않음'); } });
    assert.equal(loader.url(MANIFEST_FILE), `${base}${MANIFEST_FILE}`);
    const bundles = new Set();
    const outside = [];
    // 목록의 모든 항목을 확인한다.
    for (const entry of manifest.files) {
        bundles.add(entry.bundle);
        const url = loader.url(entry.path);
        if (/^[a-z][a-z0-9+.-]*:|^\/|\\|(^|\/)\.\.(\/|$)/i.test(entry.path) || !url.startsWith(base)) outside.push(`${entry.bundle}:${entry.path}`);
    }
    assert.deepEqual(outside, []);
    // 메뉴·음악·한국어·지형 등 모든 묶음이 검사에 포함되었다.
    assert.ok(bundles.has('menu') && bundles.has(KOREAN_BUNDLE) && bundles.size >= 3, [...bundles].join(','));
});

// 디스크의 데이터 폴더를 읽는 가짜 fetch 이다. 요청 주소를 urls 에 남기며, 데이터 폴더 밖 주소는 실패시킨다. (2026-11-05 WB02-E)
function recordingFetch(urls) {
    const base = new URL('./data/', DOCUMENT_URL).href;
    return async (url) => {
        urls.push(String(url));
        if (!String(url).startsWith(base)) return { ok: false, status: 404 };
        const relative = decodeURIComponent(String(url).slice(base.length));
        const bytes = fs.readFileSync(path.join(DATA_ROOT, ...relative.split('/')));
        return {
            ok: true,
            status: 200,
            json: async () => JSON.parse(bytes.toString('utf8')),
            arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
        };
    };
}

// 앱 적재: 영어·한국어 모두 실제 요청이 기본 데이터 폴더('./data/' 문서 기준) 아래뿐이며 manifest 와 해당 묶음 파일만 요청한다.
// 한국어 글꼴 등록은 가짜 문서에 FontFace 가 없어 실패하지만, 그 전의 요청 범위를 확인하는 데는 충분하다(실패 시 영어로 계속하는 기존 웹 적응).
test('앱 적재 요청: 언어별 묶음을 데이터 폴더 아래에서만 받는다', async (t) => {
    t.mock.method(console, 'warn', () => {});
    const base = new URL('./data/', DOCUMENT_URL).href;
    const expected = (bundles) => [MANIFEST_FILE, ...manifest.files.filter((entry) => bundles.includes(entry.bundle)).map((entry) => entry.path)].map((relative) => new AssetLoader({ baseUrl: base, fetchFunction: () => {} }).url(relative)).sort();
    // 두 언어를 차례로 적재한다.
    for (const [language, bundles] of [['english', ['menu']], ['korean', ['menu', KOREAN_BUNDLE]]]) {
        const urls = [];
        const app = new Netstorm({ language, fetch: recordingFetch(urls), audio: false });
        app._root = { ownerDocument: { baseURI: DOCUMENT_URL } };
        app._settings = new Settings();
        app._progress = { snapshot: () => ({}) };
        await app._load();
        assert.equal(app._state, 'clickToStart', `${language} 적재`);
        assert.ok(urls.every((url) => url.startsWith(base)), `${language}: ${urls.filter((url) => !url.startsWith(base)).join(',')}`);
        assert.deepEqual([...new Set(urls)].sort(), expected(bundles), `${language} 요청 목록`);
    }
});

// 링크 분배: 열 수 있는 바깥 주소(http/https/mailto)는 열기 장치로 넘기고, 열 수 없는 주소(file:·미치환 자리표시·호스트 없는 주소)는 이유와 함께 콘솔로만 알린다.
// cmd: 는 대화상자 명령으로, 주제는 도움말 창으로 가며 바깥 주소 처리를 건드리지 않는다. 실제 탭은 열지 않고 가짜 열기 장치로 받는다. (2026-11-07 WB03-AD)
test('링크의 바깥 주소: 열 수 있는 주소만 열기 장치로 가고 나머지는 이유와 함께 알린다', (t) => {
    const info = t.mock.method(console, 'info', () => {});
    const calls = [];
    const opened = [];
    const documentStub = { name: '가짜 문서' };
    const app = Object.create(Netstorm.prototype);
    app.options = { openExternal: (document, link) => { opened.push([document, link.kind, link.url]); return true; } };
    app._root = { ownerDocument: documentStub };
    app._runDialogCommand = (command) => calls.push(['command', command]);
    app._openHelp = (topic) => calls.push(['help', topic]);
    const follow = (destination) => Netstorm.prototype._followLink.call(app, destination);
    // 웹에서 닿는 자료의 바깥 링크 종류(http·mailto·file·치환되지 않은 주소)를 모두 넣어 본다. 치환되지 않은 자리표시와 file: 은 열지 않는다.
    const external = [
        ['http://www.netstormhq.com', 'web', 'http://www.netstormhq.com/'],
        ['http://te.netstormhq.com', 'web', 'http://te.netstormhq.com/'],
        ['http://{UPDATEURL}', null, null],
        ['mailto:{EMAIL1}', null, null],
        ['mailto:bugs@titanic.com', 'mail', 'mailto:bugs@titanic.com'],
        ['file:NetStormPieceList.html', null, null],
    ];
    for (const [destination] of external) follow(destination);
    assert.deepEqual(calls, []);
    assert.deepEqual(opened, external.filter(([, kind]) => kind).map(([, kind, url]) => [documentStub, kind, url]));
    const refused = external.filter(([, kind]) => !kind).map(([destination]) => destination);
    assert.equal(info.mock.callCount(), refused.length);
    assert.ok(info.mock.calls.every((call, index) => String(call.arguments[0]).includes(refused[index]) && String(call.arguments[0]).includes('열지 않습니다')));
    // 열기 장치가 실패(false)를 돌려주면 경고만 하고 게임 진행은 계속한다.
    const warn = t.mock.method(console, 'warn', () => {});
    app.options = { openExternal: () => false };
    assert.equal(app._openExternalAddress('https://example.org/a', '링크'), false);
    assert.equal(warn.mock.callCount(), 1);
    // 도움말 창의 바깥 주소 훅도 같은 판정으로 간다(출처 이름만 다르다).
    app.options = { openExternal: (document, link) => { opened.push(['도움말', link.url]); return true; } };
    assert.equal(app._openExternalAddress('http://www.activision.com', '도움말'), true);
    assert.deepEqual(opened.at(-1), ['도움말', 'http://www.activision.com/']);
    // 내부 처리: cmd: 명령과 도움말 주제.
    follow('cmd:Tell,TechSupport');
    follow('UnitsType');
    assert.deepEqual(calls, [['command', { name: 'Tell', argument: 'TechSupport' }], ['help', 'UnitsType']]);
});

// 외부 서비스 관련 기능: 시작 흐름은 UserNotUpdated 를 열지 않고, 웹 펼침 메뉴에는 Technical Help·서버 진단·DirectDraw 가 없으며,
// 미연결 명령은 아무것도 실행하지 않고, `$Zsend`/`$Input` 지시문은 컨트롤 없이 미지원 통지만 한다.
test('외부 서비스 기능: 구 검증·진단·외부 실행·서버 전송은 열리지 않는다', (t) => {
    const info = t.mock.method(console, 'info', () => {});
    // 시작 흐름: 첫 메뉴(환영 완료·팁 켜짐)에서 Blank → Tips 만 연다.
    const opened = [];
    const startup = {
        _gameData: { get: (key) => ({ WelcomeGiven: '1', tellTips: '1' })[key] ?? null },
        _mainMenu: {},
        _firstMenuPending: true,
        _openTell: (name) => opened.push(name),
    };
    Netstorm.prototype._runMainMenuStartup.call(startup);
    assert.deepEqual(opened, ['Blank', 'Tips']);
    // 환영 전에도 Welcome 만 연다.
    const welcome = [];
    Netstorm.prototype._runMainMenuStartup.call({ ...startup, _gameData: { get: () => '0' }, _openTell: (name) => welcome.push(name) });
    assert.deepEqual(welcome, ['Welcome']);
    // 웹 펼침 메뉴의 명령에는 외부 실행·서버 진단·전체화면 전환이 없다(원본 비교용 includeAll 에는 있다).
    const commands = (items) => items.flatMap((item) => (item.items ? commands(item.items) : item.command ? [`${item.command.name}:${item.command.argument}`] : []));
    const web = [...commands(buildHelpItems()), ...commands(buildOptionsItems(new Settings()))];
    for (const hidden of ['TechnicalHelp:0', 'Tell:ChangeFirewall', 'Tell:DirectDrawManager']) {
        assert.equal(web.includes(hidden), false, hidden);
        assert.equal([...commands(buildHelpItems({ includeAll: true })), ...commands(buildOptionsItems(new Settings(), { includeAll: true }))].includes(hidden), true, `원본 비교용 ${hidden}`);
    }
    // 미연결 대화상자 명령은 안내만 한다(창·명령·팁 처리 없음).
    const calls = [];
    const dialog = { _openTell: (name) => calls.push(name), _runScriptCommand: () => calls.push('script'), _runTellTip: () => calls.push('tip') };
    for (const name of ['TechnicalHelp', 'Zsend', 'StartDemo', 'MissionBegin']) Netstorm.prototype._runDialogCommand.call(dialog, { name, argument: 'x' });
    assert.deepEqual(calls, []);
    assert.equal(info.mock.callCount(), 4);
    // `$Zsend`/`$Input` 지시문은 버튼·목록 행을 만들지 않고 미지원으로 알린다(구 서버 전송·입력 창 미구현).
    const unsupported = [];
    const content = prepareDialogContent('<h2>Server</h2>\n$Zsend=Send,Zsend,47\n$Input=Name,Config,name\n$Button=OK,Tell,Blank\n', { substitute: (text) => text, hooks: { onUnsupported: (what) => unsupported.push(what) } });
    // 통지 순서는 본문 순서가 아니라 원본 지시문 추출 우선순위(004c83b0: … Input → Zsend)이다.
    assert.deepEqual(unsupported, ['지시문 input', '지시문 zsend']);
    assert.deepEqual(content.directives.buttons.map((button) => button.label), ['OK']);
    assert.deepEqual(content.directives.menuItems, []);
});
