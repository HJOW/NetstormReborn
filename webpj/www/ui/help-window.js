// 일반 도움말(F1) 창을 복원하는 모듈이다. 원본 QuickHelpWindow: 450×350 돌 패널 가운데 위쪽(y 40)에 어두운 본문 칸(스크롤바 포함)과 아래 Back·OK 돌 버튼이 있다.
// 본문은 도움말 파일의 주제 구간이며 파란 글자(링크)를 누르면 그 주제로 넘어가고(방문 기록에 쌓임), Back 은 이전 주제로, 첫 주제에서의 Back·OK·창 밖 누름은 창을 닫는다.
// 본문은 끌어서 스크롤하거나(글자 위를 누른 채 위아래로 움직임) 스크롤바(화살표·손잡이·홈)·PageUp/PageDown/위·아래 화살표 키로 움직인다.
// 배치(원본 QuickHelpWindow::Construct/Attach): 창 450×350 을 화면 가운데(폭), y 40 에 놓는다. 본문 배경은 (17,17)에서 폭 420, 높이 350 - 17 - 10 - 10 - 버튼 높이이고 눌린 질감 테두리(질감 프레임 0)다.
// 그 안쪽 (2,2)에 폭·높이 -4 의 단색(RGB 55,51,54) 칸이 있고, 본문은 그 칸을 채운다(오른쪽 9픽셀은 스크롤바). 버튼은 아래 위쪽 10픽셀을 비우고 가운데에 놓는다(Back, OK 순, 간격 10, 폭은 OK 의 1.2배로 통일).
// 원본 주소: 생성·배치 QuickHelpWindow, 요청·이력·입력 QuickHelpControl(OpenQuickHelpWindow·DispatchQuickHelpWindow·QuickHelpHistory), 본문 변환 QuickHelpHtml. 복원 기준: cpppj(10.78 경로).
// 웹 적응: 도움말 파일 안의 바깥 주소(`http://`)는 열지 않고 호스트 훅(onExternal)에 넘긴다(과거 도메인이며 접속하지 않는다). 마우스 휠로도 스크롤한다(원본에 없음).
// (2026-10-30 WB03-G: 메인 메뉴 Help → General Help 창을 위해 추가)
import { createTextContext } from '../render/glyph-text.js';
import { GumpBackgroundFlags, drawGumpBackground, planGumpBackground } from '../render/gump-visual.js';
import { findPaletteColor } from '../assets/palette.js';
import { StoneButton } from './stone-button.js';
import { ScrollingText } from './scroll-text.js';
import { VerticalSlider } from './slider.js';
import { parseStyleText } from './style-text.js';
import { renderHelpTopic } from './help-topic.js';

// 창 크기와 위쪽 위치이다(원본 Construct 의 450×350, OpenQuickHelpWindow 의 y 40). (2026-10-30 WB03-G)
export const HELP_WINDOW_WIDTH = 450;
export const HELP_WINDOW_HEIGHT = 350;
export const HELP_WINDOW_TOP = 40;

// 본문 배경의 위쪽·왼쪽 위치(17)와 오른쪽 여백(15)·왼쪽 여백(15)을 뺀 폭 계산, 본문 배경 아래쪽 여백(버튼 위 10 + 창 아래 10)이다. (2026-10-30 WB03-G)
const BODY_LEFT = 17;
const BODY_TOP = 17;
const BODY_SIDE_MARGIN = 15;
const BODY_BOTTOM_GAP = 10;

// 안쪽 단색 칸이 본문 배경 안으로 들어가는 간격(네 변)이다. (2026-10-30 WB03-G)
const INNER_INSET = 2;

// 버튼 사이 간격과 창 아래 간격(원본 10)이다. OK 버튼은 글자에 맞춘 폭의 1.2배를 쓴다. (2026-10-30 WB03-G)
const BUTTON_GAP = 10;
const OK_WIDTH_FACTOR = 1.2;

// 본문 기본 글꼴 슬롯(원본 constructText 인자 5)이다. 색은 설명 본문 색(colors.information)이다. (2026-10-30 WB03-G)
const BODY_FONT_SLOT = 5;

