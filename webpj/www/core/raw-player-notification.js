// 프레임 끝 Player 대기 결과(00490da0)와 Core 기술 합계(0042b210)의 원문 판단/쓰기 순서를 옮긴 모듈이다(cpppj `o/RawPlayerNotification.cpp/.h`, 10.78 전용).
// 포획/희생 자체는 실행하지 않고, 이미 기록된 Player+0xa0 대기 타입을 소비하는 부모만 둔다. 미선택 하위(안내·획득·갱신 등)는 hooks 로 받으며 도달한 위치에서만 요구한다.
//  - coreTechnologyWeight: Core 기술 마스크(+6)의 32개 비트를 낮은 번호부터 보고, 켜진 비트마다 현재 개수만큼 TYPE 을 조회해 비트가 같은 첫 TYPE 의 level+1 을 더한다(32비트 감김). 마지막에 Core+5 BYTE 를 합해 BYTE 로 돌려준다.
//    마스크·Core+5 는 TYPE 조회 뒤에도 현재 물리에서 다시 읽는다.
//  - notifyPendingPlayerResult: known·battle 전역이 모두 참이고 현재 owner 기록에 대기 타입(+0xa0)과 기술 링크(+0x9c)가 있으면 기술 getter(0048fc60)를 부른 뒤 대기 타입에 따라
//    2/3 은 제단 승급(+0x71 최대화, 증가하면 'AltarUpgrade' 안내), 93 은 Core 합계 43 이면 획득 후 +0x71=0, 154 는 +0x71=0 뿐, 그 밖은 4 마스크 내용물 조회 결과가 0 이면 획득한다. 마지막에 +0xa0 만 0 으로 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E8: Player 알림 이식으로 추가)

// 선택된 장치만 요구하며 미선택 효과를 빈 성공으로 대신하지 않는다. (2026-11-08 WB04-E8)
function need(device) {
    if (typeof device !== 'function') throw new Error('Player 알림의 선택된 실제 하위 미연결');
    return device;
}

// 정렬되지 않은 DWORD 를 읽는다. 물리가 4 BYTE 미만이면 RangeError. (2026-11-08 WB04-E8)
function read(bytes, offset) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('Player 알림의 실제 DWORD 물리 부족');
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// 앞선 안내/획득/갱신이 끝난 뒤 캡처한 같은 기록의 +0xa0 만 지운다(이웃 기록과 Core BYTE 는 보존). (2026-11-08 WB04-E8)
function clearPending(record) {
    for (let byte = 0; byte < 4; byte += 1) record[0xa0 + byte] = 0;
}

// 0042b210: hooks = { count() → 현재 signed TYPE 개수, type(index) → { bit, level } }. 반환은 BYTE 합계이다. (2026-11-08 WB04-E8)
export function coreTechnologyWeight(core, hooks) {
    if (core.length < 10) throw new RangeError('Core 기술 합계의 실제 10BYTE 누락');
    let total = 0;
    // 원래 32개 기술 비트를 낮은 번호부터 순회한다.
    for (let bit = 0; bit < 32; bit += 1) {
        if (!(read(core, 6) & (1 << bit))) continue;
        // 불일치 뒤 현재 개수를 다시 조회하며 같은 비트의 첫 일치에서 끝난다.
        for (let index = 0; index < (need(hooks?.count)() | 0); index += 1) {
            const type = need(hooks.type)(index);
            if ((type.bit | 0) === bit) {
                total = (total + 1 + (type.level >>> 0)) >>> 0;
                break;
            }
        }
    }
    return (total + core[5]) & 0xff;
}

// 00490da0: hooks = { global(주소), players(), technology(), weight(core10), contained(parent,4,pending,0), tell(text), grant(pending,owner), refresh() }. 부분 효과는 되돌리지 않는다. (2026-11-08 WB04-E8)
export function notifyPendingPlayerResult(hooks) {
    // known(595344)과 battle(594fbc)이 모두 참이어야 한다.
    if (!need(hooks.global)(0x595344) || !hooks.global(0x594fbc)) return;
    const owner = hooks.global(0x540c70) >>> 0;
    const players = need(hooks.players)();
    const offset = owner * 180;
    if (offset > players.length || players.length - offset < 180) throw new RangeError('Player 알림의 현재 owner 기록 부족');
    const record = players.subarray(offset, offset + 180);
    const pending = read(record, 0xa0);
    if (!pending || !read(record, 0x9c)) return;
    const technology = need(hooks.technology)();
    if (pending === 2 || pending === 3) {
        // 제단 승급: +0x71 은 대기 타입-1 과 기존 값 중 큰 값이며 증가했을 때만 안내하고 끝낸다.
        const previous = record[0x71];
        const next = pending - 1;
        record[0x71] = Math.max(next, previous) & 0xff;
        if (previous < record[0x71]) {
            need(hooks.tell)('AltarUpgrade');
            clearPending(record);
            return;
        }
    } else {
        let grant = false;
        if (pending === (hooks.global(0x5411cc) >>> 0)) {
            grant = need(hooks.weight)(record.subarray(0x6c, 0x6c + 10)) === 43;
            record[0x71] = 0;
        } else if (pending === (hooks.global(0x5412c0) >>> 0)) {
            record[0x71] = 0;
            clearPending(record);
            return;
        } else grant = (need(hooks.contained)(technology, 4, pending, 0) >>> 0) === 0;
        if (grant) {
            // 호출 순간의 현재 결과/owner 를 전달한다.
            const currentOwner = hooks.global(0x540c70) >>> 0;
            const currentPending = read(record, 0xa0);
            need(hooks.grant)(currentPending, currentOwner);
            need(hooks.refresh)();
        }
    }
    clearPending(record);
}
