// 원본 화면 장치(Screen)와 호환 출력 계산(ScreenPresentation)을 웹으로 옮긴 모듈이다(cpppj `client/Screen.cpp/.h`·`ScreenPresentation.cpp/.h`, 10.78 전용).
// 원본 004a05a0(표면)·004a13d0(모드 검사)·004a14e0/004a1550(잠금)·004a1580(자르기)·004a1800(창으로 복사)·004a2060(채우기)·004a2640(팔레트)·004a4790(지우기)·
// 004a4850(팔레트 파일 로더의 저장·적용 후단)·004a4fe0(화면 모드)·00445290(사각형 자르기)와 Windows10/11 호환 확장(DIB 줄 간격 정렬·비율 유지 확대·화면 이동)을 한 모듈에 둔다.
//  - 순수 계산(DOM 없음): ScreenMode·screenModeIsLegal·clipScreenRect·modernScreenMode·fitScreenPresentation·mapScreenPresentationPoint/Rect·scaleScreenPresentation·scrollScreenPixels.
//  - Screen: 8비트 DIB 표면(줄 간격 `(폭+3)&~3`)·잠금 깊이·자르기 영역·모드 플래그·256색 저장/현재/논리 팔레트·색 표(`Colors`)·명암 표(`ShadeMaps`)·커서 숨김 깊이를 원본 순서로 둔다.
//    창·GDI·DirectDraw·GPU 후처리(7차)·창 위치(`InitialWindowPosition`)는 웹에 대응이 없어 옮기지 않고, 표시 장치는 `devices` 훅(`present`·`presentFullScreen`·`paletteChanged`·`waitVerticalBlank`·`cursor`)으로만 받는다.
//    훅이 없으면 해당 출력만 건너뛴다(원본의 널 장치 문맥과 같음). 팔레트 파일의 BGRX 예약 바이트는 웹 `GamePalette` 가 보존하지 않으므로 RGB 파일처럼 이전 저장 값을 유지한다(E272 결정).
//  - 사각형은 `{ left, top, right, bottom }` 이며 right/bottom 은 포함하지 않는다. 팔레트 한 칸은 BGRX 네 바이트(파랑·초록·빨강·예약)이고 논리 팔레트 한 칸은 `R | G<<8 | B<<16 | 플래그<<24` 이다.
// 앱 수명에는 연결하지 않았다(`PORT_MAP` session 0). 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E393: 5-A Screen·ScreenPresentation 이식으로 추가)
import { applyScreenPaletteClear } from '../core/screen-palette-apply.js';
import { buildPaletteColorTable } from '../core/palette-colors.js';
import { buildShadeMaps, findPaletteColor } from '../assets/palette.js';
import { clipRect } from './indexed-surface.js';

// 화면 모드 플래그(원본 DAT_005c78d4)이다. 설정 `windowScreenFlags`·`workingFullScreenFlags` 의 값이며 모드가 정해지면 `global.dd…` 설정으로도 적힌다.
// 사용: (flags & ScreenMode.modeSet) 처럼 비트로 읽는다. (2026-11-13 WB04-E393)
export const ScreenMode = Object.freeze({
    // DirectDraw 전체화면(global.ddFullScreen)이다.
    fullScreen: 0x01,
    // 그리기 표면이 DIB 섹션(global.ddBackBufferDib)이다.
    backBufferDib: 0x02,
    // 페이지 플리핑(global.ddFlipping)이다.
    flipping: 0x04,
    // 구름("parallax without clouds" 검사의 짝)이다.
    clouds: 0x08,
    // 구름 시차(global.ddParallax)이다.
    parallax: 0x10,
    // 구름 움직임이다.
    animating: 0x20,
    // 커서를 직접 그린다(global.ddSoftwareMouse)이다.
    softwareMouse: 0x40,
    // 모드를 정할 때 항상 켜는 비트이다.
    modeSet: 0x80,
    // DirectDraw 가 없는데 전체화면을 요구하면 원본이 대신 쓰는 값(004a4fe0 의 `param_1 = 10`)이다.
    fallbackWindowed: 0x02 | 0x08,
});

// 팔레트 항목 수와 논리 팔레트 항목의 플래그(원본이 쓰는 4 = PC_NOCOLLAPSE)이다. (2026-11-13 WB04-E393)
const PALETTE_SIZE = 256;
const PALETTE_ENTRY_FLAGS = 4;

// 팔레트 파일 로더 뒤에 채우는 날씨 네 색의 목표 RGB(바람=노랑, 비=파랑, 천둥=빨강, 해=갈색)이다. 원본 004a4850 의 DAT_005b5da8..005b5db4 대입 순서이다. (2026-11-13 WB04-E393)
const WEATHER_RGB = Object.freeze([[255, 255, 22], [22, 22, 255], [255, 22, 22], [181, 140, 111]]);

// 원본 FUN_00445290 와 같은 사각형 자르기이다: 네 좌표를 각각 경계 안으로 넣으며 뒤집힌 사각형은 바로잡지 않는다. 결과는 새 객체이다.
// 사용: clipScreenRect({ left: -5, top: 0, right: 700, bottom: 9 }, { left: 0, top: 0, right: 640, bottom: 480 }) (2026-11-13 WB04-E393: `indexed-surface.js` 의 `clipRect` 와 같은 계산을 Screen 이름으로 내보냄)
export function clipScreenRect(rect, bounds) {
    return clipRect(rect, bounds);
}

