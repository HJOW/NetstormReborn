// PATCH 원본의 예측 그림 생성과 완료 효과 순서를 유지한다.
#include "o/RawFactoryUpgrade.h"
#include "o/RawSquidHitPoints.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kPictureOffset=0.0010000000474974513F; //PE00509144의 실제 y 오프셋이다.
// 필요한 하위만 호출 지점에서 요구하고 앞선 원문 쓰기는 유지한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Factory 업그레이드 실제 하위 미연결");return function; }
// 정렬되지 않은 원문을 낮은 BYTE부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    if (offset>raw.size() || raw.size()-offset<width) throw std::out_of_range("Factory 업그레이드 원문 범위");std::uint32_t value{};
    // BYTE를 원본 little endian 자리로 옮긴다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
}
// 실행 효과 없이 같은 풀을 사용하는지 확인한다.
RawFactoryUpgrade::RawFactoryUpgrade(SidPool& pool,SquidProcessHost& host,FactoryUpgradeState state,FactoryUpgradeHooks hooks)
    :pool_(pool),host_(host),state_(state),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&host.Pool()) throw std::invalid_argument("Factory 업그레이드 같은 PATCH 풀/Host 누락"); }
// 가상 getter가 하위 효과로 바뀌어도 원본 재조회를 유지한다.
std::uint32_t RawFactoryUpgrade::NextLevel(Sid parent) const { const auto next=Need(hooks_.level)(parent)+1U;if (std::bit_cast<std::int32_t>(next)>3) return 3;return Need(hooks_.level)(parent)+1U; }
// 추상 상태는 void일 때만 설정하며 상태WORD와 프레임DWORD를 혼동하지 않는다.
void RawFactoryUpgrade::Begin(Sid parent,float payload) const {
    const auto image=Need(hooks_.create)(pool_.Slot(parent)[10],2);const auto level=NextLevel(parent);Need(hooks_.setLevel)(image,level);
    pool_.AllocatedBytes(image)[40]|=8;if (!(pool_.Slot(image)[11]&4U)) Need(hooks_.assertion)(0x282);pool_.AllocatedBytes(image)[40]|=1;
    auto raw=pool_.AllocatedBytes(image);raw[12]=0;raw[13]=0;Need(hooks_.owner)(image,pool_.Slot(parent)[34]);
    const auto current=pool_.Slot(parent);const auto x=std::bit_cast<float>(Read(current,14,4));const auto y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(current,18,4)))+static_cast<double>(kPictureOffset));Need(hooks_.pop)(image,x,y,0);
    Need(hooks_.log)(4,"making the client upgrade picture");Need(hooks_.construction)(image,payload);Need(hooks_.depend)(parent,image);
}
// finder의 검사 조건도 호출 시점의 원본 전역을 읽는다.
Sid RawFactoryUpgrade::First(Sid parent) const { if (state_.checking && !state_.challenge && (pool_.Slot(parent)[11]&2U) && !state_.boss) Need(hooks_.assertion)(0x158);return Sid{static_cast<std::uint16_t>(Read(pool_.Slot(parent),6,2))}; }
// 후보 반환 전에 next를 저장하므로 현재 그림 삭제가 순회를 깨지 않는다.
Sid RawFactoryUpgrade::Next(Sid& next,std::size_t& steps) const {
    // 원본처럼 asset과 다른 Form만 건너뛰며 free/dead를 추가 제외하지 않는다.
    while (next.value) { if (next.value>=pool_.Capacity() || ++steps>pool_.Capacity()) throw std::logic_error("Factory 업그레이드 종속 체인 손상");const auto current=next;const auto raw=pool_.Slot(current);next=Sid{static_cast<std::uint16_t>(Read(raw,4,2))};if (raw[10]<70 && raw[10]==state_.dependType) return current; }
    return {};
}
// client 번호5..14999만 예측 그림이며 소유자/free/dead 조건은 추가하지 않는다.
bool RawFactoryUpgrade::Picture(Sid parent,Sid candidate) const { const auto raw=pool_.Slot(candidate);return raw[10]==pool_.Slot(parent)[10] && (raw[40]&8U) && candidate.value>4 && candidate.value<15000; }
// outpost는 모든 완료 효과와 예약/그림 삭제까지 생략한다.
void RawFactoryUpgrade::Complete(Sid parent) const {
    if (pool_.Slot(parent)[10]==state_.outpost) return;
    const auto level=NextLevel(parent);Need(hooks_.setLevel)(parent,level);const auto hp=Need(hooks_.maxHp)(parent);SetRawSquidHitPoints(pool_,parent,hp);Need(hooks_.adjustFrame)(parent);Need(hooks_.display)(parent);
    const auto raw=pool_.Slot(parent);Need(hooks_.soundAt)(std::bit_cast<float>(Read(raw,14,4)),std::bit_cast<float>(Read(raw,18,4)),"upgradeDone.wav");
    if (pool_.Slot(parent)[34]==state_.localPlayer) Need(hooks_.sound)("upgradeComplete.wav");Need(hooks_.flare)(parent,6,2);
    // 완료 효과가 추가한 동일 사건까지 원본의 반복 검색 순서4→3으로 제거한다.
    while (auto* process=host_.FindEvent(parent,4)) host_.Kill(*process,0);
    // 현재 실행 중인 event3도 실제 Host의 수명/등록 검사로 안전하게 제거된다.
    while (auto* process=host_.FindEvent(parent,3)) host_.Kill(*process,0);
    auto next=First(parent);std::size_t steps{};
    // 감시 Form의 다음 링크는 그림/양쪽 Depend 삭제 전에 저장한다.
    for (auto form=Next(next,steps);form.value;form=Next(next,steps)) { const auto watcher=Need(hooks_.watcher)(form);if (watcher==parent) Need(hooks_.assertion)(0xdd);if (Picture(parent,watcher)) Need(hooks_.destroy)(watcher,0); }
}
// 완료 효과 뒤 현재 소유자/AI를 다시 조회하며 outpost에도 원본 통지 접두를 적용한다.
float RawFactoryUpgrade::Finish(Sid parent) const { Need(hooks_.log)(3,"Upgrading!\n");Complete(parent);if (Need(hooks_.aiPresent)(parent)) Need(hooks_.notifyAi)(pool_.Slot(parent)[34],parent);return -1.0F; }
// 원본 첫 일치에서 멈추고 후속 감시자는 읽지 않는다.
bool RawFactoryUpgrade::HasPicture(Sid parent) const { auto next=First(parent);std::size_t steps{};
    // 저장next 규칙으로 실제 감시 Form을 방문한다.
    for (auto form=Next(next,steps);form.value;form=Next(next,steps)) { const auto watcher=Need(hooks_.watcher)(form);if (watcher==parent) Need(hooks_.assertion)(0xad);if (Picture(parent,watcher)) return true; }return false;
}
// 예측 그림이 있으면 예약 목록을 조회하지 않는다.
bool RawFactoryUpgrade::IsUpgrading(Sid parent) const { return HasPicture(parent) || host_.FindEvent(parent,4)!=nullptr || host_.FindEvent(parent,3)!=nullptr; }
// 실제 월드 연결 진단에 쓰며 복사 저장소를 만들지 않는다.
const SidPool& RawFactoryUpgrade::Pool() const { return pool_; }
}
