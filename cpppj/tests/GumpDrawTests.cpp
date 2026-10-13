// Gump Draw의 독립 PE 관찰과 실제 트리→공통 분배→현재 클립 픽셀 출력을 검사한다.
#include "ConstructionSupport.h"
#include "client/GumpDraw.h"
#include "client/DrawSort.h"
#include <algorithm>
#include <limits>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 실행기가 쓰는 격리 노드 주소와 한 노드의 원본 바이트 크기다.
constexpr std::uint32_t kNativeNodes=0x15000000,kNativeStride=128;
// 기대 Draw 관찰을 재계산하지 않고 저장 fixture에서 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPDRAW_FIXTURE).rows;return rows; }
// 원본 raw 완성 입력을 little endian 바이트로 읽는다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 16진수 두 글자가 한 입력 바이트다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// signed 화면 경계를 저장 열에서 그대로 읽는다.
ScreenRect Rect(const std::string& text) {
    const auto v=Split(text,',');return {std::stoi(v[0]),std::stoi(v[1]),std::stoi(v[2]),std::stoi(v[3])};
}
// 상태 관찰은 half-open 논리 경계 다음 inclusive VFX 경계를 쓴다.
std::string Coordinates(ScreenRect r) { return std::to_string(r.left)+","+std::to_string(r.top)+","+std::to_string(r.right)+","+std::to_string(r.bottom); }
// 실제 현재 상태의 두 경계를 기록한다. 기대 보정 계산은 하지 않는다.
std::string Coordinates(const GumpDrawState& s) { return Coordinates(s.clip)+","+Coordinates(s.vfxClip); }
// 하나의 안정된 노드 저장소를 수집/Draw에 함께 공급한다.
struct GumpScene {
    std::map<std::uint32_t,GumpCollectionNode> nodes;
    std::map<std::uint32_t,ScreenPoint> anchors;
    GumpCollectionState collection;
    GumpDrawState draw;
    std::optional<ScreenRect> projectClip;
    std::string projected;
    // 완성 입력 raw 필드를 호스트 노드로 읽으며 초기 캐시도 독립 입력으로 남긴다.
    void Load(const std::string& text) {
        // 모든 토큰의 링크/플래그/좌표/크기를 그대로 공급한다.
        for (const auto& item:Split(text,';')) {
            const auto parts=Split(item,'=');const auto raw=Bytes(parts[1]);auto& n=nodes[Number(parts[0])];
            n.flags=Get(raw,4);n.next=Get(raw,12);n.parent=Get(raw,20);n.child=Get(raw,24);n.object=Get(raw,36);n.depth=Get(raw,48);
            n.width=Float(Get(raw,68));n.height=Float(Get(raw,72));n.x=Float(Get(raw,76));n.y=Float(Get(raw,80));
            n.cached={std::bit_cast<std::int32_t>(Get(raw,108)),std::bit_cast<std::int32_t>(Get(raw,112))};
        }
    }
    // 현재 저장소 조회와 월드 투영의 완성 응답을 같은 수집기에 연결한다.
    GumpCollectionHooks Hooks() {
        return {[this](auto token)->GumpCollectionNode* { const auto it=nodes.find(token);return it==nodes.end() ? nullptr : &it->second; },
            [this](auto sid) { if (!projected.empty()) projected+=',';projected+=std::to_string(sid);
                if (projectClip) SetGumpDrawClip(draw,*projectClip);return anchors.at(sid); }};
    }
    // Draw가 실제로 갱신한 캐시만 주소순으로 기록한다.
    std::string Caches() const {
        std::string result;
        // 부착 자신/일반 노드의 캐시 보존 차이도 전체 저장소에서 확인한다.
        for (const auto& [token,n]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+","+std::to_string(n.cached.x)+","+std::to_string(n.cached.y); }
        return result;
    }
};
// 한 판본의 완성 입력/가상 응답을 재생해 모든 상태와 호출 순서를 독립 PE 관찰에 대조한다.
void Replay(std::string_view edition) {
    std::size_t replayed=0;
    // 앞선 Draw의 클립/캐시 변경이 다음 입력에 남지 않게 각 저장소를 새로 만든다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==17);if (row.size()!=17) return;
        GumpScene scene;scene.collection.root=kNativeNodes;
        scene.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        scene.Load(row[8]);scene.draw={Rect(row[4]),Rect(row[5])};if (row[7]!="-") scene.projectClip=Rect(row[7]);
        if (row[9]!="-") {
            // 기대 투영 좌표가 아니라 입력 경계의 명시 응답이다.
            for (const auto& item:Split(row[9],';')) {
                const auto v=Split(item,',');scene.anchors.emplace(Number(v[0]),ScreenPoint{std::stoi(v[1]),std::stoi(v[2])});
            }
        }
        const auto token=Number(row[2]);const auto mode=std::stoi(row[3]);const auto change=Rect(row[6]);
        const GumpCollection collector(scene.collection,scene.Hooks());const GumpDraw* nested=nullptr;std::string observed;
        GumpDraw draw(collector,scene.draw,{[&](auto current) {
            if (!observed.empty()) observed+=';';observed+=std::to_string(current)+","+Coordinates(scene.draw);
            if (current==token && mode==1) SetGumpDrawClip(scene.draw,change);
            if (current==token && mode==2) nested->Draw(kNativeNodes+2*kNativeStride);
        }});nested=&draw;draw.Draw(token);
        const bool same=observed==row[10] && Coordinates(scene.draw)==row[11] && scene.Caches()==row[12] && (scene.projected.empty() ? "-" : scene.projected)==row[13];
        CHECK(same);
        if (!same) { std::printf("GumpDraw %s %s\nactual: %s / %s\nnative: %s / %s\n",row[0].c_str(),row[1].c_str(),observed.c_str(),Coordinates(scene.draw).c_str(),row[10].c_str(),row[11].c_str());break; }
        ++replayed;
    }
    CHECK(replayed==476);
}
}
// 10.78의 전체 보호 분기/위치/경계/현재 클립 변경/중첩 관찰을 재생한다.
TEST_CASE(GumpDraw_PatchNative) { Replay("originals"); }
// CD의 독립 클립 생성/복구와 부착 부모 int→float 저장 차이를 재생한다.
TEST_CASE(GumpDraw_CdNative) { Replay("originalCD"); }
// 추가판의 별도 PE에서 관찰한 모든 입력을 같은 CD 복원에 대조한다.
TEST_CASE(GumpDraw_AdditionalNative) { Replay("original1037"); }

