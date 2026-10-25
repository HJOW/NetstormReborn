// 실제 구름 물리/파일/옵션과 기존 Native 본체를 같은 소유자에 조립해 검사한다.
#include "TestSupport.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/GifImage.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace { // 실제 보호 파일은 읽기만 하며 실패/할당 입력만 명시한다.
using namespace netstorm;using namespace netstorm::client;
std::filesystem::path CloudRoot() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"; } // 현재 저장소의 보호 자료 위치다.
ClientOptions CloudOptions() { ClientOptions options;options.gameDirectory=CloudRoot();return options; } // Run 없는 실제 Client 생성 입력이다.
template<class Error,class Operation> bool CloudRejects(Operation operation) { try { operation(); }catch (const Error&) { return true; }return false; } // 실제 선택된 오류만 인정한다.
void CloudConfig(Client& client,const char* name="d/Gifcloud.gif") { client.Configuration().Configuration().AppendSection("END");client.Configuration().Set("CloudFileSpec",name); } // 실제 현재 ConfigInterface/VFS 경로를 사용한다.
}

// PE 최초값/인접 alias/살아 있는 블록·내부/one-past/반납 뒤 거부를 검증한다.
TEST_CASE(ClientKencloudGlobals_PhysicalGlobalsAndAddressLifetime) {
    ClientKencloudGlobals cloud([](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); });const auto globals=cloud.Globals();CHECK(cloud.State().size()==2054 && globals.cache==0 && globals.phase==0 && globals.lowColor==228 && globals.speed==2 && globals.fury==10 && globals.seedCount==400 && globals.spacer==32 && cloud.Allocations()==0);CHECK(cloud.PhaseRows()[1]==8 && cloud.PhaseRows()[15]==13 && cloud.CloudName().size()==104);
    auto name=cloud.CloudName();name[84]=7;CHECK(cloud.State()[2049]==7);globals.dirtyRows[3]=29;CHECK(cloud.State()[519]==29);globals.seeds[999]=31;CHECK(cloud.State()[2027]==31); // 실제 같은 인접 물리를 빌리며 이름/표를 복사하지 않는다.
    const auto first=cloud.Allocate(36),second=cloud.Allocate(262144);CHECK(second>first+36 && cloud.Memory(first).size()==36 && cloud.Memory(first+35).size()==1 && cloud.Memory(first+36).empty());cloud.Memory(second+512)[0]=19;CHECK(cloud.Memory(second)[512]==19);cloud.Release(first);CHECK(CloudRejects<std::out_of_range>([&] { cloud.Memory(first); }) && cloud.Allocations()==1);cloud.Release(0);cloud.Release(second);CHECK(cloud.Allocations()==0);
}

// 실제512 GIF의 픽셀 전체/경계/같은 난수와 캐시 재조회 자연 반환을 확인한다.
TEST_CASE(ClientKencloudGlobals_ActualFileAndNative400SeedRandom) {
    Client client(CloudOptions());CloudConfig(client);auto cloud=client.KencloudGlobals();auto& random=client.RandomGlobals()->Game();random.SetState(0x89abcdefU);const auto pixels=DecodeBackgroundGif(client.Files().Read("d/Gifcloud.gif"));CHECK(pixels.width==512 && pixels.height==512);cloud->Build(client.Files(),client.Configuration(),random);auto globals=cloud->Globals();
    CHECK(globals.cache && globals.end==globals.cache+262144 && globals.nextRow==globals.cache+512 && globals.lastRow==globals.end-512 && cloud->Allocations()==1);CHECK(std::equal(pixels.indices.begin(),pixels.indices.end(),cloud->Memory(globals.cache).begin()));CHECK(random.State()==4201191824U && globals.spacer==110 && globals.allDirty==1 && globals.partialDirty==0); // 제품 전 Native v2 count400/state89abcdef 행의 실제 결과다.
    const auto token=globals.cache,rng=random.State();const auto bytes=cloud->Memory(token)[37];cloud->Build(client.Files(),client.Configuration(),random);CHECK(globals.cache==token && random.State()==rng && cloud->Allocations()==1 && cloud->Memory(token)[37]==bytes);cloud->Step();CHECK(globals.phase==1 && random.State()==rng && globals.dirtyRows[8]==1); // 같은 캐시의 원문 재조회/행 이동은 별도 난수를 쓰지 않는다.
}

