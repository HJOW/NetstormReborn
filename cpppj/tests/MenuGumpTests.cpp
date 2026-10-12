// 실제 두 PE의 독립 관찰과 같은 생성/트리/수명/프레임 저장소의 메뉴 처리를 비교한다.
#include "GumpRawSupport.h"
#include "client/MenuGump.h"
#include "client/GumpTree.h"
#include <fstream>
#include <map>
#include <memory>

namespace {
using namespace netstorm::test::gumpraw;
// 격리 x86의 세 안정된 토큰과 snapshot 필드 목록이다. 호스트 객체 배치는 이 주소와 무관하다.
constexpr std::array<std::uint32_t,3> kTokens{0x18000010,0x18001010,0x18002010};
constexpr std::size_t kRawSize=0xb90,kRowsPerEdition=154;
// 원본 입력/출력에 기록하는 55 DWORD의 원본 offset이다.
std::vector<std::size_t> Fields() {
    std::vector<std::size_t> fields{0,4,56,60,64,68,72,184,368,372,376};
    // 항목 offset 배열의 모든 칸을 순서대로 포함한다.
    for (std::size_t offset=380;offset<540;offset+=4) fields.push_back(offset);
    fields.insert(fields.end(),{544,548,2952,2956});return fields;
}
// 의미별 현재 필드만 원본 주소 배치에 쓴다. 기대 연산/상태 전이를 계산하지 않는다.
std::array<std::uint8_t,kRawSize> Raw(const MenuGumpNode& node) {
    std::array<std::uint8_t,kRawSize> raw{};const auto base=Bytes(Encode(static_cast<const GumpCreateNode&>(node)));
    std::copy(base.begin(),base.end(),raw.begin());Put(raw,184,node.expanding);Put(raw,368,Bits(node.itemWidth));
    Put(raw,372,Bits(node.targetWidth));Put(raw,376,Bits(node.targetHeight));
    // 현재 세로 offset도 원본 40개 DWORD 자리에 기록한다.
    for (std::size_t i=0;i<node.offsets.size();++i) Put(raw,380+i*4,Bits(node.offsets[i]));
    Put(raw,544,node.menuFlags);Put(raw,548,node.itemCount);Put(raw,2952,node.fancy);Put(raw,2956,node.title);return raw;
}
// 원본 바이트를 동일한 기본 저장소와 메뉴 파생 필드로 읽는다.
MenuGumpNode ReadRaw(std::span<const std::uint8_t> raw) {
    MenuGumpNode node;static_cast<GumpCreateNode&>(node)=Decode(raw);node.expanding=Get(raw,184);
    node.itemWidth=Float(Get(raw,368));node.targetWidth=Float(Get(raw,372));node.targetHeight=Float(Get(raw,376));
    // 입력 offset의 부호/NaN 비트도 정규화하지 않는다.
    for (std::size_t i=0;i<node.offsets.size();++i) node.offsets[i]=Float(Get(raw,380+i*4));
    node.menuFlags=Get(raw,544);node.itemCount=Get(raw,548);node.fancy=Get(raw,2952);node.title=Get(raw,2956);return node;
}
// 콜백 응답의 종류/토큰/목표/offset/DWORD를 그대로 보관한다.
struct Response { std::string kind;std::uint32_t token{},target{},offset{},value{}; };
// 공유 노드/메뉴 표/idle 표를 가지고 모든 명시 가상 콜백 당시 상태를 기록한다.
struct MenuScene {
    GumpCreateState create;
    MenuGumpState state;
    GumpLifetimeState lifetimeState;
    std::map<std::uint32_t,MenuGumpNode> nodes;
    std::vector<Response> responses;
    std::string calls;
    std::unique_ptr<GumpLifetime> lifetime;
    std::unique_ptr<MenuGump> menu;
    // 같은 파생 저장소의 안정된 주소를 기본 Gump 처리기에도 제공한다.
    MenuGumpNode* Find(std::uint32_t token) { const auto it=nodes.find(token);return it==nodes.end() ? nullptr : &it->second; }
    // 입력의 압축 DWORD 바이트를 전체 원본 offset으로 펼친다.
    void ReadNodes(const std::string& text) {
        const auto input=Split(text,';');const auto fields=Fields();
        // 모든 원본 토큰은 같은 map에서 생성되어 이후 이동/복사하지 않는다.
        for (std::size_t i=0;i<kTokens.size();++i) {
            const auto packed=Bytes(input.at(i));std::array<std::uint8_t,kRawSize> raw{};
            if (packed.size()!=fields.size()*4) throw std::runtime_error("메뉴 fixture 필드 수 오류");
            // 각 선택 필드만 원래 offset에 복사한다.
            for (std::size_t j=0;j<fields.size();++j) Put(raw,fields[j],Get(packed,j*4));
            nodes[kTokens[i]]=ReadRaw(raw);
        }
    }
    // 실제 현재 세 노드의 선택 필드를 원본 관찰과 같은 순서로 직렬화한다.
    std::string Nodes() const {
        std::string result;const auto fields=Fields();
        // 원본의 메뉴/장식/제목 토큰 순서다.
        for (auto token:kTokens) {
            const auto raw=Raw(nodes.at(token));std::vector<std::uint8_t> packed(fields.size()*4);
            // 현재 필드 값만 복사하고 기대 동작은 만들지 않는다.
            for (std::size_t i=0;i<fields.size();++i) Put(packed,i*4,Get(raw,fields[i]));
            if (!result.empty()) result+=';';result+=Hex(packed);
        }
        return result;
    }
    // 전체 메뉴 표/앞 네 idle 칸/루트를 입력에서 읽는다.
    void ReadState(const std::string& text) {
        const auto parts=Split(text,'#');state.index=Number(parts.at(0));
        const auto menus=Inputs(parts.at(1)),idle=Inputs(parts.at(2));
        if (menus.size()!=state.entries.size() || idle.size()!=4) throw std::runtime_error("메뉴 fixture 표 크기 오류");
        std::copy(menus.begin(),menus.end(),state.entries.begin());std::copy(idle.begin(),idle.end(),lifetimeState.idle.begin());
        create.entries[0]=Number(parts.at(3));create.collection.root=create.entries[0];
    }
    // 입력과 출력에 같은 현재 상태 표현을 사용한다.
    std::string State() const {
        std::string result=std::to_string(state.index)+'#';
        // 전체 controlled 메뉴 칸을 보존한다.
        for (auto value:state.entries) { if (result.back()!='#') result+=',';result+=std::to_string(value); }
        result+='#';
        // 관찰에 쓰는 앞 네 idle 칸이다. 뒷칸은 검사에서 별도로 보호한다.
        for (std::size_t i=0;i<4;++i) { if (i) result+=',';result+=std::to_string(lifetimeState.idle[i]); }
        return result+'#'+std::to_string(create.entries[0]);
    }
    // 원본 관찰처럼 콜백 직전 상태를 남긴 뒤 명시 응답 DWORD만 적용한다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint32_t> args={}) {
        if (!calls.empty()) calls+='|';calls+=kind+','+std::to_string(token);
        // 가상 호출 인자 비트는 bool/float 호스트 값과 별도로 원래 DWORD 형식으로 기록한다.
        for (auto value:args) calls+=','+std::to_string(value);
        calls+='@'+Nodes()+'@'+State();
        // 각 콜백이 지정한 입력 쓰기만 수행하여 재조회 순서를 검사한다.
        for (const auto& response:responses) if (response.kind==kind && response.token==token) {
            if (!response.target) state.index=response.value;
            else { auto raw=Raw(nodes.at(response.target));Put(raw,response.offset,response.value);nodes.at(response.target)=ReadRaw(raw); }
        }
    }
    // 실제 lifetime SetIdle와 메뉴 처리기가 같은 기본 노드를 읽도록 연결한다.
    void Build() {
        // 기록 콜백이 참조하는 장식/제목 저장소도 안정된 map 노드로 준비한다.
        for (auto token:kTokens) nodes.try_emplace(token);
        GumpLifetimeHooks life;life.node=[this](auto token) { return Find(token); };
        lifetime=std::make_unique<GumpLifetime>(create,lifetimeState,life);
        MenuGumpHooks hooks;hooks.node=[this](auto token) { return Find(token); };hooks.baseNode=hooks.node;
        hooks.push=[this](auto token) { Respond("push",token); };hooks.pop=[this](auto token) { Respond("pop",token); };
        hooks.redraw=[this](auto token,bool erase) { Respond("redraw",token,{erase ? 1U : 0U}); };
        hooks.baseRedraw=[this](auto token,bool erase) { Respond("base_redraw",token,{erase ? 1U : 0U}); };
        hooks.attach=[this](auto token,float x,float y,auto parent,auto mode) { Respond("attach",token,{Bits(x),Bits(y),parent,mode}); };
        hooks.attachObject=[this](auto token,auto object,float x,float y,auto parent,auto mode) { Respond("object",token,{object,Bits(x),Bits(y),parent,mode}); };
        menu=std::make_unique<MenuGump>(create,state,*lifetime,hooks);
    }
    // 저장한 action을 구현에 전달하고 실제 반환값만 기록한다.
    std::string Execute(const std::string& text) {
        std::string result;
        // 각 action은 앞 action의 현재 상태를 순서대로 이어받는다.
        for (const auto& action:Split(text,';')) {
            const auto parts=Split(action,',');const auto& op=parts.at(0);std::string value="-";
            // 선택 action의 DWORD 입력만 읽으며 부호 변환은 각 호출 경계에서 수행한다.
            const auto arg=[&parts](std::size_t i) { return Number(parts.at(i)); };
            if (op=="Width") menu->SetWidth(kTokens[0],Float(arg(1)));
            else if (op=="Height") menu->SetHeight(kTokens[0],Float(arg(1)));
            else if (op=="Redraw") menu->Redraw(kTokens[0],arg(1)!=0);
            else if (op=="Expand") menu->StartExpand(kTokens[0],arg(1));
            else if (op=="Idle") menu->Idle(kTokens[0]);
            else if (op=="Item") { const auto point=menu->ItemPosition(kTokens[0],std::bit_cast<std::int32_t>(arg(1)));value=std::to_string(Bits(point[0]))+','+std::to_string(Bits(point[1])); }
            else if (op=="Take") value=std::to_string(menu->TakeCurrent());
            else if (op=="Current") value=std::to_string(menu->Current());
            else if (op=="Count") value=std::to_string(menu->ItemCount());
            else if (op=="Attach") menu->AttachCurrent(std::bit_cast<std::int32_t>(arg(1)),std::bit_cast<std::int32_t>(arg(2)),arg(3));
            else if (op=="Object") menu->AttachObjectCurrent(arg(1),std::bit_cast<std::int32_t>(arg(2)),std::bit_cast<std::int32_t>(arg(3)));
            else throw std::runtime_error("메뉴 fixture action 오류");
            if (!result.empty()) result+=';';result+=value;
        }
        return result;
    }
};
}

