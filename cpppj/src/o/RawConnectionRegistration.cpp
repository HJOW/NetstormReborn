// callback 복귀 뒤 현재 슬롯/개수를 읽고 앞선 쓰기를 실패 시에도 보존한다.
#include "o/RawConnectionRegistration.h"
#include <stdexcept>

namespace netstorm::o { //호스트 물리 부족은 도달한 읽기 위치에서만 명시 거부한다.
namespace { //원문의 실제 선택 장치와 고정표 인덱스를 확인한다.
// 선행 성공을 되돌리지 않고 선택된 장치 누락에서 중단한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("연결 등록의 선택 장치 누락");return function; }
// signed 인덱스 변환 중 INT_MIN의 C++ overflow를 피하며 부족한 물리를 거부한다.
std::uint32_t& Slot(std::span<std::uint32_t> entries,std::int64_t index) { if (index<0 || static_cast<std::uint64_t>(index)>=entries.size()) throw std::out_of_range("연결 등록의 실제 표 범위");return entries[static_cast<std::size_t>(index)]; }
}
void RegisterTransceiver(std::int32_t id,std::uint32_t token,ConnectionCleanupGlobals globals,const ConnectionRegistrationHooks& hooks) {
    if (Slot(globals.transceivers,id)) Need(hooks.assertion)(0xc2);
    Slot(globals.transceivers,id)=token;Need(hooks.node)(token).sender=static_cast<std::uint32_t>(id);
}
void RemovePendingConnection(std::int32_t id,ConnectionCleanupGlobals globals) {
    Slot(globals.pending,-std::int64_t(id))=0;globals.pendingCount-=1U;
}
std::int32_t AssignPendingConnection(std::int32_t pending,std::int32_t target,std::uint32_t server,ConnectionCleanupGlobals globals,const ConnectionRegistrationHooks& hooks) {
    if (server) { if (!(target>0 && target<9)) Need(hooks.assertion)(0xb0); }
    else if (target!=9) Need(hooks.assertion)(0xb3);
    if (Slot(globals.transceivers,target)) Need(hooks.assertion)(0xb6);
    const auto token=Slot(globals.pending,-std::int64_t(pending));Slot(globals.transceivers,target)=token;Need(hooks.node)(token).sender=static_cast<std::uint32_t>(target);
    Need(hooks.buffered)(Slot(globals.transceivers,target));if (pending>=0) Need(hooks.assertion)(0xbb);
    RemovePendingConnection(pending,globals);return target;
}
}
