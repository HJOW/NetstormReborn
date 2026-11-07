// 기술 획득 부모(00492a30, Help Learn 과 로컬 획득이 공유)의 직접 조회/추가와 snapshot·이름/그림·안내의 원문 순서를 옮긴 모듈이다(cpppj `o/RawNewTechnology.cpp/.h`, 10.78 전용).
// 순서: 현재 기술 부모 getter 를 한 번 버리듯 호출 → 다시 읽은 부모로 기술 조회(004acac0(parent, 4, type, 0), 반환은 SID 가 아니라 일치한 종류 WORD) → 없을 때만 부모를 다시 읽어 contained 생성(004afa70(parent, type, 0, 4, 0x10, 0))
//   → 기술이 이미 있어도 생략하지 않는 저장 snapshot(004bfac0) → 현재 TYPE(0049a840)의 이름(0049af80, 첫 NUL 까지만)을 설정 global.newTechName 에 쓴 뒤 그림 서식('<!`"%d.*`">', signed type)을 만들어 global.newTechPic 에 쓰고 → 마지막에 안내 'NewTech'.
// 선택된 하위 장치는 호출 위치에서만 요구하며 누락 시 앞선 부분 효과를 되돌리지 않는다. 문자열은 원본 바이트 그대로의 이진 문자열이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E13: 기술 획득 부모 이식으로 추가)

// 원문 지역 256 BYTE 버퍼에 쓰는 고정 그림 서식이다. (2026-11-08 WB04-E13)
const PICTURE_FORMAT = '<!`"%d.*`">';

// 선택된 경계가 없으면 앞선 부분 효과를 유지하고 중단한다. (2026-11-08 WB04-E13)
function need(device) {
    if (typeof device !== 'function') throw new Error('기술 획득의 실제 선택 장치 미연결');
    return device;
}

// 기존 C 문자열의 첫 NUL 뒤를 Config 값으로 추가하지 않는다. (2026-11-08 WB04-E13)
function text(value) {
    const end = value.indexOf('\0');
    return end < 0 ? value : value.slice(0, end);
}

// hooks = { technologyParent(), find(parent, 4, type, 0), create(parent, type, 0, 4, 0x10, 0), snapshot(), type(type) → 토큰, name(token) → 문자열, configure(key, value), format(pattern, signedType) → 문자열, tell(key) }. (2026-11-08 WB04-E13)
export function acquireNewTechnology(type, hooks) {
    // 첫 getter 의 버린 반환과 조건부 세 번째 getter 까지 원문의 callback 순서를 유지한다.
    need(hooks.technologyParent)();
    const parent = need(hooks.technologyParent)();
    if (!need(hooks.find)(parent, 4, type, 0)) need(hooks.create)(need(hooks.technologyParent)(), type, 0, 4, 0x10, 0);
    need(hooks.snapshot)();
    const record = need(hooks.type)(type);
    const name = need(hooks.name)(record);
    need(hooks.configure)('global.newTechName', text(name));
    const picture = need(hooks.format)(PICTURE_FORMAT, type | 0);
    need(hooks.configure)('global.newTechPic', text(picture));
    need(hooks.tell)('NewTech');
}
