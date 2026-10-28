// 원본 메인 메뉴 화면(구름 배경 + 타이틀 GIF + 돌 버튼 두 줄)을 복원하는 모듈이다.
// 원본 주소: 메인 메뉴 Gump 생성 004d0120(버튼 9칸), 배치 004ce940(가상 +0x20), 위치 004d0530, 버튼 사건 004cebf0,
//            타이틀 배경 Gump 배치(UberGump 생성자의 titleMenu 이미지), 구름 파일 00470bb0(CloudFileSpec).
// 배치 규칙(004ce940):
//  1) 모든 버튼을 글자에 맞춰 크기를 정한 뒤(폭=글자 폭+12) 가장 넓은 폭을 모든 버튼에 쓴다.
//  2) 윗줄은 0~3번 칸, 아랫줄은 4번 이후 칸이며, 빈 칸(null)은 건너뛰고 각 줄을 화면 가로 가운데에 놓는다(간격 4).
//  3) 윗줄 y = 화면높이/3 + 55, 아랫줄 y = 화면높이/3 + 78 이다(1024×768 에서 311 / 334).
// 웹 판본은 원본이 CD 없을 때 Intro 칸을 비우는 것과 같은 방식으로 Multiplayer·Edit·Quit 칸을 비운다(웹에서 쓸 수 없거나 구현하지 않는 기능).
// 그 결과 윗줄은 Campaign·Demo·Help 3개, 아랫줄은 Credits·Options 2개이며 각 줄은 가로 가운데에 놓인다.
// (2026-10-23 WB03-M1: 메인 메뉴 화면 복원, 2026-10-28 WB: Edit 칸을 웹 제외로 변경)
import { StoneButton, ftol } from './stone-button.js';

// 원본 메인 메뉴의 버튼 칸 9개(원본 004d0120 의 생성 순서)이다.
//  - label: 원본 라벨(번역 조회 전 원문, 영어 번역표는 원문 그대로)이다.
//  - command: 실행 시 메뉴가 알리는 명령이다. type 은 원본 동작 종류, target 은 원본 Tell 섹션/메뉴 이름이다.
//  - web: 웹 판본에서 버튼을 만드는지 여부다. false 인 칸은 원본의 빈 칸(null)처럼 배치에서 빠진다.
//  - note: web=false 인 이유다.
// 사용: createMainMenuButtons(MAIN_MENU_SLOTS) 로 칸 배열을 만든다. (2026-10-23 WB03-M1, 2026-10-28 WB: Edit web=false)
export const MAIN_MENU_SLOTS = Object.freeze([
    Object.freeze({ label: 'Campaign', command: Object.freeze({ type: 'tell', target: 'UCampaign' }), web: true }),
    Object.freeze({ label: 'Multiplayer', command: Object.freeze({ type: 'multiplayer' }), web: false, note: '웹 판본은 멀티플레이를 구현하지 않는다(AGENTS.md).' }),
    Object.freeze({ label: 'Demo', command: Object.freeze({ type: 'tell', target: 'Demoville' }), web: true }),
    Object.freeze({ label: 'Help', command: Object.freeze({ type: 'menu', target: 'help' }), web: true }),
    Object.freeze({ label: 'Edit', command: Object.freeze({ type: 'edit' }), web: false, note: '웹 판본은 커스텀 맵 Edit 을 구현하지 않는다(AGENTS.md webpj 목표, 2026-10-27 변경). 원본 8버튼 비교(includeAll)에서만 만든다.' }),
    Object.freeze({ label: 'Intro', command: Object.freeze({ type: 'intro' }), web: false, note: '원본도 게임 CD 가 있을 때만 만든다(DAT_0054db98).' }),
    Object.freeze({ label: 'Credits', command: Object.freeze({ type: 'tell', target: 'Credits' }), web: true }),
    Object.freeze({ label: 'Options', command: Object.freeze({ type: 'menu', target: 'options' }), web: true }),
    Object.freeze({ label: 'Quit', command: Object.freeze({ type: 'quit' }), web: false, note: '웹에서는 프로그램을 종료할 수 없다.' }),
]);