// 원본 004a13d0 이다: 화면 모드 조합이 허용되는지 본다. 원본은 맞지 않는 조건마다 "Illegal screen mode" 로그를 남기고 개수를 센다(웹은 개수만 본다).
// 사용: screenModeIsLegal(11, true) → true, screenModeIsLegal(11, false) → false (DirectDraw 없는 전체화면). (2026-11-13 WB04-E393)
export function screenModeIsLegal(flags, directDrawInstalled) {
    let problems = 0;
    // DirectDraw 가 없는데 전체화면이다.
    if (!directDrawInstalled && (flags & ScreenMode.fullScreen) !== 0) problems += 1;
    // 전체화면이 아닌 플리핑이다.
    if ((flags & ScreenMode.flipping) !== 0 && (flags & ScreenMode.fullScreen) === 0) problems += 1;
    // 구름 없는 시차이다.
    if ((flags & ScreenMode.parallax) !== 0 && (flags & ScreenMode.clouds) === 0) problems += 1;
    // 구름 없는 움직임이다.
    if ((flags & ScreenMode.animating) !== 0 && (flags & ScreenMode.clouds) === 0) problems += 1;
    // 전체화면도 DIB 도 아니면 그릴 곳이 없다.
    if ((flags & (ScreenMode.fullScreen | ScreenMode.backBufferDib)) === 0) problems += 1;
    return problems === 0;
}

// Windows10/11 호환 경로의 실제 DIB/비플립 플래그로 정규화한다. 원본 `screenModeIsLegal` 계산은 바꾸지 않는다. 결과는 부호 없는 32비트 수이다.
// 사용: modernScreenMode(111) → 235, modernScreenMode(0) → 130. (2026-11-13 WB04-E393)
export function modernScreenMode(requested) {
    let flags = (requested | ScreenMode.modeSet) >>> 0;
    if (flags & ScreenMode.fullScreen) flags = (flags | ScreenMode.backBufferDib) & ~ScreenMode.flipping;
    // 창 모드는 원본 00435ba0 의 비트 정리(소프트웨어 마우스·플리핑·시차 끔)도 유지한다.
    else flags = (flags & ~(ScreenMode.softwareMouse | ScreenMode.flipping | ScreenMode.parallax)) | ScreenMode.backBufferDib;
    return flags >>> 0;
}

// 호환 전체화면은 논리 크기를 바꾸지 않고 현재 클라이언트 영역에 비율대로 가운데 맞춘다(남는 영역은 검정). 양수가 아닌 크기는 거부한다.
// 사용: fitScreenPresentation(1024, 768, 1920, 1080) → { left: 240, top: 0, right: 1680, bottom: 1080 }. (2026-11-13 WB04-E393)
export function fitScreenPresentation(width, height, clientWidth, clientHeight) {
    if (!(width > 0) || !(height > 0) || !(clientWidth > 0) || !(clientHeight > 0)) throw new RangeError('전체화면 출력의 양수 크기 필요');
    let fittedWidth = clientWidth;
    let fittedHeight = clientHeight;
    // 가로가 더 꽉 차면 높이를, 아니면 폭을 비율에 맞춘다(정수 나눗셈은 0 쪽으로 자르고 최소 1).
    if (clientWidth * height <= clientHeight * width) fittedHeight = Math.max(1, Math.trunc((clientWidth * height) / width));
    else fittedWidth = Math.max(1, Math.trunc((clientHeight * width) / height));
    const left = Math.trunc((clientWidth - fittedWidth) / 2);
    const top = Math.trunc((clientHeight - fittedHeight) / 2);
    return { left, top, right: left + fittedWidth, bottom: top + fittedHeight };
}

// 호환 출력의 실제 마우스 좌표를 논리 화면 좌표로 되돌리고 원본 Poll 의 끝 포함 범위 [0,폭]/[0,높이]로 제한한다. 출력 범위가 비면 거부한다.
// 사용: mapScreenPresentationPoint({ x: 960, y: 540 }, { left: 240, top: 0, right: 1680, bottom: 1080 }, 1024, 768) → { x: 512, y: 384 }. (2026-11-13 WB04-E393)
export function mapScreenPresentationPoint(point, output, width, height) {
    if (!(width > 0) || !(height > 0) || output.right <= output.left || output.bottom <= output.top) throw new RangeError('전체화면 입력 변환의 실제 출력 범위 필요');
    // 정수 나눗셈은 0 쪽으로 자른다(원본 64비트 정수 연산).
    const x = Math.trunc(((point.x - output.left) * width) / (output.right - output.left));
    const y = Math.trunc(((point.y - output.top) * height) / (output.bottom - output.top));
    return { x: Math.min(Math.max(x, 0), width), y: Math.min(Math.max(y, 0), height) };
}

