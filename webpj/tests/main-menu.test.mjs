// 메인 메뉴의 배치(원본 004ce940), 돌 버튼 입력 규칙, 원본 스크린샷 대조, 화면 배치 계산과 음량 변환을 검사한다.
// (2026-10-23 WB03-M1: 메인 메뉴 복원 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, decodePng, readDataMap } from './helpers.mjs';
import { MENU_ASSET_PATHS, buildMenuResources, FORT_GUMP_BLOCK } from '../www/assets/menu-assets.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { MainMenuScreen, titlePosition } from '../www/ui/main-menu.js';
import { CURSOR_PATHS } from '../www/ui/cursor.js';
import { StoneButton } from '../www/ui/stone-button.js';
import { planGumpBackground, GumpBackgroundFlags, planButtonDraw } from '../www/render/gump-visual.js';
import { centibelsToGain, volumeStepToCentibels } from '../www/audio/audio-system.js';
import { clientToLogical, computeLayout } from '../www/netstorm.js';

// 음악을 뺀 메인 메뉴 자원 묶음을 데이터 폴더에서 만든다(검사마다 공유). (2026-10-23 WB03-M1)
const resources = buildMenuResources(readDataMap([...new Set([...Object.values(MENU_ASSET_PATHS).filter((assetPath) => assetPath !== MENU_ASSET_PATHS.music), ...Object.values(CURSOR_PATHS)])]));

// 메뉴를 만들어 배치한 뒤 라벨 → 화면 사각형 표를 돌려준다. (2026-10-23 WB03-M1)
function layoutOf(width, height, includeAllButtons) {
    const menu = new MainMenuScreen({ resources, includeAllButtons });
    menu.resize(width, height);
    return { menu, rects: Object.fromEntries(menu.buttons.map((button) => [button.label, button.bounds()])) };
}

// 색 번호 표면을 화면 팔레트로 RGB 로 바꾼다. (2026-10-23 WB03-M1)
function toRgb(surface, palette) {
    const rgb = new Uint8Array(surface.width * surface.height * 3);
    // 각 픽셀의 색 번호를 RGB 로 옮긴다.
    for (let i = 0; i < surface.pixels.length; i += 1) rgb.set(palette.rgb.subarray(surface.pixels[i] * 3, surface.pixels[i] * 3 + 3), i * 3);
    return rgb;
}

// 원본 8버튼 구성이 원본 관측 좌표(docs/screens/main-menu.md, docs/videos/menu-buttons-20261003.md)와 같은지 확인한다.
test('원본 8버튼 배치: 1024x768 에서 x 356/435/514/593, y 311/334, 75x20', () => {
    const { rects } = layoutOf(1024, 768, true);
    assert.deepEqual(rects.Campaign, { left: 356, top: 311, right: 431, bottom: 331 });
    assert.deepEqual(rects.Multiplayer, { left: 435, top: 311, right: 510, bottom: 331 });
    assert.deepEqual(rects.Demo, { left: 514, top: 311, right: 589, bottom: 331 });
    assert.deepEqual(rects.Help, { left: 593, top: 311, right: 668, bottom: 331 });
    assert.deepEqual(rects.Edit, { left: 356, top: 334, right: 431, bottom: 354 });
    assert.deepEqual(rects.Credits, { left: 435, top: 334, right: 510, bottom: 354 });
    assert.deepEqual(rects.Options, { left: 514, top: 334, right: 589, bottom: 354 });
    assert.deepEqual(rects.Quit, { left: 593, top: 334, right: 668, bottom: 354 });
    assert.equal(rects.Intro, undefined);
});

