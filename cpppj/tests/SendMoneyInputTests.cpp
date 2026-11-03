// 송금 원문888입력과 X/All의 현재 금액·취소·자금 부족을 대조한다.
#include "TestSupport.h"
#include "client/SendMoneyInput.h"
#include "client/ScriptCommand.h"
#include <array>
#include <bit>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

using namespace netstorm::client;
namespace { // 입력과 제품 효과만 구성하며 fixture 기대값으로 분기를 정하지 않는다.
// TSV의 마지막 필드까지 그대로 분리한다.
std::vector<std::string> MoneySplit(std::string_view text,char separator) { std::vector<std::string> out;std::size_t begin{};for (;;) { const auto end=text.find(separator,begin);out.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return out;begin=end+1; } } // 구분자 바로 다음 위치를 순회한다.
// 입력 DWORD의 폭을 보존한다.
std::uint32_t MoneyNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); }
// 실제 송신11BYTE를 원문 관측과 같은 소문자 hex로 기록한다.
std::string MoneyHex(std::span<const std::uint8_t> bytes) { constexpr std::string_view hex="0123456789abcdef";std::string out;for (auto value:bytes) { out+=hex[value>>4];out+=hex[value&15]; }return out; } // 각 실제 BYTE를 순서대로 두 글자에 옮긴다.
}

TEST_CASE(SendMoneyInput_Native888) { // 고정 세 금액의 실제 분기·차감·지갑·패킷 순서를 비교한다.
    std::ifstream input(NETSTORM_SEND_MONEY_INPUT_FIXTURE);CHECK(input.good());if (!input) return;std::string line;std::size_t count{}; // 새 독립 관측 파일과 비교 수다.
    while (std::getline(input,line)) { // 각 입력은 다른 현재 로컬/플래그/장치 응답을 설치한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;const auto row=MoneySplit(line,'\t');CHECK(row.size()==16);if (row.size()!=16) continue;++count;
        std::string trace;const auto add=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 제품에서 실제로 호출된 효과만 기록한다.
        SendMoneyHooks h;h.local=[&] { return MoneyNumber(row[3]); };h.flags=[&](auto owner) { CHECK(owner==MoneyNumber(row[2]));return MoneyNumber(row[4]); };h.checking=[&] { return MoneyNumber(row[11]); };h.mission=[&] { return MoneyNumber(row[10]); };
        h.canAfford=[&](float amount) { add("afford:"+std::to_string(std::bit_cast<std::uint32_t>(amount)));return MoneyNumber(row[5])!=0; };
        h.addOwner=[&](auto owner,float amount) { add("addOwner:"+std::to_string(owner)+':'+std::to_string(std::bit_cast<std::uint32_t>(amount))); };h.addLocal=[&](float amount) { add("addLocal:"+std::to_string(std::bit_cast<std::uint32_t>(amount))); };
        h.wallet=[&](auto owner,auto amount) { add("wallet:"+std::to_string(owner)+':'+std::to_string(amount)); };h.send=[&](auto packet) { add("send:9:"+MoneyHex(packet)); };h.assertion=[&] { add("assert"); };
        if (!MoneyNumber(row[6]) && !MoneyNumber(row[7])) static_cast<void>(DispatchSendMoneyInput({MoneyNumber(row[1]),71,std::bit_cast<std::int32_t>(MoneyNumber(row[2]))},h)); // 일반 명령은console 검사보다 앞이며 Guide/Gump가 소비한 사건만 제외한다.
        CHECK((trace.empty() ? "-" : trace)==row[13]);CHECK(row[14]=="1" && row[15]=="0");
    }
    CHECK(count==888);
}

TEST_CASE(SendMoneyInput_AllReadsCurrentAmountAndXAffordability) { // All은 확인 순간의 SP를 읽고 원문처럼 정수 부분만 송금한다.
    float sp=1234.75F;std::int32_t wallet{};std::uint32_t reads{},checks{},dialogs{},asks{};SendMoneyHooks h; // 같은 실행기에서 바뀌는 현재 경제다.
    h.money=[&] { ++reads;return sp; };h.known=[] { return 1U; };h.canAfford=[&](float amount) { ++checks;return amount<=sp; };h.flags=[](auto owner) { CHECK(owner==2);return 0x22U; };h.checking=[] { return 1U; };h.mission=[] { return 1U; };
    h.addLocal=[&](float amount) { sp+=amount; };h.wallet=[&](auto owner,auto amount) { CHECK(owner==2);wallet+=amount; };h.numberDialog=[&](auto owner) { CHECK(owner==2);++dialogs; };h.askAll=[&](auto owner) { CHECK(owner==2);++asks; };
    CHECK(DispatchSendMoneyInput({0x02000100,0,2},h));CHECK(dialogs==1 && sp==1234.75F && !wallet);
    CHECK(DispatchSendMoneyInput({0x0200012b,0,2},h));CHECK(asks==1 && sp==1234.75F && !wallet);
    CHECK(DispatchSendMoneyInput({0x02000118,0,2},h));CHECK(reads==2 && checks==1 && sp==0.75F && wallet==1234);
    CHECK(ConfirmSendMoney(2,"9999999999",h)==1 && sp==0.75F && wallet==1234);CHECK(ConfirmSendMoney(2,"0",h)==1 && sp==0.75F);
    sp=1000;CHECK(ConfirmSendMoney(2,"700\n999",h)==1 && sp==300 && wallet==1934);CHECK(ConfirmSendMoney(2,"301",h)==1 && sp==300);
}

