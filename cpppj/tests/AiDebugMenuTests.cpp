// 원문메뉴60입력과현재Player/본문재조회·효과실패접두를게임없이검사한다.
#include "RawSceneSupport.h"
#include "client/RawAiDebugMenu.h"
#include "client/AiDebugCommandInput.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <array>
#include <bit>
#include <map>
#include <stdexcept>

using namespace netstorm::client; // 실제제품의같은메뉴/입력/장기전역을사용한다.
using namespace netstorm::test::rawscene; // 동결fixture의독립BYTE해독만공유한다.
namespace { // 이검사의명시입력과효과trace만소유한다.
constexpr std::array<std::uint32_t,9> kFields{0x258,0x2d8,0x2d4,0xb00,0x220,0x228,0x230,0x234,0x24c}; // 원문필드의정확한폭/오프셋이다.
constexpr std::array<std::uint32_t,7> kDebug{0x52f3cc,0x52f3e4,0x52f3d0,0x52f3d4,0x52f3d8,0x52f3dc,0x52f3e0}; // 실제메뉴7행의순서다.
std::string AiHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } //원문문자열을재인코딩하지않는다.
std::vector<std::uint8_t> AiDebugBytes(std::string_view text) { // 이 새 검사의 hex/빈 sentinel만 읽어 다른 support namespace와 섞지 않는다.
    if (text.empty() || text=="-") return {};if (text.size()%2) throw std::invalid_argument("AI 진단 fixture hex의 홀수 길이");
    const auto digit=[](char value)->std::uint8_t { if (value>='0' && value<='9') return static_cast<std::uint8_t>(value-'0');if (value>='a' && value<='f') return static_cast<std::uint8_t>(value-'a'+10);if (value>='A' && value<='F') return static_cast<std::uint8_t>(value-'A'+10);throw std::invalid_argument("AI 진단 fixture의 hex 문자 범위"); }; // 두 nibble의 실제 값을 읽는다.
    std::vector<std::uint8_t> result;result.reserve(text.size()/2); // 입력의 두 문자마다 한 BYTE를 소유한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>((digit(text[index])<<4)|digit(text[index+1]))); // 기대 결과가 아닌 입력/원문 출력 hex를 해독한다.
    return result;
}
template<class F> bool AiFails(F operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } //선택된미연결/잘못된물리는명시실패여야한다.
void AiEvent(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //원문효과의순서만직렬화한다.
struct MenuInput {
    std::vector<std::uint8_t> player=std::vector<std::uint8_t>(180),first=std::vector<std::uint8_t>(0xb08,0xa5),second=std::vector<std::uint8_t>(0xb08,0x5a); //독립Player한행과교체전후의정상본문이다.
    std::map<std::uint32_t,std::uint32_t> debug; // 원문실제7주소에대한현재입력값이다.
    std::uint32_t checking{},owner{},mutation{},formats{},checks{},taken{},playerReads{},captured{1}; //현재입력과효과callback의명시호출번호다.
    std::string trace,stop; // 실제효과접두와명시중단선택이다.
    // native입력의현재필드와정상포인터토큰을같은폭으로복구한다.
    explicit MenuInput(const std::vector<std::string>& data) {
        checking=Number(data[0]);owner=Number(data[1]);mutation=Number(data[3]);Put(player,0xb0,Number(data[2]) ? 1U : 0U);
        constexpr std::array<std::uint32_t,9> other{1,1234,4,9,0x40c00000,0x40800000,9,10,30}; //원문관측에공급한교체본문의독립입력이다.
        for (std::size_t i=0;i<kFields.size();++i) { Put(first,kFields[i],Number(data[4+i]));Put(second,kFields[i],other[i]); } //현재기대출력에서필드를만들지않는다.
        for (std::size_t i=0;i<kDebug.size();++i) debug[kDebug[i]]=Number(data[13+i]); //실제주소별독립값을연결한다.
    }
    // 포인터토큰1/2만살아있으며해제/NULL은미정성공으로만들지않는다.
    std::span<const std::uint8_t> Body(std::uint32_t token) const { if (token==1) return first;if (token==2) return second;throw std::out_of_range("명시본문토큰누락"); }
    // 선택한효과를기록한다음명시중단을전파한다.
    void Emit(std::string value) { AiEvent(trace,value);if (value.starts_with(stop) && !stop.empty()) throw std::runtime_error("명시메뉴효과중단"); }
    // 실제메뉴부모가필요한현재조회경계만공급한다.
    AiDebugMenuReads Reads() { return {[this] { return checking; },[this](auto value) { ++playerReads;CHECK(value==owner);return std::span<const std::uint8_t>(player); },[this](auto token) { return Body(token); },[this](auto address) { return debug.at(address); },[](auto address) { if (address==0x542a34) return kAiReportingCommand;if (address==0x542a38) return kDebugSwitchCommand;CHECK(address==0x5428d8);return 0x2000078U; }}; }
    // Native와같은외부효과/재진입입력을주며실제원문서식기몸체를합성했다고주장하지않는다.
    AiDebugMenuHooks Hooks() {
        AiDebugMenuHooks h;
        h.header=[this](auto value) { Emit("H:"+AiHex(value));if (mutation==1 && value=="NOT INITIALIZED\n") Put(player,0xb0,2); };
        h.format=[this](auto limit,auto pattern,auto args) { CHECK(limit==80 && args.size()==1);const auto value=std::holds_alternative<std::string>(args[0]) ? AiHex(std::get<std::string>(args[0])) : std::to_string(std::bit_cast<std::uint32_t>(std::get<std::int32_t>(args[0])));Emit("F:"+std::to_string(limit)+":"+AiHex(pattern)+":"+value);++formats;if (mutation==2 && formats==1) Put(player,0xb0,2);return "F"+std::to_string(formats); };
        h.checked=[this](auto on,auto label,auto command,auto parameter,auto include,auto enabled) { Emit("C:"+std::to_string(on)+":"+AiHex(label)+":"+std::to_string(command)+":"+std::to_string(parameter)+":"+std::to_string(include)+":"+std::to_string(enabled));if (mutation==4 && ++checks==9) Put(captured==1 ? first : second,0x220,0x3f800000); };
        h.begin=[this](auto flags) { Emit("B:"+std::to_string(flags)); };
        h.take=[this] { Emit("T:"+std::to_string(++taken));return 0x700U+taken; };
        h.submenu=[this](auto label,auto child,auto enabled,auto checked) { Emit("S:"+AiHex(label)+":"+std::to_string(child)+":"+std::to_string(enabled)+":"+std::to_string(checked));if (label=="AI Debug") { if (mutation==3) Put(player,0xb0,2);captured=Get(player,0xb0); } };
        return h;
    }
};
}

