// Evilray.cpp의 실제 프레임·기한·음향·표시 영역과 세 타원 출력 요청을 복원한다.
#pragma once
#include "o/RawEvilRayLifetime.h"

namespace netstorm::o { // 실제 입자 저장소를 갱신하며 별도의 게임 모형을 만들지 않는다.
// 원본004dae20에 제출하는 원형 타원 요청이다. 래스터 wrapper는 세로 반경을 radius-1로 쓴다.
struct EvilRayCircle {
    std::int32_t x{},y{},radius{}; // 두 번 정수화한 현재 화면 중심과 signed 반경이다.
    std::uint32_t color{}; // 실제 공유3색 중 선택한 전체 DWORD다.
};
// 실제 메타데이터/음향/dirty/삭제/래스터의 선택 장치다. 몸체가 도달한 곳에서만 요구한다.
struct EvilRayFrameHooks {
    std::function<std::int16_t(Sid)> shapeWidth; // 현재 source TYPE/방향/프레임의 signed SHP 폭이다.
    std::function<std::uint32_t(Sid,bool)> soundId; // false는현재 TYPE+70, true는+74 음향 번호다.
    std::function<void(float,float,std::uint32_t)> sound; // 현재 raw 좌표와 음향 번호의 실제 위치 재생이다.
    std::function<void(FieryTailDirtyRect)> invalidate; // 원문 이전/새 영역의 순서 있는 dirty 요청이다.
    std::function<void(std::uint32_t)> kill; // 원문0041bf10의 flags0이며 자신을 삭제할 수 있다.
    std::function<void(std::array<std::int32_t,4>)> clip; // 양끝 포함0,0,639,479를 현재 출력 경계에 설정한다.
    std::function<void(EvilRayCircle)> circle; // 실제 래스터 연결 전까지는 출력 인자만 제출한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // source/입자 수 진단의 정상 복귀 장치다.
};
// PATCH00453290/00453540의 독립 몸체다. 현재 시계/카메라/색 span은 호출보다 오래 살아야 한다.
class RawEvilRayFrame {
public:
    RawEvilRayFrame(const SidPool& pool,const FieryTailFrameState& state,std::span<const std::uint32_t> colors,EvilRayFrameHooks hooks); // 같은 PATCH 물리 풀을 빌리고 장치를 미리 실행하지 않는다.
    void Step(EvilRayFields& fields) const; // 최초 시계 준비→전체 delta 한 번→기한 방출/현재 꼬리→종료 또는 dirty다.
    void Draw(const EvilRayFields& fields) const; // 실제 clip→사용 꼬리의 양수 수명→세 색 타원 요청이다.
private:
    void Diagnose(std::string_view expression,std::uint32_t line) const; // 선택 진단 callable을 호출 동안 보존한다.
    std::span<const std::uint8_t> Physical(std::uint32_t sid) const; // 전체 DWORD 범위를 먼저 확인하여 SID 절삭을 막는다.
    void Invalidate(float left,float top,float right,float bottom) const; // 공통487ba0의 여유/카메라/정수화 요청이다.
    const SidPool& pool_; // source/부모의 현재 같은 raw 물리 저장소다.
    const FieryTailFrameState& state_; // 실제 현재 시각/배율/카메라이며 별도 시계를 만들지 않는다.
    std::span<const std::uint32_t> colors_; // 같은 공유3색을 그때 읽는 비소유 뷰다.
    EvilRayFrameHooks hooks_; // 도달한 원문 장치만 선택한다.
};
}
