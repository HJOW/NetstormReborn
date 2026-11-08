// 객체 부착 프로세스의 form 생성/부착/해제와 Regular 이벤트 실행을 원본 쓰기 순서대로 복원한다.
#include "o/SquidProcess.h"
#include "o/RawSquidTransmission.h" // ProcessForm/공유 Regular의 같은 송신 헤더와 임시 허용 전역을 빌린다.
#include "o/RawProcessAttach.h"
#include "o/RawBaseProcessAttach.h"
#include "o/RawSquidContainedUnpop.h"
#include "o/SquidForms.h"
#include "o/SquidFactory.h" // ProcessForm도 현재 공통 생성자/가상 수신 부모를 사용한다.
#include <bit>
#include <algorithm> // 실제 작성 버퍼 복사와 헤더 최소 길이에 사용한다.
#include <cmath>
#include <new> // 수신 공유 생성자의 실패 시 NULL 반환을 같은 배정 경계에서 보존한다.
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 종속 체인의 다음(+4)·머리(+6), 타입(+10), 상태(+11).
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11;
// Form의 비정렬 Kernel 번호 DWORD는 +0x12이며 가상 링크는 공통 현재 getter/setter가 처리한다.
constexpr std::size_t kFormPid=18;
// 상태 비트: free·dead·void·contained.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
// 타입 번호: ProcessForm, 프로세스 타입의 첫 번호, Regular/SharedRegular, form으로 취급하는 마지막 번호.
constexpr std::uint32_t kFirstProcessType=10,kRegularType=46,kSharedRegularType=61,kLastFormType=0x45;
// 판본별 프로세스 타입 끝(패치 DAT_0054116c, CD DAT_0051ca58)과 ProcessForm 가상 표의 실제 주소 기록값.
constexpr std::uint32_t kCdProcessEnd=62,kPatchFormVtable=0x512098,kCdFormVtable=0x505990;
// 부착 flags: 0x10 client 번호로 form 할당, 0x10|0x40 전송 억제, 0x80 부모 변경 허용(pfPROCESS_DEBUG).
constexpr std::uint32_t kClientForm=0x10,kLocalMask=0x50,kProcessDebug=0x80;
// abstract 상태여도 실제 Regular를 실행하는 부모 genus 비트다.
constexpr std::uint32_t kAbstractRunGenus=0x404200;
// geyser에 붙은 Regular가 허용하는 가장 먼 예약(초)이다.
constexpr double kPollLimit=0.5;
// 실제 선택 효과가 반환하기 전에 그 World 대여를 소비하지 못하게 하는 C++ 호출 수명이다.
struct RuntimeCall {
    std::shared_ptr<ProcessHostRuntimeBindings> runtime; // 호출 중 실제 처리기 묶음을 유지한다.
    std::shared_ptr<void> owner; //실제외곽World/EditorRuntime이며가짜대여토큰을만들지않는다.
    explicit RuntimeCall(std::shared_ptr<ProcessHostRuntimeBindings> value):runtime(std::move(value)),owner(runtime && runtime->pin ? runtime->pin():std::shared_ptr<void>{}) { if (runtime) ++runtime->activeCalls; } // 같은 현재 대여만 증가시킨다.
    ~RuntimeCall() { if (runtime) --runtime->activeCalls; } // 정상/예외 모두 실제 호출이 끝난 뒤 반납한다.
};
}
Sid SquidProcess::Form() const { return form_; }
Sid SquidProcess::Parent() const { return parent_; }
void SquidProcess::OnFormDestroy(std::uint32_t) {}
void SquidProcess::OnParentDying(Sid) {}
void SquidProcess::OnAttached() {}
// 실제 notify 몸체를 아직 복원하지 않은 파생 객체가 통지를 성공으로 버리지 않게 거부한다.
void SquidProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("현재 파생 프로세스의 notify1c 미복원"); }
SquidProcessHost* SquidProcess::Host() const { return host_; }
// 부착 때 Kernel이 배정한 번호를 그대로 돌려준다.
ProcessId SquidProcess::Pid() const { return pid_; }

RegularProcess::RegularProcess(std::uint32_t event,float payload):event_(event),payload_(payload) {}
// 부착하지 않은 프로세스는 Kernel에 없으므로 실행되지 않는다.
void RegularProcess::RunFrame() { if (auto* host=Host()) host->RunRegular(*this); }
std::uint32_t RegularProcess::Event() const { return event_; }
float RegularProcess::Payload() const { return payload_; }
std::uint32_t RegularProcess::Count() const { return count_; }
double RegularProcess::Time() const { return time_; }
// 패치00450670/CD004e8e70의 기존task Regular 직접쓰기에 대응하며 payload/Form/부착을 보존한다.
void RegularProcess::ResetTask(std::uint32_t event,double time) { event_=event;count_=0;time_=time; }
// 원본 Carrier426db0의 생성 이후 +10/+14 쓰기이며 다른 필드를 재초기화하지 않는다.
void RegularProcess::SetNextTime(double time) { time_=time; }
// 패치0044b9e0/CD00461b40의 생성 직후 count 직접쓰기를 같은 실제 Regular에 적용한다.
void RegularProcess::SetInitialCount(std::uint32_t count) { count_=count; }

