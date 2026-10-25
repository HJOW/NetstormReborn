// Session접합의검사초안이다.실제Chat/CoreWrite부모가없는경계를성공stub으로통과시키지않는다.
#include "DudeSpawnSupport.h"
#include "client/CampaignClientServices.h"
#include "client/CampaignTemplateStorage.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientFortGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientStaticStrings.h"
#include "client/FortCurrentNameDevices.h"
#include "client/FortSelection.h"
#include "client/EditorFortFileDevices.h"
#include "client/FortTypeConversionStorage.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawDataManagerSelection.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //Win32의min/max매크로가표준호출을바꾸지않게한다.
#endif
#include <windows.h>
#endif
using namespace netstorm::client;
namespace {
// 미복원실제장치의명시중단을일반실패나전체완료와구분한다.
struct CheckpointStop : std::runtime_error { using std::runtime_error::runtime_error; }; //상위검사가선택한정확한경계문자열만기록한다.
// 현재일반계약거부만인정하고예상밖예외를숨기지않는다.
template<class Exception,class Operation> bool CheckpointRejects(Operation operation) { try { operation(); }catch (const Exception&) { return true; }return false; }
// 입력파일의고정길이레코드만따라원문section위치를찾는다.제품파서를기대생성기로쓰지않는다.
std::size_t CheckpointSection(const std::vector<std::uint8_t>& image,std::size_t index) {
    std::size_t position=2;
    for (std::size_t current=0;current<=index;++current) { //길이WORD자체를포함한실제레코드폭을따른다.
        if (position+2>image.size()) throw std::out_of_range("checkpoint입력section길이");const auto length=std::size_t(image[position])|(std::size_t(image[position+1])<<8);
        if (length<2 || length>image.size()-position) throw std::out_of_range("checkpoint입력payload길이");if (current==index) return position+2;position+=length;
    }throw std::logic_error("checkpoint입력section없음");
}
// 보호원문에서복사한입력의Subscriber ID/State bit만명시변경하며원본파일에는쓰지않는다.
std::vector<std::uint8_t> CheckpointInput(const BaseFileSystem& files) {
    auto image=files.Read("d/thewarbegins.fort");const auto subscriber=CheckpointSection(image,0),state=CheckpointSection(image,1);
    if (subscriber+4>image.size() || state+28>image.size()) throw std::out_of_range("checkpoint명시입력폭");image[subscriber]=7;image[subscriber+1]=image[subscriber+2]=image[subscriber+3]=0;image[state]&=0xfeU;return image;
}
// 매검사소유의새작업경로만사용하며원본/CD/공유build에는쓰지않는다.
std::filesystem::path CheckpointRoot(const std::filesystem::path& root,std::string_view name) { return root/"extracted/cpp-editor-checkpoint-session-20261024-proposed/test-data"/(std::string(name)+'-'+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); }
// 실제Session에선택된장치만공급한다.게임계산/Chat/CoreWrite성공을대신하지않는다.
struct CheckpointDevices {
    SoundList names{OriginalEdition::Patch1078}; //Session보다오래살아실제이름표를빌린다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; //선택된번역의명시BYTE입력이다.
    std::uint32_t camera{},production{},titles{},configs{},chatCalls{},spRefreshes{}; //실제외부호출의순서/부분도달만관측한다.
    std::string current{"Checkpoint385"},pending{"Checkpoint385"}; //검사외부이름입력이다.제품의Client owner를생성했다고주장하지않는다.
    std::string key{"fortSpec"}; //현재FileSpec입력이다.장치선택은아래같은객체를빌린다.
    std::size_t fileSpecSelections{};std::vector<std::uint32_t> assertions; //진단/선택은부분실패의실제효과만기록한다.
    std::string lastConfigKey; //외곽이실제Config에전달한원래한정키이며저장객체의접두어해석과구별한다.
    std::unique_ptr<EditorFortFileDevices> fileDevices; //Client/Config/EditorFiles보다먼저반납할파일장치scratch다.
    RawCampaignSessionServices Make(Client& client,const std::shared_ptr<CampaignTemplateStorage>& storage,const std::vector<std::uint8_t>& input,bool defer=true) {
        names.Initialize();RawCampaignSessionServices result;result.installMode=CampaignInstallMode::ExistingEditor;result.deferExistingEditorFile=defer;result.templateStorage=storage;result.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(input);
        result.randomGlobals=client.RandomGlobals();result.fortBadges=client.FortBadges();result.fortTypeConversion=client.FortTypeConversion();result.soundNames=&names;result.subscriber=MakeCampaignSubscriberClientServices(client);
        result.unixSeconds=[] { return 946684800; };result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; //실제미정할당을명시입력으로공급한다.
        result.production.effects.configInt=[&client](auto name) { return client.Configuration().GetInt(name); };result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; }; //GUI없는소리장치의명시입력이며규칙결과를만들지않는다.
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[&client](auto name,auto bytes) { const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});client.Configuration().Set(name,DecodeOriginalText(bytes.first(static_cast<std::size_t>(end-bytes.begin())))); };
        result.install.refreshWindowTitle=[this] { ++titles; };result.install.resetCamera=[this] { ++camera; };result.ownership.refreshProduction=[this] { ++production; };result.ownership.themeMode=[] { return 0U; }; //이검사는실제화면완료를주장하지않으며발화관측장치만둔다.
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](auto text) { return std::string(text); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw CheckpointStop("선택되지않아야하는Gump"); };result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw CheckpointStop("선택되지않아야하는삭제"); };
        result.refreshSp=[this] { ++spRefreshes; };result.fortTechnologyUi=[this] { ++chatCalls;throw CheckpointStop("actual-chat-owner-required"); }; //0042f540은Chat랭크prefix다.상수0/빈갱신성공으로World를통과시키지않는다.
        result.player.log=[](auto level,auto text) { if (level!=29 || text!="Territory recovered from Battle!\n") throw CheckpointStop("예상밖실제로그"); };
        FortMissionSaveHooks mission;mission.current=[] { return 0U; };result.fortMission=std::move(mission);return result;
    }
    CampaignExistingEditorCheckpointServices Effects(Client& client,EditorFiles& work) {
        client.Configuration().Set("fortSpec","d/{local.1}.fort");client.Configuration().SetInt("startupStateCheck",0);client.Configuration().SetInt("readOnlyForts",0);
        EditorFortPathEffects paths;paths.currentName=[this]()->std::string_view { return current; };paths.selectFileSpec=[this] { ++fileSpecSelections; };paths.fileSpecKey=[this]()->std::string_view { return key; };paths.assertion=[this](auto line) { assertions.push_back(line); };
        fileDevices=std::make_unique<EditorFortFileDevices>(work,client.Files(),client.Configuration(),std::move(paths));CampaignExistingEditorCheckpointServices result;
        result.fort.currentName=[this]()->std::string_view { return current; };result.fort.pendingName=[this]()->std::string_view { return pending; };result.fort.compare=[](auto first,auto second) { return CompareDataManagerSectionNames(first,second); };
        result.fort.setName=[](auto) { throw CheckpointStop("검사에서선택하지않은이름변경"); };result.fort.closeGuide=[] { throw CheckpointStop("검사에서선택하지않은Guide종료"); };result.fort.missionName=[](auto)->std::int32_t { throw CheckpointStop("검사에서선택하지않은미션분류"); };
        result.fort.setConfig=[this,&client](auto name,auto value) { ++configs;lastConfigKey=name;client.Configuration().Set(name,DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})); };
        result.fort.coreWrite=[](auto) { throw CheckpointStop("actual-core-write-required"); }; //원문CoreWrite부모미복원경계를성공stub으로채우지않는다.
        result.fort.outside=[](auto) { throw CheckpointStop("원문다른설치분기"); };result.file.path=[this] { return fileDevices->CurrentPath(); };result.file.exists=[this](auto path) { return fileDevices->Exists(path); };result.file.title=[this] { ++titles; };result.file.assertion=[this](auto line) { assertions.push_back(line); };result.file.outside=[](auto) { throw CheckpointStop("원문다른파일분기"); };
        TemplateFileLoadHooks load;load.wrongName=[] { throw CheckpointStop("예상밖파일명불일치"); };load.tooManyTypes=[] { throw CheckpointStop("예상밖TYPE진단"); };load.rejectFile=[](auto) { throw CheckpointStop("실제작업파일삭제장치필요"); };result.load=fileDevices->BindLoad(std::move(load));
        result.startup.configInt=[&client](auto name) { return client.Configuration().GetInt(name); }; //reader772를가짜0으로두지않는다.옵션0만이실제미선택분기다.
        return result;
    }
};
}

