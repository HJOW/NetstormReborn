// NetStorm 웹 클론(webpj)의 실행 골격 모듈이다.
// 브라우저 안에 게임 Canvas 를 만들고, 컨테이너 크기·화면비(4:3, 16:10, 16:9)·기기 픽셀 비율에 맞춰
// Canvas 의 크기와 위치를 계산·적용하며, 포인터·키보드 입력을 게임 논리 좌표로 바꾸고,
// "로딩 → 클릭하여 시작 → 메인 메뉴(임시 화면)" 장면 전환과 프레임 루프를 제공한다.
// 게임 규칙·원본 자산·소리 재생은 cpppj 복원 이후 단계(WB02 이후)에서 채운다.
// 이 모듈은 import 시점에 DOM 을 건드리지 않으므로 Node 에서도 불러와 계산 함수를 검사할 수 있다.
// (2026-10-23 WB01: 실행 골격 최초 작성)

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

// 언어별 화면 문구이다. 원본 글꼴(.chfnt)과 D2Coding 글꼴 적재(WB02)와 번역표가 준비되면 그쪽으로 옮긴다.
// 모르는 언어는 영어로 대체한다. (2026-10-23 WB01)
const TEXTS = Object.freeze({
    english: Object.freeze({
        title: 'NetStorm',
        loading: 'Loading...',
        loadFailed: 'Failed to load game data.',
        retry: 'Click to retry',
        clickToStart: 'Click to start',
        menuPlaceholder: 'Main menu (placeholder)',
    }),
    korean: Object.freeze({
        title: 'NetStorm',
        loading: '불러오는 중...',
        loadFailed: '게임 데이터를 불러오지 못했습니다.',
        retry: '클릭하여 다시 시도',
        clickToStart: '클릭하여 시작',
        menuPlaceholder: '메인 메뉴 (임시 화면)',
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
//  - prepare: async (app) => void. 자산 준비 단계 훅(WB02 에서 채움). 예외를 던지면 ERROR 상태가 된다.
//  - onStateChange: (새 상태, 이전 상태) => void. 상태가 바뀔 때 호출된다.
// (2026-10-23 WB01)
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
            prepare: options.prepare ?? (async () => {}),
            onStateChange: options.onStateChange ?? null,
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
        // 오디오 준비 상태 문자열이다. none | unsupported | suspended | running | closed | failed.
        this._audioStatus = 'none';
        // 입력 상태다. pointer 는 논리 좌표, keys 는 눌린 키의 code 집합이다.
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
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
        if (this._audioContext) {
            // 닫기에 실패해도 정리를 막지 않는다.
            Promise.resolve(this._audioContext.close?.()).catch(() => {});
            this._audioContext = null;
        }
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

    // 자산 준비 훅을 실행한다. 성공하면 클릭 대기, 실패하면 오류 화면(클릭으로 재시도)이 된다.
    // 오래된 세대의 결과는 버린다. 준비 단계에서는 오디오를 재생하지 않는다.
    async _load() {
        this._loadGeneration += 1;
        const generation = this._loadGeneration;
        this._loadError = null;
        this._setState(AppState.LOADING);
        try {
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

    // 사용자의 시작 입력(클릭·Enter·Space)을 처리한다.
    // ERROR 에서는 로딩을 다시 시도하고, CLICK_TO_START 에서는 오디오를 준비한 뒤 메인 메뉴로 넘어간다.
    // 오디오 활성화는 사용자 입력 처리 안에서 동기로 요청해야 브라우저 정책을 통과하므로 상태 전환보다 먼저 한다.
    _activate() {
        if (this._state === AppState.ERROR) {
            this._load();
        } else if (this._state === AppState.CLICK_TO_START) {
            this._unlockAudio();
            this._setState(AppState.MAIN_MENU);
        }
    }

    // 루트 요소가 속한 문서의 window 를 돌려준다. iframe 안에서도 그 문서의 프레임 예약·픽셀 비율·오디오를 쓰기 위해서다.
    // 루트가 없으면(정리 후) 전역 객체를 돌려준다.
    _view() {
        return this._root?.ownerDocument.defaultView ?? globalThis;
    }

    // ----- 오디오 -----

    // AudioContext 를 만들고 재생 재개를 요청한다. 사용자 입력 처리 안에서 불러야 한다.
    // 지원하지 않거나 실패해도 게임은 계속되며 상태 문자열로만 알린다. 메인 메뉴에서 다음 입력 때 다시 시도한다.
    _unlockAudio() {
        const view = this._view();
        const AudioContextClass = view.AudioContext ?? view.webkitAudioContext;
        if (!AudioContextClass) {
            this._audioStatus = 'unsupported';
            return;
        }
        try {
            if (!this._audioContext) this._audioContext = new AudioContextClass();
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
        if (next.visible) this._render(view.performance?.now?.() ?? 0);
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
        this._listen(canvas, 'pointercancel', (event) => this._onPointerUp(event));
        this._listen(canvas, 'pointerleave', (event) => this._onPointerLeave(event));
        // 시작·재시도 입력은 오디오 정책을 모든 포인터 종류에서 통과하도록 click 으로 받는다.
        this._listen(canvas, 'click', (event) => this._onClick(event));
        // 우클릭은 게임 입력이므로 브라우저 메뉴를 막는다.
        this._listen(canvas, 'contextmenu', (event) => event.preventDefault());
        this._listen(canvas, 'keydown', (event) => this._onKeyDown(event));
        this._listen(canvas, 'keyup', (event) => this._onKeyUp(event));
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
    _onPointerDown(event) {
        this._canvas.focus({ preventScroll: true });
        try {
            this._canvas.setPointerCapture(event.pointerId);
        } catch (error) {
            // 포인터 붙잡기에 실패해도 입력 처리는 계속한다.
        }
        this._updatePointer(event);
        this._retryAudioOnGesture();
    }

    // 포인터가 움직였을 때 좌표를 갱신한다.
    _onPointerMove(event) {
        this._updatePointer(event);
    }

    // 버튼을 뗐거나 입력이 취소됐을 때 버튼 상태를 갱신한다.
    _onPointerUp(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
    }

    // 포인터가 Canvas 를 벗어났을 때 안쪽 표시를 끈다. 가장자리 스크롤 등은 이 값을 기준으로 멈춘다.
    _onPointerLeave(event) {
        this._input.pointer.inside = false;
        this._input.pointer.buttons = event.buttons ?? 0;
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
        if (!event.repeat && (event.code === 'Enter' || event.code === 'Space' || event.code === 'NumpadEnter')) this._activate();
    }

    // 키를 뗐을 때 눌린 키 집합에서 뺀다.
    _onKeyUp(event) {
        this._input.keys.delete(event.code);
    }

    // 포커스를 잃거나 탭이 숨겨질 때 눌린 키·버튼이 남지 않도록 입력 상태를 비운다.
    _clearInput() {
        this._input.keys.clear();
        this._input.pointer.buttons = 0;
        this._input.pointer.inside = false;
    }

    // 시작 이후의 입력에서 오디오가 아직 재생 가능 상태가 아니면 다시 활성화를 시도한다.
    _retryAudioOnGesture() {
        if (this._state === AppState.MAIN_MENU && this._audioStatus !== 'running' && this._audioStatus !== 'unsupported') {
            this._unlockAudio();
        }
    }

    // 탭이 숨겨지면 프레임 루프를 멈추고 입력을 비우며, 돌아오면 시계 기준점을 다시 잡고 재개한다.
    _onVisibilityChange() {
        if (this._root.ownerDocument.hidden) {
            this._stopFrameLoop();
            this._clearInput();
        } else {
            this._clock.reset();
            this._startFrameLoop();
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
        this._render(now);
        this._startFrameLoop();
    }

    // ----- 그리기 -----

    // 현재 상태에 맞는 장면을 Canvas 에 그린다. 논리 좌표로 그리고 내부 픽셀 배율(drawScaleX/Y)은 변환 행렬로 적용한다.
    _render(now) {
        const ctx = this._ctx;
        const layout = this._layout;
        if (!ctx || !layout?.visible) return;
        const { logicalWidth: width, logicalHeight: height, drawScaleX, drawScaleY } = layout;
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
            case AppState.MAIN_MENU:
                this._drawMainMenuPlaceholder(ctx, width, height);
                break;
            default:
                break;
        }
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

    // 로딩 화면: 제목과 회전하는 표시를 그린다.
    _drawLoading(ctx, width, height, now) {
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 70, 64, '#d8e4f0');
        const angle = (now / 1000) * Math.PI * 2;
        ctx.strokeStyle = '#7aa7d8';
        ctx.lineWidth = 4;
        ctx.beginPath();
        ctx.arc(width / 2, height / 2 + 10, 22, angle, angle + Math.PI * 1.5);
        ctx.stroke();
        this._drawText(ctx, texts.loading, width / 2, height / 2 + 70, 24, '#9fb4c8');
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

    // 메인 메뉴 임시 화면: 원본 메뉴(WB03)가 만들어지기 전까지 화면 구성과 좌표 변환을 눈으로 확인하는 용도다.
    // 논리 화면의 테두리와 모서리 표시, 포인터 위치의 십자선을 그린다. 원본 메뉴를 복원한 화면이 아니다.
    _drawMainMenuPlaceholder(ctx, width, height) {
        ctx.strokeStyle = '#2f4a66';
        ctx.lineWidth = 2;
        ctx.strokeRect(1, 1, width - 2, height - 2);
        ctx.strokeStyle = '#4f7aa6';
        ctx.lineWidth = 3;
        const tick = 24;
        // 네 모서리에 L 자 표시를 그린다.
        for (const [x, y, dx, dy] of [
            [0, 0, 1, 1],
            [width, 0, -1, 1],
            [0, height, 1, -1],
            [width, height, -1, -1],
        ]) {
            ctx.beginPath();
            ctx.moveTo(x + dx * tick, y);
            ctx.lineTo(x, y);
            ctx.lineTo(x, y + dy * tick);
            ctx.stroke();
        }
        const texts = this.texts;
        this._drawText(ctx, texts.title, width / 2, height / 2 - 40, 72, '#d8e4f0');
        this._drawText(ctx, texts.menuPlaceholder, width / 2, height / 2 + 40, 26, '#9fb4c8');

        const pointer = this._input.pointer;
        if (pointer.inside) {
            ctx.strokeStyle = '#ffd36b';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(pointer.x - 12, pointer.y + 0.5);
            ctx.lineTo(pointer.x + 13, pointer.y + 0.5);
            ctx.moveTo(pointer.x + 0.5, pointer.y - 12);
            ctx.lineTo(pointer.x + 0.5, pointer.y + 13);
            ctx.stroke();
        }
    }

    // 개발용 정보 겹침: 상태·화면 구성·입력·오디오·프레임 수를 왼쪽 위에 그린다. ?debug 로 켠다.
    _drawDebug(ctx) {
        const layout = this._layout;
        const pointer = this._input.pointer;
        const lines = [
            `state ${this._state}  aspect ${this._aspect}  fps ${this._clock.fps.toFixed(1)}`,
            `mode ${layout.mode}  logical ${layout.logicalWidth}x${layout.logicalHeight}`,
            `css ${layout.cssWidth}x${layout.cssHeight} @(${layout.left},${layout.top})  scale ${layout.scale.toFixed(3)}`,
            `canvas ${layout.canvasWidth}x${layout.canvasHeight}  draw ${layout.drawScaleX.toFixed(3)}x${layout.drawScaleY.toFixed(3)}  dpr ${layout.devicePixelRatio}`,
            `pointer ${pointer.x},${pointer.y} inside=${pointer.inside} buttons=${pointer.buttons} (${pointer.type})`,
            `keys ${[...this._input.keys].join(' ')}  audio ${this._audioStatus}`,
        ];
        ctx.fillStyle = 'rgba(0, 0, 0, 0.55)';
        ctx.fillRect(8, 8, 620, lines.length * 20 + 8);
        // 정보 줄을 위에서부터 차례로 그린다.
        lines.forEach((line, index) => this._drawText(ctx, line, 14, 22 + index * 20, 14, '#b8ffb8', 'left'));
    }
}

export default Netstorm;
