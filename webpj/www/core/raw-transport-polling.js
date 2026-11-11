// 여러 실제 전송 owner 를 원문 하나의 현재 ID 순회로 묶을 때 쓰는 작은 결과/조회 도구를 옮긴 모듈이다(cpppj `o/RawTransportPolling.h`, 헤더만).
// 사용: const result = transportPollResult(consumed, again); const token = currentTransportToken(registry, id);
//   실제 노드와 표는 연결 등록소(`raw-connection-registry.js`)가 계속 소유하며 이 도구는 현재 값을 읽기만 한다.
// 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E310: 4-C RawTransportPolling 이식)

// 한 번의 조회 결과를 만든다: consumed 는 한 완성 프레임을 실제 입력에서 꺼냈는지, again 은 같은 현재 정상 ID 를 다시 조회해 읽어야 하는지이다.
// 무효 프레임의 소비와 길이 0 sentinel 의 반복 중단을 구별하기 위한 값이다. (2026-11-11 WB04-E310)
export function transportPollResult(consumed = false, again = false) {
    return { consumed: Boolean(consumed), again: Boolean(again) };
}

// 원문 -31..-1 pending 과 0..21 정상 칸의 **현재** 실제 토큰만 읽는다(칸이 비어 있으면 0). 범위 밖 ID 는 오류이다. (2026-11-11 WB04-E310)
export function currentTransportToken(registry, id) {
    const table = registry.cleanup();
    if (!Number.isInteger(id) || id <= -table.pending.length || id >= table.transceivers.length) throw new RangeError('원문 현재 전송 ID 범위 오류');
    return id < 0 ? table.pending[-id] : table.transceivers[id];
}
