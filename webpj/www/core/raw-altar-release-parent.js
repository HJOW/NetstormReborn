// 패치 제단 해방 부모(00448bd0)의 현재 진단과 검색 결과/가상 공간/표시 없는 프레임 쓰기 순서를 옮긴 모듈이다(cpppj `o/RawAltarReleaseParent.cpp/.h`, 10.78 전용).
// 순서: 현재 위치/다이스 TYPE 으로 최초 검색한 SID(findDais)를 진단 전에 저장 → 현재 checking 이 비0 이면 방향(direction)을 읽어 12(투명 프레임) 이거나 아니면 두 번째로 읽어 11(희생)이면 원문 진단
//   ('!isInvisibleFrame() && !isSacrifice()', '.\Dais.cpp', 0x578) → 진단이 정상 복귀하면 저장한 검색 결과로 계속 → 현재 실제 풀 범위(valid)에 있으면 기존 해방 몸체(release) →
//   제단 Unpop(flags 0) → 현재 TYPE 의 P 첫 번호를 표시 통지 없이 raw 프레임에 쓰기(plainP) → 제단 Repop(flags 0) 순서이다.
// 진입한 방향 판정은 checking 이 중간에 0 이 되어도 다시 시작하지 않는다. 선택하지 않은 장치는 요구하지 않으며 누락/중단은 앞선 효과를 되돌리지 않는다. 포획/희생 시뮬레이션은 추가하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E15: 제단 해방 부모 이식으로 추가)

// 진단의 원문 식과 파일, 줄 번호이다. (2026-11-08 WB04-E15)
const ALTAR_EXPRESSION = '!isInvisibleFrame() && !isSacrifice()';
const ALTAR_FILE = '.\\Dais.cpp';
const ALTAR_LINE = 0x578;

// 선택한 실제 경계가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E15)
function need(device) {
    if (typeof device !== 'function') throw new Error('제단 해방의 선택된 실제 경계 미연결');
    return device;
}

// altar 는 제단 SID, checking() 은 호출 위치마다 현재 005e4794 DWORD 를 읽는 getter, hooks = { findDais(altar), direction(altar), assertion(식, 파일, 줄), valid(dais), release(dais), unpop(altar, 0), plainP(altar), repop(altar, 0) } 이다. (2026-11-08 WB04-E15)
export function runAltarReleaseParent(altar, checking, hooks) {
    // 진단 전 저장 값과 진단/해방/Unpop 뒤 현재 값의 순서를 분리한다.
    const dais = need(hooks.findDais)(altar);
    if (checking() !== 0 && (need(hooks.direction)(altar) === 12 || need(hooks.direction)(altar) === 11)) need(hooks.assertion)(ALTAR_EXPRESSION, ALTAR_FILE, ALTAR_LINE);
    if (need(hooks.valid)(dais)) need(hooks.release)(dais);
    need(hooks.unpop)(altar, 0);
    need(hooks.plainP)(altar);
    need(hooks.repop)(altar, 0);
}