// 조건부수명·순서·같은현재brain 재조회를원문독립60행전체와대조한다.
TEST_CASE(AiDebugMenu_Native60RowsAndCurrentBodyLifetime) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_AI_DEBUG_MENU_FIXTURE).rows) { //새고정관측의입력/출력을분리해읽는다.
        CHECK(row.size()==2);if (row.size()!=2) continue;const auto data=Split(row[0],',');CHECK(data.size()==20);if (data.size()!=20) continue;MenuInput input(data);OpenAiDebugMenu(input.owner,input.Reads(),input.Hooks());CHECK((input.trace.empty()?"-":input.trace)==row[1]);CHECK(input.playerReads==(input.checking && std::bit_cast<std::int32_t>(input.owner)>0 ? 1U : 0U));++count;
    }
    CHECK(count==60);
}

// 첫효과중단뒤후속메뉴를합성하지않고초기단락은미연결GUI를요구하지않는다.
TEST_CASE(AiDebugMenu_PartialEffectsAndUnchosenPhysicalBoundaries) {
    AiDebugMenuReads skipped;skipped.checking=[] { return 0U; };OpenAiDebugMenu(0,skipped,{});skipped.checking=[] { return 1U; };OpenAiDebugMenu(0xffffffffU,skipped,{});
    const auto data=Split("1,1,1,0,0,30000,0,0,0,1073741824,3,6,8,99,29,99,29,99,99,99",',');MenuInput input(data);input.stop="F:";CHECK(AiFails([&] { OpenAiDebugMenu(1,input.Reads(),input.Hooks()); }));CHECK(input.trace=="H:4e4f5420494e495449414c495a45440a;F:80:7e6b4d6f6e65793d25647e2e0a:30000" && !input.taken);
    input.trace.clear();input.stop.clear();Put(input.first,0x2d4,5);CHECK(AiFails([&] { OpenAiDebugMenu(1,input.Reads(),input.Hooks()); }));CHECK(input.trace=="H:4e4f5420494e495449414c495a45440a;F:80:7e6b4d6f6e65793d25647e2e0a:30000;H:4631" && !input.taken); //원문목표배열밖을임의문구로바꾸지않는다.
}