// 본문 칸의 단색 배경 RGB 이다(원본 nearestColor(55,51,54)). (2026-10-30 WB03-G)
const BODY_BACKGROUND_RGB = Object.freeze([55, 51, 54]);

// 장식 패널 배경 플래그(질감 + 네 테두리)와 본문 배경 플래그(질감 + 네 테두리 + 눌림, 원본 0x03e40000)이다. (2026-10-30 WB03-G)
const PANEL_FLAGS = GumpBackgroundFlags.BUTTON;
const BODY_FRAME_FLAGS = GumpBackgroundFlags.BUTTON | GumpBackgroundFlags.PRESSED;

// 키보드 스크롤 양이다(원본 DispatchQuickHelpWindow: 위·아래 화살표 10, PageUp·PageDown 은 본문 높이). 마우스 휠 한 칸의 이동량은 웹 적응으로 본문 세 줄(45)이다. (2026-10-30 WB03-G)
const KEY_STEP = 10;
const WHEEL_STEP = 45;

// 원본 CRT _ftol 처럼 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-30 WB03-G)
function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// float32 로 저장한다. (2026-10-30 WB03-G)
const f32 = Math.fround;

// 위치(float32)와 크기로 화면 정수 사각형을 만든다(원본 GumpCollection::Bounds). 왼쪽 위는 위치를 절삭하고 오른쪽 아래는 그 값 + 크기를 절삭한다. (2026-10-30 WB03-G)
function boundsOf(x, y, width, height) {
    const left = ftol(x);
    const top = ftol(y);
    return { left, top, right: ftol(left + width), bottom: ftol(top + height) };
}

// 바깥 주소로 보이는 링크 목적지인지(`스킴://`) 검사한다. (2026-10-30 WB03-G)
function isExternalDestination(destination) {
    return /^[a-z][a-z0-9+.-]*:\/\//i.test(destination) || /^mailto:/i.test(destination);
}

// 도움말 창 하나이다. 사용: const help = new HelpWindow({ resources, helpText, substitute, screen, onClose, onCommand, onExternal, onSound }); help.open('F1Help'); help.fit(); help.place(); help.draw(surface).
// 입력은 pointerDown/pointerMove/pointerUp/cancel/keyDown/wheel, 시간은 update(초)로 넘긴다. 메인 메뉴의 dialog 자리에 그대로 넣을 수 있는 같은 모양(fit·place·draw·pointer*·cancel·update·close·screen)을 가진다.
// onClose()는 창이 닫힐 때(OK·첫 주제의 Back·창 밖 누름), onCommand({ name, argument })는 `cmd:` 링크, onExternal(주소)은 바깥 주소 링크, onSound('button')은 돌 버튼·화살표를 누를 때 불린다. (2026-10-30 WB03-G)
export class HelpWindow {
    constructor({ resources, helpText, substitute = (text) => text, screen, now = () => 0, onClose = null, onCommand = null, onExternal = null, onSound = null, onUnsupported = null }) {
        // 그리기 자원 묶음이다(fonts·colors·panel·texture·palette 등).
        this.resources = resources;
        // 도움말 파일 전체 문자열이다.
        this.helpText = helpText;
        // 설정 치환 함수이다(`{키}` 해석, 조건 값).
        this.substitute = substitute;
        // 논리 화면 크기 { width, height } 이다. 창은 이 화면의 가운데 폭에 놓인다.
        this.screen = screen;
        // 시계(초)이다. 화살표 반복 이동 기한에 쓴다.
        this.now = now;
        this.onClose = onClose;
        this.onCommand = onCommand;
        this.onExternal = onExternal;
        this.onSound = onSound;
        this.onUnsupported = onUnsupported;
        // 닫혔는지 여부이다. 닫힌 창은 입력·시간·그리기 요청을 받지 않는다.
        this.closed = false;
        // 방문한 주제 기록이다(원본 QuickHelpHistory). 마지막이 지금 보는 주제이며 같은 주제(대소문자 무시)를 연달아 쌓지 않는다.
        this.history = [];
        // 지금 보는 주제 이름과 그 링크 목적지 표(LinkTable)이다.
        this.topic = '';
        this.links = null;
        // 본문(ScrollingText)과 스크롤바이다. 주제를 바꿀 때마다 새로 만든다(원본 SetText 가 자식을 지우고 다시 만듦).
        this.body = null;
        // 본문을 누른 채 끄는 중이면 이전 y 이다. 아니면 null 이다(원본 dragY + notify 비트).
        this.dragY = null;
        // 단색 본문 칸의 색 번호이다(원본 nearestColor(55,51,54)).
        this.backgroundColor = findPaletteColor(resources.palette.logical, BODY_BACKGROUND_RGB[0], BODY_BACKGROUND_RGB[1], BODY_BACKGROUND_RGB[2]);
        // Back·OK 돌 버튼이다. OK 의 글자 폭에 맞춘 값의 1.2배로 두 버튼의 폭을 통일하므로 fit 에서 크기를 정한다.
        this.backButton = new StoneButton({ label: 'Back', command: 'back' });
        this.okButton = new StoneButton({ label: 'OK', command: 'ok' });
        // 창과 자식들의 화면 위치(float32)·크기이다. place 가 정한다.
        this.x = 0;
        this.y = 0;
        this.width = HELP_WINDOW_WIDTH;
        this.height = HELP_WINDOW_HEIGHT;
        // 본문 배경·안쪽 단색 칸의 { x, y, width, height } 이다.
        this.frameRect = null;
        this.innerRect = null;
        // 버튼의 창 기준 지역 위치 { x, y } 두 개이다.
        this.buttonLocals = [];
        this._layout();
    }

