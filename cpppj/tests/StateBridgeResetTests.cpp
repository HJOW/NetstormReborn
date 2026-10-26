// 실제 PE64관측과 같은 Client 물리의 색/다리 스캔 초기화 범위를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawStateBridgeReset.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <vector>

namespace { //Native 입력을 같은 폭으로만 조립하고 실제 기대 결과는 동결 관측에서 읽는다.
using namespace netstorm::client;
using namespace netstorm::o;
// 마지막 빈 열을 제외하지 않는 원시 TSV/효과 구분자 해석이다.
std::vector<std::string> BridgeResetFields(std::string_view text,char separator='\t') {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1; } //순서를 바꾸지 않고 끝 열까지 읽는다.
}
// 입력과 canary의 원시 바이트를 복원한다.
std::vector<std::uint8_t> BridgeResetBytes(std::string_view text) {
    std::vector<std::uint8_t> bytes;
    for (std::size_t offset=0;offset<text.size();offset+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(offset,2)),nullptr,16))); //기대 수치를 계산하지 않는다.
    return bytes;
}
// 정렬되지 않은 DWORD/QWORD를 같은 little endian 비트로 읽는다.
std::uint64_t BridgeResetRead(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint64_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint64_t(bytes[at+byte])<<(8*byte);return value; } //소유자 색의 signed 해석은 호출자가 bit_cast한다.
// 원문 store 폭만 같은 입력 물리에 적용한다.
void BridgeResetPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint64_t value,std::size_t width=4) { for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //인접 canary를 보존한다.
// 호스트 물리 부족/다른 owner의 도달 위치만 예외로 관측한다.
template<class F> bool BridgeResetThrows(F&& operation) { try { operation();return false; }catch (const std::exception&) { return true; } }
// 창 없이 같은 보호 경로를 읽는 실제 Client다.
ClientOptions BridgeResetOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
}

// 두 실제 몸체의 전체 네 영역212BYTE와 실제 store의 주소/폭을 원문 기대에서 대조한다.
TEST_CASE(StateBridgeReset_IndependentNative64AndNeighborBytes) {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/state-bridge-reset-x86.tsv";std::ifstream input(path);CHECK(input.good());std::string line;std::uint32_t rows{};
    while (std::getline(input,line)) { //두 FPU 제어와 명시 입력64개를 모두 재생한다.
        if (line.empty() || line[0]=='#') continue;const auto fields=BridgeResetFields(line);CHECK(fields.size()==8);const auto before=BridgeResetBytes(fields[3]),wanted=BridgeResetBytes(fields[4]);auto actual=before;CHECK(before.size()==212 && wanted.size()==212);
        const auto stores=BridgeResetFields(fields[5],';');
        if (fields[0]=="colors") {
            std::array<std::int32_t,9> colors{};
            for (std::size_t owner=0;owner<colors.size();++owner) colors[owner]=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(BridgeResetRead(before,16+owner*4))); //Native의 실제 시작 색 표만 입력한다.
            ResetStateOwnerColors(colors);CHECK(stores.size()==9);
            for (std::size_t owner=0;owner<colors.size();++owner) { BridgeResetPut(actual,16+owner*4,std::bit_cast<std::uint32_t>(colors[owner]));const auto store=BridgeResetFields(stores[owner],':');CHECK(store.size()==3 && std::stoul(store[0])==0x531b08U+owner*4 && store[1]=="4"); } //실제 앞9DWORD와 Native store 순서/폭을 읽는다.
        } else {
            CHECK(fields[0]=="scan");BridgeDecayScanState state{std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(BridgeResetRead(before,88))),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(BridgeResetRead(before,92))),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(BridgeResetRead(before,124))),std::bit_cast<double>(BridgeResetRead(before,140,8))};
            ResetStateBridgeDecayScan(state,std::bit_cast<float>(static_cast<std::uint32_t>(BridgeResetRead(before,84))),std::bit_cast<double>(BridgeResetRead(before,188,8)));BridgeResetPut(actual,88,std::bit_cast<std::uint32_t>(state.first));BridgeResetPut(actual,92,std::bit_cast<std::uint32_t>(state.last));BridgeResetPut(actual,124,std::bit_cast<std::uint32_t>(state.cursor));BridgeResetPut(actual,140,std::bit_cast<std::uint64_t>(state.next),8);
            const std::array<std::uint32_t,4> addresses{0x52f96c,0x52f970,0x5453a8,0x5453b8};CHECK(stores.size()==addresses.size());
            for (std::size_t index=0;index<addresses.size();++index) { const auto store=BridgeResetFields(stores[index],':');CHECK(store.size()==3 && std::stoul(store[0])==addresses[index] && std::stoul(store[1])==(index==3?8U:4U)); } //원문의 세DWORD/기한QWORD 순서다.
        }
        CHECK(actual==wanted);++rows;
    }
    CHECK(rows==64);
}

