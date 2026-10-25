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
#include "client/ClientPlayerRuntime.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientSpGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ExistingEditorAfterCheckpoint.h"
#include "client/EditorLocalTransport.h"
#include "o/RawPathProcess.h"
#include "o/RawPriestState.h" // 같은 실제 사제의 HP/지면 이동 불가 조회 선언을 직접 사용한다.
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawProductionControls.h"
#include "client/FortCurrentNameDevices.h"
#include "client/FortSelection.h"
#include "client/EditorFortFileDevices.h"
#include "client/FortTypeConversionStorage.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawSunAviaryProcess.h" // 실제 사제 이동 프로세스의 동일 풀/부착 Form 조회 선언을 사용한다.
#include "o/RawDataManagerSelection.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
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

// 실제 앞선 Native 초기화의 같은 Player를 대여하고 Session 반납 뒤에도 Core0 선택 전까지 이전 데이터가 살아 있음을 확인한다.
TEST_CASE(CampaignExistingEditorCheckpoint_AdoptsNativePlayersAndPreservesCoreUntilNextCleanup) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client);InitializeClientChatManager(client);client.StaticStrings()->Initialize(10240);const auto core=client.CoreDataGlobals();const std::uint32_t metamap=0;auto& sharedSp=client.SpGlobals()->Store();
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"adopt-core-lifetime"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);
    for (std::uint32_t pass=0;pass<2;++pass) { // 앞선 Core0/실제 재할당은 Native 호출 위치에서만 수행한다.
        if (pass) { core->Select(client.GameplayGlobals()->player,metamap,0,1);CHECK(core->Cached()[4]==71 && core->Active()==0);InitializeClientPlayerRecords(client); }
        const auto base=client.PlayerRecords()->Base();const auto spBefore=sharedSp.Pools();const auto offset=sharedSp.Offset();const auto tracking=sharedSp.TrackingOffsetCurrent(); // Native 공통 초기화 뒤의 현재 저장소는 Session ctor에서 다시 초기화하지 않는다.
        { CheckpointDevices devices;auto storage=std::make_shared<CampaignTemplateStorage>();auto effects=devices.Effects(client,work);auto services=devices.Make(client,storage,ReadFileBytes(work.ResolvePath("d/Checkpoint385.fort")));const auto surface=MakeRawSurfaceStorage(assets.game);
          services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.coreDataGlobals=core;services.adoptInitializedPlayers=true;services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { RefreshClientChatRankPrefix(client,assets.game,*surface); };
          services.spGlobals=client.SpGlobals();services.randomGlobals=client.RandomGlobals(); // 실제 checkpoint도 공통 시작과 같은 SP/게임·CRT 난수/추적 DWORD를 빌린다.
          RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));CHECK(&session.SpStore()==&sharedSp && sharedSp.Pools()==spBefore && sharedSp.Offset()==offset && sharedSp.TrackingOffsetCurrent()==tracking);CHECK(client.PlayerRecords()->Base()==base && session.PlayerCoreGlobals()==core && core->Current().data()==session.PlayerRecords().data()+180+0x6c);session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work);CHECK(session.ExistingEditorCheckpointReady());core->Current()[4]=71;core->Active()=7;
        }
        CHECK(client.PlayerRecords()->Base()==base && !core->Current().empty() && core->Current()[4]==71 && core->Active()==7); // 소멸자가 임의 Core 선택 해제/캐시 복사를 하지 않는다.
    }
    core->Select(client.GameplayGlobals()->player,metamap,0,1);CHECK(core->Cached()[4]==71 && core->Active()==0); // 다음 Native 정리의 명시 선택만 현재 슬롯을 반납한다.
}

