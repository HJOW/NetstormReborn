// 원본 채팅 서식 입력 창(`Edit Back Format`·`Edit Front Format`)을 이식한다(원문 Netstorm.exe 0x4d4580/0x4d48f0 래퍼, 생성자 0x4d42b0(Back, vtable 0x517dd0)·0x4d4620(Front, vtable 0x517e38), 확정 0x4c9600/0x4c9680).
// 사용: const dialog = new ChatFormatDialog(gumpCreateState, hooks); const token = dialog.show(front, 초기 문자열); 이후 Gump 입력이 dialog.handle(token, 사건, 첫 값, 둘째 값)을 부른다(OK/Enter 확정·Cancel 취소는 송금 숫자 창과 같은 공통 기반 처리).
//   gumpCreateState·hooks 는 `SaveFortNameDialog` 와 같고, 추가 장치로 horizontal(토큰, 값) → 입력창 가로 표시 필드(원문 00451c60 `setHorizontal`)를 요구한다. 숫자 제한 장치(excluded)는 쓰지 않는다.
// 이 창은 송금 숫자 창과 같은 모달 한 장 틀(공통 장식 200×100·글자 입력창·테두리·제목·OK/Cancel)이며 차이는 다음과 같다: ① 입력창 높이 24·글자 제한(Back 500·Front 300)과 가로 표시 1, ② 제목 `Please type your back/front format.`,
// ③ 명령 번호는 원문 데이터 바이트 0x540456 값(59), ④ 래퍼가 화면 가운데에 붙이고 깊이를 바꾸거나 모달로 등록하지 않는다(원문 0x4d4580 은 attach 뒤 바로 끝난다).
// 확정(0x4c9600/0x4c9680)은 입력 문자열을 300바이트 한도로 복사(용량 거부는 -1)해 설정 키 `<구독자>chatFormat`(Back)·`<구독자>chatFormat2`(Front)에 쓰고 chat manager 를 다시 초기화(0x42e050)한 뒤 1 을 돌려주는 일이며 앱 장치가 맡는다.
// (2026-11-22 WB04-E1179: Edit Back/Front Format 대화창 이식으로 추가)
import { SaveFortNameDialog } from './save-fort-name-dialog.js';

// 두 서식 창의 원문 가상 표 주소(Back·Front)와 명령 번호(원문 데이터 바이트 0x540456), 입력 글자 제한(0x1f4=500·0x12c=300), 확정 복사 한도(0x12c=300 바이트, NUL 포함 용량)이다. (2026-11-22 WB04-E1179)
export const CHAT_FORMAT_BACK_VTABLE = 0x00517dd0;
export const CHAT_FORMAT_FRONT_VTABLE = 0x00517e38;
export const CHAT_FORMAT_COMMAND = 59;
export const CHAT_FORMAT_BACK_MAX_LENGTH = 500;
export const CHAT_FORMAT_FRONT_MAX_LENGTH = 300;
export const CHAT_FORMAT_COPY_LIMIT = 300;
// 글자 입력창 높이(원문 0x18)와 y 위치(원문 45.0f)이다. (2026-11-22 WB04-E1179)
const ENTRY_HEIGHT = 24;
const ENTRY_Y = 45;

// 설정 키는 `현재 구독자(부호 있는 %d)` 뒤에 Back 은 `chatFormat`, Front 는 `chatFormat2` 를 붙인다(원문 sprintf("%d%s")). 사용: chatFormatKey(subscriber, front). (2026-11-22 WB04-E1179)
export function chatFormatKey(subscriber, front) { return `${subscriber | 0}${front ? 'chatFormat2' : 'chatFormat'}`; }

// 선택된 장치가 없으면 그 자리에서 실패한다. (2026-11-22 WB04-E1179)
function need(hook) { if (typeof hook !== 'function') throw new Error('채팅 서식 창의 실제 장치 미연결'); return hook; }
// 원문 _ftol 의 유한 GUI 크기를 0 방향으로 절삭한 signed 32비트 정수이다. (2026-11-22 WB04-E1179)
function size(value) { if (!Number.isFinite(value) || value <= -2147483649 || value >= 2147483648) return -2147483648; return Math.trunc(value) | 0; }
// float32 곱·합·차이다. (2026-11-22 WB04-E1179)
const mul = (a, b) => Math.fround(Math.fround(a) * Math.fround(b));
const add = (a, b) => Math.fround(Math.fround(a) + Math.fround(b));
const sub = (a, b) => Math.fround(Math.fround(a) - Math.fround(b));

