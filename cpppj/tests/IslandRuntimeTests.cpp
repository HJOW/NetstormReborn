// 실제 두PE의 mutable100기록/owner/Player첫지역과 callback뒤현재목록을 동일raw저장소에서 비교한다.
#include "RawSceneSupport.h"
#include "o/RawIslandRuntime.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원본실행기에 준 네float비트와 레지던스 현재좌표 입력이다.
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
    SidPool pool; // 현재 레지던스와 표면의 단일 raw 풀이다.
    SquidHash hash; // 기존 RawPathSurfaceAt이 사용하는 현재 hash0다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // CoreData와 같은3204BYTE mutable목록이다.
    FortFinishOwnerState modes; // 기존fort/special/chaff/Player첫지역을 공유한다.
    SquidPostPopList residences; // 실제 AssetPostPop의 등록목록이다.
    std::uint32_t revision{},changed{}; // 원본의 두 독립 DWORD다.
    std::vector<std::uint32_t> input; // 실행기에 전달한15개 명시 입력이다.
    std::vector<std::string> events; // 원본assert/레지던스 가상owner의 효과 순서다.
    std::uint32_t assertions{}; // 원본진단 callback의 호출 수다.
    bool patch; // 현재 raw ownerBYTE의 판본 배치다.
    std::unique_ptr<RawIslandRuntime> runtime; // 위 단일 자료에서 실행하는 복원 몸체다.
    // 원본입력 SID를 미리 client 정책으로 확보한다.
    explicit IslandRuntimeScene(OriginalEdition edition):pool(edition,kCapacity,false),patch(edition==OriginalEdition::Patch1078) {
        // 레지던스/표면의 명시 공간번호를 모두 확보한다.
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
            if (input[14]==6) modes.playerRegions[input[6]]=5;
        };
        return hooks;
    }
    // 기대값 계산 없이 입력record의 실제필드 폭/두 종류owner와 기존목록을 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        input.clear();events.clear();assertions=0;hash.Reset();modes={};
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
void ReplayIslandRuntime(const char* edition) {
    IslandRuntimeScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 각 독립 native입력을 복원 몸체에 전달한다.
    for (const auto& row:RuntimeRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==15);scene.Prepare(row);const auto island=std::bit_cast<std::int32_t>(scene.input[1]);std::uint32_t result=0;std::string bounds="-";
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
        for (const auto region:scene.modes.playerRegions) { if (!players.empty()) players+=',';players+=std::to_string(static_cast<std::uint32_t>(region)); }
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
// 물리100기록경계/필수효과를원본의정상반환으로꾸미지않고명시적으로거부한다.
TEST_CASE(IslandRuntime_RejectsMissingEffectsAndPhysicalOverflow) {
    IslandRuntimeScene scene(OriginalEdition::Patch1078);scene.Prepare(RuntimeRows().front());Put(scene.records,0,100);
    CHECK(Throws([&scene] { scene.runtime->Add(1,2); }));CHECK(scene.runtime->Count()==100);
    CHECK(Throws([&scene] { RawIslandRuntime missing(scene.pool,scene.hash,scene.records,scene.modes,scene.residences,scene.revision,scene.changed,{}); }));
}
