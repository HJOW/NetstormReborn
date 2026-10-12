// 실제 세 PE의 독립 관찰과 같은 생성·수명 저장소의 프레임 전달을 비교한다.
#include "GumpRawSupport.h"
#include "client/GumpFrame.h"
#include "client/GumpTree.h"
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

namespace {
using namespace netstorm::test::gumpraw;
// 실제 관찰 파일의 판본당 완성 입력 행 수다. 기대 통지/재그리기 순서는 fixture에서만 읽는다.
constexpr std::size_t kFixtureCases=270;
// 전체 표를 길이:DWORD 값의 연속 구간으로 기록한다. 의미별 결과를 계산하지 않는다.
std::string Table(std::span<const std::uint32_t> values) {
    std::string result;
    // 모든 비활성 칸까지 포함하여 불필요한 표 초기화/콜백 쓰기를 검출한다.
    for (std::size_t i=0;i<values.size();) {
        const auto value=values[i];const auto begin=i;
        // 같은 값의 연속 구간만 압축한다.
        while (i<values.size() && values[i]==value) ++i;
        if (!result.empty()) result+=';';result+=std::to_string(i-begin)+':'+std::to_string(value);
    }
    return result;
}
// raw 표 입력의 구간을 정확한 크기의 배열에 펼친다. 잘못된 폭은 입력 오류로 거부한다.
void ReadTable(const std::string& text,std::span<std::uint32_t> output) {
    std::size_t cursor=0;
    // 모든 입력 구간을 원본 순서로 복사한다. 예상 호출 대상은 만들지 않는다.
    for (const auto& item:Split(text,';')) {
        const auto parts=Split(item,':');const auto size=Number(parts.at(0)),value=Number(parts.at(1));
        if (size>output.size()-cursor) throw std::runtime_error("Gump 프레임 fixture 표 범위 오류");
        std::fill_n(output.begin()+cursor,size,value);cursor+=size;
    }
    if (cursor!=output.size()) throw std::runtime_error("Gump 프레임 fixture 표 폭 오류");
}
// 한 callback의 명시 raw 쓰기 입력이다. 대상 0=개수, 1=등록표, 2=idle 표, 나머지는 노드 토큰이다.
using Patch=std::array<std::uint32_t,3>;
// 안정된 노드·공유 생성/수명 상태와 실제 콜백 당시 상태 기록을 보관한다.
struct FrameScene {
    GumpCreateState create;
    GumpLifetimeState lifetime;
    std::map<std::uint32_t,GumpCreateNode> nodes;
    std::map<std::string,std::vector<Patch>> responses;
    std::string calls;
    GumpTreeState treeState;
    std::unique_ptr<GumpTree> tree;
    // 현재 저장소에서 토큰을 조회한다. 없는 토큰은 실제 도달 시 처리기가 거부한다.
    GumpCreateNode* Find(std::uint32_t token) {
        const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second;
    }
    // CD는 idle 표의 활성 80칸만 직렬화한다. 뒤 420칸은 별도 경계 검사에서 보호한다.
    std::size_t IdleSize() const { return create.collection.edition==netstorm::o::OriginalEdition::Patch1078 ? 500U : 80U; }
    // 실제 개수와 표 전체를 Python 관찰과 같은 형식으로 기록한다.
    std::string State() const {
        return std::to_string(create.count)+'#'+Table(create.entries)+'#'+Table(std::span(lifetime.idle).first(IdleSize()));
    }
    // 노드의 호스트 메모리 배치 대신 원본의 전체 의미별 DWORD를 기록한다.
    std::string Nodes() const {
        std::string result;
        // 토큰 주소순은 원본 입력의 여섯 노드 순서와 같다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+'='+Encode(node); }
        return result;
    }
    // 콜백 직전 전체 상태/인자를 남기고 입력이 지정한 쓰기만 실행한다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+','+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 반환값 없는 원본 가상 함수의 실제 인자 비트를 그대로 기록한다.
        for (auto value:args) calls+=','+std::to_string(value);
        calls+='@'+State()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return;
        // 기대 필터 결과를 만들지 않고 콜백의 명시 raw 변경만 적용한다.
        for (const auto& patch:found->second) {
            const auto target=patch[0],offset=patch[1],value=patch[2];
            if (!target) { if (offset) throw std::runtime_error("Gump 프레임 응답 개수 offset 오류");create.count=value; }
            else if (target==1) { create.entries.at(offset)=value;create.collection.root=create.entries[0]; }
            else if (target==2) lifetime.idle.at(offset)=value;
            else { auto raw=Bytes(Encode(nodes.at(target)));Put(raw,offset,value);nodes.at(target)=Decode(raw); }
        }
    }
    // 세 가상 함수는 같은 상태 저장소에 대한 기록/입력 응답 콜백을 사용한다.
    GumpFrameHooks Hooks() {
        return {[this](auto token) { return Find(token); },[this](auto token) { Respond("Idle",token); },
            [this](auto token,bool value) { Respond("Redraw",token,{value ? 1U : 0U}); },
            [this](auto token,auto event,auto first,auto second) { Respond("Broadcast",token,{event,first,second}); }};
    }
    // 실제 기본 생성기로 루트와 세 자식의 등록/serial/개수/플래그를 만든다.
    void Create() {
        create.screenWidth=64;create.screenHeight=48;
        // 생성 대상의 저장소는 모든 콜백과 수명 처리 동안 안정된 주소를 유지한다.
        for (std::uint32_t token=1;token<=4;++token) nodes.emplace(token,GumpCreateNode{});
        GumpCreate factory(create,{[this](auto token) { return Find(token); },[](auto) { return 1U; }});
        factory.InitializeRoot();factory.Construct(2,11,20,16,0x101);factory.Construct(3,12,20,16,0x101);factory.Construct(4,13,8,6,0x101);
        // 실제 가상 Push/Attach는 같은 트리 처리기로 연결하고 생성 중의 재그리기만 준비된 경계로 둔다.
        tree=std::make_unique<GumpTree>(create,treeState,GumpTreeHooks{[this](auto token) { return Find(token); },[](auto,bool) {},
            [this](auto token) { tree->Push(token); },[this](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); },
            [](auto) { throw std::runtime_error("프레임 검사 생성 중 예상 밖 교체 삭제"); },{}});
        tree->Attach(3,0,0,1,1);tree->Attach(2,0,0,1,1);tree->Attach(4,0,0,2,1);
        nodes[2].flags|=0x20;
    }
};
// 실제 PE의 정상 반환 뒤 전체 노드/표와 각 가상 호출 직전 상태를 C++와 비교한다.
void Replay(std::string_view edition) {
    std::ifstream input(NETSTORM_GUMPFRAME_FIXTURE);if (!input) throw std::runtime_error("Gump 프레임 fixture 없음");
    std::string line;std::size_t count=0;
    // 행마다 새 저장소를 만들고 연속 action 사이에만 현재 상태를 유지한다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==11);if (row.size()!=11) return;
        if (row[0]!=edition) continue;FrameScene scene;CHECK(Number(row[1])==count++);
        scene.create.collection.edition=edition=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072;
        // 완성 노드 입력을 실제 의미별 필드로 읽고 예상 결과는 fixture 출력 열에서만 취한다.
        for (const auto& item:Split(row[3],';')) { const auto parts=Split(item,'=');scene.nodes.emplace(Number(parts[0]),Decode(Bytes(parts[1]))); }
        const auto parts=Split(row[4],'#');scene.create.count=Number(parts.at(0));ReadTable(parts.at(1),scene.create.entries);
        ReadTable(parts.at(2),std::span(scene.lifetime.idle).first(scene.IdleSize()));scene.create.collection.root=scene.create.entries[0];
        CHECK(scene.Nodes()==row[3] && scene.State()==row[4]);
        if (row[5]!="-") {
            // 중복 응답 키는 마지막 명시 입력을 사용한다. 예상 호출 대상은 계산하지 않는다.
            for (const auto& item:Split(row[5],';')) {
                const auto response=Split(item,':');std::vector<Patch> patches;
                if (!response[1].empty()) {
                    // raw 응답의 대상/offset/값만 그대로 읽는다.
                    for (const auto& patch:Split(response[1],'/')) { const auto values=Inputs(patch);patches.push_back({values.at(0),values.at(1),values.at(2)}); }
                }
                scene.responses[response[0]]=std::move(patches);
            }
        }
        GumpFrame frame(scene.create,scene.lifetime,scene.Hooks());
        // 실제 최상위 함수와 같은 인자를 처리기에 전달한다. void 반환의 EAX는 비교하지 않는다.
        for (const auto& item:Split(row[2],';')) {
            const auto action=Split(item,',');const auto token=Number(action[1]);
            if (action[0]=="Idle") frame.Idle();else if (action[0]=="RedrawNode") frame.RedrawNode(token);
            else if (action[0]=="RedrawRoots") frame.RedrawRoots();
            else if (action[0]=="Broadcast") frame.Broadcast(Number(action[2]),Number(action[3]),Number(action[4]));
            else throw std::runtime_error("Gump 프레임 action 오류");
        }
        CHECK(scene.Nodes()==row[6]);CHECK(scene.State()==row[7]);CHECK((scene.calls.empty() ? "-" : scene.calls)==row[8]);
    }
    CHECK(count==kFixtureCases);
}
}

