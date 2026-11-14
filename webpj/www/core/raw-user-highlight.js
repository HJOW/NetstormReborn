// 원본 Userinput.cpp 의 강조 모드 004d5bb0 와 마우스 타입 초기화 004d5480 을 옮긴다(cpppj client/RawUserHighlight.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 실제 선택(005caea0)과 강조(005cae90)는 다른 전역이다. 이 처리기는 모드 변경만 다루며 선택 해제(004d5c90/004d5490)와 별개이고 selectedSid 를 지우지 않는다. 모든 참조 대상은 이 처리기보다 오래 살아야 한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const highlight = new RawUserHighlight(state, { selectCursor, project, move }); highlight.highlight(5);
// (2026-11-14 WB04-E442: RawMenuControls 선행 사용자 강조 이식)

// 선택된 필수 외부 몸체가 없으면 앞선 원본 쓰기를 유지한 채 그 위치에서 거부한다(logic_error→Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('실제 사용자 강조 경계 미연결'); return callback; }

// state = { mode(005cae90 현재 강조 모드), selectedSid(005caea0 현재 선택 SID, 읽기만), mouseType(005caea4 마우스 대상 TYPE), mouseX/mouseY(005cae94/98 현재 정수 화면 좌표, 모드 5 에서만 읽음) } 를 제자리에서 바꾼다.
// hooks = { selectCursor(번호)(004a4410), project(x, y) → [xBits, yBits](004971e0 현재 카메라 역투영의 두 DWORD), move(selectedSid, xBits, yBits, 0)(004284a0) } 이며 도달한 것만 요구한다.
export class RawUserHighlight {
    // 같은 장기 선택/마우스 전역과 선택된 외부 소비자다.
    #state; #hooks;
    constructor(state, hooks) { this.#state = state; this.#hooks = hooks; }

    // 004d5480: 마우스 TYPE 만 0 으로 쓰며 강조 모드/선택/화면 좌표는 보존한다.
    resetMouse() { this.#state.mouseType = 0; }

    // 004d5bb0: 같은 모드는 효과 없이 반환하고, 변경 때만 모드 쓰기 → 마우스 TYPE 초기화 → 현재 모드의 효과(1~3 커서 3, 4 커서 2, 5 역투영 뒤 이동 강조, 그 밖은 커서 1)를 실행한다.
    // 모드 5 는 역투영 callback 의 선택 변경을 이동 인자에 섞지 않도록 역투영 전에 선택 SID 를 보관하고 역투영이 돌려준 두 DWORD 를 그대로 넘긴다.
    highlight(mode) {
        mode >>>= 0;
        if (mode === (this.#state.mode >>> 0)) return;
        this.#state.mode = mode; this.resetMouse();
        switch (mode) {
        case 1: case 2: case 3: need(this.#hooks.selectCursor)(3); return;
        case 4: need(this.#hooks.selectCursor)(2); return;
        case 5: {
            const selected = this.#state.selectedSid >>> 0;
            const point = need(this.#hooks.project)(this.#state.mouseX | 0, this.#state.mouseY | 0);
            need(this.#hooks.move)(selected, point[0] >>> 0, point[1] >>> 0, 0);
            return;
        }
        default: need(this.#hooks.selectCursor)(1);
        }
    }
}
