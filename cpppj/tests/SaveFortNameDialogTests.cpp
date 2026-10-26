// Save as 원문288관측과 실제 공통 문자/두 버튼/모달/파일 장치를 창 없이 검증한다.
#include "DudeSpawnSupport.h"
#include "client/SaveFortNameDialog.h"
#include "client/EditorFiles.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawTextEntryControls.h"
#include "client/TextEntryStorage.h"
#include "client/PaletteShade.h"
#include <cctype>
#include <filesystem>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 독립 fixture와 실제 UI의 입력·관측만 준비한다.
// 최초 원문fixture의 바이트 문자열을 그대로 읽는다.
std::string SaveNameDecode(std::string_view hex) {
    if (hex=="-") return {};std::string result;
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(i,2)),nullptr,16))); // 원문 두글자 hex를 한BYTE로 복원한다.
    return result;
}
// 같은 원문 바이트 문자열을 ASCII hex로 관찰한다.
std::string SaveNameEncode(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 공통 장치에서 실제 보호 TYPE/글꼴/모달·입력을 구성한다.
struct SaveNameScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호 자산은 읽기만 한다.
    FontStore fonts{assets.files,"Arial"};PaletteColorTable palette;std::array<std::uint32_t,256> logical{}; // 같은 실제 자산 글꼴/팔레트다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 0.0; }}};RawMenuControls* menus{}; // 같은 공통 등록표/수명이다.
    EditorFiles work{std::filesystem::temp_directory_path()/"netstorm-save-name-ui",{assets.root}}; // 원본 밖의 검사 작업 namespace다.
    std::array<std::uint8_t,20> header{};std::string saved,told;std::uint32_t pathCalls{}; // 실제 장치 callback의 관측이다.
    SaveNameScene() {
        for (std::size_t i=0;i<logical.size();++i) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 현재 논리 팔레트의 원문 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& out) { out=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);
        const auto& fort=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(fort,"A01",0);const auto metrics=assets.game.Shapes().SquidMetrics(fort.block,frame);
        buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,{}});Put(header,0,1);Put(header,8,1);const std::array<std::uint8_t,1> seed{1};static_cast<void>(work.Prepare("Seed",seed));
    }
    // 실제 작업본 Exists와 이름 callback을 같은 창에 빌린다.
    SaveFortNameDialogContext Context() {
        SaveFortNameDialogContext context;context.work=&work;context.editor.now=[] { return 0.0; };context.currentName="Battle1";
        context.translate=[](auto text) { return std::string(text); };context.fortPath=[this](auto name) { ++pathCalls;return "d/"+std::string(name)+".fort"; };
        context.header=[this] { return std::span(header); };context.tell=[this](auto section) { told=section; };
        context.setName=[this](auto name) { CHECK(Get(header,8)==0);saved=name;const std::array<std::uint8_t,4> content{1,2,3,4};work.Write(work.ResolvePath("d/"+saved+".fort").string(),content); };
        return context;
    }
    // 공통 픽셀 수집/정렬/Draw에 실제 장식/제목/문자/버튼을 제출한다.
    std::vector<std::uint8_t> Draw() {
        const auto maps=BuildGumpShadeMaps(logical);std::vector<std::uint8_t> pixels(647*480,0x5a);menus->Draw(pixels,647,480,palette,maps);return pixels;
    }
};
}
// 기대값은 C++ 규칙으로 다시 만들지 않고 실제 PE의 반환/직접쓰기/장치 trace를 재생한다.
TEST_CASE(SaveFortNameDialog_ReplaysNativeConfirmAndCancel288) {
    const auto fixture=LoadFixture(NETSTORM_SAVE_FORT_NAME_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { // 모든 원문 mode/cancel/readonly/manager/exists/문자열 교차를 재생한다.
        CHECK(row.size()==11);auto mode=Number(row[1]),cancel=Number(row[2]);std::array<std::uint8_t,20> header{};Put(header,0,Number(row[3]) ? 0x12345678U : 0U);Put(header,8,Number(row[4]));
        const auto text=SaveNameDecode(row[6]);std::string trace,name;const auto add=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; };SaveFortNameConfirmHooks hooks;
        hooks.copyText=[&](auto output) { add("copy");std::copy(text.begin(),text.end(),output.begin());output[text.size()]=0; };
        hooks.alnum=[&](auto code) { add("alnum:"+std::to_string(code));return std::isalnum(code); };
        hooks.path=[&](auto value) { add("path:"+SaveNameEncode(value));return "d/"+std::string(value)+".fort"; };hooks.exists=[&](auto) { add("exists");return static_cast<std::int32_t>(Number(row[5])); };
        hooks.header=[&] { return std::span(header); };hooks.setName=[&](auto value) { name=value;add("set:"+SaveNameEncode(value)+':'+std::to_string(Get(header,8))); };
        hooks.tell=[&](auto value) { add("tell:"+std::string(value)); };hooks.readFort=[&](auto value) { name=value;add("store:"+SaveNameEncode(value));add("queue:33554638,287454020,287454020"); }; // 명시 oracle 입력의 command DWORD를 보존하며 실제 GUI 연결의 명령표와 구별한다.
        const auto result=row[0]=="Confirm" ? ConfirmSaveFortName(mode,hooks) : CancelSaveFortName(cancel,hooks.tell);
        CHECK(result==Number(row[7]) && Get(header,8)==Number(row[8]));CHECK((name.empty() ? "-" : SaveNameEncode(name))==row[9]);CHECK((trace.empty() ? "-" : trace)==row[10]);++checked;
    }
    CHECK(checked==288);
}
// 기존 전체 부모 callback 실패는 먼저 쓴 readonly를 보존하며 선택하지 않은 장치는 요구하지 않는다.
TEST_CASE(SaveFortNameDialog_PreservesReadonlyPrefixOnSetFailure) {
    std::array<std::uint8_t,20> header{};Put(header,0,1);Put(header,8,0x12345678U);const std::uint32_t mode=1;SaveFortNameConfirmHooks h;
    h.copyText=[](auto output) { std::copy_n("New",4,output.data()); };h.alnum=[](auto code) { return std::isalnum(code); };h.path=[](auto name) { return std::string(name); };h.exists=[](auto) { return 0; };h.header=[&] { return std::span(header); };
    CHECK(Throws([&] { ConfirmSaveFortName(mode,h); }));CHECK(Get(header,8)==0);const std::uint32_t allowed=1;CHECK(CancelSaveFortName(allowed,{})==1);
}
// 같은 공통 Route의 실제 문자 입력/Enter가 readonly와 작업본 callback을 거쳐 지연 Kill한다.
TEST_CASE(SaveFortNameDialog_ActualModalTextEntryEnterAndRetirement) {
    SaveNameScene scene;const auto path=scene.work.ResolvePath("d/Ui New.fort");scene.work.Delete(path.string());const auto token=scene.menus->ShowSaveFortNameDialog(scene.Context());const auto* root=scene.menus->SaveFortNameNode(token);CHECK(root && root->command==32 && root->baseHeight==104 && scene.buttons.CurrentDialog()==token);
    const auto entry=root->entry;auto* editor=scene.menus->TextEntryControls()->Node(entry);CHECK(editor && editor->multiline==0 && editor->maxLength==40 && editor->parent==token);CHECK(scene.menus->Node(root->ok)->parent==token && scene.menus->Node(root->cancel)->parent==token);
    const auto excluded=editor->excludedChars;const auto excludedBytes=scene.menus->TextEntryControls()->Storage().Bytes(excluded);const std::vector<std::uint8_t> excludedBefore(excludedBytes.begin(),excludedBytes.end()); //원문 소멸자가 반납하지 않는 별도 제외문자 블록의 실제 주소/바이트를 보관한다.
    CHECK(scene.menus->Point("OK") && scene.menus->Point("Cancel"));const auto image=scene.Draw();CHECK(std::any_of(image.begin(),image.end(),[](auto value) { return value!=0x5a; }));
    for (const auto ch:std::string("Ui New")) scene.buttons.RouteInput({static_cast<std::uint32_t>(ch),0,0}); // 초기 전체 선택을 실제 문자 입력이 교체한다.
    CHECK(scene.menus->TextEntryControls()->Storage().Find(editor->buffer)->Text()=="Ui New");scene.buttons.RouteInput({13,0,0});
    CHECK(scene.saved=="Ui New" && scene.pathCalls==1 && scene.work.Exists(path.string())!=0);CHECK(scene.menus->SaveFortNameNode(token)->flags&0x10000U);scene.buttons.CleanupGumps();CHECK(!scene.menus->SaveFortNameNode(token) && !scene.menus->TextEntryControls()->Node(entry));
    const auto& storage=scene.menus->TextEntryControls()->Storage();CHECK(scene.buttons.CurrentDialog()==0 && storage.BufferCount()==0 && storage.AllocationCount()==1 && std::ranges::equal(storage.Bytes(excluded),excludedBefore)); //실제00452730의 비영 제외문자4관측처럼 한 블록을 보존한다. 최상위 저장소의 RAII가 나중에 호스트 메모리를 반납한다.
    scene.work.Delete(path.string());
}
// 중복 이름/빈이름은 같은 모달을 유지하고 Cancel은 원문 serial과 callback 규칙을 사용한다.
TEST_CASE(SaveFortNameDialog_ActualDuplicateEmptyEscapeAndCancel) {
    SaveNameScene scene;const auto path=scene.work.ResolvePath("d/Existing.fort");const std::array<std::uint8_t,1> bytes{9};scene.work.Write(path.string(),bytes);
    const auto token=scene.menus->ShowSaveFortNameDialog(scene.Context(),1,1,"Existing");const auto* root=scene.menus->SaveFortNameNode(token);scene.buttons.RouteInput({13,0,0});CHECK(scene.told=="NameRequired" && scene.saved.empty() && !(root->flags&0x10000U));
    scene.buttons.RouteInput({27,0,0});CHECK(!(root->flags&0x10000U));scene.menus->TextEntryControls()->Backend().SetText(root->entry,{});scene.told.clear();scene.buttons.RouteInput({13,0,0});CHECK(scene.told.empty() && !(root->flags&0x10000U));
    const auto serial=scene.menus->Node(root->cancel)->serial;scene.buttons.HandleGump(token,0x0200002cU,serial,0);CHECK(root->flags&0x10000U);scene.buttons.CleanupGumps();CHECK(!scene.menus->SaveFortNameNode(token));scene.work.Delete(path.string());
    const auto forced=scene.menus->ShowSaveFortNameDialog(scene.Context(),1,0);const auto* required=scene.menus->SaveFortNameNode(forced);scene.buttons.HandleGump(forced,0x0200002cU,scene.menus->Node(required->cancel)->serial,0);CHECK(scene.told=="NameRequired" && !(required->flags&0x10000U));scene.menus->CloseSaveFortNameDialogs();CHECK(scene.buttons.CurrentDialog()==0);
}
// 실제 마우스 발화는 재사용 가능한 등록칸 id와 다른 단조 serial을 부모에 전달한다.
TEST_CASE(SaveFortNameDialog_ActualMouseUsesSerialAfterPriorGumpRetirement) {
    SaveNameScene scene;const auto discarded=scene.menus->ShowSaveFortNameDialog(scene.Context());scene.menus->CloseSaveFortNameDialogs();CHECK(!scene.menus->SaveFortNameNode(discarded));
    const auto path=scene.work.ResolvePath("d/Mouse New.fort");scene.work.Delete(path.string());const auto token=scene.menus->ShowSaveFortNameDialog(scene.Context(),1,1,"Mouse New");const auto* root=scene.menus->SaveFortNameNode(token);const auto* button=scene.menus->Node(root->ok);CHECK(button->serial!=button->id);
    const auto point=scene.menus->Point("OK");CHECK(point);scene.buttons.RouteInput({InputCode::kLeftButton,point->x,point->y});scene.buttons.RouteInput({InputCode::kLeftButton|InputCode::kRelease,point->x,point->y});
    CHECK(scene.saved=="Mouse New" && scene.work.Exists(path.string()) && (root->flags&0x10000U));scene.buttons.CleanupGumps();CHECK(!scene.menus->SaveFortNameNode(token) && !scene.buttons.CurrentDialog());scene.work.Delete(path.string());
}
