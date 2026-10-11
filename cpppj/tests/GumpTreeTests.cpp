// 전체 원본 트리 바이트/콜백 상태와 실제 생성→부착→수집→Draw 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpTree.h"
#include "client/GumpDraw.h"
#include "client/DrawSort.h"

namespace {
using namespace netstorm::test::gumpraw;
// 기대 상태/호출은 실제 PE 관찰만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPTREE_FIXTURE).rows;return rows; }
// 안정된 생성/트리 저장소와 명시 응답·관찰 기록이다.
struct TreeScene {
    std::map<std::uint32_t,GumpCreateNode> nodes;
    GumpCreateState state;
    GumpTreeState tree;
    std::map<std::string,std::vector<std::array<std::uint32_t,3>>> responses;
    std::map<std::uint32_t,ScreenPoint> anchors;
    std::string calls;
    // 현재 토큰의 같은 객체 주소를 공급한다.
    GumpCreateNode* Find(std::uint32_t token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; }
    // 모든 노드의 현재 40 DWORD를 원본 offset으로 직렬화한다.
    std::string Nodes() const {
        std::string result;
        // 주소 순서는 fixture의 완성 입력 순서와 같다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+"="+Encode(node); }
        return result;
    }
    // 전역 입력/관찰 순서는 루트·화면·지도·패치 진단 모드다.
    std::string Globals() const {
        return std::to_string(state.collection.root)+","+std::to_string(state.screenWidth)+","+std::to_string(state.screenHeight)+","+
            std::to_string(tree.mapView.left)+","+std::to_string(tree.mapView.top)+","+std::to_string(tree.mapView.right)+","+std::to_string(tree.mapView.bottom)+","+std::to_string(tree.pushDebugMode);
    }
    // 명시 응답의 전역 DWORD만 같은 의미 필드에 쓴다.
    void Global(std::uint32_t index,std::uint32_t value) {
        const auto signedValue=std::bit_cast<std::int32_t>(value);
        switch (index) {
        case 0: state.collection.root=value;break;case 1: state.screenWidth=signedValue;break;case 2: state.screenHeight=signedValue;break;
        case 3: tree.mapView.left=signedValue;break;case 4: tree.mapView.top=signedValue;break;case 5: tree.mapView.right=signedValue;break;
        case 6: tree.mapView.bottom=signedValue;break;case 7: tree.pushDebugMode=value;break;default: throw std::runtime_error("잘못된 트리 전역 응답");
        }
    }
    // 경계의 실제 현재 상태를 기록한 뒤 기대 계산 없이 입력 DWORD 변경만 적용한다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+","+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 인자는 raw float 비트/토큰/플래그 그대로 관찰한다.
        for (auto arg:args) calls+=","+std::to_string(arg);
        calls+='@'+Globals()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return;
        // 응답은 fixture에 명시한 위치/값만 쓴다. 부착/정렬 결과는 재계산하지 않는다.
        for (const auto& patch:found->second) {
            if (!patch[0]) { Global(patch[1],patch[2]);continue; }
            auto raw=Bytes(Encode(nodes.at(patch[0])));Put(raw,patch[1],patch[2]);nodes.at(patch[0])=Decode(raw);
        }
    }
    // 원본 가상/삭제/투영 입력 경계를 같은 현재 저장소의 관찰기로 연결한다.
    GumpTreeHooks Hooks() {
        return {[this](auto token) { return Find(token); },[this](auto token,bool pushed) { Respond("Redraw",token,{pushed ? 1U : 0U}); },
            [this](auto token) { Respond("Push",token); },[this](auto token,float x,float y,auto parent,auto mode) { Respond("Attach",token,{Bits(x),Bits(y),parent,mode}); },
            [this](auto token) { Respond("Kill",token); },[this](auto sid) { Respond("Project",sid);return anchors.at(sid); }};
    }
};
// 한 판본의 완성 입력을 재생해 전체 노드·전역·정렬 반환·콜백 당시 상태를 대조한다.
void Replay(std::string_view edition) {
    std::size_t count=0;
    // 각 행의 연속 연산은 같은 현재 상태를 공유한다.
    for (const auto& row:Fixture()) {
        CHECK(row.size()==13);if (row.size()!=13) return;if (row[0]!=edition) continue;TreeScene scene;const auto values=Split(row[2],',');
        scene.state.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        // 현재 전역 입력을 signed 필요한 위치만 변환한다.
        for (std::uint32_t i=0;i<values.size();++i) scene.Global(i,Number(values[i]));
        // 모든 전체 raw 노드를 같은 생성/트리 저장소에 준비한다.
        for (const auto& item:Split(row[3],';')) { const auto part=Split(item,'=');scene.nodes.emplace(Number(part[0]),Decode(Bytes(part[1]))); }
        CHECK(scene.Nodes()==row[3]);
        if (row[5]!="-") {
            // 경계 종류/this마다 완성 응답 DWORD를 입력으로 읽는다.
            for (const auto& item:Split(row[5],';')) {
                const auto part=Split(item,':');
                // 각 응답 위치/값은 원본 fixture에 명시한 그대로다.
                for (const auto& patch:Split(part[1],'/')) { const auto p=Inputs(patch);scene.responses[part[0]].push_back({p[0],p[1],p[2]}); }
            }
        }
        if (row[6]!="-") {
            // 월드 투영의 완성 정수 결과만 공급한다.
            for (const auto& item:Split(row[6],';')) { const auto p=Split(item,',');scene.anchors.emplace(Number(p[0]),ScreenPoint{std::stoi(p[1]),std::stoi(p[2])}); }
        }
        GumpTree tree(scene.state,scene.tree,scene.Hooks());std::string results;
        // 실제 C++ 함수를 연속 호출한다. 반환 기대 값은 실제 PE 관찰에서 읽는다.
        for (const auto& item:Split(row[4],';')) {
            const auto p=Split(item,',');const auto token=Number(p[1]);std::string result="-";
            if (p[0]=="Attach") tree.Attach(token,Float(Number(p[2])),Float(Number(p[3])),Number(p[4]),Number(p[5]));
            else if (p[0]=="Push") tree.Push(token);
            else if (p[0]=="Align") { const auto position=tree.Align(token);std::array<std::uint8_t,8> raw{};Put(raw,0,Bits(position[0]));Put(raw,4,Bits(position[1]));result=Hex(raw); }
            else if (p[0]=="AddDepth") tree.AddDepth(token,Number(p[2]));
            else if (p[0]=="SetDepth") tree.SetDepth(token,Number(p[2]));
            else if (p[0]=="Move") tree.Move(token,Float(Number(p[2])),Float(Number(p[3])),Number(p[4]));
            else if (p[0]=="Relative") tree.MoveRelative(token,Number(p[2]),Float(Number(p[3])),Float(Number(p[4])),static_cast<std::uint8_t>(Number(p[5])),Number(p[6]));
            else throw std::runtime_error("알 수 없는 Gump 트리 연산");
            if (!results.empty()) results+='/';results+=result;
        }
        const bool same=scene.Nodes()==row[7] && scene.Globals()==row[8] && results==row[9] && (scene.calls.empty() ? "-" : scene.calls)==row[10];CHECK(same);
        if (!same) { std::printf("GumpTree %s %s: nodes=%d globals=%d result=%d calls=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[7],scene.Globals()==row[8],results==row[9],(scene.calls.empty() ? "-" : scene.calls)==row[10]);break; }++count;
    }
    CHECK(count==Fixture().size()/3);
}
// 실제 생성기와 같은 저장소에서 루트/일반 노드를 준비한다.
void Create(TreeScene& scene) {
    scene.nodes[1]={};scene.nodes[2]={};scene.nodes[3]={};scene.nodes[4]={};scene.state.screenWidth=32;scene.state.screenHeight=24;
    GumpCreate create(scene.state,{[&](auto token) { return scene.Find(token); },[](auto) { return 1U; }});create.InitializeRoot();
    create.Construct(2,27,8,6,1|kGumpDrawClip);create.Construct(3,27,3,2,1);create.Construct(4,27,2,2,1);
}
}
// 10.78의 전체 트리 몸체/가상 경계 당시 바이트를 대조한다.
TEST_CASE(GumpTree_PatchNative) { Replay("originals"); }
// CD의 화면 float 저장/0 부호와 독립 트리 몸체를 대조한다.
TEST_CASE(GumpTree_CdNative) { Replay("originalCD"); }
// 추가판 PE도 별도 실제 관찰과 대조한다.
TEST_CASE(GumpTree_AdditionalNative) { Replay("original1037"); }

// 생성된 Gump를 실제 부착/밀어내기/재부착하고 수집/정렬/클립 픽셀까지 같은 저장소에서 처리한다.
TEST_CASE(GumpTree_CreatedAttachPushCollectionDraw) {
    TreeScene scene;Create(scene);GumpTree tree(scene.state,scene.tree,scene.Hooks());tree.Attach(2,2,3,1,0);tree.Attach(3,1,1,2,0);
    CHECK(scene.nodes[1].child==2 && scene.nodes[2].child==3 && scene.nodes[3].depth==scene.nodes[2].depth-3U);
    GumpCreate create(scene.state,{[&](auto token) { return scene.Find(token); },{}});GumpCollection collection(scene.state.collection,create.CollectionHooks());
    std::array<RenderCollectionEntry,8> work{};const auto count=collection.Collect({0,0,32,24},work,8);CHECK(count==2);if (count!=2) return;
    CHECK(work[0].token==2 && work[1].token==3);CHECK(scene.nodes[3].cached.x==3 && scene.nodes[3].cached.y==4);
    GumpDrawState state;SetGumpDrawClip(state,{0,0,32,24});std::array<std::uint8_t,768> pixels{};GumpPixelTarget target(pixels,32,24,state);
    GumpDraw draw(collection,state,{[&](auto token) { const auto y=collection.Position(token).y;const GumpLine line{{0,y},{31,y},token+10};target.Lines(std::span(&line,1)); }});
    DrawDispatchHooks dispatch;BindGumpDraw(dispatch,draw);auto entries=std::span(work).first(2);SortDrawEntries(entries,scene.state.collection.edition);DispatchDrawEntries(entries,{},dispatch);
    CHECK(pixels[97]==0 && pixels[98]==12 && pixels[105]==12 && pixels[106]==0);CHECK(pixels[128]==13 && pixels[159]==13);
    tree.Push(2);CHECK(scene.nodes[1].child==0 && scene.nodes[2].parent==1);CHECK(collection.Collect({0,0,32,24},work,8)==0);
    tree.Attach(2,7,8,0,1);CHECK(collection.Collect({0,0,32,24},work,8)==2);CHECK(scene.nodes[3].cached.x==8 && scene.nodes[3].cached.y==9);
}

// 가상 경계를 실제 기본 Push/Attach에 바인딩하면 자식 재배치도 같은 트리의 링크를 갱신한다.
TEST_CASE(GumpTree_RealVirtualChildReflow) {
    TreeScene scene;Create(scene);GumpTree* current=nullptr;auto hooks=scene.Hooks();hooks.push=[&](auto token) { current->Push(token); };
    hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { current->Attach(token,x,y,parent,mode); };
    GumpTree tree(scene.state,scene.tree,hooks);current=&tree;tree.Attach(2,2,3,1,0);tree.Attach(3,1,1,2,0);tree.Attach(4,2,2,2,0);
    scene.nodes[3].alignFlags=0x10;scene.nodes[4].alignFlags=0x20;scene.nodes[4].secondaryDepth=7;tree.Push(2);
    tree.Attach(2,4,5,0,0);CHECK(scene.nodes[2].child==3 && scene.nodes[3].next==4 && scene.nodes[4].previous==3);
    CHECK(scene.nodes[3].x==5 && scene.nodes[4].y==4 && scene.nodes[4].depth==scene.nodes[2].depth+7U);
    CHECK(!(scene.nodes[3].flags&8) && !(scene.nodes[4].flags&8));
}

// 모든 깊이를 바꾼 뒤 재부착의 명시 secondary가 실제 수집 signed 키에 반영된다.
TEST_CASE(GumpTree_DepthChangesReachCollection) {
    TreeScene scene;Create(scene);GumpTree tree(scene.state,scene.tree,scene.Hooks());tree.Attach(2,2,3,1,0);tree.Attach(3,1,1,2,0);
    tree.SetDepth(2,0xfffffff0);tree.AddDepth(2,0x80000000);CHECK(scene.nodes[2].depth==0x7ffffff0 && scene.nodes[3].depth==0x7ffffff0);
    GumpCreate create(scene.state,{[&](auto token) { return scene.Find(token); },{}});GumpCollection collection(scene.state.collection,create.CollectionHooks());std::array<RenderCollectionEntry,4> work{};
    const auto count=collection.Collect({0,0,32,24},work,4);CHECK(count==2);if (count!=2) return;CHECK(work[0].depth==-16 && work[1].depth==-16);
}

// 원본 정상 경로에 없는 토큰/가상 경계/순환과 CD assert 입력은 호스트에서 거부한다.
TEST_CASE(GumpTree_ContractsAndCycles) {
    TreeScene scene;Create(scene);auto hooks=scene.Hooks();CHECK(Throws([&] { GumpTree invalid(scene.state,scene.tree,{}); }));GumpTree tree(scene.state,scene.tree,hooks);
    CHECK(Throws([&] { tree.Attach(99,0,0,1,0); }));const auto before=scene.Nodes();CHECK(Throws([&] { tree.Attach(2,0,0,99,0); }));CHECK(scene.Nodes()==before);
    CHECK(Throws([&] { tree.Push(2); }));scene.state.collection.edition=OriginalEdition::Cd1072;scene.nodes[2].flags=1;CHECK(Throws([&] { tree.Attach(2,0,0,1,0); }));
    scene.nodes[2].parent=1;scene.nodes[2].child=3;scene.nodes[3].child=2;CHECK(Throws([&] { tree.AddDepth(2,1); }));
    scene.nodes[2].parent=3;scene.nodes[2].alignFlags=0x40;CHECK(Throws([&] { tree.Align(2); }));
}

// 가상 redraw의 예외에는 원본 순서상 이미 쓴 링크/위치를 유지한다. 호스트 예외 원자성을 추가하지 않는다.
TEST_CASE(GumpTree_CallbackExceptionKeepsWrittenState) {
    TreeScene scene;Create(scene);auto hooks=scene.Hooks();hooks.redraw=[](auto,bool) { throw std::runtime_error("입력 처리기 실패"); };GumpTree tree(scene.state,scene.tree,hooks);
    CHECK(Throws([&] { tree.Attach(2,2,3,1,0); }));CHECK(scene.nodes[1].child==2 && scene.nodes[2].parent==1 && scene.nodes[2].x==2 && !(scene.nodes[2].flags&8));
    CHECK(Throws([&] { tree.Push(2); }));CHECK(scene.nodes[1].child==0 && (scene.nodes[2].flags&8));
}
