// 로그인송신의임시Template/strip이미지를실제파일입력과정적원문계약으로검사한다.게임/통신/Native실행은하지않는다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "DudeSpawnSupport.h"
#include <algorithm>
#include <array>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::o;
namespace { //검사입력해석은제품의Save/Parse를호출하지않고파일길이레코드만읽는다.
// 파일의35개길이WORD와payload를독립해석한다.결과생성에는제품직렬화기를사용하지않는다.
std::vector<std::vector<std::uint8_t>> LoginImageSections(std::span<const std::uint8_t> bytes) {
    if (bytes.size()<2) throw std::out_of_range("로그인이미지검사의서명누락");std::vector<std::vector<std::uint8_t>> result;std::size_t position=2;
    // 원본고정35개이름에대응하는길이와payload를파일순서대로읽는다.
    for (std::size_t index=0;index<35;++index) {
        if (position>bytes.size() || bytes.size()-position<2) throw std::out_of_range("로그인이미지검사의길이누락");const auto length=std::size_t(bytes[position])+(std::size_t(bytes[position+1])<<8);
        if (length<2 || length>bytes.size()-position) throw std::out_of_range("로그인이미지검사의payload범위");result.emplace_back(bytes.begin()+position+2,bytes.begin()+position+length);position+=length;
    }
    return result;
}
// 선택한파일이미지만공급한다.지역Template는권한/현재전역/제목을조회하면실패하도록지정한다.
TemplateFileLoadHooks LoginImageFile(std::vector<std::uint8_t> bytes) {
    TemplateFileLoadHooks file;file.read=[bytes=std::move(bytes)](auto) { return bytes; };file.typeCount=[] { return 188; };
    file.readOnly=[](auto)->bool { throw std::logic_error("지역readonlyTemplate의파일권한조회금지"); };
    file.currentSlot=[]()->std::uint32_t { throw std::logic_error("지역Template의전역현재슬롯조회금지"); };
    file.refreshTitle=[] { throw std::logic_error("지역Template의전역제목갱신금지"); };
    file.tooManyTypes=[] { throw std::logic_error("검사입력에없는TYPE진단"); };file.rejectFile=[](auto) { throw std::logic_error("검사입력에없는파일제거요청"); };return file;
}
// 명시예외관측은제품의다른진단을성공으로바꾸지않고그대로문자열로반환한다.
template<class Function> std::string LoginImageFailure(Function function) {
    try { function(); }catch (const std::exception& error) { return error.what(); }return {};
}
}

