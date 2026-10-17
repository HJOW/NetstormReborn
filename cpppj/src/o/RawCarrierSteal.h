// 크리스탈 크랩의 다른 운반체 내용물 포획과 별도60h 수신을 복원한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/RawFormPacket.h"
#include <functional>
#include <string_view>
namespace netstorm::o {
// 포획은 로컬 수신/음향 뒤 송신하고 현재 기증자 이동을 제거한다.
struct CarrierStealHooks {
    std::function<std::uint32_t(Sid)> commonHalt; //42a5f0의 실제 전체 정지 검사다.
    std::function<bool()> allocateRegular; //new28의 실제 할당 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> regular; //확보한 같은 메모리의496e80 생성자다.
    std::function<void(Sid)> check; //현재 가상+cc의 실제 운반체 검사다.
    std::function<void(Sid,std::uint32_t)> damageablePostPop; //44bf80의 같은 공통 장부다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; // 실제4ac9f0의 현재 내용물 조회다.
    std::function<void(Sid,std::uint32_t)> detach; // 내용물의 현재 가상+48(flags0)이다.
    std::function<void(Sid,Sid,std::uint32_t)> attach; // 현재 가상+50(parent,0)이다.
    std::function<void(Sid)> preview; // 실제426080의491 예약이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> sound; //4a9cb0의 전역 이름/반복/음량/좌우/우선/한도다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; //4e2e30은 로컬 수신을 완료한 뒤 호출한다.
    std::function<std::uint32_t(std::uint32_t)> movementType; //488fe0(type,0,1)의 현재 매핑이다.
    std::function<void(Sid,std::uint32_t)> removeMovement; //4b1a90(sid,type,0,1)의 실제 삭제다.
    std::function<void(Sid)> goHome; // 현재 내용물TYPE.flags2&8000이면428fd0을 요청한다.
};
// 모든 참조는 같은 실제 세션에서 빌리며 이 객체보다 오래 살아야 한다.
class RawCarrierSteal {
public:
    RawCarrierSteal(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const bool& boss,const std::uint32_t& editor,const std::uint32_t& localOwner,
        std::span<const std::uint32_t> alliances,const std::uint32_t& carrierType,
        const std::uint32_t& priestType,const FormPacketHeader& header,std::uint32_t& haltCounter,CarrierStealHooks hooks); // PATCH10.78의 실제 전역을 공유한다.
    bool Handles(Sid parent) const; // 실제크랩VT503f20만 전용 배치 후처리로 받는다.
    void PostPop(Sid parent,std::uint32_t flags) const; //42abb0의492 예약/현재check/Damageable 사슬이다.
    std::uint32_t Halt(Sid parent) const; //42ab80의 공유signed 나머지4와 포획/공통 분배다.
    std::uint32_t Try(Sid parent) const; //429ed0: 실제 포획 뒤12, 나머지10이다.
    void Receive(std::uint32_t owner,std::uint32_t flags,std::span<const std::uint8_t> packet) const; //4267c0은 선행 인자/기증자WORD를 읽지 않는다.
    const SidPool& Pool() const; // 다른 세션의 내용물/공간 연결을 검사한다.
private:
    const SidPool& pool_; // 부모/기증자/내용물의 같은50바이트 물리 저장소다.
    const SquidHash& hash_; // 실제 점 finder의 네 단계 공간이다.
    std::span<const RiftTypeRecord> types_; // 현재 genus와 내용물TYPE를 읽는다.
    const bool& boss_; // 진입의 현재 권한이다.
    const std::uint32_t& editor_; // 최초 내용물 조회 뒤 읽는 편집 모드다.
    const std::uint32_t& localOwner_; // 로컬 수신 직전의 현재 owner다.
    std::span<const std::uint32_t> alliances_; // [기증자owner*9+부모owner] 방향을 보존한다.
    const std::uint32_t& carrierType_; // 제외할 현재 크리스탈 크랩TYPE 전역이다.
    const std::uint32_t& priestType_; // 현재 사제TYPE 전역이다.
    const FormPacketHeader& header_; //40acf0의 현재BYTE/BYTE/WORD다.
    std::uint32_t& haltCounter_; //00545428은 모든 크랩이 공유하며 부모별 타이머가 아니다.
    CarrierStealHooks hooks_; // 실제 도달한 하위만 요구한다.
};
}