// 논리 화면의 변경 사각형을 출력 범위로 옮기되 오른쪽/아래는 바깥쪽으로 반올림해 확대 경계의 잔상을 막는다. 사각형은 먼저 논리 화면으로 자른다.
// 사용: mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, output, 1024, 768). (2026-11-13 WB04-E393)
export function mapScreenPresentationRect(rect, output, width, height) {
    if (!(width > 0) || !(height > 0) || output.right <= output.left || output.bottom <= output.top) throw new RangeError('전체화면 dirty 변환의 실제 출력 범위 필요');
    const cut = clipScreenRect(rect, { left: 0, top: 0, right: width, bottom: height });
    const x = output.right - output.left;
    const y = output.bottom - output.top;
    return {
        left: output.left + Math.trunc((cut.left * x) / width),
        top: output.top + Math.trunc((cut.top * y) / height),
        right: output.left + Math.trunc((cut.right * x + width - 1) / width),
        bottom: output.top + Math.trunc((cut.bottom * y + height - 1) / height),
    };
}

// 전체 원본 표면을 픽셀 중심 표본의 최근접으로 확대/축소해 destination 에 쓴다(장치별 반올림 차이를 피함). 행 간격 padding 은 건드리지 않고 크기·범위가 모자라면 거부한다.
// 사용: scaleScreenPresentation(source, sourcePitch, sourceWidth, sourceHeight, destination, destinationPitch, destinationWidth, destinationHeight). (2026-11-13 WB04-E393)
export function scaleScreenPresentation(source, sourcePitch, sourceWidth, sourceHeight, destination, destinationPitch, destinationWidth, destinationHeight) {
    if (!(sourceWidth > 0) || !(sourceHeight > 0) || !(destinationWidth > 0) || !(destinationHeight > 0) || sourcePitch < sourceWidth || destinationPitch < destinationWidth) throw new RangeError('전체화면 확대의 양수 크기/행 간격 필요');
    if (source.length < (sourceHeight - 1) * sourcePitch + sourceWidth || destination.length < (destinationHeight - 1) * destinationPitch + destinationWidth) throw new RangeError('전체화면 확대의 실제 픽셀 범위 부족');
    const columns = new Int32Array(destinationWidth);
    // 각 목적지 열의 최근접 원본 열을 한 번 계산한다.
    for (let x = 0; x < destinationWidth; x += 1) columns[x] = Math.trunc(((2 * x + 1) * sourceWidth) / (2 * destinationWidth));
    // 각 목적지 행을 원본 행의 최근접 선택에서 복사한다.
    for (let y = 0; y < destinationHeight; y += 1) {
        const row = Math.trunc(((2 * y + 1) * sourceHeight) / (2 * destinationHeight)) * sourcePitch;
        const offset = y * destinationPitch;
        for (let x = 0; x < destinationWidth; x += 1) destination[offset + x] = source[row + columns[x]];
    }
}

// 원본 DIB 처럼 양수 카메라 이동은 원본 오른쪽/아래 픽셀을 왼쪽/위로 복사한다. 겹친 영역은 행 순서로 보존하며 화면 밖 읽기는 거부한다.
// 영역 안에서 이동량이 영역 크기 이상이거나 이동이 0 이면 아무것도 하지 않는다. 사용: scrollScreenPixels(pixels, pitch, width, height, x, y, regionWidth, regionHeight, dx, dy). (2026-11-13 WB04-E393)
export function scrollScreenPixels(pixels, pitch, width, height, x, y, regionWidth, regionHeight, dx, dy) {
    if (regionWidth <= 0 || regionHeight <= 0 || Math.abs(dx) >= regionWidth || Math.abs(dy) >= regionHeight || (!dx && !dy)) return;
    if (width <= 0 || height <= 0 || pitch < width || x < 0 || y < 0 || x + regionWidth > width || y + regionHeight > height || pixels.length < pitch * height) throw new RangeError('DIB 화면 이동의 실제 픽셀 범위 오류');
    const sourceX = x + Math.max(dx, 0);
    const destinationX = x + Math.max(-dx, 0);
    const rows = regionHeight - Math.abs(dy);
    const columns = regionWidth - Math.abs(dx);
    // 아래로 복사할 때는 뒤쪽 행부터 옮겨 겹친 원본을 보존한다.
    for (let index = 0; index < rows; index += 1) {
        const row = dy > 0 ? index : rows - 1 - index;
        const sourceY = y + Math.max(dy, 0) + row;
        const destinationY = y + Math.max(-dy, 0) + row;
        pixels.copyWithin(destinationY * pitch + destinationX, sourceY * pitch + sourceX, sourceY * pitch + sourceX + columns);
    }
}

// 원본 표면 객체(생성 004a05a0)이다. 종류 1 = DIB 섹션(8비트 위→아래, 줄 간격 4바이트 정렬), 2 = 창 표면(픽셀 없음)이다. 종류 3~5(DirectDraw 표면)는 옮기지 않았다.
// 사용: new ScreenSurface('dib', 1366, 768).pitch → 1368. (2026-11-13 WB04-E393)
export class ScreenSurface {
    // 종류('dib'|'window')·가로/세로·픽셀 버퍼(DIB 만)·잠금 중인 버퍼이다.
    #kind;
    #width;
    #height;
    #pixels = null;
    #locked = null;

