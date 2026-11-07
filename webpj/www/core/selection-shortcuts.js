// 원본 P/R 키의 사제 선택(004d9884..004d98e1)과 N 키의 다음 이동 유닛 선택(004da880)을 옮긴 모듈이다(cpppj `client/PriestSelectionShortcut.cpp/.h`, `client/NextUnitSelection.cpp/.h`, 10.78 전용).
//  - selectPriestByKey(state, hooks): 조회 전에 현재 선택 SID 를 캡처한다 → localOwner 로 살아 있는 사제/운반체/제단 참여 조회(find) → 후보가 캡처한 선택과 다르면 반복 카운터를 0 으로 → 같고 카운터가 비0 이면 현재 localOwner·viewCommand 를
//    읽어 일반 명령 큐에 (명령, owner, owner)로 예약하고 끝 → 그 밖에는 select(후보, 0, 1) 후 현재 카운터를 하나 증가(32비트 감김). 카운터는 Client 수명의 원문 005caf40 DWORD 이며 bool 이나 프레임별 상태가 아니다.
//  - selectNextMovingUnit(pool, types, state, add, hooks): 현재 선택이 free 면 0 으로 보고, 소유자(+34, 캡처한 전체 DWORD)와 이동 가능 TYPE(flags2 0x30000)을 만족하는 다음 슬롯을 예약 1 다음부터 free 만 건너뛰며 한 번 순환 검색한다.
//    없으면 반환, 있으면 select(후보, 0, 1) → center(후보)(첫 선택 성공 여부와 무관) → 정지면 끝 → Shift(add)가 비0 이면 후보가 이동 가능하고 현재 선택이 살아 있는 이동 유닛일 때 multiple(후보, 1) 후 끝, 그 밖에는 select(후보, 0, 1) 한 번 더.
//    dead 나 내용물 상태를 추가로 제외하지 않으며 넓은 DWORD SID 를 WORD 로 먼저 자르지 않는다. 풀 범위 밖은 RangeError 이다.
// 선택된 장치가 없으면 그 위치에서 거부하며 선행 카운터 쓰기와 선택 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E22: 사제/다음 유닛 선택 키 이식으로 추가)

// 선택된 장치를 호출 위치에서만 요구한다. (2026-11-08 WB04-E22)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// state = { selected, repeated }(현재 선택 DWORD 와 반복 카운터, 읽고 쓰는 속성), hooks = { localOwner(), viewCommand(), find(owner), select(sid, 0, 1), queue(command, first, second) }. (2026-11-08 WB04-E22)
export function selectPriestByKey(state, hooks) {
    const message = '사제 선택 단축키 장치 미연결';
    const owner = need(hooks.localOwner, message)();
    // 조회가 현재 selected 를 바꿔도 비교는 조회 전에 캡처한 SID 이다.
    const previous = state.selected >>> 0;
    const candidate = need(hooks.find, message)(owner) >>> 0;
    if (candidate !== previous) state.repeated = 0;
    else if (state.repeated) {
        const currentOwner = need(hooks.localOwner, message)();
        const command = need(hooks.viewCommand, message)();
        need(hooks.queue, message)(command, currentOwner, currentOwner);
        return;
    }
    need(hooks.select, message)(candidate, 0, 1);
    state.repeated = ((state.repeated >>> 0) + 1) >>> 0;
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표({ flags2 }), state = { selected }, add 는 Shift 값, hooks = { localOwner(), paused(), select, center(sid), multiple(sid, 1) }. (2026-11-08 WB04-E22)
export function selectNextMovingUnit(pool, types, state, add, hooks) {
    if (pool?.edition !== 'patch1078' || types.length !== 188) throw new TypeError('다음 유닛 선택의 10.78 풀/TYPE 오류');
    const message = '다음 유닛 선택 장치 미연결';
    // 넓은 DWORD 를 WORD 로 줄이기 전에 풀 범위를 확인한다.
    const raw = (sid) => {
        if (!sid || sid >= pool.capacity || sid > 65535) throw new RangeError('다음 유닛 선택의 물리 SID 범위');
        return pool.slot(sid);
    };
    // 현재 TYPE 을 호출 시점마다 다시 읽는다.
    const mobile = (sid) => {
        const type = raw(sid)[10];
        if (type >= types.length) throw new RangeError('다음 유닛 선택의 TYPE 범위');
        return ((types[type].flags2 >>> 0) & 0x30000) !== 0;
    };
    let previous = state.selected >>> 0;
    if (previous && (raw(previous)[11] & 1)) previous = 0;
    const owner = need(hooks.localOwner, message)() >>> 0;
    // 원문 iterator 의 상한은 검색을 시작할 때 저장한다.
    const end = pool.capacity;
    // 0040eb40 과 같은 선행 증가/free 건너뛰기이다.
    const advance = (start) => {
        let sid = start + 1;
        while (sid < end && (raw(sid)[11] & 1)) sid += 1;
        return sid;
    };
    // 소유자 BYTE 와 캡처한 전체 DWORD 를 비교하고 실패 시에만 다음 슬롯으로 간다.
    const find = (start) => {
        let sid = start;
        while (sid && sid < pool.capacity) {
            if (raw(sid)[34] === owner && mobile(sid)) return sid;
            sid = advance(sid);
        }
        return 0;
    };
    let candidate = advance(1);
    if (previous) candidate = advance(previous);
    candidate = find(candidate);
    if (!candidate) candidate = find(advance(1));
    if (!candidate) return;
    need(hooks.select, message)(candidate, 0, 1);
    need(hooks.center, message)(candidate);
    if (need(hooks.paused, message)()) return;
    if (add) {
        if (!mobile(candidate)) return;
        const selected = state.selected >>> 0;
        if (selected && !(raw(selected)[11] & 1) && mobile(selected)) {
            need(hooks.multiple, message)(candidate, 1);
            return;
        }
    }
    need(hooks.select, message)(candidate, 0, 1);
}