TEST_CASE(SendMoneyInput_ReleaseAndUnrelatedNeedNoDevices) { // 뗌과 다른 입력은 월드/돈 장치를 요구하지 않는다.
    const std::array<std::pair<std::string_view,std::uint32_t>,6> names{{{"Send300Bucks",0x02000120U},{"Send700Bucks",0x02000108U},{"SendBucks",0x020000ecU},{"sEnDxBuCkS",0x02000100U},{"SendAllBucks2",0x0200012bU},{"SendAllBucks",0x02000118U}}}; // 독립 원본 이름표의 서로 다른 여섯 코드를 검사한다.
    for (const auto& [name,code]:names) { // 같은 parser/큐의 대상 보존과 실제 일반 입력의 뗌 관문을 함께 대조한다.
        CHECK(SendMoneyScriptCode(name)==code && IsSendMoneyCommand(code));InputQueue queue;ScriptStringRing strings;
        QueueScriptCommand(std::string(name)+",2",queue,strings,{SendMoneyScriptCode,{}});const auto event=queue.Pop(false);
        CHECK(event.code==code && event.x==2 && event.y==2 && queue.Empty());CHECK(DispatchSendMoneyInput({code|0x80000000U,0,0},{}));
    }
    CHECK(!DispatchSendMoneyInput({123,0,0},{}));CHECK(SendMoneyScriptCode("SendAllBucks")==0x02000118U);CHECK(SendMoneyScriptCode("sendallbucks")==0x02000118U);CHECK(SendMoneyScriptCode("Unknown")==0xffffffffU);
}

// 기존 원문888행 중 비멀티 AI flags20/22의312행만 대조하며 패킷 장치는 선택하면 실패한다.
TEST_CASE(SendMoneyInput_CheckingAiNative312) {
    std::ifstream input(NETSTORM_SEND_MONEY_INPUT_FIXTURE);CHECK(input.good());if (!input) return;std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 기존 fixture의 입력/기대값을 바꾸지 않고 AI 가지를 선택한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;const auto row=MoneySplit(line,'\t');CHECK(row.size()==16);if (row.size()!=16) return;const auto flags=MoneyNumber(row[4]);if (flags!=0x20U && flags!=0x22U) continue;++count;std::string trace;
        const auto add=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; };SendMoneyHooks hooks;hooks.local=[&] { return MoneyNumber(row[3]); };hooks.flags=[&](auto owner) { CHECK(owner==MoneyNumber(row[2]));return flags; };hooks.checking=[&] { return MoneyNumber(row[11]); };hooks.mission=[&] { return MoneyNumber(row[10]); };hooks.canAfford=[&](float amount) { add("afford:"+std::to_string(std::bit_cast<std::uint32_t>(amount)));return MoneyNumber(row[5])!=0; };
        hooks.addLocal=[&](float amount) { add("addLocal:"+std::to_string(std::bit_cast<std::uint32_t>(amount))); };hooks.wallet=[&](auto owner,auto amount) { add("wallet:"+std::to_string(owner)+':'+std::to_string(amount)); };hooks.assertionAt=[&](auto expression,auto file,auto sourceLine) { CHECK(expression=="inSingle" && file==".\\Userinput.cpp");const auto code=MoneyNumber(row[1])&0x03ffffffU;CHECK(sourceLine==(code==0x02000120U ? 0x913U : code==0x02000108U ? 0x8e9U : 0x8c2U));add("assert"); }; // 줄 번호는 실제 기계어 PUSH로 확인했다.
        hooks.addOwner=[](auto,auto) { throw std::logic_error("제외한 연결 Player 차감"); };hooks.send=[](auto) { throw std::logic_error("제외한 패킷 송금"); };
        if (!MoneyNumber(row[6]) && !MoneyNumber(row[7])) CHECK(DispatchSendMoneyInput({MoneyNumber(row[1]),71,std::bit_cast<std::int32_t>(MoneyNumber(row[2]))},hooks)); // Guide/Gump가 이미 소비한 원문 행은 일반 명령을 실행하지 않는다.
        CHECK((trace.empty() ? "-" : trace)==row[13] && row[14]=="1" && row[15]=="0");
    }CHECK(count==312);
}
