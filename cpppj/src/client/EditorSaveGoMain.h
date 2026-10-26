// 원본 SaveGoMain의 저장 선택 부모와 다음 Native State3 예약을 실제 현재 소유자에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // 원문 일반 입력 분기와 현재 Template 저장 선택만 제공한다.
inline constexpr std::uint32_t kSaveGoMainCommand=0x020000cbU; // 보호 PE00542a20의 실제 SaveGoMain 명령 ID다.
// 각 원문 읽기 위치에서 실제 현재 owner/header/모드를 조회한다.
struct CurrentMapSnapshotReads {
    std::function<std::uint32_t()> owner; //00540cac의 현재 Template owner다. 첫 조회값은 Battle 호출까지 유지한다.
    std::function<std::uint32_t(std::uint32_t)> manager; //005c89f0+owner*20의 첫 DWORD다. 최초 존재 관문에만 읽는다.
    std::function<std::uint32_t()> battle; //00594fbc의 현재 전투 모드다.
    std::function<std::uint32_t()> fort; //00594fb8의 현재 Fort 모드다. Battle 복귀 뒤 다시 읽는다.
};
// 큰 실제 snapshot 부모는 상위 Session이 소유하며 선택된 경계만 필요하다.
struct CurrentMapSnapshotHooks {
    std::function<void(std::uint32_t)> battle; //004bf880의 같은 현재 Template 전체 전투 저장이다.
    std::function<void(std::uint32_t)> fort; //004bf9a0의 같은 현재 Template 전체 Fort 저장/Notify다.
};
void SnapshotCurrentMap(const CurrentMapSnapshotReads& reads,const CurrentMapSnapshotHooks& hooks); //004bfb50의 manager 관문·독립 두 if·Battle 뒤 owner 재조회를 그대로 실행한다.
bool DispatchEditorSaveGoMain(const InputEvent& event,std::uint32_t& nextState,const std::function<void()>& saveCurrent); //020000cb의 뗌은 효과 없이 처리하며 y 비영의 실제 저장 정상 복귀 뒤에만 next3을 쓴다. 다른 명령은 false다.
}
