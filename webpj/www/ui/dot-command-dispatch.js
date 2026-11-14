// 원본 0043f6f0 의 로컬 점 명령 분배를 고정 Regexp 문법과 같은 현재 표에 수행한다(cpppj client/RawDotCommandDispatch.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 실제 원문 고정 패턴(00506a30)의 바이트 문법만 구현하며 범용 정규식이나 네트워크 전송을 대체하지 않는다. Client 가 소유하는 원문 명령표(이름/함수/count 인접 물리)와 외부 명령 장치(invoke/error)의 경계다.
// 인자는 127바이트까지 복사하고 이름은 80바이트까지 비교하며 물리 밖 원시 포인터 접근은 RangeError 다. 공유 separatorOffset 은 callback 뒤 재조회하므로 같은 상태의 중첩 분배 효과도 유지한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: dispatchDotCommand(table, state, inputBytes('.help me;.quit\0'), { invoke: (callback, argument) => code, error: (name, code) => {} }) → 1.
// (2026-11-14 WB04-E443: RawMenuControls 선행 점 명령 분배 이식)

// 명령표 안의 함수 표와 signed count 의 오프셋이다.
const CALLBACKS = 0x800; const COUNT = 0xa04;

// 재진입/callback 이 공유하는 00557db4 와 00557df0 의 관측 가능한 현재 값이다.
export class DotCommandDispatchState {
    // PE 의 기본 명령 슬롯(오류 출력 직전에도 현재 값을 다시 읽는다)이다.
    fallbackIndex = 0;
    // 마지막 Regexp 검색의 그룹 3 시작 상대 위치다. 실패는 -1 이며 빈 입력은 보존한다.
    separatorOffset = -1;
}

// 현재 실제 DWORD 네 바이트를 정렬/alias 가정 없이 읽는다(범위 밖은 RangeError).
function read(bytes, at) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('점 명령 분배 DWORD의 실제 물리 범위');
    let value = 0;
    // 네 실제 바이트만 순서대로 읽는다.
    for (let index = 0; index < 4; index += 1) value |= bytes[at + index] << (index * 8);
    return value >>> 0;
}
// NUL 을 포함한 원문 물리를 확인하고 그 앞의 바이트만 바이트 문자열로 빌린다. 인접 슬롯도 같은 물리 안에서는 원문처럼 읽는다(없으면 RangeError).
function text(bytes, at) {
    if (at >= bytes.length) throw new RangeError('점 명령 문자열의 시작 물리 범위');
    const end = bytes.indexOf(0, at);
    if (end < 0) throw new RangeError('점 명령 문자열의 실제 NUL 없음');
    let result = ''; for (let index = at; index < end; index += 1) result += String.fromCharCode(bytes[index]); return result;
}
// 원문 패턴에 직접 적힌 네 바이트(space/tab/CR/LF)만 공백으로 취급한다.
const isSpace = (value) => value === ' ' || value === '\t' || value === '\r' || value === '\n';
// C locale stricmp 의 동등 비교다(ASCII 대소문자만 무시하고 비ASCII 바이트는 변환하지 않는다).
function same(first, second) {
    if (first.length !== second.length) return false;
    const lower = (value) => (value >= 65 && value <= 90 ? value + 32 : value);
    // 끝까지 같은 바이트일 때만 같은 명령이다.
    for (let index = 0; index < first.length; index += 1) if (lower(first.charCodeAt(index) & 255) !== lower(second.charCodeAt(index) & 255)) return false;
    return true;
}
// 00506a30 의 고정 패턴을 평가하고 실패를 포함해 공유 검색 상태(separatorOffset)를 갱신한다. 선행 공백 → '.' → 이름(공백/`;`/`$` 앞까지, 점/콜론/VT/FF 허용, 80바이트 상한) →
// 인자(개행 포함, `;`/`$` 앞까지, 127바이트 상한) 이며 `$` 로 끝나면 일치 없음이다. 반환은 { name, argument }|null 이다.
function search(source, state) {
    state.separatorOffset = -1; let cursor = 0;
    while (cursor < source.length && isSpace(source[cursor])) cursor += 1;
    if (cursor === source.length || source[cursor++] !== '.') return null;
    const start = cursor;
    while (cursor < source.length && !isSpace(source[cursor]) && source[cursor] !== ';' && source[cursor] !== '$') cursor += 1;
    if (cursor === start) return null;
    const name = source.slice(start, start + Math.min(cursor - start, 80)); const argument = cursor;
    while (cursor < source.length && source[cursor] !== ';' && source[cursor] !== '$') cursor += 1;
    if (cursor < source.length && source[cursor] === '$') return null;
    if (cursor > 0x7fffffff) throw new RangeError('점 명령 Regexp의 signed 상대 위치 범위');
    state.separatorOffset = cursor;
    return { name, argument: source.slice(argument, argument + Math.min(cursor - argument, 127)) };
}
// 선택된 함수만 요구하며 반환 코드는 원문 비트 그대로 보존한다(함수 표 주소는 unsigned 감김 뒤 실제 범위를 검사한다).
function invoke(table, index, argument, hooks) {
    const callback = read(table, (CALLBACKS + Math.imul(index, 4)) >>> 0);
    if (typeof hooks.invoke !== 'function') throw new Error('점 명령 실제 함수 장치 미연결');
    return hooks.invoke(callback, argument);
}
// 오류 이름은 명령 장치가 같은 표를 바꾼 뒤의 현재 물리에서 읽는다(결과가 0 이면 아무것도 읽지 않는다).
function report(table, index, result, hooks) {
    if (!result) return;
    const name = text(table, Math.imul(index, 32) >>> 0);
    if (typeof hooks.error !== 'function') throw new Error('점 명령 오류 출력 장치 미연결');
    hooks.error(name, result);
}

