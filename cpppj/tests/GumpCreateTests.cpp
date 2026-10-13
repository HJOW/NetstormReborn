// 기본/루트 생성의 전체 바이트 독립 관찰과 동일 저장소의 수집/Draw 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpCreate.h"
#include "client/GumpDraw.h"
#include "client/DrawSort.h"

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 기대 초기화/등록 바이트는 재계산하지 않고 실제 PE fixture에서만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPCREATE_FIXTURE).rows;return rows; }
using netstorm::test::gumpraw::Bytes;
using netstorm::test::gumpraw::Decode;
using netstorm::test::gumpraw::Encode;
// 완성 RLE 입력은 연속 길이/값만 풀며 선택할 등록 칸은 계산하지 않는다.
std::array<std::uint32_t,kGumpRegistryCapacity> Table(const std::string& text) {
    std::array<std::uint32_t,kGumpRegistryCapacity> result{};std::size_t position=0;
    // 입력에 명시한 run을 그대로 채운다.
    for (const auto& part:Split(text,';')) {
        const auto value=Split(part,':');const auto count=Number(value[0]);CHECK(count<=result.size()-position);
        if (count>result.size()-position) return result;std::fill_n(result.begin()+position,count,Number(value[1]));position+=count;
    }
    CHECK(position==result.size());return result;
}
// 실제 등록 표의 연속 같은 DWORD만 길이:값으로 직렬화한다.
std::string Encode(const std::array<std::uint32_t,kGumpRegistryCapacity>& entries) {
    std::string result;
    // 현재 run의 끝까지 읽으며 칸을 선택하거나 등록 여부를 계산하지 않는다.
    for (std::size_t first=0;first<entries.size();) {
        auto end=first+1;
        // 연속 같은 토큰만 하나의 run으로 줄인다.
        while (end<entries.size() && entries[end]==entries[first]) ++end;
        if (!result.empty()) result+=';';result+=std::to_string(end-first)+":"+std::to_string(entries[first]);first=end;
    }
    return result;
}
// 전체 현재 카운터/화면 크기를 기록한다.
std::string Globals(const GumpCreateState& s) { return std::to_string(s.count)+","+std::to_string(s.serial)+","+std::to_string(s.screenWidth)+","+std::to_string(s.screenHeight); }
// 안정된 호스트 저장소와 현재 생성 전역이다.
struct GumpScene {
    // nodes: 노드. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,GumpCreateNode> nodes;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    GumpCreateState state;
    // 현재 토큰의 같은 기본/파생 표시 저장소를 반환한다.
    GumpCreateHooks Hooks() { return {[this](auto token)->GumpCreateNode* { const auto it=nodes.find(token);return it==nodes.end() ? nullptr : &it->second; },{}}; }
    // 각 노드의 실제 전체 필드를 주소순으로 기록한다.
    std::string Nodes() const {
        std::string result;
        // raw 관찰과 같은 토큰/전체 바이트 열을 만든다.
        for (const auto& [token,n]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+"="+Encode(n); }
        return result;
    }
};
// 한 판본의 연속 생성/루트/할당 응답을 재생하여 전체 출력과 카운터/등록 순서를 대조한다.
void Replay(std::string_view edition) {
    std::size_t replayed=0;
    // 입력마다 독립 저장소를 만들되 한 행의 여러 연산은 현재 상태를 공유한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==13);if (row.size()!=13) return;GumpScene scene;const auto globals=Split(row[2],',');
        scene.state.count=Number(globals[0]);scene.state.serial=Number(globals[1]);scene.state.screenWidth=std::stoi(globals[2]);scene.state.screenHeight=std::stoi(globals[3]);
        scene.state.entries=Table(row[3]);scene.state.collection.root=scene.state.entries[0];scene.state.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        // 모든 초기 노드의 raw 바이트를 비트 그대로 읽는다.
        for (const auto& item:Split(row[4],';')) { const auto part=Split(item,'=');scene.nodes.emplace(Number(part[0]),Decode(Bytes(part[1]))); }
        const auto initial=scene.Nodes();CHECK(initial==row[4]);std::vector<std::string> action;std::string allocations,results;auto hooks=scene.Hooks();
        hooks.allocate=[&](auto size) {
            const bool cleared=std::all_of(scene.state.entries.begin(),scene.state.entries.end(),[](auto token) { return token==0; });
            if (!allocations.empty()) allocations+=';';allocations+=std::to_string(size)+","+Globals(scene.state)+","+std::to_string(cleared ? 1 : 0);
            if (action.size()==6) { scene.state.count=Number(action[2]);scene.state.serial=Number(action[3]);scene.state.screenWidth=std::stoi(action[4]);scene.state.screenHeight=std::stoi(action[5]); }
            return Number(action[1]);
        };GumpCreate create(scene.state,hooks);
        // 명시 입력 연산만 실제 C++ 생성기에 전달한다. 기대 결과는 저장 관찰에서 읽는다.
        for (const auto& item:Split(row[5],';')) {
            action=Split(item,',');const auto token=action[0]=="Root" ? create.InitializeRoot() :
                create.Construct(Number(action[1]),Number(action[2]),Float(Number(action[3])),Float(Number(action[4])),Number(action[5]));
            if (!results.empty()) results+=',';results+=std::to_string(token);
        }
        const bool same=scene.Nodes()==row[6] && Encode(scene.state.entries)==row[7] && Globals(scene.state)==row[8] && results==row[9] && (allocations.empty() ? "-" : allocations)==row[10];
        CHECK(same);CHECK(scene.state.collection.root==scene.state.entries[0]);
        if (!same) {
            std::printf("GumpCreate %s %s: nodes=%d table=%d globals=%d results=%d allocation=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[6],Encode(scene.state.entries)==row[7],Globals(scene.state)==row[8],results==row[9],(allocations.empty() ? "-" : allocations)==row[10]);
            // 실제 다른 DWORD만 출력하여 필드 복원 오류의 위치를 확인한다.
            for (const auto& item:Split(row[6],';')) {
                const auto part=Split(item,'=');const auto expected=Bytes(part[1]);const auto actual=Bytes(Encode(scene.nodes.at(Number(part[0]))));
                // 서로 다른 원본 offset/현재 값/관찰 값만 기록한다.
                for (std::size_t offset=0;offset<kGumpBaseSize;offset+=4) if (Get(actual,offset)!=Get(expected,offset))
                    std::printf("token %s +%02zx: %08x / %08x\n",part[0].c_str(),offset,Get(actual,offset),Get(expected,offset));
            }
            break;
        }++replayed;
    }
    CHECK(replayed==315);
}
}
// 10.78의 전체 생성 바이트/첫 빈 칸/루트 초기화 관찰을 재생한다.
TEST_CASE(GumpCreate_PatchNative) { Replay("originals"); }
// CD의 독립 기본 생성/루트 정수→float 저장/등록 몸체 관찰을 재생한다.
TEST_CASE(GumpCreate_CdNative) { Replay("originalCD"); }
// 추가판의 별도 PE 관찰도 같은 CD 구현에 대조한다.
TEST_CASE(GumpCreate_AdditionalNative) { Replay("original1037"); }

