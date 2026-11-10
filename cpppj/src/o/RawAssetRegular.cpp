// 실제 거주 구역/간헐천/신전의 가상 Handle와 공통 Damageable 사건을 분기별로 복원한다.
#include "o/RawAssetRegular.h"
#include "o/RawResidenceRegular.h" // PATCH 거주 구역의 같은 현재 본문을 공유한다.
#include "o/RawFactoryRegular.h" // PATCH의 같은 현재 Factory 본문을 Native와 공유한다.
#include "o/RawPathAnimation.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 정렬되지 않은 DWORD/WORD의 little endian 비트를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정한 폭의 바이트만 낮은 자리부터 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 지정 폭만 쓰고 raw 슬롯의 이웃 필드를 보존한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // WORD/DWORD의 정확한 쓰기 폭을 유지한다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// x87 ftol의 signed64 절삭 후 low DWORD를 읽는다. 비유한/범위밖 입력은 원본 CRT 범위와 구분해 거부한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0)
        throw std::out_of_range("자산 Handle 좌표 CRT 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원본 signed add/sub의 32비트 넘침을 보존한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta));
}
// 원본 GameRandom 범위 helper의 한 번 소비와 unsigned 나머지다.
std::int32_t Range(GameRandom& random,std::int32_t low,std::int32_t high) {
    return Add(low,static_cast<std::int32_t>(random.Next(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low))));
}
// 원본 float 난수는 상위 WORD/65536에 두 float 차이 절댓값을 곱하고 마지막에만 float로 저장한다.
float RandomDelay(GameRandom& random) {
    const auto sample=random.Next(65536);
    return static_cast<float>(static_cast<double>(sample)*(1.0/65536.0)*20.0+15.0);
}
// 사용하는 사건에 필요한 외부 몸체가 없으면 raw 상태 변경 전에 실패한다.
template<class Callback> void Require(const Callback& callback,const char* message) {
    if (!callback) throw std::logic_error(message);
}
}
float RunNativeGeyserRegular(SidPool& pool,std::span<const RiftTypeRecord> types,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const std::function<void(Sid,std::uint32_t)>& display,const RegularHandler& damageable) { //현재가상88을먼저실행하고실제로선택된TYPE/필드만읽는다.
    const auto update=[&] { const auto current=display;Require(current,"Native간헐천현재표시미연결");current(sid,0); };update();
    if (!event) {
        const auto ratio=[&] { const auto raw=pool.Slot(sid);const auto quantity=Read(raw,12,2);const auto type=pool.Slot(sid)[10];if (type>=types.size()) throw std::out_of_range("Native간헐천현재비용TYPE물리부족");return static_cast<double>(quantity)/static_cast<double>(types[type].cost); }; //원문의반복getType/비용조회위치를보존한다.
        const auto frame=[&] { return std::bit_cast<std::int32_t>(Read(pool.Slot(sid),36)); };std::int32_t next; //원문비용분기뒤각실제프레임읽기위치에서조회한다.
        if (ratio()>0.75 && ratio()<=1.0) { next=frame()<16 ? Add(frame(),1):0;payload=std::bit_cast<float>(0x3cf5c28fU); }
        else if (ratio()>0.5 && ratio()<=0.75) { next=frame()<32 ? Add(frame(),1):17;payload=std::bit_cast<float>(0x3d75c28fU); }
        else { next=frame()<48 ? Add(frame(),1):33;payload=std::bit_cast<float>(0x3db851ecU); }
        const auto selected=pool.Slot(sid);Write({const_cast<std::uint8_t*>(selected.data()),selected.size()},36,std::bit_cast<std::uint32_t>(next));update(); //직접DWORDstore뒤현재가상88이며별도SetFrame/Unpop을추가하지않는다.
    }
    const auto base=damageable;Require(base,"Native간헐천의실제DamageableHandle미연결");return base(sid,event,count,payload);
}
// 같은 월드의 표·프레임 지정기만 허용하고 비용·원소를 사본으로 만들지 않는다.
RawAssetRegular::RawAssetRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,std::span<const std::int32_t> elements,const SquidFrame& setter,
    const RawFrameAdvance& advance,GameRandom& random,AssetPostPopState& assets,AssetRegularState& state,AssetRegularHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),elements_(elements),setter_(setter),advance_(advance),
      random_(random),assets_(assets),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (&setter.Pool()!=&pool || &advance.Pool()!=&pool || &advance.Setter()!=&setter ||
        types.size()!=count || frames.size()!=count || elements.size()!=count)
        throw std::invalid_argument("자산 Handle 풀/프레임/타입 표 불일치");
}
// raw의 vtable 기록값만 비교하며 원본 주소를 호스트 포인터로 호출하지 않는다.
unsigned RawAssetRegular::Kind(Sid sid) const {
    const auto table=Read(pool_.Slot(sid),0);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (table==(patch ? 0x50f540U : 0x501b90U)) return 1;
    if (table==(patch ? 0x50da88U : 0x500760U)) return 2;
    if (patch ? table==0x5196f0U || table==0x5197a8U || table==0x519860U || table==0x519918U :
                table==0x501d28U || table==0x501de0U || table==0x501e98U) return 3;
    if (table==(patch ? 0x509148U : 0x503298U) || table==(patch ? 0x509210U : 0x503358U)) return 4;
    return 0;
}
// +5c가 복원된 부모인지 실제 가상 표로 확인한다.
bool RawAssetRegular::Handles(Sid sid) const { return Kind(sid)!=0; }
// frame을 판본별 실제 폭으로 읽는다.
std::int32_t RawAssetRegular::CurrentFrame(Sid sid) const {
    return pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),36)) :
        static_cast<std::int32_t>(pool_.Slot(sid)[34]);
}
// 잘못된 타입/가상 표는 외부 효과보다 먼저 거부한다.
float RawAssetRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (pool_.Slot(sid)[10]>=types_.size()) throw std::out_of_range("자산 Handle 타입 범위 오류");
    switch (Kind(sid)) {
    case 1:return Residence(sid,event,count,payload);
    case 2:return Geyser(sid,event,count,payload);
    case 3:return Vortex(sid,event,count,payload);
    case 4:return Factory(sid,event,count,payload);
    default:throw std::invalid_argument("자산 Handle 가상 표 미복원");
    }
}
// PATCH004549c0/CD0047ac20: 현재 상태의 첫프레임+Next(현재count)를 지정한 뒤 공통Handle에 넘긴다.
float RawAssetRegular::Factory(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        FactoryRegularHooks current;
        current.level=[this](auto parent) { const auto read=hooks_.virtualCount;return read ? read(parent):std::int32_t(pool_.Slot(parent)[11]>>5); };
        current.firstFrame=[this](auto parent,auto level) { if (level<0 || level>=16) throw std::out_of_range("Factory 현재 firstFrame 레벨 범위");return frames_[pool_.Slot(parent)[10]].Run(static_cast<std::uint8_t>('A'+level)).first; };
        current.frameCount=[this](auto parent,auto level) { if (level<0 || level>=16) throw std::out_of_range("Factory 현재 frameCount 레벨 범위");return frames_[pool_.Slot(parent)[10]].Run(static_cast<std::uint8_t>('A'+level)).count; };
        current.random=[this](auto limit) { if (!limit) { static_cast<void>(random_.Next(1));throw std::domain_error("Factory 원문 난수0범위 나눗셈"); }return random_.Next(limit); };
        current.frame=[this](auto parent,auto value,auto flags) { setter_.Set(parent,value,flags); };current.advance=[this](auto parent,auto delta,auto flags) { advance_.Advance(parent,delta,flags); };
        current.begin=hooks_.beginFactoryUpgrade;current.finish=hooks_.finishFactoryUpgrade;current.flare=hooks_.factoryFlare;current.damageable=[this](auto parent,auto code,auto times,auto value) { return DamageableHandle(parent,code,times,value); };
        return RunFactoryRegular(pool_,sid,event,count,payload,current);
    }
    if (event==2) {
        const auto raw=pool_.Slot(sid);const auto first=frames_[raw[10]].Run(static_cast<std::uint8_t>('A'+(raw[11]>>5))).first;
        const auto current=pool_.Slot(sid);const auto size=frames_[current[10]].Run(static_cast<std::uint8_t>('A'+(current[11]>>5))).count;
        if (size<=0) throw std::logic_error("Factory Handle 원본 상태 프레임 난수 범위0");
        setter_.Set(sid,Add(first,static_cast<std::int32_t>(random_.Next(static_cast<std::uint32_t>(size)))),0);
    } else if (event==1) {
        advance_.Advance(sid,1,0);
    } else if (event==5) {
        // CD는 사건5 분기 자체가 없고 패치만 상위 상태에서 입자효과를 호출한다.
        if (pool_.Edition()==OriginalEdition::Patch1078 && (pool_.Slot(sid)[11]>>5)>2) { Require(hooks_.factoryFlare,"Factory 상위 상태 실제 플레어 미연결");hooks_.factoryFlare(sid,2,1); }
    } else if (event==3) {
        if (pool_.Edition()!=OriginalEdition::Patch1078) throw std::logic_error("CD Factory 업그레이드 사건 미복원");
        if (count) { Require(hooks_.finishFactoryUpgrade,"Factory 실제 업그레이드 완료 미연결");return hooks_.finishFactoryUpgrade(sid); }
        Require(hooks_.beginFactoryUpgrade,"Factory 실제 업그레이드 그림 미연결");hooks_.beginFactoryUpgrade(sid,payload);advance_.Advance(sid,1,0);
    } else if (event==4 && count) return 0.0f;
    return DamageableHandle(sid,event,count,payload);
}
// 사제·보호막 등 기존 처리기를 먼저 합성할 때 미복원 자산은 fallback이 명시적으로 책임진다.
RegularHandler RawAssetRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);
        if (!fallback) throw std::invalid_argument("Regular 부모 Handle 미연결");
        return fallback(sid,event,count,payload);
    };
}
// 원본 거주 구역은 마지막 base Handle에서 payload를 그대로 반환한다.
float RawAssetRegular::Residence(Sid sid,std::uint32_t event,std::uint32_t,float payload) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch) {
        ResidenceRegularHooks current;current.random=[this]()->GameRandom& { return random_; };current.notify=hooks_.notifySurface;current.frame=[this](auto parent,auto value,auto flags) { setter_.Set(parent,value,flags); };current.surface=hooks_.surface;current.owner=hooks_.setOwner;current.dude=hooks_.residenceEvent2;
        current.element=[this](auto temple) { const auto type=pool_.Slot(temple)[10];if (type>=elements_.size()) throw std::out_of_range("거주 구역 현재 신전 원소 범위");return elements_[type]; };current.themeMode=[this] { const auto get=hooks_.themeMode;return get ? get():std::uint32_t(state_.themeMode); };current.decorationType=[this] { return state_.decorationType; };current.recordScan=[this](auto value) { if (std::bit_cast<std::int32_t>(state_.maxDecorationScan)<=value) state_.maxDecorationScan=std::bit_cast<std::uint32_t>(value); };
        return RunResidenceRegular(pool_,hash_,types_,frames_,assets_.Temples(),sid,event,0,payload,current);
    }
    if (event==0) {
        Require(hooks_.notifySurface,"거주 구역 주민 통지 몸체 누락");
        auto raw=pool_.AllocatedBytes(sid);const auto word=Read(raw,12,2);
        Write(raw,12,(word&0xfff0U)|((word&15U)==0 ? 1U : 0U),2);
        hooks_.notifySurface(sid);
        const auto current=CurrentFrame(sid);const auto even=patch ? static_cast<std::uint32_t>((current/2)*2) : static_cast<std::uint32_t>(current)&~1U;
        setter_.Set(sid,std::bit_cast<std::int32_t>(even+(pool_.Slot(sid)[12]&15U)),0);
        return RandomDelay(random_);
    }
    if (event==1) {
        const auto raw=pool_.Slot(sid);const auto x=static_cast<double>(std::bit_cast<float>(Read(raw,14)));
        const auto y=static_cast<double>(std::bit_cast<float>(Read(raw,18)));
        auto searchX=Range(random_,Truncate(x-48.0),Truncate(x+32.0));
        // 두 cdecl 인수가 스택에 남은 뒤의 +20은 Y다. 두 판본 모두 Y-32부터 Y+32 사이에서 고른다.
        auto searchY=Range(random_,Truncate(y-32.0),Truncate(y+32.0));
        searchX=std::clamp(searchX,10,246);searchY=std::clamp(searchY,10,246);
        const auto bottom=Truncate(std::min(static_cast<double>(static_cast<float>(y+32.0)),246.0));
        RawSquidFinder finder(pool_,hash_,types_);auto candidate=finder.Begin({searchX-1,searchY-3,searchX+1,bottom});
        bool changed=false;std::uint32_t checked=0;
        // 최초 바뀔 장식까지만 조사하되 원본처럼 변경 직후 Next를 한 번 실행한다.
        while (candidate.value && !changed) {
            ++checked;
            const auto object=pool_.Slot(candidate);const auto type=object[10];
            if (type==state_.decorationType) {
                const auto current=CurrentFrame(candidate);const auto& codes=frames_[type].Codes();
                if (current<0 || static_cast<std::size_t>(current)>=codes.size()) throw std::out_of_range("거주 구역 장식 프레임 오류");
                const auto flags=static_cast<std::int8_t>(codes[static_cast<std::size_t>(current)].flags);
                if (flags&8) { setter_.Set(candidate,Add(current,1),0);changed=true; }
                else if (flags&16) { setter_.Set(candidate,Add(current,-1),0);changed=true; }
            }
            candidate=finder.Next();
        }
        state_.maxDecorationScan=std::max(state_.maxDecorationScan,checked);return 1.0f;
    }
    if (event==2) { Require(hooks_.residenceEvent2,"거주 구역 event2 몸체 누락");hooks_.residenceEvent2(sid); }
    if (event==3 && patch) {
        Require(hooks_.surface,"거주 구역 지면 조회 누락");Require(hooks_.setOwner,"거주 구역 소유자 지정 누락");
        const auto raw=pool_.Slot(sid);const auto support=hooks_.surface(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
        if (support.value) hooks_.setOwner(sid,pool_.Slot(support)[34]);
        auto theme=0;
        // 원본0049a4e0의005e189c/005e18a4와 같은 현재 공유 신전 목록에서 첫 소유자 일치를 읽는다.
        // 이력: 2026-10-27 — Session이 장기 목록을 빌릴 때 비어 있는 fallback을 읽어 기본 초록 프레임만 선택하던 오류를 수정했다.
        for (const auto value:assets_.Temples().Items()) {
            const auto temple=pool_.Slot(Sid{static_cast<std::uint16_t>(value)});
            if (temple[34]!=pool_.Slot(sid)[34]) continue;
            const auto type=temple[10];if (type>=elements_.size()) throw std::out_of_range("거주 구역 신전 원소 오류");
            const auto element=elements_[type]; // 원본004ac970은 설정 조회 전에 현재 TYPE+98을 읽는다.
            const bool enabled=hooks_.themeMode ? hooks_.themeMode()!=0 : state_.themeMode; // 실제 옵션 조회는 첫 일치 신전에서만 수행한다.
            if (enabled) theme=element==2 ? 1 : element==0 ? 2 : element==1 ? 3 : 0;
            break;
        }
        setter_.Set(sid,Add(std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),42)),theme*6),0);
        const auto frame=CurrentFrame(sid);
        setter_.Set(sid,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>((frame/2)*2)+(pool_.Slot(sid)[12]&15U)),0);
        return 0.1f;
    }
    return payload;
}
// 패치 간헐천은 직접 frame DWORD를 쓰며 CD는 실제 공통 frame 진행기를 사용한다.
float RawAssetRegular::Geyser(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch) { Require(hooks_.displayUpdate,"간헐천 표시 갱신 누락");hooks_.displayUpdate(sid,0); }
    if (event==0) {
        if (!patch) advance_.Advance(sid,1,0);
        const auto raw=pool_.Slot(sid);const auto ratio=static_cast<double>(Read(raw,12,2))/static_cast<double>(types_[raw[10]].cost);
        if (patch) {
            const auto frame=CurrentFrame(sid);std::int32_t next;
            if (ratio>0.75 && ratio<=1.0) { next=frame<16 ? Add(frame,1) : 0;payload=0.03f; }
            else if (ratio>0.5 && ratio<=0.75) { next=frame<32 ? Add(frame,1) : 17;payload=0.06f; }
            else { next=frame<48 ? Add(frame,1) : 33;payload=0.09f; }
            Write(pool_.AllocatedBytes(sid),36,std::bit_cast<std::uint32_t>(next));hooks_.displayUpdate(sid,0);
        } else {
            const auto delay=static_cast<double>(0.19f)-ratio*static_cast<double>(0.12f);
            // CD x87 unordered 비교도 상한을 선택한다. std::min(NaN,상한)은 이 결과를 보존하지 않는다.
            payload=static_cast<float>(delay<static_cast<double>(0.03f) ? delay : static_cast<double>(0.03f));
        }
    }
    return DamageableHandle(sid,event,count,payload);
}
// 실제 신전의 event0 Ascend 정리·event1/2 애니메이션·event3/4 이탈 처리를 같은 필드/목록으로 실행한다.
float RunVortexRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,AssetRegularState& state,const AssetRegularHooks& hooks,const VortexRegularDevices& devices,const RegularHandler& damageable,Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
    const auto currentFrame=[&pool,sid] { return pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(pool.Slot(sid),36)):static_cast<std::int32_t>(pool.Slot(sid)[34]); }; // 현재 raw 프레임 폭만 읽으며 표시 장치는 조회하지 않는다.
    const bool patch=pool.Edition()==OriginalEdition::Patch1078;const auto ownerOffset=patch ? 34U : 32U;
    if (event==0) { Require(hooks.ascendTick,"신전 Ascend 정리 Handle 누락");return hooks.ascendTick(sid,count) ? payload : -1.0f; }
    if (event==1) {
        auto delta=1;
        if (state.FortMode()) { const auto type=pool.Slot(sid)[10];const auto side=RawPathAnimation::Side(frames[type],currentFrame());delta=Add(frames[type].Run(static_cast<std::uint8_t>(side)).count,-1); }
        Require(devices.advance,"신전 현재 프레임 진행 미연결");devices.advance(sid,delta,kFrameUpdateAlternate);return payload;
    }
    if (event==2) {
        if ((count&31U)==31U) { Require(devices.frame,"신전 현재 프레임 지정 미연결");devices.frame(sid,0,kFrameUpdateAlternate);return 2.0f; }
        if (!currentFrame()) { Require(devices.frame,"신전 현재 프레임 지정 미연결");devices.frame(sid,1,kFrameUpdateAlternate); }
        Require(devices.random,"신전 현재 게임 난수 미연결");Require(devices.advance,"신전 현재 프레임 진행 미연결");devices.advance(sid,static_cast<std::int32_t>(devices.random(2)+1),kFrameUpdateAlternate);return 0.1f;
    }
    if (event==3) {
        const auto signedCount=std::bit_cast<std::int32_t>(count);
        if (!count && Read(pool.Slot(sid),12,2)==2) return 0.0f;
        Require(hooks.countForeignOwnedIslands,"신전 이탈 다른 소유자 섬 조회 누락");
        if (!count) { Require(hooks.playerLeavingNotice,"신전 이탈 알림 누락");if (hooks.state) hooks.state(sid,2);else Write(pool.AllocatedBytes(sid),12,2,2);hooks.playerLeavingNotice(pool.Slot(sid)[ownerOffset]); }
        static_cast<void>(hooks.countForeignOwnedIslands(pool.Slot(sid)[ownerOffset]));
        if (signedCount<(state.Boss() ? 3 : 23)) return 1.0f;
        if (!(hooks.suppressLeaveRequest ? hooks.suppressLeaveRequest() : state.suppressLeaveRequest)) {
            Require(hooks.islandNumber,"신전 이탈 섬 조회 누락");Require(hooks.requestLeave,"신전 이탈 요청 누락");
            const auto raw=pool.Slot(sid);const auto island=hooks.islandNumber(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
            hooks.requestLeave(sid,island&255U);if (pool.Slot(sid)[11]&1U) return -1.0f;
        }
        return 0.0f;
    }
    if (event==4) {
        const auto signedCount=std::bit_cast<std::int32_t>(count);
        if (!count) { if (hooks.state) hooks.state(sid,4);else Write(pool.AllocatedBytes(sid),12,4,2); }
        if (!state.checkingForms && static_cast<float>(signedCount)<=4.0f) return 1.0f;
        const auto raw=pool.Slot(sid);
        if (hooks.descend) hooks.descend(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
        else { // 기존 독립 fixture의 명시 입력 경계만 보존한다. 실제 Client는 위의 Descend 본체를 사용한다.
            Require(hooks.regionNumber,"신전 event4 지역 조회 누락");Require(hooks.removeAnimation,"신전 event4 통지 누락");
            const auto region=hooks.regionNumber(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
            const auto token=(state.regionGeneration<<16)|region;hooks.removeAnimation(state.animationType,token);
        }
        if (hooks.state) hooks.state(sid,0);else Write(pool.AllocatedBytes(sid),12,0,2);return 0.0f;
    }
    Require(damageable,"신전 현재 Damageable 부모 미연결");return damageable(sid,event,count,payload);
}
// 독립 Typed는 기존 효과와 같은 현재 멤버를 읽고 공통 원문 본문에만 위임한다.
float RawAssetRegular::Vortex(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    VortexRegularDevices devices;devices.frame=[this](auto parent,auto frame,auto flags) { setter_.Set(parent,frame,flags); };devices.advance=[this](auto parent,auto delta,auto flags) { static_cast<void>(advance_.Advance(parent,delta,flags)); };devices.random=[this](auto limit) { return random_.Next(limit); };
    return RunVortexRegular(pool_,frames_,state_,hooks_,devices,[this](auto parent,auto code,auto times,auto value) { return DamageableHandle(parent,code,times,value); },sid,event,count,payload);
}
// Damageable의 기본 사건만 payload 반환을 사용한다. 지정된 두 사건은 실제 하위 효과를 요구한다.
float RawAssetRegular::DamageRegular(Sid sid,std::uint32_t count,bool native) const { //사건판정뒤실제피해지면/HP/현재genus본문을같은원문순서로공유한다.
    Require(hooks_.damageGround,"Damageable 지면 피해 몸체 누락");if (!native) Require(hooks_.hitPoints,"Damageable 가상 HP 몸체 누락"); //typed의기존선검사만보존한다.
    const auto raw=pool_.Slot(sid);hooks_.damageGround(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
    const auto source=std::bit_cast<std::int32_t>(count)>>16;if (native) Require(hooks_.hitPoints,"NativeDamageable의현재HP종단미연결");const auto result=hooks_.hitPoints(sid,count&65535U,source,0);
    if (!result && !(types_[pool_.Slot(sid)[10]].flags2&0x200000U)) return -1.0f;return 0.0f;
}
float RawAssetRegular::DamageableHandle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (event==state_.damageEvent) return DamageRegular(sid,count);
    if (event==state_.constructionEvent) {
        const auto extra=pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U;
        if (pool_.Slot(sid)[extra]&1U) {
            Require(hooks_.findAttached,"Damageable 건설 프로세스 조회 누락");
            if (!hooks_.findAttached(sid,state_.constructionProcessType).value) { Require(hooks_.startConstruction,"Damageable abstract 건설 시작 누락");hooks_.startConstruction(sid,0.0f); }
        }
        return 0.0f;
    }
    return payload;
}
// runtime 연결이 동일한 단일 풀인지 검사하는 참조다.
const SidPool& RawAssetRegular::Pool() const { return pool_; }
}
