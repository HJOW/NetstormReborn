// 원문 대상 개수/현재 조회와 float 저장 위치를 유지한다. 일반 주문 피해로 대체하지 않는다.
#include "o/RawMeltdown.h"
#include "o/RawGunImpact.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //10.78 물리와 실제 원문 효과만 다룬다.
namespace { // 패킷 폭과 선택된 외부 효과의 지연 계약이다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Meltdown 실제 하위 미연결");return hook; } // 미선택 하위를 요구하지 않는다.
std::uint32_t Get(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { // 패킷의 지정 BYTE/WORD/DWORD만 조합한다.
    if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("Meltdown 패킷 읽기 범위");std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(raw[at+byte])<<(byte*8);return result; // 낮은 바이트부터 원문 폭을 읽는다.
}
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width=4) { // 패킷의 지정 폭만 쓰며 ff 채움을 보존한다.
    if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("Meltdown 패킷 쓰기 범위");
    for (std::size_t byte=0;byte<width;++byte) raw[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문의 little endian이다.
}
void Header(std::span<std::uint8_t> packet,FormPacketHeader header) { Put(packet,0,header.protocol&0x7f,1);Put(packet,1,header.message,1);Put(packet,2,std::max<std::uint16_t>(4,header.length),2); } // 실제 df590/df230의 헤더 폭이다.
float Snap(float value,bool upper) { const double adjusted=static_cast<double>(value)+(upper ? 0.9999899864196777 : 0.0);const auto low=(!std::isfinite(adjusted) || adjusted<-0x1p63 || adjusted>=0x1p63) ? 0U : static_cast<std::uint32_t>(static_cast<std::int64_t>(adjusted));return static_cast<float>(std::bit_cast<std::int32_t>(low)); } //41d770/41d7a0의_ftol 하위DWORD를float로 저장한다.
}
// 현재188TYPE/프레임과 동맹81칸만 받아 다른 판본 물리를 혼합하지 않는다.
RawMeltdown::RawMeltdown(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,MeltdownState& state,MeltdownHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || state.alliances.size()!=81) throw std::invalid_argument("Meltdown 현재10.78 물리/TYPE/동맹 계약");
}
std::uint32_t RawMeltdown::Read(Sid sid,std::size_t offset,std::size_t width) const { return Get(pool_.Slot(sid),offset,width); } // free/dead 필터는 각 원문 소비자에 맡긴다.
const RiftTypeRecord& RawMeltdown::Type(Sid sid) const { const auto type=Read(sid,10,1);if (type>=types_.size()) throw std::out_of_range("Meltdown 현재 TYPE 범위");return types_[type]; } // 현재 TYPE를 조회한다.
std::array<float,2> RawMeltdown::Center(Sid sid) const { const auto& type=Type(sid);return {static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,14)))-type.footX*.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,18)))-type.footY*.5)}; } //4adea0의 저장 위치다.
// 원문은 release를 이 객체 몸체에서 거부하지 않는다. 일반 입력의 앞선 관문과 구별한다.
void RawMeltdown::Command(Sid parent,std::uint32_t code,const std::function<void(Sid,std::uint32_t)>& base) const {
    if ((code&0x03ffffffU)==0x030000f8U) { std::array<std::uint8_t,6> packet{};Header(packet,state_.request);Put(packet,4,parent.value,2);Need(hooks_.send)(9,packet); }
    Need(base)(parent,code);
}
// 다리는 보호 프레임 비트40만 제외한다. 다른 대상은 HP값이 아니라 HP 보유 여부를 사용한다.
bool RawMeltdown::MayTarget(Sid target) const {
    const auto genus=Type(target).flags2;
    if (genus&4U) { if (!target.value) return true;const auto frame=std::bit_cast<std::int32_t>(Read(target,36));const auto codes=frames_[Read(target,10,1)].Codes();if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("Meltdown 보호 프레임 범위");return !(codes[static_cast<std::size_t>(frame)].flags&0x40U); }
    return Need(hooks_.hasHitPoints)(target) && (genus&0x80074200U)!=0;
}
// 원문은 후보의 dead를 여기서 거르지 않으며 거리의 등호는 포함하지 않는다.
void RawMeltdown::Find(float x,float y,std::int32_t radius,BombTargetList& output) const {
    if (!Need(state_.boss)()) Need(hooks_.assertion)(0x664);
    const float r=static_cast<float>(radius);const auto low=RawGunImpact::Clamp(static_cast<float>(static_cast<double>(x)-r),static_cast<float>(static_cast<double>(y)-r));
    const auto high=RawGunImpact::Clamp(static_cast<float>(static_cast<double>(r)+x),static_cast<float>(static_cast<double>(r)+y));
    RawSquidFinder finder(pool_,hash_,types_);const SquidSearchArea area{static_cast<int>(Snap(low[0],false)),static_cast<int>(Snap(low[1],false)),static_cast<int>(Snap(high[0],true)),static_cast<int>(Snap(high[1],true))};
    for (auto sid=finder.Begin(area);sid.value;sid=finder.Next()) { // 같은 네 단계 해시의 실제 순서를 따른다.
        if (sid.value>=5 && sid.value<15000) continue;const float limit=static_cast<float>(static_cast<double>(Type(sid).footX)*.5+r);
        if (Read(sid,40,1)&1U) continue;const auto center=Center(sid);const double dx=static_cast<double>(x)-center[0],dy=static_cast<double>(y)-center[1];
        if (!(std::sqrt(dx*dx+dy*dy)<static_cast<double>(limit)) || !MayTarget(sid)) continue;
        if (output.count<output.capacity) { if (output.count<0 || static_cast<std::size_t>(output.count)>=output.storage.size()) throw std::out_of_range("Meltdown 실제 대상 버퍼 범위");output.storage[static_cast<std::size_t>(output.count++)]=sid.value; }
    }
}
// 피해/삭제 뒤 현재 슬롯을 다시 읽지 않고 원문처럼 먼저 캡처한 중심을 지면 효과에 사용한다.
void RawMeltdown::Apply(float x,float y,std::int32_t radius,std::uint32_t owner,Sid target,Sid parent) const {
    if (Read(target,11,1)&2U) return;const auto genus=Type(target).flags2;const auto center=Center(target);
    if (!(genus&4U)) { const auto victim=Read(target,34,1);if (owner>=9 || victim>=9) throw std::out_of_range("Meltdown 동맹 owner 범위");if (!state_.alliances[owner*9+victim]) { Need(hooks_.damage)(target,100,owner,parent);Need(hooks_.ground)(center[0],center[1]); }return; }
    const float sum=static_cast<float>(static_cast<double>(Type(target).footX)*.5+static_cast<double>(radius));const double dx=static_cast<double>(x)-center[0],dy=static_cast<double>(y)-center[1]; // 원문 fiadd는 정수 반경을 먼저float로 축소하지 않는다.
    if (std::sqrt(dx*dx+dy*dy)<=static_cast<double>(sum)-1.0) { Need(hooks_.destroy)(target,0x100048);return; }
    if (Need(state_.boss)()) Need(hooks_.edge)(target,x,y);
}
// 검색은 최초 중심으로 하고 패킷에는 검색 뒤 현재 부모의 중심/owner를 다시 읽어 넣는다.
void RawMeltdown::ReceiveRequest(std::span<const std::uint8_t> packet) const {
    const Sid parent{static_cast<std::uint16_t>(Get(packet,4,2))};if (Read(parent,11,1)&2U) return;
    if (!Need(state_.boss)()) Need(hooks_.assertion)(0x6c3);
    std::vector<std::uint32_t> storage(100);BombTargetList list{storage,100,0};const auto initial=Center(parent);Need(hooks_.find)(initial[0],initial[1],4,list);
    std::array<std::uint8_t,220> effect;effect.fill(255);Header(effect,state_.effect);const auto center=Center(parent);
    Put(effect,4,std::bit_cast<std::uint32_t>(center[0]));Put(effect,8,std::bit_cast<std::uint32_t>(center[1]));Put(effect,12,4,1);Put(effect,13,Read(parent,34,1),1);Put(effect,14,parent.value,2);Put(effect,16,std::bit_cast<std::uint32_t>(list.count));
    for (std::int32_t index=0;index<list.count;++index) { // 디컴파일에서 사라진 실제4208b8의 WORD 복사 루프다.
        if (static_cast<std::size_t>(index)>=storage.size()) throw std::out_of_range("Meltdown 검색 뒤 대상 수 범위");Put(effect,20+static_cast<std::size_t>(index)*2,storage[static_cast<std::size_t>(index)],2);
    }
    Need(hooks_.broadcast)(effect);Need(hooks_.receive)(Need(state_.localOwner)(),0,effect);Need(hooks_.destroy)(parent,0);
}
// 시각효과 세 개와 소리를 먼저 실행하며 각 대상과 수/owner/반경은 원문 위치에서 다시 읽는다.
void RawMeltdown::ReceiveEffect(std::span<const std::uint8_t> packet) const {
    float x=static_cast<float>(static_cast<double>(Snap(std::bit_cast<float>(Get(packet,4)),true))-.5),y=static_cast<float>(static_cast<double>(Snap(std::bit_cast<float>(Get(packet,8)),true))-.5);
    const auto first=RawGunImpact::Clamp(static_cast<float>(static_cast<double>(x)-Get(packet,12,1)),static_cast<float>(static_cast<double>(y)-Get(packet,12,1)));
    Need(hooks_.visual)(first[0],first[1],1,std::bit_cast<float>(0x3d23d70aU));
    const auto second=RawGunImpact::Clamp(static_cast<float>(static_cast<double>(Get(packet,12,1))+x),y);Need(hooks_.visual)(second[0],second[1],1,std::bit_cast<float>(0x3d23d70aU));
    const auto third=RawGunImpact::Clamp(x,static_cast<float>(static_cast<double>(Get(packet,12,1))+y));Need(hooks_.visual)(third[0],third[1],1,std::bit_cast<float>(0x3d23d70aU));Need(hooks_.sound)("whirligigFire-1000.wav");
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Get(packet,16));++index) { // 효과 뒤 바뀐 현재 목록 수도 원문처럼 다시 읽는다.
        const Sid target{static_cast<std::uint16_t>(Get(packet,20+static_cast<std::size_t>(index)*2,2))};if (Read(target,11,1)&3U) continue;
        Apply(x,y,static_cast<std::int32_t>(Get(packet,12,1)),Get(packet,13,1),target,Sid{static_cast<std::uint16_t>(Get(packet,14,2))});
    }
}
// 원본은 쓰기 요청일 때만 현재 count로 WORD 길이를 갱신하며 count=-1을 진단한다.
void RawMeltdown::FinalizePacket(std::span<std::uint8_t> packet,bool writing) { if (!writing) return;const auto count=Get(packet,16);if (count==0xffffffffU) throw std::logic_error("원본 Meltdown count 진단b23");Put(packet,2,(count+10U)*2U,2); }
}
