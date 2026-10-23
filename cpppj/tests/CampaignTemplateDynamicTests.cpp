// Temp의 실제 노드/할당/생성수명을 Rename 이후 표준 파일 저장과 공유하는지 검사한다.
#include "RawSceneSupport.h"
#include "client/CampaignTemplateStorage.h"
#include <array>
#include <cstdio>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 기존35개 빈 파일 섹션을 최초 입력으로 만들며 실행 중 Temp를 고정 Terr handle로 대신하지 않는다.
FortTemplate EmptyFort() { FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);return fort; }
// 원본 진단/해제된 참조는 실제 예외여야 한다.
template<class Function> bool DynamicRejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
// 같은 MemStream의 실제 버퍼 주소와 세 DWORD를 공개하는 동기 검사다.
struct StreamObservation { const std::uint8_t* data{};std::uint32_t pointer{},position{},end{},capacity{}; }; // 호스트 주소는 비교에만 사용하고 DWORD로 자르지 않는다.
// 이름/handle의 편의 계산 없이 저장소의 같은 실제 raw stream을 조회한다.
StreamObservation Observe(CampaignTemplateStorage& storage,TemplateWriteStream stream) {
    StreamObservation observed;storage.AccessSection(stream,[&](auto fields,const auto& memory) {
        observed.pointer=Get(fields,0);observed.position=Get(fields,4);observed.end=Get(fields,8);observed.capacity=Get(fields,12);
        if (observed.pointer) observed.data=memory.buffer(observed.pointer).data();
    });return observed;
}
}
// 실제 Rename은 원래 Temp node와 allocation을 남기고 대상 노드만 반납한다.
TEST_CASE(CampaignTemplates_DynamicTempRenamePreservesNodeAndAllocation) {
    CampaignTemplateStorage storage;storage.Install(1,EmptyFort(),"dynamic.fort");storage.Borrow(8,1);
    const auto previous=storage.FindSection(1,"Terr00"),source=storage.CreateSection(8,"Temp");
    const std::array<std::uint8_t,3> bytes{2,7,9};storage.WriteSection(source,bytes);const auto before=Observe(storage,source);
    const auto renamed=storage.RenameSection(1,"Terr00","Temp");const auto after=Observe(storage,source);
    CHECK(renamed.manager==source.manager && renamed.section==source.section && renamed.generation==source.generation);
    CHECK(renamed.section!=previous.section && before.pointer==after.pointer && before.data==after.data && before.position==after.position && before.end==after.end && before.capacity==after.capacity);
    CHECK(storage.Template(8).Section("Terr00").data()==before.data && storage.Template(1).Section("Terr00").size()==3 && storage.CurrentSection(8)=="Terr00");
    const auto order=storage.SectionOrder(1);CHECK(order.size()==35 && order.back()=="Terr00" && std::count(order.begin(),order.end(),"Terr00")==1 && std::count(order.begin(),order.end(),"Temp")==0);
    CHECK(DynamicRejects([&] { storage.WriteSection(previous,bytes); }));const auto absent=storage.FindSection(8,"Temp");CHECK(!absent.manager && !absent.section && !absent.generation && storage.CurrentSection(1).empty());
    CHECK(DynamicRejects([&] { storage.CurrentWriteStream(1); }));storage.WriteSection(source,std::span(bytes).first(1));CHECK(storage.Template(1).Section("Terr00").size()==4);
    const auto selected=storage.SelectSection(8,"tErR00",0);CHECK(selected.section==source.section && !Observe(storage,source).position);
    storage.Clear(8);CHECK(storage.OwnedImages()==1);storage.Clear(1);CHECK(DynamicRejects([&] { storage.WriteSection(source,bytes); }));
}
// Temp의 반복 생성/이름 교체는20개 현재 버퍼와 저장 파일의35개 섹션을 같은 할당으로 연결한다.
TEST_CASE(CampaignTemplates_DynamicTwentyTerritoriesAndCurrentSave) {
    CampaignTemplateStorage storage;storage.Install(1,EmptyFort(),"all-territories.fort");std::array<TemplateWriteStream,20> references{};
    for (std::uint32_t index=0;index<20;++index) { // 실제 원본 부모의 Temp→TerrNN 흐름을 이름별로 반복한다.
        char name[16]{};std::snprintf(name,sizeof(name),"Terr%02u",index);const auto temporary=storage.CreateSection(1,"Temp");
        const std::array<std::uint8_t,3> bytes{2,static_cast<std::uint8_t>(index),static_cast<std::uint8_t>(index+40)};storage.WriteSection(temporary,bytes);const auto pointer=Observe(storage,temporary).data;
        references[index]=storage.RenameSection(1,name,"Temp");CHECK(references[index].section==temporary.section && references[index].generation==temporary.generation && storage.Template(1).Section(name).data()==pointer);
    }
    std::uint32_t readOnly{},owner{};std::array<std::uint8_t,9> hidden{};std::vector<std::uint8_t> saved;std::uint32_t permissions{};
    TemplateFileWriteHooks file;file.permissions=[&](auto name,auto mode) { CHECK(name=="all-territories.fort" && mode==0x180);++permissions; };file.write=[&](auto name,auto bytes) { CHECK(name=="all-territories.fort");saved.assign(bytes.begin(),bytes.end()); };
    storage.SaveFile(1,{readOnly,owner,hidden},{[] { return 188U; },[](auto) { throw std::logic_error("동적 섹션 저장의 예상 밖 진단"); }},file);
    CHECK(permissions==1 && saved.size()==132 && saved[0]=='F' && saved[1]==0 && storage.CurrentSection(1)=="Terr19");std::size_t at=2;
    for (const auto name:FortTemplate::SectionNames()) { // 실제 SaveFile의 각 WORD 길이와 현재 buffer payload를 순서대로 대조한다.
        CHECK(at+2<=saved.size());const auto length=std::uint16_t(saved[at]|std::uint16_t(saved[at+1])<<8);CHECK(length>=2);at+=2;const auto bytes=storage.Template(1).Section(name);
        CHECK(length==bytes.size()+2 && at+bytes.size()<=saved.size() && std::equal(bytes.begin(),bytes.end(),saved.begin()+at));at+=bytes.size();
    }
    CHECK(at==saved.size() && storage.SectionOrder(1).size()==35);const std::array<std::uint8_t,1> appended{99};storage.WriteSection(references.back(),appended);CHECK(storage.Template(1).Section("Terr19").back()==99);
}
// 표준 이름에서 동적 이름으로 옮긴 동일 노드도 typed cursor를 잘못 재사용하거나 원래 표준 노드를 되살리지 않는다.
TEST_CASE(CampaignTemplates_DynamicTypedCursorFollowsCurrentName) {
    CampaignTemplateStorage storage;storage.Install(1,EmptyFort(),"typed.fort");const auto temp=storage.CreateSection(1,"Temp");const std::array<std::uint8_t,2> bytes{17,29};storage.WriteSection(temp,bytes);
    const auto state=storage.RenameSection(1,"State","Temp");CHECK(state.section==temp.section && storage.State(1).position==2 && storage.State(1).end==2 && storage.State(1).bytes.data()==Observe(storage,temp).data);
    storage.State(1).position=1;const auto before=Observe(storage,temp);const auto renamed=storage.RenameSection(1,"Scratch","State");CHECK(renamed.section==temp.section && Observe(storage,temp).position==before.position && storage.State(1).bytes.empty());
    CHECK(!storage.FindSection(1,"State").section);const auto dynamicOrder=storage.SectionOrder(1);CHECK(std::count(dynamicOrder.begin(),dynamicOrder.end(),"State")==0);
    const auto restored=storage.RenameSection(1,"State","Scratch");CHECK(restored.section==temp.section && restored.generation==temp.generation && storage.State(1).position==1 && storage.State(1).end==2);
    const auto replacement=storage.CreateSection(1,"State");CHECK(replacement.section!=temp.section && DynamicRejects([&] { storage.AccessSection(temp,[](auto,const auto&) {}); }));
}
