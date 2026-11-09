// 실제 정기 분기는 공유하고 큰 의식은 선택한 현재 처리기에서 실행한다.
#include "o/RawRitualRegular.h"
#include "o/RawPathAnimation.h"
#include "o/Player.h" // 원문 동맹 인덱스의9행/9열 범위를 같은 상수로 검증한다.
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 원문 분배에 임의의 TYPE/free/dead 선검사를 추가하지 않는다.
namespace { // 원문 폭의 필드와 선택한 함수 사본만 보조한다.
template<class F> F Need(const F& function) { if (!function) throw std::logic_error("의식 정기의 선택된 실제 효과 미연결");return function; } // 호출 중 장치가 바뀌어도 사본을 복귀까지 유지한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { // 비정렬 네 바이트만 낮은 자리부터 조합한다.
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[at+byte])<<(byte*8);return value; // 반복은 인접 필드를 읽지 않는다.
}
}
float RunRuneRegular(const SidPool& pool,Sid sid,std::uint32_t event,float payload,const RuneRegularHooks& hooks) { //304에서만 검색/진행/삭제를 실행하고 나머지는 원문base 그대로 반환한다.
    if (event!=0x304) return payload;const auto dais=Need(hooks.findDais)(sid);if (dais.value && dais.value<pool.Capacity()) Need(hooks.progress)(dais);Need(hooks.destroy)(sid,0x48);return -1.0F;
}
float RunDaisHandle(const SidPool& pool,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const DaisHandleHooks& hooks) { // 저장한 frame과 next를 현재 단계 조회 두 번에 원문 순서로 전달한다.
    if (event==0x305) { const auto raw=pool.Slot(sid);const auto frame=pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)):std::int32_t(raw[34]);const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(frame)+1);const auto nextStage=Need(hooks.stage)(sid,next),stage=Need(hooks.stage)(sid,frame);Need(hooks.frame)(sid,next,0);return stage==nextStage ? 0.1F:0; }
    if (event==0x306) return Need(hooks.sacrificeAdvance)(sid);if (event==0x307 && count) { Need(hooks.ritual)(sid);return 0; }return Need(hooks.damageable)(sid,event,count,payload);
}
}

