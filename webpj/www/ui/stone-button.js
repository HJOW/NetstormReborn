// 원본 돌 버튼(Stone Buttongump) 하나의 크기 맞춤·위치·판정·누름 상태와 그리기를 담당하는 모듈이다.
// 원본 주소: 생성자 004254b0, 크기 맞춤 00424f90, 부착 00424f00, 입력 Handle(ButtonGump), 판정 004248e0, Draw 00424ff0.
// 위치는 float32 로 저장하고, 화면 정수 좌표는 원본 CRT _ftol 처럼 0 방향으로 절삭한다.
// 판정은 화면 좌표에서 float 위치를 뺀 지역 좌표가 [0,폭)×[0,높이) 안인지로 정한다.
// 동작 근거: docs/videos/menu-buttons-20261003.md(호버 없음, 누름 즉시 눌린 모양+button.wav, 안쪽에서 뗄 때만 실행).
// 복원 기준: cpppj StoneButtonGump.cpp·ButtonGump.cpp·GumpCollection.cpp. (2026-10-23 WB03-M1)
import { encodeWindows1252 } from '../assets/text-encoding.js';
import { FONT_SLOT_HEIGHTS } from '../assets/bitmap-font.js';
import { createTextContext, drawGlyphText } from '../render/glyph-text.js';
import { GumpBackgroundFlags, drawGumpBackground, planButtonDraw, planGumpBackground } from '../render/gump-visual.js';

// 크기 맞춤에서 측정 글자 폭에 더하는 값(픽셀)이다(원본 00424f90: 폭 + 12). (2026-10-23 WB03-M1)
export const STONE_BUTTON_EXTRA_WIDTH = 12;

// 크기 맞춤에서 측정 글자 높이에 더하는 값(픽셀)이다(원본 00424f90: 높이 + 6). (2026-10-23 WB03-M1)
export const STONE_BUTTON_EXTRA_HEIGHT = 6;

// 돌 버튼 생성 시 쓰는 현재 기본 글자 문맥이다. 측정에만 쓰며 플래그 0(그림자 없음)이라 그림자 오프셋을 크기에 더하지 않는다.
// 원본 메인 메뉴 버튼은 문맥 인자 0(null)으로 만들어 이 기본 문맥을 쓴다. (2026-10-23 WB03-M1)
export const STONE_BUTTON_DEFAULT_CONTEXT = createTextContext({ flags: 0, font: 0, shadowX: 1, shadowY: 1 });

// 배경 자식의 버튼 기준 상대 위치(1,1)와 크기 감소(-2)이다(원본 00424f00). (2026-10-23 WB03-M1)
const BACKGROUND_INSET = 1;
const BACKGROUND_SHRINK = 2;

// 원본 CRT _ftol 처럼 실수를 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-23 WB03-M1)
export function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// 원본 MeasureGumpText 로 글자 폭·높이를 잰다. 폭은 측정 폭 표의 합, 높이는 글꼴 슬롯의 논리 높이이며,
// 문맥 플래그 2(그림자)가 있으면 그림자 오프셋을 각각 더한다. (2026-10-23 WB03-M1)
export function measureGumpText(font, bytes, context) {
    let width = font.measure(bytes);
    let height = FONT_SLOT_HEIGHTS[context.font] ?? 0;
    if (context.flags & 2) {
        width = (width + context.shadowX) | 0;
        height = (height + context.shadowY) | 0;
    }
    return { width, height };
}

