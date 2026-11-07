// webpj 를 브라우저 없이 Node 에서 실행해 논리 화면(8비트 색 번호 → 팔레트 RGB)과 관측 보고서를 얻는 무브라우저 실행기이다.
// 실제 앱 객체(www/netstorm.js 의 Netstorm)의 자산 적재(_load)·시작 입력(_activate)·프레임(_onFrame)·포인터/키 처리기(_onPointerDown 등)를 그대로 부르고,
// DOM 에 의존하는 표시 경계만 대체한다: 루트 문서(제목·baseURI)·Canvas(좌표 사각형·커서 스타일)·표시기(Canvas 그리기)·fetch(디스크의 www/data 읽기).
// 그래서 메뉴·대화상자·도움말의 명령 분배는 브라우저와 같은 코드이며, 이 도구가 따로 흉내 내는 게임 동작은 없다.
// 단계표는 cpppj `--ui-script` 와 같은 [프레임, 동작, 인자] 이다(tests/fixtures/cpppj/*-scenarios.json). 지원 동작: click(라벨 또는 screen:x,y)·down·up·esc·key·snapshot·report.
// 프레임 순서도 cpppj 와 같다: 그 프레임의 입력 → 프레임 처리(갱신·합성) → 그 프레임의 snapshot/report.
// 사용:
//   node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json [--id campaign] --out <폴더>
//     → <폴더>/<id>/<이름>.bmp(cpppj 캡처와 같은 32비트 BMP)와 <폴더>/<id>/report.txt 를 쓴다.
//   검사·비교 도구는 runScenario() 를 불러 메모리에서 그림을 받는다.
// 한계(2026-11-07): 영어만 지원한다(한국어는 Canvas 2D 글리프 래스터라이저가 필요). 메인 메뉴 장면(M1)만 다루며 미션 월드는 아직 없다.
// (2026-11-07 WB02-H: LEFT_JOBS.webpj.md 3.2절 순서2 ①의 무브라우저 실행·논리 화면/보고서 요구로 추가)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { AppState, Netstorm } from '../www/netstorm.js';
import { Settings, findSettingKey } from '../www/platform/settings.js';
import { Progress, isProgressKey } from '../www/platform/progress.js';
import { TellWindow } from '../www/ui/tell-window.js';
import { HelpWindow } from '../www/ui/help-window.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { encodeCppBitmap, indexedToRgb } from './image-files.mjs';

// 저장소 루트의 절대 경로이다. (2026-11-07 WB02-H)
export const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// 웹 공개 데이터 폴더(www/data)의 기본 절대 경로이다. (2026-11-07 WB02-H)
export const DEFAULT_DATA_ROOT = path.join(REPO_ROOT, 'webpj', 'www', 'data');

// 한 프레임의 시간(밀리초)이다. 브라우저의 requestAnimationFrame 시각 자리에 프레임 번호 × 이 값을 넘긴다.
// cpppj `--help` 의 기본 maxFPS 75(프레임 간격 0.013333초)와 같게 둔다. 메인 메뉴 장면(M1)은 시간에 따라 바뀌는 그림이 없어 값이 결과를 바꾸지 않는다. (2026-11-07 WB02-H)
export const FRAME_MS = 1000 / 75;

// 가짜 문서의 기준 주소이다. 자산 적재기가 './data/' 를 이 주소 기준으로 풀고, 가짜 fetch 가 경로만 떼어 디스크에서 읽는다. (2026-11-07 WB02-H)
const HEADLESS_BASE_URI = 'http://headless.invalid/';

// cpppj 에만 의미가 있는 설정 키(대문자 비교)이다. 웹은 화면 크기를 scenario.screen 으로 정하므로 이 키들은 웹 설정에 넣지 않는다. (2026-11-07 WB02-H)
export const CPP_ONLY_SETTING_KEYS = Object.freeze(['SCREENW', 'SCREENH']);

// cpppj 단계표의 key 동작 이름 → 브라우저 KeyboardEvent.code 이다. 필요한 키만 둔다. (2026-11-07 WB02-H)
const KEY_CODES = Object.freeze({ f1: 'F1', enter: 'Enter', up: 'ArrowUp', down: 'ArrowDown', left: 'ArrowLeft', right: 'ArrowRight', esc: 'Escape' });

