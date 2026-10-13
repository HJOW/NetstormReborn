// 실제 원본 전체 상태/콜백과 생성→부착→지연 삭제→등록 칸 재사용을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpLifetime.h"
#include "client/GumpTree.h"

namespace {
using namespace netstorm::test::gumpraw;
// 기대 삭제 상태는 실제 세 PE의 격리 실행 관찰만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPLIFETIME_FIXTURE).rows;return rows; }
// 전체 DWORD 표를 연속 같은 값의 길이:값으로 직렬화한다. 기대 결과를 계산하지 않는다.
std::string Rle(std::span<const std::uint32_t> values) {
    std::string result;
    // 현재 표의 연속 구간만 압축한다.
    for (std::size_t i=0;i<values.size();) {
        const auto start=i;const auto value=values[i];
        // 같은 DWORD가 이어지는 끝 위치만 찾는다.
        while (i<values.size() && values[i]==value) ++i;
        if (!result.empty()) result+=';';result+=std::to_string(i-start)+":"+std::to_string(value);
    }
    return result;
}
// 원본 입력 RLE를 고정 길이 표에 푼다. 길이가 틀리면 검사 입력을 거부한다.
void ReadTable(std::string_view input,std::span<std::uint32_t> values) {
    std::size_t offset=0;
    // 각 구간은 완성 입력값이며 삭제 결과를 유추하지 않는다.
    for (const auto& item:Split(std::string(input),';')) {
        const auto p=Split(item,':');const auto length=Number(p[0]);if (length>values.size()-offset) throw std::runtime_error("수명 표 입력 범위 초과");
        std::fill_n(values.begin()+offset,length,Number(p[1]));offset+=length;
    }
    if (offset!=values.size()) throw std::runtime_error("수명 표 입력 길이 오류");
}
// 생성기와 같은 노드/등록 표 및 수명 전역을 보유하는 관찰 장면이다.
struct LifetimeScene {
    // nodes: 노드. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,GumpCreateNode> nodes;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    GumpCreateState state;
    // life: 원본의 고정 크기 idle/overlay/삭제/dialog 표와 현재 삭제 전역이다 (GumpLifetimeState).
    GumpLifetimeState life;
    // responses: 응답. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::string,std::vector<std::array<std::uint32_t,3>>> responses;
    // calls: 호출 횟수 또는 호출 인자를 대조하는 기록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string calls;
    // 안정된 주소로 현재 노드를 공급한다.
    GumpCreateNode* Find(std::uint32_t token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; }
    // 판본별 실제 표의 활성 구간만 선택한다.
    std::span<std::uint32_t> Table(std::uint32_t index) {
        switch (index) { case 1:return state.entries;case 2:return std::span(life.idle).first(state.collection.edition==OriginalEdition::Patch1078 ? 500U : 80U);
            case 3:return life.overlay;case 4:return life.kill;case 5:return life.dialogs;default:throw std::runtime_error("수명 표 index 오류"); }
    }
    // fixture 순서의 raw 스칼라를 현재 공유 필드에 연결한다.
    std::uint32_t& Global(std::uint32_t index) {
        switch (index) { case 0:return state.count;case 1:return life.killCount;case 2:return life.dialogCount;case 3:return life.focus;
            case 4:return state.collection.overrideHead;case 5:return life.currentDialog;case 6:return life.diagnostics;default:throw std::runtime_error("수명 전역 index 오류"); }
    }
    // 현재 스칼라와 전체 다섯 표를 원본과 같은 순서로 관찰한다.
    std::string State() {
        std::string result;
        // 스칼라는 모두 raw DWORD다.
        for (std::uint32_t i=0;i<7;++i) { if (i) result+=',';result+=std::to_string(Global(i)); }
        // 원본 표의 활성 길이만 직렬화한다.
        for (std::uint32_t i=1;i<=5;++i) result+='#'+Rle(Table(i));return result;
    }
    // 전체 40 DWORD를 주소 순서대로 관찰한다.
    std::string Nodes() const {
        std::string result;
        // fixture 입력의 토큰 순서를 유지한다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+"="+Encode(node); }return result;
    }
    // 경계 당시 전체 상태를 기록한 뒤 명시 입력 응답 DWORD만 쓴다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+","+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 인자 순서/raw 값은 실제 호출과 같다.
        for (auto value:args) calls+=","+std::to_string(value);
        calls+='@'+State()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return;
        // 응답 위치/값은 fixture의 완성 입력만 사용한다.
        for (const auto& p:found->second) {
            if (!p[0]) Global(p[1])=p[2];
            else if (p[0]<=5) { Table(p[0])[p[1]]=p[2];state.collection.root=state.entries[0]; }
            else { auto raw=Bytes(Encode(nodes.at(p[0])));Put(raw,p[1],p[2]);nodes.at(p[0])=Decode(raw); }
        }
    }
    // 가상/해제 경계를 현재 저장소의 관찰기로 연결한다.
    GumpLifetimeHooks Hooks() {
        return {[this](auto token) { return Find(token); },[this](auto token) { Respond("Push",token); },
            [this](auto form,auto flags) { Respond("Form",form,{flags}); },[this](auto token,auto flags) { Respond("Destructor",token,{flags}); },
            [this](auto pointer) { Respond("Free",pointer); }};
    }
};
// 원본 입력을 같은 공유 저장소에 재생해 전체 노드/표/전역/반환/경계 상태를 대조한다.
void Replay(std::string_view edition) {
    std::size_t count=0;
    // 연속 연산은 현재 상태를 이어 쓰며 행 사이에는 완전히 새 저장소를 사용한다.
    for (const auto& row:Fixture()) {
        CHECK(row.size()==13);if (row.size()!=13) return;if (row[0]!=edition) continue;LifetimeScene scene;
        scene.state.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto parts=Split(row[2],'#'),globals=Split(parts[0],',');
        // 원본 전역은 raw DWORD 입력이다.
        for (std::uint32_t i=0;i<globals.size();++i) scene.Global(i)=Number(globals[i]);
        // 고정 길이 표를 입력 RLE 그대로 준비한다.
        for (std::uint32_t i=1;i<=5;++i) ReadTable(parts[i],scene.Table(i));scene.state.collection.root=scene.state.entries[0];
        // 전체 raw 노드의 미사용 필드도 대조 대상이다.
        for (const auto& item:Split(row[3],';')) { const auto p=Split(item,'=');scene.nodes.emplace(Number(p[0]),Decode(Bytes(p[1]))); }
        CHECK(scene.State()==row[2] && scene.Nodes()==row[3]);
        if (row[5]!="-") {
            // 가상 경계 종류/this마다 명시 응답을 준비한다.
            for (const auto& item:Split(row[5],';')) {
                const auto p=Split(item,':');
                // 각 패치는 위치/값 그대로 적용되며 기대 삭제 결과를 계산하지 않는다.
                for (const auto& patch:Split(p[1],'/')) { const auto v=Inputs(patch);scene.responses[p[0]].push_back({v[0],v[1],v[2]}); }
            }
        }
        auto hooks=scene.Hooks();GumpLifetime* current=nullptr;
        if (row[6]=="1") hooks.deletingDestructor=[&](auto token,auto flags) { current->DeleteScalar(token,flags); };
        GumpLifetime lifetime(scene.state,scene.life,hooks);current=&lifetime;std::string results;
        // C++ 공개 함수를 실제 호출하고 정상 반환을 원본 관찰과 비교한다.
        for (const auto& item:Split(row[4],';')) {
            const auto p=Split(item,',');const auto token=Number(p[1]);std::string result="-";
            if (p[0]=="Kill") lifetime.Kill(token);else if (p[0]=="Cleanup") lifetime.Cleanup();else if (p[0]=="Destroy") lifetime.Destroy(token);
            else if (p[0]=="Base") lifetime.DestructBase(token);else if (p[0]=="Scalar") result=std::to_string(lifetime.DeleteScalar(token,Number(p[2])));
            else if (p[0]=="Idle") lifetime.SetIdle(token,Number(p[2])!=0);else if (p[0]=="Overlay") lifetime.SetOverlay(token,Number(p[2])!=0);
            else if (p[0]=="Dialog") result=lifetime.RemoveDialog(token) ? "1" : "0";else throw std::runtime_error("수명 연산 오류");
            if (!results.empty()) results+='/';results+=result;
        }
        const bool same=scene.Nodes()==row[7] && scene.State()==row[8] && results==row[9] && (scene.calls.empty() ? "-" : scene.calls)==row[10];CHECK(same);
        if (!same) { std::printf("GumpLifetime %s %s: nodes=%d state=%d results=%d calls=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[7],scene.State()==row[8],results==row[9],(scene.calls.empty() ? "-" : scene.calls)==row[10]);break; }++count;
    }
    CHECK(count==Fixture().size()/3);
}
// 실제 생성기로 루트/일반 Gump를 같은 저장소에 만든다.
void Create(LifetimeScene& scene) {
    scene.nodes[1]={};scene.nodes[2]={};scene.nodes[3]={};scene.nodes[4]={};scene.state.screenWidth=32;scene.state.screenHeight=24;
    GumpCreate create(scene.state,{[&](auto token) { return scene.Find(token); },[](auto) { return 1U; }});create.InitializeRoot();
    create.Construct(2,27,8,6,1);create.Construct(3,27,3,2,1);create.Construct(4,27,2,2,1);
}
}
// 10.78의 전체 삭제 수명/경계 당시 상태를 실제 PE와 대조한다.
TEST_CASE(GumpLifetime_PatchNative) { Replay("originals"); }
// CD의 80칸 idle/별도 자식 함수/삭제 쓰기 순서를 별도 PE 관찰과 대조한다.
TEST_CASE(GumpLifetime_CdNative) { Replay("originalCD"); }
// 추가판도 독립 PE 관찰 전체를 대조한다.
TEST_CASE(GumpLifetime_AdditionalNative) { Replay("original1037"); }

// 실제 생성/부착/Push/기본 소멸자/저장소 해제/빈 등록 칸 재사용까지 연결한다.
TEST_CASE(GumpLifetime_CreateTreeKillCleanupAndReuse) {
    LifetimeScene scene;Create(scene);GumpTreeState state;GumpTree* treePointer=nullptr;GumpLifetime* lifePointer=nullptr;
    std::vector<std::uint32_t> freed;auto hooks=scene.Hooks();hooks.push=[&](auto token) { treePointer->Push(token); };
    hooks.deletingDestructor=[&](auto token,auto flags) { lifePointer->DeleteScalar(token,flags); };
    hooks.release=[&](auto token) { freed.push_back(token);scene.nodes.erase(token); };
    GumpLifetime lifetime(scene.state,scene.life,hooks);lifePointer=&lifetime;
    GumpTree tree(scene.state,state,{[&](auto token) { return scene.Find(token); },[](auto,bool) {},[&](auto token) { treePointer->Push(token); },
        [&](auto token,float x,float y,auto parent,auto mode) { treePointer->Attach(token,x,y,parent,mode); },[&](auto token) { lifetime.Kill(token); },{}});treePointer=&tree;
    tree.Attach(2,2,3,1,0);tree.Attach(3,1,1,2,0);tree.Attach(4,2,2,2,0);
    lifetime.SetIdle(3,true);lifetime.SetOverlay(4,true);scene.life.focus=3;scene.state.collection.overrideHead=4;
    scene.life.dialogs[0]=3;scene.life.dialogs[1]=4;scene.life.dialogCount=2;scene.life.currentDialog=4;
    lifetime.Kill(2);CHECK(scene.life.killCount==3 && scene.life.kill[0]==4 && scene.life.kill[1]==3 && scene.life.kill[2]==2);
    CHECK(scene.nodes[1].child==0 && scene.nodes[2].child==0 && !scene.life.focus && !scene.state.collection.overrideHead && !scene.life.dialogCount);
    GumpCreate create(scene.state,{[&](auto token) { return scene.Find(token); },{}});GumpCollection collection(scene.state.collection,create.CollectionHooks());
    std::array<RenderCollectionEntry,4> work{};CHECK(collection.Collect({0,0,32,24},work,4)==0);CHECK(scene.state.count==4);
    lifetime.Kill(3);CHECK(scene.life.killCount==4);lifetime.Cleanup();CHECK(freed==std::vector<std::uint32_t>({4,3,2}));
    CHECK(scene.state.count==1 && scene.state.entries[0]==1 && scene.state.collection.root==1 && scene.life.killCount==0 && scene.nodes.size()==1);
    scene.nodes[5]={};create.Construct(5,27,1,1,1);CHECK(scene.nodes[5].id==1 && scene.nodes[5].serial==5 && scene.state.count==2);
    tree.Attach(5,1,2,1,0);CHECK(collection.Collect({0,0,32,24},work,4)==1 && work[0].token==5);
}

// 0x4000 부착 교체는 실제 Kill에 전달되어 기존 형제를 분리하고 큐에만 넣는다.
TEST_CASE(GumpLifetime_TreeReplacementDefersDestruction) {
    LifetimeScene scene;Create(scene);GumpTreeState state;GumpTree* treePointer=nullptr;auto hooks=scene.Hooks();hooks.push=[&](auto token) { treePointer->Push(token); };
    GumpLifetime lifetime(scene.state,scene.life,hooks);
    GumpTree tree(scene.state,state,{[&](auto token) { return scene.Find(token); },[](auto,bool) {},[&](auto token) { treePointer->Push(token); },
        [&](auto token,float x,float y,auto parent,auto mode) { treePointer->Attach(token,x,y,parent,mode); },[&](auto token) { lifetime.Kill(token); },{}});treePointer=&tree;
    scene.nodes[2].flags|=0x4000;scene.nodes[3].flags|=0x4000;tree.Attach(2,0,0,1,0);tree.Attach(3,0,0,1,0);
    CHECK(scene.nodes[1].child==3 && scene.life.killCount==1 && scene.life.kill[0]==2 && scene.state.count==4 && (scene.nodes[2].flags&0x10000));
}

// 기본 소멸자만 직접 실행하면 등록 개수/dialog/flags는 유지되고 표의 모든 일치만 지워진다.
TEST_CASE(GumpLifetime_BaseDoesNotUnregisterOrRemoveDialogs) {
    LifetimeScene scene;Create(scene);const auto flags=scene.nodes[2].flags;scene.life.idle[1]=scene.life.idle[499]=2;
    scene.life.overlay[0]=scene.life.overlay[99]=2;scene.life.kill[0]=scene.life.kill[999]=2;scene.life.killCount=1;
    scene.life.dialogs[0]=2;scene.life.dialogCount=1;scene.life.currentDialog=2;
    GumpLifetime lifetime(scene.state,scene.life,scene.Hooks());lifetime.DestructBase(2);
    CHECK(!scene.life.idle[1] && !scene.life.idle[499] && !scene.life.overlay[99] && !scene.life.kill[999]);
    CHECK(scene.state.count==4 && scene.state.entries[1]==2 && scene.life.killCount==1 && scene.life.dialogCount==1 && scene.nodes[2].flags==flags);
}

// CD의 실제 idle 표 밖에 있는 호스트 보관 칸은 기본 소멸자가 건드리지 않는다.
TEST_CASE(GumpLifetime_CdIdleCapacity) {
    LifetimeScene scene;Create(scene);scene.state.collection.edition=OriginalEdition::Cd1072;scene.life.idle.fill(3);scene.life.idle[79]=0;
    scene.life.idle[80]=2;GumpLifetime lifetime(scene.state,scene.life,scene.Hooks());lifetime.SetIdle(2,true);
    CHECK(scene.life.idle[79]==2);lifetime.DestructBase(2);CHECK(scene.life.idle[79]==0 && scene.life.idle[80]==2);
}

// 원본 정상 입력 밖의 잘못된 토큰/표/플래그/순환은 호스트 예외로 거부한다.
TEST_CASE(GumpLifetime_ContractsAndCycles) {
    LifetimeScene scene;Create(scene);GumpLifetime lifetime(scene.state,scene.life,scene.Hooks());CHECK(Throws([&] { GumpLifetime invalid(scene.state,scene.life,{}); }));
    CHECK(Throws([&] { lifetime.Kill(99); }));scene.nodes[2].flags|=0x80;CHECK(Throws([&] { lifetime.SetIdle(2,false); }));scene.nodes[2].flags&=~0x80U;
    scene.life.killCount=999;CHECK(Throws([&] { lifetime.Kill(2); }));scene.life.killCount=0;scene.life.dialogCount=17;CHECK(Throws([&] { lifetime.RemoveDialog(2); }));scene.life.dialogCount=0;
    scene.nodes[2].child=3;scene.nodes[3].child=2;CHECK(Throws([&] { lifetime.Kill(2); }));scene.nodes[2].child=0;scene.nodes[3].child=0;
    scene.nodes[2].id=1000;CHECK(Throws([&] { lifetime.Destroy(2); }));scene.nodes[2].id=1;
    scene.life.killCount=1;scene.life.kill[0]=2;CHECK(Throws([&] { lifetime.Cleanup(); }));scene.life.killCount=0;
    lifetime.SetOverlay(2,true);CHECK(Throws([&] { lifetime.SetOverlay(2,true); }));
}

// 해제 경계 예외가 발생하면 원본에서 그 전에 수행한 등록/flags/기본 가상 표 쓰기는 남긴다.
TEST_CASE(GumpLifetime_CallbackExceptionKeepsWrittenState) {
    LifetimeScene scene;Create(scene);auto hooks=scene.Hooks();hooks.deletingDestructor=[](auto,auto) { throw std::runtime_error("가상 소멸자 실패"); };
    GumpLifetime lifetime(scene.state,scene.life,hooks);scene.nodes[2].flags|=0x10000;CHECK(Throws([&] { lifetime.Destroy(2); }));
    CHECK(scene.state.count==3 && !scene.state.entries[1] && (scene.nodes[2].flags&4) && !(scene.nodes[2].flags&0x10000));
    hooks.release=[](auto) { throw std::runtime_error("텍스트 해제 실패"); };GumpLifetime base(scene.state,scene.life,hooks);
    scene.nodes[3].vtable=123;scene.nodes[3].textPointer=456;CHECK(Throws([&] { base.DestructBase(3); }));CHECK(scene.nodes[3].vtable==0x50ac70 && scene.nodes[3].textPointer==456);
}
