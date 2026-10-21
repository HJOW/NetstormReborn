// 디컴파일 C가 누락한 가산점과 원본 출력 SID/조회 순서를 기계어에 맞춰 보존한다.
#include "o/RawAiDropTargets.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kBlockerOffset=1.0F; //PE005012d0의 두 축 보정값이며 발자국 절반과 다르다.
constexpr std::size_t kBlockerCapacity=30; //00412950의 new(78h)/count1eh 실제 임시 목록 용량이다.
// 비정렬 little endian DWORD만 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 표적 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은 지정 네 BYTE를 읽는다.
// DWORD 감김 뒤 signed 해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본 ftol의 signed64 절삭 하위DWORD를 읽으며 무효 FISTP의 하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 선택되지 않은 하위는 요구하지 않고 선택된 미연결은 명시 실패한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 표적 실제 하위 미연결");return function; }
}
// 생성자는 실제 검색/장치를 실행하지 않는다.
RawAiDropTargets::RawAiDropTargets(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiDropTargetGlobals globals,AiDropTargetHooks hooks):memory_(memory),pool_(pool),types_(types),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI 배치 표적 같은 PATCH풀 누락"); }
// 현재 본체의 정확한 크기와 수명을 확인한다.
std::span<std::uint8_t> RawAiDropTargets::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 표적 실제0xb08본체 누락");return raw; }
// 하위 효과 뒤 현재 본체를 다시 읽는다.
std::uint32_t RawAiDropTargets::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 원본 BYTE TYPE의 현재 flags/group을 읽는다.
const RiftTypeRecord& RawAiDropTargets::Type(Sid target) const { const auto type=pool_.Slot(target)[10];if (type>=types_.size()) throw std::out_of_range("AI 배치 표적 현재TYPE 범위");return types_[type]; }
// 손상된 DWORD를 정상 SID로 잘라 쓰지 않는다.
Sid RawAiDropTargets::Object(std::uint32_t value) const { if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("AI 배치 표적 물리SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 원본 최초 포인터/count만 사용하며 중복 가산은 하지 않는다.
bool RawAiDropTargets::Contains(std::uint32_t token,std::size_t pointer,Sid target) const { const auto count=Signed(Field(token,pointer+8));if (count<=0) return false;const auto list=memory_.Bytes(Field(token,pointer));for (std::int32_t index=0;index<count;++index) if (Read(list,static_cast<std::size_t>(index)*4)==target.value) return true;return false; } //반복문은 최초 개수에서 첫 일치까지만 읽는다.
// 정밀 C가 누락한 group 가산과 실제 모든 점수를 기계어 순서대로 보존한다.
std::int32_t RawAiDropTargets::Score(std::uint32_t token,Sid target) const {
    const auto flags=Type(target).flags2;std::uint32_t value=(flags&0x4000000U) ? 5U : 10U;
    if (flags&0x200000U) value+=Field(token,0x2a8) ? 5U : 200U;if (flags&0x200U) value+=6;
    if (Type(target).group==6) value+=7;if (pool_.Slot(target)[10]==globals_.altarType) value+=8;
    if (flags&0x30000U) { if (!target.value) Need(hooks_.assertion)("sid",0xaeb);if (Need(hooks_.captive)(target).value) value+=400; }
    value=std::bit_cast<std::uint32_t>(Ftol(static_cast<double>(Need(hooks_.rate)(target))+Signed(value)));
    if (Contains(token,0x36c,target)) value+=100;if (Contains(token,0x378,target)) value+=100;
    if (Field(token,0x3f8)==target.value) { const auto hp=Signed(Read(pool_.Slot(target),26));const auto maximum=Need(hooks_.maxHitPoints)(target);if (hp>=maximum/2) value+=50; }
    const auto result=Signed(value);return pool_.Slot(target)[34]==Field(token,0x264) ? result : result/2;
}
// 출력은 검색 뒤 점수 계산 전이며0도 기록한다.
std::int32_t RawAiDropTargets::Gun(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& result) const { const auto owner=Field(token,0x260);const auto found=Need(hooks_.gun)(point.x,point.y,type,direction,owner);result=found;return found.value ? Score(token,found) : 0; }
// 특수 flags는 중심 조회 전에 반환하며 다른 방향값은 네 방향 중 하나와 정확히 같아야 한다.
bool RawAiDropTargets::Facing(RawPathPoint point,std::int32_t direction,Sid target) const { if (Type(target).flags2&0x100000U) return false;const auto center=Need(hooks_.center)(target);const auto facing=RawSquidNeighbors::Direction(point.x,point.y,center.x,center.y);return (direction==0 || direction==2 || direction==4 || direction==6) && facing==direction; }
// 모든 등장마다 점수를 계산하고 같은 양의 최고값은 출력 SID를 다시 쓰지 않는다.
std::int32_t RawAiDropTargets::Blockers(std::uint32_t token,std::uint32_t type,RawPathPoint point,std::int32_t direction,Sid& result) const {
    const auto owner=Field(token,0x260);const auto list=Need(hooks_.blockers)(owner,point,kBlockerCapacity);std::uint32_t total{};std::int32_t best{};
    // 원본 임시 목록의 순서/중복을 보존하고 방향 필터도 점수 계산 뒤 수행한다.
    for (auto value:list) { const auto target=Object(value);auto score=Score(token,target);if (type==globals_.directionalType && !Facing({static_cast<float>(static_cast<double>(point.x)-kBlockerOffset),static_cast<float>(static_cast<double>(point.y)-kBlockerOffset)},direction,target)) score=0;if (score>best) { best=score;result=Object(list.front()); }total+=std::bit_cast<std::uint32_t>(score); }
    return Signed(total);
}
// 동일 실제 풀 연결 확인용이다.
const SidPool& RawAiDropTargets::Pool() const { return pool_; }
}
