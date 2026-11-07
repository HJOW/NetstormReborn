// PATCH 10.78 의 Player 전체 초기화(004910a0)를 원문 순서로 옮긴 모듈이다(cpppj `o/RawPlayerInitialization.cpp/.h`).
// 순서: 현재 Player base 를 먼저 캡처 → 두 카운터(005951f8 로그인 Fort 수, 005951fc 마지막 로그인 Player)를 0 으로 쓰기 → 이전 base 가 비어 있지 않으면
//   Player 0..8 의 +0 공장 토큰을 각 행마다 현재 base 를 다시 읽어 아홉 번 해제(NULL 도 생략하지 않음) → 현재 base 로 Player 영역 반납 → base 슬롯에 NULL 쓰기(해제 callback 이 base 를 바꿔도 이 위치에서 다시 0)
//   → 1620 BYTE 할당(반환 주소를 슬롯과 REP 대상에 각각 사용) → 405 DWORD 를 0 으로 쓰기 → 각 owner 행: +0x64=0, +0x58=0, 120 BYTE 공장 할당(할당 내용은 초기화하지 않음, NULL 반환도 그대로 기록),
//   +0 공장 토큰·+4=30·+8=0·+0x2c=0·+0x78=127, 자기 동맹 mask(1<<owner)를 +0x54 에 쓰고 현재 base 에서 다시 읽어 방향별 동맹 몸체(00490ea0)에 전달.
// Core 선택/known0/PlayerLinks/Kernel 정리는 호출자의 앞선 원문 단계이며 여기서 암묵 실행하지 않는다. 이 모듈은 앞선 부분 효과를 되돌리지 않고 DOM 도 쓰지 않는다.
// 메모리 접근은 hooks.memory(주소, 폭)가 돌려주는 같은 실제 물리 뷰만 쓰며 NULL·짧은 물리·누락 장치는 도달한 위치에서만 예외로 거부한다. 10.78 전용이다.
// (2026-11-08 WB04-E7: Player 전체 초기화 이식으로 추가)

// 실제 Player 0..8 의 기록 수와 BYTE 간격이다. (2026-11-08 WB04-E7)
export const INITIALIZED_PLAYER_COUNT = 9;
export const INITIALIZED_PLAYER_STRIDE = 180;

// malloc Player 영역 전체 폭(1620)과 각 operator new 공장 30칸의 원문 크기(120)이다. (2026-11-08 WB04-E7)
export const INITIALIZED_PLAYER_BYTES = 1620;
export const INITIALIZED_FACTORY_BYTES = 120;

// 도달한 실제 장치만 필요로 한다. 선택하지 않은 free/동맹 경계를 미리 실행하지 않는다. (2026-11-08 WB04-E7)
function need(device) {
    if (typeof device !== 'function') throw new Error('Player 전체 초기화의 선택된 실제 장치 미연결');
    return device;
}

// 요청한 4 BYTE 전체를 확인한 뒤 원문 DWORD 를 읽는다. 부분 load 를 새 값으로 합치지 않는다. (2026-11-08 WB04-E7)
function read(hooks, address) {
    const bytes = need(hooks.memory)(address >>> 0, 4);
    if (bytes.length !== 4) throw new RangeError('Player 초기화의 실제 DWORD load 물리 부족');
    return (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)) >>> 0;
}

// 단일 DWORD store 의 호스트 물리를 먼저 확인하며 이전 store 의 결과는 그대로 보존한다. (2026-11-08 WB04-E7)
function write(hooks, address, value) {
    const bytes = need(hooks.memory)(address >>> 0, 4);
    if (bytes.length !== 4) throw new RangeError('Player 초기화의 실제 DWORD store 물리 부족');
    // 현재 물리 네 BYTE 만 쓴다.
    for (let byte = 0; byte < 4; byte += 1) bytes[byte] = (value >>> (8 * byte)) & 0xff;
}

