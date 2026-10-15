// buried의 최초 예약 두 번과 선택된 QA 쓰기를 현재 원본 입력에서 처리한다.
#include "o/RawBuriedPostPop.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBuriedVtable=0x0050a748; // PATCH 생성자004625f0이 기록하는 실제 가상 표다.
// 선택된 미연결 하위만 명시 거부하고 앞서 실행한 원문 효과는 보존한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("buried PostPop의 실제 하위 소비자 미연결");return function; }
}
// 현재 boss/로딩 주소를 빌리며 다른 판본을 동일하게 동작한다고 가정하지 않는다.
RawBuriedPostPop::RawBuriedPostPop(SidPool& pool,const bool& boss,const std::uint32_t& loading,BuriedPostPopHooks hooks)
    :pool_(pool),boss_(boss),loading_(loading),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("buried PostPop은 PATCH10.78만 복원했습니다");
}
// 가상 표 DWORD를 호스트 포인터로 실행하지 않는다.
bool RawBuriedPostPop::Handles(Sid sid) const {
    const auto raw=pool_.Slot(sid);std::uint32_t table{}; // 실제 네 BYTE를 결합한 가상 주소다.
    for (std::size_t index=0;index<4;++index) table|=std::uint32_t(raw[index])<<(index*8); // 비정렬 DWORD의 판본 바이트 순서를 보존한다.
    return table==kBuriedVtable;
}
// 공통 Validate는 실제 Pop의 projected 좌표도 같은 Graph에 전달한다.
void RawBuriedPostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    if (!Handles(sid)) throw std::invalid_argument("buried PostPop 실제 가상 표 불일치");
    if (hooks_.validateBase) hooks_.validateBase(sid,flags,pop);
}
// boss가 꺼져도 최초 예약은 수행하며 로딩 조건은 보상 재추첨만 억제한다.
void RawBuriedPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);
    if ((flags&1U) && boss_ && loading_==0) {
        const auto type=Need(hooks_.chooseType)(5);auto raw=pool_.AllocatedBytes(sid); // 원본 setQA는 반환값의 낮은 WORD만 저장한다.
        raw[12]=static_cast<std::uint8_t>(type);raw[13]=static_cast<std::uint8_t>(type>>8);
    }
    if (flags&1U) {
        if (Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(sid,0,0);
        if (Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(sid,1,std::bit_cast<float>(std::uint32_t{0x3c23d70a}));
    }
    // 004acd10→0049a840은 QA로 주소를 계산하지만 반환 주소를 사용하지 않고 메모리도 쓰지 않는다.
    Need(hooks_.base)(sid,flags);
}
// 동일 풀의 파생 분배에만 등록할 수 있도록 참조를 돌려준다.
const SidPool& RawBuriedPostPop::Pool() const { return pool_; }
}
