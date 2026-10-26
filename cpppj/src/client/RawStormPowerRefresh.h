// 현재 serial의 실제 예산 Gump를 원문 Push/Attach 순서로 재구성한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client { // 숫자 토큰은 동일 공통 등록표의 노드에만 해석한다.
//00464020: 등록1..999의 현재 슬롯에서 serial이 같고 dead가 아닌 첫 Gump를 찾는다.
std::uint32_t FindCurrentGumpSerial(std::span<const std::uint32_t> entries,std::uint32_t serial,const std::function<GumpCreateNode*(std::uint32_t)>& node);
// 실제 가상+18/+20만 호출하는 장치다. node 조회는 같은 현재 저장소의 순수 주소 조회여야 한다.
struct StormPowerRefreshHooks {
    std::function<GumpCreateNode*(std::uint32_t)> node; //같은 실제 노드의 depth와 현재 x/y를 직접 읽는다.
    std::function<void(std::uint32_t)> push; //원문 가상+18로 같은 대상만 분리한다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; //원문 가상+20에 Push 뒤 현재 x/y와0/0을 전달한다.
};
//0043dad0: 현재 serial 조회→깊이 보관→Push→현재 좌표 Attach→정상 복귀 뒤 같은 깊이 복원이다.
void RefreshStormPowerGump(std::span<const std::uint32_t> entries,std::uint32_t serial,const StormPowerRefreshHooks& hooks);
}
