// 원본 파일 없이 지형 연결·이동 거리·점유·미션 시작 상태를 검사한다.
#include "TestSupport.h"
#include "o/TerrainBuilder.h"
#include "o/Squid.h"
#include "o/Player.h"
#include "client/Renderer.h"
#include <memory>
#include <map>
#include <stdexcept>

using namespace netstorm::o;
namespace {
// 이동 검사 범위에만 지면을 놓아 허공 우회가 생기지 않게 한다.
std::unique_ptr<GroundGrid> Grid() {
    auto grid=std::make_unique<GroundGrid>();
    // 검사 사각형의 행을 순회한다.
    for (int y=10;y<=20;++y)
        // 같은 행의 지면 칸을 채운다.
        for (int x=10;x<=20;++x) grid->At(x,y)->land=true;
    grid->At(10,10)->occupant=1; return grid;
}
// 합성 자산 정의로 all/techAllowed의 생산 그룹 필터를 검사한다.
RiftTypeTable Types() {
    static const auto walker=RiftTypeDefinition::Parse("typename walker typeflags walker; { group=\"walker\"; } A00 : default : \"null.gif\" #0;");
    static const auto other=RiftTypeDefinition::Parse("typename effect typeflags ; {} A00 : default : \"null.gif\" #0;");
    std::vector<RiftTypeSource> sources;
    // 실제 판본의 번호 체계를 유지하며 한 타입만 생산 그룹으로 만든다.
    for (const auto name:TypeLoadOrder(OriginalEdition::Patch1078)) sources.push_back({name,name=="sunArcher" ? &walker : &other});
    return RiftTypeTable(OriginalEdition::Patch1078,sources);
}
}
// 32비트 오버플로·0 대체·나머지를 각각 검사한다.
TEST_CASE(Terrain_RandomAndOrientationTables) {
    std::uint32_t a=0,b=0x0bad0bad; CHECK(TerrainBuilder::Next(a,30)==TerrainBuilder::Next(b,30)); CHECK(a==b);
    a=0xffffffffu; const auto expected=static_cast<std::uint32_t>(a*0x10003u+3u); CHECK(TerrainBuilder::Next(a,15)==static_cast<int>((expected>>16)%15)); CHECK(a==expected);
    // 16개 연결 방향은 역변환해도 같은 문자를 준다.
    for (char c='A';c<='P';++c) CHECK(TerrainBuilder::Orientation(TerrainBuilder::Connection(c))==c);
    CHECK(TerrainBuilder::Normalize('P','A')==std::pair('P','P'));
    bool threw=false; try { TerrainBuilder::Next(a,0); } catch (const std::invalid_argument&) { threw=true; } CHECK(threw);
}
// 서로 다른 영역은 같은 성장 시드를 써도 청크 경계를 넘어 자라지 않는다.
TEST_CASE(Terrain_EmptyWorldAndSeparatedRegions) {
    ChunkMap map; IslandList islands; const TerrainBuilder empty(map,islands);
    CHECK(empty.At(16,16)==kEmptyTerrain && empty.At(-1,0)==kEmptyTerrain && empty.At(256,0)==kEmptyTerrain);
    const int first=islands.Add(1,0),second=islands.Add(1,1);
    map.At(2,2).island=static_cast<std::uint8_t>(first); map.At(2,2).side='P'; map.At(2,2).record[3]=7;
    map.At(5,5).island=static_cast<std::uint8_t>(second); map.At(5,5).side='P'; map.At(5,5).record[3]=7;
    const TerrainBuilder land(map,islands),again(map,islands); CHECK(std::equal(land.Mask().begin(),land.Mask().end(),again.Mask().begin()));
    int count=0;
    // 그려진 칸은 해당 영역 청크 안에 있어야 한다.
    for (int y=0;y<kWorldCells;++y)
        // 반복 생성의 모든 칸과 영역 경계를 확인한다.
        for (int x=0;x<kWorldCells;++x) if (land.At(x,y)!=kEmptyTerrain) { ++count; CHECK((land.At(x,y)==0 && x/16==2 && y/16==2) || (land.At(x,y)==1 && x/16==5 && y/16==5)); }
    CHECK(count>100);
}
// 큰 거리 동률에서도 대각선을 먼저 고르고 마지막 칸에 정확히 도착한다.
TEST_CASE(World_PathDiagonalAndNoCornerCutting) {
    auto grid=Grid(); const auto path=grid->Path({10,10},{14,12},1,2); CHECK(path.size()==4); CHECK(path.front()==CellPoint({11,11})); CHECK(path.back()==CellPoint({14,12}));
    grid->At(11,10)->land=false; const auto detour=grid->Path({10,10},{12,12},1,2); CHECK(!detour.empty()); CHECK(detour.front()==CellPoint({10,11}));
    CHECK(grid->Path({-1,0},{10,10},1,2).empty()); CHECK(grid->Path({10,10},{200,200},1,2).empty());
}
// 점유 건물·적 다리·허공은 막고 동맹 다리는 직선으로만 통과한다.
TEST_CASE(World_PathOccupancyAndBridgeOwnership) {
    auto grid=std::make_unique<GroundGrid>(); grid->At(10,10)->land=true; grid->At(10,10)->occupant=1; grid->At(12,10)->land=true;
    grid->At(11,10)->bridge=true; grid->At(11,10)->owner=2;
    CHECK(grid->Path({10,10},{12,10},1,2).empty()); CHECK(grid->Path({10,10},{12,10},1,6).size()==2);
    grid->At(11,10)->occupant=7; CHECK(grid->Path({10,10},{12,10},1,6).empty());
    grid->At(11,10)->occupant=0; grid->At(11,11)->land=true; CHECK(grid->Path({10,10},{11,11},1,6).size()==2);
}
// 1.8칸/초를 원본 타입 속성과 같은 단위로 적용한다.
TEST_CASE(World_PriestContinuousMovementAndOccupancy) {
    auto grid=Grid(); Squid priest; priest.id=1; priest.cell={10,10}; priest.x=10; priest.y=10; priest.speed=1.8; priest.route={{11,10},{12,10}}; priest.walking=true;
    CHECK(priest.Advance(0.25,*grid,2)); CHECK_NEAR(priest.x,10.45,1e-12); CHECK(priest.cell==CellPoint({10,10})); CHECK(priest.heading==2);
    CHECK(!priest.Advance(0,*grid,2)); CHECK_NEAR(priest.x,10.45,1e-12);
    CHECK(priest.Advance(2,*grid,2)); CHECK_NEAR(priest.x,12,1e-12); CHECK(!priest.walking); CHECK(grid->At(10,10)->occupant==0 && grid->At(12,10)->occupant==1);
}
// 대각선은 sqrt(2)칸의 시간이며 프레임을 나눠도 결과가 같다.
TEST_CASE(World_DiagonalTimeAndSplitFrameConsistency) {
    auto first=Grid(),second=Grid(); Squid a; a.id=1; a.cell={10,10}; a.x=10; a.y=10; a.speed=1.8; a.route={{11,11}}; a.walking=true; auto b=a;
    a.Advance(0.5,*first,2);
    // 같은 시간 합계를 작은 프레임들로 소비한다.
    for (int i=0;i<10;++i) b.Advance(0.05,*second,2);
    CHECK_NEAR(a.x,b.x,1e-12); CHECK_NEAR(a.y,b.y,1e-12); CHECK_NEAR(a.x,10+0.9/std::sqrt(2.0),1e-12);
    a.Advance(std::sqrt(2.0)/1.8-0.5,*first,2); CHECK(!a.walking); CHECK(a.cell==CellPoint({11,11}));
}
// 다음 칸이 새 객체로 막히면 점유를 훼손하지 않고 현재 논리 칸에 멈춘다.
TEST_CASE(World_NewOccupantStopsMovementSafely) {
    auto grid=Grid(); Squid a; a.id=1; a.cell={10,10}; a.x=10; a.y=10; a.speed=1.8; a.route={{11,10}}; a.walking=true;
    grid->At(11,10)->occupant=2; CHECK(a.Advance(1,*grid,2)); CHECK(!a.walking); CHECK(a.cell==CellPoint({10,10})); CHECK(grid->At(11,10)->occupant==2);
}
// 저장 Money보다 미션 SP가 우선하며 구식 ai 키와 번호별 키를 구분한다.
TEST_CASE(World_PlayerStartMoneyKnowledgeAlliesAndColor) {
    const auto types=Types(); FortTemplate fort; fort.money=100000.0f; fort.deck.push_back({70,70,1,0,2});
    const std::map<std::string,std::string> values{{"myStartMoney","3000"},{"myTech","all"},{"myAllyList","2"},{"aiName","Old AI"},{"aiTech","sunArcher"},{"aiStartMoney","25"},{"ai2Name","Numbered AI"},{"ai2Color","orange"},{"denySalvage","1"},{"techAllowed","deny;all;allow;sunArcher"}};
    const auto players=MissionPlayers::Load([&](std::string_view key)->std::optional<std::string> { const auto it=values.find(std::string(key)); return it==values.end() ? std::nullopt : std::optional(it->second); },fort,types,500);
    CHECK_NEAR(players.players[1].stormPower,3000,0); CHECK(players.players[1].storedDeck.size()==1); CHECK(players.players[1].knowledge==std::vector<int>{71});
    CHECK(players.players[2].name=="Numbered AI" && players.players[2].color==8); CHECK_NEAR(players.players[2].stormPower,25,0); CHECK(players.Allied(1,2)); CHECK(!players.Allied(1,3)); CHECK(!players.Allied(0,1));
    CHECK(players.denySalvage && players.techAllowed[71] && !players.techAllowed[70]);
}
// 미션 값이 없으면 명시한 전투 기본 금액을 쓰고 저장 Money를 대신 쓰지 않는다.
TEST_CASE(World_MissingMissionDefaultsAndPermissionReset) {
    const auto types=Types(); FortTemplate fort; fort.money=100000.0f;
    const auto get=[](std::string_view key)->std::optional<std::string> { return key=="techAllowed" ? std::optional<std::string>("deny;sunArcher;allow;all") : std::nullopt; };
    const auto players=MissionPlayers::Load(get,fort,types,500); CHECK_NEAR(players.players[1].stormPower,500,0); CHECK(players.techAllowed[70] && players.techAllowed[71]); CHECK(players.players[1].color==1);
}
// 투명 선택 표시는 물체를 그린 뒤 글자 앞에 합성하고 제거하면 이전 픽셀을 복구한다.
TEST_CASE(World_RendererOverlayLifetimeAndRemoval) {
    using namespace netstorm::client; Renderer renderer(2,2);
    auto overlay=std::make_shared<IndexedImage>(); overlay->width=2; overlay->height=2; overlay->indices={7,8,9,10}; overlay->opacity={255,0,0,255}; renderer.SetOverlay(overlay); overlay.reset();
    std::array<std::uint8_t,4> pixels{}; renderer.Draw(pixels,2,1); renderer.Present([](ScreenRect){}); CHECK((pixels==std::array<std::uint8_t,4>{7,1,1,10}));
    renderer.SetOverlay(nullptr); renderer.Draw(pixels,2,1); renderer.Present([](ScreenRect){}); CHECK((pixels==std::array<std::uint8_t,4>{1,1,1,1}));
}
