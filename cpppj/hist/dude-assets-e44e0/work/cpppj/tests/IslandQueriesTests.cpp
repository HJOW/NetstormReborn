// 원본두PE의독립섬검색반환과전체3204BYTE 불변을같은mutable목록에서대조한다.
#include "RawSceneSupport.h"
#include "o/RawIslandQueries.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 저장된원본검색결과만읽는다. C++로원본기대검색값을재계산하지않는다.
const std::vector<std::vector<std::string>>& IslandQueryRows() {
    static const auto rows=LoadFixture(NETSTORM_ISLANDQUERIES_FIXTURE).rows;return rows;
}
// 실제월드의단일3204BYTE/CoreData 모드와기존runtime에검색몸체를연결하는입력장면이다.
struct IslandQueryScene {
    SidPool pool; // runtime의판본과실제풀동일성을제공한다. 검색에서는슬롯을생성하지않는다.
    SquidHash hash; // 기존runtime의같은hash0이며검색에서는공간조회를하지않는다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // 월드가장기소유하는실제mutable100기록이다.
    FortFinishOwnerState modes; // 원본fort/special/chaff의같은현재자료다.
    SquidPostPopList residences; // owner setter에필요한같은목록이며세검색은읽지않는다.
    std::uint32_t revision{},changed{}; // 기존runtime의공유전역이며세검색에서변경하지않는다.
    RawIslandRuntime runtime; // 완료된count/Exists/현재owner getter를제공한다.
    RawIslandQueries queries; // 위runtime/현재모드를참조하는세복원검색이다.
    // 검색중가상owner/진단호출은원본몸체에없으므로뜻밖의효과가발생하면검사를실패시킨다.
    static IslandRuntimeHooks Hooks() {
        return {[](Sid,std::uint32_t) { throw std::runtime_error("IslandQueries 예상밖가상owner"); },
            [](IslandRuntimeAssertion) { throw std::runtime_error("IslandQueries 예상밖진단"); }};
    }
    // 같은풀/기록/모드수명을검색과runtime보다길게유지한다.
    explicit IslandQueryScene(OriginalEdition edition)
        :pool(edition,kCapacity,false),runtime(pool,hash,records,modes,residences,revision,changed,Hooks()),queries(runtime,modes) {}
    // 명시원본입력만구성하며검색반환/일치수는계산하지않는다.
    void Prepare(const std::vector<std::uint32_t>& input) {
        CHECK(input.size()==12);records.fill(0xa5);Put(records,0,input[0]);
        // 실제100기록의신전/존재/currentDWORD/원래BYTE/territoryBYTE와padding을각각배치한다.
        for (std::size_t index=0;index<100;++index) {
            const auto record=4+index*32;Put(records,record,static_cast<std::uint32_t>(0x11110000+index));Put(records,record+4,0);Put(records,record+8,input[3]);
            Put(records,record+12,input[5],1);Put(records,record+13,input[4],1);
            if (input[1]==1) Put(records,record+4,index%3 ? input[2] : 0);
            if (input[1]==2) {
                Put(records,record+4,index==0 || index==2 || index==99 ? input[2] : 0);
                if (index%2) { Put(records,record+8,input[3]^0x01000100U);Put(records,record+13,input[4]^0x80U,1);Put(records,record+12,input[5]^0x80U,1); }
            }
        }
        Put(records,4+input[11]*32+4,input[2]);modes.fort=input[8]!=0;modes.special=input[9]!=0;modes.chaffMode=input[10]!=0;
    }
};
// 양판본각4245개검색과전체raw/세모드/revision 불변을비교한다.
void ReplayIslandQueries(const char* edition) {
    IslandQueryScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 원본에서독립실행한모든검색입력을같은단일runtime에전달한다.
    for (const auto& row:IslandQueryRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==6);std::vector<std::uint32_t> input;
        // 실행기에전달한DWORD비트를그대로읽는다.
        for (const auto& number:Split(row[2],',')) input.push_back(Number(number));
        scene.Prepare(input);const auto before=scene.records;std::uint32_t result=0;
        if (row[1]=="FindOwnerTerritory") result=scene.queries.FindOwnerTerritory(std::bit_cast<std::int32_t>(input[6]),std::bit_cast<std::int32_t>(input[7]));
        else if (row[1]=="FindTerritory") result=scene.queries.FindTerritory(std::bit_cast<std::int32_t>(input[7]));
        else if (row[1]=="CountOwned") result=scene.queries.CountOwned(input[6]);
        else CHECK(false);
        const auto modes=std::to_string(scene.modes.fort)+','+std::to_string(scene.modes.special)+','+std::to_string(scene.modes.chaffMode);
        const bool same=result==Number(row[3]) && scene.records==before && Adler(scene.records)==Number(row[4]) && modes==row[5] && !scene.revision && !scene.changed;
        if (!same) std::fprintf(stderr,"IslandQueries %s %s input=%s result=%u expected=%s\n",edition,row[1].c_str(),row[2].c_str(),result,row[3].c_str());
        CHECK(same);++count;
    }
    CHECK(count==4245);
}
}
// 10.78의세원본검색4245개를전수비교한다.
TEST_CASE(IslandQueries_ReplaysPatch) { ReplayIslandQueries("originals"); }
// CD10.72의실제Exists/validId/Owner호출을포함한4245개를전수비교한다.
TEST_CASE(IslandQueries_ReplaysCd) { ReplayIslandQueries("originalCD"); }
// 실제Add/Delete/Reset과현재fort가다음검색에즉시반영되고물리100기록밖접근을거부하는지확인한다.
TEST_CASE(IslandQueries_ReadsLiveRecordsAndRejectsPhysicalOverflow) {
    // 두판본의같은3204BYTE mutable자료를복사벡터없이재사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        IslandQueryScene scene(edition);scene.runtime.Reset();CHECK(scene.queries.FindTerritory(2)==127);CHECK(scene.queries.CountOwned(0)==0);
        CHECK(scene.runtime.Add(1,2)==0);CHECK(scene.runtime.Add(2,2)==1);CHECK(scene.queries.CountOwned(1)==1);CHECK(scene.queries.FindOwnerTerritory(1,2)==0);
        Put(scene.records,4+8,2);CHECK(scene.queries.FindOwnerTerritory(1,2)==127);CHECK(scene.queries.CountOwned(2)==2);
        scene.modes.fort=true;Put(scene.records,4+8,0);CHECK(scene.queries.FindOwnerTerritory(7,2)==0);CHECK(scene.queries.CountOwned(0)==1);
        scene.runtime.Delete(0);CHECK(scene.runtime.Count()==2);CHECK(scene.queries.FindTerritory(2)==1);
        scene.runtime.Reset();CHECK(scene.queries.FindOwnerTerritory(0,2)==127);CHECK(scene.queries.CountOwned(0)==0);
        Put(scene.records,0,0xffffffffU);CHECK(scene.queries.FindTerritory(2)==127);CHECK(scene.queries.CountOwned(0)==0);
        Put(scene.records,0,101);CHECK(Throws([&] { scene.queries.CountOwned(0); }));
    }
}
