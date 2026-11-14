// 공통 로그(trace)의 선택 관문에서 현재 Chat UI로 문자열을 넘기는 장치다(원문 434f40→42f190, cpppj client/TraceChat.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 원문 설정을 추가하거나 강제로 켜지 않고 같은 Client 의 현재 관문만 소비한다. 현재 색/현재 Chat 포인터는 실행 위치에서 다시 읽으며 UI 소유자는 이 콜백보다 오래 살아야 한다.
// 서식은 인자 없는 일반 문자와 `%%`만 지원한다. 추가 인자가 필요한 `%` 서식은 원문이 정의하지 않은 스택을 읽으므로 명시 거부한다(Error). 전용 PE fixture 는 없다.
// 사용: forwardTraceToChat(enabled, '준비 100%%', { color: () => 0x7f7f7f, currentChat: () => chatToken, append: (chat, text, context) => chatWindow.append(chat, text, context) }).
// (2026-11-14 WB04-E437: RawMenuControls 선행 trace→Chat 전달 이식)

// 원문 434f40 지역 서식 버퍼의 1024바이트에서 끝 NUL 하나를 뺀 문자 수다.
const MAX_TEXT = 1023;

// 도달한 장치만 요구하며 현재 포인터 0 을 가짜 UI 로 대체하지 않는다. 앞선 서식/문맥 효과는 보존하고 실제 선택 장치의 부재에서 중단한다.
function need(hook) {
    if (typeof hook !== 'function') throw new Error('trace Chat의 선택 장치 미연결');
    return hook;
}

// 입력(바이트 문자열 또는 Uint8Array)을 첫 NUL 앞의 바이트 문자열로 줄인다.
function untilNul(format) {
    const value = typeof format === 'string' ? format : Array.from(format, (byte) => String.fromCharCode(byte)).join('');
    const nul = value.indexOf('\0');
    return nul < 0 ? value : value.slice(0, nul);
}

// 434f40 은 42f190 에 추가 가변인자를 주지 않으므로 정의된 일반 문자/`%%`만 처리한다. 출력은 1023자까지다(RangeError).
function formatTraceChat(format) {
    const source = untilNul(format);
    let text = '';
    // 인자 없는 서식을 왼쪽부터 한 번 읽는다.
    for (let index = 0; index < source.length; index += 1) {
        let character = source[index];
        if (character === '%') {
            index += 1;
            if (index >= source.length || source[index] !== '%') throw new Error('trace Chat 원문 서식의 미제공 가변인자 읽기');
            character = '%';
        }
        if (text.length === MAX_TEXT) throw new RangeError('trace Chat 원문1024BYTE 서식 범위');
        text += character;
    }
    return text;
}

// enabled 가 0 이면 아무 장치도 읽지 않는다. 비영이면 색을 읽어 8DWORD 문맥 [0,1,색,0,0,0,1,1]을 고정한 뒤 서식을 만들고 현재 Chat 포인터를 읽어 0 이 아닐 때만 append 한다.
// hooks = { color() → DWORD (005acd24), currentChat() → DWORD (00548740), append(chat, text, context: Uint32Array(8)) (42eca0) }. root 가 0 이어도 서식은 먼저 처리한다(오류는 그대로 난다).
export function forwardTraceToChat(enabled, format, hooks) {
    if (!(enabled >>> 0)) return;
    // 원문 434f40 의 32바이트 지역 문맥이다. 색은 서식 처리 전에 읽어 고정한다.
    const context = Uint32Array.of(0, 1, need(hooks.color)() >>> 0, 0, 0, 0, 1, 1);
    const text = formatTraceChat(format);
    const chat = need(hooks.currentChat)() >>> 0;
    if (chat) need(hooks.append)(chat, text, context);
}
