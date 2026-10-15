// 10.78 Tutorial의 공통 프레임·안내 정지·단계 예약을 현재 상태와 실제 UI 조회로 실행한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::o {
// 원본 Tutorial 객체의 공통 필드와 세 단계 전역을 하나의 장기 저장소로 유지한다.
// Tutorial1~6 판정기도 stage/timerSeconds를 이 인스턴스에서 직접 읽어야 한다.
struct MissionFrameGateState {
    std::int32_t stage{65}; // 005ca8e8의 현재 단계다. Advance는 저위 char를 증가시켜 다시 부호 확장한다.
    std::uint32_t stageDirty{1}; // 005ca8ec의 초기/현재 프레임 표식이다. 시작된 RunFrame의 첫 쓰기는0이다.
    std::uint32_t tutorialNumber{}; // 005ca8e4의1~6 분배 번호다. missionNumber와 다른 값이다.
    std::uint32_t demo{}; // +68의 데모 여부다. 비제로 전체를 데모로 해석한다.
    double startSeconds{}; // +70의 미션 시작 게임 시각이다. tellAtSeconds의 상대 경과 기준이다.
    double nextStageSeconds{}; // +78의 다음 단계 안내 기한이다. 최초0은 즉시 카운터 판정이다.
    float delaySeconds{0.5F}; // +80의 후속 안내 지연이다. double 현재 시각과 더한다.
    std::int32_t lockFrames{10}; // +84의 안내 잠금이다. 다이얼로그가 있으면10으로 되돌린다.
    std::uint32_t started{}; // +88의 시작 완료 값이다.0인 프레임은 아무 조회/효과도 실행하지 않는다.
    double demoEndSeconds{}; // +90의 예약된 데모 명령 기한이다.0은 예약 없음이다.
    double timerSeconds{}; // +98의 튜토리얼 임시 타이머다. 단계 변경에서0으로 지운다.
    double tellAtSeconds{}; // +a8의 tellAtList 상대 기한이다.0은 예약 없음이다.
    std::uint32_t scriptPaused{}; // +b8의 이 미션이 보유한 정지 표식이다. 실제 시계의 중첩 깊이와 구분한다.
    std::int32_t clearFrames{}; // +c8의 연속 빈 화면 수다. 하나라도 막히면0으로 되돌린다.
};

// 모든 호출은 원본의 해당 위치에서 현재 상태를 조회한다. 호출 중 상태가 바뀌어도 복사하지 않는다.
// tell/list/demo/Tutorial 본체는 명시 경계이며 미구현 소비자는 예외를 내는 훅을 공급해야 한다.
struct MissionFrameGateHooks {
    std::function<double()> gameSeconds; // 0055b4d0의 현재 double 게임 시각이다. 벽시각을 공급하지 않는다.
    std::function<bool()> paused; // 00460de0의 현재 중첩 정지 여부다.
    std::function<void()> checkOutcome; // 004c36c0의 실제 승패/사제 사건을 첫 순서로 실행한다.
    std::function<bool()> dialogsClear; // 004c8e90의 nonzero 반환이다. 다이얼로그가 없으면 참이다.
    std::function<bool()> knowledgeOpen; // 00445330의 현재 지식 창 여부다. tellAtList 실행을 막는다.
    std::function<bool()> guideOpen; // 004c79a0의 현재 안내 창 여부다.7프레임 재개를 막는다.
    std::function<bool()> blockingGumpOpen; // 004641d0의0x4000 자식 창 여부다.7프레임 재개를 막는다.
    std::function<void()> tellStage; // 004c2a90의 현재 단계/Demo Tell이다. 성공한 실제 창은 PauseForDialog를 호출한다.
    std::function<void()> tellAt; // 004c3470의 현재 tellAtList 항목 실행/다음 예약이다.
    std::function<void()> endDemo; // 00452f90의 현재 전역 데모 명령을 인자0/0으로 요청한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> tutorial; // 실제1~6 판정의 번호/현재 사건과 반환값이다.
    std::function<void()> pause; // 00460df0의 실제 시계 정지 깊이를 한 번 증가시킨다.
    std::function<void()> resume; // 00460e30의 실제 시계 정지 깊이를 한 번 감소시킨다.
};

// PATCH004c34c0의 공통 분기를 보존한다. 초나24Hz 틱이 아닌 실제 입력/화면 호출마다 한 번 사용한다.
class RawMissionFrameGate {
public:
    // 상태/훅 대상은 처리기보다 오래 살아야 한다. 선택되지 않은 큰 본체도 명시 경계로 공급한다.
    RawMissionFrameGate(MissionFrameGateState& state,MissionFrameGateHooks hooks);
    // started 이후 outcome→10프레임→tellAt→7프레임→demo→Tutorial 순서를 실행한다.
    std::uint32_t RunFrame(std::uint32_t event=0) const;
    // PATCH004c33f0: 실행 중이며 잠금<1일 때만 char 단계 증가/10프레임/지연/타이머0을 예약한다.
    bool Advance() const;
    // PATCH004c2a40: 현재 시계 정지와 자기 소유 표식을 함께 읽어 중복 정지를 피한다.
    void PauseForDialog() const;
    // PATCH004c3170: 이미 비제로인 기한만 현재 게임 시각+float 지연으로 다시 예약한다.
    void Postpone() const;
    // 단계 판정기와 상위 미션이 별도 복사 없이 동일 저장소를 읽고 쓴다.
    MissionFrameGateState& State() const;
private:
    MissionFrameGateState& state_; // 원본 필드/단계/타이머의 단일 저장소다.
    MissionFrameGateHooks hooks_; // 실제 시계/UI/미션 본체의 현재 조회와 효과다.
};
}
