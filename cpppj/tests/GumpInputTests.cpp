// 실제 원본 입력 계층의 전체 상태/가상 호출과 생성·선택·수명 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/GumpInput.h"
#include "client/GumpTree.h"
#include <memory>

namespace {
using namespace netstorm::test::gumpraw;
// 기계어가 기록한 완성 관찰만 기준으로 읽는다. 기대 UI 상태를 계산하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_GUMPINPUT_FIXTURE).rows;return rows; }
// 반환 DWORD와 명시 전역/표/노드 쓰기는 독립 입력이다.
struct Response { std::uint32_t value{};std::vector<std::array<std::uint32_t,3>> patches; };
// 같은 노드/등록/dialog/캡처/overlay/커서 전역을 사용하는 검사 저장소다.
struct InputScene {
    std::map<std::uint32_t,GumpCreateNode> nodes;
    GumpCreateState create;GumpLifetimeState lifetime;GumpEventState event;GumpInputState input;
    GumpTreeState treeState;std::unique_ptr<GumpTree> tree;
    std::map<std::string,Response> responses;std::map<std::uint32_t,ScreenPoint> anchors;std::string calls;
    // 호출 중 주소가 안정된 현재 생성 노드를 공급한다.
    GumpCreateNode* Find(std::uint32_t token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; }
    // 원본 순서의 일곱 선택 전역과 캡처/override/dialog 개수/overlay 사건이다.
    std::array<std::uint32_t,11> Globals() const {
        return {create.entries[0],lifetime.currentDialog,std::bit_cast<std::uint32_t>(event.point.x),std::bit_cast<std::uint32_t>(event.point.y),
            event.mask,event.bestDepth,event.selected,lifetime.focus,create.collection.overrideHead,lifetime.dialogCount,input.overlayEvent};
    }
    // raw 전역 index를 실제 공유 필드에 적용한다.
    void SetGlobal(std::uint32_t index,std::uint32_t value) {
        switch (index) {
            case 0:create.entries[0]=create.collection.root=value;break;case 1:lifetime.currentDialog=value;break;
            case 2:event.point.x=std::bit_cast<std::int32_t>(value);break;case 3:event.point.y=std::bit_cast<std::int32_t>(value);break;
            case 4:event.mask=value;break;case 5:event.bestDepth=value;break;case 6:event.selected=value;break;
            case 7:lifetime.focus=value;break;case 8:create.collection.overrideHead=value;break;case 9:lifetime.dialogCount=value;break;
            case 10:input.overlayEvent=value;break;default:throw std::runtime_error("입력 전역 index 오류");
        }
    }
    // 현재 전역의 전체 DWORD를 직렬화한다.
    std::string State() const {
        std::string result;
        // 모든 전역의 상위 비트와 정수 좌표 감김을 보존한다.
        for (auto value:Globals()) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 원본 두 표의 미사용 칸까지 직렬화한다.
    std::string Tables() const {
        std::string result;
        // overlay 표의 순서는 원본 처리 순서다.
        for (auto value:lifetime.overlay) { if (!result.empty()) result+=',';result+=std::to_string(value); }
        result+=';';bool first=true;
        // dialog 뒤 칸도 관찰하므로 새 등록의 위치와 불필요한 초기화를 검출한다.
        for (auto value:lifetime.dialogs) { if (!first) result+=',';first=false;result+=std::to_string(value); }return result;
    }
    // 모든 노드의 미사용 바이트를 포함해 원본 offset 순서로 기록한다.
    std::string Nodes() const {
        std::string result;
        // 안정된 토큰 순서는 원본 입력과 같다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+"="+Encode(node); }return result;
    }
    // 콜백 직전 실제 상태/인자를 기록한 뒤 명시 응답 쓰기만 적용한다.
    std::uint32_t Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+","+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 인자의 의미를 유추하지 않고 전체 DWORD 비트를 기록한다.
        for (auto value:args) calls+=","+std::to_string(value);
        calls+='@'+State()+'@'+Nodes()+'@'+Tables();const auto found=responses.find(key);if (found==responses.end()) return 0;
        // 0=전역, 1/2=표, 나머지=노드 토큰의 지정 DWORD 쓰기다.
        for (const auto& patch:found->second.patches) {
            if (!patch[0]) SetGlobal(patch[1],patch[2]);
            else if (patch[0]==1) lifetime.overlay.at(patch[1])=patch[2];
            else if (patch[0]==2) lifetime.dialogs.at(patch[1])=patch[2];
            else { auto raw=Bytes(Encode(nodes.at(patch[0])));Put(raw,patch[1],patch[2]);nodes.at(patch[0])=Decode(raw); }
        }
        return found->second.value;
    }
    // 월드 투영만 외부 응답이며 위치/경계/캐시는 실제 GumpCollection이 계산한다.
    GumpCollectionHooks Collection() {
        return {[this](auto token)->GumpCollectionNode* { return Find(token); },[this](auto sid) { Respond("Project",sid);return anchors.at(sid); }};
    }
    // 기본 필터 바인딩이 아니면 명시 가상 필터 응답을 공급한다.
    GumpEventHooks Events(bool binding=false) {
        GumpEventHooks hooks;hooks.node=[this](auto token) { return Find(token); };
        if (!binding) hooks.filter=[this](auto token,auto e,auto x,auto y) { return Respond("Filter",token,{e,x,y})!=0; };
        hooks.handle=[this](auto token,auto e,auto x,auto y) { return Respond("Handle",token,{e,x,y}); };return hooks;
    }
    // 직접 캡처/overlay와 하위 이벤트는 같은 가상 처리기 응답을 사용한다.
    GumpInputHooks Input() {
        return {[this](auto token) { return Find(token); },[this](auto token,auto e,auto x,auto y) { return Respond("Handle",token,{e,x,y}); },
            [this](auto code) { Respond("Overflow",code); }};
    }
    // 실제 기본 생성과 부착으로 부모/형제/자식 장면을 구성한다.
    void Create() {
        create.screenWidth=64;create.screenHeight=48;
        // 루트/직계 형제/손자 네 개를 생성기가 초기화할 저장소에 준비한다.
        for (std::uint32_t token=1;token<=4;++token) nodes.emplace(token,GumpCreateNode{});
        GumpCreate factory(create,{[this](auto token) { return Find(token); },[](auto) { return 1U; }});
        factory.InitializeRoot();factory.Construct(2,11,20,16,1);factory.Construct(3,12,20,16,1);factory.Construct(4,13,8,6,1);
        tree=std::make_unique<GumpTree>(create,treeState,GumpTreeHooks{[this](auto token) { return Find(token); },[](auto,bool) {},
            [this](auto token) { tree->Push(token); },[this](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); },[](auto) {},{}});
        tree->Attach(2,10,10,1,0);tree->Attach(3,10,10,1,0);tree->Attach(4,1,1,2,0);
    }
};
// 같은 입력에서 실제 전체 상태·반환·가상 호출 직전 상태를 원본과 대조한다.
void Replay(std::string_view edition) {
    std::size_t count=0;
    // 각 행의 연속 연산은 공유 상태를 유지하고 다음 행은 새 저장소에서 시작한다.
    for (const auto& row:Fixture()) {
        CHECK(row.size()==16);if (row.size()!=16) return;if (row[0]!=edition) continue;InputScene scene;
        scene.create.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto globals=Split(row[2],',');
        // 전역의 raw 비트는 기대 상태와 독립인 입력 그대로 공급한다.
        for (std::uint32_t i=0;i<globals.size();++i) scene.SetGlobal(i,Number(globals[i]));
        // 전체 노드는 호스트 메모리 배치 대신 의미별 필드로 복원한다.
        for (const auto& item:Split(row[3],';')) { const auto p=Split(item,'=');scene.nodes.emplace(Number(p[0]),Decode(Bytes(p[1]))); }
        const auto tables=Split(row[4],';');const auto overlay=Inputs(tables[0]),dialogs=Inputs(tables[1]);
        std::copy(overlay.begin(),overlay.end(),scene.lifetime.overlay.begin());std::copy(dialogs.begin(),dialogs.end(),scene.lifetime.dialogs.begin());
        CHECK(scene.State()==row[2] && scene.Nodes()==row[3] && scene.Tables()==row[4]);
        if (row[7]!="-") {
            // 중복 응답은 마지막 명시 입력으로 덮어쓴다. 선택/등록 결과를 계산하지 않는다.
            for (const auto& item:Split(row[7],';')) {
                const auto p=Split(item,':'),key=Split(p[0],',');Response response;response.value=Number(key[2]);
                if (p[1]!="-") {
                    // 입력 patch의 target/index/전체 DWORD만 그대로 적용한다.
                    for (const auto& raw:Split(p[1],'/')) { const auto values=Inputs(raw);response.patches.push_back({values[0],values[1],values[2]}); }
                }
                scene.responses[key[0]+","+key[1]]=std::move(response);
            }
        }
        if (row[8]!="-") {
            // 투영점만 경계 입력이며 float 위치/CRT 경계는 실제 복원 모듈을 사용한다.
            for (const auto& item:Split(row[8],';')) { const auto a=Inputs(item);scene.anchors[a[0]]={std::bit_cast<std::int32_t>(a[1]),std::bit_cast<std::int32_t>(a[2])}; }
        }
        GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,scene.Events(row[6]=="1"));
        GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,scene.Input());
        GumpLifetime lifetime(scene.create,scene.lifetime,{[&](auto token) { return scene.Find(token); },{},{},{},{}});std::string results;
        // 원본의 최상위 연산만 전달한다. void의 EAX는 의미 있는 반환으로 비교하지 않는다.
        for (const auto& item:Split(row[5],';')) {
            const auto fields=Split(item,',');const auto a=Inputs(item.substr(fields[0].size()+1));std::string result="-";
            if (fields[0]=="Capture") input.Capture(a[0]);
            else if (fields[0]=="Override") input.SetOverride(a[0],a[1]!=0);
            else if (fields[0]=="Dialog") result=input.AddDialog(a[1]) ? "1" : "0";
            else if (fields[0]=="Remove") result=lifetime.RemoveDialog(a[1]) ? "1" : "0";
            else if (fields[0]=="Route") result=std::to_string(input.Route(a[1],a[2],a[3],a[4]!=0));
            else if (fields[0]=="Overlay") result=std::to_string(input.Overlay(a[1],{std::bit_cast<std::int32_t>(a[2]),std::bit_cast<std::int32_t>(a[3])}));
            else throw std::runtime_error("입력 연산 이름 오류");
            if (!results.empty()) results+=',';results+=result;
        }
        const bool same=scene.Nodes()==row[9] && scene.State()==row[10] && scene.Tables()==row[11] && results==row[12] && (scene.calls.empty() ? "-" : scene.calls)==row[13];CHECK(same);
        if (!same) { std::printf("GumpInput %s %s: nodes=%d globals=%d tables=%d results=%d calls=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[9],scene.State()==row[10],scene.Tables()==row[11],results==row[12],(scene.calls.empty() ? "-" : scene.calls)==row[13]);break; }
        ++count;
    }
    CHECK(count==232);
}
}

