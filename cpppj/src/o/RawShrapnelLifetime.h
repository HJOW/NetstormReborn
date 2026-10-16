// 실제 파편의 부모 없는 등록→더미/부착→캡처/준비와 반납 순서를 복원한다.
#pragma once
#include "o/RawShrapnelFrame.h"

namespace netstorm::o {
// 실제 소멸자가 구별하는 원본 bitmap/입자/객체다.
enum class ShrapnelRelease { Source,Particles,Object };
// 같은 월드/캡처/Host와 실제 계산 본체를 연결하는 명시 경계다.
struct ShrapnelLifetimeHooks {
    std::function<bool(std::uint32_t)> reserve; // wrapper의 원래104바이트 요청이며 호스트 객체는 별도 소유한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제 TYPE110/flags2의 client 자산 생성이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 생성 후 현재 source 좌표로 dummy를 Pop한다.
    std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach; // parent0 등록이 끝난 뒤 실제 process16 부착이다.
    std::function<void(Sid,Sid)> depend; // 실제004b1510(form,dummy)의 상호 감시 쌍이다.
    std::function<ConstructionVisualImage(Sid)> capture; // 현재 source의 실제 plain/mapped 그림 캡처다.
    std::function<void(ShrapnelFrameFields&,ShrapnelKind)> prepare; // 같은 두 난수/시각의 실제00460000이다.
};
// 실제004604a0의 성공 생성과 세 파생/공통 소멸 몸체다.
class RawShrapnelLifetime {
public:
    // 같은 풀/registry와 모든 실제 경계를 등록 전 검사한다.
    RawShrapnelLifetime(const SidPool& pool,RawRenderRegistry& registry,ShrapnelLifetimeHooks hooks);
    // 원본 wrapper new104 실패0 분기다. 성공 뒤 호스트가 실제 객체를 확보한다.
    bool ReserveObject() const;
    // dummy 출력은 생성 직후 설정되므로 부착 전 예외의 소유권 정리에도 쓴다.
    void Initialize(ShrapnelFrameFields& fields,std::uint32_t token,Sid source,ShrapnelKind kind,Sid& dummy) const;
    // bitmap→입자→공통 등록/base→flags bit0의 객체 순서로 실제 소유 저장소를 반납한다.
    void Destroy(ShrapnelFrameFields& fields,std::uint32_t flags,const std::function<void(ShrapnelRelease)>& release) const;
private:
    const SidPool& pool_; // callback 뒤 현재 source 좌표/깊이를 다시 읽는다.
    RawRenderRegistry& registry_; // 모든 표시가 공유하는 같은 원본 목록이다.
    ShrapnelLifetimeHooks hooks_; // 실제 객체/공간/캡처/계산의 경계다.
};
}
