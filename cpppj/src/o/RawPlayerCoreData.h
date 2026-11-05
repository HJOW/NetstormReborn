// Player.cpp의 현재 CoreData 조회0048f0d0와 선택/정리0048f3b0를 실제 같은 물리에 복원한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/RawPlayerCoreRecords.h"
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
enum class OriginalEdition; // 복원 본문이 선택할 패치/CD Player 폭이며 정의는 RiftType.h가 소유한다.
// 원문 현재 전역을 직접 빌리며 Player/캐시의 별도10BYTE 사본을 만들지 않는다.
struct PlayerCoreDataGlobals {
    std::uint32_t& templateOwner; //00540cac는 선택 인자2를 가장 먼저 받는 별도 Template owner다.
    std::uint32_t& battleOwner; //00540c70은 그 다음 인자1을 받으며 두 owner를 합치지 않는다.
    std::uint32_t& playerKnown; //00595344는 비영 owner 선택 때1이며 owner0에서 임의0쓰기하지 않는다.
    std::uint32_t& active; //00595348은 owner0의 마지막에0이며 비영 선택은 이 값을 보존한다.
    const std::uint32_t& metamap; //00594fc8의 같은 현재 DWORD로 getter 진단/선택 분기를 판정한다.
    std::span<std::uint8_t>& current; //00595350의 현재 포인터 역할이다. 실제 Player+6c/캐시/외부 물리를 빌린다.
    std::span<std::uint8_t> cached; //005953f0의 같은 고정10BYTE이며 조회 때는 주소 동일성도 사용한다.
    const std::span<std::uint8_t>& players; //0059534c의 현재 Player9 물리뷰 슬롯이며 비영 선택 위치에서만 읽는다.
};
// 실제 원문 진단만 외부에 전달한다. 복귀 뒤 현재 포인터를 다시 읽는다.
struct PlayerCoreDataHooks {
    std::function<void(std::uint32_t)> assertion; //Player.cpp85의 _myCoreData == &lastKnownCoreData 진단이다.
};
//0048f0d0:metamap의 실제 포인터 동일성 진단 뒤 현재 포인터를 재조회해 같은 가변뷰를 반환한다.
std::span<std::uint8_t> ReadPlayerCoreData(PlayerCoreDataGlobals globals,const PlayerCoreDataHooks& hooks);
//0048f3b0:두 owner→0이면 이전현재10BYTE를4/4/2로cache→현재NULL/active0, 비영은known1→실제alias다.
//비영 owner의 Player선택은 실제9×180BYTE 안의 유효10BYTE여야 한다. 범위밖 가상주소를 host span으로 가장하지 않는다.
void SelectPlayerCoreData(PlayerCoreDataGlobals globals,std::uint32_t owner,std::uint32_t templateOwner);
// 복원의 선행 missionPost와 후행 savedGame·현재 checking은 독립 논리값으로 축약하지 않는다.
struct PlayerCoreRestoreState {
    RawCheckingFlag missionPost,savedGame,checking; //00594fa4/005c85d4/005e4794의 같은 장기 owner를 소비 위치에서 읽는다.
};
// 선택한 원문 진단과 패치 저장게임의 Chat 재초기화만 실제 외부 효과다.
struct PlayerCoreRestoreHooks {
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단의 표현식/파일/행과 정상 복귀를 보존한다.
    std::function<void()> initializeChat; // PATCH0042e050의 실제 새 manager/명령/Config 구성이다. CD는 호출하지 않는다.
};
//0048fb30/CD00406a90: 현재9기록 Core를 Player에4/4/2로 복원하고 선택된 active/current·저장BYTE를 갱신한다.
// 원문 DWORD 감김과 선택된 load/store의 부분 효과를 보존하며 전체 Player/기록 선행 검사를 추가하지 않는다.
void RestorePlayerCoreData(OriginalEdition edition,PlayerCoreDataGlobals globals,PlayerCoreRecordsView records,
    PlayerCoreRestoreState state,std::uint32_t owner,const PlayerCoreRestoreHooks& hooks);
}