    // 버튼 크기와 본문 칸 크기·지역 위치를 정한다(원본 Construct·Attach 의 크기 계산). 화면 위치와 무관하다. (2026-10-30 WB03-G)
    _layout() {
        const { font } = this.resources;
        this.backButton.fitToText(font);
        this.okButton.fitToText(font);
        // OK 버튼은 글자 폭 + 12 의 1.2배다(원본 Learn 버튼 폭 × 1.2).
        this.okButton.setBaseWidth(this.okButton.baseWidth * OK_WIDTH_FACTOR);
        const height = ftol(this.okButton.baseHeight);
        const larger = (a, b) => (a > b ? a : b);
        const widest = ftol(larger(this.backButton.baseWidth, larger(this.okButton.baseWidth, 0)));
        this.backButton.setBaseWidth(widest);
        this.okButton.setBaseWidth(widest);
        const count = 2;
        const offset = ftol((HELP_WINDOW_WIDTH - count * widest - count * BUTTON_GAP) * 0.5);
        const bottom = (ftol(HELP_WINDOW_HEIGHT) - height) - BUTTON_GAP;
        this.buttonLocals = [{ x: f32(offset), y: f32(bottom) }, { x: f32((offset + BUTTON_GAP) + widest), y: f32(bottom) }];
        // 본문 배경과 안쪽 칸의 지역 크기이다.
        const frameWidth = (HELP_WINDOW_WIDTH - BODY_SIDE_MARGIN) - BODY_SIDE_MARGIN;
        const frameHeight = (((HELP_WINDOW_HEIGHT - BODY_TOP) - BODY_BOTTOM_GAP) - BODY_BOTTOM_GAP) - height;
        this.frameRect = { x: f32(BODY_LEFT), y: f32(BODY_TOP), width: f32(frameWidth), height: f32(frameHeight) };
        this.innerRect = { x: f32(INNER_INSET), y: f32(INNER_INSET), width: f32((frameWidth - INNER_INSET) - INNER_INSET), height: f32((frameHeight - INNER_INSET) - INNER_INSET) };
        if (this.body) this._sizeBody();
    }

    // 본문의 크기를 안쪽 칸에 맞추고 줄 나눔과 스크롤을 다시 정한다(위치는 place 가 정한다). (2026-10-30 WB03-G)
    _sizeBody() {
        this.body.setSize(this.innerRect.width, this.innerRect.height);
    }