// raw413d50는release를가리지않고실제상위접합은공통release에서효과를차단한다.
TEST_CASE(AiDebugInput_Native200RowsAndCommonReleaseEnvelope) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_AI_DEBUG_INPUT_FIXTURE).rows) { //신규MP양성은고정관측에포함하지않는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;std::vector<std::uint8_t> player(180),body(0xb08,0xa5);Put(player,0xb0,Number(row[2]) ? 1U : 0U);Put(body,0xb00,Number(row[3]));std::uint32_t debug=Number(row[3]);
        AiDebugInputState state{[] { return 0U; },[&](auto owner) { CHECK(owner==Number(row[1]));return std::span<const std::uint8_t>(player); },[&](auto token) { CHECK(token==1);return std::span<std::uint8_t>(body); },[&](auto address) { CHECK(address==0x52f3cc);return debug; },[&](auto address,auto value) { CHECK(address==0x52f3cc);debug=value; }};
        const auto savedPlayer=player;const auto code=Number(row[0]);ApplyAiDebugCommandState({code,123,std::bit_cast<std::int32_t>((code&0x3ffffffU)==kDebugSwitchCommand ? 0x52f3ccU : Number(row[1]))},state);auto expected=std::vector<std::uint8_t>(0xb08,0xa5);Put(expected,0xb00,Number(row[4]));CHECK(body==expected && player==savedPlayer && debug==Number(row[5]));++count;
    }
    CHECK(count==200 && IsAiDebugCommand(kDebugSwitchCommand|InputCode::kShift));CHECK(DispatchAiDebugCommandInput({kAiReportingCommand|InputCode::kRelease,0,1},{}));CHECK(DispatchAiDebugCommandInput({kDebugSwitchCommand|InputCode::kRelease,0,0x52f3cc},{}));CHECK(!DispatchAiDebugCommandInput({'X',0,0},{}));
}

