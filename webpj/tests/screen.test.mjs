// 화면 장치 screen.js(Screen·ScreenSurface·ScreenMode와 호환 출력 계산)를 검사한다(cpppj `ScreenTests`·`ScreenScrollTests`·`ScreenPresentationTests` 이식 + 웹 계약).
//  - 이식: ScreenPalette_NearestColorMatchesThreeNativeBodies(원본 x86 관측 `palettecolor-x86.tsv` 786행의 색 검색 전체 몸체), ScreenPalette_PreparedLogicalColorsAndWeatherTable,
//    ScreenMode_LegalCombinations_FollowOriginalRules, ScreenRect_Clip_ClampsEachCoordinate, ScreenScroll_MatchesNativeDibPixels(원본 x86 관측 `screen-scroll-x86.tsv` 357행),
//    ScreenScroll_AllDirectionsPreserveSourceAndPadding, ScreenScroll_SelectedSurfaceBounds, ScreenPresentation_AspectFitAndLetterbox, ScreenPresentation_InversePointAndDirtyPhase,
//    ScreenPresentation_CompatibilityFlags(ModernScreenMode 부분), ScreenPresentation_WideDibUsesActualAlignedPitch, ScreenPresentation_IntegerNearestPixelsAndBounds.
//    GDI 객체 누수(ScreenPalette_ReplacementsAndDestructionReleaseGdiObjects)·창 위치(Screen_InitialWindowPosition)·게임 메뉴 명령 번호·카메라 적용 연동(ScreenScroll_CameraUses…, RawCameraCommit 단위 뒤)은 옮기지 않았다.
//  - 웹 계약: 표면 생성/오류, 잠금 깊이, 채우기·지우기의 모드/잠금/clip/표면 밖 거부, 구름 buffer 복사(blitBuffer), 팔레트 설정·적재·clear 관문·색 표·명암 표 연결, 모드 전환, 출력(update) 분기, 커서 숨김 깊이.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E393: 5-A Screen·ScreenPresentation 이식으로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { GamePalette, buildShadeMaps, findPaletteColor } from '../www/assets/palette.js';
import { buildPaletteColorTable } from '../www/core/palette-colors.js';
import {
    Screen, ScreenMode, ScreenSurface, clipScreenRect, fitScreenPresentation, mapScreenPresentationPoint, mapScreenPresentationRect,
    modernScreenMode, scaleScreenPresentation, screenModeIsLegal, scrollScreenPixels,
} from '../www/render/screen.js';

// 16진 문자열(2글자=1바이트)을 바이트 배열로 바꾼다. (2026-11-13 WB04-E393)
function bytesOf(text) {
    const bytes = new Uint8Array(text.length / 2);
    // 두 글자마다 한 바이트이다.
    for (let index = 0; index < bytes.length; index += 1) bytes[index] = parseInt(text.slice(index * 2, index * 2 + 2), 16);
    return bytes;
}

// 예외가 나는지 본다. (2026-11-13 WB04-E393)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 모드가 정해진 640×480 화면이다: init → initDibSection → setMode(10 = 창 DIB+구름). 표시 훅의 호출을 log 에 쌓는다. (2026-11-13 WB04-E393)
function makeScreen(width = 640, height = 480, flags = ScreenMode.fallbackWindowed, log = []) {
    const screen = new Screen(width, height, {
        devices: {
            present: (rect, view) => log.push(['present', rect.left, rect.top, rect.right, rect.bottom, view.pitch]),
            presentFullScreen: (output) => log.push(['full', output.left, output.top, output.right, output.bottom]),
            paletteChanged: () => log.push(['palette']),
            waitVerticalBlank: () => log.push(['vblank']),
            cursor: (visible) => log.push(['cursor', visible]),
        },
    });
    screen.init();
    screen.initDibSection();
    assert.equal(screen.setMode(flags), true);
    log.length = 0;
    return screen;
}

// 0..255 번호마다 서로 다른 RGB 를 갖는 파일 팔레트이다(번호 i → R=i, G=255-i, B=(i*7)&255). (2026-11-13 WB04-E393)
function gradientPalette() {
    const rgb = new Uint8Array(768);
    // 번호마다 세 채널을 채운다.
    for (let index = 0; index < 256; index += 1) rgb.set([index, 255 - index, (index * 7) & 255], index * 3);
    return new GamePalette(rgb);
}

const colorRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'palettecolor-x86.tsv'));
const scrollRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'screen-scroll-x86.tsv'));

test('ScreenPalette_NearestColorMatchesThreeNativeBodies: 원본 x86 관측 786행(범위 밖 RGB 포함)의 색 검색 결과가 모두 일치한다', { skip: colorRows === null }, () => {
    let cases = 0;
    // 관측 행마다 논리 팔레트의 little-endian DWORD 256개를 복원한다.
    for (const row of colorRows) {
        assert.equal(row.length, 6, row.join(' '));
        assert.equal(row[4].length, 2048);
        const bytes = bytesOf(row[4]);
        const palette = new Uint32Array(256);
        // DWORD 마다 네 바이트를 원본 순서로 합친다. 최상위 플래그도 유지한다.
        for (let index = 0; index < 256; index += 1) palette[index] = (bytes[index * 4] | (bytes[index * 4 + 1] << 8) | (bytes[index * 4 + 2] << 16) | (bytes[index * 4 + 3] << 24)) >>> 0;
        assert.equal(findPaletteColor(palette, Number(row[1]), Number(row[2]), Number(row[3])), Number(row[5]), row.slice(0, 4).join(' '));
        cases += 1;
    }
    assert.equal(cases, 786);
});

test('ScreenPalette_PreparedLogicalColorsAndWeatherTable: apply=false 로 준비한 논리 팔레트가 색 검색과 날씨 색에 쓰이고 표시 팔레트는 바뀌지 않는다', () => {
    const screen = new Screen(640, 480);
    const colors = new Uint8Array(1024);
    // ScreenColor{파랑, 초록, 빨강, 예약} 네 항목이다.
    colors.set([22, 255, 255, 193], 17 * 4);
    colors.set([255, 22, 22, 147], 38 * 4);
    colors.set([22, 22, 255, 91], 69 * 4);
    colors.set([111, 140, 181, 255], 127 * 4);
    screen.setPalette(0, 256, colors, false);
    assert.deepEqual(screen.weatherTints(), [17, 38, 69, 127]);
    assert.equal(screen.findColor(0, 0, 0), 0);
    assert.equal(screen.findColor(255, 255, 255), 255);
    // 표시 버퍼는 적용하지 않았으므로 여전히 검정이다.
    assert.equal(screen.palette()[17 * 4 + 2], 0);
    assert.equal(screen.palette()[17 * 4 + 1], 0);
    // 논리 팔레트는 R|G<<8|B<<16|플래그<<24 이며 17번의 플래그는 4 이다.
    assert.equal(screen.logicalPalette()[17], (255 | (255 << 8) | (22 << 16) | (4 << 24)) >>> 0);
});

test('ScreenMode_LegalCombinations_FollowOriginalRules: 원본 004a13d0 의 다섯 조건과 설정 기본값 10(창)·11(전체화면)', () => {
    assert.ok(screenModeIsLegal(ScreenMode.fallbackWindowed, false));
    assert.equal(ScreenMode.fallbackWindowed, 10);
    // 전체화면은 DirectDraw 가 있어야 한다.
    assert.ok(!screenModeIsLegal(ScreenMode.fullScreen, false));
    assert.ok(screenModeIsLegal(ScreenMode.fullScreen, true));
    assert.ok(screenModeIsLegal(ScreenMode.fullScreen | ScreenMode.flipping, true));
    // workingFullScreenFlags = 11(전체화면 + DIB + 구름).
    assert.ok(screenModeIsLegal(11, true) && !screenModeIsLegal(11, false));
    // 플리핑은 전체화면에서만.
    assert.ok(!screenModeIsLegal(ScreenMode.backBufferDib | ScreenMode.flipping, true));
    // 시차·움직임은 구름이 있어야 한다.
    assert.ok(!screenModeIsLegal(ScreenMode.backBufferDib | ScreenMode.parallax, true));
    assert.ok(!screenModeIsLegal(ScreenMode.backBufferDib | ScreenMode.animating, true));
    assert.ok(screenModeIsLegal(ScreenMode.backBufferDib | ScreenMode.clouds | ScreenMode.parallax | ScreenMode.animating, false));
    // 전체화면도 DIB 도 아니면 그릴 곳이 없다.
    assert.ok(!screenModeIsLegal(0, true));
    assert.ok(!screenModeIsLegal(ScreenMode.clouds | ScreenMode.modeSet, true));
});

test('ScreenRect_Clip_ClampsEachCoordinate: 네 좌표를 각각 경계 안으로 넣고 뒤집힌 사각형은 바로잡지 않는다', () => {
    const bounds = { left: 0, top: 0, right: 640, bottom: 480 };
    assert.deepEqual(clipScreenRect({ left: 10, top: 20, right: 30, bottom: 40 }, bounds), { left: 10, top: 20, right: 30, bottom: 40 });
    assert.deepEqual(clipScreenRect({ left: -5, top: -7, right: 700, bottom: 500 }, bounds), bounds);
    // 완전히 밖이면 폭·높이가 0 인 사각형이 된다.
    assert.deepEqual(clipScreenRect({ left: 650, top: 490, right: 700, bottom: 500 }, bounds), { left: 640, top: 480, right: 640, bottom: 480 });
    const reversed = clipScreenRect({ left: 30, top: 40, right: 10, bottom: 20 }, bounds);
    assert.equal(reversed.left, 30);
    assert.equal(reversed.right, 10);
});