// 각 명령 뒤 현재 공유 캡처와 현재 입력 바이트를 재조회한 뒤 나머지를 분배한다. table = 명령표 인접 물리(이름 32바이트×칸·함수 0x800·count 0xa04), state = DotCommandDispatchState,
// input = NUL 을 포함한 실제 가변 입력(Uint8Array), hooks = { invoke(함수 식별자, NUL 포함 인자 Uint8Array) → 원문 반환 코드, error(현재 이름, 비영 코드) } 이다.
// 빈 입력은 1. 일치하면 첫 중복 이름의 슬롯을 선택(signed count 안에서 ASCII 대소문자 무시, 이름 80바이트 비교)해 인자(앞 공백 포함 127바이트)로 호출하고, 일치하지 않으면 기본 슬롯에 입력 전체를 넘긴 뒤 1 을 돌려준다.
// 성공한 명령은 호출 뒤 현재 separatorOffset 으로 다음 `;` 위치로 가서 `;` 를 건너뛰고 남은 입력이 비면 1, 아니면 다음 명령을 같은 방식으로 분배한다.
export function dispatchDotCommand(table, state, input, hooks) {
    let cursor = 0;
    if (text(input, cursor) === '') return 1;
    // 성공한 점 명령은 같은 남은 입력에서 다음 명령 검색으로 이어진다.
    for (;;) {
        const source = text(input, cursor); const found = search(source, state);
        let hit = false; let index = 0;
        if (found) {
            // 원문의 signed count 를 현재 표에서 읽으며 첫 중복 이름이 선택된다.
            while ((read(table, COUNT) | 0) > (index | 0)) {
                if (same(text(table, Math.imul(index, 32) >>> 0), found.name)) { hit = true; break; }
                index += 1;
            }
        }
        if (!hit) {
            const result = invoke(table, state.fallbackIndex, input.subarray(cursor, cursor + source.length + 1), hooks);
            report(table, state.fallbackIndex, result, hooks);
            return 1;
        }
        // 원문 지역 128바이트에서 실제 캡처와 NUL 이 쓴 범위만 장치에 제공한다.
        const argument = new Uint8Array(128);
        for (let at = 0; at < found.argument.length; at += 1) argument[at] = found.argument.charCodeAt(at) & 0xff;
        argument[found.argument.length] = 0;
        const result = invoke(table, index, argument.subarray(0, found.argument.length + 1), hooks);
        report(table, index, result, hooks);
        // 중첩 callback 이 바꾼 공유 Regexp 상대 위치도 그대로 사용한다.
        const next = cursor + state.separatorOffset;
        if (next < 0 || next >= input.length) throw new RangeError('점 명령 다음 구분자의 실제 입력 물리 범위');
        cursor = next;
        if (input[cursor] === 0x3b) cursor += 1;
        if (text(input, cursor) === '') return 1;
    }
}