// 10.78 전체 몸체의 독립 관찰과 정상 콜백 순서를 대조한다.
TEST_CASE(GumpFrame_OriginalPatch) { Replay("originals"); }
// CD의 80칸 idle 한도와 실제 재귀 함수를 대조한다.
TEST_CASE(GumpFrame_OriginalCd) { Replay("originalCD"); }
// 추가 10.37 PE도 별도로 실행해 저장한 관찰을 대조한다.
TEST_CASE(GumpFrame_Original1037) { Replay("original1037"); }

// 실제 생성/idle 등록/삭제 요청과 같은 표를 사용하여 지연 삭제 전후 프레임 호출 대상을 확인한다.
TEST_CASE(GumpFrame_CreateLifetimeIntegration) {
    FrameScene scene;scene.Create();std::vector<std::uint32_t> visited;GumpLifetime* current=nullptr;
    GumpLifetimeHooks lifetimeHooks;lifetimeHooks.node=[&](auto token) { return scene.Find(token); };
    lifetimeHooks.push=[&](auto token) { scene.tree->Push(token); };
    // 실제 scalar 기본 소멸자/표 정리 뒤에만 안정된 객체 저장소를 해제한다.
    lifetimeHooks.deletingDestructor=[&](auto token,auto flags) { current->DeleteScalar(token,flags); };
    lifetimeHooks.release=[&](auto token) { scene.nodes.erase(token); };
    GumpLifetime lifetime(scene.create,scene.lifetime,lifetimeHooks);current=&lifetime;
    lifetime.SetIdle(2,true);lifetime.SetIdle(3,true);
    auto hooks=scene.Hooks();hooks.idle=[&](auto token) { visited.push_back(token);if (token==2 && scene.Find(3)) lifetime.Kill(3); };
    GumpFrame frame(scene.create,scene.lifetime,hooks);frame.Idle();
    CHECK(visited==std::vector<std::uint32_t>{2});CHECK(scene.lifetime.killCount==1 && (scene.nodes[3].flags&0x10000));
    visited.clear();hooks.broadcast=[&](auto token,auto,auto,auto) { visited.push_back(token); };
    GumpFrame notify(scene.create,scene.lifetime,hooks);notify.Broadcast(7,11,13);
    CHECK((visited==std::vector<std::uint32_t>{2,4}));
    lifetime.Cleanup();CHECK(!scene.Find(3) && scene.create.count==3 && !scene.lifetime.killCount && !scene.nodes[2].next);
    visited.clear();frame.Idle();CHECK(visited==std::vector<std::uint32_t>{2});
}

