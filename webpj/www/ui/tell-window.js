// 원본 HTML 대화상자(HtmlWindowGump)를 복원하는 모듈이다. 메인 메뉴의 Campaign·Demo·Credits 버튼이 여는 Tell 창이 이것이다.
// 돌 질감 패널(네 모서리 금장식) 안에 제목·본문(스타일 글자), 선택 목록(`$Menu=`), 돌 버튼(`$Button=`)이 들어가고, 내용에 맞춰 크기가 정해져 화면 가운데에 놓인다.
// 원본 주소: 생성 004cf120, 공장(본문 준비·배치·모달 등록) 004cf270, 전체 배치 Fit 004c7d40, 장식 배경 그리기(GumpDecoration::Draw),
//            지시문 추출 004c83b0, 유휴(Timeout) 004c8350. 복원 기준: cpppj HtmlWindowFit.cpp·GumpDecoration.cpp·HtmlWindowFactory.cpp(10.78 경로).
// 스크롤바·마우스 링크·조건 태그·그림·두 열 메뉴는 이후 단위에서 옮긴다(해당 경로에 닿으면 onUnsupported 로 알림).
// (2026-10-28 WB03-D: 메인 메뉴 다음 화면(Credits 먼저)을 원본과 같은 배치로 그리기 위해 추가)
import { createTextContext } from '../render/glyph-text.js';
import { GumpBackgroundFlags, drawGumpBackground, planGumpBackground } from '../render/gump-visual.js';
import { StoneButton } from './stone-button.js';
import { MenuList } from './menu-list.js';
import { ScrollingText } from './scroll-text.js';
import { parseStyleText } from './style-text.js';
import { applyHtmlCondition, prepareDialogContent } from './html-text.js';
import { expandFileList } from './tell-lists.js';
import { parseTellParameters } from './tell-parameters.js';
import { parseScriptCommand } from './script-command.js';
import { Config, asciiLower } from '../assets/config.js';

// 본문 기본 글꼴 슬롯이다. 원본 본문 생성 004c7aa0 은 스크롤 글자 생성자(004bc060)에 (폭 0, 높이 0, 글꼴 0, 색 = 기본 흰색 전역 005bec2c)를 넘긴다
// (Arial 14, 굵기 700, 그림자 없음). cpppj 의 HtmlBodyGump 는 슬롯 5·설명 색을 쓰지만 원본 스크린샷(Demo 본문: 굵은 흰 글자)과 디컴파일은 슬롯 0·흰색이다. (2026-10-28 WB03-D)
const BODY_FONT_SLOT = 0;

// 본문 좌우 여백 합계(40)와 본문 위아래 여백 합계(65)다. 패널 폭 = 본문 폭 + 40, 패널 높이 = 본문 높이 + 65 이다(원본 Fit 의 상수). (2026-10-28 WB03-D)
const PADDING_WIDTH = 40;
const PADDING_HEIGHT = 65;

// 본문 위쪽 여백(본문 y 위치)이다. 목록·버튼은 본문 높이 + 30 에서 시작해 본문과 10픽셀 떨어진다. (2026-10-28 WB03-D)
const BODY_TOP = 20;
const CONTROLS_GAP_BELOW_BODY = 30;

// 버튼 사이 간격과 메뉴 아래·버튼 위 간격이다(원본 15, 10). (2026-10-28 WB03-D)
const BUTTON_GAP = 15;
const MENU_BOTTOM_GAP = 10;

// 목록의 최소 왼쪽 위치이다(원본 20). (2026-10-28 WB03-D)
const MENU_MIN_X = 20;

// 긴 본문 맞춤에서 폭을 넓혀 가는 단위(20)와 높이가 같은 채로 허용하는 폭 차이(50)이다. 종횡비 판정은 폭×2 < 높이×4 이다. (2026-10-28 WB03-D)
const WIDEN_STEP = 20;
const SAME_HEIGHT_TOLERANCE = 50;

// 화면 가장자리 여백(좌우 합계 80, 화면 폭 - 40 한도)이다. (2026-10-28 WB03-D)
const SCREEN_MARGIN_WIDE = 80;
const SCREEN_MARGIN_LIMIT = 40;

// 장식 배경 플래그이다: 질감 + 네 테두리(밝은 왼쪽·위, 어두운 오른쪽·아래). 돌 버튼과 같은 값이다. (2026-10-28 WB03-D)
const PANEL_BACKGROUND_FLAGS = GumpBackgroundFlags.BUTTON;

