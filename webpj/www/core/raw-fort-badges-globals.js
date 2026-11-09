// Badges 의 원본 28개 기록(6160 BYTE)을 한 배열로 소유하고 로더/조회/저장이 같은 배열을 빌리게 하는 모듈이다(cpppj `o/RawFortBadgesGlobals.cpp/.h`, 10.78 전용).
//  - initializeFortBadgesRecords(records)(004fcbe0): 원본 CRT 가 지정한 6152 BYTE 만 구성한다. 기록마다 220 BYTE 를 0 으로 지운 뒤(첫 기록 +4..+11 의 8 BYTE 는 원본이 쓰지 않으므로 입력 그대로 둔다),
//    앞 27개 기록에 PE 상수의 타입 DWORD 와 15 BYTE 칸 7행의 ASCII 도안(나머지 칸은 0)을 쓰고 28번째 기록은 전체 0 으로 둔다.
//  - resetFortBadgeCounts(records)(0047d860): 28개 기록의 현재 수량 DWORD(+216)만 0 으로 만들고 나머지 6048 BYTE 는 그대로 둔다.
//  - RawFortBadgesGlobals: 한 Client 수명에 BSS 0 → CRT 를 한 번 적용하는 실제 6160 BYTE 소유자이다. 로더/조회/저장은 `records()` 가 돌려주는 같은 배열을 빌리며 복제본을 만들지 않는다.
// 외부 효과가 없는 순수 바이트 규칙이며 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E225: Badges 전역 장부 이식으로 추가)

// 10.78 의 실제 기록 수와 원본 stride 이다. (2026-11-10 WB04-E225)
export const FORT_BADGES_RECORD_COUNT = 28;
export const FORT_BADGES_RECORD_BYTES = 220;
// type/ASCII 도안/padding/count 를 포함한 전체 6160 BYTE 이다. (2026-11-10 WB04-E225)
export const FORT_BADGES_GLOBALS_BYTES = FORT_BADGES_RECORD_COUNT * FORT_BADGES_RECORD_BYTES;
// 각 기록에서 현재 수량 DWORD 가 놓이는 위치이다. (2026-11-10 WB04-E225)
export const FORT_BADGES_COUNT_OFFSET = 216;

// 기록 안의 배치: 타입 DWORD +0, 도안 7행이 +4 부터 15 BYTE 간격이다. 첫 기록의 +4..+11 은 CRT 가 쓰지 않는 칸이다. (2026-11-10 WB04-E225)
const TYPE_OFFSET = 0;
const ROW_BASE = 4;
const ROW_STRIDE = 15;
const UNWRITTEN_FIRST = 4;
const UNWRITTEN_END = 12;

// 원본 CRT 가 각 typed 기록에 배치하는 고정 타입과 앞 일곱 행이다(27개). 각 행은 15 BYTE 칸의 앞쪽에 쓰는 ASCII 이다. (2026-11-10 WB04-E225)
const INITIAL_RECORDS = Object.freeze([
    [71, ['', ' sss   ', '   s   ', '  ss   ', '   s   ', '       ', '       ']],
    [72, ['       ', '       ', '       ', '   s   ', ' s   s ', ' ss ss ', '   s   ']],
    [73, ['       ', '       ', '       ', '   s   ', ' s   s ', ' ss ss ', '   s   ']],
    [78, ['       ', '       ', '   s   ', '  s s  ', ' ss ss ', '       ', '       ']],
    [128, ['       ', '       ', ' ssss  ', '  s s  ', '       ', '  s    ', '       ']],
    [111, ['       ', '  s s  ', '  s s  ', '   s   ', '   s   ', '   s   ', '       ']],
    [136, ['       ', ' s   s ', '       ', '   s   ', '       ', ' s   s ', '       ']],
    [133, ['       ', '     s ', '    s  ', '   s   ', '  s    ', ' s     ', '       ']],
    [140, ['       ', ' w  w  ', 'w    w ', 'w ww w ', ' w  w  ', '  ww   ', '       ']],
    [139, ['       ', ' wwww  ', '  www  ', '   ww  ', '    w  ', ' wwww  ', '       ']],
    [138, ['       ', ' wwww  ', '  www  ', '   ww  ', '    w  ', ' wwww  ', '       ']],
    [75, ['       ', '   w   ', '  w w  ', '   w   ', '   w   ', '   ww  ', '       ']],
    [142, ['       ', ' wwwww ', '   w   ', '  w w  ', ' ww ww ', '       ', '       ']],
    [141, ['       ', '       ', 'wwwwww ', '   w   ', ' ww ww ', '  w w  ', '       ']],
    [137, ['       ', '   w   ', '   ww  ', ' w w w ', ' wwwww ', '       ', '       ']],
    [148, ['       ', '       ', ' t   t ', '  t t  ', ' t t t ', '       ', '       ']],
    [77, ['       ', '  t    ', '  t t  ', '  t t  ', '  t t  ', '   t   ', '       ']],
    [79, ['       ', ' tt tt ', ' tt tt ', ' tt tt ', ' tt tt ', ' tt tt ', '       ']],
    [131, ['       ', '   t   ', '  ttt  ', '   t   ', '  ttt  ', '  ttt  ', '       ']],
    [135, ['       ', '    tt ', ' ttttt ', ' tttt  ', ' tttt  ', ' t  t  ', '       ']],
    [143, ['       ', '  rrr  ', ' r   r ', '   r   ', '   r   ', '       ', '       ']],
    [144, ['       ', '  rrr  ', ' r   r ', '   r   ', '   r   ', '       ', '       ']],
    [76, ['       ', '  rr   ', '   rr  ', '   r   ', '  rr   ', '   r   ', '       ']],
    [147, ['       ', '   r   ', '  rrr  ', '   r   ', '  r r  ', ' rr rr ', '       ']],
    [129, ['       ', '    rr ', ' rr    ', '    rr ', ' rr    ', '    rr ', '       ']],
    [146, ['       ', ' r r r ', '       ', '   r   ', '       ', ' r   r ', '       ']],
    [145, ['       ', ' rr rr ', ' r   r ', ' rr rr ', '  r r  ', '   r   ', '       ']],
]);

