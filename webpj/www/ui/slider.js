// 원본 세로 스크롤바(Slider)를 복원하는 모듈이다. 다중 줄 본문(ScrollingText)의 오른쪽 가장자리에 붙어 위·아래 화살표 돌 버튼, 눌린 질감의 홈(thumb 바탕), 그 위의 튀어나온 손잡이로 이루어진다.
// 값은 최솟값·최댓값·현재 위치·쪽 크기(page)이며, 손잡이는 홈 길이에 쪽 크기/범위 비율을 곱한 길이로 현재 위치/범위 비율의 위치에 그린다.
// 입력(원본 ScrollGump·ThumbHandle): 화살표를 누른 채 있다가 안쪽에서 떼면 한 번 단계 이동, 0.4초 뒤부터 0.05초마다 반복 이동한다.
// 손잡이 위쪽·아래쪽 홈을 누르면 쪽 이동, 손잡이를 누르고 끌면 위치가 따라온다. 단계·쪽 이동의 실제 처리는 부모(본문)가 하며 부모가 처리하지 않으면 자신의 위치를 한 칸 옮긴다.
// 화살표 삼각형은 검정으로 그린다(아래 SLIDER_ARROW_COLOR 설명). 눌려도 삼각형은 움직이지 않는다.
// 원본 주소: Slider 생성 004a5ba0 계열, Attach 004a5cd0, SetPosition 004a6050, SetRange 004a6350, SetPage 004a6320, DragVertical 004a6170, Handle 004a61c0,
//            ThumbHandle 004a5ae0, ThumbBroadcast 004a6430. 복원 기준: cpppj ScrollGump.cpp·ScrollGumpWidgets.cpp(10.78 경로).
// (2026-10-30 WB03-G: 도움말 창 본문의 스크롤바를 원본과 같게 그리고 조작하기 위해 추가)
import { GumpBackgroundFlags, drawGumpBackground, planGumpBackground } from '../render/gump-visual.js';
import { drawVfxLine } from '../render/vfx-line.js';
import { StoneButton } from './stone-button.js';

// 스크롤바 폭(픽셀)이다. 화살표 단추 한 변도 폭과 같다(원본 생성 인자 9). (2026-10-30 WB03-G)
export const SLIDER_WIDTH = 9;

// 화살표 삼각형의 색 번호이다. cpppj 는 슬라이더 프레임 인자(1)를 화살표 단추의 색 칸에 그대로 넘기는 것으로 읽지만, 이 팔레트의 1번은 어두운 빨강(99,38,22)이고
// 원본 스크린샷(help - NetStorm Instructions.png)의 위·아래 화살표 삼각형은 둘 다 순수한 검정(0,0,0)이어서 스크린샷에 맞춰 검정(색 번호 0)을 쓴다. (2026-10-30 WB03-G)
export const SLIDER_ARROW_COLOR = 0;

// 홈(바탕)의 배경 플래그이다: 질감 + 네 테두리 + 밝은 명암 + 눌림(원본 0x03ec0000). (2026-10-30 WB03-G)
const TRACK_FLAGS = GumpBackgroundFlags.BUTTON | GumpBackgroundFlags.BRIGHT | GumpBackgroundFlags.PRESSED;

// 손잡이의 배경 플래그이다: 질감 + 네 테두리(원본 0x03e00000). (2026-10-30 WB03-G)
const HANDLE_FLAGS = GumpBackgroundFlags.BUTTON;

// 화살표를 누르고 있을 때 첫 반복까지의 지연과 반복 간격(초)이다(원본 ButtonGump 의 0.4, 0.05). (2026-10-30 WB03-G)
const FIRST_DELAY = 0.4;
const REPEAT_DELAY = 0.05;

// 화살표 방향 번호이다(원본 ConstructArrow 의 direction): 0 = 위, 2 = 아래. (2026-10-30 WB03-G)
const DIRECTION_UP = 0;
const DIRECTION_DOWN = 2;