// 원본 CRT _ftol 처럼 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-28 WB03-D)
function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// float32 로 저장한다. (2026-10-28 WB03-D)
const f32 = Math.fround;

// Tell 대화상자 하나이다. 본문(ScrollingText)·목록(MenuList)·돌 버튼 목록을 가지며 fit 으로 크기와 자식 위치를 정한다.
// 사용: const window = new TellWindow({ resources, content, screen: { width, height }, onCommand }); window.fit(); window.place(); window.draw(surface).
// 입력은 pointerDown/pointerMove/pointerUp/cancel, 시간 경과는 update(초)로 넘긴다. 명령이 실행될 때 onCommand(명령)가 불린다. (2026-10-28 WB03-D)
export class TellWindow {
    // resources 는 메인 메뉴 자원 묶음(fonts·colors·panel 등 포함), content 는 prepareDialogContent 의 결과, screen 은 논리 화면 크기이다.
    // requested 는 호출자가 바라는 폭·높이이며 Tell 은 0(내용 맞춤)이다. nowSeconds 는 Timeout 기한을 계산할 현재 시각(초)이다.
    constructor({ resources, content, screen, requested = { width: 0, height: 0 }, nowSeconds = 0, onCommand = null, onSound = null, onUnsupported = null, onTrace = null }) {
        // 그리기 자원 묶음이다.
        this.resources = resources;
        // 준비한 본문 정보(스타일 문자열·지시문·배경 이름)이다.
        this.content = content;
        // 논리 화면 크기 { width, height } 이다. 대화상자는 이 화면(보기 영역) 가운데에 놓인다.
        this.screen = screen;
        // 호출자가 요청한 폭·높이이다(원본 +0x21c0·+0x21c4). Tell 은 둘 다 0 이다.
        this.requested = requested;
        // 명령 실행 콜백이다.
        this.onCommand = onCommand;
        // 효과음 콜백이다. 'button' 은 돌 버튼 누름, 'menu' 는 목록 행 명령이다.
        this.onSound = onSound;
        // 지원하지 않는 기능에 닿았을 때 알리는 콜백이다.
        this.onUnsupported = onUnsupported;
        // 본문 폭 맞춤 반복의 각 단계를 알리는 개발용 콜백이다({ target, width, height, previousWidth, previousHeight }).
        this.onTrace = onTrace;
        // 배율 적용 전 폭(원본 +0x3c)이다.
        this.baseWidth = 0;
        // 배율 적용 전 높이(원본 +0x40)다.
        this.baseHeight = 0;
        // 배율이다.
        this.scale = 1;
        // 배율 적용 폭(float32)이다.
        this.width = 0;
        // 배율 적용 높이(float32)다.
        this.height = 0;
        // 화면 x 위치(float32)다.
        this.x = 0;
        // 화면 y 위치(float32)다.
        this.y = 0;
        // 본문 자식의 부모 기준 위치이다(float32).
        this.bodyLocal = { x: 0, y: 0 };
        // 목록 자식의 부모 기준 위치이다(float32). 목록이 없으면 null.
        this.menuLocal = null;
        // 버튼 자식들의 부모 기준 위치 목록이다.
        this.buttonLocals = [];
        // 닫혔는지 여부이다. 닫힌 창은 입력·시간을 받지 않는다.
        this.closed = false;
        // Timeout 기한(초, 시계 기준)과 명령이다. 없으면 null.
        this.timeout = content.directives?.timeout ? { deadline: nowSeconds + content.directives.timeout.seconds, command: content.directives.timeout.command } : null;
        // 창이 닫힐 때 실행할 명령($OnExit)이다. 없으면 null.
        this.onExit = content.directives?.onExit ?? null;
        const { fonts, colors } = resources;
        // 본문 글자 Gump 이다. 기본 문맥은 슬롯 0, 흰색, 그림자 오프셋 (1,1)이고 효과 플래그는 꺼져 있다(원본 +0x80: {0, 글꼴, 색, 0, 0, 0, 1, 1}).
        this.body = new ScrollingText(fonts, createTextContext({ flags: 0, font: BODY_FONT_SLOT, color: colors.white, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 }));
        // 본문 스타일 문자열을 조각으로 나눠 본문에 넣는다.
        this.body.appendFragments(parseStyleText(content.styleText, this.body.defaultContext, colors, { onUnsupported: (what, text) => this.onUnsupported?.(what, text) }));
        // 돌 버튼 목록이다(지시문 순서). 라벨은 번역을 거친 뒤 글자 크기에 맞춰진다.
        this.buttons = (content.directives?.buttons ?? []).map((spec) => new StoneButton({ label: spec.label, command: { name: spec.command.name, argument: spec.command.argument, instant: spec.instant } }));
        // 선택 목록이다. 항목이 없으면 null.
        this.menu = null;
        const menuItems = content.directives?.menuItems ?? [];
        if (menuItems.length > 0) {
            this.menu = new MenuList({ fonts, colors });
            // 지시문 순서대로 항목을 추가한다.
            for (const item of menuItems) {
                if (item.kind === 'checked') this.menu.addChecked(item.label, item.command, item.checked !== 0, item.enabled !== 0);
                else if (item.kind === 'separator') this.menu.addSeparator();
                else this.menu.addText(item.label, item.command, item.enabled !== false);
            }
        }
    }

