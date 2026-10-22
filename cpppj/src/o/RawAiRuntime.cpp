// 현재필드재조회·float지갑·벽시각·Player별수명과원본전체행동순서를보존한다.
#include "o/RawAiRuntime.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerBytes=180,kPlayers=9,kBrain=0xb0; //원본Playerstride/개수/현재본문필드다.
constexpr double kSlowFrame=0.1; //00501960의실제QWORD실행시간기준이다.
// 비정렬네BYTE만little endian으로조회한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 프레임 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //주변바이트를읽지않는다.
// 현재Player의지정네BYTE만쓴다.
void Write(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //원본나머지Player상태는보존한다.
// 도달한하위누락을빈성공으로대신하지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 전체 프레임 실제 하위 미연결");return function; }
}
// 생성은본문/Player/게임계산을변경하지않는다.
RawAiRuntime::RawAiRuntime(RawAiBrainMemory& memory,std::span<std::uint8_t> players,AiRuntimeGlobals globals,AiRuntimeHooks hooks):memory_(memory),players_(players),globals_(std::move(globals)),hooks_(std::move(hooks)) { if (players.size()!=kPlayerBytes*kPlayers) throw std::invalid_argument("AI 프레임 실제9Player 누락"); }
// 해제된본문/잘못된크기의가짜본문을거부한다.
std::span<std::uint8_t> RawAiRuntime::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 프레임 실제0xb08 본문 누락");return raw; }
// 각원본위치에서현재본문을다시조회한다.
std::uint32_t RawAiRuntime::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 실제417c70은지갑정수를float에먼저저장해현재SP float와비교한다.
void RawAiRuntime::Actions(std::uint32_t token) const {
    static_cast<void>(Body(token));Need(hooks_.collectors)(token);Need(hooks_.factories)(token);const auto removed=Need(hooks_.dryScan)(token);if (removed) Need(hooks_.dryNotice)(token,globals_.dryLog,removed==Field(token,0x280));
    Need(hooks_.priest)(token,Field(token,0x29c));Need(hooks_.drops)(token);Need(hooks_.bridges)(token);Need(hooks_.carriers)(token);Need(hooks_.economy)(token);
    const auto wallet=static_cast<float>(std::bit_cast<std::int32_t>(Field(token,0x2d8)));const auto money=Need(hooks_.money)(Field(token,0x260));if (!(wallet==money)) Need(hooks_.assertion)("money == getMoney( playerId )",0x1048);
}
// 초기화효과뒤258을다시읽고타이밍전역은전체행동뒤현재값을읽는다.
bool RawAiRuntime::Tick(std::uint32_t token) const {
    if (Field(token,0x254)) return false;if (!Field(token,0x258)) Need(hooks_.initialize)(token);if (!Field(token,0x258)) return true;
    const auto started=Need(hooks_.wall)();Need(hooks_.prepare)(token);Need(hooks_.selectGoal)(token);Actions(token);
    if (globals_.timing) { const auto elapsed=Need(hooks_.wall)()-started;if (elapsed>kSlowFrame) Need(hooks_.took)(token,elapsed); }
    return true;
}
// 254종료때현재Player본문을재조회하고소멸→반납뒤같은b0를0으로쓴다.
void RawAiRuntime::Frame() const {
    if (!Need(globals_.battle)()) return;
    for (std::size_t owner=1;owner<kPlayers;++owner) { //현재Player1..8을같은stride로차례로방문한다.
        const auto at=owner*kPlayerBytes+kBrain;const auto token=Read(players_,at);if (!token || Tick(token)) continue;const auto current=Read(players_,at);if (current) { Need(hooks_.destruct)(current);Need(hooks_.free)(current); }Write(players_,at,0);
    }
}
}