namespace { // 실제 checkpoint/UI scope/State 공통 확정을 연결하며 GUI 없는 장치 입력만 명시한다.
// 실제 Editor 큐의 외부 시계/기록/종료 장치만 제공하고 게임 sender나 처리 결과를 바꾸지 않는다.
struct EditorInstallWireScene {
    bool server{true};std::uint32_t now{100},closed{},released{}; // 아직 Native 공유 서버 DWORD가 없는 명시 장치 입력이며 Session bool로 대체하지 않는다.
    std::shared_ptr<EditorLocalTransport> wire;std::vector<std::uint32_t> senders;std::vector<std::vector<std::uint8_t>> packets; // 실제 큐와 수신 원문/음수 sender의 관측이다.
    bool callbacksReleased{},sessionOwnsRegistration{true}; // 정상 설치의 수신 반납과 기존 다른 소유자 때문에 거부한 표의 보존을 구별한다.
    PacketDispatchHooks Hooks() {
        PacketDispatchHooks hooks;hooks.now=[this] { return ++now; };hooks.record=[this](auto& connection,auto packet,auto) { senders.push_back(connection.sender);packets.emplace_back(packet.begin(),packet.end()); };
        hooks.terminate=[](auto& connection) { connection.flags|=0x40; };hooks.close=[this](auto&) { ++closed; };hooks.release=[this](auto&) { ++released; };return hooks;
    }
};
// 실제 Chat·Player·번역·작업 파일을 갖춘 같은 Client Session을 검사 동기 범위 동안 소유한다.
template<class Body> void WithEditorInstallSession(Body body,bool prepared=false,EditorInstallWireScene* wireScene=nullptr) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);if (prepared) { client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client); }InitializeClientChatManager(client);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"ui-state-commit"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game);
    services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.localStateGlobals=client.LocalStateGlobals();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { RefreshClientChatRankPrefix(client,assets.game,*surface); };
    if (prepared) {
        auto& gameplay=*client.GameplayGlobals();gameplay.world.editor=1;gameplay.world.missionPost=true;gameplay.player.fortMode=true;gameplay.cursor.screenX=37;gameplay.cursor.lockDepth=3;gameplay.energy.userCheat=true; // 준비 대여에 대한 명시 입력이며 Native cleanup/prefix 전체 실행의 증거는 아니다.
        surface->ownerMode={true,false,false};surface->links.battle=false;surface->links.mission=false;surface->spots[123]=19;const std::uint32_t metamap=0;client.CoreDataGlobals()->Select(gameplay.player,metamap,1,1);
        services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.coreDataGlobals=client.CoreDataGlobals();services.spGlobals=client.SpGlobals();services.processKernel=&client.GetKernel(); // 준비된 Native 경로에서 별도 SP 초기화나 별도 Kernel을 만들지 않는다.
    }
    if (wireScene) {
        if (!prepared) throw std::logic_error("검사의 Editor 큐도 준비된 Player 물리를 요구함");wireScene->wire=std::make_shared<EditorLocalTransport>(client.PlayerRecords()->Records(),wireScene->server,wireScene->Hooks());wireScene->wire->Open(6799);services.editorTransport=wireScene->wire;
        services.construction.configInt=[&client](auto name,std::int32_t fallback) { client.Configuration().ReadInt(name,fallback);return fallback; };services.construction.wallSeconds=[] { return 100.0; };services.construction.refresh=[&devices] { ++devices.production; }; // 실제 Client 설정과 원문 배치 뒤 외부 UI 통지를 공급하며 게임 판정을 대신하지 않는다.
    }
    const std::array<std::uint32_t,13> messages{0x5b,0x40,0x4f,0x50,0x60,0x61,0x5c,0x5f,0x7d,0x6d,0x3e,0x54,0x58};std::array<PacketRegistration,13> registrations{}; // 반납 전 현재 등록 정책을 독립적으로 고정한다.
    { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));body(session,client,effects,work);
      if (wireScene) for (std::size_t index=0;index<messages.size();++index) registrations[index]=wireScene->wire->Dispatch().Registration(messages[index]); } // 수신 callback의 같은 Session이 Client/Player보다 먼저 반납된다.
    if (wireScene) {
        for (std::size_t index=0;index<messages.size();++index) { // receive만 반납하고 원문 observer/프로토콜/정책/길이 값은 유지한다.
            const auto& current=wireScene->wire->Dispatch().Registration(messages[index]);CHECK((wireScene->sessionOwnsRegistration ? !current.receive : bool(current.receive)==bool(registrations[index].receive)) && current.version==registrations[index].version && current.mismatch==registrations[index].mismatch && current.length==registrations[index].length && bool(current.observer)==bool(registrations[index].observer));
        }
        wireScene->callbacksReleased=true;wireScene->wire->Disconnect(-1);wireScene->wire->Disconnect(9);wireScene->wire->CloseListener(); // 더 긴 Client 물리가 살아 있는 동안 전송 양끝도 명시 종료한다.
    }
}
// 원문 State가 요구하는 서로 다른 전역을 소유하고 같은 Client의720BYTE/Player를 직접 빌린다.
struct EditorInstallStateContext {
    ClientLocalStateGlobals& owner;PlayerWorldState& player; // Session 서비스와 동일한 두 실제 소유자의 참조다.
    bool server{},boss{true},battle{},post{true},saved{},mission{},simul{},meta{}; // 선택 Editor 이외의 State 분기는 실행하지 않는 명시 입력이다.
    std::uint32_t zone{},ticks{},source{},seed{},metaRefresh{},metaRebuild{}; // 원문 State의 별개 현재 DWORD다.
    std::uint8_t title{},battleByte{};double connectEnd{},seconds{},wall{100.0}; // 활동 기한과 구별한 실제 참조 입력이다.
    explicit EditorInstallStateContext(Client& client,std::uint32_t selected=1):owner(*client.LocalStateGlobals()),player(client.GameplayGlobals()->player) {
        auto& blocks=owner.Blocks();
        for (std::size_t block=0;block<blocks.size();++block) for (std::size_t word=0;word<blocks[block].size();++word) blocks[block][word]=static_cast<std::uint32_t>(block*1000+word); // 전체 부가 인자와 stale 블록까지 독립 입력으로 구별한다.
        blocks[1][0]=99;blocks[2][0]=0;blocks[3][0]=selected;blocks[4][0]=0;
    }
    LocalMissionStateGlobals Globals() { return {server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,owner.Blocks()[2][33],metaRefresh,metaRebuild,title,battleByte,connectEnd,owner.ActivityEnd(),seconds,wall}; } // menuWindow는 원문 immediate+84의 같은 DWORD다.
};
// UI tail의 원문 helper에 현재 Session/벽시각과 명시 GUI 없는 HUD·음악·팔레트 장치를 공급한다.
ExistingEditorAfterCheckpointEffects EditorInstallTail(RawCampaignSession& session,std::uint32_t& calls) {
    ExistingEditorAfterCheckpointEffects effects;effects.hud=[&](auto command) { CHECK(command==0x0200003dU);++calls; };effects.wallSeconds=[&] { ++calls;return 100.5; };effects.requestMusic=[&](auto name) { CHECK(name=="ser22.mus");++calls; };effects.fortPalette=[&] { ++calls; };effects.islandCount=[&] { ++calls;return session.IslandCount(); };return effects;
}
// 실제 checkpoint와 tail을 같은 RawLocalMissionState 부모에서 실행하고 최종 공개는 호출자에게 남긴다.
void RunEditorWireInstallState(RawCampaignSession& session,Client& client,const CampaignExistingEditorCheckpointServices& checkpoint,EditorFiles& work) {
    EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};const auto tail=EditorInstallTail(session,calls);LocalMissionStateHooks hooks;hooks.logState=[](auto) {};
    hooks.parentState=[&](auto selected) { CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);CHECK(CheckpointRejects<std::logic_error>([&] { session.PumpLocalConnection(); }));session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {});return LocalMissionParentResult::Commit; };
    RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();CHECK(calls==5 && scene.owner.Blocks()[1][0]==1 && scene.owner.Blocks()[3][0]==0 && scene.owner.Blocks()[4][0]==4);
}
// 실제 원문 패킷/슬롯의 비정렬DWORD를 관측하고 반환값을 게임 처리기에 되먹이지 않는다.
std::uint32_t EditorWireDword(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Editor 검사 현재 DWORD 범위");return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24); }
}