test('ScreenScroll_MatchesNativeDibPixels: 원본 x86 관측 357행의 전체 144바이트(padding 포함)가 모두 일치한다', { skip: scrollRows === null }, () => {
    let count = 0;
    // 각 원문 관측에 같은 독립 초기 버퍼를 공급한다.
    for (const row of scrollRows) {
        const [dx, dy] = [Number(row[0]), Number(row[1])];
        assert.equal(row[2].length, 288, row.join(' '));
        const pixels = new Uint8Array(144);
        // 원문 입력만 만들며 기대 이동은 계산하지 않는다.
        for (let index = 0; index < pixels.length; index += 1) pixels[index] = index;
        scrollScreenPixels(pixels, 16, 13, 9, 2, 1, 9, 7, dx, dy);
        assert.deepEqual(Array.from(pixels), Array.from(bytesOf(row[2])), `dx=${dx} dy=${dy}`);
        count += 1;
    }
    assert.equal(count, 357);
});

test('ScreenScroll_AllDirectionsPreserveSourceAndPadding: 모든 방향·범위 밖 이동에서 목적지 각 픽셀을 변경 전 독립 좌표로 대조한다', () => {
    const [pitch, width, height, x, y, w, h] = [16, 13, 9, 2, 1, 9, 7];
    const original = new Uint8Array(pitch * height);
    // 각 원본 픽셀을 서로 다른 번호로 구별한다.
    for (let index = 0; index < original.length; index += 1) original[index] = index;
    // 위/아래·범위 밖 이동과 0 을 함께 검사한다.
    for (let dy = -8; dy <= 8; dy += 1) {
        // 좌/우·범위 밖과 대각선의 겹침을 교차한다.
        for (let dx = -10; dx <= 10; dx += 1) {
            const actual = original.slice();
            scrollScreenPixels(actual, pitch, width, height, x, y, w, h, dx, dy);
            // 이동 범위 밖과 padding 도 변경되지 않아야 한다.
            for (let row = 0; row < height; row += 1) {
                for (let column = 0; column < pitch; column += 1) {
                    const copied = Math.abs(dx) < w && Math.abs(dy) < h && column >= x && column < x + w && row >= y && row < y + h
                        && column + dx >= x && column + dx < x + w && row + dy >= y && row + dy < y + h;
                    assert.equal(actual[row * pitch + column], original[copied ? (row + dy) * pitch + column + dx : row * pitch + column], `dx=${dx} dy=${dy} (${column},${row})`);
                }
            }
        }
    }
});

test('ScreenScroll_SelectedSurfaceBounds: 화면 밖 메모리는 쓰기 전에 거부하고 0/겹침 없는 이동은 원문처럼 반환한다', () => {
    const pixels = new Uint8Array(64);
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 8, -1, 0, 5, 5, 1, 1), RangeError);
    scrollScreenPixels(new Uint8Array(0), 8, 8, 8, -1, 0, 5, 5, 0, 0);
    scrollScreenPixels(new Uint8Array(0), 8, 8, 8, 0, 0, 5, 5, 5, 0);
    // 영역이 화면을 넘거나 버퍼가 모자라거나 줄 간격이 폭보다 작거나 크기가 0 이하이면 거부한다(각각 따로).
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 8, 4, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 8, 0, 4, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(new Uint8Array(63), 8, 8, 8, 0, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 7, 8, 8, 0, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 8, 0, 8, 0, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 0, 0, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 8, -1, 0, 5, 5, 1, 0), RangeError);
    assert.throws(() => scrollScreenPixels(pixels, 8, 8, 8, 0, -1, 5, 5, 1, 0), RangeError);
});

test('ScreenPresentation_AspectFitAndLetterbox: 4:3·와이드 논리 화면이 4:3·16:9·16:10 출력에서 늘어나지 않고 잘못된 크기는 거부한다', () => {
    assert.deepEqual(fitScreenPresentation(1024, 768, 1920, 1080), { left: 240, top: 0, right: 1680, bottom: 1080 });
    assert.deepEqual(fitScreenPresentation(1280, 720, 1600, 1200), { left: 0, top: 150, right: 1600, bottom: 1050 });
    assert.deepEqual(fitScreenPresentation(1280, 800, 1920, 1080), { left: 96, top: 0, right: 1824, bottom: 1080 });
    assert.deepEqual(fitScreenPresentation(1024, 768, 640, 480), { left: 0, top: 0, right: 640, bottom: 480 });
    assert.deepEqual(fitScreenPresentation(1366, 768, 1920, 1080), { left: 0, top: 0, right: 1920, bottom: 1079 });
    assert.throws(() => fitScreenPresentation(0, 768, 1920, 1080), RangeError);
    assert.throws(() => fitScreenPresentation(1024, 0, 1920, 1080), RangeError);
    assert.throws(() => fitScreenPresentation(1024, 768, 0, 1080), RangeError);
    assert.throws(() => fitScreenPresentation(1024, 768, 1920, 0), RangeError);
    // 매우 작은 클라이언트도 최소 1 픽셀이다.
    const tiny = fitScreenPresentation(1024, 768, 1, 1);
    assert.equal(tiny.right, 1);
    assert.equal(tiny.bottom, 1);
});

test('ScreenPresentation_InversePointAndDirtyPhase: 확대된 실제 중앙 클릭을 논리 입력으로 되돌리고 변경 사각형은 바깥쪽으로 반올림한다', () => {
    const output = { left: 240, top: 0, right: 1680, bottom: 1080 };
    assert.deepEqual(mapScreenPresentationPoint({ x: 960, y: 540 }, output, 1024, 768), { x: 512, y: 384 });
    assert.deepEqual(mapScreenPresentationPoint({ x: -200, y: -40 }, output, 1024, 768), { x: 0, y: 0 });
    assert.deepEqual(mapScreenPresentationPoint({ x: 1920, y: 1080 }, output, 1024, 768), { x: 1024, y: 768 });
    assert.deepEqual(mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, output, 1024, 768), { left: 241, top: 1, right: 243, bottom: 3 });
    assert.deepEqual(mapScreenPresentationRect({ left: -10, top: -10, right: 2000, bottom: 1000 }, output, 1024, 768), output);
    // 출력 범위가 비었거나 논리 크기가 0 이하이면 거부한다.
    assert.throws(() => mapScreenPresentationPoint({ x: 1, y: 1 }, { left: 5, top: 0, right: 5, bottom: 9 }, 1024, 768), RangeError);
    assert.throws(() => mapScreenPresentationPoint({ x: 1, y: 1 }, { left: 0, top: 5, right: 9, bottom: 5 }, 1024, 768), RangeError);
    assert.throws(() => mapScreenPresentationPoint({ x: 1, y: 1 }, output, 0, 768), RangeError);
    assert.throws(() => mapScreenPresentationPoint({ x: 1, y: 1 }, output, 1024, 0), RangeError);
    assert.throws(() => mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, { left: 5, top: 0, right: 5, bottom: 9 }, 1024, 768), RangeError);
    assert.throws(() => mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, { left: 0, top: 5, right: 9, bottom: 5 }, 1024, 768), RangeError);
    assert.throws(() => mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, output, 0, 768), RangeError);
    assert.throws(() => mapScreenPresentationRect({ left: 1, top: 1, right: 2, bottom: 2 }, output, 1024, 0), RangeError);
});

test('ScreenPresentation_CompatibilityFlags: 호환 모드는 실제 DIB/비플립을 공개하며 원본 합법성 판정의 DirectDraw 조건은 바꾸지 않는다', () => {
    assert.equal(modernScreenMode(111), 235);
    assert.equal(modernScreenMode(139), 139);
    assert.equal(modernScreenMode(0), 130);
    assert.equal(modernScreenMode(94), 138);
    assert.ok(!screenModeIsLegal(111, false));
    assert.ok(screenModeIsLegal(modernScreenMode(111), true));
    // 결과는 부호 없는 수이다.
    assert.equal(modernScreenMode(0xffffffff) >= 0, true);
});