// 각 판본의 전체 입력 몸체/하위 실제 모듈/가상 콜백 상태를 독립 대조한다.
TEST_CASE(GumpInput_Native1078) { Replay("originals"); }
TEST_CASE(GumpInput_NativeCd) { Replay("originalCD"); }
TEST_CASE(GumpInput_Native1037) { Replay("original1037"); }

// 등록/캡처/실제 Push·삭제 요청과 전체 선택이 같은 생성 저장소를 사용한다.
TEST_CASE(GumpInput_CreateCaptureDialogLifetimeIntegration) {
    InputScene scene;scene.Create();scene.nodes[1].flags|=1;scene.nodes[2].flags|=0x400;scene.nodes[3].flags|=0x400;
    GumpCollection collection(scene.create.collection,scene.Collection());auto eventHooks=scene.Events(true);
    eventHooks.handle=[](auto,auto,auto,auto) { return 0x80000007U; };auto inputHooks=scene.Input();inputHooks.handle=eventHooks.handle;
    GumpEvents events(scene.create,scene.lifetime,scene.event,collection,eventHooks);GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,inputHooks);
    CHECK(input.Route(1,2,3)==3);input.Capture(4);input.Capture(2);CHECK(scene.lifetime.focus==4 && input.Route(1,2,3)==4 && !scene.lifetime.focus);
    CHECK(input.AddDialog(2) && input.Route(1,2,3)==2);CHECK(input.AddDialog(3) && !input.AddDialog(2) && scene.lifetime.currentDialog==3);
    GumpLifetime lifetime(scene.create,scene.lifetime,{[&](auto token) { return scene.Find(token); },[&](auto token) { scene.tree->Push(token); },{},{},{}});
    CHECK(lifetime.RemoveDialog(3) && scene.lifetime.currentDialog==2);input.Capture(4);lifetime.Kill(2);
    CHECK(!scene.lifetime.focus && !scene.lifetime.currentDialog && scene.lifetime.killCount==2 && scene.nodes[3].next==0);
    CHECK(input.Route(1,2,3)==3 && events.Pick({12,12},0)==3);lifetime.SetOverlay(3,true);scene.input.overlayEvent=123;
    CHECK(input.Overlay(999,{12,12})==0x80000007U);input.SetOverride(3,true);CHECK(scene.create.collection.overrideHead==3);
    input.SetOverride(2,true);CHECK(!scene.create.collection.overrideHead);lifetime.SetOverlay(3,false);
}

