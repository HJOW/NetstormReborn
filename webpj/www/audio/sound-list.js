// Sound.cpp의 이름 표 공개 API만 옮긴다(225553df, 10.78). 실제 소리 장치/재생기는 별도 적응 작업이다.
// 사용: new SoundList(report?).initialize() 뒤 lookup(name)의 숫자 항목을 재생 계층에 넘긴다. 이름은 C++ string_view와 같은 바이트 문자열이다.
// 항목 값은 고정 논리 기준+오프셋이며 실제 메모리 주소/URL이 아니다. DOM·장치·외부 접속은 하지 않는다. (2026-11-16 WB04-E571)

// 원문 표 크기·추가 한도·머리 크기·이름 포인터 보정과 원본 관측의 고정 논리 기준이다.
export const SOUND_LIST_BYTES = 0x8000, SOUND_LIST_LIMIT = 0x7fb0, SOUND_HEADER_BYTES = 0x1c, SOUND_POINTER_BIAS = 0x1f, SOUND_LIST_BASE = 0x15000000;
// 원문 초기 대체 소리 이름의 철자다.
export const FALLBACK_SOUND_NAME = 'nonexistant.wav';
// 각 DWORD 필드의 원문 머리 안 위치다. 재생기/장치가 같은 표를 읽고 쓴다.
export const SOUND_FIELD = Object.freeze({ buffer: 0, root: 4, next: 8, attenuation: 12, volume: 16, serial: 20, priority: 24 });
// C 로캘 ASCII 비교에서만 대문자를 접는다. 상위 바이트는 그대로 둔다.
function fold(byte) { return byte >= 65 && byte <= 90 ? byte + 32 : byte; }
// byte 문자열 두 개를 길이와 ASCII 접기만으로 비교한다.
function sameName(left, right) {
    if (left.length !== right.length) return false;
    // 같은 위치의 바이트가 하나라도 다르면 다음 항목으로 간다.
    for (let index = 0; index < left.length; index += 1) if (fold(left.charCodeAt(index)) !== fold(right.charCodeAt(index))) return false;
    return true;
}
// 원문은 인코딩된 byte 문자열을 받는다. JS의 다중 바이트 문자를 암묵 절단하지 않는다.
function checkName(name) {
    if (typeof name !== 'string' || !name.length || name.includes('\0')) throw new TypeError('소리 이름이 비었거나 NUL을 포함합니다');
    // 각 UTF-16 코드가 입력 바이트 하나인지 검사한다.
    for (let index = 0; index < name.length; index += 1) if (name.charCodeAt(index) > 255) throw new TypeError('소리 이름은 인코딩된 바이트 문자열이어야 합니다');
}

