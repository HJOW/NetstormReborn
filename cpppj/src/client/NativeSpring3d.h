// Animator3d의 0x5c 바이트 이동 계산부를 같은 Rig/카메라 필드에 적용한다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::client { // 원문 시간과 카메라 높이는 실제 Client의 현재 값을 빌린다.
struct NativeSpring3dGlobals { // 별도 시간 누적기나 설정 사본을 만들지 않는 전역 뷰다.
    const double& now; //0055b4d0의 현재 게임 시각이다.
    const std::uint32_t& fast; //0055a480이 비영이면 새 높이 이동 속도를12로 정한다.
    const float& cameraHeight; //00544d74이며 flag10h의 현재 높이 고정에만 사용한다.
};
class NativeSpring3d { // 바이트와 전역 소유자는 이 동기 계산 객체보다 오래 살아야 한다.
public:
    NativeSpring3d(std::span<std::uint8_t> fields,NativeSpring3dGlobals globals); // 실제 0x5c 필드 뷰만 빌리며 원문 생성자 효과는 추가하지 않는다.
    void Initialize() const; //00418520의 초기 쓰기만 수행하고 패딩+4c와 끝 패딩을 보존한다.
    void Target(float x,float y,float z,std::uint32_t flags) const; //004185b0의 두 완료 비트 보존·선택 목표/속도·현재 시각 저장이다.
    bool Advance() const; //00418640의 엄격한 1/30초 초과 반복·가속/감속·완료 비트를 적용한다.
private:
    std::uint32_t Read(std::size_t at) const; // 비정렬 little endian DWORD를 읽는다.
    void Put(std::size_t at,std::uint32_t value) const; // 선택한 네 바이트만 쓴다.
    float Float(std::size_t at) const; // 원문 F32 필드를 비트 보존하여 읽는다.
    void Float(std::size_t at,float value) const; // 명시된 FSTP32 저장 위치를 표현한다.
    double Time() const; //+50의 비정렬 QWORD 시각을 읽는다.
    void Time(double value) const; //+50/+54의 실제 double 저장이다.
    std::span<std::uint8_t> fields_; // Rig+10 또는 카메라 계산부의 같은 실제 바이트다.
    NativeSpring3dGlobals globals_; // 현재 전역 주소를 유지하며 값을 동기화하지 않는다.
};
}