    // 주제 하나를 요청한다(원본 QuickHelpRequest::Open): 방문 기록에 쌓고(마지막과 같으면 쌓지 않음) 본문을 바꾼다. (2026-10-30 WB03-G)
    open(topic) {
        const last = this.history[this.history.length - 1];
        if (last === undefined || last.toLowerCase() !== topic.toLowerCase()) this.history.push(topic);
        this._show(topic);
    }

    // 주제 본문을 만들어 보이는 본문으로 바꾼다(원본 QuickHelpWindow::Show). 스크롤은 맨 위로 돌아간다. (2026-10-30 WB03-G)
    _show(topic) {
        const result = renderHelpTopic({ helpText: this.helpText, topic, substitute: this.substitute });
        this.topic = topic;
        this.links = result.links;
        if (!result.found) this.onUnsupported?.(`도움말 주제 '${topic}'`, '도움말 파일에서 앵커를 찾지 못했습니다.');
        const { fonts, colors } = this.resources;
        const context = createTextContext({ flags: 0, font: BODY_FONT_SLOT, color: colors.information, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 });
        const body = new ScrollingText(fonts, context);
        const slider = new VerticalSlider({
            now: this.now,
            onSound: (kind) => this.onSound?.(kind),
            onStep: (code) => body.handleStep(code),
            onChange: () => body.setScroll(slider.position),
        });
        body.attachSlider(slider);
        body.appendFragments(parseStyleText(result.styleText, context, colors, { resolvePicture: this.resources.resolvePicture, onUnsupported: (what, text) => this.onUnsupported?.(what, text) }));
        this.body = body;
        this._sizeBody();
        this.place();
        body.setScroll(0);
    }

    // 지금 보는 주제를 마친다: 이전 주제가 있으면 그 주제로, 없으면 창을 닫는다(원본 Back 버튼). (2026-10-30 WB03-G)
    back() {
        if (this.history.length === 0) {
            this.closeWindow();
            return;
        }
        this.history.pop();
        const previous = this.history[this.history.length - 1];
        if (previous === undefined) this.closeWindow();
        else this._show(previous);
    }

    // 창을 닫고 호스트에 알린다. 이미 닫혔으면 아무것도 하지 않는다. (2026-10-30 WB03-G)
    closeWindow() {
        if (this.closed) return;
        this.closed = true;
        this.onClose?.();
    }

    // 링크 번호를 따라간다: `cmd:이름,인자` 는 호스트 명령, 바깥 주소는 onExternal, 그 밖은 도움말 주제 요청이다. (2026-10-30 WB03-G)
    _followLink(tag) {
        const destination = this.links?.resolve(tag) ?? null;
        if (destination === null) return;
        if (/^cmd:/i.test(destination)) {
            const text = destination.slice(4);
            const comma = text.indexOf(',');
            this.onCommand?.({ name: comma < 0 ? text : text.slice(0, comma), argument: comma < 0 ? '' : text.slice(comma + 1) });
        } else if (isExternalDestination(destination)) {
            this.onExternal?.(destination);
        } else {
            this.open(destination);
        }
    }

    // 창 크기를 맞춘다(원본 Fit 에 해당하는 자리). 크기는 고정이므로 지역 배치만 다시 계산한다. 메인 메뉴가 화면 크기를 바꿀 때 부른다. (2026-10-30 WB03-G)
    fit() {
        this._layout();
    }

    // 화면 가운데(폭)·y 40 에 놓고 자식의 화면 위치를 정한다(원본 OpenQuickHelpWindow 의 부착). 자식 위치는 부모 위치 + 지역 위치를 float32 로 더한 값이다. (2026-10-30 WB03-G)
    place() {
        this.x = f32((this.screen.width - HELP_WINDOW_WIDTH) * 0.5);
        this.y = f32(HELP_WINDOW_TOP);
        this.frame = { x: f32(this.x + this.frameRect.x), y: f32(this.y + this.frameRect.y), width: this.frameRect.width, height: this.frameRect.height };
        this.inner = { x: f32(this.frame.x + this.innerRect.x), y: f32(this.frame.y + this.innerRect.y), width: this.innerRect.width, height: this.innerRect.height };
        if (this.body) {
            this.body.x = this.inner.x;
            this.body.y = this.inner.y;
            this.body.placeSlider();
        }
        [this.backButton, this.okButton].forEach((button, index) => button.attach(f32(this.x + this.buttonLocals[index].x), f32(this.y + this.buttonLocals[index].y)));
    }

