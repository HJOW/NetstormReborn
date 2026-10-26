// 제품 이전의 실제 PE 관측과 장기 SID 목록의 대여·정리 범위를 검증한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawStateSidLists.h"
#include "o/RawPriestLookup.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 보호 자산은 읽기만 하며 실제 게임이나 창을 실행하지 않는다.
using namespace netstorm::o;
using namespace netstorm::client;
constexpr std::uint32_t kRegistryAddress=0x592938,kHeaderAddress=0x12000100; //최초 Native 관측에 사용한 실제 표/격리 헤더 주소다.
// TSV의 명시 열을 읽으며 기대 바이트를 제품 함수로 계산하지 않는다.
std::vector<std::string> SidListFields(std::string_view line) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=line.find('\t',start);result.emplace_back(line.substr(start,end==line.npos?line.npos:end-start));if (end==line.npos) return result;start=end+1; } //마지막 열까지 보존한다.
}
// 최초 전체 관측의 hex를 실제 바이트 배열로 읽는다.
std::vector<std::uint8_t> SidListBytes(std::string_view hex) {
    std::vector<std::uint8_t> bytes;for (std::size_t index=0;index<hex.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));return bytes; //canary도 제외하지 않는다.
}
// 호스트 정렬이나 endian에 기대지 않고 네 바이트를 읽는다.
std::uint32_t SidListRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //표와 헤더의 실제 입력이다.
// CPU와 같은 DWORD폭의 쓰기를 실제 바이트에 적용한다.
void SidListPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //인접 바이트는 보존한다.
// 실제 소유 목록8개를 원문 정적 생성 순서로 순회한다. 다중선택은 별도 Client owner다.
std::array<SquidPostPopList*,8> SidListOwners(RawStateSidLists& lists) { return {&lists.factories,&lists.dudes,&lists.outposts,&lists.providers,&lists.geysers,&lists.priests,&lists.residences,&lists.temples}; }
// 정리 실패와 정상 생략을 구분하는 좁은 예외 관측이다.
template<class F> bool SidListThrows(F&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
// 디바이스 없는 실제 Client의 보호 자산 경로만 지정한다.
ClientOptions SidListOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
// 같은 owner의 실제 장기 표면을 구성하며 Native State 부모의 실행으로 세지 않는다.
std::shared_ptr<RawSurfaceStorage> SidListStorage(Client& client) { return MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux()); }
}

// 실제 세 함수의 전체 출력·canary와 reset store 순서를66개 최초 독립 관측에서 읽는다.
TEST_CASE(StateSidLists_IndependentNative66AndCurrentPointerOrder) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/state-sid-lists-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},resetRows{};
    while (std::getline(file,line)) { //정밀도2개와11가지 표 입력을 모두 재생한다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=SidListFields(line);CHECK(row.size()==10);if (row.size()!=10) continue;
        auto table=SidListBytes(row[4]),headers=SidListBytes(row[5]);std::array<std::uint32_t,20> pointers{};for (std::size_t index=0;index<20;++index) pointers[index]=SidListRead(table,16+index*4); //실제 CPU 입력을 선택 토큰으로만 해석한다.
        std::string stores;const auto argument=static_cast<std::uint32_t>(std::stoull(row[3]));
        if (row[0]=="register") RegisterStateSidList(pointers,argument);
        else if (row[0]=="unregister") UnregisterStateSidList(pointers,argument);
        else { ++resetRows;ResetRegisteredStateSidLists(pointers,[&](auto header) {
            const auto target=header+12;const auto event=std::to_string(target)+":4:0";stores+=(stores.empty()?"":";")+event;
            if (target>=kRegistryAddress && target<kRegistryAddress+80) pointers[(target-kRegistryAddress)/4]=0;
            else { CHECK(target>=kHeaderAddress && target<kHeaderAddress+640);SidListPut(headers,16+target-kHeaderAddress,0); }
        }); }
        for (std::size_t index=0;index<20;++index) SidListPut(table,16+index*4,pointers[index]); //비활성 포인터와 표 이웃32BYTE도 전체 비교한다.
        CHECK(table==SidListBytes(row[6]) && headers==SidListBytes(row[7]));if (row[0]=="reset") CHECK((stores.empty()?"-":stores)==row[8]);CHECK(std::stoul(row[9])>0);++rows;
    }
    CHECK(rows==66 && resetRows==22);
}