// 같은Client의현재설정/로그/특수DWORD를읽으며짧은전역대여가진단값을재초기화하지않는다.
TEST_CASE(AiDebugInput_ActualClientOptionsAndStableDebugWords) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(options);auto game=client.GameplayGlobals();client.Configuration().Configuration().AppendSection("END");
    CHECK(game->AiDebug().Checking()==0 && game->ReadDebugWord(0x52f3cc)==99 && game->ReadDebugWord(0x52f3d4)==29);client.Configuration().SetInt("showSoftAsserts",73);CHECK(game->AiDebug().Checking()==73);client.Configuration().SetInt("anotherOption",3);CHECK(game->AiDebug().Checking()==73);
    AiDebugInputState state;state.readDebug=[&](auto address) { return game->ReadDebugWord(address); };state.writeDebug=[&](auto address,auto value) { game->WriteDebugWord(address,value); };
    for (const auto address:std::array<std::uint32_t,3>{0x52f3cc,0x55a480,0x55a484}) { // 실제안정저장소의원시29/99를BOOL로좁히지않는다.
        game->WriteDebugWord(address,0);CHECK(DispatchAiDebugCommandInput({kDebugSwitchCommand,0,std::bit_cast<std::int32_t>(address)},state));CHECK(game->ReadDebugWord(address)==29);CHECK(DispatchAiDebugCommandInput({kDebugSwitchCommand,0,std::bit_cast<std::int32_t>(address)},state));CHECK(game->ReadDebugWord(address)==99);
    }
    CHECK(game->player.special==99 && client.ScriptMenuGlobal(0x55a480)==99);const auto* identity=&game->AiDebug();{ auto use=game->Acquire();CHECK(&game->AiDebug()==identity && game->AiDebug().Checking()==73 && game->ReadDebugWord(0x52f3cc)==99 && game->player.special==99); }{ auto use=game->AcquirePrepared();CHECK(&game->AiDebug()==identity && game->ReadDebugWord(0x52f3cc)==99); }CHECK(AiFails([&] { game->WriteDebugWord(0x12345678,29); }));
}

// 옛21개 미등록alias와 실제Toggle이름63개의 숫자/문자열/첫NUL/FIFO를 함께 대조한다.
TEST_CASE(AiDebugInput_Native84NamesAndActualQueue) {
    std::size_t count{};auto rows=LoadFixture(NETSTORM_AI_DEBUG_NAMES_FIXTURE).rows;const auto current=LoadFixture(NETSTORM_AI_DEBUG_TOGGLE_NAMES_FIXTURE).rows;CHECK(rows.size()==21 && current.size()==63);rows.insert(rows.end(),current.begin(),current.end());
    for (const auto& row:rows) { //원문대소문자와등록실패의현재고리번호까지검사한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;const auto sourceBytes=AiDebugBytes(row[1]);const std::string source(sourceBytes.begin(),sourceBytes.end());InputQueue queue;ScriptStringRing strings;std::size_t diagnostics{};
        QueueScriptCommand(source,queue,strings,{[](auto name) { CHECK(GameMenuScriptCode(name)==HtmlScriptCommandValue(name));return GameMenuScriptCode(name); },[&](auto) { ++diagnostics; }});
        CHECK(strings.Index()==Number(row[7]) && diagnostics==static_cast<std::size_t>(std::count(row[8].begin(),row[8].end(),'L')));if (row[6]=="-") CHECK(queue.Empty() && diagnostics);else {
            const auto expected=Split(row[6],',');const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && queue.Empty() && IsGameMenuCommand(event.code));
            if (expected[1].starts_with('s')) { const auto bytes=AiDebugBytes(expected[1].substr(1));CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.x))==std::string(bytes.begin(),bytes.end())); }else CHECK(std::bit_cast<std::uint32_t>(event.x)==Number(expected[1].substr(1)));
        }
        ++count;
    }
    CHECK(count==84 && GameMenuCommandValue(0x542a34).value()==kAiReportingCommand && GameMenuCommandValue(0x542a38).value()==kDebugSwitchCommand);
    const std::string_view debugNul("ToggleDebug\0ignored",19),reportingNul("ToggleReporting\0ignored",23); // 원문 __stricmp의 첫 NUL 종료를 공개 조회에서 정적으로 고정하며 새 Native 관측으로 세지 않는다.
    CHECK(AiDebugCommandScriptCode(debugNul)==kDebugSwitchCommand && GameMenuScriptCode(debugNul)==kDebugSwitchCommand);
    CHECK(AiDebugCommandScriptCode(reportingNul)==kAiReportingCommand && GameMenuScriptCode(reportingNul)==kAiReportingCommand);
}