// 없는 파일과512가 아닌 실제 GIF의 반납 뒤 fallback이 같은 Native 난수/채움을 사용하는지 검사한다.
TEST_CASE(ClientKencloudGlobals_MissingAndWrongSizeUseActualFallback) {
    for (const auto path:{"d/no-kencloud-424.gif","d/Defeat.gif"}) { // 실제 존재 실패/다른 치수의 파일을 각각 부모가 처리한다.
        Client client(CloudOptions());CloudConfig(client,path);auto cloud=client.KencloudGlobals();auto& random=client.RandomGlobals()->Game();random.SetState(0x89abcdefU);cloud->Build(client.Files(),client.Configuration(),random);const auto globals=cloud->Globals();CHECK(cloud->Allocations()==1 && globals.end==globals.cache+262144 && random.State()==4201191824U && globals.spacer==110);const auto memory=cloud->Memory(globals.cache);CHECK(std::all_of(memory.begin(),memory.end(),[](auto value) { return value==228; })); // 실제 fallback 전체 fill/행 이동 뒤도 같은 색이다.
    }
}

// fury0은 현재 할당/seed 효과를 남기고 정상 dirty reset까지 도달하지 않는다.
TEST_CASE(ClientKencloudGlobals_FallbackDivideFaultPreservesPrefix) {
    Client client(CloudOptions());CloudConfig(client,"d/no-kencloud-424.gif");auto cloud=client.KencloudGlobals();cloud->Value(ClientKencloudGlobals::Option::Fury)=0;auto& random=client.RandomGlobals()->Game();random.SetState(0x89abcdefU);CHECK(CloudRejects<std::domain_error>([&] { cloud->Build(client.Files(),client.Configuration(),random); }));const auto globals=cloud->Globals();CHECK(globals.cache && cloud->Allocations()==1 && globals.end==globals.cache+262144 && globals.allDirty==0 && random.State()==4201191824U);
}

// 같은 Client의 즉시 옵션 해석이 누락값·signed DWORD·기존 callback과 난수/파일 생략을 보존한다.
TEST_CASE(ClientKencloudGlobals_ActualConfigCallbackAndPureOwner) {
    Client client(CloudOptions());auto cloud=client.KencloudGlobals();CHECK(cloud==client.KencloudGlobals() && client.RandomGlobals()->Game().State()==55 && cloud->Allocations()==0);client.Configuration().Configuration().AppendSection("END");client.Configuration().SetInt("cloudSpeed",-1);client.Configuration().SetInt("cloudFury",-17);client.Configuration().SetInt("cloudiness",13);client.Configuration().SetInt("cloudAlgo",3);client.Configuration().SetInt("kenCloud",1);client.Configuration().SetInt("readOnlyForts",1);
    CHECK(cloud->Value(ClientKencloudGlobals::Option::Speed)==0xffffffffU && cloud->Value(ClientKencloudGlobals::Option::Fury)==0xffffffefU && cloud->Value(ClientKencloudGlobals::Option::Cloudiness)==13 && cloud->State()[2050]==3 && cloud->State()[2049]==1 && client.ReadOnlyForts()==1);cloud->ReadOptions(client.Configuration());CHECK(cloud->Value(ClientKencloudGlobals::Option::SeedCount)==400 && cloud->Allocations()==0 && client.RandomGlobals()->Game().State()==55);
}

// 실제 파일/Native 부모·장치 변화 재조회와 같은 이름 자연 반환을 한 소유자로 검증한다.
TEST_CASE(ClientKencloudGlobals_ActualTransitionUsesSameGlobalsAndDeviceOrder) {
    Client client(CloudOptions());CloudConfig(client);client.Configuration().Set("GamePalSpec","$1.COL");auto cloud=client.KencloudGlobals();cloud->Value(ClientKencloudGlobals::Option::Speed)=2;std::string trace;std::uint32_t loads{},maps{};std::array<std::uint8_t,9> name{'G','i','f','c','l','o','u','d',0};const std::span<const std::uint8_t> input(name);Renderer renderer(640,480);cloud->BindRenderer(&renderer);
    o::FortPaletteTransitionHooks effects;effects.log=[&](auto level,auto) { CHECK(level==29);trace+='L'; };effects.loadPalette=[&](auto,auto clear) { CHECK(clear==1);++loads;trace+='P'; };effects.buildMaps=[&] { ++maps;trace+='M';cloud->BindRenderer(nullptr); };cloud->Transition(input,client.Files(),client.Configuration(),client.RandomGlobals()->Game(),effects);CHECK(trace=="LPM" && loads==1 && maps==1 && cloud->Globals().cache && cloud->Globals().phase==2 && cloud->WorldRedraw()==0);
    const auto token=cloud->Globals().cache,rng=client.RandomGlobals()->Game().State();name[0]='g';trace.clear();cloud->Transition(input,client.Files(),client.Configuration(),client.RandomGlobals()->Game(),effects);CHECK(trace=="L" && loads==1 && maps==1 && cloud->Globals().cache==token && client.RandomGlobals()->Game().State()==rng); // 같은 이름은 실제 cache/Screen/maps/dirty를 건드리지 않는다.
}
