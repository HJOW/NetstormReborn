// Baseprocess.cpp·Regular.cpp: 객체에 부착되는 프로세스(ProcessForm 종속 SID + Kernel 슬롯)와 지연/주기 이벤트를 복원한다.
#pragma once
#include "o/Kernel.h"
#include "o/RawAuthorityState.h"
#include "o/RawBridgeLifecycle.h"
#include "o/RawSquidDestroy.h"
#include "o/SquidForms.h"
#include <functional>
#include <memory>

namespace netstorm::o {
class SquidProcessHost;
struct ProcessHostRuntimeBindings; // 짧은 World 대여의 C++ 반납만 식별하며 원문 process 필드로 직렬화하지 않는다.
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
    // ProcessForm 가상34가 같은 Kernel 객체의 실제 내장 notify1c에 전달하는 event/info/value다.
    // 복원하지 않은 파생 통지는 선택 시 예외로 거부한다.
    virtual void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value);
protected:
    // 부착한 host다. 부착 전에는 null이다.
    SquidProcessHost* Host() const;
    // Kernel 슬롯 번호다. 부착 전에는 0이다. 파생 프로세스가 Kill 뒤의 등록 여부를 물으려면 Kill 전에 읽어 둔다.
    // 이력: 2026-10-11 추가 — 건설 프로세스가 한 프레임에 Kill을 두 번 판단하므로.
    ProcessId Pid() const;
private:
    friend class SquidProcessHost;
    // form_: 커널 프로세스와 연결된 객체 Form의 SID.
    // parent_: 부착·통지 대상인 부모 객체 또는 부모 SID.
    Sid form_{},parent_{};
    // pid_: 커널의 프로세스 등록 번호.
    ProcessId pid_{};
    // host_: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트.
    SquidProcessHost* host_{};
    std::weak_ptr<ProcessHostRuntimeBindings> runtimeBinding_; // 독립 World 최종 fallback은 자기 대여가 만든 프로세스만 반납한다.
};
// 정해진 시각에 부모의 이벤트 처리기를 부르고 반환값으로 다음 시각을 정한다.
// 원본: Regular.cpp의 Regular(타입 46)와 SharedRegular(타입 61)는 같은 RunFrame 몸체를 쓴다.
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
    // Dude task 변경의 event/count0/다음double시각 직접쓰기를 같은 기존 Regular 객체에 적용한다.
    void ResetTask(std::uint32_t event,double time);
    void SetNextTime(double time); // 생성자/부착 콜백 뒤 event/payload/count를 보존하며 실제 기한만 덮어쓴다.
    // Damageable 삭제 후 새 Regular에 원본 +0x20 초기 count를 쓴다. event/payload/time/Form은 보존한다.
    void SetInitialCount(std::uint32_t count);