// 부모에 알리는 단계 요청 번호이다(원본 stepEvent 의 값): 1 = 아래로 한 단계, 2 = 위로 한 단계, 3 = 한 쪽 아래로, 4 = 한 쪽 위로. (2026-10-30 WB03-G)
export const SliderStep = Object.freeze({ DOWN: 1, UP: 2, PAGE_DOWN: 3, PAGE_UP: 4 });

// 32비트 정수 덧셈·뺄셈·곱셈(감김 유지)이다. 원본 x86 의 DWORD 연산과 같다. (2026-10-30 WB03-G)
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
const mul = (a, b) => Math.imul(a, b);

// 원본 CRT _ftol 처럼 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-30 WB03-G)
function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// float32 로 저장한다. (2026-10-30 WB03-G)
const f32 = Math.fround;

// 세로 스크롤바 하나이다. 사용: new VerticalSlider({ height, onStep, onSound }) → attach(x, y) → setRange·setPage·setPosition → draw(surface, resources).
// 입력은 pointerDown/pointerMove/pointerUp/cancel 에 화면 정수 좌표를 넘기고 update(초)로 반복 이동을 진행한다.
// onStep(번호)는 부모가 단계·쪽 이동을 처리했으면 true 를 돌려준다. onChange()는 끌기로 위치가 바뀐 뒤 불린다. onSound('button')은 화살표를 누를 때 불린다. (2026-10-30 WB03-G)
export class VerticalSlider {
    constructor({ width = SLIDER_WIDTH, height = 100, onStep = null, onChange = null, onSound = null, now = () => 0 } = {}) {
        // 배율 적용 전 폭·높이(원본 +0x3c·+0x40)다.
        this.baseWidth = f32(width);
        this.baseHeight = f32(height);
        // 배율이다(원본 +0x38). 대화상자는 1 이다.
        this.scale = 1;
        // 화면 위치(float32)이다. 부모 본문이 붙이면서 정한다.
        this.x = 0;
        this.y = 0;
        // 보이는지 여부이다(원본 flags 비트 1). 꺼지면 그리지도 입력을 받지도 않는다.
        this.visible = true;
        // 값 범위·현재 위치·쪽 크기·단계이다(원본 +a0·+a4·+a8·+ac·+b8). 생성 직후 값은 원본 생성자 마지막의 (0,100)·쪽 10·위치 50 이다.
        this.minimum = 0;
        this.maximum = 0;
        this.position = 0;
        this.page = 0;
        this.step = 1;
        // 부모 통지 콜백들이다.
        this.onStep = onStep;
        this.onChange = onChange;
        this.onSound = onSound;
        // 반복 기한 계산에 쓰는 시계(초)이다.
        this.now = now;
        // 위·아래 화살표 돌 버튼이다(크기 폭×폭, 빈 라벨).
        this.decrement = new StoneButton({ label: '', command: null });
        this.increment = new StoneButton({ label: '', command: null });
        // 화살표별 반복 기한(첫 지연·반복 간격, 시계 초)이다.
        this.deadlines = new Map([[this.decrement, { first: 0, repeat: 0 }], [this.increment, { first: 0, repeat: 0 }]]);
        // 홈(thumb 바탕)의 화면 위치·크기이다(float32). attach 가 정한다.
        this.thumb = { x: 0, y: 0, width: 0, height: 0 };
        // 손잡이의 홈 기준 양 끝 포함 사각형이다(원본 +c0..cc). 왼쪽·위·오른쪽·아래이다.
        this.handle = { left: 0, top: 0, right: 0, bottom: 0 };
        // 손잡이를 끄는 중이면 이전 y 이다. 아니면 null 이다(원본 flags 비트 0x100 + 이전 y).
        this.dragPreviousY = null;
        // 원본 생성자 마지막 호출과 같은 초기 값을 넣는다.
        this.setRange(0, 100);
        this.setPage(10);
        this.setPosition(50);
    }

