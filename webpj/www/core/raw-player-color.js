// PATCH 10.78 소유자 색 변경(0043ca70)과 모든 섬 받침/종유석의 프레임 변경을 옮긴 모듈이다(cpppj `o/RawPlayerColor.h`; 프레임 계산은 `o/RawBridgeConnect` 의 OwnerColorFrame).
// 순서: 현재 표(ownerColors)에 새 색을 먼저 쓰고(되돌리지 않음) 원본 iterator 처럼 SID 2 부터(0/1 번은 방문하지 않음) free 슬롯만 건너뛰며 살아 있는 슬롯을 훑는다. 슬롯 타입(+10)이 섬 받침이거나 종유석이면 소유자(+34)의
//   OwnerColorFrame 을 계산해 프레임 소비자(setFrame(sid, frame, 0))에 보낸다. 프레임 효과 뒤 현재 슬롯/표/풀 길이를 다시 읽는다. dead/void/abstract 는 제외하지 않고 다른 소유자의 섬도 현재 색 표로 함께 갱신한다.
// 상태(state): { islandType, stalagType, battle, ownerColors[9] }. 소비자가 없으면 해당 슬롯에서 거부하지만 이미 쓴 색 표는 유지한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E13: 소유자 색 변경 이식으로 추가)

// 전투가 아니거나 소유자 0 이하일 때의 중립 프레임 값이다(RawBridgeConnect 의 kNeutralOwnerFrame). (2026-11-08 WB04-E13)
export const NEUTRAL_OWNER_FRAME = 8;

// 소유자 색 표에서 프레임 번호를 얻는다. 원본은 표 값에서 1 을 뺀다(DWORD 감김 유지). 표 범위 밖 소유자는 거부한다. (2026-11-08 WB04-E13)
export function ownerColorFrame(state, owner) {
    const signed = owner | 0;
    if (!state.battle || signed <= 0) return NEUTRAL_OWNER_FRAME;
    if (signed >= state.ownerColors.length) throw new RangeError('소유자 색 표 범위 오류');
    return ((state.ownerColors[signed] >>> 0) - 1) | 0;
}

// 소유자 색 표를 바꾸고 모든 섬 받침/종유석의 프레임을 요청한다. setFrame(sid, frame, flags) 은 실제 프레임 소비자이다. (2026-11-08 WB04-E13)
export function changePlayerColor(pool, state, owner, color, setFrame) {
    if (pool?.edition !== 'patch1078' || owner >= state.ownerColors.length) throw new RangeError('PATCH 소유자 색 변경 입력 범위 오류');
    // 원본 iterator+4 는 진입 당시 풀 길이를 보존한다.
    const limit = pool.capacity;
    state.ownerColors[owner] = color | 0;
    // iterator 의 초기 1 뒤 0040eb40 이 먼저 증가하므로 0/1 번은 방문하지 않는다.
    let index = 2;
    while (index < limit && (pool.slot(index)[11] & 1)) index += 1;
    // 원본 본문은 현재 풀의 상한을 다시 읽는다.
    while (index >= 1 && index < pool.capacity) {
        const raw = pool.slot(index);
        // 소유자가 다른 섬도 현재 색 표로 함께 갱신한다.
        if (raw[10] === state.islandType || raw[10] === state.stalagType) {
            if (typeof setFrame !== 'function') throw new Error('실제 소유자 색 변경의 프레임 소비자 누락');
            setFrame(index, ownerColorFrame(state, raw[34]), 0);
        }
        index += 1;
        // dead/void/abstract 는 제외하지 않는다.
        while (index < limit && (pool.slot(index)[11] & 1)) index += 1;
    }
}
