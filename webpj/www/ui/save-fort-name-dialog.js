// 원본 입력 대화창(`SaveFortNameDialog`)의 송금 숫자 창(Send X Storm Power)과 공통 두 버튼·입력 판정을 이식한다(cpppj `client/SaveFortNameDialog.cpp/.h`, 10.78 전용, 원문 004d3860/004d3b60/00487530/00487360).
// 사용: const dialog = new SaveFortNameDialog(gumpCreateState, hooks); const token = dialog.showMoney(대상 owner); 이후 Gump 입력이 dialog.handle(token, 사건, 첫 값, 둘째 값)을 부른다.
//   gumpCreateState = { screenWidth, screenHeight, entries[0] = 루트 토큰 } (`GumpCreateState`).
//   hooks = { node(토큰) → SaveFortNameDialogNode, base(토큰) → 공통 Gump 노드, entry(토큰) → TextEntry 노드, allocate(바이트 수) → 토큰, decoration(토큰, 폭, 높이), textEntry(토큰, 폭, 높이, 문자열),
//             excluded(토큰, 입력 금지 문자열), background(토큰, 폭, 높이), title(토큰, 문자열, 제목 폭 = 창 기본 폭×0.8 의 절삭 정수), stone(토큰, 문자열), fitTitle(토큰), fitStone(토큰), push(토큰), attach(토큰, x, y, 부모, 방식),
//             depth(토큰, 깊이 변화량), modal(토큰), kill(토큰), accept(토큰) → 0/1, cancel(토큰) → 0/1, baseHandle(토큰, 사건, 첫 값, 둘째 값), translate(문자열) → 문자열 }.
// 도달한 장치만 요구하고 미연결을 정상 반환으로 바꾸지 않는다. float 산술은 원본 float32 이므로 단계마다 `Math.fround` 로 맞춘다.
// 편집용 이름 입력(Save as·설명·HTML Input·Scenario)은 웹 범위 밖(대응표 `exclude-edit`)이라 옮기지 않았다. 송금 숫자 창만 단독 미션(Players ▸ Send Money ▸ Send X)에서 도달한다.
// (2026-11-22 WB04-E1168: Send X 숫자 입력 창 이식으로 추가)
import { GumpDecorationNode } from './gump-decoration.js';

// 송금 숫자 창 부모의 원문 가상 표 주소와 명령 번호, 두 버튼 부모 사건이다. (2026-11-22 WB04-E1168)
export const SEND_MONEY_DIALOG_VTABLE = 0x00517ca0;
export const SEND_MONEY_DIALOG_COMMAND = 68;
const BUTTON_CLICK = 0x0200002c;
const ENTER = 0x02000038;
const EVENT_MASK = 0x03ffffff;
// 숫자 창 입력 상한(편집 노드 maxLength 10, 현재 길이+1 < 10 비교라 실제 입력은 아홉 문자까지)과 초기 문자열이다. (2026-11-22 WB04-E1168)
export const SEND_MONEY_MAX_LENGTH = 10;
export const SEND_MONEY_INITIAL_TEXT = '0';

// 입력 창 부모 노드다: 공통 장식 배경 뒤의 제목/OK/Cancel/편집 자식 토큰과 두 모드 DWORD(송금에서는 대상 owner·취소 가능 1)다. (2026-11-22 WB04-E1168)
export class SaveFortNameDialogNode extends GumpDecorationNode {
    // +c4/+c8/+cc/+d0 의 실제 자식 토큰이다.
    title = 0; ok = 0; cancel = 0; entry = 0;
    // +d4/+d8 의 전체 원문 DWORD 이다(송금에서는 현재 대상 owner 와 취소 가능 여부).
    fortMode = 0; canCancel = 0;
}

// 선택된 장치가 없으면 그 자리에서 실패한다. (2026-11-22 WB04-E1168)
function need(hook) { if (typeof hook !== 'function') throw new Error('입력 창의 실제 장치 미연결'); return hook; }
// 원문 _ftol 의 유한 GUI 크기를 0 방향으로 절삭한 signed 32비트 정수이다. (2026-11-22 WB04-E1168)
function size(value) { if (!Number.isFinite(value) || value <= -2147483649 || value >= 2147483648) return -2147483648; return Math.trunc(value) | 0; }
// float32 곱·합·차이다. (2026-11-22 WB04-E1168)
const mul = (a, b) => Math.fround(Math.fround(a) * Math.fround(b));
const add = (a, b) => Math.fround(Math.fround(a) + Math.fround(b));
const sub = (a, b) => Math.fround(Math.fround(a) - Math.fround(b));

// 원문 1..255 중 ASCII 숫자가 아닌 모든 바이트의 입력 금지 문자열이다(숫자만 입력 가능). (2026-11-22 WB04-E1168)
export function sendMoneyExcludedCharacters() {
    let excluded = '';
    // 원본 순서(1부터 255)로 숫자 범위 밖의 바이트를 모은다.
    for (let value = 1; value < 256; value += 1) if (value < 0x30 || value > 0x39) excluded += String.fromCharCode(value);
    return excluded;
}

