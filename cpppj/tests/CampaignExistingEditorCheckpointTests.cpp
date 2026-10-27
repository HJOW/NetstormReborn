// Session접합과 실제 State1의 한정 UI 범위를 검사한다. 원문 게임 하위는 같은 Session 본체를 사용한다.
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
#include "client/ClientConnectionActivityGlobals.h"
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
#include "client/Mission.h" //Guide 해제가 같은 Client의 Native 미션 모드를 반납한 뒤 prepared Editor가 그 물리를 빌리는지 확인한다.
#include "o/RawFortBadgesGlobals.h"
#include "o/RawSunAviaryProcess.h" // 실제 사제 이동 프로세스의 동일 풀/부착 Form 조회 선언을 사용한다.
#include "o/RawDataManagerSelection.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <functional> // 생성 직전의 명시 prefix/선택 실패 입력만 공급한다.
#include <bit>
#include <chrono>
#include <cstdio>
#include <functional>
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
    const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();
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
        input=ReadFileBytes(path);CheckpointDevices devices;auto storage=std::make_shared<CampaignTemplateStorage>();auto effects=devices.Effects(client,work);auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
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
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"custom-test01-player-ui"),{assets.root,assets.root.parent_path()/"originalCD"});CheckpointDevices devices;devices.current=devices.pending="CustomTest01421";const auto path=work.Prepare(devices.current,input);const auto effects=devices.Effects(client,work);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
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
        { CheckpointDevices devices;auto storage=std::make_shared<CampaignTemplateStorage>();auto effects=devices.Effects(client,work);auto services=devices.Make(client,storage,ReadFileBytes(work.ResolvePath("d/Checkpoint385.fort")));const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
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
    std::shared_ptr<ClientConnectionActivityGlobals> activity;std::uint32_t activityFrame{71};std::vector<std::array<std::uint32_t,2>> activityRemoved; // 같은 Client의 실제 표와64h 현재 프레임/UI 삭제만 관측한다.
    bool callbacksReleased{},sessionOwnsRegistration{true}; // 정상 설치의 수신 반납과 기존 다른 소유자 때문에 거부한 표의 보존을 구별한다.
    PacketDispatchHooks Hooks() {
        PacketDispatchHooks hooks;hooks.now=[this] { return ++now; };hooks.record=[this](auto& connection,auto packet,auto) { senders.push_back(connection.sender);packets.emplace_back(packet.begin(),packet.end()); };
        hooks.terminate=[](auto& connection) { connection.flags|=0x40; };hooks.close=[this](auto&) { ++closed; };hooks.release=[this](auto&) { ++released; };return hooks;
    }
};
// 실제 Chat·Player·번역·작업 파일을 갖춘 같은 Client Session을 검사 동기 범위 동안 소유한다.
template<class Body> void WithEditorInstallSession(Body body,bool prepared=false,EditorInstallWireScene* wireScene=nullptr,const std::function<void(RawSurfaceStorage&)>& prepareStorage={},const std::function<void(Client&,RawCampaignSessionServices&)>& beforeCtor={}) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);if (prepared) { client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client); }InitializeClientChatManager(client);
    EditorFiles work(CheckpointRoot(assets.root.parent_path(),"ui-state-commit"),{assets.root,assets.root.parent_path()/"originalCD"});work.Prepare("Checkpoint385",input);CheckpointDevices devices;auto effects=devices.Effects(client,work);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(client,storage,input);const auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());
    services.surfaceStorage=surface;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.localStateGlobals=client.LocalStateGlobals();services.editorPlayerUi=MakeEditorPlayerClientHooks(client);services.fortTechnologyUi=[&] { RefreshClientChatRankPrefix(client,assets.game,*surface); };
    if (prepared) {
        auto& gameplay=*client.GameplayGlobals();gameplay.world.editor=1;gameplay.world.missionPost=true;gameplay.FullDeck()=1;gameplay.player.fortMode=true;gameplay.cursor.screenX=37;gameplay.cursor.lockDepth=3;gameplay.energy.userCheat=true; // 준비 대여에 대한 명시 입력이며 Native cleanup/prefix 전체 실행의 증거는 아니다.
        surface->ownerMode={true,false,false};surface->links.battle=false;surface->links.mission=false;surface->spots[123]=19;const std::uint32_t metamap=0;client.CoreDataGlobals()->Select(gameplay.player,metamap,1,1);
        services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.coreDataGlobals=client.CoreDataGlobals();services.spGlobals=client.SpGlobals();services.processKernel=&client.GetKernel(); // 준비된 Native 경로에서 별도 SP 초기화나 별도 Kernel을 만들지 않는다.
    }
    if (wireScene) {
        if (!prepared) throw std::logic_error("검사의 Editor 큐도 준비된 Player 물리를 요구함");wireScene->wire=std::make_shared<EditorLocalTransport>(client.PlayerRecords()->Records(),wireScene->server,wireScene->Hooks());wireScene->wire->Open(6799);services.editorTransport=wireScene->wire;
        wireScene->activity=client.ConnectionActivityGlobals();services.connectionActivityGlobals=wireScene->activity;services.connectionFrameNumber=[wireScene] { return wireScene->activityFrame; };services.removeConnectionStatus=[wireScene](auto kind,auto player) { wireScene->activityRemoved.push_back({kind,player}); }; // 실제64h 수신은 같은 Session 모드 관문 뒤의 외부 UI 장치만 호출한다.
        services.construction.configInt=[&client](auto name,std::int32_t fallback) { client.Configuration().ReadInt(name,fallback);return fallback; };services.construction.wallSeconds=[] { return 100.0; };services.construction.refresh=[&devices] { ++devices.production; }; // 실제 Client 설정과 원문 배치 뒤 외부 UI 통지를 공급하며 게임 판정을 대신하지 않는다.
    }
    if (prepareStorage) prepareStorage(*surface);
    const std::array<std::uint32_t,13> messages{0x5b,0x40,0x4f,0x50,0x60,0x61,0x5c,0x5f,0x7d,0x6d,0x3e,0x54,0x58};std::array<PacketRegistration,13> registrations{}; // 반납 전 현재 등록 정책을 독립적으로 고정한다.
    PacketRegistration activityRegistration; //64h는 기존13종 목록 밖에서 수신 반납과 정책 보존을 별도로 관측한다.
    if (beforeCtor) beforeCtor(client,services); // 같은 fifth hook을 현재63과 같은 Session 생성 직전에 정확히 한 번 호출한다.
    { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services));body(session,client,effects,work);
      if (wireScene) activityRegistration=wireScene->wire->Dispatch().Registration(0x64);
      if (wireScene) for (std::size_t index=0;index<messages.size();++index) registrations[index]=wireScene->wire->Dispatch().Registration(messages[index]); } // 수신 callback의 같은 Session이 Client/Player보다 먼저 반납된다.
    if (wireScene) {
        for (std::size_t index=0;index<messages.size();++index) { // receive만 반납하고 원문 observer/프로토콜/정책/길이 값은 유지한다.
            const auto& current=wireScene->wire->Dispatch().Registration(messages[index]);CHECK((wireScene->sessionOwnsRegistration ? !current.receive : bool(current.receive)==bool(registrations[index].receive)) && current.version==registrations[index].version && current.mismatch==registrations[index].mismatch && current.length==registrations[index].length && bool(current.observer)==bool(registrations[index].observer));
        }
        const auto& activity=wireScene->wire->Dispatch().Registration(0x64);CHECK((wireScene->sessionOwnsRegistration ? !activity.receive : bool(activity.receive)==bool(activityRegistration.receive)) && activity.version==activityRegistration.version && activity.mismatch==activityRegistration.mismatch && activity.length==activityRegistration.length && bool(activity.observer)==bool(activityRegistration.observer)); //64h receive만 반납하고 원문 표는 유지한다.
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

// prepared Session 생성은 앞서 준비한 같은 섬/청크 물리를 소비하며 지역 새0/Reset으로 교체하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedCtorKeepsSharedIslandAndChunkData) {
    WithEditorInstallSession([](RawCampaignSession& session,Client&,const auto&,EditorFiles&) {
        const auto& surface=*session.World().Storage();CHECK(session.IslandCount()==7 && surface.islands.records[18]==0xa5 && surface.islands.records[19]==0xa5);
        CHECK(surface.chunks.Mode()==0x89abcdefU && surface.chunks.At(2,3).island==6 && surface.chunks.At(2,3).ordinal==71 && surface.chunks.At(2,3).record[5]==6);
        CHECK(!session.ExistingEditorCheckpointReady()); // 데이터 보존 검사는 checkpoint/전체 Native prefix 완료를 대신하지 않는다.
    },true,nullptr,[](RawSurfaceStorage& surface) {
        surface.islands.records.fill(0xa5);surface.islands.records[0]=7;surface.islands.records[1]=surface.islands.records[2]=surface.islands.records[3]=0;
        surface.chunks.Mode()=0x89abcdefU;surface.chunks.At(2,3)={6,'P',{1,2,3,4,5,6},71}; // 실제 Native 준비 완료가 아닌 생성자 보존성 검출용 입력이다.
    });
}

