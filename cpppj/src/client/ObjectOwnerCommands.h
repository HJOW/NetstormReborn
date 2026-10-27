// 일반 입력의 SetOwner와 편집 대상의 현재 Player Core 선택을 복원한다.
#pragma once
#include "client/InputEvent.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::client { // UI 명령과 현재 월드의 가상 소유자 변경 사이의 경계다.
inline constexpr std::uint32_t kSetObjectOwnerCommand=0x020000ccU; // 보호542a24와 SetOwner 스크립트가 공유하는 실제 명령이다.
// 원문 각 조회 시점의 현재 전역과 선택된 실제 하위만 공급한다.
struct ObjectOwnerCommandHooks {
    std::function<std::uint32_t()> editor,missionPost,selected; //5c85a4/594fa4/5caea0의 현재 DWORD다.
    std::function<void(std::uint32_t,std::uint32_t)> owner; // 선택 시 보관한 SID의 가상74이며 새 owner 전체DWORD를 받는다.
    std::function<void(std::uint32_t)> follow; // owner 변경 뒤 현재 Editor가 참이면 보관 SID로 실행하는4d5290이다.
};
// 원문4d817e..4d81cb의 모드 단락·보관SID·뒤늦은 Editor 조회·revision 감김이다.
bool DispatchObjectOwner(const InputEvent& event,std::uint32_t& revision,const ObjectOwnerCommandHooks& hooks);
//4d5290의 현재 owner BYTE→SelectPlayerCoreData(owner,1)→revision이며 owner0은 무동작이다.
void FollowEditorObjectOwner(const o::SidPool& pool,std::uint32_t sid,std::uint32_t& revision,const std::function<void(std::uint32_t,std::uint32_t)>& selectCore);
}
