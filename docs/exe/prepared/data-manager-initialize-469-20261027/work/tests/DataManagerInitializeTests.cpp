// 동결된Native76행과실제Storage/공유표수명을대조하는미빌드검사초안이다.
#include "DudeSpawnSupport.h"
#include "client/ClientTemplateSectionNames.h"
#include "client/CampaignTemplateStorage.h"
#include "client/SoloMapCatalog.h"
#include "o/ConfigInterface.h"
#include "o/RawDataManagerInitialize.h"
#include "o/RawTemplateSectionNames.h"
#include <array>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // Native장치의입력만재생하며초기화판단은제품초안본문이수행한다.
constexpr std::uint32_t kInitManager=0x10001000,kInitTable=0x10002000,kInitAlternate=0x10003000; // 실제가상입력주소와같은불투명번호다.
// Native에서실제RET만실행하는004437f0은C++효과가없으므로trace의그한항목만제외한다.
std::string InitializeTraceWithoutNoop(std::string_view text) { std::string result;for (const auto& entry:Split(text,';')) { if (entry=="baseNoop") continue;if (!result.empty()) result+=';';result+=entry; }return result; }
// 같은물리이름포인터의현재NUL을읽는다.부모에서선행길이를계산하지않는다.
std::string_view InitializeText(std::span<const std::uint8_t> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("초기화검사실제NUL범위");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; }
// 명시formatter출력과table/manager변이를동결Native와동일한순서로제공한다.
struct InitializeReplay {
    std::array<std::uint8_t,12> manager{}; // Native의고정this와root/current전체12BYTE다.
    std::array<std::uint8_t,512> table{},alternate{}; // 미정영역까지A5로명시한검사입력이며제품기본초기값이아니다.
    std::array<std::uint8_t,7> formatted{}; // 서식장치가반환하는정의된Terr00/NUL범위다.
    std::uint32_t current{},mutation{},allocation{1},creates{},selected{},index{}; // 실제전역/외부장치입력과호출횟수다.
    std::string trace; // 하위호출의고정포인터와현재문자열을관측한다.
    // Native와같은kind/root/외부장치입력을준비한다.
    InitializeReplay(std::uint32_t kind,std::uint32_t root,std::uint32_t change,std::uint32_t allocated=1):mutation(change),allocation(allocated) {
        table.fill(0xa5);alternate.fill(0xa5);Put(manager,0,0x514690);Put(manager,4,root);Put(manager,8,0xa5a5a5a5);const std::string alt="/Alt/Last//";std::copy(alt.begin(),alt.end(),alternate.begin());alternate[alt.size()]=0;
        if (kind==1) { const std::string raw="/One/Two//";std::copy(raw.begin(),raw.end(),table.begin());table[raw.size()]=0; }
        if (kind==2) { constexpr std::array<std::uint8_t,11> raw{0,'O','n','e',0,'T','w','o',0,0,0xff};std::copy(raw.begin(),raw.end(),table.begin()); }
        if (kind==3) { table[0]='/';table[1]='/';table[2]=0; }current=kind ? kInitTable : 0;
    }
    // trace에이미캡처한인자만붙인다.
    void Event(std::string entry) { if (!trace.empty()) trace+=';';trace+=entry; }
    // 읽기조회자체는전역을변경하지않고같은물리표를반환한다.
    std::span<std::uint8_t> Buffer(std::uint32_t token) { if (token==kInitTable) return table;if (token==kInitAlternate) return alternate;throw std::out_of_range("Native이름표NULL접근"); }
    // 실제getter진입과선택된할당/서식을기록하며부모모듈을그대로실행한다.
    std::span<std::uint8_t> Names() {
        Event("names:"+std::to_string(current));TemplateSectionNamesHooks hooks;hooks.allocate=[this](auto size) { Event("allocate:"+std::to_string(size));return allocation ? kInitTable : 0U; };hooks.buffer=[this](auto token) { return Buffer(token); };
        hooks.format=[this](auto number)->std::span<const std::uint8_t> { Event("format:"+std::to_string(number));formatted={'T','e','r','r',static_cast<std::uint8_t>('0'+number/10),static_cast<std::uint8_t>('0'+number%10),0};return formatted; };
        return Buffer(GetTemplateSectionNames(current,hooks));
    }
    // 한initialize호출의표포인터/인덱스를고정하되전체creates는반복사이에보존한다.
    void Initialize() {
        DataManagerInitializeHooks hooks;hooks.assertion=[this](auto kind) { Event("assert:"+std::to_string(static_cast<std::uint32_t>(kind)));if (mutation==1 && kind==DataManagerInitializeAssertion::RootNotEmpty) current=kInitAlternate; };
        hooks.names=[this] { auto bytes=Names();selected=current;index=0;return bytes; };
        hooks.create=[this](std::span<const std::uint8_t> name) {
            Event("at:"+std::to_string(selected)+':'+std::to_string(index++));const auto base=Buffer(selected);const auto address=selected+static_cast<std::uint32_t>(name.data()-base.data());const auto text=InitializeText(name);Event("create:"+std::to_string(kInitManager)+':'+std::to_string(address)+':'+Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}));++creates;Put(manager,4,0x10004000);Put(manager,8,0x10004000+creates*60);
            if (creates==1 && mutation==2) current=kInitAlternate;
            if (creates==1 && mutation==3) { // 같은표의미래항목만명시외부입력으로바꾼다.
                const std::array<std::string,2> before{std::string("State\0",6),std::string("Two\0",4)},after{std::string("Other\0",6),std::string("Six\0",4)};
                // Native와같은기지문자열만BYTE검색하며길이를바꾸지않는다.
                for (std::size_t at=0;at<before.size();++at) { const auto found=std::search(table.begin(),table.end(),before[at].begin(),before[at].end());if (found!=table.end()) std::copy(after[at].begin(),after[at].end(),found); }
            }
        };
        InitializeDataManager(manager,hooks);Event("at:"+std::to_string(selected)+':'+std::to_string(index)); // 마지막빈항목은Create가없으므로반환위치에서같은무효과관측을기록한다.
    }
};
}

