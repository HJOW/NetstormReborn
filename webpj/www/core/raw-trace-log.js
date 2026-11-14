// 원본 공유 trace 로그를 기준선 1f00a25a RawTraceLog.cpp/.h에서 옮긴다. 10.78 전용이며 DOM/FILE에 의존하지 않는다.
// 사용: writeRawTraceLog(state, level, formattedByteString, prefixBytes, hooks). console→선택된 stderr 복제→현재 stream→선택 flush 순서다.
// hooks는 출력 때마다 현재 공유 버퍼/스트림을 다시 빌린다. 브라우저 console 출력은 Client 장기 소유자 조립 때 hooks.write에 연결한다. (2026-11-14 WB04-E451)

// 원문 00542658/005ca8d0..dc/005ca4d0의 같은 장기 상태다.
export class TraceLogState {
    // PE 초기 로그 레벨이며 음수 호출과 이보다 큰 호출은 입력도 읽지 않는다.
    level = 29;
    // FILE 포인터 대신 호스트 출력 토큰을 쓴다. 0이면 첫 출력에서 standardOut을 선택한다.
    stream = 0;
    // signed 양수일 때만 공백을 넣는 DWORD와 stderr 복제/즉시 flush 관문이다.
    indent = 0; mirror = 0; flush = 0;
    // NUL 뒤 stale BYTE도 보존하는 단일 실제 물리다.
    buffer = new Uint8Array(1024);
}

// 선택한 출력/flush 장치만 그 호출 위치에서 요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('실제 trace 로그 장치 미연결'); return hook; }
// 현재 물리의 첫 NUL까지 바이트 문자열로 읽고 NUL 부재는 거부한다.
function current(state) {
    const end = state.buffer.indexOf(0); if (end < 0) throw new Error('trace 로그 공유 버퍼 NUL 없음');
    let text = '';
    // 인코딩을 바꾸거나 과거 문자열 사본을 재사용하지 않는다.
    for (let index = 0; index < end; index += 1) text += String.fromCharCode(state.buffer[index]);
    return text;
}

// 서식 변환이 끝난 실제 바이트 문자열과 원문 format의 선행 ~쌍 폭을 받는다. 문자열 인자의 ~는 접두로 오인하지 않는다.
export function writeRawTraceLog(state, level, formatted, prefixBytes, hooks) {
    level |= 0; if (level < 0 || level > (state.level | 0)) return;
    const nul = formatted.indexOf('\0'); if (nul >= 0) formatted = formatted.slice(0, nul);
    if (!Number.isInteger(prefixBytes) || prefixBytes < 0 || prefixBytes > formatted.length || (prefixBytes & 1)) throw new TypeError('trace 로그 format 접두 범위');
    // 원문 format의 실제 선행 색상 쌍만 검사한다.
    for (let index = 0; index < prefixBytes; index += 2) if (formatted[index] !== '~') throw new TypeError('trace 로그 format 접두 쌍');
    const spaces = (state.indent | 0) > 0 ? state.indent >>> 0 : 0;
    if (spaces >= state.buffer.length || formatted.length >= state.buffer.length - spaces) throw new RangeError('trace 로그 원본1024BYTE 범위');
    const line = formatted.slice(0, prefixBytes) + ' '.repeat(spaces) + formatted.slice(prefixBytes);
    // 먼저 입력을 고정한 뒤 같은 물리에 쓴다. 나머지 꼬리를 지우지 않는다.
    for (let index = 0; index < line.length; index += 1) state.buffer[index] = line.charCodeAt(index) & 0xff;
    state.buffer[line.length] = 0;
    if (!state.stream) { const standardOut = hooks.standardOut ?? 1; if (!standardOut) throw new Error('trace 로그 실제 stdout 미연결'); state.stream = standardOut; }
    let end = line.length; let stripped = false;
    // 실제 formatter 결과의 끝 CR/LF를 모두 NUL로 쓰고 개행 한 개만 되살린다.
    while (end && (state.buffer[end - 1] === 13 || state.buffer[end - 1] === 10)) { end -= 1; state.buffer[end] = 0; stripped = true; }
    if (stripped) { state.buffer[end] = 10; state.buffer[end + 1] = 0; }
    need(hooks.console)(current(state));
    if (state.stream && state.mirror) need(hooks.write)(hooks.standardError ?? 2, current(state));
    need(hooks.write)(state.stream, current(state));
    if (state.flush) need(hooks.flush)(state.stream);
}

// 직접 flush는 현재 stream 0이면 장치 없이 복귀한다. 콜백이 바꾼 현재 전역은 복구하지 않는다.
export function flushRawTraceLog(state, hooks) { if (state.stream) need(hooks.flush)(state.stream); }
