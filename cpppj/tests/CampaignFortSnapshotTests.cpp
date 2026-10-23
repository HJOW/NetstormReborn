// 전체 Fort 부모와 실제 Client 저장 관문을 작업파일/재파싱/별도 Session 재로드로 확인한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "client/ClientMain.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawFortBadgesSave.h"
#include "o/RawMissionStart.h"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <set>
#include <stdexcept>
using namespace netstorm::client;
namespace {
// 게임 계산과 저장 body를 대체하지 않고 실제 소리표/번역/설정/표시 경계만 공급한다.
struct FullSnapshotServices {
    SoundList names{OriginalEdition::Patch1078}; // Session보다 오래 사는 실제 소리표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 설치의 명시 번역 입력이다.
    std::string missionName{"HeadlessActualMission"}; // guide-only를 조건으로 쓰지 않는 현재 Mission 이름 입력이다.
    MissionStartState mission; // Client Runtime과 같은 실제 state 형식의 명시 headless 현재 입력이다.
    std::uint32_t missionReads{}; // 현재 getter가 이름 전후 두 번 호출됐는지만 관측한다.
    // actual Mission 없음도 current0으로 공급하며 서비스 자체 누락과 구별한다.
    RawCampaignSessionServices Make(bool supplyMission=true) {
        names.Initialize();RawCampaignSessionServices result; // 나머지 gameplay와 직렬화는 실제 Session에 맡긴다.
        result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.production.effects.configInt=[](std::string_view key) { if (key!="getTeleportTechFree") throw std::logic_error("전체 저장 검사의 예상밖 생산 설정");return 0; };
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
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("전체 저장 검사의 없는 UI 대상 처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("전체 저장 검사의 미선택 삭제효과"); };
        result.player.log=[](std::uint32_t,std::string_view) {};
        if (supplyMission) {
            FortMissionSaveHooks hooks; // 실제 Client에서는 현재 UberGump Runtime getter로 바꿔 주입해야 한다.
            hooks.current=[this] { ++missionReads;return 1U; };
            hooks.name=[this]()->std::optional<std::span<const std::uint8_t>> { return std::span(reinterpret_cast<const std::uint8_t*>(missionName.c_str()),missionName.size()+1); };
            hooks.field=[this](auto token,auto offset) { if (token!=1 || (offset!=0x58 && offset!=0x54)) throw std::logic_error("headless 현재 Mission 필드 범위");return offset==0x58 ? mission.sequence : mission.flags; };
            hooks.assertion=[](auto) { throw std::logic_error("전체 저장 검사의 예상밖 Mission 진단"); };result.fortMission=std::move(hooks);
        }
        return result;
    }
};
// 제품 API가 모드를 바꿔 주지 않음을 전제로 실제 분기를 명시 공급한다. generic Editor 설치 성공을 뜻하지 않는다.
void FullSnapshotFortInput(RawCampaignSession& session) {
    const auto core=session.Core(); // 아래 값들은 원본 모드 전역의 현재 검사 입력이다.
    core.ownerMode.fort=true;core.ownerMode.battle=false;core.bridgeState.mission=false;session.FortObjectSettings().mission=false;
}
// 테스트의 작업파일은 공용 build/원본/CD 밖 신규 디렉터리에만 만든다.
std::filesystem::path FullSnapshotRoot(const std::filesystem::path& root,std::string_view suffix) {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count(); // 중복 실행끼리 사용자 작업본을 덮지 않는 경로 구분자다.
    return root/"extracted/cpp-fort-snapshot-session-20261024-work/test-data"/(std::string(suffix)+'-'+std::to_string(stamp));
}
// BYTE 벡터 비교로 현재 raw 섹션과 이전 파일을 구별한다.
std::vector<std::uint8_t> FullSnapshotBytes(std::span<const std::uint8_t> bytes) { return {bytes.begin(),bytes.end()}; }
// 독립 명시 count 입력을 현재 장부 DWORD에만 쓴다. 파일이나 writer 출력을 주입하지 않는다.
void FullSnapshotBadgeInput(RawFortBadgesGlobals& badges,std::uint32_t type,std::uint32_t count) {
    auto records=badges.Records(); // 실제 장기6160BYTE 소유 배열이다.
    for (std::size_t record=0;record<kFortBadgesRecordCount;++record) { // 첫 type 일치만 바꾸는 원본 장부 입력이다.
        const auto start=record*kFortBadgesRecordBytes; // 현재 기록의 실제 stride다.
        if (Get(records,start)!=type) continue;
        for (std::size_t byte=0;byte<4;++byte) records[start+kFortBadgesCountOffset+byte]=static_cast<std::uint8_t>(count>>(byte*8)); // count 이외의 타입/ASCII/패딩은 보존한다.
        return;
    }
    throw std::logic_error("전체 저장 검사의 실제 장부 타입 입력 없음");
}
// Badges 파일의 실제 count/type/count BYTE를 제품 getter를 기대생성기로 사용하지 않고 직접 읽는다.
void CheckFullSnapshotBadges(std::span<const std::uint8_t> bytes,std::uint8_t first,std::uint8_t typed) {
    CHECK(bytes.size()==5);if (bytes.size()!=5) throw std::logic_error("전체 저장 Badges 출력 폭");
    CHECK(bytes[0]==2 && bytes[1]==0 && bytes[2]==first && bytes[3]==71 && bytes[4]==typed);
}
// Mission 이름의 WORD 길이/원문과 현재 sequence/flags를 실제 파일에서 직접 읽는다.
void CheckFullSnapshotMission(std::span<const std::uint8_t> bytes,const FullSnapshotServices& input) {
    CHECK(bytes.size()==input.missionName.size()+10);if (bytes.size()!=input.missionName.size()+10) throw std::logic_error("전체 저장 Mission 출력 폭");
    CHECK((std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8))==input.missionName.size());
    CHECK(std::equal(input.missionName.begin(),input.missionName.end(),bytes.begin()+2));
    CHECK(Get(bytes,input.missionName.size()+2)==input.mission.sequence && Get(bytes,input.missionName.size()+6)==input.mission.flags);
}
// 현재 실제 표면/spot의 빈3x3 한 곳만 선택하며 저장파일을 입력으로 돌려쓰지 않는다.
std::array<int,2> FullSnapshotPoint(const RawWorldCore& core) {
    for (int y=4;y<248;++y) { // 지도 안쪽의 각 행을 현재 실제 순서로 읽는다.
        for (int x=4;x<248;++x) { // 청크 안에서3x3이 전부 육지이고 비어 있는 위치를 찾는다.
            if (x%16>13 || y%16>13) continue;bool free=true;
            for (int dy=0;dy<3 && free;++dy) { // 건물의 실제 세 행을 확인한다.
                for (int dx=0;dx<3;++dx) { // 각 칸의 지면과 공간점유를 직접 조회한다.
                    const auto spot=core.spots[static_cast<std::size_t>(y+dy)*256+x+dx]; // 현재 동일 spot BYTE다.
                    if (!core.building.HasIsland(static_cast<float>(x+dx),static_cast<float>(y+dy)) || !(spot&6U) || (spot&0x38U)) { free=false;break; }
                }
            }
            if (free) return {x,y};
        }
    }
    throw std::logic_error("전체 저장 검사의 현재 실제 육지3x3 없음");
}
// 오류가 난 selected effect를 성공 저장으로 처리하지 않았는지 관측한다.
template<class Function> bool FullSnapshotRejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 실제 전체 부모/동일 장부/현재 Mission/Notify 파일쓰기와 별도 Session의 실제 객체 재로드를 두 번 대조한다.
TEST_CASE(CampaignFortSnapshot_WholeParentRealFileAndReload) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호 TYPE/SHP/Fort 입력은 읽기만 한다.
    ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options)); // 실제 Client의 Config/장부만 만들며 Run과 창/장치는 실행하지 않는다.
    const auto original=assets.files.Read("d/thewarbegins.fort");const auto source=FortTemplate::Parse(original,assets.game.TypeTable()); // 원본 파일과 논리 섹션의 고정 기준이다.
    const auto storage=std::make_shared<CampaignTemplateStorage>();const auto badges=client.FortBadges(); // 같은 실제 Client 장부를 복제 없이 Session에 전달한다.
    const auto root=assets.root.parent_path();EditorFiles files(FullSnapshotRoot(root,"whole-parent"),{assets.root,root/"originalCD"});const auto path=files.Prepare("WholeSnapshot",original); // 파일 장치도 Session보다 먼저 생성해 더 오래 유지한다.
    const auto& readOnly=client.ReadOnlyForts();client.Configuration().Configuration().AppendSection("END");client.Configuration().SetInt("readOnlyForts",std::bit_cast<std::int32_t>(0x80000000U)); // 실제 Config 알림이 같은 저장 DWORD를 즉시 갱신한다.
    CHECK(readOnly==0x80000000U);
    FullSnapshotServices external;auto services=external.Make();services.templateStorage=storage;services.fortBadges=badges; // 실제 선택 장치를 모두 명시한다.
    RawCampaignSession session(assets.game,assets.files,source,"thewarbegins",std::move(services));session.Load();FullSnapshotFortInput(session);
    const auto core=session.Core(); // 동일 실제 World를 조회하며 새 상태를 만들지 않는다.
    storage->InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string()); // 기존 Session 입력 뒤의 명시 작업 manager이며 제품 경로전환을 가장하지 않는다.
    session.BindFortFileSave(readOnly,files); // 실제 Client의 const DWORD 참조를 저장 부모가 호출 시점에 다시 읽는다.
    session.AddTechnology(1,"windBattery");const auto position=FullSnapshotPoint(core); // 실제 기술/World 생성의 현재 입력이다.
    const auto sid=session.World().Create(75);core.owner.Set(sid,1);session.World().Pop(sid,static_cast<float>(position[0]),static_cast<float>(position[1]),0); // 새 객체를 실제 Pool/공간/후처리에 등록한다.
    FullSnapshotBadgeInput(*badges,0,0x1234);FullSnapshotBadgeInput(*badges,71,0x5678);external.mission.sequence=7;external.mission.flags=0x89abcdefU;
    session.SnapshotFort(1);CHECK(ReadFileBytes(path)==original); // readOnly도 섹션 생성과 이미지 작성 뒤에만 파일쓰기를 억제한다.
    CHECK(!storage->Template(1).Section("Mission").empty() && storage->FileHeader(1)[0]!=0); // early skip이라면 없는 실제 생성/서명 효과다.
    std::vector<std::uint8_t> previous;std::set<std::uint32_t> moneyBits;client.Configuration().SetInt("readOnlyForts",0); // 같은 Client Config callback으로 저장 관문을 해제한다.
    CHECK(readOnly==0 && &readOnly==&client.ReadOnlyForts());
    for (std::uint32_t pass=0;pass<2;++pass) { // 서로 다른 현재 객체/장부/Mission/SP를 같은 전체 부모로 저장한다.
        core.pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(pass+6);session.SetSp(1,50000.0f+static_cast<float>(pass)); // 동일 게임 RNG를 소비하는 실제 SP쓰기를 끼워 Money와 SP가 다른 값임을 확인한다.
        FullSnapshotBadgeInput(*badges,0,0x1200U+pass+3);FullSnapshotBadgeInput(*badges,71,0x5600U+pass+9);external.mission.sequence=pass+12;external.mission.flags=0x80000000U+pass;
        const auto beforeReads=external.missionReads;session.SnapshotFort(1);CHECK(external.missionReads==beforeReads+2); // 현재 Mission은 이름 앞/뒤에서 재조회한다.
        const auto saved=ReadFileBytes(path);const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable()); // 실제 Notify가 쓴 파일만 파싱한다.
        CHECK(saved!=original && saved!=previous);CHECK(&session.FortObjectSettings().version==&storage->SerializationVersion() && session.FortObjectSettings().version==2);
        CheckFullSnapshotBadges(decoded.Section("Badges"),static_cast<std::uint8_t>(pass+3),static_cast<std::uint8_t>(pass+9));CheckFullSnapshotMission(decoded.Section("Mission"),external);
        CHECK(decoded.Section("TypeNames").size()==1+188*4 && decoded.Section("Territory").size()==120 && !decoded.Section("Deck").empty());
        CHECK(decoded.technology.complete && std::any_of(decoded.technology.items.begin(),decoded.technology.items.end(),[](const auto& item) { return item.type==75; }));
        CHECK(decoded.Section("Money").size()==4 && decoded.money && *decoded.money>=333 && *decoded.money<=1342 && *decoded.money==std::floor(*decoded.money));
        CHECK(decoded.money && *decoded.money!=session.Sp(1));moneyBits.insert(Get(decoded.Section("Money"),0)); // 금액은 현재 SP를 복사한 파일값이 아니다.
        for (const auto name:{"TypeNames","Technology","Territory","Deck","Money","Mission","Badges"}) CHECK(FullSnapshotBytes(decoded.Section(name))==FullSnapshotBytes(storage->Template(1).Section(name))); // 이미지가 같은 live buffer를 읽는다.
        const auto order=storage->SectionOrder(1);const std::array<std::string,5> tail{"Territory","Deck","Money","Mission","Badges"}; // 전체 부모 후반의 실제 노드 순서다.
        CHECK(order.size()>=tail.size() && std::equal(tail.begin(),tail.end(),order.end()-static_cast<std::ptrdiff_t>(tail.size())));
        FullSnapshotServices reloadExternal;auto reloadServices=reloadExternal.Make();reloadServices.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(saved); // 저장 파일을 별도 실제 로더 입력으로만 준다.
        RawCampaignSession reloaded(assets.game,assets.files,FortTemplate::Parse(saved,assets.game.TypeTable()),"WholeSnapshotReload",std::move(reloadServices));reloaded.Load();const auto reloadedCore=reloaded.Core();std::size_t matches{};
        for (std::uint32_t id=5;id<reloadedCore.pool.Capacity();++id) { // 새 SID를 요구하지 않고 실제 재로드된 동일 타입/좌표/owner를 읽는다.
            const auto raw=reloadedCore.pool.Slot(Sid{static_cast<std::uint16_t>(id)}); // 현재 실제 Pool의 원문 뷰다.
            if (raw[10]==75 && !(raw[11]&7U) && raw[34]==pass+6 && std::bit_cast<float>(Get(raw,14))==static_cast<float>(position[0]) && std::bit_cast<float>(Get(raw,18))==static_cast<float>(position[1])) ++matches;
        }
        CHECK(matches==1 && core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);previous=saved;
    }
    CHECK(moneyBits.size()==2);CHECK(assets.files.Read("d/thewarbegins.fort")==original); // 난수 두 결과의 관측이며 직접 RNG 상태 replay 검증으로 확대하지 않는다.
}

