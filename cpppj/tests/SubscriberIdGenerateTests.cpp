// 최초PE1024행의실제CRT공유상태/연속호출/시각·rand장치/중단/재시도소진을대조한다.
#include "TestSupport.h"
#include "o/RawSubscriberIdGenerate.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace netstorm::o;
namespace {
// 원문fixture의입력과독립관측열을가공없이나눈다.
std::vector<std::string> IdFields(std::string_view source,char delimiter) {
    std::vector<std::string> result;std::size_t start{}; //마지막빈필드도유지한다.
    for (;;) { const auto end=source.find(delimiter,start);result.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return result;start=end+1; } //NUL/공백으로추가분리하지않는다.
}
// 부모에공급할명시rand응답만정의한다.제품의후보/반환기대는계산하지않는다.
std::uint32_t IdScripted(std::uint32_t profile,std::uint32_t ordinal) {
    if (ordinal<=100) return (profile&1U) ? 0xffffffffU : 0x7fffffffU;const auto index=ordinal-101,attempt=index/32,bit=31-index%32; //warmup도선택한명시입력을소비한다.
    constexpr std::array<std::uint32_t,8> signedValues{49,50,99,100,0xffffffffU,0xffffff9cU,0x80000000U,0x7fffffffU}; //signed idiv경계의원시DWORD다.
    constexpr std::array<std::uint32_t,4> thresholdValues{49,50,99,100}; //%100과50비교를구별한다.
    if (profile==8) return signedValues[index%8];if (profile==9) return 0xffffffffU;if (profile==10) return 0x80000000U;if (profile==11) return 0x7fffffffU;if (profile==12) return thresholdValues[index%4];
    const std::array<std::uint32_t,8> masks{65,0,64,attempt ? 65U : 0U,attempt<15 ? 0U : 65U,0xffffffffU,0x80000040U,attempt<15 ? 0x80000040U : 0x80000041U}; //재시도입력의각비트응답이며기대결과가아니다.
    return (masks.at(profile)&(std::uint32_t{1}<<bit)) ? 0U : 50U;
}
// 선택한하위의명시중단은정상실패DWORD반환과구별한다.
struct IdStop {};
// 원문전역/CRT의같은주소를유지하며외부장치입력만소유한다.
struct IdDevices {
    std::array<std::uint32_t,7> input{}; //mode/플래그/CRT/시각/profile/호출횟수의원시입력이다.
    std::uint32_t initialized{},crt{},randomCalls{}; //현재원본전역과같은holdrandDWORD,함수별난수장치순번이다.
    std::string mutation,trace; //명시외부변형과그때선택된실제호출trace다.
    // 출력열을읽지않고입력만으로현재장치를만든다.
    explicit IdDevices(const std::vector<std::string>& fields) { for (std::size_t index=0;index<input.size();++index) input[index]=std::stoul(fields[index]);initialized=input[1];crt=input[2];mutation=fields[7]; } //두번째함수호출때이상태를다시초기화하지않는다.
    // 실제경계진입순서와인자만기록한다.
    void Record(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
    // 원문바깥응답이바꾼현재전역/CRT와명시중단만적용한다.
    void Mutate(std::string_view boundary) {
        const auto separator=mutation.find('-');if (separator==std::string::npos) return;const auto trigger=mutation.substr(0,separator),action=mutation.substr(separator+1);std::string base(boundary);while (!base.empty() && base.back()>='0' && base.back()<='9') base.pop_back(); //rand별순번과단일시각장치를구별한다.
        if (trigger!=boundary && trigger!=base) return;
        if (action=="stop") throw IdStop{};
        if (action=="init7") initialized=7;else if (action=="init0") initialized=0;else if (action=="initffffffff") initialized=0xffffffffU;else if (action=="crt9") crt=9;else if (action=="crt0") crt=0;else throw std::logic_error("SubscriberID검사변형미해석");
    }
    // 기존CrtRandom을실제로빌리고관측/명시rawrand응답만그주위에추가한다.
    SubscriberIdGenerateHooks Hooks(CrtRandom& random) {
        SubscriberIdGenerateHooks effects; //시간/로그만명시외부장치다.
        effects.time32=[this] { Record("T");Mutate("time1");return std::bit_cast<std::int32_t>(input[3]); };
        effects.clock=[this] { Record("C");Mutate("clock1");return input[4]; };
        effects.expired=[this](auto level,auto message) { std::string text(message);const auto newline=text.find('\n');if (newline!=text.npos) text.replace(newline,1,"\\n");Record("E:"+std::to_string(level)+':'+text);Mutate("expired1"); };
        auto hooks=BindSubscriberIdCrtRandom(random,std::move(effects));const auto seed=hooks.seed;const auto next=hooks.next; //실제CRT몸체를재구현하지않고같은객체의현재상태를쓴다.
        hooks.seed=[this,seed](auto value) { Record("S:"+std::to_string(value));Mutate("seed1");seed(value); };
        hooks.next=[this,next] { ++randomCalls;Record("R:"+std::to_string(randomCalls)+':'+std::to_string(initialized)+':'+std::to_string(crt));Mutate("rand"+std::to_string(randomCalls));return input[0] ? std::bit_cast<std::int32_t>(IdScripted(input[5],randomCalls)) : next(); };return hooks;
    }
};
// 선택장치누락만별도로검사한다.
template<class Function> bool IdRejects(Function function) { try { function(); }catch (const std::logic_error&) { return true; }return false; }
}

// 최초1024행의현재플래그/CRT/한두번함수반환/실제외부trace를그대로비교한다.
TEST_CASE(SubscriberIdGenerate_IndependentNativeParentAndCrt) {
    std::ifstream fixture(NETSTORM_SUBSCRIBER_ID_GENERATE_FIXTURE);CHECK(fixture.good());std::string line;std::size_t compared{}; //독립원문행수만집계한다.
    while (std::getline(fixture,line)) { //제품이만든기대로fixture를덮거나후보를별도계산하지않는다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=IdFields(line,'\t');CHECK(row.size()==6);if (row.size()!=6) continue;IdDevices d(IdFields(row[1],','));auto random=CrtRandom::Borrow(d.crt);auto hooks=d.Hooks(random);std::string status="normal",results;
        try { for (std::uint32_t invocation=0;invocation<d.input[6];++invocation) { d.randomCalls=0;d.Record("I:"+std::to_string(invocation+1));const auto result=GenerateSubscriberId({d.initialized},hooks);if (!results.empty()) results+=',';results+=std::to_string(result); } }catch (const IdStop&) { status="stop"; } //호출프레임순번만초기화하고CRT와전역은이어쓴다.
        CHECK(status==row[2] && (results.empty() ? "-" : results)==row[3]);const auto state=IdFields(row[4],',');CHECK(state.size()==2);CHECK(d.initialized==std::stoul(state[0]) && d.crt==std::stoul(state[1]));CHECK(d.trace==row[5]);++compared;
    }
    CHECK(compared==1024);
}

// 비선택시각장치허용/시각중단의현재전역/seed뒤감김/612회소비뒤로그누락을확인한다.
TEST_CASE(SubscriberIdGenerate_SelectedDevicesAndPartialEffects) {
    std::uint32_t initialized=1,calls{};SubscriberIdGenerateHooks noTime;noTime.next=[&] { ++calls;return 0; };CHECK(GenerateSubscriberId({initialized},noTime)==0x07ffffffU && calls==132 && initialized==1); //기존비영플래그는time/clock/seed/log를요구하지않는다.
    initialized=0;SubscriberIdGenerateHooks missingClock;missingClock.time32=[&] { initialized=7;return -1; };CHECK(IdRejects([&] { static_cast<void>(GenerateSubscriberId({initialized},missingClock)); }));CHECK(initialized==7); //clock도달실패가앞선외부상태를되돌리지않는다.
    std::uint32_t shared=99;auto random=CrtRandom::Borrow(shared);SubscriberIdGenerateHooks effects;effects.time32=[] { return 0; };effects.clock=[] { return 1U; };auto bound=BindSubscriberIdCrtRandom(random,effects);const auto seed=bound.seed;bound.seed=[&](auto value) { initialized=0xffffffffU;seed(value); };bound.next={};initialized=0;CHECK(IdRejects([&] { static_cast<void>(GenerateSubscriberId({initialized},bound)); }));CHECK(initialized==0 && shared==1); //실제같은CRTseed뒤현재플래그감김을먼저완료한뒤warmup장치누락에멈춘다.
    initialized=1;calls=0;SubscriberIdGenerateHooks exhausted;exhausted.next=[&] { ++calls;return 50; };CHECK(IdRejects([&] { static_cast<void>(GenerateSubscriberId({initialized},exhausted)); }));CHECK(calls==612 && initialized==1); //16회소진은성공으로바꾸지않고실제로그장치위치에도달한다.
}
