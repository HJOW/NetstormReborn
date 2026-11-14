// 원본 HUD 구분선(divider)의 입력(004c49f0 가상+28)과 프레임 방송(004c4b00 가상+2c)을 같은 공통 Gump 수집기/입력/트리에 연결한다(cpppj `client/HudDividerInput.cpp/.h`, 기준선 1f00a25a, 10.78 전용).
// 입력은 먼저 부모 체인을 따라 화면 좌표를 로컬 float 로 바꾼 뒤(004650a0) 좌클릭 기준점/캡처·release·hover 커서·기본 Help 순서로 처리한다. 방송은 메뉴 인스턴스가 0 일 때만 폴링→release 비트 정리→축/제한 계산→공통 Move 를 실행하며
// 마우스 좌표에서 float2(00501444)를 빼고 원문처럼 정수 제한과 비교한 뒤 제한이 선택될 때만 저장된 float 값을 쓴다. 별도 Idle 등록은 없다.
// 사용: const input = new HudDividerInput(collection, gumpInput, tree, { divider, base, baseHandle, cursor, menuInstances, poll, cursorEvent }); input.handle(token, event, first, second); input.broadcast(token, event, first, second).
//   hooks = { divider(token)→HudDividerNode, base(token)→GumpCreateNode(같은 객체여야 함), baseHandle(token,event,first,second)→DWORD(미처리 사건의 기본 Help), cursor(4|5)(커서 모양), menuInstances()→DWORD(00593024 현재 메뉴 수),
//   poll(code)→{code}(원본 00452e00 좌클릭 폴링), cursorEvent(기본 0200003b, 005427e4 hover 사건) }. 공유 조회 둘만 만들 때 필수이고 나머지는 도달한 분기에서 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E471: 5-B HudDividerInput 이식, 기준선 뒤 변경 없음)

// 원문 사건 코드 마스크·좌클릭 코드·release 비트와 노드 플래그의 기준점(0x10)/방송(0x100) 비트이다.
const CODE_MASK = 0x03ffffff; const LEFT = 0x10000; const RELEASE = 0x80000000; const ANCHOR = 0x10; const NOTIFY = 0x100;
// 00501444 는 드래그 마우스 좌표에서 빼는 float 2 이다.
const POINTER_OFFSET = 2.0;
// 기본 hover 사건 코드(005427e4)이다.
const DEFAULT_CURSOR_EVENT = 0x0200003b;

// 원문은 int 제한과 비교한 뒤 제한이 선택될 때만 저장된 float 값을 사용한다. 두 번째 비교는 첫 제한이 바꾼 값으로 한다.
function limited(value, minimum, maximum) {
    if (value < minimum) value = Math.fround(minimum);
    if (maximum < value) value = Math.fround(maximum);
    return value;
}

// float32 값의 원시 비트이다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 원시 비트를 float32 로 읽는다.
function bitsFloat(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }

// 도달한 분기의 장치만 요구한다(C++ invalid_argument → TypeError).
function need(callback, message) { if (typeof callback !== 'function') throw new TypeError(message); return callback; }

// divider 입력 처리기이다. 같은 노드/위치/입력/이동 처리기는 이 객체보다 오래 살아야 한다. (2026-11-15 WB04-E471)
export class HudDividerInput {
    // 부모 체인으로 입력마다 현재 로컬 좌표를 계산하는 수집기, 첫 캡처만 유지하는 공유 입력, anchor 보정과 가상 Push/Attach 순서를 재사용하는 트리, 현재 메뉴 수와 외부 폴링/커서/Help 경계이다.
    #collection; #input; #tree; #hooks;
    // 공유 노드 조회 둘만 확인한다(없으면 TypeError). 사용: new HudDividerInput(collection, input, tree, hooks). (2026-11-15 WB04-E471)
    constructor(collection, input, tree, hooks) {
        if (typeof hooks?.divider !== 'function' || typeof hooks.base !== 'function') throw new TypeError('Divider 입력 공유 저장소 필요');
        this.#collection = collection; this.#input = input; this.#tree = tree; this.#hooks = { cursorEvent: DEFAULT_CURSOR_EVENT, ...hooks };
    }