// 지연true는이미있는owner/cursor/version을ctor에서Clear/Install하지않으며요청입력수명도유지한다.
TEST_CASE(CampaignExistingEditorCheckpoint_DeferPreservesExistingOwnerAndDefaultStages) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();
    storage->InstallFile(1,FortTemplate::Parse(input,assets.game.TypeTable()),input,"kept.fort");storage->State(1).position=1;const auto before=std::vector<std::uint8_t>(storage->Headers().begin(),storage->Headers().end());storage->SerializationVersion()=77;CheckpointDevices devices;auto services=devices.Make(client,storage,input);std::weak_ptr<const std::vector<std::uint8_t>> request=services.missionFortImage;
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));CHECK(storage->OwnedImages()==1 && storage->Name(1)=="kept.fort" && storage->State(1).position==1 && storage->SerializationVersion()==77 && !request.expired());CHECK(std::equal(before.begin(),before.end(),storage->Headers().begin()) && !session.ExistingEditorCheckpointReady());
    CHECK(CheckpointRejects<std::logic_error>([&] { session.LoadExistingEditorSubscriber(); }) && CheckpointRejects<std::logic_error>([&] { session.LoadExistingEditorWorld(); }));CHECK(!session.CurrentTechnologyParent());
    auto eagerStorage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices eager;auto old=eager.Make(client,eagerStorage,input,false);RawCampaignSession legacy(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"legacy.fort",std::move(old));CHECK(eagerStorage->OwnedImages()==1 && eagerStorage->Name(1)=="legacy.fort");legacy.LoadExistingEditorSubscriber();CHECK(legacy.CurrentSubscriber()==7 && !legacy.ExistingEditorCheckpointReady()); //기존false의단계별API는계속동작한다.
}

