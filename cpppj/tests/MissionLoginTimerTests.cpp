// 독립 보호 PE의 로그인 타이머/집계/Begin/Abort와 전체 상태·외부 경계 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissionLoginTimer.h"
#include "client/ClientLocalStateGlobals.h"
#include <array>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint32_t,16> kLoginFlags{0,1,2,3,0x40,0x41,0x42,0x43,0x100,0x101,0x102,0x103,0x140,0x142,0x143,0xffffffff}; // native의 명시 flags입력이다.
constexpr std::array<std::string_view,6> kLoginModes{"saved","server","mission","post","geysers","asserts"}; // 현재 모드 실제 읽기 trace 순서의 이름이다.
// 원본 double의8BYTE 비트를 그대로 직렬화한다.
std::string LoginDouble(double value) { return Hex(std::bit_cast<std::array<std::uint8_t,8>>(value)); }
// fixture의double8BYTE를 유효 길이 확인 뒤 그대로 읽는다.
double LoginReadDouble(std::string_view value) {
    if (value.size()!=16) throw std::invalid_argument("로그인 double 입력 크기 오류");std::array<std::uint8_t,8> bytes{};
    // 두 hex글자씩 원문 바이트 순서로 읽는다.
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(std::string(value.substr(index*2,2)),nullptr,16));
    return std::bit_cast<double>(bytes);
}
// 원문 로그의 문자 바이트를 변환 없이 trace에 남긴다.
std::string LoginText(std::string_view value) { return Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()),value.size())); }
// 실제 기록/현재 모드/효과의 독립 입력이며 타이머의 성공/대기 결과를 미리 계산하지 않는다.
struct LoginTimerScene {
    std::array<std::uint32_t,14> numbers{}; // pattern/configTimeout/6모드와begin/connection/trigger/action/value/budget다.
    std::array<std::uint8_t,9*0xb4> players{}; // 상위 로그인과 타이머가 공유할 같은 실제9 Player 원문이다.
    std::array<bool,6> modes{}; // 각 원문 읽기 위치에 공급하는 현재 bool 상태다.
    double end{},wall{}; // 실제 예약/현재 벽시각의 비트 보존 입력이다.
    std::int32_t timeout{0x76543210}; // Start 전의 실제 기존 timeout stale 입력이다.
    std::uint32_t begin{},scriptBreak{0xabcdef01},phase{0x11223344},clock{0x12345678},budget{}; // 동일 실제 상태와 외부 변경 한도다.
    std::string trace; // 경계 진입 전 상태와 실제 모드 읽기 순서를 누적한다.
    // 독립 원문 입력을 읽고0..8의subscriber/flags만 명시값으로 준비한다.
    explicit LoginTimerScene(std::string_view input) {
        const auto fields=Split(input,'|'),words=Split(fields.at(0),',');if (fields.size()!=3 || words.size()!=numbers.size()) throw std::invalid_argument("로그인 타이머 입력 크기 오류");
        // 원문 DWORD의 부호 비트를 보존한다.
        for (std::size_t index=0;index<numbers.size();++index) numbers[index]=Number(words[index]);
        end=LoginReadDouble(fields[1]);wall=LoginReadDouble(fields[2]);begin=numbers[6];budget=numbers[13];
        modes={numbers[2]!=0,numbers[3]!=0,numbers[4]!=0,numbers[5]!=0,numbers[7]!=0,numbers[8]!=0};
        // 집계가 읽지 않는 전체 Player stale BYTE도 native와 같은 독립 입력으로 유지한다.
        for (std::size_t index=0;index<players.size();++index) players[index]=static_cast<std::uint8_t>(index*37+numbers[0]*11);
        // Player0을 포함한 실제9기록에 명시 subscriber/flags를 넣는다.
        for (std::uint32_t owner=0;owner<9;++owner) Player(owner,0,0);Player(0,0xdeadbeef,0xffffffff);
        const auto pattern=numbers[0];
        if (pattern>=16) { const auto ordinal=pattern-16;constexpr std::array<std::uint32_t,3> subs{0,7,0xffffffff}; // 0/양수/높은비트 subscriber 입력이다.
            Player(ordinal/48+1,subs[ordinal/16%3],kLoginFlags[ordinal%16]);
        } else if (pattern==1) Player(1,7,3);
        else if (pattern==2) Player(1,7,0x43);
        else if (pattern==3) { Player(1,7,3);Player(2,8,3); }
        else if (pattern==4) { Player(1,7,0x43);Player(2,8,0x102); }
        else if (pattern==5) Player(1,7,2);
        else if (pattern==6) Player(1,7,0x100);
        else if (pattern>=7 && pattern<=9) {
            // 전체8명이 연결/ready인 입력과 subscriber0의stale flags 입력을 구분한다.
            for (std::uint32_t owner=1;owner<9;++owner) Player(owner,pattern==9 ? 0U : owner,pattern==7 ? 0U : pattern==8 ? 0x43U : 0xffffffffU);
        }
    }
    // 원문 위치의 명시 Player 입력만 쓴다. 현재 기록 사본을 만들어 반환하지 않는다.
    void Player(std::uint32_t owner,std::uint32_t subscriber,std::uint32_t flags) { Put(players,owner*0xb4+0x2c,subscriber);Put(players,owner*0xb4+0x50,flags); }
    // 실제 외부 진입 순서를 이어 붙인다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 효과 직전의 실제 쓰기 순서 관찰값이다.
    std::string Snapshot() const { return LoginDouble(end)+','+std::to_string(begin)+','+std::to_string(scriptBreak)+','+std::to_string(phase)+','+std::to_string(clock); }
    // 입력에 지정된 외부 효과만 변경하며 제품의 타이머/집계 분기를 대신 계산하지 않는다.
    void Mutate(std::uint32_t trigger) {
        if (trigger!=numbers[10] || budget==0) return;--budget;const auto action=numbers[11],value=numbers[12];
        if (action==1) modes[1]=!modes[1];else if (action==2) modes[3]=!modes[3];else if (action==3) modes[0]=!modes[0];else if (action==4) modes[2]=!modes[2];
        else if (action==5) begin=value;else if (action==6) end=static_cast<double>(value);else if (action==7) Put(players,0xb4+0x50,0x43);
        else if (action==8) phase=value;else if (action==9) scriptBreak=value;
    }
    // 같은 상태의 입력 변경은 원문 외부 효과 진입 관찰 뒤에만 수행한다.
    void Event(std::string value,std::uint32_t trigger=0) { Add(value+','+Snapshot());Mutate(trigger); }
    // native가 실제 모드DWORD를 읽는 각 위치에 현재 bool값을 공급한다.
    bool Mode(std::size_t index) { Add("G,"+std::string(kLoginModes[index])+','+std::to_string(modes[index]));return modes[index]; }
    // DWORD로재해석하지않는현재모드조회기를실제각소비위치에연결한다.
    MissionLoginTimerReads Reads() {
        return {[this] { return Mode(0); },[this] { return Mode(1); },[this] { return Mode(2); },[this] { return Mode(3); },[this] { return Mode(4); },[this] { return Mode(5); }};
    }
    // 동일 실제 상태의 참조를 빌리며 scalar 사본을 전달하지 않는다.
    MissionLoginTimerState State() { return {end,timeout,begin,scriptBreak,phase,clock}; }
    // 실제 시간/전송/사제/UI는 명시 입력 경계이며 타이머의 비교/쓰기/반복은 제품이 수행한다.
    MissionLoginTimerHooks Hooks() {
        MissionLoginTimerHooks hooks;
        hooks.configInt=[this](std::string_view key) { CHECK(key=="loginTimeout");Event("Q");return std::bit_cast<std::int32_t>(numbers[1]); };
        hooks.wallSeconds=[this] { Event("W",10);return wall; };hooks.clock=[this] { Event("C",1);return 0x87654321U; };
        hooks.connection=[this](std::int32_t peer) { CHECK(peer==9);Event("N,9",2);return numbers[9]!=0; };
        hooks.remoteStatus=[this](std::int32_t peer,std::uint32_t first,std::uint32_t second) { CHECK(peer==9 && first==0 && second==0);Event("R,9,0,0",3); };
        hooks.broadcast=[this](std::span<const std::uint8_t> packet) { Event("B,"+Hex(packet),4); };
        hooks.priestRefresh=[this](std::uint32_t owner) { Event("P,"+std::to_string(owner),5); };
        hooks.saveReconnect=[this] { Event("S",6); };hooks.closeProgress=[this] { Event("X",7); };hooks.resetCamera=[this] { Event("V"); };
        hooks.abortBattle=[this] { Event("A",11); };hooks.progress=[this] { Event("U"); };hooks.serverAssertion=[this] { Event("J",9); };
        hooks.log=[this](MissionLoginTimerLog kind,std::string_view text) { Event("L,"+std::to_string(kind==MissionLoginTimerLog::Status)+','+LoginText(text),kind==MissionLoginTimerLog::Status ? 8U : 0U); };
        return hooks;
    }
    // native가 읽은 모든 타이머/모드 저장소를 동일 순서로 직렬화한다.
    std::string Observe() const {
        std::string result=LoginDouble(end)+'|'+std::to_string(std::bit_cast<std::uint32_t>(timeout))+','+std::to_string(begin)+','+std::to_string(scriptBreak)+','+std::to_string(phase)+','+std::to_string(clock);
        // bool은 원문의0/1 DWORD입력과 같은 의미값으로 기록한다.
        for (const auto mode:modes) result+=','+std::to_string(mode);return result;
    }
};
}
// 5집계·실제만료·Start/Begin/Abort/Run의 독립 정상 반환 출력과 모든현재읽기/외부효과순서를 비교한다.
TEST_CASE(MissionLoginTimer_Native1984) {
    std::size_t compared=0;
    // 기대 결과는 보호 PE의 실제 출력이며 제품 규칙으로 다시 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_LOGIN_TIMER_FIXTURE).rows) {
        if (row[0]=="kind") continue;CHECK(row.size()==5);LoginTimerScene scene(row[1]);MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players);
        RawMissionLoginTimer timer(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),scene.Hooks());std::string result="-";
        if (row[0]=="C") { const auto before=scene.players;result=std::to_string(counts.Subscribers())+','+std::to_string(counts.Participating())+','+std::to_string(counts.Disconnected())+','+std::to_string(counts.Ready())+','+std::to_string(counts.LoggedIn());CHECK(scene.players==before); }
        else if (row[0]=="E") result=std::to_string(timer.Expired());else if (row[0]=="B") timer.Begin();else if (row[0]=="A") timer.Abort();
        else if (row[0]=="R" || row[0]=="S") timer.Run(row[0]=="S");else throw std::invalid_argument("로그인 타이머 원문 함수 종류 오류");
        CHECK(result==row[2]);CHECK(scene.Observe()==row[3]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[4]);++compared;
    }
    CHECK(compared==1984);
}
// 실제missionPost는 미ready/미만료에서도Begin만생략한뒤사제상태와로그인종료를실행한다.
TEST_CASE(MissionLoginTimer_UnreadyMissionPostFinishesWithoutBegin) {
    LoginTimerScene scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002540|0000000000002440");
    MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players);RawMissionLoginTimer timer(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),scene.Hooks());
    CHECK(counts.Ready()==0 && counts.LoggedIn()==1);timer.Run(false);CHECK(scene.end==0.0 && scene.begin==0 && scene.scriptBreak==0);
    CHECK(scene.trace.find("B,")==std::string::npos && scene.trace.find("U,")==std::string::npos);
    CHECK(scene.trace.find("P,1,")!=std::string::npos && scene.trace.find("P,8,")!=std::string::npos && scene.trace.find("S,")!=std::string::npos);
}
// inactive0는현재모드/시간/전송/진단등어떤외부경계도읽지않으며누락효과가있어도선택하지않는다.
TEST_CASE(MissionLoginTimer_InactiveReadsNothing) {
    LoginTimerScene scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000000080|0000000000002440");
    MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players);RawMissionLoginTimer timer(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),{});
    const auto before=scene.Observe();timer.Run(false);CHECK(scene.trace.empty() && scene.Observe()==before);
}
// Begin송신누락과Abort하위누락은실제앞선쓰기만남기고성공/후속호출로진행하지않는다.
TEST_CASE(MissionLoginTimer_MissingSelectedConsumersStopInOrder) {
    LoginTimerScene scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440");MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players);
    auto hooks=scene.Hooks();hooks.broadcast={};RawMissionLoginTimer begin(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),hooks);
    CHECK(Throws([&] { begin.Begin(); }));CHECK(scene.end==0.0 && scene.begin==0);
    hooks=scene.Hooks();hooks.abortBattle={};RawMissionLoginTimer abort(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),hooks);
    CHECK(Throws([&] { abort.Abort(); }));CHECK(scene.begin==0 && scene.end==0.0 && scene.scriptBreak==1 && scene.phase==4);
    CHECK(scene.trace.find("X,")==std::string::npos);
}
// 일반초기화미복원을실제미션시작성공으로가장하지않고같은선택위치에서명시거부한다.
TEST_CASE(MissionLoginTimer_RejectsGeneralInitializationAndInvalidStorage) {
    LoginTimerScene scene("2,30,0,1,0,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440");
    CHECK(Throws([&] { MissionLoginPlayerCounts counts(OriginalEdition::Cd1072,scene.players); }));
    CHECK(Throws([&] { MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,std::span(scene.players).first(scene.players.size()-1)); }));
    MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players);RawMissionLoginTimer timer(OriginalEdition::Patch1078,scene.State(),counts,scene.Reads(),scene.Hooks());
    CHECK(Throws([&] { timer.Run(false); }));CHECK(scene.begin==1 && scene.end==0.0);
    CHECK(scene.trace.find("B,016e080000000080,")!=std::string::npos && scene.trace.find("P,1,")==std::string::npos);
    auto reads=scene.Reads();reads.server={};CHECK(Throws([&] { RawMissionLoginTimer missing(OriginalEdition::Patch1078,scene.State(),counts,reads,scene.Hooks()); }));
}

