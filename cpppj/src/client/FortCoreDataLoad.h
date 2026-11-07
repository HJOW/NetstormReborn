// World 없는 메타 진입과 기존 Session이 같은 실제 CoreData 읽기 몸체를 사용한다.
#pragma once
#include "client/CampaignTemplateStorage.h"
#include "client/ClientCoreDataGlobals.h"
#include <functional>

namespace netstorm::client { // 현재 Client 물리/장치만 빌리며 별도 임시 World를 만들지 않는다.
struct FortCoreDataLoadContext { //004beca0의 원문 전역과 도달 위치별 실제 장치다.
    const bool& missionPost;const std::uint32_t& editor;const bool& metamap; //두 자연 관문과 현재 Core getter 모드를 서로 구별한다.
    o::PlayerWorldState& player;ClientCoreDataGlobals& core; //현재 두 owner/known과 실제 active/current/cache 저장소다.
    std::uint32_t& loadingDepth; //005c89b8의 같은 Client 중첩 DWORD다.
    std::function<std::uint32_t&()> seed; //005c89b4의 lowWORD 읽기가 선택될 때만 실제 DWORD를 빌린다.
    std::function<void(std::uint32_t)> depthChanged; //선택적 실제 World의 기존 깊이 소비자 동기화다. World가 없는 부모는 같은 DWORD만 직접 쓴다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Template/Player/MemStream 원문 표현식·경로·행 진단이며 정상 복귀하면 계속한다.
    std::function<void()> refreshRank; //0042f540의 실제 Chat 랭크 prefix 갱신이다.
    std::function<void()> invalidData; //004bea70 불일치의 현재00540cb8→005318ec 저장이다. Core+4=1 뒤 실행한다.
};
void LoadFortCoreData(CampaignTemplateStorage& storage,std::uint32_t slot,bool minimum,FortCoreDataLoadContext context); //004beca0의 같은 stream/10BYTE·최소 병합·seed/세 무결성 블록을 읽는다. 진단 전의 부분 쓰기를 보존한다.
}
