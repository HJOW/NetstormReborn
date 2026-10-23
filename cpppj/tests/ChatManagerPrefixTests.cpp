// 제품보다 먼저 고정한 실제 PE644관측과 전체 물리·부분 효과·현재값 재조회를 대조한다.
#include "TestSupport.h"
#include "o/RawChatManagerPrefix.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::array<std::uint32_t,2> kManagers{0x12001000,0x12004000}; // 격리 PE의 실제 두 manager 할당 토큰이다.
constexpr std::array<std::uint32_t,2> kBuffers{0x12008000,0x12009000}; // 내부0x80 할당은 manager와 별개로 보존한다.

// 마지막 빈 칸도 보존하여 잘못된 fixture를 조용히 받아들이지 않는다.
std::vector<std::string> SplitChat(std::string_view text,char separator) {
    std::vector<std::string> result; // 각 원시 입력 칸의 독립 문자열이다.
    std::size_t start=0; // 아직 소비하지 않은 첫 BYTE 위치다.
    for (;;) { // 마지막 항목까지 같은 규칙으로 분리한다.
        const auto end=text.find(separator,start); // 다음 구분자의 현재 위치다.
        result.emplace_back(text.substr(start,end==text.npos?text.size()-start:end-start));
        if (end==text.npos) return result;
        start=end+1;
    }
}
// NUL 뒤 미작성 BYTE까지 비교하는 고정 소문자 hex 표현이다.
std::string ChatHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; // 원문 TSV의 자릿수 표기다.
    std::string text; // 전체 물리를 빠짐없이 출력한다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; } // 문자열 길이로 잘라 비교하지 않는다.
    return text;
}
// 하위 callback의 명시 중단만 나타내며 물리/CPU fault와 구별한다.
struct ChatStop {};

// 부모 기대를 계산하지 않고 같은 현재 전역·실제 할당과 명시 하위 효과만 제공한다.
struct ChatDevice {
    std::array<std::array<std::uint8_t,kChatManagerBytes>,2> managers{}; // 모든 미작성 필드까지 관측하는 실제 manager 물리다.
    std::array<std::array<std::uint8_t,kChatManagerBufferBytes>,2> buffers{}; // 생성자가 초기화하지 않는 실제 내부 물리다.
    std::array<std::uint8_t,256> output{},temporary{},wrapperOutput{}; // 직접 formatter의 외부 출력과 wrapper의 실제 지역 출력은 다른 주소다.
    std::array<std::uint8_t,10> core{}; // 원문 wrapper가 unsigned BYTE+4만 읽는 명시 하위 입력이다.
    std::vector<std::uint8_t> source; // 일반 setter 입력의 NUL 포함 물리다.
    std::uint32_t current{},subscriber{},active{},allocations{},profile{}; // 원문이 직접 읽는 같은 현재 DWORD와 할당 횟수다.
    std::int32_t mode{},seed{},rank{},minor{},maximum{},repeat{},frontLength{},backLength{}; // 출력에서 역산하지 않은 원시 입력이다.
    std::string mutation; // 명시 callback 뒤 현재값 변경 또는 중단이다.
    std::map<std::string,std::uint32_t> calls; // 종류별 호출 번호이며 재초기화에도 유지한다.
    std::vector<std::string> trace; // 실제 호출 인자와 순서를 누적한다.