    // 본문·목록·버튼 크기를 정하고 패널 크기와 자식 위치(부모 기준)를 정한다(원본 Fit, 004c7d40).
    // 1) 버튼을 글자에 맞추고 가장 넓은 폭을 모두에게 쓴다. 2) 버튼 열 전체 폭(span)과 요청 폭 - 40 중 큰 값을 본문 폭 후보로 삼는다.
    // 3) 본문 자연 폭이 후보보다 넓으면 후보 폭에서 시작해 20씩 넓혀 가며 줄 나눔을 다시 해, 폭 ≥ 높이×2 이거나 높이가 더 줄지 않을 때까지 맞춘다.
    // 4) 패널 폭 = 본문 폭 + 40, 높이 = 본문 높이 + 65(+ 목록 높이 + 10), 본문은 가운데·y 20, 목록은 본문 아래 가운데, 버튼은 목록 아래 가운데에 둔다.
    fit() {
        const { font } = this.resources;
        const body = this.body;
        let maximum = 0;
        // 각 버튼을 글자에 맞추고 가장 넓은 폭을 찾는다.
        for (const button of this.buttons) {
            button.fitToText(font);
            if (!(maximum > button.baseWidth)) maximum = button.baseWidth;
        }
        const count = this.buttons.length;
        let span = f32(count * maximum);
        if (count > 1) span = f32((count * BUTTON_GAP - BUTTON_GAP) + span);
        const requested = (this.requested.width | 0) - PADDING_WIDTH;
        let target = span > requested ? span : f32(requested);
        body.fitContent(0, 0);
        if (!(body.baseWidth <= target)) {
            let previousHeight = 0;
            let previousWidth = 0;
            // 후보 폭에서 시작해 폭을 넓혀 가며 가장 알맞은 폭을 찾는다.
            for (;;) {
                body.setWidth(target);
                body.fitHeight();
                this.onTrace?.({ target, width: body.baseWidth, height: body.baseHeight, previousWidth, previousHeight });
                const available = this.screen.width;
                // 화면 폭이 모자라면 화면 폭 - 80 으로 고정하고 끝낸다.
                if ((available - SCREEN_MARGIN_LIMIT) < target + PADDING_WIDTH) {
                    body.setWidth((available - SCREEN_MARGIN_WIDE) | 0);
                    body.fitHeight();
                    break;
                }
                if (this.requested.height) {
                    const height = (this.requested.height | 0) - PADDING_HEIGHT;
                    if (height > body.baseHeight) body.setHeight(height);
                }
                // 높이가 이전과 달라지면 그 높이와 이때의 폭을 기억한다(같은 높이를 이루는 가장 좁은 폭).
                if (body.baseHeight !== previousHeight) {
                    previousHeight = ftol(body.baseHeight);
                    previousWidth = ftol(target);
                }
                // 같은 높이로 폭이 50 넘게 늘어나기만 했다면 기억한 폭으로 되돌리고 끝낸다.
                if (target - previousWidth > SAME_HEIGHT_TOLERANCE) {
                    body.setWidth(previousWidth);
                    body.setHeight(previousHeight);
                    break;
                }
                target = f32(target + WIDEN_STEP);
                if (!(body.baseWidth * 2 < body.baseHeight * 4)) break;
            }
            // 화면에 다 들어가지 않으면 화면에 맞추고 스크롤바를 쓴다(스크롤바 그리기·입력은 아직 없음).
            if (this.screen.width < target + PADDING_WIDTH || this.screen.height < body.baseHeight + PADDING_HEIGHT) {
                const limit = (this.screen.width - SCREEN_MARGIN_LIMIT) | 0;
                if (target > limit) target = limit;
                body.setWidth(target);
                body.setHeight(target * 0.5);
                if (this.requested.height) {
                    const requestedHeight = (this.requested.height | 0) - PADDING_HEIGHT;
                    if (requestedHeight > body.baseHeight) body.setHeight(requestedHeight);
                }
                body.useSlider = true;
                this.onUnsupported?.('본문 스크롤바', '본문이 화면보다 깁니다');
            }
        } else {
            body.fitHeight();
        }
        const fullWidth = target + PADDING_WIDTH;
        this._setSize(fullWidth, body.baseHeight + PADDING_HEIGHT);
        const halfWidth = ftol(fullWidth * 0.5);
        let controlsY = ftol(body.baseHeight + CONTROLS_GAP_BELOW_BODY);
        this.menuLocal = null;
        if (this.menu) {
            const menuHalf = ftol(this.menu.baseWidth * 0.5);
            const menuHeight = ftol(this.menu.baseHeight);
            let menuX = (halfWidth - menuHalf) | 0;
            if (menuX < MENU_MIN_X) menuX = MENU_MIN_X;
            this.menuLocal = { x: f32(menuX), y: f32(controlsY) };
            this._setSize(this.baseWidth, menuHeight + this.baseHeight + MENU_BOTTOM_GAP);
            const menuWidth = (menuHalf * 2 + PADDING_WIDTH) | 0;
            if (menuWidth > this.baseWidth) this._setSize(menuWidth, this.baseHeight);
            controlsY = ftol(controlsY + this.menu.baseHeight + MENU_BOTTOM_GAP);
        }
        let buttonX = ftol((this.baseWidth - span) * 0.5);
        this.buttonLocals = [];
        // 버튼을 왼쪽부터 같은 폭으로 놓고 다음 버튼은 폭 + 15 만큼 옮긴다.
        for (const button of this.buttons) {
            button.setBaseWidth(maximum);
            const local = { x: f32(buttonX), y: f32(controlsY) };
            this.buttonLocals.push(local);
            buttonX = ftol(local.x + maximum + BUTTON_GAP);
        }
        this.bodyLocal = { x: f32(ftol(this.baseWidth - body.baseWidth) / 2 | 0), y: f32(BODY_TOP) };
    }

