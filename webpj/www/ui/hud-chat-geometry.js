// 원본 ChatWindow(0x13c 바이트 파생)의 실제 자식 기하(배경·본문 글·도우미·입력줄)와 입력줄 표시 전환, 문자 Enter 분배, 프레임말 전환 요청 소비를 옮긴다(원문 0042e6e0/0042f250/0042f270/0042f210/0042f310/00439b96, cpppj client/HudChatGeometry.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// Console/TextEntry 생성·입력과 네트워크 Chat 은 별도 단위다. 이 모듈은 root 자체를 그리지 않고 현재 네 자식의 크기와 가상 호출 순서를 갱신한다. 자식 포인터는 각 가상 호출 뒤에도 root 에서 다시 읽는다.
// 크기 곱셈은 원문 x87 처럼 float 저장 전 중간값(배정도)으로 배율을 곱한 뒤 float32 로 저장한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const geometry = new HudChatGeometry({ chat, node, baseAttach, push, attach, bottomIndex, requestBottom, challenge, current }); geometry.attach(token, x, y, parent, mode).
// (2026-11-14 WB04-E445: RawMenuControls 선행 Chat 기하 이식)
import { GumpCreateNode } from './gump-create.js';
import { InputCode } from '../core/input-queue.js';

// 현재 입력줄(Console)의 표시 비트(root 의 표시와 구별한다)와 보호 PE 의 실제 상수(도우미 높이 15.0 / 입력줄 왼쪽 여백 17.0)다.
const VISIBLE = 1; const HELPER_HEIGHT = 15.0; const CONSOLE_LEFT = 17.0;
// 문자 Enter 의 코드와 코드 마스크(상위 수정자 비트를 제외한 글자 부분)다.
const ENTER_CODE = 13; const CODE_MASK = 0x03ffffff;

// 원본 0x13c 바이트 ChatWindow 의 기본 Gump 와 +a0..138 상태다.
export class HudChatNode extends GumpCreateNode {
    // +a0/+a4/+a8/+ac: 실제 네 자식(입력줄·본문 글·도우미·배경) 토큰이다.
    console = 0; text = 0; helper = 0; background = 0;
    // +b0: 생성자가 쓰는 입력 버퍼 크기(128)다. +b4: 모든 재부착에서 다시 읽는 signed 정수 왼쪽 여백이다.
    capacity = 0; leftPadding = 0;
    // +b8..137: 실제 Console 입력과 공유하는 접두 문자열 128바이트다. +138: 도우미 높이 15 를 허용하는 원시 BYTE 다.
    inputPrefix = new Uint8Array(128); helperVisible = 0;
}

// 도달하지 않은 장치는 요구하지 않는다(C++ invalid_argument → TypeError, logic_error/runtime_error → Error).
function need(hook, message) { if (typeof hook !== 'function') throw new TypeError(message); return hook; }

// 같은 root 파생 노드와 공통 기본 노드를 빌려 기하를 계산한다. hooks = { chat(token), node(token), baseAttach(token,x,y,parent,mode), push(token), attach(token,x,y,parent,mode),
//   bottomIndex(token) → 현재 표시 줄(004bc2c0), requestBottom(token, line)(004ba3d0), challenge() → 현재 00594fc8, current() → 현재 Chat 토큰(00548740) } 이다.
export class HudChatGeometry {
    // 노드/장치 묶음이다(생성자는 두 노드 조회만 요구한다).
    #hooks;
    constructor(hooks) {
        if (typeof hooks?.chat !== 'function' || typeof hooks.node !== 'function') throw new TypeError('ChatWindow 같은 노드 저장소 필요');
        this.#hooks = hooks;
    }

    // root 의 파생/기본 주소가 같은 객체인지 현재 공통 트리와 대조한다(없거나 다르면 Error).
    #chat(token) {
        const node = token ? this.#hooks.chat(token) : null;
        if (!node || node !== this.#hooks.node(token)) throw new Error('ChatWindow 공유 root 없음');
        return node;
    }