// 같은 공통 등록표와 실제 장치로 송금 숫자 창을 만든다. (2026-11-22 WB04-E1168)
export class SaveFortNameDialog {
    // 화면 크기·루트 등록표와 선택된 실제 장치이다.
    #state; #hooks;
    // 공통 등록표와 실제 장치를 빌린다.
    constructor(state, hooks) {
        if (!state || !Number.isInteger(state.screenWidth) || !Number.isInteger(state.entries?.length)) throw new TypeError('입력 창의 공통 등록표 필요');
        this.#state = state; this.#hooks = { ...hooks };
    }
    // 같은 파생 부모 저장소를 요구한다.
    node(token) { const node = need(this.#hooks.node)(token); if (!(node instanceof SaveFortNameDialogNode)) throw new Error('입력 창 실제 부모 없음'); return node; }
    // 선택된 자식의 공통 필드를 요구한다.
    base(token) { const node = need(this.#hooks.base)(token); if (!node) throw new Error('입력 창 실제 자식 없음'); return node; }
    // 제목과 편집 다음의 현재 크기를 읽어 두 버튼을 가운데 간격 10 으로 배치한다(원문 00487530/00487360). (2026-11-22 WB04-E1168)
    buttons(token, y) {
        const h = this.#hooks, root = this.node(token);
        root.ok = need(h.allocate)(0x170); need(h.stone)(root.ok, need(h.translate)('OK'));
        root.cancel = h.allocate(0x170); h.stone(root.cancel, h.translate('Cancel'));
        if (!(this.base(root.ok).flags & 8)) need(h.push)(root.ok);
        if (!(this.base(root.cancel).flags & 8)) need(h.push)(root.cancel);
        need(h.fitStone)(root.ok); h.fitStone(root.cancel);
        const total = add(add(this.base(root.cancel).baseWidth, this.base(root.ok).baseWidth), 10);
        const left = mul(sub(root.baseWidth, total), 0.5);
        need(h.attach)(root.ok, left, y, token, 0);
        h.attach(root.cancel, add(add(left, this.base(root.ok).baseWidth), 10), y, token, 0);
    }
    // 숫자 창 생성이다: 공통 장식 틀(200×100)·숫자만 입력되는 한 줄·테두리·가운데 제목·두 버튼을 만들고 화면 가운데에 놓아 모달로 등록한다. 대상 owner 는 +d4 에 둔다. (2026-11-22 WB04-E1168)
    showMoney(target) {
        const h = this.#hooks, state = this.#state;
        const token = need(h.allocate)(0xd8); need(h.decoration)(token, 200, 100);
        const root = this.node(token); root.vtable = SEND_MONEY_DIALOG_VTABLE; root.command = SEND_MONEY_DIALOG_COMMAND; root.fortMode = target >>> 0; root.canCancel = 1;
        root.entry = h.allocate(0x110); need(h.textEntry)(root.entry, size(mul(root.baseWidth, 0.8)), 18, SEND_MONEY_INITIAL_TEXT);
        const entry = need(h.entry)(root.entry); if (!entry) throw new Error('송금 숫자 창의 편집 자식 없음');
        entry.maxLength = SEND_MONEY_MAX_LENGTH; entry.multiline = 0;
        need(h.excluded)(root.entry, sendMoneyExcludedCharacters());
        need(h.attach)(root.entry, mul(sub(root.baseWidth, this.base(root.entry).baseWidth), 0.5), 45, token, 0);
        const border = h.allocate(0xb8); need(h.background)(border, add(this.base(root.entry).baseWidth, 8), add(this.base(root.entry).baseHeight, 4));
        h.attach(border, sub(this.base(root.entry).x, 4), sub(this.base(root.entry).y, 2), token, 0);
        const title = need(h.translate)('Send X Storm Power'); root.title = h.allocate(0xa4); need(h.title)(root.title, title, size(mul(root.baseWidth, 0.8)));
        h.attach(root.title, this.base(root.entry).x, 10, token, 0); need(h.fitTitle)(root.title); need(h.push)(root.title);
        // 원문 +8 은 정렬 필드이다: 가상 +18 로 먼저 분리한 뒤 가운데 정렬(4)로 재부착한다.
        this.base(root.title).alignFlags = 4; h.attach(root.title, this.base(root.title).x, this.base(root.title).y, token, 2);
        this.buttons(token, add(add(this.base(root.entry).y, this.base(root.entry).baseHeight), 15));
        root.baseHeight = add(root.baseHeight, 4); root.height = mul(root.baseHeight, root.scale);
        root.baseWidth = Math.max(root.baseWidth, add(this.base(root.title).baseWidth, 32)); root.width = mul(root.baseWidth, root.scale);
        h.attach(token, mul(sub(state.screenWidth, root.baseWidth), 0.5), mul(sub(state.screenHeight, root.baseHeight), 0.5), state.entries[0], 0);
        need(h.depth)(token, ((root.depth | 0) - 76) | 0); need(h.modal)(token); return token;
    }
    // OK 단추 또는 Enter 는 확정(callback 비영이면 지연 Kill 뒤 1), Cancel 단추는 취소(callback 비영이면 지연 Kill)이며 그 밖의 사건은 기본 Help 부모에 맡긴다. 항상 처리했다는 1 을 돌려준다(위임은 그 반환값). (2026-11-22 WB04-E1168)
    handle(token, event, first, second) {
        const h = this.#hooks, code = (event & EVENT_MASK) >>> 0, root = this.node(token);
        if ((code === BUTTON_CLICK && first === this.base(root.ok).serial) || code === ENTER) {
            if (need(h.accept)(token)) { need(h.kill)(token); return 1; }
        } else {
            if (code !== BUTTON_CLICK || first !== this.base(root.cancel).serial) return need(h.baseHandle)(token, event, first, second);
            if (need(h.cancel)(token)) need(h.kill)(token);
        }
        return 1;
    }
}