// 실제 prepared 대여 뒤 SoundList 필수 계약에서 생성이 실패해도 선택 owner의 모든 입력을 남기고 Use만 반납한다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedCtorAndLateFailureKeepSelectionGlobals) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));client.StaticStrings()->Initialize(10240);client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client);InitializeClientChatManager(client);
    auto& gameplay=*client.GameplayGlobals();gameplay.world.editor=1;gameplay.world.missionPost=true;gameplay.player.fortMode=true;const std::uint32_t metamap=0;client.CoreDataGlobals()->Select(gameplay.player,metamap,1,1);
    auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;auto services=devices.Make(client,storage,input);auto surface=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux());surface->ownerMode={true,false,false};surface->links.battle=false;surface->links.mission=false;
    services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.coreDataGlobals=client.CoreDataGlobals();services.localStateGlobals=client.LocalStateGlobals();services.spGlobals=client.SpGlobals();services.processKernel=&client.GetKernel();services.surfaceStorage=surface;services.soundNames=nullptr; // 실제 후반 SoundList 검증에서만 실패하도록 나머지 준비 소유자를 제공한다.
    auto& owner=gameplay.selection;owner.enabled=0x80000000U;owner.mode=5;owner.selected=0xfedcba98U;owner.mouseType=71;owner.mouse={-33,65};owner.remembered={0x7fc00001U,19};owner.multipleCount=-7;owner.multiple={0x10037U,0xffffffffU};bool rejected{};
    try { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(services)); }catch (const std::invalid_argument& error) { rejected=std::string_view(error.what())=="세션의 실제 SoundList 누락"; }
    CHECK(rejected && surface->Generation()==1 && !surface->Borrowed() && !gameplay.Borrowed()); // World 대여까지 진행한 뒤의 실제 필수 계약 실패다.
    CHECK(owner.enabled==0x80000000U && owner.mode==5 && owner.selected==0xfedcba98U && owner.mouseType==71 && owner.mouse.x==-33 && owner.mouse.y==65 && owner.remembered[0]==0x7fc00001U && owner.remembered[1]==19 && owner.multipleCount==-7 && owner.multiple[0]==0x10037U && owner.multiple[1]==0xffffffffU);
    auto recovered=devices.Make(client,storage,input);recovered.adoptPreparedWorld=true;recovered.adoptInitializedPlayers=true;recovered.gameplayGlobals=client.GameplayGlobals();recovered.subscriberGlobals=client.SubscriberGlobals();recovered.playerRecords=client.PlayerRecords();recovered.coreDataGlobals=client.CoreDataGlobals();recovered.localStateGlobals=client.LocalStateGlobals();recovered.spGlobals=client.SpGlobals();recovered.processKernel=&client.GetKernel();recovered.surfaceStorage=surface; // 이번에는 실제 같은 SoundList도 제공해 성공한 생성과 반납을 대조한다.
    { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(recovered));CHECK(gameplay.Borrowed() && surface->Borrowed() && surface->Generation()==2 && owner.selected==0xfedcba98U && owner.mode==5 && owner.multipleCount==-7 && owner.multiple[0]==0x10037U && owner.mouse.x==-33); }
    CHECK(!gameplay.Borrowed() && !surface->Borrowed() && owner.selected==0xfedcba98U && owner.mode==5 && owner.mouseType==71 && owner.multipleCount==-7 && owner.multiple[1]==0xffffffffU && owner.remembered[0]==0x7fc00001U);
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

