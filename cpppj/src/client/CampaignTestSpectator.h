// 테스트 전용 관전 카메라 판단기다. 자동 플레이(--test-*) 중 사람이 지켜볼 수 있도록 주요 전투 위치, 사제 희생 의식 중에는 제단 위치를 카메라 목표로 고른다.
// 게임 상태를 읽기만 하며(객체 HP 감소·파괴, 의식 위치 소리) 게임 계산·난수·입력에는 관여하지 않는다. 원본 10.78에 없는 개발/검증용 기능이다.
// 사용법: 미션마다 새로 만들어 SetCurrentTestSpectator로 지정하고, 의식 소리마다 NoteRitual, 매 프레임 Due가 참이면 Observe를 불러 돌려받은 좌표로 카메라를 옮긴다.
// (2026-11-14 사용자 요청: 자동 플레이를 사람이 볼 수 있게 전투/희생 의식 위치로 화면 이동, 신규 추가)
#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace netstorm::client { // 클라이언트 연결층의 테스트 전용 관찰 기능이다.
// 관전 카메라 판단에 쓰는 시간·거리 기준이다. 시간은 게임 시각(초), 거리는 월드 칸 단위다(2026-11-14 신규).
struct TestSpectatorSettings {
    double sampleSeconds{0.5}; // 객체 표본을 읽는 간격이다.
    double windowSeconds{6.0}; // 최근 전투 사건으로 인정하는 시간 폭이다.
    double dwellSeconds{4.0}; // 전투 지점으로 옮긴 뒤 다음 전투 지점으로 옮기기 전 최소 유지 시간이다.
    double ritualHoldSeconds{5.0}; // 마지막 의식 소리 뒤 제단을 계속 우선하는 시간이다.
    float clusterCells{8.0F}; // 같은 전투로 묶는 반경이다.
    float moveCells{6.0F}; // 이보다 가까운 새 전투 지점으로는 옮기지 않는다(화면 흔들림 방지).
    double minimumWeight{20.0}; // 전투로 인정하는 최소 피해 합(HP)이다. 작은 소모는 무시한다.
    std::vector<std::uint32_t> ignoredTypes; // 표본에서 뺄 TYPE(섬·다리처럼 붕괴/소모로 HP가 줄어 전투와 무관한 것)이다. 호출자가 표본을 모을 때 쓴다.
};
// 한 시점의 소유 객체 표본이다. 호출자가 지형·다리처럼 전투와 무관한 TYPE과 중립(owner 0)을 미리 빼서 넘긴다(2026-11-14 신규).
struct TestSpectatorSample {
    std::uint32_t sid{},type{},owner{}; // 풀 SID·TYPE·소유자다.
    float x{},y{}; // 월드 좌표(칸)다.
    std::int32_t hitPoints{}; // 현재 HP다.
};
// 카메라 목표와 고른 이유다(2026-11-14 신규).
struct TestSpectatorFocus {
    float x{},y{}; // 카메라 중심으로 쓸 월드 좌표다.
    bool ritual{}; // 희생 의식(제단) 때문이면 참, 전투 때문이면 거짓이다.
    double weight{}; // 전투 지점의 최근 피해 합이다(의식이면 0).
};

// 최근 피해 위치를 모아 가장 격렬한 전투 지점을 고르고, 의식 중에는 제단을 우선하는 판단기다(2026-11-14 신규).
// 피해는 HP 감소량, 그리고 피해를 입은 적 있는 객체가 사라진 경우(파괴, 하나당 최대 60)다.
class CampaignTestSpectator {
public:
    explicit CampaignTestSpectator(TestSpectatorSettings settings={}); // 판단 기준을 정한다. 첫 Observe는 기준 HP만 기록한다.
    void NoteRitual(float x,float y,double now); // 희생 의식(사슬·몸부림) 위치 소리를 기록한다.
    bool Due(double now) const; // 다음 표본을 읽을 시각이 되었는지 돌려준다.
    // 표본을 반영하고, 카메라를 옮겨야 할 때만 새 목표를 돌려준다. 이미 같은 목표를 보고 있거나 유지 시간 전이면 비어 있다.
    std::optional<TestSpectatorFocus> Observe(std::span<const TestSpectatorSample> samples,double now);
    const std::optional<TestSpectatorFocus>& Current() const; // 마지막으로 돌려준 목표다(진단용).
    const TestSpectatorSettings& Settings() const; // 판단 기준이다(표본 수집의 제외 TYPE 확인용).
private:
    struct Event { float x,y;double weight,time; }; // 피해 사건 하나다.
    struct Known { std::uint32_t type,owner;float x,y;std::int32_t hitPoints;bool damaged; }; // 직전 표본의 객체 상태다. damaged는 관찰 중 HP가 준 적이 있는지다.
    TestSpectatorSettings settings_; // 판단 기준이다.
    std::unordered_map<std::uint32_t,Known> known_; // 직전 표본의 SID별 상태다.
    bool primed_{}; // 첫 표본(기준 HP)을 기록했는지다.
    std::vector<Event> events_; // 시간 폭 안의 피해 사건이다.
    double nextSample_{}; // 다음 표본 시각이다.
    std::optional<std::pair<float,float>> ritual_; // 마지막 의식 위치다.
    double ritualTime_{-1.0}; // 마지막 의식 소리 시각이다.
    std::optional<TestSpectatorFocus> current_; // 마지막으로 돌려준 목표다.
    double movedAt_{-1.0e9}; // 마지막으로 목표를 바꾼 시각이다.
};

// 현재 미션의 관전 판단기를 그 미션의 월드(world, 식별용 주소)와 함께 프로세스 전역에 지정한다. nullptr이면 관전 카메라를 끈다(2026-11-14 신규).
void SetCurrentTestSpectator(std::shared_ptr<CampaignTestSpectator> spectator,const void* world);
// world가 지정한 미션의 월드와 같을 때만 판단기를 돌려준다. 테스트 옵션이 없거나 다른 월드(LAN·편집 등)면 nullptr이다(2026-11-14 신규).
CampaignTestSpectator* CurrentTestSpectator(const void* world);
}
