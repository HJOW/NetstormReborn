// 대본/메뉴 GetTechnology 일반 입력(원문 542970, 492a30 기술 지급)의 분배를 옮긴 모듈이다(cpppj `client/ThemeTechnologyCommand.cpp`의 DispatchGetTechnologyInput, 10.78 전용).
// dispatchGetTechnologyInput(event, acquire): 수정키를 지운 코드가 GetTechnology(0x0200009f)가 아니면 false 이다. 뗌이면 현재 Session/효과를 요구하지 않고 true, 누름이면 두 번째 인자 y 의 전체 DWORD(기술 번호를 BYTE/WORD 로 자르지 않음)를 acquire(번호)에 그대로 전달한다(없으면 거부).
// 실제 기술 지급 몸체는 이미 옮긴 `raw-theme-technology.js`(원문 492ae0)이며, 세션(RawCampaignSession)에 연결하는 ApplyThemeTechnology 는 캠페인 세션 단위에서 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E35: GetTechnology 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';
import { RawThemeTechnology } from './raw-theme-technology.js';

// 원문 542970 의 실제 기술 지급 입력 명령 번호와 수정키 제거 마스크이다. (2026-11-08 WB04-E35)
export const GET_TECHNOLOGY_COMMAND = 0x0200009f;
// 보호 명령표 00542798 의 CheatAllTechnology 명령 번호이다. 대본의 `<$CheatAllTechnology,원소번호>` 가 이 명령이 된다(원소 0 바람·1 비·2 천둥·3 태양). (2026-11-22 WB04-E1189)
export const CHEAT_ALL_TECHNOLOGY_COMMAND = 0x02000028;
const KEY_MASK = 0x03ffffff;

// event = { code, x, y }, acquire(기술 번호 DWORD) 는 492a30 의 같은 DWORD 인자 부모이다. 소비하면 true 이다. (2026-11-08 WB04-E35)
export function dispatchGetTechnologyInput(event, acquire) {
    if (((event.code & KEY_MASK) >>> 0) !== GET_TECHNOLOGY_COMMAND) return false;
    if ((event.code & InputCode.release) === 0) {
        if (typeof acquire !== 'function') throw new TypeError('GetTechnology 의 실제 492a30 부모 미연결');
        acquire(event.y >>> 0);
    }
    return true;
}

// 선택한 원소(theme)의 기술을 현재 캠페인 Session 의 기술 부모에 모두 지급한다(cpppj `ApplyThemeTechnology`, 원문 00492ae0 CheatAllTechnology). 사용: applyThemeTechnology(session, 0).
// 매 호출 시점의 실제 타입 표·현재 기술 부모·내용물 검색/생성을 Session 에서 빌리며(게임 상태 사본 없음), 이미 가진 기술은 건너뛰고 group 10·theme 5 는 지급하지 않는다. 생성 뒤 생산 패널 갱신은 호출자가 맡는다.
// 대본 사용처: 캠페인 `bc8fury76`(Fury War) 의 원소 선택 단추(Wind/Rain/Thunder/Sun)와 AI 사제 희생 안내(ai2~ai5priestdead). (2026-11-22 WB04-E1189)
export function applyThemeTechnology(session, theme) {
    if (typeof session?.currentTechnologyParent !== 'function' || typeof session.createContained !== 'function' || typeof session.core !== 'function' || typeof session.priests !== 'function') throw new TypeError('테마 기술 지급의 실제 Session 경계 필요');
    const types = () => session.core().types;
    new RawThemeTechnology({
        typeCount: () => types().length,
        typeAt: (type) => { const record = types()[type]; if (!record) throw new RangeError('현재 기술 타입 기록 범위'); return record; },
        currentTechnology: () => session.currentTechnologyParent(),
        findContained: (parent, mask, type, index) => session.priests().carrier.findContained(parent, mask, type, index),
        createContained: (parent, type, argument, flags, attach, quality) => session.createContained(parent, type, argument, flags, attach, quality),
    }).apply(theme);
}
