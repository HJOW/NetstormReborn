// SP조회 뒤 코드 재조회와 종속 알림 전에 보존하는 next 순서를 유지한다.
#include "o/RawPlayerContentNotice.h"
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kMoneyThreshold=7000.0F; // ROM00505514의SP이상조건단정도상수다.
// 비정렬 원본 필드의 정확BYTE폭을 읽는다.
std::uint32_t NoticeWord(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t result=0;
    // 낮은BYTE부터조합하여WORD뒤의stale값을섞지않는다.
    for (std::size_t index=0;index<width;++index) result|=std::uint32_t(raw[offset+index])<<(index*8);return result;
}
}
// 미연결실제SP/등록표/UI를내용물통지성공으로대체하지않는다.
RawPlayerContentNotice::RawPlayerContentNotice(const SidPool& pool,PlayerContentNoticeState state,PlayerContentNoticeHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !hooks_.money || !hooks_.gump || !hooks_.handle)
        throw std::invalid_argument("Player 내용물의 현재 실제 SP/등록표/UI 경계 누락");
}
// Player주소에서얻은같은SID로현재종속체인을읽으며진단접두도원본순서대로실행한다.
void RawPlayerContentNotice::Notify(Sid parent,std::uint32_t form,std::uint32_t reason) const {
    auto fallback=state_.fallbackCode; // 원본EAX는첫비교직후읽고SP조회가없으면뒤조건에서도보존한다.
    if (state_.cursorA.Read()!=state_.cursorB.Read()) state_.problemCode=fallback;
    if (state_.moneyChecks) {
        if (state_.battleShown.Read()) {
            const auto money=hooks_.money(state_.localPlayer.Read()); // 출력float는조건부조회에서한번저장한현재SP다.
            fallback=state_.fallbackCode;
            if (money>kMoneyThreshold) state_.problemCode=fallback;
        }
        --state_.moneyChecks;
    }
    if (!state_.firstSpPool.Read()) state_.problemCode=fallback;
    if (state_.firstPermutation.Read()==-1) state_.problemCode=fallback;
    Sid current{static_cast<std::uint16_t>(NoticeWord(pool_.Slot(parent),6,2))};std::unordered_set<std::uint16_t> visited;
    // 현재head는진단/SP효과가끝난뒤읽으며각next는UI조회/처리전에보존한다.
    while (current.value) {
        if (current.value>=pool_.Capacity() || !visited.insert(current.value).second) throw std::logic_error("Player 내용물 UI 종속 체인 물리 범위/순환 오류");
        const auto raw=pool_.Slot(current);const auto type=std::uint32_t(raw[10]),gumpType=state_.gumpFormType;
        const Sid next{static_cast<std::uint16_t>(NoticeWord(raw,4,2))}; // 원본은다음Form을lookup/가상handle보다먼저읽는다.
        if (type==gumpType) {
            const auto token=hooks_.gump(NoticeWord(raw,18,4)); // 등록ID는WORD로줄이지않는현재DWORD다.
            if (token) hooks_.handle(token,state_.contentEvent,form,reason);
        }
        current=next;
    }
}
// 상위ParentNotice가현재PlayerVT를이몸체에보낼때같은풀을확인한다.
const SidPool& RawPlayerContentNotice::Pool() const { return pool_; }
}