// 실제 checkpoint가 끝난 설치 scope에서만 필요한 조회를 허용하고 원문 Run의 전체 확정 뒤 공개한다.
TEST_CASE(CampaignExistingEditorCheckpoint_UiScopeCommitsOnlyAfterActualStateReturn) {
    for (const std::uint32_t selected:{1U,2U}) WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) { // 두 정상 선택의 같은 전체144BYTE 확정을 독립 Session으로 검증한다.
        EditorInstallStateContext scene(client,selected);InputQueue input;std::uint32_t calls{};auto tail=EditorInstallTail(session,calls);RawCampaignCursor* cursor{};ClientLocalStateGlobals other;
        std::uint32_t selectedSid{},mouseType{};std::array<std::uint32_t,2> remembered{};std::int32_t multipleCount{};UserSelectionState selection{selectedSid,mouseType,remembered,{},multipleCount}; // 선택 공장의 실제 참조 인자를 제공하고 미완료 공개 거부만 검사한다.
        LocalMissionStateHooks hooks;hooks.logState=[&](auto state) { CHECK(state==selected || state==4); };hooks.parentState=[&](auto state) {
            CHECK(state==selected);CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {}); }));session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);
            CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorInstallUi(other,input,tail,[] {}); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));
            session.WithExistingEditorInstallUi(scene.owner,input,tail,[&] {
                CHECK(scene.owner.Blocks()[1][0]==99);cursor=&session.Cursor();static_cast<void>(session.GumpForms());session.SynchronizeProductionContents();
                CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.MakeUserSelection(selection,{}); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {}); }));
            });
            CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.GumpForms(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));return LocalMissionParentResult::Commit;
        };
        const auto request=scene.owner.Blocks()[3],previous=scene.owner.Blocks()[1];RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();CHECK(calls==5 && scene.owner.Blocks()[0]==previous && scene.owner.Blocks()[1]==request && scene.owner.Blocks()[3][0]==0 && scene.owner.Blocks()[4][0]==4);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(other); }));scene.owner.Blocks()[1][5]^=1;CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));scene.owner.Blocks()[1][5]^=1;
        session.CommitExistingEditorInstall(scene.owner);CHECK(&session.Cursor()==cursor);session.SnapshotFort(1);CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SendLocalPacket(9,{}); }));state.Poll();CHECK(scene.owner.Blocks()[1][0]==4); // 로컬 로그인/전송을 가짜로 만들지 않고 next4는 다음 Poll 한 번에만 확정한다.
    });
}

