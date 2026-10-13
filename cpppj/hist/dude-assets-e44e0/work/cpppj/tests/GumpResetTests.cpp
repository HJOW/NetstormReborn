// 두 실제 PE의 전체 Kill 요청/루트 교체와 실제 트리·기본 소멸자 수명을 같은 저장소에서 대조한다.
#include "GumpRawSupport.h"
#include "client/GumpLifetime.h"
#include "client/GumpTree.h"

namespace {
using namespace netstorm::test::gumpraw;
// 원본 관찰만 기대 결과로 읽는다. 기본 CTest는 PE/Python/게임/창을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_UIRESET_FIXTURE).rows;return rows; }
// 전체 표를 현재 값의 연속 구간으로만 압축한다. 기대 삭제 필터/결과를 계산하지 않는다.
std::string Rle(std::span<const std::uint32_t> values) {
    std::string result;
    // 비활성 칸까지 포함해 현재 표의 모든 DWORD를 직렬화한다.
    for (std::size_t i=0;i<values.size();) {
        const auto first=i;const auto value=values[i];
        // 연속해서 같은 현재 값인 구간의 끝만 찾는다.
        while (i<values.size() && values[i]==value) ++i;
        if (!result.empty()) result+=';';result+=std::to_string(i-first)+':'+std::to_string(value);
    }
    return result;
}
// 완성 입력 RLE를 정확한 길이의 실제 표에 푼다.
void ReadTable(std::string_view input,std::span<std::uint32_t> values) {
    std::size_t offset=0;
    // 각 구간의 길이/값은 실제 입력이며 기대 결과는 만들지 않는다.
    for (const auto& item:Split(std::string(input),';')) {
        const auto parts=Split(item,':');const auto length=Number(parts[0]);
        if (length>values.size()-offset) throw std::runtime_error("Gump reset 표 입력 범위 오류");
        std::fill_n(values.begin()+offset,length,Number(parts[1]));offset+=length;
    }
    if (offset!=values.size()) throw std::runtime_error("Gump reset 표 입력 길이 오류");
}
// 안정된 노드와 실제 공유 상태·signed 제외 BYTE를 현재 콜백 당시 기록과 연결한다.
struct ResetScene {
    // nodes: 노드. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,GumpCreateNode> nodes;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    GumpCreateState state;
    // life: 원본의 고정 크기 idle/overlay/삭제/dialog 표와 현재 삭제 전역이다 (GumpLifetimeState).
    GumpLifetimeState life;
    // excluded: 제외된. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::int8_t excluded{11};
    // responses: 응답. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::string,std::vector<std::array<std::uint32_t,3>>> responses;
    // calls: 호출 횟수 또는 호출 인자를 대조하는 기록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string calls;
    // 토큰에 대응하는 안정된 기본/파생 저장소를 현재 시점에 조회한다.
    GumpCreateNode* Find(std::uint32_t token) { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; }
    // CD의 실제 80칸 idle과 패치판의 500칸을 선택한다.
    std::span<std::uint32_t> Table(std::uint32_t index) {
        switch (index) { case 1:return state.entries;case 2:return std::span(life.idle).first(state.collection.edition==OriginalEdition::Patch1078 ? 500U : 80U);
            case 3:return life.overlay;case 4:return life.kill;case 5:return life.dialogs;default:throw std::runtime_error("Gump reset 표 index 오류"); }
    }
    // fixture의 DWORD 스칼라를 실제 공유 필드에 대응시킨다. 제외 BYTE는 별도 메서드로 읽는다.
    std::uint32_t& Global(std::uint32_t index) {
        switch (index) { case 0:return state.count;case 1:return life.killCount;case 2:return life.dialogCount;case 3:return life.focus;
            case 4:return state.collection.overrideHead;case 5:return life.currentDialog;case 6:return life.diagnostics;case 7:return state.serial;
            default:throw std::runtime_error("Gump reset 전역 index 오류"); }
    }
    // 현재 제외 BYTE의 비트는 직렬화에서 unsigned이며 실제 API에서는 signed로 읽는다.
    void SetGlobal(std::uint32_t index,std::uint32_t value) {
        if (index==8) excluded=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(value));else Global(index)=value;
    }
    // 전체 스칼라/다섯 표를 원본 관찰과 같은 형식으로 기록한다.
    std::string State() {
        std::string result;
        // serial도 실제 현재값을 기록해 불필요한 reset 쓰기를 검출한다.
        for (std::uint32_t i=0;i<8;++i) { if (i) result+=',';result+=std::to_string(Global(i)); }
        result+=','+std::to_string(std::bit_cast<std::uint8_t>(excluded));
        // 표의 전체 활성 길이를 보존한다.
        for (std::uint32_t i=1;i<=5;++i) result+='#'+Rle(Table(i));return result;
    }
    // 전체 raw 0xa0바이트의 미작성 필드까지 원본 순서로 기록한다.
    std::string Nodes() const {
        std::string result;
        // 입력 토큰 순서는 실제 PE의 여섯 격리 저장소 주소 순서와 같다.
        for (const auto& [token,node]:nodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+'='+Encode(node); }return result;
    }
    // 실제 callback 당시 상태/인자를 기록하고 입력에 있는 raw 쓰기만 수행한다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        const auto key=kind+','+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 가상 호출의 원본 인자 비트를 순서대로 남긴다.
        for (auto value:args) calls+=','+std::to_string(value);
        calls+='@'+State()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return;
        // 기대 필터/호출을 유추하지 않고 완성 입력의 대상/offset/값만 적용한다.
        for (const auto& patch:found->second) {
            if (!patch[0]) SetGlobal(patch[1],patch[2]);
            else if (patch[0]<=5) { Table(patch[0])[patch[1]]=patch[2];state.collection.root=state.entries[0]; }
            else { auto raw=Bytes(Encode(nodes.at(patch[0])));Put(raw,patch[1],patch[2]);nodes.at(patch[0])=Decode(raw); }
        }
    }
    // 기존 수명 처리기의 실제 외부 경계를 같은 저장소의 관찰기로 연결한다.
    GumpLifetimeHooks Hooks() {
        return {[this](auto token) { return Find(token); },[this](auto token) { Respond("Push",token); },
            [this](auto form,auto flags) { Respond("Form",form,{flags}); },[this](auto token,auto flags) { Respond("Destructor",token,{flags}); },
            [this](auto pointer) { Respond("Free",pointer); }};
    }
};
// 원본의 완성 입력을 재생하고 전체 현재 상태/콜백/정상 반환을 독립 관찰과 대조한다.
void Replay(std::string_view edition) {
    std::size_t count=0;
    // 행 사이에는 새 저장소를 사용하고 같은 행의 연속 연산만 현재 상태를 이어 쓴다.
    for (const auto& row:Fixture()) {
        CHECK(row.size()==13);if (row.size()!=13) return;if (row[0]!=edition) continue;ResetScene scene;
        scene.state.collection.edition=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const auto parts=Split(row[2],'#'),globals=Split(parts[0],',');CHECK(globals.size()==9);
        // 모든 scalar는 fixture 입력이며 제외 BYTE만 해당 폭으로 공급한다.
        for (std::uint32_t i=0;i<globals.size();++i) scene.SetGlobal(i,Number(globals[i]));
        // 고정 크기 표는 입력의 모든 칸을 그대로 읽는다.
        for (std::uint32_t i=1;i<=5;++i) ReadTable(parts[i],scene.Table(i));scene.state.collection.root=scene.state.entries[0];
        // 전체 raw 노드를 호스트 메모리 배치와 독립적인 의미별 필드로 읽는다.
        for (const auto& item:Split(row[3],';')) { const auto value=Split(item,'=');scene.nodes.emplace(Number(value[0]),Decode(Bytes(value[1]))); }
        CHECK(scene.State()==row[2] && scene.Nodes()==row[3]);
        if (row[5]!="-") {
            // 경계 종류/this 별 입력 응답을 준비하며 예상 호출 목록을 계산하지 않는다.
            for (const auto& item:Split(row[5],';')) {
                const auto response=Split(item,':');
                // 응답의 정확한 위치/값만 저장한다.
                for (const auto& patch:Split(response[1],'/')) { const auto value=Inputs(patch);scene.responses[response[0]].push_back({value[0],value[1],value[2]}); }
            }
        }
        auto hooks=scene.Hooks();GumpLifetime* current=nullptr;
        if (row[6]=="1") hooks.deletingDestructor=[&](auto token,auto flags) { current->DeleteScalar(token,flags); };
        GumpLifetime lifetime(scene.state,scene.life,hooks);current=&lifetime;std::string results;
        // 실제 C++ API를 호출하고 void 반환을 원본과 같은 '-'로 기록한다.
        for (const auto& item:Split(row[4],';')) {
            const auto action=Split(item,',');const auto token=Number(action[1]);
            if (action[0]=="KillExceptType") lifetime.KillExceptType(scene.excluded);
            else if (action[0]=="ReplaceRoot") lifetime.ReplaceRoot(token);
            else if (action[0]=="Cleanup") lifetime.Cleanup();else throw std::runtime_error("Gump reset 연산 오류");
            if (!results.empty()) results+='/';results+='-';
        }
        const bool same=scene.Nodes()==row[7] && scene.State()==row[8] && results==row[9] && (scene.calls.empty() ? "-" : scene.calls)==row[10];CHECK(same);
        if (!same) { std::printf("GumpReset %s %s: nodes=%d state=%d results=%d calls=%d\n",row[0].c_str(),row[1].c_str(),scene.Nodes()==row[7],scene.State()==row[8],results==row[9],(scene.calls.empty() ? "-" : scene.calls)==row[10]);break; }++count;
    }
    CHECK(count==Fixture().size()/2);
}
// 실제 생성·트리·기본 소멸자 수명으로 상위 Uber의 자식 이동/등록 칸 제거 순서를 검사한다.
void TransferAndReplace(OriginalEdition edition) {
    ResetScene scene;scene.state.collection.edition=edition;scene.state.screenWidth=64;scene.state.screenHeight=48;
    // 토큰 주소는 표면에 그리지 않는 검사 저장소의 안정된 식별자다.
    for (std::uint32_t token=1;token<=6;++token) scene.nodes.emplace(token,GumpCreateNode{});
    GumpCreate factory(scene.state,{[&](auto token) { return scene.Find(token); },[](auto) { return 1U; }});
    factory.InitializeRoot();factory.Construct(2,21,16,12,1);factory.Construct(3,11,8,6,1);factory.Construct(4,11,4,3,1);
    GumpTreeState treeState;GumpTree* treePointer=nullptr;GumpLifetime* lifetimePointer=nullptr;std::vector<std::uint32_t> freed;
    auto hooks=scene.Hooks();hooks.push=[&](auto token) { treePointer->Push(token); };
    hooks.deletingDestructor=[&](auto token,auto flags) { lifetimePointer->DeleteScalar(token,flags); };
    hooks.release=[&](auto token) { freed.push_back(token);scene.nodes.erase(token); };
    GumpLifetime lifetime(scene.state,scene.life,hooks);lifetimePointer=&lifetime;
    GumpTree tree(scene.state,treeState,{[&](auto token) { return scene.Find(token); },[](auto,bool) {},[&](auto token) { treePointer->Push(token); },
        [&](auto token,float x,float y,auto parent,auto mode) { treePointer->Attach(token,x,y,parent,mode); },[&](auto token) { lifetime.Kill(token); },{}});treePointer=&tree;
    tree.Attach(3,2,3,1,0);tree.Attach(2,4,5,1,0);tree.Attach(4,1,1,2,0);
    lifetime.KillExceptType(scene.excluded);lifetime.Cleanup();
    CHECK(freed==std::vector<std::uint32_t>({4,2}) && scene.state.count==2 && scene.state.serial==4 && scene.nodes.at(1).child==3);
    factory.Construct(5,54,64,48,1);const auto temporaryId=scene.nodes.at(5).id;
    scene.state.entries[temporaryId]=0;scene.nodes.at(5).id=0;
    const auto x=scene.nodes.at(3).x,y=scene.nodes.at(3).y;tree.Push(3);tree.Attach(3,x,y,5,0);
    lifetime.ReplaceRoot(5);
    CHECK(freed==std::vector<std::uint32_t>({4,2,1}) && scene.state.entries[0]==5 && scene.state.collection.root==5);
    CHECK(scene.state.count==2 && scene.state.serial==5 && scene.nodes.at(5).id==0 && scene.nodes.at(5).child==3 && scene.nodes.at(3).parent==5);
    CHECK(!scene.life.killCount && !scene.state.entries[temporaryId]);
    factory.Construct(6,21,4,3,1);CHECK(scene.nodes.at(6).id==temporaryId && scene.nodes.at(6).serial==6 && scene.state.count==3);
}
}
// 10.78의 실제 전체 reset/하위 수명/콜백 상태를 원본 PE 관찰과 대조한다.
TEST_CASE(GumpReset_PatchNative) { Replay("originals"); }
// CD의 별도 몸체/80칸 idle/쓰기 순서를 원본 CD PE 관찰과 대조한다.
TEST_CASE(GumpReset_CdNative) { Replay("originalCD"); }
// 10.78에서 제외 타입 자식의 부모 Kill/생존 직접 자식 이동/루트 교체/등록 칸 재사용을 연결한다.
TEST_CASE(GumpReset_PatchTransferAndReuse) { TransferAndReplace(OriginalEdition::Patch1078); }
// CD에서도 같은 실제 생성/트리·기본 소멸자 수명을 사용한다.
TEST_CASE(GumpReset_CdTransferAndReuse) { TransferAndReplace(OriginalEdition::Cd1072); }
