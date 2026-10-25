// 실제 Client 이름/목록·Template manager·Notify 파일 저장과 작업본 삭제의 결합을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientFortGlobals.h"
#include "client/FortCurrentNameDevices.h"
#include "client/FortSelection.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <chrono>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 각 검사는 보호 원본을 읽고 자기 작업 파일만 만든다.
// 현재 실제 자산 경로와 창을 열지 않는 Client 입력을 만든다.
ClientOptions NameOptions() { ClientOptions options;options.gameDirectory=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).root;options.noAudio=true;return options; }
// NUL 포함 안정된 문자열 주소를 실제 원문 부모에 전달한다.
std::span<const std::uint8_t> NameInput(const std::string& value) { return {reinterpret_cast<const std::uint8_t*>(value.c_str()),value.size()+1}; }
// 실제 저장/선택 경계의 예상 예외만 결과와 구별한다.
template<class Function> bool NameRejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
// 게임 Snapshot 전체의 외부 요청은 관측 장치로 구별하고 manager/Notify/파일은 실제 본체를 사용한다.
struct NameScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호 파일과 TYPE를 읽는 기존 소유자다.
    Client client{NameOptions()};ClientFortGlobals& names=*client.FortGlobals();CampaignTemplateStorage storage; // 실제 Client와 같은 이름/목록 및 단일 manager 저장소다.
    std::filesystem::path root=assets.root.parent_path()/"cpppj/build/test-data"/("fort-name-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); // 기존 작업본과 겹치지 않는 검사 소유 경로다.
    EditorFiles files{root,{assets.root,assets.root.parent_path()/"originalCD"}}; // 쓰기/삭제 모두 같은 보호 namespace를 쓴다.
    std::uint32_t owner{1},fort{1},editor{1},mission{},depth{},readOnly{},saveCount{},notifyCount{},titles{};bool inMission{}; // 원문 외부 모드/장치의 명시 검사 입력과 관측이다.
    std::array<std::uint8_t,9> hidden{};std::vector<std::string> snapshots;std::vector<std::uint32_t> assertions; // 별도 snapshot 경계 요청과 실제 진단만 기록한다.
    RawPlayerTemplateState state{OriginalEdition::Patch1078,storage.Headers(),[this](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },
        [this](auto slot)->const std::string& { return storage.Name(slot); },depth,inMission,[](auto) { throw std::logic_error("이름 결합의 예상 밖 State 진단"); },[](auto) { throw std::logic_error("PATCH 이름 결합의 CD 경계"); }}; // 동일 manager 이름을 실제 Notify가 다시 읽는다.
    // 실제 Config/State/저장 장치만 연결하며 아직 파일/manager는 만들지 않는다.
    NameScene() {
        client.Configuration().Set("fortSpec","d/{local.1}.fort");client.Configuration().Set("SOLOSpec","d/{local.1}.solo");client.Configuration().SetInt("mayEditMissions",0);
        state.BindMinimumSize([this](auto slot,auto size)->PlayerTemplateStateStream& { return storage.EnsureStateSize(slot,size); });
        state.BindFileSave([this](auto slot) { ++saveCount;storage.SaveFile(slot,{readOnly,owner,hidden},{[] { return 188U; },[](auto) { throw std::logic_error("이름 저장의 이미지 진단"); }},files.Writer()); });
    }
    // 이름 부모의 현재 getter는 호출마다 같은 명시 모드/owner를 읽는다.
    FortCurrentNameGlobals Globals() { return {[this] { return owner; },[this] { return fort; },[this] { return editor; },[this] { return mission; },names.Catalog().dirty}; }
    // snapshot 관측은 게임 전체 저장의 증거가 아니며 Notify는 실제 부모/파일 writer를 호출한다.
    FortCurrentNameHooks Hooks() {
        FortCurrentNameHooks hooks;hooks.snapshot=[this] { snapshots.emplace_back(names.Current()); };hooks.notify=[this](auto slot) { ++notifyCount;state.Changed(slot); };
        hooks.refreshTitle=[this] { ++titles; };hooks.assertion=[this](auto line) { assertions.push_back(line); };hooks.state=[this](auto slot)->std::span<const std::uint8_t> { return state.Get(slot); };return hooks;
    }
    // 현재 이름 버퍼의 명시 입력만 준비한다. Snapshot/dirty/설정을 선행 실행하지 않는다.
    void Current(const std::string& value) { SetPendingFortName(names.CurrentBytes(),NameInput(value)); }
    // 실제 TEST01 작업본과 같은 manager/State를 설치하고 빌린 슬롯의 같은 이름도 관측한다.
    std::filesystem::path Install(const std::string& name) {
        const auto bytes=assets.files.Read("d/TEST01.fort");const auto path=files.Prepare(name,bytes);storage.InstallFile(owner,FortTemplate::Parse(bytes,assets.game.TypeTable()),bytes,path.string());storage.Borrow(8,owner);Current(name);return path;
    }
};
}

