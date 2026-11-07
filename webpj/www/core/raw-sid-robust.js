// SID 슬롯의 원시 필드 건전성 검사 두 부모(004ac040 RawSquidIsRobust, 004ac140 RawIsSidRobust)를 옮긴 모듈이다(cpppj `o/RawSidRobust.cpp/.h`, 10.78 전용).
// 진단 전역 두 개(최근 실패 비트 lastRobust=005c8450, 마지막으로 읽은 TYPE type=005c7c48)는 같은 객체 `diagnostics = { lastRobust, type }` 를 읽고 쓴다.
//  - squidIsRobust(object, live, diagnostics, hooks): 진단 type 을 주소 범위 검사보다 먼저 현재 object+10 BYTE 로 쓰고 lastRobust 를 0 으로 쓴 뒤, 같은 pool 기준 하한(base+50)/상한(base+capacity*50)을 검사하고(밖이면 lastRobust=1, false),
//    live 면 가상 표 DWORD 가 0·0xdddddddd·0xcdcdcdcd 인지(2)·상태 bit0(4)·bit1(8)·TYPE 0(0x10) 실패 비트를 순서대로 쓰고, live 가 아니면 표 0(2)·bit0 없음(4)을 쓴다. 실패 수가 0 이면 true.
//  - isSidRobust(sid, live, diagnostics, hooks): signed 1..65534 밖이면 false, live 가 0 이면 pool 을 읽지 않고 true, checking 이 0 이면 true, 아니면 object=base+sid*50 을 검사해 실패 시 assertion('resolve( sid ).isRobust()', 0x218) 후 진단이 정상 복귀하면 true.
// SID 슬롯 번호에는 세대 비트를 더하지 않는다. 주소 폭은 addressMode 로 구별한다: 'original32' 는 DWORD 감김, 'host' 는 실제 호스트 포인터 폭(여기서는 안전 정수)이다. 선택된 하위 장치의 누락은 그 접근에서 거부하며 앞선 진단 쓰기는 되돌리지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E26: SID 건전성 검사 이식으로 추가)

// 선택된 하위 장치가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E26)
function need(device) {
    if (typeof device !== 'function') throw new Error('SID 원문 진단의 선택 장치 미연결');
    return device;
}

// 원문 ADD32(original32)와 호스트 포인터 산술(host)의 폭을 분리한다. (2026-11-08 WB04-E26)
function address(base, delta, mode) {
    if (mode === 'original32') return ((base >>> 0) + (delta >>> 0)) >>> 0;
    const sum = base + delta;
    if (!Number.isSafeInteger(sum)) throw new RangeError('SID 실제 호스트 주소 범위');
    return sum;
}

// 선택 시점의 현재 BYTE 를 리틀엔디언 DWORD 로 읽는다(폭 1 또는 4). 물리가 부족하면 그 접근에서 거부한다. (2026-11-08 WB04-E26)
function read(token, width, hooks) {
    const bytes = need(hooks.memory)(token, width);
    if (!bytes || bytes.length < width) throw new RangeError('SID 원문 진단의 선택 물리 범위');
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[index] * 2 ** (8 * index);
    return value >>> 0;
}

// hooks = { addressMode?('original32' 기본|'host'), poolBase(), capacity(), memory(token, width) → 바이트열, checking(), assertion(식, 줄) } 이다. (2026-11-08 WB04-E26)
export function squidIsRobust(object, live, diagnostics, hooks) {
    const mode = hooks.addressMode ?? 'original32';
    if (mode === 'original32') object >>>= 0;
    let failures = 0;
    diagnostics.lastRobust = 0;
    diagnostics.type = read(address(object, 10, mode), 1, hooks);
    const base = need(hooks.poolBase)();
    if (object < address(base, 50, mode)) { diagnostics.lastRobust = 1; return false; }
    const limit = Math.imul(need(hooks.capacity)(), 50) >>> 0;
    if (object >= address(base, limit, mode)) { diagnostics.lastRobust = 1; return false; }
    if (live) {
        const table = read(object, 4, hooks);
        if (!table || table === 0xdddddddd || table === 0xcdcdcdcd) { diagnostics.lastRobust = 2; failures = 1; }
        if ((read(address(object, 11, mode), 1, hooks) & 1) !== 0) { diagnostics.lastRobust = (diagnostics.lastRobust | 4) >>> 0; failures += 1; }
        if ((read(address(object, 11, mode), 1, hooks) & 2) !== 0) { diagnostics.lastRobust = (diagnostics.lastRobust | 8) >>> 0; failures += 1; }
        if (!read(address(object, 10, mode), 1, hooks)) { diagnostics.lastRobust = (diagnostics.lastRobust | 0x10) >>> 0; failures += 1; }
    } else {
        if (!read(object, 4, hooks)) { diagnostics.lastRobust = 2; failures = 1; }
        if ((read(address(object, 11, mode), 1, hooks) & 1) === 0) { diagnostics.lastRobust = (diagnostics.lastRobust | 4) >>> 0; failures += 1; }
    }
    return failures === 0;
}

// 번호 범위 실패는 false 이고 live 0/checking 0 은 pool 에 접근하지 않는다. 진단 복귀 뒤에는 true 이다. (2026-11-08 WB04-E26)
export function isSidRobust(sid, live, diagnostics, hooks) {
    if (sid < 1 || sid > 65534) return false;
    if (!live) return true;
    if (need(hooks.checking)()) {
        const mode = hooks.addressMode ?? 'original32';
        const object = address(need(hooks.poolBase)(), (sid >>> 0) * 50, mode);
        if (!squidIsRobust(object, 1, diagnostics, hooks)) need(hooks.assertion)('resolve( sid ).isRobust()', 0x218);
    }
    return true;
}
