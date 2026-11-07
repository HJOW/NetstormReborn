// DataManager 초기화(0044d870)의 root 검사→이름표 1회 조회→이름별 실제 Create→마지막 빈 이름 반환 제어를 옮긴 모듈이다(cpppj `o/RawDataManagerInitialize.cpp/.h`, 10.78 전용).
// manager 는 고정 this 의 raw 바이트(+4 DWORD = root)이다. root 가 비0 이면 원문 진단(Datamanager.cpp 0xc9)을 요청한 뒤 가상 +4 의 현재 이름표(names())를 정확히 한 번 얻는다. 표 포인터는 고정하되 이후 이름의 내용은
//   각 create 뒤 같은 표에서 다시 읽는다. index 0 부터 selectSnartName(Snart.cpp 0x46 진단 포함)으로 현재 이름을 선택하고, 빈 이름이면 반환, 아니면 create(이름 시작 뷰)를 호출한다(실제 0044d1b0 에 고정 manager 를 연결).
// 초기화 앞/중간의 외부 효과를 되돌리거나 표를 사전 문자열 목록으로 복사하지 않으며, 진단이 owner/표를 바꿔도 그 뒤 위치에서 조회한다. 도달한 경계만 요구한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E19: DataManager 초기화 이식으로 추가)
import { selectSnartName } from './raw-snart.js';

// 실제 Datamanager.cpp/Snart.cpp 진단 번호이다. (2026-11-08 WB04-E19)
export const DATA_MANAGER_INITIALIZE_ASSERTION = Object.freeze({ rootNotEmpty: 0xc9, nameSeparator: 0x46 });

// 도달한 경계만 요구한다. (2026-11-08 WB04-E19)
function need(device) {
    if (typeof device !== 'function') throw new Error('DataManager 초기화의 선택된 실제 하위 미연결');
    return device;
}

// 현재 고정 this 의 +4 DWORD 이다. (2026-11-08 WB04-E19)
function root(manager) {
    if (manager.length < 8) throw new RangeError('DataManager 초기화의 실제 root 물리 범위');
    return (manager[4] | (manager[5] << 8) | (manager[6] << 16) | (manager[7] << 24)) >>> 0;
}

// hooks = { assertion(번호), names() → 현재 이름표 뷰, create(이름 시작 뷰) }. (2026-11-08 WB04-E19)
export function initializeDataManager(manager, hooks) {
    if (root(manager)) need(hooks.assertion)(DATA_MANAGER_INITIALIZE_ASSERTION.rootNotEmpty);
    const names = need(hooks.names)();
    // 반환표 포인터는 고정하되 미래 이름의 내용은 각 Create 뒤 같은 표에서 다시 읽는다.
    for (let index = 0; ; index += 1) {
        const name = selectSnartName(names, index, () => need(hooks.assertion)(DATA_MANAGER_INITIALIZE_ASSERTION.nameSeparator));
        if (!name[0]) return;
        need(hooks.create)(name);
    }
}
