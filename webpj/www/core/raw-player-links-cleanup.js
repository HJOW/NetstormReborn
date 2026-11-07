// State 정리의 0048fcd0 이 같은 9 Player 의 기술 링크·목록 개수·동적 Deck 을 반납하는 몸체를 옮긴 모듈이다(cpppj `o/RawPlayerLinksCleanup.cpp/.h`, 10.78 전용).
// 순서: Player 0..8 각각에서 +0x9c(기술 링크)와 +0x08(개수)을 0 으로 쓰고, +0x68(Deck)이 비0 이면 반납 장치를 호출한 뒤 현재 base 를 다시 읽어 +0x68 을 0 으로 쓴다(0 이면 후속 0 쓰기도 생략).
// 배열/Player 자체는 반납하지 않으며 현재 owner 관문이나 기술 객체 삭제를 추가하지 않는다. 각 원문 load/store 직전에 players() 를 다시 조회하므로 free 가 현재 base 를 바꾸어도 인자와 뒤의 0 쓰기 대상을 구별한다.
// 짧은/NULL 물리는 그 위치에서만 RangeError 이며 앞선 store 는 되돌리지 않는다. (2026-11-08 WB04-E8: Player 링크 정리 이식으로 추가)

// 실제 PATCH 9 Player 의 선택된 세 DWORD 위치와 개수·간격이다. (2026-11-08 WB04-E8)
const PLAYER_COUNT = 9;
const STRIDE = 180;
const TECHNOLOGY = 0x9c;
const COUNT = 8;
const DECK = 0x68;

// 도달한 실제 장치만 그 위치에서 요구한다. (2026-11-08 WB04-E8)
function need(device) {
    if (typeof device !== 'function') throw new Error('Player 링크 정리의 선택된 실제 장치 미연결');
    return device;
}

// 선택 DWORD 의 전체 load 폭을 확인하고 값을 읽는다. (2026-11-08 WB04-E8)
function read(bytes, at) {
    if (at > bytes.length || 4 > bytes.length - at) throw new RangeError('Player 링크 정리의 실제 load 물리 범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 단일 DWORD store 전에 현재 물리를 확인하고 앞선 store 는 복구하지 않는다. 이웃 필드는 보존한다. (2026-11-08 WB04-E8)
function zero(bytes, at) {
    if (at > bytes.length || 4 > bytes.length - at) throw new RangeError('Player 링크 정리의 실제 store 물리 범위');
    // 같은 Player 의 이웃 필드는 건드리지 않고 네 BYTE 만 0 으로 쓴다.
    for (let byte = 0; byte < 4; byte += 1) bytes[at + byte] = 0;
}

// 고정 9기록을 순서대로 처리한다. hooks: { players() → 현재 Player 9×180 물리, release(deckToken) → 같은 실제 Deck 소유자 반납 }. (2026-11-08 WB04-E8)
export function cleanupPlayerLinks(hooks) {
    // 재진입은 별도의 현재 루프 위치를 가지며 같은 현재 물리를 본다.
    for (let owner = 0; owner < PLAYER_COUNT; owner += 1) {
        const offset = owner * STRIDE;
        zero(need(hooks.players)(), offset + TECHNOLOGY);
        zero(need(hooks.players)(), offset + COUNT);
        const deck = read(need(hooks.players)(), offset + DECK);
        if (deck) {
            need(hooks.release)(deck);
            // free 복귀 뒤 현재 base 를 다시 읽어 쓴다.
            zero(need(hooks.players)(), offset + DECK);
        }
    }
}
