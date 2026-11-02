// 원문355일반입력/48이름과기존저장·강조·카메라몸체의접합을검사한다.
#include "GumpRawSupport.h"
#include "client/UtilityCommandInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/GifScreenshot.h"
#include "client/RawUserHighlight.h"
#include "client/CameraHome.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::client; //제품과동일입력큐/일반명령/기존부모를사용한다.
using namespace netstorm::test::gumpraw; //원문hex/TSV의해독만공유한다.
namespace { //실제문자열/호출trace의직렬화만담는검사전용경계다.
std::string UtilityHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //기대분기/출력은계산하지않는다.
std::string UtilityText(std::string_view hex) { const auto bytes=Bytes(std::string(hex));return std::string(bytes.begin(),bytes.end()); } //fixture원문문자열의NUL도보존한다.
}

TEST_CASE(UtilityCommandInput_Native283NonMultiplayerWrappers) { //Console/정지 앞 일반명령의 국소 몸체를 대조하고 MP 항해 명령은 사용자 지시대로 제외한다.
    const auto rows=LoadFixture(NETSTORM_UTILITY_COMMAND_FIXTURE).rows;std::size_t count{},skipped{};
    for (const auto& row:rows) { //선행Guide/Gump의외부소비만분배하기전에적용한다.
        if (row[0]=="event") continue;CHECK(row.size()==15);if (row.size()!=15) continue;++count;
        if ((Number(row[0])&0x03ffffffU)==kMoveIslandCommand) { ++skipped;continue; } //분류 전 원시는 보존하며 MP mode6의 신규 검사는 수행하지 않는다.
        std::string trace;auto source=UtilityText(row[7]);
        const auto add=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; //실제제품콜백순서만기록한다.
        UtilityCommandHooks hooks;hooks.text=[&](auto) { return std::string_view(source); };hooks.screenshot=[&] { add("save"); };hooks.tell=[&](auto text) { add("tell:"+std::string(text)); };
        hooks.readString=[&](auto key,auto& output) { add("read:"+UtilityHex(key)+':'+row[8]);if (Number(row[8])) output=UtilityText(row[9]);return Number(row[8])!=0; };
        hooks.url=[&](auto text) { add("url:"+UtilityHex(text)); };hooks.run=[&](auto text,auto mode) { add("run:"+UtilityHex(text)+':'+std::to_string(mode)); };
        hooks.highlight=[&](auto mode) { add("highlight:"+std::to_string(mode)); };hooks.center=[&](auto sid) { add("center:"+std::to_string(sid)); };
        if (!Number(row[5]) && !Number(row[6])) CHECK(DispatchUtilityCommandInput({Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))},hooks)); //현재Console/정지는이명령의추가거부조건이아니다.
        CHECK((trace.empty() ? "-" : trace)==row[11]);CHECK(row[12]=="1" && row[13]=="0" && row[14]==row[10]);
    }
    CHECK(count==355 && skipped==72);
}

TEST_CASE(UtilityCommandInput_Native39NonMultiplayerNamesAndQueue) { //동결이름/인자기대값은실제PE명령표/토큰/큐관측에서만읽는다.
    const auto rows=LoadFixture(NETSTORM_UTILITY_COMMAND_NAMES_FIXTURE).rows;CHECK(rows.size()==48);std::size_t skipped{};
    for (const auto& row:rows) { //대소문자·미등록·문자열고리의현재수명을같은공개API로재생한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;
        if (row[6]!="-" && Number(Split(row[6],',')[0])==kMoveIslandCommand) { ++skipped;continue; } //같은 MP 명령의 이름/큐9행도 새 실행에서 제외한다.
        InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};
        QueueScriptCommand(UtilityText(row[1]),queue,strings,{GameMenuScriptCode,[&](auto) { ++unknown; }});CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") { CHECK(queue.Empty() && unknown);continue; }
        const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && !unknown && queue.Empty());CHECK(IsGameMenuCommand(event.code));
        const auto token=std::bit_cast<std::uint32_t>(event.y);if (expected[1].starts_with('n')) CHECK(token==Number(expected[1].substr(1)));else CHECK(UtilityHex(strings.Resolve(token))==expected[1].substr(1));
    }
    CHECK(skipped==9);
}