// 색 표의 첫9칸만 쓰고 host 범위 부족은 어떤 prefix도 쓰기 전에 거부한다.
TEST_CASE(StateBridgeReset_ColorPhysicalBoundsAndTenthCanary) {
    std::array<std::int32_t,10> colors;colors.fill(-123);const auto before=colors;CHECK(BridgeResetThrows([&] { ResetStateOwnerColors(std::span(colors).first(8)); }) && colors==before);ResetStateOwnerColors(colors);CHECK(colors[0]==0 && colors[8]==8 && colors[9]==-123);
}

// Native 색 정리는 아직 빌린 실제 World에도 도달하며 다른 모드/TYPE를 초기화하지 않는다.
TEST_CASE(StateBridgeReset_LiveWorldUsesSameOwnerColorTable) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(BridgeResetOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,storage->links,97,nullptr,storage,true);const auto core=world.Core();CHECK(storage->Borrowed() && &core.bridgeState.ownerColors==&storage->links.ownerColors);core.bridgeState.ownerColors.fill(7);const auto kind=core.bridgeState.bridgeType;const auto mission=core.bridgeState.mission;
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear43b540);CHECK(core.bridgeState.ownerColors[1]==1 && core.bridgeState.ownerColors[8]==8 && core.bridgeState.bridgeType==kind && core.bridgeState.mission==mission && storage->Borrowed());
}

// 스캔은 실제 World 반납 뒤 현재 Client 시각으로 초기화하며 지연/색/다른 Client는 보존한다.
TEST_CASE(StateBridgeReset_ClientScanRequiresReturnedMatchingOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(BridgeResetOptions());auto storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());storage->bridgeDecayPeriod=7.75F;storage->bridgeDecayScan={31,47,39,-123.5};storage->links.ownerColors.fill(6);const auto* address=&storage->bridgeDecayScan;
    { auto use=storage->AcquirePrepared();CHECK(BridgeResetThrows([&] { ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Reset422490); }) && storage->bridgeDecayScan.first==31 && storage->bridgeDecayScan.next==-123.5); }
    auto foreign=MakeRawSurfaceStorage(assets.game);foreign->bridgeDecayScan={71,87,79,-5.0};CHECK(BridgeResetThrows([&] { ResetClientStateWorldStorage(client,*foreign,StateWorldCleanupEffect::Reset422490); }) && foreign->bridgeDecayScan.cursor==79 && storage->bridgeDecayScan.cursor==39);ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Reset422490);
    CHECK(&storage->bridgeDecayScan==address && storage->bridgeDecayScan.first==15000 && storage->bridgeDecayScan.last==23001 && storage->bridgeDecayScan.cursor==15000 && storage->bridgeDecayScan.next==client.Time().game+7.75 && storage->bridgeDecayPeriod==7.75F && storage->links.ownerColors[8]==6);
}

// 데이터 대여/반납 자체를 Native 초기화로 오인하지 않고 같은 스캔의 현재 값을 유지한다.
TEST_CASE(StateBridgeReset_PreparedLeaseKeepsCurrentScanAndPeriod) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto storage=MakeRawSurfaceStorage(assets.game);CHECK(storage->bridgeDecayScan.first==-1 && storage->bridgeDecayScan.last==-1 && storage->bridgeDecayScan.cursor==0 && storage->bridgeDecayScan.next==0.0 && storage->bridgeDecayPeriod==10.0F);storage->bridgeDecayScan={21,32,27,16777217.25};storage->bridgeDecayPeriod=0.125F;const auto* address=&storage->bridgeDecayScan;
    { auto use=storage->AcquirePrepared();CHECK(storage->bridgeDecayScan.first==21 && storage->bridgeDecayScan.next==16777217.25 && storage->bridgeDecayPeriod==0.125F); }
    CHECK(&storage->bridgeDecayScan==address && storage->bridgeDecayScan.cursor==27 && storage->bridgeDecayScan.last==32);
}