// 최종 Editor의 별도64h는 실제 pending→client9 큐에서 같은 Client 표를 갱신하고 수신 반납 뒤 정책을 유지한다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActivityWireUsesClientNineAndRetiresOnlyReceive) {
    EditorInstallWireScene link;WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());const auto wire=link.wire;const auto& registration=wire->Dispatch().Registration(0x64);
        CHECK(registration.receive && registration.observer && registration.version==1 && registration.mismatch==0 && registration.length==5 && !wire->Dispatch().Registration(0x27).receive);
        session.PumpLocalConnection();CHECK(wire->PendingAlive() && wire->ClientAlive() && link.packets.empty()); // 실제 listener 수락을 한 뒤에만 존재하는 pending 끝으로 송신한다.
        CHECK(link.activity==client.ConnectionActivityGlobals());const auto owner=link.activity;owner->Frames()[9]=17;owner->WallSeconds()[9]=19;owner->ActiveStamp()=37;owner->ActiveZoneWord()=41;
        const std::array<std::uint8_t,5> packet{1,0x64,5,0,0xa5};session.SendLocalPacket(-1,packet);CHECK(wire->QueuedFrames()==1 && owner->Frames()[9]==17 && owner->WallSeconds()[9]==19 && link.packets.empty());
        session.PumpLocalConnection();CHECK(!wire->QueuedFrames() && link.packets==std::vector<std::vector<std::uint8_t>>{std::vector<std::uint8_t>(packet.begin(),packet.end())} && link.senders==std::vector<std::uint32_t>{9});
        CHECK(owner->Frames()[9]==71 && owner->WallSeconds()[9]==100 && owner->ActiveStamp()==37 && owner->ActiveZoneWord()==41 && link.activityRemoved.empty()); // 현재missionPost1은 삭제 UI를 선택하지 않는다.
        client.GameplayGlobals()->world.missionPost=false;session.Core().processState.Server()=true;link.activityFrame=0;session.SendLocalPacket(-1,packet);session.PumpLocalConnection();
        CHECK(owner->Frames()[9]==0 && owner->WallSeconds()[9]==100 && (link.activityRemoved==std::vector<std::array<std::uint32_t,2>>{{2,9}}));
        client.GameplayGlobals()->world.missionPost=true;CHECK(session.LocalConnectionPackets().empty() && session.LocalConnectionPhase()==0); // 일반 Login 기록/phase를 Editor에서 합성하지 않는다.
    },true,&link);
    const auto& after=link.wire->Dispatch().Registration(0x64);CHECK(link.callbacksReleased && !after.receive && after.observer && after.version==1 && after.mismatch==0 && after.length==5);
    CHECK(link.activity && link.activity->Frames()[9]==0 && link.activity->WallSeconds()[9]==100 && link.activity->ActiveStamp()==37 && link.activity->ActiveZoneWord()==41); // Client 반납 뒤에도 공유 owner를 빌린 소비자가 실제 마지막 값을 보존한다.
}

// 별도64h의 기존 소유자 충돌도13종을 쓰기 전에 거부하고 타인의 정책/observer/receive를 반납하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_ActivityCollisionPreservesAllPriorRegistrations) {
    EditorInstallWireScene link;link.sessionOwnsRegistration=false;WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);auto& dispatch=link.wire->Dispatch();const auto observations=std::make_shared<std::uint32_t>(); // Session 반납 뒤 보존되는 외부 observer도 살아 있는 같은 카운터를 소유한다.
        dispatch.Observe(0x64,[observations](auto) { ++*observations; });dispatch.Version(0x64,7,4);dispatch.RequireLength(0x64,9);dispatch.Register(0x64,[](auto,auto&,auto) { throw std::logic_error("선점된64h가실제수신됨"); });
        CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(*client.LocalStateGlobals()); }));
        const std::array<std::uint32_t,13> messages{0x5b,0x40,0x4f,0x50,0x60,0x61,0x5c,0x5f,0x7d,0x6d,0x3e,0x54,0x58};
        for (const auto type:messages) CHECK(!dispatch.Registration(type).receive); //64h 충돌 하나 때문에 기존13종의 부분 등록이 생겨서는 안 된다.
        const auto& before=dispatch.Registration(0x64);CHECK(before.receive && before.observer && before.version==7 && before.mismatch==4 && before.length==9 && *observations==0);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }) && CheckpointRejects<std::logic_error>([&] { session.SendLocalPacket(9,{}); }));
    },true,&link);
    const auto& after=link.wire->Dispatch().Registration(0x64);CHECK(link.callbacksReleased && after.receive && after.observer && after.version==7 && after.mismatch==4 && after.length==9);
}

// 준비된 false/false는 실제 Session 구성/현재 조회/반납 모두에서 같은 두 bool에 남는다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedAuthorityUsesSameClientOwnerWithoutTrueWrites) {
    std::shared_ptr<RawAuthorityState> retained;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        auto core=session.Core();CHECK(retained==client.GameplayGlobals()->Authority() && retained==session.World().Storage()->Authority());CHECK(&core.processState.Server()==&client.GameplayGlobals()->Server() && &core.processState.Boss()==&client.GameplayGlobals()->Boss());
        CHECK(!core.processState.Server() && !core.processState.Boss() && !core.pool.IsServer());core.processState.server=core.processState.boss=true;session.PrepareFrame({},true);CHECK(!retained->server && !retained->boss && !core.pool.IsServer());
    },true,nullptr,[&](RawSurfaceStorage& storage) { retained=storage.Authority();retained->server=retained->boss=false; });
    CHECK(retained && !retained->server && !retained->boss); // Session/World/Client가 모두 반납된 뒤에도 관측한 같은 owner 값이 남는다.
}

// 실제 prepared World 뒤 prefix 검증 실패도 owner의 현재 값을 초기화/복원하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_PreparedAuthoritySurvivesPartialConstructorFailure) {
    std::shared_ptr<RawAuthorityState> retained;bool entered{};
    CHECK(CheckpointRejects<std::logic_error>([&] {
        WithEditorInstallSession([&](RawCampaignSession&,Client&,const auto&,EditorFiles&) { entered=true; },true,nullptr,[&](RawSurfaceStorage& storage) { retained=storage.Authority();retained->server=false;retained->boss=true;storage.ownerMode.fort=false; });
    }));
    CHECK(retained && !entered && !retained->server && retained->boss);
}

