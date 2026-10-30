// NetStorm 웹 클론(webpj)의 앱 수명·장면 전환 모듈이다.
// 브라우저 안에 게임 Canvas 를 만들고, 컨테이너 크기·화면비(4:3, 16:10, 16:9)·기기 픽셀 비율에 맞춰
// Canvas 의 크기와 위치를 계산·적용하며, 포인터·키보드 입력을 게임 논리 좌표로 바꾸고,
// "로딩(자산 적재) → 클릭하여 시작 → 메인 메뉴(원본 복원)" 장면 전환과 프레임 루프를 제공한다.
// 메인 메뉴는 원본 자산(www/data)을 8비트 색 번호로 합성해 표시하고, 돌 버튼 입력·button.wav·메뉴 음악을 원본 규칙대로 처리한다.
// 메인 메뉴 버튼이 여는 다음 화면(캠페인·데모·도움말·옵션 등)과 게임 규칙은 아직 없다(후속 단계). 그 화면들의 본문(Tell 섹션)은 GameData 로 조회할 수 있다.
// 이 모듈은 import 시점에 DOM 을 건드리지 않으므로 Node 에서도 불러와 계산 함수를 검사할 수 있다.
// (2026-10-23 WB01: 실행 골격 최초 작성)
// (2026-10-23 WB03-M1: 원본 자산 적재·메인 메뉴·소리·원본 커서 연결)
// (2026-10-28 WB02-D: 데이터 아카이브(TAFF)·설정·Tell 섹션 조회(GameData) 연결, 메인 메뉴의 Edit 버튼 제거)
import { AssetLoader } from './assets/asset-loader.js';
import { GameData } from './assets/game-data.js';
import {
    BUTTON_SOUND_NAME,
    MENU_ASSET_PATHS,
    MENU_BUNDLE,
    MENU_MUSIC_BUNDLE,
    MENU_MUSIC_NAME,
    MENU_SOUND_NAME,
    MENU_SOUND_PATH,
    buildMenuResources,
} from './assets/menu-assets.js';
import { AudioSystem, createAudioContext } from './audio/audio-system.js';
import { IndexedSurface } from './render/indexed-surface.js';
import { IndexedPresenter, normalizeScaling } from './render/presenter.js';
import { Settings, findSettingKey, getLocalStorage } from './platform/settings.js';
import { Progress, isProgressKey } from './platform/progress.js';
import { MainMenuScreen } from './ui/main-menu.js';
import { PopupMenu } from './ui/popup-menu.js';
import { buildPopupItems } from './ui/option-menus.js';
import { createTellWindow } from './ui/tell-window.js';
import { TELL_TIP_COMMAND, dispatchTellTip } from './ui/tell-tip.js';
import { applyScriptConfig } from './ui/script-command.js';
import { configParseLong } from './assets/config.js';
import { HelpWindow, createHelpWindow } from './ui/help-window.js';
import { applyCursorStyle, buildCursorCss } from './ui/cursor.js';

// ---------------------------------------------------------------------------------------------
// 상수
// ---------------------------------------------------------------------------------------------

// 논리 화면의 높이(픽셀)이다. 원본이 1024x768 이 최대였으므로 모든 화면비에서 높이를 768 로 고정해
// UI 의 세로 배치를 원본과 같게 유지하고, 화면비가 넓을수록 가로 시야만 늘어나게 한다. (2026-10-23 WB01)
const LOGICAL_HEIGHT = 768;

// 지원하는 화면비 목록이다. name 은 설정·URL 에서 쓰는 이름, ratio 는 명목 가로세로비,
// width/height 는 그 화면비에서의 논리 화면 크기(픽셀, 가로는 짝수)다.
// 와이드 화면에서 UI 를 어떻게 배치하고 시야를 얼마나 넓힐지는 아직 정하지 않았으므로
// 지금의 가로 크기는 임시 정책이며 16:9/16:10 지원 완료를 뜻하지 않는다. (2026-10-23 WB01)
export const SCREEN_MODES = Object.freeze([
    Object.freeze({ name: '4:3', ratio: 4 / 3, width: 1024, height: LOGICAL_HEIGHT }),
    Object.freeze({ name: '16:10', ratio: 16 / 10, width: 1228, height: LOGICAL_HEIGHT }),
    Object.freeze({ name: '16:9', ratio: 16 / 9, width: 1366, height: LOGICAL_HEIGHT }),
]);

// 컨테이너 비율에 가장 가까운 화면비를 자동으로 고르라는 설정값이다. (2026-10-23 WB01)
export const AUTO_ASPECT = 'auto';

// Canvas 내부 픽셀 수(가로 x 세로)의 기본 상한이다. Canvas 는 화면에 그려지는 기기 픽셀 크기와 같게 만들지만,
// 5K 이상의 큰 화면에서 메모리와 그리기 비용이 과도해지지 않도록 이 값을 넘으면 같은 비율로 줄인다. (2026-10-23 WB01)
const DEFAULT_MAX_CANVAS_PIXELS = 16_777_216;

// 컨테이너 비율이 선택한 화면 모드의 비율과 이 값(상대 오차) 이내로 같으면, 비율을 유지하는 최대 크기 대신
// 컨테이너를 빈틈없이 채운다. 1366x768 은 정확한 16:9 가 아니라서 1920x1080 에서 1픽셀 틈이 생기는 것을 막는다.
// 0.5% 이내의 가로세로 배율 차이는 눈에 띄지 않는다. (2026-10-23 WB01)
const ASPECT_SNAP_TOLERANCE = 0.005;

// 프레임 간격의 상한(ms)이다. 가비지 수집 등으로 길게 멈춘 프레임이 한 번에 몰려 반영되지 않게 한다. (2026-10-23 WB01)
const MAX_FRAME_DELTA_MS = 250;

// 초당 프레임 수를 갱신하는 측정 구간(ms)이다. (2026-10-23 WB01)
const FPS_WINDOW_MS = 500;

// 원본 메인 메뉴의 창 제목(config.english 의 BlankWindowName)이다. 메인 메뉴에서 문서 제목으로 쓴다. (2026-10-23 WB03-M1)
const MAIN_MENU_WINDOW_TITLE = 'NetStorm Main Menu';

// 데이터 폴더의 기본 위치(문서 기준 상대 URL)이다. 정적 서버는 webpj/www 를 공개하므로 www/data 를 가리킨다. (2026-10-23 WB03-M1)
const DEFAULT_ASSET_BASE = './data/';

// 앱의 상태 목록이다. 로딩 → 클릭 대기 → 메인 메뉴 순으로 진행하고, 로딩에 실패하면 ERROR 에서 클릭으로 재시도한다.
// IDLE 은 init 전, DESTROYED 는 destroy 후다. 메뉴 복귀·미션 상태는 해당 단계에서 추가한다. (2026-10-23 WB01)
export const AppState = Object.freeze({
    IDLE: 'idle',
    LOADING: 'loading',
    ERROR: 'error',
    CLICK_TO_START: 'clickToStart',
    MAIN_MENU: 'mainMenu',
    DESTROYED: 'destroyed',
});

// 웹 전용 화면(로딩·오류·클릭하여 시작)의 언어별 문구이다. 원본 메인 메뉴의 버튼 라벨은 원본 글꼴로 그리므로 여기에 없다.
// 모르는 언어는 영어로 대체한다. (2026-10-23 WB01, 2026-10-23 WB03-M1: 임시 메뉴 문구 제거)
const TEXTS = Object.freeze({
    english: Object.freeze({
        title: 'NetStorm',
        loading: 'Loading...',
        loadFailed: 'Failed to load game data.',
        retry: 'Click to retry',
        clickToStart: 'Click to start',
    }),
    korean: Object.freeze({
        title: 'NetStorm',
        loading: '불러오는 중...',
        loadFailed: '게임 데이터를 불러오지 못했습니다.',
        retry: '클릭하여 다시 시도',
        clickToStart: '클릭하여 시작',
    }),
});

// Canvas 글자에 쓰는 글꼴 목록이다. 원본 영어 글꼴과 D2Coding 웹 글꼴은 WB02 에서 적재하므로
// 지금은 시스템에 설치된 글꼴로 대체하는 임시 설정이다. (2026-10-23 WB01)
const FONT_FAMILY = '"D2Coding", "Malgun Gothic", "Segoe UI", monospace';

// 게임 Canvas 가 포커스를 가졌을 때 브라우저의 기본 동작(스크롤 등)을 막을 키 코드 집합이다.
// 게임 루트가 입력을 소유하는 범위로만 한정하며, Tab 은 키보드 이동을 막지 않도록 제외한다. (2026-10-23 WB01)
const PREVENTED_KEYS = new Set([
    'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', 'PageUp', 'PageDown', 'Home', 'End',
]);

// init 이 루트 요소에 덮어쓰고 destroy 가 되돌리는 인라인 스타일 속성 목록이다. (2026-10-23 WB01)
const ROOT_STYLE_KEYS = Object.freeze([
    'position', 'left', 'top', 'right', 'bottom', 'overflow', 'background', 'userSelect', 'webkitUserSelect',
]);

// ---------------------------------------------------------------------------------------------
// 순수 계산 함수 (DOM 을 쓰지 않으므로 Node 에서 검사할 수 있다)
// ---------------------------------------------------------------------------------------------

// 값을 [min, max] 범위로 제한한다. 상한·하한 계산에서 공통으로 쓴다. (2026-10-23 WB01)
function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
}

// 기기 픽셀 단위 값을 내림한다. 정확히 맞는 크기가 부동소수점 오차(예: 1919.9999999999998)로 1픽셀 모자라지 않도록
// 아주 작은 보정값을 더한 뒤 내린다. (2026-10-23 WB01)
function floorPixel(value) {
    return Math.floor(value + 1e-6);
}

// 화면비 이름으로 화면 모드를 찾는다. 없으면 undefined 를 돌려준다. (2026-10-23 WB01)
export function findScreenMode(name) {
    return SCREEN_MODES.find((mode) => mode.name === name);
}

// 컨테이너 크기와 화면비 설정으로 사용할 화면 모드를 고른다.
// aspect 가 모드 이름이면 그 모드를, AUTO_ASPECT 이거나 모르는 이름이면 컨테이너 비율과
// 로그 비율 차이가 가장 작은 모드를 고른다. 컨테이너 크기를 쓸 수 없으면 첫 모드(4:3)다. (2026-10-23 WB01)
export function pickScreenMode(containerWidth, containerHeight, aspect = AUTO_ASPECT) {
    const forced = findScreenMode(aspect);
    if (forced) return forced;
    if (!(containerWidth > 0) || !(containerHeight > 0)) return SCREEN_MODES[0];
    const containerRatio = containerWidth / containerHeight;
    let best = SCREEN_MODES[0];
    let bestDistance = Infinity;
    // 모든 화면 모드를 돌며 컨테이너 비율과 가장 가까운 것을 찾는다.
    for (const mode of SCREEN_MODES) {
        const distance = Math.abs(Math.log(containerRatio / mode.ratio));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = mode;
        }
    }
    return best;
}

