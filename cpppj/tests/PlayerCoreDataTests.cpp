// 제품 작성 전에 고정한 PE804관측의 실제 포인터·캐시·부분 쓰기와 가변 alias를 검증한다.
#include "TestSupport.h"
#include "o/RawPlayerCoreData.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 원시 TSV의 마지막 빈 칸도 버리지 않는 단순 분리다.
std::vector<std::string> CoreSplit(std::string_view text,char separator) {
    std::vector<std::string> parts;std::size_t begin=0; // 원문 입력 칸과 아직 읽지 않은 위치다.
    for (;;) { // 원문 칸을 순서대로 모두 보존한다.
        const auto end=text.find(separator,begin);parts.emplace_back(text.substr(begin,end==text.npos?text.size()-begin:end-begin)); // 현재 한 칸이다.
        if (end==text.npos) return parts;
        begin=end+1;
    }
}
// NUL 뒤 바이트를 포함한 전체 물리 비교용 소문자 hex다.
std::string CoreHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string text; // 고정 자릿수와 출력 문자열이다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; } // C문자열로 자르지 않는다.
    return text;
}
// 명시 assertion 중단을 물리 fault와 분리한다.
struct CoreStop {};
// 실제 원시 입력과 동일 배열/슬롯만 제공하며 부모 제어 흐름의 기대를 계산하지 않는다.
struct CoreDevice {
    std::array<std::uint8_t,1620> playerBytes{}; // 실제9×180BYTE이며 선택한+6c도 같은 물리다.
    std::array<std::uint8_t,10> cached{}; //005953f0의 실제10BYTE다.
    std::array<std::uint8_t,16> external{}; // 짧은 실제 source도 별도 물리에 둔다.
    std::span<std::uint8_t> players{playerBytes},current; // 현재 base 슬롯과 선택 포인터는 사본 버퍼가 아니다.
    std::uint32_t templateOwner{66},battleOwner{55},known{},active{},metamap{},ownerArg{},templateArg{}; // 각 원문 DWORD의 독립 현재값이다.
    int mode{};std::string mutation; // 진입점과 유일한 명시 하위 변화다.
    std::vector<std::string> trace; // assertion의 실제 인자/순서다.

    // 현재 포인터는 실제 Player/캐시/외부 입력의 같은 주소를 빌린다.
    explicit CoreDevice(const std::vector<std::string>& values) {
        if (values.size()!=10) throw std::runtime_error("CoreData 원문 입력 수");
        mode=std::stoi(values[0]);const auto seed=std::stoi(values[1]);const auto kind=std::stoi(values[2]);const auto length=static_cast<std::size_t>(std::stoul(values[3])); // 원문 출력과 무관한 초기 입력이다.
        metamap=static_cast<std::uint32_t>(std::stoul(values[4]));known=static_cast<std::uint32_t>(std::stoul(values[5]));active=static_cast<std::uint32_t>(std::stoul(values[6]));
        ownerArg=static_cast<std::uint32_t>(std::stoul(values[7]));templateArg=static_cast<std::uint32_t>(std::stoul(values[8]));mutation=values[9];
        for (std::size_t byte=0;byte<playerBytes.size();++byte) playerBytes[byte]=static_cast<std::uint8_t>(seed+byte*17); // 전체 Player 미작성 BYTE도 비교한다.
        for (std::size_t byte=0;byte<cached.size();++byte) cached[byte]=static_cast<std::uint8_t>(seed+byte*11+23); // 부분 캐시 쓰기를 구별한다.
        for (std::size_t byte=0;byte<external.size();++byte) external[byte]=static_cast<std::uint8_t>(seed+byte*7+49); // 외부 source의 실제 입력이다.
        if (kind==1) current=players.subspan(0xb4+0x6c,10);
        else if (kind==2) current=players.subspan(8*0xb4+0x6c,10);
        else if (kind==3) current=cached;
        else if (kind==4) current=std::span(external).first(length);
        else if (kind!=0) throw std::runtime_error("CoreData 원문 포인터 종류");
    }
    // 같은 참조와 실제 물리뷰만 신규 두 몸체에 공급한다.
    PlayerCoreDataGlobals Globals() { return {templateOwner,battleOwner,known,active,metamap,current,cached,players}; }
    // pointer가 가리키는 같은 배열 주소로만 원문 가상주소와 대응한다.
    std::string Pointer(std::span<std::uint8_t> value) const {
        if (value.data()==nullptr) return "0";
        if (value.data()==cached.data()) return "CACHED";
        if (value.data()==external.data()) return "EXTERNAL";
        for (std::size_t owner=0;owner<9;++owner) if (value.data()==playerBytes.data()+owner*0xb4+0x6c) return "PLAYER"+std::to_string(owner); // 실제 주소를 비교하며 내용으로 owner를 추정하지 않는다.
        throw std::runtime_error("CoreData 미등록 포인터");
    }
    // 두 owner/known/active/metamap의 현재 DWORD를 원문과 같은 순서로 출력한다.
    std::string State() const { return std::to_string(templateOwner)+","+std::to_string(battleOwner)+","+std::to_string(known)+","+std::to_string(active)+","+std::to_string(metamap); }
    // 유일한 실제 하위 진단에서 현재값 변화/중단만 수행한다.
    PlayerCoreDataHooks Hooks() {
        return {[this](std::uint32_t line) {
            trace.push_back("A:"+std::to_string(line)+":_myCoreData == &lastKnownCoreData");
            if (mutation=="current0") current={};
            else if (mutation=="currentPlayer8") current=players.subspan(8*0xb4+0x6c,10);
            else if (mutation=="currentCache") current=cached;
            else if (mutation=="metamap0") metamap=0;
            else if (mutation=="active0") active=0;
            else if (mutation=="stop") throw CoreStop{};
            else if (mutation!="none") throw std::runtime_error("CoreData 원문 변경 종류");
        }};
    }
    // 정상 반환/실제 물리 읽기 실패/명시 중단을 서로 다른 원시 결과로 유지한다.
    std::pair<std::string,std::string> Run() {
        try {
            if (mode==0) { const auto result=ReadPlayerCoreData(Globals(),Hooks());return {"normal",Pointer(result)}; }
            if (mode!=1) throw std::runtime_error("CoreData 원문 진입점");
            SelectPlayerCoreData(Globals(),ownerArg,templateArg);return {"normal","-"};
        } catch (const CoreStop&) { return {"stop","-"}; }
        catch (const std::out_of_range&) { return {"fault","-"}; }
    }
};
// 최초 차이를 입력 번호와 칸으로 남겨 다른 원시 실패를 가리지 않는다.
void CheckCore(std::size_t row,std::string_view column,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"CoreData row=%zu column=%.*s actual=%s expected=%s\n",row,static_cast<int>(column.size()),column.data(),actual.c_str(),expected.c_str());
    CHECK(actual==expected);
}
}

