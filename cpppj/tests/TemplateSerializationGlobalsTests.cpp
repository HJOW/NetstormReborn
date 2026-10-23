// 두Storage와목록임시manager가실제같은005c89c0 DWORD를쓰는지검사한다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "client/SoloMapCatalog.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 원본파일을사용하지않고목록이실제읽을최소35개섹션이미지만만든다.
std::vector<std::uint8_t> SerializationCatalogImage() {
    std::array<std::uint8_t,28> state{};state[17]=2;std::vector<std::uint8_t> image{'F',0}; // 명시편집맵종류와기존State를넣어빈State생성과구별한다.
    for (std::size_t index=0;index<35;++index) { // 원본길이레코드만작성하며전체게임객체는해석하지않는다.
        std::vector<std::uint8_t> bytes;if (!index) bytes={1,0,0,0,2,0,'M','e'};else if (index==1) bytes.assign(state.begin(),state.end());
        const auto length=bytes.size()+2;image.push_back(static_cast<std::uint8_t>(length));image.push_back(static_cast<std::uint8_t>(length>>8));image.insert(image.end(),bytes.begin(),bytes.end());
    }return image;
}
// 실행 작업 디렉터리와 무관하게 빌드 아래에 합성 파일을 만들며 원본/CD에는 쓰지 않는다.
std::filesystem::path SerializationCatalogFile() {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data/template-serialization-globals";std::filesystem::create_directories(root);const auto image=SerializationCatalogImage();
    std::ofstream output(root/"shared.fort",std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<const char*>(image.data()),static_cast<std::streamsize>(image.size()));
    if (!output) throw std::runtime_error("직렬화전역검사전용Fort쓰기실패");return root;
}
// 실제파일/State부모를쓰며미정바이트와현재사용자만명시공급한다.
SoloMapCatalogEffects SerializationEffects() {
    SoloMapCatalogEffects effects;effects.typeCount=[] { return 188; };
    effects.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    effects.userName=[] { return std::string("Me"); };effects.translate=[](auto text) { return std::string(text); };return effects;
}
// 의도한물리/하위계약거부를검사실패와구분한다.
template<class Function> bool SerializationRejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// Storage생성자/참조조회는초기화하지않고실제Image생성자의원문0쓰기가같은DWORD에도달한다.
TEST_CASE(TemplateSerializationGlobals_SharedAddressAndManagerConstruction) {
    static_assert(sizeof(FortSerializationGlobals)==sizeof(std::uint32_t)); // 실제소유상태는DWORD한개다.
    auto globals=std::make_shared<FortSerializationGlobals>();globals->version=0x89abcdefU;CampaignTemplateStorage first(globals),second(globals);
    auto& firstVersion=first.SerializationVersion();auto& secondVersion=second.SerializationVersion();
    CHECK(&firstVersion==&globals->version && &secondVersion==&globals->version && firstVersion==0x89abcdefU);
    first.Install(0,FortTemplate{},"first");CHECK(globals->version==0 && secondVersion==0);
    secondVersion=2;second.Install(1,FortTemplate{},"second");CHECK(firstVersion==0 && secondVersion==0);
    globals->version=0xffffffffU;const auto image=SerializationCatalogImage();first.InstallFile(2,FortTemplate{},image,"shared.fort",188);
    CHECK(firstVersion==0 && &first.SerializationVersion()==&second.SerializationVersion());
    secondVersion=0x12345678U;CHECK(SerializationRejects([&] { second.InstallFile(2,FortTemplate{},{},"bad.fort",188); }));CHECK(firstVersion==0); // 파일거부뒤에도이미수행한생성자쓰기를되돌리지않는다.
    globals->version=77;std::weak_ptr<FortSerializationGlobals> lifetime=globals;globals.reset();CHECK(!lifetime.expired() && firstVersion==77); // 외부소유자가놓아도두Storage의실제공유객체주소는남는다.
}

// Create/쓰기/Rename/빌림/반납은manager생성을추가하지않고기존377노드수명을보존한다.
TEST_CASE(TemplateSerializationGlobals_SectionsAndClearPreserveVersion) {
    auto globals=std::make_shared<FortSerializationGlobals>();CampaignTemplateStorage storage(globals);storage.Install(0,FortTemplate{},"source");globals->version=2;
    const std::array<std::uint8_t,2> bytes{7,9};const auto temp=storage.CreateSection(0,"Temp");storage.WriteSection(temp,bytes);CHECK(globals->version==2);
    const auto old=storage.CreateSection(0,"Terr00");storage.WriteSection(old,bytes);CHECK(globals->version==2);
    const auto renamed=storage.RenameSection(0,"Terr00","Temp");CHECK(renamed.manager==temp.manager && renamed.section==temp.section && renamed.generation==temp.generation && globals->version==2);
    // 다음노드생성은원본대로이전tail할당을2BYTE로압축한다. 실제선택의pos0뒤같은옛참조로덮어써추가할당을요구하지않는다.
    const auto selected=storage.SelectSection(0,"Terr00",1);CHECK(selected.section==temp.section && selected.generation==temp.generation && globals->version==2);
    storage.WriteSection(temp,bytes);CHECK(globals->version==2 && SerializationRejects([&] { storage.WriteSection(old,bytes); }));
    storage.Borrow(8,0);CHECK(globals->version==2);storage.Clear(8);CHECK(globals->version==2 && storage.OwnedImages()==1);storage.ClearAll();CHECK(globals->version==2 && storage.OwnedImages()==0);
    storage.Clear(0);CHECK(globals->version==2);storage.ClearAll();CHECK(globals->version==2);
    { CampaignTemplateStorage nested(globals);CHECK(globals->version==2);nested.Install(0,FortTemplate{},"nested");globals->version=3; } // 실제소멸/Storage반납도현재버전을임의0으로만들지않는다.
    CHECK(globals->version==3);
}

