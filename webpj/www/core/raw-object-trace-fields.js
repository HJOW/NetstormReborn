// 객체 Base 와 내장 Form 의 원문 진단(trace) 서식과 현재 필드 조회를 옮긴 모듈이다(cpppj `o/RawObjectTraceFields.cpp/.h`, 10.78 전용).
//  - traceRawObjectFields(pool, types, suppress, sid, kind, log): 공통 Base 한 줄(`~1~y<SID 4자리>~n Base type=<TYPE>/~y<이름 앞 20자>~n [next=…] [head=…] [~rFREE~n ] [~rDEAD~n ] [~rVOID~n ] [CONTAINED ]\n`)을
//    레벨 29 로 기록한 **뒤에** 선택된 파생 필드를 슬롯에서 다시 읽어(기록 장치가 첫 줄 직후 필드를 바꿔도 바뀐 값) 두 번째 줄을 기록한다.
//    kind 별 두 번째 줄: Form(004ae1e0) 부모 WORD(+16)·이전 WORD(+14, 0 이면 생략), Process(004ae3f0) 부호 있는 PID DWORD(+18),
//    Depend(004ae530) 감시 Form 쌍(+18·+20), Content(004aea90) 내용 TYPE(+18)·이름(`sprintf(name, "XXX")` 치환 뒤 20자)·수량(+12)·flags(+20, 소문자 16진수), Base 는 두 번째 줄이 없다.
//    next/head 는 값이 있고 `suppress`(005c8454)가 0 일 때만 출력한다. 이름은 첫 NUL 에서 끝나고 Base 줄에서 20 BYTE 로 줄인다. 음수/큰 수는 C 의 `%0*d` 규칙(부호 포함 폭)으로 쓴다.
//  - Content 이름 서식은 단일 `%s`/`%%` 만 원문 경로로 받으며(`%s` 는 "XXX" 로 치환, 폭·정밀도·`-` 정렬만 지원) 추가 인자/지원하지 않는 서식/80 BYTE 임시 버퍼 초과는 거부한다(앞선 Base 줄은 이미 기록됨).
// 파생 자산의 Main/Task 는 이 다섯 본체와 별도 연결한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E230: 객체 trace 필드 서식 이식으로 추가)

// 실제 가상 +70 또는 자산의 선행 직접 호출이 선택하는 원문 본체이다. (2026-11-10 WB04-E230)
export const ObjectTraceFieldKind = Object.freeze({
    Base: 0, // 004ac400: 공통 TYPE/링크/상태 한 줄만 기록한다.
    Form: 1, // 004ae1e0: 공통 줄 뒤 현재 부모/이전 WORD 를 기록한다.
    Process: 2, // 004ae3f0: 공통 줄 뒤 현재 signed PID DWORD 를 기록한다.
    Depend: 3, // 004ae530: 공통 줄 뒤 현재 감시 Form 쌍을 기록한다.
    Content: 4, // 004aea90: 공통 줄 뒤 현재 내용 TYPE/수량/flags 를 기록한다.
});

// 로그 레벨(원문 29), 슬롯 필드 위치, 이름 축약 길이, 임시 버퍼 한도이다. (2026-11-10 WB04-E230)
const LOG_LEVEL = 29;
const NAME_LIMIT = 20;
const CONTENT_BUFFER = 80;
const TYPE_AT = 10;
const STATE_AT = 11;

// TYPE 의 전체 C문자열을 첫 NUL 에서 끝낸다. Base 는 이후 따로 20 BYTE 로 자른다. (2026-11-10 WB04-E230)
function cString(name) {
    const end = name.indexOf('\0');
    return end === -1 ? name : name.slice(0, end);
}

// 정렬되지 않은 WORD/DWORD 를 원문 little endian 폭으로 읽는다(요청한 물리 폭만). (2026-11-10 WB04-E230)
function read(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value;
}

// 부호 있는 `%0*d`: 폭은 부호를 포함하고 0 으로 채운다. (2026-11-10 WB04-E230)
function number(value, width = 0) {
    const negative = value < 0;
    const digits = String(negative ? -value : value);
    const sign = negative ? '-' : '';
    return sign + '0'.repeat(Math.max(0, width - sign.length - digits.length)) + digits;
}

// TYPE 의 이름 포인터는 범위를 검사하되 20 BYTE 제한을 적용하지 않는다. (2026-11-10 WB04-E230)
function nameOf(types, type) {
    if (type >= types.length) throw new RangeError('객체 서식 현재 TYPE 범위');
    return cString(types[type].name);
}

