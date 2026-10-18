// 훨리기그의 실제 전용 폭탄/초기화/통지/장식 프레임을 같은 원본 저장소에서 수행한다.
#pragma once
#include "o/RawFlyerProcess.h"
#include "o/SquidDisplay.h"

namespace netstorm::o {
// 실제 폭탄 TYPE의 세 프레임 필드와 소리 토큰이다. B 끝은 원본+174 값 그대로다.
struct SunFlyerFrameType {
    std::int32_t fallingFrames{},explosionFirst{},explosionEnd{}; // TYPE+170/+134/+174를 다른 구간 의미로 바꾸지 않는다.
    std::uint32_t sound{}; // 생성된 폭탄의 현재 TYPE+70 소리 토큰이다.
};
// 외부 효과 뒤에도 같은 현재 전역을 읽는다.
struct SunFlyerFrameGlobals {
    const bool& boss; // 실제00540bc4의 삭제 권한이다.
    const bool& paused; // 실제00460de0의 중첩 정지 판정이다.
    const double& now; // 실제0055b4d0의 게임 시각이다.
    const std::uint32_t& bombType; // 실제005412e4의 현재 폭탄163 번호다.
};
// 기존 실제 공통 제어와 가상 공간/객체/장치를 연결하는 경계다.
struct SunFlyerFrameHooks {
    std::function<void(std::uint32_t)> assertion; // 원본 불변식 실패의 줄 번호다.
    std::function<bool(Sid,bool)> robust; // 실제004ac140의 현재 SID 진단/유효 판정이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 현재 가상10의 삭제다.
    std::function<void(Sid,std::uint32_t)> unpop; // 실제 현재 가상48 공간 해제다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 실제 현재 가상90 좌표 등록이다.
    std::function<void(Sid,std::uint32_t)> repop; // 실제 현재 가상4c 재배치다.
    std::function<bool(Sid,Sid)> mayTarget; // 현재 부모 문맥과 현재 WORD 표적으로 원본46af80을 실행한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제 Factory의 현재 폭탄/flags2 생성이다.
    std::function<void(float,float,std::uint32_t)> sound; // 실제4a9c30의 현재 위치/폭탄 소리다.
    std::function<std::int32_t(Sid)> runCount; // 실제4af110의 현재 부모 프레임 글자 개수다.
    std::function<void(FlyerProcessFields&)> baseFrame; // 기존 실제45c9d0 공통 비행 제어다.
    std::function<void(FlyerProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t)> baseNotify; // 기존 실제45b760이다.
    std::function<void(FlyerProcessFields&)> baseParentDying; // 기존 실제45b7a0의 현재 홈 통지다.
};
// 공통 비행과 같은136바이트를 읽고 쓰며 별도 위치/표적/시각을 만들지 않는다.
class RawSunFlyerFrame {
public:
    RawSunFlyerFrame(SidPool& pool,std::span<const SquidDisplayShape> shapes,std::span<const SunFlyerFrameType> types,
        SunFlyerFrameGlobals globals,SunFlyerFrameHooks hooks); // PATCH188/SHP/현재 전역의 수명을 확인한다.
    void Initialize(FlyerProcessFields& fields) const; // 45bd00의 선택 필드만 초기화한다.
    void Frame(FlyerProcessFields& fields) const; // 45d2c0의 폭탄/발사/공통Frame 전체 순서다.
    void Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const; // 45bd60의 발사 타이머/공통 사건 분배다.
    void ClearBomb(FlyerProcessFields& fields) const; // 45be00의 현재 폭탄 삭제 뒤 DWORD0이다.
    void ParentDying(FlyerProcessFields& fields) const; // 45beb0의 폭탄 정리 뒤 공통 홈 통지다.
    std::int32_t Animation(FlyerProcessFields& fields) const; // 45be40의 두 실제 fmod와 float 저장/반올림이다.
    static double ReturnDelay(); // 실제45e170의 float60을 double로 반환한다.
private:
    float Width(Sid parent) const; // 현재 SHP 프레임의 첫float 월드 폭이며 TYPE 발자국과 구별한다.
    SidPool& pool_; // 부모/폭탄/가상 효과가 공유하는 현재 풀이다.
    std::span<const SquidDisplayShape> shapes_; // 같은 실제 로더의 현재 물리 프레임 자료다.
    std::vector<SunFlyerFrameType> types_; // 같은 TYPE 번호의 불변 폭탄 프레임/소리 필드다.
    SunFlyerFrameGlobals globals_; // 같은 월드의 현재 정지/시각/타입 참조다.
    SunFlyerFrameHooks hooks_; // 원본이 선택한 기존 실제 하위 몸체다.
};
}