// 컨테이너 크기·기기 픽셀 비율·화면비 설정에서 Canvas 배치를 계산한다. 입력이 같으면 결과도 같다.
// 논리 좌표(logical*)·CSS 표시 크기(css*, left/top)·내부 픽셀 크기(canvas*)를 분리해 돌려준다.
//  - 논리 좌표: 게임 코드가 그리는 좌표계. 화면비 모드가 정한다.
//  - CSS 표시 크기: 컨테이너 안에 비율을 유지하며 최대로 키운 크기. 기기 픽셀 경계에 맞춰 내림하고 가운데 정렬한다.
//    비율 차이가 ASPECT_SNAP_TOLERANCE 이내이면 컨테이너를 빈틈없이 채운다.
//  - 내부 픽셀 크기: 화면에 그려지는 기기 픽셀 크기와 같게 잡아 브라우저가 다시 늘리거나 줄이지 않게 한다(글자가 선명하다).
//    그릴 때는 drawScaleX/Y(논리 픽셀 1개당 내부 픽셀 수)를 변환 행렬로 적용하면 논리 좌표로 그릴 수 있다.
// 컨테이너가 0 이하이면 visible=false 이며 Canvas 를 숨겨야 한다. (2026-10-23 WB01)
export function computeLayout(
    containerWidth,
    containerHeight,
    devicePixelRatio = 1,
    aspect = AUTO_ASPECT,
    maxCanvasPixels = DEFAULT_MAX_CANVAS_PIXELS,
) {
    const dpr = Number.isFinite(devicePixelRatio) && devicePixelRatio > 0 ? devicePixelRatio : 1;
    const mode = pickScreenMode(containerWidth, containerHeight, aspect);
    const hidden = {
        visible: false,
        mode: mode.name,
        logicalWidth: mode.width,
        logicalHeight: mode.height,
        scale: 0,
        cssWidth: 0,
        cssHeight: 0,
        left: 0,
        top: 0,
        canvasWidth: mode.width,
        canvasHeight: mode.height,
        drawScaleX: 1,
        drawScaleY: 1,
        devicePixelRatio: dpr,
    };
    if (!(containerWidth > 0) || !(containerHeight > 0)) return Object.freeze(hidden);

    // 가로·세로 각각 컨테이너를 채우는 데 필요한 배율(논리 픽셀 1개의 CSS 픽셀 수)이다.
    const fillX = containerWidth / mode.width;
    const fillY = containerHeight / mode.height;
    // 비율을 유지한 채 컨테이너 안에 논리 화면 전체가 들어가는 최대 배율이다.
    const scale = Math.min(fillX, fillY);
    let cssWidth;
    let cssHeight;
    if (Math.abs(fillX / fillY - 1) <= ASPECT_SNAP_TOLERANCE) {
        // 비율이 사실상 같으면 컨테이너를 빈틈없이 채운다.
        cssWidth = floorPixel(containerWidth * dpr) / dpr;
        cssHeight = floorPixel(containerHeight * dpr) / dpr;
    } else {
        // 기기 픽셀 경계로 내림해 가장자리가 흐려지거나 컨테이너를 넘지 않게 한다.
        cssWidth = floorPixel(mode.width * scale * dpr) / dpr;
        cssHeight = floorPixel(mode.height * scale * dpr) / dpr;
    }
    if (!(cssWidth > 0) || !(cssHeight > 0)) return Object.freeze(hidden);

    let canvasWidth = Math.round(cssWidth * dpr);
    let canvasHeight = Math.round(cssHeight * dpr);
    const pixels = canvasWidth * canvasHeight;
    if (pixels > maxCanvasPixels) {
        // 너무 큰 화면은 같은 비율로 줄인다. 브라우저가 약간 늘려 보여 준다.
        const shrink = Math.sqrt(maxCanvasPixels / pixels);
        canvasWidth = Math.max(1, Math.floor(canvasWidth * shrink));
        canvasHeight = Math.max(1, Math.floor(canvasHeight * shrink));
    }
    return Object.freeze({
        visible: true,
        mode: mode.name,
        logicalWidth: mode.width,
        logicalHeight: mode.height,
        scale,
        cssWidth,
        cssHeight,
        left: floorPixel(((containerWidth - cssWidth) / 2) * dpr) / dpr,
        top: floorPixel(((containerHeight - cssHeight) / 2) * dpr) / dpr,
        canvasWidth,
        canvasHeight,
        drawScaleX: canvasWidth / mode.width,
        drawScaleY: canvasHeight / mode.height,
        devicePixelRatio: dpr,
    });
}

// 브라우저 화면 좌표(clientX/Y)를 Canvas 의 논리 좌표로 바꾼다. rect 는 Canvas 의 getBoundingClientRect() 결과다.
// 돌려주는 x, y 는 논리 픽셀의 정수(내림)이고, inside 는 논리 화면 안에 있는지 여부다.
// 포인터 캡처 중에는 Canvas 밖 좌표도 들어오므로 x, y 는 범위를 제한하지 않는다. rect 가 비면 null 이다. (2026-10-23 WB01)
export function clientToLogical(clientX, clientY, rect, logicalWidth, logicalHeight) {
    if (!rect || !(rect.width > 0) || !(rect.height > 0)) return null;
    const x = ((clientX - rect.left) * logicalWidth) / rect.width;
    const y = ((clientY - rect.top) * logicalHeight) / rect.height;
    return {
        x: Math.floor(x),
        y: Math.floor(y),
        inside: x >= 0 && x < logicalWidth && y >= 0 && y < logicalHeight,
    };
}

// 문자열 언어 이름을 지원하는 언어 키로 바꾼다. 한국어면 'korean', 그 밖에는 'english' 다.
// 'ko', 'ko-KR', 'korean' 을 한국어로 본다. (2026-10-23 WB01)
export function normalizeLanguage(value) {
    const text = String(value ?? '').toLowerCase();
    return text === 'korean' || text.startsWith('ko') ? 'korean' : 'english';
}

// ---------------------------------------------------------------------------------------------
// 프레임 시계
// ---------------------------------------------------------------------------------------------

// requestAnimationFrame 타임스탬프에서 프레임 간격·누적 시간·초당 프레임 수를 계산하는 시계다.
// 화면 표시 기회(rAF)와 원본 게임 시각(WB04 이후의 Process/Regular)은 별개이므로 이 시계는 표시용이다.
// 탭이 숨겨졌다 돌아오면 reset() 으로 기준점을 다시 잡아 긴 부재 시간을 한 번에 따라잡지 않는다. (2026-10-23 WB01)
class FrameClock {
    // 시계를 만든다. maxDeltaMs 는 한 프레임 간격의 상한(ms)이다.
    constructor(maxDeltaMs = MAX_FRAME_DELTA_MS) {
        // 한 프레임 간격의 상한(ms)이다.
        this.maxDeltaMs = maxDeltaMs;
        // 직전 프레임의 타임스탬프(ms)다. null 이면 다음 tick 이 기준점만 잡고 간격은 0 으로 돌려준다.
        this.lastTime = null;
        // 지금까지 진행한 프레임 수다.
        this.frame = 0;
        // 정지 구간을 뺀 누적 경과 시간(ms)이다.
        this.elapsedMs = 0;
        // 최근 측정한 초당 프레임 수다.
        this.fps = 0;
        // fps 측정 구간의 시작 시각(ms)이다. null 이면 다음 tick 에서 새로 시작한다.
        this.fpsStart = null;
        // 현재 측정 구간 안에서 센 프레임 수다.
        this.fpsFrames = 0;
    }

    // 기준점을 비운다. 탭 복귀·재개 직후 첫 간격이 크게 잡히지 않도록 호출한다.
    reset() {
        this.lastTime = null;
        this.fpsStart = null;
        this.fpsFrames = 0;
    }

    // 한 프레임을 진행하고 직전 프레임과의 간격(ms, 상한 적용)을 돌려준다. now 는 rAF 타임스탬프다.
    tick(now) {
        const delta = this.lastTime === null ? 0 : clamp(now - this.lastTime, 0, this.maxDeltaMs);
        this.lastTime = now;
        this.frame += 1;
        this.elapsedMs += delta;
        if (this.fpsStart === null) {
            this.fpsStart = now;
            this.fpsFrames = 0;
        }
        this.fpsFrames += 1;
        const span = now - this.fpsStart;
        if (span >= FPS_WINDOW_MS) {
            this.fps = (this.fpsFrames * 1000) / span;
            this.fpsStart = now;
            this.fpsFrames = 0;
        }
        return delta;
    }
}

// ---------------------------------------------------------------------------------------------
// Netstorm 앱
// ---------------------------------------------------------------------------------------------

// 주소창의 쿼리 문자열을 읽는다. 개발용 설정(?debug, ?aspect=16:9, ?lang=korean)에 쓴다.
// 브라우저가 아니거나 location 이 없으면 빈 값이다. (2026-10-23 WB01)
function readQueryParams() {
    return new URLSearchParams(globalThis.location?.search ?? '');
}

