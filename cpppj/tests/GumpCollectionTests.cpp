// UI 트리/좌표의 실제 PE 관찰과 공통 표시 목록 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/GumpCollection.h"
#include "client/DrawSort.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 저장한 독립 원본 관찰만 읽으며 기대 좌표/수집 목록은 재계산하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPCOLLECTION_FIXTURE).rows;return rows; }
// little endian 완성 입력을 바이트 그대로 읽는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 16진수 두 글자가 입력 바이트 하나다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 원본 관찰과 같은 signed 화면 좌표열이다.
std::string Coordinates(ScreenPoint p) { return std::to_string(p.x)+","+std::to_string(p.y); }
// 원본 경계는 left/top/right/bottom 순서다.
std::string Coordinates(ScreenRect r) { return std::to_string(r.left)+","+std::to_string(r.top)+","+std::to_string(r.right)+","+std::to_string(r.bottom); }
// 입력 노드와 명시 월드 투영 응답을 가진 호스트 저장소다.
struct GumpScene {
    std::map<std::uint32_t,GumpCollectionNode> nodes;
    std::map<std::uint32_t,std::vector<std::int64_t>> anchors;
    GumpCollectionState state;
    std::string projected;
    // C++ 필드에 원본 128바이트 입력을 그대로 공급한다.
    void Load(const std::string& input) {
        // 완성 입력 토큰별로 부모/자식/로컬 필드와 초기 캐시를 읽는다.
        for (const auto& item:Split(input,';')) {
            const auto parts=Split(item,'=');const auto raw=Bytes(parts[1]);const auto token=Number(parts[0]);auto& n=nodes[token];
            n.flags=Get(raw,4);n.next=Get(raw,12);n.parent=Get(raw,20);n.child=Get(raw,24);n.object=Get(raw,36);n.depth=Get(raw,48);
            n.width=Float(Get(raw,68));n.height=Float(Get(raw,72));n.x=Float(Get(raw,76));n.y=Float(Get(raw,80));
            n.cached={std::bit_cast<std::int32_t>(Get(raw,108)),std::bit_cast<std::int32_t>(Get(raw,112))};
        }
    }
    // 캐시는 현재 저장소에서 읽으며 월드 부착 자신을 일반 조회처럼 갱신하지 않는다.
    std::string Caches() const {
        std::string result;
        // 주소순 전체 노드 캐시를 대조한다.
        for (const auto& [token,n]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+","+Coordinates(n.cached); }
        return result;
    }
    // 위치 조회의 명시 응답은 현재 링크를 바꿀 수 있다. 기대 수집 순서는 fixture에서 읽는다.
    GumpCollectionHooks Hooks() {
        GumpCollectionHooks hooks;
        hooks.node=[this](auto token)->GumpCollectionNode* { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; };
        hooks.project=[this](auto sid) {
            if (!projected.empty()) projected+=',';projected+=std::to_string(sid);const auto& a=anchors.at(sid);
            if (a[2]) nodes.at(static_cast<std::uint32_t>(a[2])).next=static_cast<std::uint32_t>(a[3]);
            return ScreenPoint{static_cast<int>(a[0]),static_cast<int>(a[1])};
        };
        return hooks;
    }
};
// 판본별 모든 위치/경계/수집 출력을 실제 PE의 전체 작업 바이트/캐시/투영 순서와 대조한다.
void Replay(std::string_view edition) {
    std::size_t replayed=0;
    // 입력 행마다 저장소를 초기화해 앞선 수집의 캐시/링크 변경이 남지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==12);GumpScene scene;const auto config=Split(row[3],',');scene.Load(row[4]);
        scene.state={Number(config[0]),Number(config[1]),edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072};
        if (row[5]!="-") {
            // 투영의 정수점/변경할 next는 완성 응답 입력이다.
            for (const auto& item:Split(row[5],';')) {
                const auto values=Split(item,',');std::vector<std::int64_t> response;
                // SID 다음의 네 정수를 입력 응답으로 읽는다.
                for (std::size_t i=1;i<values.size();++i) response.push_back(std::stoll(values[i]));scene.anchors.emplace(Number(values[0]),std::move(response));
            }
        }
        const GumpCollection collector(scene.state,scene.Hooks());const auto target=Number(config[2]);const auto limit=std::stoi(config[3]);
        ScreenRect view{std::stoi(config[4]),std::stoi(config[5]),std::stoi(config[6]),std::stoi(config[7])};
        std::array<RenderCollectionEntry,34> guarded;std::memset(guarded.data(),0xa5,sizeof(guarded));auto work=std::span(guarded).subspan(1,32);
        std::string result;
        if (row[1]=="Position") result=Coordinates(collector.Position(target));
        else if (row[1]=="Bounds") result=Coordinates(collector.Bounds(target));
        else result=std::to_string(row[1]=="Collect" ? collector.Collect(view,work,limit) : collector.CollectNode(target,view,work,limit));
        const auto output=Hex({reinterpret_cast<const std::uint8_t*>(work.data()),work.size_bytes()});
        const auto guard=Hex({reinterpret_cast<const std::uint8_t*>(guarded.data()),16})+Hex({reinterpret_cast<const std::uint8_t*>(&guarded.back()),16});
        const bool same=result==row[6] && output==row[7] && scene.Caches()==row[8] && (scene.projected.empty() ? "-" : scene.projected)==row[9];
        CHECK(same);CHECK(guard==row[11].substr(0,64));
        if (!same) { std::printf("Gump %s %s %s: %s / %s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),result.c_str(),row[6].c_str());break; }++replayed;
    }
    CHECK(replayed==389);
}
}
// 10.78의 실제 CRT 절삭과 int→float 누적을 포함한 모든 관찰을 재생한다.
TEST_CASE(GumpCollection_PatchNative) { Replay("originals"); }
// CD의 별도 위치/교차 몸체와 월드 부착 부모 반올림 차이를 재생한다.
TEST_CASE(GumpCollection_CdNative) { Replay("originalCD"); }
// 추가판의 독립 PE 관찰도 같은 CD 복원 경로에서 대조한다.
TEST_CASE(GumpCollection_AdditionalNative) { Replay("original1037"); }