// Campaign은새지연모드로묵시변경되지않으며invalid입력은파일설치전거부된다.
TEST_CASE(CampaignExistingEditorCheckpoint_DeferRejectsCampaignMode) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;auto services=devices.Make(client,storage,input);services.installMode=CampaignInstallMode::Campaign;
    CHECK(CheckpointRejects<std::invalid_argument>([&] { RawCampaignSession bad(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"bad.fort",std::move(services)); }));CHECK(!storage->OwnedImages());
}

// 실제exists/readonly뒤read의정상false는원문ptr정리후InvalidFile분기로가며World/Title/State에가지않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_NormalReadFalseKeepsInvalidBranch) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles work(CheckpointRoot(assets.root.parent_path(),"false"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);bool invalid{},read{};
    effects.load.read=[&](auto) { read=true;CHECK(storage->OwnedImages()==1 && ExistingEditorTemplateValid(*storage,1));return std::vector<std::uint8_t>{}; };effects.file.outside=[&](auto branch) { invalid=branch==ExistingEditorInstallOutside::InvalidFile;throw CheckpointStop("invalid-file"); };
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));CHECK(CheckpointRejects<CheckpointStop>([&] { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); }));CHECK(read && invalid && !storage->OwnedImages() && !devices.titles && !devices.configs && !session.CurrentTechnologyParent() && !session.ExistingEditorCheckpointReady());
    CHECK(CheckpointRejects<std::logic_error>([&] { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); }) && CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
}

// 첫rawTitle뒤실제OS쓰기거부는State|1/이미지생성의부분효과를남겨첫Notify저장연결을검증한다.
TEST_CASE(CampaignExistingEditorCheckpoint_FirstNotifyUsesActualWriterBeforeSubscriber) {
#ifdef _WIN32
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles work(CheckpointRoot(assets.root.parent_path(),"notify"),{assets.root,assets.root.parent_path()/"originalCD"});const auto path=work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);
    HANDLE held=INVALID_HANDLE_VALUE;effects.file.title=[&] { ++devices.titles;held=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if (held==INVALID_HANDLE_VALUE) throw std::runtime_error("검사소유파일읽기공유lock실패"); }; //같은작업파일만잠그며보호자료에는장치를열지않는다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));const auto rejected=CheckpointRejects<std::runtime_error>([&] { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); });if (held!=INVALID_HANDLE_VALUE) CloseHandle(held);
    CHECK(rejected && devices.titles==1 && !devices.configs && storage->OwnedImages()==1 && (storage->State(1).bytes[0]&1U) && storage->CurrentSection(1)=="Terr19");CHECK(ReadFileBytes(path)==input && !session.CurrentTechnologyParent() && !session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
#else
    throw std::logic_error("실제Windows파일공유거부장치가필요한검사초안");
#endif
}

