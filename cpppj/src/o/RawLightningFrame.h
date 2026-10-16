// Lightning.cpp의 실제 시간 조각·흔들림·굵기 선과 소진 요청을 복원한다.
#pragma once
#include "o/RawLightningPath.h"
#include "o/RawFieryTailFrame.h"
#include <optional>
#include <string>

namespace netstorm::o {
// 공통 입자 타이머/화면 전역을 같은 저장 폭으로 사용한다.
using LightningFrameState=FieryTailFrameState;
// 실제 선과 dirty 요청은 같은 VFX/부분 갱신 소비자에게 전달한다.
using LightningLine=FieryTailLine;
// 실제 정수 dirty 사각형의 공통 형식이다.
using LightningDirtyRect=FieryTailDirtyRect;
// 빔 객체의 의미 필드다. 공유 팔레트/공통 등록의 생성과 소멸은 소유자가 수행한다.
struct LightningFrameFields {
    RenderProcessFields registration; // 현재 Form/부모와 이전 표시 영역이다.
    LightningParticles particles; // +24..+2c: 용량 전체와 사용 꼬리, 같은 소유 빔 토큰이다.
    double lastTime{},elapsed{}; // +30/+38: 마지막 시각과10배 시간 누적이다.
    float delta{},scaledDelta{}; // +40/+44: 현재 조각과 표시 속도 배율을 곱한 조각이다.
    bool first{}; // +48: 최초 Seed/파일명 소리 처리를 완료했는지 나타낸다.
    std::uint32_t palette{}; // +50: 월드가 소유하는 공유 색 객체 토큰이며 Draw는 이를 조회하지 않는다.
    std::int32_t firstLive{}; // +54: 용량 전체에서 처음 살아 있는 입자 위치다.
    LightningPathFields path; // +58이 가리키는 실제64BYTE 경로 이미지다.
    bool pathAllocated{}; // 원본 경로 포인터의 비영 여부다. 삭제 뒤 원문처럼 필드 값을 보존한다.
    float jitter{}; // +5c: 생성 시 거리의0.025배로 설정하는 흔들림 범위다.
    std::optional<std::string> sound; // +60: 최초 한 번 재생할 파일명이며 NULL은 무음이다.
};
// 계산 결과를 실제 외부 소비자에 전달한다. 소진 시 removeForm 뒤 즉시 반환한다.
struct LightningFrameHooks {
    std::function<void(LightningDirtyRect)> invalidate; // 매 시간 조각마다 이전/새 영역 순서로 전달한다.
    std::function<void(LightningLine)> line; // 이전 점→현재 점, 중심선→보조선 순서다.
    std::function<void(float,float,const std::string&)> sound; // raw 부모 좌표와 파일명이며 SoundHandle과 구별한다.
    std::function<void()> removeForm; // 원본41bf10(this,0)의 소유 월드 삭제 경계다.
};
// 원본00472310/004724f0의 표시/수명 본체다. 피해는 발사 시점의 별도 로직이다.
class RawLightningFrame {
public:
    // 같은 풀·경로·게임 난수·전역과 실제 출력 경계를 연결한다. 모든 참조는 이 객체보다 오래 살아야 한다.
    RawLightningFrame(const SidPool& pool,GameRandom& random,const RawLightningPath& path,
        const LightningFrameState& state,LightningFrameHooks hooks);
    // 최초 Seed/소리 또는 후속 시간 조각·경로·전체 용량 수명·흔들림·영역/소진을 수행한다.
    void Step(LightningFrameFields& fields) const;
    // 현재 firstLive 다음부터 사용 꼬리까지 살아 있는 점을 이전 점에 연결한다.
    void Draw(const LightningFrameFields& fields) const;
private:
    // 실제 시간 차이 float 저장과10배 delta/넓은 elapsed 누적을 보존한다.
    bool Clock(LightningFrameFields& fields) const;
    // 카메라/1픽셀 여유를 적용하여 실제 dirty 소비자에 전달한다.
    void Invalidate(float left,float top,float right,float bottom) const;
    // 실제00455930의 상위16비트와 넓은 간격 연산으로 현재 흔들림 난수를 반환한다.
    double Random(float low,float high) const;
    const SidPool& pool_; // 현재 raw 부모 좌표의 같은 풀이다.
    GameRandom& random_; // 경로와 다른 게임 계산에 공유하는 난수 상태다.
    const RawLightningPath& path_; // 원본 경로 생성/재귀/진행 계산이다.
    const LightningFrameState& state_; // 같은 Kernel의 시각/배율/카메라 전역이다.
    LightningFrameHooks hooks_; // 실제 출력/음향/월드 수명의 외부 경계다.
};
}