// 실제 준비 Session의 객체/생산/Deck은 같은 Client DWORD를 빌리고 생성/반납에서 비영 원시값을 바꾸지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_FullDeckOwnerPreparedConsumersAndRelease) {
    for (const std::uint32_t value:{0U,0x80000000U,0xffffffffU}) { // 자연 prefix의1 외 입력은 읽기와 물리 보존 검증용이며 Native writer의 추가 값으로 세지 않는다.
        std::shared_ptr<ClientGameplayGlobals> owner;const std::uint32_t* address{};
        WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
            CHECK(owner==client.GameplayGlobals() && owner->FullDeck()==value && session.FortObjectSettings().sharedFullDeck==address && session.FortObjectSettings().FortDestroyBlocked()==(value!=0));
            if (!value) return; // 준비0의 생성 보존과 실제1/비영 편집 설치의 정상 관문을 구별한다.
            session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);CHECK(session.ExistingEditorCheckpointReady() && owner->FullDeck()==value);const auto players=session.PlayerRecords();const auto at=std::size_t(session.TemplateOwner())*180+0x68;std::uint32_t token{};
            for (std::size_t byte=0;byte<4;++byte) token|=std::uint32_t(players[at+byte])<<(byte*8); // 현재 Player의 실제 Deck 포인터만 읽으며 가짜 블록을 만들지 않는다.
            auto deck=session.Production().Deck().Bytes(token);CHECK(deck.size()==0x160 && deck[0x14c]!=0);deck[2]=0x7e;const auto before=std::vector<std::uint8_t>(deck.begin(),deck.end());session.Production().RefreshCurrent();CHECK(std::equal(before.begin(),before.end(),deck.begin()) && owner->FullDeck()==value); // 같은 fullDeck 비영이 실제 재계산의 자연 return을 선택한다.
            owner->FullDeck()=0;session.Production().RefreshCurrent();CHECK(deck[2]!=0x7e && !session.FortObjectSettings().FortDestroyBlocked());owner->FullDeck()=value;
        },true,nullptr,{},[&](Client& client,RawCampaignSessionServices&) { owner=client.GameplayGlobals();address=&owner->FullDeck();owner->FullDeck()=value; });
        CHECK(owner && !owner->Borrowed() && &owner->FullDeck()==address && owner->FullDeck()==value);
    }
}

// 준비 prefix 검증 실패와 실제 World 후처리의 후반 예외도 현재 DWORD를0/1로 되돌리지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_FullDeckOwnerPreservesConstructorAndLateFailure) {
    std::shared_ptr<ClientGameplayGlobals> owner;bool constructed{},selected{},caught{};
    try { WithEditorInstallSession([&](auto&,auto&,const auto&,auto&) { constructed=true; },true,nullptr,{},[&](Client& client,RawCampaignSessionServices&) { owner=client.GameplayGlobals();owner->FullDeck()=0xffffffffU;owner->player.templateOwner=2; }); }catch (const std::logic_error& error) { caught=std::string_view(error.what())=="준비된 Editor의 실제 prefix 모드/Player 불일치"; }
    CHECK(caught && !constructed && owner && !owner->Borrowed() && owner->FullDeck()==0xffffffffU);
    caught=false;try { WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) { constructed=true;CHECK(session.FortObjectSettings().sharedFullDeck==&client.GameplayGlobals()->FullDeck());session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work); },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) { owner=client.GameplayGlobals();owner->FullDeck()=0x80000000U;services.fortTechnologyUi=[&] { selected=true;throw CheckpointStop("full-deck-after-world"); }; }); }catch (const CheckpointStop& error) { caught=std::string_view(error.what())=="full-deck-after-world"; }
    CHECK(caught && constructed && selected && owner && !owner->Borrowed() && owner->FullDeck()==0x80000000U);
}

// 기존 비준비 Editor만 fresh0 뒤 호환1을 쓰며 반납은 이 같은 owner의 현재 값을 보존한다.
TEST_CASE(CampaignExistingEditorCheckpoint_FullDeckOwnerOrdinaryEditorCompatibility) {
    std::shared_ptr<ClientGameplayGlobals> owner;WithEditorInstallSession([&](RawCampaignSession& session,Client&,const auto&,EditorFiles&) { CHECK(owner->FullDeck()==1 && session.FortObjectSettings().sharedFullDeck==&owner->FullDeck() && session.FortObjectSettings().FortDestroyBlocked()); },false,nullptr,{},[&](Client& client,RawCampaignSessionServices&) { owner=client.GameplayGlobals();owner->FullDeck()=0xffffffffU; });CHECK(owner && !owner->Borrowed() && owner->FullDeck()==1);
}

// 준비된 Native prefix의64BYTE 카메라를 같은 실제 Client/Session이 빌리고 생성·반납에서는 초기화하지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_CameraPreparedCtorAndReleasePreserveCurrentOwner) {
    std::shared_ptr<ClientGameplayGlobals> owner;const RawCameraViewState* snapshot{};
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        CHECK(owner==client.GameplayGlobals() && &session.CameraGlobals().Snapshot()==snapshot && session.CameraGlobals().Snapshot().worldBits[0]==0x3fa00000U && session.CameraGlobals().Snapshot().worldBits[1]==0x40200000U && session.CameraGlobals().Target().pixels[0]==71);
        CHECK(session.CameraGlobals().Snapshot().pixels[0]==20 && session.CameraGlobals().Snapshot().pixels[1]==27); // 픽셀20/27에서 역산하면 Y의분수2.5를 잃으므로 독립 기대다.
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices&) {
        owner=client.GameplayGlobals();auto& camera=owner->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.RendererView(),1.25F,2.5F);camera.CopyRendererSnapshot();camera.RequestPixels(71,83);snapshot=&camera.Snapshot();
    });
    CHECK(owner && !owner->Borrowed() && &owner->Camera().Snapshot()==snapshot && snapshot->worldBits[0]==0x3fa00000U && snapshot->worldBits[1]==0x40200000U && owner->Camera().Target().pixels[1]==83);
}

