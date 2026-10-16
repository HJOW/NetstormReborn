// 썬 바리케이드의 이웃 스냅샷과 효과 이후 현재 좌표/소유자 재조회를 보존한다.
#include "o/RawFencePostPop.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kFenceVtable=0x00509470; // PATCH 썬 바리케이드 생성자의 실제 가상 주소다.
// 비정렬 DWORD를 읽으며 원본 포인터를 호스트 주소로 실행하지 않는다.
std::uint32_t ReadFence(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; // 원본 little endian 네 바이트의 결합 결과다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); // 필드 밖 바이트는 읽지 않는다.
    return value;
}
// 원본이 쓴 DWORD 폭만 바꾸고 이웃 필드와 padding을 보존한다.
void WriteFence(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 낮은 바이트부터 저장한다.
}
// 미연결 효과는 선택 위치에서 실패시키며 앞선 쓰기와 예약을 보존한다.
template<class Function> const Function& NeedFence(const Function& function) {
    if (!function) throw std::logic_error("썬 바리케이드 후처리의 실제 하위 소비자 미연결");
    return function;
}
}
// CD의 다른 펜스 동작을 PATCH로 간주하지 않는다.
RawFencePostPop::RawFencePostPop(SidPool& pool,FencePostPopHooks hooks):pool_(pool),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("썬 바리케이드 후처리는 PATCH10.78만 복원했습니다");
}
// 실제 가상 표만 분배하며 타입 번호만 같은 다른 몸체를 허용하지 않는다.
bool RawFencePostPop::Handles(Sid sid) const { return ReadFence(pool_.Slot(sid),0)==kFenceVtable; }
// projected 위치를 현재 공통 후처리의 검사에 그대로 전달한다.
void RawFencePostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* projected) const {
    if (!Handles(sid)) throw std::invalid_argument("썬 바리케이드 실제 가상 표 불일치");
    if (hooks_.validateBase) hooks_.validateBase(sid,flags,projected);
}
// 매 할당의 독립 실패는 해당 생성자만 생략한다. float0.1의 비트도 유지한다.
void RawFencePostPop::Regular(Sid sid,std::uint32_t event) const {
    if (NeedFence(hooks_.reserveRegular)()) NeedFence(hooks_.addRegular)(sid,event,std::bit_cast<float>(std::uint32_t{0x3dcccccd}));
}
// 확보/예약 효과가 변경한 두 슬롯의 현재 좌표를 네 float 값으로 복사한다.
void RawFencePostPop::ClearPair(Sid first,Sid second) const {
    const auto a=pool_.Slot(first),b=pool_.Slot(second); // 각 쌍의 두 끝점은 이 호출 직전에 조회한다.
    NeedFence(hooks_.clearLine)({std::bit_cast<float>(ReadFence(a,14)),std::bit_cast<float>(ReadFence(a,18)),
        std::bit_cast<float>(ReadFence(b,14)),std::bit_cast<float>(ReadFence(b,18))});
}
// flags1에서만 최초 사건과 연결 수를 준비하고 모든 flags에서 같은 Damageable로 끝난다.
void RawFencePostPop::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);
    if (flags&1U) {
        if (NeedFence(hooks_.theme)(sid)==3) Regular(sid,0);
        std::array<Sid,4> neighbors{}; // 원문 local_1c는 네 방향을 모두 조회한 뒤 두 쌍에서 재사용한다.
        for (std::uint32_t direction=0;direction<8;direction+=2) neighbors[direction/2]=NeedFence(hooks_.neighbor)(sid,direction); // 방향 순서를0/2/4/6으로 고정한다.
        WriteFence(pool_.AllocatedBytes(sid),42,pool_.Slot(sid)[34]);
        if (neighbors[0].value && neighbors[2].value) { Regular(neighbors[0],3);Regular(neighbors[2],3);ClearPair(neighbors[0],neighbors[2]); }
        if (neighbors[1].value && neighbors[3].value) { Regular(neighbors[1],3);Regular(neighbors[3],3);ClearPair(neighbors[1],neighbors[3]); }
        const auto count=NeedFence(hooks_.connections)(sid); // 조회의 외부 효과 뒤 반환값만 원본+2e에 기록한다.
        WriteFence(pool_.AllocatedBytes(sid),46,count);Regular(sid,2);
    }
    NeedFence(hooks_.damageable)(sid,flags);
}
// 파생 등록 시 다른 월드의 후처리와 결합하지 않도록 사용한다.
const SidPool& RawFencePostPop::Pool() const { return pool_; }
}
