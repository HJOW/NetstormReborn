// 실제 Client/Storage의 기존Fort World·Deck 단계만 검사하며 전체 Editor 완료나 메뉴 실행으로 표현하지 않는다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "client/FortTypeConversionStorage.h"
#include "client/RawCampaignSession.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawFortBadgesSave.h"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace {
// 보호thewarbegins의독립Python 판독으로 고정한26기술이다. Session/Contents reader의 결과로 기대를 만들지 않는다.
constexpr std::array<std::string_view,26> kEditorSourceTechnology{
    "sunArcher","sunAviary","windBattery","rainBattery","thunderBattery","sunBlocker","thunderBlocker","sunBalloon",
    "sunCannon","rainCannon","thunderCannon","sunWalker","bulf","sunFence","windWalker","windAviary","windArcher",
    "windBalloon","windBlocker","rainAviary","rainFence","rainBalloon","rainBlocker","thunderArcher","thunderFence","rainWalker"};

// 예상한 계약 예외만 성공으로 분류하여 다른 오류로 경계 검사를 통과시키지 않는다.
template<class Exception,class Operation> bool EditorRejects(Operation operation) {
    try { operation(); }catch (const Exception&) { return true; }catch (...) { return false; }return false;
}
// 원문2BYTE header와 길이WORD만 직접 따라가 입력의 고정 섹션을 빌린다. 제품 Fort reader를 기대 생성기로 쓰지 않는다.
std::span<const std::uint8_t> EditorOriginalSection(std::span<const std::uint8_t> original,std::size_t section) {
    if (original.size()<2 || original[0]!='F') throw std::logic_error("기존Editor 보호Fort header 입력");
    std::size_t position=2; // 고정F/flag 뒤 첫 길이WORD의 위치다.
    for (std::size_t index=0;index<=section;++index) { // 필요한 원문 섹션까지 실제 길이만 소비한다.
        if (position>original.size() || original.size()-position<2) throw std::out_of_range("기존Editor 원문 길이WORD");
        const auto length=std::size_t(original[position])|(std::size_t(original[position+1])<<8); // 현재 입력의 길이에는WORD 자체가 포함된다.
        if (length<2 || length>0x7fff || length>original.size()-position) throw std::out_of_range("기존Editor 원문 섹션 폭");
        if (index==section) return original.subspan(position+2,length-2);
        position+=length;
    }
    throw std::logic_error("기존Editor 원문 섹션 선택");
}
// 반복 실행마다 원본/CD와 다른 새 작업 디렉터리를 사용하며 기존 사용자 파일을 덮지 않는다.
std::filesystem::path EditorWorkRoot(const std::filesystem::path& root,std::string_view name) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count(); // 해당 검사 실행만의 경로 구분자다.
    return root/"extracted/cpp-existing-editor-20261024-work/test-data"/(std::string(name)+'-'+std::to_string(stamp));
}
// 장치 경계만 공급한다. Player/지형/회복/Money/Badges/Contents/Deck 계산은 실제 Session을 사용한다.
struct EditorDevices {
    SoundList names{OriginalEdition::Patch1078}; // Session보다 먼저 생성하고 나중에 반납할 실제 소리표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 선택된 번역의 명시 입력이다.
    std::uint32_t technologyUi{},spRefreshes{},recoveryLogs{},missionReads{}; // 실제 외부 호출의 횟수만 관측하며 게임 결과는 만들지 않는다.
    RawCampaignSessionServices Make(Client& client,const std::shared_ptr<CampaignTemplateStorage>& storage,const std::vector<std::uint8_t>& original) { // 같은 Client 소유자와 원문을 생성자 입력에 연결한다.
        names.Initialize();RawCampaignSessionServices result; // 나머지 선택되지 않은 경계는 제품의 명시 실패를 유지한다.
        result.installMode=CampaignInstallMode::ExistingEditor;result.templateStorage=storage;
        result.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(original);
        result.fortBadges=client.FortBadges();result.fortTypeConversion=client.FortTypeConversion();result.soundNames=&names;
        result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; // 실제 미초기화 할당에 대한 명시 검사 입력이다.
        result.production.effects.configInt=[](std::string_view key) { if (key!="getTeleportTechFree") throw std::logic_error("기존Editor 검사 생산 설정");return 0; };
        result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("기존Editor 검사에 없는 UI 처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("기존Editor 검사의 미선택 삭제효과"); };
        result.refreshSp=[this] { ++spRefreshes; };result.fortTechnologyUi=[this] { ++technologyUi; };
        result.player.log=[this](auto kind,std::string_view text) { if (kind!=29 || text!="Territory recovered from Battle!\n") throw std::logic_error("기존Editor 검사의 예상밖 로그");++recoveryLogs; };
        FortMissionSaveHooks mission;mission.current=[this] { ++missionReads;return 0U; };result.fortMission=std::move(mission); // 명시 Mission0과 최종loaded 전 Snapshot 거부를 구별한다.
        return result;
    }
};
// 현재 Player의 raw Form6/mask4 체인을 직접 읽어 생성된 기술 종류와 중복을 관측한다.
std::set<std::uint32_t> EditorActualTechnology(RawCampaignSession& session) {
    const auto core=session.Core();const auto parent=session.CurrentTechnologyParent(); // 같은 실제 Pool과 현재 기술 부모를 조회한다.
    if (!parent || parent>=core.pool.Capacity()) throw std::logic_error("기존Editor 실제 기술 부모");
    std::set<std::uint32_t> result;std::set<std::uint32_t> visited; // 종류 중복과 raw 체인 순환을 별도로 검출한다.
    auto next=Get(core.pool.Slot(Sid{static_cast<std::uint16_t>(parent)}),6,2); // 실제 부모의 종속 head WORD다.
    while (next) { // 생성 결과의 현재 next만 따라가며 기대 목록을 이 경로에서 만들지 않는다.
        if (next>=core.pool.Capacity() || !visited.insert(next).second) throw std::logic_error("기존Editor 기술 체인 손상");
        const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(next)});next=Get(raw,4,2); // 같은 실제 종속 슬롯의 다음WORD다.
        if (raw[10]!=6 || !(Get(raw,20,2)&4U)) continue;
        CHECK(Get(raw,20,2)==4 && Get(raw,12,2)==0 && raw[30]==0);
        CHECK(result.insert(Get(raw,18,2)).second);
    }
    return result;
}
// 독립 고정 이름 목록을 보호 TYPE의 실제 번호로만 바꾼다. 파일 reader의 변환표나 parsed item.type은 사용하지 않는다.
std::set<std::uint32_t> EditorExpectedTechnology(const netstorm::client::GameAssets& game) {
    std::set<std::uint32_t> result; // 고정26종의 기대 집합이다.
    for (const auto name:kEditorSourceTechnology) { // 보호 TYPE의 이름/번호 관계만 빌린다.
        const auto type=game.TypeTable().Find(name);if (type<=0) throw std::logic_error("기존Editor 기대 기술 TYPE 없음");result.insert(static_cast<std::uint32_t>(type));
    }
    return result;
}
// 공개 Snapshot은 전체 외곽 완료 전 거부되어야 하며 장치 누락 때문에 실패한 것으로 오인하지 않는다.
void EditorSnapshotStillUnavailable(RawCampaignSession& session,CampaignTemplateStorage& storage,EditorDevices& devices) {
    const auto order=storage.SectionOrder(1);const auto reads=devices.missionReads; // 공개 진입 전의 실제 섹션 순서/외부 getter 관측이다.
    CHECK(EditorRejects<std::logic_error>([&] { session.SnapshotFort(1); }));
    CHECK(storage.SectionOrder(1)==order && devices.missionReads==reads);
}
}

