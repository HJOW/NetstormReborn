// 폭탄/포획/지면 검사/해방 소리의 서로 다른 단락평가와 현재 전역을 유지한다.
#include "o/RawCarrierContentNotice.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 화면 투영의 CRT 하위DWORD와 카메라 뺄셈 감김을 유지한다.
std::int32_t ProjectCoordinate(float coordinate,std::int32_t scale,std::int32_t camera) {
    const double value=static_cast<double>(coordinate)*scale+0.5;std::int64_t integer=0;
    if (std::isfinite(value) && value>=-0x1p63 && value<0x1p63) integer=static_cast<std::int64_t>(value);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer)-static_cast<std::uint32_t>(camera));
}
// 선택된 실제 효과 누락을 빈 성공으로 바꾸지 않는다.
template<class F> const F& Required(const F& hook) { if (!hook) throw std::logic_error("Carrier 내용물 통지 실제 효과 미연결");return hook; }
}
// 실제 판본과 현재 타입 표를 확인하며 생성 시 가상 효과를 실행하지 않는다.
RawCarrierContentNotice::RawCarrierContentNotice(const SidPool& pool,std::span<const RiftTypeRecord> types,
    CarrierContentNoticeState state,CarrierContentNoticeHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)),dispatch_(pool) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Carrier 내용물 통지 PATCH 자료 오류");
}
// 현재 부모의 실제 VT를 검사하므로 타입 번호만으로 파생 범위를 넓히지 않는다.
bool RawCarrierContentNotice::Handles(Sid parent) const { return dispatch_.NativeEntry(parent)==0x00429560U; }
// 원본이 사용하는 필드 폭만 읽는다.
std::uint32_t RawCarrierContentNotice::Word(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value=0;
    // 비정렬 little endian 바이트를 조립한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
// 효과 뒤 바뀐 kind와 타입 플래그를 매번 조회한다.
const RiftTypeRecord& RawCarrierContentNotice::ContentType(Sid form) const {
    const auto type=Word(form,18);if (type>=types_.size()) throw std::out_of_range("Carrier 내용물 kind 범위");return types_[type];
}
// 원본497220은 두 축을 별도로 ftol하고 현재 원점을 뺀 signed32 화면 좌표를 쓴다.
std::array<std::int32_t,2> RawCarrierContentNotice::Project(std::array<float,2> point) const {
    return {ProjectCoordinate(point[0],16,state_.cameraX),ProjectCoordinate(point[1],11,state_.cameraY)};
}
// 해당 호출에 필요한 두 현재 float의 비트다.
std::array<float,2> RawCarrierContentNotice::Position(Sid parent) const { return {std::bit_cast<float>(Word(parent,14,4)),std::bit_cast<float>(Word(parent,18,4))}; }
// 원본429560 전체의 분기를 실행한다. reason4를 이유로 나머지 조건을 통째로 생략하지 않는다.
void RawCarrierContentNotice::Notice(Sid parent,Sid form,std::uint32_t reason) const {
    if (state_.savedGame) return;
    if (state_.loadingDepth) return;
    if (!(Word(form,11,1)&2U) && (ContentType(form).flags2&0x100U) && !state_.bombBlocked) {
        const auto center=Required(hooks_.center)(parent);
        const auto first=Project(center);const auto second=Project(center);
        Required(hooks_.bolt)(parent,{first[0],0},second,5.0F);
        const auto soundCenter=Required(hooks_.center)(parent);Required(hooks_.soundAt)(soundCenter[0],soundCenter[1],"thunderCrack.wav");
        const auto value=Required(hooks_.typeWord)(Word(form,18),0x6c);
        if (Word(parent,34,1)==state_.localOwner) {
            const auto storage=Required(hooks_.allocate)(40);std::uint32_t process=0;
            if (storage) process=Required(hooks_.create)(storage,parent,value,0,12);
            if (process) Required(hooks_.duration)(process,1.5F);
        }
    } else if (Word(parent,34,1)==state_.localOwner && Word(form,18)==state_.priestType && !(Word(form,11,1)&2U)) {
        const auto sound=Required(hooks_.typeWord)(Word(parent,10,1),0x7c);Required(hooks_.sound)(sound);
    }
    if (!(Word(form,11,1)&2U) && !state_.loadingDepth && (reason&1U) && Word(form,20) && (ContentType(form).flags2&0x208100U))
        Required(hooks_.checkCarrier)(parent);
    if ((reason&2U) && Word(form,18)==state_.priestType) {
        const auto position=Position(parent);
        if (!Required(hooks_.genusAt)(position[0],position[1],0x400000U).value) {
            const auto current=Position(parent);
            if (!Required(hooks_.typeAt)(current[0],current[1],state_.freeSoundBlockerType).value) Required(hooks_.freeSound)("priestFree.wav");
        }
    }
}
}
