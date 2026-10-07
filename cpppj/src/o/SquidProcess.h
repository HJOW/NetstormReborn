// Baseprocess.cpp·Regular.cpp: 객체에 부착되는 프로세스(ProcessForm 종속 SID + Kernel 슬롯)와 지연/주기 이벤트를 복원한다.
#pragma once
#include "o/Kernel.h"
#include "o/RawBridgeLifecycle.h"
#include "o/RawSquidDestroy.h"
#include <functional>
#include <memory>

namespace netstorm::o {
class SquidProcessHost;
// 원본 BaseProcess(+4 formSid, +8 attachToSid). 프로세스마다 ProcessForm 타입의 SID 하나가 부모 객체 안에 들어간다.
class SquidProcess : public BaseProcess {
public:
    // 부모의 종속 체인에 들어 있는 form과 부착 대상 객체다. 부착 전에는 0이다.
    Sid Form() const;
    Sid Parent() const;
    // form 삭제 직전(vtable +0xc), 부모 삭제 중(vtable +0x10), 부착 직후(vtable +0x14)의 가상 통지다. 원본 base는 빈 함수다.
    virtual void OnFormDestroy(std::uint32_t flags);
    virtual void OnParentDying(Sid parent);
    virtual void OnAttached();
protected:
    // 부착한 host다. 부착 전에는 null이다.
    SquidProcessHost* Host() const;
private:
    friend class SquidProcessHost;
    Sid form_{},parent_{};
    ProcessId pid_{};
    SquidProcessHost* host_{};
};
// Regular.cpp의 RegularProcess(타입 46): 정해진 시각에 부모의 이벤트 처리기(vtable +0x5c)를 부르고 반환값으로 다음 시각을 정한다.
class RegularProcess final : public SquidProcess {
public:
    // 00496e80 ↔ CD 0048efd0의 필드 초기값이다. 시각은 부착 때의 현재 시각으로 정해진다.
    RegularProcess(std::uint32_t event,float payload);
    // 00496cb0 ↔ CD 0048f0e0. 처리기 반환값이 양수면 재예약, 0이면 종료, 음수면 그대로 둔다.
    void RunFrame() override;
    // 원본 +0x18 이벤트 번호, +0x1c payload(재예약 뒤에는 직전 반환값), +0x20 호출 횟수, +0x10 다음 실행 시각이다.
    std::uint32_t Event() const;
    float Payload() const;
    std::uint32_t Count() const;
    double Time() const;
private:
    friend class SquidProcessHost;
    double time_{};
    std::uint32_t event_{};
    float payload_{};
    std::uint32_t count_{};
};
// 프로세스 계층이 읽거나 바꾸는 원본 전역이다. 패치 주소/CD 주소는 괄호에 둔다.
struct SquidProcessState {
    double now{};                 // 게임 시각(DAT_0055b4d0 / CD 005484a8).
    bool boss{true};              // 권한 플래그(DAT_00540bc4 / CD 00540a2c). 꺼져 있으면 서버 번호 form은 flag 0x10 없이 지우지 않는다.
    std::uint32_t typeRotation{}; // 패치 Kernel 등록이 전진시키는 타입 순환 값(DAT_0059af74). CD에는 없다.
    std::uint32_t pollType{122};  // geyser(DAT_00541240 / CD 0051cb2c): 0.5초보다 먼 예약을 현재 시각으로 당긴다.
};
// 부모 객체의 이벤트 처리기(vtable +0x5c)다. 원본 base(00496e70)는 payload를 그대로 돌려준다.
using RegularHandler=std::function<float(Sid parent,std::uint32_t event,std::uint32_t count,float payload)>;

class SquidProcessHost {
public:
    // 풀/Kernel/공통 삭제/상태는 호출자가 소유하며 host보다 오래 살아야 한다. asset은 form이 아닌 객체의 삭제 훅이다.
    SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,RawSquidDestroy& destroy,
        SquidProcessState& state,RegularHandler handler={},SquidDestroyHooks asset={});
    // 0041bd20 ↔ CD 0048f590(formSid 0 경로): Kernel 등록 → ProcessForm 생성 → 타입/pid 기록 → 부모 안에 부착 → OnAttached.
    // 패치판은 부모가 free/dead이면 등록하지 않고 null을 반환한다. CD판의 그 경로(assert 뒤 계속)는 지원하지 않는다.
    SquidProcess* Attach(std::unique_ptr<SquidProcess> process,std::uint32_t processType,Sid parent,std::uint32_t flags);
    // 00496e80 ↔ CD 0048efd0: RegularProcess를 붙이고 시각을 현재 시각으로 둔다. 원본 생성자의 flags는 항상 로컬 전용 0x50이며
    // 다른 값은 BaseProcess 생성자의 할당/권한 분기를 검사할 때만 쓴다.
    RegularProcess* AddRegular(Sid parent,std::uint32_t event,float payload,std::uint32_t flags=0x50);
    // 0041bf10 ↔ CD 0048f7b0: 권한 조건이 맞으면 form을 공통 destroy로 지운다. 프로세스는 form의 preDestroy에서 소멸한다.
    void Kill(SquidProcess& process,std::uint32_t flags);
    // 004afb40 ↔ CD 004ac890: 부모의 종속 체인에서 이벤트 번호가 같은 첫 Regular 프로세스를 찾는다.
    RegularProcess* FindEvent(Sid parent,std::uint32_t event) const;
    // 004afbe0 ↔ CD 004ac9c0: (이벤트 & mask) == value인 첫 Regular 프로세스를 찾는다.
    RegularProcess* FindEventMasked(Sid parent,std::uint32_t mask,std::uint32_t value) const;
    // 공통 destroy에 줄 훅이다. ProcessForm 루트/종속의 가상 pre/post/release/destroy/Unpop을 처리하고 나머지는 asset으로 넘긴다.
    SquidDestroyHooks Hooks();
    // 판본의 프로세스 타입 번호 범위 끝(패치 63, CD 62)과 form의 실제 가상 표 기록값이다.
    std::uint32_t ProcessTypeEnd() const;
    std::uint32_t FormVtable() const;
private:
    friend class RegularProcess;
    // raw 필드를 호스트 정렬과 무관하게 읽고 쓴다. 쓰기는 할당된 슬롯만 받는다.
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    void PutWord(Sid sid,std::size_t offset,std::uint16_t value);
    // 타입 번호가 프로세스 타입인 form인지 판단한다.
    bool IsProcessForm(Sid sid) const;
    // 004b1440→004ac800→004ac560 ↔ CD 004af380→004ac2e0→004abfb0: 부모의 종속 체인 머리에 넣고 contained를 켠 뒤 Pop한다.
    void AttachForm(Sid form,Sid parent,std::uint32_t flags);
    // 004ae140/004ac6b0 ↔ CD 004af270/004ac160: void를 켜고, 부모가 free/dead가 아니면 체인에서 뺀다.
    void UnpopForm(Sid form);
    // 004ae240 ↔ CD 004af450: 프로세스에 통지하고 Kernel에서 제거한 뒤 삭제 깊이를 줄인다.
    void FormPreDestroy(Sid form,std::uint32_t flags);
    // 004ae2b0 ↔ CD 004af4c0: 부모가 지워질 때 프로세스에 통지한다.
    void FormParentDying(Sid form,Sid parent);
    // form 종속 체인에서 이전 항목 번호를 쓰는 가상 함수(vtable +0xc)다. form과 contained 자산의 필드 위치가 다르다.
    void SetPrevious(Sid sid,std::uint16_t previous);
    // Regular 실행 몸체. 판본별 abstract 처리와 처리기 반환값 분기를 구별한다.
    void RunRegular(RegularProcess& process);
    template<class Predicate> RegularProcess* Find(Sid parent,Predicate match) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    Kernel& kernel_;
    RawSquidDestroy& destroy_;
    SquidProcessState& state_;
    RegularHandler handler_;
    SquidDestroyHooks asset_;
};
// 00421530 ↔ CD 004490b0: 다리 칸에 지연 낙하 이벤트(0x2692)를 예약한다. payload는 좌표를 포장한 값이다.
RegularProcess* ScheduleBridgeFall(SquidProcessHost& host,Sid bridge,float x,float y);
// 00421fe0 ↔ CD 00449170: 그 칸에 예약된 지연 낙하가 있는지 본다.
bool HasScheduledBridgeFall(const SquidProcessHost& host,Sid bridge);
}
