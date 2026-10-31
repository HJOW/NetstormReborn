// 원본 HTML 대화상자(HtmlWindowGump)를 복원하는 모듈이다. 메인 메뉴의 Campaign·Demo·Credits 버튼이 여는 Tell 창이 이것이다.
// 돌 질감 패널(네 모서리 금장식) 안에 제목·본문(스타일 글자), 선택 목록(`$Menu=`), 돌 버튼(`$Button=`)이 들어가고, 내용에 맞춰 크기가 정해져 화면 가운데에 놓인다.
// 원본 주소: 생성 004cf120, 공장(본문 준비·배치·모달 등록) 004cf270, 전체 배치 Fit 004c7d40, 장식 배경 그리기(GumpDecoration::Draw),
//            지시문 추출 004c83b0, 유휴(Timeout) 004c8350. 복원 기준: cpppj HtmlWindowFit.cpp·GumpDecoration.cpp·HtmlWindowFactory.cpp(10.78 경로).
// 본문이 화면보다 길면 Fit 이 스크롤바를 켠다(원본: 슬라이더·스크롤 허용 켜기 → 스크롤 위치 0 요청). 본문 글자를 끌거나 스크롤바·마우스 휠로 움직인다.
// 본문의 링크(`<a href>`)를 누르면 대화상자를 닫고 링크를 분배한다(원본 HtmlWindowGump::Handle 의 tagEvent: 부모를 먼저 Kill 한 뒤 링크 처리). 분배는 호스트(onLink)가 한다.
// 두 열 메뉴는 이후 단위에서 옮긴다(해당 경로에 닿으면 onUnsupported 로 알림).
// 한국어 모드에서는 본문·단추·목록 글자를 유니코드 글꼴 코드(FontSet.encode)로 만들어 D2Coding 으로 그린다(단추 라벨은 한국어 Tell 파일의 문구다).
// (2026-10-28 WB03-D: 메인 메뉴 다음 화면(Credits 먼저)을 원본과 같은 배치로 그리기 위해 추가, 2026-10-31 WB03-K: 링크 훅과 본문 스크롤바 추가, 2026-10-31 WB03-O: 한국어 글자 부호화)
import { createTextContext } from '../render/glyph-text.js';
import { GumpBackgroundFlags, drawGumpBackground, planGumpBackground } from '../render/gump-visual.js';
import { StoneButton } from './stone-button.js';
import { MenuList } from './menu-list.js';
import { ScrollingText } from './scroll-text.js';
import { CURSOR_NORMAL } from './cursor.js';
import { VerticalSlider } from './slider.js';
import { parseStyleText } from './style-text.js';
import { LinkTable, applyHtmlCondition, prepareDialogContent } from './html-text.js';
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

