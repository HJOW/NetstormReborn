// 실제 통지의 빈 몸체/파생 분배와 독립 PE에서 관찰한 Player AI 호출을 확인한다.
#include "RawSceneSupport.h"
#include "o/RawContentParentNotice.h"
#include "o/RawPlayerDamageNotice.h"
using namespace netstorm::test::rawscene;

// 두 판본의 기본 부모는 상태·타입·내용물 인자를 읽지 않는 실제 빈 몸체다.
TEST_CASE(CampaignNotice_EmptyParentBodiesPreserveRawBytes) {
    // 각각의 실제 기본 Squid VT와 빈 통지 주소를 독립 지정한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,24000);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);
        std::fill(raw.begin(),raw.end(),std::uint8_t{0x9b}); // 가상 호출 분배에 상태/타입 필터를 추가하면 안 되는 명시 입력이다.
        Put(raw,0,edition==OriginalEdition::Patch1078 ? 0x501dd0U : 0x5058d8U);
        const std::vector<std::uint8_t> before(raw.begin(),raw.end());std::size_t calls=0;
        RawContentParentNotice notice(pool,[&calls](Sid,Sid,std::uint32_t) { ++calls; });
        CHECK(notice.NativeEntry(sid)==(edition==OriginalEdition::Patch1078 ? 0x41c100U : 0x401c20U));
        notice.Notice(sid,Sid{},1);notice.Notice(sid,Sid{65535},2);
        CHECK(calls==0 && std::equal(raw.begin(),raw.end(),before.begin()));
    }
}

// 실제 워크샾 VT는 기본 빈 몸체로 통과시키지 않고 인자 그대로 실제 파생 소비자를 요구한다.
TEST_CASE(CampaignNotice_DerivedAndUnknownVtablesAreExplicit) {
    // 두 판본의 실제 워크샾 VT/코드 주소를 지정한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,24000);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);
        Put(raw,0,edition==OriginalEdition::Patch1078 ? 0x509148U : 0x503298U);
        RawContentParentNotice missing(pool);bool rejected=false;
        try { missing.Notice(sid,Sid{20000},2); } catch (const std::logic_error&) { rejected=true; }
        CHECK(rejected && missing.NativeEntry(sid)==(edition==OriginalEdition::Patch1078 ? 0x453b50U : 0x47a6c0U));
        std::size_t called=0;RawContentParentNotice notice(pool,[&](Sid parent,Sid form,std::uint32_t reason) {
            CHECK(parent==sid && form.value==20000 && reason==2);++called;
        });
        notice.Notice(sid,Sid{20000},2);CHECK(called==1);
        Put(raw,0,0xdeadbeefU);rejected=false;
        try { notice.Notice(sid,Sid{},1); } catch (const std::logic_error&) { rejected=true; }
        CHECK(rejected && called==1);
    }
}

// PATCH의 실제 __thiscall 경계에서 얻은144개 입력/호출 여부/세 인자를 전수 대조한다.
TEST_CASE(CampaignNotice_PlayerDamageNativeWrapper) {
    std::size_t count=0; // 독립 PE에서 저장한 고유 루트 입력 개수다.
    // 각 행의 결과는 원본 래퍼 관찰값이며 C++에서 분기 기대값을 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_DAMAGE_NOTICE_FIXTURE).rows) {
        CHECK(row.size()==9);const auto owner=static_cast<std::int32_t>(std::stol(row[0]));
        SquidOwnerMode mode;mode.challenge=Number(row[1])!=0;
        std::vector<std::uint8_t> players(9*0xb4,0x46);Put(players,static_cast<std::size_t>(std::max(owner,0))*0xb4+0xb0,Number(row[2]));
        const auto before=players;std::size_t calls=0;std::array<std::uint32_t,3> observed{};
        RawPlayerDamageNotice notice(players,mode,[&](std::uint32_t token,Sid victim,Sid attacker) {
            observed={token,victim.value,attacker.value};++calls;
        });
        notice.Notice(owner,Sid{static_cast<std::uint16_t>(Number(row[3]))},Sid{static_cast<std::uint16_t>(Number(row[4]))});
        CHECK(calls==Number(row[5]) && observed[0]==Number(row[6]) && observed[1]==Number(row[7]) && observed[2]==Number(row[8]));
        CHECK(players==before);++count;
    }
    CHECK(count==144);
}

// 비영 AI는 실제 몸체 없이는 실패하며 현재 모드/AI 필드의 재조회와 호스트 범위 경계를 확인한다.
TEST_CASE(CampaignNotice_PlayerDamageCurrentStateAndMissingAi) {
    std::vector<std::uint8_t> players(9*0xb4);SquidOwnerMode mode;RawPlayerDamageNotice notice(players,mode);
    notice.Notice(1,Sid{20000},Sid{20001});Put(players,0xb4+0xb0,0x12345678U);
    mode.challenge=true;notice.Notice(39,Sid{},Sid{});mode.challenge=false;
    bool rejected=false;
    try { notice.Notice(1,Sid{20000},Sid{20001}); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);Put(players,0xb4+0xb0,0);notice.Notice(1,Sid{20000},Sid{20001});
    rejected=false;
    try { notice.Notice(9,Sid{},Sid{}); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);
}