// 게임 앱 클래스이다. 호스트 페이지는 `new Netstorm(옵션).init(루트요소)` 로 시작하고 `destroy()` 로 정리한다.
// 옵션:
//  - language: 'english' | 'korean'. 기본은 ?lang, 없으면 브라우저 언어.
//  - aspect: 'auto' | '4:3' | '16:10' | '16:9'. 기본은 ?aspect, 없으면 auto.
//  - debug: true 면 화면 구성·입력 정보를 겹쳐 그린다. 기본은 ?debug 유무.
//  - fillWindow: true(기본)면 루트 요소를 브라우저 창 전체로 채운다. false 면 호스트가 정한 루트 크기를 따른다.
//  - autoFocus: true(기본)면 시작할 때 Canvas 에 포커스를 줘 Enter/Space 로 바로 시작할 수 있게 한다.
//  - maxCanvasPixels: Canvas 내부 픽셀 수(가로 x 세로)의 상한(기본 16,777,216). 넘으면 같은 비율로 줄인다.
//  - assetBase: 데이터 폴더 URL(문서 기준, 기본 './data/'). manifest.json 과 원본 자산 복사본이 있는 곳이다.
//  - fetch: 자산을 받을 fetch 함수(기본은 루트 문서 window 의 fetch). 검사에서 가짜 함수를 넘길 수 있다.
//  - scaling: 'sharp' | 'nearest' | 'smooth'. 8비트 화면의 확대 방식(기본 ?scaling, 없으면 sharp).
//  - audio: false 면 소리를 쓰지 않는다(기본 true, ?mute 가 있으면 false).
//  - soundVolume / musicVolume: 원본 음량 단계 1~5(기본 3 / 2).
//  - prepare: async (app) => void. 내장 자산 적재 뒤 실행하는 추가 준비 훅. 예외를 던지면 ERROR 상태가 된다.
//  - onStateChange: (새 상태, 이전 상태) => void. 상태가 바뀔 때 호출된다.
//  - onMenuCommand: (명령, 라벨) => void. 메인 메뉴 버튼이 실행됐을 때 호출된다(다음 화면은 아직 없음).
// (2026-10-23 WB01, 2026-10-23 WB03-M1: 자산·소리·메뉴 옵션 추가)
export class Netstorm {
    // 앱을 만든다. DOM 은 init 에서 만든다.
    constructor(options = {}) {
        const params = readQueryParams();
        // 화면비 설정이다. 모르는 값이면 자동으로 대체한다.
        const requestedAspect = options.aspect ?? params.get('aspect') ?? AUTO_ASPECT;
        // 확정된 옵션 모음이다.
        this.options = {
            language: normalizeLanguage(options.language ?? params.get('lang') ?? globalThis.navigator?.language),
            debug: options.debug ?? params.has('debug'),
            fillWindow: options.fillWindow ?? true,
            autoFocus: options.autoFocus ?? true,
            maxCanvasPixels: Math.max(1, Math.floor(options.maxCanvasPixels ?? DEFAULT_MAX_CANVAS_PIXELS) || DEFAULT_MAX_CANVAS_PIXELS),
            assetBase: options.assetBase ?? DEFAULT_ASSET_BASE,
            fetch: options.fetch ?? null,
            scaling: normalizeScaling(options.scaling ?? params.get('scaling')),
            audio: options.audio ?? !params.has('mute'),
            // 호스트가 정한 음량 단계(1~5)이다. 없으면(null) 저장된 설정을 쓴다. 정하면 저장하지 않고 이번 실행에서만 우선한다. (2026-10-29 WB03-F)
            soundVolume: options.soundVolume ?? null,
            musicVolume: options.musicVolume ?? null,
            prepare: options.prepare ?? (async () => {}),
            onStateChange: options.onStateChange ?? null,
            onMenuCommand: options.onMenuCommand ?? null,
        };
        // 현재 화면비 설정이다. setAspect 로 바꿀 수 있다.
        this._aspect = findScreenMode(requestedAspect) ? requestedAspect : AUTO_ASPECT;
        // 현재 앱 상태다.
        this._state = AppState.IDLE;
        // 게임을 넣은 루트 요소다. destroy 후에는 null 이다.
        this._root = null;
        // 게임 Canvas 요소다.
        this._canvas = null;
        // Canvas 2D 그리기 컨텍스트다.
        this._ctx = null;
        // 가장 최근에 적용한 Canvas 배치다.
        this._layout = null;
        // 이벤트 리스너를 한꺼번에 해제하기 위한 컨트롤러다.
        this._abort = null;
        // 루트 크기 변화 관찰자다.
        this._resizeObserver = null;
        // 예약된 requestAnimationFrame 번호다. 0 이면 예약 없음이다.
        this._frameHandle = 0;
        // 표시용 프레임 시계다.
        this._clock = new FrameClock();
        // init 때 저장해 둔 루트 인라인 스타일이다. destroy 가 되돌린다.
        this._savedRootStyle = null;
        // 비동기 로딩 결과가 오래된 것인지 가리는 세대 번호다. destroy/재시도 때마다 올라간다.
        this._loadGeneration = 0;
        // 마지막 로딩 실패 원인이다. 없으면 null 이다.
        this._loadError = null;
        // 오디오 컨텍스트다. '클릭하여 시작' 입력 안에서 만든다.
        this._audioContext = null;
        // 오디오 준비 상태 문자열이다. none | unsupported | suspended | running | closed | failed | disabled.
        this._audioStatus = 'none';
        // 입력 상태다. pointer 는 논리 좌표, keys 는 눌린 키의 code 집합이다.
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
        // 메인 메뉴 그리기 자원 묶음(buildMenuResources 결과)이다. 자산 적재 전에는 null 이며, 재시작해도 다시 받지 않는다.
        this._resources = null;
        // 게임 데이터 조회 객체(GameData: 파일 조회·설정 층·언어 용어표·Tell 섹션)다. 자산 적재 전에는 null 이며 재시작해도 다시 만들지 않는다.
        this._gameData = null;
        // 버튼 효과음 파일 바이트다. 오디오 문맥이 생기면 해독한다.
        this._buttonSoundBytes = null;
        // 대화상자 목록 행 효과음(openSubGump.wav) 바이트이다. 자산 적재 전에는 null 이다. (2026-10-28 WB03-D)
        this._menuSoundBytes = null;
        // 가장 최근 프레임의 시각(초)이다. 대화상자의 Timeout 기한 계산에 쓴다. (2026-10-28 WB03-D)
        this._frameSeconds = 0;
        // 사용자 설정(옵션 메뉴 값)이다. init 에서 만들고 destroy 에서 비운다. 값 변경은 _onSettingChange 로 소리에 반영된다. (2026-10-29 WB03-F)
        this._settings = null;
        // 캠페인 진행 값(`Done*`)이다. init 에서 만들고 destroy 에서 비운다. 게임 데이터에 쓰인 진행 값은 _onConfigWrite 로 여기에 모여 로컬 저장소에 저장된다. (2026-10-30 WB03-H)
        this._progress = null;
        // 메뉴 음악 파일 바이트다. 뒤에서 받으며 다 받기 전에는 null 이다.
        this._musicBytes = null;
        // 메뉴 음악 상태 문자열이다. none | loading | loaded | decoding | ready | playing | failed.
        this._musicStatus = 'none';
        // 자산 적재 진행률이다. loaded/total 은 바이트 수다.
        this._loadProgress = { loaded: 0, total: 0 };
        // Web Audio 재생기(AudioSystem)다. '클릭하여 시작' 입력 안에서 만든다.
        this._audio = null;
        // 8비트 색 번호 화면 버퍼(논리 크기)다.
        this._surface = null;
        // 8비트 화면 표시기다.
        this._presenter = null;
        // 메인 메뉴 화면이다. 메인 메뉴에 처음 들어갈 때 만든다.
        this._mainMenu = null;
        // 시작 팁 창을 아직 보여 주지 않은 첫 메인 메뉴인지 여부이다(원본 전역 00542530, 초기 1). 환영 창이 아닌 시작 흐름이 끝나면 꺼진다. init 에서 다시 켠다. (2026-10-30 WB03-J)
        this._firstMenuPending = true;
        // 다음 프레임에 Canvas 를 반드시 다시 그려야 하는지 여부다(크기 변경 등).
        this._displayDirty = true;
        // init 때 저장한 문서 제목이다. destroy 가 되돌린다.
        this._savedTitle = null;
        // 마지막으로 적용한 커서 CSS 의 열쇠(배율·픽셀 비율)다. 같으면 다시 만들지 않는다.
        this._cursorKey = '';
    }

    // ----- 읽기 전용 속성 -----

    // 현재 앱 상태이다 (AppState 값).
    get state() {
        return this._state;
    }

    // 현재 화면비 설정이다 ('auto' 또는 모드 이름).
    get aspect() {
        return this._aspect;
    }

    // 가장 최근에 적용한 Canvas 배치이다. init 전에는 null 이다.
    get layout() {
        return this._layout;
    }

    // 게임 Canvas 요소이다. init 전/destroy 후에는 null 이다.
    get canvas() {
        return this._canvas;
    }

    // 입력 상태이다. pointer(논리 좌표 x, y / inside / buttons / type)와 keys(눌린 키 code 집합)를 가진다.
    get input() {
        return this._input;
    }

    // 오디오 준비 상태 문자열이다.
    get audioStatus() {
        return this._audioStatus;
    }

    // 표시용 프레임 시계이다.
    get clock() {
        return this._clock;
    }

    // 현재 언어의 화면 문구 모음이다.
    get texts() {
        return TEXTS[this.options.language] ?? TEXTS.english;
    }

    // 메인 메뉴 화면 객체이다. 메인 메뉴에 들어가기 전에는 null 이다. (2026-10-23 WB03-M1)
    get mainMenu() {
        return this._mainMenu;
    }

    // 메인 메뉴 그리기 자원 묶음이다. 자산 적재 전에는 null 이다. (2026-10-23 WB03-M1)
    get resources() {
        return this._resources;
    }

    // 게임 데이터 조회 객체이다. 자산 적재 전에는 null 이다. 콘솔에서 window.netstorm.gameData.tellSection('Credits') 처럼 확인할 수 있다. (2026-10-28 WB02-D)
    get gameData() {
        return this._gameData;
    }

    // 캠페인 진행 값 모음이다. init 전과 destroy 뒤에는 null 이다. 콘솔에서 window.netstorm.progress.snapshot() 으로 저장된 진행 값을 볼 수 있다.
    // 값을 쓰려면 window.netstorm.gameData.set('DoneTheWarBegins', 1) 처럼 게임 데이터에 쓴다(미션 스크립트의 Config 효과와 같은 경로, 저장도 함께 된다). (2026-10-30 WB03-H)
    get progress() {
        return this._progress;
    }

    // 메뉴 음악 상태 문자열이다. (2026-10-23 WB03-M1)
    get musicStatus() {
        return this._musicStatus;
    }

    // ----- 수명 -----