    // 패널 크기(기본·배율 적용)를 저장한다.
    _setSize(baseWidth, baseHeight) {
        this.baseWidth = f32(baseWidth);
        this.width = f32(baseWidth * this.scale);
        this.baseHeight = f32(baseHeight);
        this.height = f32(baseHeight * this.scale);
    }

    // 화면 가운데에 놓고 자식의 화면 위치를 정한다(원본 Fit 끝: 보기 영역 가운데). 보기 영역은 논리 화면 전체이다.
    // 자식 화면 위치는 부모 위치 + 지역 위치를 float32 로 더한 값이다. 화면 크기가 바뀌면 다시 부른다.
    place() {
        this.x = f32((this.screen.width - this.baseWidth) * 0.5);
        this.y = f32((this.screen.height - this.baseHeight) * 0.5);
        this.body.x = f32(this.x + this.bodyLocal.x);
        this.body.y = f32(this.y + this.bodyLocal.y);
        if (this.menu && this.menuLocal) {
            this.menu.x = f32(this.x + this.menuLocal.x);
            this.menu.y = f32(this.y + this.menuLocal.y);
        }
        // 각 버튼의 화면 위치를 정한다.
        this.buttons.forEach((button, index) => button.attach(f32(this.x + this.buttonLocals[index].x), f32(this.y + this.buttonLocals[index].y)));
    }

    // 화면 정수 경계(원본 GumpCollection::Bounds)다.
    bounds() {
        const left = ftol(this.x);
        const top = ftol(this.y);
        return { left, top, right: ftol(left + this.width), bottom: ftol(top + this.height) };
    }

