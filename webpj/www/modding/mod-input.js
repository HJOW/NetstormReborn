// 격리 모드의 출력을 호스트에서 다시 검증하고 기존 InputQueue의 FIFO/원자성을 유지한다.
import { InputCode } from '../core/input-queue.js';
import { ModError, MOD_LIMITS, MOD_KEY_NAMES, copyModJson, validateModSnapshot } from './mod-api.js';

// 공개 방향키를 원래 가상 키 누름 코드로 바꾸는 호스트 전용 표다.
const ARROW_CODES = Object.freeze({ ArrowLeft: 0x250000, ArrowRight: 0x270000, ArrowUp: 0x260000, ArrowDown: 0x280000 });
// 경계 형식이 잘못되면 큐를 바꾸기 전에 거절한다.
function check(condition, message) { if (!condition) throw new ModError('invalidArgument', message); }
// 이미 사본이 된 일반 객체의 허용/필수 필드를 정확히 검사한다.
function fields(value, names, label) { check(value !== null && typeof value === 'object' && !Array.isArray(value) && Object.keys(value).length === names.length && names.every((name) => Object.hasOwn(value, name)), `${label} 필드 오류`); }

// 등록/사건의 요청 번호와 미션을 신뢰한 호스트 값에 대조한다. Worker가 보낸 제한/이름은 신뢰하지 않는다.
export function validateModOutput(input, { eventSequence, sessionId }) {
    const output = copyModJson(input);
    fields(output, ['eventSequence', 'commands', 'logs'], '모드 출력');
    check(Number.isSafeInteger(eventSequence) && eventSequence >= 0 && output.eventSequence === eventSequence, '모드 출력 사건 번호 불일치');
    check(Array.isArray(output.commands) && output.commands.length <= MOD_LIMITS.maxCommandsPerEvent && Array.isArray(output.logs) && output.logs.length <= MOD_LIMITS.maxLogsPerEvent, '모드 출력 개수 한도');
    if (eventSequence === 0) check(output.commands.length === 0, '모드 등록 중 명령 금지');
    // 모든 명령을 먼저 검사해 위조된 뒤 요청 때문에 앞 요청만 반영되는 일을 막는다.
    for (let index = 0; index < output.commands.length; index += 1) {
        const command = output.commands[index]; fields(command, ['commandId', 'sessionId', 'name', 'args'], '명령');
        check(command.commandId === index + 1 && typeof sessionId === 'string' && command.sessionId === sessionId, '모드 요청 번호/미션 불일치');
        check(['click', 'pressKey'].includes(command.name), '공개하지 않은 원본 명령 금지');
        fields(command.args, command.name === 'click' ? ['x', 'y', 'button', 'modifiers'] : ['key', 'modifiers'], '명령 인수');
        const modifiers = command.args.modifiers; fields(modifiers, ['shift', 'control'], '명령 보조 키');
        check(typeof modifiers.shift === 'boolean' && typeof modifiers.control === 'boolean', '명령 보조 키 형식 오류');
        if (command.name === 'click') {
            check(Number.isInteger(command.args.x) && command.args.x >= 0 && command.args.x < 16384 && Number.isInteger(command.args.y) && command.args.y >= 0 && command.args.y < 16384 && ['left', 'right'].includes(command.args.button), '클릭 좌표/버튼 형식 오류');
        } else check(MOD_KEY_NAMES.includes(command.args.key), '허용되지 않은 키 요청');
    }
    // 로그도 호스트 쪽 길이/개수를 통과한 문자열만 실제 출력으로 쓴다.
    for (const log of output.logs) { fields(log, ['level', 'text'], '로그'); check(['info', 'warn', 'error'].includes(log.level) && typeof log.text === 'string' && log.text.length <= MOD_LIMITS.maxLogCharacters, '로그 형식/한도 오류'); }
    return output;
}

// 다음 원래 입력 단계 직전에 호출한다. 현재 세션/크기/모달을 재검사하고 누름/뗌 두 개를 함께 넣는다.
export function queueModOutput(input, { eventSequence, sessionId, snapshot, queue, blocked = false, pointer = { x: 0, y: 0 } }) {
    const output = validateModOutput(input, { eventSequence, sessionId }), current = validateModSnapshot(snapshot);
    check(typeof queue?.tryPushBatch === 'function' && typeof blocked === 'boolean', '모드 입력 호스트 장치 오류');
    check(Number.isInteger(pointer.x) && Number.isInteger(pointer.y) && pointer.x >= -0x80000000 && pointer.x <= 0x7fffffff && pointer.y >= -0x80000000 && pointer.y <= 0x7fffffff, '현재 포인터 좌표 오류');
    // 포인터 사본은 성공한 마지막 클릭만 갱신한다. 실패 요청은 실제 마우스를 옮기지 않는다.
    let point = { x: pointer.x, y: pointer.y }; const results = []; let moved = false;
    // 기존 사람 입력 뒤에 모드 호출 순서대로 추가하며 포화 시 자동 재시도하지 않는다.
    for (const command of output.commands) {
        let code = null;
        if (current.state.sessionId === null) code = 'noSession';
        else if (current.state.sessionId !== command.sessionId) code = 'staleSession';
        else if (blocked || current.state.paused) code = 'inputBlocked';
        const args = command.args;
        if (!code && command.name === 'click' && (args.x >= current.state.logicalSize.width || args.y >= current.state.logicalSize.height)) code = 'invalidArgument';
        if (!code) {
            const modifiers = (args.modifiers.shift ? InputCode.shift : 0) | (args.modifiers.control ? InputCode.control : 0);
            const native = command.name === 'click' ? (args.button === 'left' ? InputCode.leftButton : InputCode.rightButton) : (ARROW_CODES[args.key] ?? args.key.charCodeAt(0));
            const target = command.name === 'click' ? { x: args.x, y: args.y } : point;
            const events = [{ code: (native | modifiers) >>> 0, x: target.x, y: target.y }, { code: (native | modifiers | InputCode.release) >>> 0, x: target.x, y: target.y }];
            if (!queue.tryPushBatch(events)) code = 'queueFull';
            else if (command.name === 'click') { point = target; moved = true; }
        }
        results.push({ eventSequence, commandId: command.commandId, status: code ? 'rejected' : 'queued', code });
    }
    return { results, pointer: moved ? point : null };
}
