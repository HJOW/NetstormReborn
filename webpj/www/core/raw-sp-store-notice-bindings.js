// Player 내용물 통지의 SP 접두가 읽는 네 전역을 같은 실제 SP 저장소의 현재 조회에 연결하는 모듈이다(cpppj `o/RawSpStoreNoticeBindings.cpp/.h`, 10.78 전용).
// 네 읽기는 값을 보관하지 않고 호출할 때마다 같은 저장소의 원본 필드를 조회한다: cursorA(00568af8, 실제 Get/Set/Rotate 가 쓰는 offset)·cursorB(0054d4e0, 외부 사제 상태가 소유한 같은 추적 offset의 signed→DWORD 비트)·
// firstSpPool(실제 첫 분산 배열 주소가 NULL 이 아닌지 1/0, 포인터 토큰을 발명하지 않음)·firstPermutation(실제 Get/Set/Rotate 가 공유하는 순열 배열의 첫 signed WORD).
// store 와 bindTrackingOffset 에 연결한 외부 상태는 반환 조회기/통지 모듈보다 오래 살아야 한다. 첫 초기화 전에 추적 offset 을 연결하지 않았으면 사본으로 대체하지 않고 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E215: 생산 Deck 이식과 함께 추가)

// 네 조회기를 만든다. store 는 ScrambledSpStore 이다. 반환 객체는 RawPlayerContentNotice 의 현재 전역 getter 네 개(cursorA·cursorB·firstSpPool·firstPermutation)를 채운다. (2026-11-10 WB04-E215)
export function bindSpStoreNotice(store) {
    // 미연결 추적 전역은 첫 바인딩에서 거부한다.
    void store.trackingOffset;
    return {
        cursorA: () => store.offset >>> 0,
        cursorB: () => store.trackingOffset >>> 0,
        firstSpPool: () => (store.firstPoolReady ? 1 : 0),
        firstPermutation: () => store.permutation()[0],
    };
}
