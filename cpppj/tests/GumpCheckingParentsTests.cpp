//현재 Gump 진단의 복귀/중단과 원문 표·포인터 변경을 최초 관측에 대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "client/GumpLifetime.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { //원문의 격리 주소는 호스트에서 역참조하지 않는다.
constexpr std::uint32_t kToken=0x12001000,kMarker=0x12345678,kBefore=0xa5a5a5a5; //Native this·다른 토큰·인접 전역의 독립 입력이다.
std::string HexText(std::string_view text) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //진단 인자의 실제 바이트다.
std::string Rle(std::span<const std::uint32_t> values) { //변경된 전체 표를 값 손실 없이 직렬화한다.
    std::string result;for (std::size_t start=0;start<values.size();) { auto end=start+1;while (end<values.size() && values[end]==values[start]) ++end;if (!result.empty()) result+=';';result+=std::to_string(end-start)+':'+std::to_string(values[start]);start=end; }return result; //연속 구간의 길이와 값을 보존한다.
}
std::array<std::uint32_t,100> Overlay(std::uint32_t pattern) { //독립 원문과 같은 입력 표만 만든다.
    std::array<std::uint32_t,100> result{};if (pattern>=3) result.fill(kMarker);
    if (pattern==1) for (std::size_t index=0;index<result.size();++index) result[index]=index%2 ? 0 : kMarker; //혼합 입력은 마지막 빈 칸99를 남긴다.
    if (pattern==2 || pattern==4) result[3]=result[99]=kToken;return result;
}
const std::vector<std::vector<std::string>>& Rows() { //기대 결과는 파일에서만 읽으며 새로 계산하거나 쓰지 않는다.
    static const auto rows=[] { std::ifstream input(NETSTORM_GUMP_CHECKING_PARENTS_FIXTURE);if (!input) throw std::runtime_error("Gump 진단 최초 관측 파일 없음");std::vector<std::vector<std::string>> result;std::string line;
        while (std::getline(input,line)) if (!line.empty() && line[0]!='#') result.push_back(netstorm::test::rawscene::Split(line,'\t'));return result; //설명 한 행만 제외한다.
    }();return rows;
}
struct GumpCheckingScene { //한 관측의 실제 저장소와 별도 인접 DWORD를 소유한다.
    GumpCreateState create;GumpLifetimeState life;GumpCreateNode node; //실제 소멸 부모의 현재 상태/노드다.
    std::uint32_t checking{},before{kBefore},mutate{},resume{}; //현재 진단 DWORD·명시 복귀/효과 입력이다.
    std::string events; //선택 진단과 free 순서만 기록한다.
    GumpCheckingScene(const std::vector<std::string>& p):checking(netstorm::test::rawscene::Number(p[4])),mutate(netstorm::test::rawscene::Number(p[6])),resume(netstorm::test::rawscene::Number(p[5])) { //기대 출력 열은 사용하지 않는다.
        create.collection.edition=netstorm::o::OriginalEdition::Patch1078;life.checking=netstorm::o::RawCheckingFlag{checking};life.overlay=Overlay(netstorm::test::rawscene::Number(p[1]));node.vtable=kMarker;node.flags=netstorm::test::rawscene::Number(p[3]);node.textPointer=0x1111;node.alternateTextPointer=0x2222;
        life.idle[0]=life.idle.back()=life.kill[0]=life.kill.back()=kToken;life.idle[1]=life.kill[1]=kMarker;
    }
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; } //도달한 실제 효과만 순서대로 기록한다.
    void Mutate(std::uint32_t which) { //callback 중 현재 플래그/포인터/표 변화는 독립 입력과 같다.
        checking=0;if (which==1) { node.flags=0x13579bd;node.alternateTextPointer=0x3333;life.overlay[99]=life.overlay[98]=kMarker; }
        else { node.flags=0xcafebabe;node.alternateTextPointer=0x4444;life.overlay[50]=kToken;life.overlay[99]=kMarker; }
    }
    void Assert(std::string_view condition,std::string_view source,std::uint32_t line) { //진단 전 vtable/표 쓰기는 이미 실제 저장소에 반영돼 있어야 한다.
        Event("D:"+HexText(condition)+":"+HexText(source)+":"+std::to_string(line));if (mutate==1) Mutate(1);if (!resume) throw std::runtime_error("assertion");
    }
    std::vector<std::string> Output(std::string outcome) const { //모든 표와 현재 포인터를 원문 순서로 읽는다.
        return {std::move(outcome),events.empty() ? "-" : events,std::to_string(checking),std::to_string(node.flags),std::to_string(node.vtable),std::to_string(node.textPointer),std::to_string(node.alternateTextPointer),std::to_string(before),Rle(life.overlay),Rle(life.idle),Rle(life.kill)};
    }
};
template<class Function> bool Throws(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; } //호스트 물리 범위/선택 진단 중단만 구별한다.
}
TEST_CASE(GumpCheckingParents_NativeOverlay320AndBase48) { //전체 원문 진단·복귀·현재 포인터·표를368입력 모두 대조한다.
    using namespace netstorm::test::rawscene;std::size_t overlay{},base{};
    for (const auto& row:Rows()) { //최초 fixture를 제품 결과에 맞춰 약화하지 않는다.
        CHECK(row.size()==12);if (row.size()!=12) return;const auto p=Split(row[0],',');CHECK(p.size()==7);if (p.size()!=7) return;GumpCheckingScene s(p);std::string outcome="ok";
        try {
            if (p[0]=="O") {
                ++overlay;GumpOverlayParentHooks hooks;hooks.write=[&](auto index,auto value) { if (index==-1) s.before=value;else s.life.overlay.at(static_cast<std::size_t>(index))=value; }; //순수 부모만 원문의 인접 DWORD 쓰기를 독립 owner로 관측한다.
                hooks.flags=[&] { return s.node.flags; };hooks.setFlags=[&](auto value) { s.node.flags=value; };hooks.assertion=[&](auto condition,auto source,auto line) { s.Assert(condition,source,line); };
                RunGumpOverlayParent(kToken,Number(p[2])!=0,s.life.overlay,s.life.Checking(),hooks);
            } else {
                ++base;GumpLifetimeHooks hooks;hooks.node=[&](auto token) { CHECK(token==kToken);return &s.node; };hooks.assertion=[&](auto condition,auto source,auto line) { s.Assert(condition,source,line); };
                hooks.release=[&](auto pointer) { s.Event("F:"+std::to_string(pointer));if (s.mutate==2 && pointer==0x1111) s.Mutate(2); };
                GumpLifetime lifetime(s.create,s.life,std::move(hooks));lifetime.DestructBase(kToken);
            }
        }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        const auto output=s.Output(outcome);const bool same=output==std::vector<std::string>(row.begin()+1,row.end());CHECK(same);if (!same) { std::fprintf(stderr,"Gump 진단 입력%s 결과불일치\n",row[0].c_str());return; }
    }
    CHECK(overlay==320 && base==48);
}
TEST_CASE(GumpCheckingParents_ActualOverlayCurrentViewAndBounds) { //실제 배열의 -1은 진단 뒤 명시 실패하며 현재 DWORD와 마지막 선택 슬롯을 보존한다.
    GumpCreateState create;create.collection.edition=netstorm::o::OriginalEdition::Patch1078;GumpLifetimeState state;GumpCreateNode node;std::int32_t checking{};state.checking=netstorm::o::RawCheckingFlag{checking};std::size_t reads{},assertions{};GumpLifetimeHooks hooks;
    hooks.node=[&](auto token) { CHECK(token==7);++reads;return &node; };hooks.assertion=[&](auto condition,auto source,auto line) { CHECK(source==".\\Gump.cpp");++assertions;if (line==0x4ef) CHECK(condition=="found==-1 && free!=-1");else CHECK(condition=="found!=-1" && line==0x4f4);checking=0; };
    GumpLifetime lifetime(create,state,hooks);lifetime.SetOverlay(7,false);CHECK(!reads && !assertions);checking=std::bit_cast<std::int32_t>(0x80000000U);lifetime.SetOverlay(7,false);CHECK(!reads && assertions==1 && !checking);
    state.overlay[99]=7;lifetime.SetOverlay(7,true);CHECK(assertions==2 && state.overlay[98]==7 && (node.flags&0x80000U)); //중복 등록 진단 뒤 원래 선택한 마지막 빈 칸을 그대로 쓴다.
    state.overlay.fill(9);node.flags=0x123;CHECK(Throws([&] { lifetime.SetOverlay(7,true); }));CHECK(assertions==3 && node.flags==0x123 && std::all_of(state.overlay.begin(),state.overlay.end(),[](auto value) { return value==9; })); //소진 후 복귀는 인접 전역을 훼손하기 직전에서 멈춘다.
    state.checking.reset();state.diagnostics=0xffffffffU;CHECK(state.Checking().Read()==0xffffffffU);state.diagnostics=0;CHECK(!state.Checking()); //기존 독립 DWORD fixture의 주소/현재 값 계약도 유지한다.
}