// Abort의1쓰기 뒤 실제 하위 효과가 같은 Client 값을 바꾸면 뒤꼬리도 현재 값을 보존한다.
TEST_CASE(MissionLoginTimer_ClientScriptBreakAbortUsesCurrentReferenceAndKeepsOtherDeadlines) {
    netstorm::client::ClientLocalStateGlobals owner;LoginTimerScene scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440");MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players); // 기존 실제 Player 집계와 별도 Client 수명을 사용한다.
    owner.Begin()=17;owner.LoginEnd()=23.5;owner.ScriptBreak()=0xffffffffU;owner.StateAuxCC()=0xaabbccddU;owner.MetadataMode()=0x80000000U;owner.ConnectEnd()=37.25;owner.ActivityEnd()=41.5;owner.ReconnectSaveEnd()=std::bit_cast<double>(std::uint64_t{0x7ff800005555aaaaULL});
    const auto reconnectBits=std::bit_cast<std::uint64_t>(owner.ReconnectSaveEnd());std::uint32_t steps{};auto hooks=scene.Hooks(); // 외부 효과의 실제 전후 쓰기 순서만 관측한다.
    hooks.abortBattle=[&] { CHECK(owner.ScriptBreak()==1 && owner.Begin()==0 && owner.LoginEnd()==0.0 && scene.phase==4 && steps++==0);owner.ScriptBreak()=0xffffffffU;owner.Begin()=29;scene.phase=7; };
    hooks.closeProgress=[&] { CHECK(owner.ScriptBreak()==0xffffffffU && owner.Begin()==29 && scene.phase==7 && steps++==1);owner.ScriptBreak()=0x80000000U; };
    const MissionLoginTimerState state{owner.LoginEnd(),scene.timeout,owner.Begin(),owner.ScriptBreak(),scene.phase,scene.clock};RawMissionLoginTimer timer(OriginalEdition::Patch1078,state,counts,scene.Reads(),hooks);timer.Abort();
    CHECK(steps==2 && owner.ScriptBreak()==0x80000000U && owner.Begin()==29 && scene.phase==7 && owner.LoginEnd()==0.0);CHECK(owner.StateAuxCC()==0xaabbccddU && owner.MetadataMode()==0x80000000U && owner.ConnectEnd()==37.25 && owner.ActivityEnd()==41.5 && std::bit_cast<std::uint64_t>(owner.ReconnectSaveEnd())==reconnectBits);
    hooks.abortBattle={};hooks.closeProgress=[&] { ++steps;throw std::logic_error("선택되지 않아야 할 Abort 꼬리"); };RawMissionLoginTimer missing(OriginalEdition::Patch1078,state,counts,scene.Reads(),hooks);owner.ScriptBreak()=0;owner.Begin()=53;owner.LoginEnd()=67.75;
    CHECK(Throws([&] { missing.Abort(); }));CHECK(owner.ScriptBreak()==1 && owner.Begin()==0 && owner.LoginEnd()==0.0 && scene.phase==4 && steps==2 && owner.StateAuxCC()==0xaabbccddU && std::bit_cast<std::uint64_t>(owner.ReconnectSaveEnd())==reconnectBits); // 미연결 하위를 숨기지 않고 앞선 실제 부분 효과만 남긴다.
}

