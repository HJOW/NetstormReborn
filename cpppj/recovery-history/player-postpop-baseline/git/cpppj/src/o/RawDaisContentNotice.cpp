// 최초 부착 알림은 알터 Unpop 뒤 현재 타입을 다시 읽고 plain 프레임/WORD를 지정한다.
#include "o/RawDaisContentNotice.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 정렬되지 않은 원본 BYTE/WORD/DWORD를 지정한 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 현재 필드에 속한 바이트만 낮은 순서부터 조합한다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);
    return value;
}
}
// 다른 풀이나 빠진 가상 공간/장치 경계를 최초 raw 쓰기 전에 거부한다.
RawDaisContentNotice::RawDaisContentNotice(SidPool& pool,std::span<const RiftTypeFrames> frames,const RawRunePostPop& rune,
    const DaisContentNoticeState& state,DaisContentNoticeHooks hooks)
    :pool_(pool),frames_(frames),rune_(rune),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&rune.Pool() || frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)
        || !hooks_.unpop || !hooks_.repop || !hooks_.stopVoice || !hooks_.notifySurface)
        throw std::invalid_argument("다이스 내용물 알림의 동일 풀/필수 효과 누락");
}
// 생성자 VT에 없는 부모 알림을 일반 body로 대신하지 않는다.
bool RawDaisContentNotice::Handles(Sid dais) const {
    const auto raw=pool_.Slot(dais);return Read(raw,0,4)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchDaisVtable : kCdDaisVtable);
}
// 첫 프레임의 원본 signed DWORD 또는 하위 BYTE를 그대로 쓴다.
void RawDaisContentNotice::WriteFrame(Sid altar,std::int32_t frame) const {
    auto raw=pool_.AllocatedBytes(altar);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto value=std::bit_cast<std::uint32_t>(frame);const std::size_t offset=patch ? 36 : 34,width=patch ? 4 : 1;
    // 표시 setter를 호출하지 않고 지정한 폭만 바꾼다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 004ac220/CD004abb00의 WORD 폭과 현재 다리 후속 분기다.
void RawDaisContentNotice::WriteWord(Sid altar,std::uint16_t word) const {
    auto raw=pool_.AllocatedBytes(altar);raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);
    if (raw[10]==state_.bridgeType) hooks_.notifySurface(altar);
}
// reason2 제거는 자연반환이며 kind 검사는 현재 전역 사제 타입과 비교한다.
void RawDaisContentNotice::Notice(Sid dais,Sid form,std::uint32_t reason) const {
    if (!Handles(dais)) throw std::invalid_argument("다이스 내용물 알림의 실제 부모 VT 오류");
    if (!(reason&1) || Read(pool_.Slot(form),18,2)!=state_.priestType) return;
    const auto altar=rune_.AltarAt(dais);
    if (altar.value) {
        hooks_.unpop(altar,0);const auto type=pool_.Slot(altar)[10];if (type>=frames_.size()) throw std::out_of_range("다이스 알림의 현재 알터 타입 범위");
        WriteFrame(altar,frames_[type].Run('O').first);WriteWord(altar,static_cast<std::uint16_t>(Read(pool_.Slot(form),12,2)&127));hooks_.repop(altar,0);
    }
    if (pool_.Slot(dais)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]==state_.localOwner) hooks_.stopVoice(state_.cue,0,0);
}
// 실제 ContentFormHost의 같은 월드 연결을 확인할 때 사용한다.
const SidPool& RawDaisContentNotice::Pool() const { return pool_; }
}