TEST_CASE(UtilityCommandInput_AliasFailureAndOwnedStringsRunMode) { //현재Config조회실패는URL을생략하고재진입의공유문자열변경은이전키/Run인자를훼손하지않는다.
    CHECK(!DispatchUtilityCommandInput({'D',0,0},{}));CHECK(DispatchUtilityCommandInput({kUrlAliasCommand|InputCode::kRelease,0,0},{}));std::string scratch="Help",seen;std::uint32_t urls{},runs{};UtilityCommandHooks hooks;
    hooks.text=[&](auto token) { CHECK(token==7);return std::string_view(scratch); };hooks.readString=[&](auto key,auto&) { CHECK(key=="Help URL");scratch.assign(300,'X');return false; };
    CHECK(DispatchUtilityCommandInput({kUrlAliasCommand,0,7},hooks));CHECK(!urls);scratch=std::string("Before\0After",12);
    hooks.readString=[&](auto key,auto& value) { CHECK(key=="Before URL");scratch="Changed";value=std::string("help:temple\0tail",16);return true; };hooks.url=[&](auto text) { ++urls;seen=text;scratch.assign(400,'Y');CHECK(text=="help:temple"); };
    CHECK(DispatchUtilityCommandInput({kUrlAliasCommand,0,7},hooks));CHECK(urls==1 && seen=="help:temple");scratch="Init(3)";hooks.run=[&](auto text,auto mode) { ++runs;scratch.assign(400,'Z');CHECK(text=="Init(3)" && mode==1); };
    CHECK(DispatchUtilityCommandInput({kRunCommand,0,7},hooks));CHECK(runs==1);scratch.assign(252,'A');bool failed{};
    try { static_cast<void>(DispatchUtilityCommandInput({kUrlAliasCommand,0,7},hooks)); }catch (const std::length_error&) { failed=true; }CHECK(failed && urls==1); //원문버퍼밖쓰기의명시호스트경계이며원문정상길이fixture와구별한다.
}

TEST_CASE(UtilityCommandInput_ScreenshotExistingSaveFailureOrder) { //연속캡처active와독립인실제번호/잠금부모가실패하면Tell을실행하지않는다.
    std::uint32_t counter{},captureActive{7};std::string trace;bool fail=true;ScreenshotSaveHooks disk;disk.set=[&](auto,auto) { trace+='S'; };disk.read=[&](auto,auto& name) { trace+='R';name="output.gif";return true; };disk.exists=[&](auto) { trace+='E';return false; };
    disk.lock=[&] { trace+='L'; };disk.gif=[&](auto) { trace+='G';if (fail) throw std::runtime_error("명시저장장치실패"); };disk.unlock=[&] { trace+='U'; };disk.log=[&](auto) { trace+='O'; };
    UtilityCommandHooks hooks;hooks.screenshot=[&] { static_cast<void>(SaveNumberedScreenshot(counter,disk)); };hooks.tell=[&](auto text) { CHECK(text=="TellGifWritten");trace+='T'; };bool stopped{};
    try { static_cast<void>(DispatchUtilityCommandInput({kTakeScreenShotCommand,0,0},hooks)); }catch (const std::runtime_error&) { stopped=true; }CHECK(stopped && counter==1 && trace=="SRELGU" && captureActive==7);
    fail=false;trace.clear();CHECK(DispatchUtilityCommandInput({kTakeScreenShotCommand,0,0},hooks));CHECK(counter==2 && trace=="SRELGUOT" && captureActive==7);
}

TEST_CASE(UtilityCommandInput_ExistingCurrentSidCamera) { //현재 raw SID 전체를 기존 카메라 몸체에 넘기며 소유자나 낮은WORD로 치환하지 않는다. MP 강조는 검사하지 않는다.
    UtilityCommandHooks hooks;
    SidPool pool(OriginalEdition::Patch1078,24000,true);const auto sid=pool.Allocate();auto raw=pool.AllocatedBytes(sid);raw[10]=1;raw[11]=0;Put(raw,14,std::bit_cast<std::uint32_t>(10.5F));Put(raw,18,std::bit_cast<std::uint32_t>(20.25F));std::vector<RiftTypeRecord> types(2);types[1].footX=3;types[1].footY=4;CameraHomeState state;std::array<float,2> point{};std::uint32_t centers{};
    CameraHome camera(pool,types,state,[&](auto x,auto y) { ++centers;point={x,y}; });hooks.center=[&](auto value) { if (value>65535U || value>=pool.Capacity()) throw std::out_of_range("명시물리SID범위");camera.CenterOn(Sid{static_cast<std::uint16_t>(value)}); };
    CHECK(DispatchUtilityCommandInput({kLookAtSidCommand,23,sid.value},hooks));CHECK((point==std::array<float,2>{7.5F,16.25F}) && centers==1);raw[11]=2;CHECK(DispatchUtilityCommandInput({kLookAtSidCommand,0,sid.value},hooks) && centers==1);
    CHECK(DispatchUtilityCommandInput({kLookAtSidCommand,0,0},hooks) && centers==1);bool failed{};try { static_cast<void>(DispatchUtilityCommandInput({kLookAtSidCommand,0,65536},hooks)); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && centers==1);
    SidPool fresh(OriginalEdition::Patch1078,24000,true);types[0].footX=types[0].footY=0;std::uint32_t freshCenters{};CameraHome firstMenu(fresh,types,state,[&](auto x,auto y) { ++freshCenters;CHECK(x==0 && y==0); });
    hooks.center=[&](auto value) { firstMenu.CenterOn(Sid{static_cast<std::uint16_t>(value)}); };CHECK(fresh.Slot(Sid{1})[11]==0 && fresh.Slot(Sid{2})[11]==1);
    CHECK(DispatchUtilityCommandInput({kLookAtSidCommand,0,1},hooks) && freshCenters==1);CHECK(DispatchUtilityCommandInput({kLookAtSidCommand,0,2},hooks) && freshCenters==1); //실제 첫 메뉴의 예약SID1은 살아 있고 SID2는 free라서 중심 효과가 다르다.
}