    // 종류와 크기로 만든다. DIB 는 `pitch * height` 바이트를 0 으로 채운 버퍼를 갖는다.
    constructor(kind, width, height) {
        if (kind !== 'dib' && kind !== 'window') throw new RangeError('화면 표면 종류 오류');
        if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) throw new RangeError(`화면 표면 크기 오류: ${width}x${height}`);
        this.#kind = kind;
        this.#width = width;
        this.#height = height;
        if (kind === 'dib') this.#pixels = new Uint8Array(this.pitch * height);
    }

    // 표면 종류이다.
    get kind() {
        return this.#kind;
    }

    // 표면의 폭이다.
    get width() {
        return this.#width;
    }

    // 표면의 높이이다.
    get height() {
        return this.#height;
    }

    // 줄 간격이다: DIB 는 실제 Win32 DIB 처럼 4바이트 정렬(1366 폭 → 1368), 창 표면은 폭과 같다.
    get pitch() {
        return this.#kind === 'dib' ? (this.#width + 3) & ~3 : this.#width;
    }

    // 호환 확대 출력이 잠금 수를 바꾸지 않고 읽는 실제 DIB 버퍼이다(창 표면은 null).
    get pixels() {
        return this.#pixels;
    }

    // 004a08b0: DIB 는 버퍼를, 창 표면은 null 을 돌려준다.
    lock() {
        this.#locked = this.#kind === 'dib' ? this.#pixels : null;
        return this.#locked;
    }

    // 004a0950: 풀 것이 없는 표면도 잠금 표시를 지운다.
    unlock() {
        this.#locked = null;
    }
}

// 원본 Screen.cpp 의 전역 묶음을 객체 하나로 둔 화면 장치이다. 사용: const screen = new Screen(640, 480); screen.init(); screen.initDibSection(); screen.setMode(10);
//   screen.loadPalette(palette); const pixels = screen.lock(); screen.fillRect(rect, color); screen.unlock(); screen.update(rect).
// 창·장치 문맥 대신 `options.devices` 의 표시 훅을 받고, `options.paletteLoaded` 는 클라이언트가 소유한 00555e08 `{ value }` 상자이다(화면 재생성으로 초기화하지 않음).
// (2026-11-13 WB04-E393)
export class Screen {
    // 가로/세로, 클라이언트가 소유한 palette loaded 상자, 표시 훅이다.
    #width;
    #height;
    #paletteLoaded;
    #devices;
    // 표면(창/그리기 DIB)과 현재 그리기·표시 표면이다. DAT_005c792c·005c7930·005c7948·005c794c.
    #windowSurface = null;
    #dibSurface = null;
    #drawSurface = null;
    #primarySurface = null;
    // 화면 모드 플래그(DAT_005c78d4), 잠금 깊이(DAT_005c7920), 잠금 중인 버퍼(DAT_005c78f8)이다.
    #flags = 0;
    #lockDepth = 0;
    #buffer = null;
    // 현재 자르기 영역(DAT_005c7a44..50, right/bottom 은 포함하지 않음)이다.
    #clip = { left: 0, top: 0, right: 0, bottom: 0 };
    // 현재 팔레트(DAT_0059afe0)와 저장 팔레트(DAT_005acd28)이다(BGRX 256칸), 논리 팔레트(DAT_005c7954)이다(R|G<<8|B<<16|플래그<<24).
    #current = new Uint8Array(PALETTE_SIZE * 4);
    #saved = new Uint8Array(PALETTE_SIZE * 4);
    #logical = new Uint32Array(PALETTE_SIZE);
    // 파일 팔레트 로더가 다시 계산하는 기본/표시/날씨 색 표와 돌 배경 명암 표, 클라이언트 현재 9표·고정 색 정렬 입력의 조회기이다.
    #colors = Object.freeze({ basic: Object.freeze(new Array(9).fill(0)), weather: Object.freeze([0, 0, 0, 0]), named: Object.freeze([]) });
    #shadeMaps = null;
    #currentColorMaps = null;
    #colorSortInput = null;
    // Win32 커서 숨김 깊이(DAT_005c7a3c), 창 테두리 두께(DAT_005c7904/08), 활성 여부, 호스트 클라이언트 영역 크기이다.
    #cursorHidden = 0;
    #border = { x: 0, y: 0 };
    #active = false;
    #client = null;

