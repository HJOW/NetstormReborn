// 원본 Guide의 Tutorial0..6 가상 단계를 같은 현재 미션 상태/세계 소비자에 분배한다.
#pragma once
#include "client/MissionGuideInput.h"

namespace netstorm::client {
// 큰 초기화/Start/프레임과 원본 Tell/UI의 현재 실제 소비자다. 원본 순번을 별도로 복제하지 않는다.
struct TutorialMissionStageHooks {
    std::function<void()> initialize; // 실제004c3290 상당의 설정/월드 설치와 마지막 flags bit1 쓰기다.
    std::function<void(std::string_view,std::uint32_t)> tellGlobal; // 단계2의004cf970("init")이며 원본 두 번째 Tell 인자는1이다.
    std::function<void()> start,frame; // 기존 실제 RawMissionStart/공통 프레임 몸체다. Start가 같은 순번을 증가시킨다.
    std::function<void()> showStage; // 순번 비제로의 First가 현재 단계 안내를 다시 표시한다.
    std::function<std::uint32_t(std::string_view)> tellMission; // 단계5의현재 미션 Success/Failure 섹션 반환 DWORD다.
    std::function<void()> closeTransient,pause; // Tell 성공 뒤 실제 팝업 Kill과 같은 자기 정지다.
};
// 원본 가상표의0..6이다. 초기화/Start/프레임은 큰 실제 몸체에 위임하고 단계2/5/빈6을 직접 복원한다.
void RunTutorialMissionStage(o::MissionStartState& state,std::uint32_t sequence,const TutorialMissionStageHooks& hooks);
// 00463a40: 현재 Guide 미션이 있을 때에만 가상+28의 Pause를 호출한다. Start 가상+10과 구별한다.
void PauseMissionWorldEntry(const std::function<bool()>& current,const std::function<void()>& pause);
}