// tail 중단은 scope를 닫고 부분 효과를 보존하며 loaded나 UI 재준비로 우회하지 못한다.
TEST_CASE(CampaignExistingEditorCheckpoint_UiTailFailureKeepsPublicAccessClosed) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        EditorInstallStateContext scene(client);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);const auto phase=scene.owner.Blocks()[1];bool reached{};InputQueue input;std::uint32_t calls{};auto tail=EditorInstallTail(session,calls);tail.fortPalette=[&] { ++calls;throw CheckpointStop("실제 UI tail 팔레트 중단"); };
        CHECK(CheckpointRejects<CheckpointStop>([&] { session.WithExistingEditorInstallUi(scene.owner,input,tail,[&] { static_cast<void>(session.Cursor());session.SynchronizeProductionContents();reached=true; }); }));
        CHECK(reached && calls==4 && scene.owner.ActivityEnd()==110.5 && scene.owner.Blocks()[1]==phase && scene.owner.Blocks()[4][0]==0);CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {}); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));
    });
}

// 원문 early return과 immediate14의 실제 공통 확정은 정상 Editor 요청1/next4 완료와 구별한다.
TEST_CASE(CampaignExistingEditorCheckpoint_UiTailCannotCommitEarlyReturnOrImmediate) {
    for (const bool immediate:{false,true}) WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) { // 두 다른 반환 결과를 각각 실제 분배기로 실행한다.
        EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};auto tail=EditorInstallTail(session,calls);std::unique_ptr<RawLocalMissionState> state;
        if (immediate) tail.fortPalette=[&] { ++calls;state->Request(14); };LocalMissionStateHooks hooks;hooks.logState=[](auto) {};hooks.parentState=[&](auto selected) {
            CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {});return immediate ? LocalMissionParentResult::Commit : LocalMissionParentResult::ReturnWithoutCommit;
        };
        state=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state->Run();CHECK(scene.owner.Blocks()[1][0]==(immediate ? 14U : 99U));CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
    });
}

// 명시 준비 입력의 같은 Player/Core/커서/spot을 ctor에서 초기화하지 않고 실제 checkpoint/UI까지 빌린다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedCtorPreservesSameClientState) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        const auto& gameplay=*client.GameplayGlobals();CHECK(gameplay.cursor.screenX==37 && gameplay.cursor.lockDepth==3 && gameplay.energy.userCheat);CHECK(session.Core().ownerMode.fort && !session.Core().ownerMode.battle && !session.Core().bridgeState.mission && session.Core().spots[123]==19);CHECK(session.PlayerRecords().data()==client.PlayerRecords()->Records().data() && client.CoreDataGlobals()->Current().data()==session.PlayerRecords().data()+180+0x6c);
        EditorInstallStateContext scene(client);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);bool scope{};InputQueue input;std::uint32_t calls{};const auto tail=EditorInstallTail(session,calls);session.WithExistingEditorInstallUi(scene.owner,input,tail,[&] { static_cast<void>(session.Cursor());session.SynchronizeProductionContents();scope=true; });CHECK(scope && calls==5 && session.ExistingEditorCheckpointReady());CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // 실제 State 공통 확정 없는 준비 대여 자체는 공개 완료가 아니다.
    },true);
}

