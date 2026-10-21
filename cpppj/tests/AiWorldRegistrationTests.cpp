// 원본 초기 등록 순서·지연 용량·현재 동맹/표면·간헐천 제거를 검사한다. 작성·컴파일만 수행한다.
#include "o/RawAiWorldRegistration.h"
#include "o/RawGeyserAiRegistration.h"
#include "ConstructionSupport.h"
using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 실제 backing에 명시 입력을 쓰며 원본 실행 관측 fixture나 게임 대체 AI로 사용하지 않는다.
struct WorldRegistrationInput {
    RawAiBrainMemory memory{0x43000000,[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); }}; // 할당 직후의 명시 입력이다.
    SidPool pool{OriginalEdition::Patch1078,32768}; // 초기 스캔의 현재 raw 슬롯을 읽는 같은 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 각 분기를 선택하는 명시 genus 입력이다.
    std::array<std::uint8_t,1620> players{};std::array<std::uint32_t,81> allies{}; // 현재 Player와 방향 동맹의 원문이다.
    std::vector<std::uint16_t> surface=std::vector<std::uint16_t>(65536); // 현재 hash0 WORD 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},outpost{126};double now{}; // 같은 실제 본체와 현재 전역이다.
    std::vector<std::string> trace;std::size_t farther{},missing{}; // 하위 호출 순서와 선택을 관찰한다.
    std::unique_ptr<RawAiObjectNotice> objects;std::unique_ptr<RawAiWorldRegistration> registration; // 입력 저장소보다 먼저 해제한다.
    // 목록은 충분한 실제512DWORD 입력 backing을 갖고 native 생성자를 대신했다고 주장하지 않는다.
    WorldRegistrationInput() {
        auto raw=memory.Bytes(token);std::fill(raw.begin(),raw.end(),std::uint8_t{});Field(0x260,1);Put(players,180+0xb0,token);
        // 원본16개 목록을 같은 실제 메모리에 준비한다.
        for (const auto at:{0x284U,0x2dcU,0x2e8U,0x2f4U,0x300U,0x30cU,0x318U,0x324U,0x330U,0x33cU,0x348U,0x354U,0x360U,0x36cU,0x378U,0x384U}) { Field(at,memory.Allocate(2048));Field(at+4,512); }
        types[71].flags2=0x4000;types[72].flags2=0x4000000;types[74].flags2=0x10000000;types[75].flags2=2;types[76].flags2=0x1000002;types[77].flags2=0x30000;
        AiObjectNoticeHooks notices;notices.carrierTargets=[](Sid) { return std::array<std::uint32_t,2>{}; };notices.log=[this](auto,const auto& log) { trace.push_back("worker"+std::to_string(log.object)); };
        notices.distanceSquared=[this](Sid sid,auto other) { CHECK(other==3 && Field(0x308)==1);trace.push_back("guard"+std::to_string(sid.value));return 1.0; };
        objects=std::make_unique<RawAiObjectNotice>(memory,pool,types,players,now,outpost,[] { return false; },std::move(notices));
        AiWorldRegistrationHooks hooks;hooks.center=[this](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))}; };hooks.fartherGeyser=[this](auto,auto) { ++farther; };hooks.missingSurface=[this](auto) { ++missing; };
        registration=std::make_unique<RawAiWorldRegistration>(memory,pool,types,players,allies,surface,*objects,std::move(hooks));
    }
    // 본체의 현재 DWORD만 읽는다.
    std::uint32_t Field(std::size_t at) { return Get(memory.Bytes(token),at); }
    // 현재 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 명시 raw 장면 입력이며 실제 할당/플레이의 결과로 가장하지 않는다.
    void Slot(std::uint16_t number,std::uint8_t type,std::uint8_t owner,float x=0,float y=0,std::uint8_t state=0) { auto raw=InputRaw(pool,Sid{number});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=state;raw[34]=owner;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y)); }
    // 현재 활성 목록을 읽는 관찰이며 예상 결과를 계산하지 않는다.
    std::vector<std::uint32_t> List(std::size_t at) { std::vector<std::uint32_t> result;for (std::uint32_t i=0;i<Field(at+8);++i) result.push_back(Get(memory.Bytes(Field(at)),i*4));return result; } // 반복문은 현재 count만 방문한다.
};
// 기존 시설/수집자를 먼저, 방어 자산을 나중에 등록하며 free와 dead를 서로 구별한다.
TEST_CASE(AiWorldRegistration_FullScanDefersGuardsAndSkipsSidOne) {
    WorldRegistrationInput input;input.Slot(1,73,1);input.Slot(2,72,1);input.Slot(3,71,1);input.Slot(4,72,1);input.Slot(5,77,1,0,0,2);input.Slot(6,74,0,20,20);input.Slot(8,73,2);input.Slot(9,69,1);input.Slot(10,75,1);input.Slot(11,73,1,0,0,1);input.surface[20*256+20]=10;input.Field(0x3c8,99);const auto allocations=input.memory.Allocations().size();
    input.registration->FullScan(input.token);
    CHECK(input.trace==std::vector<std::string>({"worker5","guard2","guard4"}));CHECK(input.Field(0x6d4+71*4)==1 && input.Field(0x6d4+72*4)==2 && input.Field(0x6d4+77*4)==1);CHECK(!input.Field(0x6d4+73*4) && !input.Field(0x6d4+74*4) && !input.Field(0x6d4+75*4));CHECK(input.List(0x360)==std::vector<std::uint32_t>({2,4}) && input.List(0x324)==std::vector<std::uint32_t>({6}) && input.List(0x330)==std::vector<std::uint32_t>({6}));CHECK(input.Field(0x3c8)==3 && !input.Field(0x25c));
    CHECK(input.memory.Allocations().size()==allocations+1 && input.memory.Allocations().back().size==1200 && input.memory.Frees().size()==1 && !input.memory.Contains(input.memory.Allocations().back().token));
}
// 임시 목록은 처음300개만 보존하며 시설 이후에 지연 순서대로 같은 통지를 보낸다.
TEST_CASE(AiWorldRegistration_DelayedCapacityAndExceptionLifetime) {
    WorldRegistrationInput input;input.Slot(3,71,1);
    // 용량보다5개 많은 방어 자산을 명시 입력으로 둔다.
    for (std::uint16_t sid=100;sid<405;++sid) input.Slot(sid,72,1);
    input.registration->FullScan(input.token);CHECK(input.Field(0x6d4+72*4)==300 && input.List(0x360).size()==300 && input.List(0x360).front()==100 && input.List(0x360).back()==399);
    WorldRegistrationInput failed;failed.Slot(3,71,1);failed.Slot(5,77,1);failed.Field(0x3c8,99);failed.Field(0x2dc,0xdeadbeef);CHECK(Throws([&] { failed.registration->FullScan(failed.token); }));CHECK(failed.Field(0x3c8)==99 && failed.Field(0x6d4+71*4)==1 && failed.memory.Frees().size()==1); // 실패 전 실제 쓰기는 보존하지만 임시1200BYTE는 반납한다.
}
// 방향 동맹·현재 신전·동일 거리 유지와 자신의 거리만 float 저장하는 원문을 구분한다.
TEST_CASE(AiWorldRegistration_AllyDistanceKeepsNativeFloatBoundary) {
    WorldRegistrationInput input;input.Slot(50,74,0,10,0);input.Slot(60,73,2,20,0);Put(input.players,360+0x7c,60);input.allies[9+2]=1;CHECK(!input.registration->CloserToAlly(input.token,50));input.Slot(60,73,2,15,0);CHECK(input.registration->CloserToAlly(input.token,50));input.allies[9+2]=0;input.allies[18+1]=1;CHECK(!input.registration->CloserToAlly(input.token,50));
    input.allies[9+2]=1;input.Slot(50,74,0,1,0.0003F);input.Slot(60,73,2,0,-0.00001F);CHECK(input.registration->CloserToAlly(input.token,50)); // 실제 동맹 거리는 조금 더 멀어도 자신의 fstp DWORD 올림 경계가 비교 기준이다.
    input.Slot(50,74,0,1,0.0002F);input.Slot(60,73,2,0,0.00001F);CHECK(!input.registration->CloserToAlly(input.token,50)); // 자신의 DWORD 내림보다 상대 거리가 크므로 추가하지 않는다.
    input.registration->Geyser(input.token,50,1);CHECK(input.farther==0 && input.missing==1);input.Slot(60,73,2,1,0.0002F);input.registration->Geyser(input.token,50,1);CHECK(input.farther==1 && input.Field(0x32c)==1);
}
// 표면 소유·섬 마스크·범위 밖 진단과 제거의 좌표 비조회/모든 중복 압축을 확인한다.
TEST_CASE(AiWorldRegistration_GeyserSurfaceAndRemovalContracts) {
    WorldRegistrationInput input;input.Slot(50,74,0,-0.8F,1.9F);input.Slot(51,75,1);input.surface[256]=51;input.registration->Geyser(input.token,50,1);CHECK(input.List(0x324)==std::vector<std::uint32_t>({50}) && input.List(0x330)==std::vector<std::uint32_t>({50}));
    input.Slot(51,75,2);input.registration->Geyser(input.token,50,2);CHECK(input.Field(0x32c)==2 && input.Field(0x338)==1 && !input.Field(0x350));input.Slot(51,76,0);input.registration->Geyser(input.token,50,1);CHECK(input.Field(0x350)==1);input.Slot(50,74,0,300,2);input.registration->Geyser(input.token,50,1);CHECK(input.missing==1 && input.Field(0x350)==2);
    // 다섯 목록에 같은 제거 입력과 stale 꼬리를 명시해 좌표를 읽지 않는0 분기를 고립한다.
    for (const auto at:{0x324U,0x330U,0x33cU,0x348U,0x354U}) { auto list=input.memory.Bytes(input.Field(at));Put(list,0,0xffffffff);Put(list,4,7);Put(list,8,0xffffffff);Put(list,12,8);input.Field(at+8,4); }
    input.registration->Geyser(input.token,0xffffffff,0);
    // 활성 결과만 압축하며 세 번째 칸의 과거 값은 그대로 남긴다.
    for (const auto at:{0x324U,0x330U,0x33cU,0x348U,0x354U}) CHECK(input.List(at)==std::vector<std::uint32_t>({7,8}) && Get(input.memory.Bytes(input.Field(at)),8)==0xffffffff);
}
// 하위가 다음 Player를 바꾸면 현재 포인터를 읽고 과거 owner는 다시 방문하지 않는다.
TEST_CASE(AiWorldRegistration_GeyserWrapperRereadsNextPlayer) {
    std::array<std::uint8_t,1620> players{};Put(players,0xb0,99);Put(players,180+0xb0,111);Put(players,360+0xb0,222);RawGeyserAiRegistration registration(OriginalEdition::Patch1078,players);std::vector<std::uint32_t> called;
    registration.BindPatch([&](auto token,auto sid,auto state) { CHECK(sid==0xffffffff && state==7);called.push_back(token);if (token==111) { Put(players,180+0xb0,444);Put(players,360+0xb0,333); } });registration.Register(0xffffffff,7);CHECK(called==std::vector<std::uint32_t>({111,333}));CHECK(Throws([&] { registration.BindPatch({}); }));
    std::array<std::uint8_t,1548> cdPlayers{};RawGeyserAiRegistration cd(OriginalEdition::Cd1072,cdPlayers);CHECK(Throws([&] { cd.BindPatch([](auto,auto,auto) {}); }));
}
}