    // 대화상자를 그린다: 장식 배경(질감 + 명암 테두리) → 네 모서리 금장식 → 본문 → 목록 → 돌 버튼 순서다(원본 부모 Draw 뒤 자식 Draw).
    // 모서리 그림은 패널의 네 꼭짓점(왼쪽 위·오른쪽 위·오른쪽 아래·왼쪽 아래)에 각 프레임의 기준점 오프셋을 더해 그린다.
    draw(surface) {
        const { panel, shadeMaps, fonts } = this.resources;
        const rect = this.bounds();
        const passes = planGumpBackground(rect, surface.bounds, panel.textureMetrics.width, panel.textureMetrics.height, PANEL_BACKGROUND_FLAGS);
        drawGumpBackground(surface, panel.texture, passes, shadeMaps);
        const position = { x: rect.left, y: rect.top };
        const right = (position.x + ftol(this.width)) | 0;
        const bottom = (position.y + ftol(this.height)) | 0;
        const anchors = [{ x: position.x, y: position.y }, { x: right, y: position.y }, { x: right, y: bottom }, { x: position.x, y: bottom }];
        // 네 모서리 그림을 차례로 그린다.
        panel.corners.forEach((image, index) => surface.drawImage(image, (anchors[index].x + image.left) | 0, (anchors[index].y + image.top) | 0));
        this.body.draw(surface, fonts);
        this.menu?.draw(surface, this.resources);
        // 모든 버튼을 그린다.
        for (const button of this.buttons) button.draw(surface, this.resources);
    }

    // 지정한 명령을 실행한다. 창이 닫혔으면 무시한다.
    _run(command) {
        if (this.closed || !command) return;
        this.onCommand?.(command);
    }

    // 왼쪽 버튼 누름을 처리한다. 돌 버튼을 누르면 눌린 모양과 button 효과음, 즉시 실행 버튼(ButtonInstant)이면 바로 명령을 실행한다.
    // 목록 행을 누르면 즉시 명령을 실행하고 menu 효과음을 낸다. 어디를 눌렀든 대화상자(모달)가 입력을 소비하므로 항상 true 이다.
    pointerDown(x, y) {
        if (this.closed) return false;
        // 돌 버튼 중 누른 위치를 포함하는 첫 버튼이 입력을 붙잡는다.
        for (const button of this.buttons) {
            if (button.pointerDown(x, y)) {
                this.onSound?.('button');
                if (button.command.instant) {
                    button.cancel();
                    this._run(button.command);
                }
                return true;
            }
        }
        const command = this.menu?.pointerDown(x, y) ?? null;
        if (command) {
            this.onSound?.('menu');
            this._run(command);
        }
        return true;
    }

    // 포인터 이동을 처리한다. 붙잡힌 돌 버튼의 눌린 모양과 목록 행 강조를 갱신한다. 그림이 바뀌면 true.
    pointerMove(x, y) {
        if (this.closed) return false;
        let changed = false;
        // 붙잡힌 버튼은 포인터가 안에 있을 때만 눌린 모양을 보인다.
        for (const button of this.buttons) if (button.broadcast(x, y)) changed = true;
        if (this.menu?.pointerMove(x, y)) changed = true;
        return changed;
    }

    // 왼쪽 버튼 뗌을 처리한다. 붙잡힌 돌 버튼 안에서 뗐으면 그 버튼의 명령을 실행한다. 그림이 바뀌었으면 true.
    pointerUp(x, y) {
        if (this.closed) return false;
        let activated = null;
        let changed = false;
        // 붙잡힌 버튼을 찾아 붙잡기를 풀고 실행 여부를 받는다.
        for (const button of this.buttons) {
            const wasCaptured = button.captured;
            if (button.pointerUp(x, y)) activated = button;
            if (wasCaptured) changed = true;
        }
        if (activated) this._run(activated.command);
        return changed;
    }

    // 진행 중인 누름을 실행 없이 취소한다(포커스 상실·화면 크기 변경). 그림이 바뀌었으면 true.
    cancel() {
        let changed = false;
        // 모든 버튼의 붙잡기·눌린 모양을 푼다.
        for (const button of this.buttons) if (button.cancel()) changed = true;
        if (this.menu?.pointerLeave()) changed = true;
        return changed;
    }

    // 시간 경과를 알린다(원본 유휴 4c8350). Timeout 기한이 지나면 한 번만 그 명령을 실행하고 true 를 돌려준다. 아니면 false 이다.
    update(nowSeconds) {
        if (this.closed || !this.timeout || nowSeconds < this.timeout.deadline) return false;
        const { command } = this.timeout;
        this.timeout = null;
        this._run(command);
        return true;
    }