    // 높이만 바꾼다(원본 본문 Attach 가 슬라이더 높이를 본문 높이로 맞춘다). 다음 attach 에서 배치가 다시 계산된다. (2026-10-30 WB03-G)
    setHeight(height) {
        this.baseHeight = f32(height);
    }

    // 값 범위를 정한다(원본 SetRange). 두 끝을 정렬하고 같으면 최댓값을 +1 하며, 쪽 크기를 범위 길이로 제한한 뒤 현재 위치를 두 번 다시 적용한다. (2026-10-30 WB03-G)
    setRange(first, last) {
        const minimum = first < last ? first : last;
        let maximum = first < last ? last : first;
        if (first === last) maximum = add(minimum, 1);
        this.maximum = maximum;
        this.minimum = minimum;
        const length = sub(maximum, minimum);
        if (length < this.page) this.page = length;
        this.setPosition(this.position);
        this.setPosition(this.position);
    }

    // 쪽 크기를 정한다(원본 SetPage). 범위 길이보다 크면 길이로 줄이고(음수는 하한을 두지 않음) 현재 위치를 다시 적용한다. (2026-10-30 WB03-G)
    setPage(page) {
        const length = sub(this.maximum, this.minimum);
        this.page = length < page ? length : page;
        this.setPosition(this.position);
    }

    // 위치를 정한다(원본 SetPosition). 최솟값 이상, 최댓값 - 쪽 크기 이하로 제한하고 손잡이 위치를 다시 계산한다. 쪽 크기가 범위보다 크면 원본은 assert 이므로 오류다. (2026-10-30 WB03-G)
    setPosition(position) {
        if (sub(this.maximum, this.minimum) < this.page) throw new RangeError('스크롤바 쪽 크기가 범위보다 큽니다.');
        let result = position | 0;
        if (result < this.minimum) result = this.minimum;
        if (this.maximum < add(this.page, result)) result = sub(this.maximum, this.page);
        this.position = result;
        this._layoutHandle();
    }

    // 화면 위치에 붙이고 세 자식(위 화살표·아래 화살표·홈)의 크기와 위치, 손잡이 사각형을 정한다(원본 Attach). 자식 위치는 부모 위치 + 지역 위치를 float32 로 더한 값이다. (2026-10-30 WB03-G)
    attach(x, y) {
        this.x = f32(x);
        this.y = f32(y);
        const thickness = ftol(this.baseWidth);
        const arrow = (button, localY) => {
            button.baseWidth = this.baseWidth;
            button.width = f32(this.baseWidth * button.scale);
            button.baseHeight = f32(thickness);
            button.height = f32(thickness * button.scale);
            button.attach(f32(this.x), f32(this.y + localY));
        };
        arrow(this.decrement, 0);
        arrow(this.increment, f32(this.baseHeight - thickness));
        const thumbHeight = f32(this.baseHeight - add(thickness, thickness));
        this.thumb = { x: f32(this.x), y: f32(this.y + thickness), width: this.baseWidth, height: thumbHeight };
        this._layoutHandle();
    }

    // 현재 값과 크기로 손잡이의 홈 기준 사각형(양 끝 포함)을 계산한다(원본 Attach 끝). 위치·쪽 크기를 홈 길이 비율로 환산해 절삭한다. (2026-10-30 WB03-G)
    _layoutHandle() {
        const thickness = ftol(this.baseWidth);
        const available = this.baseHeight - add(thickness, thickness);
        const range = sub(this.maximum, this.minimum);
        const page = ftol((this.page * available) / range);
        const position = ftol((this.position * available) / range);
        const cross = sub(ftol(this.baseWidth), 1);
        this.handle = { left: 1, top: add(position, 1), right: cross, bottom: sub(add(position, page), 1) };
    }