test('ScreenPresentation_WideDibUsesActualAlignedPitch: 1366 폭의 DIB 는 행마다 1368바이트이므로 끝 행 쓰기가 옆 행과 섞이지 않는다', () => {
    const surface = new ScreenSurface('dib', 1366, 768);
    assert.equal(surface.pitch, 1368);
    const bytes = surface.lock();
    assert.ok(bytes);
    bytes[1365] = 17;
    bytes[1368] = 31;
    bytes[1368 * 767 + 1365] = 47;
    assert.deepEqual([bytes[1365], bytes[1368], bytes[1368 * 767 + 1365]], [17, 31, 47]);
    assert.equal(bytes.length, 1368 * 768);
    surface.unlock();
    // 창 표면은 픽셀이 없고 폭이 줄 간격이다.
    const window = new ScreenSurface('window', 1366, 768);
    assert.equal(window.pitch, 1366);
    assert.equal(window.lock(), null);
    assert.equal(window.pixels, null);
    assert.equal(new ScreenSurface('dib', 1024, 768).pitch, 1024);
    assert.equal(new ScreenSurface('dib', 1025, 8).pitch, 1028);
    assert.throws(() => new ScreenSurface('dib', 0, 8), RangeError);
    assert.throws(() => new ScreenSurface('dib', 8, 0), RangeError);
    assert.throws(() => new ScreenSurface('dib', 8.5, 8), RangeError);
    assert.throws(() => new ScreenSurface('bogus', 8, 8), RangeError);
});

test('ScreenPresentation_IntegerNearestPixelsAndBounds: 확대/축소의 중앙 표본·실제 행 간격·padding 보존과 범위 거부', () => {
    const source = Uint8Array.of(1, 2, 77, 77, 3, 4, 77, 77);
    const output = new Uint8Array(12).fill(99);
    scaleScreenPresentation(source, 4, 2, 2, output, 4, 3, 3);
    assert.deepEqual(Array.from(output), [1, 2, 2, 99, 3, 4, 4, 99, 3, 4, 4, 99]);
    const large = Uint8Array.of(0, 1, 2, 3, 77, 4, 5, 6, 7, 77, 8, 9, 10, 11, 77);
    const small = new Uint8Array(8).fill(99);
    scaleScreenPresentation(large, 5, 4, 3, small, 4, 3, 2);
    assert.deepEqual(Array.from(small), [0, 2, 3, 99, 8, 10, 11, 99]);
    assert.throws(() => scaleScreenPresentation(source.subarray(0, 5), 4, 2, 2, output, 4, 3, 3), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 4, 2, 2, output.subarray(0, 9), 4, 3, 3), RangeError);
    // 크기·줄 간격이 잘못되면 각각 거부한다.
    assert.throws(() => scaleScreenPresentation(source, 4, 0, 2, output, 4, 3, 3), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 4, 2, 0, output, 4, 3, 3), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 4, 2, 2, output, 4, 0, 3), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 4, 2, 2, output, 4, 3, 0), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 1, 2, 2, output, 4, 3, 3), RangeError);
    assert.throws(() => scaleScreenPresentation(source, 4, 2, 2, output, 2, 3, 3), RangeError);
});

test('Screen 생성·모드 전환: 크기 오류 거부, 표면이 없으면 모드를 정하지 못하고, 같은 모드는 장치를 건드리지 않으며, 바뀌면 현재 팔레트를 저장 팔레트로 복사한다', () => {
    assert.throws(() => new Screen(0, 480), RangeError);
    assert.throws(() => new Screen(640, 0), RangeError);
    assert.throws(() => new Screen(640.5, 480), RangeError);
    const log = [];
    const screen = new Screen(640, 480, { devices: { cursor: (visible) => log.push(['cursor', visible]), paletteChanged: () => log.push(['palette']) } });
    // init 이전(창 표면 없음)과 initDibSection 이전(DIB 없음)에는 모드를 정할 수 없다.
    assert.equal(screen.setMode(10), false);
    screen.init();
    assert.equal(screen.setMode(10), false);
    screen.initDibSection();
    assert.equal(screen.flags, 0);
    assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 0, bottom: 0 });
    // 허용되지 않는 조합(플리핑만)은 정규화 뒤에도 DIB 가 켜지므로 합법이 되고, 전체화면+구름 없는 시차는 정규화로도 막히지 않는다.
    assert.equal(screen.setMode(10), true);
    assert.equal(screen.flags, 138);
    // 커서는 숨김 → 표시 → 표시 순으로 깊이 -1 에 도달한다.
    assert.deepEqual(log.filter((entry) => entry[0] === 'cursor'), [['cursor', false], ['cursor', true], ['cursor', true]]);
    assert.ok(screen.cursorVisible());
    log.length = 0;
    // 같은 (정규화된) 모드는 아무 장치도 건드리지 않고 잠금 깊이만 0 으로 되돌린다.
    screen.lock();
    assert.equal(screen.setMode(138), true);
    assert.deepEqual(log, []);
    assert.equal(screen.lock() !== null, true);
    screen.unlock();
    // 전체화면 모드(11 → 139)로 바뀌면 활성 훅이 없을 때 활성으로 두고 팔레트를 다시 적용한다.
    assert.equal(screen.setMode(11), true);
    assert.equal(screen.flags, 139);
    assert.equal(screen.active, true);
    assert.ok(log.some((entry) => entry[0] === 'palette'));
    // 활성 훅이 있으면 그 값을 따른다.
    const inactive = new Screen(640, 480, { devices: { foreground: () => false } });
    inactive.init();
    inactive.initDibSection();
    assert.equal(inactive.setMode(10), true);
    assert.equal(inactive.active, false);
    // DIB 도 전체화면도 아닌 값은 정규화 때문에 늘 DIB 가 켜져 합법이다: 0 → 130.
    const zero = new Screen(64, 64);
    zero.init();
    zero.initDibSection();
    assert.equal(zero.setMode(0), true);
    assert.equal(zero.flags, 130);
});

test('Screen 잠금: 표면이 연결되기 전에는 거부하고, 중첩 깊이를 세어 처음 잠글 때만 버퍼를 얻고 마지막에 푼다', () => {
    const screen = new Screen(16, 8);
    screen.init();
    screen.initDibSection();
    assert.throws(() => screen.lock(), /drawSurf/);
    assert.equal(screen.setMode(10), true);
    const first = screen.lock();
    assert.ok(first instanceof Uint8Array);
    assert.equal(screen.lock(), first);
    // 안쪽 잠금을 풀어도 버퍼는 남아 있어 채우기가 동작한다.
    screen.unlock();
    screen.setClip(0, 0, 16, 8);
    screen.fillRect({ left: 0, top: 0, right: 2, bottom: 1 }, 5);
    assert.deepEqual(Array.from(first.subarray(0, 3)), [5, 5, 0]);
    screen.unlock();
    // 마지막 잠금이 풀리면 버퍼가 없어 채우기는 아무 일도 하지 않는다.
    screen.fillRect({ left: 0, top: 0, right: 4, bottom: 1 }, 9);
    assert.deepEqual(Array.from(first.subarray(0, 4)), [5, 5, 0, 0]);
});

test('Screen 채우기·지우기: 모드 설정 비트와 잠금이 있어야 하고 현재 clip 으로 자르며 줄 간격 padding 과 표면 밖으로 쓰지 않는다', () => {
    const screen = new Screen(6, 4);
    screen.init();
    screen.initDibSection();
    // initDibSection 은 잠근 버퍼를 얻어 두지만 모드 플래그가 0 이면 채우지 않는다(같은 DIB 이므로 모드 설정 뒤 전부 0 이어야 한다).
    screen.setClip(0, 0, 6, 4);
    screen.fillRect({ left: 0, top: 0, right: 6, bottom: 4 }, 3);
    assert.equal(screen.setMode(10), true);
    assert.ok(screen.lock().every((value) => value === 0));
    screen.setClip(1, 1, 5, 3);
    // 요청한 사각형이 clip 보다 커도 clip 안쪽 (1..5)×(1..3)만 채운다. 폭 6 의 줄 간격은 8 이다.
    screen.fillRect({ left: 0, top: 0, right: 6, bottom: 4 }, 7);
    const pixels = screen.lock();
    screen.unlock();
    const rows = [];
    // 줄 간격 8 의 각 행에서 처음 8칸(padding 2 포함)을 읽는다.
    for (let row = 0; row < 4; row += 1) rows.push(Array.from(pixels.subarray(row * 8, row * 8 + 8)));
    assert.deepEqual(rows, [[0, 0, 0, 0, 0, 0, 0, 0], [0, 7, 7, 7, 7, 0, 0, 0], [0, 7, 7, 7, 7, 0, 0, 0], [0, 0, 0, 0, 0, 0, 0, 0]]);
    screen.unlock();
    // clip 이 표면보다 크면 표면 밖 쓰기는 거부한다.
    screen.lock();
    screen.setClip(0, 0, 100, 100);
    assert.throws(() => screen.fillRect({ left: 0, top: 0, right: 100, bottom: 1 }, 1), RangeError);
    assert.throws(() => screen.fillRect({ left: 0, top: 0, right: 1, bottom: 100 }, 1), RangeError);
    // clip 왼쪽/위가 음수이면 음수 열/행 쓰기도 거부한다(TypedArray 의 음수 인덱스가 끝에서 세는 것을 막는다).
    screen.setClip(-5, -5, 100, 100);
    assert.throws(() => screen.fillRect({ left: -3, top: 0, right: 1, bottom: 1 }, 1), RangeError);
    assert.throws(() => screen.fillRect({ left: 0, top: -2, right: 1, bottom: 1 }, 1), RangeError);
    screen.unlock();
    // clear: 잠그고 채우고 푼다. 예외가 나도 잠금은 풀린다.
    screen.setClip(0, 0, 6, 4);
    screen.clear({ left: 0, top: 0, right: 2, bottom: 1 }, 9);
    assert.deepEqual(Array.from(pixels.subarray(0, 3)), [9, 9, 0]);
    screen.setClip(0, 0, 100, 100);
    assert.throws(() => screen.clear({ left: 0, top: 0, right: 100, bottom: 1 }, 1), RangeError);
    screen.setClip(0, 0, 6, 4);
    screen.clear({ left: 2, top: 0, right: 3, bottom: 1 }, 4);
    assert.equal(pixels[2], 4);
    // 모드 플래그가 0 이면 지우기는 잠금조차 하지 않는다.
    const unset = new Screen(6, 4);
    unset.init();
    unset.initDibSection();
    unset.clear({ left: 0, top: 0, right: 6, bottom: 4 }, 9);
});