SquidProcessHost::SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,RawSquidDestroy& destroy,
    SquidProcessState& state,RegularHandler handler,SquidDestroyHooks asset,ProcessFormTransmission transmit)
    :pool_(pool),types_(types.begin(),types.end()),kernel_(kernel),destroy_(&destroy),state_(state),
     handler_(std::move(handler)),asset_(std::move(asset)),transmit_(std::move(transmit)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("프로세스 타입 판본/크기 오류");
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("프로세스/삭제 SID 풀이 다릅니다");
}
// 같은 Native Host 주소를 다음 World에서도 유지하고 실제 현재 처리기만 대여한다.
SquidProcessHost::SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,SquidProcessState& state,ProcessHostRuntimeGetter current)
    :pool_(pool),sharedTypes_(types),kernel_(kernel),state_(state),currentRuntime_(std::move(current)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !currentRuntime_) throw std::invalid_argument("장기 ProcessHost의 실제 판본/현재 대여 계약");
}
// 생성하지 않은 빈 World를 합성하지 않고 선택된 실제 처리기 부재를 그 위치에서 보고한다.
std::shared_ptr<ProcessHostRuntimeBindings> SquidProcessHost::Runtime() const {
    auto value=currentRuntime_();if (!value || !value->destroy || &value->destroy->Pool()!=&pool_) throw std::logic_error("장기 ProcessHost의 현재 World 처리기 미연결");return value;
}
// 직접 fixture와 현재 World 대여는 각각 자기 실제 삭제 몸체를 사용한다.
RawSquidDestroy& SquidProcessHost::Destroy() const { return currentRuntime_ ? *Runtime()->destroy : *destroy_; }
std::size_t SquidProcessHost::TypeCount() const { return sharedTypes_.empty() ? types_.size() : sharedTypes_.size(); } // 직접 fixture의 사본과 장기 실제 TYPE를 구분한다.
const RiftTypeRecord& SquidProcessHost::Type(std::size_t type) const { if (type>=TypeCount()) throw std::out_of_range("ProcessHost 현재 TYPE 범위");return sharedTypes_.empty() ? types_[type] : sharedTypes_[type]; } // 옵션/다음 World가 바꾼 실제 flags도 즉시 읽는다.
RegularHandler SquidProcessHost::Regular() const { return currentRuntime_ ? Runtime()->regular : handler_; } // 현재 사건 함수만 짧게 빌린다.
SquidDestroyHooks SquidProcessHost::Asset() const { return currentRuntime_ ? Runtime()->asset : asset_; } // 현재 자산 효과를 복사해 호출 중 수명을 지킨다.
ProcessFormTransmission SquidProcessHost::Transmission() const { //현재대여의실제파생writer가있으면우선하고반납뒤에는같은장기기본writer를사용한다.
    if (currentRuntime_) { const auto current=currentRuntime_();if (current && (current->transmit.writeData || current->transmit.writeFullData || current->transmit.packet || current->transmit.transmit)) return current->transmit; }return transmit_;
}
void SquidProcessHost::BindDefaultFormTransmission(ProcessFormTransmission transmit,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion) { //같은장기Storage의실제직렬화와진단만최초한번보관한다.
    if (!currentRuntime_ || transmit_.writeData || !transmit.writeData || !transmit.writeFullData || !transmit.packet || !transmit.transmit || !assertion) throw std::invalid_argument("장기ProcessForm의실제기본직렬화장치계약");transmit_=std::move(transmit);state_.assertion=std::move(assertion);
}
void SquidProcessHost::BindFormFactory(SquidFactory& factory) { // 장기 Host의 옛 World 주소 저장과 다른 Pool 연결을 거부한다.
    if (currentRuntime_ || &factory.Pool()!=&pool_ || (directFormFactory_ && directFormFactory_!=&factory)) throw std::invalid_argument("ProcessForm 직접 공장 연결의 같은 수명/Pool 계약");directFormFactory_=&factory;
}
Sid SquidProcessHost::CreateOrTakeForm(std::uint32_t flags,Sid form) { // 인자 계산 뒤 현재 번호를 읽고 실제 선택 장치만 요구한다.
    const auto apply=[&](SquidFactory& factory) { if (&factory.Pool()!=&pool_) throw std::invalid_argument("ProcessForm 공장의 실제 Pool 불일치");const auto type=state_.ProcessFormType();if (form.value) { factory.Take(type,form);return form; }return factory.Create(type,(flags&kClientForm) ? 2U : 0U); }; // 지정 슬롯의 Take 반환은 원문처럼 무시하고 원래 SID를 후단에 사용한다.
    if (currentRuntime_) { const auto runtime=Runtime();if (!runtime->factory) throw std::logic_error("현재 World의 ProcessForm 공장 미연결");return apply(*runtime->factory); }
    if (directFormFactory_) return apply(*directFormFactory_);
    SquidFactory independent(pool_,types_);independent.BindDiagnostics({types_,state_.assertion,{}});return apply(independent); // 독립 입력의 명시 TYPE snapshot에도 같은 공통 부모를 적용한다.
}
bool SquidProcessHost::Owns(const BaseProcess& process) const { const auto* squid=dynamic_cast<const SquidProcess*>(&process);return squid && squid->host_==this; } // 같은 실제 Host 포인터를 비교한다.
// 이것은 World 최종 C++ 반납이며 Native Kernel.Clear의 단일 순회를 대신하지 않는다.
void SquidProcessHost::DrainRuntimeBindings(const std::shared_ptr<ProcessHostRuntimeBindings>& runtime) {
    const auto pinned=runtime;RuntimeCall call(pinned); // 소멸 callback의 반납 재진입에도 같은 호출 대여를 안정적으로 유지한다.
    kernel_.DrainOwnedForShutdown([this,&pinned](const BaseProcess& candidate) { // 슬롯0/덮어쓴 소유권도 같은 모듈이 살아 있는 대여 반납에서 정리한다.
        const auto* process=dynamic_cast<const SquidProcess*>(&candidate);return process && process->host_==this && process->runtimeBinding_.lock()==pinned;
    });
}
bool SquidProcessHost::HasOnlyRegularProcesses() const {
    for (ProcessId id=1;id<=kernel_.Capacity();++id) { // 단일 현재 표를 읽으며 다른 Host/Client 프로세스를 자기 소유로 판정하지 않는다.
        const auto* process=kernel_.Get(id);if (process && Owns(*process) && !dynamic_cast<const RegularProcess*>(process)) return false;
    }return true;
}
std::uint32_t SquidProcessHost::ProcessTypeEnd() const { return pool_.Edition()==OriginalEdition::Patch1078 ? state_.ProcessTypeEnd() : kCdProcessEnd; } // PATCH는 같은 현재0054116c의 signed 상한 비트를 반환한다.
std::uint32_t SquidProcessHost::FormVtable() const { return pool_.Edition()==OriginalEdition::Patch1078 ? kPatchFormVtable : kCdFormVtable; }
std::uint16_t SquidProcessHost::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);
    return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
