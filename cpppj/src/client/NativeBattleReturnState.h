// 일반 전투 복귀 State6의 결과 배경·정리 전 지역값과 Fort 뒤 옵션/결과 안내를 복원한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client { // 원문 큰 Fort 부모의 앞뒤만 조립하고 공통144BYTE 확정은 기존 분배기에 남긴다.
struct NativeBattleReturnGlobals {
    o::LocalMissionStateStorage state; // 실제 현재6/이전phase14와 같은 다섯 블록이다.
    std::uint32_t& sourceType;std::uint32_t& outcome; //0055a420/005c85ac의 현재 DWORD이며 보관 지역값과 구별한다.
    const bool& battle;std::span<char> background; //00594fbc와 현재 배경 NUL 문자열의 실제 물리다.
};
struct NativeBattleReturnSnapshot {
    std::uint32_t outcome{},source{}; // cleanup 전에 보관한 결과/출처이며 없는 출처 배경은 이 지역 source만0으로 바꾼다.
};
struct NativeBattleReturnHooks {
    std::function<void()> deleteSavedState,leaveBattle,prefix; // 선택된 삭제·이전 Battle14 실제52h 통지·공통 Fort prefix다.
    std::function<void(std::string_view)> requestMusic; // 승패/출처가 요청하는 현재 음악 장치다.
    std::function<std::optional<std::string>(std::uint32_t)> sourceBackground; // 현재 TYPE의 .gif 존재를 판정하고 기존 이름만 돌려준다.
    std::function<o::LocalMissionParentResult(const NativeBattleReturnSnapshot&)> installFort; // 실제 파일/Player/월드/덱/후반 UI를 같은 부모에서 이어가며 미션 조기 반환도 보존한다.
};
struct NativeBattleReturnTailHooks {
    std::function<void()> multiplayerOptions; //004ccf30의 실제 Multiplayer Options 공장이다. 인터넷 검증/공식 서버 접속은 연결하지 않는다.
    std::function<void(std::string_view)> tell; // 현재 BattleOver%d 섹션의 실제 Tell 장치다.
};
o::LocalMissionParentResult InstallNativeBattleReturnState(NativeBattleReturnGlobals globals,const NativeBattleReturnHooks& hooks); //004b6dd0 case6의 정리 전 캡처→배경/음악→이전 전투 통지→공통 prefix→실제 Fort 연속 실행이다.
void CompleteNativeBattleReturn(const NativeBattleReturnSnapshot& snapshot,std::uint32_t& outcome,const NativeBattleReturnTailHooks& hooks); // 공통 HUD1/주변음기한/ser22 뒤 결과0이면 옵션, 결과 비0이면 조건부 안내 뒤 현재 결과0을 쓴다.
}
