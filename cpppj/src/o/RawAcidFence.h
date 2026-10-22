// 애시드145의 전용 주기·양축 이웃과13개 높이 영역의 포획을 복원한다.
#pragma once
#include "o/RawArcSpire.h"
#include "o/RawAcidZap.h"

namespace netstorm::o {
// 원본48c7a0의 첫0.3F 예약 뒤 같은 Damageable 후처리다.
void PostPopAcidFence(const SidPool& pool,Sid parent,std::uint32_t flags,const std::function<bool()>& reserve,
    const std::function<void(Sid,std::uint32_t,float)>& regular,const std::function<void(Sid,std::uint32_t)>& damageable);
// 각 영역이 비교할 원본 현재TYPE 전역이다. 비행체와 운반체를 구분한다.
struct AcidFenceTypes {
    std::uint32_t rainFlyer{144},sunFlyer{73},windFlyer{138}; //541298/54117c/541280의 개별 비행체다.
    std::uint32_t rainBalloon{146},windBalloon{141},sunBalloon{111}; //5412a0/54128c/541214의 운반체다.
};
// 순서 있는 검색/예약/피해/전송은 같은 실제 세션에 연결한다.
struct AcidFenceHooks {
    std::function<float(Sid)> range; //현재 TYPE+44의 원래 float다.
    std::function<Sid(float,float,std::uint32_t)> find; //4b1fa0의 위치/타입 검색이다.
    std::function<void(SquidSearchArea,const std::function<void(Sid)>&)> iterate; //4b16d0/4b1810의 flags1 후보 순서다.
    std::function<bool()> reserveRegular; //new40의 실제 성공/실패다.
    std::function<void(Sid,std::uint32_t,float)> regular; //확보 후496e80의event1/0.2F 예약이다.
    std::function<void(Sid,std::int32_t,std::uint8_t,Sid)> damage; //사제의 현재 가상98에1000/현재owner/source를 준다.
    std::function<void(std::span<std::uint8_t>)> broadcast; //4e2e30이며 로컬 수신 전에 호출한다.
    std::function<void(std::uint32_t)> assertion; //송신 뒤 패킷 불변식2fa..2fd 진단이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; //4acee0의 실제 프레임 갱신이다.
};
// 모든 참조는 같은 월드의 현재 상태이며 이 객체보다 오래 살아야 한다.
class RawAcidFence {
public:
    RawAcidFence(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        std::span<const std::uint8_t> spots,std::span<const std::uint32_t> alliances,const bool& boss,const std::uint32_t& editor,
        const bool& friendlyFire,const std::uint32_t& localOwner,const AcidFenceTypes& special,const RawAcidZap& zap,AcidFenceHooks hooks); //10.78의188타입/81동맹/256² 지도를 빌린다.
    float Handle(Sid parent,std::uint32_t event,std::uint32_t count,float period) const; //48ea30은count0에도공격하고공통Regular로위임하지않는다.
    void Attack(Sid parent) const; //48c830의 +X/+Y 첫 양방향 동맹과 본체 애니메이션이다.
    void Strike(Sid parent,std::span<const Sid> neighbors,std::int32_t xOriented) const; //48ca70의 이웃별 예약/13영역/40개 중복/송신·수신이다.
private:
    std::array<float,2> Position(Sid sid) const; //각 영역과 하위 효과 뒤 현재 좌표를 다시 읽는다.
    bool Allied(std::uint32_t first,std::uint32_t second) const; //원본의 정방향 동맹만 비교한다.
    std::uint8_t SpotUnion(SquidSearchArea area) const; //각 축1..255의 포함 사각형OR다.
    void Scan(Sid parent,Sid other,std::array<float,4> bounds,std::uint32_t genus,const std::uint32_t* exactType,std::span<Sid> victims,std::size_t& count) const; //현재TYPE전역은각후보에서읽으며nullptr만타입제한없음을뜻한다.
    const SidPool& pool_; //부모/이웃/후보의 같은50바이트 저장소다.
    std::span<const RiftTypeRecord> types_; //현재genus/발자국을 읽는다.
    std::span<const RiftTypeFrames> frames_; //현재 numFrames를 읽는다.
    std::span<const std::uint8_t> spots_; //실제 표면bit1이다.
    std::span<const std::uint32_t> alliances_; //방향 있는81개 현재값이다.
    const bool& boss_; const std::uint32_t& editor_; const bool& friendlyFire_; //피해 후보의 현재 권한/편집/친군사격 관문이다.
    const std::uint32_t& localOwner_; //48bfe0을 호출할 때의 현재 로컬 번호다.
    const AcidFenceTypes& special_; //13영역의 현재 비교TYPE다.
    const RawAcidZap& zap_; //기존 실제40ad30/48bfe0과 같은 패킷/수신이다.
    AcidFenceHooks hooks_; //현재 검색/장치/세계의 실제 효과다.
    RawArcSpire neighbors_; //동일48c300 몸체만 재사용하는 현재 양방향 동맹 이웃 검색이다.
};
}