// Draw는 visible/dead 필터나 자식 재귀를 추가하지 않는다. flagless 경로는 위치도 읽지 않는다.
TEST_CASE(GumpDraw_CurrentFlagsAndNoUnclippedCacheWrite) {
    GumpScene scene;scene.collection.root=1;scene.nodes[1]={};scene.nodes[2]={0x10000,0,999,999,0,0,4,4,2,3,{17,19}};
    scene.draw={{0,0,8,8},{91,92,93,94}};int calls=0;GumpCollection collector(scene.collection,scene.Hooks());
    GumpDraw draw(collector,scene.draw,{[&](auto token) { CHECK(token==2);++calls;SetGumpDrawClip(scene.draw,{1,2,5,6}); }});
    draw.Draw(2);CHECK(calls==1);CHECK(scene.nodes[2].cached.x==17 && scene.nodes[2].cached.y==19);CHECK(Coordinates(scene.draw)=="1,2,5,6,1,2,4,5");
    scene.nodes[2].parent=1;scene.nodes[2].flags=kGumpDrawClip;SetGumpDrawClip(scene.draw,{0,0,8,8});draw.Draw(2);
    CHECK(calls==2);CHECK(scene.nodes[2].cached.x==2 && scene.nodes[2].cached.y==3);CHECK(Coordinates(scene.draw)=="0,0,8,8,0,0,7,7");
}

// 원본 예외 언와인드의 보호 의도를 C++ 예외에서 확인한다. 실제 PE 예외 전파 실행 근거와 구분한다.
TEST_CASE(GumpDraw_ExceptionRestoresProtectedClip) {
    GumpScene scene;scene.collection.root=1;scene.nodes[1]={};scene.nodes[2]={kGumpDrawClip,0,1,0,0,0,4,4,2,3};
    scene.draw={{0,0,8,8},{91,92,93,94}};GumpCollection collector(scene.collection,scene.Hooks());
    GumpDraw draw(collector,scene.draw,{[&](auto) { SetGumpDrawClip(scene.draw,{1,2,5,6});throw std::runtime_error("검사 가상 Draw 실패"); }});
    CHECK(Throws([&] { draw.Draw(2); }));CHECK(Coordinates(scene.draw)=="0,0,8,8,0,0,7,7");
    scene.nodes[2].flags=0;CHECK(Throws([&] { draw.Draw(2); }));CHECK(Coordinates(scene.draw)=="1,2,5,6,1,2,4,5");
}