// 원문File전체꼬리는실제Storage/ID/Config/두Title로끝나고World의실제Chat장치부재에서부분중단한다.
TEST_CASE(CampaignExistingEditorCheckpoint_FileTailThenActualWorldChatBoundary) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles work(CheckpointRoot(assets.root.parent_path(),"chat"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));bool stopped{};try { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="actual-chat-owner-required"; }
    //이검사의두등록Config는모두무명이다.기존원문852~855행처럼Set은접두어를떼지만Get은원래키를찾으므로실제저장키를대조한다.
    const auto configured=client.Configuration().Configuration().Get("fort.mySubHandle"); //실제 장기 fort 객체에 저장된 한정 키를 루트 조회로 읽는다.
    const bool fileTailMatches=stopped && devices.chatCalls==1 && devices.titles==2 && devices.configs==1 && session.CurrentSubscriber()==7 && devices.lastConfigKey=="fort.mySubHandle" && configured && *configured==std::string(session.CurrentHandle()); //원래 전달 키와 실제 fort 저장값을 함께 대조한다.
    if (!fileTailMatches) std::fprintf(stderr,"Editor checkpoint stopped=%u chat=%u title=%u config=%u subscriber=%u configured=%s handle=%s\n",stopped?1U:0U,devices.chatCalls,devices.titles,devices.configs,session.CurrentSubscriber(),configured?configured->c_str():"<missing>",std::string(session.CurrentHandle()).c_str());
    CHECK(fileTailMatches);CHECK(storage->OwnedImages()==1 && session.CurrentTechnologyParent() && !devices.camera && !session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.LoadExistingEditorWorld(); }) && CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
}

// 실제Editor의post/editor자연관문은seed/refresh/SP없는현재Session에서도선택하위를요구하지않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_CoreWriteNaturalGateKeepsActualStorage) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();storage->InstallFile(1,FortTemplate::Parse(input,assets.game.TypeTable()),input,"kept.fort");CheckpointDevices devices;
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));const auto current=storage->CurrentSection(1);const auto headers=std::vector<std::uint8_t>(storage->Headers().begin(),storage->Headers().end());const auto core=storage->Template(1).Section("CoreData");const auto bytes=std::vector<std::uint8_t>(core.begin(),core.end());const auto players=session.PlayerRecords();const auto playerBytes=std::vector<std::uint8_t>(players.begin(),players.end());const auto random=client.RandomGlobals()->Game().State();
    session.WriteExistingEditorCoreData(1);const auto unchanged=storage->Template(1).Section("CoreData");CHECK(storage->CurrentSection(1)==current && std::equal(headers.begin(),headers.end(),storage->Headers().begin()) && std::equal(bytes.begin(),bytes.end(),unchanged.begin(),unchanged.end()));CHECK(std::equal(playerBytes.begin(),playerBytes.end(),session.PlayerRecords().begin()) && client.RandomGlobals()->Game().State()==random && !session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
}
// manager0도Editor자연ret보다먼저원문45f진단을선택한다.미공급진단은그위치에서명시중단한다.
TEST_CASE(CampaignExistingEditorCheckpoint_CoreWriteManagerDiagnosticBeforeGate) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));bool selected{};
    try { session.WriteExistingEditorCoreData(1); }catch (const std::logic_error& error) { selected=std::string_view(error.what()).ends_with(":1119"); }CHECK(selected && !storage->OwnedImages() && !session.ExistingEditorCheckpointReady());
}
// 같은현재challenge/Metamap을읽어460진단을선택하고그뒤자동정상복귀나loaded확정을하지않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_CoreWriteMetamapDiagnosticBeforeGate) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();storage->InstallFile(1,FortTemplate::Parse(input,assets.game.TypeTable()),input,"kept.fort");CheckpointDevices devices;RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",devices.Make(client,storage,input));session.Core().ownerMode.challenge=true;bool selected{};
    try { session.WriteExistingEditorCoreData(1); }catch (const std::logic_error& error) { selected=std::string_view(error.what()).ends_with(":1120"); }CHECK(selected && storage->OwnedImages()==1 && !session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
}

