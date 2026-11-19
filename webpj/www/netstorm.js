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
// (2026-10-31 WB03-N: 언어가 한국어면 갈무리11 글꼴·번역표 묶음을 받아 메인 메뉴·펼침 메뉴 라벨을 한국어로 그린다)
import { AssetLoader } from './assets/asset-loader.js';
import { GameData } from './assets/game-data.js';
import { GamePalette } from './assets/palette.js';
import { loadTerrainScene } from './assets/terrain-scene.js';
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
import { KOREAN_BUNDLE, buildKoreanSupport } from './assets/korean-assets.js';
import { KOREAN_FONT_FAMILY } from './assets/korean-font-config.js';
import { AudioSystem, createAudioContext, volumeStepToCentibels } from './audio/audio-system.js';
import { SoundList } from './audio/sound-list.js';
import { MissionAudioAssets } from './audio/mission-audio-assets.js';
import { createSoundState, SoundPlayer } from './audio/sound-player.js';
import { WebSoundDevice } from './audio/web-sound-device.js';
import { IndexedSurface } from './render/indexed-surface.js';
import { IndexedPresenter, normalizeScaling } from './render/presenter.js';
import { Screen, ScreenMode } from './render/screen.js';
import { Renderer } from './render/renderer.js';
import { SquidRenderer } from './render/squid-renderer.js';
import { Settings, findSettingKey, getLocalStorage } from './platform/settings.js';
import { Progress, isProgressKey } from './platform/progress.js';
import { reportGameError } from './platform/error-reporting.js';
import { BrowserInputPoll } from './platform/browser-input-poll.js';
import { queueBrowserMouseEvent } from './platform/browser-mouse-events.js';
import { queueBrowserVirtualKeyEvent, queueBrowserCharacterEvent } from './platform/browser-key-events.js';
import { InputQueue } from './core/input-queue.js';
import { queueCommittedText } from './core/display-text-input.js';
import { GumpPointerBoundaryState, prepareGumpPointerBoundaryWithQueue } from './ui/gump-pointer-boundary.js';
import { ClientFrameClock } from './core/client-frame-clock.js';
import { ClientLocalStateGlobals } from './core/client-local-state-globals.js';
import { ClientKencloudGlobals } from './core/client-kencloud-globals.js';
import { ClientColorSortInput } from './core/client-color-sort-input.js';
import { MemoryColorMapFiles } from './core/color-map-files.js';
import { ClientHtmlWindowGlobals } from './core/client-html-window-globals.js';
import { ClientConnectionGlobals } from './core/client-connection-globals.js';
import { ClientConnectionActivityGlobals } from './core/client-connection-activity-globals.js';
import { bindCampaignAiClientServices, bindCampaignClientInputServices, bindCampaignOwnershipTheme, bindCampaignProductionRefresh, captureCampaignMissionStarting, runCampaignMissionStarting } from './core/campaign-client-services.js';
import { CHAT_RECORD_BYTES, RawChatChannels } from './core/raw-chat-channels.js';
import { ClientRandomGlobals } from './core/random.js';
import { ClientSpGlobals } from './core/client-sp-globals.js';
import { ClientSubscriberGlobals } from './core/client-subscriber-globals.js';
import { ClientGameplayGlobals } from './core/client-gameplay-globals.js';
import { ClientCoreDataGlobals } from './core/client-core-data-globals.js';
import { ClientPlayerRecords } from './core/client-player-records.js';
import { initializeClientPlayerRecords } from './core/client-player-runtime.js';
import { CampaignTemplateStorage } from './core/campaign-template-storage.js';
import { loadClientWorldAssets } from './core/client-world-assets.js';
import { ClientMissionAssets } from './core/client-mission-assets.js';
import { CampaignGameWorld } from './core/campaign-game-world.js';
import { parseFort } from './assets/fort.js';
import { createFortTypes } from './assets/type-flags.js';
import { PATCH_TYPE_COUNT } from './assets/type-table.js';
import { ClientCampaignGuide } from './core/client-campaign-guide.js';
import { ClientFortGlobals } from './core/client-fort-globals.js';
import { ClientFortCurrentName } from './core/client-fort-current-name.js';
import { refreshWindowTitle } from './core/window-title.js';
import { decodeWindows1252, encodeWindows1252 } from './assets/text-encoding.js';
import { displayTextCodec } from './assets/display-text.js';
import { CampaignSessionAssets } from './core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from './core/priest-placement-assets.js';
import { RawSurfaceStorage } from './core/raw-surface-storage.js';
import { Kernel } from './core/kernel.js';
import { bindClientKernelDiagnostics } from './core/client-diagnostics.js';
import { ClientTimeSource } from './core/client-time-source.js';
import { resetGameStorage } from './platform/reset-storage.js';
import { createCanvasGlyphRasterizer } from './platform/glyph-rasterizer.js';
import { MainMenuScreen } from './ui/main-menu.js';
import { ClientGumpRoot } from './ui/client-gump-root.js';
import { UberGumpCamera } from './ui/uber-gump-camera.js';
import { TerrainPreview } from './ui/terrain-preview.js';
import { PopupMenu } from './ui/popup-menu.js';
import { buildPopupItems } from './ui/option-menus.js';
import { createTellWindow } from './ui/tell-window.js';
import { createMissionLoadingWindow, missionLoginProgressText } from './ui/mission-loading-window.js';
import { TELL_TIP_COMMAND, dispatchTellTip } from './ui/tell-tip.js';
import { applyScriptConfig, parseScriptCommand } from './ui/script-command.js';
import { dispatchHelpLink } from './ui/help-link.js';
import { resolveExternalLink } from './ui/external-link.js';
import { openExternalLink } from './platform/external-opener.js';
import { configParseLong } from './assets/config.js';
import { HelpWindow, createHelpWindow } from './ui/help-window.js';
import { CURSOR_NORMAL, applyCursorStyle, buildCursorCss } from './ui/cursor.js';

// ---------------------------------------------------------------------------------------------
// 상수
// ---------------------------------------------------------------------------------------------

// 논리 화면의 높이(픽셀)이다. 원본이 1024x768 이 최대였으므로 모든 화면비에서 높이를 768 로 고정해
// UI 의 세로 배치를 원본과 같게 유지하고, 화면비가 넓을수록 가로 시야만 늘어나게 한다. (2026-10-23 WB01)
const LOGICAL_HEIGHT = 768;

// 지원하는 화면비 목록이다. name 은 설정·URL 에서 쓰는 이름, ratio 는 명목 가로세로비,
// width/height 는 그 화면비에서의 논리 화면 크기(픽셀, 가로는 짝수)다.
// 작거나 세로가 짧은 창의 기본 논리 크기다. 큰 16:9 창의 상위 크기는 WIDE_LOGICAL_SIZES에서 고른다. (2026-11-19 WB04-E943)
export const SCREEN_MODES = Object.freeze([
    Object.freeze({ name: '4:3', ratio: 4 / 3, width: 1024, height: LOGICAL_HEIGHT }),
    Object.freeze({ name: '16:10', ratio: 16 / 10, width: 1228, height: LOGICAL_HEIGHT }),
    Object.freeze({ name: '16:9', ratio: 16 / 9, width: 1366, height: LOGICAL_HEIGHT }),
]);

// 16:9 창의 논리 크기 후보들이다. CSS 픽셀로 담을 수 있는 가장 큰 후보를 사용한다.
// 고정 단계로 선택해 창을 세밀하게 조절할 때마다 논리 표면을 다시 만들지 않게 한다. (2026-11-19 WB04-E943)
const WIDE_LOGICAL_SIZES = Object.freeze([
    Object.freeze({ width: 1920, height: 1080 }),
    Object.freeze({ width: 1600, height: 900 }),
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
export const MAIN_MENU_WINDOW_TITLE = 'NetStorm Main Menu';

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
    // 게임 규칙 없이 지면·절벽·받침을 점검하는 개발용 정적 화면이다. (2026-11-02 WB03-AA)
    TERRAIN_PREVIEW: 'terrainPreview',
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
        resetConfirm: 'Reset all settings and campaign progress? The game will reload.',
        resetFailed: 'Could not reset game data. Check storage access and try again.',
    }),
    korean: Object.freeze({
        title: 'NetStorm',
        loading: '불러오는 중...',
        loadFailed: '게임 데이터를 불러오지 못했습니다.',
        retry: '클릭하여 다시 시도',
        clickToStart: '클릭하여 시작',
        resetConfirm: '모든 설정과 캠페인 진행 상황을 초기화할까요? 게임을 다시 불러옵니다.',
        resetFailed: '게임 데이터를 초기화하지 못했습니다. 저장소 접근을 확인한 후 다시 시도하세요.',
    }),
});

