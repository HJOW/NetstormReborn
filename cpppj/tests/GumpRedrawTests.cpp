// 실제 PE의 기본 UI 갱신 사각형·가시성/캐시·지도 clamp 관측을 그대로 재생한다.
#include "ConstructionSupport.h"
#include "client/GumpRedraw.h"
#include <map>
#include <sstream>

namespace { // 입력은 원문 raw 노드이며 기대 사각형은 독립 fixture에서만 읽는다.
using namespace netstorm::test::construction; // 기존 UTF-8 fixture/원본 DWORD 읽기 도구다.
using namespace netstorm::client; // 같은 제품의 위치/갱신 본체를 사용한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //원문 하위DWORD를 signed 좌표로 해석한다.
std::vector<std::uint8_t> Bytes(const std::string& text) { //완성128바이트 노드 입력을 원래 순서로 읽는다.
    std::vector<std::uint8_t> result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); //두16진수 글자가 한 입력BYTE다.
    return result;
}
template<class... Values> std::string Numbers(Values... values) { //기대 관측과 같은32비트 decimal 인수열이다.
    std::ostringstream stream;bool first=true;const auto append=[&](auto value) { if (!first) stream<<',';first=false;stream<<static_cast<std::uint32_t>(value); };(append(values),...);return stream.str();
}
ScreenRect Rectangle(const std::string& text) { //LTRB의 signed 원문 비트를 복원한다.
    const auto values=Inputs(text);if (values.size()!=4) throw std::invalid_argument("갱신 사각형 fixture 필드 오류");return {Signed(values[0]),Signed(values[1]),Signed(values[2]),Signed(values[3])};
}
// 같은 원문 토큰의 현재 노드/투영 응답이며 표시용 게임 상태를 새로 만들지 않는다.
struct RedrawScene {
    std::map<std::uint32_t,GumpCollectionNode> nodes; //원문128바이트에서 사용하는 현재 필드와 위치 캐시다.
    std::map<std::uint32_t,ScreenPoint> anchors; //원문497220의 명시 응답이며 투영 본체는 기존 fixture의 별도 범위다.
    std::string projected; //실제 조회된 SID 순서다.
    void Load(const std::string& text) { //원문이 읽는 필드만 같은 바이트 위치에서 복원한다.
        for (const auto& input:Split(text,';')) { //각 행은 원문 주소=완성raw 노드다.
            const auto fields=Split(input,'=');const auto bytes=Bytes(fields[1]);auto& n=nodes[Number(fields[0])];
            n.flags=Get(bytes,4);n.next=Get(bytes,12);n.parent=Get(bytes,20);n.child=Get(bytes,24);n.object=Get(bytes,36);n.depth=Get(bytes,48);
            n.width=Float(Get(bytes,68));n.height=Float(Get(bytes,72));n.x=Float(Get(bytes,76));n.y=Float(Get(bytes,80));n.cached={Signed(Get(bytes,108)),Signed(Get(bytes,112))};
        }
    }
    GumpCollectionHooks Hooks() { //현재 노드/투영 조회를 제품 위치 본체에 빌린다.
        GumpCollectionHooks hooks;hooks.node=[this](auto token)->GumpCollectionNode* { const auto found=nodes.find(token);return found==nodes.end() ? nullptr : &found->second; };
        hooks.project=[this](auto sid) { if (!projected.empty()) projected+=',';projected+=std::to_string(sid);return anchors.at(sid); };return hooks;
    }
    std::string Caches() const { //주소순으로 모든 실제 노드 캐시를 직렬화한다.
        std::string result;
        for (const auto& [token,n]:nodes) { if (!result.empty()) result+=';';result+=Numbers(token,n.cached.x,n.cached.y); } //원문 메모리 순서다.
        return result;
    }
};
}

