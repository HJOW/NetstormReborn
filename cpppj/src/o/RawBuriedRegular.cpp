// 원본 오벨리스크는 event0만 애니메이션/지연을 바꾸고 다른 사건은 기본 Handle로 반환한다.
#include "o/RawBuriedRegular.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBuriedHandleTable=0x0050a748; // 10.78 생성자가 기록하는 실제 오벨리스크 가상 표다.
// 정렬되지 않은 네 바이트만 little endian DWORD로 결합한다.
std::uint32_t ReadBuried(std::span<const std::uint8_t> bytes) {
    std::uint32_t result{}; // 가상 표 주소 기록값이며 호스트 포인터가 아니다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[index])<<(8*index); // 지정한 네 바이트만 읽는다.
    return result;
}
}
// 다른 판본의 가상 표/프레임 배치를 PATCH와 동일하다고 가정하지 않는다.
RawBuriedRegular::RawBuriedRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,GameRandom& random)
    :pool_(pool),frames_(frames),setter_(setter),random_(random) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188 || &setter.Pool()!=&pool)
        throw std::invalid_argument("오벨리스크 Handle의 판본/TYPE/프레임 풀 불일치");
}
// TYPE 번호만 같은 다른 부모를 잘못 분배하지 않는다.
bool RawBuriedRegular::Handles(Sid sid) const { return ReadBuried(pool_.Slot(sid))==kBuriedHandleTable; }
// event1의004acd10→0049a840은 QA로 주소를 계산한 반환값을 버린다. 다른 실제 상태/효과 변경은 없다.
float RawBuriedRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("오벨리스크 Handle 실제 가상 표 불일치");
    if (event!=0) return payload; // 기본00496e70도 전달된 float만 반환하며 event1에 추가 효과는 없다.
    const auto type=pool_.Slot(sid)[10];
    if (type>=frames_.size()) throw std::out_of_range("오벨리스크 Handle TYPE 범위 오류");
    const auto run=frames_[type].Run('B'); // 원본TYPE+178/+138의 현재 B구간 수/첫 프레임이다.
    const auto divisor=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(run.count)+1U);
    const auto signedCount=std::bit_cast<std::int32_t>(count); // 원본 idiv는 countDWORD를 signed로 읽는다.
    if (!divisor || (signedCount==std::numeric_limits<std::int32_t>::min() && divisor==-1))
        throw std::domain_error("오벨리스크 Handle 원본 idiv 범위 오류");
    const auto remainder=signedCount%divisor;
    if (remainder) {
        const auto frame=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(run.first)+static_cast<std::uint32_t>(remainder)-1U);
        setter_.Set(sid,frame,0);return 0.1f;
    }
    setter_.Set(sid,frames_[type].Run('A').first,0);
    return static_cast<float>(random_.Next(6)+2); //004558f0(2,8)은 한 번 소비하며8은 포함하지 않는다.
}
// 미등록 부모는 빈 성공으로 바꾸지 않고 기존 분배기로 넘긴다.
RegularHandler RawBuriedRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);
        if (!fallback) throw std::logic_error("오벨리스크 외 Regular 부모 미연결");
        return fallback(sid,event,count,payload);
    };
}
}
