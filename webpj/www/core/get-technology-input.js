// 대본/메뉴 GetTechnology 일반 입력(원문 542970, 492a30 기술 지급)의 분배를 옮긴 모듈이다(cpppj `client/ThemeTechnologyCommand.cpp`의 DispatchGetTechnologyInput, 10.78 전용).
// dispatchGetTechnologyInput(event, acquire): 수정키를 지운 코드가 GetTechnology(0x0200009f)가 아니면 false 이다. 뗌이면 현재 Session/효과를 요구하지 않고 true, 누름이면 두 번째 인자 y 의 전체 DWORD(기술 번호를 BYTE/WORD 로 자르지 않음)를 acquire(번호)에 그대로 전달한다(없으면 거부).
// 실제 기술 지급 몸체는 이미 옮긴 `raw-theme-technology.js`(원문 492ae0)이며, 세션(RawCampaignSession)에 연결하는 ApplyThemeTechnology 는 캠페인 세션 단위에서 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E35: GetTechnology 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';

// 원문 542970 의 실제 기술 지급 입력 명령 번호와 수정키 제거 마스크이다. (2026-11-08 WB04-E35)
export const GET_TECHNOLOGY_COMMAND = 0x0200009f;
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
