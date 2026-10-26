// 실제 Client의 초기 안내 DWORD/현재 옵션 writer/음향 결과의 동일 주소를 창 없이 검사한다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/CampaignLoginServices.h"
#include "o/RawMissionLoginTimer.h"
#include <array>
#include <type_traits>
#include <utility>

namespace { //보호 자산은 읽기만 하며 Run/Window/파일 저장을 호출하지 않는다.
using namespace netstorm::client;
using namespace netstorm::o;
// 비활성 실제 음향 계층을 사용하는 Client의 기존 headless 구성이다.
ClientOptions MainMenuOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
static_assert(std::is_same_v<decltype(std::declval<ClientAudio&>().ResultState()),std::uint32_t&>); //mutable getter는 실제 DWORD를 직접 빌린다.
static_assert(std::is_same_v<decltype(std::declval<const ClientAudio&>().ResultState()),const std::uint32_t&>); //const getter도 임시 사본을 반환하지 않는다.
}

// 초기1/BSS0/1은 별도 실제 주소이고 Native immediate+8c와도 합쳐지지 않는다.
TEST_CASE(ClientMainMenuGlobals_InitialValuesAndStableIndependentAddresses) {
    Client client(MainMenuOptions());const auto owner=client.LocalStateGlobals();CHECK(owner->FirstMenuPending()==1 && !owner->UserUpdated() && owner->TellTips()==1);
    const auto first=&owner->FirstMenuPending(),updated=&owner->UserUpdated(),tips=&owner->TellTips();CHECK(first!=updated && first!=tips && updated!=tips && first!=&owner->State().immediate[35]);
    owner->State().immediate[35]=0x12345678;owner->FirstMenuPending()=0;owner->UserUpdated()=0x80000000U;owner->TellTips()=0xffffffffU;
    CHECK(first==&std::as_const(*owner).FirstMenuPending() && updated==&std::as_const(*owner).UserUpdated() && tips==&std::as_const(*owner).TellTips());
    client.Configuration().SetInt("unrelatedMainMenuOption",17);CHECK(!*first && *updated==0x80000000U && *tips==0xffffffffU && owner->State().immediate[35]==0x12345678);
}

// 실제 Set 알림이 반환되기 전에 같은 tellTips DWORD를 signed ReadInt의 전체32비트로 갱신한다.
TEST_CASE(ClientMainMenuGlobals_TellTipsUsesCurrentDwordAndSynchronousOptionWriter) {
    Client client(MainMenuOptions());auto& configuration=client.Configuration();configuration.Configuration().AppendSection("END");const auto owner=client.LocalStateGlobals();const auto address=&owner->TellTips();
    owner->FirstMenuPending()=0x5a;owner->UserUpdated()=0xa5;
    const std::array<std::pair<const char*,std::uint32_t>,5> values{{{"0",0},{"1",1},{"-1",0xffffffffU},{"2147483648",0x80000000U},{"",0}}};
    for (const auto& [text,expected]:values) { //각 입력의 부호/상위비트를 현재 getter 주소에서 직접 관측한다.
        configuration.Set("tellTips",text);CHECK(address==&owner->TellTips() && *address==expected && owner->FirstMenuPending()==0x5a && owner->UserUpdated()==0xa5);
    }
    configuration.SetInt("tellTips",-7);client.InterpretStateOptions();CHECK(*address==0xfffffff9U && owner->FirstMenuPending()==0x5a && owner->UserUpdated()==0xa5);
}

// 같은 Audio 결과 DWORD에 실제 Login Timer의 Abort store와 후속 외부 효과가 순서대로 나타나야 한다.
TEST_CASE(ClientMainMenuGlobals_LoginTimerAndSceneUseTheSameCanonicalResultDword) {
    Client client(MainMenuOptions());client.InitializeNativeAudio();auto& audio=client.WorldAudio();CampaignLoginServices services;services.requestPhase=&audio.ResultState();
    CHECK(services.requestPhase==&audio.Scene().resultState && services.requestPhase==&std::as_const(audio).ResultState());*services.requestPhase=0xffffffffU;CHECK(audio.Scene().resultState==0xffffffffU);
    std::array<std::uint8_t,1620> players{};MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,players);double end=23.0;std::int32_t timeout{};std::uint32_t begin=7,scriptBreak{},clock{},abort{},close{}; //Timer가 직접 쓰는 서로 다른 Native 입력이다.
    MissionLoginTimerReads reads;reads.savedGame=reads.server=reads.mission=reads.missionPost=reads.initialGeysers=reads.checkingAssertions=[] { return false; };
    MissionLoginTimerHooks hooks;hooks.abortBattle=[&] { CHECK(audio.Scene().resultState==4 && !begin && end==0.0 && scriptBreak==1);++abort;audio.ResultState()=0x87654321U; };hooks.closeProgress=[&] { CHECK(*services.requestPhase==0x87654321U);++close; };
    RawMissionLoginTimer timer(OriginalEdition::Patch1078,{end,timeout,begin,scriptBreak,*services.requestPhase,clock},counts,std::move(reads),std::move(hooks));timer.Abort();
    CHECK(abort==1 && close==1 && audio.ResultState()==0x87654321U && audio.Scene().resultState==0x87654321U);
    client.Configuration().SetInt("tellTips",0);CHECK(audio.ResultState()==0x87654321U && services.requestPhase==&audio.Scene().resultState); //옵션 재해석이 결과 전역을 초기화하지 않는다.
}

// 실제 안내 순번은 옵션과 독립된 초기1 DWORD이며 Set 알림에서 signed 전체32비트를 동기 반영한다.
TEST_CASE(ClientMainMenuGlobals_TipNumberHasIndependentCurrentDwordAndSynchronousWriter) {
    Client client(MainMenuOptions());auto& configuration=client.Configuration();configuration.Configuration().AppendSection("END");const auto owner=client.LocalStateGlobals();const auto address=&owner->TipNumber();
    CHECK(*address==1 && address==&std::as_const(*owner).TipNumber() && address!=&owner->TellTips() && address!=&owner->FirstMenuPending() && address!=&owner->UserUpdated());
    owner->TipNumber()=0x12345678U;configuration.SetInt("unrelatedTipOption",7);CHECK(*address==0x12345678U); //순번 키가 없을 때 현재값을 초기1로 되돌리지 않는다.
    configuration.SetInt("tellTips",0);const std::array<std::pair<const char*,std::uint32_t>,6> values{{{"0",0},{"1",1},{"-1",0xffffffffU},{"2147483648",0x80000000U},{"4294967296",0},{"",0}}};
    for (const auto& [text,expected]:values) { //현재 순번의 부호/넘침과 다른 안내 flags 보존을 같은 실제 주소에서 확인한다.
        configuration.Set("tipNumber",text);CHECK(*address==expected && address==&owner->TipNumber() && !owner->TellTips() && owner->FirstMenuPending()==1 && !owner->UserUpdated());
    }
    configuration.SetInt("tipNumber",-7);client.InterpretStateOptions();CHECK(*address==0xfffffff9U && !owner->TellTips());
}