// 분류와목록메타의두실제Image생성자가같은ClientDWORD를각각0으로쓰는시점을관측한다.
TEST_CASE(TemplateSerializationGlobals_CatalogBothTemporaryManagers) {
    const auto root=SerializationCatalogFile();BaseFileSystem files(root);ConfigInterface config({});config.Set("fortSpec","shared.fort");
    auto globals=std::make_shared<FortSerializationGlobals>();CampaignTemplateStorage client(globals);auto& version=client.SerializationVersion();version=0x11111111U;
    std::vector<std::uint32_t> paths,reads,users;auto effects=SerializationEffects();effects.serializationGlobals=globals;
    effects.fileSpecKey=[&] {
        paths.push_back(version);if (paths.size()==3) { CHECK(version==0);version=0x33333333U; }return std::string_view("fortSpec"); // 메타Load직전에분류생성자의0을관측하고새표식을쓴다.
    };
    effects.typeCount=[&] {
        reads.push_back(version);version=reads.size()==1 ? 0x22222222U : 0x44444444U;return 188; // 실제manager생성자직전마다다른표식을써서각0쓰기누락을드러낸다.
    };
    effects.userName=[&] { users.push_back(version);version=0x55555555U;return std::string("Me"); }; // 두번째파일/State/Subscriber읽기뒤같은DWORD를직접본다.
    SoloMapCatalog catalog(files,config,std::move(effects));CHECK(version==0x11111111U);CHECK(catalog.Refresh(2)==1);
    CHECK(paths==std::vector<std::uint32_t>({0x11111111U,0x11111111U,0}));CHECK(reads==std::vector<std::uint32_t>({0x11111111U,0x33333333U}));
    CHECK(users==std::vector<std::uint32_t>{0} && version==0x55555555U && &version==&globals->version);
    CHECK(catalog.Refresh(0)==1 && version==0x55555555U && reads.size()==2); // 캐시재사용은새manager생성을하지않는다.
    CHECK(catalog.Kind("shared")==2 && version==0 && reads.size()==3); // 직접분류도같은전역을초기화한다.
}

// 독립Storage/Client와기본catalog의전역은서로의manager생성으로간섭하지않는다.
TEST_CASE(TemplateSerializationGlobals_SeparateClientsAndDefaultOwners) {
    CampaignTemplateStorage first,second;auto& firstVersion=first.SerializationVersion();auto& secondVersion=second.SerializationVersion();CHECK(&firstVersion!=&secondVersion);
    firstVersion=7;secondVersion=9;first.Install(0,FortTemplate{},"first");CHECK(firstVersion==0 && secondVersion==9);
    firstVersion=8;second.Install(0,FortTemplate{},"second");CHECK(firstVersion==8 && secondVersion==0);
    const auto root=SerializationCatalogFile();BaseFileSystem files(root);ConfigInterface config({});config.Set("fortSpec","shared.fort");
    auto globalsA=std::make_shared<FortSerializationGlobals>(),globalsB=std::make_shared<FortSerializationGlobals>();CampaignTemplateStorage clientA(globalsA),clientB(globalsB);
    auto effects=SerializationEffects();effects.serializationGlobals=globalsA;SoloMapCatalog catalogA(files,config,std::move(effects));globalsA->version=31;globalsB->version=41;
    CHECK(catalogA.Kind("shared")==2 && clientA.SerializationVersion()==0 && clientB.SerializationVersion()==41);
    clientA.SerializationVersion()=51;SoloMapCatalog independent(files,config,SerializationEffects());CHECK(clientA.SerializationVersion()==51 && independent.Kind("shared")==2);
    CHECK(clientA.SerializationVersion()==51 && clientB.SerializationVersion()==41 && independent.Refresh(2)==1);CHECK(clientA.SerializationVersion()==51 && clientB.SerializationVersion()==41);
    globalsA->version=61;ConfigInterface missing({});missing.Set("fortSpec","missing.fort");auto missingEffects=SerializationEffects();missingEffects.serializationGlobals=globalsA;SoloMapCatalog absent(files,missing,std::move(missingEffects));
    CHECK(SerializationRejects([&] { absent.Kind("missing"); }) && globalsA->version==61); // 실제파일이없어Image생성자에미도달한임시Storage는0쓰기를추가하지않는다.
}