test('Screen 구름 buffer 복사(blitBuffer): 대상 clip 으로 자르고 4바이트씩 읽은 뒤 쓰며, 잠금이 없거나 물리 범위를 넘으면 거부한다', () => {
    const screen = makeScreen(8, 4);
    const source = Uint8Array.from({ length: 24 }, (_, index) => index + 1);
    // 잠금 없이 복사하면 거부한다(마지막 잠금이 풀리면 버퍼가 없다).
    screen.lock();
    screen.unlock();
    assert.throws(() => screen.blitBuffer(0, 0, source, 6, { left: 0, top: 0, right: 3, bottom: 2 }), /잠금 누락/);
    const pixels = screen.lock();
    screen.setClip(0, 0, 8, 4);
    // 6 폭 소스의 (1,1) 에서 5×2 를 대상 (2,1) 에 복사한다.
    screen.blitBuffer(2, 1, source, 6, { left: 1, top: 1, right: 6, bottom: 3 });
    const row = (index) => Array.from(pixels.subarray(index * 8, index * 8 + 8));
    assert.deepEqual(row(1), [0, 0, 8, 9, 10, 11, 12, 0]);
    assert.deepEqual(row(2), [0, 0, 14, 15, 16, 17, 18, 0]);
    // 현재 clip 으로 대상이 잘리면 소스 시작도 같은 만큼 이동한다: clip 왼쪽 3 → 대상 열 2 는 건너뛰고 소스 열도 하나 건너뛴다.
    pixels.fill(0);
    screen.setClip(3, 0, 8, 4);
    screen.blitBuffer(2, 1, source, 6, { left: 1, top: 1, right: 4, bottom: 2 });
    assert.deepEqual(row(1), [0, 0, 0, 9, 10, 0, 0, 0]);
    // 화면 밖으로 완전히 나가면 아무것도 쓰지 않는다.
    pixels.fill(0);
    screen.setClip(0, 0, 8, 4);
    screen.blitBuffer(20, 20, source, 6, { left: 0, top: 0, right: 3, bottom: 2 });
    assert.ok(pixels.every((value) => value === 0));
    // 소스가 모자라면 거부한다.
    assert.throws(() => screen.blitBuffer(0, 0, source.subarray(0, 4), 6, { left: 0, top: 0, right: 3, bottom: 2 }), RangeError);
    assert.throws(() => screen.blitBuffer(0, 0, source, -1, { left: 0, top: 0, right: 3, bottom: 2 }), RangeError);
    // 거꾸로 된 소스 사각형(폭 음수)은 대상이 비어 있어 거부하지 않고 아무것도 쓰지 않는다.
    pixels.fill(0);
    screen.blitBuffer(0, 0, source, 6, { left: 3, top: 0, right: 0, bottom: 2 });
    assert.ok(pixels.every((value) => value === 0));
    // 같은 버퍼의 겹친 영역은 한 워드씩 먼저 읽은 뒤 쓴다: 앞으로 한 칸 이동하는 겹침.
    pixels.set([1, 2, 3, 4, 5, 6, 7, 8], 0);
    screen.blitBuffer(1, 0, pixels, 8, { left: 0, top: 0, right: 7, bottom: 1 });
    // 첫 네 칸은 읽은 뒤 쓰므로 [1,2,3,4] 가 한 칸 뒤로 가고, 마지막 세 칸은 한 칸씩 읽고 쓰므로 방금 쓴 4 가 번진다.
    assert.deepEqual(row(0), [1, 1, 2, 3, 4, 4, 4, 4]);
    screen.unlock();
});

test('Screen 팔레트 설정(setPalette): 논리 팔레트는 apply 와 무관하게 갱신하고 0/255 를 고정하며, apply 일 때만 현재 팔레트에 복사하고 부분 범위·count 0 을 따른다', () => {
    const log = [];
    const screen = makeScreen(640, 480, ScreenMode.fallbackWindowed, log);
    const colors = new Uint8Array(1024);
    // 5번 항목: 파랑 30, 초록 20, 빨강 10, 예약 99.
    colors.set([30, 20, 10, 99], 5 * 4);
    colors.set([1, 2, 3, 4], 0);
    colors.set([5, 6, 7, 8], 255 * 4);
    screen.setPalette(0, 256, colors, true);
    // 0번과 255번은 인자 배열에서도 고정값으로 덮어쓴다.
    assert.deepEqual(Array.from(colors.subarray(0, 4)), [0, 0, 0, 0]);
    assert.deepEqual(Array.from(colors.subarray(1020, 1024)), [255, 255, 255, 0]);
    assert.equal(screen.logicalPalette()[0], 0);
    assert.equal(screen.logicalPalette()[255], 0x00ffffff);
    assert.equal(screen.logicalPalette()[5], (10 | (20 << 8) | (30 << 16) | (4 << 24)) >>> 0);
    assert.deepEqual(Array.from(screen.palette().subarray(20, 24)), [30, 20, 10, 99]);
    assert.deepEqual(log, [['palette']]);
    // 부분 범위(10..12)는 start 가 0 이 아니면 인자의 첫 칸을 덮어쓰지 않고, 끝이 256 이 아니면 끝 칸도 건드리지 않는다.
    const partial = Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8);
    screen.setPalette(10, 2, partial, true);
    assert.deepEqual(Array.from(partial), [1, 2, 3, 4, 5, 6, 7, 8]);
    assert.deepEqual(Array.from(screen.palette().subarray(40, 48)), [1, 2, 3, 4, 5, 6, 7, 8]);
    // 끝이 256 이면 start 가 0 이 아니어도 마지막 칸을 흰색으로 덮는다.
    const tail = Uint8Array.of(9, 9, 9, 9, 9, 9, 9, 9);
    screen.setPalette(254, 2, tail, true);
    assert.deepEqual(Array.from(tail), [9, 9, 9, 9, 255, 255, 255, 0]);
    // apply 가 아니면 현재 팔레트는 그대로이다.
    log.length = 0;
    const quiet = Uint8Array.of(70, 80, 90, 0);
    screen.setPalette(20, 1, quiet, false);
    assert.deepEqual(log, []);
    assert.deepEqual(Array.from(screen.palette().subarray(80, 84)), [0, 0, 0, 0]);
    assert.equal(screen.logicalPalette()[20], (90 | (80 << 8) | (70 << 16) | (4 << 24)) >>> 0);
    // count 0 + apply 는 인자를 전체 팔레트로 보고 0번부터 256칸을 복사한다(색 배열이 모자라면 거부).
    assert.throws(() => screen.setPalette(0, 0, new Uint8Array(8), true), RangeError);
    const whole = new Uint8Array(1024).fill(6);
    screen.setPalette(0, 0, whole, true);
    assert.deepEqual(Array.from(screen.palette().subarray(40, 44)), [6, 6, 6, 6]);
    // 잘못된 범위는 거부한다.
    assert.throws(() => screen.setPalette(-1, 1, new Uint8Array(4), false), RangeError);
    assert.throws(() => screen.setPalette(0, -1, new Uint8Array(4), false), RangeError);
    assert.throws(() => screen.setPalette(0.5, 1, new Uint8Array(4), false), RangeError);
    assert.throws(() => screen.setPalette(0, 2, new Uint8Array(4), false), RangeError);
    // 전체화면 모드의 적용은 화면 전체를 다시 낸다.
    const full = makeScreen(640, 480, 11, log);
    log.length = 0;
    full.setPalette(0, 256, new Uint8Array(1024), true);
    assert.deepEqual(log.map((entry) => entry[0]), ['palette', 'full']);
});

