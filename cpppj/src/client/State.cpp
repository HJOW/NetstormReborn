// 입력 큐와 상태 전환 큐를 분리해 버튼 처리가 끝난 뒤 장면/미션 자원을 교체한다.
#include "client/State.h"
#include <stdexcept>

namespace netstorm::client {
// 이미 대기 중인 전환이 있으면 한 번의 입력 묶음이 여러 화면을 건너뛰지 않는다.
void State::Post(DialogAction action) { if (pending_.empty()) { pending_.push_back(std::move(action));if (++pendingSequence_==0) ++pendingSequence_; } } // 빈 큐의 새 항목만 구별하며 unsigned 감김에서도0은 빈 입력으로 남긴다.
// 명령의 문자열 자원도 다음 루프까지 소유한다.
std::optional<DialogAction> State::Take() {
    if (pending_.empty()) return {};
    auto action = std::move(pending_.front()); pending_.pop_front(); return action;
}
// 실제 전환 큐의 같은 저장소를 읽으며 조회만으로 전환을 꺼내지 않는다.
bool State::Pending() const { return !pending_.empty(); }
// Native Poll이 예약한 다음 상태를 같은 Tick에서 두 번째로 실행하지 않고 취소/부분 실패도 보존한다.
std::optional<DialogAction> State::TakeAfterPoll(const std::function<void()>& poll) {
    if (!poll) throw std::invalid_argument("호스트 Tick의 실제 Poll 장치 누락");
    const auto previous=pending_.empty() ? 0U : pendingSequence_;poll();
    if (!previous || pending_.empty() || previous!=pendingSequence_) return {};return Take();
}
// 미션이 교체되기 전에 그 미션의 지연 요새 요청을 반납해 새 미션에 오래된 지도를 설치하지 않는다.
bool State::Cancel(std::string_view command) { if (pending_.empty() || pending_.front().command!=command) return false;pending_.pop_front();return true; }
}