// 실패 캡처 뒤 재전달이 없으며 성공에서는 콜백이 바꾼 현재 캡처를 반환한다.
TEST_CASE(GumpInput_CaptureCurrentValueAndNoFallback) {
    InputScene scene;scene.Create();std::vector<std::uint32_t> handled;std::uint32_t filters=0;bool success=false;
    auto eventHooks=scene.Events();eventHooks.filter=[&](auto,auto,auto,auto) { ++filters;return true; };
    auto hooks=scene.Input();hooks.handle=[&](auto token,auto,auto,auto) { handled.push_back(token);scene.lifetime.focus=success ? 3U : 2U;return success ? 7U : 0U; };
    GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,eventHooks);
    GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,hooks);input.Capture(4);
    CHECK(input.Route(1,2,3)==0 && scene.lifetime.focus==2 && handled==std::vector<std::uint32_t>({4}) && !filters);
    success=true;CHECK(input.Route(1,2,3)==3 && !scene.lifetime.focus && handled==std::vector<std::uint32_t>({4,2}));
}

// overlay는 부모 visible과 실제 Pick을 확인하고 사건 전역/전체 반환/첫 0 반환을 보존한다.
TEST_CASE(GumpInput_OverlayActualPickParentsAndZeroReturn) {
    InputScene scene;scene.Create();scene.nodes[1].flags|=1;scene.nodes[2].depth=1;scene.nodes[3].depth=5;scene.nodes[4].depth=2;
    scene.lifetime.overlay[0]=2;scene.input.overlayEvent=123;std::uint32_t calls=0,received=0,value=0x80000007;
    auto hooks=scene.Input();hooks.handle=[&](auto,auto e,auto,auto) { ++calls;received=e;return value; };
    GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,scene.Events());
    GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,hooks);CHECK(input.Overlay(999,{12,12})==value && calls==1 && received==123);
    scene.nodes[1].flags=0;scene.nodes[2].cached={99,99};CHECK(!input.Overlay(999,{12,12}) && scene.nodes[2].cached.x==99 && calls==1);
    scene.nodes[1].flags=0x10001;CHECK(input.Overlay(999,{12,12})==value && calls==2);
    scene.nodes[2].flags=9;CHECK(!input.Overlay(999,{12,12}) && calls==2);scene.nodes[2].flags=1;scene.nodes[2].depth=10;
    CHECK(!input.Overlay(999,{12,12}) && scene.event.selected==4 && calls==2);scene.lifetime.overlay[99]=4;
    CHECK(input.Overlay(999,{12,12})==value && calls==3);CHECK(!input.Overlay(999,{19,12}));
    scene.nodes[2].depth=1;scene.lifetime.overlay[99]=2;value=0;const auto before=calls;
    CHECK(!input.Overlay(999,{12,12}) && calls==before+1);
}