// Native static callsite의 실제 용량과 등록순서9개/빈11칸을 직접 확인한다.
TEST_CASE(StateSidLists_ExactStaticCapacitiesAndNineActualHeaders) {
    RawStateSidLists lists;const auto owners=SidListOwners(lists);constexpr std::array<std::size_t,8> capacities{500,30,100,256,100,16,50,20}; //템플20과 아웃포스트100을 바꾸면 실패한다.
    constexpr std::array<std::uint32_t,9> headers{0x55a4d0,0x55a4ec,0x55a708,0x592f80,0x5950d8,0x5954c0,0x59a9d4,0x5caf28,0x5e1898}; //최초 PE 정적 감사의 실제 this 순서다.
    for (std::size_t index=0;index<owners.size();++index) CHECK(owners[index]->Capacity()==capacities[index] && owners[index]->Count()==0); //첫 활성 개수만0이다.
    CHECK(std::equal(headers.begin(),headers.end(),lists.registeredHeaders.begin()));CHECK(std::all_of(lists.registeredHeaders.begin()+9,lists.registeredHeaders.end(),[](auto value) { return value==0; }));
}

// 선택된 쓰기 실패는 완료 prefix를 보존하며 빈 표에는 효과 함수를 요구하지 않는다.
TEST_CASE(StateSidLists_DemandFailureAndDuplicateRegistration) {
    std::array<std::uint32_t,21> headers{};headers[20]=0x87654321;ResetRegisteredStateSidLists(headers,{});CHECK(SidListThrows([&] { RegisterStateSidList(std::span(headers).first(19),1); }));
    RegisterStateSidList(headers,7);RegisterStateSidList(headers,7);RegisterStateSidList(headers,0);CHECK(headers[0]==7 && headers[1]==7 && headers[2]==0 && headers[20]==0x87654321);std::uint32_t count=9,calls{};
    CHECK(SidListThrows([&] { ResetRegisteredStateSidLists(headers,[&](auto header) { CHECK(header==7);if (++calls==2) throw std::logic_error("명시 실제 store 중단");count=0; }); }));CHECK(calls==2 && count==0 && headers[0]==7 && headers[1]==7 && headers[20]==0x87654321);
    UnregisterStateSidList(headers,7);CHECK(headers[0]==0 && headers[1]==0 && headers[20]==0x87654321);CHECK(SidListThrows([&] { ResetRegisteredStateSidLists(std::span(headers).first(19),{}); }));
}

// count만 지우고 데이터/용량/포인터/등록표·다른 State 물리를 보존한다.
TEST_CASE(StateSidLists_ClientResetUsesSameEightCountsAndSignedSelection) {
    Client client(SidListOptions());auto storage=SidListStorage(client);auto& lists=storage->sidLists;const auto owners=SidListOwners(lists);auto& selection=client.GameplayGlobals()->selection;selection.multiple={0x10037U,0xffffffffU};selection.multipleCount=-7;selection.selected=0x12345678;const auto selectionData=selection.multiple;const auto registry=lists.registeredHeaders;
    std::array<std::vector<std::uint32_t>,8> before;std::array<const std::uint32_t*,8> pointers{};
    for (std::size_t index=0;index<owners.size();++index) { auto& list=*owners[index];for (std::size_t slot=0;slot<list.entries.size();++slot) list.entries[slot]=std::uint32_t(0xa5000000U+index*1000+slot);list.count=index==0?0xffffffffU:std::uint32_t(index+1);before[index]=list.entries;pointers[index]=list.entries.data(); } //손상된 개수도 원문은 미리 검증하지 않고0으로 쓴다.
    storage->priestOwnerState[1]=17;storage->totalMade.localCounts[3]=19;storage->spots[100]=23;storage->hash.Cell(0,1,1)=29;
    ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear472830);
    for (std::size_t index=0;index<owners.size();++index) CHECK(owners[index]->count==0 && owners[index]->entries==before[index] && owners[index]->entries.data()==pointers[index]); //실제 포인터와 비활성 데이터도 보존한다.
    CHECK(lists.registeredHeaders==registry && selection.multipleCount==0 && selection.multiple==selectionData && selection.selected==0x12345678);CHECK(storage->priestOwnerState[1]==17 && storage->totalMade.localCounts[3]==19 && storage->spots[100]==23 && storage->hash.Cell(0,1,1)==29);
}