// 생성자부터 실제Fort모드를 쓰고 첫Player·저장HP·고정SP·26기술과 공유Client 장부를 적용한다.
TEST_CASE(CampaignExistingEditor_FirstWorldModesPlayerHpSpAndTechnology) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 보호 원문은 읽기만 한다.
    ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); // Run 없이 실제 Client의 장부/변환표 수명만 생성한다.
    auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles files(EditorWorkRoot(assets.root.parent_path(),"first-world"),{assets.root,assets.root.parent_path()/"originalCD"}); // 두 소유자는 Session보다 오래 산다.
    const auto path=files.Prepare("ExistingFirstWorld",original);EditorDevices devices; // 실제 작업본과 장치는 Session 생성 전에 준비한다.
    const auto badgeAddress=client.FortBadges()->Records().data();const auto conversionAddress=&client.FortTypeConversion()->Current(); // 같은 Client 저장소 주소를 보존한다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),path.string(),devices.Make(client,storage,original));
    session.BindFortFileSave(client.ReadOnlyForts(),files);const auto core=session.Core(); // 실제 장치 참조도 Session보다 오래 유지한다.
    CHECK(storage->Name(1)==path.string() && core.pool.FreeCount()==core.pool.Capacity()-5 && session.CurrentTechnologyParent()==0);
    CHECK(session.Editor()==1 && core.ownerMode.fort && !core.ownerMode.battle && !core.ownerMode.challenge && !core.bridgeState.battle && !core.bridgeState.mission);
    CHECK(session.FortObjectSettings().special && session.FortObjectSettings().fort && !session.FortObjectSettings().mission && session.FortObjectSettings().chaffMode && session.FortObjectSettings().FortDestroyBlocked());
    CHECK(EditorRejects<std::logic_error>([&] { session.LoadExistingEditorDeck(); }));EditorSnapshotStillUnavailable(session,*storage,devices);
    session.LoadExistingEditorWorld();
    CHECK(session.PlayerKnown()==1 && session.LocalOwner()==1 && session.TemplateOwner()==1 && session.CurrentTechnologyParent()==5);
    const auto player=core.pool.Slot(Sid{5}); // 원본 Player.Create(type152,flags2)의 첫 클라이언트 슬롯이다.
    CHECK(player[10]==152 && player[34]==1 && !(player[11]&5U) && Get(session.PlayerRecords(),0xb4+0x9c)==5);
    CHECK(session.Sp(1)==194162.0F); // 원본10.78 Fort Money 정책값이며 저장Money float100000의 복사가 아니다.
    std::size_t priests{},players{};std::set<std::uint8_t> priestOwners; // 보호thewarbegins의 독립 집계는 사제2개이며 현재Player는1개다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) { // 실제 살아 있는 원문 슬롯만 관측한다.
        const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(id)});if (raw[11]&7U) continue;
        if (raw[10]==152) ++players;
        if (raw[10]==158) { ++priests;priestOwners.insert(raw[34]);CHECK(Get(raw,26)==static_cast<std::uint32_t>(assets.game.TypeTable().Types()[158].maxHitPoints)); }
    }
    // 원문 고정 집계와 다를 때 실제 개수와 소유자만 출력하며 기대값이나 살아 있는 슬롯을 바꾸지 않는다.
    if (priests!=2 || players!=1 || priestOwners!=std::set<std::uint8_t>({1,2})) std::fprintf(stderr,"ExistingEditor actual priests=%zu players=%zu owners=%zu owner1=%d owner2=%d\n",priests,players,priestOwners.size(),priestOwners.contains(1),priestOwners.contains(2));
    CHECK(priests==2 && players==1 && priestOwners==std::set<std::uint8_t>({1,2}));
    CHECK(EditorActualTechnology(session)==EditorExpectedTechnology(assets.game) && devices.technologyUi==1);
    CHECK(client.FortBadges()->Records().data()==badgeAddress && &client.FortTypeConversion()->Current()==conversionAddress && *conversionAddress==0);
    CHECK(ReadFortBadgeCount(client.FortBadges()->Records(),71)==0 && session.FortObjectSettings().loadingDepth==0);
    CHECK(ReadFileBytes(path)==original && assets.files.Read("d/thewarbegins.fort")==original);EditorSnapshotStillUnavailable(session,*storage,devices);
}