// 실제 World 뒤 선택된 SoundList 생성실패는 준비된64BYTE/논리Renderer/목표의 현재값을 반납 시에도 유지한다.
TEST_CASE(CampaignExistingEditorCheckpoint_CameraPreparedCtorFailureKeepsOwner) {
    std::shared_ptr<ClientGameplayGlobals> owner;bool rejected{};
    try {
        WithEditorInstallSession([&](auto&,auto&,const auto&,auto&) { throw std::logic_error("카메라 준비 생성실패의 body 도달 오류"); },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) {
            owner=client.GameplayGlobals();auto& camera=owner->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.RendererView(),3.125F,4.875F);camera.CopyRendererSnapshot();camera.RequestPixels(71,83);services.soundNames=nullptr;
        });
    }catch (const std::invalid_argument& error) { rejected=std::string(error.what()).find("SoundList")!=std::string::npos; }
    CHECK(rejected && owner && !owner->Borrowed() && owner->Camera().Snapshot().worldBits[0]==0x40480000U && owner->Camera().Snapshot().worldBits[1]==0x409c0000U && owner->Camera().RendererView().worldBits[1]==0x409c0000U && owner->Camera().Target().pixels[0]==71);
}

// 실제 prepared World 대여 뒤 필수 SoundList 누락으로 실패해도 보조 전역과 다른 네 기한을 보존한다.
TEST_CASE(CampaignExistingEditorCheckpoint_StateAuxPreparedCtorAndLateFailurePreserveCurrentOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=CheckpointInput(assets.files);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));client.StaticStrings()->Initialize(10240);client.SpGlobals()->Store().Initialize();InitializeClientPlayerRecords(client);InitializeClientChatManager(client); // 생성만 하며 GUI/메인 루프를 시작하지 않는다.
    auto& gameplay=*client.GameplayGlobals();gameplay.world.editor=1;gameplay.world.missionPost=true;gameplay.player.fortMode=true;const std::uint32_t metamap=0;client.CoreDataGlobals()->Select(gameplay.player,metamap,1,1);auto storage=std::make_shared<CampaignTemplateStorage>();CheckpointDevices devices;const auto surface=MakeRawSurfaceStorage(assets.game,gameplay.Authority(),gameplay.GameAux()); // 같은 실제 권한과 게임 보조 owner를 함께 대여한다.
    surface->ownerMode={true,false,false};surface->links.battle=false;surface->links.mission=false;const auto owner=client.LocalStateGlobals();owner->ScriptBreak()=0x80000000U;owner->AbortPending()=0xffffffffU;owner->StateAuxCC()=0xfedcba98U;owner->MetadataMode()=0x87654321U;owner->ConnectionNotified()=0x80000001U;
    owner->CachedPeerIndex()=0;owner->CachedPeerValue()=71;owner->CachedPeerFieldA4()=0xffffffffU;owner->CachedPeerFieldA8()=0x12345678U;owner->ReconnectSaveEnd()=std::bit_cast<double>(std::uint64_t{0x7ff8000076543210ULL});owner->LoginEnd()=61.25;owner->ConnectEnd()=73.5;owner->CaptureEnd()=89.75;owner->ActivityEnd()=103.125;
    const auto snapshot=[&] { return std::array<std::uint64_t,14>{owner->ScriptBreak(),owner->AbortPending(),owner->StateAuxCC(),owner->MetadataMode(),owner->ConnectionNotified(),owner->CachedPeerIndex(),owner->CachedPeerValue(),owner->CachedPeerFieldA4(),owner->CachedPeerFieldA8(),std::bit_cast<std::uint64_t>(owner->ReconnectSaveEnd()),std::bit_cast<std::uint64_t>(owner->LoginEnd()),std::bit_cast<std::uint64_t>(owner->ConnectEnd()),std::bit_cast<std::uint64_t>(owner->CaptureEnd()),std::bit_cast<std::uint64_t>(owner->ActivityEnd())}; }; // 서로 다른5개 double의 원시 비트를 산술 비교하지 않는다.
    const auto before=snapshot();const auto* scriptAddress=&owner->ScriptBreak();const auto* reconnectAddress=&owner->ReconnectSaveEnd(); // 실패와 재대여에서도 같은 주소를 요구한다.
    const auto make=[&] { auto services=devices.Make(client,storage,input);services.adoptPreparedWorld=true;services.adoptInitializedPlayers=true;services.gameplayGlobals=client.GameplayGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.coreDataGlobals=client.CoreDataGlobals();services.localStateGlobals=owner;services.spGlobals=client.SpGlobals();services.processKernel=&client.GetKernel();services.surfaceStorage=surface;return services; }; // 실제 공통 prefix 소유자만 제공하며 Native cleanup을 호출했다고 주장하지 않는다.
    auto missing=make();missing.soundNames=nullptr;bool rejected{};try { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(missing)); }catch (const std::invalid_argument& error) { rejected=std::string_view(error.what())=="세션의 실제 SoundList 누락"; }
    CHECK(rejected && surface->Generation()==1 && !surface->Borrowed() && !gameplay.Borrowed() && snapshot()==before); // 첫 실제 World가 만들어진 뒤 실패해야 앞선 검증 거부와 구별된다.
    { auto recovered=make();RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(input,assets.game.TypeTable()),"deferred.fort",std::move(recovered));CHECK(surface->Generation()==2 && surface->Borrowed() && gameplay.Borrowed() && snapshot()==before && !session.ExistingEditorCheckpointReady());owner->StateAuxCC()=0xffffffffU;owner->ScriptBreak()=0; } // 정상 생성/현재 외부 쓰기/반납을 같은 owner로 이어서 검증한다.
    auto expected=before;expected[0]=0;expected[2]=0xffffffffU;CHECK(!surface->Borrowed() && !gameplay.Borrowed() && snapshot()==expected && &owner->ScriptBreak()==scriptAddress && &owner->ReconnectSaveEnd()==reconnectAddress);
}

