// 입력 큐와 상태 전환 큐를 분리해 버튼 처리가 끝난 뒤 장면/미션 자원을 교체한다.
#include "client/State.h"

namespace netstorm::client {
// 이미 대기 중인 전환이 있으면 한 번의 입력 묶음이 여러 화면을 건너뛰지 않는다.
void State::Post(DialogAction action) { if (pending_.empty()) pending_.push_back(std::move(action)); }
// 명령의 문자열 자원도 다음 루프까지 소유한다.
std::optional<DialogAction> State::Take() {
    if (pending_.empty()) return {};
    auto action = std::move(pending_.front()); pending_.pop_front(); return action;
}
}