// 가득 찬 표는 중복 검색보다 먼저 로그를 호출한다. 미사용 마지막 칸은 중복으로 취급하지 않는다.
TEST_CASE(GumpInput_DialogCapacityBeforeDuplicate) {
    InputScene scene;scene.Create();std::uint32_t overflows=0;auto hooks=scene.Input();
    hooks.overflow=[&](auto code) { CHECK(code==19);++overflows;scene.lifetime.currentDialog=4; };
    GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,scene.Events());
    GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,hooks);scene.lifetime.dialogs.fill(3);scene.lifetime.dialogCount=16;
    CHECK(!input.AddDialog(3) && overflows==1 && scene.lifetime.dialogCount==16 && scene.lifetime.currentDialog==4);
    scene.lifetime.dialogCount=15;scene.lifetime.dialogs[15]=2;CHECK(input.AddDialog(2) && scene.lifetime.dialogCount==16 && scene.lifetime.currentDialog==2);
}

// 잘못된 토큰/부모 순환/누락 경계/음수 dialog 개수는 원본 정상 계약 밖의 호스트 입력이다.
TEST_CASE(GumpInput_Contracts) {
    InputScene scene;scene.Create();GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,scene.Events());
    GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,scene.Input());CHECK(Throws([&] { GumpInput invalid(scene.create,scene.lifetime,scene.input,collection,events,{}); }));
    input.Capture(99);CHECK(Throws([&] { input.Route(1,2,3); }));scene.lifetime.focus=0;
    scene.lifetime.overlay[0]=99;CHECK(Throws([&] { input.Overlay(1,{12,12}); }));scene.lifetime.overlay[0]=2;scene.nodes[2].parent=2;
    CHECK(Throws([&] { input.Overlay(1,{12,12}); }));scene.nodes[2].parent=1;scene.lifetime.dialogCount=0xffffffff;
    CHECK(Throws([&] { input.AddDialog(2); }));auto hooks=scene.Input();hooks.handle={};hooks.overflow={};
    GumpInput missing(scene.create,scene.lifetime,scene.input,collection,events,hooks);scene.lifetime.focus=2;CHECK(Throws([&] { missing.Route(1,2,3); }));
    scene.lifetime.dialogCount=16;CHECK(Throws([&] { missing.AddDialog(2); }));
}

// 처리기 예외 이전의 캡처/노드 쓰기는 그대로 남긴다. 실제 PE 예외 전파는 별도 범위다.
TEST_CASE(GumpInput_CallbackExceptionKeepsWrites) {
    InputScene scene;scene.Create();auto hooks=scene.Input();hooks.handle=[&](auto token,auto,auto,auto)->std::uint32_t {
        scene.nodes[token].command=999;scene.lifetime.focus=3;throw std::runtime_error("입력 처리기 실패");
    };
    GumpCollection collection(scene.create.collection,scene.Collection());GumpEvents events(scene.create,scene.lifetime,scene.event,collection,scene.Events());
    GumpInput input(scene.create,scene.lifetime,scene.input,collection,events,hooks);input.Capture(2);CHECK(Throws([&] { input.Route(1,2,3); }));
    CHECK(scene.nodes[2].command==999 && scene.lifetime.focus==3);
}
