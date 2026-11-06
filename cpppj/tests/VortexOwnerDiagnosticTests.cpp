// 실제 두 PE의 missingIsland 접두와 상세/진단 장치 순서를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawVortexOwner.h"
#include "o/RawVortexOwnerDiagnostic.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 실제 부모/ftol의96관측과 신전 필드 제거·상세 문자열·진단 위치를 대조한다.
TEST_CASE(VortexOwnerDiagnostic_IndependentNative96LocalInputs) {
    std::size_t count{}; // 최초 원문 관측을 실제로 재생한 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_VORTEX_OWNER_DIAGNOSTIC_FIXTURE).rows) { // MP/포획/희생/공격 없이 진단 진입까지만 확인한다.
        CHECK(row.size()==8);if (row.size()!=8) continue;const bool patch=row[0]=="originals";const auto args=Split(row[1],',');CHECK(args.size()==5);if (args.size()!=5) continue;
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? 0x519860U : 0x501de0U);raw[10]=123;raw[patch ? 34U : 32U]=static_cast<std::uint8_t>(Number(args[2]));Put(raw,14,Number(args[0]));Put(raw,18,Number(args[1]));
        const auto stride=patch ? 0xb4U : 0xacU,field=patch ? 0x7cU : 0x74U;std::vector<std::uint8_t> players(9*stride); // 같은 아홉 Player의 실제 물리 폭이다.
        for (std::size_t player=0;player<9;++player) { Put(players,player*stride+field,5);Put(players,player*stride+field+4,0x12345678); } // Native와 같은 입력 두DWORD만 채운다.
        std::vector<RiftTypeRecord> types(patch ? 188U : 171U);SquidPostPopState book;SquidOwnerMode mode;SquidOwner base(pool,types,book,mode);SquidPostPopList outposts;std::uint32_t revision{};std::string detail;std::size_t details{},assertions{};
        VortexOwnerHooks hooks;hooks.islandNumber=[](float,float) { return 7; };hooks.islandExists=[](std::int32_t) { return false; };
        hooks.setIslandOwner=[](auto,auto) { CHECK(false); };hooks.regionOwnership=[](auto,auto) { CHECK(false); };hooks.refreshProduction=[] { CHECK(false); };
        hooks.missingIsland=[&](Sid sid,std::int32_t island,std::uint32_t owner) {
            ReportVortexOwnerMissingIsland(pool,sid,island,owner,Number(args[4]),{
                [&](auto text) { ++details;detail=text; },
                [&](auto expression,auto file,auto line) { ++assertions;CHECK(details==1 && expression==row[3] && file==row[4] && std::to_string(line)==row[5]);throw std::string("diagnostic"); }});
        }; // 상세를 먼저 쓴 뒤 명시 진단 중단으로 원문 접두와 동일한 관측 경계를 만든다.
        RawVortexOwner owner(pool,base,players,outposts,revision,std::move(hooks));bool stopped=false;try { owner.Set(Sid{5},Number(args[3])); }catch (const std::string&) { stopped=true; }
        std::string fields;for (std::size_t player=0;player<9;++player) for (std::size_t part=0;part<2;++part) { if (!fields.empty()) fields+=',';fields+=std::to_string(Get(players,player*stride+field+4*part)); } // 이전 Player만 지워졌는지 실제18필드를 읽는다.
        CHECK(stopped && details==1 && assertions==1 && Hex(std::span(reinterpret_cast<const std::uint8_t*>(detail.data()),detail.size()))==row[2] && Hex(raw)==row[6] && fields==row[7]);++count;
    }
    CHECK(count==96);
}

// 진단 장치가 정상 복귀하면 helper도 복귀하며, 누락된 진단은 앞선 상세를 보존한다.
TEST_CASE(VortexOwnerDiagnostic_ResumesAndKeepsDetailBeforeMissingAssertion) {
    SidPool pool(OriginalEdition::Patch1078,32768);std::string detail;std::uint32_t calls{}; // 독립 장치가 기록한 실제 상세/복귀 수다.
    ReportVortexOwnerMissingIsland(pool,Sid{5},7,2,5,{[&](auto text) { detail=text; },[&](auto,auto,auto) { ++calls; }});CHECK(!detail.empty() && calls==1);
    detail.clear();bool stopped=false;try { ReportVortexOwnerMissingIsland(pool,Sid{5},7,2,5,{[&](auto text) { detail=text; },{}}); }catch (const std::logic_error&) { stopped=true; }CHECK(stopped && !detail.empty() && calls==1);
}