// 완료0 store는 사제/로그 효과 뒤에 발생하며 뒤 효과의 현재 값 변경을 다시0으로 덮지 않는다.
TEST_CASE(MissionLoginTimer_ClientScriptBreakCompletionUsesCurrentReferenceAfterEffects) {
    netstorm::client::ClientLocalStateGlobals owner;LoginTimerScene scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002540|0000000000002440");MissionLoginPlayerCounts counts(OriginalEdition::Patch1078,scene.players); // 미ready/미만료의 기존 missionPost 자연 완료 분기를 선택한다.
    owner.ScriptBreak()=0xffffffffU;owner.LoginEnd()=10.5;owner.Begin()=0;owner.ReconnectSaveEnd()=47.125;owner.ActivityEnd()=-0.0;std::uint32_t priests{},steps{};auto hooks=scene.Hooks(); // 8610/544d58은 LoginEnd005c85f0와 별개의 장기 입력이다.
    hooks.priestRefresh=[&](std::uint32_t player) { CHECK(player==++priests && owner.ScriptBreak()==(player==1 ? 0xffffffffU : 0x80000000U));owner.ScriptBreak()=0x80000000U; };
    hooks.saveReconnect=[&] { CHECK(priests==8 && owner.ScriptBreak()==0 && owner.Begin()==0 && owner.LoginEnd()==10.5 && steps++==0);owner.ScriptBreak()=0xffffffffU; };
    hooks.closeProgress=[&] { CHECK(owner.ScriptBreak()==0xffffffffU && owner.LoginEnd()==0.0 && steps++==1);owner.ScriptBreak()=0x80000000U; };
    RawMissionLoginTimer timer(OriginalEdition::Patch1078,{owner.LoginEnd(),scene.timeout,owner.Begin(),owner.ScriptBreak(),scene.phase,scene.clock},counts,scene.Reads(),hooks);timer.Run(false);
    CHECK(priests==8 && steps==2 && owner.ScriptBreak()==0x80000000U && owner.LoginEnd()==0.0 && owner.ReconnectSaveEnd()==47.125 && std::bit_cast<std::uint64_t>(owner.ActivityEnd())==0x8000000000000000ULL);CHECK(scene.trace.find("B,")==std::string::npos && scene.trace.find("U,")==std::string::npos); // 미선택 Begin/진행 UI를 가짜 호출로 채우지 않는다.
}
