// 원본 메시지 표(004d3090)의 20칸 serial 표와 경고 시간 판정을 옮긴다(cpppj client/MessageBoard.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 화면 오른쪽 위에 쌓이는 메시지 Gump 를 종류/플레이어/인자로 검색해 중복을 만들지 않고, enabled=false 면 일치 메시지를 지연 삭제하며 true 면 최초 빈 칸에 원본 순서대로 생성/부착한다.
// 실제 Gump 조회/생성/삭제/부착은 호스트 훅이며 전역 20 DWORD 표와 억제/화면 폭은 `MessageBoardState` 가 보관한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const board = new MessageBoard(new MessageBoardState(), { lookup, kill, create, createText, width, attach }); board.show(kind, player, true, parameter, '글'); warningExpired(warningDeadline(now), later).
// (2026-11-14 WB04-E445: RawMenuControls 선행 메시지 표 이식)

// 표 칸 수와 첫 호출 초기화 표식, 메시지 한 줄의 세로 간격, 억제 중에도 통과하는 두 종류, 중복 검색을 멈추지 않는 Warning 종류다.
const BOARD_SLOTS = 20; const FIRST_CALL_MARK = 0xffffffff; const ROW_HEIGHT = 12; const PASS_KINDS = [11, 16]; const WARNING_KIND = 17;
// 경고 메시지의 지연 시간(초)이다(00418070 이 float 지연 10 초를 double 벽 시각에 더한다).
const WARNING_DELAY = 10.0;

// 현재 직계 자식 Gump 에서 읽은 메시지의 식별 필드다(실제 노드의 호스트 토큰과 원본 serial, +a4/+a8/+b8 의 중복/삭제 검색 키).
export class MessageBoardRecord {
    // 호스트 토큰과 원본 serial 이다.
    token = 0; serial = 0;
    // 종류/플레이어/인자(원본 +a4/+a8/+b8)다.
    kind = 0; player = 0; parameter = 0;
}

// 실제 전역 20 DWORD 와 출력 억제/현재 화면 폭이다.
export class MessageBoardState {
    // 첫 칸 0xffffffff 는 원본의 첫 호출 초기화 표식이다(첫 호출에서 모두 0 으로 채운다).
    serials = Uint32Array.from({ length: BOARD_SLOTS }, (_, index) => (index === 0 ? FIRST_CALL_MARK : 0));
    // 원본 00594fa4 의 현재 비영 상태(출력 억제)와 005c78f0 의 현재 화면 폭이다.
    suppressed = false; screenWidth = 0;
}

// 메시지 표 본체다. hooks = { suppressed()(선택, 있으면 매 호출 진입에 현재 값을 읽는다), lookup(serial) → 레코드|null, kill(token), create(kind, player, parameter, text) → 레코드,
//   createText(kind, player, parameter, text|null) → 레코드(원문 NULL 과 빈 문자열을 구분하며 있으면 create 보다 우선), width(token) → 현재 폭(ftol), attach(token, x, y) } 이다.
export class MessageBoard {
    // 현재 20칸 표/억제/해상도 상태와 실제 조회·생성·삭제·부착 훅이다.
    #state; #hooks;
    // 생성/삭제/부착 연결 누락은 사용 전에 TypeError 로 거부한다(C++ invalid_argument).
    constructor(state, hooks) {
        if (typeof hooks?.lookup !== 'function' || typeof hooks.kill !== 'function' || (typeof hooks.create !== 'function' && typeof hooks.createText !== 'function') || typeof hooks.width !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('메시지 표 실제 Gump 연결 누락');
        this.#state = state; this.#hooks = hooks;
    }

    // 유효한 문자열 호출도 같은 부모를 쓰며 빈 문자열을 NULL 로 바꾸지 않는다.
    show(kind, player, enabled, parameter, text) { this.showText(kind, player, enabled, parameter, text); }

    // 원문은 호출 진입에 현재 억제 값을 한 번 읽고 20칸 초기화/검색보다 먼저 판정한다(종류 11/16 은 억제 중에도 통과). 검색 중 발견한 첫 빈 칸을 보관하며 stale serial 도 같은 칸에서 지운다.
    // 중복(종류·플레이어·인자 모두 같고 종류가 17 이 아님)에서 멈추므로 그 뒤의 stale 칸은 미리 정리하지 않는다. enabled=false 면 일치 레코드를 삭제 요청하고 그 칸을 0 으로 쓴다. true 이고 빈 칸이 있으면 첫 빈 칸에 생성하고 serial 을 쓴 뒤
    // 현재 폭(생성/측정 후)만큼 화면 오른쪽에서 떨어진 x(uint32 감김)와 y = 칸×12 에 부착한다. 빈 칸이 없으면 아무것도 생성하지 않는다. text 가 null/undefined(원문 NULL)인데 createText 가 없으면 Error 다.
    showText(kind, player, enabled, parameter, text) {
        const state = this.#state; const hooks = this.#hooks;
        const suppressed = typeof hooks.suppressed === 'function' ? hooks.suppressed() : state.suppressed;
        if (suppressed && !PASS_KINDS.includes(kind)) return;
        if (state.serials[0] === FIRST_CALL_MARK) state.serials.fill(0);
        let firstEmpty = BOARD_SLOTS; let index = 0; let found = null;
        // 표의 각 칸을 차례로 검색한다(중복 일치에서 멈춘다).
        for (; index < BOARD_SLOTS; index += 1) {
            if (state.serials[index]) {
                found = hooks.lookup(state.serials[index]);
                if (!found) state.serials[index] = 0;
                else if (found.kind === kind && found.player === player && found.parameter === parameter && found.kind !== WARNING_KIND) break;
                else found = null;
            }
            if (!state.serials[index] && firstEmpty === BOARD_SLOTS) firstEmpty = index;
        }
        if (!enabled) {
            if (found) hooks.kill(found.token);
            if (index < BOARD_SLOTS) state.serials[index] = 0;
            return;
        }
        if (firstEmpty === BOARD_SLOTS) return;
        const hasText = text !== null && text !== undefined;
        if (typeof hooks.createText !== 'function' && !hasText) throw new Error('메시지 NULL 포인터의 실제 생성 장치 미연결');
        const record = typeof hooks.createText === 'function' ? hooks.createText(kind, player, parameter, hasText ? text : null) : hooks.create(kind, player, parameter, text);
        state.serials[firstEmpty] = record.serial;
        const width = hooks.width(record.token); const x = (state.screenWidth - width) | 0;
        hooks.attach(record.token, x, firstEmpty * ROW_HEIGHT);
    }
}

// 00418070: float 지연 10 초를 double 벽 시각에 더해 double 기한으로 저장한다(디컴파일 C 의 시각 float 변환을 적용하지 않는다).
export function warningDeadline(wallSeconds) { return wallSeconds + WARNING_DELAY; }

// 004d3030: 기한 0 은 영구이며 벽 시각이 기한에 도달하면 삭제를 요청한다. NaN 기한/시각은 비교가 거짓이고 음수의 비영 기한도 원래 비교대로 처리한다.
export function warningExpired(deadline, wallSeconds) { return deadline !== 0.0 && wallSeconds >= deadline; }
