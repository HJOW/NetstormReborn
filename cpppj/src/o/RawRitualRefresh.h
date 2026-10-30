// 의식 뒤 로컬 Core/기록/전체 snapshot의 세 실제 호출을 원래 순서로 연결한다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <functional>

namespace netstorm::o { // 단일 플레이와 기존 의식 부모가 같은 실제 저장 경계를 사용한다.
struct RitualRefreshHooks { // 원문449d98~449dc5의 선택된 자식만 공급한다.
    std::function<void()> core; //0044fc00의 현재 기술bit/다이스 count 갱신이다. 화면 갱신으로 대신하지 않는다.
    std::function<std::uint32_t()> templateOwner; //00540cac의 현재값이며 Core와 CoreData 뒤 각각 읽는다.
    std::function<void(std::uint32_t)> coreData; //004beb30의 현재 Template에 대한 실제 CoreData 기록이다.
    std::function<void()> snapshot; //004bfac0의 두 독립 현재 Battle/Fort snapshot이다.
};
//10.78의 로컬 분기 내부다. 호출자는 missionPost/현재 owner의 기존 상위 조건을 유지한다.
void RefreshLocalRitualData(OriginalEdition edition,const RitualRefreshHooks& hooks);
}