namespace netstorm::o { // 실제 제단의 프레임/난수/선택 효과 순서를 공용 본문에 둔다.
float RunAltarRegular(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid altar,std::uint32_t event,std::uint32_t count,float payload,const AltarSacrificeState& state,const AltarRegularHooks& hooks) {
    constexpr float kFinishedDelay=3.0F,kStrugglePayload=0.08F; // 원문 지연 상수다.
    constexpr std::uint32_t kSacrificeDestroy=0x400000; // 실제 다이스 최종 삭제 flags다.
    constexpr std::array<std::string_view,5> kChains{"chainRattle23.wav","chainRattle24.wav","chainRattle25.wav","chainRattle26.wav","chainRattle27.wav"}; // 실제 다섯 소리 선택표다.
    constexpr std::array<std::string_view,3> kStruggles{"priestStruggleFade01.wav","priestStruggleFade02.wav","priestStruggleFade03.wav"}; // 실제 세 소리 선택표다.
    const auto valid=[&pool](Sid sid) { return sid.value>0 && sid.value<pool.Capacity(); }; // 원문 유효 주소 범위만 판정한다.
    const auto position=[&pool](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }; // 선택 시 현재 두 좌표를 읽는다.
    const auto direction=[&pool,frames](Sid sid) { const auto raw=pool.Slot(sid);if (raw[10]>=frames.size()) throw std::out_of_range("제단 현재 방향 TYPE 물리 범위");return RawPathAnimation::Direction(frames[raw[10]],pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(Read(raw,36)):std::int32_t(raw[34])); }; // 실제 현재 TYPE/프레임 코드다.
    const auto firstFrame=[&pool,frames](Sid sid,std::uint8_t letter) { const auto type=pool.Slot(sid)[10];if (type>=frames.size()) throw std::out_of_range("제단 현재 frame TYPE 물리 범위");return frames[type].Run(letter).first; }; // 저장한 대상의 현재 글자 첫번호다.
    auto dais=Need(hooks.dais)(altar);std::array<float,2> center{};const bool hasDais=valid(dais);if (hasDais) { const auto point=position(dais);center={static_cast<float>(static_cast<double>(point[0])-kFinishedDelay),static_cast<float>(static_cast<double>(point[1])-kFinishedDelay)}; }
    if (event==21) {
        if (pool.Edition()==OriginalEdition::Patch1078) { const auto captured=Need(hooks.priest)(dais);if (Need(hooks.boss)() && valid(captured)) { const std::uint32_t oldOwner=pool.Slot(captured)[12]&127,owner=pool.Slot(dais)[34];if (oldOwner>kPlayerCount || owner>kPlayerCount) throw std::out_of_range("제단 자동 해방 동맹 인덱스");if (Need(hooks.alliance)(oldOwner,owner)) { Need(hooks.release)(altar);return -1.0F; } } }
        if (direction(altar)==15) return payload;std::uint32_t nextEvent=20;if (Need(hooks.randomInt)(0,100000)/1000<state.escapePercent) { nextEvent=22;Need(hooks.advance)(altar,-1,0); }if (Need(hooks.reserveShared)()) Need(hooks.addShared)(altar,nextEvent,kStrugglePayload);
        if (hasDais && direction(dais)!=10) { const auto chain=Need(hooks.randomInt)(0,5);if (chain<0 || chain>=static_cast<std::int32_t>(kChains.size())) throw std::out_of_range("제단 chain 난수 범위");Need(hooks.sound)(center[0],center[1],kChains[static_cast<std::size_t>(chain)],0);const auto struggle=Need(hooks.randomInt)(0,3000)/1000;if (struggle<0 || struggle>=static_cast<std::int32_t>(kStruggles.size())) throw std::out_of_range("제단 struggle 난수 범위");Need(hooks.sound)(center[0],center[1],kStruggles[static_cast<std::size_t>(struggle)],0); }
        return static_cast<float>(static_cast<double>(Need(hooks.randomFloat)(0,2))+state.struggleSeconds);
    }
    if (event==20 || event==22) { if (direction(altar)!=14) return 0;const std::int32_t delta=event==20 ? 1 : -1;if (Need(hooks.advance)(altar,delta,0)) { if (delta==-1) Need(hooks.advance)(altar,1,0);return 0; }return payload; }
    if (event==23) return payload;if (event!=24) return Need(hooks.damageable)(altar,event,count,payload);dais=Need(hooks.dais)(altar);if (!valid(dais)) throw std::logic_error("제단24 원본의 다이스0 프레임읽기 경계");
    if (direction(dais)==10) { if (Need(hooks.boss)()) { Need(hooks.destroy)(dais,kSacrificeDestroy);return -1.0F; }return payload; }
    if (!Need(hooks.advance)(altar,1,0)) {
        if (count<3 || Need(hooks.randomInt)(0,100000)/1000<50) { if (valid(dais)) Need(hooks.escapeFrame)(dais,count); }if (Need(hooks.randomFloat)(0,1)<payload) { const auto point=position(altar);Need(hooks.sound)(point[0],point[1],"altarBurnCollapse.wav",0); }return payload;
    }
    dais=Need(hooks.dais)(altar);if (!valid(dais)) return payload;if (Need(hooks.boss)()) { const auto captured=Need(hooks.priest)(dais);if (captured.value) Need(hooks.destroy)(captured,8); }Need(hooks.removeAnimation)(altar);const auto point=position(altar);Need(hooks.sound)(point[0],point[1],"explosion.wav",0);Need(hooks.frame)(dais,firstFrame(dais,'K'),0);Need(hooks.frame)(altar,firstFrame(altar,'M'),0);return kFinishedDelay;

}
}
