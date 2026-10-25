// 제품 전에 고정한 원본204관측의 전체 물리/현재 전역/부분 중단을 대조한다.
#include "TestSupport.h"
#include "o/RawStaticStrings.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientMain.h"
#include "client/CampaignClientServices.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // Native 주소는 명시 입력 물리의 식별자로만 사용한다.
constexpr std::uint32_t kStaticArena1=0x12010000U,kStaticArena2=0x12020000U,kStaticSource=0x12400000U; // 원본 관측의 독립 두8192BYTE와 외부서식 입력 주소다.
struct StaticStop {}; // 명시 하위 중단과 실제 물리 오류를 구별한다.
std::vector<std::string> StaticSplit(const std::string& text,char delimiter) { std::vector<std::string> result;std::istringstream stream(text);std::string field;while (std::getline(stream,field,delimiter)) result.push_back(field);return result; } // fixture의 필드 순서를 변경하지 않는다.
std::uint32_t StaticNumber(const std::string& text) { return static_cast<std::uint32_t>(std::stoull(text)); } // 원문 DWORD 입력만 읽는다.
std::vector<std::uint8_t> StaticHex(const std::string& text) { std::vector<std::uint8_t> result;for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16)));return result; } // 전체8192BYTE 결과를 해시 대신 직접 대조한다.
std::string StaticNormalized(std::uint32_t address) {
    if (!address) return "0";for (std::uint32_t index=0;index<2;++index) { const auto base=index?kStaticArena2:kStaticArena1;if (address>=base && address-base<8192) return std::to_string(index+1)+'+'+std::to_string(address-base); } // Native 기록과 같은 할당/offset 식별자다.
    std::ostringstream out;out<<"0x"<<std::hex<<address;return out.str();
}
std::string StaticText(std::span<const std::uint8_t> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("검사 임시 문자열 NUL 범위");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())}; } // 실제 작성된 NUL 앞부분만 읽는다.
}