// Fort 모드/실제 Mission/실제 장부/파일 장치 누락을 해당 원본 위치에서만 거부하고 앞선 쓰기를 보존한다.
TEST_CASE(CampaignFortSnapshot_SelectedMissingBoundariesKeepPartialWrites) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 각 경우의 보호 기준이다.
    for (std::uint32_t missing=0;missing<4;++missing) { //0=Fort모드,1=Mission,2=Badges,3=FileSave의 명시 누락 입력이다.
        FullSnapshotServices external;auto services=external.Make(missing!=1);const auto storage=std::make_shared<CampaignTemplateStorage>();services.templateStorage=storage;
        if (missing!=2) services.fortBadges=std::make_shared<RawFortBadgesGlobals>();
        const auto root=assets.root.parent_path();EditorFiles files(FullSnapshotRoot(root,"missing-"+std::to_string(missing)),{assets.root,root/"originalCD"});const auto path=files.Prepare("PartialSnapshot",original);std::uint32_t readOnly{}; // 장치와 관문은 Session보다 오래 유지한다.
        RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"thewarbegins",std::move(services));session.Load();FullSnapshotFortInput(session);const auto core=session.Core();
        storage->InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string()); // Notify가 선택되면 저장 가능한 현재 헤더/장치 입력이다.
        if (missing!=3) session.BindFortFileSave(readOnly,files);
        if (!missing) core.ownerMode.fort=false;
        const auto before=storage->SectionOrder(1);CHECK(FullSnapshotRejects([&] { session.SnapshotFort(1); }));CHECK(ReadFileBytes(path)==original);
        if (!missing) CHECK(storage->SectionOrder(1)==before); // 모드 진단은 첫 child 앞에서 중단해야 한다.
        else {
            CHECK(storage->Template(1).Section("Money").size()==4 && storage->SerializationVersion()==2); // Mission/장부/파일 누락을 upfront검사로 바꾸지 않는다.
            if (missing==1) CHECK(storage->Template(1).Section("Mission").empty() && FullSnapshotBytes(storage->Template(1).Section("Badges"))==FullSnapshotBytes(FortTemplate::Parse(original,assets.game.TypeTable()).Section("Badges")));
            if (missing==2) CHECK(FullSnapshotBytes(storage->Template(1).Section("Badges"))==std::vector<std::uint8_t>{0}); // Create/초기countBYTE뒤 첫 실제 getter에서만 누락을 거부한다.
            if (missing==3) CHECK(storage->Template(1).Section("Mission").size()==external.missionName.size()+10 && FullSnapshotBytes(storage->Template(1).Section("Badges"))==std::vector<std::uint8_t>{0}); // 마지막 Notify의 실제 저장 누락까지 본문은 유지한다.
        }
    }
    CHECK(assets.files.Read("d/thewarbegins.fort")==original);
}

