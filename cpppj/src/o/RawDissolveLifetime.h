// 원본450510/4881d0의 분해 생성과44fd80/423aa0의 소유권 반납이다.
#pragma once
#include "o/RawDissolveFrame.h"
#include "o/RawShrapnelLifetime.h"

namespace netstorm::o {
// 같은 월드·원본 캡처·Host와 실제 입자 계산을 연결하는 경계다.
struct DissolveLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; //실제120바이트 객체 확보 요청이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //실제 dummy110/client2 생성이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; //생성 이후 현재 source 좌표에 flags16으로 배치한다.
    std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach; //등록 뒤 process18의 실제 부착이다.
    std::function<void(Sid,Sid)> depend; //실제 Form과 원래 생성한 dummy의 상호 감시다.
    std::function<ConstructionVisualImage(Sid)> capture; //현재 대상의 실제 원본 bitmap을 소유하여 반환한다.
    std::function<void(DissolveFrameFields&,Sid,Sid,std::int32_t)> prepare; //450460의 캡처 뒤 현재 부모/폴 입자 계산이다.
};
// 최초 등록·더미·캡처 순서를 유지하며 현재 부착 부모에서 깊이를 읽는다.
class RawDissolveLifetime {
public:
    RawDissolveLifetime(const SidPool& pool,RawRenderRegistry& registry,DissolveLifetimeHooks hooks); //같은10.78 풀과 필수 경계를 검사한다.
    bool ReserveObject() const; //원본120바이트 요청을 수행한다. 실패 시 원본은 비정상 경로이며 호스트는 안전하게 거절한다.
    void Initialize(DissolveFrameFields& fields,std::uint32_t token,Sid source,Sid first,Sid second,std::int32_t xOriented,Sid& dummy) const; //정상 생성/등록/캡처/준비의 전체 순서다.
    void Destroy(DissolveFrameFields& fields,std::uint32_t flags,const std::function<void(ShrapnelRelease)>& release) const; //bitmap→입자→등록/base→객체의 반납이다.
private:
    const SidPool& pool_; //하위 callback 이후 현재 source를 다시 읽는다.
    RawRenderRegistry& registry_; //다른 모든 표시와 공유하는 실제 목록이다.
    DissolveLifetimeHooks hooks_; //세계/캡처/계산의 실제 경계다.
};
}