// 웹 판본(Multiplayer·Edit·Intro·Quit 빈 칸)이 원본 배치 규칙대로 3+2 가운데 배치·최대 폭(Campaign 55+12)이 되는지 확인한다.
// (2026-10-28 WB: AGENTS.md webpj 목표 변경으로 Edit 칸을 빼 3+3 에서 3+2 가 되었다.)
test('웹 5버튼 배치: 빈 칸을 건너뛰고 각 줄 가운데, 폭 67', () => {
    const { menu, rects } = layoutOf(1024, 768, false);
    assert.deepEqual(menu.buttons.map((button) => button.label), ['Campaign', 'Demo', 'Help', 'Credits', 'Options']);
    assert.equal(rects.Edit, undefined);
    assert.deepEqual(rects.Campaign, { left: 407, top: 311, right: 474, bottom: 331 });
    assert.deepEqual(rects.Demo, { left: 478, top: 311, right: 545, bottom: 331 });
    assert.deepEqual(rects.Help, { left: 549, top: 311, right: 616, bottom: 331 });
    // 아랫줄은 2개뿐이라 (1024 - (2*71 - 4)) * 0.5 = 443 에서 시작하고 반 픽셀이 없다.
    assert.deepEqual(rects.Credits, { left: 443, top: 334, right: 510, bottom: 354 });
    assert.deepEqual(rects.Options, { left: 514, top: 334, right: 581, bottom: 354 });
    assert.equal(menu.buttons[0].x, 407.5);
    assert.equal(menu.buttons[3].x, 443);
    const wide = layoutOf(1366, 768, false).rects;
    assert.deepEqual(wide.Campaign, { left: 578, top: 311, right: 645, bottom: 331 });
    assert.deepEqual(wide.Credits, { left: 614, top: 334, right: 681, bottom: 354 });
    assert.deepEqual(wide.Options, { left: 685, top: 334, right: 752, bottom: 354 });
    const original800 = layoutOf(800, 600, true).rects;
    assert.deepEqual(original800.Campaign, { left: 244, top: 255, right: 319, bottom: 275 });
});

// 타이틀 GIF 위치 공식(float(W/2) - 639*0.5 를 절삭)을 확인한다.
test('타이틀 위치', () => {
    assert.deepEqual(titlePosition(1024, 768, resources.title), { x: 192, y: 144 });
    assert.deepEqual(titlePosition(1366, 768, resources.title), { x: 363, y: 144 });
    assert.deepEqual(titlePosition(1228, 768, resources.title), { x: 294, y: 144 });
});

// 돌 버튼의 누름·끌기·뗌 규칙(호버 없음, 안쪽에서 뗄 때만 실행)과 판정 경계를 확인한다.
test('돌 버튼 입력 규칙', () => {
    const commands = [];
    let sounds = 0;
    const menu = new MainMenuScreen({ resources, includeAllButtons: true, onCommand: (command, label) => commands.push(label), onButtonSound: () => { sounds += 1; } });
    menu.resize(1024, 768);
    const credits = menu.buttons.find((button) => button.label === 'Credits');
    // 판정 경계: x 435~509, y 334~353(그려진 높이 19보다 1 더 아래까지).
    assert.equal(menu.pointerDown(434, 340), false);
    assert.equal(menu.pointerDown(435, 354), false);
    assert.equal(menu.pointerDown(509, 353), true);
    assert.equal(credits.pressed, 1);
    assert.equal(sounds, 1);
    // 누른 채 바깥으로 나가면 평소 모양, 돌아오면 다시 눌린 모양이다.
    menu.pointerMove(470, 440);
    assert.equal(credits.pressed, 0);
    menu.pointerMove(470, 340);
    assert.equal(credits.pressed, 1);
    // 이웃 버튼에서 떼면 불발이다.
    assert.equal(menu.pointerUp(540, 340), false);
    assert.deepEqual(commands, []);
    // 바깥에서 눌러 안쪽에서 떼면 불발이다.
    assert.equal(menu.pointerDown(470, 450), false);
    assert.equal(menu.pointerUp(470, 340), false);
    // 정상 클릭은 뗄 때 실행된다.
    menu.pointerDown(470, 340);
    assert.deepEqual(commands, []);
    assert.equal(menu.pointerUp(470, 340), true);
    assert.deepEqual(commands, ['Credits']);
    assert.equal(sounds, 2);
    // 취소는 실행 없이 눌린 모양을 푼다.
    menu.pointerDown(470, 340);
    menu.cancel();
    assert.equal(credits.pressed, 0);
    assert.equal(menu.pointerUp(470, 340), false);
    assert.deepEqual(commands, ['Credits']);
});