// 초기화64행의전체manager/두표/순서를같은외부입력으로재생한다.
TEST_CASE(DataManagerInitialize_Native64) {
    const auto rows=LoadFixture(NETSTORM_DATA_MANAGER_INITIALIZE_FIXTURE).rows;CHECK(rows.size()==64);
    // 각행의repeat도원래같은Client/manager에서계속한다.
    for (const auto& row:rows) { CHECK(row.size()==11);if (row.size()!=11) return;InitializeReplay scene(Number(row[0]),Number(row[1]),Number(row[3]));for (std::uint32_t call=0;call<Number(row[2]);++call) scene.Initialize();CHECK(row[4]=="1" && row[5]=="0x0");CHECK(scene.current==Number(row[6]));CHECK(Hex(scene.manager)==row[7]);CHECK(Hex(scene.table)==row[8]);CHECK(Hex(scene.alternate)==row[9]);CHECK(scene.trace==InitializeTraceWithoutNoop(row[10])); }
}

// getter12행의최초512할당/재사용/NULL공개후실패를전체BYTE에대조한다.
TEST_CASE(TemplateSectionNames_Native12) {
    const auto rows=LoadFixture(NETSTORM_TEMPLATE_SECTION_NAMES_FIXTURE).rows;CHECK(rows.size()==12);
    // 기존표이면할당입력0에서도실제할당을선택하지않는다.
    for (const auto& row:rows) { CHECK(row.size()==10);if (row.size()!=10) return;InitializeReplay scene(Number(row[0]),0,0,Number(row[1]));bool completed=true;try { for (std::uint32_t call=0;call<Number(row[2]);++call) static_cast<void>(scene.Names()); }catch (const std::out_of_range&) { completed=false; }CHECK(completed==(row[3]=="1"));CHECK(row[4]==(completed ? "0x0" : "0x4bcced"));CHECK(scene.current==Number(row[5]));CHECK(Hex(scene.manager)==row[6]);CHECK(Hex(scene.table)==row[7]);CHECK(Hex(scene.alternate)==row[8]);CHECK(scene.trace==InitializeTraceWithoutNoop(row[9])); }
}

// 알려진할당입력을사용해277BYTE밖235BYTE의미작성과동일포인터/변환상태를검사한다.
TEST_CASE(ClientTemplateSectionNames_LazyMutableAndUnknownTail) {
    std::size_t calls{};ClientTemplateSectionNames names([&](auto size) { ++calls;CHECK(size==512);auto bytes=std::make_unique<std::uint8_t[]>(size);std::fill_n(bytes.get(),size,std::uint8_t{0xa5});return bytes; });CHECK(names.CurrentToken()==0 && !calls);auto bytes=names.Names();CHECK(calls==1 && names.CurrentToken()==1);const auto pointer=bytes.data();CHECK(std::all_of(bytes.begin()+277,bytes.end(),[](auto value) { return value==0xa5; }));CHECK(bytes[0]=='/' && bytes[275]=='/' && bytes[276]==0);CHECK(ReadSnartName(bytes,0,{})=="Subscriber" && ReadSnartName(bytes,34,{})=="Terr19" && ReadSnartName(bytes,35,{}).empty());CHECK(bytes[0]==0 && bytes[276]==0xff);CHECK(names.Names().data()==pointer && calls==1 && bytes[0]==0);CHECK(std::all_of(bytes.begin()+277,bytes.end(),[](auto value) { return value==0xa5; }));
}