// prepared 표식만으로 누락된 실제 Client 소유자를 만들거나 전역을 초기화하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedCtorRejectsIncompleteOwnersBeforeBorrow) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;auto services=devices.Make(client,storage,input);services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();client.GameplayGlobals()->cursor.screenX=73;
    CHECK(CheckpointRejects<std::invalid_argument>([&] { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services)); }));CHECK(!client.GameplayGlobals()->Borrowed() && client.GameplayGlobals()->cursor.screenX==73 && storage->OwnedImages()==0);
}

// 실제 생산 패널이 미완료 Session의 같은 기술 부모에 Form을 붙이는 경로도 설치 범위에서만 연다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActualProductionFormsInsideInstallScope) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);FontStore fonts(assets.files,"Arial");double wall=100.0;EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};auto tail=EditorInstallTail(session,calls);
        RawButtonControls controls(OriginalEdition::Patch1078,{[&] { return wall; },{},{},[&](auto form,auto flags) { session.GumpForms().Destroy({static_cast<std::uint16_t>(form)},flags); }});
        RawMenuHooks menu;menu.command=[](auto,auto,auto) {};menu.readInt=[](auto,std::int32_t&) {};menu.sound=[](auto) {};menu.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };menu.cursor=[](auto) {};menu.objectForms=[&]() -> GumpFormHost& { return session.GumpForms(); };menu.gameSeconds=[] { return 0.0; };controls.EnableMenus(assets.game,fonts,std::move(menu));
        const auto& texture=assets.game.Find("fortGump");const auto metrics=assets.game.Shapes().SquidMetrics(texture.block,0);const std::array<GumpControl,0> empty{};const std::array<std::string,0> labels{};controls.SetControls(empty,labels,640,480,{static_cast<std::uint32_t>(texture.block+70),0,metrics.width,metrics.height,{}});
        LocalMissionStateHooks hooks;hooks.logState=[](auto) {};hooks.parentState=[&](auto selected) {
            CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);
            session.WithExistingEditorInstallUi(scene.owner,input,tail,[&] {
                const auto freeBefore=session.Core().pool.FreeCount();auto& panel=controls.EnableProduction(session,assets.game,{[&] { return wall; },[](std::string_view) {},[](std::string_view) { throw std::logic_error("Editor 패널 설치의 예상 밖 Tell"); },[](std::string_view,std::int32_t fallback) { return fallback; },[](std::string_view text) { return std::string(text); }});
                CHECK(controls.Production()==&panel && session.CurrentTechnologyParent()!=0 && session.Core().pool.FreeCount()<freeBefore);CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
            });return LocalMissionParentResult::Commit;
        };
        RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();CHECK(controls.Production() && calls==5);session.CommitExistingEditorInstall(scene.owner);controls.DisableProduction();CHECK(!controls.Production() && session.Core().bookkeeping.depth==0); // UI Form을 같은 실제 Session보다 먼저 반납한다.
    });
}