// 기대 출력은 실제 PE의 관찰 파일에서만 읽는다. 게임/PE/에뮬레이터는 C++ 테스트에서 실행하지 않는다.
TEST_CASE(MenuGump_OriginalMachineObservations) {
    std::ifstream stream(NETSTORM_MENUGUMP_FIXTURE);if (!stream) throw std::runtime_error("메뉴 독립 fixture 없음");
    std::string line;std::getline(stream,line);std::map<std::string,std::size_t> counts;
    // 모든 독립 입력/콜백 당시 상태/최종 노드·표/반환값을 전수 비교한다.
    while (std::getline(stream,line)) {
        const auto row=Split(line,'\t');MenuScene scene;scene.create.collection.edition=row[0]=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072;
        scene.ReadNodes(row.at(3));scene.ReadState(row.at(4));
        if (row.at(5)!="-") {
            // fixture에는 예상 응답 순서가 아닌 콜백 종류별 원본 입력 쓰기만 있다.
            for (const auto& text:Split(row[5],';')) { const auto p=Split(text,',');scene.responses.push_back({p.at(0),Number(p.at(1)),Number(p.at(2)),Number(p.at(3)),Number(p.at(4))}); }
        }
        scene.Build();const auto result=scene.Execute(row.at(2));
        if (scene.Nodes()!=row.at(6) || scene.State()!=row.at(7) || (scene.calls.empty() ? "-" : scene.calls)!=row.at(8) || result!=row.at(9))
            std::fprintf(stderr,"MenuGump mismatch: %s case %s (%s)\n",row[0].c_str(),row[1].c_str(),row[2].c_str());
        CHECK(scene.Nodes()==row[6]);CHECK(scene.State()==row[7]);CHECK((scene.calls.empty() ? "-" : scene.calls)==row[8]);CHECK(result==row[9]);
        CHECK(std::all_of(scene.lifetimeState.idle.begin()+4,scene.lifetimeState.idle.end(),[](auto token) { return token==0; }));++counts[row[0]];
    }
    CHECK(counts.size()==2);CHECK(counts["originals"]==kRowsPerEdition);CHECK(counts["originalCD"]==kRowsPerEdition);
}