// 시작·로딩 안내 Canvas 글꼴 목록이다. 한국어 글꼴이 등록되기 전에는 뒤의 시스템 글꼴로 대체한다.
const FONT_FAMILY = `"${KOREAN_FONT_FAMILY}", "Malgun Gothic", "Segoe UI", monospace`;

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
//  - 논리 좌표: 게임 코드가 그리는 좌표계. 화면비와 창의 CSS 픽셀 크기로 결정한다.
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
    // 고DPI 기기에서도 창의 CSS 크기에 맞춰 논리 좌표를 고른다. 기기 픽셀 배율은 Canvas 내부 크기에만 반영한다.
    const wide = mode.name === '16:9' ? WIDE_LOGICAL_SIZES.find(
        (size) => containerWidth >= size.width && containerHeight >= size.height,
    ) : undefined;
    const { width, height } = wide ?? mode;
    const hidden = {
        visible: false,
        mode: mode.name,
        logicalWidth: width,
        logicalHeight: height,
        scale: 0,
        cssWidth: 0,
        cssHeight: 0,
        left: 0,
        top: 0,
        canvasWidth: width,
        canvasHeight: height,
        drawScaleX: 1,
        drawScaleY: 1,
        devicePixelRatio: dpr,
    };
    if (!(containerWidth > 0) || !(containerHeight > 0)) return Object.freeze(hidden);

    // 가로·세로 각각 컨테이너를 채우는 데 필요한 배율(논리 픽셀 1개의 CSS 픽셀 수)이다.
    const fillX = containerWidth / width;
    const fillY = containerHeight / height;
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
        cssWidth = floorPixel(width * scale * dpr) / dpr;
        cssHeight = floorPixel(height * scale * dpr) / dpr;
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
        logicalWidth: width,
        logicalHeight: height,
        scale,
        cssWidth,
        cssHeight,
        left: floorPixel(((containerWidth - cssWidth) / 2) * dpr) / dpr,
        top: floorPixel(((containerHeight - cssHeight) / 2) * dpr) / dpr,
        canvasWidth,
        canvasHeight,
        drawScaleX: canvasWidth / width,
        drawScaleY: canvasHeight / height,
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
//  - soundVolume / musicVolume: 원본 음량 단계 1~5(최초 기본값 5 / 5, 기존 저장값 우선).
//  - prepare: async (app) => void. 내장 자산 적재 뒤 실행하는 추가 준비 훅. 예외를 던지면 ERROR 상태가 된다.
//  - onStateChange: (새 상태, 이전 상태) => void. 상태가 바뀔 때 호출된다.
//  - onMenuCommand: (명령, 라벨) => void. 메인 메뉴 버튼이 실행됐을 때 호출된다(다음 화면은 아직 없음).
//  - previewMap: 개발용 정적 지형을 클릭하여 시작 뒤 연다. 기본은 ?map=파일이름, 없으면 null이다. 미션은 실행하지 않는다. (2026-11-02 WB03-AA)
//  - originalComparison: true 면 웹이 빼는 원본 화면 요소(8버튼·펼침 메뉴 전체 항목·User Made Campaigns 행)를 보이는 원본 비교 모드이다(기본 false).
//    cpppj 오라클과 비교하는 무브라우저 실행(tools/headless.mjs)이 쓴다. 그 요소의 기능은 구현하지 않는다. (2026-11-07 WB02-H)
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
            previewMap: options.previewMap ?? (params.has('map') ? params.get('map') || 'savetheisland' : null),
            fillWindow: options.fillWindow ?? true,
            autoFocus: options.autoFocus ?? true,
            maxCanvasPixels: Math.max(1, Math.floor(options.maxCanvasPixels ?? DEFAULT_MAX_CANVAS_PIXELS) || DEFAULT_MAX_CANVAS_PIXELS),
            assetBase: options.assetBase ?? DEFAULT_ASSET_BASE,
            fetch: options.fetch ?? null,
            scaling: normalizeScaling(options.scaling ?? params.get('scaling')),
            audio: options.audio ?? !params.has('mute'),
            // 원본 비교 모드이다. 참이면 웹 판본이 빼는 원본 화면 요소(메인 메뉴 Multiplayer·Edit·Quit 버튼, 펼침 메뉴의 Direct Draw·Resolution·Technical Help 등,
            // Campaign 목록의 User Made Campaigns 행)를 원본처럼 보여 cpppj 오라클·원본 스크린샷과 같은 배치로 비교한다. 그 요소들의 동작(Edit·멀티플레이 등)은 여전히 구현하지 않는다.
            // 무브라우저 실행(tools/headless.mjs)의 오라클 비교 전용이며 기본값 false 이다. (2026-11-07 WB02-H)
            originalComparison: options.originalComparison ?? false,
            // 호스트가 정한 음량 단계(1~5)이다. 없으면(null) 저장된 설정을 쓴다. 정하면 저장하지 않고 이번 실행에서만 우선한다. (2026-10-29 WB03-F)
            soundVolume: options.soundVolume ?? null,
            musicVolume: options.musicVolume ?? null,
            // 바깥 링크를 여는 함수 (문서, 판정 결과) => 열었으면 true 이다. 기본(null)은 platform/external-opener.js 의 새 탭/창 열기이다.
            // 무브라우저 실행(tools/headless.mjs)과 검사가 실제 탭을 열지 않도록 바꿔 끼운다. (2026-11-07 WB03-AD)
            openExternal: options.openExternal ?? null,
            prepare: options.prepare ?? (async () => {}),
            onStateChange: options.onStateChange ?? null,
            onMenuCommand: options.onMenuCommand ?? null,
        };
        // 현재 화면비 설정이다. setAspect 로 바꿀 수 있다.
        this._aspect = findScreenMode(requestedAspect) ? requestedAspect : AUTO_ASPECT;
        // 현재 앱 상태다.
        this._state = AppState.IDLE;
        // 개발용 지형 화면이다. 일반 메뉴에서는 null이며 destroy/메뉴 복귀에서 참조를 놓는다. (2026-11-02 WB03-AA)
        this._terrainPreview = null;
        // 게임을 넣은 루트 요소다. destroy 후에는 null 이다.
        this._root = null;
        // 게임 Canvas 요소다.
        this._canvas = null;
        // IME 포커스가 필요할 때만 만드는 실제 편집 요소다.
        this._clientImeElement = null;
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
        // Client 게임 시계와 보정 전역은 init에서 생성하고 destroy에서 반납한다.
        this._clientLocal = null; this._gameClock = null; this._hiddenGamePause = false;
        // 같은 Client 수명의 구름·HTML과 연결표·연결 활동 전역이다.
        this._clientKencloud = null; this._clientHtml = null; this._clientConnections = null; this._clientConnectionActivity = null;
        // 같은 Client가 장기 소유하는 게임·CRT 난수와 SP 저장소다.
        this._clientRandom = null; this._clientSp = null;
        // 같은 Client가 소유하는 원본 채팅 채널 100기록과 그 연산이다.
        this._clientChatRecords = null; this._clientChatChannels = null;
        // 미션 Session이 공유하는 효과음 이름표는 Client 수명에 소유한다.
        this._clientSoundNames = null;
        // 현재 브라우저 효과음 장치와 원본 재생 전역·준비 중인 수명이다.
        this._clientSoundState = null; this._clientSoundDevice = null; this._clientSoundPlayer = null; this._clientSoundPrepare = null;
        // 시작 팔레트와 최초 색 정렬 입력은 Client 수명 동안 같은 객체로 유지한다.
        this._clientPaletteScreen = null; this._clientColorSort = null;
        // 논리 화면에 대응하는 실제 Renderer와 월드 변경 영역 전달 장치다.
        this._clientRenderer = null; this._clientSurfaceDisplay = null;
        // 설치 폴더에 쓰지 않는 색표 캐시는 같은 페이지의 앱 재시작 사이에 유지한다.
        this._clientColorMapFiles = new MemoryColorMapFiles();
        // Client 공통 Player·Core·Template과 Subscriber는 Session보다 오래 산다.
        this._clientSubscriber = null; this._clientGameplay = null; this._clientCoreData = null; this._clientPlayers = null; this._clientTemplates = null;
        // 자산 적재 뒤 구성해 Session보다 오래 유지하는 월드 물리와 Kernel이다.
        this._gameAssets = null; this._surfaceStorage = null; this._clientKernel = null;
        // 실제 미션 Guide는 자산과 장기 저장소가 준비된 뒤 생성한다.
        this._clientGuide = null; this._clientFort = null; this._clientFortName = null;
        // 원본 공통 Gump 루트와 입력 전역은 Client 수명에 보유한다.
        this._clientGump = null;
        // 설치된 GameWorld와 그 월드를 빌리는 현재 미션 세대다.
        this._campaignWorld = null; this._campaignWorldMissionToken = 0;
        // 현재 메뉴에 표시한 미션 진행 창의 소유 수명이다.
        this._missionLoadingWindow = null;
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
        // 재시작 시 현재 브라우저 입력과 창 활성 상태를 다시 빌리는 폴링 소유자다.
        this._clientInputPoll = new BrowserInputPoll(() => this._input, () => this._layout, () => this._root?.ownerDocument?.hasFocus?.() ?? true);
        // 원본 40칸 입력 큐와 창 안팎 정적 기록은 같은 Client 수명이다.
        this._clientInputQueue = new InputQueue(); this._clientPointerBoundary = new GumpPointerBoundaryState();
        // 원본 입력 소비자가 설치되기 전에는 독립 메뉴와 같은 사건을 중복 예약하지 않는다.
        this._clientNativeInputEnabled = false;
        // 원본 조합 중 키 중복을 막는 현재 브라우저 조합 상태다.
        this._clientComposing = false;
        // 같은 편집 동작의 후속 beforeinput을 중복 예약하지 않기 위한 본문이다.
        this._clientImePending = null;
        // 메인 메뉴 그리기 자원 묶음(buildMenuResources 결과)이다. 자산 적재 전에는 null 이며, 재시작해도 다시 받지 않는다.
        this._resources = null;
        // 같은 공개 자산 매니페스트와 현재 오디오의 미션 지연 적재 수명이다.
        this._assetLoader = null; this._missionAudioAssets = null;
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
        // 마지막으로 적용한 커서 CSS 의 열쇠(커서 번호·배율·픽셀 비율)다. 같으면 다시 만들지 않는다.
        this._cursorKey = '';
        // 지금 적용된 원본 커서 번호이다(1 기본, 15 스크롤, 16 링크). (2026-10-31 WB03-L)
        this._cursorNumber = CURSOR_NORMAL;
        // 번호 → 현재 배율로 만든 CSS 커서 값의 캐시이다. 배율·픽셀 비율(_cursorSizeKey)이 바뀌면 비운다. 커서가 바뀔 때마다 그림을 새로 만들지 않으려는 것이다. (2026-10-31 WB03-L)
        this._cursorCache = new Map();
        // 캐시가 만들어진 배율·픽셀 비율의 열쇠이다. (2026-10-31 WB03-L)
        this._cursorSizeKey = '';
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
        try {
            return this._init(root);
        } catch (error) {
            reportGameError('initialization', error, { state: this._state });
            throw error;
        }
    }

    // 초기화 본문을 실행한다. 실패 보고는 공개 init 에서 맡고 기존 부분 정리와 예외 전달은 유지한다. (2026-11-19 WB04-E862)
    _init(root) {
        if (!root || root.nodeType !== 1) throw new TypeError('Netstorm.init: 게임을 넣을 DOM 요소가 필요합니다.');
        if (this._root) this.destroy();

        const doc = root.ownerDocument;
        // Canvas 를 먼저 만들어 보고, 2D 컨텍스트를 못 얻으면 루트를 건드리기 전에 실패한다.
        const canvas = doc.createElement('canvas');
        const ctx = canvas.getContext('2d', { alpha: false });
        if (!ctx) throw new Error('Netstorm.init: Canvas 2D 컨텍스트를 만들 수 없습니다.');

        // 원본처럼 공통 SP를 설정·화면 해석보다 먼저 초기화하고 실패 시 루트는 건드리지 않는다.
        this._initializeClientRuntime(doc.hidden);
        this._root = root;
        this._canvas = canvas;
        this._ctx = ctx;
        this._abort = new AbortController();
        this._clock = new FrameClock();
        this._input = { pointer: { x: -1, y: -1, inside: false, buttons: 0, type: 'mouse' }, keys: new Set() };
        // 재시작 시 현재 브라우저 입력과 창 활성 상태를 다시 빌리는 폴링 소유자다.
        this._clientInputPoll = new BrowserInputPoll(() => this._input, () => this._layout, () => this._root?.ownerDocument?.hasFocus?.() ?? true);
        // 원본 40칸 입력 큐와 창 안팎 정적 기록은 같은 Client 수명이다.
        this._clientInputQueue = new InputQueue(); this._clientPointerBoundary = new GumpPointerBoundaryState();
        // 원본 입력 소비자가 설치되기 전에는 독립 메뉴와 같은 사건을 중복 예약하지 않는다.
        this._clientNativeInputEnabled = false;
        // 원본 조합 중 키 중복을 막는 현재 브라우저 조합 상태다.
        this._clientComposing = false;
        // 같은 편집 동작의 후속 beforeinput을 중복 예약하지 않기 위한 본문이다.
        this._clientImePending = null;
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
        this._cursorNumber = CURSOR_NORMAL;
        this._cursorCache.clear();
        this._cursorSizeKey = '';
        this._savedTitle = doc.title;
        try {
            this._applyRootStyle();
            this._applyCanvasStyle();
            root.appendChild(canvas);
            this._bindEvents();
            this._applyLayout();
            this._initializeClientRendererView();
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

    // 원본 공통 시작의 SP 초기화와 게임 시계 생성을 같은 Client 소유자로 연결한다.
    _initializeClientRuntime(hidden) {
        const source = new ClientTimeSource(), random = new ClientRandomGlobals();
        const sp = new ClientSpGlobals(random, () => source.unixSeconds()); sp.store().initialize();
        const local = new ClientLocalStateGlobals(), clock = new ClientFrameClock(source, local.gameTicks());
        const cloud = new ClientKencloudGlobals(), html = new ClientHtmlWindowGlobals(), connections = new ClientConnectionGlobals(local), activity = new ClientConnectionActivityGlobals();
        const subscriber = new ClientSubscriberGlobals(), gameplay = new ClientGameplayGlobals(subscriber), fort = new ClientFortGlobals();
        const chatRecords = new Uint8Array(CHAT_RECORD_BYTES), chatChannels = new RawChatChannels(chatRecords, () => subscriber.mySubscriber().value);
        const coreData = new ClientCoreDataGlobals(), players = new ClientPlayerRecords(), templates = new CampaignTemplateStorage();
        const soundNames = new SoundList(); soundNames.initialize();
        players.bindCoreData(coreData);
        initializeClientPlayerRecords({ gameplayGlobals: () => gameplay, playerRecords: () => players });
        if (hidden) clock.pushGamePause();
        this._clientRandom = random; this._clientSp = sp; this._clientLocal = local; this._gameClock = clock; this._hiddenGamePause = Boolean(hidden);
        this._clientKencloud = cloud; this._clientHtml = html; this._clientConnections = connections; this._clientConnectionActivity = activity;
        this._clientSubscriber = subscriber; this._clientGameplay = gameplay; this._clientCoreData = coreData; this._clientPlayers = players; this._clientTemplates = templates; this._clientFort = fort;
        this._clientSoundNames = soundNames;
        this._clientChatRecords = chatRecords; this._clientChatChannels = chatChannels;
    }

    // 현재 설정에서 구름 전역 옵션을 읽는다. 설정이나 Client가 아직 없으면 실제 초기화 시 다시 적용한다.
    _syncClientCloudOptions() {
        if (this._clientKencloud && this._gameData) this._clientKencloud.readOptions(this._gameData);
    }

    // 실제 게임 자산에서 장기 물리를 만들고 같은 Client 권한·시각·Kernel과 진단을 연결한다.
    _initializeClientWorldStorage(game, files) {
        if (this._surfaceStorage || !this._clientGameplay || !this._gameClock) throw new Error('Client 월드 저장소의 중복 또는 공통 수명 누락');
        const assets = new CampaignSessionAssets(game, files), metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
        const gameplay = this._clientGameplay, clock = this._gameClock;
        const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
        const kernel = new Kernel(); storage.processes.bindDevices(kernel, clock.time());
        storage.bindChecking(() => gameplay.aiDebug().checking().value);
        storage.bindDisplayDiagnostics(gameplay.aiDebug().assertionDetail(), (expression, file, line) => console.error(`원본 표시 진단: ${file}:${line} ${expression}`));
        storage.bindGraphClock(() => clock.gameSeconds()); bindClientKernelDiagnostics(kernel, storage, (line) => console.info(line));
        this._gameAssets = game; this._surfaceStorage = storage; this._clientKernel = kernel;
    }

    // 실제 fortPal 자산을 적용해 논리 색을 캡처한 뒤 창 모드 DIB와 출력 장치를 준비한다.
    _initializeClientPalette(game) {
        if (this._clientPaletteScreen || this._clientColorSort || !this._layout || !game) throw new Error('Client 팔레트의 중복 또는 시작 수명 누락');
        const screen = new Screen(this._layout.logicalWidth, this._layout.logicalHeight, { devices: {
            paletteChanged: (palette) => this._applyClientScreenPalette(palette), present: (rect, view) => this._presentClientScreen(rect, view),
        } });
        const colors = new ClientColorSortInput();
        screen.init(); screen.loadPalette(game.palette, 1);
        colors.captureWithPrimary(screen.logicalPalette());
        screen.bindColorSortInput(() => colors.logical);
        screen.initDibSection();
        if (!screen.setMode(ScreenMode.fallbackWindowed)) throw new Error('Client Screen의 창 모드 DIB 초기화 실패');
        screen.setClip(0, 0, screen.width, screen.height);
        this._clientPaletteScreen = screen; this._clientColorSort = colors;
        this._initializeClientRenderer(screen.width, screen.height);
    }

    // 논리 화면 크기에 맞는 Renderer와 그 변경 표를 빌리는 월드 표시 장치를 함께 교체한다.
    _initializeClientRenderer(width, height) {
        const renderer = new Renderer(width, height);
        this._clientRenderer = renderer; this._clientSurfaceDisplay = new SquidRenderer(renderer);
    }

    // 현재 Screen의 BGRX 색을 실제 표시기의 RGB 표로 전달하고 다음 화면 출력을 예약한다.
    _applyClientScreenPalette(palette) {
        if (!(palette instanceof Uint8Array) || palette.length !== 1024) throw new TypeError('Client Screen 팔레트 BGRX 계약');
        const rgb = new Uint8Array(768);
        // 색 번호와 원본 채널 순서를 보존하며 표시기 형식으로 옮긴다.
        for (let index = 0; index < 256; index += 1) {
            rgb[index * 3] = palette[index * 4 + 2]; rgb[index * 3 + 1] = palette[index * 4 + 1]; rgb[index * 3 + 2] = palette[index * 4];
        }
        this._presenter?.setPalette(rgb); this._displayDirty = true;
    }

    // 현재 DIB의 잘린 사각형을 실제 색 번호 표면에 복사하고 보이는 Canvas에 즉시 출력한다.
    _presentClientScreen(rect, view) {
        const surface = this._surface, layout = this._layout;
        if (!surface || !layout || !this._presenter || surface.width !== view.width || surface.height !== view.height ||
            layout.logicalWidth !== view.width || layout.logicalHeight !== view.height || !view.pixels || view.pitch < view.width) throw new Error('Client Screen의 현재 DIB 출력 수명 누락');
        // DIB 패딩을 제외하고 지정된 영역의 각 행만 같은 색 번호로 복사한다.
        for (let row = rect.top; row < rect.bottom; row += 1) {
            const source = row * view.pitch + rect.left, target = row * surface.width + rect.left;
            surface.pixels.set(view.pixels.subarray(source, source + rect.right - rect.left), target);
        }
        this._displayDirty = true;
        if (layout.visible) {
            if (!this._ctx) throw new Error('Client Screen의 현재 Canvas 문맥 누락');
            this._presenter.present(this._ctx, surface, layout, true);
        }
    }

    // 최초 TYPE 등록 뒤 같은 primary로 소유자 표와 9색표 캐시를 만들고 Screen 명암 조회에 연결한다.
    _initializeClientPaletteCache(game) {
        const screen = this._clientPaletteScreen, colors = this._clientColorSort, clouds = this._clientKencloud;
        if (!screen || !colors || !clouds || !game || colors.mapsReady) throw new Error('Client 색표 캐시의 중복 또는 시작 수명 누락');
        const bridge = game.typeNumber(game.find('bridge'));
        colors.initializeCache(this._clientColorMapFiles, 'cache/!color.dat', screen.colors().basic[1], () => {
            colors.ownerTables.initialize(colors.primary, PATCH_TYPE_COUNT, bridge, clouds.worldRedraw());
        });
        screen.bindColorMaps(() => ({ bright: colors.maps[4], dark: colors.maps[6] }));
    }

    // 논리 크기가 바뀌면 현재 색과 loaded 상자·고정 색표를 유지한 새 창 모드 Screen을 만든다.
    _resizeClientPaletteScreen(width, height) {
        const previous = this._clientPaletteScreen, colors = this._clientColorSort;
        if (!previous || !colors?.mapsReady) throw new Error('Client Screen 크기 변경의 현재 색표 수명 누락');
        if (previous.width === width && previous.height === height) return;
        const palette = GamePalette.parse(previous.palette().slice()), mode = previous.flags;
        const screen = new Screen(width, height, { paletteLoaded: previous.paletteLoaded, devices: {
            paletteChanged: (value) => this._applyClientScreenPalette(value), present: (rect, view) => this._presentClientScreen(rect, view),
        } });
        screen.init(); screen.initDibSection();
        screen.bindColorSortInput(() => colors.logical); screen.bindColorMaps(() => ({ bright: colors.maps[4], dark: colors.maps[6] }));
        screen.loadPalette(palette, 0);
        if (!screen.setMode(mode)) throw new Error('Client Screen 크기 변경의 창 모드 초기화 실패');
        screen.setClip(0, 0, width, height); this._clientPaletteScreen = screen;
        this._initializeClientRenderer(width, height);
    }

    // 같은 Client의 구름·파일·Screen·최초 색표를 사용해 원본 팔레트 전환 부모를 실행한다.
    _changeClientFortPalette(name) {
        const data = this._gameData, cloud = this._clientKencloud, random = this._clientRandom, colors = this._clientColorSort;
        if (typeof name !== 'string' || !data || !cloud || !random || !this._clientPaletteScreen || !colors?.mapsReady) throw new Error('Client 팔레트 전환의 실제 수명 누락');
        const encoded = encodeWindows1252(name), argument = new Uint8Array(encoded.length + 1); argument.set(encoded);
        cloud.transition(argument, data.files, data, random.game, {
            log: (level, message) => console.info(`Palette ${level} ${message}`),
            // 현재 설정 경로를 실제 VFS에서 읽고 Screen의 지우기 관문을 통과한다.
            loadPalette: (path, clear) => {
                const end = path?.indexOf(0) ?? -1; if (end < 0) throw new RangeError('팔레트 파일 경로 NUL 누락');
                const name = decodeWindows1252(path.subarray(0, end)), bytes = data.files.read(name);
                if (!bytes) throw new Error(`팔레트 파일 누락: ${name}`);
                this._clientPaletteScreen.loadPalette(GamePalette.parse(bytes), clear);
            },
            // 같은 최초 색으로 9표를 다시 만들고 현재 World 표시기에도 실제 Screen 색을 전한다.
            buildMaps: () => {
                const screen = this._clientPaletteScreen; colors.rebuild(screen.colors().basic[1]);
                this._campaignWorld?.setPalette(screen.palette(), colors.logical, colors.maps);
                if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true;
            },
        });
    }

    // 같은 Client 설정·Player·월드 저장소에 Guide 수명을 연결한다.
    _initializeClientGuide() {
        if (this._clientGuide || !this._gameData || !this._surfaceStorage || !this._clientGameplay || !this._clientPlayers) throw new Error('Client Guide의 중복 또는 공통 수명 누락');
        this._clientGuide = new ClientCampaignGuide(this._gameData, this._clientGameplay, this._clientPlayers, this._surfaceStorage);
        // 선택된 이름 효과만 현재 World의 같은 Session에서 빌린다. World가 없으면 해당 효과를 요구한 위치에서 중단한다.
        const fortSession = () => { if (!this._campaignWorld) throw new Error('Fort 현재 이름의 실제 Session 미설치'); return this._campaignWorld.session().bindFortCurrentName(); };
        this._clientFortName = new ClientFortCurrentName(this._clientFort, this._clientTemplates, this._gameData, this._clientGameplay, this._surfaceStorage,
            { refreshTitle: () => this._refreshFortWindowTitle(), assertion: (line) => console.error(`Fort 현재 이름 원본 진단: ${line}`),
                snapshot: () => fortSession().snapshot(), notify: (slot) => fortSession().notify(slot), state: (slot) => fortSession().state(slot) });
        this._clientGuide.bindTutorialInit({ templates: this._clientTemplates, local: this._clientLocal, subscriber: this._clientSubscriber, types: this._gameAssets.typeTable,
            selectFort: (name) => { this._selectCampaignFort(name); const bytes = this._clientFort.currentBytes(); return decodeWindows1252(bytes.subarray(0, bytes.indexOf(0))); } });
        this._clientGuide.bindLoop({ dialogsClear: () => !this._mainMenu?.dialog, gameSeconds: () => this._gameClock.gameSeconds(),
            worldConnecting: (token) => Boolean(this._campaignWorld?.connecting && token === this._campaignWorldMissionToken), tellGlobal: (name, mode) => { if (mode !== 1) throw new Error('Guide init Tell 인자 위반'); this._openTell(name); } });
    }

    // 현재 폴링과 같은 Client 입력 큐로 원본 포인터 안팎 보정을 실행한다.
    _prepareClientPointer(language) {
        return prepareGumpPointerBoundaryWithQueue(this._clientPointerBoundary, language, (code) => this._clientInputPoll.poll(code), this._clientInputQueue);
    }

    // 실제 Client 진단 DWORD와 현재 논리 화면으로 공통 Gump 루트를 만든다.
    _initializeClientGump() {
        if (this._clientGump || !this._layout || !this._clientGameplay) throw new Error('Client Gump의 중복 또는 공통 수명 누락');
        const gameplay = this._clientGameplay;
        this._clientGump = new ClientGumpRoot(this._layout.logicalWidth, this._layout.logicalHeight, () => gameplay.aiDebug().checking().value,
            () => { this._displayDirty = true; }, (text) => { gameplay.aiDebug().assertionDetail().value = text; });
    }

    // 현재 Fort·Guide·설정으로 원본 창 제목을 계산해 실제 문서에 적용한다.
    _refreshFortWindowTitle() {
        if (!this._root || !this._clientFort || !this._clientTemplates || !this._clientGameplay || !this._surfaceStorage || !this._gameData) throw new Error('Fort 제목의 실제 Client 수명 미설치');
        const bytes = this._clientFort.currentBytes(), end = bytes.indexOf(0);
        if (end < 0) throw new RangeError('Fort 제목의 현재 이름 NUL 범위');
        const fortName = decodeWindows1252(bytes.subarray(0, end)), selected = this._clientGuide?.current(), owner = this._clientGameplay.player.templateOwner;
        const headers = this._clientTemplates.headers(); if (owner >= 9) throw new RangeError('Fort 제목의 현재 Template 범위');
        const view = new DataView(headers.buffer, headers.byteOffset + owner * 20, 20);
        const state = { fortName, editor: this._clientGameplay.world.editor, mission: this._clientGameplay.world.missionPost,
            templatePresent: view.getUint32(0, true) !== 0, templateReadOnly: view.getUint32(8, true) !== 0,
            demo: selected?.runtime.gate.demo ?? 0, testBattle: selected?.runtime.mode64 ?? 0 };
        refreshWindowTitle(state, { pathSpec: (key, argument) => this._gameData.pathSpec(key, argument), missionTitle: () => selected?.script.get('title') ?? '', setTitle: (title) => { this._root.ownerDocument.title = title; } });
    }

    // 같은 Client의 현재 Fort 이름 선택 부모를 실행한다. 실제 Guide Init이 호출할 경계다.
    _selectCampaignFort(name) { if (!this._clientFortName) throw new Error('Client Fort 이름 장치 미설치'); this._clientFortName.set(name); }

    // 실제 앱 설정과 게임 자산에서 한 미션의 스크립트·Fort를 준비한다. 반환 소유자는 호출자가 반납한다.
    _prepareCampaignMission(name) {
        if (!this._gameData || !this._gameAssets || !this._surfaceStorage) throw new Error('미션 준비 전 실제 Client 자산 미설치');
        return new ClientMissionAssets(this._gameData, this._gameAssets, name);
    }

    // 미션 조립은 현재 Client의 난수·SP·Player·Core·Template·Subscriber와 시계를 대여한다. 이 호출은 Session을 생성하지 않는다.
    _campaignSharedServices() {
        const clock = this._gameClock, random = this._clientRandom, sp = this._clientSp, local = this._clientLocal;
        const subscriber = this._clientSubscriber, gameplay = this._clientGameplay, coreData = this._clientCoreData, players = this._clientPlayers, templates = this._clientTemplates;
        if (!clock || !random || !sp || !local || !subscriber || !gameplay || !coreData || !players || !templates || !this._clientSoundNames) throw new Error('캠페인의 실제 Client 공통 수명 미설치');
        const surface = this._surfaceStorage, kernel = this._clientKernel, request = this._clientGuide?.fortRequest();
        const colors = this._clientColorSort, screen = this._clientPaletteScreen;
        if (Boolean(colors) !== Boolean(screen) || (colors && !colors.mapsReady)) throw new Error('캠페인의 실제 Client 색표 수명 미설치');
        return { soundNames: this._clientSoundNames, randomGlobals: random, spGlobals: sp, localStateGlobals: local, subscriberGlobals: subscriber, gameplayGlobals: gameplay, coreDataGlobals: coreData,
            playerRecords: players, templateStorage: templates, connectionActivityGlobals: this._clientConnectionActivity, connectionRegistry: this._clientConnections, connectionFrameNumber: () => clock.time().number, ...(surface && kernel ? { surfaceStorage: surface, processKernel: kernel } : {}),
            ...(colors ? { colorSortInput: colors, ownerColorTables: colors.ownerTables, screenPalette: () => this._clientPaletteScreen.palette() } : {}),
            ...(this._clientGuide ? { missionRestrictions: this._clientGuide.restrictions(), missionOptions: gameplay.battleOptionsOwner() } : {}), ...(request ? { missionFortRequest: request } : {}),
            unixSeconds: () => clock.unixSeconds(), frameTime: () => clock.time(), paused: () => clock.paused() };
    }

    // State7 부모가 실제 앱의 장기 전역과 같은 프레임 시계를 빌리도록 한다.
    _campaignStateClient() {
        if (!this._surfaceStorage || !this._clientLocal || !this._clientGameplay || !this._clientKencloud || !this._clientHtml || !this._gameClock) throw new Error('캠페인 State7의 실제 Client 공통 수명 미설치');
        return { edition: () => 'patch1078', surfaceStorage: () => this._surfaceStorage, localStateGlobals: () => this._clientLocal,
            gameplayGlobals: () => this._clientGameplay, kencloudGlobals: () => this._clientKencloud, htmlWindowGlobals: () => this._clientHtml,
            time: () => this._gameClock.time(), wallSeconds: () => this._gameClock.wallSeconds() };
    }

    // 월드 설치가 실제 번역·설정·현재 창 제목을 사용하도록 표시 바이트 경계를 연결한다.
    _campaignInstallServices() {
        return {
            translateYou: () => {
                if (!this._resources || !this._gameData) throw new Error('미션 설치의 실제 번역 자원 누락');
                const encoded = displayTextCodec.encode(this._resources.translate('You'));
                return Uint8Array.from([...encoded, 0]);
            },
            setConfig: (key, bytes) => {
                if (!this._gameData || !(bytes instanceof Uint8Array)) throw new TypeError('미션 설정의 실제 저장소 또는 바이트 누락');
                const end = bytes.indexOf(0); if (end < 0) throw new RangeError('미션 설정 문자열 NUL 누락');
                this._gameData.set(key, displayTextCodec.decode(bytes.subarray(0, end)));
            },
            refreshWindowTitle: () => this._refreshFortWindowTitle(),
        };
    }

    // 같은 앱의 현재 설정·Gump·입력 큐를 미션 소유권과 자산 후처리에 빌려준다.
    _campaignGameplayServices() {
        const services = {};
        bindCampaignOwnershipTheme({ configuration: () => {
            if (!this._gameData) throw new Error('미션 소유권의 실제 설정 수명 누락');
            return this._gameData;
        } }, services);
        bindCampaignProductionRefresh({ route: (...args) => {
            if (!this._clientGump) throw new Error('미션 생산 갱신의 실제 Gump 수명 누락');
            return this._clientGump.input().route(...args);
        } }, services);
        bindCampaignClientInputServices({ input: () => {
            if (!this._clientInputQueue) throw new Error('미션 명령의 실제 입력 큐 수명 누락');
            return this._clientInputQueue;
        } }, services);
        services.assets.queueCommand = services.developerQueue;
        // AI 이름·난이도와 채팅은 같은 Client 설정·번역·Subscriber 기록을 읽는다.
        services.ai = { assertion: (expression) => console.error(`미션 AI 원본 진단: ${expression}`) };
        bindCampaignAiClientServices({ configuration: () => this._gameData, languageNumber: () => this.options.language === 'korean' ? 7 : 0,
            translate: (text) => this._resources.translate(text), encodeDisplayText: (text) => displayTextCodec.encode(text),
            decodeDisplayText: (bytes) => displayTextCodec.decode(bytes), chatChannels: () => this._clientChatChannels }, services.ai);
        // 미션 효과음은 같은 Client 이름표와 현재 재생기에 전달한다.
        const player = () => { if (!this._clientSoundPlayer) throw new Error('미션 효과음의 실제 재생기 미준비'); return this._clientSoundPlayer; };
        services.sound = { wallSeconds: () => this._gameClock.wallSeconds(), play: (...args) => player().play(...args),
            playOnceAt: (...args) => player().playOnceAt(...args), playLoopAt: (...args) => player().playLoopAt(...args),
            stop: (...args) => player().stop(...args), isPlaying: (...args) => player().isPlaying(...args) };
        services.assets.postPop.playImpactSound = (sound) => player().play(sound, 0, 0, 0, 0, 0);
        // Session 초기 카메라 요청을 같은 앱의 실제 화면·변경 표·Gump에 적용한다.
        services.cameraCommit = (context) => this._commitCampaignCamera(context);
        // 카메라 패킷은 도킹 영역이 아닌 현재 논리 Screen의 전체 크기를 읽는다.
        services.cameraScreenSize = () => {
            const screen = this._clientPaletteScreen;
            if (!screen) throw new Error('미션 카메라의 실제 Screen 수명 누락');
            return { x: screen.width, y: screen.height };
        };
        // 표면 변경은 크기 변경으로 교체된 현재 Renderer에도 같은 어댑터로 전달한다.
        services.surfaceDisplay = {
            suppressed: () => {
                if (!this._clientSurfaceDisplay) throw new Error('미션 표면의 실제 Renderer 수명 누락');
                return this._clientSurfaceDisplay.suppressed();
            },
            invalidate: (rect, flags) => {
                if (!this._clientSurfaceDisplay) throw new Error('미션 표면의 실제 Renderer 수명 누락');
                this._clientSurfaceDisplay.invalidate(rect, flags); this._displayDirty = true;
            },
        };
        return services;
    }

    // 원본 497960 카메라 적용 장치를 현재 앱 Screen·Renderer·World에 연결한다.
    _commitCampaignCamera(context) {
        const gameplay = this._clientGameplay, screen = this._clientPaletteScreen, renderer = this._clientRenderer, gump = this._clientGump;
        if (!gameplay || !screen || !renderer || !gump || !this._clientKencloud || !this._clientLocal || !this._gameClock || context?.camera !== gameplay.camera()) throw new Error('미션 카메라 적용의 실제 Client 수명 불일치');
        const game = { camera: () => gameplay.camera(), cursor: () => gameplay.cursor, selection: () => gameplay.selection,
            patchDiagnostic: () => gameplay.patchDiagnostic(), worldCells: () => gameplay.worldCells() };
        const client = { configuration: () => this._gameData, screen: () => this._clientPaletteScreen, renderer: () => this._clientRenderer,
            gameplay: () => game, localState: () => this._clientLocal, time: () => this._gameClock.time(),
            active: () => this._root?.ownerDocument?.hasFocus?.() ?? true, poll: (code) => this._clientInputPoll.poll(code),
            cursor: () => { throw new Error('미션 소프트웨어 커서 장치 미연결'); } };
        const app = this, rebuild = { get value() { return app._displayDirty; }, set value(value) { app._displayDirty = Boolean(value); } };
        const buttons = { redrawRoots: () => this._clientGump.redrawRoots() };
        const camera = new UberGumpCamera({ client, buttons: () => buttons, world: () => this._campaignWorld, gameMenuState: () => null,
            pointerBoundary: this._clientPointerBoundary, worldRedraw: this._clientKencloud.worldRedraw(), rebuild });
        camera.commitCameraNow();
    }

    // 현재 논리 화면과 Client 전역으로 카메라 Renderer·snapshot·한계를 원본 순서로 초기화한다.
    _initializeClientRendererView() {
        const layout = this._layout, game = this._clientGameplay, clouds = this._clientKencloud;
        if (!layout || !game || !clouds) throw new Error('Renderer 초기화의 현재 화면 또는 Client 수명 누락');
        if (!Number.isInteger(layout.logicalWidth) || !Number.isInteger(layout.logicalHeight)) throw new TypeError('Renderer 초기화의 논리 화면 정수 크기 누락');
        game.camera().initializeNativeRenderer(layout.logicalWidth, layout.logicalHeight, game.worldCells().value, clouds.worldRedraw(), game.mapRevision());
    }

    // 로컬 Login이 현재 Client의 설정·시계·진단과 표시 무효화를 직접 조회한다.
    _campaignLoginServices() {
        return {
            // State7은 실제 화면 크기와 공유 카메라 전역을 다시 초기화한다.
            initializeRenderer: () => this._initializeClientRendererView(),
            // State7에서 현재 battlePal을 조회해 같은 Client 팔레트 전환을 실행한다.
            battlePalette: () => this._changeClientFortPalette(this._gameData.pathSpec('battlePal')),
            // State7·13의 현재 HUD 모드를 같은 Client Gump 입력 트리에 숨김 포함으로 전달한다.
            uiMode: (mode) => {
                if (!this._clientGump || !this._clientLocal) throw new Error('로그인 HUD 모드의 실제 Client Gump 수명 누락');
                this._clientGump.input().route(this._clientLocal.hudModeCommand().value, mode, 0, true);
            },
            // 미션 시작과 로그인 진행 문구는 같은 현재 메뉴의 진행 창을 사용한다.
            startingProgress: () => { if (!this._resources) throw new Error('미션 진행 창의 실제 자원 수명 누락'); this._showMissionLoading(this._resources.translate('Starting Mission...')); },
            progress: (kind, remaining) => { if (!this._resources) throw new Error('미션 진행 창의 실제 자원 수명 누락'); this._showMissionLoading(missionLoginProgressText(kind, remaining, (text) => this._resources.translate(text))); },
            closeProgress: () => this._closeMissionLoading(),
            // 서버 포트는 공통 전역을, 다른 정수는 현재 설정과 기본값을 사용한다.
            configInt: (key, fallback) => {
                if (!this._gameData || !this._clientGameplay) throw new Error('로그인 설정의 실제 Client 수명 누락');
                return key === 'gameServerPort' ? this._clientGameplay.gameServerPort().value | 0 : this._gameData.readInt(key, fallback).value;
            },
            // 프레임 고정값과 달리 요청 순간의 벽시각과 게임 시각을 각각 읽는다.
            wallSeconds: () => this._gameClock.wallSeconds(),
            gameSeconds: () => this._gameClock.gameSeconds(),
            // 브라우저에는 C 런타임 clock()이 없어 같은 Client의 단조 밀리초를 사용한다.
            clock: () => this._gameClock.milliseconds(),
            // 현재 메뉴와 최종 화면이 다시 그려지도록 표시한다.
            refresh: () => { if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true; },
            // 비전투·미션·대본 중단을 먼저 제외한 뒤 현재 Session의 전투 종료를 확인한다.
            playerUiBusy: () => {
                if (!this._surfaceStorage || !this._clientGameplay || !this._clientLocal) throw new Error('로그인 Player의 실제 Client 수명 누락');
                if (!this._surfaceStorage.ownerMode.battle || this._clientGameplay.world.missionPost || this._clientLocal.scriptBreak().value) return false;
                if (!this._campaignWorld) throw new Error('전투 종료 판정의 실제 World 누락');
                return this._campaignWorld.session().battleFinished();
            },
            // Player 원문 진단을 출력하고 같은 표시 화면을 무효화한다.
            playerNotice: (kind, owner, value) => { console.info(`Player UI ${kind} ${owner} ${value}`); if (this._mainMenu) this._mainMenu.dirty = true; this._displayDirty = true; },
            // 원본 연결 진단을 브라우저 콘솔에 기록한다.
            log: (text) => console.info(text),
        };
    }

    // 현재 메뉴에 원본 본문·Cancel 메타를 가진 진행 창을 표시한다.
    _showMissionLoading(message) {
        const menu = this._mainMenu, resources = this._resources, data = this._gameData;
        if (!menu || !resources || !data) throw new Error('미션 진행 창의 실제 메뉴·자원 수명 누락');
        const next = createMissionLoadingWindow({ message, cancelText: resources.translate('Cancel'), resources, gameData: data,
            screen: { width: menu.width, height: menu.height }, nowSeconds: this._frameSeconds, now: () => this._frameSeconds,
            onCommand: (command) => this._onDialogCommand(command), onSound: () => this._audio?.playSound(BUTTON_SOUND_NAME) });
        this._missionLoadingWindow?.close(); this._missionLoadingWindow = next; menu.setDialog(next); this._displayDirty = true;
    }

    // 다른 대화상자가 열린 경우에는 그 창을 건드리지 않고 진행 창 수명만 반납한다.
    _closeMissionLoading() {
        const window = this._missionLoadingWindow; if (!window) return;
        window.close(); this._missionLoadingWindow = null;
        if (this._mainMenu?.dialog === window) { this._mainMenu.setDialog(null); this._displayDirty = true; }
    }

    // 같은 실제 Client의 State7 부모에서 선택된 Fort World를 설치하고 State8 내부 접속을 건다.
    _runCampaignState7(services, viewport) {
        const client = this._campaignStateClient(), request = this._clientGuide?.fortRequest();
        if (!request || this._campaignWorld) throw new Error('캠페인 State7의 현재 요청 또는 World 수명 오류');
        runCampaignMissionStarting(client, () => {
            captureCampaignMissionStarting(client, request);
            const world = this._installCampaignWorld(services, viewport); world.prepareLocalConnectionForState();
        }, (address, port) => {
            if (!this._campaignWorld) throw new Error('캠페인 State8의 현재 World 없음');
            this._campaignWorld.session().connectLocalConnectionForState(address, port);
        });
    }

    // 현재 Guide의 선택된 Fort와 같은 Client 공통 소유자로 GameWorld를 설치한다. State7·로그인은 호출자의 책임이다.
    _installCampaignWorld(services, viewport) {
        const current = this._clientGuide?.current(), request = this._clientGuide?.fortRequest();
        if (this._campaignWorld || !current || !request || request.consumed || !this._gameAssets || !this._gameData) throw new Error('캠페인 World의 현재 Guide/요청 또는 단독 수명 계약 오류');
        const fort = parseFort(current.script.readFortImage(request.fort), createFortTypes(this._gameAssets.typeTable));
        const install = { ...this._campaignInstallServices(), ...services?.install };
        const localLogin = services?.localLogin === null ? null : { ...this._campaignLoginServices(), ...services?.localLogin };
        const gameplay = this._campaignGameplayServices();
        const ownership = { ...gameplay.ownership, ...services?.ownership };
        const assets = { ...gameplay.assets, ...services?.assets, postPop: { ...gameplay.assets.postPop, ...services?.assets?.postPop } };
        const ai = { ...gameplay.ai, ...services?.ai, names: { ...gameplay.ai.names, ...services?.ai?.names }, participation: { ...gameplay.ai.participation, ...services?.ai?.participation } };
        const world = new CampaignGameWorld(this._gameAssets, this._gameData.files, fort, request.fort, { ...gameplay, ...services, ...this._campaignSharedServices(), install, localLogin, ownership, assets, ai }, viewport);
        this._campaignWorld = world; this._campaignWorldMissionToken = current.runtime.saveToken;
        return world;
    }

    // 현재 World의 Session을 반납하고 같은 미션 연결 대기 관문을 해제한다.
    _releaseCampaignWorld() {
        if (!this._campaignWorld) return;
        this._campaignWorld.dispose(); this._campaignWorld = null; this._campaignWorldMissionToken = 0;
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
        this._closeMissionLoading();
        this._mainMenu = null;
        this._terrainPreview = null;
        this._settings = null;
        this._progress = null;
        // Guide를 먼저 반납하고 장기 Process 소유자는 TYPE·Kernel과 시계가 살아 있는 동안 반납한다.
        this._clientGuide?.dispose(); this._clientGuide = null; this._clientFortName = null; this._clientFort = null;
        this._releaseCampaignWorld();
        this._clientGump?.dispose(); this._clientGump = null;
        this._clientConnections?.shutdownDevices();
        this._clientKencloud = null; this._clientHtml = null; this._clientConnections = null; this._clientConnectionActivity = null;
        this._surfaceStorage?.dispose();
        this._clientSoundDevice?.dispose(); this._clientSoundDevice = null; this._clientSoundPlayer = null; this._clientSoundState = null; this._clientSoundPrepare = null;
        this._gameClock = null; this._clientLocal = null; this._hiddenGamePause = false; this._clientSoundNames = null;
        this._clientSp = null; this._clientRandom = null;
        this._clientChatRecords = null; this._clientChatChannels = null;
        this._clientPaletteScreen = null; this._clientColorSort = null; this._clientRenderer = null; this._clientSurfaceDisplay = null;
        this._clientTemplates = null; this._clientPlayers = null; this._clientCoreData = null; this._clientGameplay = null; this._clientSubscriber = null;
        this._clientKernel = null; this._surfaceStorage = null; this._gameAssets = null;
        this._missionAudioAssets?.dispose(); this._missionAudioAssets = null;
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
        this._clientImeElement?.remove(); this._clientImeElement = null;
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
                // 한국어면 메뉴 묶음 뒤에 한국어 묶음(글꼴·번역표)을 받으므로 진행률의 전체 크기에 미리 더해 둔다. (2026-10-31 WB03-N)
                const koreanTotal = this.options.language === 'korean' ? this._bundleBytes(loader, KOREAN_BUNDLE) : 0;
                let menuTotal = 0;
                const files = await loader.loadBundle(MENU_BUNDLE, (loaded, total) => {
                    menuTotal = total;
                    if (generation === this._loadGeneration) this._loadProgress = { loaded, total: total + koreanTotal };
                });
                if (generation !== this._loadGeneration) return;
                const korean = this.options.language === 'korean' ? await this._loadKorean(loader, menuTotal, koreanTotal, generation) : null;
                if (generation !== this._loadGeneration) return;
                this._resources = buildMenuResources(files, korean);
                // 설정 층에는 웹 사용자 설정(언어·음량)과 저장된 캠페인 진행 값을 원본 options.cfg 자리에 넣는다. 해상도·전체화면 값은 넣지 않는다.
                // 이후 게임 데이터에 쓰이는 값은 _onConfigWrite 가 설정·진행 저장소로 옮긴다. (2026-10-30 WB03-H: 진행 값과 쓰기 통지 연결)
                // 한국어 언어 자료(tell·offical*·config 용어표)는 같은 데이터 폴더의 느슨한 파일로 합쳐 넘긴다. 한국어 묶음을 받지 못했으면(korean 이 null) 영어 파일로 대체된다. (2026-10-31 WB03-O)
                this._gameData = GameData.build({
                    files: korean?.languageFiles?.size ? new Map([...files, ...korean.languageFiles]) : files,
                    language: this.options.language,
                    options: { ...this._settings.snapshot(), ...this._progress.snapshot() },
                    onChange: (key, value) => this._onConfigWrite(key, value),
                });
                this._assetLoader = loader;
                this._buttonSoundBytes = files.get(MENU_ASSET_PATHS.buttonSound) ?? null;
                this._menuSoundBytes = files.get(MENU_SOUND_PATH) ?? null;
                this._presenter?.setPalette(this._resources.palette.rgb);
                this._applyCursor();
                this._startMusicDownload(loader);
            }
            // 같은 자산이 재사용되는 재시작에서도 Client 장기 물리는 새로 구성한다.
            if (!this._surfaceStorage) {
                const palettePath = this._gameData.pathSpec('GamePalSpec', this._gameData.pathSpec('fortPal'));
                this._initializeClientWorldStorage(loadClientWorldAssets(this._gameData.files, palettePath), this._gameData.files);
            }
            this._syncClientCloudOptions();
            if (!this._clientPaletteScreen) this._initializeClientPalette(this._gameAssets);
            if (!this._clientColorSort.mapsReady) this._initializeClientPaletteCache(this._gameAssets);
            if (!this._clientGump) this._initializeClientGump();
            if (!this._clientGuide) this._initializeClientGuide();
            await this.options.prepare(this);
        } catch (error) {
            if (generation !== this._loadGeneration) return;
            reportGameError('asset-loading', error, { state: this._state, generation });
            this._loadError = error;
            this._setState(AppState.ERROR);
            return;
        }
        if (generation !== this._loadGeneration) return;
        this._setState(AppState.CLICK_TO_START);
    }

    // 매니페스트에서 묶음에 속한 파일의 전체 크기(바이트)를 구한다. 묶음이 없으면 0 이다. (2026-10-31 WB03-N)
    _bundleBytes(loader, bundle) {
        return loader.entries(bundle).reduce((sum, entry) => sum + entry.size, 0);
    }

    // 현재 적재 세대에서 한국어 글꼴·번역을 준비하고, 실패하면 진단을 남기고 영어 자료로 대체한다.
    // 이전 세대의 응답과 진행률은 새 앱 수명에서 무시한다. (2026-11-19 WB04-E944)
    async _loadKorean(loader, menuTotal, koreanTotal, generation = this._loadGeneration) {
        try {
            const files = await loader.loadBundle(KOREAN_BUNDLE, (loaded) => {
                if (generation === this._loadGeneration) this._loadProgress = { loaded: menuTotal + loaded, total: menuTotal + koreanTotal };
            });
            if (generation !== this._loadGeneration) return null;
            const document = this._root.ownerDocument;
            return await buildKoreanSupport(files, { createRasterizer: ({ regular, bold }) => createCanvasGlyphRasterizer({ document, regular, bold }) });
        } catch (error) {
            if (generation === this._loadGeneration) {
                reportGameError('korean-assets', error, { state: this._state, generation },
                    'Netstorm: 한국어 글꼴·번역표를 준비하지 못해 영어 메뉴로 계속합니다.');
            }
            return null;
        }
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
                if (this._root) reportGameError('audio-download', error, { state: this._state, asset: MENU_ASSET_PATHS.music });
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
            // 개발용 맵도 클릭/오디오 활성화 관문 뒤에만 연다. 없는 맵은 메뉴를 유지하며 원인을 알린다. (2026-11-02 WB03-AA)
            if (this.options.previewMap) {
                try { this.previewMap(this.options.previewMap); }
                catch (error) { console.warn('Netstorm: 지형 미리보기를 열지 못했습니다.', error); }
            }
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
                // 원본 비교 모드는 원본 8버튼 배치를 쓴다. (2026-11-07 WB02-H)
                includeAllButtons: this.options.originalComparison,
            });
        }
        this._ensureSurface();
        this._setState(AppState.MAIN_MENU);
        this._root.ownerDocument.title = this._resources.translate(MAIN_MENU_WINDOW_TITLE);
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
        // 원본 비교 모드는 웹이 빼는 원본 항목도 만든다. Technical Help 는 원본 가용성 판정대로 비활성이다(웹에는 기술 도움말 프로그램이 없음). (2026-11-07 WB02-H)
        const items = buildPopupItems(target, this._settings, { translate: this._resources.translate, includeAll: this.options.originalComparison, technicalHelpAvailable: false });
        if (!items) {
            console.warn(`Netstorm: 알 수 없는 펼침 메뉴 '${target}' 입니다.`);
            return false;
        }
        const popup = new PopupMenu({
            resources: this._resources,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            onCommand: (command) => this._onPopupCommand(command),
            onSound: () => this._audio?.playSound(MENU_SOUND_NAME),
            // 관측 보고용 대상 이름이다. (2026-11-07 WB02-H)
            name: target,
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
        } else if (name === 'resetall') {
            this._resetAll();
        } else {
            console.info(`Netstorm: 펼침 메뉴 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
        }
        this._displayDirty = true;
    }

    // 확인을 받은 뒤 게임 소유 저장 항목을 지우고 현재 앱을 정리한 다음 페이지를 다시 불러온다. (2026-11-19 WB04-E863)
    _resetAll() {
        const view = this._view();
        if (!view.confirm(this.texts.resetConfirm)) return;
        try {
            resetGameStorage(getLocalStorage(view));
        } catch (error) {
            reportGameError('reset-storage', error, { state: this._state });
            view.alert(this.texts.resetFailed);
            return;
        }
        this.destroy();
        view.location.reload();
    }

    // 설정 값이 바뀌었을 때 소리·음악·음량과 미션 효과음의 좌우 교환을 바로 적용한다.
    // 바뀐 값은 게임 데이터 설정 층에도 옮겨 적어 두 곳이 어긋나지 않게 한다(되돌아오는 통지는 막는다). (2026-10-30 WB03-H)
    _onSettingChange(key) {
        if (key === 'sound' || key === 'music' || key === 'soundVolume' || key === 'musicVolume' || key === 'swapLeftRightSpeakers') this._applySettings();
        const value = this._settings?.get(key);
        if (value !== undefined) this._gameData?.set(key, value, { silent: true });
        this._syncClientCloudOptions();
    }

    // 게임 데이터 설정 층에 쓴 값을 저장소로 옮긴다(원본에서는 메인 루프가 변경 표시를 보고 options.cfg 를 저장한다). 진행 값(`Done*`)은 진행 저장소에,
    // 설정 키(대소문자 무시)는 설정 저장소에 넣어 로컬 저장소에 바로 저장한다. 그 밖의 값(`global.` 실행 중 값 등)은 저장하지 않는다. (2026-10-30 WB03-H)
    _onConfigWrite(key, value) {
        this._syncClientCloudOptions();
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
        this._syncClientCloudOptions();
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
        this._syncCampaignSoundSettings();
        if (settings.get('music') === 1) this._playMenuMusic();
    }

    // Tell 대상으로 대화상자를 열어 메인 메뉴 위에 올린다(원본 Tell 명령 004cf730). 대상은 `섹션이름` 또는 `섹션이름(인자,…)` 이다(지역 인자는 본문의 `{param.1}` 이 읽는다).
    // `Blank` 는 창을 열지 않는 명령이다. 섹션이 없으면 원본처럼 오류 로그만 남기고 아무것도 열지 않는다. 본문이 영숫자 없는 빈 내용이어도 열지 않는다.
    // 본문의 스크립트 명령(`<$config,키=값>` 등)은 창이 올라간 뒤(창이 없어도) 순서대로 실행한다. 열린 대화상자의 명령은 _onDialogCommand 가 받는다. 열었으면 true 를 돌려준다.
    // 본문의 링크(`<a href>`)를 누르면 _onDialogLink 가 받는다. 본문이 화면보다 길면 스크롤바가 생긴다(시계는 _frameSeconds).
    // (2026-10-28 WB03-D, 2026-10-30 WB03-J: Blank·지역 인자·본문 스크립트 명령 추가, 2026-10-31 WB03-K: 링크·스크롤바 연결)
    _openTell(name) {
        if (!this._mainMenu || !this._gameData || !this._resources) return false;
        const result = createTellWindow({
            gameData: this._gameData,
            resources: this._resources,
            name,
            screen: { width: this._mainMenu.width, height: this._mainMenu.height },
            nowSeconds: this._frameSeconds,
            now: () => this._frameSeconds,
            onCommand: (command) => this._onDialogCommand(command),
            onLink: (destination) => this._onDialogLink(destination),
            onSound: (kind) => this._audio?.playSound(kind === 'menu' ? MENU_SOUND_NAME : BUTTON_SOUND_NAME),
            onUnsupported: (what, text) => console.warn(`Netstorm: Tell '${name}' 의 ${what} 은(는) 아직 지원하지 않습니다.`, text),
            // 원본 비교 모드는 웹이 빼는 목록 행(User Made Campaigns)도 만든다. (2026-11-07 WB02-H)
            excludedLists: this.options.originalComparison ? [] : undefined,
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
    // 도움말 파일(`Quick Help Spec`)이 없으면 원본처럼 오류 로그만 남기고 열지 않는다. 바깥 주소 링크는 `_openExternalAddress` 로 새 탭/창에 연다(2026-11-07 WB03-AD, 그 전에는 콘솔 안내만). 열었으면 true 를 돌려준다. (2026-10-30 WB03-G)
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
            onExternal: (address) => this._openExternalAddress(address, '도움말'),
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

    // 대화상자 본문의 링크(Tell 의 `<a href>`)를 처리한다. 원본 HtmlWindowGump::Handle 의 tagEvent 처럼 대화상자를 먼저 닫고(모달 해제) 링크를 분배한 뒤,
    // 닫힌 창의 $OnExit 명령을 실행한다(원본은 닫힌 창을 정리할 때 OnExit 를 큐에 넣으므로 링크 처리보다 뒤다). (2026-10-31 WB03-K)
    _onDialogLink(destination) {
        const dialog = this._mainMenu?.dialog ?? null;
        const onExit = dialog ? dialog.close() : null;
        this._mainMenu?.setDialog(null);
        this._displayDirty = true;
        this._followLink(destination);
        if (onExit) this._runDialogCommand(onExit);
    }

    // 링크 목적지를 원본 규칙(ui/help-link.js)으로 분배한다: `cmd:` 는 스크립트 명령으로 실행하고, 도움말 주제는 도움말 창을 열며(원본 mode 1, 학습 0),
    // 바깥 주소(`http:`·`mailto:` 등)는 `_openExternalAddress` 가 판정해 열 수 있는 것만 브라우저 새 탭/창으로 연다(2026-11-07 WB03-AD, 그 전에는 콘솔 안내만). 접두·본문 한도를 넘으면 알리고 아무것도 하지 않는다. (2026-10-31 WB03-K)
    _followLink(destination) {
        try {
            dispatchHelpLink(destination, {
                script: (body) => {
                    const command = parseScriptCommand(body);
                    this._runDialogCommand({ name: command.name, argument: command.argument });
                },
                help: (topic) => this._openHelp(topic),
                external: (original) => this._openExternalAddress(original, '링크'),
            });
        } catch (error) {
            if (!(error instanceof RangeError)) throw error;
            console.warn(`Netstorm: 링크 목적지가 원본 한도(접두 80, 본문 256)를 넘어 따라가지 않습니다: ${destination}`);
        }
    }

    // 링크·도움말의 바깥 주소 하나를 처리한다(2026-11-05 사용자 결정: 원본의 셸 실행 대신 브라우저 새 탭/창으로 연다).
    // ui/external-link.js 가 열 수 있다고 판정한 주소(치환이 끝난 http/https 와 mailto)만 열고, 그 밖(file:·미치환 자리표시·호스트 없는 주소·다른 접두)은
    // 이유와 함께 콘솔로 알리기만 한다. 팝업 차단을 피하도록 링크를 누른 포인터 입력 처리 안에서 동기로 부르며, 열기에 실패해도 게임 진행은 계속한다.
    // 열기 방식은 options.openExternal(link, document) 로 바꿀 수 있고(무브라우저 실행·검사용), 기본은 platform/external-opener.js 이다.
    // 열었으면 true, 열지 않았거나 실패했으면 false 를 돌려준다. where 는 콘솔 안내에 붙는 출처 이름이다. (2026-11-07 WB03-AD)
    _openExternalAddress(original, where) {
        const link = resolveExternalLink(original);
        if (!link.open) {
            console.info(`Netstorm: ${where}의 바깥 주소 '${original}' 는 열지 않습니다(${link.reason})`);
            return false;
        }
        const opened = (this.options.openExternal ?? openExternalLink)(this._root?.ownerDocument ?? null, link);
        if (!opened) console.warn(`Netstorm: ${where}의 바깥 주소 '${original}' 를 열지 못했습니다.`);
        return opened === true;
    }

    // 닫힌 뒤 실행할 대화상자 명령 하나를 처리한다. (2026-10-28 WB03-D)
    _runDialogCommand(command) {
        const name = String(command.name).toLowerCase();
        if (name === 'tell') this._openTell(command.argument);
        else if (name === 'config') this._runScriptCommand(command);
        else if (name === 'telltip') this._runTellTip(command);
        else if (name !== 'donothing') console.info(`Netstorm: 대화상자 명령 '${command.name}' 은(는) 아직 구현되지 않았습니다.`, command);
    }

    // 이미 받은 공개 자산으로 정적 지형을 연다. 플레이어/미션 상태·진행 저장·현재 미션 Config는 바꾸지 않는다.
    // 사용: 클릭하여 시작한 뒤 app.previewMap('savetheisland'). 실패 시 기존 화면을 유지하고 오류를 던진다. (2026-11-02 WB03-AA)
    previewMap(name) {
        if (![AppState.MAIN_MENU, AppState.TERRAIN_PREVIEW].includes(this._state) || !this._gameData || !this._resources) {
            throw new Error('지형 미리보기는 시작 입력 뒤에 열 수 있습니다.');
        }
        const scene = loadTerrainScene(this._gameData.files, name);
        const preview = new TerrainPreview(scene, this._resources, this.options.language);
        preview.resize(this._layout.logicalWidth, this._layout.logicalHeight);
        this._mainMenu?.cancel();
        this._terrainPreview = preview;
        this._setState(AppState.TERRAIN_PREVIEW);
        this._root.ownerDocument.title = `NetStorm - ${scene.name}`;
        this._displayDirty = true;
        return preview;
    }

    // 개발용 지형 화면에서 메뉴로 돌아온다. 시작 환영/팁 흐름을 다시 실행하지 않는다. (2026-11-02 WB03-AA)
    closeTerrainPreview() {
        if (!this._terrainPreview) return false;
        this._terrainPreview.cancel();
        this._terrainPreview = null;
        this._input.keys.clear();
        this._setState(AppState.MAIN_MENU);
        this._root.ownerDocument.title = this._resources.translate(MAIN_MENU_WINDOW_TITLE);
        if (this._mainMenu) this._mainMenu.dirty = true;
        this._displayDirty = true;
        return true;
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
        this._terrainPreview?.resize(width, height);
    }

    // 루트 요소가 속한 문서의 window 를 돌려준다. iframe 안에서도 그 문서의 프레임 예약·픽셀 비율·오디오를 쓰기 위해서다.
    // 루트가 없으면(정리 후) 전역 객체를 돌려준다.
    _view() {
        return this._root?.ownerDocument.defaultView ?? globalThis;
    }

    // ----- 오디오 -----

    // 원본 현재 소리 화면은 카메라 snapshot과 현재 미션 지도 영역을 사용한다.
    _campaignSoundView() {
        const screen = this._clientPaletteScreen, gameplay = this._clientGameplay;
        if (!screen || !gameplay) throw new Error('미션 음향의 현재 화면·카메라 수명 누락');
        const pixels = gameplay.camera().snapshot().pixels, clip = this._campaignWorld?.mapView() ?? { left: 0, top: 0, right: screen.width, bottom: screen.height };
        return { cameraX: pixels.x, cameraY: pixels.y, left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
    }

    // 같은 재생 전역에 현재 효과음 옵션과 전체 음량을 적용한다.
    _syncCampaignSoundSettings() {
        const state = this._clientSoundState, player = this._clientSoundPlayer, settings = this._settings;
        if (!state || !player || !settings) return;
        state.enabled = Boolean(state.device && settings.get('sound') === 1);
        state.swapSpeakers = settings.get('swapLeftRightSpeakers') === 1;
        state.maxPlaying = this._gameData?.readInt('maxSimulSounds', 8).value ?? 8;
        player.setMasterVolume(volumeStepToCentibels(settings.get('soundVolume')));
    }

    // 현재 Client에 실제 또는 비활성 효과음 장치를 한 번 준비하고 동시 요청은 공유한다.
    async _prepareCampaignAudio() {
        if (this._clientSoundPlayer) return this._clientSoundPlayer;
        if (this._clientSoundPrepare) return this._clientSoundPrepare;
        const audio = this._audio, names = this._clientSoundNames;
        if (!names || !this._gameClock || !this._settings) throw new Error('미션 효과음의 실제 Client 수명 누락');
        const task = (async () => {
            if (audio && this.options.audio) await this._prepareMissionSounds();
            if (this._audio !== audio || this._clientSoundNames !== names || !this._gameClock) throw new Error('미션 효과음 준비 중 Client 수명 변경');
            const state = createSoundState(); state.currentView = () => this._campaignSoundView();
            let device = null, hooks;
            if (audio && this.options.audio) {
                device = new WebSoundDevice(audio, this._missionAudioLoader()); hooks = device.hooks(); state.initialized = state.device = true;
            } else {
                // 오디오 미지원 또는 사용자가 오디오를 끈 경우 원본 noAudio 장치처럼 재생을 비활성화한다.
                const unavailable = () => { throw new Error('비활성 효과음 장치 호출'); };
                hooks = { status: unavailable, play: unavailable, setPosition: unavailable, setVolume: unavailable, setPan: unavailable, stop: unavailable,
                    duplicate: () => ({ result: -1, buffer: 0 }), load: () => ({ buffer: 0, attenuation: 0 }) };
            }
            const player = new SoundPlayer(names, state, hooks, (expression, line) => console.error(`소리 진단 ${line}: ${expression}`));
            this._clientSoundState = state; this._clientSoundDevice = device; this._clientSoundPlayer = player; this._syncCampaignSoundSettings();
            return player;
        })();
        this._clientSoundPrepare = task;
        try { return await task; } finally { if (this._clientSoundPrepare === task) this._clientSoundPrepare = null; }
    }

    // 현재 매니페스트와 활성 재생기를 빌려 필요한 미션 음향 하나만 준비한다.
    _missionAudioLoader() {
        if (!this._assetLoader || !this._audio || this._audio.closed) throw new Error('미션 음향의 실제 자산·재생기 수명 누락');
        if (!this._missionAudioAssets) this._missionAudioAssets = new MissionAudioAssets(this._assetLoader, this._audio, { language: this.options.language });
        return this._missionAudioAssets;
    }

    // 미션 효과음의 현재 원본 파일을 비동기로 받고 해독한다.
    _prepareMissionSound(name) { return this._missionAudioLoader().loadSound(name); }
    // 미션 시작 전 현재 언어의 효과음을 제한된 요청으로 준비한다.
    _prepareMissionSounds(options) { return this._missionAudioLoader().preloadSounds(options); }
    // 선택한 미션 음악 한 곡만 비동기로 받고 해독한다.
    _prepareMissionMusic(name) { return this._missionAudioLoader().loadMusic(name); }

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
                    .catch((error) => {
                        if (this._audioContext !== context) return;
                        this._audioStatus = 'failed';
                        reportGameError('audio-unlock', error, { state: this._state });
                    });
            }
        } catch (error) {
            this._audioStatus = 'failed';
            reportGameError('audio-unlock', error, { state: this._state });
        }
    }

    // 받은 버튼 효과음을 현재 재생기에 해독해 등록한다. 실패하면 소리 없이 계속한다. (2026-10-23 WB03-M1)
    _prepareSounds() {
        const audio = this._audio;
        if (!audio) return;
        if (this._buttonSoundBytes) {
            audio.loadSound(BUTTON_SOUND_NAME, this._buttonSoundBytes).catch((error) => {
                if (this._audio === audio) reportGameError('audio-decode', error, { state: this._state, asset: BUTTON_SOUND_NAME });
            });
        }
        if (this._menuSoundBytes) {
            audio.loadSound(MENU_SOUND_NAME, this._menuSoundBytes).catch((error) => {
                if (this._audio === audio) reportGameError('audio-decode', error, { state: this._state, asset: MENU_SOUND_NAME });
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
                reportGameError('audio-music', error, { state: this._state, asset: MENU_MUSIC_NAME });
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
        if (this._state === AppState.MAIN_MENU || this._state === AppState.TERRAIN_PREVIEW) this._ensureSurface();
        if (this._clientPaletteScreen) this._resizeClientPaletteScreen(next.logicalWidth, next.logicalHeight);
        this._clientGump?.resize(next.logicalWidth, next.logicalHeight);
        this._campaignWorld?.resize(next.logicalWidth, next.logicalHeight);
        this._applyCursor();
        if (next.visible) this._render(view.performance?.now?.() ?? 0);
    }

    // 포인터 아래에서 보일 원본 커서 번호를 고른다. 메인 메뉴에서 포인터가 게임 화면 안에 있으면 메뉴(열린 대화상자의 본문)가 고르고(링크 위 16, 스크롤 가능한 본문 위 15),
    // 그 밖은 기본 커서(1)이다. 고른 번호의 커서 자원이 묶음에 없으면 기본 커서로 대신한다. 원본은 매 포인터 갱신마다 이 선택을 다시 한다(NotifyPointer 의 Overlay). (2026-10-31 WB03-L)
    _wantedCursor() {
        const pointer = this._input.pointer;
        if (this._state !== AppState.MAIN_MENU || !this._mainMenu || !this._resources || !pointer.inside) return CURSOR_NORMAL;
        const number = this._mainMenu.cursorAt(pointer.x, pointer.y);
        return this._resources.cursors[number] ? number : CURSOR_NORMAL;
    }

    // 원본 커서를 현재 화면 배율에 맞춰 Canvas 에 적용한다. 어떤 번호의 커서를 쓸지는 _wantedCursor 가 고른다.
    // 자원이 없거나 번호·배율·픽셀 비율이 지난번과 같으면 아무것도 하지 않는다. 매 프레임(_onFrame: 포인터 이동·스크롤·창 열림/닫힘으로 포인터 아래 내용이 바뀜)과 화면 배치를 바꿀 때 부른다.
    // (2026-10-23 WB03-M1, 2026-10-31 WB03-L: 번호별 커서와 캐시 추가)
    _applyCursor() {
        const layout = this._layout;
        if (!this._canvas || !this._resources || !layout?.visible) return;
        const number = this._wantedCursor();
        const scale = Math.max(1, Math.round(layout.scale));
        const sizeKey = `${scale}:${layout.devicePixelRatio}`;
        const key = `${number}:${sizeKey}`;
        if (key === this._cursorKey) return;
        // 배율·픽셀 비율이 바뀌면 이전 배율로 만든 커서 그림은 버린다.
        if (sizeKey !== this._cursorSizeKey) {
            this._cursorCache.clear();
            this._cursorSizeKey = sizeKey;
        }
        this._cursorKey = key;
        this._cursorNumber = number;
        try {
            let css = this._cursorCache.get(number);
            if (!css) {
                css = buildCursorCss(this._root.ownerDocument, this._resources.cursors[number], layout.scale, layout.devicePixelRatio);
                this._cursorCache.set(number, css);
            }
            applyCursorStyle(this._canvas, css);
        } catch (error) {
            // 커서 그림을 만들 수 없는 환경이면 기본 커서를 그대로 쓴다.
            console.warn('Netstorm: 원본 커서를 적용하지 못했습니다.', error);
        }
    }

    // ----- 이벤트 -----

    // 대상에 리스너를 붙인다. destroy 의 abort 로 한꺼번에 해제되도록 signal 을 함께 넘긴다.
    _listen(target, type, handler, options = {}) {
        const app = this;
        // 입력 처리에서 생긴 예외에 이벤트 종류와 앱 상태를 더한 뒤 브라우저에 원래 예외를 전달한다. (2026-11-19 WB04-E862)
        function onEvent(event) {
            try {
                return handler.call(this, event);
            } catch (error) {
                reportGameError('event', error, { state: app._state, eventType: type });
                throw error;
            }
        }
        target.addEventListener(type, onEvent, { ...options, signal: this._abort.signal });
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
        // 확정 전 조합 키는 큐에 넣지 않고 조합 종료 본문만 원자적으로 예약한다.
        this._listen(canvas, 'compositionstart', () => this._onCompositionStart());
        this._listen(canvas, 'compositionend', (event) => this._onCompositionEnd(event));
        // 마우스 휠은 열린 도움말 창 본문을 스크롤한다(웹 적응). 도움말이 열려 있을 때만 페이지 스크롤을 막는다. (2026-10-30 WB03-G)
        this._listen(canvas, 'wheel', (event) => this._onWheel(event), { passive: false });
        this._listen(canvas, 'blur', (event) => { if (event.relatedTarget !== this._clientImeElement) this._clearInput(); });
        this._listen(doc, 'visibilitychange', () => this._onVisibilityChange());

        // 비동기 크기 관찰 오류는 현재 상태와 함께 보고한 뒤 브라우저에 전달한다. (2026-11-19 WB04-E944)
        if (typeof view?.ResizeObserver === 'function') {
            this._resizeObserver = new view.ResizeObserver(() => {
                try {
                    this._applyLayout();
                } catch (error) {
                    reportGameError('layout', error, { state: this._state, source: 'resize-observer' });
                    throw error;
                }
            });
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
        this._listen(
            query,
            'change',
            () => {
                this._applyLayout();
                this._watchPixelRatio();
            },
            { once: true },
        );
    }

    // 실제 문자 편집이 활성화될 때만 IME가 포커스할 수 있는 입력 요소를 만든다.
    _activateClientTextInput(x, y) {
        if (!this._root || !this._canvas || !this._layout?.visible || !this._clientNativeInputEnabled || !Number.isFinite(x) || !Number.isFinite(y)) throw new Error('Client IME 입력의 활성 화면·좌표 누락');
        let element = this._clientImeElement;
        if (!element) {
            element = this._root.ownerDocument.createElement('textarea'); this._clientImeElement = element;
            element.setAttribute('aria-label', this.options.language === 'korean' ? '문자 입력' : 'Text input'); element.setAttribute('autocomplete', 'off'); element.setAttribute('autocapitalize', 'off');
            element.spellcheck = false; element.tabIndex = -1;
            Object.assign(element.style, { position: 'fixed', width: '1px', height: '1px', opacity: '0', padding: '0', border: '0', resize: 'none', overflow: 'hidden', pointerEvents: 'none' });
            this._listen(element, 'keydown', (event) => this._onKeyDown(event)); this._listen(element, 'keyup', (event) => this._onKeyUp(event));
            this._listen(element, 'compositionstart', () => this._onCompositionStart());
            this._listen(element, 'compositionend', (event) => { this._onCompositionEnd(event); element.value = ''; });
            this._listen(element, 'beforeinput', (event) => this._onClientBeforeInput(event));
            this._listen(element, 'input', () => { if (!this._clientComposing) element.value = ''; });
            this._listen(element, 'blur', (event) => { if (event.relatedTarget !== this._canvas) this._clearInput(); });
            this._root.appendChild(element);
        }
        const rect = this._canvas.getBoundingClientRect();
        element.style.left = `${rect.left + x * rect.width / this._layout.logicalWidth}px`;
        element.style.top = `${rect.top + y * rect.height / this._layout.logicalHeight}px`;
        element.focus({ preventScroll: true }); return element;
    }

    // 편집 종료는 미완성 조합을 버리고 기존 Canvas 포커스로 돌아간다.
    _deactivateClientTextInput() {
        const element = this._clientImeElement; if (!element) return;
        this._clientComposing = false; this._clientImePending = null; element.value = ''; this._canvas?.focus({ preventScroll: true }); element.remove(); this._clientImeElement = null;
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
        this._clientInputPoll.recordMouseDown(event.button);
        if (this._clientNativeInputEnabled && this._layout?.visible) queueBrowserMouseEvent(this._clientInputQueue, event, this._input.pointer, false);
        this._retryAudioOnGesture();
        const pointer = this._input.pointer;
        if (this._state === AppState.MAIN_MENU && this._mainMenu && event.button === 0 && pointer.inside) {
            this._mainMenu.pointerDown(pointer.x, pointer.y);
        }
        if (this._state === AppState.TERRAIN_PREVIEW && event.button === 0 && pointer.inside) this._terrainPreview?.pointerDown(pointer.x, pointer.y);
    }

    // 포인터가 움직였을 때 좌표를 갱신하고, 메인 메뉴의 붙잡힌 버튼 모양을 갱신한다(원본 Broadcast). 포인터 아래 커서 모양은 다음 프레임의 _onFrame 이 고른다. (원본도 매 갱신마다 선택한다)
    _onPointerMove(event) {
        this._updatePointer(event);
        if (this._state === AppState.MAIN_MENU && this._mainMenu) this._mainMenu.pointerMove(this._input.pointer.x, this._input.pointer.y);
        if (this._state === AppState.TERRAIN_PREVIEW) this._terrainPreview?.pointerMove(this._input.pointer.x, this._input.pointer.y);
    }

    // 버튼을 뗐을 때 버튼 상태를 갱신하고, 메인 메뉴에서는 왼쪽 버튼 뗌을 돌 버튼에 전달한다(안쪽에서 떼면 실행).
    _onPointerUp(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        if (this._clientNativeInputEnabled && this._layout?.visible) queueBrowserMouseEvent(this._clientInputQueue, event, this._input.pointer, true);
        if (this._state === AppState.MAIN_MENU && this._mainMenu && event.button === 0) {
            this._mainMenu.pointerUp(this._input.pointer.x, this._input.pointer.y);
        }
        if (event.button === 0) this._terrainPreview?.cancel();
    }

    // 브라우저가 포인터 입력을 취소했을 때(터치 제스처 등) 버튼 상태를 비우고 메뉴 누름을 실행 없이 취소한다. (2026-10-23 WB03-M1)
    _onPointerCancel(event) {
        this._updatePointer(event);
        this._input.pointer.buttons = event.buttons ?? 0;
        this._mainMenu?.cancel();
        this._terrainPreview?.cancel();
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
        this._clientInputPoll.recordKeyDown(event.code, this._input.keys.has(event.code) || event.repeat);
        this._input.keys.add(event.code);
        if (this._clientNativeInputEnabled && !this._clientComposing) {
            const editing = event.target === this._clientImeElement && this._clientImeElement !== null;
            if (editing && !event.isComposing) this._clientImePending = null;
            queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, false);
            const queued = queueBrowserCharacterEvent(this._clientInputQueue, event, this._input.pointer);
            if (editing && queued && typeof event.key === 'string' && [...event.key].length === 1) this._markClientImePending(event.key);
        }
        // 조합 중 Enter 등은 기존 메뉴 명령으로도 중복 실행하지 않는다.
        if (this._clientNativeInputEnabled && (this._clientComposing || event.isComposing)) return;
        if (PREVENTED_KEYS.has(event.code)) event.preventDefault();
        this._retryAudioOnGesture();
        // 미리보기의 키 정책은 게임 명령과 분리한다. Escape는 메뉴로 돌아가며 방향키는 카메라만 바꾼다. (2026-11-02 WB03-AA)
        if (this._state === AppState.TERRAIN_PREVIEW) {
            if (event.code === 'Escape') { event.preventDefault(); this.closeTerrainPreview(); }
            else if (this._terrainPreview?.keyDown(event.code)) event.preventDefault();
            return;
        }
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

    // 마우스 휠을 열린 대화상자(도움말 창, 스크롤바가 있는 Tell 창)에 넘긴다. 처리했으면 페이지가 따라 스크롤되지 않게 막는다. (2026-10-30 WB03-G, 2026-10-31 WB03-K: Tell 창 추가)
    _onWheel(event) {
        const dialog = this._mainMenu?.dialog;
        if (this._state !== AppState.MAIN_MENU || !dialog || typeof dialog.wheel !== 'function') return;
        event.preventDefault();
        if (dialog.wheel(event.deltaY)) this._mainMenu.dirty = true;
    }

    // 키를 뗐을 때 눌린 키 집합에서 뺀다.
    _onKeyUp(event) {
        this._input.keys.delete(event.code);
        if (event.target === this._clientImeElement && this._clientImePending?.text === event.key) this._clientImePending = null;
        if (this._clientNativeInputEnabled && !this._clientComposing) queueBrowserVirtualKeyEvent(this._clientInputQueue, event, this._input.pointer, true);
    }

    // 원본 입력을 사용하는 동안 조합 키를 일반 문자·가상 키 경로에서 제외한다.
    _onCompositionStart() { if (this._clientNativeInputEnabled) { this._clientComposing = true; this._clientImePending = null; } }

    // 같은 입력의 후속 beforeinput만 한 이벤트 턴 동안 중복 검사한다.
    _markClientImePending(text) {
        const pending = { text }; this._clientImePending = pending;
        const view = this._root?.ownerDocument?.defaultView ?? globalThis;
        view.setTimeout(() => { if (this._clientImePending === pending) this._clientImePending = null; }, 0);
    }

    // 확정 본문을 같은 원본 큐에 원자적으로 넣고 미지원·공간 부족은 진단한다.
    _queueClientCommittedText(text) {
        try {
            const pointer = this._input.pointer;
            if (!queueCommittedText(this._clientInputQueue, text, pointer.x | 0, pointer.y | 0)) throw new Error('확정 문자 입력 큐 공간 부족');
            return true;
        } catch (error) { reportGameError('composition', error, { state: this._state }); return false; }
    }

    // 확정된 조합 본문만 큐에 넣고 같은 후속 입력을 중복하지 않는다.
    _onCompositionEnd(event) {
        if (!this._clientComposing) return;
        this._clientComposing = false;
        if (!this._clientNativeInputEnabled || !event?.data) return;
        if (this._queueClientCommittedText(event.data)) this._markClientImePending(event.data);
    }

    // 키다운 없이 삽입된 텍스트를 받고 이미 큐에 넣은 동일 입력은 건너뛴다.
    _onClientBeforeInput(event) {
        if (!this._clientNativeInputEnabled || this._clientComposing || event.isComposing || !event.data || !['insertText', 'insertReplacementText'].includes(event.inputType)) return;
        event.preventDefault();
        if (this._clientImePending?.text === event.data) { this._clientImePending = null; return; }
        this._clientImePending = null; this._queueClientCommittedText(event.data);
    }

    // 포커스를 잃거나 탭이 숨겨질 때 눌린 키·버튼이 남지 않도록 입력 상태를 비우고, 메뉴 누름을 실행 없이 취소한다.
    _clearInput() {
        this._clientComposing = false; this._clientImePending = null;
        this._input.keys.clear();
        this._input.pointer.buttons = 0;
        this._input.pointer.inside = false;
        this._mainMenu?.cancel();
        this._terrainPreview?.cancel();
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
        const hidden = this._root.ownerDocument.hidden;
        if (hidden) {
            if (this._gameClock && !this._hiddenGamePause) { this._gameClock.pushGamePause(); this._hiddenGamePause = true; }
            this._stopFrameLoop();
            this._clearInput();
        } else {
            if (this._gameClock && this._hiddenGamePause) { this._gameClock.popGamePause(); this._hiddenGamePause = false; }
            this._clock.reset();
            this._displayDirty = true;
            this._startFrameLoop();
        }
        // 탭 전환 중 오디오 장치 오류는 표시 루프와 독립적으로 보고한다. (2026-11-19 WB04-E944)
        const audio = this._audio;
        if (!audio) return;
        const update = hidden ? audio.suspend() : audio.resume();
        update.catch((error) => {
            if (this._audio === audio) reportGameError('audio-visibility', error, { state: this._state, hidden });
        });
    }

    // ----- 프레임 루프 -----

    // 프레임 루프를 시작한다. 이미 예약돼 있거나 탭이 숨겨져 있으면 아무것도 하지 않는다.
    _startFrameLoop() {
        if (this._frameHandle || !this._root || this._root.ownerDocument.hidden) return;
        this._frameHandle = this._view().requestAnimationFrame((now) => {
            try {
                this._onFrame(now);
            } catch (error) {
                reportGameError('frame', error, { state: this._state, frameTime: now });
                throw error;
            }
        });
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
        // 원본 Client 순서대로 화면 입력과 갱신 전에 같은 게임 프레임을 고정한다.
        this._gameClock?.captureFrame();
        this._clock.tick(now);
        this._frameSeconds = now / 1000;
        // 원본 루프 2번: 현재 미션의 로컬 State만 Poll한다. 수신 콜백 안의 재진입은 정리와 전환을 미룬다.
        const stateWorld = this._campaignWorld;
        if (stateWorld && !stateWorld.session().connectionCallbackActive()) stateWorld.session().pollLocalConnectionState();
        // 원본 루프 3번: 자연 종료한 버퍼를 포함해 패킷 처리 전에 현재 재생 수를 갱신한다.
        this._clientSoundPlayer?.recount();
        // 원본 루프 4번: Poll 뒤 현재 World를 다시 조회해 같은 내부 전송을 진행한다. 완료 후 화면 설치는 별도 경계다.
        const packetWorld = this._campaignWorld;
        if (packetWorld && !packetWorld.session().connectionCallbackActive()) packetWorld.session().pumpLocalConnection();
        // 원본 루프 8번의 Guide를 상태·패킷 처리 뒤, 입력 처리보다 먼저 실행한다.
        this._clientGuide?.run();
        if (this._state === AppState.MAIN_MENU) {
            // 원본 UserInput 은 매 프레임 마우스 위치를 폴링해 Gump 에 알린다. 그래서 포인터가 멈춰 있어도 그 아래에 새로 열린 펼침 메뉴 행이 바로 강조된다
            // (cpppj 오라클 help-popup/options-popup 관측). 브라우저는 포인터가 움직일 때만 사건을 주므로 마지막 포인터 위치를 매 프레임 다시 알린다. (2026-11-07 WB02-H)
            const pointer = this._input.pointer;
            if (this._mainMenu && pointer.inside) this._mainMenu.pointerMove(pointer.x, pointer.y);
            this._mainMenu?.update(this._frameSeconds);
            // 스크롤·창 열림/닫힘·키 입력으로 포인터 아래 내용이 바뀌었을 수 있으므로 커서를 다시 고른다(원본은 매 포인터 갱신마다 선택). (2026-10-31 WB03-L)
            this._applyCursor();
        }
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
        if (this._state === AppState.TERRAIN_PREVIEW && this._terrainPreview && this._surface) {
            const changed = this._terrainPreview.dirty;
            if (!changed && !this._displayDirty && !this.options.debug) return;
            if (changed) this._terrainPreview.render(this._surface);
            this._presenter.present(ctx, this._surface, layout, changed);
            this._displayDirty = false;
            return;
        }
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
