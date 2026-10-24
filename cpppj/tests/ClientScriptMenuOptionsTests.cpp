// 실제 Client의 설정 알림/장기 전역을 원문 설정 슬라이스와 대조한다. 창/게임 루프는 실행하지 않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace netstorm::client;
namespace { // 창 없는 실제 Client와 원문 입력 열만 준비한다.
ClientOptions CheckedOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // 보호 VFS를 읽지만 Run/설정 저장은 호출하지 않는다.
}

// 누락 ReadInt/이전 허용값/거부값과 두 선행0 쓰기를 실제 설정 알림에서 재생한다.
TEST_CASE(ClientScriptMenuOptions_IndependentOriginalSlice) {
    std::ifstream fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/html-checked-options-x86.tsv");CHECK(fixture.good());std::string line,group;std::unique_ptr<Client> client;std::size_t count{};
    while (std::getline(fixture,line)) { // 자료의 기대값은 원문에서 읽으며 C++에서 생성하지 않는다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);std::uint32_t before{},special{},auxiliary{},after{},afterSpecial{},afterAuxiliary{},stack{},reads{};std::string value;
        row>>before>>value>>special>>auxiliary>>after>>afterSpecial>>afterAuxiliary>>stack>>reads;CHECK(!row.fail());const auto current=std::to_string(before)+':'+value;
        if (group!=current) { // 누락 그룹을 새 실제 Config에서 검사하며 Client 단일 소유자 규칙을 유지한다.
            client.reset();client=std::make_unique<Client>(CheckedOptions());group=current;client->Configuration().Configuration().AppendSection("END");if (value!="-") client->Configuration().SetInt("userCheat",std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoul(value))));
        }
        auto globals=client->GameplayGlobals();globals->energy.userCheat=before;globals->player.special=special!=0;globals->specialAuxiliary=auxiliary;client->Configuration().onChanged();
        CHECK(globals->energy.userCheat==after && client->ScriptMenuGlobal(0x5cab30)==after);CHECK((globals->player.special ? 1U : 0U)==afterSpecial && client->ScriptMenuGlobal(0x55a480)==afterSpecial);CHECK(globals->specialAuxiliary==afterAuxiliary && stack==56 && reads==1);++count;
    }
    CHECK(count==144);
}

// 원문 ReadInt의 실제 문자열 파서와 동기 알림을 사용해 유일한 허용값/누락 보존/주소 수명을 검사한다.
TEST_CASE(ClientScriptMenuOptions_ActualConfigTextAndCurrentGlobals) {
    Client client(CheckedOptions());auto globals=client.GameplayGlobals();auto& config=client.Configuration();const auto* address=&globals->energy.userCheat;config.Configuration().AppendSection("END");
    const std::array<std::pair<const char*,std::uint32_t>,10> cases{{{"8675309",0x845fed},{"  +8675309tail",0x845fed},{"-4286291987",0x845fed},{"8675308",0},{"8675310",0},{"-8675309",0},{"0x845fed",0},{"",0},{"abc",0},{"4294967295",0}}}; // ConfigParseLong의 원문32비트·decimal 계약을 거친다.
    for (const auto& [text,expected]:cases) { // Set이 반환되기 전에 같은 현재 전역이 갱신되어야 한다.
        globals->player.special=true;globals->specialAuxiliary=0xffffffffU;config.Set("userCheat",text);CHECK(address==&globals->energy.userCheat && *address==expected && !globals->player.special && !globals->specialAuxiliary);
    }
    config.SetInt("userCheat",0x845fed);globals->player.special=true;config.SetInt("unrelatedOption",17);CHECK(*address==0x845fed && !globals->player.special);
    globals->player.special=true;CHECK(client.ScriptMenuGlobal(0x55a480)==1 && client.ScriptMenuGlobal(0x5cab30)==0x845fed);bool rejected{};try { client.ScriptMenuGlobal(0x5c85a4); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
