// 기본 이름의 실제 원문 조건·두 선택 순서와 선택 뒤 작업 파일 이관을 확인한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "client/FortCurrentNameDevices.h"
#include "client/EditorMapCatalogFiles.h"
#include "client/EditorFortFileDevices.h"
#include "o/RawExistingEditorInstallOuter.h"
#include "o/RawDataManagerSelection.h"
#include <chrono>

namespace { // 이름 부모/파일 경계는 명시 장치이며 기대 선택은 독립 PE 결과에서만 읽는다.
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
struct DefaultFileReached {}; // 파일 설치 직전의 실제 도달을 전체 Editor 설치 완료와 구별한다.
// 빈 원문 문자열을 포함해 명시 ASCII 입력의 전체 BYTE를 읽는다.
std::string DefaultText(std::string_view text) {
    if (text=="-") return {};std::string result;
    for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; // 두16진수마다 한BYTE를 복원한다.
}
std::string DefaultHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 원문 C문자열 인자를 fixture와 같은 hex로 기록한다.
}

// 같은 이름 비교·Guide 해제·SetName 중 현재 이름 변이와 빈/점 설정에서도 기본 선택을 한 번만 수행한다.
TEST_CASE(EditorDefaultName_IndependentNative252CurrentGateAndSelectionOrder) {
    const auto rows=LoadFixture(NETSTORM_EDITOR_DEFAULT_NAME_FIXTURE).rows;CHECK(rows.size()==252);std::size_t index{};
    for (const auto& row:rows) { // 원문의 첫 관측만 기대값으로 사용한다.
        auto current=DefaultText(row[0]);const auto pending=DefaultText(row[1]),configured=DefaultText(row[2]);auto mission=Number(row[3]);const auto recognized=Number(row[4]),mutation=Number(row[5]);std::size_t selected{};std::string events;
        ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { return {}; });config.Set("newFort",configured);
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 같은 원문의 실제 호출 순서를 보관한다.
        const auto select=[&](std::string_view name) { event("N:"+DefaultHex(name));++selected;current=name;if (selected==1 && mutation) current=mutation==1 ? ".fromSetter":""; };
        ExistingEditorFortHooks hooks;hooks.currentName=[&]()->std::string_view { return current; };hooks.pendingName=[&]()->std::string_view { return pending; };
        hooks.compare=[&](auto first,auto second) { event("C:"+DefaultHex(first)+':'+DefaultHex(second));return CompareDataManagerSectionNames(first,second); };
        hooks.inMission=[&] { return mission; };hooks.missionName=[&](auto name) { event("M:"+DefaultHex(name));return static_cast<std::int32_t>(recognized); };hooks.closeGuide=[&] { event("G");mission=0; };hooks.setName=select;
        hooks.defaultName=[&] { event("Q:newFort");SelectConfiguredNewFort(config,select); };hooks.file=[] { throw DefaultFileReached{}; };
        bool reached{};try { InstallExistingEditorFort(hooks); }catch (const DefaultFileReached&) { reached=true; }
        const auto before=netstorm::test::FailureCount();CHECK(reached);CHECK((current.empty()?"-":DefaultHex(current))==row[6]);CHECK(mission==Number(row[7]));CHECK(events==row[8]);
        if (netstorm::test::FailureCount()!=before) std::fprintf(stderr,"EditorDefaultName Native row=%zu\n",index);++index;
    }
}

// 기본 이름 선택 후의 실제 경로에서 보호 원문을 작업본으로 이관하며 앞선 빈 pending 경로를 쓰지 않는다.
TEST_CASE(EditorDefaultName_LateSelectedPathPreparesActualSourceBeforeExists) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");
    const auto root=assets.root.parent_path()/"extracted/cpp-editor-default-name-653-disk"/std::filesystem::path(u8"한글-😀")/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto files=std::make_shared<EditorFiles>(root,std::vector<std::filesystem::path>{assets.root,assets.root.parent_path()/"originalCD"});EditorCatalogSourceDevices source;source.root=assets.root;source.archiveOnly=[&](auto path) { return assets.files.TryReadArchiveOnly(path); };EditorCatalogWorkspace workspace(files,std::move(source));
    ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { return {}; });config.Set("fortSpec","d/{local.1}.fort");config.Set("newFort","thewarbegins");std::string current,pending;std::size_t selected{};
    EditorFortPathEffects paths;paths.currentName=[&]()->std::string_view { return current; };paths.selectFileSpec=[] {};paths.fileSpecKey=[]()->std::string_view { return "fortSpec"; };paths.resolve=[&](auto path) { return workspace.Resolve(path); };paths.prepare=[&](auto path) { static_cast<void>(workspace.Prepare(path)); };paths.assertion=[](auto) { throw std::logic_error("기본 이름 뒤 선택 경로의 예상 밖 진단"); };EditorFortFileDevices device(*files,assets.files,config,std::move(paths));
    ExistingEditorFortHooks hooks;hooks.currentName=[&]()->std::string_view { return current; };hooks.pendingName=[&]()->std::string_view { return pending; };hooks.compare=CompareDataManagerSectionNames;
    hooks.defaultName=[&] { SelectConfiguredNewFort(config,[&](auto name) { current=name;++selected; }); };
    hooks.file=[&] { CHECK(current=="thewarbegins" && selected==1);const std::string path(device.PrepareCurrent());CHECK(device.Exists(path)==5 && device.Read(path)==original);throw DefaultFileReached{}; };
    bool reached{};try { InstallExistingEditorFort(hooks); }catch (const DefaultFileReached&) { reached=true; }CHECK(reached && selected==1);
}
