// 복원한 EvilRay 수명/프레임/타원 요청을 실제 공통 표시 목록과 Host/Kernel에 연결한다.
#pragma once
#include "o/RawEvilRayFrame.h"
#include "o/RenderProcessSystem.h"

namespace netstorm::o { // 원문 process10의 실제 소유권과 표시 수명이다.
class EvilRayVisualSystem; // 각 등록 객체보다 오래 사는 같은 시스템이다.
// 같은 Kernel이 소유하고 실제 ProcessForm/공통 renderer가 호출하는 파생 객체다.
class EvilRayVisualProcess final:public SquidProcess {
public:
    explicit EvilRayVisualProcess(EvilRayVisualSystem& system); // 생성/등록/부착의 완성은 System.Add가 수행한다.
    ~EvilRayVisualProcess() override; // 입자/공통 등록/조회 토큰을 원문 순서로 반납한다.
    void RunFrame() override; // 같은 현재 시각의 실제 Frame 몸체다.
    void OnFormDestroy(std::uint32_t flags) override; // 입자 소멸 전에 원문 삭제 영역을 무효화한다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 실제 가상1c의 mode3 Stop이며 info/value는 원문처럼 읽지 않는다.
    void Draw(); // 같은 단일 kind3 목록의 실제 타원 출력이다.
    const EvilRayFields& Fields() const; // 삭제 전까지만 유효한 단일 의미 저장소다.
private:
    friend class EvilRayVisualSystem;
    EvilRayVisualSystem& system_; // 실제 현재 장치/목록/시계를 빌린다.
    EvilRayFields fields_; // 원본 필드를 복사 프레임 없이 같은 저장소에서 갱신한다.
    bool ready_{}; // 공통 부착 중에는 파생 VT/필드가 아직 완성되지 않아 Run/Draw/통지를 보류한다.
};
// 계산을 공급하지 않고 실제 확보/메타데이터/음향/래스터와 반납 경계만 연결한다.
struct EvilRaySystemHooks {
    EvilRayLifetimeHooks lifetime; // 실제50입자와 첫 공유3색이다.
    EvilRayFrameHooks frame; // 같은 현재 SHP/음향/dirty/타원이다. kill은 시스템이 실제 Host에 연결한다.
    std::function<void(EvilRayRelease)> release; // 반납 관찰은 예외/재진입 없이 실제 소유권 반납을 허용해야 한다.
};
// 표시 시스템/전역/색 뷰/장치는 이 객체보다 오래 살아야 한다.
class EvilRayVisualSystem {
public:
    EvilRayVisualSystem(RenderProcessSystem& rendering,EvilRayLifetimeState& lifetime,FieryTailFrameState& frame,FieryTailFramePalette& colors,EvilRaySystemHooks hooks); // 같은 PATCH pool/Host와 단일 배율을 검증한다.
    ~EvilRayVisualSystem(); // 실제 Host가 살아 있을 때 남은 Form/부모0 소유권을 반납한다.
    EvilRayVisualSystem(const EvilRayVisualSystem&)=delete; // 콜백이 참조하는 고정 주소를 유지한다.
    EvilRayVisualSystem& operator=(const EvilRayVisualSystem&)=delete; // 다른 월드로 상태를 복사하지 않는다.
    EvilRayVisualProcess* Add(Sid parent); // 원문 공통 등록→부착→파생 초기화 순서이며 parent0은 등록만 수행한다.
    std::size_t Size() const; // 다른 효과와 구별되는 현재10 소유 수다.
private:
    friend class EvilRayVisualProcess;
    void CheckScale() const; // frame/collector가 같은 원본 배율을 사용하는지 확인한다.
    EvilRayFrameHooks FrameHooks(EvilRayVisualProcess& process); // 이번 실제 객체의 Host 자기 삭제를 선택한다.
    void Run(EvilRayVisualProcess& process); // 자기 삭제 뒤 객체를 다시 읽지 않는다.
    void Draw(EvilRayVisualProcess& process); // 현재 색 span으로 실제 Draw 요청을 제출한다.
    void Notify(EvilRayVisualProcess& process,std::uint32_t mode); // 현재 수명 몸체의 실제 Stop에 전달한다.
    void Removing(const EvilRayVisualProcess& process) const; // 공통 원문 삭제 영역의 실제 dirty 통지다.
    void Destroy(EvilRayVisualProcess& process); // 실제 입자/등록/base/토큰을 원문 순서로 반납한다.
    RenderProcessSystem& rendering_; // 다른 효과와 공유하는 단일 목록/Host다.
    FieryTailFrameState& frame_; // 실제 현재 시각/카메라/배율이다.
    FieryTailFramePalette& colors_; // 현재 공유3색 메모리의 비소유 뷰다.
    EvilRaySystemHooks hooks_; // 실제 장치/반납 경계다.
    RawEvilRayLifetime lifetime_; // 원문과 대조한 생성/Stop/소멸 몸체다.
    std::unordered_map<std::uint32_t,EvilRayVisualProcess*> processes_; // 소유권은 pending/Kernel/unattached에만 있다.
    std::unique_ptr<EvilRayVisualProcess> pending_; // 공통 부착 전 한 개의 유일한 소유권이다.
    std::vector<std::unique_ptr<EvilRayVisualProcess>> unattached_; // 원문 parent0은 Kernel 부착 없이 표시 등록을 유지한다.
    bool creating_{}; // C++ 생성 중 중첩 Add를 거부한다.
};
}