// CD의 비활성 420칸은 실행/변경하지 않으며 패치판은 마지막 500번째 칸까지 호출한다.
TEST_CASE(GumpFrame_IdleEditionLimitAndUnfilteredNodes) {
    FrameScene scene;scene.Create();scene.nodes[2].flags=0x10008;scene.lifetime.idle[79]=2;scene.lifetime.idle[80]=3;scene.lifetime.idle[499]=4;
    std::vector<std::uint32_t> visited;auto hooks=scene.Hooks();hooks.idle=[&](auto token) { visited.push_back(token); };
    GumpFrame frame(scene.create,scene.lifetime,hooks);scene.create.collection.edition=netstorm::o::OriginalEdition::Cd1072;frame.Idle();
    CHECK(visited==std::vector<std::uint32_t>{2});CHECK(scene.lifetime.idle[80]==3 && scene.lifetime.idle[499]==4);
    visited.clear();scene.create.collection.edition=netstorm::o::OriginalEdition::Patch1078;frame.Idle();
    CHECK((visited==std::vector<std::uint32_t>{2,3,4}));
}

// 통지 성공 수가 아닌 비영 칸 수를 세며 콜백 이후 현재 signed count로 종료한다.
TEST_CASE(GumpFrame_BroadcastCurrentCountAndRootExclusion) {
    FrameScene scene;scene.Create();scene.nodes[1].flags=0x101;scene.nodes[2].flags=1;scene.create.count=1;
    std::vector<std::uint32_t> visited;auto hooks=scene.Hooks();hooks.broadcast=[&](auto token,auto e,auto x,auto y) {
        CHECK(e==0x80000001 && x==11 && y==13);visited.push_back(token);scene.create.count=1;
    };
    GumpFrame frame(scene.create,scene.lifetime,hooks);frame.Broadcast(0x80000001,11,13);CHECK(visited.empty());
    scene.create.count=4;frame.Broadcast(0x80000001,11,13);CHECK(visited==std::vector<std::uint32_t>{3});
    scene.create.count=0xffffffff;visited.clear();frame.Broadcast(0x80000001,11,13);CHECK(visited.empty());
}