// 두 버퍼는 같은 Client에서 고정되고 진단 뒤 실제 source와 전방 겹침/부분 쓰기를 보존한다.
TEST_CASE(FortNameDevices_ClientBuffersAndPendingPhysicalCopy) {
    NameScene scene;CHECK(scene.client.FortGlobals().get()==&scene.names && scene.names.Current().empty() && scene.names.Pending().empty() && scene.names.Catalog().dirty==1);const auto current=scene.names.CurrentBytes().data(),pending=scene.names.PendingBytes().data();CHECK(current!=pending);
    scene.Current("Current");const std::string input="Pending";SetPendingFortName(scene.names.PendingBytes(),NameInput(input));CHECK(scene.names.Current()=="Current" && scene.names.Pending()=="Pending");
    std::array<std::uint8_t,8> source{'B','a','d','.',0,0x71,0x72,0x73};std::uint32_t diagnostics{};
    SetPendingFortName(scene.names.PendingBytes(),source,[&] { ++diagnostics;source[0]='O';source[1]='K';source[2]=0; });CHECK(diagnostics==1 && scene.names.Pending()=="OK");
    SetPendingFortName(scene.names.PendingBytes(),scene.names.PendingBytes());CHECK(scene.names.Pending()=="OK" && current==scene.names.CurrentBytes().data() && pending==scene.names.PendingBytes().data());
    std::array<std::uint8_t,2> shortTarget{0x71,0x72};CHECK(NameRejects([&] { SetPendingFortName(shortTarget,NameInput(input)); }) && shortTarget[0]=='P' && shortTarget[1]=='e');
    scene.names.FileSpec().SelectSolo();const auto key=scene.names.FileSpec().Current().data();scene.names.SelectFileSpec();CHECK(scene.names.SelectedFileSpec()=="SOLOSpec" && key==scene.names.FileSpec().Current().data());
}

// 실제 현재 이름 변경의 dirty는 다음 목록에서 오래된 캐시를 반납하고 작업 디스크를 다시 읽게 한다.
TEST_CASE(FortNameDevices_CurrentSelectionInvalidatesSameCatalog) {
    NameScene scene;const auto original=scene.assets.files.Read("d/TEST01.fort");scene.files.Prepare("Visible",original);BaseFileSystem source(scene.root);
    SoloMapCatalogEffects effects;effects.typeCount=[] { return 188; };effects.allocationBytes=[](auto) { throw std::logic_error("정상 작업 맵의 빈 State"); };effects.userName=[] { return std::string{}; };effects.translate=[](auto text) { return std::string(text); };effects.fileSpecKey=[&] { return scene.names.FileSpec().Current(); };effects.listState=&scene.names.Catalog();
    SoloMapCatalog catalog(source,scene.client.Configuration(),std::move(effects));auto& cache=scene.names.Catalog();cache.names[0]="Cached";cache.count=1;cache.dirty=0;CHECK(&catalog.Cached()==&cache && catalog.Refresh(2)==1 && cache.names[0]=="Cached"); // 이미 정상인 캐시를 명시 검사 입력으로 만든다.
    FortCurrentNameDevices devices(scene.names,scene.storage,scene.client.Configuration(),scene.Globals(),scene.Hooks(),&scene.files);const std::string selected="Selected";devices.Set(NameInput(selected));
    CHECK(scene.names.Current()==selected && cache.dirty==1 && scene.titles==1 && scene.snapshots.empty() && !scene.notifyCount && !scene.storage.OwnedImages());CHECK(scene.client.Configuration().Configuration().Get("lastEditFort")==selected);
    CHECK(catalog.Refresh(2)==1 && cache.names[0]=="Visible" && !cache.dirty && scene.names.Current()==selected);
}

// 같은 manager/빌린 슬롯/기존 stream은 새 이름에도 유지되고 실제 Notify가 새 파일에 저장한 뒤 이전 파일만 삭제한다.
TEST_CASE(FortNameDevices_WritableManagerNotifyRenameAndDelete) {
    NameScene scene;const auto old=scene.Install("Old");const auto manager=scene.storage.Headers()[scene.owner*20];const auto stream=scene.storage.CreateSection(scene.owner,"Temp");scene.storage.WriteSection(stream,std::array<std::uint8_t,2>{0x31,0x32});
    const auto before=ReadFileBytes(old);auto bytes=scene.state.Get(scene.owner);bytes[12]^=0x53;const auto marker=bytes[12];FortCurrentNameDevices devices(scene.names,scene.storage,scene.client.Configuration(),scene.Globals(),scene.Hooks(),&scene.files);
    const std::string selected="New";devices.Set(NameInput(selected));const auto destination=scene.files.ResolvePath("d/New.fort");
    CHECK(!std::filesystem::exists(old) && std::filesystem::is_regular_file(destination) && scene.storage.OwnedImages()==1 && scene.storage.Headers()[scene.owner*20]==manager);CHECK(scene.storage.Name(scene.owner)==destination.string() && scene.storage.Name(8)==destination.string());
    CHECK((scene.snapshots==std::vector<std::string>{"Old","New"}) && scene.notifyCount==1 && scene.saveCount==1 && scene.titles==1 && scene.assertions.empty());CHECK(scene.client.Configuration().Configuration().Get("lastEditFort")==selected && scene.names.Catalog().dirty==2);
    const auto saved=ReadFileBytes(destination);CHECK(saved!=before && FortTemplate::Parse(saved,scene.assets.game.TypeTable()).Section("State")[12]==marker);scene.storage.WriteSection(stream,std::array<std::uint8_t,1>{0x33});CHECK(scene.storage.FindSection(8,"Temp").generation==stream.generation);
}

