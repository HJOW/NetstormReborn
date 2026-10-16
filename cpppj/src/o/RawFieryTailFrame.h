// Fierytail.cpp의 실제 입자 프레임과 선 출력 요청을 복원한다.
#pragma once
#include "o/RawConstructionLifetime.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 원본24바이트 입자다. Fiery Tail이 쓰지 않는 속도/마지막 DWORD도 보존한다.
struct FieryTailParticle {
    float x{},y{}; // 월드 배율을 적용한 입자 픽셀 좌표다.
    float velocityX{},velocityY{}; // 공통 입자 저장소의 속도이며 이 프레임에서는 바꾸지 않는다.
    float life{}; // 양수일 때만 감소/표시하는 남은 수명이다.
    std::uint32_t spare{}; // 공통 입자의 마지막 DWORD이며 그대로 유지한다.
};
// 등록/프레임 필드의 소유 저장소다. 생성·공통 등록은 이 몸체의 호출자가 수행한다.
struct FieryTailFrameFields {
    RenderProcessFields registration; // 부모 SID와 현재 표시 영역이다.
    std::vector<FieryTailParticle> particles; // 원본 capacity 전체 배열이며 정식 생성자는120개를 지운다.
    std::int32_t total{}; // 실제 사용한 꼬리 수다. 죽은 입자를 재사용해도 줄지 않는다.
    double lastTime{},elapsed{}; // 원본 +30/+38의 마지막 시각과 누적 delta다.
    float delta{},scaledDelta{}; // 원본 +40/+44의10배 시간 간격과 배율 적용 값이다.
    bool first{}; // 원본 +48: 최초10입자 생성을 완료했는지 나타낸다.
    std::uint32_t palette{}; // +50: 월드가 소유하는 공유 색 표 토큰이며 효과 소멸 때 반납하지 않는다.
    std::int32_t maximumLife{4}; // +54: 생성 입자의 수명과 색 표 분모다.
    float previousX{},previousY{}; // +58/+5c: 직전 부모의 픽셀 좌표다.
    std::int32_t offset{}; // +60: 후속 방출 좌표와 선의 끝에 더하는 정수다.
};
// 원본 색수와 읽을 수 있는 메모리를 분리한다. 인접 DWORD는 근거 있는 외부 입력이어야 한다.
struct FieryTailFramePalette {
    std::int32_t count{}; // 원본 색수이며 진단 분기는 이 값을 변경한다.
    std::span<const std::uint32_t> readable; // 색수 다음을 포함해 실제 공급된 DWORD만 읽을 수 있다.
};
// 원본 프레임/Draw가 현재 읽는 전역과 화면 좌표다.
struct FieryTailFrameState {
    double now{}; // 같은 Kernel의 현재 게임 시각이다.
    float pixelScaleX{16},pixelScaleY{11},rate{1}; // 부모 raw 좌표의 배율과 scaledDelta 배율이다.
    float cameraX{},cameraY{}; // 입자 좌표와 dirty 영역에서 빼는 카메라 픽셀 좌표다.
    std::int32_t diagnostic{}; // 원본005318ec의 현재 값이며 의미를 임의로 재해석하지 않는다.
};
// 원본 SetRect가 dirty 소비자에 넘기는 정수 사각형이다.
struct FieryTailDirtyRect {
    std::int32_t left{},top{},right{},bottom{}; // 두 끝에1픽셀 여유와 카메라를 적용한 영역이다.
};
// 원본004dade0에 전달하는 선 요청이다. 실제 래스터화는 출력 소비자가 수행한다.
struct FieryTailLine {
    std::int32_t x1{},y1{},x2{},y2{}; // 입자와 부모의 현재 화면 좌표다.
    std::uint32_t color{}; // 원본 팔레트가 반환한 전체 DWORD다.
};
// 실제 dirty/선 출력의 외부 경계이며 빈 성공으로 대체하지 않는다.
struct FieryTailFrameHooks {
    std::function<void(FieryTailDirtyRect)> invalidate; // 이전/새 영역을 매 시간 조각마다 순서대로 받는다.
    std::function<void(FieryTailLine)> line; // 실제 선 출력 인자를 순서대로 받는다.
};
// PATCH004580f0/004583a0 몸체다. 생성/Host 수명과 장치 출력은 별도 연결이 필요하다.
class RawFieryTailFrame {
public:
    // 모든 참조와 색 표의 읽기 span은 이 객체보다 오래 살아야 하며 PATCH 판본만 지원한다.
    RawFieryTailFrame(const SidPool& pool,GameRandom& random,const FieryTailFrameState& state,
        FieryTailFramePalette& palette,FieryTailFrameHooks hooks);
    // 최초10입자 또는 실제 타이머 조각별2입자/전체 용량 감소/dirty를 수행한다.
    void Step(FieryTailFrameFields& fields) const;
    // 원본에 전달한 부모 화면 끝점에서 각 살아 있는 입자까지 선을 제출한다.
    // 최대 수명이 읽는 색수 다음 DWORD도 공급돼야 한다. 진단2는 색수/최대 수명을 원본처럼 변경한다.
    void Draw(FieryTailFrameFields& fields,std::int32_t endpointX,std::int32_t endpointY) const;
private:
    // 사용 꼬리 또는 첫 수명<=0 슬롯을 선택한다. NaN은 재사용하지 않는다.
    FieryTailParticle* Append(FieryTailFrameFields& fields) const;
    // Y→X 난수 소비 뒤 빈 슬롯이 있을 때만 좌표/수명을 쓴다.
    void Emit(FieryTailFrameFields& fields,float x,float y,bool initial) const;
    // 실제 float 저장 위치와 lastTime 갱신을 보존하며 마지막 시간 조각 여부를 반환한다.
    bool Clock(FieryTailFrameFields& fields) const;
    // 원본00487e50의 색수/float 역수/__ftol 조회다. 공급된 메모리 밖 읽기는 거부한다.
    std::uint32_t Color(float life,float maximum) const;
    // 원본 SetRect의 카메라 감산과1픽셀 여유를 정수화한 뒤 실제 소비자에 전달한다.
    void Invalidate(float left,float top,float right,float bottom) const;
    const SidPool& pool_; // 실제 부모 raw 슬롯과 판본이다.
    GameRandom& random_; // 다른 게임 로직과 공유하는 원본 난수 상태다.
    const FieryTailFrameState& state_; // 현재 게임 시각과 화면 전역이다.
    FieryTailFramePalette& palette_; // 같은 효과들이 공유하는 색수와 읽기 메모리다.
    FieryTailFrameHooks hooks_; // dirty와 선의 실제 출력 소비자다.
};
}