    // 홈의 화면 정수 사각형이다(원본 GumpCollection::Bounds). (2026-10-30 WB03-G)
    thumbBounds() {
        const left = ftol(this.thumb.x);
        const top = ftol(this.thumb.y);
        return { left, top, right: ftol(left + this.thumb.width), bottom: ftol(top + this.thumb.height) };
    }

    // 화면 정수 좌표가 스크롤바 전체(폭 × 높이) 안인지 검사한다. (2026-10-30 WB03-G)
    contains(x, y) {
        return this.visible && x >= ftol(this.x) && x < ftol(this.x) + this.baseWidth && y >= ftol(this.y) && y < ftol(this.y) + this.baseHeight;
    }

    // 손잡이를 끌었을 때 새 위치를 구한다(원본 DragVertical). 범위 × (이동량 + 현재 손잡이 위쪽) / (높이 - 폭×2) 를 절삭해 위치로 삼는다. (2026-10-30 WB03-G)
    _dragVertical(delta) {
        const numerator = mul(sub(this.maximum, this.minimum), add(delta, this.handle.top));
        const denominator = this.baseHeight - this.baseWidth * 2;
        this.setPosition(ftol(numerator / denominator));
    }

    // 부모에 단계 요청을 보내고 처리되지 않았으면 자신의 위치를 한 칸 옮긴다(원본 Handle 의 화살표 분기). code 는 SliderStep 값이다. (2026-10-30 WB03-G)
    _requestStep(code) {
        if (this.onStep?.(code)) return;
        if (code === SliderStep.UP) this.setPosition(sub(this.position, this.step));
        else if (code === SliderStep.DOWN) this.setPosition(add(this.step, this.position));
    }

    // 화살표 하나의 누름 기한을 지금 시각 기준으로 다시 잡는다(원본 ResetDeadlines). (2026-10-30 WB03-G)
    _resetDeadlines(button) {
        const now = this.now();
        this.deadlines.set(button, { first: now + FIRST_DELAY, repeat: now + REPEAT_DELAY });
    }

    // 왼쪽 버튼 누름을 처리한다. 화살표면 눌린 모양과 button 효과음, 홈이면 손잡이 위·아래는 쪽 이동, 손잡이는 끌기 시작이다. 처리했으면 true. (2026-10-30 WB03-G)
    pointerDown(x, y) {
        if (!this.visible) return false;
        for (const button of [this.decrement, this.increment]) {
            if (button.pointerDown(x, y)) {
                this._resetDeadlines(button);
                this.onSound?.('button');
                return true;
            }
        }
        const bounds = this.thumbBounds();
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return false;
        const local = sub(y, bounds.top);
        if (local < this.handle.top) this._requestStep(SliderStep.PAGE_UP);
        else if (this.handle.bottom < local) this._requestStep(SliderStep.PAGE_DOWN);
        else this.dragPreviousY = y;
        return true;
    }