namespace { // 서로 떨어진17주소를 packed image나 같은 자료형 배열로 가장하지 않는다.
// 각 실제 폭의 비트만 모아 생성/반납 전후 전체 필드를 대조한다.
std::array<std::uint64_t,17> CheckpointGameAuxValues(const RawGameAuxState& owner) {
    return {owner.OrdinaryGeysers(),owner.GeyserFrameFlag(),owner.SavedGeyserCount(),std::bit_cast<std::uint64_t>(owner.GeyserDeadline()),std::bit_cast<std::uint64_t>(owner.CleanupDeadline()),std::bit_cast<std::uint64_t>(owner.SecondaryGeyserDeadline()),std::bit_cast<std::uint64_t>(owner.ScheduledDrawDeadline()),std::bit_cast<std::uint32_t>(owner.TotalCost()),owner.ResourcePoolBits(),owner.RefundPercent(),std::bit_cast<std::uint32_t>(owner.AllowAnyCapture()),owner.Archipelago(),std::bit_cast<std::uint32_t>(owner.LostX()),std::bit_cast<std::uint32_t>(owner.LostY()),std::bit_cast<std::uint32_t>(owner.DefaultSp()),owner.GeyserType(),owner.SubscriberMarker()};
}
// 17개 모두 구별되는 현재 입력이며 실제 World/Session 계산의 결과를 합성하지 않는다.
void SetCheckpointGameAuxValues(RawGameAuxState& owner) {
    owner.OrdinaryGeysers()=17;owner.GeyserFrameFlag()=19;owner.SavedGeyserCount()=23;owner.GeyserDeadline()=-0.0;owner.CleanupDeadline()=29.25;owner.SecondaryGeyserDeadline()=31.5;owner.ScheduledDrawDeadline()=37.75;owner.TotalCost()=-41;owner.StoreResourcePoolBits(0xfedcba98U);owner.RefundPercent()=43;owner.AllowAnyCapture()=-47;owner.Archipelago()=0x12345678U;owner.LostX()=std::bit_cast<float>(0xc61c4000U);owner.LostY()=-0.0F;owner.DefaultSp()=-53;owner.GeyserType()=123;owner.SubscriberMarker()=59;
}
}

// 실제준비Session의성공생성/반납은17필드와그주소를그대로보존한다.
TEST_CASE(GameAux_PreparedCtorAndReleaseKeepAllCurrentFields) {
    std::shared_ptr<RawGameAuxState> owner;std::array<std::uint64_t,17> before{};std::shared_ptr<RawSurfaceStorage> surface;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        CHECK(client.GameplayGlobals()->GameAux()==owner && session.World().Storage()->GameAux()==owner && CheckpointGameAuxValues(*owner)==before && &session.Core().bookkeeping.TotalCost()==&owner->TotalCost());
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) { owner=client.GameplayGlobals()->GameAux();SetCheckpointGameAuxValues(*owner);before=CheckpointGameAuxValues(*owner);surface=services.surfaceStorage; });
    CHECK(owner && surface && !surface->Borrowed() && CheckpointGameAuxValues(*owner)==before); // 실제checkpoint/HUD설치완료의증거가아닌준비대여수명이다.
}
// World준비대여뒤SoundList필수장치실패도17필드와다음정상대여를보존한다.
TEST_CASE(GameAux_PreparedCtorLateFailureReleasesLeaseWithoutReset) {
    std::shared_ptr<RawGameAuxState> owner;std::shared_ptr<RawSurfaceStorage> surface;std::shared_ptr<ClientGameplayGlobals> gameplay;std::array<std::uint64_t,17> before{};bool rejected{};
    try { WithEditorInstallSession([&](auto&,auto&,const auto&,auto&) { throw std::logic_error("게임보조실패검사의body도달오류"); },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) { gameplay=client.GameplayGlobals();owner=gameplay->GameAux();SetCheckpointGameAuxValues(*owner);before=CheckpointGameAuxValues(*owner);surface=services.surfaceStorage;services.soundNames=nullptr; }); }catch (const std::invalid_argument& error) { rejected=std::string(error.what()).find("SoundList")!=std::string::npos; }
    CHECK(rejected && owner && surface && gameplay && !surface->Borrowed() && !gameplay->Borrowed() && surface->Generation()==1 && CheckpointGameAuxValues(*owner)==before);
    { auto use=surface->AcquirePrepared();CHECK(surface->Generation()==2 && CheckpointGameAuxValues(*owner)==before); }CHECK(!surface->Borrowed() && CheckpointGameAuxValues(*owner)==before);
}

// prepared실제Session은같은장기커서의30DWORD/측정/배치/snap을만들거나반납하면서초기화하지않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_CursorDataPreparedBorrowAndReleasePreserveCurrentValues) {
    std::shared_ptr<ClientGameplayGlobals> owner;const std::uint32_t* address{};
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        auto& data=owner->CursorGlobals();CHECK(owner==client.GameplayGlobals() && !session.HasCursorRuntimeForState() && data.Lifecycle().previews.data()==address && data.Lifecycle().previews[29]==0x87654321U && data.Lifecycle().previewCount==-7 && data.Lifecycle().currentGump==0x12345678U && data.Placement().description=="prepared-cursor" && data.SnapToGrid()==0x80000000U);
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices&) { owner=client.GameplayGlobals();auto& data=owner->CursorGlobals();address=data.Lifecycle().previews.data();data.Lifecycle().previews[29]=0x87654321U;data.Lifecycle().previewCount=-7;data.Lifecycle().currentGump=0x12345678U;data.Placement().description="prepared-cursor";data.SnapToGrid()=0x80000000U; });
    CHECK(owner && !owner->Borrowed() && owner->CursorGlobals().Lifecycle().previews.data()==address && owner->CursorGlobals().Lifecycle().previews[29]==0x87654321U && owner->CursorGlobals().Lifecycle().previewCount==-7 && owner->CursorGlobals().Placement().description=="prepared-cursor" && owner->CursorGlobals().SnapToGrid()==0x80000000U);
}