// 원문으로 만든 작업manager의 명시편집 입력을 회복하고 같은 장부에 Badges를 쓰며188TYPE 전체필터의 FullDeck를 만든다.
TEST_CASE(CampaignExistingEditor_WorkingTerritoryBadgesAndFullDeck) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 원문 바이트/보호 TYPE가 독립 기대의 기준이다.
    ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); // 실제 공유 장부/변환표를 생성하며 Run은 실행하지 않는다.
    auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles files(EditorWorkRoot(assets.root.parent_path(),"working-recovery"),{assets.root,assets.root.parent_path()/"originalCD"}); // 작업본 소유자를 Session보다 먼저 생성한다.
    const auto path=files.Prepare("ExistingRecovery",original);EditorDevices devices; // 이 파일과 원본은 이후 메모리편집에도 아직 원문 상태다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),path.string(),devices.Make(client,storage,original));session.BindFortFileSave(client.ReadOnlyForts(),files);
    const auto territory=EditorOriginalSection(original,6),originalState=EditorOriginalSection(original,1); // 제품 회복 reader가 아닌 원문 길이표를 직접 읽는다.
    if (territory.size()!=120 || originalState.size()!=28) throw std::logic_error("기존Editor 보호Territory/State 폭");
    const auto stream=storage->SelectSection(1,"Territory",1); // 정상Storage API로 미저장 작업manager의 세flags만 명시편집한다.
    storage->AccessSection(stream,[&](auto fields,const auto& memory) { auto bytes=memory.buffer(Get(fields,0));for (const std::size_t record:{1U,7U,19U}) bytes[record*6+1]|=4U; }); // 실제빌림 안에서만 현재data에 쓴다.
    storage->State(1).bytes[0]|=4U; // 같은 작업 State의 회복 입력이며 Session 모드/성공값을 강제하지 않는다.
    const std::array<std::uint8_t,5> badgeInput{2,71,23,75,9}; // 원문 Badges0 대신 검사할 두 setter의 명시 작업본 입력이다.
    storage->WriteSection(storage->CreateSection(1,"Badges"),badgeInput);
    const auto badgeAddress=client.FortBadges()->Records().data();const std::vector<std::uint8_t> beforeBadges(client.FortBadges()->Records().begin(),client.FortBadges()->Records().end()); // 실제 장부 전체의 비count필드 보존 기준이다.
    session.LoadExistingEditorWorld();
    const auto recovered=storage->Template(1).Section("Territory"); // 제품 결과는 독립 원문120BYTE와 대조한다.
    CHECK(recovered.size()==territory.size() && std::equal(territory.begin(),territory.end(),recovered.begin()) && devices.recoveryLogs==3);
    CHECK(storage->State(1).bytes.size()==originalState.size() && std::equal(originalState.begin(),originalState.end(),storage->State(1).bytes.begin()));
    CHECK(client.FortBadges()->Records().data()==badgeAddress && ReadFortBadgeCount(client.FortBadges()->Records(),71)==23 && ReadFortBadgeCount(client.FortBadges()->Records(),75)==9);
    for (std::size_t record=0;record<kFortBadgesRecordCount;++record) { // 두 countDWORD 이외의 모든 실제6160BYTE가 보존되는지 확인한다.
        const auto offset=record*kFortBadgesRecordBytes;const auto type=Get(beforeBadges,offset);const auto count=type==71 ? 23U : type==75 ? 9U : Get(beforeBadges,offset+kFortBadgesCountOffset); // getter 결과가 아닌 입력 장부/명시 두 값을 기대한다.
        CHECK(Get(client.FortBadges()->Records(),offset+kFortBadgesCountOffset)==count);
        CHECK(std::equal(beforeBadges.begin()+static_cast<std::ptrdiff_t>(offset),beforeBadges.begin()+static_cast<std::ptrdiff_t>(offset+kFortBadgesCountOffset),badgeAddress+offset));
    }
    session.LoadExistingEditorDeck();const auto core=session.Core();const auto deckToken=Get(session.PlayerRecords(),0xb4+0x68); // 실제 현재Player가 소유한 같은 Deck 포인터다.
    const auto deck=session.Production().Deck().Bytes(deckToken);std::vector<std::uint32_t> expected,actual; // 기대는 보호TYPE 필드, 관측은 현재 실제352BYTE에서 별도로 읽는다.
    CHECK(core.types.size()==188);
    for (std::uint32_t type=0;type<assets.game.TypeTable().Types().size();++type) { // 원문 FullDeck가188개 TYPE 모두에 적용하는 선택 규칙이다.
        const auto& record=assets.game.TypeTable().Types()[type];if (record.group!=10 || (record.flags2&0x100U) || type==154 || type==122) expected.push_back(type);
    }
    const auto count=Get(deck,332);std::uint32_t chanceSum{}; // 원문count/chance DWORD와 고정5BYTE entry를 직접 읽는다.
    CHECK(count==expected.size() && count<=66 && Get(deck,340)==0);
    if (count>66) throw std::logic_error("기존Editor FullDeck 관측 물리범위");
    for (std::uint32_t index=0;index<count;++index) { // 활성power/남은수/미정아닌 entry필드를 모두 대조한다.
        const auto offset=std::size_t(index)*5;actual.push_back(deck[offset]);chanceSum+=deck[offset+1];CHECK(deck[offset+1]<=4 && deck[offset+2]==1 && deck[offset+3]==255 && deck[offset+4]==0);
    }
    CHECK(actual==expected && Get(deck,336)==chanceSum && client.FortTypeConversion()->Current()==0 && session.FortObjectSettings().loadingDepth==0);
    const std::vector<std::uint8_t> poolBefore(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto order=storage->SectionOrder(1); // 완료한 World의 재진입이 생성/회복을 반복하지 않는지 보존한다.
    CHECK(EditorRejects<std::logic_error>([&] { session.LoadExistingEditorWorld(); }));CHECK(std::equal(poolBefore.begin(),poolBefore.end(),core.pool.Bytes().begin()) && storage->SectionOrder(1)==order && devices.recoveryLogs==3);
    CHECK(ReadFileBytes(path)==original && assets.files.Read("d/thewarbegins.fort")==original);EditorSnapshotStillUnavailable(session,*storage,devices);
}