    // 루트 요소 안에 게임을 만들고 시작한다. 이미 시작된 앱이면 먼저 destroy 한다.
    // 자산 준비는 비동기로 진행되며, 끝나면 '클릭하여 시작' 상태가 된다. this 를 돌려준다.
    init(root) {
        if (!root || root.nodeType !== 1) throw new TypeError('Netstorm.init: 게임을 넣을 DOM 요소가 필요합니다.');
        if (this._root) this.destroy();

        const doc = root.ownerDocument;
        // Canvas 를 먼저 만들어 보고, 2D 컨텍스트를 못 얻으면 루트를 건드리기 전에 실패한다.
        const canvas = doc.createElement('canvas');
        const ctx = canvas.getContext('2d', { alpha: false });
        if (!ctx) throw new Error('Netstorm.init: Canvas 2D 컨텍스트를 만들 수 없습니다.');

        this._root = root;
        this._canvas = canvas;
        this._ctx = ctx;
        this._abort = new AbortController();
        this._clock = new FrameClock();
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
        this._audioStatus = 'none';
        // 저장된 설정을 읽는다(저장소를 쓸 수 없으면 기본값으로 계속). 호스트가 음량을 정했으면 저장하지 않고 우선 적용한다. (2026-10-29 WB03-F)
        const storage = getLocalStorage(doc.defaultView);
        this._settings = new Settings({ storage, onChange: (key) => this._onSettingChange(key) });
        if (this._settings.loadProblem) console.warn(`Netstorm: ${this._settings.loadProblem}. 기본 설정으로 시작합니다.`);
        if (this.options.soundVolume !== null) this._settings.set('soundVolume', this.options.soundVolume, { persist: false });
        if (this.options.musicVolume !== null) this._settings.set('musicVolume', this.options.musicVolume, { persist: false });
        // 저장된 캠페인 진행 값을 읽는다(저장소를 쓸 수 없으면 진행 값 없이 계속). 이전 수명에서 게임 데이터를 이미 만들었다면(재시작) 새로 읽은 값을 그 설정 층에 옮겨 적는다. (2026-10-30 WB03-H)
        this._progress = new Progress({ storage });
        if (this._progress.loadProblem) console.warn(`Netstorm: ${this._progress.loadProblem}. 진행 값 없이 시작합니다.`);
        this._syncGameData();
        // 새 수명은 처음 켠 게임처럼 시작 창 흐름과 Tell 이름 기록을 다시 시작한다. (2026-10-30 WB03-J)
        this._firstMenuPending = true;
        if (this._gameData) this._gameData.currentTell = '';
        this._presenter = new IndexedPresenter(doc, this.options.scaling);
        // 이전 수명에서 이미 받은 자원이 있으면(재시작) 팔레트를 바로 설치한다.
        if (this._resources) this._presenter.setPalette(this._resources.palette.rgb);
        this._displayDirty = true;
        this._cursorKey = '';
        this._savedTitle = doc.title;
        try {
            this._applyRootStyle();
            this._applyCanvasStyle();
            root.appendChild(canvas);
            this._bindEvents();
            this._applyLayout();
            if (this.options.autoFocus) canvas.focus({ preventScroll: true });
            this._startFrameLoop();
            this._load();
        } catch (error) {
            // 초기화 도중 실패하면 만든 것을 모두 정리하고 원인을 그대로 전달한다.
            this.destroy();
            throw error;
        }
        return this;
    }

    // 게임을 정리한다. 프레임 예약·이벤트·관찰자·오디오·진행 중 로딩을 모두 해제하고 Canvas 를 제거한다.
    // 여러 번 호출해도 안전하며, 이후 init 으로 다시 시작할 수 있다. 처음 골격의 destroy(div) 형태로 불러도 인자는 무시한다.
    // init 도중 실패한 반쪽 상태도 이 함수가 정리하므로 상태 이름이 아니라 루트 요소의 유무로 판단한다.
    destroy() {
        if (!this._root) return;
        // 진행 중인 로딩 결과가 정리 뒤에 상태를 바꾸지 못하게 세대를 올린다.
        this._loadGeneration += 1;
        this._stopFrameLoop();
        this._abort?.abort();
        this._abort = null;
        this._resizeObserver?.disconnect();
        this._resizeObserver = null;
        this._mainMenu?.cancel();
        this._mainMenu = null;
        this._settings = null;
        this._progress = null;
        if (this._audio) {
            // 음악을 멈추고 문맥을 닫는다. 닫기에 실패해도 정리를 막지 않는다.
            this._audio.close();
            this._audio = null;
            this._audioContext = null;
        } else if (this._audioContext) {
            // 닫기에 실패해도 정리를 막지 않는다.
            Promise.resolve(this._audioContext.close?.()).catch(() => {});
            this._audioContext = null;
        }
        // 받은 음악 바이트는 재시작을 위해 남기고, 오디오 문맥에 묶인 해독·재생 상태만 되돌린다.
        if (this._musicStatus === 'decoding' || this._musicStatus === 'ready' || this._musicStatus === 'playing') this._musicStatus = 'loaded';
        this._presenter?.release();
        this._presenter = null;
        this._surface = null;
        if (this._savedTitle !== null && this._root.ownerDocument) this._root.ownerDocument.title = this._savedTitle;
        this._savedTitle = null;
        this._canvas?.remove();
        this._restoreRootStyle();
        this._root = null;
        this._canvas = null;
        this._ctx = null;
        this._layout = null;
        this._input.keys.clear();
        this._setState(AppState.DESTROYED);
    }

    // 화면비 설정을 바꾼다. 'auto' 이거나 SCREEN_MODES 의 이름이어야 하며 그 밖의 값은 RangeError 다.
    // 시작 후에 부르면 즉시 배치를 다시 계산한다.
    setAspect(name) {
        if (name !== AUTO_ASPECT && !findScreenMode(name)) {
            throw new RangeError(`Netstorm.setAspect: 지원하지 않는 화면비입니다: ${name}`);
        }
        this._aspect = name;
        if (this._canvas) this._applyLayout();
    }

    // ----- 상태 -----

    // 앱 상태를 바꾸고 알림 훅을 부른다. 같은 상태로의 전환은 무시한다.
    // 호스트가 넘긴 훅이 예외를 던져도 상태 전환과 로딩 흐름이 깨지지 않도록 예외는 콘솔에만 남긴다.
    _setState(next) {
        if (next === this._state) return;
        const previous = this._state;
        this._state = next;
        try {
            this.options.onStateChange?.(next, previous);
        } catch (error) {
            console.error('Netstorm: onStateChange 훅에서 예외가 발생했습니다.', error);
        }
    }

    // 자산을 적재하고 추가 준비 훅을 실행한다. 성공하면 클릭 대기, 실패하면 오류 화면(클릭으로 재시도)이 된다.
    // 메인 메뉴 필수 묶음(menu)은 기다리고, 음악 묶음(menu-music)은 뒤에서 받는다. 이미 받은 자원은 재시도·재시작에서 다시 받지 않는다.
    // 오래된 세대의 결과는 버린다. 준비 단계에서는 오디오를 재생하지 않는다.
    async _load() {
        this._loadGeneration += 1;
        const generation = this._loadGeneration;
        this._loadError = null;
        this._setState(AppState.LOADING);
        try {
            if (!this._resources) {
                const loader = this._createAssetLoader();
                await loader.loadManifest();
                if (generation !== this._loadGeneration) return;
                const files = await loader.loadBundle(MENU_BUNDLE, (loaded, total) => {
                    this._loadProgress = { loaded, total };
                });
                if (generation !== this._loadGeneration) return;
                this._resources = buildMenuResources(files);
                // 설정 층에는 웹 사용자 설정(언어·음량)과 저장된 캠페인 진행 값을 원본 options.cfg 자리에 넣는다. 해상도·전체화면 값은 넣지 않는다.
                // 이후 게임 데이터에 쓰이는 값은 _onConfigWrite 가 설정·진행 저장소로 옮긴다. (2026-10-30 WB03-H: 진행 값과 쓰기 통지 연결)
                this._gameData = GameData.build({
                    files,
                    language: this.options.language,
                    options: { ...this._settings.snapshot(), ...this._progress.snapshot() },
                    onChange: (key, value) => this._onConfigWrite(key, value),
                });
                this._buttonSoundBytes = files.get(MENU_ASSET_PATHS.buttonSound) ?? null;
                this._menuSoundBytes = files.get(MENU_SOUND_PATH) ?? null;
                this._presenter?.setPalette(this._resources.palette.rgb);
                this._applyCursor();
                this._startMusicDownload(loader);
            }
            await this.options.prepare(this);
        } catch (error) {
            if (generation !== this._loadGeneration) return;
            this._loadError = error;
            this._setState(AppState.ERROR);
            return;
        }
        if (generation !== this._loadGeneration) return;
        this._setState(AppState.CLICK_TO_START);
    }

    // 데이터 폴더 URL 과 fetch 함수로 자산 적재기를 만든다. 데이터 폴더는 루트 문서의 주소를 기준으로 푼다. (2026-10-23 WB03-M1)
    _createAssetLoader() {
        const doc = this._root.ownerDocument;
        const view = this._view();
        const fetchFunction = this.options.fetch ?? (typeof view.fetch === 'function' ? view.fetch.bind(view) : null);
        if (!fetchFunction) throw new Error('이 브라우저에서는 fetch 를 쓸 수 없습니다.');
        return new AssetLoader({ baseUrl: new URL(this.options.assetBase, doc.baseURI), fetchFunction });
    }

    // 메뉴 음악 묶음을 뒤에서 받는다. 다 받으면 오디오가 준비된 경우 해독해 메인 메뉴에서 재생한다.
    // 실패해도 게임은 계속되며(원본도 음악 파일이 없으면 소리 없이 진행) 콘솔과 상태 문자열로만 알린다. (2026-10-23 WB03-M1)
    _startMusicDownload(loader) {
        if (this._musicBytes || this._musicStatus === 'loading' || !this.options.audio) return;
        this._musicStatus = 'loading';
        loader
            .loadBundle(MENU_MUSIC_BUNDLE)
            .then((files) => {
                this._musicBytes = files.get(MENU_ASSET_PATHS.music) ?? null;
                if (!this._musicBytes) throw new Error(`메뉴 음악 파일이 묶음에 없습니다: ${MENU_ASSET_PATHS.music}`);
                this._musicStatus = 'loaded';
                this._prepareMusic();
            })
            .catch((error) => {
                this._musicStatus = 'failed';
                console.warn('Netstorm: 메뉴 음악을 받지 못했습니다. 음악 없이 계속합니다.', error);
            });
    }

    // 사용자의 시작 입력(클릭·Enter·Space)을 처리한다.
    // ERROR 에서는 로딩을 다시 시도하고, CLICK_TO_START 에서는 오디오를 준비한 뒤 메인 메뉴로 넘어간다.
    // 오디오 활성화는 사용자 입력 처리 안에서 동기로 요청해야 브라우저 정책을 통과하므로 상태 전환보다 먼저 한다.
    _activate() {
        if (this._state === AppState.ERROR) {
            this._load();
        } else if (this._state === AppState.CLICK_TO_START) {
            this._unlockAudio();
            this._enterMainMenu();
        }
    }