test('Screen 팔레트 적재(loadPalette): 저장 팔레트에 BGRX 로 넣고 적용하며 loaded 상자·색 표·명암 표를 만들고, 조회기가 연결되면 그 값을 쓴다', () => {
    const log = [];
    const loaded = { value: 77 };
    const screen = new Screen(16, 8, { paletteLoaded: loaded, devices: { paletteChanged: () => log.push('palette') } });
    screen.init();
    screen.initDibSection();
    assert.equal(screen.setMode(10), true);
    log.length = 0;
    // 적재 전 색 표는 0, 명암 표는 없다.
    assert.deepEqual(screen.colors().basic, new Array(9).fill(0));
    assert.deepEqual(screen.colors().named, []);
    assert.equal(screen.shadeMaps(), null);
    assert.equal(screen.paletteLoaded, loaded);
    const palette = gradientPalette();
    screen.loadPalette(palette);
    assert.equal(loaded.value, 1);
    assert.deepEqual(log, ['palette']);
    // 파일 색이 BGRX 로 현재 팔레트에 들어가고 0/255 는 고정된다.
    assert.deepEqual(Array.from(screen.palette().subarray(40, 44)), [(10 * 7) & 255, 245, 10, 0]);
    assert.deepEqual(Array.from(screen.palette().subarray(0, 4)), [0, 0, 0, 0]);
    assert.deepEqual(Array.from(screen.palette().subarray(1020, 1024)), [255, 255, 255, 0]);
    // 색 표·명암 표는 적용된 논리 팔레트에서 계산한다.
    assert.deepEqual(screen.colors(), buildPaletteColorTable(screen.logicalPalette()));
    assert.deepEqual(screen.shadeMaps(), buildShadeMaps(screen.logicalPalette()));
    // 번개처럼 일시적인 setPalette 는 색 표를 다시 계산하지 않는다.
    const before = screen.colors();
    screen.setPalette(0, 256, new Uint8Array(1024), true);
    assert.equal(screen.colors(), before);
    // 고정 색 정렬 입력 조회기가 있으면 명암 표를 그 팔레트로 만든다.
    const sort = new Uint32Array(256);
    for (let index = 0; index < 256; index += 1) sort[index] = (index | (index << 8) | (index << 16)) >>> 0;
    screen.bindColorSortInput(() => sort);
    screen.loadPalette(palette);
    assert.deepEqual(screen.shadeMaps(), buildShadeMaps(sort));
    // 현재 9표 조회기가 연결되면 연결 즉시와 매 조회에서 그 값을 쓴다.
    let maps = { bright: Uint8Array.of(1), dark: Uint8Array.of(2) };
    screen.bindColorMaps(() => maps);
    assert.equal(screen.shadeMaps(), maps);
    maps = { bright: Uint8Array.of(3), dark: Uint8Array.of(4) };
    assert.equal(screen.shadeMaps(), maps);
    // 조회기 없이 연결하면 거부한다.
    assert.throws(() => screen.bindColorMaps(null), TypeError);
    assert.throws(() => screen.bindColorSortInput(null), TypeError);
});

test('Screen 팔레트 적재의 clear 관문: 지우기가 켜져 있으면 잠금 → 검정 채우기 → 풀기 → (전체화면은 수직 동기) → 전체 viewport 출력(clip 복구) → flip 관문 순이고 모드 비트가 없으면 거부한다', () => {
    const log = [];
    const screen = makeScreen(8, 4, 10, log);
    screen.lock();
    const pixels = screen.lock();
    pixels.fill(9);
    screen.unlock();
    screen.unlock();
    screen.setClip(1, 1, 3, 3);
    log.length = 0;
    screen.loadPalette(gradientPalette(), 1);
    // 현재 clip 이 (1,1,3,3) 이라도 전체 viewport 를 검정으로 채우고(fillBlack 은 clip 을 쓰는 채우기이므로 clip 안쪽만) 출력 뒤 clip 을 복구한다.
    assert.deepEqual(screen.clip(), { left: 1, top: 1, right: 3, bottom: 3 });
    assert.deepEqual(log.map((entry) => entry[0]), ['present', 'palette']);
    assert.deepEqual(log[0].slice(1), [0, 0, 8, 4, 8]);
    // clear 가 0 이면 지우지도 출력하지도 않는다.
    log.length = 0;
    screen.loadPalette(gradientPalette(), 0);
    assert.deepEqual(log.map((entry) => entry[0]), ['palette']);
    // 전체화면은 수직 동기 훅을 부른다(clear 켬).
    const full = makeScreen(8, 4, 11, log);
    log.length = 0;
    full.loadPalette(gradientPalette(), 1);
    assert.deepEqual(log.map((entry) => entry[0]), ['vblank', 'full', 'palette', 'full']);
    // 모드 비트(0x80)가 없는 상태(플래그만 있고 modeSet 없음)는 별도 fillBlt 경계를 가장하지 않고 거부한다. 같은 clear 에서 flags 0 이면 관문 자체를 건너뛴다.
    const unset = new Screen(8, 4);
    unset.init();
    unset.initDibSection();
    unset.loadPalette(gradientPalette(), 1);
    assert.equal(unset.paletteLoaded.value, 1);
});

test('Screen 출력(update): 사각형을 화면으로 자르고 clip 을 화면 전체로 되돌리며 빈 사각형은 보내지 않고, 전체화면은 활성이고 DIB 가 있을 때만 출력 범위를 보낸다', () => {
    const log = [];
    const screen = makeScreen(640, 480, 10, log);
    screen.setClip(10, 10, 20, 20);
    screen.update({ left: -5, top: -5, right: 700, bottom: 500 });
    assert.deepEqual(log, [['present', 0, 0, 640, 480, 640]]);
    assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 640, bottom: 480 });
    log.length = 0;
    // 빈 사각형(폭 0·높이 0·뒤집힘)은 출력하지 않지만 clip 은 되돌린다.
    screen.setClip(1, 1, 2, 2);
    screen.update({ left: 5, top: 5, right: 5, bottom: 9 });
    screen.update({ left: 5, top: 5, right: 9, bottom: 5 });
    screen.update({ left: 9, top: 9, right: 5, bottom: 5 });
    assert.deepEqual(log, []);
    assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 640, bottom: 480 });
    // 출력 장치가 없는 화면은 오류 없이 상태만 바꾼다.
    const bare = new Screen(16, 8);
    bare.init();
    bare.initDibSection();
    bare.setMode(10);
    bare.update({ left: 0, top: 0, right: 4, bottom: 4 });
    // 전체화면: 활성이고 클라이언트 크기를 알면 비율 맞춘 출력 범위를 보낸다.
    const full = makeScreen(1024, 768, 11, log);
    full.setClientSize(1920, 1080);
    log.length = 0;
    full.update({ left: 0, top: 0, right: 4, bottom: 4 });
    assert.deepEqual(log, [['full', 240, 0, 1680, 1080]]);
    // 비활성이면 보내지 않는다.
    log.length = 0;
    full.setActive(false);
    full.update({ left: 0, top: 0, right: 4, bottom: 4 });
    assert.deepEqual(log, []);
    // 표면이 연결되기 전의 창 모드는 보내지 않는다.
    const early = new Screen(16, 8, { devices: { present: () => log.push('early') } });
    early.init();
    early.initDibSection();
    early.update({ left: 0, top: 0, right: 4, bottom: 4 });
    assert.deepEqual(log, []);
});

test('Screen 출력 범위와 입력 변환: 창 모드는 1:1 이고 호환 전체화면만 클라이언트 영역에 맞춘 범위로 되돌리며 클라이언트 크기를 모르면 논리 크기이다', () => {
    const screen = makeScreen(1024, 768, 10);
    screen.setClientSize(1920, 1080);
    assert.deepEqual(screen.presentationRect(), { left: 0, top: 0, right: 1024, bottom: 768 });
    assert.deepEqual(screen.toLogicalPoint({ x: 300, y: 200 }), { x: 300, y: 200 });
    const full = makeScreen(1024, 768, 11);
    assert.deepEqual(full.presentationRect(), { left: 0, top: 0, right: 1024, bottom: 768 });
    full.setClientSize(1920, 1080);
    assert.deepEqual(full.presentationRect(), { left: 240, top: 0, right: 1680, bottom: 1080 });
    assert.deepEqual(full.toLogicalPoint({ x: 960, y: 540 }), { x: 512, y: 384 });
    // 클라이언트 크기가 0 이하이면 모르는 것으로 본다.
    full.setClientSize(0, 100);
    assert.deepEqual(full.presentationRect(), { left: 0, top: 0, right: 1024, bottom: 768 });
});

test('Screen 이동·플립·커서 깊이: 이동은 DIB 모드에서만 하고 0 이동은 잠그지 않으며, 플립 관문은 플리핑 비트를 거부하고, 커서 깊이는 숨김 횟수를 센다', () => {
    const log = [];
    const screen = makeScreen(8, 4, 10, log);
    const pixels = screen.lock();
    screen.unlock();
    // 잠금 해제된 뒤에도 같은 DIB 버퍼이다.
    pixels.set([1, 2, 3, 4, 5, 6, 7, 8], 0);
    screen.scroll(0, 0, 8, 1, 2, 0);
    assert.deepEqual(Array.from(pixels.subarray(0, 8)), [3, 4, 5, 6, 7, 8, 7, 8]);
    // 이동량이 0 이면 잠그지 않는다: 표면 없이도 반환한다.
    const idle = new Screen(8, 4);
    idle.init();
    idle.initDibSection();
    assert.throws(() => idle.scroll(0, 0, 8, 1, 0, 0), /DIB 장치 필요/);
    assert.equal(screen.setMode(10), true);
    screen.scroll(0, 0, 8, 1, 0, 0);
    // 범위를 벗어난 이동은 잠금을 풀고 거부한다.
    assert.throws(() => screen.scroll(4, 0, 8, 1, 1, 0), RangeError);
    const again = screen.lock();
    assert.equal(again, pixels);
    screen.unlock();
    // 플립 관문: 합법 모드(비트 4 없음)는 통과하고 플리핑 비트가 남은 상태는 거부한다.
    screen.flip();
    // 커서 깊이.
    screen.restoreMovieCursor();
    assert.equal(screen.cursorVisible(), true);
    screen.showCursor(false);
    assert.equal(screen.cursorVisible(), false);
    screen.showCursor(false);
    screen.showCursor(true);
    assert.equal(screen.cursorVisible(), false);
    screen.showCursor(true);
    assert.equal(screen.cursorVisible(), true);
    // 자르기 영역의 pane 은 양 끝 포함이다. 테두리는 0 이다.
    screen.setClip(3, 4, 10, 20);
    assert.deepEqual(screen.pane(), { left: 3, top: 4, right: 9, bottom: 19 });
    assert.deepEqual(screen.borderOffset(), { x: 0, y: 0 });
    assert.equal(screen.width, 8);
    assert.equal(screen.height, 4);
    assert.equal(screen.pitch, 8);
    assert.equal(new Screen(1366, 768).pitch, 1366);
    const wide = new Screen(1366, 768);
    wide.init();
    wide.initDibSection();
    assert.equal(wide.pitch, 1368);
});

