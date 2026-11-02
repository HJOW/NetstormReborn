// 원문238입력/30이름과 같은 Client priority/State 저장/UI kind reader/팔레트 부모의 접합을 검사한다.
#include "GumpRawSupport.h"
#include "client/ConfigurationCommandInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/ClientMain.h"
#include "client/CampaignTemplateStorage.h"
#include "client/SoloMapCatalog.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawFortPaletteTransition.h"
#include "o/ConfigInterface.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <map>
#include <stdexcept>
using namespace netstorm::client; //실제 제품과 같은 명령/공유 owner/Config/저장 부모를 사용한다.
using namespace netstorm::o; //현재 State/Notify/팔레트 부모와 실제 파일 이미지 형식을 재사용한다.
using namespace netstorm::test::gumpraw; //fixture의 원시 해독만 공유한다.
namespace { //검사 장치의 문자열 해독·부분 중단·명시 메모리 입력이다.
std::string ConfigurationHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //원문 문자열의 빈 값과 NUL 뒤 바이트를 보존한다.
std::string ConfigurationText(std::string_view hex) { const auto bytes=Bytes(std::string(hex));return {bytes.begin(),bytes.end()}; } //TSV의 실제 hex를 좁은 문자열로 해독한다.
template<class F> bool ConfigurationFails(F operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } //명시 실패가 조용히 성공으로 바뀌지 않았는지 검사한다.
ClientOptions ConfigurationClientOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; } //Run/창/OS 우선순위 호출을 하지 않는 기존 headless Client 구성이다.
FortTemplate ConfigurationEmptyFort() { FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);return fort; } //현재 원문35개 섹션을 빈 입력으로 소유한다.
}

TEST_CASE(ConfigurationCommandInput_Native238EnvelopeAndCurrentReads) { //공통 Guide/Gump/뗌 뒤 순서와 Console/정지 독립성을 실제 PE 출력에 대조한다.
    const auto rows=LoadFixture(NETSTORM_CONFIGURATION_COMMAND_FIXTURE).rows;std::size_t count{};
    for (const auto& row:rows) { //각 행의 기대 trace/상태는 동결 원문 관측에서만 읽는다.
        if (row[0]=="event") continue;CHECK(row.size()==21);if (row.size()!=21) continue;++count;
        auto battle=Number(row[7]),editor=Number(row[8]),owner=Number(row[9]);auto priority=std::bit_cast<std::int32_t>(Number(row[10]));auto source=ConfigurationText(row[11]);std::array<std::uint8_t,28> state;state.fill(0xa5);std::string trace;std::map<std::string,std::uint32_t> calls;
        const auto add=[&](std::string name,std::string arguments=std::string{}) { //명시 하위 효과를 기록한 뒤 입력에 지정된 현재 메모리 변화만 적용한다.
            if (!trace.empty()) trace+=';';trace+=name+arguments;const auto trigger=name+std::to_string(++calls[name]);
            if (row[14]!="-") for (const auto& effect:Split(row[14],';')) { //원문 장치와 동일한 명시 mutation을 실제 C++ 저장소에 적용한다.
                const auto fields=Split(effect,'/');CHECK(fields.size()==3);if (fields.size()!=3 || fields[0]!=trigger) continue;const auto address=Number(fields[1]),value=Number(fields[2]);
                if (address==0x54db2c) priority=std::bit_cast<std::int32_t>(value);else if (address==0x540cac) owner=value;else if (address==0x594fbc) battle=value;else CHECK(false);
            }
        };
        ConfigurationCommandHooks hooks;hooks.text=[&](auto) { return std::string_view(source); };hooks.set=[&](auto key,auto value) { add("set",':'+ConfigurationHex(key)+':'+ConfigurationHex(value)); };
        hooks.get=[&](auto key) { add("get",':'+ConfigurationHex(key)+":0:0:0");return ConfigurationText(row[12]); };hooks.palette=[&](auto name) { add("palette",':'+ConfigurationHex(name)); };
        hooks.thread=[&](auto value) { add("current-thread");add("thread",":4294967294:"+std::to_string(value)); };hooks.config=[&](auto assignment) { add("config",':'+std::string(assignment)); };
        hooks.state=[&](auto slot) { add("state",':'+std::to_string(slot));return std::span(state); };hooks.notify=[&](auto slot) { add("notify",':'+std::to_string(slot)); };hooks.assertion=[&](auto line) { add("assert",':'+std::to_string(line)); };
        if (!Number(row[5]) && !Number(row[6])) CHECK(DispatchConfigurationCommandInput({Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))},{[&] { return battle; },[] { return 0U; },[&] { return editor; },[&] { return owner; },priority},hooks)); //MP challenge 경로는 이번 새 관측에 포함하지 않는다.
        CHECK((trace.empty() ? "-" : trace)==row[15]);CHECK(row[16]=="1" && row[17]=="0");CHECK(std::bit_cast<std::uint32_t>(priority)==Number(row[18]) && owner==Number(row[19]) && Hex(state)==row[20]);
    }
    CHECK(count==238);
}