// Bounds의 투영이 클립을 바꾸면 바뀐 현재 영역을 저장한다. 부착 자신 캐시는 덮지 않는다.
TEST_CASE(GumpDraw_ProjectionChangesClipBeforeGuard) {
    GumpScene scene;scene.collection.root=1;scene.nodes[1]={};scene.nodes[2]={kGumpDrawClip|0x2000,0,1,0,7,0,10,10,2,3,{17,19}};
    scene.anchors[7]={4,5};scene.projectClip=ScreenRect{5,7,21,31};SetGumpDrawClip(scene.draw,{0,0,8,8});GumpCollection collector(scene.collection,scene.Hooks());
    std::string observed;GumpDraw draw(collector,scene.draw,{[&](auto) { observed=Coordinates(scene.draw); }});draw.Draw(2);
    CHECK(observed=="6,8,16,18,6,8,15,17");CHECK(Coordinates(scene.draw)=="5,7,21,31,5,7,20,30");
    CHECK(scene.projected=="7");CHECK(scene.nodes[2].cached.x==17 && scene.nodes[2].cached.y==19);
}

// 실제 수집/CRT 정렬/kind 2 보호/버튼 네 선/투명 이미지 출력을 한 흐름으로 확인한다.
TEST_CASE(GumpDraw_CommonTreeDispatchPixels) {
    GumpScene scene;scene.collection.root=1;scene.nodes[1].child=2;
    scene.nodes[2]={1|kGumpDrawClip,0,1,3,0,6,6,6,2,2};scene.nodes[3]={1|kGumpDrawClip,0,2,0,0,5,10,10,-2,-2};
    SetGumpDrawClip(scene.draw,{0,0,12,12});std::array<std::uint8_t,146> guarded;guarded.fill(9);auto pixels=std::span(guarded).subspan(1,144);
    GumpPixelTarget target(pixels,12,12,scene.draw);const IndexedImage image{2,2,{55,0,255,44},{255,0,255,255}};
    GumpCollection collector(scene.collection,scene.Hooks());std::vector<std::uint32_t> calls;
    GumpDraw draw(collector,scene.draw,{[&](auto token) {
        calls.push_back(token);
        if (token==2) { const auto plan=PlanButtonDraw({collector.Bounds(token),0,0,0,0,0},{},17);target.Lines(plan.lines); }
        else target.Image(image,{0,0});
    }});DrawCollectionHooks collection;BindGumpCollection(collection,collector);DrawDispatchHooks dispatch;BindGumpDraw(dispatch,draw);
    std::array<RenderCollectionEntry,4> work{};DrawCollectionRequest request;request.screen={0,0,12,12};request.dirtyFlags=4;
    const auto result=CollectDrawEntries(request,work,collection);CHECK(result.count==2);if (result.count!=2) return;
    auto entries=std::span(work).first(2);SortDrawEntries(entries,OriginalEdition::Patch1078);DispatchDrawEntries(entries,{},dispatch);
    CHECK(calls==std::vector<std::uint32_t>({2,3}));CHECK(pixels[27]==17);CHECK(pixels[0]==55);CHECK(pixels[1]==9);CHECK(pixels[12]==255);
    CHECK(guarded.front()==9 && guarded.back()==9);CHECK(Coordinates(scene.draw)=="0,0,12,12,0,0,11,11");
}

// 실제 VFX 전역을 사용하고 이미지 투명/불투명 0/255와 선택적 소스 색 치환을 유지한다.
TEST_CASE(GumpDraw_PixelImageTransparencyCurrentInclusiveClip) {
    GumpDrawState state{{0,0,8,4},{2,1,4,2}};std::array<std::uint8_t,32> pixels;pixels.fill(9);
    GumpPixelTarget target(pixels,8,4,state);const IndexedImage image{4,1,{55,0,255,44},{0,255,255,255}};ColorMap colors{};
    // 소스 색 0/255도 현재 변환 표의 입력이다.
    for (std::size_t i=0;i<colors.size();++i) colors[i]=static_cast<std::uint8_t>(i+10);
    target.Image(image,{1,1},&colors);CHECK(pixels[9]==9);CHECK(pixels[10]==10);CHECK(pixels[11]==9);CHECK(pixels[12]==54);CHECK(pixels[13]==9);
    SetGumpDrawClip(state,{1,2,3,3});target.Image(image,{1,2});CHECK(pixels[17]==9);CHECK(pixels[18]==0);CHECK(pixels[19]==9);
    const auto before=pixels;SetGumpDrawClip(state,{4,2,4,3});target.Image(image,{1,2});CHECK(pixels==before);
}

