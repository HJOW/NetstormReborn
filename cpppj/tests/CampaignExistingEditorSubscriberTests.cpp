// 제안단계검사다.실제Client/Session/Storage/CRT를사용하며전체Editor설치성공으로표현하지않는다.
#include "DudeSpawnSupport.h"
#include "client/CampaignClientServices.h"
#include "client/CampaignTemplateStorage.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/FortTypeConversionStorage.h"
#include "o/RawFortBadgesGlobals.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
using namespace netstorm::client;
namespace {
// 현재실제fields의little endian DWORD만읽는다.
std::uint32_t SubscriberSessionU(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //섹션의현재pos/end를별도로관측한다.
// 원본파일대신같은작업manager의실제Subscriber원문만편집한다.
void SubscriberSessionInput(CampaignTemplateStorage& storage,std::uint32_t identity,std::string_view name) {
    if (name.size()>63) throw std::out_of_range("Subscriber접합검사명시이름물리범위");std::vector<std::uint8_t> bytes; //공유제품의parsedSubscriber를갱신하지않는원시입력이다.
    for (std::uint32_t byte=0;byte<4;++byte) bytes.push_back(static_cast<std::uint8_t>(identity>>(byte*8))); //파일ID의원문DWORD폭이다.
    bytes.push_back(static_cast<std::uint8_t>(name.size()));bytes.push_back(0);bytes.insert(bytes.end(),name.begin(),name.end());const auto stream=storage.CreateSection(1,"Subscriber");storage.WriteSection(stream,bytes);
}
// 현재선택/같은반환노드의cursor를실제Storage빌림안에서만읽는다.
void SubscriberSessionCursor(CampaignTemplateStorage& storage,std::uint32_t expected) {
    CHECK(storage.CurrentSection(1)=="Subscriber");const auto current=storage.CurrentWriteStream(1); //별도선택으로pos를초기화하지않는다.
    storage.AccessSection(current,[&](auto fields,const auto&) { CHECK(SubscriberSessionU(fields,4)==expected && SubscriberSessionU(fields,8)==expected); });
}
// 실제Session생성에선택된외부장치만공급한다.World/메뉴/파일외곽은실행하지않는다.
struct SubscriberSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; //Session보다먼저만들고늦게반납하는실제소리이름표다.
    RawCampaignSessionServices Make(Client& client,const std::shared_ptr<CampaignTemplateStorage>& storage,const std::vector<std::uint8_t>& original) {
        names.Initialize();RawCampaignSessionServices result; //미선택등록설정/clock/log는기본누락그대로둔다.
        result.installMode=CampaignInstallMode::ExistingEditor;result.templateStorage=storage;result.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(original);
        result.randomGlobals=client.RandomGlobals();result.fortBadges=client.FortBadges();result.fortTypeConversion=client.FortTypeConversion();result.soundNames=&names;
        result.unixSeconds=[] { return 946684800; }; //기존게임시드생성자의필수시각장치이며Subscriber전용clock은아직없다.
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; //실제미초기화Deck할당에대한명시외부입력이다.
        result.production.effects.configInt=[](std::string_view) { return 0; };result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };return result;
    }
};
// 실제명시누락계약만실패로인정한다.
template<class Operation> bool SubscriberSessionRejects(Operation operation) { try { operation(); }catch (const std::logic_error&) { return true; }return false; }
}

// 정상파일ID는등록설정/clock/log를미리요구하지않고실제바뀐원문과같은cursor만소비한다.
TEST_CASE(CampaignExistingEditorSubscriber_KnownIdUsesActualStorage) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); //Run/GUI없이실제Client전역만소유한다.
    auto storage=std::make_shared<CampaignTemplateStorage>();SubscriberSessionDevices devices;RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"SubscriberProbe384.fort",devices.Make(client,storage,original));
    SubscriberSessionInput(*storage,7,"RawSubscriber384");const auto state=client.RandomGlobals()->Crt().State(),initialized=client.RandomGlobals()->SubscriberIdInitialized();session.LoadExistingEditorSubscriber();
    CHECK(session.CurrentSubscriber()==7 && session.CurrentHandle()=="RawSubscriber384" && session.CurrentTechnologyParent()==0);SubscriberSessionCursor(*storage,22);CHECK(client.RandomGlobals()->Crt().State()==state && client.RandomGlobals()->SubscriberIdInitialized()==initialized); //World/등록난수는실행되지않았다.
}

