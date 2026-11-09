// 10.78 Xlat(번역표)의 일곱 파일 버퍼와, 같은 원문 키의 언어별 값을 클라이언트 수명에 보관하는 원문 바이트 파서·번역 장부를 옮긴 모듈이다(cpppj `o/RawXlat.cpp/.h`, 10.78 전용: 004de710 파일 읽기 뒤 마지막 BYTE/CR/레코드 병합, 004de9a0 번역 조회, 004dea00 세 파일 적재).
//  - 키와 번역은 원문 바이트 문자열이다: 문자 코드 0~255 하나가 원문 1바이트이다(`xlatBinaryString(bytes)` 가 바이트열을 이 표현으로 바꾼다). UTF-8 변환은 호출자가 수행하며 이 모듈은 인코딩을 바꾸지 않는다.
//  - new RawXlat(): 파일 버퍼 7칸(textBuffer[0..6])과 레코드 장부를 비워 만든다.
//    load(which, bytes): 004de710 의 파일 읽기 뒤 몸체 — 파일 버퍼를 설치하고(이미 있으면 Error: 원문 재적재 진단, 길이 0 이면 RangeError) normalizeFileBuffer 와 parseFileBuffer 를 이어 실행한다.
//    hasBuffer(which): 같은 슬롯의 설치 여부이다(길이 0 설치와 미설치를 구별). allocateFileBuffer(which, length): 실제 File 부모 전용 정확 길이 설치로 영으로 채운 Uint8Array 를 돌려준다(중복 설치는 거부).
//    normalizeFileBuffer(which): 마지막 BYTE 를 NUL 로 쓰고 NUL 전까지의 CR(13)을 하나씩 앞으로 당긴다(이동 뒤 같은 위치를 다시 검사). parseFileBuffer(which): 정규화한 같은 버퍼에서 `*` 키 줄 / `-` 값 줄 / `=` 끝 줄 레코드를 읽어
//    키 앞 BYTE 와 구분자 줄 첫 BYTE 를 NUL 로 쓰고 완성된 레코드만 기존 장부에 병합한다(불완전한 마지막 레코드는 저장하지 않음).
//    translate(original, language): 004de9a0 — 언어 0(영어 우회)·1 은 원문 그대로, 그 밖에는 `original` 의 첫 NUL 앞을 키로 정확 조회하며 없으면 `original` 그대로, 그 언어 칸이 비어 있으면 `original`, 빈 번역은 빈 문자열이다(언어 7 이상/음수는 RangeError).
//    buffer(which): 파일 버퍼를 사본 없이 돌려준다(원문 부분 쓰기 관측용, 미설치는 빈 Uint8Array). size(): 언어 수와 별개인 고유 원문 키 수이다.
//  - initializeRawXlat(xlat, hooks): 004dea00 의 고정 순서 German(3)→English(1)→Portuguese(6)로 d/xlat.* 세 파일을 읽어 적재한다. hooks = { exists(path)(0041ab10 출처 관문, 내용은 읽지 않음), read(path)→Uint8Array(같은 경로의 실제 원문 바이트),
//    missing(path)(없을 때 9번 원문 진단) } 는 도달한 위치에서만 요구한다(없으면 그 지점에서 Error). 일부 파일이 없어도 전체 초기화를 취소하지 않고 다른 언어 파일로 대신하지 않으며 프랑스어/스페인어를 임의로 추가하지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E242: 10.78 다중 번역 장부 이식으로 추가)

// 파일 버퍼 칸 수(일곱 언어 칸)와 레코드 구분자 BYTE(`*` 키·`-` 값·`=` 끝)·CR·LF 이다. (2026-11-10 WB04-E242)
const LANGUAGE_SLOTS = 7;
const CR = 13;
const LF = 10;
const KEY_MARK = 0x2a;
const VALUE_MARK = 0x2d;
const END_MARK = 0x3d;
// 004dea00 의 고정 순서와 경로이다(German 3 → English 1 → Portuguese 6). (2026-11-10 WB04-E242)
const STARTUP_SOURCES = Object.freeze([[3, 'd/xlat.german'], [1, 'd/xlat.english'], [6, 'd/xlat.portuguese']]);