test('Screen 소프트웨어 마우스 모드: 커서 훅은 소프트웨어 마우스 비트가 없고 숨김 깊이가 1 미만일 때만 하드웨어 커서를 보이도록 알린다', () => {
    const log = [];
    const screen = new Screen(8, 4, { devices: { cursor: (visible) => log.push(visible) } });
    screen.showCursor(true);
    screen.showCursor(false);
    screen.showCursor(false);
    assert.deepEqual(log, [true, true, false]);
    assert.equal(screen.cursorVisible(), false);
});

// ---- 변형 시험 보강: 검증 경계·초기 상태·부분 범위·구름 복사 (2026-11-13 WB04-E393) ----

test('Screen 초기 상태: 표면이 없는 새 화면은 플래그 0·clip 0·테두리 0·비활성이며 색 표는 0, 명암 표는 없고 줄 간격은 폭이다', () => {
    const screen = new Screen(6, 4);
    assert.equal(screen.flags, 0);
    assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 0, bottom: 0 });
    assert.deepEqual(screen.pane(), { left: 0, top: 0, right: -1, bottom: -1 });
    assert.deepEqual(screen.borderOffset(), { x: 0, y: 0 });
    assert.equal(screen.active, false);
    assert.equal(screen.cursorVisible(), true);
    assert.equal(screen.pitch, 6);
    assert.equal(screen.paletteLoaded.value, 0);
    assert.deepEqual(screen.colors(), { basic: new Array(9).fill(0), weather: [0, 0, 0, 0], named: [] });
    assert.equal(screen.shadeMaps(), null);
    assert.ok(screen.palette().every((value) => value === 0) && screen.palette().length === 1024);
    assert.ok(screen.logicalPalette().every((value) => value === 0));
    // 저장 팔레트도 0 이다: null 색 배열로 논리 팔레트만 준비하면 0/255 외에는 플래그만 남는다.
    screen.setPalette(0, 256, null, false);
    assert.equal(screen.logicalPalette()[1], (4 << 24) >>> 0);
    assert.equal(screen.logicalPalette()[0], 0);
    assert.equal(screen.logicalPalette()[255], 0x00ffffff);
    // 가장 작은 크기도 허용한다.
    assert.equal(new Screen(1, 1).width, 1);
    assert.equal(new ScreenSurface('dib', 1, 1).pitch, 4);
    assert.equal(new ScreenSurface('window', 1, 1).pitch, 1);
    // 팔레트 loaded 상자는 옵션이 있으면 그 객체를 그대로 쓴다.
    const box = { value: 5 };
    assert.equal(new Screen(2, 2, { paletteLoaded: box }).paletteLoaded, box);
});

test('Screen 팔레트 부분 범위: 범위 밖 칸과 논리 팔레트를 건드리지 않고 256을 넘는 요청은 끝에서 멈추며 count 0 은 인자 전체를 0번부터 복사한다', () => {
    const screen = makeScreen(8, 4);
    const snapshot = Uint32Array.from(screen.logicalPalette());
    // 10..11 번만 준비한다: 다른 논리 칸은 그대로여야 한다(0/255 고정은 이미 같은 값).
    const part = Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8, 99, 99, 99, 99);
    screen.setPalette(10, 2, part, true);
    const after = screen.logicalPalette();
    // 10, 11 을 뺀 모든 칸은 변하지 않아야 한다.
    for (let index = 0; index < 256; index += 1) {
        if (index === 10 || index === 11) continue;
        assert.equal(after[index], snapshot[index], `논리 팔레트 ${index}`);
    }
    // 현재 팔레트도 두 칸만 바뀌고 인자가 더 길어도 뒤 칸을 건드리지 않는다.
    assert.deepEqual(Array.from(screen.palette().subarray(40, 52)), [1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0]);
    // start 250 에서 10칸을 요청하면 255 번까지만 적용하고 오류 없이 끝난다.
    const tail = new Uint8Array(40);
    for (let index = 0; index < 40; index += 1) tail[index] = index + 1;
    screen.setPalette(250, 10, tail, true);
    // 끝이 정확히 256 이 아니므로(260) 255번 칸은 인자 값 그대로이고(논리 팔레트만 흰색으로 고정된다).
    assert.deepEqual(Array.from(screen.palette().subarray(1000, 1024)), Array.from({ length: 24 }, (_, index) => index + 1));
    assert.equal(screen.logicalPalette()[255], 0x00ffffff);
    assert.equal(screen.logicalPalette()[250], (tail[2] | (tail[1] << 8) | (tail[0] << 16) | (4 << 24)) >>> 0);
    // 같은 start 0 에서 count 만큼만 보더라도 인자의 둘째 칸은 지우지 않는다(첫 칸만 0).
    const first = Uint8Array.of(9, 9, 9, 9, 8, 8, 8, 8);
    screen.setPalette(0, 2, first, false);
    assert.deepEqual(Array.from(first), [0, 0, 0, 0, 8, 8, 8, 8]);
    // count 0 + apply: 인자 전체(1024바이트)를 0번부터 복사하고 0/255 칸은 고정값이 된다.
    const whole = new Uint8Array(1024);
    for (let index = 0; index < 1024; index += 1) whole[index] = (index * 3 + 1) & 255;
    screen.setPalette(0, 0, whole, true);
    assert.deepEqual(Array.from(screen.palette()), Array.from(whole));
    assert.deepEqual(Array.from(whole.subarray(0, 4)), [0, 0, 0, 0]);
    // 인자가 전체 크기보다 짧으면(100바이트) count 0 도 거부한다.
    assert.throws(() => screen.setPalette(0, 0, new Uint8Array(100), true), RangeError);
});

test('Screen 모드 전환은 같은 팔레트를 다시 적용한다: 전환 전 현재 팔레트가 전환 뒤에도 그대로이고 전체화면 비트가 있으면 화면 전체 출력을 한 번 요청한다', () => {
    const log = [];
    const screen = makeScreen(8, 4, 10, log);
    const colors = new Uint8Array(1024);
    // 번호마다 다른 색과 예약 바이트를 둔다.
    for (let index = 1; index < 255; index += 1) colors.set([index, 255 - index, (index * 5) & 255, 7], index * 4);
    screen.setPalette(0, 256, colors, true);
    const before = Uint8Array.from(screen.palette());
    log.length = 0;
    assert.equal(screen.setMode(11), true);
    assert.deepEqual(Array.from(screen.palette()), Array.from(before));
    // 전체화면으로 바뀌며 팔레트 적용이 화면 전체 출력을 한 번 요청한다(활성 상태에서).
    assert.equal(log.filter((entry) => entry[0] === 'full').length, 1);
    assert.equal(log.filter((entry) => entry[0] === 'palette').length, 1);
});

