// 건설 표시의 프레임 갱신·인접 입자 추가·팔레트 조회·Draw 제출을 복원한다.
#pragma once
#include "o/RawConstructionVisual.h"
#include <span>

namespace netstorm::o {
// 생성한 표시 상태와 RunFrame의 시각 helper가 쓰는 두 float다.
struct ConstructionFrameFields {
    ConstructionVisualFields visual; // 앞 생성 단계에서 소유권을 넘겨받은 픽셀/입자 상태다.
    float delta{},scaledDelta{}; // +0x40/+0x44: 원본 시각 차이*10 및 전역 배율을 곱한 값이다.
};
// 프레임 시각과 표시 영역의 현재 전역이다. 경계 뒤 필요한 값은 현재 자료에서 읽는다.
struct ConstructionFrameState {
    double now{}; // 패치 0055b4d0 / CD 005484a8: 현재 게임 시각이다.
    float rate{1.0F}; // 패치 0059a934 / CD 00565c4c: scaledDelta에만 곱하는 전역 배율이다.
    float cameraX{},cameraY{}; // 패치 0059a938/3c / CD 00565c50/54: dirty/Draw에서 빼는 화면 원점이다.
};
// 원본 색 표의 색수 DWORD와 읽을 수 있는 DWORD 배열이다. 토큰은 visual.palette와 같아야 한다.
struct ConstructionFramePalette {
    std::uint32_t token{}; // 앞 생성 단계의 공유 팔레트 식별자다.
    std::int32_t count{}; // +0: 수명 비율에 곱할 색수다. 배열 길이와 구별한다.
    std::span<const std::uint32_t> entries; // +4: 원본이 읽을 수 있는 색/인접 DWORD다. 범위 밖은 호스트에서 거부한다.
};
// 원본 dirty 큐에 전달하는 signed 정수 사각형이다.
struct ConstructionFrameRect {
    std::int32_t left{},top{},right{},bottom{}; // 카메라와 1픽셀 여유를 적용한 영역이다.
};
// 팔레트 출력 버퍼를 화면에 제출하는 요청이다. 경계는 이 span을 보관하지 말고 호출 중 읽어야 한다.
struct ConstructionFrameDraw {
    std::int32_t x{},y{},pitch{}; // 현재 부모 좌표/카메라/hotspot을 CRT로 변환한 위치와 원본 폭이다.
    ConstructionFrameRect clip; // 원본 입력 영역 (0,0,width,height)다.
    std::span<const std::uint8_t> pixels; // 현재 출력 버퍼다. 실제 장치 blit는 후속 경계다.
};
// 타입의 최대 HP, dirty 큐, 화면 장치, 프로세스 종료 경계다. 모두 연결한 뒤 사용한다.
struct ConstructionFrameHooks {
    std::function<std::int32_t(Sid)> maxHitPoints; // 원본 최대 HP helper다. 호출 중 raw/전역을 바꿀 수 있다.
    std::function<void(ConstructionFrameRect)> invalidate; // 이전/새 영역을 순서대로 큐에 넣는 경계다.
    std::function<void(const ConstructionFrameDraw&)> draw; // 원본 blit 인자와 현재 픽셀을 받는다.
    std::function<void(std::uint32_t)> kill; // abstract가 해제된 뒤 flags 0의 BaseProcess Kill 요청이다.
};
// 실제 RunFrame과 표시 제출 몸체다. RenderProcess 목록 등록과 장치 렌더링은 호출자가 연결한다.
class RawConstructionFrame {
public:
    // 풀/표시 전역/시각/색 표/난수는 이 객체보다 오래 살아야 한다. 누락된 경계는 생성 때 거부한다.
    RawConstructionFrame(SidPool& pool,ConstructionVisualState& visualState,ConstructionFrameState& state,
        const ConstructionFramePalette& palette,GameRandom& random,ConstructionFrameHooks hooks);
    // 00422f00 / CD 004ea7e0: 시각→HP 비율 활성화/인접 확장→만료/색→영역 갱신→abstract 종료.
    void Run(ConstructionFrameFields& fields) const;
    // 00423200 / CD 004eaa90: 용량 0은 생략하고 현재 부모/카메라로 출력 버퍼를 제출한다.
    void Draw(const ConstructionFrameFields& fields) const;
    // 00422ea0 / CD 004ea780: 내부 선형 픽셀에서 오른쪽·왼쪽·위 순서로 불투명 후보를 추가한다.
    void Expand(ConstructionFrameFields& fields,std::uint32_t pixel) const;
    // 00422e30 / CD 004ea700: 용량 안에서 입자를 추가한다. CD는 잘못된 픽셀도 total을 먼저 늘린다.
    bool Append(ConstructionFrameFields& fields,std::int32_t pixel) const;
    // 00487e50 / CD 0045d340: 판본별 나눗셈/반올림 순서로 실제 색 DWORD를 조회한다.
    std::uint32_t Color(float life,float maximum) const;
    // 00418400/30 / CD 0045e280/b0: 첫 프레임 초기화와 판본별 시각 누적을 수행한다.
    void Clock(ConstructionFrameFields& fields) const;
private:
    // 픽셀/입자 저장소의 폭과 카운터를 실제 쓰기 전에 검사한다.
    void Validate(const ConstructionFrameFields& fields) const;
    // 한 입자의 수명을 실제 게임 공유 난수에서 선택한다. float 저장 이전 값도 필요해 double로 돌려준다.
    double Life(float minimum,float maximum) const;
    // 이전/새 영역을 큐에 넣은 뒤 현재 표시 사각형을 갱신한다.
    void Rectangle(ConstructionVisualFields& fields,float left,float top,float right,float bottom) const;
    SidPool& pool_; // 현재 부모의 raw 상태다.
    ConstructionVisualState& visualState_; // 현재 좌표 배율이다.
    ConstructionFrameState& state_; // 현재 게임 시각/카메라다.
    const ConstructionFramePalette& palette_; // 현재 공유 색 표의 읽을 수 있는 자료다.
    GameRandom& random_; // 앞 건설 초기화와 공유하는 게임 난수다.
    ConstructionFrameHooks hooks_; // 아직 연결하지 않은 월드/클라이언트 경계다.
};
}