// 제품 계산으로 기대값을 만들지 않고 원문1,474개 행 전체의 요청·캐시·투영을 대조한다.
TEST_CASE(GumpRedraw_Original1474RequestsCachesAndMapClamp) {
    const auto fixture=LoadFixture(NETSTORM_GUMP_REDRAW_FIXTURE).rows;std::size_t count{},whole{},map{},clamp{};
    for (const auto& row:fixture) { //사례마다 현재 노드와 지도/출력 저장소를 새로 만든다.
        CHECK(row.size()==11);if (row.size()!=11) continue;
        auto bounds=Rectangle(row[4]);std::string requests,caches="-",projected="-";
        if (row[0]=="Clamp") { const auto result=ClampOriginalScreenRect(Rectangle(row[10]),bounds);requests=Numbers(result.left,result.top,result.right,result.bottom);++clamp; }
        else {
            RedrawScene scene;scene.Load(row[5]);
            if (row[6]!="-") for (const auto& encoded:Split(row[6],';')) { const auto values=Inputs(encoded);scene.anchors[values[0]]={Signed(values[1]),Signed(values[2])}; } //원문 투영 경계의 완성 응답이다.
            GumpCollectionState state{scene.nodes.begin()->first,0,OriginalEdition::Patch1078};auto hooks=scene.Hooks();const GumpCollection collection(state,hooks);
            GumpRedraw redraw(collection,{hooks.node,[&] { return bounds; },[&](auto rectangle,auto flags) { if (!requests.empty()) requests+=';';requests+=Numbers(rectangle.left,rectangle.top,rectangle.right,rectangle.bottom,flags); }});
            if (row[0]=="Whole") { redraw.Whole(Number(row[3]),Number(row[2]));++whole; } else { CHECK(row[0]=="Map");redraw.Map(Number(row[3]),Number(row[2]));++map; }
            if (requests.empty()) requests="-";caches=scene.Caches();projected=scene.projected.empty() ? "-" : scene.projected;
        }
        const bool same=requests==row[7] && caches==row[8] && projected==row[9];CHECK(same);
        if (!same) { std::fprintf(stderr,"Gump Redraw %s/%s: %s != %s; cache %s != %s\n",row[0].c_str(),row[1].c_str(),requests.c_str(),row[7].c_str(),caches.c_str(),row[8].c_str());break; }++count;
    }
    CHECK(count==1474 && whole==728 && map==728 && clamp==18);
}

// 실제 호출 순서는 Position/크기 뒤 지도 조회이며 손상된 노드/순환은 호스트 계약으로 거부한다.
TEST_CASE(GumpRedraw_CurrentProjectionThenMapAndHostContracts) {
    RedrawScene scene;scene.nodes[1].flags=1;auto& n=scene.nodes[2];n.flags=0x2001;n.parent=1;n.object=7;n.width=12.75F;n.height=9.5F;
    GumpCollectionState state{1};ScreenRect map{0,0,10,10};std::string order;std::vector<ScreenRect> requests;auto hooks=scene.Hooks();
    hooks.project=[&](auto sid) { CHECK(sid==7);order+='P';map={100,200,300,400};n.width=20.75F;return ScreenPoint{111,211}; }; //현재 조회가 바꾼 크기/지도는 그 뒤 요청에서 읽어야 한다.
    GumpCollection collection(state,hooks);GumpRedraw redraw(collection,{hooks.node,[&] { order+='M';return map; },[&](auto rectangle,auto flags) { order+='I';CHECK(flags==0);requests.push_back(rectangle); }});
    redraw.Map(2,0xffffffffU);CHECK(order=="PMI" && requests.size()==1);if (!requests.empty()) CHECK(requests[0].left==111 && requests[0].right==131 && requests[0].top==211 && requests[0].bottom==220);
    order.clear();requests.clear();scene.nodes[1].flags=0;redraw.Whole(2);CHECK(order.empty() && requests.empty());
    scene.nodes[1].flags=1;n.flags=1;scene.nodes[1].parent=2;CHECK(Throws([&] { redraw.Whole(2); }));CHECK(Throws([&] { redraw.Map(99); }));
    CHECK(Throws([&] { GumpRedraw invalid(collection,{}); }));
}