// 대여/다른 Client 관문과 현재 등록 해제/순서의 차이가 실제 count 결과에 반영된다.
TEST_CASE(StateSidLists_ClientGuardAndRegisteredSubsetKeepNativePrefix) {
    Client client(SidListOptions());auto storage=SidListStorage(client);auto& lists=storage->sidLists;lists.priests.count=3;auto use=storage->AcquirePrepared();CHECK(SidListThrows([&] { ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear472830); }) && lists.priests.count==3);use.reset();auto foreign=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);foreign->sidLists.priests.count=13;CHECK(SidListThrows([&] { ResetClientStateWorldStorage(client,*foreign,StateWorldCleanupEffect::Clear472830); }) && foreign->sidLists.priests.count==13 && lists.priests.count==3); //다른 실제 권한 owner를 한 Client의 엄격한 관문에 제공한다.
    UnregisterStateSidList(lists.registeredHeaders,0x5954c0);lists.factories.count=5;ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear472830);CHECK(lists.factories.count==0 && lists.priests.count==3);
    lists.registeredHeaders.fill(0);lists.registeredHeaders[0]=0x55a4d0;lists.registeredHeaders[1]=0x12345678;lists.registeredHeaders[2]=0x5954c0;lists.factories.count=7;
    CHECK(SidListThrows([&] { ResetClientStateWorldStorage(client,*storage,StateWorldCleanupEffect::Clear472830); }));CHECK(lists.factories.count==0 && lists.priests.count==3 && lists.registeredHeaders[1]==0x12345678);
}

// 두 번의 prepared World가 같은 목록/count를 빌리며 반납은 현재 데이터를 지우지 않는다.
TEST_CASE(StateSidLists_PreparedWorldSharesBookkeepingAcrossLeases) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);BridgeConnectState links=storage->links;auto& providers=storage->sidLists.providers;auto& factories=storage->sidLists.factories;providers.entries[255]=0xfedcba98;providers.count=1;providers.entries[0]=17;const auto* data=providers.entries.data();
    for (int lease=0;lease<2;++lease) { //실제 World 두 세대의 참조와 생존한 활성/비활성 값을 확인한다.
        RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,links,97,nullptr,storage,true);auto core=world.Core();CHECK(&core.bookkeeping.Providers()==&providers && &core.bookkeeping.Factories()==&factories && providers.entries.data()==data && providers.entries[255]==0xfedcba98);
        CHECK(core.bookkeeping.Providers().count==1 && core.bookkeeping.Providers().Items()[0]==17);CHECK(core.bookkeeping.providers.entries.empty() && core.bookkeeping.factories.entries.empty()); //독립 fallback은 실제 목록의 복사본을 만들지 않는다.
    }
    CHECK(!storage->Borrowed() && storage->Generation()==2 && providers.count==1 && providers.entries.data()==data && providers.entries[255]==0xfedcba98);
}

// 일반 설치의 명시 fresh 정책만 초기화하며 Native reset과 prepared 대여는 현재 배열을 보존한다.
TEST_CASE(StateSidLists_GeneralFreshAndPreparedPreserveDistinctPolicies) {
    auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);auto owners=SidListOwners(storage->sidLists);std::array<const std::uint32_t*,8> pointers{};
    for (std::size_t index=0;index<owners.size();++index) { pointers[index]=owners[index]->entries.data();owners[index]->entries.back()=17;owners[index]->count=1; } //모든 실제 목록의 현재 비영 입력이다.
    { auto use=storage->AcquirePrepared();CHECK(SidListThrows([&] { storage->Acquire(); }));for (auto* list:owners) CHECK(list->count==1 && list->entries.back()==17); } //실패한 중복 대여는 fresh 쓰기 전에 거부한다.
    { auto use=storage->Acquire();for (std::size_t index=0;index<owners.size();++index) CHECK(owners[index]->count==0 && owners[index]->entries.data()==pointers[index] && std::all_of(owners[index]->entries.begin(),owners[index]->entries.end(),[](auto value) { return value==0; })); } //주소를 바꾸는 새 vector 할당을 하지 않는다.
}