// 원본 limit은 최상위별 제한이다. 실제 버퍼를 넘는 쓰기는 호스트 경계에서 거부한다.
TEST_CASE(GumpCollection_PerRootLimitAndSafeBuffer) {
    GumpScene scene;scene.state.root=1;scene.nodes[1].child=2;scene.nodes[2]={1,3,1,0,0,4,10,10,1,1};scene.nodes[3]={1,0,1,0,0,5,10,10,20,20};
    GumpCollection collector(scene.state,scene.Hooks());std::array<RenderCollectionEntry,2> work{};
    CHECK(collector.Collect({0,0,100,100},work,1)==2);CHECK(work[0].token==2 && work[1].token==3);
    CHECK(Throws([&] { collector.Collect({0,0,100,100},std::span(work).first(1),1); }));
    CHECK(collector.Collect({0,0,100,100},{},0)==0);CHECK(collector.CollectNode(999,{},{},0)==0);
}

// 순환/누락/잘못된 월드 부모는 유효 원본 포인터 계약 밖이므로 예외로 거부한다.
TEST_CASE(GumpCollection_RejectsDamagedLinksAndMissingProjection) {
    GumpScene scene;scene.state.root=1;scene.nodes[1].child=2;scene.nodes[2]={1,0,1,0,0,0,10,10,1,1};
    GumpCollection collector(scene.state,scene.Hooks());std::array<RenderCollectionEntry,8> work{};
    CHECK(Throws([&] { GumpCollection invalid(scene.state,{}); }));CHECK(Throws([&] { collector.Position(999); }));
    scene.nodes[2].parent=2;CHECK(Throws([&] { collector.Position(2); }));scene.nodes[2].parent=1;
    scene.nodes[2].next=2;CHECK(Throws([&] { collector.Collect({0,0,100,100},work,8); }));scene.nodes[2].next=0;
    scene.nodes[2].child=2;CHECK(Throws([&] { collector.Collect({0,0,100,100},work,8); }));scene.nodes[2].child=0;
    scene.nodes[2].flags=0x2001;scene.nodes[2].parent=999;CHECK(Throws([&] { collector.Position(2); }));scene.nodes[2].parent=1;
    auto hooks=scene.Hooks();hooks.project={};GumpCollection missing(scene.state,hooks);CHECK(Throws([&] { missing.Position(2); }));
}

// 공통 UI 슬롯을 실제 Gump 트리에 연결해 순차 병합/정렬/kind 2 제출과 UI 전용 dirty를 확인한다.
TEST_CASE(GumpCollection_CommonSubmissionIntegration) {
    GumpScene scene;scene.state.root=1;scene.nodes[1].child=2;scene.nodes[2]={1,3,1,0,0,3,10,10,1,1};scene.nodes[3]={1,0,1,0,0,7,10,10,20,20};
    GumpCollection collector(scene.state,scene.Hooks());DrawCollectionHooks hooks;BindGumpCollection(hooks,collector);
    bool collected=false;hooks.construction=[&](auto,auto output,auto) { collected=true;output.front()={0,0,8,5,3,19};return 1; };
    std::array<RenderCollectionEntry,8> work{};DrawCollectionRequest request;request.screen={0,0,100,100};
    auto result=CollectDrawEntries(request,work,hooks);CHECK(result.count==3 && collected);auto entries=std::span(work).first(3);
    SortDrawEntries(entries,OriginalEdition::Patch1078);std::vector<std::uint32_t> drawn;DrawDispatchHooks dispatch;
    dispatch.ui=dispatch.construction=[&](auto token) { drawn.push_back(token); };DispatchDrawEntries(entries,{},dispatch);
    CHECK(drawn==std::vector<std::uint32_t>({3,8,2}));CHECK(work[0].kind==2 && work[0].tail==0 && work[1].tail==19);
    collected=false;request.dirtyFlags=4;result=CollectDrawEntries(request,work,hooks);CHECK(result.count==2 && !collected);
}
