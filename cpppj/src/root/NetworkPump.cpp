// 한 번 캡처한 시각과 현재 토큰 재조회로 R 통신 순서/유휴 종료를 보존한다.
#include "root/NetworkPump.h"
#include <bit>
#include <stdexcept>

namespace netstorm::root { // R 전용 연결표/Challenge 표만 순회한다.
namespace { // 원문 signed DWORD 연산과 선택된 장치의 수명을 좁게 표현한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R Network 실제 장치 미연결");return function; } // 선택하지 않은 callback을 선행 요구하지 않는다.
bool Expired(std::int32_t timeout,std::uint32_t now,std::uint32_t received) { return timeout<std::bit_cast<std::int32_t>(now-received); } // x86 SUB 감김 뒤 signed 비교를 유지한다.
void CheckCount(std::uint32_t count,std::uint32_t line,const NetworkHooks& hooks) { if (count>=64) Need(hooks.assertion)("count < FD_SETSIZE","c:\\ns\\r\\r.cpp",line); } // PE004cd860=1499에 원문 분기별 줄 offset을 더한 값이다.
void Timeout(std::uint32_t token,std::string_view prefix,const NetworkHooks& hooks) { const auto text=Need(hooks.describe)(token);Need(hooks.log)(9,std::string(prefix)+text+"\n"); } // 실제 주소 서식화 뒤에만 level9 로그를 호출한다.
void AddMaximum(std::uintptr_t socket,std::uintptr_t& maximum) { if (static_cast<std::intptr_t>(socket)>=static_cast<std::intptr_t>(maximum)) maximum=socket; } // OS 핸들은 현대 포인터 폭으로 비교한다. Winsock은 nfds를 사용하지 않는다.
}
void NetworkSocketSet::Add(std::uintptr_t socket) { if (count<sockets.size()) sockets[count++]=socket; } // 집합이 가득 차면 원문처럼 추가만 생략한다.
NetworkPumpResult PumpNetwork(ConnectionState& connections,NetworkState& state,std::int32_t waitSeconds,const NetworkHooks& hooks) {
    const auto now=Need(hooks.clock)();NetworkSocketSets sets;std::uintptr_t maximum{};NetworkPumpResult result; // 지역 FullTime은 select/콜백이 오래 걸려도 같은 값을 유지한다.
    for (std::size_t index=0;index<connections.transceivers.size();++index) { // 원문 일반0..255를 현재 슬롯으로 순회한다.
        auto& current=connections.transceivers[index];if (!current) continue;
        if (state.mainTimeout) {
            const auto received=Need(hooks.lastReceived)(current); // 원문은 마지막 수신 DWORD 조회 뒤 현재 옵션을 비교한다.
            if (Expired(state.mainTimeout,now,received) && !Need(hooks.noMainTimeout)(current) && !(Need(hooks.socketFlags)(current)&0x80U)) { Timeout(current,"Main transceiver timeout ",hooks);Need(hooks.terminate)(current);continue; }
        }
        if (sets.read.count<64) sets.read.Add(Need(hooks.socket)(current));
        if (sets.error.count<64) sets.error.Add(Need(hooks.socket)(current));
        const auto compared=Need(hooks.socket)(current);if (static_cast<std::intptr_t>(compared)>=static_cast<std::intptr_t>(maximum)) maximum=Need(hooks.socket)(current); // 비교와 선택 분기의 두 원문 getter를 보존한다.
        ++result.sockets;
        if (!(Need(hooks.socketFlags)(current)&0x80U) && !Need(hooks.socketStatus)(current)) {
            Need(hooks.log)(0x27,"Root: unconnected incoming transceiver on "+std::to_string(index)+"\n");
            if (sets.write.count<64) sets.write.Add(Need(hooks.socket)(current));
        }
        CheckCount(result.sockets,1499U+0x2eU,hooks);
    }
    for (std::size_t index=0;index<connections.pending.size();++index) { // 원문 대기 집합 작성은0도 포함한다. 수신 단계는1부터다.
        auto& current=connections.pending[index];if (!current) continue;
        if (state.pendingTimeout) {
            const auto received=Need(hooks.lastReceived)(current); // 대기도 getter 복귀 뒤 현재 옵션을 비교한다.
            if (Expired(state.pendingTimeout,now,received)) { Timeout(current,"Pending transceiver timeout ",hooks);Need(hooks.closePending)(-static_cast<std::int32_t>(index));continue; }
        }
        if (sets.read.count<64) sets.read.Add(Need(hooks.socket)(current));
        if (sets.error.count<64) sets.error.Add(Need(hooks.socket)(current));
        const auto compared=Need(hooks.socket)(current);if (static_cast<std::intptr_t>(compared)>=static_cast<std::intptr_t>(maximum)) maximum=Need(hooks.socket)(current);
        ++result.sockets;CheckCount(result.sockets,1499U+0x40U,hooks);
    }
    for (std::size_t index=0;index<connections.challenges.size();++index) { // 현재51개 Challenge의40개 전송표를 별도로 읽는다.
        if (!connections.challenges[index]) continue;
        for (std::uint32_t participant=0;participant<40;++participant) { // 타임아웃 전 현재 비영 참가자를 먼저 센다.
            const auto current=Need(hooks.challengeConnection)(connections.challenges[index],participant);if (!current) continue;
            ++result.challengeParticipants;
            if (state.challengeTimeout) {
                const auto received=Need(hooks.lastReceived)(current); // 이 참가자는 이미 세었으며 종료해도 이번 개수에서 빼지 않는다.
                if (Expired(state.challengeTimeout,now,received)) { Timeout(current,"Chal transciever timeout ",hooks);Need(hooks.terminate)(current);continue; }
            }
            const auto socket=Need(hooks.socket)(current);sets.read.Add(socket);AddMaximum(socket,maximum);++result.sockets;CheckCount(result.sockets,1499U+0x65U,hooks);
        }
    }
    if (Need(hooks.auxiliaryActive)()) { const auto compared=Need(hooks.auxiliarySets)(sets);if (static_cast<std::intptr_t>(compared)>=static_cast<std::intptr_t>(maximum)) maximum=Need(hooks.auxiliarySets)(sets); } // 원문 조건에 따라7f510이 두 번 호출될 수 있다.
    result.selected=Need(hooks.select)(maximum+std::uintptr_t{1},sets,waitSeconds); // select 오류에서도 아래 원문 순회를 생략하지 않는다.
    for (std::size_t index=1;index<connections.pending.size();++index) if (connections.pending[index]) Need(hooks.receive)(connections.pending[index],1); //85c10의 실제 현재1..31 가상 수신이다.
    for (std::size_t index=0;index<connections.challenges.size();++index) if (connections.challenges[index]) { // 수신 callback 뒤 같은 현재 Challenge를 다시 조회한다.
        Need(hooks.receiveChallenge)(connections.challenges[index]);Need(hooks.sendChallenge)(connections.challenges[index]);
    }
    for (std::size_t index=0;index<connections.transceivers.size();++index) if (connections.transceivers[index]) { // 일반 수신이 현재 슬롯을 지우면 flush를 생략한다.
        Need(hooks.receive)(connections.transceivers[index],1);if (connections.transceivers[index]) Need(hooks.flush)(connections.transceivers[index]);
    }
    if (Need(hooks.auxiliaryActive)()) Need(hooks.auxiliaryPoll)();
    if (connections.localLaunch && !result.challengeParticipants) {
        if (state.previousChallengeParticipants) state.emptySince=now;
        if (state.emptyExitTimeout && Expired(state.emptyExitTimeout,now,state.emptySince)) { Need(hooks.exit)(1);throw std::logic_error("R LAN 유휴 _exit가 복귀함"); }
    }
    state.previousChallengeParticipants=result.challengeParticipants;return result;
}
}
