// Player 의 9×16 BYTE Core 장부와 별도 Core 10 BYTE 초기화 원문을 옮긴 모듈이다(cpppj `o/RawPlayerCoreRecords.cpp/.h`).
//  - 장부는 실제 00595360 의 9기록(번호 0..8)이며 기록 하나는 16 BYTE: +0 구독자 ID DWORD, +4 Core(DWORD+DWORD+WORD, 10 BYTE), 끝 2 BYTE 는 padding 이다.
//  - initializePlayerCore(0042ce70): 같은 Core 에 DWORD0 → DWORD4 → WORD8 을 0 으로 쓴다. 짧은 물리에서는 앞선 store 만 남기고 10 BYTE 전체를 미리 검사하지 않는다.
//  - initializePlayerCoreRecords(004feac0): 9회 각 기록의 +4 에서 Core 초기화를 먼저 하고 그 뒤 같은 기록의 ID DWORD 를 0 으로 쓴다. padding 2 BYTE 와 인접 lastKnown 은 건드리지 않는다.
//  - writePlayerCoreMetadata(0048f350): owner×16 주소에 ID DWORD → 복제된 Core DWORD/DWORD/WORD 를 쓴다. 논리 owner 상한 관문은 추가하지 않고 주소의 32비트 감김을 보존하며,
//    선택된 store 가 bytes 밖이면 호스트 물리 계약 오류(RangeError)이다.
// records 는 { bytes: Uint8Array, origin } 이다: bytes 는 실제 물리 안의 장부 시작을 빌려 주소 산술을 보존하고(정상 Client 는 144 BYTE), origin 은 bytes 시작에서 장부 첫 주소까지의 원문 32비트 상대 위치이다.
// 근거: cpppj 기준선 1f00a25a, 원본 관측 player-core-records-x86.tsv(192행, tests/player-core-records.test.mjs). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E3: Player Core 장부 이식으로 추가)

// 장부의 기록 수·기록 폭·전체 BYTE 수이다(각 기록 끝 2 BYTE 는 padding). (2026-11-08 WB04-E3)
export const PLAYER_CORE_RECORD_COUNT = 9;
export const PLAYER_CORE_RECORD_STRIDE = 16;
export const PLAYER_CORE_RECORD_BYTES = 144;

// 단일 원문 store 의 전체 물리 폭을 확인한 뒤 같은 폭만 쓴다(낮은 바이트부터). 이웃 필드와 padding 은 보존한다. (2026-11-08 WB04-E3)
function store(bytes, offset, width, value) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('Core 장부의 실제 store 물리 범위');
    for (let byte = 0; byte < width; byte += 1) bytes[offset + byte] = (value >>> (byte * 8)) & 0xff;
}

// 호출 인자로 이미 복제된 10 BYTE 의 DWORD/WORD 를 원문 레지스터 값으로 읽는다. (2026-11-08 WB04-E3)
function value(core, offset, width) {
    let result = 0;
    for (let byte = 0; byte < width; byte += 1) result |= core[offset + byte] << (byte * 8);
    return result >>> 0;
}

// 최초 자식 호출의 주소만 고정하며 그 자식의 각 store 가 짧은 물리를 따로 판정한다. (2026-11-08 WB04-E3)
function at(bytes, offset) {
    if (offset > bytes.length) throw new RangeError('Core 장부 초기화 자식의 실제 시작 물리');
    return bytes.subarray(offset);
}

// 0042ce70: 같은 Core 에 DWORD 0 → DWORD 4 → WORD 8 을 0 으로 쓴다(lastKnown/지역 Core 에도 재사용). 10 BYTE 전체 선행 검사를 추가하지 않는다.
// 사용: initializePlayerCore(core.subarray(0, 10)). (2026-11-08 WB04-E3)
export function initializePlayerCore(core) {
    store(core, 0, 4, 0);
    store(core, 4, 4, 0);
    store(core, 8, 2, 0);
}

// 004feac0: 9회 각 기록의 +4 Core 를 초기화한 뒤 같은 기록의 ID 에 0 을 쓴다(실제 CRT 는 Core 를 먼저 초기화한다). 정상 9회만 수행하며 매 기록 끝 padding 2 는 건드리지 않는다.
// 사용: initializePlayerCoreRecords({ bytes: records, origin: 0 }). (2026-11-08 WB04-E3)
export function initializePlayerCoreRecords(records) {
    let offset = records.origin >>> 0;
    for (let record = 0; record < PLAYER_CORE_RECORD_COUNT; record += 1) {
        initializePlayerCore(at(records.bytes, offset + 4));
        store(records.bytes, offset, 4, 0);
        offset += PLAYER_CORE_RECORD_STRIDE;
    }
}

// 0048f350: owner×16 주소의 ID DWORD 에 subscriber, 이어서 복제된 Core(10 BYTE)의 DWORD/DWORD/WORD 를 쓴다. 입력 Core 는 원문 값 인자와 같은 복제본이며 도중에 다시 읽지 않는다.
// owner 주소는 32비트로 감긴다. 사용: writePlayerCoreMetadata({ bytes, origin: 0 }, 8, 0x12345678, new Uint8Array(10)). (2026-11-08 WB04-E3)
export function writePlayerCoreMetadata(records, owner, subscriber, core) {
    const copy = Uint8Array.from(core);
    const offset = ((records.origin >>> 0) + Math.imul(owner >>> 0, PLAYER_CORE_RECORD_STRIDE)) >>> 0;
    store(records.bytes, offset, 4, subscriber >>> 0);
    store(records.bytes, (offset + 4) >>> 0, 4, value(copy, 0, 4));
    store(records.bytes, (offset + 8) >>> 0, 4, value(copy, 4, 4));
    store(records.bytes, (offset + 12) >>> 0, 2, value(copy, 8, 2));
}
