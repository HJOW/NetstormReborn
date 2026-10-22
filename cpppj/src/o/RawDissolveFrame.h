// 애시드 포획의120바이트 Dissolve 효과를 원본 입자/시계/픽셀 순서로 복원한다.
#pragma once
#include "o/RawShrapnelFrame.h"

namespace netstorm::o {
// 원본24바이트 입자/캡처/공통 등록은 파편과 같은 배치이며 Dissolve 고유 꼬리를 구분한다.
struct DissolveFrameFields {
    ShrapnelFrameFields common; //+00..60의 같은 등록/입자/시계/bitmap 필드다. 중력은 사용하지 않는다.
    std::int32_t opaqueCount{}; //+70의 투명색255를 제외한 실제 원본 픽셀 수다.
    bool clockStarted{}; //+74의 첫 Frame에서만418400을 호출하는 상태다.
    bool particlesAllocated{}; //new0도 비영 포인터를 반납하므로 용량0과 실제 확보 여부를 구분한다.
};
// 모든 참조와 실제 출력/메모리 경계는 이 계산기보다 오래 살아야 한다.
class RawDissolveFrame {
public:
    RawDissolveFrame(const SidPool& pool,GameRandom& random,const ShrapnelFrameState& state,ShrapnelFrameHooks hooks); //같은10.78/16·11 좌표계다.
    void Initialize(DissolveFrameFields& fields,Sid first,Sid second,std::int32_t xOriented) const; //450460의 캡처 이후 부모 위치/깊이/원점과4500a0을 적용한다.
    void Prepare(DissolveFrameFields& fields,std::int32_t originX,std::int32_t originY,Sid first,Sid second,std::int32_t xOriented) const; //4500a0의 열→행 순서·두 게임 난수·속도/수명이다.
    void Step(DissolveFrameFields& fields) const; //44fe40의 최초 시계·무중력 이동·원본 dirty·소진이다.
    void Draw(const DissolveFrameFields& fields) const; //44ffc0의 현재 bitmap 색/사용 입자/정수 카메라 순서다.
private:
    std::array<float,2> Position(Sid sid) const; //원본 부모와 두 폴의 현재 raw 좌표를 읽는다.
    double Random() const; //455930(1,1.3)의 넓은 반환값과 게임 난수 한 번이다.
    std::int32_t Jitter(std::int32_t width) const; //4558f0(-width,width)의 반열린 범위를 소비한다.
    void Invalidate(float left,float top,float right,float bottom) const; //487ba0의 원본1픽셀 여유/카메라다.
    const SidPool& pool_; //현재 부모 더미와 폴이 공유하는 raw 저장소다.
    GameRandom& random_; //다른 생산/공격과 같은 게임 난수 스트림이다.
    const ShrapnelFrameState& state_; //같은 Kernel 시각/배율/카메라다.
    ShrapnelFrameHooks hooks_; //실제 입자 할당·dirty·픽셀·Form 삭제 경계다.
};
}
