// 글자별 구간을 한 번 보정한 뒤 공통 프레임 지정의 표시/공간 효과를 실행한다.
#include "o/RawFrameAdvance.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 프레임은 패치 +0x24 DWORD, CD +0x22 BYTE에 저장된다.
constexpr std::size_t kPatchFrame=36,kCdFrame=34;
// 정렬에 의존하지 않고 패치의 little endian 프레임 비트를 읽는다.
std::uint32_t ReadFrame(std::span<const std::uint8_t> raw) {
    std::uint32_t bits=0;
    // 필드 네 바이트만 합쳐 주변 상태를 섞지 않는다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[kPatchFrame+i])<<(8*i);
    return bits;
}
}
// 불일치한 풀 연결은 실제 프레임/해시를 바꾸기 전에 거부한다.
RawFrameAdvance::RawFrameAdvance(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter)
    :pool_(pool),frames_(frames),setter_(setter) {
    if (&pool!=&setter.Pool() || frames.empty()) throw std::invalid_argument("프레임 진행 연결 오류");
}
// 원본의 signed 비교와 32비트 넘침을 unsigned 연산/bit_cast로 보존한다. modulo나 반복 보정은 하지 않는다.
// 원본: 004afc90 / CD 004acce0. 구간 밖 메모리 읽기는 호스트에서 예외로 진단한다.
bool RawFrameAdvance::Advance(Sid sid,std::int32_t delta,std::uint32_t flags) const {
    const auto raw=pool_.Slot(sid);
    if (raw[10]>=frames_.size()) throw std::out_of_range("프레임 진행 타입 오류");
    const auto current=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(ReadFrame(raw)) :
        static_cast<std::int32_t>(raw[kCdFrame]);
    const auto& frames=frames_[raw[10]];
    const auto side=RawPathAnimation::Side(frames,current);
    const auto run=frames.Run(static_cast<std::uint8_t>(side));
    const auto first=static_cast<std::uint32_t>(run.first),count=static_cast<std::uint32_t>(run.count);
    auto relative=static_cast<std::uint32_t>(current)+static_cast<std::uint32_t>(delta)-first;
    bool wrapped=false;
    if (std::bit_cast<std::int32_t>(relative)>=run.count) { relative-=count;wrapped=true; }
    else if (std::bit_cast<std::int32_t>(relative)<0) { relative+=count;wrapped=true; }
    setter_.Set(sid,std::bit_cast<std::int32_t>(first+relative),flags);
    return wrapped;
}
// イベント構成時の raw プール一致検査に使用する。
const SidPool& RawFrameAdvance::Pool() const { return pool_; }
// 同じプールでも異なる効果指定器への接続を防ぐ。
const SquidFrame& RawFrameAdvance::Setter() const { return setter_; }
}