    // 모듈 계산에 앞서 PE와 같은 비0 초기 입력을 전체 물리에 쓴다.
    explicit ChatDevice(const std::vector<std::string>& values) {
        if (values.size()!=13) throw std::runtime_error("Chat fixture 입력 수");
        mode=std::stoi(values[0]);seed=std::stoi(values[1]);
        const auto selected=std::stoul(values[2]); // 0은 NULL이고1/2는 서로 다른 실제 할당이다.
        current=selected==0?0:kManagers.at(selected-1);active=static_cast<std::uint32_t>(std::stoul(values[3]));rank=std::stoi(values[4]);
        subscriber=static_cast<std::uint32_t>(std::stoul(values[5]));profile=static_cast<std::uint32_t>(std::stoul(values[6]));
        minor=std::stoi(values[7]);maximum=std::stoi(values[8]);repeat=std::stoi(values[9]);frontLength=std::stoi(values[10]);backLength=std::stoi(values[11]);mutation=values[12];
        for (std::size_t object=0;object<managers.size();++object) for (std::size_t byte=0;byte<managers[object].size();++byte) managers[object][byte]=static_cast<std::uint8_t>(seed+byte*17+object*59); // 미작성 전체 BYTE의 독립 입력이다.
        for (std::size_t buffer=0;buffer<buffers.size();++buffer) for (std::size_t byte=0;byte<buffers[buffer].size();++byte) buffers[buffer][byte]=static_cast<std::uint8_t>(seed+byte*17+buffer*71+13); // 내부 new가 이 물리를 덮지 않는지 확인한다.
        output.fill(static_cast<std::uint8_t>(seed));core[4]=static_cast<std::uint8_t>(rank);
        if (mode==2) {
            source.assign(static_cast<std::size_t>(rank),'A');source.push_back(0);
            if (mutation=="alias-forward" || mutation=="alias-backward") {
                const auto offset=mutation=="alias-forward"?0x243:0x245; // 같은 manager 안의 순방향 겹침을 실제 주소로 제공한다.
                constexpr std::array<std::uint8_t,5> text{'A','B','C','D',0}; // 원문에 먼저 쓴 NUL 포함 입력이다.
                std::copy(text.begin(),text.end(),managers[0].begin()+offset);
            }
        }
    }
    // 호출 경계 앞의 실제 인자를 기록하고 종류별 번호를 반환한다.
    std::string Event(const std::string& kind,std::string text) { trace.push_back(std::move(text));return kind+std::to_string(++calls[kind]); }
    // 부모 밖 하위 장치가 바꾸는 같은 현재 전역만 수정한다.
    void Mutate(const std::string& boundary) {
        const auto split=mutation.find('-'); // alias 입력은 실제 source로 처리하므로 이 관문을 통과하지 않는다.
        if (split==mutation.npos || mutation.substr(0,split)!=boundary) return;
        const auto action=mutation.substr(split+1); // 이 호출에 지정한 하위 효과다.
        if (action=="stop") throw ChatStop{};
        if (action=="manager0") current=0;
        else if (action=="manager1") current=kManagers[0];
        else if (action=="manager2") current=kManagers[1];
        else if (action=="subscriber9") subscriber=9;
        else if (action=="maximum0") maximum=0;
        else if (action=="maximumNegative") maximum=-1;
        else if (action=="minor3") minor=3;
        else throw std::runtime_error("Chat 미해석 변경");
    }
    // 토큰은 host 포인터로 축소하지 않고 실제 소유 배열의 같은 span을 빌린다.
    std::span<std::uint8_t> Memory(std::uint32_t token) {
        for (std::size_t index=0;index<kManagers.size();++index) if (token==kManagers[index]) return managers[index]; // 같은 할당 주소만 허용한다.
        for (std::size_t index=0;index<kBuffers.size();++index) if (token==kBuffers[index]) return buffers[index]; // 내부 할당은 다른 실제 배열이다.
        throw std::out_of_range("Chat 실제 NULL/미등록 할당");
    }
    // PE의 가상 주소를 fixture와 같은 식별자로 표현한다.
    static std::string Normalized(std::uint32_t address) {
        if (address==0) return "0";
        for (std::size_t index=0;index<kManagers.size();++index) if (address>=kManagers[index] && address-kManagers[index]<kChatManagerBytes) return "M"+std::to_string(index+1)+"+"+std::to_string(address-kManagers[index]); // offset도 실제 인자다.
        for (std::size_t index=0;index<kBuffers.size();++index) if (address==kBuffers[index]) return "B"+std::to_string(index+1); // 내부 할당 주소다.
        std::array<char,16> text{}; // NULL+offset 같은 범위 밖 인자도 호출 진입 trace에 남긴다.
        std::snprintf(text.data(),text.size(),"0x%x",address);return text.data();
    }
    // 실제 current 슬롯의 logical 번호와 살아 있는 전역·할당 시도를 출력한다.
    std::string State() const {
        const auto index=current==0?0:current==kManagers[0]?1:current==kManagers[1]?2:-1; // fixture 식별자는 포인터 사본 소유자가 아니다.
        return std::to_string(index)+","+std::to_string(subscriber)+","+std::to_string(allocations);
    }
    // trace를 생략 없이 같은 TSV 표현으로 작성한다.
    std::string Trace() const {
        std::string text; // 호출이 없을 때만 '-'로 표시한다.
        for (const auto& event:trace) { if (!text.empty()) text+=';';text+=event; } // 현재 순서대로 연결한다.
        return text.empty()?"-":text;
    }
    // 부모 분기를 구현하지 않은 명시 할당/설정/CRT/그림 개수 장치를 제공한다.
    ChatManagerPrefixHooks Hooks() {
        ChatManagerPrefixHooks hooks; // 선택한 원문 하위만 실제 효과를 제공한다.
        hooks.allocate=[this](std::uint32_t amount) {
            std::uint32_t result=0; // 실패 입력도 별도 반환하며 뒤의 진단/부분 쓰기를 부모에 맡긴다.
            if (amount==kChatManagerBytes) { ++allocations;result=(profile&1)?0:kManagers[(allocations-1)%2]; }
            else if (amount==kChatManagerBufferBytes) result=(profile&2)?0:kBuffers[(allocations==0?0:allocations-1)%2];
            else throw std::runtime_error("Chat 예상 밖 할당 크기");
            const auto boundary=Event("new","N:"+std::to_string(amount)+":"+Normalized(result));Mutate(boundary);return result;
        };
        hooks.memory=[this](auto token) { return Memory(token); };
        hooks.assertion=[this](std::uint32_t line) {
            const auto text=line==220?"address != NULL":line==898?"TheChatSystem != NULL":line==2474?"maxMinor > 0":nullptr; // PE가 넘긴 실제 진단 문자열이다.
            if (!text) throw std::runtime_error("Chat 예상 밖 진단");
            const auto boundary=Event("assert","A:"+std::to_string(line)+":"+text);Mutate(boundary);
        };
        hooks.registerCommand=[this](std::string_view name,std::uint32_t callback,std::uint32_t flags) {
            const auto boundary=Event("command","C:"+std::string(name)+":"+Normalized(callback)+":"+std::to_string(flags));Mutate(boundary);
        };
        hooks.readConfig=[this](std::string_view key)->std::optional<std::string> {
            const auto ordinal=calls["read"]%2; // 각 초기화의 front/back 지역 입력이며 반복에도2개씩 진행한다.
            const auto length=ordinal==0?frontLength:backLength; // 누락 ReadString은 지역 첫 NUL을 보존한다.
            const auto boundary=Event("read","R:"+std::string(key)+":"+std::to_string(length));
            std::optional<std::string> value; // 실제 지역 사본은 뒤의 다른 Config 호출에도 변하지 않는다.
            if (length>=0) value=std::string(static_cast<std::size_t>(length),ordinal==0?'F':'G');
            Mutate(boundary);return value;
        };
        hooks.copyFormat=[this](auto token,auto offset,auto text,auto count) {
            const auto boundary=Event("strncpy","P:"+Normalized(token+offset)+":"+std::to_string(count)); // NULL 인자도 CRT 진입 뒤 물리 오류로 남긴다.
            auto bytes=Memory(token);if (offset>bytes.size()) throw std::out_of_range("Chat CRT offset");CopyChatManagerFormat(bytes.subspan(offset),text,count);Mutate(boundary);
        };
        hooks.maximumRank=[this] { const auto value=maximum;const auto boundary=Event("max","X:"+std::to_string(value));Mutate(boundary);return value; };
        hooks.minorCount=[this] { const auto value=minor;const auto boundary=Event("minor","I:"+std::to_string(value));Mutate(boundary);return value; };
        hooks.appendIcon=[this](auto target,char kind,std::int32_t value) {
            AppendChatRankIcon(target,kind,value);const auto boundary=Event("append",std::string("F:~[Iicon.")+kind+"%d]:"+std::to_string(value));Mutate(boundary);
        };
        hooks.core=[this]()->std::span<const std::uint8_t> { const auto boundary=Event("core","K");Mutate(boundary);return core; };
        return hooks;
    }
    // 공개 본문만 선택하며 중단 이후 재호출이나 상태 복구를 하지 않는다.
    std::string Run() {
        const auto hooks=Hooks(); // 같은 장치/현재 슬롯은 재초기화 사이에도 유지한다.
        try {
            for (std::int32_t invocation=0;invocation<repeat;++invocation) { // 원문과 같이 새 지역 미정 입력만 매 호출 공급한다.
                temporary.fill(static_cast<std::uint8_t>(seed));
                const ChatManagerPrefixGlobals globals{current,subscriber,active}; // 매번 같은 실제 슬롯을 빌린다.
                if (mode==0) CHECK(ConstructChatManager(kManagers[0],hooks)==kManagers[0]);
                else if (mode==1) InitializeChatManager(globals,hooks);
                else if (mode==2) {
                    std::span<const std::uint8_t> text=source; // 일반 입력은 별도 실제 source다.
                    if (mutation=="alias-forward" || mutation=="alias-backward") text=std::span(managers[0]).subspan(mutation=="alias-forward"?0x243:0x245);
                    SetChatManagerRankPrefix(managers[0],text);
                }
                else if (mode==3) { const auto text=ChatPrefixIcons(output,temporary,rank,hooks);CHECK(text.data()==output.data());CHECK(text.size()==256); }
                else if (mode==4) RefreshChatManagerRankPrefix(globals,hooks,wrapperOutput,temporary); // 원문 wrapper는 자기 stack에 출력하므로 fixture의 외부 OUTPUT은 그대로다.
                else throw std::runtime_error("Chat fixture 진입점");
            }
            return "normal";
        } catch (const ChatStop&) { return "stop"; }
        catch (const std::out_of_range&) { return "fault"; }
        catch (const std::domain_error&) { return "fault"; }
    }
};

