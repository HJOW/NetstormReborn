// 원문 고정표의 각 현재 슬롯과 callback 뒤의 현재 값을 구별한다.
#include "o/RawConnectionCleanup.h"
#include <stdexcept>

namespace netstorm::o { //게임 모드/Player/로그인 성공을 연결 정리에서 만들지 않는다.
namespace { //선택한 장치를 실제 호출 위치에서만 요구한다.
// 앞선 실제 쓰기를 남기고 누락된 외부 효과에서 중단한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("연결 정리의 선택 장치 누락");return function; }
// 호스트 span의 물리 부족을 해당 슬롯을 읽기 전에 명시적으로 거부한다.
std::uint32_t& Slot(std::span<std::uint32_t> entries,std::size_t index) { if (index>=entries.size()) throw std::out_of_range("연결 정리의 실제 표 범위 부족");return entries[index]; }
// 동일 토큰의 실제 연결에만 지연 삭제 부모를 적용하고 반납 뒤 노드를 읽지 않는다.
void CloseToken(std::uint32_t token,const ConnectionCleanupHooks& hooks) {
    auto& connection=Need(hooks.node)(token);DeferConnectionClose(connection,[&] { Need(hooks.destroy)(token); },[&] { Need(hooks.release)(token); });
}
// 실제 양수 연결 한 칸의 현재 동기화와 종료 후0쓰기를 공유한다.
void CloseMainSlot(ConnectionCleanupGlobals globals,std::size_t index,const ConnectionCleanupHooks& hooks) {
    const auto token=Slot(globals.transceivers,index);if (!token) return;
    if (token==globals.synchronization) globals.synchronization=0;
    CloseToken(token,hooks);Slot(globals.transceivers,index)=0;
}
}
void DeferConnectionClose(PacketConnection& connection,const std::function<void()>& destroy,const std::function<void()>& release) {
    const auto depth=connection.depth;connection.flags|=0x40U;
    if (depth==0) { Need(destroy)();Need(release)(); }
}
void ClosePendingConnection(std::int32_t id,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks) {
    if (id>=0 || id<=-32) return;const auto index=static_cast<std::uint32_t>(-id);const auto token=Slot(globals.pending,index);if (!token) return;
    CloseToken(token,hooks);const auto count=globals.pendingCount-1U;Slot(globals.pending,index)=0;globals.pendingCount=count;
}
void DisconnectTransceiver(std::int32_t id,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks) {
    if (id<0) { ClosePendingConnection(id,globals,hooks);return; }
    if (id>=22) Need(hooks.assertion)(250); //원문 진단이 반환하면 다음 실제 span 접근에서 물리 부족을 거부한다.
    const auto index=static_cast<std::uint32_t>(id);CloseMainSlot(globals,index,hooks);
    const auto buffer=Slot(globals.buffers,index);if (buffer) { Need(hooks.release)(buffer);Slot(globals.buffers,index)=0; }
}
void CloseAllTransceivers(ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks) {
    for (std::size_t index=0;index<kTransceiverSlots;++index) CloseMainSlot(globals,index,hooks); //방문 후 callback이 앞 칸을 채워도 같은 호출에서 되돌아가지 않는다.
}
void EndPlayerConnections(std::uint32_t server,ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks) {
    if (!server) return;
    for (std::size_t index=1;index<9;++index) CloseMainSlot(globals,index,hooks); //진입 server 조건은 한 번이며 연결은 매 슬롯의 현재값이다.
}
void CloseAllPendingConnections(ConnectionCleanupGlobals globals,const ConnectionCleanupHooks& hooks) {
    for (std::size_t index=0;index<kPendingConnectionSlots;++index) { const auto token=Slot(globals.pending,index);if (token) { CloseToken(token,hooks);Slot(globals.pending,index)=0; } } //pendingCount는 이 부모가 갱신하지 않는다.
}
}
