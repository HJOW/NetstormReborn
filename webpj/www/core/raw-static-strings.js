// Staticstring.cpp 의 임시 문자열 공유 고리(004b92b0 초기화·004b9320 예약·004b9390 서식 외곽)를 같은 네 DWORD(005c8940/44/48/4c)에 실행하는 모듈이다(cpppj `o/RawStaticStrings.cpp/.h`, 10.78 전용).
//  - initializeStaticStrings(size): 이미 base 가 있으면 원문 진단(Staticstring.cpp 17)을 요청 → size 를 쓰고 새 할당(allocate(size)) → 현재 size 를 다시 읽어 base 에 할당 주소를 쓰고
//    DWORD 단위(size>>2 번)와 나머지 BYTE(size&3 번)로 0 을 채움 → current=base 에 0xd5 표식 BYTE → current+1. last 는 이 부모가 쓰지 않는다.
//  - reserveStaticString(size): base 가 없으면 8192 로 먼저 초기화 → end=size+base(32비트 감김), start=current. start+size+1 이 end 이상(unsigned)이면 current=base, base 에 0xd5, start=current+1
//    → last=start, current=start+size 에 0xd5, current+1 → last 반환. 예약 폭은 출력 절단 한도가 아니다.
//  - formatStaticString(size, pattern): 예약 결과가 pattern 주소와 같으면 CRT 서식을 호출하지 않고, 다르면 format(결과, pattern) 호출. CRT 가 last/current 를 바꿔도 캡처한 반환 주소를 유지한다.
// Native 주소는 장치 토큰(32비트 수)이며 호스트 포인터로 절단하지 않는다. 각 하위 호출 뒤 원문 위치에서 전역을 다시 읽고, 한 store 이전의 부분 효과를 유지한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E18: 임시 문자열 고리 이식으로 추가)

// 이미 base 가 있을 때의 진단 줄 번호, 표식 BYTE, lazy 초기 크기이다. (2026-11-08 WB04-E18)
const REINIT_LINE = 17;
const MARKER = 0xd5;
const LAZY_SIZE = 8192;

// 도달한 장치만 요구한다. (2026-11-08 WB04-E18)
function need(device) {
    if (typeof device !== 'function') throw new Error('임시 문자열 실제 장치 미연결');
    return device;
}

// 한 DWORD/BYTE 전체 폭을 확인한 뒤 해당 바이트만 작성한다. (2026-11-08 WB04-E18)
function store(hooks, address, width, value) {
    const bytes = need(hooks.memory)(address >>> 0, width);
    if (bytes.length < width) throw new RangeError('임시 문자열 원문 store 물리 범위');
    bytes.fill(value, 0, width);
}

// globals = { size, base, current, last }(읽고 쓰는 u32 속성), hooks = { allocate(n) → 주소, memory(주소, 폭) → 같은 물리 뷰, assertion(줄), format(목적, 서식) }. (2026-11-08 WB04-E18)
export function initializeStaticStrings(size, globals, hooks) {
    if (globals.base) need(hooks.assertion)(REINIT_LINE);
    globals.size = size >>> 0;
    const allocated = need(hooks.allocate)(size >>> 0) >>> 0;
    // 원문은 new 복귀 뒤 현재 size 를 읽고 반환 주소를 base 에 쓴다.
    const count = globals.size >>> 0;
    globals.base = allocated;
    let destination = allocated;
    // rep stosd 의 전체 폭/부분 효과이다.
    for (let word = 0; word < (count >>> 2); word += 1, destination = (destination + 4) >>> 0) store(hooks, destination, 4, 0);
    // 마지막 나머지만 BYTE 로 쓴다.
    for (let byte = 0; byte < (count & 3); byte += 1, destination = (destination + 1) >>> 0) store(hooks, destination, 1, 0);
    const base = globals.base >>> 0;
    globals.current = base;
    store(hooks, base, 1, MARKER);
    globals.current = ((globals.current >>> 0) + 1) >>> 0;
}

// 예약한 문자열의 시작 주소를 돌려준다. (2026-11-08 WB04-E18)
export function reserveStaticString(size, globals, hooks) {
    let base = globals.base >>> 0;
    if (!base) {
        initializeStaticStrings(LAZY_SIZE, globals, hooks);
        base = globals.base >>> 0;
    }
    // 두 주소 합은 원문 DWORD 로 감긴다.
    const end = ((globals.size >>> 0) + base) >>> 0;
    let start = globals.current >>> 0;
    if ((((start + (size >>> 0) + 1) >>> 0)) >= end) {
        globals.current = base;
        store(hooks, base, 1, MARKER);
        start = ((globals.current >>> 0) + 1) >>> 0;
    }
    globals.last = start;
    globals.current = (start + (size >>> 0)) >>> 0;
    store(hooks, globals.current, 1, MARKER);
    globals.current = ((globals.current >>> 0) + 1) >>> 0;
    return globals.last >>> 0;
}

// 예약 결과와 서식 주소가 같으면 CRT 서식을 부르지 않는다. (2026-11-08 WB04-E18)
export function formatStaticString(size, pattern, globals, hooks) {
    const result = reserveStaticString(size, globals, hooks);
    if (result !== (pattern >>> 0)) need(hooks.format)(result, pattern >>> 0);
    return result;
}