// 한 원시 칸의 차이를 입력 번호와 함께 남겨 최초 실패를 숨기지 않는다.
void CheckChatColumn(std::size_t row,std::string_view name,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"Chat row=%zu column=%.*s actual=%s expected=%s\n",row,static_cast<int>(name.size()),name.data(),actual.c_str(),expected.c_str());
    CHECK(actual==expected);
}
}

// 양 x87 모드644행의 정상/CPU fault/명시 중단과 모든 실제 할당의 미작성 필드까지 대조한다.
TEST_CASE(ChatManagerPrefix_ReplaysIndependentPe644) {
#ifdef NETSTORM_CHAT_MANAGER_PREFIX_FIXTURE
    const std::filesystem::path path=NETSTORM_CHAT_MANAGER_PREFIX_FIXTURE; // 공용 CMake가 명시하는 고정 원문 자료다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/chat-manager-prefix-x86.tsv"; // 격리 검사도 같은 원문 파일을 읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("Chat 원문 fixture 열기 실패");
    std::size_t rows=0; // 원문 보고서의 관측 수와 별도로 센다.
    std::map<std::string,std::size_t> statuses; // 실패 관측도 정상 통과로 합치지 않는다.
    std::string line; // 원시 TSV 한 행이다.
    while (std::getline(input,line)) { // 고정 원문 전체를 빠짐없이 실행한다.
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto values=SplitChat(line,'\t');if (values.size()!=11) throw std::runtime_error("Chat 원문 TSV 칸 수");
        CHECK(values[0]=="639"||values[0]=="895");CHECK(std::stoul(values[10])>0);
        ChatDevice device(SplitChat(values[1],','));const auto status=device.Run();++rows;++statuses[status]; // 기대를 원문 출력에서 재계산하지 않는다.
        CheckChatColumn(rows,"status",status,values[2]);CheckChatColumn(rows,"state",device.State(),values[3]);CheckChatColumn(rows,"output256",ChatHex(device.output),values[4]);
        CheckChatColumn(rows,"managerA2712",ChatHex(device.managers[0]),values[5]);CheckChatColumn(rows,"managerB2712",ChatHex(device.managers[1]),values[6]);
        CheckChatColumn(rows,"bufferA128",ChatHex(device.buffers[0]),values[7]);CheckChatColumn(rows,"bufferB128",ChatHex(device.buffers[1]),values[8]);CheckChatColumn(rows,"trace",device.Trace(),values[9]);
    }
    CHECK(rows==644);CHECK(statuses["normal"]==524);CHECK(statuses["fault"]==94);CHECK(statuses["stop"]==26);
}