    // 창 전체의 화면 정수 경계이다. (2026-10-30 WB03-G)
    bounds() {
        return boundsOf(this.x, this.y, this.width, this.height);
    }

    // 창을 그린다: 장식 패널(질감 + 명암 테두리 + 네 모서리 금장식) → 본문 눌린 테두리 → 안쪽 단색 칸 → 본문 글자 → 스크롤바 → Back·OK 버튼 순서이다. (2026-10-30 WB03-G)
    draw(surface) {
        if (this.closed) return;
        const { panel } = this.resources;
        const rect = this.bounds();
        drawGumpBackground(surface, panel.texture, planGumpBackground(rect, surface.bounds, panel.textureMetrics.width, panel.textureMetrics.height, PANEL_FLAGS), this.resources.shadeMaps);
        const right = (rect.left + ftol(this.width)) | 0;
        const bottom = (rect.top + ftol(this.height)) | 0;
        const anchors = [{ x: rect.left, y: rect.top }, { x: right, y: rect.top }, { x: right, y: bottom }, { x: rect.left, y: bottom }];
        // 네 모서리 금장식을 각 꼭짓점에 그린다.
        panel.corners.forEach((image, index) => surface.drawImage(image, (anchors[index].x + image.left) | 0, (anchors[index].y + image.top) | 0));
        const frame = boundsOf(this.frame.x, this.frame.y, this.frame.width, this.frame.height);
        drawGumpBackground(surface, panel.texture, planGumpBackground(frame, surface.bounds, panel.textureMetrics.width, panel.textureMetrics.height, BODY_FRAME_FLAGS), this.resources.shadeMaps);
        const inner = boundsOf(this.inner.x, this.inner.y, this.inner.width, this.inner.height);
        // 안쪽 칸을 단색으로 채운다(표면 밖은 자른다).
        for (let y = Math.max(0, inner.top); y < Math.min(surface.height, inner.bottom); y += 1) {
            surface.pixels.fill(this.backgroundColor, y * surface.width + Math.max(0, inner.left), y * surface.width + Math.min(surface.width, inner.right));
        }
        this.body.draw(surface, this.resources.fonts);
        this.body.drawSlider(surface, this.resources);
        this.backButton.draw(surface, this.resources);
        this.okButton.draw(surface, this.resources);
    }

    // 왼쪽 버튼 누름을 처리한다. 창 밖이면 닫고, 안이면 스크롤바 → Back·OK 버튼 → 본문(링크는 바로 따라가고 그 밖은 끌기 시작) 순으로 받는다. 모달이므로 항상 true 이다. (2026-10-30 WB03-G)
    pointerDown(x, y) {
        if (this.closed) return false;
        const localX = f32(x - this.x);
        const localY = f32(y - this.y);
        if (!(localX >= 0) || localX >= this.width || !(localY >= 0) || localY >= this.height) {
            this.closeWindow();
            return true;
        }
        if (this.body.slider.pointerDown(x, y)) return true;
        for (const button of [this.backButton, this.okButton]) {
            if (button.pointerDown(x, y)) {
                this.onSound?.('button');
                return true;
            }
        }
        const bounds = this.body.bounds();
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return true;
        const fragment = this.body.hitTest(x, y);
        if (fragment && fragment.tag) this._followLink(fragment.tag);
        else this.dragY = y;
        return true;
    }

