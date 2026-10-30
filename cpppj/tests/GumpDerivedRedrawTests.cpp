// 독립 PE의 장식/메뉴 행 요청·위치 캐시·투영 순서를 제품 본체와 비교한다.
#include "ConstructionSupport.h"
#include "client/GumpDecoration.h"
#include "client/MenuGumpInput.h"
#include <map>
#include <sstream>

namespace { // 완성 입력을 읽으며 원본의 기대 좌표를 계산하지 않는다.
using namespace netstorm::test::construction; // 기존 fixture 숫자/바이트 도구다.
using namespace netstorm::client; // 같은 제품의 위치/파생 UI 요청 본체다.
constexpr std::uint32_t kRoot=0x15000000,kTarget=kRoot+128; // 격리 PE가 사용한 두 토큰이며 호스트 주소가 아니다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // signed 원문 비트를 보존한다.
std::vector<std::uint8_t> Bytes(const std::string& text) { // 입력의 실제 바이트 순서를 읽는다.
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 두 글자가 한 BYTE다.
    return result;
}
template<class... Values> std::string Numbers(Values... values) { // 원문 관측의 DWORD 인수 표기다.
    std::ostringstream stream;bool first=true;const auto append=[&](auto value) { if (!first) stream<<',';first=false;stream<<static_cast<std::uint32_t>(value); };(append(values),...);return stream.str();
}
void Load(GumpCreateNode& n,const std::string& input) { // 실제 요청/Position이 읽는 원본 필드만 같은 offset에서 읽는다.
    const auto bytes=Bytes(input);n.flags=Get(bytes,4);n.next=Get(bytes,12);n.parent=Get(bytes,20);n.child=Get(bytes,24);n.object=Get(bytes,36);n.depth=Get(bytes,48);
    n.width=Float(Get(bytes,68));n.height=Float(Get(bytes,72));n.x=Float(Get(bytes,76));n.y=Float(Get(bytes,80));n.cached={Signed(Get(bytes,108)),Signed(Get(bytes,112))};
}
void Unexpected() { throw std::logic_error("파생 갱신 관측 범위 밖 호출"); } // 사용하지 않는 생성/트리 효과를 성공으로 대체하지 않는다.
}

// 원본의 1,152개 완성 입력과 모든 요청·캐시·투영을 C++로 재생한다.
TEST_CASE(GumpDerivedRedraw_Original1152DecorationAndMenuRows) {
    const auto fixture=LoadFixture(NETSTORM_GUMP_DERIVED_REDRAW_FIXTURE).rows;std::size_t count{},decorationCount{},compactCount{},rowCount{};
    for (const auto& row:fixture) { // 각 입력은 독립 저장소/위치 캐시/실제 파생 본체를 사용한다.
        CHECK(row.size()==12);if (row.size()!=12) continue;
        const auto argument=Number(row[2]);GumpCreateNode root;GumpDecorationNode decorationNode;MenuGumpNode menuNode;Load(root,row[3]);
        auto* target=row[0]=="Decoration" ? static_cast<GumpCreateNode*>(&decorationNode) : static_cast<GumpCreateNode*>(&menuNode);Load(*target,row[4]);
        menuNode.menuFlags=Number(row[5]);menuNode.heights[argument]=Float(Number(row[6]));menuNode.offsets[argument]=Float(Number(row[7]));
        const auto base=[&](std::uint32_t token)->GumpCreateNode* { return token==kRoot ? &root : token==kTarget ? target : nullptr; };
        const auto menuLookup=[&](std::uint32_t token)->MenuGumpNode* { return token==kTarget && row[0]!="Decoration" ? &menuNode : nullptr; };
        const auto decorationLookup=[&](std::uint32_t token)->GumpDecorationNode* { return token==kTarget && row[0]=="Decoration" ? &decorationNode : nullptr; };
        const auto backgroundLookup=[&](std::uint32_t token)->GumpBackgroundNode* { return token!=kTarget ? nullptr : row[0]=="Decoration" ? static_cast<GumpBackgroundNode*>(&decorationNode) : static_cast<GumpBackgroundNode*>(&menuNode); };
        std::map<std::uint32_t,ScreenPoint> anchors;std::string projected,requests;
        if (row[8]!="-") for (const auto& encoded:Split(row[8],';')) { const auto values=Inputs(encoded);anchors[values[0]]={Signed(values[1]),Signed(values[2])}; } // 월드 투영의 명시 응답만 공급한다.
        GumpCreateState create;create.collection.root=kRoot;GumpCollectionHooks collectionHooks;collectionHooks.node=base;
        collectionHooks.project=[&](auto sid) { if (!projected.empty()) projected+=',';projected+=std::to_string(sid);return anchors.at(sid); };
        const GumpCollection collection(create.collection,collectionHooks);GumpCreate factory(create,{base,{}});GumpLifetimeState life;
        GumpLifetime lifetime(create,life,{base});GumpEventState eventState;GumpEvents events(create,life,eventState,collection,{base});GumpTreeState treeState;
        GumpTree tree(create,treeState,{base,[](auto,auto) { Unexpected(); },[](auto) { Unexpected(); },[](auto,auto,auto,auto,auto) { Unexpected(); },[](auto) { Unexpected(); }});
        MenuGumpState menuState;MenuGump menu(create,menuState,lifetime,{menuLookup,base});
        const auto invalidate=[&](ScreenRect rect) { if (!requests.empty()) requests+=';';requests+=Numbers(rect.left,rect.top,rect.right,rect.bottom,0); };
        if (row[0]=="Decoration") {
            const std::int8_t color{};GumpBackgroundCreate backgroundFactory(create,factory,color,{backgroundLookup});GumpDrawState clip;GumpBackgroundDrawState backgroundState;
            GumpBackgroundDraw backgroundDraw(collection,clip,backgroundState,{backgroundLookup});GumpDecorationState state;
            GumpDecoration decoration(create,state,backgroundFactory,backgroundDraw,collection,lifetime,{decorationLookup,base,{},{},invalidate});
            decoration.Redraw(kTarget,argument!=0);++decorationCount;
        } else {
            MenuGumpInputState state;MenuGumpInputHooks hooks;hooks.node=menuLookup;hooks.base=base;hooks.invalidate=invalidate;
            MenuGumpInput input(create,state,menu,collection,events,lifetime,tree,std::move(hooks));input.RedrawItem(kTarget,Signed(argument));
            if (row[0]=="Compact") ++compactCount;else { CHECK(row[0]=="Row");++rowCount; }
        }
        if (requests.empty()) requests="-";if (projected.empty()) projected="-";
        const auto caches=Numbers(kRoot,root.cached.x,root.cached.y)+';'+Numbers(kTarget,target->cached.x,target->cached.y);
        const bool same=requests==row[9] && caches==row[10] && projected==row[11];CHECK(same);
        if (!same) { std::fprintf(stderr,"Derived Redraw %s/%s: %s != %s; cache %s != %s\n",row[0].c_str(),row[1].c_str(),requests.c_str(),row[9].c_str(),caches.c_str(),row[10].c_str());break; }++count;
    }
    CHECK(count==1152 && decorationCount==384 && compactCount==384 && rowCount==384);
}