// World대여뒤SoundList검증이실패해도prepared커서의현재비트/문구/주소는이전값으로되돌리지않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_CursorDataPreparedFailureKeepsOwner) {
    std::shared_ptr<ClientGameplayGlobals> owner;bool rejected{};
    try { WithEditorInstallSession([&](auto&,auto&,const auto&,auto&) { throw std::logic_error("커서prepared실패검사의body도달오류"); },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) { owner=client.GameplayGlobals();auto& data=owner->CursorGlobals();data.Lifecycle().previews[29]=0xffffffffU;data.Lifecycle().previewCount=-3;data.Placement().description="partial-cursor";data.SnapToGrid()=0x87654321U;services.soundNames=nullptr; }); }catch (const std::invalid_argument& error) { rejected=std::string(error.what()).find("SoundList")!=std::string::npos; }
    CHECK(rejected && owner && !owner->Borrowed() && owner->CursorGlobals().Lifecycle().previews[29]==0xffffffffU && owner->CursorGlobals().Lifecycle().previewCount==-3 && owner->CursorGlobals().Placement().description=="partial-cursor" && owner->CursorGlobals().SnapToGrid()==0x87654321U);
}

// 파일 전 HUD가 실제 현재 Deck을 생성하되 커서/저장/일반 생산 입력은 공통 확정까지 닫혀 있어야 한다.
TEST_CASE(CampaignExistingEditorPrefix_ActualDeckBeforeCheckpointAndScopeRetirement) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        auto& owner=*client.LocalStateGlobals();owner.State().current[0]=1;ClientLocalStateGlobals other;
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(other,[] {}); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));
        const auto kernelBefore=client.GetKernel().Size(); //44f760의 새 Deck은 저장 블록이며 열기 전에는 Kernel 프로세스를 만들지 않는다.
        session.WithExistingEditorPrefixUi(owner,[&] {
            session.SynchronizeProductionContents();
            CHECK(CheckpointRejects<std::logic_error>([&] { session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work); }));
            CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));
            CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
        });
        const auto players=client.PlayerRecords()->Records();const auto at=180U+0x68U;
        CHECK(players[at] || players[at+1] || players[at+2] || players[at+3]);
        CHECK(client.GetKernel().Size()==kernelBefore);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(owner,[] {}); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));
        session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);CHECK(session.ExistingEditorCheckpointReady());
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(owner,[] {}); }));
    },true);
}

// prefix callback의 실제 부분 효과를 보존하고 예외 복귀 뒤 설치 전 접근 허용 범위가 남지 않아야 한다.
TEST_CASE(CampaignExistingEditorPrefix_FailureClosesScopeWithoutPublishingRuntime) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        auto& owner=*client.LocalStateGlobals();owner.State().current[0]=1;
        CHECK(CheckpointRejects<CheckpointStop>([&] { session.WithExistingEditorPrefixUi(owner,[&] { session.SynchronizeProductionContents();throw CheckpointStop("실제 prefix UI 중단"); }); }));
        CHECK(!session.ExistingEditorCheckpointReady() && owner.State().phase[0]==0 && owner.State().current[0]==1);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(owner,[] {}); }));
    },true);
}

// 미준비 Session 및 State2는 State1 prefix의 우회 설치 통로를 열지 않아야 한다.
TEST_CASE(CampaignExistingEditorPrefix_RequiresPreparedOwnerAndCurrentState1) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        auto& owner=*client.LocalStateGlobals();owner.State().current[0]=1;
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(owner,[] {}); }));
    });
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        auto& owner=*client.LocalStateGlobals();owner.State().current[0]=2;
        CHECK(CheckpointRejects<std::logic_error>([&] { session.WithExistingEditorPrefixUi(owner,[] {}); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); }));
    },true);
}

// 호스트 입력 두 몸체는 기존 UI scope에서도 닫고 실제 공통 State 확정/최종 공개 뒤에만 구성한다.
TEST_CASE(CampaignExistingEditorCheckpoint_InputFactoriesStayClosedUntilActualStateCommit) {
    EditorInstallWireScene link; //같은 State 도우미의 실제 수신 차단 검사에도 실제 로컬 양끝을 제공한다.
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        auto& owner=*client.LocalStateGlobals();auto& globals=client.GameplayGlobals()->selection;
        CHECK(CheckpointRejects<std::logic_error>([&] { session.MakeCarrierInput({}); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.MakeUserSelection(globals.SelectionState(),{}); }));
        RunEditorWireInstallState(session,client,checkpoint,work);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.MakeCarrierInput({}); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.MakeUserSelection(globals.SelectionState(),{}); }));
        session.CommitExistingEditorInstall(owner);
        const auto carrier=session.MakeCarrierInput({});const auto selection=session.MakeUserSelection(globals.SelectionState(),{});
        CHECK(carrier && selection && owner.State().phase[0]==1 && owner.State().next[0]==4);
    },true,&link);
}

// 같은 실제 Guide 스크립트의 Config/Native 모드 해제 뒤 prepared Editor가 강제 덮어쓰기 없이 설치되어야 한다.
TEST_CASE(CampaignExistingEditorCheckpoint_GuideReleaseClearsSameNativeMissionBeforeEditor) {
    EditorInstallWireScene link;bool observed{}; //실제 Config 알림에서 확인한 스크립트 해제/Native0/Config0 순서의 관측이다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        CHECK(observed && !session.Core().bridgeState.mission && client.Configuration().GetInt("global.inMission")==0);
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        CHECK(!session.Core().bridgeState.mission && client.Configuration().GetInt("global.inMission")==0 && session.ExistingEditorCheckpointReady());
    },true,&link,{},[&](Client& client,RawCampaignSessionServices& services) {
        auto& configuration=client.Configuration();configuration.Set("missionSpec","d/{local.1}.english");auto& missionMode=services.surfaceStorage->links.mission;
        auto mission=CreateGuideMissionScript(configuration,"TEST01",missionMode);CHECK(mission && mission->Loaded() && missionMode && configuration.GetInt("global.inMission")==1);
        const auto notify=configuration.onChanged;configuration.onChanged=[&] { //실제 Set 알림은 스크립트 등록 반납과 Native 모드0 이후에만 실행되어야 한다.
            CHECK(!mission && !configuration.Registry().FindByName("mission") && !missionMode && configuration.GetInt("global.inMission")==0);observed=true;if (notify) notify();
        };
        ReleaseGuideMissionScript(mission,missionMode,configuration);configuration.onChanged=notify;
        CHECK(observed && services.gameplayGlobals->world.editor==1 && services.gameplayGlobals->world.missionPost && services.surfaceStorage->ownerMode.fort);
    });
}