// 실제 사제 접두의 용량 관문은16번째까지 등록하고17번째부터 SID/count를 보존한다.
TEST_CASE(StateSidLists_ActualPriestPrefixUsesSixteenAndSharedLookup) {
    RawStateSidLists lists;PriestPostPopState state;state.sharedPriests=&lists.priests;SidPool pool(OriginalEdition::Patch1078,32768,false);GameRandom random;ScrambledSpStore sp(random,[] { return 1U; });SquidPostPopState book;SquidRewardState mode;std::vector<RiftTypeRecord> types(188);SquidReward hp(pool,types,book,mode,&sp);std::uint32_t searched{},reserved{};
    RawPriestPostPop prefix(pool,hp,state,{[&](Sid,std::uint32_t) { ++searched;return false; },[&] { ++reserved;return false; },[](Sid,std::uint32_t,float) { CHECK(false); }}); //외부 예약 실패를 명시해 HP/프로세스 생성 생략도 검증한다.
    std::vector<std::uint32_t> created;
    for (std::uint32_t index=0;index<17;++index) { //원본 실제 가상 표/owner를 가진17개 raw 사제를 차례로 최초 등록한다.
        const auto sid=pool.Allocate(2);CHECK(sid.value!=0);auto raw=pool.AllocatedBytes(sid);SidListPut(raw,0,kPatchPriestVtable);raw[10]=kPriestType;raw[11]=4;raw[12]=1;raw[34]=1;raw[40]=0;created.push_back(sid.value);prefix.Prefix(sid,1);
        CHECK(lists.priests.count==std::min(index+1,16U));
    }
    CHECK(searched==17 && reserved==17 && state.priests.entries.empty() && lists.priests.entries.size()==16);CHECK(std::equal(lists.priests.Items().begin(),lists.priests.Items().end(),created.begin()));RawPriestLookup lookup(pool,state);CHECK(lookup.Find(1)==created.front());
    std::int32_t selection=7;const auto data=lists.priests.entries;ResetRegisteredStateSidLists(lists.registeredHeaders,[&](auto header) { lists.ResetCount(header,selection); });CHECK(lookup.Find(1)==0 && lists.priests.count==0 && lists.priests.entries==data && selection==0);
}

// 실제 다중선택의 원문30칸은 일반 fresh에도 유지되고 prepared/반납/Native count 정리에는 보존된다.
TEST_CASE(StateSidLists_SelectionThirtySlotsAcrossActualGameplayLeases) {
    ClientGameplayGlobals gameplay;auto& owner=gameplay.selection;CHECK(owner.multiple.size()==30 && owner.multipleCount==0);const auto* pointer=owner.multiple.data();owner.multiple[29]=0xfedcba98;owner.multipleCount=-7;
    { auto use=gameplay.AcquirePrepared();CHECK(owner.multiple.size()==30 && owner.multiple.data()==pointer && owner.multiple[29]==0xfedcba98 && owner.multipleCount==-7); } //준비 대여와 반납은 원문 정리를 중복 수행하지 않는다.
    CHECK(owner.multiple[29]==0xfedcba98 && owner.multipleCount==-7);{ auto use=gameplay.Acquire();CHECK(owner.multiple.size()==30 && owner.multiple.data()==pointer && owner.multipleCount==0 && std::all_of(owner.multiple.begin(),owner.multiple.end(),[](auto value) { return value==0; })); } //일반 fresh만 같은 고정 용량에 기존 새 입력 의미를 적용한다.
    RawStateSidLists lists;owner.multiple[29]=0x89abcdef;owner.multipleCount=29;ResetRegisteredStateSidLists(lists.registeredHeaders,[&](auto header) { lists.ResetCount(header,owner.multipleCount); });CHECK(owner.multiple.size()==30 && owner.multiple.data()==pointer && owner.multiple[29]==0x89abcdef && owner.multipleCount==0);
}
