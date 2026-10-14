// 원본의 signed 문자 계산과 BYTE/DWORD 프레임 차이를 보존한다.
#include "o/RawGunMode.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
// 판본의 실제 전체 프레임 표를 확인하고 사본을 만들지 않는다.
RawGunMode::RawGunMode(const SidPool& pool,std::span<const RiftTypeFrames> frames,const std::uint32_t& windArcherType)
    :pool_(pool),frames_(frames),windArcherType_(windArcherType) {
    if (frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("Gun mode 판본 프레임 표 불일치");
}
// current graph/frame을 항상 실제 슬롯에서 읽으며 타입별 기본 방향0으로 대신하지 않는다.
std::uint32_t RawGunMode::Get(Sid sid) const {
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto type=raw[10];if (type<70 || type>=frames_.size()) throw std::invalid_argument("Gun mode 현재 자산 TYPE 오류");
    if (type==windArcherType_ && !(raw[patch ? 40U : 35U]&8)) return static_cast<std::uint32_t>(raw[patch ? 30U : 28U])*2U;
    std::uint32_t frame=raw[34]; // CD는 현재 BYTE를 zero-extend한다.
    if (patch) {
        frame=0;
        // 패치의 정렬되지 않은 DWORD 전체를 원본 little endian으로 읽는다.
        for (std::size_t i=0;i<4;++i) frame|=static_cast<std::uint32_t>(raw[36+i])<<(8*i);
    }
    const auto codes=frames_[type].Codes();if (frame>=codes.size()) throw std::out_of_range("Gun mode 현재 프레임 물리 범위 오류");
    const auto side=std::bit_cast<std::int8_t>(codes[frame].side); // 원본char는 signed BYTE다.
    const auto result=static_cast<std::int32_t>(side)*2-152;return result<0 || result>7 ? 0U : static_cast<std::uint32_t>(result);
}
// 현재 World/표적 검색과 같은 실제 Pool인지 확인한다.
const SidPool& RawGunMode::Pool() const { return pool_; }
}