// 버튼 사이 간격(float 4.0, 원본 상수 00502314)이다. (2026-10-23 WB03-M1)
const BUTTON_GAP = 4;

// 줄 가운데 맞춤 계수(float 0.5, 원본 상수 00500ee4)이다. (2026-10-23 WB03-M1)
const CENTER_FACTOR = 0.5;

// 윗줄에 들어가는 칸 수(0~3번)이다. 나머지 칸은 모두 아랫줄이다. (2026-10-23 WB03-M1)
const FIRST_ROW_SLOTS = 4;

// 화면높이/3 에 더하는 윗줄·아랫줄 y 오프셋(0x37, 0x4e)이다. (2026-10-23 WB03-M1)
const FIRST_ROW_OFFSET = 0x37;
const SECOND_ROW_OFFSET = 0x4e;

// 칸 정의 배열로 버튼 칸 배열을 만든다. includeAll 이 true 면 원본처럼 Intro 를 뺀 8개를, false 면 웹 판본(web=true)만 만든다.
// 빈 칸은 null 로 남겨 배치 규칙이 원본 빈 칸 처리와 같게 동작하게 한다. (2026-10-23 WB03-M1)
export function createMainMenuButtons(slots = MAIN_MENU_SLOTS, includeAll = false) {
    return slots.map((slot) => {
        const enabled = includeAll ? slot.label !== 'Intro' : slot.web;
        return enabled ? new StoneButton({ label: slot.label, command: slot.command }) : null;
    });
}

// 한 줄의 전체 폭 누적값을 원본 순서(0.0 부터 각 버튼 폭+4 를 float32 로 더함)로 계산한다. (2026-10-23 WB03-M1)
function rowTotal(buttons) {
    let total = 0;
    // 줄에 속한 버튼마다 폭과 간격을 더한다. 빈 칸은 건너뛴다.
    for (const button of buttons) if (button) total = Math.fround(Math.fround(total + button.baseWidth) + BUTTON_GAP);
    return total;
}

// 한 줄의 버튼들을 가운데 맞춰 붙인다. 시작 x = (화면폭 - (누적값 - 4)) × 0.5 이고, 버튼마다 폭+4 만큼 오른쪽으로 옮긴다. (2026-10-23 WB03-M1)
function placeRow(buttons, screenWidth, y) {
    let x = Math.fround((screenWidth - Math.fround(rowTotal(buttons) - BUTTON_GAP)) * CENTER_FACTOR);
    // 빈 칸을 건너뛰며 버튼을 차례로 붙인다.
    for (const button of buttons) {
        if (!button) continue;
        button.attach(x, y);
        x = Math.fround(Math.fround(x + button.baseWidth) + BUTTON_GAP);
    }
}

// 원본 004ce940 의 메인 메뉴 버튼 배치를 실행한다. buttons 는 칸 배열(null 허용), font 는 측정용 원본 글꼴이다.
// 크기 맞춤 → 최대 폭 통일 → 윗줄(0~3번)·아랫줄(4번 이후) 가운데 배치 순서이며, 위치는 float32 로 저장된다.
// 사용: layoutMainMenuButtons(buttons, font, 1024, 768) 뒤 각 버튼의 bounds() 로 화면 위치를 읽는다. (2026-10-23 WB03-M1)
export function layoutMainMenuButtons(buttons, font, screenWidth, screenHeight) {
    let widest = 0;
    // 모든 버튼을 글자에 맞추고 가장 넓은 기본 폭을 찾는다.
    for (const button of buttons) {
        if (!button) continue;
        button.fitToText(font);
        if (widest < button.baseWidth) widest = button.baseWidth;
    }
    // 모든 버튼에 같은 폭을 쓴다(배율 적용 폭도 함께 갱신).
    for (const button of buttons) if (button) button.setBaseWidth(widest);
    const third = Math.trunc(screenHeight / 3);
    placeRow(buttons.slice(0, FIRST_ROW_SLOTS), screenWidth, third + FIRST_ROW_OFFSET);
    placeRow(buttons.slice(FIRST_ROW_SLOTS), screenWidth, third + SECOND_ROW_OFFSET);
}

