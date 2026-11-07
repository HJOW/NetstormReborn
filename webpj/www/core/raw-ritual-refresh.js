// 의식 뒤 로컬 Core/기록/전체 snapshot 의 세 실제 호출(449d98~449dc5)을 원래 순서로 연결하는 모듈이다(cpppj `o/RawRitualRefresh.cpp/.h`, 10.78 전용).
// 순서: core()(0044fc00 기술 bit/다이스 count 갱신) → templateOwner()(00540cac 현재값 읽기) → coreData(owner)(004beb30 현재 Template 에 CoreData 기록) → templateOwner() 한 번 더(원문은 쓰지 않는 값도 다시 계산) → snapshot()(004bfac0).
// 큰 의식/포획 처리를 재현하지 않고 같은 로컬 완료 분기의 세 실제 자식만 실행한다. 호출자는 missionPost/현재 owner 의 기존 상위 조건을 유지한다. 장치 누락은 그 호출 위치에서만 거부하며 앞선 정상 효과는 되돌리지 않는다.
// (2026-11-08 WB04-E11: 의식 뒤 로컬 갱신 이식으로 추가)

// 선택된 장치만 호출 위치에서 요구한다. (2026-11-08 WB04-E11)
function need(device) {
    if (typeof device !== 'function') throw new Error('의식 뒤 로컬 갱신의 실제 선택 장치 미연결');
    return device;
}

// hooks = { core(), templateOwner() → 현재 owner, coreData(owner), snapshot() }. (2026-11-08 WB04-E11)
export function refreshLocalRitualData(hooks) {
    need(hooks.core)();
    const owner = need(hooks.templateOwner)();
    need(hooks.coreData)(owner);
    // 원문은 마지막 cdecl snapshot 앞에서도 사용하지 않는 owner/ECX 를 다시 계산한다.
    need(hooks.templateOwner)();
    need(hooks.snapshot)();
}
