// 크로스보우 발사의 원본 필드 쓰기·실패 재시도·현재 시계 조회 순서를 보존한다.
#include "o/RawCrossbowAnimation.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 비정렬 원본 DWORD/WORD를 지정한 폭으로 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(bytes[offset+i])<<(8*i); // 현재 필드 폭 밖을 읽지 않는다.
    return result;
}
// 원본 저장 폭 밖의 padding을 변경하지 않는다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 낮은 바이트부터 기록한다.
}
// 실제 double 시각을 정렬에 의존하지 않고 저장한다.
void Time(GunAnimationFields& fields,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,48,static_cast<std::uint32_t>(bits));Put(fields.bytes,52,static_cast<std::uint32_t>(bits>>32));
}
// 현재 추적 좌표의 float raw 값을 반환한다.
std::array<float,2> Point(const GunAnimationFields& fields,std::size_t offset) {
    return {std::bit_cast<float>(Read(fields.bytes,offset)),std::bit_cast<float>(Read(fields.bytes,offset+4))};
}
}
// 크로스보우는 우선 실제10.78에 한정하며 다른 판본의 주소를 추정하지 않는다.
RawCrossbowAnimation::RawCrossbowAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const ArcherTypeData> animationTypes,CrossbowAnimationState& state,GunAnimationHooks hooks)
    :pool_(pool),types_(types),animationTypes_(animationTypes),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || animationTypes.size()!=types.size() || state.type>=types.size()) throw std::invalid_argument("크로스보우 실제 판본/TYPE 자료 오류");
}
// 00468050은64BYTE 중 지정한 필드만 초기화하며 공통 Host 부착이 Form/Parent를 채운다.
void RawCrossbowAnimation::Initialize(GunAnimationFields& fields,Sid gunForm,Sid target) const {
    if (!gunForm.value || gunForm.value>=pool_.Capacity()) throw std::out_of_range("크로스보우 GunForm 오류");
    Put(fields.bytes,0,0x50b120);Put(fields.bytes,4,0);Put(fields.bytes,8,0);Put(fields.bytes,32,0x50b11c);fields.bytes[40]=0xff;Put(fields.bytes,42,gunForm.value,2);Put(fields.bytes,44,target.value,2);Time(fields,state_.now);Put(fields.bytes,56,0);
}
// 실제 부모 번호/TYPE가 이 몸체의 현재 입력인지 확인한다.
Sid RawCrossbowAnimation::Parent(const GunAnimationFields& fields) const {
    const auto parent=fields.Parent();if (!parent.value || parent.value>=pool_.Capacity() || pool_.Slot(parent)[10]!=state_.type) throw std::logic_error("크로스보우 미복원 부모 TYPE/번호");return parent;
}
// 실제 함수는 최초 부모 snapshot과 별개로 프레임 변경 시 부모를 다시 조회한다.
void RawCrossbowAnimation::SetFrame(GunAnimationFields& fields,std::int32_t frame) const {
    if (!hooks_.unpop || !hooks_.repop) throw std::logic_error("크로스보우 실제 프레임 공간 경계 누락");
    const auto parent=Parent(fields);hooks_.unpop(parent,0);Put(pool_.AllocatedBytes(parent),36,static_cast<std::uint32_t>(frame));hooks_.repop(parent,0x40);
}
// 완료 통지가 현재 animation을 없앨 수 있으므로 필요한 번호를 미리 보존한다.
void RawCrossbowAnimation::Finish(GunAnimationFields& fields) const {
    if (!hooks_.notify || !hooks_.kill) throw std::logic_error("크로스보우 실제 완료 경계 누락");const auto gun=fields.GunForm(),form=fields.Form();hooks_.notify(gun,1,6,7);hooks_.kill(form,0x10);
}
// 00466f50의 일반 투사체 분기를 실행하며 실제 ctor가 없으면 성공으로 간주하지 않는다.
bool RawCrossbowAnimation::Launch(GunAnimationFields& fields) const {
    Parent(fields);const auto target=fields.Target();if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("크로스보우 발사 표적 오류");const auto raw=pool_.Slot(target);
    if (raw[11]&3) return false;if (raw[10]>=types_.size()) throw std::out_of_range("크로스보우 표적 TYPE 오류");if (!(types_[raw[10]].flags1&0x10)) return false;
    if (!hooks_.targetPoint || !hooks_.createProjectile) throw std::logic_error("크로스보우 실제 Missile 생성 경계 누락");
    const auto corner=hooks_.targetPoint(fields.Parent(),fields.Target());GunProjectileRequest request;request.firing=fields.Parent();request.target=fields.Target();request.speed=state_.speed;request.displayType=state_.displayType;request.effectType=0;request.targetPosition=Point(fields,12);request.targetCenter=corner;request.firingPosition={-10000,-10000};hooks_.createProjectile(request);return true;
}
// 004674c0은 due 전 추적하지 않으며 발사 실패 당시 살아 있던 표적이면 완료를 보류한다.
void RawCrossbowAnimation::Frame(GunAnimationFields& fields) const {
    if (!(fields.Time()<=state_.now)) return;
    const auto parent=Parent(fields);const auto source=pool_.Slot(parent);const auto target=fields.Target();if (!target.value || target.value>=pool_.Capacity()) throw std::out_of_range("크로스보우 현재 표적 오류");const auto raw=pool_.Slot(target);const bool dead=(raw[11]&3)!=0;
    if (!dead) {
        if (!hooks_.center) throw std::logic_error("크로스보우 실제 표적 중심 경계 누락");Put(fields.bytes,12,Read(raw,14));Put(fields.bytes,16,Read(raw,18));const auto center=hooks_.center(target);Put(fields.bytes,20,std::bit_cast<std::uint32_t>(center[0]));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(center[1]));
    }
    const auto& data=animationTypes_[source[10]];if (data.directions<=0 || data.frameCount/data.directions<=0) throw std::logic_error("크로스보우 실제 방향/프레임 자료 미공급");
    const auto per=data.frameCount/data.directions,frame=static_cast<std::int32_t>(Read(source,36));
    if (frame%per==0) { SetFrame(fields,frame+per-1);if (!Launch(fields) && !dead) return;Finish(fields);return; }
    SetFrame(fields,frame-1);fields.bytes[40]|=8;Time(fields,state_.now);Time(fields,state_.now+static_cast<double>(state_.delay));
}
// 현재 adapter는 같은 풀과 TYPE 계약을 그대로 사용한다.
const SidPool& RawCrossbowAnimation::Pool() const { return pool_; }
std::uint32_t RawCrossbowAnimation::Type() const { return state_.type; }
// 원본 필드와 처리기 참조만 실제 프로세스에 저장한다.
CrossbowAnimation::CrossbowAnimation(const RawCrossbowAnimation& logic,GunAnimationFields fields):logic_(logic),fields_(std::move(fields)) {}
// Kernel의 실제 발사 프레임을 한 번 처리한다.
void CrossbowAnimation::RunFrame() { logic_.Frame(fields_); }
// Host가 확정한 Form/Parent를 원본 DWORD에 쓴다.
void CrossbowAnimation::OnAttached() { Put(fields_.bytes,4,Form().value);Put(fields_.bytes,8,Parent().value); }
// 후속 관찰과 전송은 이 원본 이미지에 접근한다.
GunAnimationFields& CrossbowAnimation::Fields() { return fields_; }
// 완료 GunProcess와 실제 크로스보우 발사 클래스의 수명을 같은 Host로 처리한다.
GunAnimationHooks MakeCrossbowAnimationHostHooks(SquidProcessHost& host,GunAnimationHooks hooks) {
    hooks.notify=[&host](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(form));if (!gun) throw std::logic_error("크로스보우 완료 GunProcess 조회 실패");gun->Notify(event,info,value); };
    hooks.kill=[&host](Sid form,std::uint32_t flags) { auto* animation=dynamic_cast<CrossbowAnimation*>(host.FromForm(form));if (!animation) throw std::logic_error("크로스보우 완료 animation 조회 실패");host.Kill(*animation,flags); };return hooks;
}
// 실제 크로스보우 GunProcess 부모에 TYPE35/flags0x10으로 원본 발사를 부착한다.
CrossbowAnimation* CreateCrossbowAnimation(SquidProcessHost& host,const RawCrossbowAnimation& logic,Sid gunForm,Sid target) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("크로스보우 Host/Pool 불일치");auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun || host.Pool().Slot(gun->Parent())[10]!=logic.Type()) throw std::logic_error("크로스보우 factory 부모 오류");GunAnimationFields fields;logic.Initialize(fields,gunForm,target);return dynamic_cast<CrossbowAnimation*>(host.Attach(std::make_unique<CrossbowAnimation>(logic,std::move(fields)),35,gun->Parent(),0x10));
}
}