// 현재/pending가 다른 실제 Client 이름 부모를 먼저 거쳐 같은 작업 파일/Subscriber/World 경계까지 설치한다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActualCurrentNameSelectsWorkingFile) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"actual-name"),{assets.root,assets.root.parent_path()/"originalCD"});const std::string selected="Checkpoint419";const auto actual=work.Prepare(selected,input);CheckpointDevices devices;auto effects=devices.Effects(client,work);auto& names=*client.FortGlobals();
    SetPendingFortName(names.PendingBytes(),{reinterpret_cast<const std::uint8_t*>(selected.c_str()),selected.size()+1});
    auto services=devices.Make(client,storage,input);services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services)); // 설치 전 메뉴를 열지 않고 같은 Client의 실제 현재 전역/Subscriber/Player를 함께 빌린다.
    auto hooks=session.BindFortCurrentName({});hooks.assertion=[&](auto line) { devices.assertions.push_back(line); };hooks.refreshTitle=[&] { ++devices.titles; };
    CHECK(CheckpointRejects<std::logic_error>([&] { hooks.snapshot(); }) && CheckpointRejects<std::logic_error>([&] { hooks.notify(session.TemplateOwner()); })); // 실제 외곽 전에는 공개 저장/Notify를 우회하지 않는다.
    FortCurrentNameDevices current(names,*storage,client.Configuration(),{[&] { return session.TemplateOwner(); },[&] { return session.Core().ownerMode.fort ? 1U : 0U; },[&] { return client.GameplayGlobals()->world.editor; },[&] { return session.Core().bridgeState.mission ? 1U : 0U; },names.Catalog().dirty},std::move(hooks),&work);
    effects.fort.currentName=[&] { return names.Current(); };effects.fort.pendingName=[&] { return names.Pending(); };effects.fort.setName=[&](auto name) { const std::string source(name);current.Set({reinterpret_cast<const std::uint8_t*>(source.c_str()),source.size()+1}); };
    EditorFortPathEffects paths;paths.currentName=[&] { return names.Current(); };paths.selectFileSpec=[&] { names.SelectFileSpec(); };paths.fileSpecKey=[&] { return names.SelectedFileSpec(); };paths.assertion=[&](auto line) { devices.assertions.push_back(line); };
    devices.fileDevices=std::make_unique<EditorFortFileDevices>(work,client.Files(),client.Configuration(),std::move(paths));effects.load=devices.fileDevices->BindLoad(std::move(effects.load));
    bool stopped{};try { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="actual-chat-owner-required"; }
    CHECK(stopped && names.Current()==selected && names.Pending()==selected && names.Catalog().dirty==2 && client.Configuration().Configuration().Get("lastEditFort")==selected);CHECK(ReadFileBytes(storage->Name(session.TemplateOwner()))==ReadFileBytes(actual) && session.CurrentSubscriber()==7 && devices.titles==3 && devices.assertions.empty());CHECK(!session.ExistingEditorCheckpointReady() && CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
}

// 실제 비영 Chat 시작과 active0 자연 관문으로 종전 중단 위치를 지나 전체 저장/State 통지 뒤 다음 UI 경계를 확인한다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActualChatRuntimeAdvancesWorldAndSnapshot) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));InitializeClientChatManager(client);
    CHECK(client.ChatManagerGlobals()->Current()!=0 && client.CoreDataGlobals()->Active()==0);auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles work(CheckpointRoot(assets.root.parent_path(),"chat-runtime"),{assets.root,assets.root.parent_path()/"originalCD"});const auto path=work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);auto services=devices.Make(client,storage,input);
    const auto surface=MakeRawSurfaceStorage(assets.game);services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();
    services.fortTechnologyUi=[&] { ++devices.chatCalls;RefreshClientChatRankPrefix(client,assets.game,*surface); }; // 동기 실제 부모가 current/active/Core/TYPE를 판정한다.
    effects.players.translate=[](auto)->std::span<const std::uint8_t> { throw CheckpointStop("actual-editor-player-translation-required"); }; // 이 검사는 다음 미연결 UI 경계를 완료로 세지 않는다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));bool stopped{};try { session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="actual-editor-player-translation-required"; }
    if (!(stopped && devices.chatCalls==1 && devices.camera==1 && devices.production==5 && session.CurrentTechnologyParent()!=0)) std::fprintf(stderr,"Actual Chat checkpoint stopped=%u chat=%u camera=%u production=%u technology=%u\n",stopped?1U:0U,devices.chatCalls,devices.camera,devices.production,session.CurrentTechnologyParent()); // 실제 하위 통지까지 포함한 부분 효과가 다른 경우 진단한다.
    CHECK(stopped && devices.chatCalls==1 && devices.camera==1 && devices.production==5 && session.CurrentTechnologyParent()!=0);CHECK(!(storage->State(session.TemplateOwner()).bytes[0]&1U) && ReadFileBytes(path)!=input && !session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // 이 입력의 전체 World/저장 하위도 같은 생산 장치에 통지하므로 외곽 직접 호출1회와 총횟수5를 구별한다.
}