// Player 기록 9개를 전체 초기화한다. 32비트 주소 계산은 원문처럼 감긴다.
//  - globals: { fortCount, lastLoginPlayer (쓰기 가능한 값 속성), playerBase() (각 원문 load 위치에서 현재 주소 조회), writePlayerBase(주소) (같은 현재 슬롯에 직접 기록) }
//  - hooks: { memory(주소, 폭) → 같은 물리 뷰, freeFactory(토큰), freePlayers(주소), mallocPlayers(폭) → 주소/NULL, newFactory(폭) → 주소, alliance(owner, mask) }
// (2026-11-08 WB04-E7)
export function initializePlayerRecords(globals, hooks) {
    // 4910a0 은 두 카운터 쓰기보다 먼저 첫 base/NULL 관문을 캡처한다.
    const previous = need(globals.playerBase)() >>> 0;
    globals.fortCount = 0;
    globals.lastLoginPlayer = 0;
    if (previous !== 0) {
        // free 복귀 뒤 다음 행은 현재 전역 base 를 다시 읽는다.
        for (let owner = 0; owner < INITIALIZED_PLAYER_COUNT; owner += 1) {
            const token = read(hooks, ((need(globals.playerBase)() >>> 0) + owner * INITIALIZED_PLAYER_STRIDE) >>> 0);
            // 원문은 NULL 공장도 CRT free 호출을 생략하지 않는다.
            need(hooks.freeFactory)(token);
        }
        need(hooks.freePlayers)(need(globals.playerBase)() >>> 0);
        // free callback 의 base 변경이 있어도 원문이 이 위치에서 NULL 을 다시 쓴다.
        need(globals.writePlayerBase)(0);
    }
    // 같은 malloc 반환을 REP 의 EDI 와 전역에 각각 사용한다.
    const allocated = need(hooks.mallocPlayers)(INITIALIZED_PLAYER_BYTES) >>> 0;
    need(globals.writePlayerBase)(allocated);
    // REP 의 405 DWORD 는 캡처한 반환 주소를 순서대로 쓴다.
    for (let word = 0; word < INITIALIZED_PLAYER_BYTES / 4; word += 1) write(hooks, (allocated + word * 4) >>> 0, 0);
    // 각 행의 원문 전역 load 위치와 순서를 보존한다.
    for (let owner = 0; owner < INITIALIZED_PLAYER_COUNT; owner += 1) {
        const offset = owner * INITIALIZED_PLAYER_STRIDE;
        write(hooks, ((need(globals.playerBase)() >>> 0) + offset + 0x64) >>> 0, 0);
        write(hooks, ((need(globals.playerBase)() >>> 0) + offset + 0x58) >>> 0, 0);
        // new 호출 전에 EDI 에 캡처한 실제 행 주소다.
        const record = ((need(globals.playerBase)() >>> 0) + offset) >>> 0;
        // NULL 반환도 부모 자체는 진단/추가 초기화하지 않는다.
        const factory = need(hooks.newFactory)(INITIALIZED_FACTORY_BYTES) >>> 0;
        write(hooks, record, factory);
        write(hooks, (record + 4) >>> 0, 30);
        write(hooks, (record + 8) >>> 0, 0);
        write(hooks, (record + 0x2c) >>> 0, 0);
        write(hooks, (record + 0x78) >>> 0, 127);
        // 원문 owner 0..8 의 자기 동맹 bit 이다.
        const mask = (1 << owner) >>> 0;
        write(hooks, ((need(globals.playerBase)() >>> 0) + offset + 0x54) >>> 0, mask);
        // 동맹 인자도 new 전에 캡처한 record 가 아니라 현재 base 에서 읽는다.
        const currentMask = read(hooks, ((need(globals.playerBase)() >>> 0) + offset + 0x54) >>> 0);
        need(hooks.alliance)(owner, currentMask);
    }
}
