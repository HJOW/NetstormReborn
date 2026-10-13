// Burning.cpp의 캡처와 실제 프레임을 기존 생성자·연소점·Append 몸체에 연결한다.
#pragma once
#include "o/RawBurningSources.h"
#include "o/SquidDisplay.h"

namespace netstorm::o {
// 생성자가 초기화하지 않는 두 float는 프레임 helper가 처음 쓸 때만 갱신한다.
struct BurningFrameFields {
    float delta{},scaledDelta{}; // 원본 +40/+44이며 기존 생성자의 완료 필드에 포함하지 않는다.
};
// 실제 프레임이 매호출 읽는 게임 시각·픽셀 배율·바람·카메라와 viewport 전역이다.
struct BurningFrameState {
    double now{}; // 패치0055b4d0/CD005484a8의 현재 게임 시각이다.
    float pixelScaleX{1},pixelScaleY{1}; // 패치0059a924/28·CD00565c3c/40의 두 월드→픽셀 배율이다.
    float cameraX{},cameraY{}; // 패치0059a938/3c·CD00565c50/54의 현재 픽셀 카메라다.
    float wind{},deltaMultiplier{}; // 실제 입자 바람과 timer +44의 현재 배율이다.
    SquidDisplayRect viewport; // 양판본 실제 clipping/visible helper가 읽는 현재 경계다.
    bool insetViewport{}; // 현재 모드 전역이 켜지면 visible 경계를 양쪽100픽셀씩 줄인다.
};
// 캡처는 실제 client ShapeToBuffer에 연결한다. nonzero 소리 정지는 별도 실제 소비자가 필요하다.
struct BurningFrameHooks {
    std::function<ConstructionVisualImage(Sid)> capture; // 현재 victim의 실제 보호 SHP/plain RLE 캡처를 반환한다.
    std::function<void(std::uint32_t)> stopSound; // 실제 소리 핸들0은 원본 helper처럼 바로 반환한다.
};
// 00424530/CD004a60e0의 단계·visible·timer·source·이동입자·dirty 영역 몸체다.
class RawBurningFrame {
public:
    // 모든 처리기는 같은 SID 풀을 사용하며 state/hooks/sink는 이 객체보다 오래 살아야 한다.
    RawBurningFrame(const SidPool& pool,std::span<const RiftTypeRecord> types,
        const RawBurningVisual& visual,const RawBurningSources& sources,GameRandom& random,
        const BurningFrameState& state,SquidDisplaySink& sink,BurningFrameHooks hooks);
    // 실제423d50/CD4a5c70: 기존 이미지/행 반납→입자 zero→현재 그림 캡처→새 행 zero다.
    void Capture(BurningVisualFields& fields) const;
    // 실제 capacity0 반환과 단계0/1을 보존하고 visible 프레임에서만 시각/입자를 갱신한다.
    void Step(BurningVisualFields& fields,BurningFrameFields& frame) const;
    // 실제 효과 Host나 현재 피해 월드가 같은 풀을 사용하도록 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 raw TYPE의 일반 HP 표식과 Dais의 extra bit1을 원본 비가상 helper로 읽는다.
    bool HasHitPoints(std::span<const std::uint8_t> raw) const;
    // 원본 float 난수의 넓은 반환값을 유지하며 같은 게임 전역을 한 번 전진한다.
    double Random(float minimum,float maximum) const;
    // 실제 현재 viewport에 대한 두 차례0 방향 정수화/카메라 빼기/겹침 검사다.
    bool Visible(const RenderProcessFields& registration) const;
    // 원본 SetRect처럼 이전/새 영역을 각각 clipping한 뒤 sink에 순서대로 전달한다.
    void Invalidate(float left,float top,float right,float bottom) const;
    const SidPool& pool_; // 현재 판본과 victim raw 슬롯의 동일 저장소다.
    std::span<const RiftTypeRecord> types_; // 실제 HasHP가 읽는 현재 타입 표다.
    const RawBurningVisual& visual_; // 실제 기존 입자 Append 몸체다.
    const RawBurningSources& sources_; // 현재HP/캡처에서 source를 선택하는 실제 몸체다.
    GameRandom& random_; // source 정수난수와 float 난수가 함께 소비하는 원본 전역이다.
    const BurningFrameState& state_; // 호출자가 현재 시각/카메라/바람을 갱신한다.
    SquidDisplaySink& sink_; // 기존 실제 dirty 표/Renderer를 소비하는 현재 출력 대상이다.
    BurningFrameHooks hooks_; // 실제 프레임 캡처와 nonzero 소리 하위 소비자다.
};
}
