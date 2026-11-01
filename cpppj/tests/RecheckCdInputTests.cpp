// 원문 RecheckCD 일반 입력 332행과 실제 등록 표 조회·이름/메타/큐·Client 장기 owner를 대조한다.
#include "TestSupport.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientMain.h"
#include "client/DialogExitCommand.h"
#include "client/GameMenuCommands.h"
#include "client/GumpCreate.h"
#include "client/RawStormPowerRefresh.h"
#include "client/RecheckCdInput.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace netstorm::client; //같은 제품의 현재 입력/등록 표 조회/장기 owner를 사용한다.
namespace { //원문 표의 입력 열과 객체 배치 열만 제품 입력으로 사용한다.
// 탭으로 나뉜 한 행을 필드 목록으로 나눈다. 관측 열은 해석하지 않고 원시 문자열 그대로 둔다. 단위696에서 추가했다.
std::vector<std::string> Fields(const std::string& line) { std::istringstream stream(line);std::vector<std::string> result;std::string field;while (std::getline(stream,field,'\t')) result.push_back(field);return result; }
// Native DWORD 십진 표기를 signed host 값으로 잘라내지 않고 그대로 읽는다. 단위696에서 추가했다.
std::uint32_t Number(const std::string& text) { return static_cast<std::uint32_t>(std::stoul(text)); }
// 원문 한 행의 칸:serial:플래그 목록이다. 칸 999는 464020의 조회 범위(0..998) 밖이라 제품 표(1000칸 중 루트 제외 999칸)에 놓지 않는다.
struct LayoutObject { std::uint32_t slot{},serial{},flags{}; };
// 한 행의 배치 열을 객체 목록으로 읽는다. 빈 배치는 "-"이다. 단위696에서 추가했다.
std::vector<LayoutObject> ParseLayout(const std::string& text) {
    std::vector<LayoutObject> result;if (text=="-") return result;
    std::istringstream list(text);std::string item;
    while (std::getline(list,item,',')) { //객체 하나는 칸:serial:플래그 세 숫자다.
        std::istringstream parts(item);std::string slot,serial,flags;std::getline(parts,slot,':');std::getline(parts,serial,':');std::getline(parts,flags,':');
        result.push_back({Number(slot),Number(serial),Number(flags)});
    }
    return result;
}
}

// 원문 332입력: 뗌은 효과 없이 소비하고 그 밖에는 현재 serial 조회→Kill→0쓰기이며 모드/수정키/인자는 영향이 없다.
TEST_CASE(RecheckCdInput_Native332CurrentSerialLookupKillAndZero) {
    std::ifstream file(NETSTORM_RECHECK_CD_FIXTURE);CHECK(static_cast<bool>(file));std::string line;std::getline(file,line);std::size_t count{},kills{},zeroSerialKills{};
    while (std::getline(file,line)) { //매 행의 현재 serial과 등록 표를 새로 설치한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);CHECK(row.size()==11);if (row.size()!=11) return;
        const auto objects=ParseLayout(row[5]);std::vector<GumpCreateNode> nodes(objects.size());std::array<std::uint32_t,kGumpRegistryCapacity> entries{}; //루트 칸0을 뺀 999칸이 원문 464020의 0..998칸이다.
        for (std::size_t index=0;index<objects.size();++index) { //객체 번호는 설치 순서이며 토큰은 번호+1이다.
            nodes[index].serial=objects[index].serial;nodes[index].flags=objects[index].flags;
            if (objects[index].slot+1<kGumpRegistryCapacity) entries[objects[index].slot+1]=static_cast<std::uint32_t>(index+1);
        }
        std::string trace;RecheckCdHooks hooks;
        hooks.find=[&](auto serial) { return FindCurrentGumpSerial(entries,serial,[&](auto token)->GumpCreateNode* { return &nodes.at(token-1); }); }; //실제 제품의 현재 등록 표 조회를 그대로 쓴다.
        hooks.kill=[&](auto token) { trace="kill:"+std::to_string(token-1); }; //공통 Kill은 대상 객체 번호만 기록한다.
        auto serial=Number(row[4]);const auto input=serial;const InputEvent event{Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))};
        CHECK(Number(row[3])<4 && DispatchRecheckCdInput(event,serial,hooks));
        CHECK((trace.empty() ? "-":trace)==row[6] && serial==Number(row[7]) && row[9]=="1" && row[10]=="0");
        const bool released=(event.code&InputCode::kRelease)!=0;CHECK(row[8]==(released ? "0":"1") && serial==(released ? input:0U)); //쓰기 횟수는 뗌이면 0, 아니면 1이다.
        if (!trace.empty()) { ++kills;if (input==0) ++zeroSerialKills; }
        ++count;
    }
    CHECK(count==332 && kills==105 && zeroSerialKills==50); //serial 0도 살아 있는 serial 0 Gump를 찾아 Kill한 원문 관측 50행을 포함한다.
}