// 실제 idle 등록→GumpFrame 가상 분배→확장 완료를 같은 저장소에서 검사한다.
TEST_CASE(MenuGump_SharedLifetimeFrameAndFallback) {
    // 두 판본의 다른 idle 표 길이에서도 메뉴/기존 처리기의 분배를 보존한다.
    for (auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        MenuScene scene;scene.create.collection.edition=edition;
        MenuGumpNode node;node.flags=1;node.scale=1;node.baseWidth=node.width=100;node.baseHeight=node.height=80;node.targetWidth=100;node.targetHeight=80;
        scene.nodes.emplace(kTokens[0],node);scene.Build();scene.menu->StartExpand(kTokens[0],0x80000);
        CHECK(scene.lifetimeState.idle[0]==kTokens[0]);CHECK(scene.Find(kTokens[0])->flags&0x80);
        int fallbackIdle=0,fallbackRedraw=0;GumpFrameHooks hooks;hooks.node=[&scene](auto token) { return scene.Find(token); };
        hooks.idle=[&](auto) { ++fallbackIdle; };hooks.redraw=[&](auto,bool) { ++fallbackRedraw; };
        BindMenuGumpFrame(hooks,[&scene](auto token) { return token==kTokens[0] ? scene.menu.get() : nullptr; });
        GumpFrame frame(scene.create,scene.lifetimeState,hooks);frame.Idle();CHECK(scene.Find(kTokens[0])->height==80);CHECK(scene.Find(kTokens[0])->expanding==1);
        frame.Idle();CHECK(scene.Find(kTokens[0])->expanding==0);CHECK(scene.lifetimeState.idle[0]==kTokens[0]);
        frame.RedrawNode(kTokens[0]);CHECK(scene.calls.find("base_redraw")!=std::string::npos);
        hooks.idle(123);hooks.redraw(123,false);CHECK(fallbackIdle==1);CHECK(fallbackRedraw==1);
    }
}

