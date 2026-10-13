// Ascend의 기존 독립Count 입력을 실제3204바이트 IslandList와 같은풀에서 재사용한다.
#include "RawSceneSupport.h"
#include "o/RawIslandRuntime.h"
#include "o/RawVortexAscend.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제목록을 공유하는 Count 전용장면이다. 외부효과를 호출하면 입력계약 위반으로 실패한다.
struct LiveIslandScene {
    SidPool pool; // 섬과Ascend가 공유하는 실제client 풀이다.
    SquidHash hash; // 같은월드의 표면/공간해시다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // countDWORD와100개32바이트기록의 단일저장소다.
    FortFinishOwnerState modes; // 실제목록이 참조하는fort/Player 모드다.
    SquidPostPopList residences; // 현재레지던스 등록목록이며 Count는 수정하지 않는다.
    std::uint32_t revision{}; // owner변경revision DWORD다.
    std::uint32_t changed{}; // 패치의별도owner변경 DWORD다.
    std::vector<std::uint8_t> spots; // 실제65536칸 공유spot다.
    std::vector<RiftTypeRecord> types; // 현재판본의실제길이 타입표다.
    GameRandom random; // Count에서소비하지않는 게임난수다.
    VortexAscendState state; // Ascend의현재표면타입 전역이다.
    RawIslandRuntime islands; // 위3204바이트를복사없이참조하는 실제목록이다.
    RawVortexAscend ascend; // 같은pool/list에연결하는 생산Ascend다.
    // Count에서 호출될 수 없는 하위효과를 명시적인 오류로 공급한다.
    static VortexAscendHooks AscendHooks() {
        return {
            [](float,float)->std::int32_t { throw std::logic_error("Count의섬좌표효과 호출"); },
            []()->bool { throw std::logic_error("Count의후보확보 호출"); },
            [] { throw std::logic_error("Count의후보해제 호출"); },
            [](Sid,std::uint32_t) { throw std::logic_error("Count의가상삭제 호출"); }
        };
    }
    // 실제판본/공유배열/엄격한외부효과로 구성하며 복사된8바이트섬목록은 만들지 않는다.
    explicit LiveIslandScene(OriginalEdition edition)
        :pool(edition,kCapacity,false),spots(65536),types(edition==OriginalEdition::Patch1078 ? 188U : 171U),
        islands(pool,hash,records,modes,residences,revision,changed,{
            [](Sid,std::uint32_t) { throw std::logic_error("Count의레지던스owner 호출"); },
            [](IslandRuntimeAssertion) { throw std::logic_error("Count의missing 진단 호출"); }}),
        ascend(pool,hash,spots,types,islands,random,state,AscendHooks()) {}
};
}

// 이전1496개중Count40개만재사용한다. 양판본의원본반환/전체목록보존/난수보존을검사한다.
TEST_CASE(VortexIslandBinding_ReusesOriginalCountRowsOnLiveRecords) {
    const auto rows=LoadFixture(NETSTORM_VORTEXASCEND_FIXTURE).rows;
    std::size_t count=0; // 재사용한기존원본입력의총수이며 새native관찰이아니다.
    // 각판본에서실제pool/runtime/Ascend를구성한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        LiveIslandScene scene(edition);
        const std::string editionName=edition==OriginalEdition::Patch1078 ? "originals" : "originalCD";
        // 저장된Count 입력/기대값만선택하며 반환값을재계산하지않는다.
        for (const auto& row:rows) {
            if (row[0]!=editionName || row[1]!="Count") continue;
            const auto input=Split(row[2],',');CHECK(input.size()==2);
            const auto variant=Number(input[1]);
            constexpr std::array<std::uint32_t,6> owners{0,1,2,8,0xffffffff,1}; // 원본실행기에넣었던owner DWORD다.
            scene.records.fill(0xa5);Put(scene.records,0,6);scene.random.SetState(0x6b4291);
            // 원본6개입력의exists/owner만실제32바이트간격에쓴다. 이웃/padding은보존한다.
            for (std::size_t index=0;index<owners.size();++index) {
                Put(scene.records,8+index*32,static_cast<std::uint32_t>((index+variant)%3!=0));
                Put(scene.records,12+index*32,owners[(index+variant)%owners.size()]);
            }
            const auto before=scene.records;
            CHECK(scene.ascend.CountForeignOwnedIslands(Number(input[0]))==Number(row[3]));
            CHECK(scene.records==before);CHECK(scene.random.State()==0x6b4291);++count;
        }
    }
    CHECK(count==40);
}

// 생성/삭제/owner변경 뒤 현재목록을 다시 읽고 signed count와 다른풀 연결거부를 검사한다.
TEST_CASE(VortexIslandBinding_SeesMutableCountOwnerAndRejectsOtherPool) {
    // 두판본의실제mutable IslandList를독립적으로연결한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        LiveIslandScene scene(edition);scene.islands.Reset();
        CHECK(scene.ascend.CountForeignOwnedIslands(1)==0);
        CHECK(scene.islands.Add(2,5)==0);CHECK(scene.ascend.CountForeignOwnedIslands(1)==1);
        CHECK(scene.islands.Add(1,5)==1);CHECK(scene.ascend.CountForeignOwnedIslands(1)==1);
        Put(scene.records,12,1);CHECK(scene.ascend.CountForeignOwnedIslands(1)==0);
        Put(scene.records,12,0xffffffff);CHECK(scene.ascend.CountForeignOwnedIslands(1)==1);
        scene.islands.Delete(0);CHECK(scene.islands.Count()==2);CHECK(scene.ascend.CountForeignOwnedIslands(1)==0);
        Put(scene.records,0,0x80000000);CHECK(scene.ascend.CountForeignOwnedIslands(1)==0);
        SidPool other(edition,kCapacity,false); // 다른SID기록을 가진 별도pool로 잘못된 연결을 검출한다.
        bool rejected=false; // 실제constructor가 동일pool 계약을 거부했는지 보관한다.
        try { RawVortexAscend wrong(other,scene.hash,scene.spots,scene.types,scene.islands,scene.random,scene.state,LiveIslandScene::AscendHooks()); }
        catch (const std::invalid_argument&) { rejected=true; }
        CHECK(rejected);
    }
}