// 루트 할당 실패는 표를 비운 상태로 남기고 카운터/override/이전 객체를 보존한다.
TEST_CASE(GumpCreate_RootFailurePreservesOtherState) {
    GumpScene scene;scene.nodes[1].untouched=0x81234567;scene.state.entries.fill(1);scene.state.count=12;scene.state.serial=23;scene.state.collection={1,9};
    const auto before=scene.Nodes();auto hooks=scene.Hooks();hooks.allocate=[&](auto size) { CHECK(size==160);CHECK(scene.state.collection.root==0);return 0U; };
    GumpCreate create(scene.state,hooks);CHECK(create.InitializeRoot()==0);CHECK(scene.state.count==12 && scene.state.serial==23 && scene.state.collection.overrideHead==9);
    CHECK(scene.Nodes()==before);CHECK(std::all_of(scene.state.entries.begin(),scene.state.entries.end(),[](auto token) { return token==0; }));
}

// 할당 뒤의 현재 화면 크기/카운터와 serial 감김을 사용해 루트를 만든다.
TEST_CASE(GumpCreate_CurrentGlobalsAfterAllocation) {
    GumpScene scene;scene.nodes[1].untouched=0x81234567;scene.state.screenWidth=640;scene.state.screenHeight=480;scene.state.collection.overrideHead=9;
    auto hooks=scene.Hooks();hooks.allocate=[&](auto size) { CHECK(size==160);scene.state.screenWidth=16777217;scene.state.screenHeight=-16777217;scene.state.count=997;scene.state.serial=0xffffffff;return 1U; };
    GumpCreate create(scene.state,hooks);CHECK(create.InitializeRoot()==1);const auto& root=scene.nodes.at(1);
    CHECK(root.width==16777216.0F && root.height==-16777216.0F);CHECK(root.id==0 && root.flags==9 && root.serial==0 && root.command==0);
    CHECK(root.untouched==0x81234567);CHECK(scene.state.count==998 && scene.state.serial==0 && scene.state.collection.overrideHead==9);
}