test('Screen 팔레트 적재 전체 대조: 파일 팔레트 모든 색이 BGRX 로 현재 팔레트에 들어가고 예약 바이트는 이전 값을 유지하며 clear 는 clip 안쪽만 검정으로 채운다', () => {
    const screen = makeScreen(8, 4, 10);
    const palette = gradientPalette();
    // 예약 바이트를 미리 만들어 두면(저장 팔레트의 이전 값) 적재 뒤에도 같은 값이어야 한다.
    const reserved = new Uint8Array(1024);
    for (let index = 0; index < 256; index += 1) reserved[index * 4 + 3] = (index * 11) & 255;
    screen.setPalette(0, 256, reserved, true);
    // 모드가 바뀌면 현재 팔레트가 저장 팔레트로 복사되므로 예약 바이트가 저장 팔레트에도 들어간다.
    assert.equal(screen.setMode(11), true);
    screen.loadPalette(palette);
    const current = screen.palette();
    // 0/255 를 뺀 각 색을 파일 값과 대조한다.
    for (let index = 1; index < 255; index += 1) {
        assert.deepEqual(Array.from(current.subarray(index * 4, index * 4 + 4)), [palette.rgb[index * 3 + 2], palette.rgb[index * 3 + 1], palette.rgb[index * 3], (index * 11) & 255], `색 ${index}`);
    }
    // clear: 화면을 9 로 채운 뒤 clip(1,1,3,3) 상태에서 적재하면 clip 안쪽 네 칸만 검정이 되고 출력 뒤 clip 은 복구된다.
    const pixels = screen.lock();
    pixels.fill(9);
    screen.unlock();
    screen.setClip(1, 1, 3, 3);
    screen.loadPalette(palette, 1);
    const grid = [];
    const surface = screen.lock();
    // 줄 간격 8 의 네 행을 읽는다.
    for (let row = 0; row < 4; row += 1) grid.push(Array.from(surface.subarray(row * 8, row * 8 + 8)));
    screen.unlock();
    assert.deepEqual(grid, [[9, 9, 9, 9, 9, 9, 9, 9], [9, 0, 0, 9, 9, 9, 9, 9], [9, 0, 0, 9, 9, 9, 9, 9], [9, 9, 9, 9, 9, 9, 9, 9]]);
    // 이 화면은 전체화면(139)이라 팔레트 적용이 화면 전체 출력(update)을 요청해 clip 이 화면 전체로 돌아간다(창 모드의 clip 복구는 clear 관문 시험이 확인한다).
    assert.deepEqual(screen.clip(), { left: 0, top: 0, right: 8, bottom: 4 });
});

test('Screen 구름 복사 경계: 거꾸로 된 대상은 거부하고 소스 줄 간격 음수·물리 범위 한 칸 초과를 각각 거부하며 정확히 맞는 경우는 허용하고 4바이트 읽고 쓰기를 구분한다', () => {
    const screen = makeScreen(8, 4);
    const pixels = screen.lock();
    screen.setClip(0, 0, 8, 4);
    const source = Uint8Array.from({ length: 64 }, (_, index) => index + 1);
    const full = { left: 0, top: 0, right: 8, bottom: 4 };
    // 정확히 화면 전체를 복사한다(마지막 칸까지).
    screen.blitBuffer(0, 0, source, 8, full);
    assert.deepEqual(Array.from(pixels.subarray(0, 32)), Array.from(source.subarray(0, 32)));
    // 거꾸로 된 소스 사각형은 대상 위치가 0 이 아니면 음수 폭이 되어 거부한다(너비/높이 각각).
    assert.throws(() => screen.blitBuffer(5, 0, source, 8, { left: 3, top: 0, right: 0, bottom: 2 }), RangeError);
    assert.throws(() => screen.blitBuffer(0, 3, source, 8, { left: 0, top: 3, right: 2, bottom: 0 }), RangeError);
    // 대상이 비어 있어도(폭 0) 음수 소스 줄 간격은 거부한다.
    assert.throws(() => screen.blitBuffer(0, 0, source, -1, { left: 0, top: 0, right: 0, bottom: 2 }), RangeError);
    // 소스가 한 칸 모자라면 거부하고 딱 맞으면 허용한다: 4×1 을 소스 4바이트에서 복사.
    pixels.fill(0);
    screen.blitBuffer(0, 0, source.subarray(0, 4), 4, { left: 0, top: 0, right: 4, bottom: 1 });
    assert.deepEqual(Array.from(pixels.subarray(0, 4)), [1, 2, 3, 4]);
    assert.throws(() => screen.blitBuffer(0, 0, source.subarray(0, 3), 4, { left: 0, top: 0, right: 4, bottom: 1 }), RangeError);
    // 소스 시작이 음수인 사각형(위쪽 -1, 왼쪽 -5)은 거부한다.
    assert.throws(() => screen.blitBuffer(0, 0, source, 8, { left: 0, top: -1, right: 4, bottom: 0 }), RangeError);
    assert.throws(() => screen.blitBuffer(0, 0, source, 8, { left: -5, top: 0, right: 4, bottom: 1 }), RangeError);
    // 소스 시작 위치가 안전 정수를 넘는 값이면 대상이 비어 있지 않을 때만 거부한다(너비 0 또는 높이 0 은 아무것도 읽지 않는다).
    const huge = { left: 0, top: 2 ** 30, right: 1, bottom: 2 ** 30 + 1 };
    assert.throws(() => screen.blitBuffer(0, 0, source, 2 ** 31 - 1, huge), RangeError);
    screen.blitBuffer(0, 0, source, 2 ** 31 - 1, { left: 0, top: 2 ** 30, right: 0, bottom: 2 ** 30 + 1 });
    screen.blitBuffer(0, 0, source, 2 ** 31 - 1, { left: 0, top: 2 ** 30, right: 1, bottom: 2 ** 30 });
    // 곱이 안전 정수 정확히 2^53-1 이어도(441651191 × 20394401) 읽을 소스가 없어 거부한다.
    assert.throws(() => screen.blitBuffer(0, 0, source, 20394401, { left: 0, top: 441651191, right: 1, bottom: 441651192 }), RangeError);
    // 4바이트 폭 겹침: 한 워드를 먼저 읽고 쓰므로 [1,2,3,4] 가 한 칸 뒤로 간다(바이트 단위였다면 1 이 번진다).
    pixels.set([1, 2, 3, 4, 5, 6, 7, 8], 0);
    screen.blitBuffer(1, 0, pixels, 8, { left: 0, top: 0, right: 4, bottom: 1 });
    assert.deepEqual(Array.from(pixels.subarray(0, 8)), [1, 1, 2, 3, 4, 6, 7, 8]);
    // 8바이트 폭은 두 워드이다.
    pixels.set([1, 2, 3, 4, 5, 6, 7, 8], 0);
    pixels.fill(0, 8, 16);
    screen.blitBuffer(0, 1, pixels, 8, { left: 0, top: 0, right: 8, bottom: 1 });
    assert.deepEqual(Array.from(pixels.subarray(8, 16)), [1, 2, 3, 4, 5, 6, 7, 8]);
    screen.unlock();
});

test('Screen 채우기 경계: 거꾸로 된 사각형은 쓰지 않고 줄 간격 padding 끝과 마지막 칸까지 허용하며 clip 이 표면보다 크면 한 칸 밖에서 거부한다', () => {
    const screen = makeScreen(6, 4);
    const pixels = screen.lock();
    screen.setClip(0, 0, 8, 4);
    // 거꾸로 된 사각형(right<left)은 폭 0 이다.
    screen.fillRect({ left: 3, top: 0, right: 1, bottom: 2 }, 5);
    assert.ok(pixels.every((value) => value === 0));
    // 줄 간격 8 의 끝(padding)까지 전체를 채운다: 마지막 행 마지막 칸 포함.
    screen.fillRect({ left: 0, top: 0, right: 8, bottom: 4 }, 6);
    assert.ok(pixels.every((value) => value === 6));
    assert.equal(pixels.length, 32);
    // 한 칸 더 넓은 clip 은 줄 간격을 넘어 거부한다(오른쪽 9, 아래 5).
    screen.setClip(0, 0, 9, 4);
    assert.throws(() => screen.fillRect({ left: 0, top: 0, right: 9, bottom: 1 }, 1), RangeError);
    screen.setClip(0, 0, 8, 5);
    assert.throws(() => screen.fillRect({ left: 0, top: 4, right: 8, bottom: 5 }, 1), RangeError);
    screen.unlock();
});

test('Screen 클라이언트 크기·표면 크기 경계: 1×1 클라이언트는 허용하고 한 축이 0 이면 모르는 것으로 보며 DIB 줄 간격은 폭의 4배수 올림이다', () => {
    const full = makeScreen(1024, 768, 11);
    full.setClientSize(1, 1);
    assert.deepEqual(full.presentationRect(), { left: 0, top: 0, right: 1, bottom: 1 });
    full.setClientSize(100, 0);
    assert.deepEqual(full.presentationRect(), { left: 0, top: 0, right: 1024, bottom: 768 });
    full.setClientSize(0, 100);
    assert.deepEqual(full.presentationRect(), { left: 0, top: 0, right: 1024, bottom: 768 });
    // 폭마다 4바이트 정렬 줄 간격을 확인한다.
    for (const [width, pitch] of [[1, 4], [2, 4], [3, 4], [4, 4], [5, 8], [1366, 1368], [1367, 1368], [1368, 1368], [1369, 1372]]) assert.equal(new ScreenSurface('dib', width, 2).pitch, pitch, `폭 ${width}`);
});

