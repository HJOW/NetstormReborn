#include "client/MessageBoard.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 선택된 분기의 모든 실제 하위를 생성 전에 확인한다.
MessageBoard::MessageBoard(MessageBoardState& state,MessageBoardHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.lookup || !hooks_.kill || !hooks_.create || !hooks_.width || !hooks_.attach) throw std::invalid_argument("메시지 표 실제 Gump 연결 누락");
}
// 검색 중 발견한 첫 빈 칸을 보관하며 stale serial도 같은 칸에서 지운다.
void MessageBoard::Show(std::int32_t kind,std::int32_t player,bool enabled,std::int32_t parameter,std::string_view text) const {
    if (state_.suppressed && kind!=11 && kind!=16) return;
    if (state_.serials[0]==0xffffffffU) state_.serials.fill(0);
    std::size_t firstEmpty=state_.serials.size(),index=0;std::optional<MessageBoardRecord> found;
    // 원본은 중복 일치에서 멈추므로 그 뒤의 stale 칸을 미리 정리하지 않는다.
    for (;index<state_.serials.size();++index) {
        if (state_.serials[index]) {
            found=hooks_.lookup(state_.serials[index]);
            if (!found) state_.serials[index]=0;
            else if (found->kind==kind && found->player==player && found->parameter==parameter && found->kind!=17) break;
            else found.reset();
        }
        if (!state_.serials[index] && firstEmpty==state_.serials.size()) firstEmpty=index;
    }
    if (!enabled) {
        if (found) hooks_.kill(found->token);
        if (index<state_.serials.size()) state_.serials[index]=0;
        return;
    }
    if (firstEmpty==state_.serials.size()) return;
    const auto record=hooks_.create(kind,player,parameter,text);state_.serials[firstEmpty]=record.serial;
    const auto width=hooks_.width(record.token);
    const auto x=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(state_.screenWidth)-static_cast<std::uint32_t>(width));
    hooks_.attach(record.token,x,static_cast<std::int32_t>(firstEmpty*12));
}
// 실제 명령은 fld float 지연→fadd double 시각→fstp double이다. 디컴파일 C의 시각 float 변환을 적용하지 않는다.
double WarningDeadline(double wallSeconds) { return wallSeconds+10.0; }
// NaN 기한/시각은 비교가 거짓이며 음수의 비영 기한도 원래 비교대로 처리한다.
bool WarningExpired(double deadline,double wallSeconds) { return deadline!=0.0 && wallSeconds>=deadline; }
}
