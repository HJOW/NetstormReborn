// 현재 상태를 효과 전에 과도하게 캐시하지 않고 원본의 조건별 반복 조회를 유지한다.
#include "o/RawFenceRegular.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kFenceHandleTable=0x00509470; //10.78 썬 바리케이드의 실제 가상 표다.
// 비정렬 DWORD의 실제 네 바이트만 읽는다.
std::uint32_t ReadFenceRegular(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; //현재 원본 기록의 little endian 결합값이다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); //이웃 필드로 읽기를 넓히지 않는다.
    return value;
}
// +2a/+2e의 원본 DWORD 폭만 쓴다.
void WriteFenceRegular(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //낮은 바이트부터 기록한다.
}
// 미선택 효과는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존한 채 거부한다.
template<class Callback> const Callback& NeedFenceRegular(const Callback& callback) {
    if (!callback) throw std::logic_error("썬 바리케이드 Handle의 실제 하위 미연결");return callback;
}
}
// 원본 판본과 현재 프레임 지정기의 단일 저장소를 확인한다.
RawFenceRegular::RawFenceRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,FenceRegularHooks hooks)
    :pool_(pool),frames_(frames),setter_(setter),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188 || &setter.Pool()!=&pool)
        throw std::invalid_argument("썬 바리케이드 Handle 판본/프레임/풀 불일치");
}
// TYPE 이름만 같은 다른 부모를 이 처리기로 호출하지 않는다.
bool RawFenceRegular::Handles(Sid sid) const { return ReadFenceRegular(pool_.Slot(sid),0)==kFenceHandleTable; }
// 현재 owner의 DWORD 캐시는 네 이웃 검색을 마친 뒤에만 쓴다.
void RawFenceRegular::ClearNeighbors(Sid sid) const {
    std::array<Sid,4> neighbors{}; //원본 local_1c의 스냅샷이며 효과 뒤 다시 검색하지 않는다.
    for (std::uint32_t direction=0;direction<8;direction+=2) neighbors[direction/2]=NeedFenceRegular(hooks_.neighbor)(sid,direction); //원본0/2/4/6 순서다.
    WriteFenceRegular(pool_.AllocatedBytes(sid),42,pool_.Slot(sid)[34]);
    for (const auto neighbor:neighbors) { //먼저 모은 네 SID를 같은 순서로 처리한다.
        if (!neighbor.value) continue;
        NeedFenceRegular(hooks_.clearMarkers)(sid,neighbor);
        const auto a=pool_.Slot(sid),b=pool_.Slot(neighbor); //mark 삭제 효과 뒤의 현재 좌표를 읽는다.
        NeedFenceRegular(hooks_.clearLine)({std::bit_cast<float>(ReadFenceRegular(a,14)),std::bit_cast<float>(ReadFenceRegular(a,18)),
            std::bit_cast<float>(ReadFenceRegular(b,14)),std::bit_cast<float>(ReadFenceRegular(b,18))});
    }
    Schedule(sid);
}
// 확보 실패 이후에도 상위 Handle의 다음 연결 수 조회는 계속된다.
void RawFenceRegular::Schedule(Sid sid) const {
    if (NeedFenceRegular(hooks_.reserveRegular)()) NeedFenceRegular(hooks_.addRegular)(sid,2,0.1f);
}
// 보호 bit20은 사건 번호보다 먼저 검사한다. event0의 두 변경 분기는 독립이다.
float RawFenceRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("썬 바리케이드 Handle 실제 가상 표 불일치");
    if (pool_.Slot(sid)[40]&0x20U) return NeedFenceRegular(hooks_.damageable)(sid,event,count,payload);
    if (event==0) {
        const auto raw=pool_.Slot(sid);const auto type=raw[10];
        if (type>=frames_.size()) throw std::out_of_range("썬 바리케이드 Handle TYPE 범위 오류");
        const auto current=std::bit_cast<std::int32_t>(ReadFenceRegular(raw,36));
        const auto direction=RawPathAnimation::Direction(frames_[type],current);
        if (direction<0 || direction>=16) throw std::out_of_range("썬 바리케이드 Handle 글자 범위 오류");
        const auto size=frames_[type].Run(static_cast<std::uint8_t>('A'+direction)).count;
        const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(current)+1U);
        if (!size || (next==std::numeric_limits<std::int32_t>::min() && size==-1)) throw std::domain_error("썬 바리케이드 Handle 원본 idiv 범위 오류");
        setter_.Set(sid,next%size,0); //원본은 구간의 첫 번호를 더하지 않고 절대 나머지를 지정한다.
        if (ReadFenceRegular(pool_.Slot(sid),42)!=pool_.Slot(sid)[34]) ClearNeighbors(sid);
        const auto observed=NeedFenceRegular(hooks_.connections)(sid); //원본은 하위 호출을 마친 뒤 캐시DWORD를 읽는다.
        if (observed!=ReadFenceRegular(pool_.Slot(sid),46)) {
            const auto changed=NeedFenceRegular(hooks_.connections)(sid); //원본은 비교 때와 기록 때 두 번 현재 값을 조회한다.
            WriteFenceRegular(pool_.AllocatedBytes(sid),46,changed);ClearNeighbors(sid);
        }
        return 0.1f;
    }
    if (event==1) { NeedFenceRegular(hooks_.refreshNeighbors)(sid);return 0.0f; }
    if (event==2) { NeedFenceRegular(hooks_.rebuild)(sid);return 0.0f; }
    if (event==3) throw std::logic_error("썬 바리케이드 사건3의 원본 잔류 x87 반환은 호출자 관찰 전 미복원");
    return NeedFenceRegular(hooks_.damageable)(sid,event,count,payload);
}
// 이미 복원한 다른 부모는 순서와 예외를 유지한 채 fallback으로 넘긴다.
RegularHandler RawFenceRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);
        if (!fallback) throw std::logic_error("썬 바리케이드 외 Regular 부모 미연결");return fallback(sid,event,count,payload);
    };
}
}