test('호환 출력 계산의 경계: 1 크기·출력 원점이 0 이 아닌 변환·올림 위상·정확히 맞는 버퍼와 더 큰 목적지의 보존을 확인한다', () => {
    // 1×1 논리 화면과 1×1 출력도 허용한다.
    assert.deepEqual(fitScreenPresentation(1, 1, 1, 1), { left: 0, top: 0, right: 1, bottom: 1 });
    assert.deepEqual(mapScreenPresentationPoint({ x: 0, y: 0 }, { left: 0, top: 0, right: 1, bottom: 1 }, 1, 1), { x: 0, y: 0 });
    assert.deepEqual(mapScreenPresentationRect({ left: 0, top: 0, right: 1, bottom: 1 }, { left: 0, top: 0, right: 1, bottom: 1 }, 1, 1), { left: 0, top: 0, right: 1, bottom: 1 });
    // 출력 원점이 0 이 아니면 좌표는 원점을 뺀 뒤 배율로 되돌린다: x 는 (150-10)*8/100=11.2→8(상한), y 는 (120-20)*6/200=3.
    const output = { left: 10, top: 20, right: 110, bottom: 220 };
    assert.deepEqual(mapScreenPresentationPoint({ x: 150, y: 120 }, output, 8, 6), { x: 8, y: 3 });
    assert.deepEqual(mapScreenPresentationPoint({ x: 60, y: 120 }, output, 8, 6), { x: 4, y: 3 });
    // 사각형은 원점을 더하고 오른쪽/아래는 올림이다: 논리 7×5 → 출력 10×10, right=5 → ceil(50/7)=8, bottom=3 → ceil(30/5)=6.
    const small = { left: 3, top: 4, right: 13, bottom: 14 };
    assert.deepEqual(mapScreenPresentationRect({ left: 1, top: 1, right: 5, bottom: 3 }, small, 7, 5), { left: 3 + Math.trunc(10 / 7), top: 4 + 2, right: 3 + 8, bottom: 4 + 6 });
    // 확대 버퍼: 정확히 필요한 크기는 허용하고, 행 간격이 폭과 같아도 허용한다.
    const source = Uint8Array.of(1, 2, 3, 4);
    const exact = new Uint8Array(4);
    scaleScreenPresentation(source, 2, 2, 2, exact, 2, 2, 2);
    assert.deepEqual(Array.from(exact), [1, 2, 3, 4]);
    const sourceExact = Uint8Array.of(1, 2, 77, 77, 3, 4);
    const destinationExact = new Uint8Array(11).fill(99);
    scaleScreenPresentation(sourceExact, 4, 2, 2, destinationExact, 4, 3, 3);
    assert.deepEqual(Array.from(destinationExact), [1, 2, 2, 99, 3, 4, 4, 99, 3, 4, 4]);
    assert.throws(() => scaleScreenPresentation(sourceExact, 4, 2, 2, new Uint8Array(10), 4, 3, 3), RangeError);
    // 더 큰 목적지의 뒤쪽 칸은 건드리지 않는다.
    const big = new Uint8Array(20).fill(99);
    scaleScreenPresentation(source, 2, 2, 2, big, 4, 3, 3);
    assert.deepEqual(Array.from(big.subarray(11)), new Array(9).fill(99));
    // 1×1 크기 확대/축소.
    const one = new Uint8Array(1);
    scaleScreenPresentation(Uint8Array.of(7), 1, 1, 1, one, 1, 1, 1);
    assert.equal(one[0], 7);
});

test('화면 이동의 검증 순서: 영역이 없거나 이동이 영역 이상이거나 이동이 없으면 잘못된 기하라도 먼저 반환하고, 정확히 맞는 기하는 허용한다', () => {
    const bad = new Uint8Array(0);
    // 각 조기 반환 조건은 기하 검증보다 앞선다(잘못된 pitch/크기를 줘도 오류가 나지 않는다).
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 0, 5, 1, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 0, 1, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 5, 5, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 5, 0, 5);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 5, -5, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 5, 0, -5);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, 5, 0, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, -1, 5, 1, 0);
    scrollScreenPixels(bad, 0, 0, 0, -1, -1, 5, -1, 1, 0);
    // 정확히 맞는 기하: 영역이 오른쪽/아래 끝까지, 줄 간격이 폭과 같고 버퍼 길이가 정확히 pitch*height.
    const pixels = Uint8Array.from({ length: 16 }, (_, index) => index);
    scrollScreenPixels(pixels, 4, 4, 4, 0, 0, 4, 4, 1, 1);
    assert.deepEqual(Array.from(pixels), [5, 6, 7, 3, 9, 10, 11, 7, 13, 14, 15, 11, 12, 13, 14, 15]);
    // 기하 오류는 각각 따로 거부한다: 폭 0, 높이 0, pitch<폭, x<0, y<0, 영역이 폭/높이를 한 칸 넘음, 버퍼 한 칸 모자람.
    const probe = (...args) => assert.throws(() => scrollScreenPixels(new Uint8Array(16), ...args), RangeError);
    probe(4, 0, 4, 0, 0, 2, 2, 1, 0);
    probe(4, 4, 0, 0, 0, 2, 2, 1, 0);
    probe(3, 4, 4, 0, 0, 2, 2, 1, 0);
    probe(4, 4, 4, -1, 0, 2, 2, 1, 0);
    probe(4, 4, 4, 0, -1, 2, 2, 1, 0);
    probe(4, 4, 4, 3, 0, 2, 2, 1, 0);
    probe(4, 4, 4, 0, 3, 2, 2, 0, 1);
    assert.throws(() => scrollScreenPixels(new Uint8Array(15), 4, 4, 4, 0, 0, 2, 2, 1, 0), RangeError);
});

test('호환 출력 계산의 마지막 경계: 큰 폭 축소의 첫 열 표본, 폭/높이 1 의 세로·가로 이동, 줄 간격 0 의 구름 복사, 폭 0 채우기와 전체 화면 clear 의 첫 칸', () => {
    // 폭 10 → 2 축소: 각 열의 중앙 표본은 trunc(1*10/4)=2 와 trunc(3*10/4)=7 이다(첫 열도 0 이 아니다).
    const wide = Uint8Array.from({ length: 10 }, (_, index) => index);
    const narrow = new Uint8Array(2);
    scaleScreenPresentation(wide, 10, 10, 1, narrow, 2, 2, 1);
    assert.deepEqual(Array.from(narrow), [2, 7]);
    // 폭 1 화면의 세로 이동: 영역 폭이 1 이어도 위로 한 칸 이동한다.
    const column = Uint8Array.of(1, 2, 3);
    scrollScreenPixels(column, 1, 1, 3, 0, 0, 1, 3, 0, 1);
    assert.deepEqual(Array.from(column), [2, 3, 3]);
    // 높이 1 영역의 가로 이동도 한 칸 이동한다.
    const row = Uint8Array.of(1, 2, 3);
    scrollScreenPixels(row, 3, 3, 1, 0, 0, 3, 1, 1, 0);
    assert.deepEqual(Array.from(row), [2, 3, 3]);
    // 구름 복사의 소스 줄 간격 0 은 같은 소스 행을 반복해 읽는다(허용).
    const screen = makeScreen(8, 4);
    const pixels = screen.lock();
    screen.setClip(0, 0, 8, 4);
    screen.blitBuffer(0, 0, Uint8Array.of(1, 2, 3, 4), 0, { left: 0, top: 0, right: 4, bottom: 2 });
    assert.deepEqual(Array.from(pixels.subarray(0, 4)), [1, 2, 3, 4]);
    assert.deepEqual(Array.from(pixels.subarray(8, 12)), [1, 2, 3, 4]);
    // 폭 0 채우기는 clip 이 표면보다 커서 왼쪽이 줄 간격을 넘어도 쓰는 칸이 없으므로 거부하지 않는다.
    screen.setClip(0, 0, 100, 100);
    screen.fillRect({ left: 9, top: 0, right: 9, bottom: 1 }, 5);
    screen.unlock();
    // 전체 화면 clear: clip 이 전체이면 첫 행·첫 열(0,0)도 검정이 된다.
    const cleared = makeScreen(8, 4, 10);
    const surface = cleared.lock();
    surface.fill(9);
    cleared.unlock();
    cleared.setClip(0, 0, 8, 4);
    cleared.loadPalette(gradientPalette(), 1);
    const after = cleared.lock();
    cleared.unlock();
    assert.ok(after.every((value) => value === 0));
});

test('findPaletteColor: 가장 작은 거리의 번호를 고르고 동률은 앞 번호를 유지하며 1 차이 나는 거리도 구별한다', () => {
    const logical = new Uint32Array(256).fill(0x00ffffff);
    // 목표 (0,0,0) 에서 0번은 거리 5(차이 2,1,0), 1번은 거리 4(차이 2,0,0), 2번은 거리 4 로 1번과 동률이다.
    logical[0] = (2 | (1 << 8)) >>> 0;
    logical[1] = 2;
    logical[2] = (2 << 16) >>> 0;
    assert.equal(findPaletteColor(logical, 0, 0, 0), 1);
    // 1 만 가까운 경우에도 순서가 뒤집히지 않는다(5 대 4 와 4 대 5 를 모두 확인).
    logical[0] = 2;
    logical[1] = (2 | (1 << 8)) >>> 0;
    assert.equal(findPaletteColor(logical, 0, 0, 0), 0);
    // 모두 같은 거리이면 첫 번호이다.
    logical.fill(0x00010203);
    assert.equal(findPaletteColor(logical, 3, 2, 1), 0);
    // 초기 최솟값(float32 1e7)보다 멀면 0번을 돌려준다: 모든 거리가 1e7 이상인 범위 밖 RGB.
    logical.fill(0);
    logical[5] = 0x00ffffff;
    assert.equal(findPaletteColor(logical, 4000, 4000, 4000), 0);
});