TEST_CASE(ConfigurationCommandInput_Native30NamesAndRealQueue) { //원문 명령표/대소문자/숫자·고리 인자가 같은 일반40칸 큐에 도착한다.
    const auto rows=LoadFixture(NETSTORM_CONFIGURATION_NAMES_FIXTURE).rows;CHECK(rows.size()==30);CHECK(GameMenuCommandValue(0x54279c).value()==kChangePalCommand && GameMenuCommandValue(0x54291c).value()==kChangePriorityCommand && GameMenuCommandValue(0x542a10).value()==kSetFortTypeCommand);
    for (const auto& row:rows) { //문자열 토큰을 임의 포인터 또는 정수로 바꾸지 않는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};
        QueueScriptCommand(ConfigurationText(row[1]),queue,strings,{GameMenuScriptCode,[&](auto) { ++unknown; }});CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") { CHECK(queue.Empty() && unknown);continue; }
        const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && !unknown && queue.Empty() && IsGameMenuCommand(event.code));
        const auto token=std::bit_cast<std::uint32_t>(event.y);if (expected[1].starts_with('n')) CHECK(token==Number(expected[1].substr(1)));else CHECK(ConfigurationHex(strings.Resolve(token))==expected[1].substr(1));
    }
}

TEST_CASE(ConfigurationCommandInput_PrioritySameClientConfigReaderAndPartialEffects) { //새 쓰기·기존 설정 알림·메인 루프가 빌리는 priority DWORD를 분리 복제하지 않는다.
    Client client(ConfigurationClientOptions());auto& config=client.Configuration();config.Configuration().AppendSection("END");auto& priority=client.ThreadPriorityValue();const auto address=&priority;priority=73;
    config.SetInt("unrelatedPriorityKey",9);CHECK(priority==73 && address==&client.ThreadPriorityValue());
    for (const auto value:std::array<std::int32_t,6>{0,1,2,3,-1,std::numeric_limits<std::int32_t>::min()}) { //현재 Config의 signed32비트를 기존 ReadInt 경로가 같은 주소에 쓴다.
        config.SetInt("highPriorityValue",value);CHECK(priority==value && address==&client.ThreadPriorityValue());
    }
    ConfigurationCommandHooks hooks;std::string trace;hooks.thread=[&](auto value) { CHECK(value==15 && priority==2);priority=-7;trace+='T'; };hooks.config=[&](auto assignment) { CHECK(assignment=="highPriorityValue = \"-7\"");ApplyScriptConfig(assignment,config);trace+='C'; };
    CHECK(DispatchConfigurationCommandInput({kChangePriorityCommand,123,2},{{},{},{},{},priority},hooks));CHECK(priority==-7 && config.GetInt("highPriorityValue")==-7 && trace=="TC");
    hooks.thread=[&](auto value) { CHECK(value==0 && priority==-1);trace+='F';throw std::runtime_error("명시 OS 경계 중단"); };trace.clear();CHECK(ConfigurationFails([&] { DispatchConfigurationCommandInput({kChangePriorityCommand,0,-1},{{},{},{},{},priority},hooks); }));CHECK(priority==-1 && config.GetInt("highPriorityValue")==-7 && trace=="F");
    hooks.thread=[&](auto value) { CHECK(value==1 && priority==1);trace+='T'; };hooks.config=[&](auto) { trace+='X';throw std::runtime_error("명시 Config 경계 중단"); };trace.clear();CHECK(ConfigurationFails([&] { DispatchConfigurationCommandInput({kChangePriorityCommand,0,1},{{},{},{},{},priority},hooks); }));CHECK(priority==1 && config.GetInt("highPriorityValue")==-7 && trace=="TX");
    CHECK(DispatchConfigurationCommandInput({kChangePriorityCommand|InputCode::kRelease,0,2},{{},{},{},{},priority},{}));CHECK(priority==1 && !DispatchConfigurationCommandInput({'X',0,0},{{},{},{},{},priority},{}));
}