// 반 픽셀 위치에서 원본처럼 누름(정수 사각형)과 뗌(float 지역 좌표) 판정이 1픽셀 어긋나는지 확인한다.
test('반 픽셀 위치의 누름/뗌 판정 차이', () => {
    // 웹 메뉴에서 실제로 반 픽셀 위치(407.5)에 놓이는 첫 버튼 Campaign 을 쓴다. (2026-10-28 WB: Edit 제거에 맞춰 라벨 변경)
    const button = new StoneButton({ label: 'Campaign' });
    button.fitToText(resources.font);
    button.setBaseWidth(67);
    button.attach(407.5, 334);
    assert.deepEqual(button.bounds(), { left: 407, top: 334, right: 474, bottom: 354 });
    assert.equal(button.boundsContain(407, 340), true);
    assert.equal(button.contains(407, 340), false);
    assert.equal(button.contains(474, 340), true);
    assert.equal(button.boundsContain(474, 340), false);
});

// 배경 질감 계획(화면 격자 기준점·여분 타일·명암 방향)과 버튼 외곽선·글자 위치 계획을 확인한다.
test('돌 버튼 배경·외곽선 계획', () => {
    const passes = planGumpBackground({ left: 357, top: 335, right: 434, bottom: 353 }, { left: 0, top: 0, right: 1024, bottom: 768 }, 200, 146, GumpBackgroundFlags.BUTTON);
    assert.equal(passes.length, 5);
    assert.deepEqual(passes.map((pass) => pass.shade), [0, 1, 2, 1, 2]);
    assert.deepEqual(passes[0].origins[0], { x: 199, y: 290 });
    assert.deepEqual(passes[1].clip, { left: 357, top: 335, right: 359, bottom: 353 });
    const pressed = planGumpBackground({ left: 357, top: 335, right: 434, bottom: 353 }, { left: 0, top: 0, right: 1024, bottom: 768 }, 200, 146, GumpBackgroundFlags.BUTTON | GumpBackgroundFlags.PRESSED);
    assert.deepEqual(pressed.map((pass) => pass.shade), [0, 2, 1, 2, 1]);
    const contexts = resources.textContexts;
    const normal = planButtonDraw({ rect: { left: 435, top: 334, right: 510, bottom: 354 }, textWidth: 42, textHeight: 14, value: 0, pressed: 0, flags: 0 }, contexts, 15);
    assert.deepEqual(normal.lines, [
        { x1: 436, y1: 334, x2: 508, y2: 334, color: 15 },
        { x1: 436, y1: 352, x2: 508, y2: 352, color: 15 },
        { x1: 435, y1: 335, x2: 435, y2: 351, color: 15 },
        { x1: 509, y1: 335, x2: 509, y2: 351, color: 15 },
    ]);
    assert.deepEqual(normal.textOffset, { x: 17, y: 2 });
    const down = planButtonDraw({ rect: { left: 435, top: 334, right: 510, bottom: 354 }, textWidth: 42, textHeight: 14, value: 0, pressed: 1, flags: 0 }, contexts, 15);
    assert.deepEqual(down.textOffset, { x: 18, y: 3 });
});

