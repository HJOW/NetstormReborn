// 원문 서버 재조회와 별개 세 DWORD store·Player 방송 뒤 활동0 순서를 보존한다.
#include "o/RawConnectionEnded.h"
#include <stdexcept>

namespace netstorm::o { // 실제 부모는 게임 상태를 전환하지 않고 다음 상태 DWORD만 예약한다.
namespace { // 선택된 장치와 현재 flags 한 DWORD의 물리만 확인한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("연결 종료 통지의 실제 장치 미연결");return value; } // 미선택 장치는 요구하지 않는다.
void ChangeFlags(const ConnectionEndedGlobals& globals,std::uint32_t owner,std::uint32_t clear,std::uint32_t set) {
    auto bytes=Need(globals.players)();const auto at=std::size_t(owner)*180+0x50;if (at+4>bytes.size()) throw std::out_of_range("연결 종료 Player flags 물리 부족");std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8); // 실제 현재 비정렬 DWORD만 읽는다.
    value=(value&~clear)|set;
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 인접 Player 필드는 보존한다.
}
}
void ReceiveConnectionEnded(std::int32_t sender,std::uint32_t connection,ConnectionEndedGlobals globals,const ConnectionEndedHooks& hooks) {
    if (connection==globals.synchronization) globals.synchronization=0;
    if (!globals.server) { if (sender==9) { Need(hooks.saveServerFailure)();if (globals.server) return; }Need(hooks.disconnect)(sender); }
    else if (sender!=9) Need(hooks.disconnect)(sender); // 서버의9번 자기 끝은 이 부모에서 닫지 않는다.
    if (globals.server && sender>0 && sender<9) {
        const auto owner=static_cast<std::uint32_t>(sender);ChangeFlags(globals,owner,0,0x100);ChangeFlags(globals,owner,1,0);ChangeFlags(globals,owner,0x40,0);
        Need(hooks.broadcastPlayer)(owner);globals.frames[owner]=0; // 송신이 실패하면 앞선 flags는 남고 활동 프레임은 아직 지우지 않는다.
    }
    if (sender==10 && globals.phase==23 && globals.nextPhase!=24 && globals.nextPhase!=12) globals.nextPhase=20;
}
}