    // 메인 메뉴로 들어간다. 메뉴 화면을 만들고(처음 한 번) 현재 논리 크기로 배치하며, 창 제목과 메뉴 음악을 원본처럼 맞춘다.
    // (2026-10-23 WB03-M1)
    _enterMainMenu() {
        if (!this._resources) return;
        if (!this._mainMenu) {
            this._mainMenu = new MainMenuScreen({
                resources: this._resources,
                onCommand: (command, label, point) => this._onMenuCommand(command, label, point),
                onButtonSound: () => this._audio?.playSound(BUTTON_SOUND_NAME),
            });
        }
        this._ensureSurface();
        this._setState(AppState.MAIN_MENU);
        this._root.ownerDocument.title = MAIN_MENU_WINDOW_TITLE;
        this._displayDirty = true;
        this._playMenuMusic();
        this._runMainMenuStartup();
    }

    // 메인 메뉴에 들어갈 때의 시작 창 흐름이다(원본 State3 부모 004b6fa9 이후의 Tell 순서).
    //  - WelcomeGiven 이 0 이면 환영 창(`Welcome`)만 연다(`Welcome2` 를 지나면 welcomeGiven=1 이 저장된다).
    //  - 아니면 `Blank` 를 보낸 뒤, 첫 메뉴이고 tellTips 가 켜져 있으면 팁 창(`Tips`, "Did You Know?")을 연다. 첫 메뉴 표시는 이 갈래의 끝에서 꺼진다.
    // 원본은 팁 창 앞에 "Not Validated" 창(`UserNotUpdated`, 자동 업데이트 검증 실패 안내)도 겹쳐 열지만, 이 창은 구현하지 않기로 한 프로젝트 결정(2026-09-30)과
    // 웹에는 업데이터가 없다는 점에 따라 열지 않는다. 원본의 저장 상태 확인 질문(startup)도 웹에는 저장 상태가 없어 없다. (2026-10-30 WB03-J)
    _runMainMenuStartup() {
        const data = this._gameData;
        if (!data || !this._mainMenu) return;
        if (configParseLong(data.get('WelcomeGiven') ?? '0') === 0) {
            this._openTell('Welcome');
            return;
        }
        this._openTell('Blank');
        if (this._firstMenuPending && configParseLong(data.get('tellTips') ?? '0') !== 0) this._openTell('Tips');
        this._firstMenuPending = false;
    }

    // 메인 메뉴 버튼이 실행됐을 때 호출된다. 원본은 여기서 Tell 섹션(캠페인·데모·크레딧)이나 펼침 메뉴(도움말·옵션)를 연다.
    // 그 화면들은 아직 복원하지 않았으므로 호스트 훅과 콘솔에만 알린다. Tell 명령이면 원본 데이터에서 섹션을 실제로 찾아 줄 수를 함께 알린다.
    // (2026-10-23 WB03-M1, 2026-10-28 WB02-D: Tell 섹션 조회 확인 추가)
    // point 는 menu 명령이면 펼침 메뉴의 왼쪽 위 기준점(버튼 가운데 x, 버튼 위쪽 + 칸 정의의 popupDy)이다. (2026-10-29 WB03-F: 펼침 메뉴 연결)
    _onMenuCommand(command, label, point = null) {
        try {
            this.options.onMenuCommand?.(command, label);
        } catch (error) {
            console.error('Netstorm: onMenuCommand 훅에서 예외가 발생했습니다.', error);
        }
        // Tell 명령(Campaign·Demo·Credits)은 대화상자를, menu 명령(Help·Options)은 펼침 메뉴를 연다.
        if (command.type === 'tell') {
            this._openTell(command.target);
            return;
        }
        if (command.type === 'menu') {
            this._openPopup(command.target, point);
            return;
        }
        console.info(`Netstorm: 메인 메뉴 '${label}' 실행(다음 화면은 아직 구현되지 않음)`, command);
    }

    // 메인 메뉴의 펼침 메뉴(Help·Options)를 point(왼쪽 위 기준점)에 연다. 모르는 대상이면 열지 않는다. 기준점은 원본 스크린샷 측정값과 같은 규칙으로 정해진다(ui/main-menu.js popupAnchor). 열었으면 true. (2026-10-29 WB03-F)
    _openPopup(target, point) {
        if (!this._mainMenu || !this._resources || !this._settings) return false;
        const items = buildPopupItems(target, this._settings);
        if (!items) {
            console.warn(`Netstorm: 알 수 없는 펼침 메뉴 '${target}' 입니다.`);
            return false;
        }
        const popup = new PopupMenu({
            resources: this._resources,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            onCommand: (command) => this._onPopupCommand(command),
            onSound: () => this._audio?.playSound(MENU_SOUND_NAME),
        });
        // 포인터 위치가 없으면(호스트가 직접 호출) 현재 포인터 위치를 쓴다.
        popup.open(items, point ?? { x: Math.max(0, this._input.pointer.x), y: Math.max(0, this._input.pointer.y) });
        this._mainMenu.setPopup(popup);
        this._displayDirty = true;
        return true;
    }