// 돌 버튼 하나이다. 라벨·명령과 원본 노드 필드(크기·배율·위치·누름 상태)를 가진다.
// 사용: new StoneButton({ label: 'Campaign', command }) → fitToText(font) → setBaseWidth(w) → attach(x, y) → draw(surface, resources).
// 입력은 pointerDown/broadcast/pointerUp/cancel 순서로 넘기며, pointerUp 이 true 를 돌려주면 실행(click 통지)이다. (2026-10-23 WB03-M1)
// 한국어처럼 번역된 라벨은 display(표시 문구)와 encode(부호화 함수, FontSet.encode)로 넘긴다. 코드 배열이 Uint16Array 면 유니코드 글꼴로 재고 그린다. (2026-10-31 WB03-N)
export class StoneButton {
    // 라벨 문자열과 실행 명령(임의 객체)으로 버튼을 만든다. 크기는 fitToText 전까지 0 이다.
    // label 은 원본 영어 라벨(번역 조회의 키이자 명령 통지에 쓰는 이름), display 는 화면에 쓰는 번역된 문구(생략하면 label), encode 는 문구를 코드 배열로 바꾸는 함수이다.
    constructor({ label, command = null, display = label, encode = encodeWindows1252 }) {
        // 원본 영어 라벨(번역 전 문구)이다. 명령 통지에서 어느 버튼인지 알리는 이름이다.
        this.label = String(label);
        // 화면에 표시할 번역된 문구이다. 영어에서는 label 과 같다. (2026-10-31 WB03-N)
        this.text = String(display);
        // 글꼴용 라벨 코드 배열이다(원본 글꼴은 Windows-1252 바이트, 한국어 글꼴은 UTF-16 코드 단위). 원본 strncpy(63) 처럼 63개까지만 쓴다.
        this.labelBytes = encode(this.text).subarray(0, 63);
        // 실행할 때 메뉴에 돌려줄 명령 객체다.
        this.command = command;
        // 측정한 글자 폭(픽셀)이다. Draw 의 글자 가운데 맞춤에 쓴다.
        this.textWidth = 0;
        // 측정한 글자 높이(픽셀)다.
        this.textHeight = 0;
        // 배율 적용 전 폭(원본 +0x3c)이다. 메뉴 배치의 간격 계산이 읽는다.
        this.baseWidth = 0;
        // 배율 적용 전 높이(원본 +0x40)다.
        this.baseHeight = 0;
        // 배율(원본 +0x38)이다. 원본 메인 메뉴는 1이다.
        this.scale = 1;
        // 배율 적용 폭(원본 +0x44, float32)이다. 판정과 화면 경계에 쓴다.
        this.width = 0;
        // 배율 적용 높이(원본 +0x48, float32)다.
        this.height = 0;
        // 부모(메인 메뉴 Gump) 기준 x 위치(float32)다.
        this.x = 0;
        // 부모 기준 y 위치(float32)다.
        this.y = 0;
        // 원본 value 상태(+a0)다. 토글이 아닌 메인 메뉴 버튼은 항상 0 이다.
        this.value = 0;
        // 원본 pressed 상태(+a4)다. 1이면 눌린 모양(테두리 명암 반전, 글자 1픽셀 이동)으로 그린다.
        this.pressed = 0;
        // 버튼 플래그(+ac)다. 1=비활성, 8=외곽선 생략. 메인 메뉴 버튼은 0 이다.
        this.flags = 0;
        // 왼쪽 버튼 누름으로 입력을 붙잡았는지(원본 Capture + notify 비트) 여부다.
        this.captured = false;
    }

    // 이 버튼의 라벨을 재고 그릴 글꼴을 고른다. 라벨이 유니코드 코드 배열(Uint16Array)이면 fonts(FontSet)의 유니코드 글꼴, 아니면 원본 비트맵 글꼴 font 이다. (2026-10-31 WB03-N)
    labelFont(font, fonts) {
        return this.labelBytes instanceof Uint16Array ? fonts.unicodeFont(STONE_BUTTON_DEFAULT_CONTEXT) : font;
    }

    // 현재 기본 문맥으로 라벨을 재고 폭+12·높이+6 으로 크기를 맞춘다(원본 00424f90). 한국어 라벨은 fonts(FontSet)의 유니코드 글꼴로 잰다. (2026-10-31 WB03-N: fonts 추가)
    fitToText(font, fonts = null) {
        const measured = measureGumpText(this.labelFont(font, fonts), this.labelBytes, STONE_BUTTON_DEFAULT_CONTEXT);
        this.textWidth = measured.width;
        this.textHeight = measured.height;
        this.baseWidth = Math.fround(this.textWidth + STONE_BUTTON_EXTRA_WIDTH);
        this.width = Math.fround(this.baseWidth * this.scale);
        this.baseHeight = Math.fround(this.textHeight + STONE_BUTTON_EXTRA_HEIGHT);
        this.height = Math.fround(this.baseHeight * this.scale);
    }

    // 배치가 정한 공통 폭을 쓴다(원본 004ce940 이 모든 버튼에 최대 폭을 대입하는 부분).
    setBaseWidth(width) {
        this.baseWidth = Math.fround(width);
        this.width = Math.fround(this.baseWidth * this.scale);
    }

    // 부모 기준 위치(float32)에 붙인다(원본 가상 Attach 의 위치 저장 부분). 누름 상태는 유지한다.
    attach(x, y) {
        this.x = Math.fround(x);
        this.y = Math.fround(y);
    }

    // 화면 정수 사각형(원본 Bounds)을 돌려준다. 왼쪽 위는 위치를 절삭한 값, 오른쪽 아래는 그 값 + 폭/높이를 절삭한 값이다.
    bounds() {
        const left = ftol(this.x);
        const top = ftol(this.y);
        return { left, top, right: ftol(left + this.width), bottom: ftol(top + this.height) };
    }