// 송금 숫자 창과 같은 부모 노드·OK/Cancel 처리를 쓰는 채팅 서식 입력 창이다. (2026-11-22 WB04-E1179)
export class ChatFormatDialog extends SaveFortNameDialog {
    // 화면 크기·루트 등록표와 선택된 실제 장치(부모 클래스의 비공개 사본과 별개로 이 창이 직접 부르는 장치)이다.
    #state; #hooks;
    // 공통 등록표와 실제 장치를 빌린다.
    constructor(state, hooks) { super(state, hooks); this.#state = state; this.#hooks = { ...hooks }; }
    // 서식 창을 만든다: 공통 장식 틀(200×100)·글자 입력창(너비 틀의 0.8, 높이 24, 초기 문자열)·테두리·가운데 제목·두 버튼을 만들어 화면 가운데에 붙인다. 모달 등록·깊이 변경은 하지 않는다(원문 0x4d4580).
    // front 가 참이면 Front(vtable 0x517e38, 글자 제한 300, 제목 front), 거짓이면 Back(0x517dd0, 제한 500, 제목 back)이다. Back 은 입력창을 붙인 뒤 가로 표시를 켜고 Front 는 붙이기 전에 켠다(원문 두 생성자의 순서). (2026-11-22 WB04-E1179)
    show(front, initialText) {
        const h = this.#hooks, state = this.#state, isFront = Boolean(front);
        const token = need(h.allocate)(0xd8); need(h.decoration)(token, 200, 100);
        const root = this.node(token); root.vtable = isFront ? CHAT_FORMAT_FRONT_VTABLE : CHAT_FORMAT_BACK_VTABLE; root.command = CHAT_FORMAT_COMMAND; root.fortMode = 1;
        // 원문 취소 판정(가상 +0x40, 0x435060)은 항상 1 이므로 웹의 취소 가능 표식도 켠다.
        root.canCancel = 1;
        root.entry = h.allocate(0x110); need(h.textEntry)(root.entry, size(mul(root.baseWidth, 0.8)), ENTRY_HEIGHT, initialText ?? '');
        const entry = need(h.entry)(root.entry); if (!entry) throw new Error('채팅 서식 창의 편집 자식 없음');
        entry.maxLength = isFront ? CHAT_FORMAT_FRONT_MAX_LENGTH : CHAT_FORMAT_BACK_MAX_LENGTH;
        if (isFront) need(h.horizontal)(root.entry, 1);
        entry.multiline = 0;
        need(h.attach)(root.entry, mul(sub(root.baseWidth, this.base(root.entry).baseWidth), 0.5), ENTRY_Y, token, 0);
        if (!isFront) need(h.horizontal)(root.entry, 1);
        const border = h.allocate(0xb8); need(h.background)(border, add(this.base(root.entry).baseWidth, 8), add(this.base(root.entry).baseHeight, 4));
        h.attach(border, sub(this.base(root.entry).x, 4), sub(this.base(root.entry).y, 2), token, 0);
        const title = need(h.translate)(isFront ? 'Please type your front format.' : 'Please type your back format.'); root.title = h.allocate(0xa4); need(h.title)(root.title, title, size(mul(root.baseWidth, 0.8)));
        h.attach(root.title, this.base(root.entry).x, 10, token, 0); need(h.fitTitle)(root.title); need(h.push)(root.title);
        // 원문 +8 은 정렬 필드이다: 가상 +18 로 먼저 분리한 뒤 가운데 정렬(4)로 재부착한다.
        this.base(root.title).alignFlags = 4; h.attach(root.title, this.base(root.title).x, this.base(root.title).y, token, 2);
        this.buttons(token, add(add(this.base(root.entry).y, this.base(root.entry).baseHeight), 15));
        root.baseHeight = add(root.baseHeight, 4); root.height = mul(root.baseHeight, root.scale);
        root.baseWidth = Math.max(root.baseWidth, add(this.base(root.title).baseWidth, 32)); root.width = mul(root.baseWidth, root.scale);
        h.attach(token, mul(sub(state.screenWidth, root.baseWidth), 0.5), mul(sub(state.screenHeight, root.baseHeight), 0.5), state.entries[0], 0);
        return token;
    }
}