// 실제할당0은물리실패로남으며같은current0을보존한다.
TEST_CASE(ClientTemplateSectionNames_AllocationZeroIsFailure) {
    std::size_t calls{};ClientTemplateSectionNames names([&](auto size)->std::unique_ptr<std::uint8_t[]> { ++calls;CHECK(size==512);return {}; });CHECK(Throws([&] { static_cast<void>(names.Names()); }));CHECK(names.CurrentToken()==0 && calls==1);
}

// 실제빈Image생성과보호TEST01 reader를같은Client표에연결해목록/stream/cursor/수명을검사한다.
TEST_CASE(CampaignTemplates_EmptyImageAndExistingFileShareLazyNames) {
    std::size_t calls{};const auto names=std::make_shared<ClientTemplateSectionNames>([&](auto size) { ++calls;auto bytes=std::make_unique<std::uint8_t[]>(size);std::fill_n(bytes.get(),size,std::uint8_t{0xa5});return bytes; });CampaignTemplateStorage fresh({},names);fresh.SerializationVersion()=0xdeadbeef;const auto token=fresh.AllocateEmptyManager(276);CHECK(token && fresh.OwnedImages()==1 && Get(fresh.Headers(),20)==0 && !calls);CHECK(fresh.ConstructEmptyManager(token,"d/New.fort")==token && fresh.SerializationVersion()==0);CHECK(Get(fresh.ManagerBytes(token),4)==0 && !calls);Put(fresh.Headers(),20,token);fresh.InitializeEmptyManager(token);const auto pointer=names->Names().data();CHECK(calls==1);const auto order=fresh.SectionOrder(1);CHECK(order==std::vector<std::string>(FortTemplate::SectionNames().begin(),FortTemplate::SectionNames().end()) && fresh.CurrentSection(1)=="Terr19");
    // 각stream은같은실제raw생성의비NULL버퍼/pos0/end0/16384용량을가진다.
    for (const auto name:FortTemplate::SectionNames()) { const auto stream=fresh.FindSection(1,name);fresh.AccessSection(stream,[](auto raw,const auto&) { CHECK(Get(raw,0)!=0 && Get(raw,4)==0 && Get(raw,8)==0 && Get(raw,12)==16384); }); }
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto file=assets.files.Read("d/TEST01.fort");CampaignTemplateStorage existing({},names);existing.InstallFile(1,FortTemplate::Parse(file,assets.game.TypeTable()),file,"d/TEST01.fort");CHECK(names->Names().data()==pointer && calls==1 && names->Names()[0]==0);fresh.ClearAll();existing.ClearAll();CHECK(names->Names().data()==pointer && calls==1 && !fresh.OwnedImages() && !existing.OwnedImages());
}

// 실제목록의분류/메타임시manager와주Storage가한512표를공유하는지확인한다.
TEST_CASE(SoloMapCatalog_ProbeManagersShareClientSectionNames) {
    std::size_t allocations{};const auto names=std::make_shared<ClientTemplateSectionNames>([&](auto size) { ++allocations;auto bytes=std::make_unique<std::uint8_t[]>(size);std::fill_n(bytes.get(),size,std::uint8_t{0xa5});return bytes; });CampaignTemplateStorage main({},names);const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config({});config.Set("fortSpec","d/TEST01.fort");SoloMapCatalogEffects effects;effects.sectionNames=main.SectionNamesOwner();effects.typeCount=[] { return 188; };effects.userName=[] { return std::string("author"); };effects.translate=[](auto text) { return std::string(text); };effects.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5}); };SoloMapCatalog catalog(assets.files,config,std::move(effects));CHECK(!allocations);CHECK(catalog.Kind("TEST01")==2);const auto pointer=names->Names().data();CHECK(allocations==1 && names->Names()[0]==0);CHECK(catalog.Refresh(2)==1 && catalog.Cached().names[0]=="TEST01");catalog.Invalidate();CHECK(catalog.Kind("TEST01")==2 && catalog.Refresh(2)==1);CHECK(names->Names().data()==pointer && allocations==1 && main.SectionNamesOwner()==names);
}