// 선의 역순/끝점/현재 클립, 극단 inclusive 끝점의 64비트 교차와 색 BYTE 절삭을 확인한다.
TEST_CASE(GumpDraw_PixelLinesAndExtremeClip) {
    GumpDrawState state;SetGumpDrawClip(state,{2,2,6,6});std::array<std::uint8_t,64> pixels;pixels.fill(9);GumpPixelTarget target(pixels,8,8,state);
    const std::array<GumpLine,2> lines{{{{7,3},{1,3},273},{{4,7},{4,1},18}}};target.Lines(lines);
    CHECK(pixels[26]==17 && pixels[29]==17 && pixels[25]==9 && pixels[30]==9);CHECK(pixels[20]==18 && pixels[44]==18 && pixels[52]==9);
    state.vfxClip={0,0,std::numeric_limits<int>::max(),std::numeric_limits<int>::max()};
    const GumpLine extreme{{std::numeric_limits<int>::min(),0},{std::numeric_limits<int>::max(),0},255};target.Lines(std::span(&extreme,1));
    CHECK(std::all_of(pixels.begin(),pixels.begin()+8,[](auto color) { return color==255; }));
    SetGumpDrawClip(state,{0,0,std::numeric_limits<int>::min(),std::numeric_limits<int>::min()});CHECK(state.vfxClip.right==std::numeric_limits<int>::max());
    SetGumpDrawClip(state,{6,6,2,2});const auto before=pixels;target.Lines(lines);CHECK(pixels==before);
}

// 지원 대각선은 양 끝을 포함하며 클립 변경 뒤에는 범위 안의 픽셀만 쓴다.
// 이력: 기존 ScrollGump의 VFX 대각선 연결 뒤 남아 있던 예외 기대를 실제 픽셀 검사로 바꿨다.
TEST_CASE(GumpDraw_DiagonalLinesUseCurrentClip) {
    GumpDrawState state;SetGumpDrawClip(state,{0,0,8,8});std::array<std::uint8_t,64> pixels{};
    GumpPixelTarget target(pixels,8,8,state);const GumpLine diagonal{{2,2},{3,3},1};
    target.Lines(std::span(&diagonal,1));CHECK(pixels[18]==1 && pixels[27]==1);
    CHECK(std::count(pixels.begin(),pixels.end(),std::uint8_t{1})==2);
    pixels.fill(0);SetGumpDrawClip(state,{0,0,3,3});target.Lines(std::span(&diagonal,1));
    CHECK(pixels[18]==1 && pixels[27]==0);CHECK(std::count(pixels.begin(),pixels.end(),std::uint8_t{1})==1);
}

// 잘못된 저장소/필수 처리기/표면/이미지는 C++ 경계에서 거부한다.
TEST_CASE(GumpDraw_Contracts) {
    GumpScene scene;scene.collection.root=1;scene.nodes[1]={};scene.nodes[2]={kGumpDrawClip,0,999,0,0,0,4,4,2,3};
    SetGumpDrawClip(scene.draw,{0,0,8,8});GumpCollection collector(scene.collection,scene.Hooks());GumpDraw draw(collector,scene.draw,{[](auto) {}});
    CHECK(Throws([&] { GumpDraw missing(collector,scene.draw,{}); }));CHECK(Throws([&] { draw.Draw(999); }));
    CHECK(Throws([&] { draw.Draw(2); }));CHECK(Coordinates(scene.draw)=="0,0,8,8,0,0,7,7");
    std::array<std::uint8_t,64> pixels{};CHECK(Throws([&] { GumpPixelTarget bad(pixels,0,8,scene.draw); }));
    CHECK(Throws([&] { GumpPixelTarget bad(std::span(pixels).first(63),8,8,scene.draw); }));GumpPixelTarget target(pixels,8,8,scene.draw);
    CHECK(Throws([&] { target.Image({2,2,{1},{255}},{0,0}); }));
}
