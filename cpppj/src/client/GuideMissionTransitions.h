// 원본 Guide의 Begin/Fork/Restart/End 교체 순서와 제어 사건을 현재 미션 수명에 연결한다.
#pragma once
#include "client/MissionGuideInput.h"

namespace netstorm::client {
// 현재 조회·빈 종료 가상 함수·해제/공장/설정은 같은 실제 Client 저장소를 사용한다.
struct GuideMissionTransitionHooks {
    std::function<bool()> current; // 실제 현재 미션 유무다. 해제 재진입 뒤에도 다시 읽는다.
    std::function<void()> finish,release; // 현재 미션 가상+1c와00463ab0의 실제 수명 소비자다.
    std::function<void(std::string_view)> create; // 복사한 Guide 이름으로 빈 새 미션/Script를 만든다. 설치/Start는 Guide 단계다.
    std::function<std::string(std::string_view)> get; // 현재 미션의 fork%d 값을 소유 문자열로 읽는다.
    std::function<void(std::uint32_t)> mode,demo; // Restart가 공장 뒤에 복원하는+64/+68의0/1 DWORD다.
};
// 00463b70: 현재가 있으면 finish/release 한 번 뒤 이름을 복사해 공장을 호출한다.
void BeginGuideMission(std::string_view name,const GuideMissionTransitionHooks& hooks);
// 00463da0: 현재 fork값을 먼저 복사한 뒤 finish/release하고 Begin의 재진입 확인/공장을 수행한다.
void ForkGuideMission(std::uint32_t parameter,const GuideMissionTransitionHooks& hooks);
// 00463cf0: 캡처한 이름/정규화 필드로 release부터 시작하고 공장 뒤 원래 모드/데모를 복원한다.
void RestartGuideMission(const MissionReloadRequest& request,const GuideMissionTransitionHooks& hooks);
// 00463e40의 End: 가상 종료와 해제를 한 번씩 수행하며 공장은 호출하지 않는다.
void EndGuideMission(const GuideMissionTransitionHooks& hooks);
// 명령 분배의 외부 수명과 원본 잘못된 Begin 인자 진단 경계다.
struct GuideMissionCommandHooks {
    std::function<void(std::uint32_t)> begin,fork; // 원본 세 번째 DWORD를 그대로 받는다. Begin은 실제 문자열 수명을 해석한다.
    std::function<void()> restart,end; // 같은 현재 미션의 재시작/종료다.
    std::function<void(std::uint32_t)> invalidBegin; // signed param2<=100의 원본 진단이다. 진단이 반환하면 Begin을 계속한다.
};
// Begin/Restart/Fork/End만 원본 순서와 상위 플래그 제거로 분배한다. 그 외는 false다.
bool DispatchGuideMissionCommand(const InputEvent& event,const GuideMissionCommandHooks& hooks);
}