    // 펼침 메뉴 행의 명령을 실행한다(메뉴는 이미 닫혀 있다). Toggle 은 설정 키를 뒤집고, Volume 은 `키=단계` 로 음량을 정하며, Tell 은 대화상자를 연다.
    // F1Help 는 일반 도움말 창을 연다(2026-10-30 WB03-G).
    _onPopupCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'f1help') {
            this._openHelp('F1Help');
        } else if (name === 'toggle') {
            this._settings?.toggle(command.argument);
        } else if (name === 'volume') {
            const [key, value] = String(command.argument).split('=');
            this._settings?.set(key, Number(value));
        } else if (name === 'tell') {
            this._openTell(command.argument);
        } else {
            console.info(`Netstorm: 펼침 메뉴 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
        }
        this._displayDirty = true;
    }

    // 설정 값이 바뀌었을 때 소리에 반영한다. 소리·음악·음량은 바로 적용되고, 바람 소리·좌우 교환·가장자리 스크롤·자동 데모·시작 팁은 값만 저장되며 해당 기능이 생길 때 쓰인다. (2026-10-29 WB03-F)
    // 바뀐 값은 게임 데이터 설정 층에도 옮겨 적어 두 곳이 어긋나지 않게 한다(되돌아오는 통지는 막는다). (2026-10-30 WB03-H)
    _onSettingChange(key) {
        if (key === 'sound' || key === 'music' || key === 'soundVolume' || key === 'musicVolume') this._applySettings();
        const value = this._settings?.get(key);
        if (value !== undefined) this._gameData?.set(key, value, { silent: true });
    }

    // 게임 데이터 설정 층에 쓴 값을 저장소로 옮긴다(원본에서는 메인 루프가 변경 표시를 보고 options.cfg 를 저장한다). 진행 값(`Done*`)은 진행 저장소에,
    // 설정 키(대소문자 무시)는 설정 저장소에 넣어 로컬 저장소에 바로 저장한다. 그 밖의 값(`global.` 실행 중 값 등)은 저장하지 않는다. (2026-10-30 WB03-H)
    _onConfigWrite(key, value) {
        if (isProgressKey(key)) {
            this._progress?.set(key, value);
            return;
        }
        const settingKey = findSettingKey(key);
        if (settingKey) this._settings?.set(settingKey, value);
    }

    // 설정과 진행 저장소의 현재 값을 게임 데이터 설정 층에 옮겨 적는다(통지 없이). 게임 데이터는 재시작해도 다시 만들지 않으므로 init 이 저장소를 새로 읽을 때마다 맞춘다.
    // 게임 데이터가 아직 없으면(첫 적재 전) 하지 않으며, 첫 적재가 같은 값을 넣는다. (2026-10-30 WB03-H)
    _syncGameData() {
        const data = this._gameData;
        if (!data || !this._settings || !this._progress) return;
        // 설정과 진행 값을 하나씩 옮긴다.
        for (const [key, value] of Object.entries({ ...this._settings.snapshot(), ...this._progress.snapshot() })) data.set(key, value, { silent: true });
    }

    // 현재 설정(소리 켜짐·음악 켜짐·음량 단계)을 재생기에 적용한다. 음악을 다시 켠 경우 메인 메뉴 곡을 이어 재생한다. (2026-10-29 WB03-F)
    _applySettings() {
        const settings = this._settings;
        const audio = this._audio;
        if (!settings || !audio) return;
        audio.setSoundVolumeStep(settings.get('soundVolume'));
        audio.setMusicVolumeStep(settings.get('musicVolume'));
        audio.setSoundEnabled(settings.get('sound') === 1);
        audio.setMusicEnabled(settings.get('music') === 1);
        if (settings.get('music') === 1) this._playMenuMusic();
    }

    // Tell 대상으로 대화상자를 열어 메인 메뉴 위에 올린다(원본 Tell 명령 004cf730). 대상은 `섹션이름` 또는 `섹션이름(인자,…)` 이다(지역 인자는 본문의 `{param.1}` 이 읽는다).
    // `Blank` 는 창을 열지 않는 명령이다. 섹션이 없으면 원본처럼 오류 로그만 남기고 아무것도 열지 않는다. 본문이 영숫자 없는 빈 내용이어도 열지 않는다.
    // 본문의 스크립트 명령(`<$config,키=값>` 등)은 창이 올라간 뒤(창이 없어도) 순서대로 실행한다. 열린 대화상자의 명령은 _onDialogCommand 가 받는다. 열었으면 true 를 돌려준다.
    // (2026-10-28 WB03-D, 2026-10-30 WB03-J: Blank·지역 인자·본문 스크립트 명령 추가)
    _openTell(name) {
        if (!this._mainMenu || !this._gameData || !this._resources) return false;
        const result = createTellWindow({
            gameData: this._gameData,
            resources: this._resources,
            name,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            nowSeconds: this._frameSeconds,
            onCommand: (command) => this._onDialogCommand(command),
            onSound: (kind) => this._audio?.playSound(kind === 'menu' ? MENU_SOUND_NAME : BUTTON_SOUND_NAME),
            onUnsupported: (what, text) => console.warn(`Netstorm: Tell '${name}' 의 ${what} 은(는) 아직 지원하지 않습니다.`, text),
        });
        if (result.missing) console.warn(`Netstorm: TELL cannot find "${name}".`);
        if (result.window) {
            this._mainMenu.setDialog(result.window);
            this._displayDirty = true;
        }
        // 본문 안의 스크립트 명령은 원본이 명령 큐에 넣어 다음 입력에서 처리하는 것과 같게, 창을 올린 뒤 순서대로 실행한다.
        for (const command of result.inlineCommands) this._runScriptCommand(command);
        return result.window !== null;
    }

    // 스크립트 명령(본문의 `<$이름,인자>` 와 버튼 명령 공통) 하나를 실행한다. Config 는 `키=값` 을 게임 데이터 설정에 쓰고(설정·진행 저장소로 이어짐), Warning 은 콘솔에 남긴다.
    // 아직 연결하지 않은 명령은 안내만 한다. 실행했으면 true 이다. (2026-10-30 WB03-J)
    _runScriptCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'config' && this._gameData) {
            applyScriptConfig(String(command.argument), (key, value) => this._gameData.set(key, value));
            return true;
        }
        if (name === 'warning') {
            console.warn(`Netstorm: ${command.argument}`);
            return true;
        }
        console.info(`Netstorm: 스크립트 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
        return false;
    }

    // 시작 팁 창의 Prior Tip / Next Tip / OK 버튼(`TellTip,1|2|3`)을 처리한다(원본 TellTip). 현재 팁 번호는 `tipNumber` 설정이며, 바뀐 번호가 설정에 쓰여 저장된다.
    // OK(3)는 다음 팁 번호만 저장하고 창을 다시 열지 않으며, 나머지는 같은 `Tips` 창을 새 번호로 다시 연다. 버튼 인자가 숫자로 시작하지 않으면 0 으로 본다. (2026-10-30 WB03-J)
    _runTellTip(command) {
        const data = this._gameData;
        if (!data) return;
        const argument = String(command.argument ?? '');
        const parameter = argument.length > 0 && argument[0] >= '0' && argument[0] <= '9' ? configParseLong(argument) : 0;
        dispatchTellTip({ code: TELL_TIP_COMMAND, y: parameter }, configParseLong(data.get('tipNumber') ?? '0') >>> 0, {
            format: (value) => String(value),
            setConfig: (key, value) => data.set(key, value),
            tell: (section) => this._openTell(section),
        });
    }

    // 일반 도움말 창을 열어 메인 메뉴 위에 올린다(원본 F1Help: 메인 메뉴 Help 목록의 General Help 행과 F1 키). 이미 열려 있으면 그 창에서 주제만 첫 주제로 되돌린다.
    // 도움말 파일(`Quick Help Spec`)이 없으면 원본처럼 오류 로그만 남기고 열지 않는다. 바깥 주소 링크는 열지 않고 콘솔에 알린다(과거 도메인이라 접속하지 않음). 열었으면 true 를 돌려준다. (2026-10-30 WB03-G)
    _openHelp(topic) {
        if (!this._mainMenu || !this._gameData || !this._resources) return false;
        const open = this._mainMenu.dialog;
        if (open instanceof HelpWindow && !open.closed) {
            open.open(topic);
            this._mainMenu.dirty = true;
            return true;
        }
        const result = createHelpWindow({
            gameData: this._gameData,
            resources: this._resources,
            topic,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            now: () => this._frameSeconds,
            onClose: () => {
                if (this._mainMenu?.dialog === result.window) this._mainMenu.setDialog(null);
                this._displayDirty = true;
            },
            onCommand: (command) => this._onDialogCommand(command),
            onExternal: (address) => console.info(`Netstorm: 도움말의 바깥 주소 '${address}' 는 열지 않습니다(원본의 과거 주소).`),
            onSound: () => this._audio?.playSound(BUTTON_SOUND_NAME),
            onUnsupported: (what, text) => console.warn(`Netstorm: 도움말의 ${what} 은(는) 아직 지원하지 않습니다.`, text),
        });
        if (result.missing) console.warn(`Netstorm: 도움말 파일을 찾지 못했습니다: ${result.path}`);
        if (!result.window) return false;
        this._mainMenu.setDialog(result.window);
        this._displayDirty = true;
        return true;
    }

    // 대화상자에서 실행된 명령(버튼·목록 행·Timeout)을 처리한다. 원본처럼 먼저 대화상자를 닫고(OnExit 명령 실행) 명령을 실행한다.
    // `Tell,이름` 은 다음 대화상자를 열고, `DoNothing` 은 닫기만 한다. `Config,키=값` 은 설정을 쓰고 `TellTip,번호` 는 팁 창을 처리한다(2026-10-30 WB03-J).
    // 그 밖의 명령(StartDemo·MissionBegin 등)은 아직 구현되지 않아 닫고 안내만 남긴다. (2026-10-28 WB03-D)
    _onDialogCommand(command) {
        const dialog = this._mainMenu?.dialog ?? null;
        const onExit = dialog ? dialog.close() : null;
        this._mainMenu?.setDialog(null);
        this._displayDirty = true;
        this._runDialogCommand(command);
        if (onExit) this._runDialogCommand(onExit);
    }

    // 닫힌 뒤 실행할 대화상자 명령 하나를 처리한다. (2026-10-28 WB03-D)
    _runDialogCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'tell') this._openTell(command.argument);
        else if (name === 'config') this._runScriptCommand(command);
        else if (name === 'telltip') this._runTellTip(command);
        else if (name !== 'donothing') console.info(`Netstorm: 대화상자 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
    }

    // 논리 화면 크기의 8비트 표면을 준비한다. 크기가 바뀌면 새로 만들고 메인 메뉴를 다시 배치한다. (2026-10-23 WB03-M1)
    _ensureSurface() {
        const layout = this._layout;
        if (!layout) return;
        const width = layout.logicalWidth;
        const height = layout.logicalHeight;
        if (!this._surface || this._surface.width !== width || this._surface.height !== height) {
            this._surface = new IndexedSurface(width, height);
            this._mainMenu?.resize(width, height);
            this._displayDirty = true;
        } else if (this._mainMenu && (this._mainMenu.width !== width || this._mainMenu.height !== height)) {
            this._mainMenu.resize(width, height);
        }
    }

    // 루트 요소가 속한 문서의 window 를 돌려준다. iframe 안에서도 그 문서의 프레임 예약·픽셀 비율·오디오를 쓰기 위해서다.
    // 루트가 없으면(정리 후) 전역 객체를 돌려준다.
    _view() {
        return this._root?.ownerDocument.defaultView ?? globalThis;
    }

    // ----- 오디오 -----

    // AudioContext 와 재생기(AudioSystem)를 만들고 재생 재개를 요청한다. 사용자 입력 처리 안에서 불러야 한다.
    // 문맥은 원본 표본 빈도(22,050Hz)로 만들고, 받은 효과음·음악을 해독해 둔다.
    // 지원하지 않거나 실패해도 게임은 계속되며 상태 문자열로만 알린다. 메인 메뉴에서 다음 입력 때 다시 시도한다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 재생기·해독 연결)
    _unlockAudio() {
        if (!this.options.audio) {
            this._audioStatus = 'disabled';
            return;
        }
        try {
            if (!this._audioContext) {
                const context = createAudioContext(this._view());
                if (!context) {
                    this._audioStatus = 'unsupported';
                    return;
                }
                this._audioContext = context;
                this._audio = new AudioSystem(context, { soundVolumeStep: this._settings.get('soundVolume'), musicVolumeStep: this._settings.get('musicVolume') });
                this._applySettings();
                this._prepareSounds();
                this._prepareMusic();
            }
            const context = this._audioContext;
            this._audioStatus = context.state;
            if (context.state !== 'running') {
                context
                    .resume()
                    .then(() => {
                        if (this._audioContext === context) this._audioStatus = context.state;
                    })
                    .catch(() => {
                        if (this._audioContext === context) this._audioStatus = 'failed';
                    });
            }
        } catch (error) {
            this._audioStatus = 'failed';
        }
    }

    // 받은 버튼 효과음을 현재 재생기에 해독해 등록한다. 실패하면 소리 없이 계속한다. (2026-10-23 WB03-M1)
    _prepareSounds() {
        const audio = this._audio;
        if (!audio) return;
        if (this._buttonSoundBytes) {
            audio.loadSound(BUTTON_SOUND_NAME, this._buttonSoundBytes).catch((error) => {
                console.warn('Netstorm: 버튼 효과음을 해독하지 못했습니다.', error);
            });
        }
        if (this._menuSoundBytes) {
            audio.loadSound(MENU_SOUND_NAME, this._menuSoundBytes).catch((error) => {
                console.warn('Netstorm: 대화상자 목록 효과음을 해독하지 못했습니다.', error);
            });
        }
    }

    // 받은 메뉴 음악을 현재 재생기에 해독한다. 재생기와 음악 바이트가 모두 있을 때만 진행하며, 끝나면 메인 메뉴에서 재생한다.
    // (2026-10-23 WB03-M1)
    _prepareMusic() {
        const audio = this._audio;
        if (!audio || !this._musicBytes || this._musicStatus !== 'loaded') return;
        this._musicStatus = 'decoding';
        audio
            .loadMusic(MENU_MUSIC_NAME, this._musicBytes)
            .then(() => {
                if (this._audio !== audio) return;
                this._musicStatus = 'ready';
                this._playMenuMusic();
            })
            .catch((error) => {
                if (this._audio !== audio) return;
                this._musicStatus = 'failed';
                console.warn('Netstorm: 메뉴 음악을 해독하지 못했습니다. 음악 없이 계속합니다.', error);
            });
    }

    // 메인 메뉴이고 음악이 해독돼 있으면 메뉴 곡(ser22.mus)을 반복 재생한다(원본 SceneMusic Start: 전투 밖 곡). (2026-10-23 WB03-M1)
    _playMenuMusic() {
        if (this._state !== AppState.MAIN_MENU || !this._audio) return;
        if (this._musicStatus !== 'ready' && this._musicStatus !== 'playing') return;
        if (this._audio.playMusic(MENU_MUSIC_NAME)) this._musicStatus = 'playing';
    }

    // ----- 스타일과 배치 -----

    // 루트 요소의 스타일을 정한다. 기존 인라인 값은 저장해 destroy 에서 되돌린다. CSS 파일은 쓰지 않는다.
    // fillWindow 면 루트가 브라우저 창 전체를 채우고, 아니면 호스트가 정한 크기를 따르되 Canvas 의 절대 배치 기준이 되게 한다.
    _applyRootStyle() {
        const root = this._root;
        const style = root.style;
        this._savedRootStyle = {};
        // 되돌릴 속성의 현재 인라인 값을 모두 저장한다.
        for (const key of ROOT_STYLE_KEYS) this._savedRootStyle[key] = style[key];
        if (this.options.fillWindow) {
            style.position = 'fixed';
            style.left = '0';
            style.top = '0';
            style.right = '0';
            style.bottom = '0';
        } else if (!style.position && root.ownerDocument.defaultView?.getComputedStyle(root).position === 'static') {
            style.position = 'relative';
        }
        style.overflow = 'hidden';
        style.background = '#000';
        style.userSelect = 'none';
        style.webkitUserSelect = 'none';
    }

    // 저장해 둔 루트 인라인 스타일을 되돌린다.
    _restoreRootStyle() {
        if (!this._root || !this._savedRootStyle) return;
        // 저장한 속성을 모두 원래 값으로 되돌린다.
        for (const key of ROOT_STYLE_KEYS) this._root.style[key] = this._savedRootStyle[key];
        this._savedRootStyle = null;
    }

    // Canvas 요소의 고정 스타일을 정한다. 위치와 크기는 _applyLayout 이 계산해 덮어쓴다.
    // 터치 스크롤을 막고, 확대 시 픽셀이 뭉개지지 않게 하며, 키보드 입력을 받도록 포커스를 가능하게 한다.
    _applyCanvasStyle() {
        const canvas = this._canvas;
        const style = canvas.style;
        style.position = 'absolute';
        style.display = 'block';
        style.margin = '0';
        style.padding = '0';
        style.border = '0';
        style.outline = 'none';
        style.touchAction = 'none';
        style.imageRendering = 'pixelated';
        style.visibility = 'hidden';
        canvas.tabIndex = 0;
    }

    // 루트 크기에서 Canvas 배치를 다시 계산해 적용한다. 바뀐 부분만 DOM 에 쓰고, 바뀌면 곧바로 한 번 그려 깜박임을 줄인다.
    _applyLayout() {
        const root = this._root;
        const canvas = this._canvas;
        if (!root || !canvas) return;
        const view = this._view();
        const dpr = view.devicePixelRatio || 1;
        const next = computeLayout(root.clientWidth, root.clientHeight, dpr, this._aspect, this.options.maxCanvasPixels);
        const previous = this._layout;
        this._layout = next;
        if (!previous || previous.canvasWidth !== next.canvasWidth || previous.canvasHeight !== next.canvasHeight) {
            // Canvas 내부 크기를 바꾸면 내용이 지워지고 컨텍스트 상태도 초기화된다.
            canvas.width = next.canvasWidth;
            canvas.height = next.canvasHeight;
        }
        const style = canvas.style;
        style.width = `${next.cssWidth}px`;
        style.height = `${next.cssHeight}px`;
        style.left = `${next.left}px`;
        style.top = `${next.top}px`;
        style.visibility = next.visible ? 'visible' : 'hidden';
        this._displayDirty = true;
        if (this._state === AppState.MAIN_MENU) this._ensureSurface();
        this._applyCursor();
        if (next.visible) this._render(view.performance?.now?.() ?? 0);
    }

    // 원본 기본 커서를 현재 화면 배율에 맞춰 Canvas 에 적용한다. 자원이 없거나 배율·픽셀 비율이 같으면 아무것도 하지 않는다.
    // (2026-10-23 WB03-M1)
    _applyCursor() {
        const layout = this._layout;
        if (!this._canvas || !this._resources || !layout?.visible) return;
        const scale = Math.max(1, Math.round(layout.scale));
        const key = `${scale}:${layout.devicePixelRatio}`;
        if (key === this._cursorKey) return;
        this._cursorKey = key;
        try {
            applyCursorStyle(this._canvas, buildCursorCss(this._root.ownerDocument, this._resources.cursor, layout.scale, layout.devicePixelRatio));
        } catch (error) {
            // 커서 그림을 만들 수 없는 환경이면 기본 커서를 그대로 쓴다.
            console.warn('Netstorm: 원본 커서를 적용하지 못했습니다.', error);
        }
    }

    // ----- 이벤트 -----

    // 대상에 리스너를 붙인다. destroy 의 abort 로 한꺼번에 해제되도록 signal 을 함께 넘긴다.
    _listen(target, type, handler, options = {}) {
        target.addEventListener(type, handler, { ...options, signal: this._abort.signal });
    }

    // 입력·크기·가시성 이벤트를 연결한다.
    _bindEvents() {
        const canvas = this._canvas;
        const doc = this._root.ownerDocument;
        const view = doc.defaultView;

        this._listen(canvas, 'pointerdown', (event) => this._onPointerDown(event));
        this._listen(canvas, 'pointermove', (event) => this._onPointerMove(event));
        this._listen(canvas, 'pointerup', (event) => this._onPointerUp(event));
        this._listen(canvas, 'pointercancel', (event) => this._onPointerCancel(event));
        this._listen(canvas, 'pointerleave', (event) => this._onPointerLeave(event));
        // 시작·재시도 입력은 오디오 정책을 모든 포인터 종류에서 통과하도록 click 으로 받는다.
        this._listen(canvas, 'click', (event) => this._onClick(event));
        // 우클릭은 게임 입력이므로 브라우저 메뉴를 막는다.
        this._listen(canvas, 'contextmenu', (event) => event.preventDefault());
        this._listen(canvas, 'keydown', (event) => this._onKeyDown(event));
        this._listen(canvas, 'keyup', (event) => this._onKeyUp(event));
        // 마우스 휠은 열린 도움말 창 본문을 스크롤한다(웹 적응). 도움말이 열려 있을 때만 페이지 스크롤을 막는다. (2026-10-30 WB03-G)
        this._listen(canvas, 'wheel', (event) => this._onWheel(event), { passive: false });
        this._listen(canvas, 'blur', () => this._clearInput());
        this._listen(doc, 'visibilitychange', () => this._onVisibilityChange());

        // 루트 크기가 바뀌면 배치를 다시 계산한다. 관찰자가 없는 환경에서는 창 크기 변경으로 대신한다.
        if (typeof view?.ResizeObserver === 'function') {
            this._resizeObserver = new view.ResizeObserver(() => this._applyLayout());
            this._resizeObserver.observe(this._root);
        } else if (view) {
            this._listen(view, 'resize', () => this._applyLayout());
        }
        this._watchPixelRatio();
    }

    // 기기 픽셀 비율(화면 배율·창을 옮긴 모니터)이 바뀌면 배치를 다시 계산하도록 감시한다.
    // 한 번 바뀌면 새 비율로 감시를 다시 건다. 지원하지 않는 환경에서는 아무것도 하지 않는다.
    _watchPixelRatio() {
        const view = this._root?.ownerDocument.defaultView;
        if (typeof view?.matchMedia !== 'function' || !this._abort) return;
        const query = view.matchMedia(`(resolution: ${view.devicePixelRatio || 1}dppx)`);
        query.addEventListener(
            'change',
            () => {
                this._applyLayout();
                this._watchPixelRatio();
            },
            { once: true, signal: this._abort.signal },
        );
    }

    // 포인터 이벤트로 입력 상태의 포인터 정보를 갱신한다. 논리 화면과 맞지 않으면 갱신하지 않는다.
    _updatePointer(event) {
        const layout = this._layout;
        if (!layout?.visible) return;
        const point = clientToLogical(
            event.clientX,
            event.clientY,
            this._canvas.getBoundingClientRect(),
            layout.logicalWidth,
            layout.logicalHeight,
        );
        if (!point) return;
        const pointer = this._input.pointer;
        pointer.x = point.x;
        pointer.y = point.y;
        pointer.inside = point.inside;
        pointer.buttons = event.buttons ?? 0;
        pointer.type = event.pointerType || 'mouse';
    }

    // 버튼을 눌렀을 때: 포커스를 받고, 화면 밖으로 끌어도 이벤트를 받도록 포인터를 붙잡는다.
    // 메인 메뉴에서는 왼쪽 버튼 누름만 돌 버튼에 전달한다(원본: 오른쪽·가운데 버튼은 반응 없음). 게임 화면 밖(레터박스) 누름은 무시한다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 메인 메뉴 입력 연결)
    _onPointerDown(event) {
        this._canvas.focus({ preventScroll: true });
        try {
            this._canvas.setPointerCapture(event.pointerId);
        } catch (error) {
            // 포인터 붙잡기에 실패해도 입력 처리는 계속한다.
        }
        this._updatePointer(event);
        this._retryAudioOnGesture();
        const pointer = this._input.pointer;
        if (this._state === AppState.MAIN_MENU && this._mainMenu && event.button === 0 && pointer.inside) {
            this._mainMenu.pointerDown(pointer.x, pointer.y);
        }
    }

    // 포인터가 움직였을 때 좌표를 갱신하고, 메인 메뉴의 붙잡힌 버튼 모양을 갱신한다(원본 Broadcast).
    _onPointerMove(event) {
        this._updatePointer(event);
        if (this._state === AppState.MAIN_MENU && this._mainMenu) this._mainMenu.pointerMove(this._input.pointer.x, this._input.pointer.y);
    }

    // 버튼을 뗐을 때 버튼 상태를 갱신하고, 메인 메뉴에서는 왼쪽 버튼 뗌을 돌 버튼에 전달한다(안쪽에서 떼면 실행).
    _onPointerUp(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        if (this._state === AppState.MAIN_MENU && this._mainMenu && event.button === 0) {
            this._mainMenu.pointerUp(this._input.pointer.x, this._input.pointer.y);
        }
    }

    // 브라우저가 포인터 입력을 취소했을 때(터치 제스처 등) 버튼 상태를 비우고 메뉴 누름을 실행 없이 취소한다. (2026-10-23 WB03-M1)
    _onPointerCancel(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        this._mainMenu?.cancel();
    }

    // 포인터가 Canvas 를 벗어났을 때 안쪽 표시를 끈다. 가장자리 스크롤 등은 이 값을 기준으로 멈춘다.
    _onPointerLeave(event) {
        this._input.pointer.inside = false;
        this._input.pointer.buttons = event.buttons ?? 0;
        // 펼침 메뉴의 행 강조는 포인터를 따라가므로 Canvas 밖으로 나가면 지운다. (2026-10-29 WB03-F)
        if (this._mainMenu?.popup?.pointerLeave()) this._mainMenu.dirty = true;
    }

    // 왼쪽 클릭을 시작·재시도 입력으로 처리한다.
    _onClick(event) {
        if (event.button === 0) this._activate();
    }

    // 키를 눌렀을 때: 눌린 키 집합에 넣고, 게임이 쓰는 스크롤 키의 브라우저 기본 동작을 막고, Enter/Space 는 시작 입력으로 쓴다.
    _onKeyDown(event) {
        this._input.keys.add(event.code);
        if (PREVENTED_KEYS.has(event.code)) event.preventDefault();
        this._retryAudioOnGesture();
        // Esc 는 열려 있는 펼침 메뉴를 닫는다(원본 입력 0x020000be: RequestClosePopups). (2026-10-29 WB03-F)
        if (event.code === 'Escape' && this._state === AppState.MAIN_MENU && this._mainMenu?.closePopup()) this._displayDirty = true;
        // F1 은 일반 도움말을 연다(원본 입력 0x70 → F1Help). 브라우저의 도움말 동작은 막는다. 열린 도움말 창은 PageUp·PageDown·위·아래 화살표로 본문을 스크롤한다. (2026-10-30 WB03-G)
        if (this._state === AppState.MAIN_MENU && this._mainMenu) {
            if (event.code === 'F1') {
                event.preventDefault();
                if (!event.repeat) this._openHelp('F1Help');
            } else if (this._mainMenu.dialog instanceof HelpWindow && this._mainMenu.dialog.keyDown(event.code)) {
                this._mainMenu.dirty = true;
            }
        }
        if (!event.repeat && (event.code === 'Enter' || event.code === 'Space' || event.code === 'NumpadEnter')) this._activate();
    }

    // 마우스 휠을 열린 도움말 창에 넘긴다. 처리했으면 페이지가 따라 스크롤되지 않게 막는다. (2026-10-30 WB03-G)
    _onWheel(event) {
        const dialog = this._mainMenu?.dialog;
        if (this._state !== AppState.MAIN_MENU || !(dialog instanceof HelpWindow)) return;
        event.preventDefault();
        if (dialog.wheel(event.deltaY)) this._mainMenu.dirty = true;
    }

    // 키를 뗐을 때 눌린 키 집합에서 뺀다.
    _onKeyUp(event) {
        this._input.keys.delete(event.code);
    }

    // 포커스를 잃거나 탭이 숨겨질 때 눌린 키·버튼이 남지 않도록 입력 상태를 비우고, 메뉴 누름을 실행 없이 취소한다.
    _clearInput() {
        this._input.keys.clear();
        this._input.pointer.buttons = 0;
        this._input.pointer.inside = false;
        this._mainMenu?.cancel();
    }

    // 시작 이후의 입력에서 오디오가 아직 재생 가능 상태가 아니면 다시 활성화를 시도한다.
    _retryAudioOnGesture() {
        if (this._state === AppState.MAIN_MENU && this._audioStatus !== 'running' && this._audioStatus !== 'unsupported' && this._audioStatus !== 'disabled') {
            this._unlockAudio();
        }
    }

    // 탭이 숨겨지면 프레임 루프를 멈추고 입력을 비우며 소리를 멈추고, 돌아오면 시계 기준점을 다시 잡고 재개한다.
    // 원본 효과음·음악 버퍼는 창이 포커스를 잃으면 들리지 않으므로(GLOBALFOCUS 없음) 웹에서는 탭 숨김을 그 대응으로 쓴다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 소리 일시 정지 추가)
    _onVisibilityChange() {
        if (this._root.ownerDocument.hidden) {
            this._stopFrameLoop();
            this._clearInput();
            this._audio?.suspend().catch(() => {});
        } else {
            this._clock.reset();
            this._displayDirty = true;
            this._startFrameLoop();
            this._audio?.resume().catch(() => {});
        }
    }

    // ----- 프레임 루프 -----

    // 프레임 루프를 시작한다. 이미 예약돼 있거나 탭이 숨겨져 있으면 아무것도 하지 않는다.
    _startFrameLoop() {
        if (this._frameHandle || !this._root || this._root.ownerDocument.hidden) return;
        this._frameHandle = this._view().requestAnimationFrame((now) => this._onFrame(now));
    }

    // 프레임 루프를 멈춘다. 예약된 프레임이 없으면 아무것도 하지 않는다.
    _stopFrameLoop() {
        if (!this._frameHandle) return;
        this._view().cancelAnimationFrame(this._frameHandle);
        this._frameHandle = 0;
    }

    // 매 프레임 호출된다. 시계를 진행하고 화면을 그린 뒤 다음 프레임을 예약한다.
    // 게임 갱신(Process/Regular)은 WB04 이후에 여기서 시계 값과 별도의 게임 시각으로 호출한다.
    _onFrame(now) {
        this._frameHandle = 0;
        if (!this._canvas) return;
        this._clock.tick(now);
        this._frameSeconds = now / 1000;
        if (this._state === AppState.MAIN_MENU) this._mainMenu?.update(this._frameSeconds);
        this._render(now);
        this._startFrameLoop();
    }

    // ----- 그리기 -----

    // 현재 상태에 맞는 장면을 Canvas 에 그린다. 웹 전용 화면은 논리 좌표로 그리고 내부 픽셀 배율(drawScaleX/Y)은 변환 행렬로 적용한다.
    // 메인 메뉴는 8비트 표면을 합성해 표시기로 Canvas 전체에 그리며, 바뀐 것이 없으면(디버그 표시 제외) 다시 그리지 않는다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 원본 메인 메뉴 표시)
    _render(now) {
        const ctx = this._ctx;
        const layout = this._layout;
        if (!ctx || !layout?.visible) return;
        const { logicalWidth: width, logicalHeight: height, drawScaleX, drawScaleY } = layout;
        if (this._state === AppState.MAIN_MENU && this._mainMenu && this._surface) {
            const changed = this._mainMenu.dirty;
            if (!changed && !this._displayDirty && !this.options.debug) return;
            if (changed) this._mainMenu.render(this._surface);
            this._presenter.present(ctx, this._surface, layout, changed);
            this._displayDirty = false;
            ctx.setTransform(drawScaleX, 0, 0, drawScaleY, 0, 0);
            if (this.options.debug) this._drawDebug(ctx);
            return;
        }
        ctx.setTransform(drawScaleX, 0, 0, drawScaleY, 0, 0);
        ctx.imageSmoothingEnabled = false;
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, width, height);

        switch (this._state) {
            case AppState.LOADING:
                this._drawLoading(ctx, width, height, now);
                break;
            case AppState.ERROR:
                this._drawError(ctx, width, height);
                break;
            case AppState.CLICK_TO_START:
                this._drawClickToStart(ctx, width, height, now);
                break;
            default:
                break;
        }
        this._displayDirty = true;
        if (this.options.debug) this._drawDebug(ctx);
    }

    // 글자를 그린다. align 은 가로 정렬, alpha 는 투명도다.
    _drawText(ctx, text, x, y, size, color, align = 'center', alpha = 1) {
        ctx.globalAlpha = alpha;
        ctx.fillStyle = color;
        ctx.font = `${size}px ${FONT_FAMILY}`;
        ctx.textAlign = align;
        ctx.textBaseline = 'middle';
        ctx.fillText(text, x, y);
        ctx.globalAlpha = 1;
    }

    // 로딩 화면: 제목과 회전하는 표시, 자산 적재 진행 막대·백분율을 그린다(웹 전용 화면).
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 진행 막대 추가)
    _drawLoading(ctx, width, height, now) {
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 70, 64, '#d8e4f0');
        const angle = (now / 1000) * Math.PI * 2;
        ctx.strokeStyle = '#7aa7d8';
        ctx.lineWidth = 4;
        ctx.beginPath();
        ctx.arc(width / 2, height / 2 + 10, 22, angle, angle + Math.PI * 1.5);
        ctx.stroke();
        const { loaded, total } = this._loadProgress;
        const ratio = total > 0 ? Math.min(1, loaded / total) : 0;
        const barWidth = 320;
        const barLeft = Math.round(width / 2 - barWidth / 2);
        const barTop = Math.round(height / 2 + 52);
        ctx.strokeStyle = '#4f7aa6';
        ctx.lineWidth = 2;
        ctx.strokeRect(barLeft, barTop, barWidth, 10);
        ctx.fillStyle = '#7aa7d8';
        ctx.fillRect(barLeft + 2, barTop + 2, Math.round((barWidth - 4) * ratio), 6);
        this._drawText(ctx, `${texts.loading} ${Math.floor(ratio * 100)}%`, width / 2, height / 2 + 90, 24, '#9fb4c8');
    }

    // 로딩 실패 화면: 실패 안내와 원인, 재시도 안내를 그린다.
    _drawError(ctx, width, height) {
        const texts = this.texts;
        const reason = String(this._loadError?.message ?? this._loadError ?? '').slice(0, 120);
        this._drawText(ctx, texts.title, width / 2, height / 2 - 90, 64, '#d8e4f0');
        this._drawText(ctx, texts.loadFailed, width / 2, height / 2 - 10, 26, '#ff8a80');
        if (reason) this._drawText(ctx, reason, width / 2, height / 2 + 30, 18, '#c9a0a0');
        this._drawText(ctx, texts.retry, width / 2, height / 2 + 90, 24, '#d8e4f0');
    }

    // '클릭하여 시작' 화면: 제목과 깜박이는 시작 안내를 그린다. 이 화면에서는 소리를 재생하지 않는다.
    _drawClickToStart(ctx, width, height, now) {
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 60, 72, '#d8e4f0');
        const blink = 0.55 + 0.45 * Math.sin(now / 350);
        this._drawText(ctx, texts.clickToStart, width / 2, height / 2 + 50, 30, '#9fd0ff', 'center', blink);
    }

    // 개발용 정보 겹침: 상태·화면 구성·입력·오디오·음악·프레임 수를 왼쪽 위에 그린다. ?debug 로 켠다.
    // (2026-10-23 WB01, 2026-10-23 WB03-M1: 음악·확대 방식 표시 추가)
    _drawDebug(ctx) {
        const layout = this._layout;
        const pointer = this._input.pointer;
        const lines = [
            `state ${this._state}  aspect ${this._aspect}  fps ${this._clock.fps.toFixed(1)}`,
            `mode ${layout.mode}  logical ${layout.logicalWidth}x${layout.logicalHeight}  scaling ${this.options.scaling}`,
            `css ${layout.cssWidth}x${layout.cssHeight} @(${layout.left},${layout.top})  scale ${layout.scale.toFixed(3)}`,
            `canvas ${layout.canvasWidth}x${layout.canvasHeight}  draw ${layout.drawScaleX.toFixed(3)}x${layout.drawScaleY.toFixed(3)}  dpr ${layout.devicePixelRatio}`,
            `pointer ${pointer.x},${pointer.y} inside=${pointer.inside} buttons=${pointer.buttons} (${pointer.type})`,
            `keys ${[...this._input.keys].join(' ')}  audio ${this._audioStatus}  music ${this._musicStatus}`,
        ];
        ctx.fillStyle = 'rgba(0, 0, 0, 0.55)';
        ctx.fillRect(8, 8, 620, lines.length * 20 + 8);
        // 정보 줄을 위에서부터 차례로 그린다.
        lines.forEach((line, index) => this._drawText(ctx, line, 14, 22 + index * 20, 14, '#b8ffb8', 'left'));
    }
}

export default Netstorm;
