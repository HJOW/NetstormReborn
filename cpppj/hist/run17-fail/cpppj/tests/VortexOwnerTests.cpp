// 신전 가상 Owner의 Player 기록·가변 목록·현재 좌표·공통 Owner를 두 PE 출력에 대조한다.
#include "RawSceneSupport.h"
#include "DudeSpaceSupport.h"
#include "client/RawCampaignOwnership.h"
#include "o/RawVortexOwner.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 기록 번호와 실제 두 가상 표는 원본 실행기의 동일 입력이다.
constexpr std::uint32_t kVortexPatch=0x519860,kVortexCd=0x501de0;
// 독립 PE에서 저장한 입력과 기대값만 읽는다.
const auto& Rows() { static const auto rows=LoadFixture(NETSTORM_VORTEXOWNER_FIXTURE).rows;return rows; }
// 현재 pool/9 Player/소유자별 목록을 원본과 같은 초기 입력으로 준비한다.
struct VortexScene {
    SidPool pool; // 명시 SID50/60/61/62를 같은 저장소에 확보한다.
    std::vector<RiftTypeRecord> types; // 원본 공통 Owner가 읽는 현재 vortex genus다.
    SquidPostPopState book; // 실제 소유자별 목록의 capacity/count/stale 데이터를 공유한다.
    SquidOwnerMode mode; // 실제 전투1/요새0/challenge0 조건이다.
    std::unique_ptr<SquidOwner> owner; // 대체 함수 없이 기존 실제 공통 Owner를 사용한다.
    std::vector<std::uint8_t> players; // 신전 후처리와 공유하는 실제9 Player BYTE다.
    SquidPostPopList outposts; // 패치의 매반복 현재count/항목이다.
    std::uint32_t revision{37},paints{}; // 현재DWORD와 첫 외부 도장 호출 수다.
    std::vector<std::string> events; // 원본 외부 효과의 실제 인자 순서다.
    std::vector<std::uint32_t> input; // 독립 원본 실행기에 공급한6개 입력이다.
    bool patch; // 현재 판본의 필드/목록/revision 분배 조건이다.
    // 풀 번호와 공통 모듈을 실제 입력에서 구성한다.
    VortexScene(OriginalEdition edition,const std::vector<std::string>& row)
        :pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188U : 171U),
         players(9*(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU),0xa5),patch(edition==OriginalEdition::Patch1078) {
        // 이전 raw 관찰과 같은 명시 번호를 확보한다.
        for (std::uint16_t sid=5;sid<128;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 기대값 대신 입력 필드만 읽는다.
        for (const auto& value:Split(row[1],',')) input.push_back(Number(value));
        CHECK(input.size()==6);types[82].flags2=0x200;mode.battle=true;
        // 기존 공통 Owner가 쓰는9개의 실제 목록을 같은 입력으로 준비한다.
        for (auto& list:book.ownerFactories) { list.entries={1,2,3,4,5,6};list.count=0; }
        owner=std::make_unique<SquidOwner>(pool,types,book,mode);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kVortexPatch : kVortexCd);raw[10]=82;raw[11]=static_cast<std::uint8_t>(input[2]);
        raw[OwnerOffset()]=static_cast<std::uint8_t>(input[0]);raw[ExtraOffset()]=static_cast<std::uint8_t>(input[3]);
        Put(raw,14,Bits(40.5f));Put(raw,18,Bits(50.25f));
        // 신전 네DWORD만 원본 입력으로 채우고 다른 Player BYTE는 유지한다.
        for (std::size_t player=0;player<9;++player) {
            Put(players,Temple(player),input[4]);Put(players,Temple(player)+4,0x11223344);
            Put(players,Temple(player)+8,0x55667788);Put(players,Temple(player)+12,0x99aabbcc);
        }
        outposts.entries={60,61,62};outposts.count=2;
        // 원본 추가목록의 현재owner·좌표 입력이다. free/dead 여부로 후보를 거르지 않는다.
        for (std::size_t index=0;index<3;++index) {
            auto other=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(60+index)});
            other[OwnerOffset()]=static_cast<std::uint8_t>(std::array<std::uint32_t,3>{input[0],input[1],8}[index]);
            Put(other,14,Bits(70.5f+static_cast<float>(index)));Put(other,18,Bits(80.25f+static_cast<float>(index)));
        }
    }
    // 실제 판본별 raw 필드 위치다.
    std::size_t OwnerOffset() const { return patch ? 34U : 32U; }
    std::size_t ExtraOffset() const { return patch ? 40U : 35U; }
    std::size_t Temple(std::size_t player) const { return player*(patch ? 0xb4U : 0xacU)+(patch ? 0x7cU : 0x74U); }
    // 원본과 같은 명시 외부 효과 변경을 공급하며 신전 몸체 결과는 계산하지 않는다.
    VortexOwnerHooks Hooks() {
        VortexOwnerHooks hooks;
        hooks.islandNumber=[](float,float) { return 3; };hooks.islandExists=[](std::int32_t) { return true; };
        hooks.setIslandOwner=[this](std::int32_t island,std::uint32_t player) {
            events.push_back("I:"+std::to_string(island)+':'+std::to_string(player));
            if (input[5]==1) { auto raw=pool.AllocatedBytes(kSource);raw[OwnerOffset()]=8;Put(raw,14,Bits(42.75f));Put(raw,18,Bits(60.5f)); }
        };
        hooks.regionOwnership=[this](float x,float y) {
            events.push_back("P:"+std::to_string(Bits(x))+':'+std::to_string(Bits(y)));
            if (paints==0) {
                if (input[5]==1 || input[5]==3) outposts.count=3;
                if (input[5]==2) { outposts.count=0;pool.AllocatedBytes(kSource)[ExtraOffset()]=1; }
                if (input[5]==3) pool.AllocatedBytes(kSource)[OwnerOffset()]=1;
            }
            ++paints;
        };
        hooks.refreshProduction=[this] { events.push_back("R");if (patch && input[5]==3) revision=0xffffffffU; };
        hooks.missingIsland=[](Sid,std::int32_t,std::uint32_t) { throw std::logic_error("이번 정상 PE 입력은 missingIsland를 요청하지 않습니다"); };
        return hooks;
    }
    // 실제 Player 필드·공통목록·raw 슬롯·revision을 원본 출력 형식으로 수집한다.
    std::vector<std::string> Output() const {
        std::string actions,temples,lists; // 원본과 대조할 실제 문자열이다.
        // 외부 callback의 순서를 그대로 기록한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        // 아홉 Player의 네 신전DWORD 모두를 읽는다.
        for (std::size_t player=0;player<9;++player) for (std::size_t field=0;field<4;++field) {
            if (!temples.empty()) temples+=',';temples+=std::to_string(Get(players,Temple(player)+field*4));
        }
        // 기존 실제 공통 Owner의 capacity 전체 stale 데이터도 대조한다.
        for (const auto& list:book.ownerFactories) {
            if (!lists.empty()) lists+='|';lists+=std::to_string(list.count)+':';
            // 활성 count 바깥을 포함해 원본 메모리 결과를 비교한다.
            for (std::size_t index=0;index<list.entries.size();++index) { if (index) lists+=',';lists+=std::to_string(list.entries[index]); }
        }
        return {actions,Hex(pool.Slot(kSource)),temples,lists,std::to_string(revision)};
    }
};
// 원본 내부 Base 호출표식은 독립 공통 Owner 결과로 확인하고 외부 콜백 순서와 구별한다.
std::string ExternalEvents(const std::string& value) {
    std::string result; // 외부 효과만 남긴 원본 관찰이다.
    // 내부 B표식은 원본 함수 호출 계수이며 외부 callback이 아니다.
    for (const auto& event:Split(value,';')) if (!event.starts_with("B:")) { if (!result.empty()) result+=';';result+=event; }
    return result;
}
// 구현 계산으로 기대값을 만들지 않고 같은 입력의 실제 두PE 결과를 전수 재생한다.
void Replay(const char* edition) {
    std::size_t count=0; // 해당 판본에서 비교한 고유 입력 수다.
    // 같은 실제 명령에서 나온 각 반환을 전체 raw/Player/목록으로 확인한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==7);
        VortexScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);
        RawVortexOwner vortex(scene.pool,*scene.owner,scene.players,scene.outposts,scene.revision,scene.Hooks());
        CHECK(vortex.Handles(kSource));vortex.Set(kSource,scene.input[1]);
        auto expected=std::vector<std::string>(row.begin()+2,row.end());expected[0]=ExternalEvents(expected[0]); // 실제 외부 경계 순서다.
        const auto output=scene.Output();CHECK(output==expected);++count;
        if (output!=expected) { std::printf("VortexOwner %s input%s 행%zu 불일치\n",edition,row[1].c_str(),count);break; }
    }
    CHECK(count==864);
}
}
// 패치의 현재 추가목록·revision·효과 뒤 owner/좌표 변경을 실제PE와 비교한다.
TEST_CASE(VortexOwner_PatchNativeCurrentPlayersAndLists) { Replay("originals"); }
// CD의 추가목록/revision 생략과 초기 신전DWORD 폭을 실제PE와 비교한다.
TEST_CASE(VortexOwner_CdNativeCurrentPlayersAndLists) { Replay("originalCD"); }
// 미복원 가상표/잘못된9기록은 최초 raw/Player 변경 전에 거부한다.
TEST_CASE(VortexOwner_RejectsForeignTableAndPlayerSize) {
    VortexScene scene(OriginalEdition::Patch1078,Rows().front());const auto before=scene.players;
    CHECK(Throws([&] { RawVortexOwner bad(scene.pool,*scene.owner,std::span<std::uint8_t>(scene.players).first(10),scene.outposts,scene.revision,scene.Hooks()); }));
    RawVortexOwner vortex(scene.pool,*scene.owner,scene.players,scene.outposts,scene.revision,scene.Hooks());Put(scene.pool.AllocatedBytes(kSource),0,0x11111111);
    CHECK(Throws([&] { vortex.Set(kSource,1); }) && scene.players==before && scene.events.empty());
}
// 실제 보호 TYPE/Factory의 신전 Owner와 섬 첫 지역을 동일9 Player 기록에서 공유한다.
TEST_CASE(VortexOwner_CampaignSameCoreAndFirstRegion) {
    using namespace netstorm::client;
    // 두 판본의 실제 신전 생성자/가상 표와 같은 풀을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& data=netstorm::test::dudespace::Assets(edition); // 실제 보호 자산이다.
        BridgeConnectState bridges;bridges.bridgeType=82;bridges.islandType=94;bridges.stalagType=95;bridges.connectorType=155;bridges.noIslandType=157;bridges.battle=true;
        RawSurfaceWorld world(edition,data.types,data.frames,data.shapes,bridges,97);auto core=world.Core();
        AssetPostPopState assets;assets.residences.entries.resize(64);assets.outposts.entries.resize(64);
        FortFinishOwnerState modes;std::array<std::uint8_t,RawIslandRuntime::kBytes> records{};
        std::uint32_t revision=0,changed=0,vortexRevision=0,refresh=0; // 실제 독립 전역들이다.
        RawIslandRuntime islands(core.pool,core.hash,records,modes,assets.residences,revision,changed,
            {[&](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); },[](IslandRuntimeAssertion) { throw std::logic_error("정상 섬 입력의 예상하지 않은 진단"); }});
        islands.Reset();CHECK(islands.Add(1,0)==0);islands.Bounds(0,{10,10,50,50});
        const auto stride=edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU; // 실제 기록 크기다.
        const auto region=edition==OriginalEdition::Patch1078 ? 0x78U : 0x70U,temple=region+4; // 서로 인접한 원본 두 필드다.
        std::vector<std::uint8_t> players(9*stride); // 모든 실제 Player 기능이 빌릴 동일 저장소다.
        // 최초 첫 지역은 원본 sentinel127이며 신전SID는0이다.
        for (std::size_t player=0;player<9;++player) Put(players,player*stride+region,127);
        std::vector<PriestPlainCanonType> frames(data.frames.size());std::vector<std::uint32_t> themes(data.types.size());
        // 불변 프레임 자료는 같은 보호 TYPE 번호 순서를 유지한다.
        for (std::size_t type=0;type<frames.size();++type) frames[type].frames=data.frames[type];
        GameRandom random(7);RegionPaintState paint;RegionOwnershipState ownership;
        RawCampaignOwnership binding(world,islands,assets,players,frames,themes,random,paint,ownership,vortexRevision,97,121,
            {[&](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); },[&] { ++refresh; },[] { return 1U; },
             [](Sid,std::int32_t,std::uint32_t) { throw std::logic_error("아직 void인 실제 신전은 지역 조회를 요청하지 않습니다"); }});
        const auto sid=world.Create(123); // 실제 윈드 신전 생성자가 쓰는 가상표다.
        CHECK(binding.Vortex().Records().data()==players.data() && &binding.Vortex().Pool()==&core.pool);
        binding.SetOwner(sid,1);CHECK(core.pool.Slot(sid)[edition==OriginalEdition::Patch1078 ? 34U : 32U]==1);
        CHECK(Get(players,stride+temple)==sid.value && Get(players,stride+temple+4)==sid.value && refresh==1);
        islands.Owner(0,2);CHECK(Get(players,2*stride+region)==0 && modes.playerRegions[2]==127);
        CHECK(Get(players,stride+temple)==sid.value && Get(players,stride+region)==127);
        world.ClearProcesses();
    }
}