// 원본 스크린샷(screenShots/mainMenu.png, 창 테두리 포함 1027x802, 클라이언트 원점 (2,33))과 원본 8버튼 합성을 대조한다.
// 스크린샷은 손실 압축 잡음(평균 차이 약 ±3)이 있어 정확 일치 대신 평균 채널 차이로 판정한다.
// 버튼 질감 프레임 1(A01)이 프레임 0(A00)보다 원본에 가깝다는 것도 함께 확인한다.
test('원본 메인 메뉴 스크린샷 대조', (t) => {
    const shotPath = path.join(REPO_ROOT, 'screenShots', 'mainMenu.png');
    if (!fs.existsSync(shotPath)) {
        t.skip('screenShots/mainMenu.png 가 없는 환경');
        return;
    }
    const shot = decodePng(shotPath);
    assert.equal(shot.width, 1027);
    assert.equal(shot.height, 802);
    const originX = 2;
    const originY = 33;
    // 영역의 평균 채널 절대 차이를 구한다.
    const meanDifference = (rgb, left, top, right, bottom) => {
        let sum = 0;
        let count = 0;
        // 영역의 각 행을 처리한다.
        for (let y = top; y < bottom; y += 1) {
            // 행의 각 픽셀 세 채널 차이를 더한다.
            for (let x = left; x < right; x += 1) {
                const a = (y * 1024 + x) * 3;
                const b = ((y + originY) * shot.width + x + originX) * 3;
                sum += Math.abs(rgb[a] - shot.rgb[b]) + Math.abs(rgb[a + 1] - shot.rgb[b + 1]) + Math.abs(rgb[a + 2] - shot.rgb[b + 2]);
                count += 3;
            }
        }
        return sum / count;
    };
    const compose = (overrides) => {
        const menu = new MainMenuScreen({ resources: { ...resources, ...overrides }, includeAllButtons: true });
        menu.resize(1024, 768);
        const surface = new IndexedSurface(1024, 768);
        menu.render(surface);
        return toRgb(surface, resources.palette);
    };
    const rgb = compose({});
    const full = meanDifference(rgb, 0, 0, 1024, 768);
    const buttons = meanDifference(rgb, 356, 311, 668, 354);
    assert.ok(full < 4, `전체 평균 차이 ${full}`);
    assert.ok(buttons < 9, `버튼 평균 차이 ${buttons}`);
    const shapes = new ShapeDatabase(readDataMap([MENU_ASSET_PATHS.shapes]).get(MENU_ASSET_PATHS.shapes));
    const frameZero = compose({ texture: shapes.decode(FORT_GUMP_BLOCK, 0), textureMetrics: shapes.squidMetrics(FORT_GUMP_BLOCK, 0) });
    const frameZeroButtons = meanDifference(frameZero, 356, 311, 668, 354);
    assert.ok(buttons < frameZeroButtons, `프레임1 ${buttons} < 프레임0 ${frameZeroButtons}`);
});