// 실제 Config 알림이 중단되어도 앞선 스크립트/Native 해제를 되돌리지 않고 빈 현재의 재해제도 추가 쓰기를 만들지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_GuideReleaseKeepsPartialEffectsOnConfigNotificationFailure) {
    bool observed{}; //선택된 Config 알림 중단과 다음 실제 Editor 생성까지의 부분효과를 구별한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        CHECK(observed && !session.Core().bridgeState.mission && client.Configuration().GetInt("global.inMission")==0);
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) {
        auto& configuration=client.Configuration();configuration.Set("missionSpec","d/{local.1}.english");auto& missionMode=services.surfaceStorage->links.mission;
        auto mission=CreateGuideMissionScript(configuration,"TEST01",missionMode);CHECK(mission && mission->Loaded() && missionMode);
        const auto notify=configuration.onChanged;std::uint32_t notifications{};configuration.onChanged=[&] { //이미 실행한 원문 prefix를 확인한 뒤 선택 장치의 실제 예외를 전파한다.
            ++notifications;CHECK(!mission && !missionMode && !configuration.Registry().FindByName("mission") && configuration.GetInt("global.inMission")==0);throw CheckpointStop("guide-release-notification");
        };
        CHECK(CheckpointRejects<CheckpointStop>([&] { ReleaseGuideMissionScript(mission,missionMode,configuration); }));
        CHECK(!mission && !missionMode && notifications==1);ReleaseGuideMissionScript(mission,missionMode,configuration);CHECK(notifications==1);
        configuration.onChanged=notify;observed=true;
    });
}

// 준비 Session은 이전 Guide를 유지하고 원문 이름 비교/맵 속성 판정이 선택한 늦은 closeGuide 뒤에만 World를 연다.
TEST_CASE(CampaignExistingEditorCheckpoint_GuideLifetimeSurvivesPreparedCtorUntilSelectedLateClose) {
    std::unique_ptr<MissionScript> mission;EditorInstallWireScene link;std::uint32_t classified{},closed{},renamed{}; //실제 이전 스크립트 수명과 외곽 분기 도달 횟수다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        auto& mode=session.Core().bridgeState.mission;CHECK(mission && mode && client.Configuration().GetInt("global.inMission")==1);
        auto selected=checkpoint;std::string current="FormerCampaign"; //독립 입력인 이전 이름은 실제 파일 조회 전에 원문 setName 위치에서 선택 이름으로 바뀐다.
        selected.fort.currentName=[&]()->std::string_view { return current; };
        selected.fort.missionName=[&](auto name) { CHECK(name=="Checkpoint385" && mission && mode && !closed && !renamed);++classified;return 0; };
        selected.fort.closeGuide=[&] { CHECK(classified==1 && mission && mode && !renamed);ReleaseGuideMissionScript(mission,mode,client.Configuration());++closed; };
        selected.fort.setName=[&](auto name) { CHECK(name=="Checkpoint385" && !mission && !mode && closed==1);current=name;++renamed; };
        RunEditorWireInstallState(session,client,selected,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        CHECK(!mission && !mode && classified==1 && closed==1 && renamed==1 && client.Configuration().GetInt("global.inMission")==0);
    },true,&link,{},[&](Client& client,RawCampaignSessionServices& services) {
        client.Configuration().Set("missionSpec","d/{local.1}.english");mission=CreateGuideMissionScript(client.Configuration(),"TEST01",services.surfaceStorage->links.mission);CHECK(mission && mission->Loaded());
    });
}

// 이름이 같은 원문 경로는 closeGuide를 강제로 선택하지 않고 미션 설치 분기로 빠져 일반 Editor World를 열지 않는다.
TEST_CASE(CampaignExistingEditorCheckpoint_SameNameKeepsGuideAndSelectsMissionOutside) {
    std::unique_ptr<MissionScript> mission;bool outside{}; //같은 현재 미션의 범위 밖 분기와 일반 Editor 설치 완료를 구별한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        auto selected=checkpoint;selected.fort.closeGuide=[] { throw std::logic_error("같은 이름에서 선택하면 안 되는 Guide 해제"); };
        selected.fort.outside=[&](auto branch) { CHECK(branch==ExistingEditorInstallOutside::Mission);outside=true;throw CheckpointStop("same-name-mission"); };
        CHECK(CheckpointRejects<CheckpointStop>([&] { session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work); }));
        CHECK(outside && mission && session.Core().bridgeState.mission && !session.ExistingEditorCheckpointReady());
        ReleaseGuideMissionScript(mission,session.Core().bridgeState.mission,client.Configuration()); //검사 종료의 실제 스크립트 소유권을 Client보다 먼저 반납한다.
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) {
        client.Configuration().Set("missionSpec","d/{local.1}.english");mission=CreateGuideMissionScript(client.Configuration(),"TEST01",services.surfaceStorage->links.mission);CHECK(mission && mission->Loaded());
    });
}

//같은 실제 Client/Session 설치 도우미로 저장·State3 예약과 장기 덱 소유권을 검사한다.
#include "CampaignSaveGoMainTests.inc"

// 파일 부재의 신규 자료와 같은 Client/State 확정/자동 섬 추가 큐를 검증한다. 기존 사례 본문은 보존한다.
#include "CampaignNewFortTests.inc"
#include "CampaignNewFortConfigTests.inc" // 실제 새 자료의 설정 장치가 NUL을 받는지 별도로 대조한다.

// 실제 Editor 섬 배치와 독립 미리보기 검사를 같은 장치 수명에서 실행한다.
#include "CampaignEditorIslandTests.inc"

// 실제 기존/신규 Fort의 같은 소유자에서 원본 Bitmap 생성을 검증한다.
#include "CampaignIslandBitmapTests.inc"

// 같은 실제 Editor와 Image 변환의 소유권/크기를 이어서 검증한다.
#include "CampaignEditorImageTests.inc"

// 같은 실제 Session과 드래그 미리보기의 좌표/색상/읽기 전용 계약을 확인한다.
#include "CampaignEditorIslandMotionTests.inc"

// 새 공통 UI 검사가 같은 실제 Editor 설치 helper를 동기 범위에 빌린다.
#include "CampaignEditorIslandWindowSession.inc"