// which 가 7 칸 안의 정수인지 확인한다(원문 `std::array::at` 의 물리 범위 검사). (2026-11-10 WB04-E242)
function slotOf(which) {
    if (!Number.isInteger(which) || which < 0 || which >= LANGUAGE_SLOTS) throw new RangeError('Xlat 파일 버퍼 칸의 물리 범위');
    return which;
}

// start 부터 현재 NUL 의 위치를 물리 범위 안에서 찾는다. (2026-11-10 WB04-E242)
function endOf(bytes, start) {
    if (start >= bytes.length) throw new RangeError('Xlat 실제 문자열 시작 범위');
    const end = bytes.indexOf(0, start);
    if (end < 0) throw new RangeError('Xlat 실제 문자열 NUL 범위');
    return end;
}

// strchr LF 뒤 위치이다. 현재 문자열 안에 LF 가 없으면 null 이다. (2026-11-10 WB04-E242)
function nextLine(bytes, start) {
    const end = endOf(bytes, start);
    const line = bytes.subarray(start, end).indexOf(LF);
    return line < 0 ? null : start + line + 1;
}

// 바이트열을 원문 바이트 문자열(문자 코드 = 바이트 값)로 바꾼다. 브라우저에 Buffer 가 없고 TextDecoder('latin1')은 0x80~0x9f 를 바꾸므로 직접 만든다. (2026-11-10 WB04-E242)
export function xlatBinaryString(bytes) {
    let text = '';
    for (let at = 0; at < bytes.length; at += 4096) text += String.fromCharCode.apply(null, bytes.subarray(at, at + 4096)); // 호출 인자 한계를 피하려고 묶음으로 변환한다.
    return text;
}

// start 부터 첫 NUL 앞까지의 원문 바이트 문자열이다(NUL 뒤는 키/번역에 포함하지 않는다). (2026-11-10 WB04-E242)
function textOf(bytes, start) {
    return xlatBinaryString(bytes.subarray(start, endOf(bytes, start)));
}

// 번역 장부와 일곱 파일 버퍼이다. 보호 파일에는 쓰지 않고 클라이언트 소유의 읽은 버퍼만 수정한다. (2026-11-10 WB04-E242)
export class RawXlat {
    #buffers = new Array(LANGUAGE_SLOTS).fill(null);
    #records = new Map();

