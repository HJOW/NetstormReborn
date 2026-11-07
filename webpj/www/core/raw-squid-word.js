// 공통 Squid 의 두 작은 setter 를 옮긴 모듈이다: 표면 WORD(패치 004ac220)와 현재 HP(패치 004adca0). 정확한 저장 폭과 이웃 필드 보존이 계약이다.
//  - setRawSquidWord(004ac220): 표면 WORD(+12)를 낮은 BYTE 부터 먼저 저장하고, 그 객체의 현재 TYPE 이 현재 다리 TYPE 이면 현재 다리 후속 통지(notify)를 부른다. 통지 부재는 앞선 저장을
//    되돌리지 않고 그 위치에서 오류이다. 패치판은 free/contained 여부로 실제 메모리 쓰기를 미리 거부하지 않는다(물리 슬롯).
//  - setRawSquidHitPoints(004adca0): 현재 HP(+26)에 패치판 DWORD(4바이트)만 쓴다. 옆 필드는 보존하며 공간/상태/회복/표시 효과를 추가하지 않는다
//    (건설/Rune 같이 공통 setter 를 직접 부르는 곳이 쓰고, 사제의 가상 HP 전환 효과는 호출자가 기존 몸체를 연결한다).
// 근거: cpppj `o/RawSquidWord.h`, `o/RawSquidHitPoints.cpp/.h`(기준선 1f00a25a). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E2: 공통 작은 setter 이식으로 추가)

// 표면 WORD(+12)·TYPE(+10)·현재 HP(+26) 위치와 HP 폭(10.78 은 DWORD)이다. (2026-11-08 WB04-E2)
const WORD = 12, TYPE = 10, HIT_POINTS = 26, HIT_POINT_WIDTH = 4;

// 004ac220: 표면 WORD 를 저장하고 현재 다리 TYPE 이면 통지한다. bridgeType 은 호출 시점의 현재 다리 TYPE DWORD, notify(sid) 는 선택된 표면 통지이다.
// 사용: setRawSquidWord(pool, sid, 0x1234, 82, (target) => building.notifySurface(target)). (2026-11-08 WB04-E2)
export function setRawSquidWord(pool, sid, word, bridgeType, notify) {
    if (pool?.edition !== 'patch1078') throw new TypeError('공통 WORD setter는 10.78 전용');
    const raw = pool.physical(sid);
    raw[WORD] = word & 0xff;
    raw[WORD + 1] = (word >>> 8) & 0xff;
    // 현재 DWORD 와 TYPE BYTE 를 비교한 뒤 callable 수명을 지킨다.
    if (raw[TYPE] === (bridgeType >>> 0)) {
        const selected = notify;
        if (typeof selected !== 'function') throw new Error('공통 WORD setter의 선택된 표면 통지 미연결');
        selected(sid);
    }
}

// 004adca0: 현재 HP(+26)에 DWORD 를 낮은 바이트부터 쓴다(할당된 슬롯만). value 는 signed 32비트 정수이다.
// 사용: setRawSquidHitPoints(pool, sid, 91). (2026-11-08 WB04-E2)
export function setRawSquidHitPoints(pool, sid, value) {
    if (pool?.edition !== 'patch1078') throw new TypeError('공통 HP setter는 10.78 전용');
    const raw = pool.allocated(sid);
    const bits = value >>> 0;
    // 낮은 바이트부터 판본의 HP 폭까지만 쓴다.
    for (let index = 0; index < HIT_POINT_WIDTH; index += 1) raw[HIT_POINTS + index] = (bits >>> (8 * index)) & 0xff;
}