// 생성자 소유자 누락과 기술UI의 실제 선택 실패를 구별하고 후자의 Player/SP/기술/회복 부분효과를 되돌리지 않는다.
TEST_CASE(CampaignExistingEditor_MissingOwnerAndUiKeepPartialEffects) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 모든 누락 경우가 같은 보호 원문으로 시작한다.
    ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); // 두 독립 Session은 같은 실제 Client 장부/변환표를 차례로 빌린다.
    {
        auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles files(EditorWorkRoot(assets.root.parent_path(),"missing-owner"),{assets.root,assets.root.parent_path()/"originalCD"}); // 실패한 생성자 뒤에도 작업manager가 남는 소유 수명이다.
        const auto path=files.Prepare("MissingConversion",original);EditorDevices devices;auto services=devices.Make(client,storage,original);services.fortTypeConversion.reset(); // 필요한 소유자만 명시 누락한다.
        CHECK(EditorRejects<std::logic_error>([&] { RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),path.string(),std::move(services)); }));
        CHECK(storage->OwnedImages()==1 && storage->Name(1)==path.string() && ReadFileBytes(path)==original && client.FortTypeConversion()->Current()==0); // child factory 앞 파일/manager 설치효과는 남는다.
    }
    auto storage=std::make_shared<CampaignTemplateStorage>();EditorFiles files(EditorWorkRoot(assets.root.parent_path(),"missing-ui"),{assets.root,assets.root.parent_path()/"originalCD"}); // 장치/저장소 모두 두번째 Session보다 오래 산다.
    const auto path=files.Prepare("MissingTechnologyUi",original);EditorDevices devices;auto services=devices.Make(client,storage,original);services.fortTechnologyUi={}; // UI만 실제도달시점에 누락시킨다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),path.string(),std::move(services));session.BindFortFileSave(client.ReadOnlyForts(),files);
    CHECK(EditorRejects<std::logic_error>([&] { session.LoadExistingEditorWorld(); }));const auto core=session.Core(); // 첫World가 기술UI에서 중단한 실제 부분상태다.
    CHECK(session.CurrentTechnologyParent()==5 && session.Sp(1)==194162.0F && EditorActualTechnology(session)==EditorExpectedTechnology(assets.game));
    CHECK(client.FortTypeConversion()->Current()==0 && session.FortObjectSettings().loadingDepth==1 && devices.technologyUi==0); // 기술wrapper의cleanup 뒤 외곽BeginLoad 깊이만 남는다.
    const auto territory=EditorOriginalSection(original,6),recovered=storage->Template(1).Section("Territory"); // 앞선 회복은 UI 실패에도 유지된다.
    CHECK(recovered.size()==territory.size() && std::equal(territory.begin(),territory.end(),recovered.begin()));
    const std::vector<std::uint8_t> poolBefore(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto order=storage->SectionOrder(1); // 재호출로 부분효과를 중복시키지 않아야 한다.
    CHECK(EditorRejects<std::logic_error>([&] { session.LoadExistingEditorWorld(); }));CHECK(EditorRejects<std::logic_error>([&] { session.LoadExistingEditorDeck(); }));
    CHECK(std::equal(poolBefore.begin(),poolBefore.end(),core.pool.Bytes().begin()) && storage->SectionOrder(1)==order && session.FortObjectSettings().loadingDepth==1);
    EditorSnapshotStillUnavailable(session,*storage,devices);CHECK(ReadFileBytes(path)==original && assets.files.Read("d/thewarbegins.fort")==original);
}
