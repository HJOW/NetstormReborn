// Template9의 최초 전체 원문과 실제 소유/빌림·State 섹션의 같은 저장소를 검사한다. 실행은 통합 단계로 이월한다.
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h"
#include "o/RawFortImage.h"
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 최초 관측의 전체16진 바이트를 입력/기대 영역으로 읽는다.
std::vector<std::uint8_t> LifetimeBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 제품의 소유/반납 판단을 계산하지 않고 한 바이트씩 읽는다.
    for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));return result;
}
// 기존 파서가 사용하는 State의 실제 섹션 위치다.
std::size_t LifetimeStateIndex() { const auto names=FortTemplate::SectionNames();return static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin()); }
}
// 전역/단일/초기화와 각 소멸/free 직전 전체 헤더를 독립240행에 대조한다.
TEST_CASE(TemplateLifetime_Native240) {
    const auto rows=LoadFixture(NETSTORM_TEMPLATE_LIFETIME_FIXTURE).rows;CHECK(rows.size()==240);
    // 각 행은 같은 전체196BYTE 입력과 현재 효과 변이를 사용한다.
    for (const auto& row:rows) {
        CHECK(row.size()==4);if (row.size()!=4) return;const auto input=Split(std::string_view(row[0]).substr(1,row[0].size()-2),',');CHECK(input.size()==6);if (input.size()!=6) return;
        auto bytes=LifetimeBytes(row[1]);auto headers=std::span(bytes).subspan(8,180);const auto selected=Number(input[4]),variant=Number(input[5]);std::string trace; // 원문과 같은 현재 표/효과 입력이다.
        const auto record=[&](char kind,std::uint32_t handle) { if (!trace.empty()) trace+=';';trace+=kind;trace+=','+std::to_string(handle)+','+Hex(bytes); }; // 실제 경계 직전 상태를 관측한다.
        TemplateLifetimeHooks effects;effects.destroy=[&](auto handle) { record('D',handle);const auto slot=(handle-0x12100000U)/0x1000U;if (variant==1) { Put(headers,slot*20+4,0x87654321);if (slot<8) Put(headers,(slot+1)*20+4,1); } }; // 고정 힙 입력 주소는 원문의 외부 불투명 handle이다.
        effects.release=[&](auto handle) { record('F',handle);const auto slot=(handle-0x12100000U)/0x1000U;if (variant==2) { Put(headers,slot*20,0xdead0000);Put(headers,slot*20+16,0x76543210);if (slot<8) Put(headers,(slot+1)*20,0); } };
        if (input[0]=="\"A\"") ClearTemplateHeaders(headers,effects);else if (input[0]=="\"S\"") ReleaseTemplateHeader(headers.subspan(selected*20,20),effects);else InitializeTemplateHeader(headers.subspan(selected*20,20));
        CHECK(Hex(bytes)==row[2] && (trace.empty() ? "-" : trace)==row[3]);
    }
}
// 실제 파일의 State 변경은 같은 manager의 저장 이미지에도 반영되고 빌린 헤더는 같은 cursor를 사용한다.
TEST_CASE(CampaignTemplates_ExplicitOwnershipBorrowAndSerializedState) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CampaignTemplateStorage storage;storage.Install(1,FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable()),"thewarbegins");
    CHECK(storage.OwnedImages()==1 && storage.State(1).bytes.data()==storage.Template(1).Section("State").data());storage.Borrow(8,1);CHECK(&storage.Template(8)==&storage.Template(1) && &storage.State(8)==&storage.State(1) && &storage.Territory(8)==&storage.Territory(1));
    storage.State(8).position=7;storage.State(8).bytes[0]^=0x40;const auto current=storage.State(1).bytes[0];CHECK(storage.State(1).position==7 && storage.Template(1).Section("State")[0]==current);
    const auto image=RawFortImage(OriginalEdition::Patch1078,storage.Template(1),{[&] { return static_cast<std::uint32_t>(assets.game.TypeTable().Types().size()); },{}}).Save();const auto saved=FortTemplate::Parse(image.Bytes(),assets.game.TypeTable());CHECK(saved.Section("State")[0]==current);
    storage.Clear(8);CHECK(storage.OwnedImages()==1 && Get(storage.Headers(),8*20)==0 && storage.State(1).position==7);storage.Borrow(0,1);Put(storage.Headers(),2*20+8,0x7391);storage.ClearAll();CHECK(!storage.OwnedImages() && Get(storage.Headers(),0)==0 && Get(storage.Headers(),20)==0 && Get(storage.Headers(),2*20+8)==0x7391); // 빈 포인터의 stale 필드는 전역 정리에서 보존한다.
}
// 짧은 State의 확장은 원문 복사 뒤 실제 Fort 섹션을 교체하며 빌린 manager의 cursor도 같다.
TEST_CASE(CampaignTemplates_ShortStateGrowthUsesSameSection) {
    FortTemplate fort;fort.sections.resize(LifetimeStateIndex()+1);fort.sections[LifetimeStateIndex()]={0x12,0x34,0x56};CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"short");storage.Borrow(8,1);std::uint32_t depth{};bool mission{}; // 원본28BYTE 확장의 명시 입력이며 결과를 주입하지 않는다.
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.State(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("State 확장의 예상 밖 진단"); },[](auto) {},[&](auto slot,auto size) { return storage.GrowState(slot,size); },[&](auto slot) { storage.CommitStateGrowth(slot); });
    const auto bytes=state.Get(8);CHECK(bytes.size()==28 && bytes[0]==0x12 && bytes[1]==0x34 && bytes[2]==0x56 && std::all_of(bytes.begin()+3,bytes.end(),[](auto value) { return value==0; }));CHECK(storage.State(1).position==28 && storage.State(1).end==28 && bytes.data()==storage.Template(1).Section("State").data());storage.ClearAll();CHECK(!storage.OwnedImages());
}
