// Animator3d의 실제 Kernel/ProcessForm Rig와 객체·Gump·Image 수명을 복원한다.
#pragma once
#include "client/NativeSpring3d.h"
#include "client/NativeMetaDisplay.h"
#include "o/SquidProcess.h"

namespace netstorm::client { // 실제 소비자는 현재 Client/CS와 같은 Pool의 장치만 공급한다.
struct NativeIslandRigGlobals { // 진행 중 전역을 별도 사본으로 동기화하지 않는다.
    NativeMetaPollGlobals meta; // 현재 CS·Pool 상한·TYPE·권한·카메라·화면 물리 뷰다.
    NativeSpring3dGlobals spring; // 같은 게임 시각·special DWORD·카메라 높이 주소다.
};
using NativeIslandRigGlobalsGetter=std::function<NativeIslandRigGlobals()>; // 프레임마다 현재 UI/viewport를 다시 빌리며 오래된 Uber의 참조를 저장하지 않는다.
struct NativeIslandRigHooks { // 도달한 실제 가상 효과만 그 위치에서 요구한다.
    NativeMetaDisplayHooks object; // 같은 raw/Unpop/QA/Pop/Destroy/Gump 조회/진단이다.
    std::function<void(std::uint32_t)> killGump; //00466480의 현재 등록 ID로 선택한 실제 Gump Kill이다.
};
class NativeIslandRig final:public o::SquidProcess { // 원문0x90 필드와 C++ 공통 Host 수명을 함께 유지한다.
public:
    NativeIslandRig(o::Sid object,float height,std::uint32_t kill,NativeIslandRigGlobalsGetter globals,NativeIslandRigHooks hooks); //00418b10의 부착 전 초기화이며 실제 Attach는 공장이 한다. getter는 CS 소유자도 반납까지 유지해야 한다.
    ~NativeIslandRig() override; //004195b0의 Image 반납 뒤 공통 C++ base 반납이다.
    void OnAttached() override; //00418de0의 현재 CS Image 복사·QA/배율·특수 비트·재배치다.
    void RunFrame() override; //00418eb0 전체의 진행/투영/가시성·완료/삭제 흐름이다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 실제 VT+1c=00480260은 빈 함수다. 미복원 기본 예외로 보내지 않는다.
    void Target(float x,float y,float z,std::uint32_t flags); // 같은 내장 Spring3d 목표를 갱신한다.
    void SetKill(std::uint32_t value); //00418ca0의 기존 kill 불가역 관문과 실제 Gump 삭제·배율/QA 쓰기다.
    void SetDelay(std::uint32_t value); // 원문+70 DWORD를 그대로 쓴다. 완료 때 하위BYTE만 객체에 복사한다.
    std::span<const std::uint8_t> Fields() const; // 정적 진단/현재 장치가 같은 원문 필드를 읽는다.
private:
    NativeSpring3d Spring(); // 내장+10의 실제0x5c 필드 뷰다.
    std::span<std::uint8_t> CheckObject(std::uint32_t object,std::uint32_t line) const; // SID와 현재 두 TYPE의 원문 진단 후 같은 실제 raw를 반환한다.
    void Gumps(std::uint32_t object,int mode) const; // 기본 true Form finder로 mode<0 Kill,0 숨김,1 표시를 수행한다.
    std::array<std::uint8_t,0x90> fields_{}; // 원문 필드 이미지이며 미작성 패딩을 후속 메서드가 덮지 않는다.
    NativeIslandRigGlobalsGetter globals_;NativeIslandRigHooks hooks_; // Host의 프레임/삭제까지 현재 실제 전역과 장치를 조회한다.
    ClientChallengeGlobals& challenge_; // destructor는 UI getter를 호출하지 않고 같은 장기 Image 할당 소유자만 사용한다.
};
NativeIslandRig* FindOrCreateNativeIslandRig(o::SquidProcessHost& host,o::Kernel& kernel,o::Sid object,float height,std::uint32_t kill,NativeIslandRigGlobalsGetter globals,const NativeIslandRigHooks& hooks); //004192a0의 현재TYPE 슬롯0..39999 첫 부모 일치 또는 flags10h 실제 부착이다.
void EnterNativeBattleRig(NativeIslandRig& rig,std::uint32_t delay); //004194c0의 높이0/flags8 목표·지연·kill0이다.
void EnterNativeParticipantRig(NativeIslandRig& rig,bool self,std::uint32_t delay); //004193f0의 자기 flags18h/일반8h 목표·지연·kill0이다.
void LeaveNativeIslandRig(NativeIslandRig& rig,bool downward); //00419390/419460의 높이±12·지연30/-31·kill1이다.
void MoveNativeParticipantRig(NativeIslandRig& rig,std::span<const std::uint8_t> participant); //00419520의 현재9c/a0 수평 목표·지연0·kill0이다.
}
