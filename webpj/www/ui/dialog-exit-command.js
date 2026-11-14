// 원본 HTML 부모의 OnExit 를 메타 해석 시 고정하고 실제 반납 시 같은 입력 큐에 한 번 예약한다.
// (cpppj client/DialogExitCommand.cpp/.h, 기준선 1f00a25a, 10.78 전용)
// 메타는 명령 전체 토큰을 다시 자르지 않고 이미 얻은 regex 캡처 원문을 조회한다. 같은 페이지에서 여러 번 해석하면 마지막 결과가 남고 따옴표/공백/쉼표를 제거하지 않는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E435: RawMenuControls 선행 HTML 대화상자 종료 명령 이식)
import { configParseLong } from '../assets/config.js';

// 원문 regex 캡처의 지역 명령(80)/인자(256) 물리 버퍼 크기(NUL 포함)다.
const COMMAND_BUFFER = 80; const ARGUMENT_BUFFER = 256;

// HTML 부모 +21e0/+21e4 의 해석된 명령과 인자다. 문자열 인자는 기존 80칸 고리의 토큰이다. command 0 은 소멸 시 예약하지 않으며 0xffffffff 는 별도 거부하지 않는다.
export class DialogExitCommandState {
    // 해석된 명령 코드와 숫자 또는 현재 문자열 고리 토큰이다(둘 다 DWORD).
    command = 0; parameter = 0;
}

// 원문 C 문자열의 첫 NUL 뒤는 버리고 한 바이트 넘치는 원본 NUL 쓰기는 호스트에서 명시 거부한다.
function captured(source, capacity) {
    const nul = source.indexOf('\0'); if (nul >= 0) source = source.slice(0, nul);
    if (source.length >= capacity) throw new RangeError('OnExit 메타 캡처의 원본 지역 버퍼 NUL 범위 초과');
    return source;
}

// 원본 c83b0 의 lookup→진단 0→첫 BYTE isdigit→atol/문자열 고리→두 필드 저장 순서다. 원문은 메타를 읽을 때 문자열 고리를 즉시 진행하며 소멸 시 다시 해석하면 고리 수명이 달라진다.
// hooks = { commands: { lookup(명령) → 코드, unknown(명령) → 진단 }, storeText(문자열) → 고리 토큰 }. 모든 조회/변환이 끝난 뒤 두 필드를 함께 교체한다. 사용: resolveDialogExitCommand(state, 'Config', 'A=1', hooks)
export function resolveDialogExitCommand(state, command, argument, hooks) {
    command = captured(command, COMMAND_BUFFER); argument = captured(argument, ARGUMENT_BUFFER);
    if (typeof hooks?.commands?.lookup !== 'function') throw new TypeError('OnExit 현재 명령표 조회 누락');
    const code = hooks.commands.lookup(command) >>> 0; // native 4c85da 의 현재 명령표 결과다.
    if (!code) {
        if (typeof hooks.commands.unknown !== 'function') throw new Error('OnExit code 0 의 원본 진단 미연결');
        hooks.commands.unknown(command);
    }
    let parameter; // native 4c863c 가 보관할 숫자 또는 현재 고리 토큰이다.
    if (argument.length > 0 && argument[0] >= '0' && argument[0] <= '9') parameter = configParseLong(argument) >>> 0;
    else {
        if (typeof hooks.storeText !== 'function') throw new Error('OnExit 현재 문자열 고리 미연결');
        parameter = hooks.storeText(argument) >>> 0;
    }
    state.command = code; state.parameter = parameter; // native 4c8a58/5e 처럼 모든 조회/변환이 끝난 뒤 두 필드를 교체한다.
}

// 실제 HTML 부모는 한 번만 소멸한다. 먼저 보유값을 비운 뒤 queue(code, param, param)한다(콜백이 페이지 수명을 다시 정리해도 같은 예약을 반복하지 않는다). 명령 0 이면 예약하지 않는다.
// queue 는 함수 (command, first, second) 다. 사용: disposeDialogExitCommand(state, (command, first, second) => queue.pushCommand(command, first | 0, second | 0))
export function disposeDialogExitCommand(state, queue) {
    if (state.command && typeof queue !== 'function') throw new Error('OnExit 실제 입력 큐 미연결');
    const command = state.command; const parameter = state.parameter; state.command = 0; state.parameter = 0;
    if (command) queue(command, parameter, parameter);
}

// 이미 검증된 실제 ScriptStringRing 을 빌려 문자열 고리와 현재 명령 조회/진단을 묶는다. 수명을 이 helper 의 반환값보다 길게 유지해야 한다.
export function makeDialogExitResolveHooks(strings, commands) { return { commands, storeText: (text) => strings.store(text) }; }

// 같은 40칸 InputQueue 의 실제 예약 입구이며 별도 페이지 전환 큐를 만들지 않는다. 원문 52f90 의 DWORD 두 인자를 비트 보존해 예약한다.
export function disposeDialogExitCommandWithQueue(state, queue) {
    disposeDialogExitCommand(state, (command, first, second) => queue.pushCommand(command, first | 0, second | 0));
}