private:
    friend class SquidProcessHost;
    // time_: 시각.
    double time_{};
    // event_: 대상 처리기에 전달할 이벤트 ID.
    std::uint32_t event_{};
    // payload_: 예약 이벤트와 함께 전달할 실수 인자.
    float payload_{};
    // count_: 현재 항목 또는 실행 횟수.
    std::uint32_t count_{};
};
// 프로세스 계층이 읽거나 바꾸는 원본 전역이다. 패치 주소/CD 주소는 괄호에 둔다.
struct SquidProcessState {
    double now{};                 // 게임 시각(DAT_0055b4d0 / CD 005484a8).
    bool boss{true};              // 권한 플래그(DAT_00540bc4 / CD 00540a2c). 꺼져 있으면 서버 번호 form은 flag 0x10 없이 지우지 않는다.
    std::uint32_t typeRotation{}; // 패치 Kernel 등록이 전진시키는 타입 순환 값(DAT_0059af74). CD에는 없다.
    std::uint32_t pollType{122};  // geyser(DAT_00541240 / CD 0051cb2c): 0.5초보다 먼 예약을 현재 시각으로 당긴다.
    bool server{true}; // ProcessForm34가 매 통지 읽는 서버 전역(DAT_00540bc0 / CD00540a28)이다.
    RawAuthorityState* sharedAuthority{}; // 실행기보다 오래 사는 실제 두 bool이다. 미연결이면 기존 값형 입력만 읽는다.
    double* sharedNow{}; // 실제 Client FrameTime.game의 현재 주소다. 장치 소유자는 Host보다 오래 유지한다.
    std::uint32_t* sharedPollType{}; // 명시 장기 TYPE owner가 있을 때만 연결하며 Session 지역 슬롯은 빌리지 않는다.
    double& Now() { return sharedNow ? *sharedNow : now; } // 전체 제품의 같은 현재0055b4d0을 읽고 독립 입력만 fallback을 사용한다.
    const double& Now() const { return sharedNow ? *sharedNow : now; } // const 소비자도 같은 실제 시각 주소를 읽는다.
    std::uint32_t& PollType() { return sharedPollType ? *sharedPollType : pollType; } // Regular의 현재00541240을 같은 장기 DWORD에서 조회한다.
    const std::uint32_t& PollType() const { return sharedPollType ? *sharedPollType : pollType; } // TYPE 번호 사본을 동기화하지 않는다.
    bool& Boss() { return sharedAuthority ? sharedAuthority->boss : boss; } // Kill의 원문 소비 위치에서 현재 권한을 직접 읽거나 쓴다.
    const bool& Boss() const { return sharedAuthority ? sharedAuthority->boss : boss; } // const 조회도 같은 물리값을 읽는다.
    bool& Server() { return sharedAuthority ? sharedAuthority->server : server; } // Form 통지/서버 상태 출력이 현재 주소를 직접 사용한다.
    const bool& Server() const { return sharedAuthority ? sharedAuthority->server : server; } // fallback 동기화 사본을 만들지 않는다.
};
// 부모 객체의 이벤트 처리기(vtable +0x5c)다. 원본 base(00496e70)는 payload를 그대로 돌려준다.
using RegularHandler=std::function<float(Sid parent,std::uint32_t event,std::uint32_t count,float payload)>;
// 004b1440/CD 004af380의 파생 WriteData→공유 버퍼 송신 경계다. 버퍼 자체는 직렬화기가 소유한다.
struct ProcessFormTransmission {
    // writeData: 전송할 Form 데이터를 버퍼에 작성하는 콜백.
    std::function<std::int32_t(SquidProcess&,bool parentChanged)> writeData;
    // transmit: 작성한 패킷이나 Form 데이터를 송신하는 콜백.
    std::function<void(std::int32_t length)> transmit;
};

// 한 World가 빌리는 현재 삭제/사건/전송 처리기이며 Host 자체의 장기 수명과 구별한다.
struct ProcessHostRuntimeBindings {
    RawSquidDestroy* destroy{}; // 현재 World의 실제 공통 삭제 몸체다. Pool 정체성은 대여 시 검증한다.
    RegularHandler regular; // 현재 부모의 가상 Regular 사건이며 이전 World 콜백을 Host에 남기지 않는다.
    SquidDestroyHooks asset; // 같은 현재 가상 분배기의 자산 선택/삭제/공간 효과다.
    ProcessFormTransmission transmit; // 현재 실제 Form 직렬화와 송신 경계다.
    std::size_t activeCalls{}; // 선택된 Host 효과의 C++ 대여 수이며 진행 중 Native 반납을 거부한다.
};
// 현재 World 대여를 호출 위치에서 얻으며 반환된 공유 뷰는 외부 효과 호출 중 바인딩 자체의 수명을 유지한다.
using ProcessHostRuntimeGetter=std::function<std::shared_ptr<ProcessHostRuntimeBindings>()>;