bool SquidProcessHost::IsProcessForm(Sid sid) const {
    if (sid.value==0 || sid.value>=pool_.Capacity()) return false;
    const auto raw=pool_.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
    return table==FormVtable(); // 가상 삭제 효과는 현재 클래스가 선택하며 변경 가능한 TYPE/프로세스 상한을 선행 검사하지 않는다.
}
void SquidProcessHost::AttachForm(Sid form,Sid parent,std::uint32_t flags) {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 실제 효과가 끝나기 전 현재 대여를 반납하지 않는다.
    ProcessAttachHooks hooks;SquidProcess* selected{}; // 서버 lookup이 저장한 실제 포인터만 writer에서 사용한다.
    hooks.server=[this] { return pool_.IsServer(); };
    hooks.lookup=[&](Sid sid) { selected=FromForm(sid); }; // flags가 로컬이어도 서버 lookup은 먼저 실행한다. NULL은 writer 선택 때만 오류다.
    hooks.writeData=[&](bool changed) { if (!selected) throw std::logic_error("Process 부착의 선택된 실제 PID 미연결");const auto device=Transmission().writeData;if (!device) throw std::logic_error("Process 부착의 선택된 실제 writer 미연결");return device(*selected,changed); };
    hooks.transmit=[this](std::int32_t size) { const auto device=Transmission().transmit;if (!device) throw std::logic_error("Process 부착의 선택된 실제 송신 장치 미연결");device(size); };
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto device=currentRuntime_ ? Runtime()->assertion : state_.assertion;if (!device) throw std::logic_error("Process 부착의 선택된 원문 진단 장치 미연결");device(expression,file,line); }; // 진단 선택 위치에서 현재 대여만 요구한다.
    hooks.attach=[this](Sid form,Sid parent,std::uint32_t flags) {
    const auto apply=[&](SquidFactory& factory) { factory.AttachForm(form,parent,flags); }; // 파생 직렬화 접두 뒤에만 공통 현재 genus/링크/Pop 진단을 실행한다.
    if (currentRuntime_) { const auto runtime=Runtime();if (!runtime->factory) throw std::logic_error("현재 World의 공통 Form 부착 공장 미연결");apply(*runtime->factory); }
    else if (directFormFactory_) apply(*directFormFactory_);
    else { SquidFactory independent(pool_,types_);independent.BindDiagnostics({types_,state_.assertion,{}});apply(independent); } // 독립 입력도 같은 현재 표/진단 부모를 쓴다.
    }; // 원문 파생 접두를 마친 위치에서만 현재 공통 공장을 요구한다.
    const auto table=std::uint32_t(Word(form,0))|(std::uint32_t(Word(form,2))<<16); // TYPE를 바꾸어도 실제 생성 VT의 +74를 선택한다.
    if (table==FormVtable()) AttachProcessForm(pool_,form,parent,flags,hooks);
    else {
        bool common=false;for (const auto kind:{1U,3U,6U}) if (table==SquidForms(pool_).Vtable(kind)) common=true; // 확인된 내장 Form 세 클래스의 +74는 공통 부모다.
        if (!common) throw std::logic_error("BaseProcess 부착의 미복원 현재 VT+74");hooks.attach(form,parent,flags);
    }
}

