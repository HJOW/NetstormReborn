// 원본 실제 커서 선택/콜백 전체 상태와 생성·부착·삭제의 같은 저장소 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpEvents.h"
#include "client/GumpTree.h"

namespace {
using namespace netstorm::test::gumpraw;
// 선택/전달 기대값은 실제 세 PE의 전체 몸체 관찰만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPEVENTS_FIXTURE).rows;return rows; }
// 콜백의 반환 DWORD와 명시 raw 쓰기 입력이다. 기대 상태 계산은 포함하지 않는다.
struct Response { /* value: 읽기·쓰기·통지에 전달하는 값. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint32_t value{};/* patches: 패치. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::array<std::uint32_t,3>> patches; };
// 생성·위치 수집·수명·선택이 같은 현재 노드를 사용하는 검사 저장소다.
struct EventScene {
    // nodes: 노드. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,GumpCreateNode> nodes;
    // create: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState create;
    // lifetime: 등록된 객체의 삭제·해제를 처리하는 수명 관리자. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpLifetimeState lifetime;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    GumpEventState state;
    // responses: 응답. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::string,Response> responses;
    // anchors: 앵커. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,ScreenPoint> anchors;
    // calls: 호출 횟수 또는 호출 인자를 대조하는 기록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string calls;
    // 처리 중 주소가 안정된 현재 노드만 공급한다.
    GumpCreateNode* Find(std::uint32_t token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; }
    // 원본 전역 순서대로 현재 raw DWORD를 읽는다.
    std::array<std::uint32_t,7> Globals() const {
        return {create.entries[0],lifetime.currentDialog,std::bit_cast<std::uint32_t>(state.point.x),std::bit_cast<std::uint32_t>(state.point.y),state.mask,state.bestDepth,state.selected};
    }
    // 명시 응답의 전역 index를 실제 공유 필드에 연결한다.
    void SetGlobal(std::uint32_t index,std::uint32_t value) {
        switch (index) {
            case 0:create.entries[0]=create.collection.root=value;break;case 1:lifetime.currentDialog=value;break;
            case 2:state.point.x=std::bit_cast<std::int32_t>(value);break;case 3:state.point.y=std::bit_cast<std::int32_t>(value);break;
            case 4:state.mask=value;break;case 5:state.bestDepth=value;break;case 6:state.selected=value;break;
            default:throw std::runtime_error("이벤트 전역 입력 index 오류");
        }
    }
    // 현재 선택 전역만 실제 raw 값으로 직렬화한다.
    std::string State() const {
        std::string result;
        // 일곱 전역은 모두 DWORD 비트다.
        for (auto value:Globals()) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 원본 offset에 대응하는 전체 노드의 현재 필드만 직렬화한다.
    std::string Nodes() const {
        std::string result;
        // 토큰 순서는 fixture와 동일하다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+"="+Encode(node); }return result;
    }
    // 콜백 직전 전체 상태와 인자를 기록한 뒤 완성 입력 쓰기만 적용한다.
    std::uint32_t Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+","+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 불투명 이벤트 인자는 해석하지 않고 전체 비트를 그대로 기록한다.
        for (auto value:args) calls+=","+std::to_string(value);
        calls+='@'+State()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return 0;
        // 입력에 명시한 노드 offset/전역 index만 바꾼다.
        for (const auto& patch:found->second.patches) {
            if (!patch[0]) SetGlobal(patch[1],patch[2]);
            else { auto bytes=Bytes(Encode(nodes.at(patch[0])));Put(bytes,patch[1],patch[2]);nodes.at(patch[0])=Decode(bytes); }
        }
        return found->second.value;
    }
    // same-node 수집기에 실제 월드 투영 응답을 공급한다.
    GumpCollectionHooks Collection() {
        return {[this](auto token)->GumpCollectionNode* { return Find(token); },[this](auto sid) { Respond("Project",sid);return anchors.at(sid); }};
    }
    // 기본 필터 바인딩 여부에 따라 실제 BaseFilter 또는 명시 가상 필터를 사용한다.
    GumpEventHooks Hooks(bool binding=false) {
        GumpEventHooks hooks;hooks.node=[this](auto token) { return Find(token); };
        if (!binding) hooks.filter=[this](auto token,auto event,auto first,auto second) { return Respond("Filter",token,{event,first,second})!=0; };
        hooks.handle=[this](auto token,auto event,auto first,auto second) { return Respond("Handle",token,{event,first,second}); };return hooks;
    }
};
// 같은 입력을 원본과 C++에 재생해 전체 상태/콜백 순서·인자/반환을 대조한다.
void Replay(std::string_view edition) {
    std::size_t count=0;
    // 한 행의 연속 연산은 공유 상태를 유지하며 다음 행에서는 새 저장소를 만든다.
    for (const auto& row:Fixture()) {
        CHECK(row.size()==14);if (row.size()!=14) return;if (row[0]!=edition) continue;EventScene scene;
        scene.create.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto globals=Split(row[2],',');
        // raw 전역/미사용 노드 바이트는 독립 입력을 그대로 읽는다.
        for (std::uint32_t i=0;i<globals.size();++i) scene.SetGlobal(i,Number(globals[i]));
        // 호스트 배치는 원본과 다르므로 공통 raw 직렬화기로 의미별 필드만 공급한다.
        for (const auto& item:Split(row[3],';')) { const auto p=Split(item,'=');scene.nodes.emplace(Number(p[0]),Decode(Bytes(p[1]))); }
        CHECK(scene.State()==row[2] && scene.Nodes()==row[3]);
        if (row[6]!="-") {
            // 중복 명시 응답은 마지막 입력으로 덮어쓴다. 선택/처리 결과를 유추하지 않는다.
            for (const auto& item:Split(row[6],';')) {
                const auto p=Split(item,':'),key=Split(p[0],',');Response response;response.value=Number(key[2]);
                if (p[1]!="-") {
                    // 위치/값을 완성 입력 그대로 저장한다.
                    for (const auto& raw:Split(p[1],'/')) { const auto fields=Inputs(raw);response.patches.push_back({fields[0],fields[1],fields[2]}); }
                }
                scene.responses[key[0]+","+key[1]]=std::move(response);
            }
        }
        if (row[7]!="-") {
            // 월드 투영의 정수점만 외부 경계이며 위치/경계/캐시는 실제 수집기가 계산한다.
            for (const auto& item:Split(row[7],';')) { const auto values=Inputs(item);scene.anchors[values[0]]={std::bit_cast<std::int32_t>(values[1]),std::bit_cast<std::int32_t>(values[2])}; }
        }
        GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.state,collection,scene.Hooks(row[5]=="1"));std::string results;
        // 명시 최상위 연산을 실제 복원 API에만 전달한다.
        for (const auto& item:Split(row[4],';')) {
            const auto fields=Split(item,',');const auto token=Number(fields[1]);const auto a=Inputs(item.substr(fields[0].size()+1));std::uint32_t result=0;
            if (fields[0]=="Base") result=events.BaseFilter(token,a[1],a[2],a[3]) ? 1U : 0U;
            else if (fields[0]=="Top") result=events.PickTopLevel({std::bit_cast<std::int32_t>(a[1]),std::bit_cast<std::int32_t>(a[2])});
            else if (fields[0]=="Pick") result=events.Pick({std::bit_cast<std::int32_t>(a[1]),std::bit_cast<std::int32_t>(a[2])},a[3]);
            else if (fields[0]=="Dispatch") result=events.Dispatch(token,a[1],a[2],a[3],a[4]!=0);
            else if (fields[0]=="Bubble") result=events.Bubble(token,a[1],a[2],a[3]);else throw std::runtime_error("이벤트 연산 입력 오류");
            if (!results.empty()) results+=',';results+=std::to_string(result);
        }
        const bool same=scene.Nodes()==row[8] && scene.State()==row[9] && results==row[10] && (scene.calls.empty() ? "-" : scene.calls)==row[11];CHECK(same);
        if (!same) { std::printf("GumpEvents %s %s: nodes=%d globals=%d results=%d calls=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[8],scene.State()==row[9],results==row[10],(scene.calls.empty() ? "-" : scene.calls)==row[11]);break; }
        ++count;
    }
    CHECK(count==257);
}
// 실제 생성/부착과 선택/전달/삭제 큐가 같은 저장소를 쓰는 정상 장면이다.
void Create(EventScene& scene) {
    scene.create.screenWidth=64;scene.create.screenHeight=48;
    // 루트/부모/형제/자식 네 개를 실제 기본 생성자로 준비한다.
    for (std::uint32_t token=1;token<=4;++token) scene.nodes.emplace(token,GumpCreateNode{});
    GumpCreate create(scene.create,{[&](auto token) { return scene.Find(token); },[&](auto) { return 1U; }});
    create.InitializeRoot();create.Construct(2,11,20,16,1);create.Construct(3,12,20,16,1);create.Construct(4,13,8,6,1);
    GumpTreeState state;GumpTree* current=nullptr;
    GumpTree tree(scene.create,state,{[&](auto token) { return scene.Find(token); },[](auto,bool) {},[&](auto token) { current->Push(token); },
        [&](auto token,float x,float y,auto parent,auto mode) { current->Attach(token,x,y,parent,mode); },[](auto) {},{}});current=&tree;
    tree.Attach(2,10,10,1,0);tree.Attach(3,10,10,1,0);tree.Attach(4,1,1,2,0);
}
}

// 기본 필터/경계/전체 트리/콜백 당시 상태와 반환을 판본별로 독립 대조한다.
TEST_CASE(GumpEvents_Native1078) { Replay("originals"); }
TEST_CASE(GumpEvents_NativeCd) { Replay("originalCD"); }
TEST_CASE(GumpEvents_Native1037) { Replay("original1037"); }

// 동일 노드의 위치/깊이와 실제 지연 삭제가 후속 커서 선택에도 즉시 반영된다.
TEST_CASE(GumpEvents_CreateTreeLifetimeIntegration) {
    EventScene scene;Create(scene);GumpCollection collection(scene.create.collection,scene.Collection());auto hooks=scene.Hooks(true);
    std::vector<std::uint32_t> handled;hooks.handle=[&](auto token,auto,auto,auto) { handled.push_back(token);return 7U; };
    GumpEvents events(scene.create,scene.lifetime,scene.state,collection,hooks);
    scene.nodes[3].depth=4;scene.nodes[2].depth=5;scene.nodes[4].depth=3;
    CHECK(events.PickTopLevel({12,12})==3);CHECK(events.Pick({12,12},0)==4);CHECK(scene.nodes[4].cached.x==11 && scene.nodes[4].cached.y==11);
    scene.nodes[3].flags|=0x400;CHECK(events.Dispatch(1,1,2,3)==3 && handled==std::vector<std::uint32_t>({3}));
    scene.lifetime.dialogs[0]=4;scene.lifetime.dialogCount=1;scene.lifetime.currentDialog=4;
    CHECK(events.BaseFilter(4,0xffffffff,999,999));
    auto lifeHooks=GumpLifetimeHooks{[&](auto token) { return scene.Find(token); },[](auto) {},{},[](auto,auto) {},{}};
    GumpLifetime lifetime(scene.create,scene.lifetime,lifeHooks);lifetime.Kill(2);
    CHECK(events.Pick({12,12},0)==3 && !scene.lifetime.currentDialog && scene.lifetime.killCount==2);
}

// 전달은 visible만 제한하고 cursor pick은 void/dead를 제외한다. 처리기 반환은 성공 토큰과 구분된다.
TEST_CASE(GumpEvents_DispatchUsesItsOwnFlagsAndReturnContract) {
    EventScene scene;Create(scene);GumpCollection collection(scene.create.collection,scene.Collection());auto hooks=scene.Hooks();
    hooks.filter=[](auto,auto,auto,auto) { return true; };hooks.handle=[](auto token,auto,auto,auto) { return token==3 ? 0x80000007U : 0U; };
    GumpEvents events(scene.create,scene.lifetime,scene.state,collection,hooks);scene.nodes[3].flags=0x10009;
    CHECK(events.Dispatch(1,1,2,3)==3);CHECK(events.Pick({12,12},0)!=3);
    scene.nodes[3].flags=0;CHECK(events.Dispatch(1,1,2,3)==0);CHECK(events.Dispatch(1,1,2,3,true)==3);
    hooks.handle=[](auto token,auto,auto,auto) { return token==2 ? 0x80000007U : 0U; };GumpEvents bubble(scene.create,scene.lifetime,scene.state,collection,hooks);
    CHECK(bubble.Bubble(4,1,2,3)==0x80000007U);
}

// help 사건은 화면 밖에서 일반 마우스/키보드 플래그로 통과하지 않으며 오른쪽/아래 끝을 제외한다.
TEST_CASE(GumpEvents_BaseFilterDialogAndHalfOpenBounds) {
    EventScene scene;Create(scene);GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.state,collection,scene.Hooks(true));
    scene.nodes[2].flags|=0xe00;CHECK(events.BaseFilter(2,0x4010000,10,10));CHECK(!events.BaseFilter(2,0x10000,30,10));
    CHECK(!events.BaseFilter(2,0x20000,10,26));CHECK(!events.BaseFilter(2,0x10000,9,10));
    CHECK(events.BaseFilter(2,1,0,0));CHECK(events.BaseFilter(2,0x2000001,999,999));scene.lifetime.currentDialog=2;CHECK(events.BaseFilter(2,0x10000,999,999));
}

// 정상 입력 밖의 토큰/누락 처리기/부모·형제·자식 순환은 호스트 계약으로 거부한다.
TEST_CASE(GumpEvents_ContractsAndCycles) {
    EventScene scene;Create(scene);GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.state,collection,scene.Hooks(true));
    CHECK(Throws([&] { GumpEvents invalid(scene.create,scene.lifetime,scene.state,collection,{}); }));CHECK(Throws([&] { events.Dispatch(99,1,2,3); }));
    auto hooks=scene.Hooks();hooks.handle={};GumpEvents noHandler(scene.create,scene.lifetime,scene.state,collection,hooks);CHECK(Throws([&] { noHandler.Bubble(2,1,2,3); }));
    scene.nodes[2].parent=2;CHECK(Throws([&] { events.Bubble(2,1,2,3); }));scene.nodes[2].parent=1;
    scene.nodes[3].next=3;CHECK(Throws([&] { events.PickTopLevel({12,12}); }));scene.nodes[3].next=2;
    scene.nodes[4].child=2;CHECK(Throws([&] { events.Dispatch(2,1,2,3); }));
}

// 콜백 예외 전의 현재 노드/전역 쓰기는 되돌리지 않는다. 원본 실제 예외 전파와는 별도 호스트 계약이다.
TEST_CASE(GumpEvents_CallbackExceptionKeepsCurrentWrites) {
    EventScene scene;Create(scene);GumpCollection collection(scene.create.collection,scene.Collection());auto hooks=scene.Hooks();
    hooks.filter=[&](auto token,auto,auto,auto)->bool { scene.nodes[token].command=999;scene.lifetime.currentDialog=token;throw std::runtime_error("필터 실패"); };
    GumpEvents events(scene.create,scene.lifetime,scene.state,collection,hooks);CHECK(Throws([&] { events.Dispatch(3,1,2,3); }));
    CHECK(scene.nodes[3].command==999 && scene.lifetime.currentDialog==3);
}