// 최상위 next 저장과 재귀 뒤 하위 next 재조회의 차이 및 하위 dead/void 통과를 확인한다.
TEST_CASE(GumpFrame_RedrawCurrentChildAndSavedRootLinks) {
    FrameScene scene;scene.Create();scene.nodes[4].flags=0x10009;std::vector<std::uint32_t> visited;auto hooks=scene.Hooks();
    hooks.redraw=[&](auto token,bool value) { CHECK(!value);visited.push_back(token);if (token==2) scene.nodes[2].next=0; };
    GumpFrame frame(scene.create,scene.lifetime,hooks);scene.create.collection.overrideHead=4;scene.nodes[1].flags=0;frame.RedrawRoots();
    CHECK((visited==std::vector<std::uint32_t>{2,4,3}));visited.clear();scene.nodes[2].child=3;scene.nodes[3].next=4;
    hooks.redraw=[&](auto token,bool) { visited.push_back(token);if (token==3) scene.nodes[3].next=0; };
    GumpFrame current(scene.create,scene.lifetime,hooks);current.RedrawNode(2);CHECK((visited==std::vector<std::uint32_t>{2,3}));
}

// 없는 토큰/콜백과 잘못된 재귀 링크는 호스트 오류다. 조기 종료한 경계에는 조회가 필요 없다.
TEST_CASE(GumpFrame_ContractsAndCycles) {
    FrameScene scene;scene.Create();CHECK(Throws([&] { GumpFrame invalid(scene.create,scene.lifetime,{}); }));
    auto hooks=scene.Hooks();hooks.idle={};hooks.redraw={};hooks.broadcast={};GumpFrame missing(scene.create,scene.lifetime,hooks);
    missing.Idle();scene.create.count=0;missing.Broadcast(1,2,3);scene.nodes[2].flags=0x42001;scene.nodes[3].flags=0x42001;missing.RedrawRoots();
    scene.lifetime.idle[0]=999;CHECK(Throws([&] { missing.Idle(); }));scene.lifetime.idle[0]=2;CHECK(Throws([&] { missing.Idle(); }));
    scene.nodes[2].flags=1;CHECK(Throws([&] { missing.RedrawNode(2); }));scene.nodes[2].flags=0x101;scene.create.count=4;
    CHECK(Throws([&] { missing.Broadcast(1,2,3); }));GumpFrame frame(scene.create,scene.lifetime,scene.Hooks());
    scene.nodes[2].flags=0x21;scene.nodes[2].child=2;CHECK(Throws([&] { frame.RedrawNode(2); }));
    scene.nodes[2].child=0;scene.nodes[2].next=2;CHECK(Throws([&] { frame.RedrawRoots(); }));
}

// 콜백이 쓰고 예외를 던진 경우 쓰기는 보존한다. 실제 원본 PE의 C++ 예외 전파는 검사 범위 밖이다.
TEST_CASE(GumpFrame_CallbackExceptionKeepsState) {
    FrameScene scene;scene.Create();scene.lifetime.idle[0]=2;auto hooks=scene.Hooks();
    hooks.idle=[&](auto token) { scene.nodes[token].command=999;scene.lifetime.idle[0]=3;throw std::runtime_error("idle 경계 실패"); };
    GumpFrame frame(scene.create,scene.lifetime,hooks);CHECK(Throws([&] { frame.Idle(); }));
    CHECK(scene.nodes[2].command==999 && scene.lifetime.idle[0]==3);
}
