// Flyingshrapnel.cpp의 픽셀 묶음 생성·중력/수명·현재 색 픽셀을 복원한다.
#pragma once
#include "o/RawFieryTailFrame.h"
#include "o/CrtRandom.h"

namespace netstorm::o {
// 원본 세 가상 픽셀 생성기다. 셋째의 의미 이름은 추측하지 않는다.
enum class ShrapnelKind { Explosion,Collapse,Variant2 };
// 원본24BYTE 입자의 색 주소는 같은 캡처 bitmap의 offset+1 토큰이다.
struct ShrapnelParticle {
    float x{},y{}; // 현재 월드 픽셀 좌표다.
    float velocityX{},velocityY{},life{}; // 실제 속도/10배 시간 수명이다.
    std::uint32_t pixel{}; // 0은 미사용이며 나머지는 캡처 픽셀 offset+1이다.
};
// 실제104BYTE 객체의 의미 필드다. 그림과 입자 저장소의 수명은 소유자가 관리한다.
struct ShrapnelFrameFields {
    RenderProcessFields registration; // 같은 공통 목록/Form/부모와 표시 영역이다.
    std::vector<ShrapnelParticle> particles; // 실제 용량 전체이며 미사용0 입자도 보존한다.
    std::int32_t total{}; // 실제로 사용한 꼬리 수다. 죽은 슬롯을 재사용하지 않는다.
    double lastTime{},elapsed{}; // +30/+38: 마지막 시각과 누적10배 시간이다.
    float delta{},scaledDelta{}; // +40/+44: 한 프레임 전체 간격과 표시 배율이다.
    bool first{}; // +48: 공통 생성자가0으로 쓰지만 이 프레임은 검사하지 않는다.
    ConstructionVisualImage image; // +50..+60: 원래 색을 유지하는 캡처/치수/기준점이다.
    float gravity{6}; // +64: 폭발6·붕괴float0.24·셋째float1.2다.
};
// 공통 입자 전역과 같은 시각/배율/카메라 형식이다.
using ShrapnelFrameState=FieryTailFrameState;
// 실제004a2230에 전달할 정수 픽셀 요청이다.
struct ShrapnelPixel {
    std::int32_t x{},y{}; // 먼저 입자 좌표 절삭, 그 뒤 카메라를 뺀 현재 화면 좌표다.
    std::uint32_t color{}; // 캡처 bitmap에서 읽은 byte를 zero extend한 값이다.
};
// Native new/free·dirty·픽셀·소진의 실제 외부 소비자다.
struct ShrapnelFrameHooks {
    std::function<bool(std::uint32_t)> reserve; // 입자 count×24 또는 bitmap 복사 크기다.
    std::function<void(bool)> release; // true는 작업 복사본, false는 이전 입자 저장소다.
    std::function<void(FieryTailDirtyRect)> invalidate; // 원본 이전/새 영역의 순서다.
    std::function<void(ShrapnelPixel)> pixel; // 실제 clip/pitch를 가진 표시 장치에 전달한다.
    std::function<void()> removeForm; // 소진 시 실제41bf10(this,0)으로 연결한다.
};
// 실제00460000/0045fc70과 세 가상 묶음,0045fd60/0045fef0의 계산 본체다.
class RawShrapnelFrame {
public:
    // 두 난수/풀/전역/경계는 이 객체보다 오래 살아야 한다. 두 난수 스트림을 합치지 않는다.
    RawShrapnelFrame(const SidPool& pool,GameRandom& game,CrtRandom& crt,const ShrapnelFrameState& state,ShrapnelFrameHooks hooks);
    // 부모 raw 위치−캡처 기준점 절삭→불투명 수의 용량→작업 복사본/묶음 생성→타이머 초기화다.
    void Prepare(ShrapnelFrameFields& fields,ShrapnelKind kind) const;
    // 간격 제한 없이 실제 한 시간 조각과 용량 전체를 갱신한다. 진입 양수 입자가 있어야 다음 프레임까지 남는다.
    void Step(ShrapnelFrameFields& fields) const;
    // 사용 꼬리의 양수 수명만 현재 캡처 색으로 출력한다. source bitmap을 수정하지 않는다.
    void Draw(const ShrapnelFrameFields& fields) const;
private:
    // 실제455930의 상위16비트/넓은 간격이다. 끝점 역순도 원문처럼 low에 절댓값 간격을 더한다.
    double Random(float low,float high) const;
    // 현재 픽셀의 가상 속도/기본 수명을 계산하고 묶음을 만든다.
    void Seed(ShrapnelFrameFields& fields,ShrapnelKind kind,std::span<std::uint8_t> work,std::int32_t originX,std::int32_t originY,std::int32_t x,std::int32_t y) const;
    // 각 반복 조건마다 CRT 난수를 다시 뽑는004600a0이다.
    void Split(ShrapnelFrameFields& fields,std::span<std::uint8_t> work,std::int32_t originX,std::int32_t originY,std::int32_t x,std::int32_t y,float vx,float vy,float life) const;
    // 실제 dirty의 여유/카메라/정수화 순서를 사용한다.
    void Invalidate(float left,float top,float right,float bottom) const;
    const SidPool& pool_; // 현재 효과 부모의 raw 좌표/깊이다.
    GameRandom& game_; // 폭발 속도의 게임 난수 스트림이다.
    CrtRandom& crt_; // 묶음 크기/수명에 쓰는 원본 CRT 스트림이다.
    const ShrapnelFrameState& state_; // 같은 Kernel 시각/카메라/배율이다.
    ShrapnelFrameHooks hooks_; // 실제 메모리/표시/소진 경계다.
};
}