// 준비 Editor의 같은 실제 큐에서 건설/이동/회수 요청이 송신 뒤 한 pending씩 처리되고 Session 반납 때 수신기만 해제된다.
TEST_CASE(CampaignExistingEditorCheckpoint_EditorWireQueuesNegativeSenderConstructionMoveAndSalvage) {
    EditorInstallWireScene link;WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        const auto wire=link.wire;CHECK(wire->Players().data()==session.PlayerRecords().data() && !wire->Dispatch().Registration(0x3e).receive);CHECK(CheckpointRejects<std::logic_error>([&] { session.SendLocalPacket(9,{}); }));RunEditorWireInstallState(session,client,checkpoint,work);CHECK(!wire->Dispatch().Registration(0x40).receive);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        const std::array<std::uint32_t,13> messages{0x5b,0x40,0x4f,0x50,0x60,0x61,0x5c,0x5f,0x7d,0x6d,0x3e,0x54,0x58};
        for (const auto type:messages) { const auto& registered=wire->Dispatch().Registration(type);CHECK(registered.receive && registered.version==1 && registered.mismatch==0); } // 기존 세 등록 함수의13종과 버전/정책을 모두 확인한다.
        CHECK(wire->Dispatch().Registration(0x3e).observer && wire->Dispatch().Registration(0x58).observer && !wire->Dispatch().Registration(0x40).observer && !wire->Dispatch().Registration(0x54).observer);CHECK(wire->Dispatch().Registration(0x3e).length==25 && wire->Dispatch().Registration(0x40).length==18 && wire->Dispatch().Registration(0x54).length==8 && wire->Dispatch().Registration(0x58).length==16);CHECK(!wire->Dispatch().Registration(0x27).receive && session.LocalConnectionPhase()==0 && session.LocalConnectionPackets().empty());
        auto core=session.Core();RawPathPoint point{};bool found{};
        for (int y=8;y<248 && !found;++y) for (int x=8;x<248;++x) if (session.MayPlaceConstruction({82,0,static_cast<float>(x),static_cast<float>(y),0,1,0})) { point={static_cast<float>(x),static_cast<float>(y)};found=true;break; } // 현재 실제 MayPlace가 허용한 다리 한 칸을 독립 입력으로 선택한다.
        CHECK(found);if (!found) return;ConstructionLocalMessage message;message.type=82;message.x=point.x;message.y=point.y;message.quality=3;message.timeLow=0x89abcdefU;message.timeHigh=0x76543210U;std::array<std::uint8_t,25> construction{};WriteConstructionCommand(message,construction);const auto freeBefore=core.pool.FreeCount();const auto money=session.Sp(1);session.SendLocalPacket(9,construction);CHECK(wire->QueuedFrames()==1 && link.packets.empty() && core.pool.FreeCount()==freeBefore && session.Construction().LastPacket().empty());
        const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;const auto movement=session.Priests().MovementProcessType(core.pool.Slot(priest)[10]);const auto raw=core.pool.Slot(priest);CHECK(raw[10]==158 && raw[34]==1 && !(raw[40]&0x20U) && !session.Priests().PriestState().GroundImmobile(priest)); // 보호 입력의 실제 owner1 사제이며 HP/현재 지면/강제 비트도 실제 몸체로 확인한다.
        const RawPathPoint source{std::bit_cast<float>(EditorWireDword(raw,14)),std::bit_cast<float>(EditorWireDword(raw,18))};RawPathPoint destination{};bool movable{};auto input=session.MakeCarrierInput({});
        for (int dy=-5;dy<=5 && !movable;++dy) for (int dx=-5;dx<=5;++dx) if (dx || dy) { auto candidate=RawPathPoint{source.x+static_cast<float>(dx),source.y+static_cast<float>(dy)};if (input->Evaluate(priest,candidate)==0x12) { destination=candidate;movable=true;break; } } // 원문004277f0/기존 Native 첫 입력의 일반 연결 지면은18이다.3은 간헐천/귀환/제단 접근점이므로 받지 않는다.
        CHECK(movable);if (!movable) return;session.RequestCarrierMove(priest,destination,0);CHECK(wire->QueuedFrames()==2 && link.packets.empty() && !FindAttachedProcess(core.pool,priest,movement).value && core.pool.FreeCount()==freeBefore);
        session.PumpLocalConnection();CHECK(wire->QueuedFrames()==1 && link.packets.size()==1 && link.senders==std::vector<std::uint32_t>{0xffffffffU} && link.packets.front()==std::vector<std::uint8_t>(construction.begin(),construction.end()));const auto notice=session.Construction().LastPacket();CHECK(notice.size()==29 && notice[4]==82 && notice[15]==1 && notice[17]==3 && notice[26]==1);if (notice.size()!=29) return;const Sid bridge{static_cast<std::uint16_t>(notice[27]|(std::uint16_t(notice[28])<<8))};CHECK(core.pool.Slot(bridge)[10]==82 && core.pool.Slot(bridge)[34]==1 && !(core.pool.Slot(bridge)[11]&3) && session.Sp(1)==money && EditorWireDword(notice,18)==0 && EditorWireDword(notice,22)==0); // 실제 request 본문만 Fort owner를 보정하고 wire 시각은 원문대로 소비하지 않는다.
        session.PumpLocalConnection();CHECK(wire->QueuedFrames()==0 && link.packets.size()==2 && link.packets.back()[1]==0x40 && link.senders.back()==0xffffffffU);const auto form=FindAttachedProcess(core.pool,priest,movement);CHECK(form.value && dynamic_cast<RawPathProcess*>(core.processes.FromForm(form)));CHECK(wire->Pending().sender==0xffffffffU && wire->Client().sender==9 && !(wire->Pending().flags&2U));
        const auto beforeSalvage=session.Sp(1);session.RequestSalvage(bridge.value);CHECK(wire->QueuedFrames()==1 && link.packets.size()==2 && !(core.pool.Slot(bridge)[11]&1U) && session.Sp(1)==beforeSalvage);session.PumpLocalConnection();CHECK(!wire->QueuedFrames() && link.packets.size()==3 && link.packets.back()[1]==0x54 && link.senders.back()==0xffffffffU && (core.pool.Slot(bridge)[11]&1U));CHECK(client.GameplayGlobals()->player.battleOwner==1 && client.GameplayGlobals()->player.templateOwner==1 && session.LocalConnectionPhase()==0 && !wire->Dispatch().Registration(0x27).receive);
    },true,&link);CHECK(link.callbacksReleased && link.closed==3 && link.released==3);
}