    // 배경 자식의 화면 사각형이다. 버튼 기준 (1,1) 에 붙고 크기는 버튼보다 2 작다(원본 00424f00).
    backgroundBounds() {
        const left = ftol(Math.fround(this.x + BACKGROUND_INSET));
        const top = ftol(Math.fround(this.y + BACKGROUND_INSET));
        const width = Math.fround(this.width - BACKGROUND_SHRINK);
        const height = Math.fround(this.height - BACKGROUND_SHRINK);
        return { left, top, right: ftol(left + width), bottom: ftol(top + height) };
    }

    // 화면 정수 좌표가 버튼 판정 영역 안인지 검사한다(원본 ScreenToLocal + 004248e0).
    // 지역 좌표 = float32(화면 좌표 - float 위치) 이며 [0,폭)×[0,높이) 안이면 참이다. 높이는 그려진 높이보다 1 크다.
    contains(screenX, screenY) {
        const localX = Math.fround(screenX - this.x);
        const localY = Math.fround(screenY - this.y);
        return localX >= 0 && localX < this.width && localY >= 0 && localY < this.height;
    }

    // 화면 정수 좌표가 정수 화면 사각형 [left,right)×[top,bottom) 안인지 검사한다.
    // 원본 입력 분배(GumpEvents BaseFilter, 도움말 플래그 0x200)가 누름을 받을 버튼을 고를 때 쓰는 판정이다.
    // 위치가 반 픽셀(.5)이면 contains 의 float 판정과 1픽셀 어긋날 수 있으며, 이 차이도 원본 그대로 둔다.
    boundsContain(screenX, screenY) {
        const rect = this.bounds();
        return rect.left <= screenX && screenX < rect.right && rect.top <= screenY && screenY < rect.bottom;
    }

    // 왼쪽 버튼을 정수 화면 사각형 안에서 눌렀을 때 입력을 붙잡고 눌린 모양으로 바꾼다. 처리했으면 true(이때 button.wav 를 낸다).
    // 비활성 버튼과 영역 밖 누름은 false 다. 눌린 모양은 이후 broadcast 가 float 판정으로 다시 정한다(원본 Handle → Broadcast).
    pointerDown(screenX, screenY) {
        if (this.flags & 1 || !this.boundsContain(screenX, screenY)) return false;
        this.captured = true;
        this.pressed = 1;
        return true;
    }

    // 붙잡은 동안 현재 포인터 위치로 눌린 모양을 갱신한다(원본 Broadcast: 안쪽이면 1, 밖이면 0). 모양이 바뀌면 true.
    broadcast(screenX, screenY) {
        if (!this.captured) return false;
        const inside = this.contains(screenX, screenY) ? 1 : 0;
        if (inside === this.pressed) return false;
        this.pressed = inside;
        return true;
    }

    // 왼쪽 버튼을 뗐을 때 붙잡기를 풀고, 뗀 위치가 안쪽이며 눌린 상태였으면 true(실행)를 돌려준다.
    // 바깥에서 뗐으면 불발이다. 붙잡지 않은 버튼은 false 다.
    pointerUp(screenX, screenY) {
        if (!this.captured) return false;
        const activated = this.contains(screenX, screenY) && this.pressed !== 0;
        this.captured = false;
        this.pressed = 0;
        return activated;
    }

    // 실행 없이 붙잡기와 눌린 모양을 해제한다(포커스 상실·화면 전환 등). 모양이 바뀌었으면 true.
    cancel() {
        const changed = this.captured || this.pressed !== 0;
        this.captured = false;
        this.pressed = 0;
        return changed;
    }

    // 버튼을 그린다: 배경 질감(명암 테두리) → 외곽선 네 개 → 글자 순서다(원본 배경 자식 → 부모 Draw).
    // resources: { font, texture(질감 그림), textureMetrics{width,height}, shadeMaps, buttonPalette, textContexts }.
    draw(surface, resources) {
        const down = this.value !== 0 || this.pressed !== 0;
        const backgroundFlags = GumpBackgroundFlags.BUTTON | (down ? GumpBackgroundFlags.PRESSED : 0);
        const passes = planGumpBackground(this.backgroundBounds(), surface.bounds, resources.textureMetrics.width, resources.textureMetrics.height, backgroundFlags);
        drawGumpBackground(surface, resources.texture, passes, resources.shadeMaps);
        const rect = this.bounds();
        const plan = planButtonDraw(
            { rect, textWidth: this.textWidth, textHeight: this.textHeight, value: this.value, pressed: this.pressed, flags: this.flags },
            resources.textContexts,
            resources.buttonPalette.edge,
        );
        surface.drawLines(plan.lines);
        drawGlyphText(surface, this.labelFont(resources.font, resources.fonts), this.labelBytes,(rect.left + plan.textOffset.x) | 0, (rect.top + plan.textOffset.y) | 0, plan.text);
    }
}