    // 화면 크기와 선택 옵션({ paletteLoaded: { value }, devices })으로 만든다. 표면은 init·initDibSection 에서 만든다.
    constructor(width, height, options = {}) {
        if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) throw new RangeError(`Screen 크기 오류: ${width}x${height}`);
        this.#width = width;
        this.#height = height;
        this.#paletteLoaded = options.paletteLoaded ?? { value: 0 };
        this.#devices = options.devices ?? {};
    }

    // 원본 004a1270("init screen"): 창 표면과 빈 논리 팔레트를 만든다.
    init() {
        this.#windowSurface = new ScreenSurface('window', this.#width, this.#height);
        this.#logical.fill(0);
    }

    // 원본 004a4570("init dib section"): 8비트 DIB 표면을 만들고 모드 플래그와 자르기 영역을 비운다. 잠근 버퍼는 곧바로 얻어 둔다.
    initDibSection() {
        this.#dibSurface = new ScreenSurface('dib', this.#width, this.#height);
        this.#flags = 0;
        this.#buffer = this.#dibSurface.lock();
        this.#clip = { left: 0, top: 0, right: 0, bottom: 0 };
    }

    // 원본 004a4fe0 의 창 모드 경로이다: 모드를 호환 DIB/비플립으로 정규화하고 같으면 true, 허용되지 않거나 DIB 가 없으면 false 를 돌려준다.
    // 바뀌면 현재 팔레트를 저장 팔레트로 복사한 뒤 표면을 연결하고 저장 팔레트를 다시 적용하며 커서 깊이를 0 으로 맞춘다. 창 위치/모양은 웹에 없다.
    setMode(requested) {
        const flags = modernScreenMode(requested);
        this.#lockDepth = 0;
        if (flags === this.#flags) return true;
        if (!screenModeIsLegal(flags, true) || !this.#dibSurface || !this.#windowSurface) return false;
        this.showCursor(false);
        this.#saved.set(this.#current);
        this.#drawSurface = this.#dibSurface;
        this.#primarySurface = this.#windowSurface;
        this.#flags = flags;
        // 전체화면은 테두리를 없앤다(웹은 호스트가 창을 관리하므로 테두리 두께가 항상 0).
        this.#border = { x: 0, y: 0 };
        this.setPalette(0, PALETTE_SIZE, null, true);
        this.#cursorHidden = 1;
        this.showCursor(true);
        this.showCursor(true);
        this.setActive(this.#devices.foreground ? Boolean(this.#devices.foreground()) : true);
        return true;
    }

    // 호스트가 알려 주는 클라이언트 영역 크기이다. 호환 전체화면의 출력 범위(`presentationRect`)와 입력 변환이 읽는다. null 이면 논리 크기를 쓴다.
    setClientSize(width, height) {
        this.#client = width > 0 && height > 0 ? { width, height } : null;
    }

    // 현재 실제 출력 범위이다: 창 모드는 논리 화면 전체, 호환 전체화면은 클라이언트 영역에 비율대로 맞춘 범위이다.
    presentationRect() {
        if (!(this.#flags & ScreenMode.fullScreen)) return { left: 0, top: 0, right: this.#width, bottom: this.#height };
        if (!this.#client) return { left: 0, top: 0, right: this.#width, bottom: this.#height };
        return fitScreenPresentation(this.#width, this.#height, this.#client.width, this.#client.height);
    }

    // 창 모드의 1:1 입력은 그대로 두고 호환 전체화면만 확대를 되돌려 논리 좌표로 바꾼다.
    toLogicalPoint(point) {
        return this.#flags & ScreenMode.fullScreen ? mapScreenPresentationPoint(point, this.presentationRect(), this.#width, this.#height) : { x: point.x, y: point.y };
    }

    // 활성 여부를 바꾼다(호환 전체화면 `update` 는 활성일 때만 출력). 웹은 커서 제한이 없다.
    setActive(active) {
        this.#active = Boolean(active);
    }

    // 현재 활성 여부이다.
    get active() {
        return this.#active;
    }

    // 원본 004a2640: 팔레트의 [start, start+count) 를 colors(BGRX 바이트, 없으면 저장 팔레트)로 바꾼다. 논리 팔레트는 apply 와 무관하게 갱신하고 0번은 검정, 255번은 흰색으로 고정한다.
    // 인자 colors 의 0번 칸과(범위 끝이 256 이면) 255번 칸도 같은 값으로 덮어쓴다(원본 동작). apply 가 참이면 현재 팔레트에 복사하고 `devices.paletteChanged` 를 부르며, 전체화면이면 화면 전체를 다시 낸다.
    setPalette(start, count, colors, apply) {
        const source = colors ?? this.#saved;
        const end = start + count;
        if (!Number.isInteger(start) || !Number.isInteger(count) || start < 0 || count < 0) throw new RangeError('Screen 팔레트 범위 오류');
        if (source.length < Math.max(count, 1) * 4) throw new RangeError('Screen 팔레트 색 배열이 짧습니다.');
        // 논리 팔레트 항목은 빨강·초록·파랑·플래그 순서이다.
        for (let index = start; index < end && index < PALETTE_SIZE; index += 1) {
            const at = (index - start) * 4;
            this.#logical[index] = (source[at + 2] | (source[at + 1] << 8) | (source[at] << 16) | (PALETTE_ENTRY_FLAGS << 24)) >>> 0;
        }
        // 0번은 검정, 255번은 흰색으로 고정한다(플래그 없음).
        this.#logical[0] = 0;
        this.#logical[PALETTE_SIZE - 1] = 0x00ffffff;
        if (start === 0) source.fill(0, 0, 4);
        // 원본은 colors[255] 에 쓴다(start 가 0 이라고 가정한 코드). 부분 배열도 범위를 넘지 않게 끝 항목에 쓴다.
        if (end === PALETTE_SIZE) {
            const at = (PALETTE_SIZE - 1 - start) * 4;
            source.set([0xff, 0xff, 0xff, 0], at);
        }
        if (!apply) return;
        let first = start;
        let amount = count;
        if (amount === 0) {
            first = 0;
            amount = PALETTE_SIZE;
            if (source.length < PALETTE_SIZE * 4) throw new RangeError('Screen 팔레트 색 배열이 짧습니다.');
        }
        // 현재 팔레트에 복사한다.
        for (let index = 0; index < amount && first + index < PALETTE_SIZE; index += 1) this.#current.set(source.subarray(index * 4, index * 4 + 4), (first + index) * 4);
        if (this.#devices.paletteChanged) this.#devices.paletteChanged(this.#current);
        // 원본 8비트 독점 팔레트와 달리 현대 데스크톱은 이미 출력한 RGB 픽셀도 다시 칠해야 번개/날씨가 전체에 적용된다.
        if ((this.#flags & ScreenMode.fullScreen) && this.#drawSurface) this.update({ left: 0, top: 0, right: this.#width, bottom: this.#height });
    }

    // 원본 004a4850 의 저장·적용 후단이다: 웹 `GamePalette`(rgb 768바이트)의 256색을 저장 팔레트에 BGRX 순서로 넣고 적용한다(예약 바이트는 이전 값을 유지). clear 가 0 이 아니면 적용 전에 화면을 지운다.
    loadPalette(palette, clear = 0) {
        // 색 번호를 그대로 유지하며 R,G,B → B,G,R 로 옮긴다.
        for (let index = 0; index < PALETTE_SIZE; index += 1) {
            this.#saved[index * 4] = palette.rgb[index * 3 + 2];
            this.#saved[index * 4 + 1] = palette.rgb[index * 3 + 1];
            this.#saved[index * 4 + 2] = palette.rgb[index * 3];
        }
        this.applySavedPalette(clear);
    }

    // 저장 팔레트를 적용하는 공통 후단이다: loaded 상자를 1 로 만들고(clear 관문의 lock→fill 0→unlock→수직 동기→viewport 출력→flip), `setPalette(0,256,null,true)` 뒤 색 표와 명암 표를 다시 계산한다.
    applySavedPalette(clear) {
        applyScreenPaletteClear(clear, this.#flags, this.#paletteLoaded, {
            // 같은 현재 깊이/표면을 잠근다.
            lock: () => { this.lock(); },
            // 비트 0x80 이 없으면 원문 별도 fillBlt 경계를 빈 성공으로 가장하지 않는다.
            fillBlack: () => {
                if ((this.#flags & ScreenMode.modeSet) === 0) throw new Error('팔레트전환Surface.fillBlt미복원');
                this.fillRect({ left: 0, top: 0, right: this.#width, bottom: this.#height }, 0);
            },
            // 중단되어도 이전 부분 효과를 되돌리지 않는다.
            unlock: () => { this.unlock(); },
            // GDI 자연 관문 뒤에만 표시로 간다.
            waitVerticalBlank: () => { this.#waitVerticalBlank(); },
            // 이전 clip 을 viewport 복사 뒤 복구한다.
            presentPreservingClip: () => { this.presentPaletteClear(); },
            // bit4 가 없을 때만 자연 생략한다.
            flip: () => { this.#flipPaletteClear(); },
        });
        this.setPalette(0, PALETTE_SIZE, null, true);
        this.#colors = buildPaletteColorTable(this.#logical);
        this.#shadeMaps = this.#currentColorMaps ? this.#currentColorMaps() : buildShadeMaps(this.#colorSortInput ? this.#colorSortInput() : this.#logical);
    }

    // 원본과 다른 호환 전체화면은 독점 DirectDraw vblank 대신 현재 합성을 동기화한다(전체화면일 때 훅 호출).
    #waitVerticalBlank() {
        if ((this.#flags & ScreenMode.fullScreen) !== 0 && this.#devices.waitVerticalBlank) this.#devices.waitVerticalBlank();
    }

    // 004a46d0: 현재 구현의 viewport 는 SetMode 에서 정한 0,0,폭,높이이다. Update 가 바꾼 clip 을 앞선 값으로 복구한다.
    presentPaletteClear() {
        const previous = { ...this.#clip };
        this.update({ left: 0, top: 0, right: this.#width, bottom: this.#height });
        this.#clip = previous;
    }

    // 004a16d0 은 bit4 가 0 이면 자연 반환한다. 합법 모드는 이 관문을 지나가며 flip 장치를 요구하는 모드는 거부한다.
    #flipPaletteClear() {
        if ((this.#flags & ScreenMode.flipping) !== 0) throw new Error('팔레트전환DirectDraw flip장치미복원');
    }

    // 004a16d0 의 실제 bit4 관문을 공유한다. 호환 DIB 는 flip 을 요청하지 않는다.
    flip() {
        this.#flipPaletteClear();
    }

    // 004a1ad0 의 DIB 행 이동을 현재 같은 표면에서 수행한다. DIB 모드가 아니면 거부하고 이동량이 0 이면 잠그지도 않는다.
    scroll(x, y, width, height, dx, dy) {
        if (!(this.#flags & ScreenMode.backBufferDib)) throw new Error('화면 이동의 현재 DIB 장치 필요');
        if (!dx && !dy) return;
        const pixels = this.lock();
        try {
            scrollScreenPixels(pixels.subarray(0, this.pitch * this.#height), this.pitch, this.#width, this.#height, x, y, width, height, dx, dy);
        } finally {
            this.unlock();
        }
    }

    // 00555e08 실제 DWORD 상자를 같은 Screen 수명으로 빌려 준다.
    get paletteLoaded() {
        return this.#paletteLoaded;
    }

    // 마지막 파일 팔레트로 계산한 원본 기본/표시/날씨 색 표이다. 번개의 일시 `setPalette` 나 모드 적용으로 다시 계산하지 않는다.
    colors() {
        return this.#colors;
    }

    // 파일 팔레트의 GUI 밝음/어두움 변환표({ bright, dark })이다. 클라이언트 현재 9표 조회기가 연결돼 있으면 매 조회에서 그 값을 쓴다. 팔레트를 적재하기 전에는 null 이다.
    shadeMaps() {
        if (this.#currentColorMaps) this.#shadeMaps = this.#currentColorMaps();
        return this.#shadeMaps;
    }

    // 실제 클라이언트의 장기 9표 조회기를 연결한다(초기 캐시 준비 뒤와 해상도 재생성 뒤). 연결 즉시 한 번 읽는다.
    bindColorMaps(current) {
        if (typeof current !== 'function') throw new TypeError('Screen 현재9색표 조회기 누락');
        this.#currentColorMaps = current;
        this.#shadeMaps = current();
    }

    // 클라이언트의 고정 초기 색 정렬 입력(논리 팔레트 256칸) 조회기를 연결한다. 연결이 없으면 현재 논리 팔레트를 쓴다.
    bindColorSortInput(current) {
        if (typeof current !== 'function') throw new TypeError('Screen 고정ColorSort 입력조회기누락');
        this.#colorSortInput = current;
    }

    // 현재 논리 팔레트(R|G<<8|B<<16|플래그<<24, 256칸)이다. 화면 RGB 사본으로 바꾸지 않는다.
    logicalPalette() {
        return this.#logical;
    }

    // 현재 팔레트 BGRX 256칸(1024바이트)이다. 읽기 전용으로 쓴다.
    palette() {
        return this.#current;
    }

    // 현재 논리 팔레트에서 RGB 색을 찾는다. `setPalette(..., apply=false)` 로 준비한 논리 팔레트도 반영하며 원본 32비트 감김·x87 비교 순서를 따른다.
    findColor(red, green, blue) {
        return findPaletteColor(this.#logical, red, green, blue);
    }

    // 현재 팔레트에서 날씨 RGB 네 색(노랑·파랑·빨강·갈색)에 가장 가까운 번호를 검색한다.
    weatherTints() {
        return WEATHER_RGB.map(([red, green, blue]) => this.findColor(red, green, blue));
    }

    // 원본 004a14e0: 그리기 표면 잠금(중첩 횟수를 센다). 처음 잠글 때만 버퍼를 얻는다.
    lock() {
        const first = this.#lockDepth === 0;
        this.#lockDepth += 1;
        if (first) {
            if (!this.#drawSurface) throw new Error('drawSurf');
            this.#buffer = this.#drawSurface.lock();
        }
        return this.#buffer;
    }

    // 원본 004a1550: 마지막 잠금이 풀릴 때 표면을 푼다.
    unlock() {
        this.#lockDepth -= 1;
        if (this.#lockDepth === 0) {
            this.#drawSurface.unlock();
            this.#buffer = null;
        }
    }

    // 원본 004a1580: 그리기 자르기 영역을 정한다(right/bottom 은 포함하지 않는다).
    setClip(left, top, right, bottom) {
        this.#clip = { left, top, right, bottom };
    }

    // 현재 exclusive clip 을 바꾸지 않고 복사해 읽는다.
    clip() {
        return { ...this.#clip };
    }

    // 004a1fe0→004a0c10: 잠근 DIB 에 원시 색 번호 사각형을 복사한다. 원본 소스 사각형을 대상 크기로 바꾼 뒤 화면/현재 clip 으로 자르고(32비트 감김 좌표), 4바이트/나머지 1바이트씩 앞으로 복사한다.
    // 겹친 입력도 해당 DWORD 를 먼저 읽은 뒤 쓴다. 잠금이 없거나 물리 범위를 넘으면 거부한다.
    blitBuffer(x, y, source, sourcePitch, sourceRect) {
        const add = (first, second) => (first + second) | 0;
        const subtract = (first, second) => (first - second) | 0;
        const clip = clipScreenRect(this.#clip, { left: 0, top: 0, right: this.#width, bottom: this.#height });
        const target = clipScreenRect({ left: x, top: y, right: add(x, subtract(sourceRect.right, sourceRect.left)), bottom: add(y, subtract(sourceRect.bottom, sourceRect.top)) }, clip);
        if (!this.#buffer) throw new Error('구름 buffer 복사의 실제 잠금 누락');
        const width = subtract(target.right, target.left);
        const height = subtract(target.bottom, target.top);
        const pitch = this.pitch;
        if (width < 0 || height < 0 || sourcePitch < 0) throw new RangeError('구름 buffer 복사의 현재 물리 폭');
        // 자르기 뒤 원문 source 시작 위치이다(64비트 곱 — 안전 정수를 넘으면 복사 범위를 벗어난 것으로 본다).
        const first = BigInt(add(sourceRect.top, subtract(target.top, y))) * BigInt(sourcePitch) + BigInt(add(sourceRect.left, subtract(target.left, x)));
        const exact = first >= -BigInt(Number.MAX_SAFE_INTEGER) && first <= BigInt(Number.MAX_SAFE_INTEGER);
        if (!exact && width > 0 && height > 0) throw new RangeError('구름 buffer 복사의 실제 물리 범위');
        const origin = exact ? Number(first) : 0;
        const word = new Uint8Array(4);
        // 행마다 현재 원시 source 와 같은 실제 잠긴 DIB 를 사용한다.
        for (let row = 0; row < height; row += 1) {
            const sourceRow = origin + row * sourcePitch;
            const destinationRow = (target.top + row) * pitch + target.left;
            // 원문의 DWORD 앞으로 복사와 마지막 0~3바이트를 같은 순서로 실행한다.
            for (let column = 0; column < width;) {
                const bytes = width - column >= 4 ? 4 : 1;
                const from = sourceRow + column;
                const to = destinationRow + column;
                if (from < 0 || from + bytes > source.length || to < 0 || to + bytes > pitch * this.#height) throw new RangeError('구름 buffer 복사의 실제 물리 범위');
                for (let byte = 0; byte < bytes; byte += 1) word[byte] = source[from + byte];
                for (let byte = 0; byte < bytes; byte += 1) this.#buffer[to + byte] = word[byte];
                column += bytes;
            }
        }
    }

    // 원본 004a2060 의 "모드가 정해진 뒤" 경로이다: 현재 clip 으로 자른 사각형을 줄마다 같은 색 바이트로 채운다. 모드가 정해지지 않았거나 잠그지 않았으면 아무것도 하지 않는다.
    // 표면 밖 쓰기는 원본의 미정의 동작 대신 거부한다.
    fillRect(rect, color) {
        const cut = clipScreenRect(rect, this.#clip);
        if ((this.#flags & ScreenMode.modeSet) === 0 || !this.#buffer) return;
        const pitch = this.pitch;
        const width = Math.max(cut.right - cut.left, 0);
        // 위에서 아래로 한 줄씩 채운다.
        for (let y = cut.top; y < cut.bottom; y += 1) {
            const start = pitch * y + cut.left;
            if (width > 0 && (cut.left < 0 || cut.left + width > pitch || y < 0 || start + width > this.#buffer.length)) throw new RangeError('Screen.fillRect 의 표면 밖 쓰기');
            this.#buffer.fill(color & 255, start, start + width);
        }
    }

    // 원본 004a4790: 잠그고 채우고 푼다. 화면 모드가 정해지기 전(플래그 0)에는 아무것도 하지 않는다.
    clear(rect, color) {
        if (this.#flags === 0) return;
        this.lock();
        try {
            this.fillRect(rect, color);
        } finally {
            this.unlock();
        }
    }

    // 원본 004a1800: 사각형을 화면으로 자르고 자르기 영역을 화면 전체로 되돌린 뒤 표시 장치로 보낸다. 빈 사각형이면 보내지 않는다.
    // 호환 전체화면은 활성이고 DIB 가 있을 때만 전체 출력 범위(`presentationRect`)와 함께 `devices.presentFullScreen` 을, 창 모드는 표면이 연결돼 있으면 `devices.present(rect, view)` 를 부른다.
    update(rect) {
        const screen = { left: 0, top: 0, right: this.#width, bottom: this.#height };
        const cut = clipScreenRect(rect, screen);
        this.#clip = { ...screen };
        if (cut.right <= cut.left || cut.bottom <= cut.top) return;
        const view = { pixels: this.#dibSurface?.pixels ?? null, pitch: this.pitch, width: this.#width, height: this.#height, palette: this.#current };
        if (this.#flags & ScreenMode.fullScreen) {
            if (!this.#active || !this.#dibSurface) return;
            if (this.#devices.presentFullScreen) this.#devices.presentFullScreen(this.presentationRect(), view);
            return;
        }
        if (this.#primarySurface && this.#drawSurface && this.#devices.present) this.#devices.present(cut, view);
    }

    // 원본 004a40d0: 시스템 커서를 보이거나 숨긴다(숨김 깊이를 센다). 소프트웨어 마우스 모드가 아니고 깊이가 1 미만이면 하드웨어 커서를 보인다.
    showCursor(show) {
        this.#cursorHidden += show ? -1 : 1;
        if (this.#devices.cursor) this.#devices.cursor((this.#flags & ScreenMode.softwareMouse) === 0 && this.#cursorHidden < 1);
    }

    // 004a4fc0 은 중첩 깊이를 1 로 둔 뒤 한 번 표시해 0 으로 맞춘다.
    restoreMovieCursor() {
        this.#cursorHidden = 1;
        this.showCursor(true);
    }

    // 소프트웨어 커서도 현재 원본 숨김 깊이를 따른다.
    cursorVisible() {
        return this.#cursorHidden < 1;
    }

    // 현재 화면 모드 플래그이다.
    get flags() {
        return this.#flags;
    }

    // 논리 화면의 가로 크기이다.
    get width() {
        return this.#width;
    }

    // 논리 화면의 세로 크기이다.
    get height() {
        return this.#height;
    }

    // 그리기 표면의 줄 간격(DAT_005c78f0)이다: DIB 가 있으면 4바이트 정렬, 없으면 폭이다.
    get pitch() {
        return this.#dibSurface ? this.#dibSurface.pitch : this.#width;
    }

    // 현재 자르기 영역(양 끝 포함 — VFX pane 좌표, DAT_005b5de0~ec)이다.
    pane() {
        return { left: this.#clip.left, top: this.#clip.top, right: this.#clip.right - 1, bottom: this.#clip.bottom - 1 };
    }

    // 창 테두리의 왼쪽·위쪽 두께이다. 웹은 항상 0 이다.
    borderOffset() {
        return { ...this.#border };
    }
}
