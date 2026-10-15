// 원본 상태 변경의 폭과 호출 이후 현재 필드 재조회를 보존한다.
#include "o/RawGunProcess.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 같은 방향별 DWORD의 nonzero를 읽으며 기존 bool 표는 독립 입력 대조에만 남긴다.
bool GunProcessState::IsAlly(std::uint32_t owner,std::uint32_t target) const {
    if (owner>=allies.size() || target>=allies.size()) throw std::out_of_range("Gun 현재 방향별 동맹 범위 오류");
    return allianceRead ? allianceRead(owner,target)!=0 : allies[owner][target];
}
namespace {
// outer GunProcess 및 inner Baseprocess의 원본 필드 위치다.
constexpr std::size_t kDirty=8,kTime=16,kTarget=24,kStage=28,kGroup=32,kInner=40,kForm=44,kParent=48;
// 두 PE의 기록용 가상 표와 idle 검색의 float 지연 바이어스다.
constexpr std::uint32_t kPatchOuter=0x50b668,kCdOuter=0x501118,kPatchInner=0x50b648,kCdInner=0x5010f8;
constexpr float kPollBias=0.4f,kPollRandom=0.2f;
// 비정렬 little endian 필드를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정한 바이트 폭만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// padding과 다른 필드를 보존하며 원본 쓰기 폭으로 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 낮은 바이트부터 기록한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 double 필드를 비트 그대로 읽고 기록한다.
double GetTime(std::span<const std::uint8_t> bytes) {
    const auto bits=static_cast<std::uint64_t>(Read(bytes,kTime))|(static_cast<std::uint64_t>(Read(bytes,kTime+4))<<32);
    return std::bit_cast<double>(bits);
}
void PutTime(GunProcessFields& fields,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,kTime,static_cast<std::uint32_t>(bits));Put(fields.bytes,kTime+4,static_cast<std::uint32_t>(bits>>32));
}
// 다음 시각/상태/표적 변경은 해당 원본 dirty 비트만 OR한다.
void Time(GunProcessFields& fields,double value) { fields.bytes[kDirty]|=2;PutTime(fields,value); }
void Stage(GunProcessFields& fields,std::uint32_t value) { fields.bytes[kDirty]|=8;Put(fields.bytes,kStage,value); }
void Target(GunProcessFields& fields,Sid value) { fields.bytes[kDirty]|=4;Put(fields.bytes,kTarget,value.value,2); }
// 현재 raw IEEE float 좌표를 읽는다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(Read(raw,offset)); }
}
// raw 객체에서 실제 필드만 조회한다.
std::uint8_t GunProcessFields::Dirty() const { return bytes[kDirty]; }
double GunProcessFields::Time() const { return GetTime(bytes); }
Sid GunProcessFields::Target() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kTarget,2))}; }
std::uint32_t GunProcessFields::Stage() const { return Read(bytes,kStage); }
std::int32_t GunProcessFields::Group() const { return std::bit_cast<std::int32_t>(Read(bytes,kGroup)); }
Sid GunProcessFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; }
Sid GunProcessFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; }
// 현재 raw 자산과 같은 판본/크기의 불변 타입 표를 연결한다.
RawGunProcess::RawGunProcess(SidPool& pool,std::span<const RiftTypeRecord> types,GunProcessState& state,GunProcessHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("GunProcess 타입 판본/크기 오류");
}
// inner 부모의 현재 타입·group을 읽는다. 이벤트가 바꾼 group은 매 프레임 다시 검사한다.
std::span<const std::uint8_t> RawGunProcess::Parent(const GunProcessFields& fields) const {
    if (!fields.Parent().value || fields.Parent().value>=pool_.Capacity()) throw std::out_of_range("GunProcess 부모 SID 오류");
    const auto raw=pool_.Slot(fields.Parent());
    if (raw[10]>=types_.size()) throw std::out_of_range("GunProcess 부모 타입 오류");
    return raw;
}
// 실제 outer/inner vtable·zero 필드·현재 시각·group·flags0 부착의 순서를 복원한다.
void RawGunProcess::Construct(GunProcessFields& fields,Sid parent) const {
    if (!hooks_.attach) throw std::invalid_argument("GunProcess 생성 부착 경계 오류");
    Initialize(fields,parent);hooks_.attach(fields,parent);
}
// 실제 생성자 초기화 접두를 보존하고 host flags0 부착은 별도 factory가 즉시 수행한다.
void RawGunProcess::Initialize(GunProcessFields& fields,Sid parent) const {
    if (!parent.value || parent.value>=pool_.Capacity()) throw std::invalid_argument("GunProcess 생성 부모 SID 오류");
    const auto raw=pool_.Slot(parent);const auto group=types_.at(raw[10]).group;
    if (group!=0 && group!=1) throw std::logic_error("GunProcess 생성 group 원본 assert");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    Put(fields.bytes,0,patch ? kPatchOuter : kCdOuter);Put(fields.bytes,kInner,patch ? kPatchInner : kCdInner);
    fields.bytes[kDirty]=0xff;Put(fields.bytes,kForm,0);Put(fields.bytes,kParent,0);
    Target(fields,{});Time(fields,state_.now);Stage(fields,0);fields.bytes[kDirty]|=0x10;Put(fields.bytes,kGroup,std::bit_cast<std::uint32_t>(group));
}
// 새 부착과 동일 form 재수신은 애니메이션 대기 단계만 되돌린다.
void RawGunProcess::Attached(GunProcessFields& fields) {
    if (fields.Stage()==3) Stage(fields,2);
    if (fields.Stage()==6) Stage(fields,5);
}
// clear(false)는 stage를 보존하며 clear(true)는 idle로 돌아간다. 전송 필요성은 변경 전 값으로 결정한다.
void RawGunProcess::ClearTarget(GunProcessFields& fields,bool restart) const {
    if (!state_.boss) return;
    if (!state_.server) throw std::logic_error("GunProcess clear 서버 원본 assert");
    const auto old=fields.Target();const bool transmit=restart ? (old.value || fields.Stage()!=1) : old.value!=0;
    // 표적0/전송없음의 자연 분기는 실제 외부 호출이 없으므로 미사용 경계를 요구하지 않는다.
    if ((old.value && !hooks_.removeDependency) || (transmit && !hooks_.repopForm)) throw std::logic_error("GunProcess clear 실제 효과 경계 누락");
    if (old.value) hooks_.removeDependency(fields.Form(),old);
    Target(fields,{});if (restart) Stage(fields,1);
    if (transmit) hooks_.repopForm(fields);
}
// 표적 교체는 공격 모드와 무관한 서버 작업이다. 원본은 current AnimationForm을 반복 조회한다.
void RawGunProcess::SetTarget(GunProcessFields& fields,Sid target) const {
    if (!state_.server || !hooks_.removeDependency || !hooks_.addDependency || !hooks_.animationForm || !hooks_.killAnimation || !hooks_.refreshRange || !hooks_.repopForm)
        throw std::logic_error("GunProcess 표적 지정 효과 누락");
    if (fields.Target().value) hooks_.removeDependency(fields.Form(),fields.Target());
    Target(fields,target);hooks_.addDependency(fields.Target(),fields);Stage(fields,2);
    if (hooks_.animationForm(fields.Parent()).value) {
        if (pool_.Edition()==OriginalEdition::Patch1078) {
            if (hooks_.animationForm(fields.Parent()).value) hooks_.killAnimation(hooks_.animationForm(fields.Parent()),0x40);
        } else {
            // CD의 stop helper도 현재 animation을 검사한 뒤 다시 조회하므로 총 세 번이다.
            if (hooks_.animationForm(fields.Parent()).value) hooks_.killAnimation(hooks_.animationForm(fields.Parent()),0x40);
        }
    }
    hooks_.refreshRange(fields.Parent());hooks_.repopForm(fields);
}
// patch debug에서만 실제 애니메이션 조회와 원본 assert를 실행한다.
void RawGunProcess::CheckNoAnimation(GunProcessFields& fields) const {
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.checking) {
        if (!hooks_.animationForm || hooks_.animationForm(fields.Parent()).value) throw std::logic_error("GunProcess getAnimFormSid==0 원본 assert");
    }
}
// 상태 전환은 시간 및 현재 표적 검사를 통과한 한 프레임에 한 단계만 진행한다.
void RawGunProcess::Frame(GunProcessFields& fields) const {
    auto parent=Parent(fields);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto extra=patch ? 40U : 35U,owner=patch ? 34U : 32U;
    if (fields.Group()!=types_[parent[10]].group || (parent[extra]&9)) throw std::logic_error("GunProcess Frame group/abstract/astral 원본 assert");
    if ((parent[extra]&0x20) || state_.fixedMode || state_.disabled) return;
    // 원본 boss 조건이 거짓이면 표적 슬롯/소유자/동맹 조회 자체를 실행하지 않는다.
    if (state_.boss) {
        const auto target=fields.Target();const bool valid=target.value && target.value<pool_.Capacity();
        bool clear=!valid;
        if (valid) {
            const auto raw=pool_.Slot(target);const auto first=parent[owner],second=raw[owner];
            if (first>kPlayerCount || second>kPlayerCount) throw std::out_of_range("GunProcess 동맹 소유자 범위 오류");
            clear=(raw[11]&3) || (state_.IsAlly(first,second) && !state_.friendlyFire);
        }
        if (clear) ClearTarget(fields,false);
    }
    parent=Parent(fields);
    // 실제 x87 분기: 패치 test C0는 unordered도 반환하고 CD test C0|C3는 unordered를 진행한다.
    if (patch ? !(fields.Time()<=state_.now) : state_.now<fields.Time()) return;
    if (parent[extra]&1) return;
    const auto type=parent[10];const auto stage=fields.Stage();
    switch (stage) {
    case 0:
        CheckNoAnimation(fields);
        if (patch && state_.checking && fields.Target().value) throw std::logic_error("GunProcess idle 표적 원본 assert");
        break;
    case 1:
        if (patch && state_.checking && fields.Target().value) throw std::logic_error("GunProcess 검색 표적 원본 assert");
        if (state_.server && (!state_.fortMode || state_.friendlyFire)) {
            if (!hooks_.targetingMode || !hooks_.findTarget || !hooks_.randomBetween) throw std::logic_error("GunProcess 검색 경계 누락");
            const auto mode=hooks_.targetingMode(fields.Parent(),parent[owner]);
            const auto found=hooks_.findTarget(Coordinate(parent,14),Coordinate(parent,18),type,mode,parent[owner]);
            if (found.value) SetTarget(fields,found);
            else {
                Time(fields,state_.now);const auto random=hooks_.randomBetween(0,kPollRandom);
                PutTime(fields,(random+static_cast<double>(kPollBias))+state_.now);
            }
        }
        if (patch && state_.checking) { if (!hooks_.validateIslandOwners) throw std::logic_error("GunProcess debug 지면 진단 누락");hooks_.validateIslandOwners(fields.Parent()); }
        return;
    case 2:
        CheckNoAnimation(fields);
        if (fields.Target().value) {
            const auto group=types_.at(Parent(fields)[10]).group;
            if ((group!=0 || type==state_.windArcherType) && type!=state_.thunderCannonType && type!=state_.rainCannonType) {
                if (!hooks_.startAnimation) throw std::logic_error("GunProcess 회전 애니메이션 경계 누락");
                hooks_.startAnimation(stage,fields.Form(),fields.Target());Stage(fields,3);
            } else Stage(fields,4);
            return;
        }
        break;
    case 3:case 6:case 9:return;
    case 4:
        CheckNoAnimation(fields);fields.bytes[kDirty]|=8;
        if (fields.Target().value) { Put(fields.bytes,kStage,5);return; }
        Put(fields.bytes,kStage,1);return;
    case 5:
        if (fields.Target().value) {
            CheckNoAnimation(fields);if (!hooks_.startAnimation) throw std::logic_error("GunProcess 발사 애니메이션 경계 누락");
            hooks_.startAnimation(stage,fields.Form(),fields.Target());Stage(fields,6);return;
        }
        break;
    case 7:CheckNoAnimation(fields);Fire(fields);return;
    case 8:
        if (!hooks_.startAnimation) throw std::logic_error("GunProcess 종료 애니메이션 경계 누락");
        hooks_.startAnimation(8,fields.Form(),{});Stage(fields,9);return;
    case 10:break;
    default:throw std::logic_error("GunProcess 단계 원본 assert");
    }
    Stage(fields,1);
}
// Animation 종료 메시지와 감시 표적 삭제 메시지는 원본 단계/권한 조건으로 처리한다.
void RawGunProcess::Notify(GunProcessFields& fields,std::uint32_t event,std::uint32_t,std::uint32_t value) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (event==1) {
        if (value==4 || (value==7 && fields.Stage()==6)) {
            if (patch && state_.checking && (!hooks_.animationForm || !hooks_.animationForm(fields.Parent()).value)) throw std::logic_error("GunProcess notify AnimationForm 원본 assert");
            Stage(fields,value);return;
        }
        if (value!=7 && value!=10) throw std::logic_error("GunProcess animation notify 원본 assert");
    } else if (state_.server && event==2) {
        if (patch && state_.checking && value!=fields.Target().value) throw std::logic_error("GunProcess notify 표적 원본 assert");
        if (value==fields.Target().value) {
            ClearTarget(fields,true);
            if (!(Parent(fields)[11]&2)) { if (!hooks_.repopForm) throw std::logic_error("GunProcess notify 재전송 누락");hooks_.repopForm(fields); }
        }
    }
}
// 실제 shot 판정/생성이 성공하면 상태2, 실패하면 idle 재시작이며 두 경우 현재 cooldown은 기록한다.
void RawGunProcess::Fire(GunProcessFields& fields) const {
    const auto parent=Parent(fields);
    if (!hooks_.parentBlocked || !hooks_.shotContext || !hooks_.shoot || !hooks_.firingInterval) throw std::logic_error("GunProcess 실제 발사 경계 누락");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch && state_.checking) { if (!hooks_.checkForms) throw std::logic_error("GunProcess 발사 Form 진단 누락");hooks_.checkForms(fields.Parent()); }
    if (hooks_.parentBlocked(fields.Parent()) || state_.fixedMode) return;
    const auto context=hooks_.shotContext(fields.Parent());const auto target=fields.Target();
    const bool valid=target.value && target.value<pool_.Capacity() && !(pool_.Slot(target)[11]&3);
    if (valid && hooks_.shoot(context,target)) Stage(fields,2);else ClearTarget(fields,true);
    const auto interval=hooks_.firingInterval(fields.Parent());Time(fields,state_.now+interval);
    if (patch && state_.checking) hooks_.checkForms(fields.Parent());
    static_cast<void>(parent);
}
// 수신 뒤 동작은 현재 raw 단계를 검사하며 stop helper의 현재 animation 두 번 조회를 보존한다.
void RawGunProcess::ReceiveUpdate(GunProcessFields& fields,std::uint8_t dirty,std::uint32_t parentChanged) const {
    if (!parentChanged || !(dirty&8) || (fields.Stage()!=3 && fields.Stage()!=6)) return;
    if (!hooks_.animationForm || !hooks_.killAnimation) throw std::logic_error("GunProcess 수신 animation 중지 경계 누락");
    if (hooks_.animationForm(fields.Parent()).value) hooks_.killAnimation(hooks_.animationForm(fields.Parent()),0x40);
}
// 원본 쓰기 폭/dirty 소비를 유지하며 부족한 버퍼는 변경 전에 거부한다.
std::uint16_t RawGunProcess::WriteData(GunProcessFields& fields,std::span<std::uint8_t> packet,
    std::uint32_t form,std::uint32_t parent,bool force,std::uint8_t protocol,std::uint8_t message) {
    const auto dirty=force ? 0xffU : fields.Dirty();const auto length=9U+(parent ? 4U : 0U)+((dirty&2) ? 8U : 0U)+((dirty&4) ? 2U : 0U)+((dirty&8) ? 4U : 0U)+((dirty&0x10) ? 4U : 0U);
    if (packet.size()<length) throw std::out_of_range("GunProcess 패킷 버퍼 크기 오류");
    if (force) fields.bytes[kDirty]=0xff;
    packet[0]=protocol;packet[1]=message;Put(packet,4,form);std::size_t cursor=9;
    if (parent) { fields.bytes[kDirty]|=1;Put(packet,cursor,parent);cursor+=4; }
    // 현재 dirty가 선택한 8/2/4/4바이트 raw 필드를 순서대로 복사한다.
    for (const auto [bit,offset,width]:{std::array<std::size_t,3>{2,kTime,8},{4,kTarget,2},{8,kStage,4},{16,kGroup,4}})
        if (fields.Dirty()&bit) { std::copy_n(fields.bytes.begin()+static_cast<std::ptrdiff_t>(offset),width,packet.begin()+static_cast<std::ptrdiff_t>(cursor));cursor+=width; }
    Put(packet,2,static_cast<std::uint32_t>(cursor),2);packet[8]=fields.Dirty();fields.bytes[kDirty]=0;
    return static_cast<std::uint16_t>(cursor);
}
// 가상 writer는 includeParent/force가 하나라도 참이면 inner 부모 DWORD를 전송한다.
std::uint16_t RawGunProcess::WriteProcess(GunProcessFields& fields,std::span<std::uint8_t> packet,
    bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message) {
    return WriteData(fields,packet,Read(fields.bytes,kForm),(includeParent || force) ? Read(fields.bytes,kParent) : 0U,force,protocol,message);
}
// Kernel 어댑터의 풀 혼용을 구성 시 거부하도록 반환한다.
const SidPool& RawGunProcess::Pool() const { return pool_; }
// 원본 raw 필드의 소유권을 실제 Kernel 객체에 넘긴다.
GunProcess::GunProcess(const RawGunProcess& logic,GunProcessFields fields):logic_(logic),fields_(std::move(fields)) {}
// actual inner 가상 +0x18을 복원한 Frame 몸체로 넘긴다.
void GunProcess::RunFrame() { logic_.Frame(fields_); }
// 원본 Baseprocess 부착의 결과를 현재 raw 필드에 반영한다.
void GunProcess::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value);RawGunProcess::Attached(fields_); }
// 외부 전송/메시지는 Kernel과 같은 raw 필드를 사용한다.
GunProcessFields& GunProcess::Fields() { return fields_; }
const GunProcessFields& GunProcess::Fields() const { return fields_; }
// inner +0x1c의 실제 메시지를 현재 객체 raw에 처리한다.
void GunProcess::Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) { logic_.Notify(fields_,event,info,value); }
// 원본 writer 시점에는 Baseprocess Attach 필드가 이미 쓰였지만 OnAttached는 아직 실행되지 않았다.
std::uint16_t GunProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force,std::uint8_t protocol,std::uint8_t message) {
    Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value);
    return RawGunProcess::WriteProcess(fields_,packet,parentChanged,force,protocol,message);
}
// 실제 flags0 부착을 유지한다. host의 미연결 전송 예외는 호출자에게 그대로 전달된다.
GunProcess* AttachGunProcess(SquidProcessHost& host,const RawGunProcess& logic,GunProcessFields fields,Sid parent,std::uint32_t type) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("GunProcess/Host SID 풀 불일치");
    auto process=std::make_unique<GunProcess>(logic,std::move(fields));return dynamic_cast<GunProcess*>(host.Attach(std::move(process),type,parent,0));
}
// new0x38의 생성자 필드를 준비한 뒤 같은 SID 저장소의 실제 flags0/Kernel 부착으로 넘긴다.
GunProcess* CreateGunProcess(SquidProcessHost& host,const RawGunProcess& logic,Sid parent,std::uint32_t type) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("GunProcess/Host SID 풀 불일치");
    GunProcessFields fields;logic.Initialize(fields,parent);return AttachGunProcess(host,logic,std::move(fields),parent,type);
}
}
