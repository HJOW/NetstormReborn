// SunArcher의current-field 재조회와float각상태저장을원본호출순서로보존한다.
#include "o/RawArcherAnimation.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본20칸비균등회전각표와실제라디안→도 계수다.
constexpr std::array<float,20> kAngles{0,21,35,52,71,86,97,115,121,148,180,205,237,256,272,282,291,314,328,345};
constexpr double kDegreesPerRadian=57.295827908797776;
// 회전의가상float 대기간격과각도끝의원본float clamp다.
constexpr float kRotateDelay=0.1f,kLastAngle=359.9999f;
// 원본비정렬little endian 필드를읽는다.
std::uint32_t ReadArcher(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정한현재필드폭만조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본지정필드밖padding을보존하며쓴다.
void PutArcher(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 지정한하위바이트만현재저장폭으로쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 실제float32 각상태/좌표를읽고저장한다.
float AFloat(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(ReadArcher(raw,offset)); }
void AFloat(std::span<std::uint8_t> raw,std::size_t offset,float value) { PutArcher(raw,offset,std::bit_cast<std::uint32_t>(value)); }
// 원본현재예약시각쓰기만dirty8을켠다.
void ArcherTime(ArcherAnimationFields& fields,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);fields.bytes[40]|=8;PutArcher(fields.bytes,48,static_cast<std::uint32_t>(bits));PutArcher(fields.bytes,52,static_cast<std::uint32_t>(bits>>32)); }
// 현재좌표쌍을원본float폭으로읽는다.
std::array<float,2> APoint(std::span<const std::uint8_t> raw,std::size_t offset) { return {AFloat(raw,offset),AFloat(raw,offset+4)}; }
// 원본BoardCoord의float 인자를fmod→음수보정→끝각clamp한다.
float NormalizeArcher(float angle) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Archer 비유한각도");double value=std::fmod(static_cast<double>(angle),360.0);if (value<0) value+=360.0;return value>=360.0 ? kLastAngle : static_cast<float>(value);
}
}
// 원본공통SID/시각필드조회다.
Sid ArcherAnimationFields::Form() const { return Sid{static_cast<std::uint16_t>(ReadArcher(bytes,4))}; }
Sid ArcherAnimationFields::Parent() const { return Sid{static_cast<std::uint16_t>(ReadArcher(bytes,8))}; }
Sid ArcherAnimationFields::GunForm() const { return Sid{static_cast<std::uint16_t>(ReadArcher(bytes,42,2))}; }
Sid ArcherAnimationFields::Target() const { return Sid{static_cast<std::uint16_t>(ReadArcher(bytes,44,2))}; }
double ArcherAnimationFields::Time() const { return std::bit_cast<double>(static_cast<std::uint64_t>(ReadArcher(bytes,48))|(static_cast<std::uint64_t>(ReadArcher(bytes,52))<<32)); }
// 동일판본의동적방향수/물리프레임과실제경계수명을검사한다.
RawArcherAnimation::RawArcherAnimation(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const ArcherTypeData> animationTypes,
    ArcherAnimationState& state,ArcherAnimationHooks hooks):pool_(pool),types_(types),animationTypes_(animationTypes),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || animationTypes.size()!=types.size() || state.type>=types.size()) throw std::invalid_argument("Archer Animation 타입/방향자료 오류");
}
// 원본기본초기화뒤Host부착이현재form/parent를채운다.
void RawArcherAnimation::Initialize(ArcherAnimationFields& fields,bool fire,Sid gunForm,Sid target) const {
    if (!gunForm.value || gunForm.value>=pool_.Capacity()) throw std::out_of_range("Archer gunForm SID 오류");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    PutArcher(fields.bytes,0,fire ? (patch ? 0x50b1c0U : 0x5054d0U) : (patch ? 0x50b450U : 0x505570U));PutArcher(fields.bytes,4,0);PutArcher(fields.bytes,8,0);PutArcher(fields.bytes,32,fire ? (patch ? 0x50b1bcU : 0x5054ccU) : (patch ? 0x50b448U : 0x50556cU));fields.bytes[40]=0xff;PutArcher(fields.bytes,42,gunForm.value,2);PutArcher(fields.bytes,44,target.value,2);ArcherTime(fields,state_.now);if (fire) PutArcher(fields.bytes,56,0);
}
// 실제부모현재타입을매조회에서검사한다.
std::span<const std::uint8_t> RawArcherAnimation::Parent(const ArcherAnimationFields& fields) const {
    const auto sid=fields.Parent();if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Archer 부모 SID 오류");const auto raw=pool_.Slot(sid);if (raw[10]!=state_.type) throw std::logic_error("Archer 미복원 부모 타입");return raw;
}
// 현재물리프레임의원본판본폭이다.
std::int32_t RawArcherAnimation::Frame(Sid parent) const { const bool patch=pool_.Edition()==OriginalEdition::Patch1078;return static_cast<std::int32_t>(ReadArcher(pool_.Slot(parent),patch ? 36U : 34U,patch ? 4U : 1U)); }
// 기존원본프레임공간순서를사용한다.
void RawArcherAnimation::SetFrame(Sid parent,std::int32_t frame) const {
    if (!hooks_.common.unpop || !hooks_.common.repop) throw std::logic_error("Archer 실제프레임공간경계 누락");hooks_.common.unpop(parent,0);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;PutArcher(pool_.AllocatedBytes(parent),patch ? 36U : 34U,static_cast<std::uint32_t>(frame),patch ? 4U : 1U);hooks_.common.repop(parent,0x40);
}
// source가free/dead면기존추적좌표를보존한다.
bool RawArcherAnimation::Track(ArcherAnimationFields& fields) const {
    const auto sid=fields.Target();if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Archer 표적 SID 오류");const auto raw=pool_.Slot(sid);if (raw[11]&3) return false;if (!hooks_.common.center) throw std::logic_error("Archer 실제표적중심경계 누락");const auto center=hooks_.common.center(sid);PutArcher(fields.bytes,12,ReadArcher(raw,14));PutArcher(fields.bytes,16,ReadArcher(raw,18));AFloat(fields.bytes,20,center[0]);AFloat(fields.bytes,24,center[1]);return true;
}
// 원본회전helper는가장가까운각도를고르며동률에서는앞선칸을보존한다.
std::int32_t RawArcherAnimation::NearestDirection(float angle,OriginalEdition edition) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Archer 회전비유한각도");double best=-1000.0;std::int32_t result=0;
    // 오름차순표의현재최선차이가양수가되면원본처럼다음칸조회없이끝낸다.
    for (std::size_t i=0;i<kAngles.size() && best<0;++i) {
        const double difference=static_cast<double>(kAngles[i])-angle;if (std::abs(difference)<std::abs(best)) { result=static_cast<std::int32_t>(i);best=edition==OriginalEdition::Patch1078 ? difference : static_cast<float>(difference); }
    }
    if (result==19 && std::abs(360.0-angle)<std::abs(best)) return 0;return result;
}
// 실제Firector가현재0..32 프레임을22.5도각으로변환한다.
float RawArcherAnimation::InitialAngle(std::int32_t frame) { if (frame>15) frame-=16;const double angle=frame*-22.5+360.0;if (!(angle>=0 && angle<=360)) throw std::out_of_range("Archer 초기각 원본 assert");return static_cast<float>(angle); }
// 원본firehelper의fmod결과와16방향 sprung offset2/frame16을유지한다.
std::int32_t RawArcherAnimation::FireFrame(float angle,bool sprung) {
    if (!std::isfinite(angle)) throw std::invalid_argument("Archer 발사비유한각도");double inverse=360.0-std::fmod(static_cast<double>(angle),360.0);if (!(inverse>0 && inverse<=360)) throw std::out_of_range("Archer 발사각 원본 assert");if (inverse>=360) inverse=359;const float value=static_cast<float>(inverse*0.044444444444444446);const auto frame=static_cast<std::int32_t>(value);return (frame+(sprung ? 2 : 0))%16+(sprung ? 16 : 0);
}
// 좌표동률은0이며원본의근사pi 계수로현재가리키는각도를계산한다.
double RawArcherAnimation::Degrees(std::array<float,2> from,std::array<float,2> to) { if (from==to) return 0;return std::atan2(-(static_cast<double>(to[1])-from[1]),static_cast<double>(to[0])-from[0])*kDegreesPerRadian; }
// 원본BoardCoord.Angle은450-atanDegrees를float로저장후정규화한다.
float RawArcherAnimation::DirectionAngle(std::array<float,2> from,std::array<float,2> to,OriginalEdition) { if (from==to) return 0;return NormalizeArcher(static_cast<float>(450.0-Degrees(from,to))); }
// Fire부착후출력해제도현재원본WORD bit0만바꾼다.
void RawArcherAnimation::InitializeFire(ArcherAnimationFields& fields) const {
    Parent(fields);if (!hooks_.notifySurface) throw std::logic_error("Archer 초기출력공간통지 누락");AFloat(fields.bytes,64,InitialAngle(Frame(fields.Parent())));AFloat(fields.bytes,68,state_.initialSpeed);AFloat(fields.bytes,72,state_.initialAcceleration);PutArcher(fields.bytes,76,0);PutArcher(fields.bytes,80,0);pool_.AllocatedBytes(fields.Parent())[12]&=0xfe;hooks_.notifySurface(fields.Parent());
}
// 실제완료통지뒤Kill이이객체를없앨수있으므로추가필드를읽지않는다.
void RawArcherAnimation::Finish(ArcherAnimationFields& fields,std::uint32_t info,std::uint32_t value) const { if (!hooks_.common.notify || !hooks_.common.kill) throw std::logic_error("Archer 실제완료경계 누락");const auto gun=fields.GunForm(),form=fields.Form();hooks_.common.notify(gun,1,info,value);hooks_.common.kill(form,0x10); }
// 표적재조회는due검사앞에있으며한틱에방향한칸만진행한다.
void RawArcherAnimation::Rotate(ArcherAnimationFields& fields) const {
    const auto raw=Parent(fields);const auto& data=animationTypes_[raw[10]];if (data.directions<=0 || data.frameCount/data.directions<=0) throw std::logic_error("Archer 실제방향/물리프레임자료 미공급");const auto per=data.frameCount/data.directions,current=Frame(fields.Parent())/per;float angle;
    if (!fields.Target().value) { if (!hooks_.aimDirection) throw std::logic_error("Archer 실제표적0방향조회 누락");angle=hooks_.aimDirection(fields.Parent())*45.0f; }
    else { Track(fields);if (!hooks_.common.center) throw std::logic_error("Archer 실제현재중심경계 누락");angle=DirectionAngle(hooks_.common.center(fields.Parent()),APoint(fields.bytes,20),pool_.Edition()); }
    const auto wanted=NearestDirection(angle,pool_.Edition());if (!(fields.Time()<=state_.now)) return;if (current==wanted) { Finish(fields,3,4);return; }
    const auto backward=current<wanted ? current-wanted+data.directions : current-wanted,forward=wanted<current ? wanted-current+data.directions : wanted-current;const auto step=forward<backward ? 1 : -1;const auto next=current==0 && step==-1 ? data.directions-1 : (current+step)%data.directions;SetFrame(fields.Parent(),next*per);ArcherTime(fields,state_.now+static_cast<double>(kRotateDelay));
    if (!hooks_.common.randomInteger || !hooks_.common.sound) throw std::logic_error("Archer 실제회전소리경계 누락");const auto number=hooks_.common.randomInteger(1000,10000)/1000;hooks_.common.sound(fields.Parent(),"sunDiscThrowerRatchet0"+std::to_string(number)+".wav");
}
// HP가없는표적이나free/dead 표적은원본처럼생성없이false를반환한다.
bool RawArcherAnimation::Launch(ArcherAnimationFields& fields) const {
    Parent(fields);const auto sid=fields.Target();if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Archer launch 표적 SID 오류");const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("Archer launch 표적 타입 오류");if ((raw[11]&3) || !(types_[raw[10]].flags1&0x10)) return false;
    if (!hooks_.common.targetPoint || !hooks_.common.createProjectile) throw std::logic_error("Archer 실제sunDisc Missile생성경계 누락");GunProjectileRequest request;request.firing=fields.Parent();request.target=sid;request.speed=35;request.displayType=state_.displayType;request.effectType=0;request.targetPosition=APoint(fields.bytes,12);request.targetCenter=hooks_.common.targetPoint(fields.Parent(),sid);hooks_.common.createProjectile(request);return true;
}
// 실제각상태는매틱갱신되며총예약시각으로발사호출을대신하지않는다.
void RawArcherAnimation::Fire(ArcherAnimationFields& fields) const {
    if (!fields.Target().value) throw std::logic_error("Archer Fire 표적0 원본 assert");
    if (state_.delta>0 && state_.delta<2.0) {
        const auto raw=Parent(fields);const bool alive=Track(fields);const double delta=static_cast<float>(state_.delta);AFloat(fields.bytes,64,static_cast<float>(delta*AFloat(fields.bytes,68)+AFloat(fields.bytes,64)));AFloat(fields.bytes,68,static_cast<float>(delta*AFloat(fields.bytes,72)+AFloat(fields.bytes,68)));const float threshold=static_cast<float>(static_cast<double>(state_.winding)*360.0+Degrees(APoint(raw,14),APoint(fields.bytes,20))-state_.fireBias);const auto position=AFloat(fields.bytes,64);
        if (!ReadArcher(fields.bytes,80)) {
            if (!ReadArcher(fields.bytes,76) && threshold-state_.springOffset<=position) { if (!hooks_.common.sound) throw std::logic_error("Archer 실제sasprung.wav 경계 누락");PutArcher(fields.bytes,76,1);AFloat(fields.bytes,72,-AFloat(fields.bytes,72));hooks_.common.sound(fields.Parent(),"sasprung.wav"); }
            if (!ReadArcher(fields.bytes,80) && threshold<=position) { if (!hooks_.notifySurface || !hooks_.common.sound) throw std::logic_error("Archer 실제발사출력/사운드경계 누락");PutArcher(fields.bytes,76,1);PutArcher(fields.bytes,80,1);pool_.AllocatedBytes(fields.Parent())[12]|=1;hooks_.notifySurface(fields.Parent());if (!Launch(fields) && alive) return;hooks_.common.sound(fields.Parent(),"safired.wav"); }
        }
        if (ReadArcher(fields.bytes,80) && ReadArcher(fields.bytes,76) && threshold+state_.resetOffset<=position) PutArcher(fields.bytes,76,0);
        if (position>0) { const auto next=FireFrame(position,ReadArcher(fields.bytes,76)!=0);if (next!=Frame(fields.Parent())) { SetFrame(fields.Parent(),next);if (AFloat(fields.bytes,68)<0) { if (!hooks_.common.sound) throw std::logic_error("Archer 실제sarachet.wav 경계 누락");hooks_.common.sound(fields.Parent(),"sarachet.wav"); }else if (Frame(fields.Parent())==2) { if (!hooks_.common.sound) throw std::logic_error("Archer 실제whoosh 경계 누락");hooks_.common.sound(fields.Parent(),"sunDiscThrowerWhoosh.wav"); } } }
    }
    const auto speed=AFloat(fields.bytes,68);if ((ReadArcher(fields.bytes,80) && speed<=state_.finishSpeed && std::abs(static_cast<double>(NormalizeArcher(AFloat(fields.bytes,64)))-state_.finishAngle)<state_.finishTolerance) || AFloat(fields.bytes,64)<0) Finish(fields,6,7);
}
// 동일월드adapter가읽는현재풀과타입전역이다.
const SidPool& RawArcherAnimation::Pool() const { return pool_; }
std::uint32_t RawArcherAnimation::Type() const { return state_.type; }
// 실제파생객체의원본이미지/종류를소유한다.
ArcherAnimation::ArcherAnimation(const RawArcherAnimation& logic,ArcherAnimationFields fields,bool fire):logic_(logic),fields_(std::move(fields)),fire_(fire) {}
// 실제 Kernel 틱에서 원본 회전 또는 발사 몸체를 선택한다.
void ArcherAnimation::RunFrame() { if (fire_) logic_.Fire(fields_);else logic_.Rotate(fields_); }
// 현재 Form/부모 번호를 원본 비정렬 이미지와 동기화한다.
void ArcherAnimation::OnAttached() { PutArcher(fields_.bytes,4,Form().value);PutArcher(fields_.bytes,8,Parent().value); }
// 원본 생성자의 부착 후 처리 및 독립 관찰에서 소유 이미지를 사용한다.
ArcherAnimationFields& ArcherAnimation::Fields() { return fields_; }
// 가상GunNotify와실제Archer Kill을동일Host에연결한다.
ArcherAnimationHooks MakeArcherAnimationHostHooks(SquidProcessHost& host,ArcherAnimationHooks hooks) {
    hooks.common.notify=[&host](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(form));if (!gun) throw std::logic_error("Archer 완료GunProcess 조회 오류");gun->Notify(event,info,value); };
    hooks.common.kill=[&host](Sid form,std::uint32_t flags) { auto* animation=dynamic_cast<ArcherAnimation*>(host.FromForm(form));if (!animation) throw std::logic_error("Archer 완료process 조회 오류");host.Kill(*animation,flags); };return hooks;
}
// 실제부착뒤Firector 나머지를수행해원본출력통지순서를유지한다.
ArcherAnimation* CreateArcherAnimation(SquidProcessHost& host,const RawArcherAnimation& logic,std::uint32_t stage,Sid gunForm,Sid target) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("Archer/Host SID 풀 불일치");auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun || host.Pool().Slot(gun->Parent())[10]!=logic.Type() || (stage!=2 && stage!=5)) throw std::logic_error("Archer factory 단계/부모 오류");ArcherAnimationFields fields;logic.Initialize(fields,stage==5,gunForm,target);auto* animation=dynamic_cast<ArcherAnimation*>(host.Attach(std::make_unique<ArcherAnimation>(logic,std::move(fields),stage==5),stage==5 ? 35U : 36U,gun->Parent(),0x10));if (animation && stage==5) logic.InitializeFire(animation->Fields());return animation;
}
// 첫부모type조회로새분기를선택하며기존캐논callback수명을보존한다.
GunProcessHooks MakeArcherAnimationProcessHooks(SquidProcessHost& host,const RawArcherAnimation& logic,GunProcessHooks hooks) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("Archer/Gun SID 풀 불일치");hooks.startAnimation=[&host,&logic,fallback=std::move(hooks.startAnimation)](std::uint32_t stage,Sid gunForm,Sid target) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun) throw std::logic_error("Archer factory GunProcess 조회 오류");if (host.Pool().Slot(gun->Parent())[10]==logic.Type()) { if (!CreateArcherAnimation(host,logic,stage,gunForm,target)) throw std::logic_error("Archer 삭제중factory 실패");return; }if (!fallback) throw std::logic_error("Archer 외부모 실제animation 미복원");fallback(stage,gunForm,target); };return hooks;
}
// 동일실제Missile 생성/공간/Kernel경계를sunDisc launch에연결한다.
ArcherAnimationHooks MakeArcherMissileHooks(SquidProcessHost& host,const RawMissile& missile,ArcherAnimationHooks hooks) { hooks.common=MakeGunMissileHooks(host,missile,std::move(hooks.common));return hooks; }
}
