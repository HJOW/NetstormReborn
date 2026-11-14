// 테스트 전용 관전 카메라 판단기의 구현이다. 피해 사건 수집·전투 지점 선택·의식 우선·화면 유지 규칙을 담는다.
// (2026-11-14 사용자 요청으로 신규 추가)
#include "client/CampaignTestSpectator.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace netstorm::client { // 관전 판단기 구현이다.
namespace { // 이 파일에서만 쓰는 전역과 계산 도우미다.
// 현재 미션의 판단기와 그 월드 식별 주소다. UberGump가 미션 설치 때 바꾸고 프레임 처리에서 읽는다.
struct Current { std::shared_ptr<CampaignTestSpectator> spectator;const void* world{}; };
Current& Storage() { static Current current; return current; }
// 두 월드 좌표 사이의 제곱 거리다.
double DistanceSquared(float ax,float ay,float bx,float by) { const double dx=static_cast<double>(ax)-bx,dy=static_cast<double>(ay)-by;return dx*dx+dy*dy; }
// 파괴된 객체 하나가 기여하는 피해 상한이다. 큰 건물 하나가 전투 판단을 독점하지 않게 한다.
constexpr double kDestroyedWeightCap=60.0;
}

CampaignTestSpectator::CampaignTestSpectator(TestSpectatorSettings settings):settings_(std::move(settings)) {}
// 같은 의식은 몇 초마다 소리를 내므로 마지막 위치와 시각만 유지한다.
void CampaignTestSpectator::NoteRitual(float x,float y,double now) { ritual_=std::make_pair(x,y);ritualTime_=now; }
// 표본 간격을 게임 시각으로 판단한다(정지 중에는 시각이 멈춰 표본도 멈춘다).
bool CampaignTestSpectator::Due(double now) const { return now>=nextSample_; }
const std::optional<TestSpectatorFocus>& CampaignTestSpectator::Current() const { return current_; }
const TestSpectatorSettings& CampaignTestSpectator::Settings() const { return settings_; }

// HP 감소·사라진 소유 객체를 피해 사건으로 모으고, 의식 → 전투 순서로 목표를 정한다.
std::optional<TestSpectatorFocus> CampaignTestSpectator::Observe(std::span<const TestSpectatorSample> samples,double now) {
    nextSample_=now+settings_.sampleSeconds;
    std::unordered_map<std::uint32_t,Known> seen; // 이번 표본의 SID별 상태다.
    seen.reserve(samples.size());
    // 같은 SID·TYPE의 HP가 줄었으면 그만큼을 그 위치의 피해로 기록하고, 그 객체를 피해 입은 객체로 표시한다.
    for (const auto& sample:samples) {
        auto& current=seen[sample.sid];current={sample.type,sample.owner,sample.x,sample.y,sample.hitPoints,false};
        if (!primed_) continue;
        const auto previous=known_.find(sample.sid);
        if (previous==known_.end() || previous->second.type!=sample.type) continue;
        current.damaged=previous->second.damaged;
        if (sample.hitPoints<previous->second.hitPoints) { events_.push_back({sample.x,sample.y,static_cast<double>(previous->second.hitPoints-sample.hitPoints),now});current.damaged=true; }
    }
    // 피해를 입은 적 있는 소유 객체가 사라졌으면 마지막 위치에서 파괴된 것으로 본다. 피해 없이 사라진 것(건설 완료 교체·내용물 진입 등)은 세지 않는다.
    // (2026-11-14 보정: 실제 1-1 실행에서 피해 없는 사라짐이 전투로 잡혀 기지로 카메라가 돌아가던 문제)
    for (const auto& [sid,previous]:known_) {
        if (!primed_ || seen.contains(sid) || previous.hitPoints<=0 || !previous.damaged) continue;
        events_.push_back({previous.x,previous.y,std::min(kDestroyedWeightCap,static_cast<double>(previous.hitPoints)),now});
    }
    known_=std::move(seen);primed_=true;
    std::erase_if(events_,[&](const Event& event) { return now-event.time>settings_.windowSeconds; }); // 시간 폭 밖의 사건을 버린다.
    // 의식 소리가 최근에 났으면 제단을 최우선으로 보며 유지 시간을 기다리지 않는다.
    if (ritual_ && now-ritualTime_<=settings_.ritualHoldSeconds) {
        const auto [x,y]=*ritual_;
        if (current_ && current_->ritual && DistanceSquared(current_->x,current_->y,x,y)<1.0) return std::nullopt; // 이미 그 제단을 보고 있다.
        current_=TestSpectatorFocus{x,y,true,0.0};movedAt_=now;return current_;
    }
    if (events_.empty()) return std::nullopt;
    const double radius=static_cast<double>(settings_.clusterCells)*settings_.clusterCells;
    std::size_t best{};double bestScore=-1.0; // 주변 피해 합이 가장 큰 사건이다.
    // 각 사건을 중심으로 반경 안 피해 합을 구해 가장 격렬한 곳을 찾는다(사건 수가 적어 제곱 시간이면 충분하다).
    for (std::size_t index=0;index<events_.size();++index) {
        double score{};
        // 같은 반경의 모든 사건 피해를 더한다.
        for (const auto& other:events_) if (DistanceSquared(events_[index].x,events_[index].y,other.x,other.y)<=radius) score+=other.weight;
        if (score>bestScore) { bestScore=score;best=index; }
    }
    if (bestScore<settings_.minimumWeight) return std::nullopt; // 작은 소모는 전투로 보지 않는다.
    double sumX{},sumY{},sumWeight{}; // 가장 격렬한 곳 주변 사건의 가중 중심이다.
    // 같은 반경의 사건을 피해 크기로 가중 평균한다.
    for (const auto& other:events_) if (DistanceSquared(events_[best].x,events_[best].y,other.x,other.y)<=radius) { sumX+=other.x*other.weight;sumY+=other.y*other.weight;sumWeight+=other.weight; }
    const TestSpectatorFocus focus{static_cast<float>(sumX/sumWeight),static_cast<float>(sumY/sumWeight),false,bestScore};
    const bool fromRitual=current_ && current_->ritual; // 의식이 끝난 뒤에는 바로 전투로 돌아간다.
    if (current_ && !fromRitual) {
        if (now-movedAt_<settings_.dwellSeconds) return std::nullopt; // 최소 유지 시간 전이다.
        const double near=static_cast<double>(settings_.moveCells)*settings_.moveCells;
        if (DistanceSquared(current_->x,current_->y,focus.x,focus.y)<near) return std::nullopt; // 이미 같은 전투를 보고 있다.
    }
    current_=focus;movedAt_=now;return current_;
}

void SetCurrentTestSpectator(std::shared_ptr<CampaignTestSpectator> spectator,const void* world) { Storage()={std::move(spectator),world}; }
CampaignTestSpectator* CurrentTestSpectator(const void* world) { const auto& current=Storage();return current.spectator && world && current.world==world ? current.spectator.get() : nullptr; }
}
