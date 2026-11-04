// 공통004b0d30의현재조회·저장genus·진단복귀·효과순서를원문그대로분배한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 같은물리슬롯을사용하며상위의가상분배나Activate깊이증가를대신하지않는다.
struct SquidPostPopParentHooks { // 도달한실제하위만요구하며전체사전검사를호출하지않는다.
    std::function<bool()> suppressed,graphsEnabled; // 메타맵/현재그래프활성을원문읽기위치에서조회한다.
    std::function<const RiftTypeRecord&(std::uint8_t)> type; // 각rawTYPE조회에같은현재배열의필드를반환한다.
    std::function<void(Sid)> region,add; //00462d40/004633c0의실제공간실행이다.
    std::function<void(Sid)> resetGraph; //004ad3e0의현재invalidGraph조회/진단/쓰기다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //checking무관80b이며정상복귀할수있다.
    std::function<void(Sid)> cost,provider,factory; // 비용→현재공급원→저장genus의전역공장순서다.
    std::function<void(std::uint32_t,Sid)> ownerFactory,aiNotice; //각호출직전현재owner를다시읽는다.
    std::function<std::uint32_t()> localOwner; // AI통지후현재로컬owner와비교한다.
    std::function<void(std::uint32_t)> localCount,globalCount; // 각호출직전현재TYPE BYTE를전달한다.
    std::function<void()> recheckSelection; //004da650의실제선택재검사이며장부다음/깊이감소전이다.
};
// suppressed는raw/TYPE보다먼저읽으며실패한하위앞부분효과와증가된깊이를그대로남긴다.
void RunSquidPostPopParent(const SidPool& pool,Sid sid,std::uint32_t flags,std::uint32_t& depth,const SquidPostPopParentHooks& hooks);
}