// 화면비별 논리 배치, 16:9 상한과 고DPI 입력 역변환을 확인한다. (2026-11-19 WB04-E943)
test('화면 배치 계산', () => {
    const exact = computeLayout(1024, 768, 1, 'auto');
    assert.equal(exact.mode, '4:3');
    assert.equal(exact.canvasWidth, 1024);
    assert.equal(exact.drawScaleX, 1);
    const wide = computeLayout(1920, 1080, 1, 'auto');
    assert.equal(wide.mode, '16:9');
    assert.equal(wide.logicalWidth, 1280);
    assert.equal(wide.logicalHeight, 720);
    assert.equal(wide.cssWidth, 1920);
    const middle = computeLayout(1919, 1079, 1, 'auto');
    assert.deepEqual([middle.logicalWidth, middle.logicalHeight], [1280, 720]);
    const compact = computeLayout(1599, 899, 2, 'auto');
    assert.deepEqual([compact.logicalWidth, compact.logicalHeight], [1280, 720]);
    const highDpi = computeLayout(2560, 1440, 2, 'auto');
    assert.deepEqual([highDpi.logicalWidth, highDpi.logicalHeight], [1280, 720]);
    assert.deepEqual([highDpi.canvasWidth, highDpi.canvasHeight], [5120, 2880]);
    const forced = computeLayout(1920, 1080, 1, '16:10');
    assert.deepEqual([forced.logicalWidth, forced.logicalHeight], [1280, 800]);
    // 같은 16:10 논리 화면은 창 크기·DPI가 바뀌어도 유지하고 마우스 좌표만 역변환한다.
    const ten = computeLayout(1280, 800, 1, 'auto'), tenDpi = computeLayout(640, 400, 2, '16:10');
    assert.deepEqual([ten.logicalWidth, ten.logicalHeight, ten.drawScaleX, ten.drawScaleY], [1280, 800, 1, 1]);
    assert.deepEqual([tenDpi.logicalWidth, tenDpi.logicalHeight, tenDpi.canvasWidth, tenDpi.canvasHeight], [1280, 800, 1280, 800]);
    assert.deepEqual(clientToLogical(320, 200, { left: 0, top: 0, width: tenDpi.cssWidth, height: tenDpi.cssHeight }, 1280, 800), { x: 640, y: 400, inside: true });
    const point = clientToLogical(1280, 720, { left: 0, top: 0, width: highDpi.cssWidth, height: highDpi.cssHeight },
        highDpi.logicalWidth, highDpi.logicalHeight);
    assert.deepEqual(point, { x: 640, y: 360, inside: true });
    for(const [width,height,logicalHeight] of [[1280,960,960],[1280,1024,1024],[3840,2400,800],[3840,2880,960],[3840,3072,1024]]) { // 큰 네 화면비도1280 상한과 같은 비율을 지킨다.
        const layout=computeLayout(width,height,1,'auto');
        assert.deepEqual([layout.logicalWidth,layout.logicalHeight],[1280,logicalHeight]);
        assert.equal(layout.logicalWidth*height,layout.logicalHeight*width);
    }
});

// 원본 음량 단계 → 1/100 dB → 증폭률 변환을 확인한다.
test('음량 단계 변환', () => {
    assert.deepEqual([0, 1, 2, 3, 4, 5, 6].map(volumeStepToCentibels), [0, -4000, -2000, -1000, -500, 0, 0]);
    assert.ok(Math.abs(centibelsToGain(-2000) - 0.1) < 1e-12);
    assert.ok(Math.abs(centibelsToGain(-1000) - 0.31622776601683794) < 1e-12);
    assert.equal(centibelsToGain(0), 1);
});

// 높이 800 초과(4차 와이드)에서는 원문식(H/3+55)이 버튼을 가운데 타이틀 로고 위로 올리므로 그림 기준 같은 위치(((H-480)/2+112)+55/78)를 쓴다. 800 이하는 원문 그대로이다. (2026-11-22 WB04-E1224)
test('메인 메뉴 와이드 보정: 높이 800 초과는 타이틀 그림 기준 위치, 800 이하는 원문 H/3', () => {
    // 1024x768 은 원문 그대로(311/334)이다. 1280x800 은 한계값이라 원문 H/3(266+55=321)이다.
    assert.equal(layoutOf(1024, 768, true).rects.Campaign.top, 311);
    assert.equal(layoutOf(1280, 800, true).rects.Campaign.top, 321);
    // 1920x1080: 타이틀 위쪽 y=300 에서 167 아래(=467)이고 두 번째 줄은 +23(78-55)이다. 1600x900 은 (900-480)/2+112+55=377.
    const wide = layoutOf(1920, 1080, true).rects; assert.equal(wide.Campaign.top, 467); assert.equal(wide.Edit.top, 490);
    assert.equal(layoutOf(1600, 900, true).rects.Campaign.top, 377);
    // 로고(가운데 640x480) 아래 167 관계가 1024x768 과 같다: 타이틀 위쪽은 H/2-240.
    for (const [width, height] of [[1024, 768], [1600, 900], [1920, 1080]]) assert.equal(layoutOf(width, height, true).rects.Campaign.top - (Math.trunc(height / 2) - 240), 167);
});