// 타이틀 GIF 의 화면 위치를 구한다. 원본은 float(화면폭/2) - 그림폭×0.5, float(화면높이/2) - 그림높이×0.5 를 쓰고 정수로 절삭한다.
// 1024×768 에서 640×480 타이틀은 (192,144) 에 놓인다. (2026-10-23 WB03-M1)
export function titlePosition(screenWidth, screenHeight, image) {
    const x = Math.fround(Math.trunc(screenWidth / 2) - image.width * CENTER_FACTOR);
    const y = Math.fround(Math.trunc(screenHeight / 2) - image.height * CENTER_FACTOR);
    return { x: ftol(x), y: ftol(y) };
}

// 메인 메뉴 화면이다. 구름 배경·타이틀·버튼을 8비트 표면에 그리고, 왼쪽 버튼 입력을 원본 돌 버튼 규칙으로 처리한다.
// 사용: new MainMenuScreen({ resources, onCommand, onButtonSound }) → resize(w, h) → render(surface).
// 입력은 pointerDown/pointerMove/pointerUp/cancel 에 논리 좌표(정수)를 넘긴다. 키보드는 원본처럼 버튼에 영향을 주지 않는다.
// resources: { font, clouds, title, texture, textureMetrics, shadeMaps, buttonPalette, textContexts }. (2026-10-23 WB03-M1)
export class MainMenuScreen {
    // 화면 자원과 콜백으로 메뉴를 만든다. includeAllButtons 는 원본 비교용으로 8개 버튼을 모두 만들지 여부다.
    constructor({ resources, onCommand = null, onButtonSound = null, includeAllButtons = false }) {
        // 그리기에 쓰는 원본 자원 묶음이다.
        this.resources = resources;
        // 버튼을 실행했을 때 명령 객체와 라벨을 받는 콜백이다.
        this.onCommand = onCommand;
        // 버튼을 누르는 순간 button.wav 를 내기 위한 콜백이다.
        this.onButtonSound = onButtonSound;
        // 버튼 칸 배열(빈 칸은 null)이다.
        this.slots = createMainMenuButtons(MAIN_MENU_SLOTS, includeAllButtons);
        // 화면 가로 크기(논리 픽셀)다.
        this.width = 0;
        // 화면 세로 크기(논리 픽셀)다.
        this.height = 0;
        // 화면을 다시 합성해야 하는지 여부다. 그림이 바뀔 때 켜고 render 가 끈다.
        this.dirty = true;
        // 열려 있는 Tell 대화상자(TellWindow)다. 없으면 null. 대화상자가 열려 있는 동안에는 메인 메뉴 돌 버튼을 숨기고 입력을 대화상자가 독점한다.
        // (원본 스크린샷: Credits·Campaign·Demo 창이 열리면 메인 메뉴 버튼 두 줄이 보이지 않는다.) (2026-10-28 WB03-D)
        this.dialog = null;
    }

    // 대화상자를 열거나(TellWindow) 닫는다(null). 바뀌는 순간 진행 중인 돌 버튼 누름은 실행 없이 취소하고 현재 화면 크기에 맞춰 대화상자를 다시 맞춘다. (2026-10-28 WB03-D)
    setDialog(dialog) {
        this.cancel();
        this.dialog = dialog;
        if (dialog) this._fitDialog();
        this.dirty = true;
    }

    // 대화상자를 현재 논리 화면 크기에 맞춰 다시 배치한다(화면 크기가 바뀌었거나 방금 열린 경우). (2026-10-28 WB03-D)
    _fitDialog() {
        if (!this.dialog || this.width === 0) return;
        this.dialog.screen = { width: this.width, height: this.height };
        this.dialog.fit();
        this.dialog.place();
    }