// Content 의 `sprintf(name, "XXX")` 를 정상 단일 `%s`/`%%` 서식에만 적용한다. 추가 인자·지원하지 않는 서식·80 BYTE 초과는 원문 미정의 경로라 거부한다. (2026-11-10 WB04-E230)
function contentName(format) {
    let result = '';
    let consumed = false; // 정상 한 개 문자열 인자를 소비했는지이다.
    for (let index = 0; index < format.length;) { // C문자열 전체를 읽고 각 변환의 너비/정밀도를 그대로 적용한다.
        if (format[index] !== '%') { result += format[index]; index += 1; }
        else if (index + 1 < format.length && format[index + 1] === '%') { result += '%'; index += 2; }
        else {
            index += 1;
            let flags = '';
            while (index < format.length && '-+ #0'.includes(format[index])) { flags += format[index]; index += 1; } // 원본 printf 플래그를 읽는다.
            let width = '';
            while (index < format.length && format[index] >= '0' && format[index] <= '9') { width += format[index]; index += 1; } // 상수 너비만 지원한다.
            let precision = null;
            if (index < format.length && format[index] === '.') { // 문자열 정밀도의 상수 BYTE 한도이다.
                index += 1;
                let digits = '';
                while (index < format.length && format[index] >= '0' && format[index] <= '9') { digits += format[index]; index += 1; }
                precision = digits === '' ? 0 : Number(digits);
            }
            if (consumed || index >= format.length || format[index] !== 's') throw new TypeError('Content trace 의 추가/미정의 CRT 인자');
            if (flags.replaceAll('-', '') !== '') throw new TypeError('Content trace 의 `%s` 는 `-` 외 플래그를 지원하지 않는다');
            consumed = true;
            index += 1;
            let text = 'XXX';
            if (precision !== null) text = text.slice(0, precision);
            const pad = Math.max(0, Number(width || 0) - text.length);
            text = flags.includes('-') ? text + ' '.repeat(pad) : ' '.repeat(pad) + text;
            if (text.length >= CONTENT_BUFFER) throw new RangeError('Content trace 80BYTE 임시 버퍼 범위'); // 원본 80 BYTE 임시 버퍼와 같은 크기이다.
            result += text;
        }
        if (result.length >= CONTENT_BUFFER) throw new RangeError('Content trace 80BYTE 임시 버퍼 범위');
    }
    return result;
}

// 이미 발생한 공통 출력을 취소하지 않고 실제 로그 장치 부재를 거부한다. (2026-11-10 WB04-E230)
function emit(log, line) {
    if (typeof log !== 'function') throw new TypeError('객체 서식 실제 로그 미연결');
    log(LOG_LEVEL, line);
}

// 공통 Base 의 현재 TYPE/링크/상태를 기록한 뒤 선택된 파생 필드를 다시 조회한다.
//  pool: 10.78 SidPool(slot(sid) 는 free/dead 슬롯도 읽음), types: 188개 {name} 레코드, suppress: () => 정수 또는 정수(005c8454 덤프 모드), sid: 정수,
//  kind: ObjectTraceFieldKind, log(level, bytes): 같은 로그 장치(서식 완료 문자열을 받음). 범위 밖 SID/TYPE 는 RangeError, 다른 판본/표 크기는 TypeError 이다. (2026-11-10 WB04-E230)
export function traceRawObjectFields(pool, types, suppress, sid, kind, log) {
    if (pool?.edition !== 'patch1078' || types?.length !== 188) throw new TypeError('객체 서식 PATCH 풀/TYPE 필요');
    const dump = typeof suppress === 'function' ? suppress : () => suppress;
    if (sid >= pool.capacity) throw new RangeError('객체 서식 물리 SID 범위');
    let raw = pool.slot(sid);
    const name = nameOf(types, raw[TYPE_AT]);
    let line = `~1~y${number(sid, 4)}~n Base type=${number(raw[TYPE_AT])}`;
    if (name !== '') line += `/~y${name.slice(0, NAME_LIMIT)}~n`;
    line += ' ';
    if (read(raw, 4, 2) && !dump()) line += `next=~y${number(read(raw, 4, 2), 4)}~n `;
    if (read(raw, 6, 2) && !dump()) line += `head=~y${number(read(raw, 6, 2), 4)}~n `;
    if (raw[STATE_AT] & 1) line += '~rFREE~n ';
    if (raw[STATE_AT] & 2) line += '~rDEAD~n ';
    if (raw[STATE_AT] & 4) line += '~rVOID~n ';
    if (raw[STATE_AT] & 8) line += 'CONTAINED ';
    line += '\n';
    emit(log, line);
    if (kind === ObjectTraceFieldKind.Base) return;
    raw = pool.slot(sid); // 기록 장치가 첫 줄 직후 바꾼 현재 필드를 다시 읽는다.
    switch (kind) {
    case ObjectTraceFieldKind.Form: {
        let previous = ''; // 파생 Form 의 현재 이전 WORD 는 덤프 모드에서도 표시한다.
        if (read(raw, 14, 2)) previous = `prev=~y${number(read(raw, 14, 2), 4)}~n `;
        emit(log, `~1     Form parent=~y${number(read(raw, 16, 2), 4)}~n ${previous}\n`);
        return;
    }
    case ObjectTraceFieldKind.Process:
        emit(log, `~1     Process #~w${number(read(raw, 18, 4) | 0, 3)}~n\n`);
        return;
    case ObjectTraceFieldKind.Depend:
        emit(log, `~1     Depend watcherFid=~y${number(read(raw, 18, 2), 4)}~n watchedFid=~y${number(read(raw, 20, 2), 4)}~n\n`);
        return;
    case ObjectTraceFieldKind.Content: {
        const type = read(raw, 18, 2);
        const content = contentName(nameOf(types, type));
        emit(log, `~1     ContentType=${number(type)}/~y${content.slice(0, NAME_LIMIT)}~n q=${number(read(raw, 12, 2))} flags=${read(raw, 20, 2).toString(16)}\n`);
        return;
    }
    default:
        throw new TypeError('객체 서식 미등록 본체');
    }
}
