// 벌룬의 그림자 후처리42ac90과 그림자 우선 삭제42ae80을 같은 월드에 연결한다.
#pragma once
#include "o/RawCarrierPostPop.h"
#include "o/RawCarrierPreDestroy.h"

namespace netstorm::o {
// 현재 TYPE와 실제 가상 월드 효과만 빌린다.
struct BalloonBodyHooks {
    std::function<std::int32_t(std::uint32_t)> frames; //캐시한 TYPE 주소의 현재+114를 읽는다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530(현재부모TYPE,2)의 실제 생성이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //현재 가상+90이다.
    std::function<void(Sid,std::uint32_t)> unpop,destroy; //그림자의 현재 가상+48/+10이다.
    std::function<void(Sid)> check; //현재 부모 가상+cc다. 비서버의 정확한 분기에서만 필요하다.
    std::function<bool()> allocateRegular; // 클라우드 플로터의 최초 new28 성공 여부다. NULL이면 주기 생성만 생략한다.
    std::function<void(Sid,std::uint32_t,float)> constructRegular; // 확보한 같은 Regular에 event3/0.1초를 쓰고 부모에 부착한다.
};
// 상위 Pop/삭제의 공통 깊이는 기존 실제 Damageable/Carrier가 각각 한 번 끝낸다.
class RawBalloonBody {
public:
    RawBalloonBody(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
        const bool& boss,const RawCarrierPostPop& post,const RawCarrierPreDestroy& pre,BalloonBodyHooks hooks); //참조는 객체보다 오래 살아야 한다.
    bool Handles(Sid sid) const; // PATCH5040c0 벌룬/에어십과504198 클라우드 플로터의 실제 표만 처리한다.
    void Validate(Sid sid,std::uint32_t flags) const; //현재 원시 부모를 검사한다. 상위 ValidateBase가 공통 장부를 검사한다.
    void PostPop(Sid sid,std::uint32_t flags) const; //그림자 생성/좌표/표시깊이/전용check→Damageable다.
    void PreDestroy(Sid sid,std::uint32_t flags) const; //현재 그림자를 먼저 삭제하고 같은 Carrier 기본 전처리를 실행한다.
    float CloudRegular(Sid sid,std::uint32_t event,std::uint32_t count,float payload,
        const std::function<void(Sid,std::int32_t,std::uint32_t)>& advance,
        const std::function<float(Sid,std::uint32_t,std::uint32_t,float)>& base) const; // 실제42af60은 event3만 프레임을 진행하며 나머지는42a750에 위임한다.
    const SidPool& Pool() const; //동일 월드 연결 검사다.
private:
    SidPool& pool_; //부모와 그림자의 단일 원시 저장소다.
    std::span<const RiftTypeRecord> types_; //최초 TYPE 주소를 효과 뒤에도 보존한다.
    std::span<const std::uint8_t> spots_; //고도0의 깊이를 결정하는 현재 칸 비트다.
    const bool& boss_; //마지막 check와 Carrier 전처리가 읽는 현재 생성 권한이다.
    const RawCarrierPostPop& post_; //같은 공통 깊이를 끝내는 실제 Damageable 후처리다.
    const RawCarrierPreDestroy& pre_; //그림자 삭제 뒤 실제 Damageable/내용물/보충 부모다.
    BalloonBodyHooks hooks_; //현재 TYPE 프레임과 큰 가상 월드 경계다.
};
}