// 원문 bigSoundList 32768바이트와 같은 머리/이름/연결을 장기 소유한다. 항목은 지우지 않는다.
export class SoundList {
    // 표 물리·DWORD 뷰와 assert 보고 함수다.
    #raw = new Uint8Array(SOUND_LIST_BYTES); #view = new DataView(this.#raw.buffer); #report;
    // 다음 오프셋·초기 대체 항목과 한 번만 초기화하는 표식이다.
    #firstFree = 0; #fallback = 0; #initialized = false;
    // 보고가 돌아오면 실행을 계속하며 보고 자체가 던지면 그 접두를 보존한다.
    constructor(report = null) { this.#report = report; }
    // 현재 웹 범위의 원본 판본이다.
    get edition() { return 'patch1078'; }
    // 초기화 표식을 먼저 쓰고 대체 이름을 한 번 조회한다.
    initialize() { if (this.#initialized) return; this.#initialized = true; this.#fallback = this.lookup(FALLBACK_SOUND_NAME); }
    // 표 밖은 빈 이름으로 처리하고 안에서는 NUL 또는 표 끝까지 읽는다.
    #nameAt(offset) {
        let name = '';
        // 이름의 각 바이트를 변환 없이 JS byte 문자열에 붙인다.
        for (let at = offset; at < SOUND_LIST_BYTES && this.#raw[at]; at += 1) name += String.fromCharCode(this.#raw[at]);
        return name;
    }
    // 이름과 NUL을 머리 초기화보다 먼저 쓴다.
    #putName(at, name) {
        // 이름 바이트를 표 안에 순서대로 복사한다.
        for (let index = 0; index < name.length; index += 1) this.#raw[at + SOUND_HEADER_BYTES + index] = name.charCodeAt(index);
        this.#raw[at + SOUND_HEADER_BYTES + name.length] = 0;
    }
    // 숫자 항목의 머리 전체가 표 안에 있는지 확인하고 오프셋을 반환한다.
    #offset(entry) {
        const at = (entry >>> 0) - SOUND_LIST_BASE;
        if (at < 0 || at + SOUND_HEADER_BYTES > SOUND_LIST_BYTES) throw new RangeError('소리 항목 머리가 표 밖입니다');
        return at;
    }
    // null은 즉시0, 기존 이름은 첫 항목, 새 이름은 순회 끝에 추가한다. +10 음량은 쓰지 않는다.
    lookup(name) {
        if (name === null) return 0;
        checkName(name);
        let at = 0;
        // 복제 항목까지 같은 물리 순서로 비교하며 빈 이름에서 멈춘다.
        for (;;) {
            const existing = this.#nameAt(at + SOUND_HEADER_BYTES);
            if (!existing.length) break;
            if (sameName(existing, name)) return SOUND_LIST_BASE + at;
            at += SOUND_HEADER_BYTES + existing.length + 1;
        }
        if (at >= SOUND_LIST_LIMIT) return this.#fallback;
        if (at !== this.#firstFree && this.#report) this.#report('s == firstFreeSound', 0x1c7);
        const next = at + SOUND_HEADER_BYTES + name.length + 1;
        if (next > SOUND_LIST_BYTES) throw new RangeError('소리 이름이 표의 끝을 넘습니다');
        this.#putName(at, name);
        this.#view.setUint32(at, 0, true); this.#view.setUint32(at + 4, SOUND_LIST_BASE + at, true);
        this.#view.setUint32(at + 8, 0, true); this.#view.setUint32(at + 12, 0, true);
        this.#view.setUint32(at + 20, 0, true); this.#view.setUint32(at + 24, 0, true);
        this.#firstFree = next;
        return SOUND_LIST_BASE + at;
    }
    // 표 안 포인터 경로는 표를 읽지 않고0x1f를 뺀다. 결과가 표 시작보다 앞일 수 있다.
    lookupPointer(pointer) { pointer >>>= 0; if (pointer < SOUND_LIST_BASE || pointer - SOUND_LIST_BASE >= SOUND_LIST_BYTES) throw new RangeError('표 안을 가리키는 소리 이름 포인터가 아닙니다'); return (pointer - SOUND_POINTER_BIAS) >>> 0; }
    // 초기 대체 항목이다. initialize 전에는0이다.
    get fallback() { return this.#fallback; }
    // 원문 다음 빈 위치의 논리 항목 값이다.
    get firstFree() { return SOUND_LIST_BASE + this.#firstFree; }
    // 머리 뒤의 이름을 현재 표에서 읽는다. 끝에 이름 바이트조차 없는 항목은 거부한다.
    name(entry) { const at = this.#offset(entry); if (at + SOUND_HEADER_BYTES === SOUND_LIST_BYTES) throw new RangeError('소리 항목 값이 표 밖입니다'); return this.#nameAt(at + SOUND_HEADER_BYTES); }
    // 같은 실제 표의 읽기용 뷰다. C++ const span과 같이 호출자는 수정하지 않는다.
    raw() { return this.#raw; }
    // 전체 머리를 검증한 뒤 지정 DWORD를 읽는다.
    field(entry, field) { return this.#view.getUint32(this.#offset(entry) + field, true); }
    // 같은 표의 지정 DWORD를 쓴다. 값의 의미/실제 버퍼는 이 계층이 해석하지 않는다.
    setField(entry, field, value) { this.#view.setUint32(this.#offset(entry) + field, value >>> 0, true); }
    // 원문 이름→Root→Next→Buffer→앞 Next→빈 위치 순서로 복제한다. 쓰지 않는 머리 바이트는 보존한다.
    appendDuplicate(named, previous, buffer) {
        const name = this.name(named), root = this.field(previous, SOUND_FIELD.root), at = this.#firstFree;
        const next = at + SOUND_HEADER_BYTES + name.length + 1;
        if (next > SOUND_LIST_BYTES) throw new RangeError('복제 소리 항목이 표의 끝을 넘습니다');
        this.#putName(at, name); this.#view.setUint32(at + 4, root, true); this.#view.setUint32(at + 8, 0, true); this.#view.setUint32(at, buffer >>> 0, true);
        this.setField(previous, SOUND_FIELD.next, SOUND_LIST_BASE + at); this.#firstFree = next;
        return SOUND_LIST_BASE + at;
    }
}