// 현재 실제 manager/active가0인 자연 분기는 Core/formatter/버퍼 장치가 없어도 복귀한다.
TEST_CASE(ChatManagerPrefix_NaturalGuardsNeedNoUnselectedDevices) {
    std::uint32_t current=0,subscriber=7,active=1; // 최초 manager 미생성과 활성 관문을 독립 구별한다.
    RefreshChatManagerRankPrefix({current,subscriber,active},{},{},{});CHECK(current==0);CHECK(subscriber==7);CHECK(active==1);
    current=0x1234;active=0;
    RefreshChatManagerRankPrefix({current,subscriber,active},{},{},{});CHECK(current==0x1234);CHECK(subscriber==7);CHECK(active==0);
}

// 실사용 순수 CRT 연결은 같은 실제 span에 쓰며 기존 사용자 경계는 덮어쓰지 않는다.
TEST_CASE(ChatManagerPrefix_MemoryCopyAdapterBorrowsActualBytes) {
    std::array<std::uint8_t,80> bytes;bytes.fill(0xa5); // NUL padding 밖 물리 보존도 확인한다.
    std::uint32_t memoryReads=0,customCalls=0; // 관측용 횟수이며 부모 알고리즘을 흉내 내지 않는다.
    ChatManagerPrefixHooks hooks; // 메모리 장치만으로 실제 작은 CRT 어댑터를 구성한다.
    hooks.memory=[&](auto token)->std::span<std::uint8_t> { ++memoryReads;if (token!=17) throw std::out_of_range("검사 token");return bytes; };
    auto bound=BindChatManagerMemoryCopy(hooks); // bytes 사본이 아니라 같은 물리를 빌린다.
    bound.copyFormat(17,20,"AB",60);CHECK(memoryReads==1);CHECK(bytes[19]==0xa5);CHECK(bytes[20]=='A');CHECK(bytes[21]=='B');
    CHECK(std::all_of(bytes.begin()+22,bytes.end(),[](auto byte){return byte==0;}));
    hooks.copyFormat=[&](auto,auto,auto,auto) { ++customCalls; }; // 사용자 callback은 현재값 재조회/계측을 유지한다.
    bound=BindChatManagerMemoryCopy(hooks);bound.copyFormat(0,0,"unused",1);CHECK(customCalls==1);CHECK(memoryReads==1);
    hooks.copyFormat={};bound=BindChatManagerMemoryCopy(hooks);
    bool rejected=false; // 선택된 잘못된 실제 토큰만 도달 위치에서 거부한다.
    try { bound.copyFormat(0,20,"AB",60); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);CHECK(memoryReads==2);
}
