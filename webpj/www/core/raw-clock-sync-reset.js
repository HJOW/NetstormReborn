// 시간 동기화 경로 004c1f20 의 세 DWORD 쓰기만 같은 실제 저장소에 실행하는 모듈이다(cpppj `o/RawClockSyncReset.cpp/.h`, 10.78 전용).
// 순서: connection(005c8bf4 동기화 연결 token) → replyMode(005c8c00 게임 시각 응답 모드) → gameTicks(0055b4ac) 를 차례로 0 으로 쓴다.
// 큰 명령 큐·시계·시간 동기화 배열과 인접 송신·수신 카운터는 초기화하지 않고, 호출은 현재 값을 읽지 않으며 추가 하위 호출·난수·소유자 생성도 하지 않는다.
// globals 는 세 DWORD 를 읽고 쓸 수 있는 속성(connection·replyMode·gameTicks)을 가진 객체이며 이 모듈은 값을 복제하거나 메모리를 소유하지 않는다.
// (2026-11-08 WB04-E11: 시간 동기화 초기화 이식으로 추가)

// 세 DWORD 를 원문 순서대로 0 으로 쓴다. 빌린 저장소는 이 호출보다 오래 살아야 하며 바인딩 시 초기화 효과는 없다. (2026-11-08 WB04-E11)
export function resetClockSynchronization(globals) {
    globals.connection = 0;
    globals.replyMode = 0;
    globals.gameTicks = 0;
}
