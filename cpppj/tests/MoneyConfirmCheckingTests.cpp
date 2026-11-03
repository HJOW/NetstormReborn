// 숫자 확인의 비멀티432행을 실제CPU와 비교하며 연결 플레이어/패킷 가지는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "client/SendMoneyInput.h"
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 입력/현재 장치만 구성하며 기대 결과는 보호 fixture에서 읽는다.
// 두 SP 조회와 창+d4/현재 checking의 독립 변화를 한 호출 동안 유지한다.
struct MoneyConfirmScene {
    std::uint32_t known{},first{},second{},checking{},mission{},flags{},editor{},target{},local{},action{},reads{}; // 원문에 공급한 현재 DWORD 입력이다.
    std::string text,events;SendMoneyHooks hooks; // 문자 입력과 실제 외부 호출 순서다.
    explicit MoneyConfirmScene(const std::vector<std::string>& row) { // fixture 출력은 입력 조립에 사용하지 않는다.
        text=Decode(row[1]);known=Number(row[2]);first=Number(row[3]);second=Number(row[4]);checking=Number(row[5]);mission=Number(row[6]);flags=Number(row[7]);editor=Number(row[8]);target=Number(row[9]);local=Number(row[10]);action=Number(row[11]);
        hooks.known=[this] { return known; };hooks.money=[this] { return Money(); };hooks.canAfford=[this](float amount) { const auto current=Money();return editor || amount<=current; };
        hooks.flags=[this](auto) { return flags; };hooks.checking=[this] { return checking; };hooks.mission=[this] { return mission; };
        hooks.assertion=[this] { events+="A,1564:inSingle:.\\Ui.cpp;";if (action&4U) { checking=0;mission=1;target=8; } };
        hooks.addLocal=[this](float amount) { events+="L,"+std::to_string(std::bit_cast<std::uint32_t>(amount))+';';if (action&8U) target=5; };hooks.wallet=[this](auto owner,auto amount) { events+="W,"+std::to_string(owner)+','+std::to_string(amount)+';'; };
    }
    float Money() { const auto index=reads++;const auto value=index ? second : first;events+="M,"+std::to_string(local)+','+std::to_string(value)+';';if ((!index && (action&1U)) || (index && (action&2U))) { checking=0x80000000;mission=0;target=index ? 4U : 3U; }return std::bit_cast<float>(value); } // 실제 getter 응답 뒤 현재 입력을 바꾼다.
    static std::string Decode(std::string_view hex) { if (hex=="-") return {};std::string result;for (std::size_t index=0;index<hex.size();index+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16));return result; } // 입력 HEX의 각 두 글자를 한BYTE로 읽는다.
    std::vector<std::string> Run() { const auto result=ConfirmSendMoney(target,text,hooks);return {std::to_string(result),std::to_string(checking),std::to_string(mission),std::to_string(target),std::to_string(reads),events.empty() ? "-" : events}; } // 실제 반환/현재값/순서만 출력한다.
};
const auto& MoneyConfirmRows() { static const auto rows=LoadFixture(NETSTORM_MONEY_CONFIRM_CHECKING_FIXTURE).rows;return rows; } // 최초432행을 읽기 전용으로 사용한다.
}
// 현재 checking/두 SP·대상 재조회와 진단 뒤 저장 대상을 실제 원문 부모에 대조한다.
TEST_CASE(MoneyConfirmChecking_NativeAiAndUnselected432) {
    std::size_t count{};
    for (const auto& row:MoneyConfirmRows()) { // 첫 불일치의 원시 입력/기대값을 유지한다.
        CHECK(row.size()==18);MoneyConfirmScene scene(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+12,row.end());CHECK(actual==expected);
        if (actual!=expected) { /* 원시 기대값과 다른 각 출력 필드를 기록한다. */ for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("송금 숫자 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count; // 불일치한 출력 필드만 기록한다.
    }CHECK(count==432);
}
// 선택되지 않은 진단/자금 장치는 요구하지 않고 실패 전 차감 효과는 유지한다.
TEST_CASE(MoneyConfirmChecking_SelectedDevicesAndPartialEffects) {
    SendMoneyHooks empty;CHECK(ConfirmSendMoney(2,"0",empty)==1);empty.known=[] { return 0U; };CHECK(ConfirmSendMoney(2,"700",empty)==1);
    auto row=MoneyConfirmRows().front();row[1]="373030";row[2]="1";row[5]="0";row[6]="0";row[7]="32";MoneyConfirmScene skipped(row);skipped.hooks.mission={};skipped.hooks.assertion={};CHECK(skipped.Run()[0]=="1" && skipped.events.find("W,2,700;")!=std::string::npos);
    row[5]="2147483648";MoneyConfirmScene stopped(row);stopped.hooks.assertion=[&] { stopped.checking=0;throw std::runtime_error("원문 진단 중단"); };CHECK(Throws([&] { stopped.Run(); }));CHECK(!stopped.checking && stopped.reads==2 && stopped.events.find("L,")==std::string::npos);
    row[5]="0";MoneyConfirmScene noWallet(row);noWallet.hooks.wallet={};CHECK(Throws([&] { noWallet.Run(); }));CHECK(noWallet.events.find("L,")!=std::string::npos && noWallet.events.find("W,")==std::string::npos);
}
