// 실제 공통 Player 할당·Core0 선행·초기화된 기록 대여와 실패의 부분 수명을 확인한다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace { // GUI/설정 저장 없이 실제 Client 소유자를 사용한다.
using namespace netstorm::client;
ClientOptions PlayerRuntimeOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";return options; } // 실제 archive를 읽기만 한다.
template<class Function> bool PlayerRuntimeRejects(Function operation) { try { operation(); }catch (const std::logic_error&) { return true; }return false; } // 실제 수명 관문 거부만 인정한다.
std::uint32_t PlayerRuntimeDword(std::span<const std::uint8_t> bytes,std::size_t offset) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; } // 실제 Native 헤더의 네 BYTE만 읽는다.
}

TEST_CASE(ClientPlayerRuntime_CommonStartupUsesActualRecordsCoreViewAndAlliances) {
    Client client(PlayerRuntimeOptions());const auto records=client.PlayerRecords();const auto core=client.CoreDataGlobals();CHECK(!records->Base() && !records->Initialized() && core->Players().empty());InitializeClientPlayerRecords(client);
    CHECK(records->Base()!=0 && records->Initialized() && records->BoundCoreData()==core && core->Players().data()==records->Records().data() && core->Players().size()==1620);CHECK(core->Current().empty() && core->Active()==0 && client.GameplayGlobals()->player.playerKnown==0);
    for (std::uint32_t owner=0;owner<9;++owner) { // 같은 실제9기록과 방향별81칸이 원문 동맹 자식으로 구성된다.
        CHECK(PlayerRuntimeDword(records->Records(),owner*180+4)==30 && PlayerRuntimeDword(records->Records(),owner*180+8)==0 && PlayerRuntimeDword(records->Records(),owner*180+0x54)==(1U<<owner));for (std::uint32_t target=0;target<9;++target) CHECK(records->Alliances()[owner*9+target]==(owner==target?1U:0U)); // 대칭 표를 별도 계산해 주입하지 않는다.
    }
}

TEST_CASE(ClientPlayerRuntime_LiveCoreRequiresNativeZeroSelectionBeforeFree) {
    Client client(PlayerRuntimeOptions());InitializeClientPlayerRecords(client);const auto records=client.PlayerRecords();const auto core=client.CoreDataGlobals();const std::uint32_t metamap=0;auto& player=client.GameplayGlobals()->player;core->Select(player,metamap,3,1);core->Active()=7;std::array<std::uint8_t,10> source{};
    for (std::size_t index=0;index<source.size();++index) source[index]=core->Current()[index]=static_cast<std::uint8_t>(71+index); // 실제 이전 Player+6c의 명시 입력이며 해제 뒤 그 span을 읽지 않는다.
    const auto base=records->Base();const std::vector<std::uint8_t> before(records->Records().begin(),records->Records().end());player.playerKnown=0;CHECK(PlayerRuntimeRejects([&] { InitializeClientPlayerRecords(client); }));CHECK(records->Base()==base && records->Initialized() && std::equal(before.begin(),before.end(),records->Records().begin()) && core->Active()==7 && !core->Current().empty());
    core->Select(player,metamap,0,1);CHECK(core->Current().empty() && core->Active()==0 && std::equal(source.begin(),source.end(),core->Cached().begin()));InitializeClientPlayerRecords(client);
    CHECK(records->Base()!=base && core->Players().data()==records->Records().data() && core->Current().empty() && std::equal(source.begin(),source.end(),core->Cached().begin()));core->Select(player,metamap,1,1);CHECK(core->Current().data()==records->Records().data()+180+0x6c && player.playerKnown==1);
}

TEST_CASE(ClientPlayerRuntime_AdoptsInitializedStorageWithoutAnotherNativeInitialization) {
    Client client(PlayerRuntimeOptions());InitializeClientPlayerRecords(client);const auto records=client.PlayerRecords();const auto base=records->Base();const std::vector<std::uint8_t> before(records->Records().begin(),records->Records().end());records->FortCount()=5;records->LastLoginPlayer()=7;
    { auto use=records->Acquire();use->AdoptCurrent();netstorm::o::SquidPostPopList list;use->BindFactory(list,1);CHECK(list.Count()==0 && records->Base()==base && records->FortCount()==5 && records->LastLoginPlayer()==7 && std::equal(before.begin(),before.end(),records->Records().begin()));CHECK(PlayerRuntimeRejects([&] { InitializeClientPlayerRecords(client); }) && PlayerRuntimeRejects([&] { use->AdoptCurrent(); })); }
    CHECK(records->Base()==base && records->Initialized()); // 대여 반납은 Core/Player 데이터를 임의 초기화하지 않는다.
}

TEST_CASE(ClientPlayerRuntime_FailedAllocationKeepsPartialViewAndRejectsAdoption) {
    auto core=std::make_shared<ClientCoreDataGlobals>();ClientPlayerRecords records([](auto)->void { throw std::runtime_error("검사 명시 factory 할당 입력 중단"); });records.BindCoreData(core);bool stopped{};
    try { records.Initialize({}); }catch (const std::runtime_error&) { stopped=true; }CHECK(stopped && records.Base()!=0 && records.Records().size()==1620 && core->Players().data()==records.Records().data() && !records.Initialized());auto use=records.Acquire();CHECK(PlayerRuntimeRejects([&] { use->AdoptCurrent(); })); // 정상 복귀하지 않은 실제 부분 할당을 완료 기록으로 가장하지 않는다.
}
