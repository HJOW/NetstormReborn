// 실제 Player152 가상+5c의 두 소어 루저 검사 사건을 현재 두owner/실제 Regular Host에 연결한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include "o/SquidOwner.h"
#include <string_view>

namespace netstorm::o {
// 알 수 없는 사건에서 진단 후 payload를 반환하는 원본 분류다.
enum class PlayerRegularAssertion {
    UnknownEvent // local/count!=0인데 event가1/2가아니면PATCH41d/CD3c9줄을보고한다.
};
// 실제 로그/새예약/Template State·변경통지를 필수 하위 효과로 연결한다.
struct PlayerRegularHooks {
    std::function<void(PlayerRegularAssertion)> assertion; // 진단 후 현재 payload 반환을 계속 수행한다.
    std::function<void(std::uint32_t,std::string_view)> log; // group29의 원본문자열을 순서대로 전달한다.
    std::function<bool()> reserveRegular; // new40 성공/실패를 보존한다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 실제 같은 Host가 event2/payload60/currenttime/count0을 만든다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> templateState; // 현재Template의실제 State getter4bd550/CD427920를 호출한다.
    std::function<void(std::uint32_t)> templateChanged; // getter뒤다시읽은현재Template의Notify4bd540/CD427910이다.
};
// event1/count0 자연반환은 최초payload5를 현재+5로 재예약하는 실제Regular RunFrame에 맡긴다.
class RawPlayerRegular {
public:
    // 같은 Player의현재9기록/두owner와 실제battleMode를 참조하고 모든 하위 효과를 확인한다.
    RawPlayerRegular(const RawPlayerWorld& player,const SquidOwnerMode& mode,PlayerRegularHooks hooks);
    // PATCH490810/CD407ef0: local/count 조건→event1 로그/Regular2 또는event2 State|4/Notify→반환이다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // actual Player 생성자 VT의 분배 판단을 재사용한다.
    bool Handles(Sid sid) const;
    // 상위 runtime의 다른Pool 연결을 효과 전에 확인한다.
    const SidPool& Pool() const;
private:
    const RawPlayerWorld& player_; // 같은 현재 Player 기록과 상태/VT 판별 몸체다.
    const SquidOwnerMode& mode_; // actual inBattleMode를 로그 이후에도 다시 읽는 살아 있는 공유 전역이다.
    PlayerRegularHooks hooks_; // 미복원 로그/Template/할당 효과를 빈 함수로 덮지 않는다.
};
}
