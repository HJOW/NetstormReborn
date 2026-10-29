// 실제 원문 부모의 고정 관측과 같은 지역60BYTE·외부 순서·부분중단을 대조한다.
#include "TestSupport.h"
#include "client/ChatRecipients.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace { // 원문 입력/출력 인코딩만 읽으며 기대 반환값을 계산하지 않는다.
using namespace netstorm::client;
// 원문 TSV의 빈 열도 순서대로 보존한다.
std::vector<std::string> RecipientFields(std::string_view text) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { // 각 TAB 사이의 원시 열을 그대로 복사한다.
        const auto end=text.find('\t',start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1;
    }
}
// 최초 원문의 바이트 입력을 유니코드 변환 없이 복원한다.
std::string RecipientBytes(std::string_view text) {
    if (text=="-") return {};std::string result;
    for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16)); // 두 hex문자를 한BYTE로 읽는다.
    return result;
}
// 미작성 꼬리도 포함해 원문 지역 전체를 같은 hex로 표현한다.
std::string RecipientHex(std::string_view text) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //원문 단일BYTE의 고정표다.
    for (const unsigned char byte:text) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; //각 BYTE를 생략 없이 기록한다.
}
// 실제 하위 중단은 부모 정상복귀와 구별한다.
struct RecipientStop { std::string where; }; //원문 장치에서 선택된 중단 이름이다.
}

// 실제350관측의 반환/지역/전체 외부 순서와 현재 번역·선택중단을 한 번씩 비교한다.
TEST_CASE(ChatRecipients_IndependentNative350) {
    std::ifstream input(NETSTORM_CHAT_RECIPIENTS_FIXTURE);if (!input) throw std::runtime_error("채팅 대상 원문 관측 없음");std::string line;std::getline(input,line);std::size_t count{},stops{};
    while (std::getline(input,line)) { // 최초 관측 각 행의 입력으로만 현재 장치를 구성한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=RecipientFields(line);CHECK(row.size()==8);if (row.size()!=8) throw std::runtime_error("채팅 대상 관측 열수 오류");
        const auto manager=std::stoul(row[0]),mode=std::stoul(row[2]);const auto text=RecipientBytes(row[1]=="~" ? "-" : row[1]);netstorm::o::PlayerIdentityRecord scratch;scratch.fill(0xa5);
        std::vector<std::string> trace;std::size_t resolutions{};ChatRecipientHooks hooks;
        hooks.resolve=[&](auto& record) { const std::string name(reinterpret_cast<const char*>(record.data()+8));trace.push_back("R:"+RecipientHex(name));++resolutions;if (mode==5 && resolutions==2) throw RecipientStop{"resolve"};if (mode==4 && name=="Unknown") { constexpr std::string_view replacement="EVERYONE";std::copy(replacement.begin(),replacement.end(),record.begin()+8);record[8+replacement.size()]=0; }return mode==2 || (mode!=1 && (name=="Alice" || name=="ALICE")); };
        hooks.translate=[&](auto key) { trace.push_back("T:"+RecipientHex(key));if (mode==6) throw RecipientStop{"translate"};return std::string(mode==3 && key=="EVERYONE" ? "Tous" : key); };
        std::string status="normal";bool result{};try { result=CheckChatRecipients(manager,row[1]=="~" ? std::nullopt : std::optional<std::string_view>(text),scratch,hooks); }catch (const RecipientStop& stopped) { status=stopped.where;++stops; }
        CHECK(status==row[3]);if (status=="normal") CHECK(std::to_string(result)==row[4]);if (row[5]!="-") CHECK(RecipientHex({reinterpret_cast<const char*>(scratch.data()),scratch.size()})==row[5]);
        std::string joined;
        for (const auto& event:trace) { if (!joined.empty()) joined+=';';joined+=event; } //외부장치의 실제 순서만 직렬화한다.
        CHECK((joined.empty() ? "-" : joined)==row[6]);++count;
    }CHECK(count==350 && stops>0);
}

// 자연 관문 밖에서는 미선택 장치를 요구하지 않고, 도달한 누락은 즉시 드러낸다.
TEST_CASE(ChatRecipients_LazyDevicesAndExactSeparators) {
    netstorm::o::PlayerIdentityRecord scratch;scratch.fill(0xa5);const auto before=scratch;CHECK(!CheckChatRecipients(0,"Alice",scratch,{}) && scratch==before);CHECK(!CheckChatRecipients(1,std::nullopt,scratch,{}) && scratch==before);
    CHECK(CheckChatRecipients(1," \r\n\t",scratch,{}));bool rejected{};try { static_cast<void>(CheckChatRecipients(1,"Alice",scratch,{})); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    ChatRecipientHooks hooks;hooks.resolve=[](auto&) { return true; };CHECK(CheckChatRecipients(1,"A\vB\fC",scratch,hooks));CHECK(std::string(reinterpret_cast<const char*>(scratch.data()+8))=="A\vB\fC");
}