// 부착 callback이 바꾼 현재 스택 칸을 지우며 새 루트를 실제 객체 Attach 인자로 사용한다.
TEST_CASE(MenuGump_CurrentStackCallbackAndObjectBoundary) {
    MenuScene scene;scene.nodes.emplace(kTokens[0],MenuGumpNode{});scene.Build();scene.state.index=2;
    scene.state.entries[1]=123;scene.state.entries[2]=kTokens[0];scene.state.entries[3]=456;scene.create.entries[0]=kTokens[2];
    scene.responses.push_back({"object",kTokens[0],0,0,3});scene.menu->AttachObjectCurrent(99,16777217,-21);
    CHECK(scene.state.index==2);CHECK(scene.state.entries[3]==0);CHECK(scene.state.entries[2]==kTokens[0]);CHECK(scene.state.entries[1]==123);
    CHECK(scene.calls.starts_with("object,"+std::to_string(kTokens[0])+",99,"+std::to_string(Bits(16777216.0F))+","+std::to_string(Bits(-21.0F))+","+std::to_string(kTokens[2])+",0@"));
}

// 무효 외부 token/index를 변경 전에 거부하고 도달하지 않은 장식 콜백은 요구하지 않는다.
TEST_CASE(MenuGump_InvalidHostContractsAndDeferredCallbacks) {
    MenuScene scene;scene.nodes.emplace(kTokens[0],MenuGumpNode{});scene.Find(kTokens[0])->scale=1;scene.Build();
    scene.menu->SetWidth(kTokens[0],17);CHECK(scene.Find(kTokens[0])->width==17);
    bool rejected=false;try { scene.menu->TakeCurrent(); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);CHECK(scene.state.index==0);
    rejected=false;try { scene.menu->ItemPosition(kTokens[0],40); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
    scene.state.index=20;rejected=false;try { scene.menu->Current(); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}

// 실제 GumpTree Attach→UI 수집으로 controlled 메뉴를 옮기고 관리 표만 제거한다.
TEST_CASE(MenuGump_CurrentAttachSharesTreeAndCollection) {
    // CD는 부착 전에 void가 필요하므로 두 판본 모두 생성 직후 상태를 사용한다.
    for (auto edition:{netstorm::o::OriginalEdition::Patch1078,netstorm::o::OriginalEdition::Cd1072}) {
        MenuScene scene;scene.create.collection.edition=edition;scene.Build();scene.create.screenWidth=640;scene.create.screenHeight=480;
        scene.create.entries[0]=scene.create.collection.root=kTokens[2];auto& root=*scene.Find(kTokens[2]);
        root.flags=1;root.scale=1;root.baseWidth=root.width=640;root.baseHeight=root.height=480;
        auto& node=*scene.Find(kTokens[0]);node.flags=9;node.scale=1;node.baseWidth=node.width=100;node.baseHeight=node.height=80;
        scene.state.index=1;scene.state.entries[1]=kTokens[0];int redraws=0;
        GumpTreeState state;GumpTreeHooks treeHooks;treeHooks.node=[&scene](auto token) { return scene.Find(token); };
        // 트리의 필수 가상 경계도 같은 실제 트리/수명 처리기로 연결한다.
        const GumpTree* boundTree=nullptr;
        treeHooks.push=[&boundTree](auto token) { boundTree->Push(token); };
        treeHooks.attach=[&boundTree](auto token,float x,float y,auto parent,auto mode) { boundTree->Attach(token,x,y,parent,mode); };
        treeHooks.kill=[&scene](auto token) { scene.lifetime->Kill(token); };
        treeHooks.redraw=[&redraws](auto,bool) { ++redraws; };GumpTree tree(scene.create,state,treeHooks);boundTree=&tree;
        MenuGumpHooks hooks;hooks.node=[&scene](auto token) { return scene.Find(token); };hooks.baseNode=hooks.node;
        hooks.attach=[&tree](auto token,float x,float y,auto parent,auto mode) { tree.Attach(token,x,y,parent,mode); };
        MenuGump menu(scene.create,scene.state,*scene.lifetime,hooks);menu.AttachCurrent(21,32,kTokens[2]);
        CHECK(menu.Current()==0);CHECK(scene.state.index==0);CHECK(node.parent==kTokens[2]);CHECK(root.child==kTokens[0]);CHECK(node.x==21);CHECK(node.y==32);CHECK(redraws==1);
        GumpCollectionHooks collectionHooks;collectionHooks.node=[&scene](auto token) { return scene.Find(token); };
        GumpCollection collection(scene.create.collection,collectionHooks);std::array<netstorm::o::RenderCollectionEntry,4> output{};
        CHECK(collection.Collect({0,0,640,480},output,4)==1);CHECK(collection.Bounds(kTokens[0]).left==21);
    }
}
