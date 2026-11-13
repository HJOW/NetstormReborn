// 스펠의 월드/파편 표시에서만 적용하는 임시 도움말 그림 제외 규칙을 옮긴 모듈이다(cpppj `client/SpellFrameVisibility.h`, 10.78 전용).
// 원본 10.78 도 Edit 비석→스펠 전환 뒤 도움말 그림을 깜빡인다. 2026-10-28 사용자 요청에 따라 cpppj 가 원본과 다르게 그 그림만 임시로 숨기며, 웹도 같은 규칙을 따른다.
// 프레임 진행·예약·효과·삭제는 원래대로 유지하고, 도움말 창과 게임 규칙은 바꾸지 않는다. 호출자는 월드 그리기(GameWorld)·공장 추가 그림(RawCampaignFactoryDisplay)·파편 캡처(RawCampaignSession)이다.
//  - suppressSpellHelpFrame(type, special, frame): 폭탄 종류(TypeFlag2.bomb) 타입이고 도움말 프레임이 있으며(≥0) 지금 그릴 프레임이 바로 그 도움말 프레임일 때만 true(그리기를 건너뛴다).
// 10.78 판본 인자(`edition == Patch1078`)는 웹이 10.78 만 다뤄 옮기지 않았다(CD 10.72 는 범위 밖). type 은 `flags2` 를 가진 타입 기록, special 은 `helpFrame` 을 가진 특수 프레임 기록이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E381: 5-A 시각 효과 소단위로 추가, 기준선 뒤 cpppj 변경 없음)
import { TypeFlag2 } from '../assets/type-flags.js';

// 이 스펠 그림이 도움말 전용 프레임이라 월드/파편 표시에서 숨겨야 하면 true 이다. frame 은 그릴 프레임 번호이며 정수 비교만 한다(부호 있는 64비트 값). (2026-11-13 WB04-E381)
export function suppressSpellHelpFrame(type, special, frame) {
    return (type.flags2 & TypeFlag2.bomb) !== 0 && special.helpFrame >= 0 && frame === special.helpFrame;
}
