// Animator3d의 카메라 높이 프로세스와 실제 Client 바인더를 복원한다.
#pragma once
#include "client/NativeSpring3d.h"
#include "client/NativeMetaProjection.h"
#include "o/SquidProcess.h"

namespace netstorm::client { // 카메라·시각·메타 갱신 예약은 기존 실제 owner의 같은 주소다.
class Client; // 바인더는 실제 현재 Client를 사용하며 생성만으로 프레임/통신을 진행하지 않는다.
struct NativeCameraRigGlobals { // 카메라 필드·시각·현재 TYPE와 기존 메타 갱신 예약의 비소유 뷰다.
    MetaProjectionCamera& camera; //00544d6c..78의 같은 세 float/animating DWORD다.
    NativeSpring3dGlobals spring; // 현재 게임 시각·special·같은 카메라 높이다.
    const std::uint32_t& type; //00541144의 현재53 TYPE이며 검색/생성이 공유한다.
    std::uint32_t& refresh;std::uint32_t& rebuild; //005c85d8/dc의 기존 메타 갱신 예약값이다. 별도 통계 사본을 만들지 않는다.
};
struct NativeCameraRigHooks { // 선택한 실제 효과만 그 위치에서 요구한다.
    std::function<std::uint32_t()> currentObject; //00470d20의 playerKnown→메타 자기 섬/현재 기술 부모 조회다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> raw; // 같은 현재 Pool의50BYTE를 읽는다.
    std::function<o::Kernel&()> kernel;std::function<o::SquidProcessHost&()> host; // owner0 조기 반환 뒤에만 현재 장치를 조회한다.
    std::function<void(std::int32_t)> denominator; //004978d0의 실제 Renderer 분모/월드 다시 그리기 쓰기다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Animator3d의17f/185/344 진단이다.
};
class NativeCameraRig final:public o::SquidProcess { // 실제 Kernel과 ProcessForm의 C++ 소유권을 사용한다.
public:
    NativeCameraRig(float initial,float target,NativeCameraRigGlobals globals,NativeCameraRigHooks hooks); //00419600의 부착 전 spring·목표·animating 증가를 수행한다.
    ~NativeCameraRig() override; //00419190의 현재 개수 감소/underflow 진단이다.
    void OnAttached() override; // 공통 Host가 쓴 실제 Form/Parent를 원문 필드 이미지에도 반영한다. Native 가상 몸체는 빈 함수다.
    void RunFrame() override; //00419210의 완료/예약→spring→기존 높이 분모→새 높이 쓰기다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override; // 실제 VT+1c=00480260의 빈 함수다.
    void Target(float target); // 기존 프로세스에는 초기 높이를 다시 쓰지 않고 목표만 바꾼다.
    std::span<const std::uint8_t> Fields() const; // 같은0x70 원문 필드를 읽는 진단 뷰다.
private:
    NativeSpring3d Spring(); //+10의 같은0x5c 필드와 실제 전역을 빌린다.
    std::array<std::uint8_t,0x70> fields_{}; // 공통 base와 내장 spring이며 끝 패딩은 후속 메서드가 쓰지 않는다.
    NativeCameraRigGlobals globals_;NativeCameraRigHooks hooks_; // callbacks는 같은 장기 전역 소유자를 마지막 반납까지 유지해야 한다.
};
void RetargetNativeCameraRig(float initial,float target,NativeCameraRigGlobals globals,const NativeCameraRigHooks& hooks); //00419730 전체:현재owner0 반환→현재TYPE/부모 검색→목표 변경 또는 생성/두번째owner 조회/flags10h 부착이다.
void AnimateClientMetaCamera(Client& client,float initial,float target); // 실제 Client/CS/같은 Kernel·Host/Renderer·현재 게임 시각을 위 부모에 연결한다.
void StartClientMetaIntroCamera(Client& client); //00419840의 initial-12/target0 전용 wrapper다. 참가자 생성 caller 연결은 후속이다.
}