// C++ 소유권을 가진 신규 입력도 같은 원문 부모에 전달한다.
SquidProcess* SquidProcessHost::Attach(std::unique_ptr<SquidProcess> process,std::uint32_t processType,Sid parent,std::uint32_t flags,Sid form) {
    if (!process) throw std::invalid_argument("부착할 프로세스가 없습니다");
    if (kernel_.Find(*process)) { auto* existing=process.release();return Reattach(*existing,processType,parent,flags,form); } // 동일 포인터의 C++ 중복 소유권만 해제한다.
    auto* attached=process.get();return AttachBase(*attached,processType,parent,flags,form,&process) ? attached : nullptr;
}
// 실제 Kernel이 소유한 기존 포인터는 새 unique_ptr 없이 같은 상위 부모에 등록한다.
SquidProcess* SquidProcessHost::Reattach(SquidProcess& process,std::uint32_t processType,Sid parent,std::uint32_t flags,Sid form) {
    if (!kernel_.Find(process) || (process.host_ && process.host_!=this)) throw std::invalid_argument("재수신 프로세스의 C++ Kernel/host 소유권 불일치");
    return AttachBase(process,processType,parent,flags,form,nullptr) ? &process : nullptr;
}
// 장기 Host의 현재 장치와 실제 두 필드를 원문 부모에 제공하며 부분 실패를 숨기지 않는다.
bool SquidProcessHost::AttachBase(SquidProcess& process,std::uint32_t processType,Sid parent,std::uint32_t flags,Sid form,std::unique_ptr<SquidProcess>* owned) {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr);process.host_=this;process.runtimeBinding_=currentRuntime_ ? currentRuntime_() : std::shared_ptr<ProcessHostRuntimeBindings>{};
    BaseProcessAttachHooks hooks;hooks.typeEnd=[this] { return ProcessTypeEnd(); };hooks.processFormType=[this] { return state_.ProcessFormType(); };
    hooks.assertion=[this](auto expression,auto file,auto line) { const auto device=currentRuntime_ ? Runtime()->assertion : state_.assertion;if (!device) throw std::logic_error("BaseProcess 부착의 선택된 원문 진단 장치 미연결");device(expression,file,line); };
    hooks.registerProcess=[&] { if (pool_.Edition()==OriginalEdition::Patch1078) state_.typeRotation=(state_.typeRotation+1U)%static_cast<std::uint32_t>(TypeCount());const auto pid=owned ? kernel_.Add(std::move(*owned)) : kernel_.Add(process);process.pid_=pid;return pid; }; // 원문 Kernel helper는 슬롯 검색 전에 회전을 전진시킨다.
    hooks.create=[this](std::uint32_t allocationFlags) { return CreateOrTakeForm(allocationFlags==2 ? kClientForm : 0U,Sid{}); };
    hooks.take=[this](Sid selected) { static_cast<void>(CreateOrTakeForm(0,selected)); };
    hooks.parent=[this](Sid selected) { return CommonContainedParent(pool_,selected); };
    hooks.attach=[this](Sid selected,Sid into,std::uint32_t popFlags) { AttachForm(selected,into,popFlags); };
    hooks.onAttached=[&] { process.OnAttached(); };
    return AttachBaseProcess(pool_,{process.form_,process.parent_},processType,parent,form,flags,hooks);
}
RegularProcess* SquidProcessHost::AddRegular(Sid parent,std::uint32_t event,float payload,std::uint32_t flags) {
    return AttachRegular(std::make_unique<RegularProcess>(event,payload),parent,false,flags); // 첫 실행은 현재 시각 이후의 Kernel 프레임이다.
}
// 같은 Regular 객체를 공유 타입으로 부착한다. 부모 체인/Kernel/예약 규칙을 재사용한다.
// 원본: 00496f00 / CD 0048f1d0. 실행 vtable +0x18은 일반 Regular와 동일하다.
RegularProcess* SquidProcessHost::AddSharedRegular(Sid parent,std::uint32_t event,float payload) {
    return AttachRegular(std::make_unique<RegularProcess>(event,payload),parent,true); // 공유 생성자도 같은 부착 뒤 시각 저장을 사용한다.
}
RegularProcess* SquidProcessHost::ReceiveSharedRegular(std::span<const std::uint8_t> packet) { // 원문 full 생성자는 일반 공유 생성자의 깊이 진단/현재 시각 저장을 호출하지 않는다.
    if (pool_.Edition()!=OriginalEdition::Patch1078 || packet.size()<28) throw std::invalid_argument("공유 Regular 수신의 PATCH28BYTE 계약");
    const auto read=[packet](std::size_t offset,std::size_t width) { std::uint64_t value{};for (std::size_t i=0;i<width;++i) value|=std::uint64_t(packet[offset+i])<<(i*8);return value; }; // 반복은 WORD/DWORD/double의 원래 비트만 읽는다.
    const Sid parent{static_cast<std::uint16_t>(read(6,2))};auto process=std::unique_ptr<RegularProcess>(new(std::nothrow) RegularProcess(static_cast<std::uint32_t>(read(16,4)),std::bit_cast<float>(static_cast<std::uint32_t>(read(20,4)))));if (!process) return nullptr;
    process->time_=std::bit_cast<double>(read(8,8));process->count_=static_cast<std::uint32_t>(read(24,4));process->shared_=true;process->parent_=parent;
    return static_cast<RegularProcess*>(Attach(std::move(process),state_.SharedRegularType(),parent,0x50)); // 같은 실제41bd20 부모를 사용하고 패킷+4의 미작성 SID를 읽지 않는다.
}
// 이미 확보한 예약은 같은 현재 생성자 번호로 부착하며 SharedRegular의 원문 진단 후에도 현재 값을 다시 읽는다.
RegularProcess* SquidProcessHost::AttachRegular(std::unique_ptr<RegularProcess> process,Sid parent,bool shared,std::uint32_t flags) {
    if (process) process->shared_=shared; // 원문 두 생성자의 실제 writer 가상 표 선택을 부착 전에 보존한다.
    if (shared && pool_.Edition()==OriginalEdition::Patch1078) {
        const auto runtime=currentRuntime_ ? Runtime() : nullptr;RuntimeCall call(runtime); // 진단이 대여 반납을 시도해도 호출 중 World를 보존한다.
        const auto depth=runtime ? runtime->postPopDepth : std::function<std::uint32_t()>{};
        if (runtime && !depth) throw std::logic_error("SharedRegular 현재 Pop 깊이 미연결");
        if (depth ? depth()!=0 : state_.postPopDepth!=0) {
            const auto assertion=runtime ? runtime->assertion : state_.assertion; // 정상 복귀 중 연결 교체에도 실행 중 callable은 유지한다.
            if (!assertion) throw std::logic_error("SharedRegular 원문 진단 장치 미연결");
            assertion("basePostPop == 0",R"(\Ns\O\Regular.cpp)",0x70);
        }
    }
    const auto type=pool_.Edition()==OriginalEdition::Patch1078 ? (shared ? state_.SharedRegularType() : state_.RegularType()) : (shared ? kSharedRegularType : kRegularType); // 공유 진단 다음의 현재 전체 DWORD를 선택한다.
    auto* attached=static_cast<RegularProcess*>(Attach(std::move(process),type,parent,flags));
    if (attached) attached->time_=state_.Now(); // 원문496e80/496f00은 부착 호출이 돌아온 뒤 현재double시각을 읽는다.
    return attached;
}
// 실제 부착과 사제 낙하 연결이 같은 풀인지 검사할 때 사용한다.
const SidPool& SquidProcessHost::Pool() const { return pool_; }
// 현재 사건 표를 교체해 이미 생성된 Regular와 새 Regular가 같은 파생 월드 처리기를 사용하게 한다.
void SquidProcessHost::SetRegularHandler(RegularHandler handler) { if (currentRuntime_) Runtime()->regular=std::move(handler);else handler_=std::move(handler); }
// 전송 정책을 우회하는 flags 변경 없이 현재 미션 직렬화 경계만 갱신한다.
void SquidProcessHost::SetFormTransmit(ProcessFormTransmission transmit) { if (currentRuntime_) Runtime()->transmit=std::move(transmit);else transmit_=std::move(transmit); }
// 원문004ae360/00496e00의 같은 Kernel 객체를 사용한다. 호출 중 현재 World 대여를 반납하지 않는다.
std::int32_t SquidProcessHost::WriteFormPacket(Sid form,std::uint32_t argument,std::span<std::uint8_t> packet,const RawSquidTransmissionState& transmission) const {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr);const auto parent=CommonContainedParent(pool_,form);
    if (parent.value>=5 && parent.value<15000) {
        const auto assertion=call.runtime && call.runtime->assertion ? call.runtime->assertion : state_.assertion;if (!assertion) throw std::logic_error("ProcessForm writer의 부모 SID 진단 미연결");assertion("!getParent().isClientSid()","\\Ns\\O\\Squid.cpp",0xac2);
    }
    auto* process=FromForm(form);if (!process) throw std::logic_error("ProcessForm writer의 실제 PID 객체 누락");
    if (const auto* regular=dynamic_cast<RegularProcess*>(process)) {
        if (!regular->Shared() || !transmission.clientSidAllowed) return 0; // 일반 Regular는 실제 빈 writer이며 공유 writer는 동기화 허용 때만 작성한다.
        if (packet.size()<28) throw std::out_of_range("SharedRegular의 실제28BYTE 버퍼 범위");
        const auto put=[packet](std::size_t offset,std::uint64_t value,std::size_t width) { // 원문 WORD/DWORD/double 필드만 쓰며 미작성+4/+5를 보존한다.
            for (std::size_t index=0;index<width;++index) packet[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 각 필드의 낮은 바이트부터 지정 폭만 쓴다.
        };
        const auto header=transmission.sharedRegular;packet[0]=header.protocol&0x7fU;packet[1]=header.message;put(2,std::max<std::uint32_t>(4,header.length),2);
        put(6,regular->Parent().value,2);put(8,std::bit_cast<std::uint64_t>(regular->Time()),8);put(16,regular->Event(),4);put(20,std::bit_cast<std::uint32_t>(regular->Payload()),4);put(24,regular->Count(),4);return 28;
    }
    const auto device=Transmission();if (!device.writeFullData) throw std::logic_error("ProcessForm의 실제 전체 파생 writer 미연결");const auto size=device.writeFullData(*process,argument);
    if (size>0) {
        if (!device.packet) throw std::logic_error("ProcessForm의 실제 작성 버퍼 미연결");const auto bytes=device.packet(size);
        if (static_cast<std::size_t>(size)>bytes.size() || static_cast<std::size_t>(size)>packet.size()) throw std::out_of_range("ProcessForm의 실제 패킷 복사 범위");
        if (bytes.data()!=packet.data()) std::copy_n(bytes.begin(),size,packet.begin()); // 같은2048BYTE 버퍼를 쓴 Dais/건설 writer는 전체 포인터가 같으면 다시 겹쳐 복사하지 않는다.
    }return size;
}
// 원문 일반/공유 Regular의 가상 writer 선택만 공개한다.
bool RegularProcess::Shared() const { return shared_; }
// 현재 raw 부모를 다시 읽어 프로세스의 오래된 parent 복사값으로 재부착하지 않는다.
void SquidProcessHost::RefreshForm(SquidProcess& process) {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 현재 World의 호출 대여를 끝까지 유지한다.
    if (process.host_!=this || !IsRegistered(process.pid_,&process)) throw std::logic_error("Form 갱신 프로세스의 C++ 소유권 불일치");
    const auto parent=CommonContainedParent(pool_,process.Form());const auto raw=pool_.Slot(parent);const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 부모는 첫 getter에서 저장하고 Form 필드는 각 가상 진입 전에 다시 읽는다.
    const auto assertion=[this,patch](std::string_view expression,std::uint32_t patchLine,std::uint32_t cdLine) { const auto device=currentRuntime_ ? Runtime()->assertion : state_.assertion;if (!device) throw std::logic_error("Form 갱신의 선택된 원문 진단 장치 미연결");device(expression,patch ? "\\Ns\\O\\Baseprocess.cpp" : "\\ns\\o\\BaseProcess.cpp",patch ? patchLine : cdLine); };
    if (raw[11]&kDead) assertion("!parent.isDead()",0xb0,0xab);
    if (raw[11]&kFree) assertion("!parent.isFree()",0xb1,0xac);
    UnpopForm(process.Form());AttachForm(process.Form(),parent,0);
}
void SquidProcessHost::UnpopForm(Sid form) {
    const auto runtime=currentRuntime_ ? Runtime() : nullptr;RuntimeCall call(runtime); // 실제 분리/진단이 돌아올 때까지 현재 World를 보존한다.
    if (runtime) { if (!runtime->factory) throw std::logic_error("Process Unpop의 현재 공장 미연결");runtime->factory->UnpopForm(form);return; }
    if (directFormFactory_) { directFormFactory_->UnpopForm(form);return; }
    FormUnpopHooks hooks;hooks.assertion=state_.assertion;SquidForms(pool_,{}, {},std::move(hooks)).Unpop(form); // 독립 Host도 같은 공통 해제/진단 순서를 사용한다.
}
void SquidProcessHost::FormPreDestroy(Sid form,std::uint32_t flags) {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 실제 효과가 끝나기 전 현재 대여를 반납하지 않는다.
    const auto raw=pool_.Slot(form);ProcessId pid=0;
    // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
    auto* process=dynamic_cast<SquidProcess*>(kernel_.Get(pid));
    if (!process) throw std::logic_error("form의 프로세스가 Kernel에 없습니다");
    process->OnFormDestroy(flags);
    // 원본은 통지가 끝난 뒤 Form의 현재 PID를 다시 읽는다. 콜백 중 다른 번호로 바뀐 경우 그 슬롯을 제거한다.
    pid=0;
    const auto current=pool_.Slot(form);
    // 재진입이 바꾼 DWORD만 새로 조립하고 처음 조회한 프로세스 포인터는 다시 읽지 않는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(current[kFormPid+i])<<(8*i);
    kernel_.Remove(pid);
    Destroy().CompletePreDestroy();
}
void SquidProcessHost::FormParentDying(Sid form,Sid parent) {
    const auto raw=pool_.Slot(form);ProcessId pid=0;
    // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
    auto* current=kernel_.Get(pid);if (!current) return; // 원문 +1c의 NULL 조회만 빈 통지로 생략한다.
    auto* process=dynamic_cast<SquidProcess*>(current);
    if (!process) throw std::logic_error("현재 Process 부모 통지의 실제 가상+10 미연결"); // 비어 있지 않은 다른 Kernel 클래스를 빈 성공으로 숨기지 않는다.
    process->OnParentDying(parent);
}
void SquidProcessHost::Kill(SquidProcess& process,std::uint32_t flags) {
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 실제 효과가 끝나기 전 현재 대여를 반납하지 않는다.
    const auto form=process.form_;
    // 권한이 없고 form이 서버 번호이며 client flag도 없으면 아무것도 하지 않는다.
    if (!state_.Boss() && !(form.value>4 && form.value<pool_.Layout().serverFirst) && !(flags&kClientForm)) return;
    if (form.value==0 || form.value>=pool_.Capacity() || (pool_.Slot(form)[kState]&kDead))
        throw std::logic_error("프로세스 form 상태 오류");
    Destroy().Destroy(form,flags,Hooks());
}
// 슬롯의 현재 주소와 비교만 한다. 이미 해제된 프로세스의 주소를 넘겨도 객체를 읽지 않는다.
bool SquidProcessHost::IsRegistered(ProcessId pid,const SquidProcess* process) const { return process && kernel_.Get(pid)==process; }
// 정렬되지 않은 form의 현재 Kernel 번호를 읽는다. 다른 종류의 BaseProcess는 null로 거부한다.
SquidProcess* SquidProcessHost::FromForm(Sid form) const {
    const auto raw=pool_.Slot(form);ProcessId pid=0;
    // 원본 getter처럼 DWORD 전체를 조립하며 WORD로 절단하지 않는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
    return dynamic_cast<SquidProcess*>(kernel_.Get(pid));
}
// 실제 가상34는 server0이면 PID조차 읽지 않는다. 정상 현재 PID의 가상notify만 호출한다.
void SquidProcessHost::NotifyDependency(Sid watcher,Sid watched,Sid dependForm) const {
    if (!state_.Server()) return;
    auto* process=FromForm(watcher);
    if (!process) throw std::logic_error("현재 Depend 통지의 실제 ProcessForm/PID 미연결");
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 직접 호출한 가상 통지가 끝날 때까지 현재 대여를 유지한다.
    process->Notify(2,dependForm.value,watched.value);
}
template<class Predicate> RegularProcess* SquidProcessHost::Find(Sid parent,Predicate match) const {
    if (parent.value==0 || parent.value>=pool_.Capacity()) throw std::out_of_range("이벤트 조회 SID 범위 오류");
    std::uint16_t current=Word(parent,kHead);std::uint32_t guard=0;
    // 가장 최근에 붙은 종속부터 다음 링크를 따라간다. 자산 타입은 건너뛴다.
    while (current) {
        if (current>=pool_.Capacity() || ++guard>pool_.Capacity()) throw std::logic_error("종속 체인 손상");
        const auto raw=pool_.Slot(Sid{current});const auto next=Word(Sid{current},kNext);
        const auto type=raw[kType];const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        const bool process=!patch || (type>=kFirstProcessType && type<=kLastFormType && static_cast<std::int32_t>(type)<std::bit_cast<std::int32_t>(ProcessTypeEnd())); // 원문의 프로세스 finder는 자산/일반 Form을 먼저 건너뛴다.
        if (process && (type==(patch ? state_.RegularType() : kRegularType) || type==(patch ? state_.SharedRegularType() : kSharedRegularType))) {
            ProcessId pid=0;
            // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
            for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
            auto* regular=dynamic_cast<RegularProcess*>(kernel_.Get(pid));
            if (!regular) throw std::logic_error("현재 Regular 종속의 Kernel PID/실제 클래스 미연결"); // 원문은현재예약의포인터+18을읽는다. 미등록/다른클래스를빈검색으로숨기지않는다.
            if (match(*regular)) return regular;
        }
        current=next;
    }
    return nullptr;
}
RegularProcess* SquidProcessHost::FindEvent(Sid parent,std::uint32_t event) const {
    return Find(parent,[event](const RegularProcess& process) { return process.Event()==event; });
}
RegularProcess* SquidProcessHost::FindEventMasked(Sid parent,std::uint32_t mask,std::uint32_t value) const {
    return Find(parent,[mask,value](const RegularProcess& process) { return (process.Event()&mask)==value; });
}
void SquidProcessHost::RunRegular(RegularProcess& process) {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto raw=pool_.Slot(process.parent_);
    if (raw[kState]&(kFree|kDead)) return;
    // geyser는 0.5초보다 먼 예약을 현재 시각으로 당긴다.
    if (raw[kType]==state_.PollType() && process.time_-state_.Now()>kPollLimit) process.time_=state_.Now();
    // 패치판만 abstract 부모(factory·vortex·dais 제외)를 건너뛴다. CD판은 genus를 읽기만 한다.
    if (patch && (raw[40]&1) && !(Type(raw[kType]).flags2&kAbstractRunGenus)) return;
    // 아직 시각이 안 됐거나 비교할 수 없으면(NaN) 실행하지 않는다.
    if (!(process.time_<=state_.Now())) return;
    if (raw[kState]&kVoid) return;
    const auto pid=process.pid_;
    RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // free/dead/void/시각 관문을 모두 지난 실제 사건 호출에만 대여를 유지한다.
    const auto handler=Regular();const float result=handler ? handler(process.parent_,process.event_,process.count_,process.payload_) : process.payload_;
    // 처리기가 이 프로세스를 지웠으면 더 건드리지 않는다(원본의 해제 표식 검사에 해당).
    if (kernel_.Get(pid)!=&process) return;
    if (result>0.0f || (patch && std::isnan(result))) {
        // 반환값이 다음 payload와 지연이 된다. 패치판은 NaN도 재예약으로 취급한다.
        process.payload_=result;
        process.time_=static_cast<double>(result)+state_.Now();
        ++process.count_;
    } else if (result==0.0f) Kill(process,0);
}
// 원본은 new(0x28) 뒤 Regular 생성자를 부른다. 좌표 포장은 앞서 기계어로 대조한 계산을 그대로 쓴다.
RegularProcess* ScheduleBridgeFall(SquidProcessHost& host,Sid bridge,float x,float y) {
    return host.AddRegular(bridge,kBridgeFallEvent,RawBridgeLifecycle::DelayedFallPayload(x,y));
}
bool HasScheduledBridgeFall(const SquidProcessHost& host,Sid bridge) { return host.FindEvent(bridge,kBridgeFallEvent)!=nullptr; }
SquidDestroyHooks SquidProcessHost::Hooks() {
    SquidDestroyHooks hooks;
    // form은 선택 대상이 아니므로 자산 훅이 없으면 선택 없음으로 답한다.
    if (currentRuntime_) hooks.selected=[this] { RuntimeCall call(currentRuntime_());const auto asset=Asset();return asset.selected ? asset.selected() : Sid{}; };
    else if (asset_.selected) hooks.selected=asset_.selected;else hooks.selected=[] { return Sid{}; };
    hooks.unpopForm=[this](Sid sid) { UnpopForm(sid); };
    hooks.emit=[this](const SquidDestroyEvent& event) {
        RuntimeCall call(currentRuntime_ ? currentRuntime_() : nullptr); // 선택한 가상 효과가 같은 현재 대여를 사용하는 동안 유지한다.
        // pre/post는 루트가, release/destroy는 종속이 ProcessForm일 때만 여기서 처리한다.
        if (IsProcessForm(event.sid)) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy:FormPreDestroy(event.sid,event.flags);return;
            case SquidDestroyEffect::PostDestroy:Destroy().CompletePostDestroy();return;
            case SquidDestroyEffect::ReleaseDependent:FormParentDying(event.sid,event.parent);return;
            case SquidDestroyEffect::DestroyDependent:Destroy().Destroy(event.sid,event.flags,Hooks());return;
            default:break;
            }
        }
        const auto asset=Asset();if (!asset.emit) throw std::logic_error("프로세스 host의 자산 삭제 훅 미연결");
        asset.emit(event);
    };
    return hooks;
}
}