    // 창을 닫는다. 이후 입력·시간·그리기 요청을 받지 않는다. 닫을 때 실행할 $OnExit 명령을 돌려준다(없으면 null).
    close() {
        this.closed = true;
        return this.onExit;
    }
}

// Tell 대상 문자열로 대화상자를 만든다(원본 004cf730 Tell 명령). 대상은 `섹션이름` 또는 `섹션이름(인자1,인자2)` 이다.
//  1) BLANK(대소문자 무시)는 창을 만들지 않는 조기 분기이다({ blank: true }). 이름도 기록하지 않는다.
//  2) 지역 설정 객체 `param` 을 등록하고 대상의 인자를 `1="값"`… 줄로 넣는다(본문의 `{param.1}`). param 은 본문 처리가 끝날 때까지만 살아 있다.
//  3) 섹션 이름을 현재 Tell 이름으로 기록하고(이름이 바뀌면 이전 이름이 `global.priorTell`) 섹션 본문을 찾는다. 없으면 { missing: true } 이다(원본: `TELL cannot find "이름".` 로그).
//  4) 본문을 설정 치환·변환해 창을 만든다. 조건 태그(`<?식>`)는 평가하고, 본문의 스크립트 명령(`<$config,키=값>` 등)은 실행하지 않고 inlineCommands 로 순서대로 돌려준다
//     (원본도 즉시 실행하지 않고 명령 큐에 넣어 다음 입력에서 처리하므로, 호출자가 창을 올린 뒤 실행한다).
// gameData 는 GameData(설정 치환·Tell 섹션·이름 기록), resources 는 메뉴 자원 묶음이다. 본문이 영숫자가 없는 빈 본문이면 창을 만들지 않고 { empty: true } 로 알린다.
// 대상이 원본 지역 입력 범위(1024바이트)를 넘으면 없는 섹션처럼 { missing: true } 이다.
// 반환: { window, empty, missing, blank, inlineCommands[{ name, argument, number }] }.
// 사용: const result = createTellWindow({ gameData, resources, name: 'Credits', screen, nowSeconds, onCommand }); result.window?.fit();
// (2026-10-28 WB03-D, 2026-10-30 WB03-J: BLANK·지역 인자·이름 기록·조건 태그·인라인 명령 추가)
export function createTellWindow({ gameData, resources, name, screen, nowSeconds = 0, onCommand = null, onSound = null, onUnsupported = null, onTrace = null, hooks = {} }) {
    const inlineCommands = [];
    if (asciiLower(name) === 'blank') return { window: null, empty: false, missing: false, blank: true, inlineCommands };
    // 지역 인자 객체이다. 같은 이름의 바깥 객체는 일시적으로 가려지고 본문 처리 뒤 내려간다.
    const parameters = new Config(gameData.registry, 'param');
    try {
        let sectionName;
        try {
            sectionName = parseTellParameters(name, { substitute: (text) => gameData.substitute(text), append: (line) => parameters.append(line) });
        } catch (error) {
            if (error instanceof RangeError) return { window: null, empty: false, missing: true, blank: false, inlineCommands };
            throw error;
        }
        const body = gameData.findTellSection(sectionName);
        if (body === null) return { window: null, empty: false, missing: true, blank: false, inlineCommands };
        // 본문 변환·지시문 추출에 쓰는 훅이다: `@`·`#` 파일 목록은 설정 객체를 등록해 행을 만들고, 조건 태그는 설정 값으로 평가하며,
        // 스크립트 명령은 모아 두고, 지원하지 않는 기능은 onUnsupported 로 알린다.
        const mergedHooks = {
            expandMenu: (label, command, argument) => expandFileList(gameData, label, command, argument),
            condition: (parserState, raw, closing) => applyHtmlCondition(parserState, raw, closing, (text) => gameData.substitute(text)),
            command: (parserState, raw) => inlineCommands.push(parseScriptCommand(raw)),
            onUnsupported,
            ...hooks,
        };
        const content = prepareDialogContent(body, { substitute: (text) => gameData.substitute(text), okButton: gameData.get('OkButton') ?? undefined, hooks: mergedHooks });
        if (content.empty) return { window: null, empty: true, missing: false, blank: false, inlineCommands };
        const window = new TellWindow({ resources, content, screen, nowSeconds, onCommand, onSound, onUnsupported, onTrace });
        window.fit();
        window.place();
        return { window, empty: false, missing: false, blank: false, inlineCommands };
    } finally {
        parameters.dispose();
    }
}
