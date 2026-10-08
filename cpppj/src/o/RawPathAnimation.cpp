// 정지 전환 뒤의 경로 종료는 외부 효과다. 원본 전체 PathProcess와 보행 타이머 복원은 후속이다.
#include "o/RawPathAnimation.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 타입·프레임 위치와 도착 재등록의 원본 flags다.
constexpr std::size_t kType=10,kPatchFrame=36,kCdFrame=34;
constexpr std::uint32_t kArrivalPopFlags=0x40;
}
// 원본 가상 효과 두 개는 생략할 수 없다. 프레임 표의 수명은 내부 복사로 보장한다.
RawPathAnimation::RawPathAnimation(SidPool& pool,std::span<const RiftTypeFrames> frames,PathAnimationHooks hooks)
    :pool_(pool),frames_(frames.begin(),frames.end()),hooks_(std::move(hooks)) {
    if (!hooks_.unpop || !hooks_.repop) throw std::invalid_argument("보행 정지 효과 누락");
}
// 배열 밖의 원본 메모리 읽기는 호스트에서 변경 전 예외로 거부한다.
int RawPathAnimation::Side(const RiftTypeFrames& frames,std::int32_t frame) {
    if (frame<0 || static_cast<std::size_t>(frame)>=frames.Codes().size()) throw std::out_of_range("보행 프레임 범위 오류");
    return std::bit_cast<std::int8_t>(frames.Codes()[static_cast<std::size_t>(frame)].side);
}
// L~P와 음수 side도 원본 signed 계산을 유지한다.
int RawPathAnimation::Direction(const RiftTypeFrames& frames,std::int32_t frame) { return Side(frames,frame)-'A'; }
// 원본은 타입의 +0x130 글자 구간 표를 읽는다. Run은 그 표의 복원된 생성 결과다.
std::int32_t RawPathAnimation::RestFrame(const RiftTypeFrames& frames,std::int32_t frame) {
    const int side=Side(frames,frame);
    if (side<'A' || side>'P') throw std::out_of_range("보행 정지 글자 표 범위 오류");
    return frames.Run(static_cast<std::uint8_t>(side)).first+1;
}
// CD에서 프레임 다음의 extra 바이트를 섞지 않는다.
std::int32_t RawPathAnimation::Current(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    if (raw[kType]<kFirstAssetTypeNumber || raw[kType]>=frames_.size()) throw std::out_of_range("보행 정지 자산 타입 오류");
    if (pool_.Edition()!=OriginalEdition::Patch1078) return raw[kCdFrame];
    std::uint32_t value=0;
    // 패치의 프레임 DWORD는 정렬되지 않은 little endian 필드다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[kPatchFrame+i])<<(8*i);
    return std::bit_cast<std::int32_t>(value);
}
// 읽기와 글자 표 조회를 마친 뒤부터는 실제 원본 호출/쓰기 순서다.
void RawPathAnimation::Stop(Sid sid) const {
    // free/예약 SID와 타입/현재 프레임 오류는 가상 효과를 부르기 전에 거부한다.
    pool_.AllocatedBytes(sid);
    const auto current=Current(sid);
    const auto target=RestFrame(frames_[pool_.Slot(sid)[kType]],current);
    hooks_.unpop(sid,0);
    auto raw=pool_.AllocatedBytes(sid);
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        // DWORD 전체를 쓰며 다른 필드와 위치는 보존한다.
        for (std::size_t i=0;i<4;++i) raw[kPatchFrame+i]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(target)>>(8*i));
    } else raw[kCdFrame]=static_cast<std::uint8_t>(target); // CD는 원본 mov byte처럼 하위 바이트만 쓴다.
    hooks_.repop(sid,kArrivalPopFlags);
}
}