    // 포인터 이동을 처리한다. 스크롤바·버튼의 눌린 모양을 갱신하고, 본문을 끄는 중이면 이동량만큼 반대로 스크롤한다. 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    pointerMove(x, y) {
        if (this.closed) return false;
        let changed = this.body.slider.pointerMove(x, y);
        for (const button of [this.backButton, this.okButton]) if (button.broadcast(x, y)) changed = true;
        if (this.dragY !== null) {
            const delta = y - this.dragY;
            if (delta !== 0) {
                this.dragY = y;
                this.body.scrollBy(-delta);
                changed = true;
            }
        }
        return changed;
    }

    // 왼쪽 버튼 뗌을 처리한다. 붙잡힌 버튼을 안쪽에서 뗐으면 실행(Back·OK)하고, 끌기·스크롤바 누름을 끝낸다. 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    pointerUp(x, y) {
        if (this.closed) return false;
        let changed = this.body.slider.pointerUp(x, y);
        if (this.dragY !== null) {
            this.dragY = null;
            changed = true;
        }
        for (const button of [this.backButton, this.okButton]) {
            const wasCaptured = button.captured;
            const activated = button.pointerUp(x, y);
            if (wasCaptured) changed = true;
            if (activated) {
                if (button.command === 'back') this.back();
                else this.closeWindow();
            }
        }
        return changed;
    }

    // 진행 중인 누름·끌기를 실행 없이 취소한다(포커스 상실·화면 크기 변경). 그림이 바뀌었으면 true. (2026-10-30 WB03-G)
    cancel() {
        let changed = this.body.slider.cancel();
        if (this.dragY !== null) {
            this.dragY = null;
            changed = true;
        }
        for (const button of [this.backButton, this.okButton]) if (button.cancel()) changed = true;
        return changed;
    }

    // 키 입력을 처리한다(원본 DispatchQuickHelpWindow): PageUp·PageDown 은 본문 높이, 위·아래 화살표는 10픽셀 스크롤한다. 처리했으면 true. code 는 KeyboardEvent.code 이다. (2026-10-30 WB03-G)
    keyDown(code) {
        if (this.closed) return false;
        if (code === 'PageUp') this.body.scrollBy(ftol(-this.body.baseHeight));
        else if (code === 'PageDown') this.body.scrollBy(ftol(this.body.baseHeight));
        else if (code === 'ArrowUp') this.body.scrollBy(-KEY_STEP);
        else if (code === 'ArrowDown') this.body.scrollBy(KEY_STEP);
        else return false;
        return true;
    }

    // 마우스 휠을 처리한다(웹 적응). deltaY 가 양수면 아래로, 음수면 위로 본문 세 줄만큼 스크롤한다. 처리했으면 true. (2026-10-30 WB03-G)
    wheel(deltaY) {
        if (this.closed || deltaY === 0) return false;
        this.body.scrollBy(deltaY > 0 ? WHEEL_STEP : -WHEEL_STEP);
        return true;
    }

    // 시간 경과를 알린다. 눌린 화살표의 반복 이동을 진행한다. 그림이 바뀌었으면 true. nowSeconds 는 메인 메뉴 호환용이며 시계는 생성 때 받은 now 를 쓴다. (2026-10-30 WB03-G)
    update() {
        if (this.closed) return false;
        return this.body.slider.update();
    }

    // 창을 닫는다(메인 메뉴가 다른 대화상자로 바꿀 때). 이후 입력·시간·그리기를 받지 않는다. $OnExit 같은 닫기 명령은 없으므로 null 을 돌려준다. (2026-10-30 WB03-G)
    close() {
        this.closed = true;
        return null;
    }
}

// 도움말 창을 만든다. 도움말 파일은 설정 키 `Quick Help Spec`(`\d\help.<언어>`)의 경로에서 읽고 언어 파일이 없으면 영어 파일로 대체한다. 파일이 없으면 null 이다(원본: 로드 오류).
// topic 은 처음 보일 주제 이름(F1 은 'F1Help')이다. 사용: const result = createHelpWindow({ gameData, resources, topic: 'F1Help', screen }); result.window?.fit(). (2026-10-30 WB03-G)
export function createHelpWindow({ gameData, resources, topic = 'F1Help', screen, now = () => 0, onClose = null, onCommand = null, onExternal = null, onSound = null, onUnsupported = null }) {
    const path = gameData.get('Quick Help Spec');
    const helpText = path ? gameData.readText(path) : null;
    if (helpText === null) return { window: null, missing: true, path };
    const window = new HelpWindow({ resources, helpText, substitute: (text) => gameData.substitute(text), screen, now, onClose, onCommand, onExternal, onSound, onUnsupported });
    window.open(topic);
    window.fit();
    window.place();
    return { window, missing: false, path };
}
