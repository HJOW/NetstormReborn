// R.exe의원문번호선택/진단복귀뒤쓰기/서로다른개수초기값을보존한다.
#include "root/ConnectionState.h"
#include <bit>
#include <stdexcept>

namespace netstorm::root { //새Root서버의실제장치를주입할수있는원문몸체이며게임클라이언트의22칸표를재사용하지않는다.
namespace { //선택장치/원문DWORD부호와물리범위만처리한다.
// 실제선택된외부장치만요구하며미연결을서버성공으로바꾸지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R 연결부모의선택된실제장치미연결");return function; }
// x86 NEG의하위DWORD와signed비교를호스트signed오버플로없이보존한다.
std::uint32_t PendingIndex(std::int32_t id) { return 0U-std::bit_cast<std::uint32_t>(id); }
// 원문pending의물리32칸만허용하며진단반환뒤실제잘못된접근은거부한다.
std::uint32_t& PendingSlot(ConnectionState& state,std::int32_t id) { return state.pending.at(PendingIndex(id)); }
// 제공된R의고정원문파일/실제줄번호를그대로진단장치에전달한다.
void Assert(const ConnectionHooks& hooks,std::string_view expression,std::uint32_t line,bool root=false) { Need(hooks.assertion)(expression,root ? "c:\\ns\\r\\r.cpp":"c:\\ns\\zacket\\zacket.cpp",line); }
}
// 일반0칸은읽기대상이될수있어도새일반번호의검색대상은아니다.
std::uint32_t FindFreeTransceiver(const ConnectionState& state) {
    for (std::uint32_t id=1;id<state.transceivers.size();++id) if (!state.transceivers[id]) return id; //원문번호순으로현재슬롯을읽으며예약표시는쓰지않는다.
    return 0;
}
std::uint32_t PendingTransceiver(const ConnectionState& state,std::int32_t id) { return state.pending.at(PendingIndex(id)); } //원문lower직접조회이며R상위관문과합치지않는다.
// 원문의특이한negative관문도기계어00474842..47의NEG/CMP/JGE와같다.
std::uint32_t ReadTransceiver(const ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks) {
    if (id<0) {
        if (std::bit_cast<std::int32_t>(PendingIndex(id))<32) return 0;
        if (state.assertions && !(state.modeFlags&0x13U)) Assert(hooks,"smModeFlags & (smROOT|smDIAG)",195,true);
        return PendingTransceiver(state,id);
    }
    return state.transceivers.at(static_cast<std::uint32_t>(id));
}
void InitializePending(ConnectionState& state) { if (!state.pendingInitialized) { state.pending.fill(0);state.pendingInitialized=1; } } //Native최초listener초기화는개수data1을다시쓰지않는다.
// 첫빈대기번호가없으면원문처럼현재표/개수를그대로둔다.
void RegisterPending(ConnectionState& state,std::uint32_t token,const ConnectionHooks& hooks) {
    for (std::uint32_t index=1;index<state.pending.size();++index) { //실제원문은1..31의첫현재0칸만선택한다.
        if (state.pending[index]) continue;state.pending[index]=token;Need(hooks.sender)(token,0U-index);++state.pendingCount;
        if (std::bit_cast<std::int32_t>(state.pendingCount)>=32) Assert(hooks,"numPending <= zaMAX_PENDING-1",1204);return;
    }
}
// 연결승격은노드를닫지않으며앞선진단이반환해도뒤의원문쓰기를계속한다.
void UnregisterPending(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks) {
    if (id>=0) Assert(hooks,"id < 0",1175);auto& slot=PendingSlot(state,id);if (!slot) Assert(hooks,"pending[-id] != NULL",1176);
    slot=0;--state.pendingCount;if (std::bit_cast<std::int32_t>(state.pendingCount)<1) Assert(hooks,"numPending >= 1",1181);
}
// 현재closecallback이슬롯을변경해도원문은그같은칸에마지막0을쓴다.
void ClosePending(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks) {
    if (id<0 && id>-32) { auto& slot=PendingSlot(state,id);if (slot) { Need(hooks.close)(slot);slot=0;--state.pendingCount; } }
    if (std::bit_cast<std::int32_t>(state.pendingCount)<1) Assert(hooks,"numPending >= 1",1191);
}
void CloseAllPending(ConnectionState& state,const ConnectionHooks& hooks) {
    for (auto& slot:state.pending) if (slot) { Need(hooks.close)(slot);slot=0; } //원문은0칸도닫고개수를별도수정하지않는다.
}
// 일반연결번호와상위BYTE의Challenge번호를선택한뒤같은현재토큰을조회한다.
void ReceiveConnectionEnded(ConnectionState& state,std::int32_t id,const ConnectionHooks& hooks) {
    if (state.exitRequested) { Need(hooks.exit)(0);throw std::logic_error("R 원문_exit장치가정상복귀했습니다"); }
    if (id<0) ClosePending(state,id,hooks);
    if (id<256) {
        if (id>0) { auto& token=state.transceivers[static_cast<std::uint32_t>(id)];if (token) { Need(hooks.close)(token);token=0; } }
    } else {
        const auto challenge=id>>8;if (challenge>0 && challenge<state.challengeStateCount) { const auto token=state.challenges.at(static_cast<std::uint32_t>(challenge));if (token) Need(hooks.endParticipant)(token,static_cast<std::uint32_t>(id)&0xffU); }
    }
}
void SelectLanLaunch(ConnectionState& state,std::span<const std::string> arguments) { if (arguments.size()==2 && std::string_view(arguments[1]).substr(0,arguments[1].find('\0'))=="lan") state.localLaunch=1; } //다른인자는현재DWORD를지우지않는다.
//모드해석은현재DWORD의원문OR와마지막증가만실행하며접속성공을만들지않는다.
void InterpretMode(ConnectionState& state,const std::function<void(std::string_view,std::int32_t&)>& readInt) {
    std::int32_t value{};Need(readInt)("diagModeRecv",value);if (value) state.modeFlags|=kDiagReceive;
    value=0;Need(readInt)("diagModeXmit",value);if (value) state.modeFlags|=kDiagTransmit;
    value=0;Need(readInt)("rootMode",value);if (value) state.modeFlags|=kRootMode;
    if (state.localLaunch) { state.modeFlags|=kLocalMode;state.challengeStateCount=1; }
    state.challengeStateCount=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state.challengeStateCount)+1U);
}
}