// 일반 등록은 첫 빈 칸이고 미등록 객체도 serial은 얻는다. 표가 가득 찬 경우의 원본 정상 동작도 유지한다.
TEST_CASE(GumpCreate_FirstHoleUnregisteredAndFullTable) {
    GumpScene scene;scene.nodes[1]={};scene.nodes[2]={};scene.state.entries.fill(17);scene.state.entries[7]=0;scene.state.entries[999]=0;scene.state.count=12;scene.state.serial=23;
    GumpCreate create(scene.state,scene.Hooks());CHECK(create.Construct(1,27,4,5,1)==1);CHECK(scene.nodes[1].id==7 && scene.state.entries[7]==1 && scene.state.entries[999]==0);
    CHECK(scene.state.count==13 && scene.state.serial==24);create.Construct(2,28,6,7,0x100001);CHECK(scene.nodes[2].id==0 && scene.nodes[2].serial==25 && scene.state.count==13);
    scene.state.entries.fill(17);create.Construct(2,29,8,9,1);CHECK(scene.nodes[2].id==0 && scene.state.count==14);CHECK(scene.state.entries[0]==17 && scene.state.entries[999]==17);
}

// 생성 저장소의 표시 필드를 그대로 수집/Draw에 연결한다. 트리 부착은 이 검사에서 명시 입력으로 준다.
TEST_CASE(GumpCreate_CollectionDrawSameStorage) {
    GumpScene scene;scene.nodes[1]={};scene.nodes[2]={};scene.state.screenWidth=16;scene.state.screenHeight=16;
    auto hooks=scene.Hooks();hooks.allocate=[](auto) { return 1U; };GumpCreate create(scene.state,hooks);create.InitializeRoot();create.Construct(2,27,4,4,1|kGumpDrawClip);
    scene.nodes[1].child=2;scene.nodes[2].parent=1;scene.nodes[2].x=2;scene.nodes[2].y=3;
    GumpCollection collector(scene.state.collection,create.CollectionHooks());std::array<RenderCollectionEntry,4> work{};DrawCollectionHooks collection;BindGumpCollection(collection,collector);
    const auto result=CollectDrawEntries({{0,0,16,16},{},{},4,0},work,collection);CHECK(result.count==1);if (result.count!=1) return;
    CHECK(work[0].token==2 && work[0].depth==-40 && work[0].kind==2);CHECK(scene.nodes[2].cached.x==2 && scene.nodes[2].cached.y==3);
    GumpDrawState state;SetGumpDrawClip(state,{0,0,16,16});std::array<std::uint8_t,256> pixels{};GumpPixelTarget target(pixels,16,16,state);
    GumpDraw draw(collector,state,{[&](auto token) { CHECK(token==2);const GumpLine line{{0,3},{15,3},17};target.Lines(std::span(&line,1)); }});
    DrawDispatchHooks dispatch;BindGumpDraw(dispatch,draw);auto entries=std::span(work).first(1);SortDrawEntries(entries,scene.state.collection.edition);DispatchDrawEntries(entries,{},dispatch);
    CHECK(pixels[50]==17 && pixels[53]==17 && pixels[49]==0 && pixels[54]==0);CHECK(state.clip.right==16);CHECK(scene.state.count==2 && scene.state.entries[1]==2);
    scene.nodes[2].flags=0;CHECK(collector.Collect({0,0,16,16},work,4)==0);
}

// 숫자 한도/assert 입력·없는 저장소/할당 경계를 변경 전에 거부하고 미등록 생성은 한도와 구분한다.
TEST_CASE(GumpCreate_ContractsAndCountLimit) {
    GumpScene scene;scene.nodes[1].untouched=17;scene.state.count=999;auto hooks=scene.Hooks();GumpCreate create(scene.state,hooks);const auto before=scene.Nodes();
    CHECK(Throws([&] { GumpCreate invalid(scene.state,{}); }));CHECK(Throws([&] { create.Construct(0,0,1,1,1); }));CHECK(Throws([&] { create.Construct(99,0,1,1,1); }));
    CHECK(Throws([&] { create.Construct(1,0,1,1,1); }));CHECK(scene.Nodes()==before && scene.state.serial==0 && scene.state.count==999);
    CHECK(Throws([&] { create.InitializeRoot(); }));create.Construct(1,0,1,1,0x100001);CHECK(scene.state.count==999 && scene.state.serial==1);
    hooks.allocate=[](auto) { return 99U; };GumpCreate missing(scene.state,hooks);CHECK(Throws([&] { missing.InitializeRoot(); }));CHECK(scene.state.collection.root==0);
}