    // 자식은 생성자가 연결한 실제 공통 Gump 주소만 허용한다(없으면 Error).
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node) throw new Error('ChatWindow 실제 자식 없음');
        return node;
    }

    // 원문은 기본 크기를 float32 에 저장한 뒤 저장 전 중간값(배정도)으로 배율을 곱해 표시 크기를 float32 로 저장한다.
    #size(token, width, height) {
        const node = this.#node(token); node.baseWidth = Math.fround(width); node.width = Math.fround(width * node.scale);
        node.baseHeight = Math.fround(height); node.height = Math.fround(height * node.scale);
    }

    // 현재 자식의 가상 Push 경계만 실행한다(호출 뒤 바뀐 root 자식 포인터는 호출자가 다시 조회한다).
    #push(token) { this.#node(token); need(this.#hooks.push, 'ChatWindow 가상 Push 필요')(token); }

    // 현재 자식의 가상 Attach(mode 0) 호출이다. 좌표는 float32 로 전달한다.
    #attachVirtual(token, x, y, parent) { this.#node(token); need(this.#hooks.attach, 'ChatWindow 가상 Attach 필요')(token, Math.fround(x), Math.fround(y), parent, 0); }

    // 0042e6e0: 기존 부모 부착 뒤 배경→본문 글→도우미→입력줄 순서로 현재 필드를 다시 읽으며 재부착한다. 입력줄이 보일 때만 본문 높이에서 입력줄 높이를 빼고, 도우미는 입력줄 표시와 원시 BYTE 두 관문을 모두 통과해야 높이 15 를 갖는다.
    attach(token, x, y, parent, mode) {
        const root = this.#chat(token); need(this.#hooks.baseAttach, 'ChatWindow 기본 Attach 필요')(token, x, y, parent, mode);
        this.#push(root.background);
        const backgroundHeight = root.baseHeight - ((this.#node(root.console).flags & VISIBLE) ? this.#node(root.console).baseHeight : 0.0);
        this.#size(root.background, root.baseWidth - (root.leftPadding | 0), backgroundHeight);
        const helper = this.#node(root.helper); const showHelper = root.helperVisible && (this.#node(root.console).flags & VISIBLE);
        helper.baseHeight = showHelper ? Math.fround(HELPER_HEIGHT) : 0; helper.height = Math.fround((showHelper ? HELPER_HEIGHT : 0.0) * helper.scale);
        this.#attachVirtual(root.background, root.leftPadding | 0, 0, token);
        const bottom = need(this.#hooks.bottomIndex, 'ChatWindow 실제 표시 줄 조회004bc2c0 필요')(root.text); // 변경 전 본문 높이에서 현재 줄 위치를 읽는다.
        this.#push(root.text); this.#size(root.text, this.#node(root.background).baseWidth, this.#node(root.background).baseHeight);
        need(this.#hooks.requestBottom, 'ChatWindow 실제 맨 아래 요청 필요')(root.text, bottom);
        this.#attachVirtual(root.text, 0, 0, root.background);
        this.#push(root.helper); this.#attachVirtual(root.helper, root.leftPadding | 0, root.baseHeight - this.#node(root.helper).baseHeight, token);
        this.#push(root.console); const input = this.#node(root.console); const consoleWidth = this.#node(root.text).baseWidth; // 원문은 입력줄 높이를 그대로 두고 본문 폭만 복사한다.
        input.baseWidth = Math.fround(consoleWidth); input.width = Math.fround(consoleWidth * input.scale);
        this.#attachVirtual(root.console, (root.leftPadding | 0) + CONSOLE_LEFT, root.baseHeight - this.#node(root.console).baseHeight, token);
    }

    // 0042f250: 값이 0 이 아니면 BYTE 1, 아니면 0 으로 쓴다. 재부착은 하지 않고 root 크기/기본 플래그도 바꾸지 않는다(다음 attach 가 소비한다).
    setHelperVisible(token, value) { this.#chat(token).helperVisible = (value >>> 0) !== 0 ? 1 : 0; }

    // 0042f270: 토큰이 0 이면 아무것도 하지 않는다. root 와 입력줄을 Push 한 뒤 값이 0 이 아니거나 현재 challenge 가 비영이면 입력줄을 보이게(아니면 숨기게) 하고, 입력줄과 root 를 각자 현재 좌표로 재부착한다(parent 0).
    // value 가 0 이면 challenge 조회가 필요하다(없으면 TypeError). 좌표는 각 콜백 뒤에 다시 읽는다.
    setInputVisible(token, value) {
        if (!token) return;
        const root = this.#chat(token); this.#push(token); this.#push(root.console);
        const zero = (value >>> 0) === 0; if (zero && typeof this.#hooks.challenge !== 'function') throw new TypeError('ChatWindow 현재 challenge 필요');
        const show = !zero || this.#hooks.challenge(); const input = this.#node(root.console); // 표시 판정(challenge 포함)이 끝난 뒤에 현재 입력줄을 다시 조회한다.
        if (show) input.flags = (input.flags | VISIBLE) >>> 0; else input.flags = (input.flags & ~VISIBLE) >>> 0;
        const current = root.console; this.#attachVirtual(current, this.#node(current).x, this.#node(current).y, 0);
        this.#attachVirtual(token, root.x, root.y, 0);
    }

    // 0042f210: 현재 Chat 의 입력줄만 Push 하고(root Push/전체 기하 갱신 없음) Push 이후의 현재 root 와 입력줄을 다시 읽어 표시 비트를 뒤집은 뒤 같은 입력줄만 재부착한다. 현재 Chat 이 없으면 아무것도 하지 않는다.
    toggleInputVisible() {
        if (typeof this.#hooks.current !== 'function') throw new Error('ChatWindow 현재 포인터 조회 필요');
        const before = this.#hooks.current(); if (!before) return;
        this.#push(this.#chat(before).console);
        const root = this.#chat(this.#hooks.current()); const flagged = this.#node(root.console); flagged.flags = (flagged.flags ^ VISIBLE) >>> 0;
        const input = root.console; this.#attachVirtual(input, this.#node(input).x, this.#node(input).y, 0);
    }

    // 0042f310: 현재 Chat 이 없으면 자식/노드 장치를 조회하지 않고 0, 있으면 입력줄의 표시 비트만 돌려준다.
    inputVisible() {
        if (typeof this.#hooks.current !== 'function') throw new Error('ChatWindow 현재 포인터 조회 필요');
        const current = this.#hooks.current(); return current ? (this.#node(this.#chat(current).console).flags & VISIBLE) : 0;
    }
}

// 일반 입력의 Guide/Gump/모달 뒤에서 문자 Enter 의 표시/부모 열기 순서만 실행한다. 뗌은 먼저 거부되고 Ctrl/Shift/Alt 수정자 비트는 문자 Enter 를 막지 않는다(코드 마스크 뒤가 13 이어야 한다).
// 입력줄이 이미 보이면 처리하지 않는다(false). 보이지 않으면 표시 → 열기를 호출하고 true 를 돌려준다. 입력줄 조회가 없으면 Error 이고, 표시/열기 중 하나라도 없으면 Error 다.
export function dispatchChatInputKey(event, visible, show, open) {
    if ((event.code & InputCode.release) || ((event.code & CODE_MASK) !== ENTER_CODE)) return false;
    if (typeof visible !== 'function') throw new Error('Enter의 현재 입력줄 조회 필요');
    if (visible()) return false;
    if (typeof show !== 'function' || typeof open !== 'function') throw new Error('Enter의 입력줄 표시/Chat 열기 필요');
    show(); open(); return true;
}

// 원본 439b96 의 프레임말 요청을 한 번 소비한다. request 는 `{ value }` 상자이며 0 이면 아무것도 하지 않고, 아니면 전환 장치(없으면 Error)를 부른 뒤 0 으로 쓴다(전환이 던지면 요청을 지우지 않는다).
export function consumeChatInputToggle(request, toggle) {
    if (!request.value) return;
    if (typeof toggle !== 'function') throw new Error('프레임말 Chat 전환 장치 필요');
    toggle(); request.value = 0;
}