// 디스크의 데이터 폴더를 읽는 fetch 대체 함수를 만든다. URL 의 '/data/' 뒤 경로를 dataRoot 기준으로 읽고, 없으면 404 응답이다. (2026-11-07 WB02-H)
export function diskFetch(dataRoot = DEFAULT_DATA_ROOT) {
    return async (url) => {
        const pathname = decodeURIComponent(new URL(url).pathname);
        const marker = pathname.indexOf('/data/');
        const relative = marker >= 0 ? pathname.slice(marker + '/data/'.length) : pathname.replace(/^\//, '');
        const full = path.join(dataRoot, ...relative.split('/'));
        if (!full.startsWith(dataRoot) || !fs.existsSync(full)) return { ok: false, status: 404, body: null };
        const bytes = fs.readFileSync(full);
        return {
            ok: true,
            status: 200,
            body: null,
            json: async () => JSON.parse(bytes.toString('utf8')),
            arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
        };
    };
}

// 표시 경계를 대체하는 가짜 Canvas 이다. 좌표 사각형은 논리 화면 크기 그대로(1:1)이며, 커서 그림 생성에 필요한 2D 문맥 메서드만 둔다(그림은 버린다). (2026-11-07 WB02-H)
function fakeCanvas(width = 0, height = 0) {
    return {
        width,
        height,
        style: {},
        focus() {},
        setPointerCapture() {},
        getBoundingClientRect: () => ({ left: 0, top: 0, width, height }),
        getContext: () => ({ imageSmoothingEnabled: false, createImageData: (w, h) => ({ data: new Uint8ClampedArray(w * h * 4) }), putImageData() {}, drawImage() {}, setTransform() {} }),
        toDataURL: () => 'data:image/png;base64,',
    };
}

// 가짜 루트 문서이다. 제목 쓰기·기준 주소·Canvas 생성·프레임 예약(아무것도 하지 않음)만 제공한다. (2026-11-07 WB02-H)
function fakeDocument() {
    return {
        title: '',
        hidden: false,
        baseURI: HEADLESS_BASE_URI,
        createElement: () => fakeCanvas(),
        defaultView: { devicePixelRatio: 1, requestAnimationFrame: () => 1, cancelAnimationFrame() {}, performance: { now: () => 0 } },
    };
}

// 무브라우저 앱 하나를 만들어 메인 메뉴까지 연다. 반환: { app, openedLinks, frame(n), click(label), down/up(label), key(name), snapshot(), report() }.
// options: { language('english'), screen({ width, height }), originalComparison(true), settings(설정 키 → 값), dataRoot }.
// settings 의 키는 웹 설정(Settings: welcomeGiven·tellTips 등, 대소문자 무시)·진행 값(Done*)·cpppj 전용 키(CPP_ONLY_SETTING_KEYS) 중 하나여야 하며, 그 밖이면 Error 이다. (2026-11-07 WB02-H)
export async function createHeadless({ language = 'english', screen = { width: 1024, height: 768 }, originalComparison = true, settings = {}, dataRoot = DEFAULT_DATA_ROOT } = {}) {
    if (language !== 'english') throw new Error('무브라우저 실행은 아직 영어만 지원한다(한국어 글리프 래스터라이저는 Canvas 2D 가 필요).');
    // 바깥 링크는 실제 탭을 열지 않고 열려던 주소만 이 목록에 기록한다(반환 값의 openedLinks). (2026-11-07 WB03-AD)
    const openedLinks = [];
    const openExternal = (document, link) => {
        openedLinks.push(link.url);
        return true;
    };
    const app = new Netstorm({ language, audio: false, originalComparison, fetch: diskFetch(dataRoot), autoFocus: false, previewMap: null, debug: false, openExternal });
    const doc = fakeDocument();
    // Netstorm.init 의 DOM 준비 대신 표시 경계 대체물을 넣는다(init 의 설정·진행 저장소 준비와 같은 순서).
    app._root = { ownerDocument: doc };
    app._canvas = fakeCanvas(screen.width, screen.height);
    app._ctx = app._canvas.getContext('2d');
    app._presenter = { present() {}, setPalette() {}, release() {} };
    app._layout = { logicalWidth: screen.width, logicalHeight: screen.height, visible: true, scale: 1, devicePixelRatio: 1, drawScaleX: 1, drawScaleY: 1 };
    app._settings = new Settings({ storage: null, onChange: (key) => app._onSettingChange(key) });
    app._progress = new Progress({ storage: null });
    // 시나리오 설정을 웹 설정·진행 값에 넣는다. cpppj 전용 키는 건너뛰고 모르는 키는 거부한다.
    for (const [key, value] of Object.entries(settings)) {
        if (CPP_ONLY_SETTING_KEYS.includes(key.toUpperCase())) continue;
        const settingKey = findSettingKey(key);
        if (settingKey) app._settings.set(settingKey, Number(value), { persist: false });
        else if (isProgressKey(key)) app._progress.set(key, String(value));
        else throw new Error(`무브라우저 설정에 대응하는 웹 키가 없음: ${key}`);
    }
    await app._load();
    if (app.state !== AppState.CLICK_TO_START) throw app._loadError ?? new Error(`자산 적재 뒤 상태 ${app.state}`);
    app._activate();
    if (app.state !== AppState.MAIN_MENU) throw new Error(`시작 입력 뒤 상태 ${app.state}`);
    const pointerEvent = (point, buttons) => ({ clientX: point.x + 0.5, clientY: point.y + 0.5, button: 0, buttons, pointerId: 1, pointerType: 'mouse' });
    // 라벨 또는 screen:x,y 를 논리 좌표로 바꾼다. 없는 조작이면 cpppj 와 같이 오류이다.
    const resolve = (argument) => {
        const screenMatch = /^screen:(\d+),(\d+)$/.exec(argument);
        if (screenMatch) return { x: Number(screenMatch[1]), y: Number(screenMatch[2]) };
        const point = app.mainMenu?.controlPoint(argument) ?? null;
        if (!point) throw new Error(`UI control not found: ${argument}`);
        return point;
    };
    return {
        app,
        // 바깥 링크로 열려던 주소의 기록이다(실제 탭은 열지 않음). (2026-11-07 WB03-AD)
        openedLinks,
        // n 번째 프레임을 처리한다(브라우저 requestAnimationFrame 콜백과 같은 _onFrame).
        frame(n) {
            app._onFrame(n * FRAME_MS);
        },
        // 라벨 위치에서 왼쪽 버튼을 누르고 뗀다(cpppj click: 같은 프레임에 누름+뗌).
        click(argument) {
            const point = resolve(argument);
            app._onPointerDown(pointerEvent(point, 1));
            app._onPointerUp(pointerEvent(point, 0));
        },
        // 라벨 위치에서 왼쪽 버튼을 누르기만 한다(cpppj down).
        down(argument) {
            app._onPointerDown(pointerEvent(resolve(argument), 1));
        },
        // 라벨 위치에서 왼쪽 버튼을 떼기만 한다(cpppj up).
        up(argument) {
            app._onPointerUp(pointerEvent(resolve(argument), 0));
        },
        // 키를 눌렀다 뗀다. name 은 KEY_CODES 의 이름이다.
        key(name) {
            const code = KEY_CODES[String(name).toLowerCase()];
            if (!code) throw new Error(`Unknown UI key: ${name}`);
            app._onKeyDown({ code, repeat: false, preventDefault() {} });
            app._onKeyUp({ code });
        },
        // 현재 논리 화면을 팔레트 RGB 그림으로 돌려준다(표시기가 쓰는 같은 팔레트).
        snapshot() {
            return indexedToRgb(app._surface, app.resources.palette.rgb);
        },
        // cpppj 보고서와 비교할 수 있는 관측 줄을 돌려준다(같은 키 이름: size·page, 웹 전용: web_popup·web_dialog·web_title·web_cursor).
        // page 는 cpppj 와 같이 Tell 창이 열려 있으면 현재 Tell 이름, 아니면 'main' 이다.
        report() {
            const menu = app.mainMenu;
            const dialog = menu?.dialog ?? null;
            const page = dialog instanceof TellWindow ? app.gameData.currentTell : 'main';
            const popup = menu?.popup?.isOpen ? menu.popup.name : '';
            const kind = dialog instanceof TellWindow ? 'tell' : dialog instanceof HelpWindow ? 'help' : dialog ? 'other' : 'none';
            return `size\t${app._surface.width}\t${app._surface.height}\npage\t${page}\nweb_popup\t${popup}\nweb_dialog\t${kind}\nweb_title\t${doc.title}\nweb_cursor\t${app._wantedCursor()}\n`;
        },
        // 구름 배경만 그린 화면(메인 메뉴 render 의 첫 단계와 같은 타일)을 팔레트 RGB 로 돌려준다. 비교 도구가 구름 차이를 분류할 때 쓴다.
        background() {
            const surface = new IndexedSurface(app._surface.width, app._surface.height);
            if (app.resources.clouds) surface.tile(app.resources.clouds);
            else surface.fill(0);
            return indexedToRgb(surface, app.resources.palette.rgb);
        },
    };
}

// 시나리오 하나를 실행한다. shared 는 시나리오 묶음 파일의 공통 칸({ language, screen, settings })이다.
// 반환: { snapshots: { 이름: RGB 그림 }, reports: { 라벨: 문자열 }, background: 구름 배경만 그린 RGB 그림 }. 단계는 프레임 오름차순이어야 하며 지원하지 않는 동작은 Error 이다. (2026-11-07 WB02-H)
export async function runScenario(scenario, shared = {}, { dataRoot = DEFAULT_DATA_ROOT } = {}) {
    const steps = scenario.steps.map(([frame, operation, argument = '']) => ({ frame, operation, argument }));
    // 프레임 번호는 양수이고 오름차순이어야 한다(cpppj ReadUiSteps 와 같은 조건).
    steps.forEach((step, index) => {
        if (!(step.frame > 0) || (index > 0 && step.frame < steps[index - 1].frame)) throw new Error(`UI frames must be positive and sorted: ${scenario.id}`);
    });
    const headless = await createHeadless({
        language: shared.language ?? 'english',
        screen: shared.screen ?? { width: 1024, height: 768 },
        settings: { ...(shared.settings ?? {}), ...(scenario.settings ?? {}) },
        dataRoot,
    });
    const snapshots = {};
    const reports = {};
    const last = Math.max(scenario.frames ?? 0, ...steps.map((step) => step.frame));
    // 프레임마다 입력 → 처리 → 관측 순서로 진행한다.
    for (let frame = 1; frame <= last; frame += 1) {
        const current = steps.filter((step) => step.frame === frame);
        // 이번 프레임의 입력 단계를 실행한다.
        for (const step of current) {
            if (step.operation === 'click') headless.click(step.argument);
            else if (step.operation === 'down') headless.down(step.argument);
            else if (step.operation === 'up') headless.up(step.argument);
            else if (step.operation === 'esc') headless.key('esc');
            else if (step.operation === 'key') headless.key(step.argument);
            else if (step.operation !== 'snapshot' && step.operation !== 'report') throw new Error(`무브라우저 실행이 지원하지 않는 동작: ${step.operation}`);
        }
        headless.frame(frame);
        // 이번 프레임의 관측 단계를 기록한다.
        for (const step of current) {
            if (step.operation === 'snapshot') snapshots[step.argument] = headless.snapshot();
            if (step.operation === 'report') reports[step.argument] = headless.report();
        }
    }
    return { snapshots, reports, background: headless.background() };
}

// 시나리오 묶음에서 오라클 수집 결과에 영향을 주는 칸(언어·화면·공통 설정, 시나리오별 id·설정·프레임·단계표)만 뽑은 정규 문자열이다.
// 비교 기록(expected·title·background 등)은 빼므로, 기록을 고쳐도 오라클을 다시 모을 필요가 없다. cpp-oracle 메타와 검사가 같은 함수로 해시를 비교한다. (2026-11-07 WB02-H)
export function collectionInputs(data) {
    return JSON.stringify({
        language: data.language ?? 'english',
        screen: data.screen ?? null,
        settings: data.settings ?? {},
        scenarios: data.scenarios.map((scenario) => ({ id: scenario.id, settings: scenario.settings ?? {}, frames: scenario.frames ?? 0, steps: scenario.steps })),
    });
}

// 시나리오 묶음 파일을 읽는다. (2026-11-07 WB02-H)
export function readScenarios(file) {
    const data = JSON.parse(fs.readFileSync(file, 'utf8'));
    if (data.format !== 'netstorm-webpj-oracle-scenarios' || data.version !== 1) throw new Error(`시나리오 형식이 아님: ${file}`);
    return data;
}

// 명령줄 진입점이다. 시나리오를 실행해 BMP 와 보고서를 쓴다. (2026-11-07 WB02-H)
async function main(argv) {
    const option = (name) => { const at = argv.indexOf(name); return at >= 0 ? argv[at + 1] : null; };
    const scenariosFile = option('--scenarios');
    const out = option('--out');
    if (!scenariosFile || !out) { console.error('사용: node webpj/tools/headless.mjs --scenarios <json> [--id <id>] --out <폴더>'); return 1; }
    const data = readScenarios(path.resolve(scenariosFile));
    const only = option('--id');
    // 고른 시나리오마다 실행해 파일로 쓴다.
    for (const scenario of data.scenarios.filter((entry) => !only || entry.id === only)) {
        const result = await runScenario(scenario, data);
        const folder = path.join(path.resolve(out), scenario.id);
        fs.mkdirSync(folder, { recursive: true });
        for (const [name, image] of Object.entries(result.snapshots)) fs.writeFileSync(path.join(folder, `${name}.bmp`), encodeCppBitmap(image));
        fs.writeFileSync(path.join(folder, 'report.txt'), Object.entries(result.reports).map(([label, text]) => `frame\t${label}\n${text}`).join(''), 'utf8');
        console.log(`${scenario.id}: 그림 ${Object.keys(result.snapshots).length}개 → ${folder}`);
    }
    return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = await main(process.argv.slice(2));
