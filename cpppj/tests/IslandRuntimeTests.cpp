// 실제 두PE의 mutable100기록/owner/Player첫지역과 callback뒤현재목록을 동일raw저장소에서 비교한다.
#include "RawSceneSupport.h"
#include "o/RawIslandRuntime.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include <cstdio>
#include <utility>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원본실행기에 준 네float비트와 거주 구역 현재좌표 입력이다.
constexpr std::array<std::array<std::uint32_t,4>,9> kRuntimeBounds{{
    {0x41200000,0x41a00000,0x42700000,0x428c0000},{0,0,0x42700000,0x428c0000},
    {0xc0000000,0x41a00000,0x42700000,0x428c0000},{0x42480000,0x42540000,0x42480000,0x42540000},
    {0x7fc00001,0x41a00000,0x42700000,0x428c0000},{0x41200000,0x7fc00001,0x42700000,0x428c0000},
    {0x41200000,0x41a00000,0x42700000,0x7fc00001},{0x41200000,0x41a00000,0x7fc00001,0x428c0000},
    {0x41200000,0x41a00000,0x43800000,0x428c0000}}};
constexpr std::array<std::array<float,2>,4> kRuntimePoints{{{10,20},{50.75f,52.5f},{60,70},{61,71}}};
// 독립 fixture의 기대값을 다시 계산하지 않고 읽는다.
const std::vector<std::vector<std::string>>& RuntimeRows() {
    static const auto rows=LoadFixture(NETSTORM_ISLANDRUNTIME_FIXTURE).rows;return rows;
}
// 단일 raw pool/hash/record와 기존모드/Player/등록목록의 입력 장면이다.
struct IslandRuntimeScene {
    SidPool pool; // 현재 거주 구역과 표면의 단일 raw 풀이다.
    SquidHash hash; // 기존 RawPathSurfaceAt이 사용하는 현재 hash0다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // CoreData와 같은3204BYTE mutable목록이다.
    FortFinishOwnerState modes; // 기존fort/special/chaff/Player첫지역을 공유한다.
    SquidPostPopList residences; // 실제 AssetPostPop의 등록목록이다.
    std::uint32_t revision{},changed{}; // 원본의 두 독립 DWORD다.
    std::vector<std::uint32_t> input; // 실행기에 전달한15개 명시 입력이다.
    std::vector<std::string> events; // 원본assert/거주 구역 가상owner의 효과 순서다.
    std::uint32_t assertions{}; // 원본진단 callback의 호출 수다.
    bool patch; // 현재 raw ownerBYTE의 판본 배치다.
    std::unique_ptr<RawIslandRuntime> runtime; // 위 단일 자료에서 실행하는 복원 몸체다.
    std::vector<std::uint8_t> players; // 공유모드에서 원본 첫 지역을 저장하는 실제9 Player BYTE다.
    // 원본입력 SID를 미리 client 정책으로 확보한다.
    explicit IslandRuntimeScene(OriginalEdition edition):pool(edition,kCapacity,false),patch(edition==OriginalEdition::Patch1078) {
        // 거주 구역/표면의 명시 공간번호를 모두 확보한다.
        for (std::uint16_t sid=5;sid<=127;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 가상owner의 실제raw 효과와 callback 직후 현재목록/모드/Player 변화를 관찰한다.
    IslandRuntimeHooks Hooks() {
        IslandRuntimeHooks hooks;
        hooks.assertion=[this](IslandRuntimeAssertion assertion) { ++assertions;events.push_back(assertion==IslandRuntimeAssertion::InvalidBounds ? "A:B" : "A:E"); };
        hooks.setResidenceOwner=[this](Sid sid,std::uint32_t owner) {
            pool.AllocatedBytes(sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(owner);events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner));
            if (sid.value!=60) return;
            if (input[14]==1) residences.entries[1]=63;
            if (input[14]==2) residences.count=1;
            if (input[14]==3) hash.Cell(0,51,53)=72;
            if (input[14]==4) { modes.fort=true;modes.special=false; }
            if (input[14]==6) {
                if (players.empty()) modes.playerRegions[input[6]]=5;
                else Put(players,input[6]*(patch ? 0xb4U : 0xacU)+(patch ? 0x78U : 0x70U),5);
            }
        };
        return hooks;
    }
    // 기대값 계산 없이 입력record의 실제필드 폭/두 종류owner와 기존목록을 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        input.clear();events.clear();players.clear();assertions=0;hash.Reset();modes={};
        // 원본DWORD의 비트를 그대로 읽는다.
        for (const auto& number:Split(row[2],',')) input.push_back(Number(number));
        CHECK(input.size()==15);records.fill(static_cast<std::uint8_t>(input[6]));Put(records,0,input[0]);
        // 100개 모두에 원본record/type/data 입력을 배치한다.
        for (std::size_t index=0;index<100;++index) {
            const auto record=4+index*32;Put(records,record,static_cast<std::uint32_t>(0x11110000+index));Put(records,record+4,static_cast<std::uint32_t>(index<input[0]));Put(records,record+8,input[3]);
            Put(records,record+12,input[5],1);Put(records,record+13,input[4],1);
            // 네 float는 실행기에 전달한 비트를 그대로 저장한다.
            for (std::size_t field=0;field<4;++field) Put(records,record+16+field*4,kRuntimeBounds[input[13]][field]);
        }
        if (input[1]<100) Put(records,4+input[1]*32+4,input[2]);
        modes.fort=input[10]!=0;modes.special=input[8]!=0;modes.chaffMode=input[9]!=0;revision=input[11];changed=input[12];
        residences.entries={60,61,62,63};residences.count=4;
        // 행을 재사용해도 raw의 이전 입력이 남지 않게 명시 공간을 초기화한다.
        for (std::uint16_t sid=5;sid<=127;++sid) { auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        // 실제 pool/hash와 기존표면 helper에 동일 type/float/표면owner 입력을 전달한다.
        for (std::size_t index=0;index<4;++index) {
            const auto sid=Sid{static_cast<std::uint16_t>(60+index)};auto raw=pool.AllocatedBytes(sid);raw[10]=89;raw[patch ? 34 : 32]=7;
            Put(raw,14,std::bit_cast<std::uint32_t>(kRuntimePoints[index][0]));Put(raw,18,std::bit_cast<std::uint32_t>(kRuntimePoints[index][1]));
            const auto support=Sid{static_cast<std::uint16_t>(70+std::min<std::size_t>(index,2))};
            pool.AllocatedBytes(support)[patch ? 34 : 32]=std::array<std::uint8_t,3>{2,3,8}[std::min<std::size_t>(index,2)];
            const auto x=static_cast<int>(static_cast<double>(kRuntimePoints[index][0])+static_cast<double>(0.9999f)),y=static_cast<int>(static_cast<double>(kRuntimePoints[index][1])+static_cast<double>(0.9999f));
            hash.Cell(0,x,y)=support.value;
        }
        runtime=std::make_unique<RawIslandRuntime>(pool,hash,records,modes,residences,revision,changed,Hooks());
    }
};
// 원본scalar/전체3204raw/출력bounds/공유전역/등록목록/가상owner를 전수 비교한다.
void ReplayIslandRuntime(const char* edition,bool sharedPlayers=false) {
    IslandRuntimeScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 각 독립 native입력을 복원 몸체에 전달한다.
    for (const auto& row:RuntimeRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==15);scene.Prepare(row);const auto island=std::bit_cast<std::int32_t>(scene.input[1]);std::uint32_t result=0;std::string bounds="-";
        if (sharedPlayers) {
            const auto stride=scene.patch ? 0xb4U : 0xacU,field=scene.patch ? 0x78U : 0x70U; // 원본 Player 첫 지역DWORD의 위치다.
            scene.players.assign(9*stride,0xa5);
            // 기대값 대신 기존 입력상태의 아홉 지역 필드만 준비한다.
            for (std::size_t owner=0;owner<9;++owner) Put(scene.players,owner*stride+field,static_cast<std::uint32_t>(scene.modes.playerRegions[owner]));
            scene.runtime->BindPlayerRecords(scene.players);scene.modes.playerRegions.fill(999);
        }
        const auto& kind=row[1];
        if (kind=="Reset") scene.runtime->Reset();
        else if (kind=="Count") result=scene.runtime->Count();
        else if (kind=="Exists") result=scene.runtime->Exists(island);
        else if (kind=="Add") result=scene.runtime->Add(scene.input[6],static_cast<std::uint8_t>(scene.input[5]));
        else if (kind=="Delete") scene.runtime->Delete(island);
        else if (kind=="OwnerGet") result=scene.runtime->Owner(island);
        else if (kind=="OwnerSet") scene.runtime->Owner(island,scene.input[6]);
        else if (kind=="TempleGet") result=scene.runtime->Temple(island);
        else if (kind=="TempleSet") scene.runtime->Temple(island,scene.input[6]);
        else if (kind=="Match") result=scene.runtime->Matches(island,std::bit_cast<std::int32_t>(scene.input[6]),std::bit_cast<std::int32_t>(scene.input[7]));
        else if (kind=="GetBounds") {
            const auto value=scene.runtime->Bounds(island);std::array<std::uint8_t,16> raw{};const std::array fields{value.left,value.top,value.right,value.bottom};
            // 실제로 반환한 float비트를 읽어 비교한다.
            for (std::size_t field=0;field<4;++field) Put(raw,field*4,std::bit_cast<std::uint32_t>(fields[field]));bounds=Hex(raw);
        } else if (kind=="SetBounds") {
            const auto& value=kRuntimeBounds[scene.input[13]];scene.runtime->Bounds(island,{std::bit_cast<float>(value[0]),std::bit_cast<float>(value[1]),std::bit_cast<float>(value[2]),std::bit_cast<float>(value[3])});
        }
        std::string players,effects,entries,owners;
        // 현재 Player의 첫 지역을 읽는다.
        for (std::size_t owner=0;owner<9;++owner) {
            if (!players.empty()) players+=',';
            players+=std::to_string(sharedPlayers ? Get(scene.players,owner*(scene.patch ? 0xb4U : 0xacU)+(scene.patch ? 0x78U : 0x70U)) : static_cast<std::uint32_t>(scene.modes.playerRegions[owner]));
        }
        if (sharedPlayers) {
            // 첫 지역 네BYTE 외의 Player 전부는 그대로 유지되어야 한다.
            for (std::size_t index=0;index<scene.players.size();++index) {
                const auto offset=index%(scene.patch ? 0xb4U : 0xacU); // 현재 기록 안의 BYTE 위치다.
                const auto field=scene.patch ? 0x78U : 0x70U; // 현재 판본의 첫 지역 시작 주소다.
                if (offset<field || offset>=field+4) CHECK(scene.players[index]==0xa5);
            }
            CHECK(std::ranges::all_of(scene.modes.playerRegions,[](std::int32_t value) { return value==999; }));
        }
        // callback을 포함한 실제효과 순서를 저장한다.
        for (const auto& event:scene.events) { if (!effects.empty()) effects+=';';effects+=event; }if (effects.empty()) effects="-";
        // 첫4개 현재등록 SID와 실제ownerBYTE를 비교한다.
        for (std::size_t index=0;index<4;++index) {
            if (index) { entries+=',';owners+=','; }entries+=std::to_string(scene.residences.entries[index]);owners+=std::to_string(scene.pool.Slot(Sid{static_cast<std::uint16_t>(60+index)})[scene.patch ? 34 : 32]);
        }
        const auto modes=std::to_string(scene.modes.fort)+','+std::to_string(scene.modes.special)+','+std::to_string(scene.modes.chaffMode);
        const bool same=result==Number(row[3]) && Hex(scene.records)==row[4] && bounds==row[5] && scene.revision==Number(row[6]) && scene.changed==Number(row[7]) && players==row[8] && effects==row[9] &&
            scene.residences.count==Number(row[10]) && entries==row[11] && owners==row[12] && modes==row[13] && scene.assertions==Number(row[14]);
        CHECK(same);++count;
        if (!same) { std::printf("%s IslandRuntime 행%zu %s %s\n기대 result/rev/changed %s/%s/%s effects %s owners %s\n실제 %u/%u/%u effects %s owners %s\n",
            edition,count,kind.c_str(),row[2].c_str(),row[3].c_str(),row[6].c_str(),row[7].c_str(),row[9].c_str(),row[12].c_str(),result,scene.revision,scene.changed,effects.c_str(),owners.c_str());break; }
    }
    CHECK(count==1009);
}
}
// Patch의원래owner/현재owner와100기록의field별초기화/진단을actual출력과비교한다.
TEST_CASE(IslandRuntime_ReplaysPatch) { ReplayIslandRuntime("originals"); }
// CD의조용한getter/setter와boundsunordered/owner전파차이를actual출력과비교한다.
TEST_CASE(IslandRuntime_ReplaysCd) { ReplayIslandRuntime("originalCD"); }
// 기존 패치 원본1009입력을 실제 Player BYTE와 효과 뒤 현재값으로 재생한다.
TEST_CASE(IslandRuntime_PatchSharedPlayerRecords) { ReplayIslandRuntime("originals",true); }
// CD의 첫 지역 위치와 주변 바이트를 같은 기존 원본1009입력으로 검증한다.
TEST_CASE(IslandRuntime_CdSharedPlayerRecords) { ReplayIslandRuntime("originalCD",true); }
// 기존 양판본 Native Reset6행의 전체3204BYTE를 Session 없는 같은 공통 helper와 비교한다.
TEST_CASE(IslandRuntime_DataOnlyResetMatchesExistingNativeBothEditions) {
    std::size_t count{}; // 실제 기존 fixture에서 대조한 Reset 행 수다.
    for (const auto& row:RuntimeRows()) { // 새 기대값이나 fixture 행을 생성하지 않는다.
        if (row[1]!="Reset") continue;
        IslandRuntimeScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);scene.Prepare(row);
        const auto revision=scene.revision,changed=scene.changed;const auto entries=scene.residences.entries; // reset과 무관한 현재 전역/목록의 입력이다.
        ResetRawIslandRecords(scene.records);
        CHECK(Hex(scene.records)==row[4] && scene.revision==revision && scene.changed==changed && scene.residences.entries==entries && scene.events.empty());++count;
    }
    CHECK(count==6);
}
// 별도3204BYTE 장기 배열을 실제 실행기가 계속 읽고 쓰며 잘못된 helper 폭은 첫 쓰기 전에 거부한다.
TEST_CASE(IslandRuntime_DataOnlyStorageAliasesRuntimeAndRejectsWrongWidth) {
    RawIslandStorage storage;storage.records.fill(0xa5);IslandRuntimeScene scene(OriginalEdition::Patch1078); // padding의 비영 입력도 실제 장기 배열에 둔다.
    RawIslandRuntime runtime(scene.pool,scene.hash,storage.records,scene.modes,scene.residences,scene.revision,scene.changed,scene.Hooks());
    ResetRawIslandRecords(storage.records);CHECK(runtime.Count()==0 && storage.records[18]==0xa5 && storage.records[19]==0xa5);
    CHECK(runtime.Add(3,7)==0 && Get(storage.records,0)==1 && Get(storage.records,12)==3 && storage.records[16]==7 && storage.records[17]==3);
    Put(storage.records,4,0x12345678);CHECK(runtime.Temple(0)==0x12345678);runtime.Reset();CHECK(Get(storage.records,0)==0 && Get(storage.records,4)==0 && storage.records[18]==0xa5 && storage.records[19]==0xa5);
    const auto before=storage.records;bool rejected{}; // 범위가 하나 모자란 입력에서 부분 초기화가 없어야 한다.
    try { ResetRawIslandRecords(std::span(storage.records).first(RawIslandStorage::kBytes-1)); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && storage.records==before);
}
// 물리100기록경계/필수효과를원본의정상반환으로꾸미지않고명시적으로거부한다.
TEST_CASE(IslandRuntime_RejectsMissingEffectsAndPhysicalOverflow) {
    IslandRuntimeScene scene(OriginalEdition::Patch1078);scene.Prepare(RuntimeRows().front());Put(scene.records,0,100);
    CHECK(Throws([&scene] { scene.runtime->Add(1,2); }));CHECK(scene.runtime->Count()==100);
    CHECK(Throws([&scene] { RawIslandRuntime missing(scene.pool,scene.hash,scene.records,scene.modes,scene.residences,scene.revision,scene.changed,{}); }));
}