// 유효 비영 Player 선택과 NULL/짧은 현재 source·진단 변경/중단의 PE804관측을 전부 대조한다.
TEST_CASE(PlayerCoreData_ReplaysIndependentPe804) {
#ifdef NETSTORM_PLAYER_CORE_DATA_FIXTURE
    const std::filesystem::path path=NETSTORM_PLAYER_CORE_DATA_FIXTURE; // 공용 등록에서 지정할 고정 원문 경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/player-core-data-x86.tsv"; // 격리 빌드도 같은 최초 자료를 읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("CoreData fixture 열기 실패");
    std::size_t rows=0;std::map<std::string,std::size_t> statuses;std::string line; // 전체 원시 수와 상태별 수를 별도 계산한다.
    while (std::getline(input,line)) { // 양 정밀도의804행을 생략하지 않는다.
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto values=CoreSplit(line,'\t');if (values.size()!=11) throw std::runtime_error("CoreData fixture 칸 수");
        CHECK(values[0]=="639"||values[0]=="895");CHECK(std::stoul(values[10])>0);
        CoreDevice device(CoreSplit(values[1],','));const auto [status,result]=device.Run();++rows;++statuses[status]; // 기대는 실제 원문 칸에서만 읽는다.
        CheckCore(rows,"status",status,values[2]);CheckCore(rows,"return",result,values[3]);CheckCore(rows,"state",device.State(),values[4]);CheckCore(rows,"current",device.Pointer(device.current),values[5]);
        CheckCore(rows,"cached10",CoreHex(device.cached),values[6]);CheckCore(rows,"players1620",CoreHex(device.playerBytes),values[7]);CheckCore(rows,"external16",CoreHex(device.external),values[8]);
        std::string trace;for (const auto& event:device.trace) { if (!trace.empty()) trace+=';';trace+=event; } // 실제 호출 순서를 그대로 연결한다.
        CheckCore(rows,"trace",trace.empty()?"-":trace,values[9]);
    }
    CHECK(rows==804);CHECK(statuses["normal"]==730);CHECK(statuses["fault"]==72);CHECK(statuses["stop"]==2);
}

// 반환뷰의 쓰기가 실제 Player에 반영되고 정리가 같은10BYTE를 실제 캐시에 보존하는지 확인한다.
TEST_CASE(PlayerCoreData_ReturnedViewAliasesPlayerAndCachedStorage) {
    CoreDevice device(CoreSplit("1,165,0,10,0,7,9,8,3,none",',')); // 별도 Template owner3과 실제 Player8을 선택한다.
    SelectPlayerCoreData(device.Globals(),8,3);auto current=ReadPlayerCoreData(device.Globals(),{}); // active는 비영 선택에서 임의1로 바꾸지 않는다.
    CHECK(current.data()==device.playerBytes.data()+8*0xb4+0x6c);CHECK(current.size()==10);CHECK(device.templateOwner==3);CHECK(device.active==9);
    current[4]=201;CHECK(device.playerBytes[8*0xb4+0x70]==201); // 동일 주소를 빌리므로 별도 동기화가 없다.
    const auto known=device.known;SelectPlayerCoreData(device.Globals(),0,1);CHECK(device.current.empty());CHECK(device.active==0);CHECK(device.known==known);CHECK(device.cached[4]==201);
    device.metamap=1;SelectPlayerCoreData(device.Globals(),1,7);current=ReadPlayerCoreData(device.Globals(),{});CHECK(current.data()==device.cached.data());
    current[8]=177;CHECK(device.cached[8]==177);CHECK(device.playerBytes[8*0xb4+0x74]!=177); // 실제 cache alias이며 이전 Player를 덮지 않는다.
}

// 범위 밖 가상 포인터를 host 주소로 꾸미지 않는 API 물리 관문이며 Native 유효 선택 대조와 구별한다.
TEST_CASE(PlayerCoreData_RejectsUnrepresentablePlayerViewAfterEarlierStores) {
    CoreDevice device(CoreSplit("1,165,3,10,0,7,9,8,3,none",',')); // 현재 cache 주소는 물리 오류 뒤에도 그대로다.
    device.players=std::span(device.playerBytes).first(100); // 유효9×180BYTE 계약을 일부러 만족하지 않는 장치다.
    bool rejected=false; // PE의 잘못된 owner 포인터 반환 성공을 검증한 사례로 표현하지 않는다.
    try { SelectPlayerCoreData(device.Globals(),8,3); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);CHECK(device.templateOwner==3);CHECK(device.battleOwner==8);CHECK(device.known==1);CHECK(device.current.data()==device.cached.data());CHECK(device.active==9);
}