TEST_CASE(ConfigurationCommandInput_SetFortTypeActualStateNotifyAndSavedKindReader) { //같은 현재 manager의 새 State 버퍼와 실제 Notify/저장 이미지/UI 분류 reader를 끝까지 공유한다.
    CampaignTemplateStorage storage;storage.Install(1,ConfigurationEmptyFort(),"first.fort");storage.Install(2,ConfigurationEmptyFort(),"second.fort");std::uint32_t depth{},owner{1},editor{1},readOnly{},saveOwner{},saves{};bool mission{};std::int32_t priority{};std::array<std::uint8_t,9> hidden{};std::vector<std::uint8_t> saved;std::uint32_t savedSlot{};
    RawPlayerTemplateState state(OriginalEdition::Patch1078,storage.Headers(),[&](auto slot)->PlayerTemplateStateStream& { return storage.SelectState(slot); },[&](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,[](auto) { throw std::logic_error("명시State/Notify진단"); },[](auto) { CHECK(false); });
    state.BindInitialization({[](auto,std::span<std::uint8_t,28> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); },[&](auto slot,std::span<const std::uint8_t,28> bytes)->PlayerTemplateStateStream& { return storage.InitializeState(slot,bytes); }});state.BindMinimumSize([&](auto slot,auto minimum)->PlayerTemplateStateStream& { return storage.EnsureStateSize(slot,minimum); });
    state.BindFileSave([&](auto slot) { ++saves;savedSlot=slot;TemplateFileWriteHooks file;file.permissions=[](auto,auto mode) { CHECK(mode==0x180); };file.write=[&](auto,auto bytes) { saved.assign(bytes.begin(),bytes.end()); };storage.SaveFile(slot,{readOnly,saveOwner,hidden},{[] { return 188U; },[](auto) { CHECK(false); }},file); }); //전체 저장 부모는 메모리 장치에만 쓰며 디스크는 열지 않는다.
    ConfigurationCommandHooks hooks;hooks.state=[&](auto slot) { return state.Get(slot); };hooks.notify=[&](auto slot) { state.Changed(slot); };hooks.assertion=[](auto line) { CHECK(line==2012); };
    ConfigurationCommandReads reads{{},{},[&] { return editor; },[&] { return owner; },priority};CHECK(DispatchConfigurationCommandInput({kSetFortTypeCommand,99,257},reads,hooks));CHECK(state.Get(1)[17]==1 && saves==1 && savedSlot==1 && saved.size()>28);
    BaseFileSystem files(std::filesystem::path{});ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { return {}; });config.Configuration().AppendSection("END");config.Set("fortSpec","{local.1}.fort");SoloMapCatalogEffects effects;effects.typeCount=[] { return 188; };effects.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };effects.readFile=[&](auto name)->std::optional<std::vector<std::uint8_t>> { CHECK(name=="current.fort");return saved; };SoloMapCatalog catalog(files,config,std::move(effects));CHECK(catalog.Kind("current")==1); //메뉴 목록이 같은 저장 State+17과 원문 local.1 경로 치환을 실제 reader로 읽는다.
    state.Get(2);hooks.state=[&](auto slot) { const auto result=state.Get(slot);owner=2;return result; };CHECK(DispatchConfigurationCommandInput({kSetFortTypeCommand,0,254},reads,hooks));CHECK(state.Get(1)[17]==254 && state.Get(2)[17]==0 && savedSlot==2 && saves==2); //getter 뒤 owner 재조회로 첫 State 쓰기와 둘째 Notify를 분리한다.
    Put(storage.Headers(),2*20+8,1);CHECK(DispatchConfigurationCommandInput({kSetFortTypeCommand,0,7},reads,hooks));CHECK(state.Get(2)[17]==7 && saves==2); //기존 isMine의 실제 Notify 자연 반환은 파일 쓰기를 요구하지 않는다.
    const auto old=storage.CreateSection(1,"OldStateLifetime");storage.Clear(1);storage.Install(1,ConfigurationEmptyFort(),"replacement.fort");owner=1;hooks.state=[&](auto slot) { return state.Get(slot); };CHECK(DispatchConfigurationCommandInput({kSetFortTypeCommand,0,3},reads,hooks));CHECK(state.Get(1)[17]==3 && saves==3 && savedSlot==1 && storage.Name(1)=="replacement.fort");CHECK(ConfigurationFails([&] { storage.WriteSection(old,std::array<std::uint8_t,1>{9}); })); //같은 getter service는 새 manager를 읽고 반납된 참조는 거부한다.
}