// 실제 현재 Mission0은 명시된 정상 입력이며 장치 누락과 달리 빈 Mission을 만들고 Notify를 계속한다.
TEST_CASE(CampaignFortSnapshot_ExplicitNoMissionAndPreventSave) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort"); // 보호 입력은 읽기만 한다.
    FullSnapshotServices external;auto services=external.Make(false);FortMissionSaveHooks mission;std::uint32_t currentReads{}; // 존재0일 때 다른 Mission 효과는 요구하지 않는 명시 입력이다.
    mission.current=[&] { ++currentReads;return 0U; };services.fortMission=std::move(mission);
    const auto storage=std::make_shared<CampaignTemplateStorage>();services.templateStorage=storage;services.fortBadges=std::make_shared<RawFortBadgesGlobals>(); // 실제 저장소와 정상 빈 장부다.
    const auto root=assets.root.parent_path();EditorFiles files(FullSnapshotRoot(root,"no-mission"),{assets.root,root/"originalCD"});const auto path=files.Prepare("NoMissionSnapshot",original);std::uint32_t readOnly{}; // 같은 장치/관문이 Session보다 오래 유지된다.
    RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"thewarbegins",std::move(services));session.Load();FullSnapshotFortInput(session);
    storage->InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string());storage->Headers()[1*20+16]=1; // 같은 실제 preventSave DWORD의 현재값1이다.
    session.SnapshotFort(1);CHECK(currentReads==1 && ReadFileBytes(path)==original); // 파일 장치 미설치라도 원본 preventSave가 마지막에 자연 반환한다.
    CHECK(storage->Template(1).Section("Mission").empty() && storage->Template(1).Section("Money").size()==4 && FullSnapshotBytes(storage->Template(1).Section("Badges"))==std::vector<std::uint8_t>{0});
    storage->Headers()[1*20+16]=0;session.BindFortFileSave(readOnly,files); // 같은 현재 헤더를 풀고 실제 파일 장치에 연결한다.
    session.SnapshotFort(1);const auto saved=ReadFileBytes(path);const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable()); // 이번 정상 반환은 실제 파일 재읽기로 별도 확인한다.
    CHECK(currentReads==2 && saved!=original && decoded.Section("Mission").empty() && decoded.Section("Money").size()==4 && FullSnapshotBytes(decoded.Section("Badges"))==std::vector<std::uint8_t>{0});
    CHECK(assets.files.Read("d/thewarbegins.fort")==original);
}
