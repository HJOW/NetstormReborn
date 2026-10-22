// 원본의 signed 비교·float 저장 지점·효과 뒤 현재 프레임 재조회 순서를 보존한다.
#include "o/RawDudeRegular.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 누락된 원본 효과는 선택한 시점에만 진단한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("Dude 정기 실제 효과 미연결");return effect; }
// 비정렬 raw의 지정 폭을 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 주변 필드를 합치거나 호스트 포인터로 해석하지 않는다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);return value;
}
}
// 원본 초기 생성자와 같은 capacity/count이며 이 소유권은 Session 수명에 묶인다.
void InitializeDudePopulation(SquidPostPopList& population) { if (!population.entries.empty() || population.count) throw std::logic_error("Dude population 중복 초기화");population.entries.resize(kDudePopulationCapacity); }
// 다른 판본/누락된 Run 배열을 첫 실행 전에 거부한다.
RawDudeRegular::RawDudeRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,GameRandom& random,DudeRegularState state,DudeRegularHooks hooks)
    :pool_(pool),frames_(frames),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188) throw std::invalid_argument("Dude 정기 판본/프레임 연결 오류");static_cast<void>(state_.providers.Items());
}
// 가상+5c=450fa0인 실제 부모만 이 분배에 들어온다.
bool RawDudeRegular::Handles(Sid sid) const { const auto raw=pool_.Slot(sid);return raw[10]==70 && Read(raw,0)==0x508cc8U; }
// free는 Handle의 원래 fallback 관문이 읽으므로 여기서 제거하지 않는다.
std::span<const std::uint8_t> RawDudeRegular::Raw(Sid sid) const { if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("Dude 정기 SID 범위");const auto raw=pool_.Slot(sid);if (raw[10]>=frames_.size()) throw std::out_of_range("Dude 정기 현재 타입 범위");return raw; }
// 효과 호출 뒤 현재 위치를 읽을 때도 같은 raw만 사용한다.
RawPathPoint RawDudeRegular::Position(Sid sid) const { const auto raw=Raw(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// signed side의 의미와 범위 진단은 기존 원본 프레임 조회를 재사용한다.
std::int32_t RawDudeRegular::Stage(Sid sid) const { const auto raw=Raw(sid);return RawPathAnimation::Direction(frames_[raw[10]],std::bit_cast<std::int32_t>(Read(raw,36))); }
// WORD task와 BYTE35 및 frame DWORD의 이웃 바이트를 보존한다.
void RawDudeRegular::Write(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const {
    auto raw=pool_.AllocatedBytes(sid);
    // 원본 폭만 낮은 바이트부터 쓴다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 진단 callback이 복귀하면 원본처럼 목표 진입점과 이동 호출을 계속한다.
bool RawDudeRegular::MoveTo(Sid sid,Sid target,bool quickly) const { if (!target.value) return false;if (Raw(target)[11]&3U) Need(hooks_.assertion)(0xde);return Need(hooks_.move)(sid,Need(hooks_.entrance)(target),quickly); }
// 난수 호출 순서를 바꾸지 않고 원본 공급자 active 목록의 현재 항목을 선택한다.
float RawDudeRegular::Wander(Sid sid,std::int32_t count,bool quickly) const {
    if (!count || random_.Next(100)>=10) {
        const auto providers=state_.providers.Items();
        if (!providers.empty()) {
            const auto next=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.providerIndex)+random_.Next(1)+1U); //원본4558f0(1,2)는 상한 제외라 간격은1이며 난수는 한 번 소비한다.
            state_.providerIndex=next%static_cast<std::int32_t>(providers.size());
            if (state_.providerIndex<0) throw std::out_of_range("Dude 공급자 순환 음수 색인");const auto number=providers[static_cast<std::size_t>(state_.providerIndex)];
            if (number>0xffffU) throw std::out_of_range("Dude 공급자 SID 폭");if (MoveTo(sid,Sid{static_cast<std::uint16_t>(number)},quickly)) return 10.0F;
        }
    }
    Need(hooks_.task)(sid,0x5103,0);return 0.1F;
}
// path 인자는 원래 전달되지만451210 몸체에서는 읽지 않는다.
float RawDudeRegular::Return(Sid sid,Sid path,std::int32_t count) const { (void)path;if (count>4) { Need(hooks_.destroy)(sid,0);return -1.0F; }return MoveTo(sid,Need(hooks_.nearestResidence)(sid),false) ? 10.0F : 2.0F; }
// 원래 위치는 공간 효과 전에 보존하고 이후 현재 프레임/가상 Pop은 기존 월드가 처리한다.
float RawDudeRegular::Drop(Sid sid,Sid path,std::int32_t count) const {
    if (count) { if (state_.checking && !path.value) Need(hooks_.assertion)(0x127);return 1.0F; }
    const auto origin=Position(sid);const auto residence=Need(hooks_.nearestResidence)(sid);
    if (residence.value) { Need(hooks_.unpop)(sid,0);const auto point=Need(hooks_.entrance)(residence);Need(hooks_.pop)(sid,point,0);if (Need(hooks_.move)(sid,origin,false)) return 1.0F; }
    Need(hooks_.destroy)(sid,0);return -1.0F;
}
// 원본의 fild→곱/합→float 저장과 unordered y 상한을 같은 순서로 처리한다.
float RawDudeRegular::Fall(Sid sid,std::int32_t count) const {
    if (Raw(sid)[11]&2U) return 0;if (count>100) { Need(hooks_.destroy)(sid,0);return -1.0F; }if (count==10) Write(sid,35,20,1);
    if (Stage(sid)==8) { const auto type=Raw(sid)[10];Need(hooks_.setFrame)(sid,frames_[type].Run('A').first,0); }
    const auto point=Position(sid);float y=static_cast<float>((static_cast<double>(count)*0.5+1.0)*0.0625+static_cast<double>(point.y));if (254.0F<y) y=254.0F;
    Need(hooks_.unpop)(sid,0);const auto direction=Stage(sid);if (direction<0 || direction>15) throw std::out_of_range("Dude 낙하 단계 범위");const auto run=frames_[Raw(sid)[10]].Run(static_cast<std::uint8_t>('A'+direction));
    if (run.count<=0) throw std::out_of_range("Dude 낙하 현재 Run0");const auto frame=static_cast<std::uint32_t>(run.first)+random_.Next(static_cast<std::uint32_t>(run.count));Write(sid,36,frame,4);Need(hooks_.pop)(sid,{point.x,y},0x40);return 0.05F;
}
// 원본의 poll 우선·free/마스크 fallback·낙하 우선·경로 대기·task 쓰기 순서다.
float RawDudeRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("Dude 정기 실제 타입/가상 표 오류");if (event==state_.pollEvent) return 0;
    if ((Raw(sid)[11]&1U) || (event&0xfff0U)!=0x5100U) { if (state_.checking) Need(hooks_.assertion)(399);return payload; }
    const auto signedCount=std::bit_cast<std::int32_t>(count);if (event!=0x5105 && Need(hooks_.checkSpace)(sid)) return Fall(sid,signedCount);
    const auto path=Need(hooks_.findPath)(sid,state_.pathType);if (path.value && count && event!=0x5107) return payload;Write(sid,12,event,2);
    switch (event) {
    case 0x5101:Need(hooks_.assertion)(0x179);[[fallthrough]];
    case 0x5102:return Wander(sid,signedCount,false);
    case 0x5103:return Return(sid,path,signedCount);
    case 0x5104:return Drop(sid,path,signedCount);
    case 0x5105:return Fall(sid,signedCount);
    case 0x5106:case 0x5107:
        if (signedCount>10) { Need(hooks_.destroy)(sid,0);return -1.0F; }if (event==0x5107 && !path.value) static_cast<void>(Wander(sid,0,true));return 0.2F;
    default:Need(hooks_.assertion)(0x185);return payload;
    }
}
// composition root의 같은 풀 연결을 확인한다.
const SidPool& RawDudeRegular::Pool() const { return pool_; }
}
