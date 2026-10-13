// 실제 PE의 방향별 프레임 표·발사 표시·완료 사건을 원본 필드 폭으로 실행한다.
#include "o/RawGunAnimation.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 outer Baseprocess와 내장 dirty 데이터의 byte offset이다.
constexpr std::size_t kForm=4,kParent=8,kPosition=12,kCenter=20,kInner=32,kDirty=40,kGun=42,kTarget=44,kTime=48,kRest=56;
// 원위치 stage8이 8번 방향까지 읽으므로 원본의 다섯 행 전체를 보존한다.
constexpr std::array<std::array<std::uint32_t,28>,5> kSunRotate{{
    {{4,32768,12,19,5,6,7,8,9,10,11,1,13,14,15,16,20,16,17,18,21,8,8,22,32768,32768,32768,32768}},
    {{4,11,12,32768,5,6,7,8,22,8,9,10,13,14,15,16,17,18,19,3,16,20,23,16,32768,32768,32768,32768}},
    {{32768,11,12,19,0,4,5,6,7,8,9,10,13,14,15,16,20,16,17,18,21,8,8,22,32768,32768,32768,32768}},
    {{4,11,32768,19,5,6,7,8,22,8,9,10,2,12,13,14,15,16,17,18,16,20,23,16,32768,32768,32768,32768}},
    {{4,11,12,19,5,6,7,8,32768,8,9,10,13,14,15,16,32768,16,17,18,32768,20,23,16,32768,32768,32768,32768}}
}};
// 회전/발사 각각의 실제 float 대기 간격이다.
constexpr float kRotateDelay=0.2f,kFireDelay=0.07f;
// 미정렬 little endian 필드를 정확한 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 바이트부터 원본 필드 폭만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 주변 padding을 보존하며 원본 필드 폭으로 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 낮은 바이트부터 지정한 필드만 쓴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 현재 double 필드를 raw 비트 그대로 기록한다.
void PutTime(GunAnimationFields& fields,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,kTime,static_cast<std::uint32_t>(bits));Put(fields.bytes,kTime+4,static_cast<std::uint32_t>(bits>>32));
}
// dirty8과 다음 실행 시각을 같은 순서로 기록한다.
void Time(GunAnimationFields& fields,double value) { fields.bytes[kDirty]|=8;PutTime(fields,value); }
// 원본 float32 좌표를 읽는다.
float Coordinate(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Read(bytes,offset)); }
// BoardCoord의 4방향 판정이며 Y 절댓값 동률에서는 세로 방향이 앞선다.
std::int32_t Direction(std::array<float,2> from,std::array<float,2> to) {
    const float dx=static_cast<float>(static_cast<double>(to[0])-from[0]),dy=static_cast<float>(static_cast<double>(to[1])-from[1]);
    if (std::abs(dx)<=std::abs(dy)) return dy>0 ? 4 : 0;
    return dx<=0 ? 6 : 2;
}
}
// 원본 필드 이미지의 직접 조회다.
Sid GunAnimationFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; }
Sid GunAnimationFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; }
Sid GunAnimationFields::GunForm() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kGun,2))}; }
Sid GunAnimationFields::Target() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kTarget,2))}; }
double GunAnimationFields::Time() const { return std::bit_cast<double>(static_cast<std::uint64_t>(Read(bytes,kTime))|(static_cast<std::uint64_t>(Read(bytes,kTime+4))<<32)); }
std::int32_t GunAnimationFields::RestDirection() const { return std::bit_cast<std::int32_t>(Read(bytes,kRest)); }
// 같은 판본의 타입 표만 받아 실제 현재 HP/group을 사용한다.
RawGunAnimation::RawGunAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,GunAnimationState& state,GunAnimationHooks hooks)
    :pool_(pool),types_(types.begin(),types.end()),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("GunAnimation 타입 판본/크기 오류");
}
// 원본 ctor는 좌표·padding을 초기화하지 않으며 setter가 gun/target dirty2/4를 켠다.
void RawGunAnimation::Initialize(GunAnimationFields& fields,GunAnimationKind kind,Sid gunForm,Sid target) const {
    if (!gunForm.value || gunForm.value>=pool_.Capacity()) throw std::out_of_range("GunAnimation gunForm 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto outer=kind==GunAnimationKind::SunRotate ? (patch ? 0x50b260U : 0x505520U) : kind==GunAnimationKind::SunFire ? (patch ? 0x50b318U : 0x505480U) : (patch ? 0x50b2c8U : 0x505660U);
    const auto inner=kind==GunAnimationKind::SunRotate ? (patch ? 0x50b258U : 0x505518U) : kind==GunAnimationKind::SunFire ? (patch ? 0x50b310U : 0x505478U) : (patch ? 0x50b2c4U : 0x505658U);
    Put(fields.bytes,0,outer);Put(fields.bytes,kForm,0);Put(fields.bytes,kParent,0);Put(fields.bytes,kInner,inner);fields.bytes[kDirty]=0xff;
    Put(fields.bytes,kGun,gunForm.value,2);Put(fields.bytes,kTarget,kind==GunAnimationKind::SunRest ? 0U : target.value,2);PutTime(fields,state_.now);
    if (kind!=GunAnimationKind::SunRotate) Put(fields.bytes,kRest,0);
}
// 판본별 현재 frame 폭을 보존하며 표의 물리 범위를 검사한다.
std::int32_t RawGunAnimation::CurrentFrame(Sid parent) const {
    if (!parent.value || parent.value>=pool_.Capacity()) throw std::out_of_range("GunAnimation 부모 범위 오류");
    const auto raw=pool_.Slot(parent);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[10]!=state_.sunCannonType) throw std::logic_error("GunAnimation 미복원 부모 파생 타입");
    return static_cast<std::int32_t>(Read(raw,patch ? 36U : 34U,patch ? 4U : 1U));
}
// 실제 공간 효과 경계가 모두 연결된 경우에만 부모 프레임을 바꾼다.
void RawGunAnimation::SetFrame(Sid parent,std::uint32_t frame) const {
    if (!hooks_.unpop || !hooks_.repop) throw std::logic_error("GunAnimation 프레임 공간 효과 누락");
    hooks_.unpop(parent,0);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;Put(pool_.AllocatedBytes(parent),patch ? 36U : 34U,frame,patch ? 4U : 1U);hooks_.repop(parent,0x40);
}
// 표적이 free/dead이면 기존 추적 좌표를 유지한다.
bool RawGunAnimation::TrackTarget(GunAnimationFields& fields) const {
    const auto target=fields.Target();if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("GunAnimation 표적 범위 오류");
    const auto raw=pool_.Slot(target);if (raw[11]&3) return false;
    if (!hooks_.center) throw std::logic_error("GunAnimation 실제 중심 경계 누락");
    Put(fields.bytes,kPosition,Read(raw,14));Put(fields.bytes,kPosition+4,Read(raw,18));const auto center=hooks_.center(target);
    Put(fields.bytes,kCenter,std::bit_cast<std::uint32_t>(center[0]));Put(fields.bytes,kCenter+4,std::bit_cast<std::uint32_t>(center[1]));return true;
}
// launch는 유효 HP 표적에서만 실제 Missile 생성 효과를 요청한다.
bool RawGunAnimation::Launch(GunAnimationFields& fields) const {
    const auto target=fields.Target();if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("GunAnimation launch 표적 오류");
    const auto raw=pool_.Slot(target);if ((raw[11]&3) || !(types_.at(raw[10]).flags1&0x10)) return false;
    if (!hooks_.targetPoint || !hooks_.createProjectile) throw std::logic_error("GunAnimation 실제 Missile 생성 경계 누락");
    GunProjectileRequest request;request.firing=fields.Parent();request.target=target;request.displayType=state_.displayType;request.effectType=state_.effectType;
    request.targetPosition={Coordinate(fields.bytes,kPosition),Coordinate(fields.bytes,kPosition+4)};request.targetCenter=hooks_.targetPoint(request.firing,target);hooks_.createProjectile(request);return true;
}
// notify는 삭제 전에 현재 GunProcess에 전달되며 Kill 뒤 fields를 다시 읽지 않는다.
void RawGunAnimation::Finish(GunAnimationFields& fields,std::uint32_t info,std::uint32_t value) const {
    if (!hooks_.notify || !hooks_.kill) throw std::logic_error("GunAnimation 완료 효과 누락");
    const auto gun=fields.GunForm(),form=fields.Form();hooks_.notify(gun,1,info,value);hooks_.kill(form,0x10);
}
// 각 프레임은 한 물리 단계만 진행하고 현재 시각에서 다음 간격을 더한다.
void RawGunAnimation::Frame(GunAnimationFields& fields,GunAnimationKind kind) const {
    const auto parent=fields.Parent();const auto current=CurrentFrame(parent);
    if (current<0 || current>=(kind==GunAnimationKind::SunFire ? 48 : 28)) throw std::out_of_range("GunAnimation 썬 캐논 물리 프레임 오류");
    if (kind==GunAnimationKind::SunFire) {
        const bool alive=TrackTarget(fields);if (!(fields.Time()<=state_.now)) return;
        const auto code=current<24 ? static_cast<std::uint32_t>(current+24)|0x4000U : static_cast<std::uint32_t>(current-24)|0x8000U;
        SetFrame(parent,code&0x3fffU);if ((code&0x4000U) && !Launch(fields) && alive) return;
        if (code&0x8000U) { Finish(fields,6,7);return; }
        Time(fields,state_.now+static_cast<double>(kFireDelay));return;
    }
    if (!(fields.Time()<=state_.now)) return;
    std::int32_t direction=fields.RestDirection()/2;
    if (kind==GunAnimationKind::SunRotate) {
        if (!fields.Target().value || !hooks_.center) throw std::logic_error("GunAnimation 회전 표적/중심 경계 누락");
        const auto to=hooks_.center(fields.Target()),from=hooks_.center(parent);direction=Direction(from,to)/2;
    }
    if (direction<0 || direction>4) throw std::out_of_range("GunAnimation 원위치 방향 오류");
    const auto code=kSunRotate[static_cast<std::size_t>(direction)][static_cast<std::size_t>(current)];
    if (code&0x8000U) {
        if (kind==GunAnimationKind::SunRotate) { Finish(fields,3,4);return; }
        if (fields.RestDirection()==8) { Finish(fields,9,10);return; }
        Put(fields.bytes,kRest,static_cast<std::uint32_t>(fields.RestDirection()+2));
    } else {
        if (kind==GunAnimationKind::SunRotate) {
            if (!hooks_.randomInteger || !hooks_.sound) throw std::logic_error("GunAnimation 회전 소리 경계 누락");
            const auto number=hooks_.randomInteger(1000,10000)/1000;hooks_.sound(parent,"sunCannonRaiseLower"+std::to_string(number)+".wav");
        }
        SetFrame(parent,code&0x3fffU);
    }
    Time(fields,state_.now+static_cast<double>(kRotateDelay));
}
// 원본 dirty 접두 packet은 회전0x3c/발사0x3b 메시지를 가상 표로 구별한다.
std::uint16_t RawGunAnimation::WritePacket(GunAnimationFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force,std::uint8_t protocol) {
    const auto dirty=force ? 0xffU : fields.bytes[kDirty];const auto length=9U+((includeParent || force) ? 4U : 0U)+((dirty&2) ? 2U : 0U)+((dirty&4) ? 2U : 0U)+((dirty&8) ? 8U : 0U);
    if (packet.size()<length) throw std::out_of_range("GunAnimation packet 크기 오류");
    if (force) fields.bytes[kDirty]=0xff;const auto outer=Read(fields.bytes,0);packet[0]=protocol;packet[1]=static_cast<std::uint8_t>((outer==0x50b318U || outer==0x505480U) ? 0x3b : 0x3c);Put(packet,4,Read(fields.bytes,kForm));std::size_t cursor=9;
    if (includeParent || force) { fields.bytes[kDirty]|=1;Put(packet,cursor,Read(fields.bytes,kParent));cursor+=4; }
    // dirty가 선택한 WORD/WORD/double을 현재 raw 폭 그대로 복사한다.
    for (const auto [bit,offset,width]:{std::array<std::size_t,3>{2,kGun,2},{4,kTarget,2},{8,kTime,8}})
        if (fields.bytes[kDirty]&bit) { std::copy_n(fields.bytes.begin()+static_cast<std::ptrdiff_t>(offset),width,packet.begin()+static_cast<std::ptrdiff_t>(cursor));cursor+=width; }
    Put(packet,2,static_cast<std::uint32_t>(cursor),2);packet[8]=fields.bytes[kDirty];fields.bytes[kDirty]=0;return static_cast<std::uint16_t>(cursor);
}
// 어댑터가 같은 풀/부모 타입을 검사하도록 실제 자료를 제공한다.
const SidPool& RawGunAnimation::Pool() const { return pool_; }
std::uint32_t RawGunAnimation::SunCannonType() const { return state_.sunCannonType; }
// 원본 파생 선택과 raw 필드를 실제 Kernel 객체가 소유한다.
GunAnimation::GunAnimation(const RawGunAnimation& logic,GunAnimationFields fields,GunAnimationKind kind):logic_(logic),fields_(std::move(fields)),kind_(kind) {}
void GunAnimation::RunFrame() { logic_.Frame(fields_,kind_); }
void GunAnimation::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value); }
std::uint16_t GunAnimation::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force) {
    Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value);return RawGunAnimation::WritePacket(fields_,packet,parentChanged,force);
}
GunAnimationFields& GunAnimation::Fields() { return fields_; }
const GunAnimationFields& GunAnimation::Fields() const { return fields_; }
// 실제 GunProcess 가상 통지와 AnimationProcess flags0x10 Kill을 연결한다.
GunAnimationHooks MakeGunAnimationHostHooks(SquidProcessHost& host,GunAnimationHooks hooks) {
    hooks.notify=[&host](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(form));if (!gun) throw std::logic_error("GunAnimation 완료 GunProcess 조회 오류");gun->Notify(event,info,value); };
    hooks.kill=[&host](Sid form,std::uint32_t flags) { auto* animation=dynamic_cast<GunAnimation*>(host.FromForm(form));if (!animation) throw std::logic_error("GunAnimation 완료 process 조회 오류");host.Kill(*animation,flags); };
    return hooks;
}
// gunForm의 현재 GunProcess와 부모를 검증하고 원본 flags0x10·타입35/36으로 생성한다.
GunAnimation* CreateGunAnimation(SquidProcessHost& host,const RawGunAnimation& logic,std::uint32_t stage,Sid gunForm,Sid target) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("GunAnimation/Host SID 풀 불일치");
    auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun) throw std::logic_error("GunAnimation factory GunProcess 오류");
    if (host.Pool().Slot(gun->Parent())[10]!=logic.SunCannonType()) throw std::logic_error("GunAnimation 미복원 파생 factory");
    const auto kind=stage==2 ? GunAnimationKind::SunRotate : stage==5 ? GunAnimationKind::SunFire : GunAnimationKind::SunRest;
    if (stage!=2 && stage!=5 && stage!=8) throw std::out_of_range("GunAnimation factory 단계 오류");
    GunAnimationFields fields;logic.Initialize(fields,kind,gunForm,target);auto process=std::make_unique<GunAnimation>(logic,std::move(fields),kind);
    return dynamic_cast<GunAnimation*>(host.Attach(std::move(process),stage==2 || stage==8 ? 36U : 35U,gun->Parent(),0x10));
}
// GunProcess의 미복원 생성 경계를 같은 월드의 실제 animation factory로 교체한다.
GunProcessHooks MakeGunAnimationProcessHooks(SquidProcessHost& host,const RawGunAnimation& logic,GunProcessHooks hooks) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("GunAnimation/Host SID 풀 불일치");
    hooks.startAnimation=[&host,&logic](std::uint32_t stage,Sid gunForm,Sid target) { if (!CreateGunAnimation(host,logic,stage,gunForm,target)) throw std::logic_error("GunAnimation 부모 삭제 중 factory 실패"); };
    return hooks;
}
}
