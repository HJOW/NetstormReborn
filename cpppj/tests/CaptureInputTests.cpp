// 원문 일반입력192행과 실제 Client owner/공유고리/이름·큐를 대조한다.
#include "TestSupport.h"
#include "client/CaptureInput.h"
#include "client/ClientCaptureSequence.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/GameMenuCommands.h"
#include "client/DialogExitCommand.h"
#include "client/ScriptCommand.h"
#include "o/OriginalText.h" // 6차: 한글 표시 바이트(EncodeDisplayText) 기대값이다.
#include "platform/Utf8Path.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace netstorm::client; //같은 제품의 현재 입력/장기 owner를 사용한다.
namespace { //원문 표의 앞5열만 제품의 입력으로 사용한다.
std::vector<std::string> Fields(const std::string& line) { std::istringstream stream(line);std::vector<std::string> result;std::string field;while (std::getline(stream,field,'\t')) result.push_back(field);return result; } //관측 열은 원시 문자열 그대로 둔다.
std::uint32_t Number(const std::string& text) { return static_cast<std::uint32_t>(std::stoul(text)); } //Native DWORD를 signed host값으로 잘라내지 않는다.
}

// 현재 active를 먼저 읽으며 param2=1만 시작하고 release/수정키/모드 결과를 원문과 비교한다.
TEST_CASE(CaptureInput_Native192CurrentActiveAndSecondParameter) {
    std::ifstream file(NETSTORM_CAPTURE_INPUT_FIXTURE);CHECK(static_cast<bool>(file));std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { //매 행의 초기 전역과 전체 인자를 재설치한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);CHECK(row.size()==9);if (row.size()!=9) return;
        InputEvent event{Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))};auto active=Number(row[4]);std::string trace;
        CaptureInputHooks hooks;hooks.start=[&] { trace="start";active=1; };hooks.stop=[&] { trace="stop";active=0; };hooks.tell=[&](auto section) { trace="tell:"+std::string(section); };
        CHECK(Number(row[3])<4 && DispatchCaptureInput(event,active,hooks));CHECK((trace.empty() ? "-":trace)==row[5] && active==Number(row[6]) && row[7]=="1" && row[8]=="0");++count;
    }
    CHECK(count==192);
}

// 원문 이름/HTML메타/실제 큐의 숫자1이 같은 입력을 시작하고 이미 활성일 때는 종료한다.
TEST_CASE(CaptureInput_MetadataQueueAndSelectedEffects) {
    CHECK(GameMenuScriptCode("CaPtUrE")==kCaptureCommand && GameMenuCommandValue(0x542968)==kCaptureCommand && IsGameMenuCommand(kCaptureCommand));CHECK(GameMenuScriptCode(" Capture")==0xffffffffU);
    InputQueue queue;ScriptStringRing strings;DialogExitCommandState meta;ResolveDialogExitCommand(meta,"Capture","1",MakeDialogExitResolveHooks(strings,{GameMenuScriptCode,{}}));DisposeDialogExitCommand(meta,queue);const auto event=queue.Pop(false);CHECK(event.code==kCaptureCommand && event.x==1 && event.y==1 && meta.command==0);
    std::uint32_t active{};std::size_t starts{},stops{};CaptureInputHooks hooks;hooks.start=[&] { ++starts;active=1; };hooks.stop=[&] { ++stops;active=0; };CHECK(DispatchCaptureInput(event,active,hooks) && starts==1 && active==1);CHECK(DispatchCaptureInput({kCaptureCommand,0,-1},active,hooks) && stops==1 && active==0);
    CHECK(DispatchCaptureInput({kCaptureCommand|InputCode::kRelease,0,1},active,{}));CHECK(!DispatchCaptureInput({0x0200005b,0,0},active,{}));bool failed{};try { DispatchCaptureInput({kCaptureCommand,0,0},active,{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
}

// 창/Run 전에 inactive 프레임은 화면을 요구하지 않고 Client의 같은 장기 상태를 빌린다.
TEST_CASE(CaptureInput_ActualClientOwnerWithoutScreenAndStableStart) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));auto& owner=client.ScreenCapture();const auto* state=&owner.State();owner.Frame();CHECK(!state->active && !state->count && !owner.AllocatedFrames());
    CHECK(DispatchCaptureInput({kCaptureCommand,0,1},state->active,{[&] { owner.Start(); },{}, {}}));CHECK(state==&client.ScreenCapture().State() && state->active==1 && state->first==1 && state->count==0 && owner.AllocatedFrames()==0);client.Configuration().Set("GifSequenceName","unchanged-owner");CHECK(state==&owner.State() && state->active==1);
}

// 실제 공유 고리의 예약80과 완성 본문 폭을 구별하고 Unicode/장치/root 조합을 확인한다.
TEST_CASE(CaptureInput_ActualSharedReportAndUnicodeWritePath) {
    ClientStaticStrings strings;strings.Initialize(10240);const std::string first(90,'a'),last(91,'b');const auto bytes=strings.FormatCaptureRange(first,last);const auto text=std::string(reinterpret_cast<const char*>(bytes.data()));CHECK(text=="Wrote "+first+" through "+last+".\n" && text.size()>80 && bytes[80]=='a');
    const auto nul=strings.FormatCaptureRange(std::string_view("one\0ignored",11),std::string_view("two\0ignored",11));CHECK(std::string(reinterpret_cast<const char*>(nul.data()))=="Wrote one through two.\n");
    CHECK(CaptureNoticeFontBytes("plain000.bmp")=="plain000.bmp" && CaptureNoticeFontBytes("한 😀.bmp")==[]{ const auto han=netstorm::o::EncodeDisplayText("한");return std::string(han.begin(),han.end())+" [U+1F600].bmp"; }()); // 6차: 한글은 확장 글꼴 바이트로 표시하고 표시할 수 없는 이모지만 [U+코드]로 식별한다.CHECK(CaptureNoticeFontBytes("é€")==std::string("\xe9\x80",2));bool invalid{};try { CaptureNoticeFontBytes(std::string("\xf0\x80\x80\x80",4)); }catch (const std::invalid_argument&) { invalid=true; }CHECK(invalid);
#ifdef _WIN32
    const std::filesystem::path root(u8"H:/게임 😀"),relative(u8"d/프레임000.bmp");CHECK(ResolveCaptureBitmapPath(root,"d/sequence000.bmp")==root/"d/sequence000.bmp");CHECK(ResolveCaptureBitmapPath(root,netstorm::platform::PathToUtf8(relative))==root/relative);
    const std::filesystem::path absolute(u8"C:/캡처 😀/frame000.bmp");CHECK(ResolveCaptureBitmapPath(root,netstorm::platform::PathToUtf8(absolute))==absolute);CHECK(ResolveCaptureBitmapPath(root,"H:frame000.bmp")==root/"frame000.bmp");CHECK(ResolveCaptureBitmapPath(root,"\\d\\frame000.bmp")==root/"d/frame000.bmp");
#endif
}
