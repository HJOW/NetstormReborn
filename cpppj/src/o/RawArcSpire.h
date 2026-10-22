// 아크 스파이어149의 원본 이웃 검색·연쇄 빔·Regular를 같은 raw 부모에 적용한다.
#pragma once
#include "o/RawSquidFinder.h"
#include <functional>

namespace netstorm::o {
// 원본48c080의 최초 Regular 예약 뒤 공통 Damageable 후처리를 실행한다.
void PostPopArcSpire(const SidPool& pool,Sid parent,std::uint32_t flags,
    const std::function<bool()>& reserve,const std::function<void(Sid,std::uint32_t,float)>& regular,
    const std::function<void(Sid,std::uint32_t)>& damageable);
// 하위 공간 검색/메타/피해/빔은 현재 세션의 실제 소비자를 연결한다.
struct ArcSpireHooks {
    std::function<float(Sid)> range,damageRate; // 현재 TYPE+44/+48의 원본 float다.
    std::function<Sid(float,float,std::uint32_t)> find; //4b1fa0의 같은 위치·타입 검색이다.
    std::function<void(SquidSearchArea,const std::function<void(Sid)>&)> iterate; //4b16d0/4b1810의 flags1 검색 순서를 소비자에게 전달한다.
    std::function<void(Sid,Sid)> beam; //472700의 생존 관문·shortZap.wav·실제 빔 생성이다.
    std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> damage; // 현재 target+98에 damage/owner/source를 넘긴다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; //4acee0의 실제 Unpop/프레임/Pop이다.
    std::function<float(Sid,std::uint32_t,std::uint32_t,float)> damageable; //44b900의 같은 Regular 사슬이다.
};
// 타입·동맹·편집 모드와 공간 효과는 이 모듈보다 오래 살아야 한다.
class RawArcSpire {
public:
    RawArcSpire(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        std::span<const std::uint8_t> spots,std::span<const std::uint32_t> alliances,const std::uint32_t& editor,ArcSpireHooks hooks); //10.78의188타입/81동맹/256² 지도를 빌린다.
    float Handle(Sid parent,std::uint32_t event,std::uint32_t count,float period) const; //48c6e0의 count 감김·반전 프레임·공통 Regular다.
    void Attack(Sid parent,float period) const; //48c110의 +X/+Y 검색·각각 별도 연쇄 효과다.
    bool Collect(Sid parent,std::span<Sid> buffer,std::uint32_t& count,std::int32_t x,std::int32_t y,std::uint32_t type) const; //48c300의 양방향 동맹·현재 extra/좌표 검사다.
    void Strike(Sid parent,std::span<const Sid> neighbors,std::int32_t damage) const; //48c410의40개 적 후보·살아남은 체인·마지막 이웃 빔이다.
private:
    std::array<float,2> Position(Sid sid) const; // 하위 효과 뒤 현재 부모 좌표를 다시 읽는다.
    bool Allied(std::uint32_t first,std::uint32_t second) const; //행first/열second의 방향을 보존한다.
    std::uint8_t SpotUnion(SquidSearchArea area) const; //4ab7c0의 각 축1..255 clamp/포함 사각형 OR다.
    const SidPool& pool_; // 부모/이웃/후보가 공유하는 유일한50바이트 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 후보의 flags1/flags2다.
    std::span<const RiftTypeFrames> frames_; // 현재 부모의 numFrames를 읽는 같은 코드 표다.
    std::span<const std::uint8_t> spots_; // 현재 표면 bit1을 읽는 실제 지도다.
    std::span<const std::uint32_t> alliances_; //원본00595200의 방향 있는81개 DWORD다.
    const std::uint32_t& editor_; // 피해 직전에 다시 읽는 원본005c85a4다.
    ArcSpireHooks hooks_; // 실제 큰 하위의 소비자다.
};
}
