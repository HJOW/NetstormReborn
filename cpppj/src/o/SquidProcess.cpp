// 객체 부착 프로세스의 form 생성/부착/해제와 Regular 이벤트 실행을 원본 쓰기 순서대로 복원한다.
#include "o/SquidProcess.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드: 종속 체인의 다음(+4)·머리(+6), 타입(+10), 상태(+11).
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11;
// form은 이전 항목을 +0xe, 부모를 +0x10, Kernel 번호를 +0x12에 둔다. contained 자산은 부모가 +0xe, 이전 항목이 +0x10이다.
constexpr std::size_t kFormPrevious=14,kFormParent=16,kFormPid=18,kAssetPrevious=16;
// 상태 비트: free·dead·void·contained.
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4,kContained=8;
// 타입 번호: ProcessForm, 프로세스 타입의 첫 번호, Regular/SharedRegular, form으로 취급하는 마지막 번호.
constexpr std::uint32_t kProcessFormType=2,kFirstProcessType=10,kRegularType=46,kSharedRegularType=61,kLastFormType=0x45;
// 판본별 프로세스 타입 끝(패치 DAT_0054116c, CD DAT_0051ca58)과 ProcessForm 가상 표의 실제 주소 기록값.
constexpr std::uint32_t kPatchProcessEnd=63,kCdProcessEnd=62,kPatchFormVtable=0x512098,kCdFormVtable=0x505990;
// 부착 flags: 0x10 client 번호로 form 할당, 0x10|0x40 전송 억제, 0x80 부모 변경 허용(pfPROCESS_DEBUG).
constexpr std::uint32_t kClientForm=0x10,kLocalMask=0x50,kProcessDebug=0x80;
// form을 넣을 수 없는 부모 genus(emplacement·vortex·factory)와 abstract여도 실행하는 genus.
constexpr std::uint32_t kNoContainerGenus=0x44200,kAbstractRunGenus=0x404200;
// geyser에 붙은 Regular가 허용하는 가장 먼 예약(초)이다.
constexpr double kPollLimit=0.5;
}
Sid SquidProcess::Form() const { return form_; }
Sid SquidProcess::Parent() const { return parent_; }
void SquidProcess::OnFormDestroy(std::uint32_t) {}
void SquidProcess::OnParentDying(Sid) {}
void SquidProcess::OnAttached() {}
SquidProcessHost* SquidProcess::Host() const { return host_; }

RegularProcess::RegularProcess(std::uint32_t event,float payload):event_(event),payload_(payload) {}
// 부착하지 않은 프로세스는 Kernel에 없으므로 실행되지 않는다.
void RegularProcess::RunFrame() { if (auto* host=Host()) host->RunRegular(*this); }
std::uint32_t RegularProcess::Event() const { return event_; }
float RegularProcess::Payload() const { return payload_; }
std::uint32_t RegularProcess::Count() const { return count_; }
double RegularProcess::Time() const { return time_; }

SquidProcessHost::SquidProcessHost(SidPool& pool,std::span<const RiftTypeRecord> types,Kernel& kernel,RawSquidDestroy& destroy,
    SquidProcessState& state,RegularHandler handler,SquidDestroyHooks asset)
    :pool_(pool),types_(types.begin(),types.end()),kernel_(kernel),destroy_(destroy),state_(state),
     handler_(std::move(handler)),asset_(std::move(asset)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("프로세스 타입 판본/크기 오류");
    if (&destroy.Pool()!=&pool) throw std::invalid_argument("프로세스/삭제 SID 풀이 다릅니다");
}
std::uint32_t SquidProcessHost::ProcessTypeEnd() const { return pool_.Edition()==OriginalEdition::Patch1078 ? kPatchProcessEnd : kCdProcessEnd; }
std::uint32_t SquidProcessHost::FormVtable() const { return pool_.Edition()==OriginalEdition::Patch1078 ? kPatchFormVtable : kCdFormVtable; }
std::uint16_t SquidProcessHost::Word(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);
    return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
