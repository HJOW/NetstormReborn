// 원문 전체 흐름과 기계어의 부호 확장·FTOL/높이 판정을 같은 실제 장치로 처리한다.
#include "client/NativeIslandRig.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client { // Kernel이 소유한 Rig는 가짜 SID나 별도 메타 월드를 만들지 않는다.
namespace { // 원문 비정렬 필드 폭과 선택한 장치의 실패를 보존한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("IslandRig 실제 장치 미연결");return function; } // 선택한 효과 위치에서만 하위를 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("IslandRig 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } // 각 필드를 하위BYTE부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("IslandRig 실제 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 지정된1/2/4BYTE만 변경한다.
float Float(std::span<const std::uint8_t> bytes,std::size_t at) { return std::bit_cast<float>(Read(bytes,at)); } // 저장된 F32 비트를 읽는다.
}
// 생성자 원문은 실제 객체를 확인한 뒤 내장 좌표/kill 비트를 쓰고 필요하면 Gump를 지운다.
NativeIslandRig::NativeIslandRig(o::Sid object,float height,std::uint32_t kill,NativeIslandRigGlobalsGetter globals,NativeIslandRigHooks hooks):globals_(std::move(globals)),hooks_(std::move(hooks)),challenge_(Need(globals_)().meta.display.challenge) {
    Put(fields_,8,0);Put(fields_,4,0);Put(fields_,0,0x501ab0);Spring().Initialize();Put(fields_,0x78,0);Put(fields_,0x7c,0);Put(fields_,0x84,0);Put(fields_,0x80,0);const auto raw=CheckObject(object.value,0x1d4);
    const auto x=Read(raw,0xe),y=Read(raw,0x12);Put(fields_,0x70,0);Put(fields_,0x74,(Read(fields_,0x74)&0xfffffff8U)|(kill&1U));Put(fields_,0x1c,x);Put(fields_,0x10,x);Put(fields_,0x20,y);Put(fields_,0x14,y);Put(fields_,0x24,std::bit_cast<std::uint32_t>(height));Put(fields_,0x18,std::bit_cast<std::uint32_t>(height));if (Read(fields_,0x74,1)&1U) Gumps(object.value,-1);
}
NativeIslandRig::~NativeIslandRig() { challenge_.ClearArray(std::span(fields_).subspan(0x78,16)); } // 실제 두 버퍼 해제 뒤+10 shapeSize와 패딩을 보존한다.
NativeSpring3d NativeIslandRig::Spring() { return {std::span(fields_).subspan(0x10,0x5c),Need(globals_)().spring}; } // 같은 필드/전역 주소만 빌린다.
std::span<std::uint8_t> NativeIslandRig::CheckObject(std::uint32_t object,std::uint32_t line) const { const auto globals=Need(globals_)();if (object<1 || object>=globals.meta.display.capacity) Need(hooks_.object.assertion)("s.isValid()",".\\Animator3d.cpp",line);const auto raw=Need(hooks_.object.raw)(object);if (raw[10]!=globals.meta.display.challenge.BattleType() && raw[10]!=globals.meta.display.challenge.ParticipantType()) Need(hooks_.object.assertion)("s.getType() == battleIslandType || s.getType() == playerIslandType",".\\Animator3d.cpp",line+1U);return raw; } // 진단이 복귀하면 같은 raw/현재TYPE로 다음 진단을 수행한다.
// 기본 Form finder의 next 선저장과 각 현재TYPE 필터를 실제 효과 전에 수행한다.
void NativeIslandRig::Gumps(std::uint32_t object,int mode) const {
    const auto g=Need(globals_)().meta;const auto owner=Need(hooks_.object.raw)(object);if (g.checking && !g.metamap && (Read(owner,0xb,1)&2U) && !g.boss) Need(hooks_.object.assertion)("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy","\\Ns\\O\\Squidfinder.cpp",0x158);
    auto current=Read(owner,6,2);std::unordered_set<std::uint32_t> visited;
    // 손상된 순환만 호스트 예외로 거부하며 정상 목록의 삭제 뒤에도 미리 저장한 next로 진행한다.
    while (current) { if (!visited.insert(current).second) throw std::logic_error("IslandRig Form finder 순환");const auto raw=Need(hooks_.object.raw)(current);const auto next=Read(raw,4,2);if (Read(raw,10,1)<70 && Read(raw,10,1)!=g.excludedType) { const auto type=Read(raw,10,1);if (!(type>9 && static_cast<std::int32_t>(type)<std::bit_cast<std::int32_t>(g.processTypeEnd))) { const auto id=Read(raw,0x12);if (auto* node=Need(hooks_.object.findGump)(id)) { if (mode<0) Need(hooks_.killGump)(id);else if (mode) node->flags|=1U;else node->flags&=~1U; } } }current=next; }
}
// Attach가 갱신한 실제 Form/Parent를 원문+4/+8에도 기록한 뒤 현재 CS Image를 깊게 복사한다.
void NativeIslandRig::OnAttached() {
    Put(fields_,4,Form().value);Put(fields_,8,Parent().value);const auto object=Read(fields_,8);const auto raw=Need(hooks_.object.raw)(object);const auto type=Read(raw,10,1);auto& challenge=challenge_;std::span<const std::uint8_t> source;
    if (type==challenge.BattleType()) source=challenge.Battle(Read(raw,0xc,2)).subspan(0x24,20);else if (type==challenge.ParticipantType()) source=challenge.Participant(Read(raw,0xc,2)).subspan(0xbc,20);else { if (Read(fields_,0x74,1)&1U) Need(hooks_.object.destroy)(object,0);else Host()->Kill(*this,0);return; }
    challenge.CopyImage(std::span(fields_).subspan(0x78,20),source,[this](auto expression,auto line) { Need(hooks_.object.assertion)(expression,".\\Image.cpp",line); });Need(hooks_.object.unpop)(object,0);Put(raw,8,0,2);
    if (!(Read(fields_,0x74,1)&1U)) Put(raw,0x1a,1);else { Put(raw,0x1a,0x8001);Need(hooks_.object.setKind)(object,Read(fields_,4)); }Put(raw,0x28,Read(raw,0x28,1)|8U,1);const auto y=Float(raw,0x12),x=Float(raw,0xe);Need(hooks_.object.move)(object,x,y,0);
}
// 이미 kill인 Rig는 다시 살릴 수 없고 kill 전환의 부분 효과를 취소하지 않는다.
void NativeIslandRig::SetKill(std::uint32_t value) { if (Read(fields_,0x74,1)&1U) return;if (value) { const auto object=Read(fields_,8);const auto raw=CheckObject(object,0x200);Gumps(object,-1);Put(raw,0x1a,Read(raw,0x1a)|0x8000U);Need(hooks_.object.setKind)(object,Read(fields_,4)); }const auto flags=Read(fields_,0x74);Put(fields_,0x74,flags^((flags^value)&1U)); }
void NativeIslandRig::Target(float x,float y,float z,std::uint32_t flags) { Spring().Target(x,y,z,flags); } // 원문 내장 spring 호출이다.
void NativeIslandRig::SetDelay(std::uint32_t value) { Put(fields_,0x70,value); } // 부호를 정규화하지 않는다.
std::span<const std::uint8_t> NativeIslandRig::Fields() const { return fields_; } // 현재 같은 원문 필드를 읽는다.
// Native는 이동 완료 관문을 spring 갱신 전에 읽으며 이번에 완료돼도 다음 프레임에 반납한다.
void NativeIslandRig::RunFrame() {
    const auto object=Read(fields_,8);const auto raw=Need(hooks_.object.raw)(object);
    if ((Read(fields_,0x28)&0x60000000U)==0x60000000U) { if (Read(fields_,0x74,1)&1U) { Need(hooks_.object.destroy)(object,0);return; }Need(hooks_.object.unpop)(object,0);Put(raw,0x23,Read(fields_,0x70,1),1);Put(raw,8,0,2);Put(raw,0x1a,0);Put(raw,0x28,Read(raw,0x28,1)&0xf7U,1);Need(hooks_.object.move)(object,Float(fields_,0x1c),Float(fields_,0x20),0);Host()->Kill(*this,0);return; }
    Spring().Advance();const auto z=Float(fields_,0x24);const auto current=Need(globals_)().meta.display;const auto point=ProjectMetaCoordinates(current.challenge.Projection().camera,current.view,Float(fields_,0x1c),Float(fields_,0x20),z);const std::uint32_t scaleClipped=((point.scale<0.9 || point.scale>1.1) && point.scale!=0.0F) ? 1U:0U;
    const auto flags=Read(fields_,0x74);const auto oldClipped=(flags&2U) ? 0xffffffffU:0U,oldScaleClipped=(flags&4U) ? 0xffffffffU:0U;if (oldClipped!=point.clipped || oldScaleClipped!=scaleClipped) Gumps(object,(!point.clipped && !scaleClipped) ? 1:0);
    Put(fields_,0x74,(((scaleClipped&1U)<<1)|(point.clipped&1U))<<1|(Read(fields_,0x74)&0xfffffff9U));Need(hooks_.object.unpop)(object,0);const double scaled=static_cast<double>(point.scale)*65536.0;Put(raw,8,std::bit_cast<std::uint32_t>(CameraFtolLow(scaled)),2);const auto currentFlags=Read(fields_,0x74),fixed=std::bit_cast<std::uint32_t>(CameraFtolLow(scaled));Put(raw,0x1a,((((currentFlags&1U)<<2)|(currentFlags&2U))<<13)|((fixed>>16)&0x3fffU));if (z>0.0F) Put(raw,0x23,30,1);Need(hooks_.object.move)(object,point.position[0],point.position[1],0);
}
// 실제 가상표의00480260은 RET이며 어떤 통지도 추가 처리하지 않는다.
void NativeIslandRig::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {}
// Native Kernel finder는0부터 상한까지 현재 Form TYPE를 읽고 첫 같은 부모만 반환한다.
NativeIslandRig* FindOrCreateNativeIslandRig(o::SquidProcessHost& host,o::Kernel& kernel,o::Sid object,float height,std::uint32_t kill,NativeIslandRigGlobalsGetter globals,const NativeIslandRigHooks& hooks) {
    for (o::ProcessId pid=0;pid<=kernel.Capacity();++pid) { const auto* process=dynamic_cast<o::SquidProcess*>(kernel.Get(pid));if (!process) continue;const auto form=Need(hooks.object.raw)(process->Form().value);if (Read(form,10,1)!=Need(globals)().meta.display.challenge.IslandAnimatorType()) continue;auto* selected=kernel.Get(pid);if (!selected) Need(hooks.object.assertion)("p != NULL",".\\Animator3d.cpp",0x327);auto* rig=dynamic_cast<NativeIslandRig*>(selected);if (!rig) throw std::logic_error("현재 IslandAnimator TYPE의 실제 Rig 클래스 미연결");if (rig->Parent()==object) return rig; } // 매 슬롯에서 현재 객체와 TYPE를 읽으며 active/자기소유 필터는 없다.
    auto rig=std::unique_ptr<NativeIslandRig>(new(std::nothrow) NativeIslandRig(object,height,kill,globals,hooks));if (!rig) return nullptr;return static_cast<NativeIslandRig*>(host.Attach(std::move(rig),Need(globals)().meta.display.challenge.IslandAnimatorType(),object,0x10));
}
void EnterNativeBattleRig(NativeIslandRig& rig,std::uint32_t delay) { rig.Target(0,0,0,8);rig.SetDelay(delay);rig.SetKill(0); } //004194c0은 생성 시 높이12/kill0을 caller가 공급한다.
void EnterNativeParticipantRig(NativeIslandRig& rig,bool self,std::uint32_t delay) { rig.Target(0,0,0,self ? 0x18U:8U);rig.SetDelay(delay);rig.SetKill(0); } //004193f0의 생성 높이는-12/kill0이다.
void LeaveNativeIslandRig(NativeIslandRig& rig,bool downward) { rig.Target(0,0,downward ? -12.0F:12.0F,8);rig.SetDelay(downward ? 0xffffffe1U:30U);rig.SetKill(1); } //00419460의 음수 지연도 DWORD 비트로 보존한다.
void MoveNativeParticipantRig(NativeIslandRig& rig,std::span<const std::uint8_t> participant) { const auto y=Float(participant,0xa0),x=Float(participant,0x9c);rig.Target(x,y,0,4);rig.SetDelay(0);rig.SetKill(0); } //00419520의 원문 인자 읽기 순서를 지킨다.
}
