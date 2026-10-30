// 프레임말 Player 대기 결과와 Core 기술 합계의 원문 판단/쓰기 순서를 공유한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>
#include <string_view>
namespace netstorm::o { //실제 Session과 독립 원문 관측이 같은 순수 부모를 사용한다.
struct CoreTechnologyWeightType { //실제 TYPE의 두 signedDWORD만 읽는다.
    std::int32_t bit; //+1ec의 기술 비트 번호다.
    std::int32_t level; //+94의 실제 level-1이며 합산은 원문32비트로 감긴다.
};
struct CoreTechnologyWeightHooks { //각 비트/항목의 현재 개수/타입을 조회한다.
    std::function<std::int32_t()> count; //00541348의 현재 signed 타입 개수다.
    std::function<CoreTechnologyWeightType(std::int32_t)> type; //0049a840의 실제 TYPE 조회이며 같은 비트의 첫 일치만 사용한다.
};
std::uint8_t CoreTechnologyWeight(std::span<const std::uint8_t> core,const CoreTechnologyWeightHooks& hooks); //0042b210의32비트 순회/level+1 합계/마지막 Core+5 BYTE 합산이다.
struct PlayerNotificationHooks { //미선택 하위는 요구하지 않으며 실제 Player 기록을 직접 빌린다.
    std::function<std::uint32_t(std::uint32_t)> global; //known/battle/현재owner/noIsland/nugget의 원래 주소 식별자다.
    std::function<std::span<std::uint8_t>()> players; //현재9×180BYTE의 실제 Player base다. 선택된 owner 기록은 한 번 캡처한다.
    std::function<std::uint32_t()> technology; //0048fc60을 반드시 호출한다. 대기 타입은 이 함수의 두 번째 반환값이 아니다.
    std::function<std::uint8_t(std::span<const std::uint8_t>)> weight; //캡처한 Player+6c의 Core 기술 합계다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> contained; //004acac0(parent,4,pending,0)의 실제 내용물 조회다.
    std::function<void(std::string_view)> tell; //004cf960("AltarUpgrade")의 실제 안내다.
    std::function<void(std::uint32_t,std::uint32_t)> grant; //00448260의 호출 순간 현재 결과/owner를 전달한다.
    std::function<void()> refresh; //00448310의 Core/CoreData/현재 snapshot이다.
};
void NotifyPendingPlayerResult(OriginalEdition edition,const PlayerNotificationHooks& hooks); //00490da0의 Sleep 뒤 대기 결과 확인·BYTE/DWORD 쓰기·부분효과다.
}