TEST_CASE(ConfigurationCommandInput_ChangePalActualConfigAndExistingPaletteParent) { //현재 모드로 고른 Config 키를 Set 뒤 다시 읽어 실제 GamePalSpec/clear1/색 표 부모에 넘긴다.
    ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { return {}; });config.Configuration().AppendSection("END");config.Set("GamePalSpec","{local.1}.COL");std::uint32_t battle{},cache{},count{},renderer{1},dirty{};std::int32_t priority{};std::array<std::uint8_t,128> palette{},cloud{};std::vector<std::uint8_t> path;std::string scratch="requested",configured="substituted",trace,loaded;bool stop{};
    const auto text=[](auto bytes) { const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});return std::string(reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())); }; //NUL 경계를 가진 기존 부모 입력만 해독한다.
    FortPaletteTransitionHooks devices;devices.log=[&](auto,auto) { trace+='L'; };devices.compare=[&](auto first,auto second) { return text(first)==text(second) ? 0 : 1; };devices.release=[](auto) { CHECK(false); };devices.buildCloud=[&] { trace+='B';cache=1; };devices.stepCloud=[] { CHECK(false); };
    devices.pathSpec=[&](auto key,auto input,auto second,auto third) { CHECK(key=="GamePalSpec" && !second && !third);const auto value=config.PathSpec(key,text(input));path.assign(value.begin(),value.end());path.push_back(0);trace+='P';return std::span<const std::uint8_t>(path); };devices.loadPalette=[&](auto bytes,auto clear) { CHECK(clear==1);loaded=text(bytes);trace+='R';if (stop) throw std::runtime_error("명시palette reader실패"); };devices.buildMaps=[&] { trace+='M'; };
    ConfigurationCommandHooks hooks;hooks.text=[&](auto token) { CHECK(token==7);return std::string_view(scratch); };hooks.set=[&](auto key,auto requested) { CHECK(key=="fortPal" && requested=="requested");config.Set(key,configured);battle=1;scratch.assign(300,'X'); };hooks.get=[&](auto key) { CHECK(key=="fortPal");return config.PathSpec(key); };
    hooks.palette=[&](auto selected) { const std::string owned(selected);const std::span<const std::uint8_t> input(reinterpret_cast<const std::uint8_t*>(owned.c_str()),owned.size()+1);TransitionFortPalette(input,{palette,cloud,cache,count,renderer,dirty},devices); };
    CHECK(DispatchConfigurationCommandInput({kChangePalCommand,9,7},{[&] { return battle; },[] { return 0U; },{},{},priority},hooks));CHECK(loaded=="substituted.COL" && trace=="LBPRM" && dirty==1 && text(palette)=="substituted" && text(cloud)=="substituted");
    battle=0;scratch="requested";configured="changed";cache=0;dirty=9;trace.clear();stop=true;CHECK(ConfigurationFails([&] { DispatchConfigurationCommandInput({kChangePalCommand,0,7},{[&] { return battle; },[] { return 0U; },{},{},priority},hooks); }));CHECK(trace=="LBPR" && dirty==9 && cache==1 && config.PathSpec("fortPal")=="changed" && text(palette)=="changed"); //reader 중단 전에 설정/이름/구름 쓰기는 남고 maps/dirty는 수행하지 않는다.
}

#include "PriorityLoopTests.inc" //범위 밖 raw 요청과 반복 NORMAL의 원문96행을 확인한다.