// 실제 현재00594fa4가 바뀌면 음수 sender의 비용 선검사도 그 소비 위치에서 다시 읽는다.
TEST_CASE(CampaignExistingEditorCheckpoint_EditorWireReadsCurrentChargeGateWithoutSenderNormalization) {
    EditorInstallWireScene link;WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());CHECK(session.Core().types[75].cost>0);ConstructionLocalMessage message;message.type=75;message.x=50;message.y=50;std::array<std::uint8_t,25> packet{};WriteConstructionCommand(message,packet);client.GameplayGlobals()->world.missionPost=false;session.SendLocalPacket(9,packet);bool actualShortfall{};
        try { session.PumpLocalConnection(); }catch (const std::out_of_range& error) { actualShortfall=std::string_view(error.what())=="건설 비용 처리 플레이어 번호 오류"; } // sender-1의 실제 Money0→비용 부족 진단을 확인하며 이를 owner1로 정규화하지 않는다.
        CHECK(actualShortfall && link.packets.size()==1 && link.packets.front()==std::vector<std::uint8_t>(packet.begin(),packet.end()) && link.senders.front()==0xffffffffU && link.wire->Pending().depth==0 && session.Construction().LastPacket().empty());client.GameplayGlobals()->world.missionPost=true;
    },true,&link);CHECK(link.callbacksReleased);
}

// 다른 처리기가 이미 등록된 전송 표를 부분 덮어쓰거나 미완료 Session의 입력을 공개하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_EditorWireRejectsOwnedRegistrationBeforePublishing) {
    EditorInstallWireScene link;link.sessionOwnsRegistration=false;WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);std::uint32_t otherCalls{};link.wire->Dispatch().Register(0x58,[&](auto,auto&,auto) { ++otherCalls; });CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(*client.LocalStateGlobals()); }));CHECK(link.wire->Dispatch().Registration(0x58).receive && !link.wire->Dispatch().Registration(0x40).receive && !link.wire->Dispatch().Registration(0x3e).receive && !otherCalls);CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SendLocalPacket(9,{}); }));
    },true,&link);CHECK(link.callbacksReleased);
}

// 전송의 이전/다른 Player 물리와 로그인 동시 사용은 gameplay 대여/모드 변경 전 거부한다.
TEST_CASE(CampaignExistingEditorCheckpoint_EditorWireRejectsForeignPlayersAndLoginBeforeBorrow) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));InitializeClientPlayerRecords(client);std::array<std::uint8_t,1620> foreign{};EditorInstallWireScene wire;const auto transport=std::make_shared<EditorLocalTransport>(foreign,wire.server,wire.Hooks());client.GameplayGlobals()->cursor.screenX=73;
    for (const bool login:{false,true}) { // 두 잘못된 수명 계약을 각각 생성 전 검사한다.
        auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;auto services=devices.Make(client,storage,input);services.editorTransport=transport;services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();if (login) services.localLogin.emplace();
        CHECK(CheckpointRejects<std::invalid_argument>([&] { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services)); }));CHECK(!client.GameplayGlobals()->Borrowed() && client.GameplayGlobals()->cursor.screenX==73 && storage->OwnedImages()==0);
    }
}