    // GumpTree/Collection 이 읽는 주소와 파생 필드 주소가 동일해야 한다(없거나 다르면 Error).
    #node(token) {
        const node = token ? this.#hooks.divider(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('Divider 입력 공유 노드 없음');
        return node;
    }

    // 004c49f0: 먼저 화면 좌표를 로컬로 바꾼 뒤 좌클릭 누름(축 비트 1|2 가 있을 때)은 기준점/방송 비트와 로컬 위치 두 값을 저장하고 입력을 캡처한 뒤 1 을 반환한다. 좌클릭 release 는 축 비트와 무관하게 방송 비트를 지우고 1 이다
    // (focus 해제는 공통 Route 가 담당). hover 사건은 세로 축(2)이면 커서 5, 가로 축(1)이면 커서 4 를 요청하고 1 이다. 그 밖은 기본 Help 처리 결과를 그대로 반환한다. first/second 는 signed 화면 좌표 DWORD 이다.
    handle(token, event, first, second) {
        const node = this.#node(token);
        const local = this.#collection.screenToLocal(token, { x: first | 0, y: second | 0 });
        const code = (event >>> 0) & CODE_MASK; const released = ((event >>> 0) & RELEASE) !== 0;
        if (!released && code === LEFT && (node.axis & 3)) {
            node.flags = (node.flags | ANCHOR) >>> 0; node.positioning[0] = floatBits(local[0]); node.positioning[1] = floatBits(local[1]);
            node.flags = (node.flags | NOTIFY) >>> 0; this.#input.capture(token); return 1;
        }
        if (released && code === LEFT) { node.flags = (node.flags & ~NOTIFY) >>> 0; return 1; }
        if (code === this.#hooks.cursorEvent) {
            if (node.axis & 2) { need(this.#hooks.cursor, 'Divider 세로 커서 장치 필요')(5); return 1; }
            if (node.axis & 1) { need(this.#hooks.cursor, 'Divider 가로 커서 장치 필요')(4); return 1; }
        }
        return need(this.#hooks.baseHandle, 'Divider 기본 Help 처리 필요')(token, event, first, second);
    }

    // 004c4b00: 드래그는 visible/notify/dead 선택을 끝낸 공통 Broadcast 가 호출하며 이 본체는 그 조건을 재검사하지 않는다. 메뉴 인스턴스가 0 일 때만 좌클릭 폴링을 하고 release 이면 방송 비트를 지운다.
    // 세로 축(2)은 y = 마우스 y − 2(축 8 이면 정수 제한 minY/maxY)와 x = 저장된 로컬 x + 노드 x, 가로 축(1)은 x = 마우스 x − 2(축 4 이면 minX/maxX)와 y = 저장된 로컬 y + 노드 y 이며 둘 다 아니면 반환한다. 최종 인자는 단정도로 줄여 공통 Move 에 넘긴다.
    broadcast(token, event, first, second) {
        void event;
        const node = this.#node(token);
        if (need(this.#hooks.menuInstances, 'Divider 현재 메뉴 인스턴스 수 필요')()) return;
        if ((need(this.#hooks.poll, 'Divider 좌클릭 폴링 필요')(LEFT).code >>> 0) & RELEASE) node.flags = (node.flags & ~NOTIFY) >>> 0;
        let x; let y;
        if (node.axis & 2) {
            let position = (second | 0) - POINTER_OFFSET;
            if (node.axis & 8) position = limited(position, node.minY, node.maxY);
            y = Math.fround(position); x = Math.fround(bitsFloat(node.positioning[0]) + node.x);
        } else {
            if (!(node.axis & 1)) return;
            let position = (first | 0) - POINTER_OFFSET;
            if (node.axis & 4) position = limited(position, node.minX, node.maxX);
            x = Math.fround(position); y = Math.fround(bitsFloat(node.positioning[1]) + node.y);
        }
        this.#tree.move(token, x, y, 0);
    }
}
