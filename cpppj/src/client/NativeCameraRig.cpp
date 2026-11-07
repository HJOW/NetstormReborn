// 원문 CameraRig의 현재 개수/갱신 예약·목표 재사용·높이 저장 순서를 보존한다.
#include "client/NativeCameraRig.h"
#include "client/ClientMain.h"
#include "client/ClientChallengeGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawCampaignSession.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 실제 Client 장치는 OS/소켓을 열거나 게임 프레임을 임의 실행하지 않는다.
namespace { // 원문 DWORD/F32 필드와 실제 선택 callback을 공유한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("CameraRig 실제 장치 미연결");return function; } // 원문 효과에 도달한 위치에서만 장치를 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("CameraRig 실제 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } // 비정렬 네 BYTE를 하위부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("CameraRig 실제 쓰기 범위");for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } // 해당 DWORD만 쓴다.
}
// 실제 Attach/두 번째 owner 조회는 상위 공장에서 이 초기화 직후에 계속 수행한다.
NativeCameraRig::NativeCameraRig(float initial,float target,NativeCameraRigGlobals globals,NativeCameraRigHooks hooks):globals_(globals),hooks_(std::move(hooks)) { Put(fields_,8,0);Put(fields_,4,0);Put(fields_,0,0x501ae8);Spring().Initialize();Put(fields_,0x1c,0);Put(fields_,0x10,0);Put(fields_,0x20,0);Put(fields_,0x14,0);Put(fields_,0x24,std::bit_cast<std::uint32_t>(initial));Put(fields_,0x18,std::bit_cast<std::uint32_t>(initial));Spring().Target(0,0,target,8);++globals_.camera.animating;if (!globals_.camera.animating) Need(hooks_.assertion)("animating != 0",".\\Animator3d.cpp",0x17f); }
NativeCameraRig::~NativeCameraRig() { --globals_.camera.animating;if (globals_.camera.animating==0xffffffffU) Need(hooks_.assertion)("animating != 0xFFFFFFFF",".\\Animator3d.cpp",0x185); } // unsigned DWORD 감김 뒤 현재 값을 진단한다.
NativeSpring3d NativeCameraRig::Spring() { return {std::span(fields_).subspan(0x10,0x5c),globals_.spring}; } // 실제 같은 내장 필드를 빌린다.
void NativeCameraRig::OnAttached() { Put(fields_,4,Form().value);Put(fields_,8,Parent().value); } // Native base 필드 쓰기의 C++ 거울이며 추가 가상 효과는 없다.
void NativeCameraRig::Target(float target) { Spring().Target(0,0,target,8); } // 재사용은 현재 위치/개수를 보존한다.
std::span<const std::uint8_t> NativeCameraRig::Fields() const { return fields_; } // 조회가 프레임을 진행하지 않는다.
void NativeCameraRig::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {} //00480260의 실제 빈 몸체다.
// 분모는 아직 이전 카메라 높이로 계산하고 실제 새 높이를 그 뒤에 저장한다.
void NativeCameraRig::RunFrame() { if ((Read(fields_,0x28)&0x60000000U)==0x60000000U) { ++globals_.refresh;Host()->Kill(*this,0);return; }if (Spring().Advance()) ++globals_.rebuild;Need(hooks_.denominator)(MetaProjectionDenominator(globals_.camera));globals_.camera.height=std::bit_cast<float>(Read(fields_,0x24)); }
// 첫 owner와 생성자 뒤의 두 번째 owner를 합치지 않아 원래 조기 반환과 자기 반납을 보존한다.
void RetargetNativeCameraRig(float initial,float target,NativeCameraRigGlobals globals,const NativeCameraRigHooks& hooks) {
    const auto object=Need(hooks.currentObject)();if (!object) return;auto& kernel=Need(hooks.kernel)();
    for (o::ProcessId pid=0;pid<=kernel.Capacity();++pid) { const auto* process=dynamic_cast<o::SquidProcess*>(kernel.Get(pid));if (!process) continue;const auto raw=Need(hooks.raw)(process->Form().value);if (raw.size()<11) throw std::out_of_range("CameraRig 현재 ProcessForm 물리 부족");if (raw[10]!=globals.type) continue;auto* selected=kernel.Get(pid);if (!selected) Need(hooks.assertion)("p != NULL",".\\Animator3d.cpp",0x344);auto* rig=dynamic_cast<NativeCameraRig*>(selected);if (!rig) throw std::logic_error("현재 CameraAnimator TYPE의 실제 클래스 미연결");if (rig->Parent().value==object) { rig->Target(target);return; } } // Native finder의0..39999 순서/현재 Form TYPE·첫 같은 부모를 유지한다.
    auto rig=std::unique_ptr<NativeCameraRig>(new(std::nothrow) NativeCameraRig(initial,target,globals,hooks));if (!rig) return;const auto current=Need(hooks.currentObject)();if (!current) return;if (current>65535U) throw std::out_of_range("CameraRig 현재 DWORD owner의 WORD 물리 범위");static_cast<void>(Need(hooks.host)().Attach(std::move(rig),globals.type,o::Sid{static_cast<std::uint16_t>(current)},0x10));
}
// getter가 실제 장기 전역을 pin하며 owner0에서는 Session/Host를 요구하지 않는다.
void AnimateClientMetaCamera(Client& client,float initial,float target) {
    const auto game=client.GameplayGlobals();const auto challenge=client.ChallengeGlobals();const auto local=client.LocalStateGlobals();const auto clock=client.FrameClockOwner();NativeCameraRigHooks hooks;
    hooks.currentObject=[&client,game,challenge,clock] { static_cast<void>(clock);if (!game->player.playerKnown) return 0U;if (client.SurfaceStorage()->ownerMode.challenge) { if (!game->player.playerKnown) return 0U;return Read(challenge->Participant(game->player.battleOwner),0xb8); }return game->RequireSession().CurrentTechnologyParent(); }; //00470d20/479650의 두 playerKnown 조회와 실제 기술 부모를 유지한다.
    hooks.raw=[&client](auto number)->std::span<const std::uint8_t> { const auto storage=client.SurfaceStorage();if (number>65535U || number>=storage->pool.Capacity()) throw std::out_of_range("CameraRig 실제 DWORD SID 물리 범위");return storage->pool.Slot(o::Sid{static_cast<std::uint16_t>(number)}); };
    hooks.kernel=[&client]()->o::Kernel& { return client.GetKernel(); };hooks.host=[&client]()->o::SquidProcessHost& { return client.SurfaceStorage()->Processes().Host(); };
    hooks.denominator=[game,cloud=client.KencloudGlobals(),local](auto value) { static_cast<void>(local);game->Camera().SelectDenominator(value,cloud->WorldRedraw()); }; //메타 갱신 예약 owner도 프로세스 반납까지 함께 유지한다.
    hooks.assertion=[&client](auto expression,auto file,auto line) { client.SurfaceStorage()->AssertDisplay(expression,file,line); };
    RetargetNativeCameraRig(initial,target,{challenge->Projection().camera,{clock->game,game->player.special,challenge->Projection().camera.height},challenge->CameraAnimatorType(),local->MetaRefresh(),local->MetaRebuild()},hooks);
}
void StartClientMetaIntroCamera(Client& client) { AnimateClientMetaCamera(client,-12.0F,0.0F); } //00419840의 두 원문 float 인자다.
}
