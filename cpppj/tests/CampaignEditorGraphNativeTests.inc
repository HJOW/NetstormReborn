// 같은 실제 새 맵 설치 helper로 원본 후보8 배치 직후의 개별 표면·Graph 장부를 대조한다.
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>

namespace { // 원본 번호 자체 대신 연결 성분에 속한 실제 표면의 다중집합을 비교한다.
// 타입·상태·소유자·float32 X/Y 비트로 표면을 식별하며 SID/그림 변형은 포함하지 않는다.
using NativeSurfaceKey723=std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>;
// 같은 좌표에 중복 표면이 있어도 개수를 잃지 않는 Graph별 소속이다.
using NativeSurfaceGroups723=std::map<std::uint32_t,std::multiset<NativeSurfaceKey723>>;
// 최초 원본 전체 Pool에서 고정한 표면674개를 읽으며 현재 제품 결과로 기대값을 만들지 않는다.
NativeSurfaceGroups723 NativePlacementSurfaces723() {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/editor-graph-placement-723-surfaces.tsv";std::ifstream input(path);if (!input) throw std::runtime_error("원본 배치 직후 표면 관측 없음");
    std::string line;std::getline(input,line);NativeSurfaceGroups723 result;
    while (std::getline(input,line)) { // 원본 물리 필드의 폭/정수 비트만 읽고 graph 번호는 별도 key에 보존한다.
        std::istringstream row(line);std::uint32_t sid,type,state,owner,x,y,graph;std::int32_t frame;
        if (!(row>>sid>>type>>state>>owner>>frame>>x>>y>>graph)) throw std::runtime_error("원본 배치 표면 관측 행 오류");
        result[graph].emplace(type,state,owner,x,y);
    }
    return result;
}
// 실제 현재 풀의 표면 상태/소유자/좌표와 Graph BYTE를 직접 읽는다. 새 flood나 프레임 변경은 하지 않는다.
NativeSurfaceGroups723 CurrentPlacementSurfaces723(RawCampaignSession& session) {
    const auto core=session.Core();NativeSurfaceGroups723 result;
    for (const auto& item:session.World().Objects()) { // 같은 렌더링 자산 목록에서 TYPE의 실제 surface비트를 확인한다.
        const auto raw=core.pool.Slot(item.sid);if (!(core.types[raw[10]].flags1&TypeFlag1::kSurface)) continue;
        result[raw[30]].emplace(raw[10],raw[11],raw[34],EditorWireDword(raw,14),EditorWireDword(raw,18));
    }
    return result;
}
}

// 원본 n720의 같은 저장 레코드를 새 맵에 배치해 장부678/실제670과 noGraph4개의 위치까지 확인한다.
TEST_CASE(CampaignEditorGraphNative_SameNativeCandidatePlacementMembershipAndLedger) {
    WithEditorIslandWindowSession480(true,[&](RawCampaignSession& session,Client&) {
        CHECK(session.IslandCount()==0);ChunkMap preview(14,14);CHECK(session.BuildEditorIslandMap(preview));
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
        const auto image=ReadFileBytes(std::filesystem::path(__FILE__).parent_path()/"fixtures/editor-graph-native-720.fort");
        const auto fort=FortTemplate::Parse(image,assets.game.TypeTable());const auto territory=fort.Section("Territory");CHECK(territory.size()==120);
        std::array<std::uint8_t,6> record{};std::copy_n(territory.begin(),6,record.begin());record[1]=3; // 원본 후보의 active/preview 비트이며 저장 뒤 population 비트8은 배치 부모가 다시 쓴다.
        CHECK(session.CanPlaceEditorIsland(preview,record));CHECK(session.DropEditorIsland(preview,record));
        const auto expected=NativePlacementSurfaces723(),actual=CurrentPlacementSurfaces723(session);const auto core=session.Core();
        CHECK(expected.at(1).size()==670 && expected.at(254).size()==4 && actual.size()==2);
        CHECK(actual.contains(254) && actual.at(254)==expected.at(254));
        std::size_t components{};for (const auto& [number,members]:actual) { // 다른 Pool의 Graph 번호는 허용하되 구성원/실제 앞 두WORD는 원본과 같아야 한다.
            if (number==254) continue;++components;CHECK(members==expected.at(1));CHECK(number>0 && number<Graph::kCount);
            const auto& ledger=core.graph.Records()[number];CHECK(ledger.surfaces==678 && ledger.inUse==1);
        }
        CHECK(components==1 && session.IslandCount()==1);
        if (const auto path=std::getenv("NETSTORM_EDITOR_GRAPH_REPORT")) { // 명시 검사 출력만 쓰며 일반 게임/저장 프로필에는 관측을 섞지 않는다.
            std::ofstream output(path,std::ios::binary);output<<"frame\t0\tplaced\n"<<session.World().Report();CHECK(output.good());
        }
    });
}
