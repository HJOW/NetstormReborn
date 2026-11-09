// 원문 00421f90 의 현재 checking/다리 genus 진단 뒤 지연 낙하 예약 순서를 옮긴 모듈이다(cpppj `o/RawBridgeFallParent.cpp/.h` 의 RunBridgeFallParent, 10.78 전용).
// 순서: 현재 checking DWORD 를 한 번 읽어 비0 이면 같은 현재 부모의 genus(genus(bridge))에 다리 비트(gBRIDGE)가 없을 때 원문 진단('s.getGenus() & gBRIDGE', '\Ns\O\Bridge.cpp', 0x323)을 요청한다.
//   진단이 복귀하더라도 같은 SID 와 원래 float 인자로 기존 00421530 좌표 포장/Regular 생성(schedule)을 호출한다. 선택한 genus 뒤에는 checking 을 다시 검사하지 않는다.
// 추가 free/dead/종류 관문은 넣지 않는다. checking 0 은 genus/진단 장치를 미리 요구하지 않는다.
//  - scheduleBridgeFallChecked(host, pool, types, bridge, x, y, checking, assertion): 실제 SquidProcessHost 에 연결한 진단 부모(cpppj ScheduleBridgeFallChecked, 10.78 전용). host 의 풀이 pool 과 같아야 하고(다르면 TypeError),
//    genus 는 pool 슬롯 +10 의 현재 TYPE 번호로 types 의 flags2 를 읽으며(표 밖이면 RangeError), 예약은 기존 scheduleBridgeFall(00421530 좌표 포장/Regular 생성)이다. 진단 실패는 등록 전에 멈춘다.
//    assertion 이 없으면 진단 조건/파일/줄을 담은 오류를 던지고(실제 진단 UI 가 연결되지 않은 호스트는 선택한 조건만 명시 실패), 있으면 복귀를 허용해 같은 SID 와 원래 좌표로 예약까지 이어진다. 반환은 만든 Regular 프로세스이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E15: 다리 낙하 부모 이식으로 추가, 2026-11-10 WB04-E244: 실제 Host 연결 scheduleBridgeFallChecked 추가)

import { scheduleBridgeFall } from './squid-process.js';

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

// 진단 콜백이 없는 호스트의 기본 진단이다: 원문 조건/파일/줄을 담아 등록 전에 명시 실패한다(cpppj 기본 assertion 의 logic_error). (2026-11-10 WB04-E244)
function defaultAssertion(condition, source, line) {
    throw new Error(`원문 다리 낙하 진단 ${source}:${line}: ${condition}`);
}

// 실제 SquidProcessHost/SidPool/현재 TYPE 표에 연결한 진단 부모이다. host.pool 이 pool 과 다르면 TypeError, 부모 슬롯의 TYPE 번호가 표 밖이면 RangeError 이다.
// checking 은 현재 DWORD getter(() → 부호 없는 수), assertion 은 선택 진단 콜백(식, 파일, 줄)이다. 반환은 scheduleBridgeFall 이 등록한 Regular 프로세스이다. (2026-11-10 WB04-E244)
export function scheduleBridgeFallChecked(host, pool, types, bridge, x, y, checking, assertion = null) {
    if (host.pool !== pool) throw new TypeError('다리 낙하 부모의 실제 Host/Pool 불일치');
    let result = null;
    runBridgeFallParent(bridge, x, y, checking, {
        // 같은 현재 부모의 TYPE 번호로 현재 flags2 를 읽는다. 표 밖이면 원문 물리 범위 밖 읽기를 오류로 바꾼다.
        genus: (sid) => {
            const type = pool.slot(sid)[10];
            if (type >= types.length) throw new RangeError('다리 낙하 진단의 현재 TYPE 물리 범위');
            return types[type].flags2;
        },
        schedule: (sid, px, py) => { result = scheduleBridgeFall(host, sid, px, py); },
        assertion: assertion ?? defaultAssertion,
    });
    return result;
}