// 마우스 휠 한 칸의 스크롤 양(본문 세 줄, 픽셀)이다. 원본에 휠 스크롤은 없고 도움말 창과 같은 웹 적응 값이다. (2026-10-31 WB03-K)
const WHEEL_STEP = 45;

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
// 입력은 pointerDown/pointerMove/pointerUp/cancel/wheel, 시간 경과는 update(초)로 넘긴다. 명령이 실행될 때 onCommand(명령)가 불린다. (2026-10-28 WB03-D)
// 본문 링크를 누르면 onLink(목적지 문자열)가 불린다. links 는 본문 변환이 모은 링크 표(LinkTable)이며 없으면 링크가 없는 본문이다.
// now 는 스크롤바 화살표의 반복 기한에 쓰는 시계(초)를 돌려주는 함수이다. (2026-10-31 WB03-K: links·onLink·now 추가)
export class TellWindow {
    // resources 는 메인 메뉴 자원 묶음(fonts·colors·panel 등 포함), content 는 prepareDialogContent 의 결과, screen 은 논리 화면 크기이다.
    // requested 는 호출자가 바라는 폭·높이이며 Tell 은 0(내용 맞춤)이다. nowSeconds 는 Timeout 기한을 계산할 현재 시각(초)이다.
    constructor({ resources, content, screen, requested = { width: 0, height: 0 }, nowSeconds = 0, links = null, now = () => 0, onCommand = null, onLink = null, onSound = null, onUnsupported = null, onTrace = null }) {
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
        // 본문 링크 콜백이다. 링크 목적지 문자열을 받는다(창을 닫는 일과 링크 분배는 호출자가 한다).
        this.onLink = onLink;
        // 본문 링크 표이다(링크 번호 → 목적지). 링크가 없으면 null.
        this.links = links;
        // 시계(초)를 돌려주는 함수이다. 스크롤바 화살표의 반복 이동 기한에 쓴다.
        this.now = now;
        // 본문을 누른 채 끄는 중이면 이전 y 이다. 아니면 null 이다(원본 dragY + notify 비트).
        this.dragY = null;
        // 효과음 콜백이다. 'button' 은 돌 버튼 누름(스크롤바 화살표 포함), 'menu' 는 목록 행 명령이다.
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
        // 글자 부호화는 한국어일 때 UTF-16 코드 단위(유니코드 글꼴), 영어일 때 Windows-1252 바이트(원본 글꼴)다. (2026-10-31 WB03-O)
        const encode = (text) => fonts.encode(text);
        this.body.appendFragments(parseStyleText(content.styleText, this.body.defaultContext, colors, { encode, onUnsupported: (what, text) => this.onUnsupported?.(what, text) }));
        // 돌 버튼 목록이다(지시문 순서). 라벨은 번역을 거친 뒤 글자 크기에 맞춰진다. 라벨은 Tell 파일(`$Button=라벨,…`)의 문구이며 한국어 파일에서는 이미 한국어다. (2026-10-31 WB03-O: encode)
        this.buttons = (content.directives?.buttons ?? []).map((spec) => new StoneButton({ label: spec.label, command: { name: spec.command.name, argument: spec.command.argument, instant: spec.instant }, encode }));
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
    // 5) 본문이 화면(폭 - 40, 높이)에 들어가지 않으면 폭을 화면에 맞추고 높이를 폭의 절반(요청 높이가 더 크면 그 값)으로 줄여 스크롤바를 켠다(_enableScroll).
    // 다시 맞출 때(화면 크기 변경)는 이전 스크롤바와 스크롤 위치를 버리고 처음부터 잰다. 원본은 창마다 한 번만 맞추지만 웹은 화면 크기가 바뀌면 다시 맞춘다. (2026-10-31 WB03-K)
    fit() {
        const { font, fonts } = this.resources;
        const body = this.body;
        body.slider = null;
        body.useSlider = false;
        body.scroll = 0;
        body.dirty = true;
        this.dragY = null;
        let maximum = 0;
        // 각 버튼을 글자에 맞추고 가장 넓은 폭을 찾는다.
        for (const button of this.buttons) {
            button.fitToText(font, fonts);
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
            // 화면에 다 들어가지 않으면 화면에 맞추고 스크롤바를 쓴다.
            if (this.screen.width < target + PADDING_WIDTH || this.screen.height < body.baseHeight + PADDING_HEIGHT) {
                const limit = (this.screen.width - SCREEN_MARGIN_LIMIT) | 0;
                if (target > limit) target = limit;
                body.setWidth(target);
                body.setHeight(target * 0.5);
                if (this.requested.height) {
                    const requestedHeight = (this.requested.height | 0) - PADDING_HEIGHT;
                    if (requestedHeight > body.baseHeight) body.setHeight(requestedHeight);
                }
                this._enableScroll();
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
        // 스크롤바를 켠 본문은 크기가 정해진 뒤 스크롤 위치를 맨 위로 요청한다(원본 RequestScroll(0)). 스크롤바의 범위·쪽 크기·손잡이가 이때 정해진다.
        if (body.slider) body.setScroll(0);
    }

    // 본문에 스크롤바를 달고 스크롤을 허용한다(원본 Fit 의 SetSlider(1)·SetScrollable(1)). 스크롤바의 단계 요청은 본문이 처리하고,
    // 끌어서 바뀐 위치는 본문 스크롤에 반영한다. 스크롤바를 붙이는 위치는 place 가 정한다(본문 오른쪽 가장자리). (2026-10-31 WB03-K)
    _enableScroll() {
        const body = this.body;
        const slider = new VerticalSlider({
            now: this.now,
            onSound: (kind) => this.onSound?.(kind),
            onStep: (code) => body.handleStep(code),
            onChange: () => body.setScroll(slider.position),
        });
        body.attachSlider(slider);
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
        // 스크롤바가 있으면 본문 오른쪽 가장자리에 붙이고 높이를 본문 높이에 맞춘다(원본 본문 Attach). (2026-10-31 WB03-K)
        this.body.placeSlider();
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

    // 대화상자를 그린다: 장식 배경(질감 + 명암 테두리) → 네 모서리 금장식 → 본문(+ 스크롤바) → 목록 → 돌 버튼 순서다(원본 부모 Draw 뒤 자식 Draw).
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
        this.body.drawSlider(surface, this.resources);
        this.menu?.draw(surface, this.resources);
        // 모든 버튼을 그린다.
        for (const button of this.buttons) button.draw(surface, this.resources);
    }

    // 지정한 명령을 실행한다. 창이 닫혔으면 무시한다.
    _run(command) {
        if (this.closed || !command) return;
        this.onCommand?.(command);
    }

    // 왼쪽 버튼 누름을 처리한다. 스크롤바가 있으면 먼저 스크롤바가 받는다(화살표·홈·손잡이).
    // 돌 버튼을 누르면 눌린 모양과 button 효과음, 즉시 실행 버튼(ButtonInstant)이면 바로 명령을 실행한다.
    // 목록 행을 누르면 즉시 명령을 실행하고 menu 효과음을 낸다. 본문 링크를 누르면 효과음 없이 바로 링크를 따라가고(onLink), 링크가 아닌 본문 글자·빈 곳은 끌기를 시작한다
    // (끌기는 스크롤바가 있을 때만 본문을 움직인다). 어디를 눌렀든 대화상자(모달)가 입력을 소비하므로 항상 true 이다. (2026-10-31 WB03-K: 스크롤바·링크 추가)
    pointerDown(x, y) {
        if (this.closed) return false;
        if (this.body.slider?.pointerDown(x, y)) return true;
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
            return true;
        }
        const bounds = this.body.bounds();
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return true;
        const fragment = this.body.hitTest(x, y);
        if (fragment && fragment.tag) this._followLink(fragment.tag);
        else if (this.body.slider) this.dragY = y;
        return true;
    }

    // 링크 번호를 따라간다(원본 tagEvent): 번호가 링크 표에 없으면(0 포함) 아무 일도 없다. 있으면 목적지를 호스트(onLink)에 넘긴다.
    // 호스트가 이 창을 닫고 링크를 분배한다(원본: 부모 Kill 뒤 도움말/명령/바깥 주소 분배). 닫힌 창은 pointerDown 이 먼저 거르므로 여기까지 오지 않는다. (2026-10-31 WB03-K)
    _followLink(tag) {
        const destination = this.links?.resolve(tag) ?? null;
        if (destination === null) return;
        this.onLink?.(destination);
    }

    // 포인터 이동을 처리한다. 붙잡힌 돌 버튼의 눌린 모양과 목록 행 강조, 스크롤바의 눌린 모양을 갱신하고, 본문을 끄는 중이면 이동량만큼 반대로 스크롤한다. 그림이 바뀌면 true.
    pointerMove(x, y) {
        if (this.closed) return false;
        let changed = this.body.slider?.pointerMove(x, y) ?? false;
        // 붙잡힌 버튼은 포인터가 안에 있을 때만 눌린 모양을 보인다.
        for (const button of this.buttons) if (button.broadcast(x, y)) changed = true;
        if (this.menu?.pointerMove(x, y)) changed = true;
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

    // 왼쪽 버튼 뗌을 처리한다. 붙잡힌 돌 버튼 안에서 뗐으면 그 버튼의 명령을 실행한다. 스크롤바 누름·본문 끌기를 끝낸다. 그림이 바뀌었으면 true.
    pointerUp(x, y) {
        if (this.closed) return false;
        let activated = null;
        let changed = this.body.slider?.pointerUp(x, y) ?? false;
        if (this.dragY !== null) {
            this.dragY = null;
            changed = true;
        }
        // 붙잡힌 버튼을 찾아 붙잡기를 풀고 실행 여부를 받는다.
        for (const button of this.buttons) {
            const wasCaptured = button.captured;
            if (button.pointerUp(x, y)) activated = button;
            if (wasCaptured) changed = true;
        }
        if (activated) this._run(activated.command);
        return changed;
    }

    // 진행 중인 누름·끌기를 실행 없이 취소한다(포커스 상실·화면 크기 변경). 그림이 바뀌었으면 true.
    cancel() {
        let changed = this.body.slider?.cancel() ?? false;
        if (this.dragY !== null) {
            this.dragY = null;
            changed = true;
        }
        // 모든 버튼의 붙잡기·눌린 모양을 푼다.
        for (const button of this.buttons) if (button.cancel()) changed = true;
        if (this.menu?.pointerLeave()) changed = true;
        return changed;
    }

    // 화면 좌표 위에서 보일 커서 번호를 돌려준다. 링크 위는 가리키는 손(16), 스크롤바가 달린 본문 위는 상하 화살표(15), 그 밖(버튼·목록·패널·닫힌 창)은 기본 화살표(1)이다.
    // 판정은 본문(ScrollingText.cursorAt)이 한다. 사용: const number = window.cursorAt(x, y); (2026-10-31 WB03-L)
    cursorAt(x, y) {
        if (this.closed) return CURSOR_NORMAL;
        return this.body.cursorAt(x, y);
    }

    // 마우스 휠을 처리한다(웹 적응, 원본에 없음). 스크롤바가 있을 때 deltaY 가 양수면 아래로, 음수면 위로 본문 세 줄(45픽셀)만큼 스크롤한다. 처리했으면 true. (2026-10-31 WB03-K)
    wheel(deltaY) {
        if (this.closed || deltaY === 0 || !this.body.slider) return false;
        this.body.scrollBy(deltaY > 0 ? WHEEL_STEP : -WHEEL_STEP);
        return true;
    }

    // 시간 경과를 알린다(원본 유휴 4c8350). 눌린 스크롤바 화살표의 반복 이동을 진행하고, Timeout 기한이 지나면 한 번만 그 명령을 실행한다. 그림이 바뀌었거나 명령이 실행됐으면 true 이다.
    update(nowSeconds) {
        if (this.closed) return false;
        let changed = this.body.slider?.update() ?? false;
        if (this.timeout && nowSeconds >= this.timeout.deadline) {
            const { command } = this.timeout;
            this.timeout = null;
            this._run(command);
            changed = true;
        }
        return changed;
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
//  5) 본문의 링크(`<a href="목적지">`)는 링크 표(LinkTable)에 모아 StyleText 태그 `~[T번호]` 로 바꾼다. 링크를 누르면 창의 onLink(목적지)가 불린다.
// gameData 는 GameData(설정 치환·Tell 섹션·이름 기록), resources 는 메뉴 자원 묶음이다. 본문이 영숫자가 없는 빈 본문이면 창을 만들지 않고 { empty: true } 로 알린다.
// 대상이 원본 지역 입력 범위(1024바이트)를 넘으면 없는 섹션처럼 { missing: true } 이다.
// now 는 스크롤바 화살표 반복에 쓰는 시계(초)를 돌려주는 함수이다.
// 반환: { window, empty, missing, blank, inlineCommands[{ name, argument, number }] }.
// 사용: const result = createTellWindow({ gameData, resources, name: 'Credits', screen, nowSeconds, onCommand, onLink }); result.window?.fit();
// (2026-10-28 WB03-D, 2026-10-30 WB03-J: BLANK·지역 인자·이름 기록·조건 태그·인라인 명령 추가, 2026-10-31 WB03-K: 링크 표·onLink·now 추가)
export function createTellWindow({ gameData, resources, name, screen, nowSeconds = 0, now = () => 0, onCommand = null, onLink = null, onSound = null, onUnsupported = null, onTrace = null, hooks = {} }) {
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
        // 스크립트 명령은 모아 두고, 링크는 링크 표에 모으며, 지원하지 않는 기능은 onUnsupported 로 알린다.
        const links = new LinkTable();
        const mergedHooks = {
            expandMenu: (label, command, argument) => expandFileList(gameData, label, command, argument),
            condition: (parserState, raw, closing) => applyHtmlCondition(parserState, raw, closing, (text) => gameData.substitute(text)),
            command: (parserState, raw) => inlineCommands.push(parseScriptCommand(raw)),
            link: (parserState, raw, closing) => links.text(raw, closing),
            onUnsupported,
            ...hooks,
        };
        const content = prepareDialogContent(body, { substitute: (text) => gameData.substitute(text), okButton: gameData.get('OkButton') ?? undefined, hooks: mergedHooks });
        if (content.empty) return { window: null, empty: true, missing: false, blank: false, inlineCommands };
        const window = new TellWindow({ resources, content, screen, nowSeconds, links, now, onCommand, onLink, onSound, onUnsupported, onTrace });
        window.fit();
        window.place();
        return { window, empty: false, missing: false, blank: false, inlineCommands };
    } finally {
        parameters.dispose();
    }
}