TEST_CASE(CampaignTemplateLoginImage_ActualFileStripsFiveSectionsAndKeepsGlobalOwner) { //원문004bf3e0의고정4개/현재Terr1개외payload는모두비어야한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto bytes=assets.files.Read("d/thewarbegins.fort");const auto original=LoginImageSections(bytes);bytes[1]=0xfe; //source의FE서명이NONAME의기본0서명으로복제되지않는별도입력이다.
    auto globals=std::make_shared<FortSerializationGlobals>();std::size_t nameAllocations{};auto names=std::make_shared<ClientTemplateSectionNames>([&](auto size) { ++nameAllocations;auto allocation=std::make_unique<std::uint8_t[]>(size);std::fill_n(allocation.get(),size,std::uint8_t{0xa5});return allocation; });
    CampaignTemplateStorage storage(globals,names);storage.InstallFile(1,{},bytes,"global-kept.fort");storage.SelectState(1).position=5;const auto* allocation=storage.State(1).bytes.data();const auto headers=std::vector<std::uint8_t>(storage.Headers().begin(),storage.Headers().end());const auto namePointer=names->Names().data();
    CHECK(nameAllocations==1 && !original[4].empty()); //원본Chaff의비영payload도strip후사라지는지확인한다.
    // 서로다른현재영역은각각자기Terr만보유하며이전호출의임시manager를재사용하지않는다.
    for (const std::uint32_t territory:{0U,7U,19U}) {
        std::size_t reads{},typeReads{};auto file=LoginImageFile(bytes);globals->version=123;
        file.read=[&](auto name) { ++reads;CHECK(name=="source.fort" && globals->version==0 && storage.OwnedImages()==1 && storage.CurrentSection(1)=="State");globals->version=19;return bytes; };
        file.typeCount=[&] { ++typeReads;if (typeReads==1) { CHECK(globals->version==19);globals->version=37; }else { CHECK(typeReads==2 && globals->version==0);globals->version=53; }return 188; }; //source생성뒤읽기와NONAME생성뒤저장의같은전역0쓰기를구별한다.
        const auto image=storage.ReadLoginImage("source.fort",file,territory);const auto sections=LoginImageSections(image.Bytes());const auto sectionNames=FortTemplate::SectionNames();const auto selected="Terr"+std::string(territory<10 ? "0" : "")+std::to_string(territory);std::size_t expectedEnd=72;
        CHECK(image.storage.size()==32768 && image.Bytes()[0]=='F' && !image.Bytes()[1] && reads==1 && typeReads==2 && globals->version==53);
        // 원문5개이름의독립payload와빈나머지를모두비교한다.
        for (std::size_t index=0;index<sectionNames.size();++index) {
            const auto name=sectionNames[index];const bool keep=name=="Subscriber" || name=="Territory" || name=="TypeNames" || name=="CoreData" || name==selected;
            if (keep) { CHECK(sections[index]==original[index]);expectedEnd+=original[index].size(); }else CHECK(sections[index].empty());
        }
        CHECK(image.end==expectedEnd && storage.OwnedImages()==1 && storage.Name(1)=="global-kept.fort" && storage.CurrentSection(1)=="State");
        CHECK(std::equal(storage.Headers().begin(),storage.Headers().end(),headers.begin()) && storage.State(1).position==5 && storage.State(1).bytes.data()==allocation && names->Names().data()==namePointer && nameAllocations==1);
    }
}

TEST_CASE(CampaignTemplateLoginImage_FalsePartialAndDeviceExceptionsPreserveGlobalOwner) { //임시실패뒤전역owner/current를그대로두고다음정상읽기도가능해야한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto bytes=assets.files.Read("d/thewarbegins.fort");CampaignTemplateStorage storage;storage.InstallFile(1,{},bytes,"kept.fort");storage.SelectState(1).position=3;const auto headers=std::vector<std::uint8_t>(storage.Headers().begin(),storage.Headers().end());
    // 부재정상false/장치예외/일부섹션읽기예외/현재Terr누락을각각독립임시수명으로실행한다.
    for (std::uint32_t variant=0;variant<4;++variant) {
        auto file=LoginImageFile(bytes);const auto territory=variant==3 ? 20U : 0U;storage.SerializationVersion()=91;
        if (!variant) file.read=[](auto) { return std::vector<std::uint8_t>{}; };
        if (variant==1) file.read=[](auto)->std::vector<std::uint8_t> { throw std::runtime_error("선택파일장치중단"); };
        if (variant==2) file.read=[](auto) { return std::vector<std::uint8_t>{'F',0xfe,5,0,11,13,17,8,0}; }; //첫Subscriber노드는붙고둘째길이읽기가물리범위를벗어난다.
        const auto failure=LoginImageFailure([&] { storage.ReadLoginImage("temporary.fort",file,territory); });CHECK(!failure.empty());
        if (!variant) CHECK(failure=="로그인송신의지역Template읽기실패진단");if (variant==1) CHECK(failure=="선택파일장치중단");if (variant==3) CHECK(failure=="로그인이미지복제의원문섹션누락진단");
        CHECK(storage.SerializationVersion()==0 && storage.OwnedImages()==1 && storage.CurrentSection(1)=="State" && storage.State(1).position==3 && std::equal(storage.Headers().begin(),storage.Headers().end(),headers.begin()));
        const auto recovered=storage.ReadLoginImage("temporary.fort",LoginImageFile(bytes),0);CHECK(recovered.end>72 && storage.OwnedImages()==1 && storage.Name(1)=="kept.fort");
    }
}
