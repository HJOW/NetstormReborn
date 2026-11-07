// 원문 00421f90 의 현재 checking/다리 genus 진단 뒤 지연 낙하 예약 순서를 옮긴 모듈이다(cpppj `o/RawBridgeFallParent.cpp/.h` 의 RunBridgeFallParent, 10.78 전용).
// 순서: 현재 checking DWORD 를 한 번 읽어 비0 이면 같은 현재 부모의 genus(genus(bridge))에 다리 비트(gBRIDGE)가 없을 때 원문 진단('s.getGenus() & gBRIDGE', '\Ns\O\Bridge.cpp', 0x323)을 요청한다.
//   진단이 복귀하더라도 같은 SID 와 원래 float 인자로 기존 00421530 좌표 포장/Regular 생성(schedule)을 호출한다. 선택한 genus 뒤에는 checking 을 다시 검사하지 않는다.
// 추가 free/dead/종류 관문은 넣지 않는다. checking 0 은 genus/진단 장치를 미리 요구하지 않는다. 실제 Host 연결(ScheduleBridgeFallChecked)은 SquidProcessHost 의 낙하 예약 단위에서 잇는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E15: 다리 낙하 부모 이식으로 추가)

// 진단의 원문 식과 파일, 줄 번호, 다리 genus 비트(flags2 0x4)이다. (2026-11-08 WB04-E15)
const BRIDGE_EXPRESSION = 's.getGenus() & gBRIDGE';
const BRIDGE_FILE = '\\Ns\\O\\Bridge.cpp';
const BRIDGE_LINE = 0x323;
const BRIDGE_GENUS = 0x4;

// 선택된 진단/현재 타입/예약 경계가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E15)
function need(device) {
    if (typeof device !== 'function') throw new Error('다리 낙하 부모의 선택된 실제 장치 미연결');
    return device;
}

// bridge 는 다리 SID, x/y 는 float 인자, checking() 은 현재 DWORD getter, hooks = { genus(sid) → flags2, schedule(sid, x, y), assertion(식, 파일, 줄) }. (2026-11-08 WB04-E15)
export function runBridgeFallParent(bridge, x, y, checking, hooks) {
    if (checking() !== 0 && !((need(hooks.genus)(bridge) >>> 0) & BRIDGE_GENUS)) need(hooks.assertion)(BRIDGE_EXPRESSION, BRIDGE_FILE, BRIDGE_LINE);
    need(hooks.schedule)(bridge, x, y);
}