// 객체 Form과 커널 프로세스의 부착·통지·수명을 연결하는 호스트이다.
class SquidProcessHost {
public:
    // 풀/Kernel/공통 삭제/상태는 호출자가 소유하며 host보다 오래 살아야 한다. asset은 form이 아닌 객체의 삭제 훅이다.
    SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,RawSquidDestroy& destroy,
        SquidProcessState& state,RegularHandler handler={},SquidDestroyHooks asset={},ProcessFormTransmission transmit={});
    // 장기 Pool/Kernel/상태를 빌리고 현재 World의 가상 처리기만 선택 위치에서 조회한다.
    SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,SquidProcessState& state,ProcessHostRuntimeGetter current);
    bool Owns(const BaseProcess& process) const; // 잔류 객체가 실제로 이 Host를 빌리는지 조회하며 PID나 SID를 정규화하지 않는다.
    void DrainRuntimeBindings(const std::shared_ptr<ProcessHostRuntimeBindings>& runtime); // 독립 World의 최종 fallback만 자기 현재 대여를 정리하며 이전 Native 잔류는 보존한다.
    bool HasOnlyRegularProcesses() const; // 현재 Host의 잔류 중46/61 C++ Regular 이외 수명은 별도 복원을 요구한다.
    // 0041bd20 ↔ CD 0048f590: Kernel 등록 → ProcessForm 생성/지정 슬롯 Take → 타입/pid 기록 → 부모 부착 → OnAttached.
    // 패치판은 부모가 free/dead이면 등록하지 않고 null을 반환한다. CD판의 그 경로(assert 뒤 계속)는 지원하지 않는다.
    // form=0은 기존 번호 할당이다. 신규 포인터의 지정 form은 free 서버 번호를 받고 raw payload·풀 목록/카운터를 보존한다.
    // 이미 소유한 포인터는 Reattach로 넘긴다. 기존 live Form에 신규 포인터를 넣는 원본 assert 경로는 거부한다.
    SquidProcess* Attach(std::unique_ptr<SquidProcess> process,std::uint32_t processType,Sid parent,std::uint32_t flags,Sid form={});
    // 같은 Kernel이 이미 소유한 포인터의 지정 Form을 재수신한다(0041bd20/CD 0048f590).
    // live Form은 기존 부모/PID를 검증한 뒤 OnAttached만 부르고 타입·링크·상태를 그대로 둔다.
    // free 서버 Form은 기존 PID로 Take/타입 쓰기/부착을 수행한다. 다른 객체의 live Form 교체는 원본 assert 경로로 거부한다.
    SquidProcess* Reattach(SquidProcess& process,std::uint32_t processType,Sid parent,std::uint32_t flags,Sid form);
    // 00496e80 ↔ CD 0048efd0: RegularProcess를 붙이고 시각을 현재 시각으로 둔다. 원본 생성자의 flags는 항상 로컬 전용 0x50이며
    // 다른 값은 BaseProcess 생성자의 할당/권한 분기를 검사할 때만 쓴다.
    RegularProcess* AddRegular(Sid parent,std::uint32_t event,float payload,std::uint32_t flags=0x50);
    // 일반 Regular와 같은 실행 몸체를 타입 61로 부착한다. 시각은 부착 후 현재 시각이다.
    // 원본: SharedRegular 생성자 00496f00 / CD 0048f1d0. 네트워크 저장 가상 함수는 별도 미복원이다.
    RegularProcess* AddSharedRegular(Sid parent,std::uint32_t event,float payload);
    // 모듈을 연결할 때 다른 월드의 풀을 섞지 않도록 참조를 제공한다.
    const SidPool& Pool() const;
    // 월드 파생 사건을 등록할 때 기존 fallback을 합성한 실제 처리기를 교체한다. 다음 Regular 호출은 현재 처리기를 읽는다.
    void SetRegularHandler(RegularHandler handler);
    // 미션 runtime의 실제 직렬화기를 연결한다. 기존 프로세스도 다음 Pop에서 이 경계를 읽는다.
    void SetFormTransmit(ProcessFormTransmission transmit);
    // 0041c010/CD 0048f8d0: 현재 부모를 읽고 live Form Unpop(0)→같은 부모 Attach(flags0)을 수행한다.
    // 파생 OnAttached를 다시 부르지 않으며 서버 전송이 필요한 경우 현재 전송 경계를 사용한다.
    void RefreshForm(SquidProcess& process);
    // 0041bf10 ↔ CD 0048f7b0: 권한 조건이 맞으면 form을 공통 destroy로 지운다. 프로세스는 form의 preDestroy에서 소멸한다.
    void Kill(SquidProcess& process,std::uint32_t flags);
    // 그 번호의 Kernel 슬롯에 지금도 이 프로세스가 등록돼 있는지 본다. 주소만 비교하고 객체를 읽지 않으므로
    // Kill이 프로세스를 없앴는지 확인할 때 쓸 수 있다. 번호와 주소는 Kill 전에 읽어 둔 값을 넘긴다.
    // 이력: 2026-10-11 추가 — 건설 프로세스의 둘째 Kill 판단을 위해.
    bool IsRegistered(ProcessId pid,const SquidProcess* process) const;
    // 004ae2d0 / CD 004af4e0: form +0x12의 현재 DWORD 번호로 Kernel의 프로세스를 조회한다. 비어 있으면 null이다.
    SquidProcess* FromForm(Sid form) const;
    // PATCH004ae280/CD004af490: server일 때 현재 Form PID의 실제 notify(2,dependForm,watched)를 호출한다.
    void NotifyDependency(Sid watcher,Sid watched,Sid dependForm) const;
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
    std::shared_ptr<ProcessHostRuntimeBindings> Runtime() const; // 실제 선택된 현재 World 대여를 요구한다.
    RawSquidDestroy& Destroy() const; // 소비 위치마다 현재 공통 삭제 몸체를 조회한다.
    const RiftTypeRecord& Type(std::size_t type) const; // 현재 TYPE 배열의 정확한 한 항목을 확인한다.
    std::size_t TypeCount() const; // 현재 실제 타입 표의 판본 길이다.
    RegularHandler Regular() const; // 호출 중 현재 대여 반납이 있어도 복사한 외부 함수의 수명을 지킨다.
    SquidDestroyHooks Asset() const; // 현재 자산 가상 분배를 원래 효과 직전에 조회한다.
    ProcessFormTransmission Transmission() const; // 직렬화와 송신의 각 소비 위치에서 현재 경계를 얻는다.
    ProcessHostRuntimeGetter currentRuntime_; // 장기 owner만 캡처하며 old World 구현 주소는 저장하지 않는다.
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_; // 기존 직접 생성자 fixture의 타입 snapshot을 보존한다.
    std::span<const RiftTypeRecord> sharedTypes_; // 장기 Host는 같은 Storage의 실제 현재 TYPE를 직접 읽는다.
    // kernel_: 프로세스 등록과 프레임 실행을 담당하는 커널.
    Kernel& kernel_;
    // destroy_: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    RawSquidDestroy* destroy_{}; // 기존 독립 생성자의 직접 대여이며 장기 Host에서는 사용하지 않는다.
    // state_: 프로세스 계층이 읽거나 바꾸는 원본 전역이다. 패치 주소/CD 주소는 괄호에 둔다 (SquidProcessState).
    SquidProcessState& state_;
    // handler_: 외부 이벤트·프로세스 동작을 실제 코드에 전달하는 처리기.
    RegularHandler handler_;
    // asset_: 출력할 타입 정의와 셰이프 블록을 연결한 자산.
    SquidDestroyHooks asset_;
    ProcessFormTransmission transmit_; // 실제 서버 부착의 파생 WriteData/공유 버퍼 송신 경계다.
};
// 00421530 ↔ CD 004490b0: 다리 칸에 지연 낙하 이벤트(0x2692)를 예약한다. payload는 좌표를 포장한 값이다.
RegularProcess* ScheduleBridgeFall(SquidProcessHost& host,Sid bridge,float x,float y);
// 00421fe0 ↔ CD 00449170: 그 칸에 예약된 지연 낙하가 있는지 본다.
bool HasScheduledBridgeFall(const SquidProcessHost& host,Sid bridge);
}