    // 포인터 이동을 처리한다. 붙잡힌 화살표의 눌린 모양을 갱신하고(바뀌면 반복 기한을 다시 잡음), 손잡이를 끄는 중이면 위치를 옮긴다. 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    pointerMove(x, y) {
        let changed = false;
        for (const button of [this.decrement, this.increment]) {
            if (button.broadcast(x, y)) {
                this._resetDeadlines(button);
                changed = true;
            }
        }
        if (this.dragPreviousY !== null) {
            const delta = sub(y, this.dragPreviousY);
            if (delta !== 0) {
                this.dragPreviousY = y;
                this._dragVertical(delta);
                this.onChange?.();
                changed = true;
            }
        }
        return changed;
    }

    // 왼쪽 버튼 뗌을 처리한다. 붙잡힌 화살표를 안쪽에서 뗐으면 한 단계 이동하고, 끌기를 끝낸다. 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    pointerUp(x, y) {
        let changed = false;
        for (const [button, code] of [[this.decrement, SliderStep.UP], [this.increment, SliderStep.DOWN]]) {
            const wasCaptured = button.captured;
            if (button.pointerUp(x, y)) this._requestStep(code);
            if (wasCaptured) changed = true;
        }
        if (this.dragPreviousY !== null) {
            this.dragPreviousY = null;
            changed = true;
        }
        return changed;
    }

    // 진행 중인 누름·끌기를 실행 없이 취소한다(포커스 상실·화면 크기 변경). 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    cancel() {
        let changed = this.decrement.cancel();
        if (this.increment.cancel()) changed = true;
        if (this.dragPreviousY !== null) {
            this.dragPreviousY = null;
            changed = true;
        }
        return changed;
    }

    // 시간 경과를 알린다(원본 Idle). 눌린 채 첫 기한(0.4초)이 지난 화살표는 반복 기한(0.05초)마다 한 단계씩 이동한다. 이동했으면 true. (2026-10-30 WB03-G)
    update() {
        let moved = false;
        for (const [button, code] of [[this.decrement, SliderStep.UP], [this.increment, SliderStep.DOWN]]) {
            if (!button.captured || button.pressed === 0) continue;
            const deadline = this.deadlines.get(button);
            const now = this.now();
            if (deadline.first <= now && deadline.repeat <= now) {
                this._requestStep(code);
                this.deadlines.set(button, { first: deadline.first, repeat: this.now() + REPEAT_DELAY });
                moved = true;
            }
        }
        return moved;
    }

    // 스크롤바를 그린다: 두 화살표(돌 버튼 + 삼각형) → 홈(눌린 질감) → 손잡이(튀어나온 질감)의 순서다. 꺼져 있으면 그리지 않는다.
    // resources: 돌 버튼과 같은 { texture, textureMetrics, shadeMaps, buttonPalette, textContexts, font }. (2026-10-30 WB03-G)
    draw(surface, resources) {
        if (!this.visible) return;
        for (const [button, direction] of [[this.decrement, DIRECTION_UP], [this.increment, DIRECTION_DOWN]]) {
            button.draw(surface, resources);
            const rect = button.bounds();
            const left = add(rect.left, 2);
            const top = add(rect.top, 2);
            const right = sub(rect.right, 2);
            const bottom = sub(rect.bottom, 2);
            const middleX = Math.trunc(add(left, right) / 2);
            // 위 화살표는 아래 변에서 위 꼭짓점으로, 아래 화살표는 위 변에서 아래 꼭짓점으로 삼각형을 이룬다.
            const baseY = direction === DIRECTION_UP ? bottom : top;
            const tipY = direction === DIRECTION_UP ? top : bottom;
            drawVfxLine(surface, { x: left, y: baseY }, { x: right, y: baseY }, SLIDER_ARROW_COLOR);
            drawVfxLine(surface, { x: left, y: baseY }, { x: middleX, y: tipY }, SLIDER_ARROW_COLOR);
            drawVfxLine(surface, { x: right, y: baseY }, { x: middleX, y: tipY }, SLIDER_ARROW_COLOR);
        }
        const bounds = this.thumbBounds();
        const { width, height } = resources.textureMetrics;
        drawGumpBackground(surface, resources.texture, planGumpBackground(bounds, surface.bounds, width, height, TRACK_FLAGS), resources.shadeMaps);
        // 손잡이는 홈 기준 양 끝 포함 좌표를 그대로 배경 사각형(끝 미포함)으로 넘긴다(원본 DrawThumb).
        const handle = { left: add(bounds.left, this.handle.left), top: add(bounds.top, this.handle.top), right: add(bounds.left, this.handle.right), bottom: add(bounds.top, this.handle.bottom) };
        drawGumpBackground(surface, resources.texture, planGumpBackground(handle, surface.bounds, width, height, HANDLE_FLAGS), resources.shadeMaps);
    }
}