// 실제0046ef10의0059a8b0 증가와005949d4의1 저장을 서로 다른 Client owner에서 관측한다.
TEST_CASE(IslandRuntime_ActualClientOwnersKeepWorldIncrementAndMapStoreSeparate) {
    netstorm::client::ClientKencloudGlobals cloud;netstorm::client::ClientGameplayGlobals gameplay;IslandRuntimeScene scene(OriginalEdition::Patch1078);
    auto residence=scene.pool.AllocatedBytes(Sid{60});std::fill(residence.begin(),residence.end(),std::uint8_t{});residence[10]=89;residence[34]=7;
    Put(residence,14,std::bit_cast<std::uint32_t>(50.75F));Put(residence,18,std::bit_cast<std::uint32_t>(52.5F));scene.pool.AllocatedBytes(Sid{70})[34]=2;scene.hash.Cell(0,51,53)=70;
    scene.residences.entries={60};scene.residences.count=1;std::uint32_t notices{},assertions{},expectedWorld{18},expectedOwner{3};bool mutateDuringNotice{}; // 실제 거주 구역과 관문 뒤 관찰 입력이다.
    IslandRuntimeHooks hooks;
    hooks.assertion=[&](IslandRuntimeAssertion reason) {
        CHECK(reason==IslandRuntimeAssertion::MissingIsland);++assertions;cloud.WorldRedraw()=40;gameplay.MapRevision()=0x76543210U;Put(scene.records,8,1); // 복귀 진단이 같은 존재/전역을 바꾸면 본문은 그 현재값에서 계속한다.
    };
    hooks.setResidenceOwner=[&](Sid sid,std::uint32_t owner) {
        CHECK(sid==Sid{60} && owner==2 && cloud.WorldRedraw()==expectedWorld && gameplay.MapRevision()==1 && Get(scene.records,12)==expectedOwner);++notices;scene.pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner);
        if (mutateDuringNotice) { cloud.WorldRedraw()=700;gameplay.MapRevision()=99; } // 원문 전역 쓰기 뒤의 실제 가상 효과를 반환 후 사본으로 덮지 않아야 한다.
    };
    RawIslandRuntime runtime(scene.pool,scene.hash,scene.records,scene.modes,scene.residences,cloud.WorldRedraw(),gameplay.MapRevision(),std::move(hooks));runtime.Reset();CHECK(runtime.Add(1,0)==0);runtime.Bounds(0,{10,20,60,70});
    cloud.WorldRedraw()=17;gameplay.MapRevision()=0xffffffffU;runtime.Owner(0,3);
    CHECK(cloud.WorldRedraw()==18 && gameplay.MapRevision()==1 && runtime.Owner(0)==3 && scene.pool.Slot(Sid{60})[34]==2 && notices==1 && assertions==0);
    // 진단의 현재 존재/전역 변경을 재조회한 뒤 같은1 저장을 먼저 수행하고 가상 효과의 후속 변경은 유지한다.
    Put(scene.records,8,0);cloud.WorldRedraw()=17;gameplay.MapRevision()=0xffffffffU;expectedWorld=41;expectedOwner=4;mutateDuringNotice=true;runtime.Owner(0,4);
    CHECK(cloud.WorldRedraw()==700 && gameplay.MapRevision()==99 && runtime.Owner(0)==4 && notices==2 && assertions==1);
    // unsigned 감김은 증가하는 세계 DWORD에만 적용하며 미니맵 DWORD는 여전히1을 직접 저장한다.
    cloud.WorldRedraw()=0xffffffffU;gameplay.MapRevision()=0xffffffffU;expectedWorld=0;expectedOwner=5;mutateDuringNotice=false;runtime.Owner(0,5);
    CHECK(cloud.WorldRedraw()==0 && gameplay.MapRevision()==1 && runtime.Owner(0)==5 && notices==3 && assertions==1);
}
