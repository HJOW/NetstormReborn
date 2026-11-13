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
    if (client.Edition()==o::OriginalEdition::Cd1072) {
        client.PlayerRecords()->Initialize([&client](auto owner,auto mask) { // CD00406130의 첫 저장과 방향별9동맹 쓰기를 실제 장기 owner에 연결한다.
            mask|=1U<<owner;
            auto rows=client.PlayerRecords()->Records();const auto offset=owner*o::kCdInitializedPlayerStride+0x54; // CD Player의 동맹 mask 필드는+54다.
            for (std::uint32_t byte=0;byte<4;++byte) rows[offset+byte]=static_cast<std::uint8_t>(mask>>(byte*8)); // 원문 DWORD 저장을 같은 실제 행에 쓴다.
            auto& alliances=client.PlayerRecords()->Alliances();
            for (std::uint32_t target=0;target<9;++target) alliances[owner*9+target]=(mask&(1U<<target)) ? 1U:0U; // 원문은 현재 방향의 아홉 DWORD만 갱신한다.
            if (client.GameplayGlobals()->player.playerKnown!=0) throw std::logic_error("CD 동맹 초기화의 known 비영 Chat 장치 미연결"); // 최초 known0 가지는 여기서 정상 반환하며 뒤의 Chat 갱신을 성공값으로 대신하지 않는다.
        });
        return;
    }
    client.PlayerRecords()->Initialize([&client](auto owner,auto mask) {
        o::PlayerParticipationState state;const std::array<std::uint8_t,9> initialHidden{}; // 보호PE0052f81e의 최초9BYTE0이며 known0에서는 실제로 읽지 않는다.
        o::RawPlayerParticipation initial(o::OriginalEdition::Patch1078,client.PlayerRecords()->Records(),client.PlayerRecords()->Alliances(),initialHidden,state,{});
        const auto unavailable=[]()->bool { throw std::logic_error("공통 Player 동맹 시작의 미선택 World 장치"); }; // known0이 아닌 선택을 상수 모드로 통과시키지 않는다.
        initial.BindCurrentInputs({[&client] { return client.GameplayGlobals()->player.battleOwner; },[&client] { return client.GameplayGlobals()->player.playerKnown; },unavailable,unavailable,unavailable});initial.SetAlliance(owner,mask);
    });
}
}