// 파일ID-1의실제ReadInt가같은ID참조를갱신하면새ID/clock을선택하지않는다.
TEST_CASE(CampaignExistingEditorSubscriber_ActualClientRegisteredId) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); //실제Client설정callback을교체하지않는다.
    client.Configuration().SetInt("registeredSubId",77);auto storage=std::make_shared<CampaignTemplateStorage>();SubscriberSessionDevices devices;auto services=devices.Make(client,storage,original);services.subscriber=MakeCampaignSubscriberClientServices(client);services.subscriber.clock={}; //미선택clock의선행거부를잡는입력이다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"SubscriberRegistered384.fort",std::move(services));SubscriberSessionInput(*storage,0xffffffffU,"Registered384");const auto state=client.RandomGlobals()->Crt().State(),initialized=client.RandomGlobals()->SubscriberIdInitialized();session.LoadExistingEditorSubscriber();
    CHECK(session.CurrentSubscriber()==77 && session.CurrentHandle()=="Registered384");SubscriberSessionCursor(*storage,19);CHECK(client.RandomGlobals()->Crt().State()==state && client.RandomGlobals()->SubscriberIdInitialized()==initialized);CHECK(!client.Configuration().Configuration().Get("registerdSubId")); //원문오탈자쓰기경로도미선택이다.
}

// 원문1024fixture의actualCRT/init1/state1행을기대로같은Client CRT소비와실제Config오탈자쓰기를대조한다.
TEST_CASE(CampaignExistingEditorSubscriber_ActualSharedCrtAndConfig) {
    constexpr std::uint32_t expectedId=87625992U,expectedCrt=75413397U; //독립native입력0,1,1,0,0,0,1,none의첫반환/현재CRT다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); //제목/렌더링은시작하지않는다.
    client.Configuration().Configuration().RemoveKey("registeredSubId");auto storage=std::make_shared<CampaignTemplateStorage>();SubscriberSessionDevices devices;auto services=devices.Make(client,storage,original);services.subscriber=MakeCampaignSubscriberClientServices(client);services.subscriber.clock={}; //init1은시간/clock을선택하지않아야한다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"SubscriberGenerate384.fort",std::move(services));SubscriberSessionInput(*storage,0xffffffffU,"Generated384");auto random=client.RandomGlobals();random->Crt().SetState(1);random->SubscriberIdInitialized()=1; //같은실제owner에명시원시CRT입력을준다.
    session.LoadExistingEditorSubscriber();CHECK(session.CurrentSubscriber()==expectedId && session.CurrentHandle()=="Generated384");CHECK(random->Crt().State()==expectedCrt && random->SubscriberIdInitialized()==1);CHECK(client.Configuration().GetInt("registerdSubId")==static_cast<std::int32_t>(expectedId));CHECK(!client.Configuration().Configuration().Get("registeredSubId"));SubscriberSessionCursor(*storage,18); //성공시log9누락도요구하지않는다.
}

// 필수등록Config누락은파일ID/이름/cursor를쓴뒤선택된위치에서거부하며rollback하지않는다.
TEST_CASE(CampaignExistingEditorSubscriber_MissingRegistrationPreservesPrefix) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); //실제공유소유자를Session보다오래유지한다.
    auto storage=std::make_shared<CampaignTemplateStorage>();SubscriberSessionDevices devices;RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"SubscriberMissing384.fort",devices.Make(client,storage,original));SubscriberSessionInput(*storage,0xffffffffU,"Partial384");const auto state=client.RandomGlobals()->Crt().State();
    CHECK(SubscriberSessionRejects([&] { session.LoadExistingEditorSubscriber(); }));CHECK(session.CurrentSubscriber()==0xffffffffU && session.CurrentHandle()=="Partial384" && client.RandomGlobals()->Crt().State()==state);SubscriberSessionCursor(*storage,16);
}
