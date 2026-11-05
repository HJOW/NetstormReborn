// 원본 10.78 입력 사건 큐(DAT_0055a510 의 40칸 고리, 읽기 DAT_0055a508·쓰기 DAT_0055a50c)를 옮긴 모듈이다.
// 창 프로시저가 키·마우스 사건을 넣고(00452f90), 프로그램은 명령 사건을 예약하며(00452fe0), 입력·명령 처리(004d62b0)가 프레임마다 꺼낸다(00452f00).
// 한 칸은 가득 참 구분에 쓰여 39개까지 담기고, 가득 차면 방금 넣은 사건을 버린다. 갱신 단계에서 예약한 명령은 다음 프레임 입력에서 소비된다(main-loop.js 순서).
// 근거: cpppj `client/InputEvent.h/.cpp`(마지막 변경 bfb780c4·225f2322), `tests/ScreenTests.cpp` InputQueue_* 검사, docs/exe/main-loop.md 2절.
// 브라우저 키/포인터 사건을 원본 코드로 바꾸는 창 프로시저(00436550/00436020)와 폴링(00452e00)은 실제 미션 입력과 함께 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C3: 세션 장기 상태의 입력/예약 큐 이식으로 추가)

// 사건 코드 비트이다(창 프로시저 00436550·00436020 이 만든다). 글자가 아닌 키는 가상 키 코드를 16비트 왼쪽으로 밀고, 글자는 아래 16비트에 둔다. (2026-11-05 WB04-C3)
export const InputCode = Object.freeze({
    release: 0x80000000,       // 뗌(키 올림·버튼 올림)
    pressedSince: 0x40000000,  // 폴링: 지난 확인 뒤 눌린 적 있음
    shift: 0x20000000,
    control: 0x10000000,
    alt: 0x08000000,
    outside: 0x04000000,       // 폴링: 커서가 창 밖이거나 창 비활성
    command: 0x02000000,       // 프로그램이 넣은 명령 사건(00452fe0)
    leftButton: 0x00010000,
    rightButton: 0x00020000,
    middleButton: 0x00040000,
    virtualKeyShift: 16,
});

// 큐 칸 수(원본 0x28)이다. 39개까지 담긴다. (2026-11-05 WB04-C3)
export const INPUT_QUEUE_CAPACITY = 40;

// 꺼낼 때 바꾸는 글자(DEL 0x7f → Ctrl+Backspace 8)의 판정 마스크와 명령 사건에 남기는 코드 비트이다. (2026-11-05 WB04-C3)
const CHARACTER_MASK = 0x03ffffff;
const DELETE_CHARACTER = 0x7f;
const COMMAND_CODE_MASK = 0x0100ffff;

// 빈 사건(코드·좌표 0)이다. 비었거나 걸러진 뗌을 꺼낼 때 돌려준다. (2026-11-05 WB04-C3)
function emptyEvent() {
    return { code: 0, x: 0, y: 0 };
}

// 사건 코드(DWORD)와 좌표(signed DWORD)를 검사한다. (2026-11-05 WB04-C3)
function checkEvent(code, x, y) {
    if (!Number.isInteger(code) || code < 0 || code > 0xffffffff) throw new RangeError('입력 사건 코드는 32비트 부호 없는 정수여야 합니다.');
    for (const value of [x, y]) if (!Number.isInteger(value) || value < -0x80000000 || value > 0x7fffffff) throw new RangeError('입력 사건 좌표는 signed 32비트 정수여야 합니다.');
}

// 원본 40칸 입력 사건 고리 큐이다. 사건은 { code, x, y } 이며 꺼낸 사건은 사본이다.
// 사용: const queue = new InputQueue(); queue.push(InputCode.leftButton, 10, 20); queue.pushCommand(0x0200001f, 0, 0); while (!queue.empty) handle(queue.pop(false)). (2026-11-05 WB04-C3)
export class InputQueue {
    // 40칸 사건 저장소이다.
    #events = Array.from({ length: INPUT_QUEUE_CAPACITY }, emptyEvent);
    // 다음에 읽을 칸이다.
    #read = 0;
    // 다음에 쓸 칸이다.
    #write = 0;

    // 00452f90: 쓰기 칸에 넣고 한 칸 옮긴다. 읽기 칸과 만나면 한 칸 되돌려 방금 넣은 사건을 버린다(가득 참).
    push(code, x, y) {
        checkEvent(code, x, y);
        this.#events[this.#write] = { code, x, y };
        this.#write = (this.#write + 1) % INPUT_QUEUE_CAPACITY;
        if (this.#write === this.#read) this.#write = (this.#write + INPUT_QUEUE_CAPACITY - 1) % INPUT_QUEUE_CAPACITY;
    }

    // 00452fe0: 명령 사건을 예약한다. 코드는 0x0100ffff 만 남기고 명령 표시(0x02000000)를 붙여 같은 방식으로 넣는다.
    pushCommand(code, x, y) {
        checkEvent(code, x, y);
        this.push((((code & COMMAND_CODE_MASK) | InputCode.command) >>> 0), x, y);
    }

    // 00452f00: 하나 꺼낸다. 비었으면 빈 사건이다. 글자 DEL(0x7f)은 Ctrl+Backspace(8)로 바꾼다(보조 키 중 Ctrl 자리만 새로 켬).
    // skipReleases 가 참이면 뗌 사건도 큐에서는 빠지지만 빈 사건을 돌려준다(원본은 꺼낸 뒤에 뗌 여부를 본다).
    pop(skipReleases) {
        if (this.#read === this.#write) return emptyEvent();
        const event = { ...this.#events[this.#read] };
        this.#read = (this.#read + 1) % INPUT_QUEUE_CAPACITY;
        if (((event.code & CHARACTER_MASK) >>> 0) === DELETE_CHARACTER) event.code = (((event.code & 0xff000008) | 0x10000008) >>> 0);
        if (!skipReleases || (event.code & InputCode.release) === 0) return event;
        return emptyEvent();
    }

    // 꺼낼 사건이 없는지이다(읽기 칸 = 쓰기 칸).
    get empty() { return this.#read === this.#write; }

    // 꺼낼 수 있는 사건 수이다(진단용, 큐를 바꾸지 않는다).
    get size() { return (this.#write - this.#read + INPUT_QUEUE_CAPACITY) % INPUT_QUEUE_CAPACITY; }
}