// 저장된 isMine은 첫 snapshot과 이전 파일 삭제만 생략하고 같은 manager의 새 이름/Notify/후속 snapshot을 유지한다.
TEST_CASE(FortNameDevices_ReadonlyOldFileIsRetained) {
    NameScene scene;const auto old=scene.Install("Kept");const auto original=ReadFileBytes(old);scene.storage.Headers()[scene.owner*20+8]=1;FortCurrentNameDevices devices(scene.names,scene.storage,scene.client.Configuration(),scene.Globals(),scene.Hooks(),&scene.files);
    const std::string selected="Copy";devices.Set(NameInput(selected));CHECK(ReadFileBytes(old)==original && std::filesystem::is_regular_file(scene.files.ResolvePath("d/Copy.fort")));CHECK((scene.snapshots==std::vector<std::string>{"Copy"}) && scene.notifyCount==1 && scene.saveCount==1 && !scene.storage.Headers()[scene.owner*20+8]);
}

// 실제 Notify가 실패하면 이미 쓴 current/manager 이름을 유지하고 뒤의 삭제/제목/dirty/설정은 실행하지 않는다.
TEST_CASE(FortNameDevices_FailedNotifyKeepsPartialNameWithoutDeletingOld) {
    NameScene scene;const auto old=scene.Install("Before");auto hooks=scene.Hooks();hooks.notify=[](auto) { throw std::runtime_error("명시 Notify 실패"); };FortCurrentNameDevices devices(scene.names,scene.storage,scene.client.Configuration(),scene.Globals(),std::move(hooks),&scene.files);
    const std::string selected="After";std::string reason;try { devices.Set(NameInput(selected)); }catch (const std::exception& error) { reason=error.what(); }CHECK(reason=="명시 Notify 실패");
    const auto expected=scene.files.ResolvePath("d/after.fort").string();if (scene.names.Current()!=selected || scene.storage.Name(1)!=expected) std::fprintf(stderr,"Fort name partial reason=%s current=%s manager=%s expected=%s\n",reason.c_str(),std::string(scene.names.Current()).c_str(),scene.storage.Name(1).c_str(),expected.c_str()); // 가상 게임 경로는 기존 resolver가 소문자로 정규화하지만 실제 이름 버퍼는 After를 보존한다.
    CHECK(scene.names.Current()==selected && scene.storage.Name(1)==expected);CHECK(std::filesystem::is_regular_file(old) && !std::filesystem::exists(scene.files.ResolvePath("d/After.fort")) && scene.snapshots==std::vector<std::string>{"Before"} && !scene.titles && scene.names.Catalog().dirty==1);
}

// 고정256BYTE 목적 범위를 넘으면 먼저 쓴 부분만 남고 뒤의 외부 효과를 실행하지 않는다.
TEST_CASE(FortNameDevices_PhysicalCurrentNameOverflowPreservesPrefix) {
    NameScene scene;FortCurrentNameDevices devices(scene.names,scene.storage,scene.client.Configuration(),scene.Globals(),scene.Hooks());const std::string selected(260,'N');CHECK(NameRejects([&] { devices.Set(NameInput(selected)); }));
    const auto bytes=scene.names.CurrentBytes();CHECK(std::all_of(bytes.begin(),bytes.end(),[](auto byte) { return byte=='N'; }) && NameRejects([&] { static_cast<void>(scene.names.Current()); }) && !scene.titles && scene.names.Catalog().dirty==1 && scene.snapshots.empty());
}

// 삭제 요청은 자기 단일 파일에만 도달하며 보호 경로/루트 밖을 거부하고 디렉터리는 유지한다.
TEST_CASE(FortNameDevices_DeleteUsesOnlyCheckedSingleFile) {
    NameScene scene;const auto original=scene.assets.files.Read("d/TEST01.fort");const auto path=scene.files.Prepare("Delete",original);CHECK(NameRejects([&] { scene.files.Delete((scene.assets.root/"d/TEST01.fort").string()); }));CHECK(NameRejects([&] { scene.files.Delete((scene.root.parent_path()/"outside.fort").string()); }));
    scene.files.Delete(path.string());CHECK(!std::filesystem::exists(path));scene.files.Delete(path.string());const auto directory=scene.root/"empty";std::filesystem::create_directory(directory);scene.files.Delete(directory.string());CHECK(std::filesystem::is_directory(directory) && scene.assets.files.Read("d/TEST01.fort")==original);
}