void SquidProcessHost::PutWord(Sid sid,std::size_t offset,std::uint16_t value) {
    auto raw=pool_.AllocatedBytes(sid);
    raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8);
}
bool SquidProcessHost::IsProcessForm(Sid sid) const {
    if (sid.value==0 || sid.value>=pool_.Capacity()) return false;
    const auto type=pool_.Slot(sid)[kType];
    return type>=kFirstProcessType && type<ProcessTypeEnd();
}
// form의 가상 setPrev는 +0xe, contained 자산의 것은 +0x10에 쓴다.
void SquidProcessHost::SetPrevious(Sid sid,std::uint16_t previous) {
    PutWord(sid,pool_.Slot(sid)[kType]<=kLastFormType ? kFormPrevious : kAssetPrevious,previous);
}
void SquidProcessHost::AttachForm(Sid form,Sid parent,std::uint32_t flags) {
    // 004b1440: 부모가 바뀌는 부착은 프로세스 디버그 flag가 있어야 한다. 새 form의 부모 필드는 0이다.
    if (Word(form,kFormParent)!=parent.value && !(flags&kProcessDebug)) throw std::logic_error("프로세스 form 부모 변경 flag 누락");
    if (types_.at(pool_.Slot(form)[kType]).flags2&kNoContainerGenus) throw std::logic_error("프로세스 form genus 오류");
    // 004ac800: 부모 기록 → 기존 머리 앞에 연결 → 머리 교체 → 이전 항목 0 → contained.
    PutWord(form,kFormParent,parent.value);
    const auto head=Word(parent,kHead);
    if (head==form.value) throw std::logic_error("프로세스 form 종속 체인 자기 참조");
    PutWord(form,kNext,head);
    if (head) SetPrevious(Sid{head},form.value);
    PutWord(parent,kHead,form.value);
    PutWord(form,kFormPrevious,0);
    auto raw=pool_.AllocatedBytes(form);
    raw[kState]|=kContained;
    // 004ac560: void를 끈다. 전송은 로컬 flags로 건너뛰고 postPop 깊이는 form의 postPop이 바로 되돌린다.
    if (!(raw[kState]&kVoid)) throw std::logic_error("프로세스 form Pop은 void 상태여야 합니다");
    raw[kState]&=static_cast<std::uint8_t>(~kVoid);
}
SquidProcess* SquidProcessHost::Attach(std::unique_ptr<SquidProcess> process,std::uint32_t processType,Sid parent,std::uint32_t flags) {
    if (!process) throw std::invalid_argument("부착할 프로세스가 없습니다");
    if (processType<kFirstProcessType || processType>=ProcessTypeEnd()) throw std::invalid_argument("프로세스 타입 번호 오류");
    if (parent.value==0 || parent.value>=pool_.Capacity()) throw std::out_of_range("프로세스 부착 SID 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (pool_.Slot(parent)[kState]&(kFree|kDead)) {
        // 패치판은 조용히 건너뛴다. CD판은 assert를 보고한 뒤 계속하므로 지원하지 않는다.
        if (patch) return nullptr;
        throw std::logic_error("CD판의 free/dead 부모 부착은 지원하지 않습니다");
    }
    // 서버는 로컬 flags가 없으면 프로세스를 직렬화해 전송한다. 그 경로는 복원하지 않았다.
    if (pool_.IsServer() && !(flags&kLocalMask)) throw std::logic_error("프로세스 전송 직렬화는 복원하지 않았습니다");
    auto* attached=process.get();
    const auto pid=kernel_.Add(std::move(process));
    const auto rotation=state_.typeRotation;
    // 패치판 Kernel 등록은 타입 순환 값을 하나 전진시킨다(0049c2c0).
    if (patch) state_.typeRotation=(state_.typeRotation+1)%static_cast<std::uint32_t>(types_.size());
    Sid form{};
    try {
        // 004af530: 번호 할당 → ProcessForm 생성자(가상 표) → 타입. form의 postCreate는 빈 함수다.
        form=pool_.Allocate((flags&kClientForm) ? 2U : 0U);
    } catch (...) {
        // 번호 소진이면 등록을 되돌려 form 없는 프로세스가 남지 않게 한다.
        kernel_.Remove(pid);state_.typeRotation=rotation;
        throw;
    }
    auto raw=pool_.AllocatedBytes(form);
    const auto vtable=FormVtable();
    // 가상 표 기록값은 호스트에서 역참조하지 않는다.
    for (std::size_t i=0;i<4;++i) raw[i]=static_cast<std::uint8_t>(vtable>>(8*i));
    raw[kType]=static_cast<std::uint8_t>(kProcessFormType);
    // 004ae2e0: 타입 번호를 프로세스 타입으로 바꾸고 Kernel 번호를 기록한다.
    raw[kType]=static_cast<std::uint8_t>(processType);
    for (std::size_t i=0;i<4;++i) raw[kFormPid+i]=static_cast<std::uint8_t>(pid>>(8*i));
    attached->form_=form;attached->parent_=parent;attached->pid_=pid;attached->host_=this;
    AttachForm(form,parent,flags|kProcessDebug);
    attached->OnAttached();
    return attached;
}
RegularProcess* SquidProcessHost::AddRegular(Sid parent,std::uint32_t event,float payload,std::uint32_t flags) {
    auto* attached=static_cast<RegularProcess*>(Attach(std::make_unique<RegularProcess>(event,payload),kRegularType,parent,flags));
    // 생성자는 부착 뒤 시각을 현재 시각으로 둔다. 첫 호출은 다음 Kernel 프레임이다.
    if (attached) attached->time_=state_.now;
    return attached;
}
// 같은 Regular 객체를 공유 타입으로 부착한다. 부모 체인/Kernel/예약 규칙을 재사용한다.
// 원본: 00496f00 / CD 0048f1d0. 실행 vtable +0x18은 일반 Regular와 동일하다.
RegularProcess* SquidProcessHost::AddSharedRegular(Sid parent,std::uint32_t event,float payload) {
    auto* attached=static_cast<RegularProcess*>(Attach(std::make_unique<RegularProcess>(event,payload),kSharedRegularType,parent,0x50));
    if (attached) attached->time_=state_.now;
    return attached;
}
// 실제 부착과 사제 낙하 연결이 같은 풀인지 검사할 때 사용한다.
const SidPool& SquidProcessHost::Pool() const { return pool_; }
void SquidProcessHost::UnpopForm(Sid form) {
    const auto raw=pool_.Slot(form);
    if (raw[kType]>kLastFormType || (raw[kState]&kVoid)) throw std::logic_error("프로세스 form Unpop 상태 오류");
    pool_.AllocatedBytes(form)[kState]|=kVoid;
    const Sid parent{Word(form,kFormParent)};
    if (parent.value==0 || parent.value>=pool_.Capacity()) throw std::logic_error("프로세스 form 부모 번호 오류");
    const auto parentState=pool_.Slot(parent)[kState];
    if (parentState&kFree) return;
    // 004ac6b0: 부모가 지워지는 중(dead)이면 체인을 고치지 않고 contained도 남긴다.
    if (!(pool_.Slot(form)[kState]&kContained)) throw std::logic_error("프로세스 form이 contained가 아닙니다");
    if (parentState&kDead) return;
    const auto next=Word(form,kNext),previous=Word(form,kFormPrevious);
    if (previous) {
        if (next==previous) throw std::logic_error("프로세스 form 종속 체인 자기 참조");
        PutWord(Sid{previous},kNext,next);
    }
    if (next) SetPrevious(Sid{next},previous);
    if (Word(parent,kHead)==form.value) {
        if (previous) throw std::logic_error("프로세스 form 머리의 이전 항목 오류");
        PutWord(parent,kHead,next);
    }
    pool_.AllocatedBytes(form)[kState]&=static_cast<std::uint8_t>(~kContained);
}
void SquidProcessHost::FormPreDestroy(Sid form,std::uint32_t flags) {
    const auto raw=pool_.Slot(form);ProcessId pid=0;
    // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
    auto* process=dynamic_cast<SquidProcess*>(kernel_.Get(pid));
    if (!process) throw std::logic_error("form의 프로세스가 Kernel에 없습니다");
    process->OnFormDestroy(flags);
    kernel_.Remove(pid);
    destroy_.CompletePreDestroy();
}
void SquidProcessHost::FormParentDying(Sid form,Sid parent) {
    const auto raw=pool_.Slot(form);ProcessId pid=0;
    // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
    for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
    if (auto* process=dynamic_cast<SquidProcess*>(kernel_.Get(pid))) process->OnParentDying(parent);
}
void SquidProcessHost::Kill(SquidProcess& process,std::uint32_t flags) {
    const auto form=process.form_;
    // 권한이 없고 form이 서버 번호이며 client flag도 없으면 아무것도 하지 않는다.
    if (!state_.boss && !(form.value>4 && form.value<pool_.Layout().serverFirst) && !(flags&kClientForm)) return;
    if (form.value==0 || form.value>=pool_.Capacity() || (pool_.Slot(form)[kState]&kDead))
        throw std::logic_error("프로세스 form 상태 오류");
    destroy_.Destroy(form,flags,Hooks());
}
template<class Predicate> RegularProcess* SquidProcessHost::Find(Sid parent,Predicate match) const {
    if (parent.value==0 || parent.value>=pool_.Capacity()) throw std::out_of_range("이벤트 조회 SID 범위 오류");
    std::uint16_t current=Word(parent,kHead);std::uint32_t guard=0;
    // 가장 최근에 붙은 종속부터 다음 링크를 따라간다. 자산 타입은 건너뛴다.
    while (current) {
        if (current>=pool_.Capacity() || ++guard>pool_.Capacity()) throw std::logic_error("종속 체인 손상");
        const auto raw=pool_.Slot(Sid{current});const auto next=Word(Sid{current},kNext);
        if (raw[kType]==kRegularType || raw[kType]==kSharedRegularType) {
            ProcessId pid=0;
            // 정렬되지 않은 DWORD를 낮은 바이트부터 읽는다.
            for (std::size_t i=0;i<4;++i) pid|=static_cast<ProcessId>(raw[kFormPid+i])<<(8*i);
            auto* regular=dynamic_cast<RegularProcess*>(kernel_.Get(pid));
            if (regular && match(*regular)) return regular;
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
    if (raw[kType]==state_.pollType && process.time_-state_.now>kPollLimit) process.time_=state_.now;
    // 패치판만 abstract 부모(factory·vortex·dais 제외)를 건너뛴다. CD판은 genus를 읽기만 한다.
    if (patch && (raw[40]&1) && !(types_.at(raw[kType]).flags2&kAbstractRunGenus)) return;
    // 아직 시각이 안 됐거나 비교할 수 없으면(NaN) 실행하지 않는다.
    if (!(process.time_<=state_.now)) return;
    if (raw[kState]&kVoid) return;
    const auto pid=process.pid_;
    const float result=handler_ ? handler_(process.parent_,process.event_,process.count_,process.payload_) : process.payload_;
    // 처리기가 이 프로세스를 지웠으면 더 건드리지 않는다(원본의 해제 표식 검사에 해당).
    if (kernel_.Get(pid)!=&process) return;
    if (result>0.0f || (patch && std::isnan(result))) {
        // 반환값이 다음 payload와 지연이 된다. 패치판은 NaN도 재예약으로 취급한다.
        process.payload_=result;
        process.time_=static_cast<double>(result)+state_.now;
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
    if (asset_.selected) hooks.selected=asset_.selected;
    else hooks.selected=[] { return Sid{}; };
    hooks.unpopForm=[this](Sid sid) { UnpopForm(sid); };
    hooks.emit=[this](const SquidDestroyEvent& event) {
        // pre/post는 루트가, release/destroy는 종속이 ProcessForm일 때만 여기서 처리한다.
        if (IsProcessForm(event.sid)) {
            switch (event.effect) {
            case SquidDestroyEffect::PreDestroy:FormPreDestroy(event.sid,event.flags);return;
            case SquidDestroyEffect::PostDestroy:destroy_.CompletePostDestroy();return;
            case SquidDestroyEffect::ReleaseDependent:FormParentDying(event.sid,event.parent);return;
            case SquidDestroyEffect::DestroyDependent:destroy_.Destroy(event.sid,event.flags,Hooks());return;
            default:break;
            }
        }
        if (!asset_.emit) throw std::logic_error("프로세스 host의 자산 삭제 훅 미연결");
        asset_.emit(event);
    };
    return hooks;
}
}
