// 기본/루트 생성의 전체 바이트 독립 관찰과 동일 저장소의 수집/Draw 연결을 검사한다.
#include "ConstructionSupport.h"
#include "client/GumpCreate.h"
#include "client/GumpDraw.h"
#include "client/DrawSort.h"

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 기대 초기화/등록 바이트는 재계산하지 않고 실제 PE fixture에서만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPCREATE_FIXTURE).rows;return rows; }
// 원본 입력/출력의 16진수를 바이트로 바꾼다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 한 실제 raw 바이트다.
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
// 원본 0xa0바이트를 의미별 호스트 필드로 읽는다. 호스트 메모리 배치는 원본 배치와 별개다.
GumpCreateNode Decode(std::span<const std::uint8_t> raw) {
    GumpCreateNode n;n.vtable=Get(raw,0);n.flags=Get(raw,4);n.alignFlags=Get(raw,8);n.next=Get(raw,12);n.previous=Get(raw,16);
    n.parent=Get(raw,20);n.child=Get(raw,24);n.id=Get(raw,28);n.command=Get(raw,32);n.object=Get(raw,36);n.form=Get(raw,40);n.serial=Get(raw,44);
    n.depth=Get(raw,48);n.secondaryDepth=Get(raw,52);n.scale=Float(Get(raw,56));n.baseWidth=Float(Get(raw,60));n.baseHeight=Float(Get(raw,64));
    n.width=Float(Get(raw,68));n.height=Float(Get(raw,72));n.x=Float(Get(raw,76));n.y=Float(Get(raw,80));
    // 위치/속도 여섯 필드는 원본 DWORD 비트로 유지한다.
    for (std::size_t i=0;i<n.positioning.size();++i) n.positioning[i]=Get(raw,84+i*4);
    n.cached={std::bit_cast<std::int32_t>(Get(raw,108)),std::bit_cast<std::int32_t>(Get(raw,112))};n.untouched=Get(raw,116);
    n.textPointer=Get(raw,120);n.alternateTextPointer=Get(raw,124);
    // 글자 문맥 여덟 필드도 생성 전 임의 입력을 그대로 공급한다.
    for (std::size_t i=0;i<n.textContext.size();++i) n.textContext[i]=Get(raw,128+i*4);
    return n;
}
// 현재 필드를 원본 offset으로만 직렬화한다. 기대 값/생성 분기 계산은 하지 않는다.
std::string Encode(const GumpCreateNode& n) {
    std::array<std::uint8_t,kGumpBaseSize> raw{};
    Put(raw,0,n.vtable);Put(raw,4,n.flags);Put(raw,8,n.alignFlags);Put(raw,12,n.next);Put(raw,16,n.previous);Put(raw,20,n.parent);
    Put(raw,24,n.child);Put(raw,28,n.id);Put(raw,32,n.command);Put(raw,36,n.object);Put(raw,40,n.form);Put(raw,44,n.serial);Put(raw,48,n.depth);Put(raw,52,n.secondaryDepth);
    Put(raw,56,Bits(n.scale));Put(raw,60,Bits(n.baseWidth));Put(raw,64,Bits(n.baseHeight));Put(raw,68,Bits(n.width));Put(raw,72,Bits(n.height));Put(raw,76,Bits(n.x));Put(raw,80,Bits(n.y));
    // 실제 현재 위치/속도 필드를 원래 여섯 위치로 쓴다.
    for (std::size_t i=0;i<n.positioning.size();++i) Put(raw,84+i*4,n.positioning[i]);
    Put(raw,108,std::bit_cast<std::uint32_t>(n.cached.x));Put(raw,112,std::bit_cast<std::uint32_t>(n.cached.y));Put(raw,116,n.untouched);Put(raw,120,n.textPointer);Put(raw,124,n.alternateTextPointer);
    // 실제 현재 문맥을 원본 바이트 순서로 쓴다.
    for (std::size_t i=0;i<n.textContext.size();++i) Put(raw,128+i*4,n.textContext[i]);
    return Hex(raw);
}
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
    std::map<std::uint32_t,GumpCreateNode> nodes;
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
