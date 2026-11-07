// 원본 10.78 공통 Activate(004ac560)를 옮긴 모듈이다. 저장된 flags/상태와 진단 뒤의 현재 서버·깊이를 구별해 다음 순서를 따른다.
//  유효성 진단(isValid 0x35b) → void 진단(isVoid 0x35c) → void 비트를 지우고 전송 직후 비트가 있으면 진단(0x35f) → 현재 서버 판정:
//    서버이면 client 번호(5..14999)는 서버 Pop 비트(8) 진단(0x364) 후 flags|0x10, 그 밖에는 0x10 이 없으면 flags|8, flags 에 0x50 비트가 없으면
//    (flags&7 이면 전송중 비트 0x10) 현재 가상 writer 로 송신하고 정상 복귀 뒤에만 전송중 비트를 지운다. 서버가 아니면 8 비트가 없을 때 flags|0x10.
//  → 깊이를 올리고 현재 가상 +20 후처리(postPop)를 부른 뒤 깊이가 달라졌으면 진단(curPostPop == basePostPop, 0x392).
// 앞선 void/송신/깊이 변경은 진단 또는 하위 실패가 발생해도 되돌리지 않는다. 진단은 정상 복귀하면 그 위치 다음부터 계속한다.
// 근거: cpppj `o/RawSquidActivate.cpp/.h`(기준선 1f00a25a). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E1: 공통 Pop 이식으로 추가)

// 원문 진단 파일 이름이다. (2026-11-08 WB04-E1)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';

// 선택된 실제 장치를 꺼낸다. 없으면 그 위치에서 오류이며 앞선 쓰기는 남는다. (2026-11-08 WB04-E1)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`Activate의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 004ac560 전체 부모를 실행한다. depth 는 공통 postPop 깊이 장부({ value })이고 hooks 는 { server() → boolean, transmit(sid, flags), postPop(sid, flags), assertion(expression, file, line) } 이다.
// 사용: runSquidActivate(pool, sid, flags, { value: 0 }, hooks). (2026-11-08 WB04-E1)
export function runSquidActivate(pool, sid, flags, depth, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('현재 Activate는 10.78 전용');
    let current = flags >>> 0;
    const diagnose = (expression, line) => need(hooks.assertion, 'assertion')(expression, SQUID_CPP, line);
    if (!sid || sid >= pool.capacity) diagnose('isValid()', 0x35b);
    const raw = pool.physical(sid);
    if (!(raw[11] & 4)) diagnose('isVoid()', 0x35c);
    const state = raw[11];
    raw[11] &= 0xfb;
    if (state & 0x10) diagnose('!xmitJustCreatedFlag', 0x35f);
    if (need(hooks.server, 'server')()) {
        if (sid >= 5 && sid < 15000) {
            if (current & 8) diagnose('!(popFlags & pfSERVER_POP)', 0x364);
            current = (current | 0x10) >>> 0;
        } else if (!(current & 0x10)) current = (current | 8) >>> 0;
        if (!(current & 0x50)) {
            if (current & 7) raw[11] |= 0x10;
            need(hooks.transmit, 'transmit')(sid, current);
            // 하위 정상 반환 뒤에만 전송중 비트를 지운다.
            raw[11] &= 0xef;
        }
    } else if (!(current & 8)) current = (current | 0x10) >>> 0;
    const before = depth.value;
    depth.value = (depth.value + 1) >>> 0;
    need(hooks.postPop, 'postPop')(sid, current);
    if (depth.value !== before) diagnose('curPostPop == basePostPop', 0x392);
}