    // 004de710 의 파일 읽기 뒤 몸체이다: 버퍼를 설치하고 정규화한 뒤 병합한다. 설치 뒤의 실패는 설치를 되돌리지 않는다.
    load(which, source) {
        slotOf(which);
        if (this.#buffers[which]) throw new Error('Xlat textBuffer[which] == NULL 원문 진단'); // 원문 재적재 진단 뒤 해제된 옛 키 주소의 동작을 정상 경로로 합성하지 않는다.
        if (source.length === 0) throw new RangeError('Xlat 원문 길이0의 마지막BYTE 물리 범위');
        this.#buffers[which] = Uint8Array.from(source);
        this.normalizeFileBuffer(which);
        this.parseFileBuffer(which);
    }

    // 같은 슬롯의 실제 설치 여부이다. 빈 설치도 미설치와 구별한다.
    hasBuffer(which) { return this.#buffers[slotOf(which)] !== null; }

    // 실제 File 부모 전용 정확 길이 설치이다. 이미 설치된 슬롯의 미정 포인터 재사용은 거부한다.
    allocateFileBuffer(which, length) {
        slotOf(which);
        if (this.#buffers[which]) throw new Error('Xlat 기존 버퍼 재사용의 원문 미정 포인터 경계');
        this.#buffers[which] = new Uint8Array(length);
        return this.#buffers[which];
    }

    // 끝 BYTE/NUL·CR 정리는 같은 원래 크기의 버퍼에만 기록한다: 원문은 파일 전체 크기로 memmove 하고 이동 뒤 같은 위치를 다시 검사한다.
    normalizeFileBuffer(which) {
        const bytes = this.#buffers[slotOf(which)];
        if (!bytes || bytes.length === 0) throw new RangeError('Xlat 원문 길이0의 마지막BYTE 물리 범위');
        bytes[bytes.length - 1] = 0;
        for (let at = 0; at < bytes.length && bytes[at];) { // CR 이면 뒤를 한 칸 당기고 같은 위치를 다시 보며, 아니면 다음으로 간다.
            if (bytes[at] === CR) bytes.copyWithin(at, at + 1, bytes.length);
            else at += 1;
        }
    }

    // 정규화한 같은 버퍼를 기존 레코드 장부에 병합한다(File 소멸 뒤 원문 위치).
    parseFileBuffer(which) {
        const bytes = this.#buffers[slotOf(which)];
        if (!bytes || bytes.length === 0) throw new RangeError('Xlat 실제 파싱 버퍼 누락');
        let cursor = 0;
        while (cursor !== null && bytes[cursor]) { // 레코드마다 이전 cursor 부터 새 레코드를 읽는다.
            let state = 0;
            let key = null;
            let value = null;
            let complete = false;
            while (cursor !== null && bytes[cursor] && !complete) { // 구분자는 줄 첫 BYTE 에서만 검사하며 불완전한 마지막 레코드는 저장하지 않는다.
                const at = cursor;
                const next = nextLine(bytes, at);
                const first = bytes[at];
                if (!state && first === KEY_MARK) {
                    key = next;
                    state = 1;
                } else if (state === 1 && first === VALUE_MARK) {
                    if (!at) throw new RangeError('Xlat 원문 키 앞BYTE 쓰기');
                    bytes[at - 1] = 0;
                    bytes[at] = 0;
                    value = next;
                    state = 2;
                } else if (state === 2 && first === END_MARK) {
                    if (!at) throw new RangeError('Xlat 원문 번역 앞BYTE 쓰기');
                    bytes[at - 1] = 0;
                    bytes[at] = 0;
                    complete = true;
                }
                cursor = next;
            }
            if (complete) {
                if (key === null || value === null) throw new RangeError('Xlat 원문 완성 레코드의NULL 주소');
                const name = textOf(bytes, key);
                const row = this.#records.get(name) ?? new Array(LANGUAGE_SLOTS).fill(null);
                row[which] = textOf(bytes, value); // NULL 언어 칸과 빈 번역을 구별하고 다른 언어의 값을 보존한다.
                this.#records.set(name, row);
            }
        }
    }

    // 영어 우회(언어 0·1)/정확 키/NULL 칸 폴백이다. 빈 번역은 등록된 값이므로 원문 대신 빈 문자열을 돌려준다.
    translate(original, language) {
        if (typeof original !== 'string') throw new TypeError('Xlat 번역 키는 원문 바이트 문자열이어야 합니다');
        if (language === 0 || language === 1) return original;
        const nul = original.indexOf('\0');
        const row = this.#records.get(nul < 0 ? original : original.slice(0, nul));
        if (!row) return original;
        if (!Number.isInteger(language) || language < 0 || language >= LANGUAGE_SLOTS) throw new RangeError('Xlat 번역 언어 칸의 물리 범위');
        return row[language] ?? original;
    }

    // 실제 파일 크기를 유지한 NUL/CR 이동 결과를 빌린다(미설치는 빈 배열).
    buffer(which) { return this.#buffers[slotOf(which)] ?? new Uint8Array(0); }

    // 원문 해시 충돌과 무관한 정확 키 개수이다.
    size() { return this.#records.size; }
}

// 004dea00: German(3) → English(1) → Portuguese(6) 순서로 존재 확인·읽기·병합한다. 일부 파일 부재로 전체 초기화를 취소하거나 같은 언어로 다른 파일을 대신 읽지 않는다. (2026-11-10 WB04-E242)
export function initializeRawXlat(xlat, hooks) {
    for (const [which, path] of STARTUP_SOURCES) { // 각 파일의 존재/읽기/병합 뒤 다음 파일로 간다.
        if (typeof hooks?.exists !== 'function') throw new Error('Xlat 실제 파일 존재 장치 미연결');
        if (!hooks.exists(path)) {
            if (typeof hooks.missing !== 'function') throw new Error('Xlat 실제 부재 로그 미연결');
            hooks.missing(path);
            continue;
        }
        if (typeof hooks.read !== 'function') throw new Error('Xlat 실제 파일 읽기 미연결');
        xlat.load(which, hooks.read(path));
    }
}
