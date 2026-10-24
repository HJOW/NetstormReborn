// 초기 HUD가 읽는 실제 전역과 Session 실행 효과의 수명을 분리한다.
#include "client/ClientGameplayGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// prvalue 직접 구성으로 CampaignWorldGlobals의 값 복사/이동을 거치지 않고 실제 두 DWORD에 연결한다.
ClientGameplayGlobals::ClientGameplayGlobals(std::shared_ptr<ClientSubscriberGlobals> subscribers)
    :subscribers_(std::move(subscribers)),world(subscribers_ ? o::CampaignWorldGlobals::BorrowSubscribers(subscribers_->MySubscriber(),subscribers_->LoginSubscriber()) : o::CampaignWorldGlobals{}) {}
// Session 모듈보다 먼저 대여하여 아직 살아 있는 전역을 초기화하지 않는다.
ClientGameplayGlobals::Use::Use(ClientGameplayGlobals& globals):globals_(globals) {
    if (globals_.borrowed_) throw std::logic_error("현재 게임 전역 중복 대여");
    globals_.borrowed_=true;globals_.player={};globals_.cursor=CursorDropState{.lockDepth=1};globals_.finder={};globals_.world.editor=0;globals_.world.missionPost=true;
}
// Player/커서의 기존 C++ 소멸 의미를 장기 전역의 명시 무효화로 바꾸며 두 Subscriber는 유지한다.
ClientGameplayGlobals::Use::~Use() { globals_.current_=nullptr;globals_.player.playerKnown=0;globals_.cursor=CursorDropState{.lockDepth=1};globals_.world.editor=0;globals_.borrowed_=false; }
// 완성 전의 부분 생성 객체를 실제 게임 효과 대상으로 노출하지 않는다.
void ClientGameplayGlobals::Use::Publish(RawCampaignSession& session) { if (globals_.current_) throw std::logic_error("현재 게임 Runtime 중복 연결");globals_.current_=&session; }
// 현재 실제 객체의 공개 메서드가 부분 반납된 Impl을 다시 읽지 않도록 한다.
void ClientGameplayGlobals::Use::Withdraw(RawCampaignSession& session) { if (globals_.current_!=&session) throw std::logic_error("현재 게임 Runtime 반납 불일치");globals_.current_=nullptr; }
// 실제 대여 객체가 모든 주소를 반납할 때까지 단일 상태를 유지한다.
std::unique_ptr<ClientGameplayGlobals::Use> ClientGameplayGlobals::Acquire() { return std::unique_ptr<Use>(new Use(*this)); }
// 미연결 게임 효과를 빈 성공/가짜 Session으로 대체하지 않는다.
RawCampaignSession& ClientGameplayGlobals::RequireSession() const { if (!current_) throw std::logic_error("선택한 생산 효과의 현재 게임 Runtime 미연결");return *current_; }
// 전역 대여 관측은 아무 게임 값을 바꾸지 않는다.
bool ClientGameplayGlobals::Borrowed() const { return borrowed_; }
// 실제 Subscriber 소유자의 정체성을 조회한다.
const std::shared_ptr<ClientSubscriberGlobals>& ClientGameplayGlobals::Subscribers() const { return subscribers_; }
}
