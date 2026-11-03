// 진단 앞의 읽기와 복귀 뒤 예약을 원문 위치에서 수행한다.
#include "o/RawBridgeFallParent.h"
#include "o/SquidProcess.h"
#include <stdexcept>
#include <string>

namespace netstorm::o { // 같은 실제 낙하 이벤트/Kernel 자식과 진단 부모를 구별한다.
namespace { // 도달한 선택 장치만 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("다리 낙하 부모의 선택된 실제 장치 미연결");return function; } // checking0은 genus/진단을 미리 요구하지 않는다.
}
void RunBridgeFallParent(Sid bridge,float x,float y,RawCheckingFlag checking,const BridgeFallParentHooks& hooks) { // 원문은 checking을 한 번 읽고 선택한 genus 뒤에는 다시 검사하지 않는다.
    if (checking && !(Need(hooks.genus)(bridge)&TypeFlag2::kBridge)) Need(hooks.assertion)("s.getGenus() & gBRIDGE","\\Ns\\O\\Bridge.cpp",0x323);
    Need(hooks.schedule)(bridge,x,y);
}
RegularProcess* ScheduleBridgeFallChecked(SquidProcessHost& host,const SidPool& pool,std::span<const RiftTypeRecord> types,
    Sid bridge,float x,float y,RawCheckingFlag checking,
    const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion) { // 기존 좌표 포장/실제 등록과 반환 의미는 바꾸지 않는다.
    if (&host.Pool()!=&pool) throw std::invalid_argument("다리 낙하 부모의 실제 Host/Pool 불일치");
    if (pool.Edition()!=OriginalEdition::Patch1078) return ScheduleBridgeFall(host,bridge,x,y); // CD의 원문은 이Patch검사부모없이직접예약한다.
    BridgeFallParentHooks hooks;RegularProcess* result{};
    hooks.genus=[&](auto sid) { const auto type=pool.Slot(sid)[10];if (type>=types.size()) throw std::out_of_range("다리 낙하 진단의 현재 TYPE 물리 범위");return types[type].flags2; };
    hooks.schedule=[&](auto sid,auto px,auto py) { result=ScheduleBridgeFall(host,sid,px,py); };
    hooks.assertion=assertion ? assertion:std::function<void(std::string_view,std::string_view,std::uint32_t)>([](auto condition,auto source,auto line) { throw std::logic_error("원문 다리 낙하 진단 "+std::string(source)+":"+std::to_string(line)+": "+std::string(condition)); }); // 실제진단UI가미연결된호스트는선택한조건만명시실패한다.
    RunBridgeFallParent(bridge,x,y,checking,hooks);return result;
}
}
