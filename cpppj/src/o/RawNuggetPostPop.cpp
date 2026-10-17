// 원본484f90의 조회→할당→난수→생성 순서와 독립 실패를 보존한다.
#include "o/RawNuggetPostPop.h"
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택한 실제 경계만 요구한다. 미리보기 flags4에는 예약/난수가 필요 없다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("너겟 실제 PostPop 하위 몸체 미연결");return function; }
}
RawNuggetPostPop::RawNuggetPostPop(SidPool& pool,NuggetPostPopHooks hooks):pool_(pool),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("너겟 PostPop은10.78 몸체입니다"); }
void RawNuggetPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    if (flags&1U) {
        if (!Need(hooks_.hasRegular)(sid,1) && Need(hooks_.allocate)()) { const auto payload=Need(hooks_.random)(0.0F,0.3F);Need(hooks_.construct)(sid,1,payload); }
        if (Need(hooks_.allocate)()) { const auto payload=Need(hooks_.random)(0.0F,0.5F);Need(hooks_.construct)(sid,3,payload); }
    }
    Need(hooks_.damageable)(sid,flags);
}
const SidPool& RawNuggetPostPop::Pool() const { return pool_; }
}