    // 시간 경과를 알린다(초). 대화상자의 Timeout 기한을 확인한다. 명령이 실행되어 그림이 바뀔 수 있다. (2026-10-28 WB03-D)
    update(nowSeconds) {
        if (this.dialog?.update(nowSeconds)) this.dirty = true;
    }

    // 실제로 만들어진 버튼 목록(빈 칸 제외)이다.
    get buttons() {
        return this.slots.filter((button) => button !== null);
    }

    // 화면 크기에 맞춰 버튼을 다시 배치한다. 진행 중인 누름은 실행 없이 취소한다.
    resize(width, height) {
        this.cancel();
        this.width = width;
        this.height = height;
        layoutMainMenuButtons(this.slots, this.resources.font, width, height);
        this._fitDialog();
        this.dirty = true;
    }

    // 화면을 합성한다: 구름 타일(화면 (0,0) 기준) → 타이틀 → 버튼 순서다. 합성 뒤 dirty 를 끈다.
    // 구름 그림이 없으면(원본 크기 검사 실패) 색 번호 0 으로 채운다.
    render(surface) {
        const { clouds, title } = this.resources;
        if (clouds) surface.tile(clouds);
        else surface.fill(0);
        const position = titlePosition(surface.width, surface.height, title);
        surface.drawImage(title, position.x, position.y);
        // 대화상자가 없을 때만 모든 버튼을 칸 순서대로 그린다(대화상자가 열리면 버튼이 사라진다).
        if (!this.dialog) for (const button of this.buttons) button.draw(surface, this.resources);
        else this.dialog.draw(surface);
        this.dirty = false;
    }

    // 왼쪽 버튼 누름을 처리한다. 버튼이 받으면 눌린 모양으로 바꾸고 button.wav 를 낸다. 처리했으면 true.
    pointerDown(x, y) {
        // 대화상자가 열려 있으면 입력을 모두 대화상자가 받는다(모달).
        if (this.dialog) {
            const handled = this.dialog.pointerDown(x, y);
            this.dirty = true;
            return handled;
        }
        // 정수 화면 사각형이 누름 위치를 포함하는 첫 버튼에 누름을 전달한다(버튼끼리는 겹치지 않는다).
        for (const button of this.buttons) {
            if (button.pointerDown(x, y)) {
                this.dirty = true;
                this.onButtonSound?.();
                return true;
            }
        }
        return false;
    }

    // 포인터 이동을 처리한다. 붙잡힌 버튼은 포인터가 안에 있을 때만 눌린 모양을 보인다.
    pointerMove(x, y) {
        if (this.dialog) {
            if (this.dialog.pointerMove(x, y)) this.dirty = true;
            return;
        }
        // 붙잡힌 버튼의 눌린 모양을 현재 위치로 갱신한다.
        for (const button of this.buttons) if (button.broadcast(x, y)) this.dirty = true;
    }

    // 왼쪽 버튼 뗌을 처리한다. 붙잡힌 버튼 안에서 뗐으면 그 버튼의 명령을 실행(onCommand)하고 true 를 돌려준다.
    pointerUp(x, y) {
        if (this.dialog) {
            // 명령이 대화상자를 닫거나 바꿀 수 있으므로 뗌 처리 뒤에는 항상 다시 그린다.
            this.dialog.pointerUp(x, y);
            this.dirty = true;
            return false;
        }
        let activated = null;
        // 붙잡힌 버튼을 찾아 붙잡기를 풀고 실행 여부를 받는다.
        for (const button of this.buttons) {
            const wasCaptured = button.captured;
            if (button.pointerUp(x, y)) activated = button;
            if (wasCaptured) this.dirty = true;
        }
        if (activated) this.onCommand?.(activated.command, activated.label);
        return activated !== null;
    }

    // 진행 중인 누름을 실행 없이 취소한다(포커스 상실·탭 숨김·화면 크기 변경).
    cancel() {
        // 모든 버튼의 붙잡기와 눌린 모양을 푼다.
        for (const button of this.buttons) if (button.cancel()) this.dirty = true;
        if (this.dialog?.cancel()) this.dirty = true;
    }
}