// 실제 번역/공유 고리와 전체 게임 후처리를 사용해 checkpoint 정상 복귀와 별도 Session의 저장본 재설치를 확인한다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActualPlayerUiCompletesAndReloads) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);InitializeClientChatManager(client);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"player-ui-roundtrip"),{assets.root,assets.root.parent_path()/"originalCD"});const auto path=work.Prepare("Checkpoint385",input);const auto shared=client.StaticStrings();std::size_t previous{};
    for (std::uint32_t pass=0;pass<2;++pass) { // 첫 정상 저장의 실제 디스크 바이트를 별도 두 번째 Session 입력으로 다시 읽는다.
        input=ReadFileBytes(path);CheckpointDevices devices;auto storage=std::make_shared<CampaignTemplateStorage>();auto effects=devices.Effects(client,work);auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game);
        services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { ++devices.chatCalls;RefreshClientChatRankPrefix(client,assets.game,*surface); }; // 같은 실제 Client 장치를 공급하고 게임 child는 Session의 본체를 사용한다.
        RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work);
        CHECK(session.ExistingEditorCheckpointReady() && session.CurrentHandle()=="Player" && session.CurrentSubscriber()==7 && devices.chatCalls==1 && devices.camera==1);const auto players=session.PlayerRecords();
        for (std::uint32_t owner=1;owner<9;++owner) { // 같은 실제 Player 이름과 기술 부모를 읽어 새 EditorPlayers 설치 결과를 확인한다.
            const auto bytes=players.subspan(owner*180+12);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});CHECK(end!=bytes.end());const std::string name(reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin()));CHECK(name==(owner==1?"Human Player":"Computer #"+std::to_string(owner-1)));
            std::uint32_t parent{};for (std::uint32_t byte=0;byte<4;++byte) parent|=std::uint32_t(players[owner*180+0x9c+byte])<<(8*byte);CHECK(parent!=0); // 슬롯별 실제 생성 부모이며 미리 계산한 가짜SID가 아니다.
        }
        CHECK(!(storage->State(session.TemplateOwner()).bytes[0]&1U) && client.StaticStrings()==shared && shared->Globals().size==10240);const auto position=shared->Globals().current-shared->Globals().base;CHECK(position>previous);previous=position;
        const auto saved=ReadFileBytes(path);const auto restored=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(!restored.Section("State").empty() && !(restored.Section("State")[0]&1U));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // checkpoint 정상 복귀는 최종 HUD/State/loaded 공개와 구별한다.
    }
}

// 보호 커스텀 TEST01의 바이트는 편집하지 않고 별도 작업 namespace에서 같은 실제 설치/저장을 실행한다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActualCustomTest01PlayerUiAndFile) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=assets.files.Read("d/TEST01.fort");ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);InitializeClientChatManager(client);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"custom-test01-player-ui"),{assets.root,assets.root.parent_path()/"originalCD"});CheckpointDevices devices;devices.current=devices.pending="CustomTest01421";const auto path=work.Prepare(devices.current,input);const auto effects=devices.Effects(client,work);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game);
    services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { ++devices.chatCalls;RefreshClientChatRankPrefix(client,assets.game,*surface); };
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work);
    CHECK(session.ExistingEditorCheckpointReady() && session.CurrentHandle()=="Player" && session.CurrentSubscriber()!=0 && devices.chatCalls==1 && session.IslandCount()>0);const auto saved=ReadFileBytes(path);const auto result=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(!result.Section("State").empty() && !(result.Section("State")[0]&1U));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // 실제 커스텀 checkpoint가 끝나도 최종 loaded는 이 부모 밖이다.
}
