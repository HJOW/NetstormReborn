// 실제 SP 초기화48관측·장기 난수/추적 참조와 Client 생성자/공통 시작의 차이를 검증한다.
#include "TestSupport.h"
#include "client/ClientSpGlobals.h"
#include "client/ClientMain.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace { // 원문 fixture와 명시 할당 입력만 읽으며 게임 계산을 기대값으로 재작성하지 않는다.
std::vector<std::string> SpInitSplit(const std::string& line) { std::vector<std::string> fields;std::istringstream input(line);std::string field;while (std::getline(input,field,'\t')) fields.push_back(field);return fields; } // 헤더/열을 원시 순서대로 분리한다.
std::uint32_t SpInitNumber(const std::string& field) { return static_cast<std::uint32_t>(std::stoull(field)); } // 원문 DWORD 비트를 signed 변환 없이 읽는다.
struct SpInitStop:std::exception {}; // operator_new 정상 반환 전 중단하는 명시 검사 입력이다.
}

TEST_CASE(ClientSpGlobals_ReplaysNativeInitializationAllocations48) {
    std::ifstream input(NETSTORM_SP_INITIALIZATION_FIXTURE);if (!input) throw std::runtime_error("SP 초기화 fixture 없음");std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 두 x87 모드와 첫/재초기화의 정상/중단을 빠짐없이 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=SpInitSplit(line);CHECK(row.size()==12);const auto seed=SpInitNumber(row[0]),before=SpInitNumber(row[1]),stop=SpInitNumber(row[2]);std::uint32_t allocations{};std::int32_t tracking{};bool stopped{};
        netstorm::o::GameRandom random;netstorm::o::ScrambledSpStore store(random,[seed] { return seed; },[&](std::span<std::uint8_t> bytes) {
            CHECK(bytes.size()==36);++allocations;for (std::size_t byte=0;byte<bytes.size();++byte) bytes[byte]=static_cast<std::uint8_t>(allocations*13+byte*7); // 원문 operator_new에 제공했던 같은 명시 입력이다.
            if (stop && allocations==before*32+stop) throw SpInitStop{};
        });store.BindTrackingOffset(tracking);
        try { for (std::uint32_t pass=0;pass<=before;++pass) store.Initialize(); }catch (const SpInitStop&) { stopped=true; } // 부분 실패를 lazy 재실행이나 rollback으로 숨기지 않는다.
        CHECK(stopped==(row[4]=="stop") && allocations==SpInitNumber(row[5]) && store.Initialized()==bool(SpInitNumber(row[6])));CHECK(store.Offset()==SpInitNumber(row[7]) && std::bit_cast<std::uint32_t>(tracking)==SpInitNumber(row[8]) && random.State()==SpInitNumber(row[9]));CHECK((store.FirstPoolCurrent()!=nullptr)==bool(SpInitNumber(row[10])));
        const auto& pools=store.Pools();CHECK(row[11].size()==pools.size()*8);
        for (std::size_t byte=0;byte<pools.size()*4;++byte) CHECK(((pools[byte/4]>>((byte%4)*8))&255U)==std::stoul(row[11].substr(byte*2,2),nullptr,16)); // 실제 현재 포인터가 가리킨1152BYTE 전체와 비교한다.
        ++rows;
    }
    CHECK(rows==48);
}

TEST_CASE(ClientSpGlobals_ExplicitStartupUsesSameRandomAndLongLivedTracking) {
    auto random=std::make_shared<netstorm::client::ClientRandomGlobals>();std::uint32_t timeCalls{},allocations{};random->GameMode()=7;
    auto owner=std::make_shared<netstorm::client::ClientSpGlobals>(random,[&] { ++timeCalls;return 946684800; },[&](auto bytes) { CHECK(bytes.size()==36);std::fill(bytes.begin(),bytes.end(),static_cast<std::uint8_t>(++allocations)); });
    auto& store=owner->Store();auto* tracking=&store.TrackingOffsetCurrent();CHECK(!store.Initialized() && !store.FirstPoolCurrent() && store.Offset()==0 && *tracking==0 && timeCalls==0 && allocations==0 && random->Game().State()==55);
    store.Initialize();CHECK(store.Initialized() && store.FirstPoolCurrent()==store.Pools().data() && timeCalls==1 && allocations==32 && random->GameMode()==7 && random->CrtState()!=1);CHECK(tracking==&store.TrackingOffsetCurrent() && *tracking==static_cast<std::int32_t>(store.Offset()));
    store.Set(1,0x44960000U);const auto pools=store.Pools();const auto state=random->Game().State();const auto& readOnly=*owner;CHECK(&readOnly.Store()==&store && owner->RandomGlobals()==random && readOnly.Store().Pools()==pools && random->Game().State()==state);
    auto retained=owner;owner.reset();CHECK(&retained->Store()==&store && &store.TrackingOffsetCurrent()==tracking && store.Get(1)==0x44960000U && timeCalls==1 && allocations==32); // 소비자 교체나 읽기 조회는 재시드를 추가하지 않는다.
}

TEST_CASE(ClientSpGlobals_ActualClientCreationAndGetterDoNotInitialize) {
    netstorm::client::ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";std::shared_ptr<netstorm::client::ClientSpGlobals> retained;
    { netstorm::client::Client client(options);retained=client.SpGlobals();CHECK(retained->RandomGlobals()==client.RandomGlobals() && !retained->Store().Initialized() && client.RandomGlobals()->Game().State()==55 && client.RandomGlobals()->CrtState()==1);retained->Store().Initialize();retained->Store().Set(1,0x447a0000U); }
    CHECK(retained->Store().Initialized() && retained->Store().Get(1)==0x447a0000U); // 실제 Client 반납 뒤에도 장기 소비자가 같은 난수/SP 소유자를 함께 유지한다.
    { netstorm::client::Client fresh(options);CHECK(fresh.SpGlobals()!=retained && !fresh.SpGlobals()->Store().Initialized() && fresh.RandomGlobals()->Game().State()==55 && fresh.RandomGlobals()->CrtState()==1); }
}