TEST_CASE(StaticStrings_ReplaysIndependentPe204) {
    std::ifstream input(NETSTORM_STATIC_STRINGS_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{};
    while (std::getline(input,line)) { // 매 행의 명시 초기 입력만 제품에 공급한다.
        if (line.empty() || line[0]=='#') continue;const auto fields=StaticSplit(line,'\t'),parameters=StaticSplit(fields.at(0),',');CHECK(fields.size()==8 && parameters.size()==9);
        const auto mode=StaticNumber(parameters[0]),capacity=StaticNumber(parameters[1]),initial=StaticNumber(parameters[2]),cursor=StaticNumber(parameters[3]),length=StaticNumber(parameters[4]),seed=StaticNumber(parameters[5]),repeat=StaticNumber(parameters[6]),pattern=StaticNumber(parameters[7]);const auto change=parameters[8];
        std::array<std::array<std::uint8_t,8192>,2> arenas;for (std::uint32_t block=0;block<2;++block) for (std::uint32_t offset=0;offset<8192;++offset) arenas[block][offset]=static_cast<std::uint8_t>(seed+block*71+offset*13); // 미작성 입력 전체도 원문과 같다.
        std::uint32_t size=capacity,base=initial?kStaticArena1:0,current=initial?kStaticArena1+cursor:0,last=initial?kStaticArena1+7:0,result{};std::string status="normal";std::vector<std::string> trace;netstorm::o::StaticStringGlobals globals{size,base,current,last};netstorm::o::StaticStringHooks hooks;
        hooks.memory=[&](auto address,auto bytes)->std::span<std::uint8_t> {
            for (std::uint32_t block=0;block<2;++block) { const auto begin=block?kStaticArena2:kStaticArena1;if (address>=begin && address-begin<=8192 && bytes<=8192-(address-begin)) return std::span(arenas[block]).subspan(address-begin,bytes); } // 선택 접근의 전체 폭만 실제 범위 검사한다.
            throw std::out_of_range("검사 임시 고리 명시 물리 fault");
        };
        hooks.assertion=[&](auto number) { trace.push_back("A:"+std::to_string(number));if (change=="assert-stop") throw StaticStop{}; };
        hooks.allocate=[&](auto number) { trace.push_back("N:"+std::to_string(number));if (change=="new-stop") throw StaticStop{};if (change=="new-size4") size=4;return change=="new-null"?0U:kStaticArena2; };
        hooks.format=[&](auto destination,auto source) { trace.push_back("F:"+StaticNormalized(destination)+':'+StaticNormalized(source)+":7");if (change=="format-stop") throw StaticStop{};hooks.memory(destination,1)[0]='R';hooks.memory(destination+1,1)[0]=0;if (change=="format-cursor3") current=kStaticArena1+3; }; // 독립 관측과 같은 명시 CRT 장치다. 실제 printf 검증과 구별한다.
        try { for (std::uint32_t invocation=0;invocation<repeat;++invocation) { // 같은 현재 고리를 반복 호출 사이 유지한다.
            if (mode==0) netstorm::o::InitializeStaticStrings(capacity,globals,hooks);else if (mode==1) result=netstorm::o::ReserveStaticString(length,globals,hooks);else result=netstorm::o::FormatStaticString(length,pattern?kStaticArena1+pattern:kStaticSource,globals,hooks);
        } }catch (const StaticStop&) { status="stop"; }catch (const std::out_of_range&) { status="fault"; }
        CHECK(status==fields[1]);const auto expected=StaticSplit(fields[2],',');CHECK(size==StaticNumber(expected[0]) && base==StaticNumber(expected[1]) && current==StaticNumber(expected[2]) && last==StaticNumber(expected[3]));if (mode) CHECK(result==StaticNumber(fields[3]));
        CHECK(std::equal(arenas[0].begin(),arenas[0].end(),StaticHex(fields[4]).begin()) && std::equal(arenas[1].begin(),arenas[1].end(),StaticHex(fields[5]).begin()));std::string joined;for (const auto& value:trace) { if (!joined.empty()) joined+=';';joined+=value; } // 기록은 호출 순서를 그대로 유지한다.
        CHECK((joined.empty()?"-":joined)==fields[6]);++rows;
    }
    CHECK(rows==204);
}

// 실제 공통10240과 lazy8192를 구별하며 고리 예약30 뒤의 같은 물리 쓰기/순환 alias를 확인한다.
TEST_CASE(StaticStrings_ActualOwnerReservationAndAlias) {
    netstorm::client::ClientStaticStrings lazy;CHECK(lazy.Globals().base==0);auto first=lazy.Reserve(30);CHECK(lazy.Globals().size==8192 && first.size()==8191 && first[30]==0xd5);first[0]='Q';first[1]=0;
    lazy.Reserve(8160);const auto aliased=lazy.FormatInteger(30,first,7);CHECK(aliased.data()==first.data() && StaticText(aliased)=="Q"); // wrap은 원래 첫 주소를 반환하므로 CRT도 호출하지 않는다.
    netstorm::client::ClientStaticStrings started;started.Initialize(10240);const std::string text=std::string(40,'L')+"%03d";const auto result=started.FormatInteger(30,{reinterpret_cast<const std::uint8_t*>(text.c_str()),text.size()+1},7);CHECK(started.Globals().size==10240 && StaticText(result)==std::string(40,'L')+"007" && result[30]=='L'); // 30BYTE 절단을 만들지 않는다.
}

// 실제 서식 오류는 먼저 쓴 예약/표식을 남기고 미공급 인자를 가짜값으로 대체하지 않는다.
TEST_CASE(StaticStrings_CrtFailurePreservesReservation) {
    netstorm::client::ClientStaticStrings owner;const std::uint8_t invalid[]{'%','s',0};bool rejected{};try { owner.FormatInteger(30,invalid,7); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && owner.Globals().size==8192 && owner.Globals().current==owner.Globals().base+32 && owner.Globals().last==owner.Globals().base+1);
}

// 실제 Client 번역과 같은 고리의 서로 다른 결과 주소가 일곱 Computer 이름 복사 동안 유지된다.
TEST_CASE(StaticStrings_ActualClientEditorTranslationAndSharedLifetime) {
    netstorm::client::ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";netstorm::client::Client client(std::move(options));client.StaticStrings()->Initialize(10240);const auto shared=client.StaticStrings();auto hooks=netstorm::client::MakeEditorPlayerClientHooks(client);
    CHECK(StaticText(hooks.translate("Human Player"))=="Human Player" && StaticText(hooks.translate("Player"))=="Player");std::vector<std::span<const std::uint8_t>> outputs;
    for (std::int32_t value=1;value<=7;++value) outputs.push_back(hooks.format(30,hooks.translate("Computer #%d"),value)); // 각 번역 scratch는 다음 조회에서만 재사용하고 결과는 같은 별도 고리에 둔다.
    for (std::size_t index=0;index<outputs.size();++index) CHECK(StaticText(outputs[index])=="Computer #"+std::to_string(index+1)); // 뒤 서식이 앞의 아직 살아 있는 결과를 덮지 않는다.
    CHECK(client.StaticStrings()==shared && shared->Globals().size==10240 && shared->Globals().current==shared->Globals().base+1+7*31);
}