// 필수 장치가 없으면 빈 성공으로 넘기지 않고, Kill 예외는 0쓰기 앞에서 전파한다.
TEST_CASE(RecheckCdInput_MissingDeviceAndKillFailureKeepSerial) {
    std::uint32_t serial=7;bool failed{};
    try { DispatchRecheckCdInput({kRecheckCdCommand,0,0},serial,{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && serial==7);
    RecheckCdHooks hooks;hooks.find=[](auto) { return 5U; };failed=false;
    try { DispatchRecheckCdInput({kRecheckCdCommand,0,0},serial,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed && serial==7); //Kill 장치가 없으면 0을 쓰지 않는다.
    hooks.kill=[](auto) { throw std::runtime_error("공통 Kill 실패"); };failed=false;
    try { DispatchRecheckCdInput({kRecheckCdCommand,0,0},serial,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && serial==7);
    hooks.find=[](auto) { return 0U; };CHECK(DispatchRecheckCdInput({kRecheckCdCommand,0,0},serial,hooks) && serial==0); //찾지 못하면 Kill 장치 없이도 0만 쓴다.
    serial=9;CHECK(DispatchRecheckCdInput({kRecheckCdCommand|InputCode::kRelease,0,0},serial,{}) && serial==9);CHECK(!DispatchRecheckCdInput({0x0200009dU,0,0},serial,{}) && serial==9); //뗌은 장치를 요구하지 않고 다른 명령은 소비하지 않는다.
}

// 원문 이름표(543370→542ae8)·HTML 메타·실제 큐가 같은 입력을 만들고 일반 메뉴 명령표에도 포함된다.
TEST_CASE(RecheckCdInput_NameMetadataQueueAndCommandTable) {
    CHECK(GameMenuScriptCode("ReChEcKcD")==kRecheckCdCommand && GameMenuCommandValue(0x542ae8)==kRecheckCdCommand && IsGameMenuCommand(kRecheckCdCommand));CHECK(GameMenuScriptCode(" RecheckCD")==0xffffffffU);
    InputQueue queue;ScriptStringRing strings;DialogExitCommandState meta;ResolveDialogExitCommand(meta,"RecheckCD","0",MakeDialogExitResolveHooks(strings,{GameMenuScriptCode,{}}));DisposeDialogExitCommand(meta,queue);const auto event=queue.Pop(false);CHECK(event.code==kRecheckCdCommand && meta.command==0);
    std::uint32_t serial=3;std::vector<std::uint32_t> killed;RecheckCdHooks hooks;hooks.find=[](auto value) { return value+100; };hooks.kill=[&](auto token) { killed.push_back(token); };
    CHECK(DispatchRecheckCdInput(event,serial,hooks) && killed==std::vector<std::uint32_t>{103} && serial==0);
}

// 실제 Client가 005c85ec를 최초 BSS 0으로 장기 보관하고 같은 참조를 모든 입력이 빌린다.
TEST_CASE(RecheckCdInput_ActualClientOwnerStartsZeroAndStaysStable) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));
    auto& serial=client.CdGlobals()->RecheckSerial();const auto* address=&serial;CHECK(serial==0 && address==&std::as_const(*client.CdGlobals()).RecheckSerial());
    serial=77;std::uint32_t found{};CHECK(DispatchRecheckCdInput({kRecheckCdCommand,0,0},client.CdGlobals()->RecheckSerial(),{[&](auto value) { found=value;return 0U; },{}}));
    CHECK(found==77 && serial==0 && address==&client.CdGlobals()->RecheckSerial()); //조회는 쓰기 전의 현재 값이며 같은 owner의 같은 주소가 0이 된다.
}