// 원본 물리 장부 밖의 접근은 임의 초기화로 대신하지 않고 효과 전에 거부한다. 바이트 배열이 아니면 TypeError, 길이가 6160 이 아니면 RangeError 이다. (2026-11-10 WB04-E225)
function requirePhysical(records) {
    if (!(records instanceof Uint8Array)) throw new TypeError('Badges 장부는 Uint8Array 여야 한다');
    if (records.length !== FORT_BADGES_GLOBALS_BYTES) throw new RangeError('Badges 실제 6160BYTE 장부 누락');
}

// 비정렬 DWORD 네 바이트를 little endian 으로 쓴다. 인접한 원본 자료는 보존한다. (2026-11-10 WB04-E225)
function putDword(records, at, value) {
    for (let byte = 0; byte < 4; byte += 1) records[at + byte] = (value >>> (byte * 8)) & 0xff;
}

// 004fcbe0: CRT 에는 외부 효과/분기가 없으며 지정 영역의 최종 구성과 유일한 미작성 8 BYTE(첫 기록 +4..+11)를 보존한다. (2026-11-10 WB04-E225)
export function initializeFortBadgesRecords(records) {
    requirePhysical(records);
    for (let index = 0; index < FORT_BADGES_RECORD_COUNT; index += 1) { // 마지막 28번째 기록은 원본 55 DWORD 전체 0 구성을 적용한다.
        const at = index * FORT_BADGES_RECORD_BYTES;
        for (let byte = 0; byte < FORT_BADGES_RECORD_BYTES; byte += 1) { // 원본이 쓰지 않는 첫 기록 +4..+11 만 입력 그대로 둔다.
            if (index !== 0 || byte < UNWRITTEN_FIRST || byte >= UNWRITTEN_END) records[at + byte] = 0;
        }
        if (index === INITIAL_RECORDS.length) continue;
        const [type, rows] = INITIAL_RECORDS[index];
        putDword(records, at + TYPE_OFFSET, type);
        for (let row = 0; row < rows.length; row += 1) { // 뒤쪽 칸과 빈 행은 앞선 원본 0 구성으로 남긴다.
            for (let column = 0; column < rows[row].length; column += 1) records[at + ROW_BASE + row * ROW_STRIDE + column] = rows[row].charCodeAt(column);
        }
    }
}

// 0047d860: 현재 수량만 지우며 CRT 자료/타입/첫 기록의 미정 바이트를 다시 구성하지 않는다. (2026-11-10 WB04-E225)
export function resetFortBadgeCounts(records) {
    requirePhysical(records);
    for (let index = 0; index < FORT_BADGES_RECORD_COUNT; index += 1) putDword(records, index * FORT_BADGES_RECORD_BYTES + FORT_BADGES_COUNT_OFFSET, 0); // 원본 28개의 count 주소만 지운다.
}

// 한 Client 수명에 BSS 0 → CRT 를 한 번 적용하는 실제 6160 BYTE 소유자이다. 사용: const globals = new RawFortBadgesGlobals(); globals.records() 를 로더/조회/저장이 같은 배열로 빌린다.
// C++ 의 복사/이동 금지는 JS 에 대응이 없어 장부 배열을 복제하지 않는 것으로 갈음한다(`records()` 는 항상 같은 배열을 돌려준다). (2026-11-10 WB04-E225)
export class RawFortBadgesGlobals {
    // 원본 BSS 처럼 처음 0 인 단일 고정 장부이며 타입/ASCII 자료도 함께 소유한다.
    #records;

    // 새 배열을 최초 0 으로 준비한 뒤 원본 CRT 구성을 한 번 적용한다. (2026-11-10 WB04-E225)
    constructor() {
        this.#records = new Uint8Array(FORT_BADGES_GLOBALS_BYTES);
        initializeFortBadgesRecords(this.#records);
    }

    // 로더/현재 수량 setter/getter/writer 가 복제 없이 빌리는 같은 배열이다. (2026-11-10 WB04-E225)
    records() { return this.#records; }

    // 원본 시작 reset 을 명시 호출하며 전체 CRT 초기화는 반복하지 않는다. (2026-11-10 WB04-E225)
    resetCounts() { resetFortBadgeCounts(this.#records); }
}
