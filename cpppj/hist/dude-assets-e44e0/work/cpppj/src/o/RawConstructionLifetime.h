// Particle.cpp의 표시 목록 등록/제거와 Build.cpp의 버퍼 소멸 순서를 복원한다.
#pragma once
#include "o/RawConstructionVisual.h"
#include <span>

namespace netstorm::o {
// RenderProcess의 공통 필드다. token은 원본 포인터 대신 쓰는 0이 아닌 호스트 식별자다.
struct RenderProcessFields {
    std::uint32_t token{}; // 목록에 넣은 객체의 식별자다. 소멸자도 같은 값을 사용한다.
    std::int32_t form{},parent{}; // +4/+8: 부착 전 0, base 소멸 뒤 -12345인 원본 DWORD다.
    float left{},top{},right{},bottom{}; // +0xc..+0x18: 등록 생성자가 0으로 만드는 표시 영역이다.
    std::int32_t slot{-1},zOrder{}; // +0x1c/+0x20: 0부터 시작하는 목록 번호와 signed 부모 깊이다.
};
// 표시 목록의 메모리 확보/반납과 BaseProcess 부착 경계다. 등록을 완료한 뒤 attach를 호출한다.
struct RenderRegistryHooks {
    std::function<bool(std::uint32_t)> reserve; // 원본 32비트 포인터 배열의 확보 바이트 수다.
    std::function<void()> releaseArray; // 기존 목록의 내용을 복사한 뒤 기존 배열을 반납한다.
    std::function<void(RenderProcessFields&,std::uint32_t,Sid,std::uint32_t)> attach; // 타입/부모/flags를 받는다. 지정 form은 0이다.
};
// 표시 객체의 소유권은 호출자가 가지며 이 목록은 token만 보관한다. 원본 Kernel 목록과 구별한다.
class RawRenderRegistry {
public:
    // pool은 목록보다 오래 살아야 한다. initial은 이미 등록된 객체 토큰이며 0은 빈 칸이다.
    // 원본: 00487fd0 / CD 0045c6c0. 원본 포인터 크기의 확보량을 경계에 전달한다.
    RawRenderRegistry(const SidPool& pool,RenderRegistryHooks hooks,std::vector<std::uint32_t> initial={});
    // 필드 초기화→첫 빈 칸/40칸 확장→slot/슬롯 0 깊이→부모 부착→진입 부모의 현재 깊이 순서다.
    // parent=0이면 부착하지 않는다. token은 목록 안에서 유일해야 한다.
    void Construct(RenderProcessFields& fields,std::uint32_t token,Sid parent,std::uint32_t type);
    // 원본: 00487880→0041bd00 / CD 0045c840→0048f570. 일치한 등록만 지우고 base 부모/form을 -12345로 바꾼다.
    // slot/zOrder/사각형은 남긴다. 잘못된 번호·중복 삭제는 변경 전에 예외로 거부한다.
    void Destroy(RenderProcessFields& fields);
    // 현재 슬롯 순서와 빈 칸을 읽는다. 다음 Construct의 확장 뒤 span은 무효가 될 수 있다.
    std::span<const std::uint32_t> Slots() const;
    // 수집기와 표시 등록이 같은 부모 raw 풀을 공유하는지 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 raw 부모의 signed BYTE 깊이를 읽는다. 번호 0도 원본처럼 읽을 수 있다.
    std::int32_t Depth(Sid parent) const;
    const SidPool& pool_; // 현재 부모 자료와 판본이다.
    RenderRegistryHooks hooks_; // 목록 확보/반납·부착의 외부 효과다.
    std::vector<std::uint32_t> slots_; // 원본 systems 배열의 토큰 표현이다.
};
// 원본 free 호출에서 구별하는 세 소유 버퍼와 scalar deleting destructor의 객체 자체다.
enum class ConstructionRelease { Source,Output,Particles,Object };
// 캡처→출력→입자→표시 등록 제거/base 소멸→flags bit 0의 객체 반납 순서다. 공유 팔레트는 반납하지 않는다.
// 사용: release는 호출 중에만 자료를 읽고 예외를 던지지 않아야 한다. 이 함수 뒤 같은 등록을 다시 삭제하지 않는다.
// 원본: 00422d00/00423540/00423820 ↔ CD 004ea360/004ead20/004ead40/004eadb0.
void DestroyConstructionVisual(ConstructionVisualFields& visual,RenderProcessFields& registration,
    RawRenderRegistry& registry,std::uint32_t flags,const std::function<void(ConstructionRelease)>& release);
// 기존 표시 생성의 attach 경계를 원본 목록 등록 몸체로 연결한다. 나머지 캡처/팔레트 경계는 그대로 사용한다.
// 사용: registry/registration은 반환한 hooks보다 오래 살아야 한다. 실제 Kernel 부착은 registry의 attach 책임이다.
ConstructionVisualHooks MakeConstructionRegistryHooks(RawRenderRegistry& registry,RenderProcessFields& registration,
    std::uint32_t token,ConstructionVisualHooks hooks);
}
