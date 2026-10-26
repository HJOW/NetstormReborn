// 금액/SP/Idle/문자열을 다시 계산하지 않고 실제 예산 Gump의 재부착만 수행한다.
#include "client/RawStormPowerRefresh.h"
#include <stdexcept>

namespace netstorm::client { // 원문 정상 복귀의 store와 중단 prefix를 구별한다.
namespace { // 도달한 순수 노드 조회만 요구한다.
// 같은 등록표의 생존 토큰을 현재 주소로 읽으며 누락된 장치를 해당 조회 위치에서 거부한다.
GumpCreateNode& CurrentNode(const std::function<GumpCreateNode*(std::uint32_t)>& node,std::uint32_t token) {
    if (!node) throw std::logic_error("현재 Gump serial의 실제 주소 조회 누락");auto* value=node(token);if (!value) throw std::out_of_range("현재 Gump serial의 생존 노드 누락");return *value;
}
}
std::uint32_t FindCurrentGumpSerial(std::span<const std::uint32_t> entries,std::uint32_t serial,const std::function<GumpCreateNode*(std::uint32_t)>& node) {
    if (entries.size()<kGumpRegistryCapacity) throw std::invalid_argument("현재 Gump serial의 실제1000슬롯 누락");
    for (std::size_t index=1;index<kGumpRegistryCapacity;++index) { //루트0칸은 생략하고 각 현재 슬롯을 앞에서부터 조회한다.
        const auto token=entries[index];if (!token) continue;const auto& current=CurrentNode(node,token);if (current.serial==serial && !(current.flags&0x10000U)) return token;
    }
    return 0;
}
void RefreshStormPowerGump(std::span<const std::uint32_t> entries,std::uint32_t serial,const StormPowerRefreshHooks& hooks) {
    const auto token=FindCurrentGumpSerial(entries,serial,hooks.node);if (!token) return;
    auto& current=CurrentNode(hooks.node,token);const auto depth=current.depth;
    if (!hooks.push) throw std::logic_error("SP 표시 재부착의 실제 Push 누락");hooks.push(token);
    if (!hooks.attach) throw std::logic_error("SP 표시 재부착의 실제 Attach 누락");hooks.attach(token,current.x,current.y,0,0);
    current.depth=depth; //원문처럼 두 가상 호출의 정상 복귀 뒤에만 한 DWORD를 복원한다.
}
}
