// 실제 공통 시작의 known0→전체9기록/9factory/81방향 동맹을 같은 소유자에 구성한다.
#include "client/ClientPlayerRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "o/RawPlayerParticipation.h"
#include <array>
#include <stdexcept>

namespace netstorm::client { // 공통 시작과 전체 State 정리의 다른 효과를 구별한다.
void InitializeClientPlayerRecords(Client& client) {
    if (client.GameplayGlobals()->player.playerKnown!=0) throw std::logic_error("공통 Player 시작의 실제 known0 관문");
    client.PlayerRecords()->Initialize([&client](auto owner,auto mask) {
        o::PlayerParticipationState state;const std::array<std::uint8_t,9> initialHidden{}; // 보호PE0052f81e의 최초9BYTE0이며 known0에서는 실제로 읽지 않는다.
        o::RawPlayerParticipation initial(o::OriginalEdition::Patch1078,client.PlayerRecords()->Records(),client.PlayerRecords()->Alliances(),initialHidden,state,{});
        const auto unavailable=[]()->bool { throw std::logic_error("공통 Player 동맹 시작의 미선택 World 장치"); }; // known0이 아닌 선택을 상수 모드로 통과시키지 않는다.
        initial.BindCurrentInputs({[&client] { return client.GameplayGlobals()->player.battleOwner; },[&client] { return client.GameplayGlobals()->player.playerKnown; },unavailable,unavailable,unavailable});initial.SetAlliance(owner,mask);
    });
}
}
