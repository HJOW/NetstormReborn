// 원본 공통 Fire와 각 파생 단계의 snapshot/재조회 순서를 유지한다.
#include "o/RawCannonFire.h"
#include "o/SpStore.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 미정렬 DWORD/WORD를 현재 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(bytes[offset+i])<<(8*i); // 현재 필드만 조합한다.
    return result;
}
// padding을 보존하면서 실제 원본 필드 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 낮은 바이트부터 저장한다.
}
// 실제 double 시각을 원본 비트로 기록한다.
void Time(GunAnimationFields& fields,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,48,static_cast<std::uint32_t>(bits));Put(fields.bytes,52,static_cast<std::uint32_t>(bits>>32));
}
// 추적 위치의 float32 두 값을 읽는다.
std::array<float,2> Point(const GunAnimationFields& fields,std::size_t offset) { return {std::bit_cast<float>(Read(fields.bytes,offset)),std::bit_cast<float>(Read(fields.bytes,offset+4))}; }
}
// 먼저10.78의 실제 파생만 지원하며 CD 가상 표를 추정하지 않는다.
RawCannonFire::RawCannonFire(SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,CannonFireState& state,GunAnimationHooks hooks,CannonBeamHooks beam)
    :pool_(pool),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)),beam_(std::move(beam)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || state.type>=types.size()) throw std::invalid_argument("캐논 발사 실제 판본/타입 자료 오류");
}
// 썬00468460과 썬더004684d0의 공통 접두·파생 가상 표를 보존한다.
void RawCannonFire::Initialize(GunAnimationFields& fields,Sid gunForm,Sid target) const {
    if (!gunForm.value || gunForm.value>=pool_.Capacity()) throw std::out_of_range("캐논 발사 GunForm 오류");const bool sun=state_.kind==CannonFireKind::Sun;
    const bool vander=state_.kind==CannonFireKind::Vander; // 실제 반더 outer/inner 가상 표를 두 캐논과 구별한다.
    Put(fields.bytes,0,vander ? 0x50b170U : sun ? 0x50b318U : 0x50b370U);Put(fields.bytes,4,0);Put(fields.bytes,8,0);Put(fields.bytes,32,vander ? 0x50b16cU : sun ? 0x50b310U : 0x50b368U);fields.bytes[40]=0xff;Put(fields.bytes,42,gunForm.value,2);Put(fields.bytes,44,target.value,2);Time(fields,state_.now);Put(fields.bytes,56,0);
}
// 범위 밖/미복원 TYPE는 현재 원본 슬롯을 수정하기 전에 거부한다.
Sid RawCannonFire::Parent(const GunAnimationFields& fields) const {
    const auto parent=fields.Parent();if (!parent.value || parent.value>=pool_.Capacity() || pool_.Slot(parent)[10]!=state_.type) throw std::logic_error("캐논 발사 미복원 부모 TYPE/번호");return parent;
}
// 가상 단계에서 부모가 바뀔 수 있으므로 같은 getter를 다시 조회한다.
void RawCannonFire::SetFrame(GunAnimationFields& fields,std::uint32_t frame) const {
    if (!hooks_.unpop || !hooks_.repop) throw std::logic_error("캐논 발사 실제 공간 경계 누락");const auto parent=Parent(fields);hooks_.unpop(parent,0);Put(pool_.AllocatedBytes(parent),36,frame);hooks_.repop(parent,0x40);
}
// GunForm은 통지 전에, 삭제할 현재 Form은 통지 뒤에 조회하는 원문 순서다.
void RawCannonFire::Finish(GunAnimationFields& fields) const {
    if (!hooks_.notify || !hooks_.kill) throw std::logic_error("캐논 발사 실제 완료 경계 누락");const auto gun=fields.GunForm();hooks_.notify(gun,1,6,7);hooks_.kill(fields.Form(),0x10);
}
// 썬은24프레임 앞뒤 변환이며 썬더는 현재 물리 프레임의 문자 구간을 순서대로 진행한다.
std::uint32_t RawCannonFire::Step(GunAnimationFields& fields,std::int32_t frame) const {
    if (state_.kind==CannonFireKind::Vander) return static_cast<std::uint32_t>(frame)|0xc000U; // 실제00468130은 발사/완료 비트를 동시에 세운다.
    if (state_.kind==CannonFireKind::Sun) return frame<24 ? static_cast<std::uint32_t>(frame+24)|0x4000U : static_cast<std::uint32_t>(frame-24)|0x8000U;
    const auto parent=Parent(fields);const auto raw=pool_.Slot(parent);const auto current=Read(raw,36);const auto& table=frames_[raw[10]].frames;const auto codes=table.Codes();if (current>=codes.size()) throw std::out_of_range("썬더 캐논 현재 프레임 코드 범위 오류");const auto run=table.Run(codes[current].side);const auto last=run.first+run.count-1,next=frame+1;
    if (next>last) return static_cast<std::uint32_t>(run.first)|0x8000U;
    if (next==last) return static_cast<std::uint32_t>(next)|0x4000U;
    if (next==last-state_.growlOffset) { if (!hooks_.sound) throw std::logic_error("썬더 캐논 Growl 음향 경계 누락");hooks_.sound(Parent(fields),"thunderCannonGrowl.wav"); }
    if (next==last-state_.roarOffset) { if (!hooks_.sound) throw std::logic_error("썬더 캐논 Roar 음향 경계 누락");hooks_.sound(Parent(fields),"thunderCannonRoar.wav"); }
    return static_cast<std::uint32_t>(next);
}
// 공통 Launch의 초기 raw 주소 snapshot과 일반 Missile/즉시 빔의 각 분기를 보존한다.
bool RawCannonFire::Launch(GunAnimationFields& fields) const {
    const auto source=Parent(fields),target=fields.Target();const auto initialSource=pool_.Slot(source);
    if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("캐논 발사 표적 번호 오류");const auto raw=pool_.Slot(target);if (raw[11]&3) return false;if (raw[10]>=types_.size()) throw std::out_of_range("캐논 발사 표적 TYPE 오류");if (!(types_[raw[10]].flags1&0x10)) return false;
    if (!hooks_.targetPoint) throw std::logic_error("캐논 발사 실제 표적 코너 경계 누락");
    if (initialSource[10]==state_.vanderType) {
        if (!beam_.create || !beam_.refresh || !beam_.hit || !beam_.random) throw std::logic_error("반더 실제 빔/피해/난수 경계 누락");
    } else if (!hooks_.createProjectile) throw std::logic_error("캐논 발사 실제 Missile 경계 누락");
    const auto corner=hooks_.targetPoint(fields.Parent(),fields.Target());
    if (initialSource[10]==state_.vanderType) {
        if (!beam_.create || !beam_.refresh || !beam_.hit || !beam_.random) throw std::logic_error("현재 반더 실제 빔 경계 누락");
        beam_.create(fields.Parent(),fields.Target());bool avoided=(raw[11]&3)!=0;
        if (!avoided && raw[10]==state_.floaterType) avoided=static_cast<double>(beam_.random->Next(65536))*(1.0/65536.0)*1000<state_.avoidanceCutoff;
        if (avoided) beam_.refresh(source);else beam_.hit(source,fields.Target());return (raw[11]&3)==0;
    }
    if (!hooks_.createProjectile) throw std::logic_error("현재 캐논 실제 Missile 경계 누락");GunProjectileRequest request;request.firing=fields.Parent();request.target=fields.Target();request.speed=state_.speed;request.displayType=state_.displayType;request.effectType=state_.effectType;request.targetPosition=Point(fields,12);request.targetCenter=corner;request.firingPosition={-10000,-10000};hooks_.createProjectile(request);return true;
}
// 00467330은 처음 추적한 표적의 생존 상태를 효과 이후 현재 표적과 구별한다.
void RawCannonFire::Frame(GunAnimationFields& fields) const {
    FrameWithStep(fields,[this,&fields](auto current) { return Step(fields,current); });
}
// 실제 공통 부모가 선택한 가상 Step 뒤에도 현재 부모/표적/시각을 같은 위치에서 재조회한다.
void RawCannonFire::FrameWithStep(GunAnimationFields& fields,const std::function<std::uint32_t(std::int32_t)>& step) const {
    const auto target=fields.Target();if (!target.value || target.value>=pool_.Capacity()) throw std::logic_error("캐논 발사 표적0 원본 assert");const auto raw=pool_.Slot(target);const bool dead=(raw[11]&3)!=0;
    if (!dead) { if (!hooks_.center) throw std::logic_error("캐논 발사 실제 표적 중심 경계 누락");Put(fields.bytes,12,Read(raw,14));Put(fields.bytes,16,Read(raw,18));const auto center=hooks_.center(target);Put(fields.bytes,20,std::bit_cast<std::uint32_t>(center[0]));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(center[1])); }
    const auto source=pool_.Slot(Parent(fields));if (!(fields.Time()<=state_.now)) return;const auto current=static_cast<std::int32_t>(Read(source,36));if (!step) throw std::logic_error("캐논 발사의 실제 가상 Step 미연결");const auto code=step(current);SetFrame(fields,code&0x3fffU);
    if ((code&0x4000U) && !Launch(fields) && !dead) return;
    if (code&0x8000U) { Finish(fields);return; }
    fields.bytes[40]|=8;Time(fields,state_.now);const auto delay=static_cast<double>(state_.delay)*((code&0x4000U) && source[10]==state_.scaledType ? static_cast<double>(state_.launchScale) : 1.0);Time(fields,state_.now+delay);
}
// 동일 실제 풀과 복원 TYPE를 factory 계약에 제공한다.
const SidPool& RawCannonFire::Pool() const { return pool_; }
std::uint32_t RawCannonFire::Type() const { return state_.type; }
// 실제 원본 이미지와 장기 처리기만 보존한다.
CannonFire::CannonFire(const RawCannonFire& logic,GunAnimationFields fields):logic_(logic),fields_(std::move(fields)) {}
// 같은 Kernel의 실제 공통 발사 프레임을 실행한다.
void CannonFire::RunFrame() { logic_.Frame(fields_); }
// Host가 확정한 Form/Parent를 원본 DWORD에 쓴다.
void CannonFire::OnAttached() { Put(fields_.bytes,4,Form().value);Put(fields_.bytes,8,Parent().value); }
// 후속 관찰/전송은 실제 소유 필드만 조회한다.
GunAnimationFields& CannonFire::Fields() { return fields_; }
// 실제 GunProcess 완료와 파생 CannonFire 클래스의 kill을 연결한다.
GunAnimationHooks MakeCannonFireHostHooks(SquidProcessHost& host,GunAnimationHooks hooks) {
    hooks.notify=[&host](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(form));if (!gun) throw std::logic_error("캐논 발사 완료 GunProcess 조회 실패");gun->Notify(event,info,value); };
    hooks.kill=[&host](Sid form,std::uint32_t flags) { auto* animation=dynamic_cast<CannonFire*>(host.FromForm(form));if (!animation) throw std::logic_error("캐논 발사 완료 animation 조회 실패");host.Kill(*animation,flags); };return hooks;
}
// 실제 부모에35/flags0x10을 부착하며 새 풀이나 대체 GunProcess를 만들지 않는다.
CannonFire* CreateCannonFire(SquidProcessHost& host,const RawCannonFire& logic,Sid gunForm,Sid target) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("캐논 발사 Host/Pool 불일치");auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun || host.Pool().Slot(gun->Parent())[10]!=logic.Type()) throw std::logic_error("캐논 발사 factory 부모 오류");GunAnimationFields fields;logic.Initialize(fields,gunForm,target);return dynamic_cast<CannonFire*>(host.Attach(std::make_unique<CannonFire>(logic,std::move(fields)),35,gun->Parent(),0x10));
}
}
