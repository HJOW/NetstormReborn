// 최초 Notify1596관측을 제안 patch가 적용된 실제 RawPlayerTemplateState와 비교한다.
#include "TestSupport.h"
#include "o/RawPlayerTemplateState.h"
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
constexpr std::uint32_t kNotifyManager11=0x12000100U,kNotifyManager22=0x12000200U; // 최초 격리 입력의 실제 manager 포인터 비트다.
// 마지막 빈 칸도 유지하는 fixture 필드 분리다.
std::vector<std::string> NotifyFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { // 원래 구분자의 순서를 보존한다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1;
    }
}
// 원본 네 BYTE만 little endian DWORD로 읽는다.
std::uint32_t NotifyGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 주변 기록은 읽지 않는다.
}
// 지정 DWORD만 쓰고 이웃 필드는 보존한다.
void NotifyPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원본 네 BYTE 폭을 유지한다.
}
// 전체 물리 바이트를 원본 관측의 소문자 hex로 표현한다.
std::string NotifyHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 한 BYTE의 두 글자 표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; // 미사용 꼬리까지 비교한다.
}
// 명시 manager ID를 실제 raw 주소로 변환한다.
std::uint32_t NotifyPointer(std::uint32_t manager) { if (!manager) return 0;if (manager==11) return kNotifyManager11;if (manager==22) return kNotifyManager22;throw std::out_of_range("Notify manager ID 범위"); }
// raw manager 주소를 명시 하위 장치 ID로 변환한다.
std::uint32_t NotifyManager(std::uint32_t pointer) { if (!pointer) return 0;if (pointer==kNotifyManager11) return 11;if (pointer==kNotifyManager22) return 22;throw std::out_of_range("Notify manager 포인터 범위"); }
// 원문 callback의 지정 emu_stop에 해당하는 중단 표식이다.
struct NotifyStop {};
// 최초 입력의 실제 header/이름과 진단·Save 응답만 제공한다.
struct NotifyDevices {
    std::uint32_t manager{},name{},mine{},transient{},prevent{},missionInput{}; // 최초 입력의 각 DWORD다.
    bool mission{}; // 기존 클래스가 직접 빌리는 같은 현재 inMission 저장소다.
    std::uint32_t depth{}; // Notify는 읽거나 쓰지 않는 기존 장기 로딩 깊이다.
    std::array<std::uint8_t,180> headers{}; // 다른8칸도 보존을 확인하는 전체 Template 표다.
    std::array<std::array<std::uint8_t,64>,2> managers{}; // 첫BYTE 변경 외 모든 manager 바이트도 비교한다.
    std::array<std::string,2> names; // 같은 두 manager의 실제 C문자열 저장소다.
    std::array<PlayerTemplateStateStream,9> states{}; // 이 Notify 검사에서 선택되지 않는 기존 State API 입력이다.
    std::string mutation,trace; // 명시 callback 변경과 실제 발생한 순서다.
    // 최초 fixture 입력열만 읽고 관측 기대값은 사용하지 않는다.
    explicit NotifyDevices(std::string_view input) {
        const auto fields=NotifyFields(input,',');if (fields.size()!=7) throw std::runtime_error("Notify 입력 칸 수 오류");manager=std::stoul(fields[0]);name=std::stoul(fields[1]);mine=std::stoul(fields[2]);transient=std::stoul(fields[3]);prevent=std::stoul(fields[4]);missionInput=std::stoul(fields[5]);mission=missionInput!=0;mutation=fields[6];
        for (std::size_t index=0;index<headers.size();++index) headers[index]=static_cast<std::uint8_t>(index*13+7); // 전체 표의 미정 바이트 입력이다.
        NotifyPut(headers,20,NotifyPointer(manager));NotifyPut(headers,24,0x12345678U);NotifyPut(headers,28,mine);NotifyPut(headers,32,transient);NotifyPut(headers,36,prevent);
        const std::array<std::string,4> first{"", ".map", "map.fort", std::string("\0hidden",7)};names={first.at(name),"B.fort"};
        for (std::size_t index=0;index<2;++index) { // 실제 manager 이름/NUL 뒤 공간도 처음 패턴을 유지한다.
            managers[index].fill(0xa5);
            for (std::size_t byte=0;byte<=names[index].size();++byte) managers[index][12+byte]=byte==names[index].size() ? 0 : static_cast<std::uint8_t>(names[index][byte]); // 첫NUL이 있는 원문도 뒤 입력을 보존한다.
        }
    }
    // 한 시간순 trace에 파일명/진단/저장 진입을 기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 지정 callback의 현재 필드 변경만 수행하고 이후 분기/재조회는 제품에 맡긴다.
    void Mutate(std::string_view boundary) {
        for (const auto& part:NotifyFields(mutation,'+')) { // 여러 진단의 순차 변경도 독립 입력으로 처리한다.
            const auto split=part.find('-');if (split==part.npos || part.substr(0,split)!=boundary) continue;const auto action=part.substr(split+1);
            if (action.starts_with("manager")) NotifyPut(headers,20,NotifyPointer(std::stoul(action.substr(7))));
            else if (action.starts_with("mine")) NotifyPut(headers,28,std::stoul(action.substr(4)));
            else if (action.starts_with("transient")) NotifyPut(headers,32,std::stoul(action.substr(9)));
            else if (action.starts_with("prevent")) NotifyPut(headers,36,std::stoul(action.substr(7)));
            else if (action.starts_with("mission")) { missionInput=std::stoul(action.substr(7));mission=missionInput!=0; }
            else if (action=="nameEmpty") { managers[0][12]=0;if (!names[0].empty()) names[0][0]=0; }
            else if (action=="nameDot") { managers[0][12]='.';if (names[0].empty()) names[0]=".";else names[0][0]='.'; }
            else if (action=="stop") throw NotifyStop{};
        }
    }
    // 실제 현재 manager의 첫파일명BYTE 조회를 같은 manager별 문자열로 연결한다.
    const std::string& Name(std::uint32_t slot) {
        CHECK(slot==1);const auto current=NotifyManager(NotifyGet(headers,20));if (!current) throw std::runtime_error("Notify patch가 첫manager0을 조회 전에 거부하지 않았습니다");const auto index=current==11 ? 0U : 1U;Record("name:"+std::to_string(current)+':'+std::to_string(managers[index][12]));return names[index];
    }
    // 원문행별 callback을 실제 클래스 진단 enum에 연결한다.
    void Assert(PlayerTemplateStateAssertion reason) {
        std::uint32_t line{};std::string boundary;
        switch (reason) {
        case PlayerTemplateStateAssertion::DotFileName:line=449;boundary="dot";break;case PlayerTemplateStateAssertion::MineTransientFlag:line=451;boundary="transient";break;
        case PlayerTemplateStateAssertion::InMissionSave:line=454;boundary="mission";break;case PlayerTemplateStateAssertion::MissingNotifyTemplate:line=455;boundary="missing";break;
        default:throw std::logic_error("Notify 검사밖 State 진단");
        }
        Record("assert:"+std::to_string(line));Mutate(boundary);
    }
    // 제안 patch가 적용된 실제 클래스를 실행하고 첫 물리 경계와 지정 중단만 정규화한다.
    std::string Run() {
        RawPlayerTemplateState state(OriginalEdition::Patch1078,headers,[this](auto slot)->PlayerTemplateStateStream& { return states.at(slot); },[this](auto slot)->const std::string& { return Name(slot); },depth,mission,[this](auto reason) { Assert(reason); },[](auto) { throw std::logic_error("Notify는 로딩 깊이를 쓰지 않습니다"); });
        state.BindFileSave([this](auto slot) { CHECK(slot==1);Record("save:"+std::to_string(NotifyManager(NotifyGet(headers,20))));Mutate("save"); });
        try { state.Changed(1);return "return"; }catch (const NotifyStop&) { return "stop"; }catch (const std::out_of_range& error) {
            if (std::string_view(error.what())!="Player Notify manager0 파일명 첫BYTE 물리접근경계") throw;Record("fault:12:1");return "fault";
        }
    }
    // 전체 header/manager를 최초 관측과 같은 원래 순서로 표현한다.
    std::string Raw() const { return NotifyHex(headers)+NotifyHex(managers[0])+NotifyHex(managers[1]); }
};
}
// 최초798입력의두정밀도·정상/중단/물리경계와모든callback효과를대조한다.
TEST_CASE(FortNotify_IndependentNative1596) {
    std::ifstream input(NETSTORM_FORT_NOTIFY_FIXTURE);if (!input) throw std::runtime_error("Notify 최초 x86 fixture 없음");std::string line;std::size_t compared{},faults{},stops{};std::set<std::pair<std::string,std::string>> unique;
    while (std::getline(input,line)) { // 제품 실행 후에만 최초 관측열을 기대값으로 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=NotifyFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Notify fixture 칸 수 오류");CHECK(unique.emplace(row[0],row[1]).second);NotifyDevices devices(row[1]);const auto status=devices.Run();
        CHECK(status==row[2]);CHECK(std::to_string(devices.missionInput)==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);++compared;if (status=="fault") ++faults;if (status=="stop") ++stops;
    }
    CHECK(compared==1596 && faults==128 && stops==80);
}
// 빈정상CString은관문뒤저장장치누락실패를유지하고선택안된저장은요구하지않는다.
TEST_CASE(FortNotify_EmptyCStringKeepsSelectedSaveFailure) {
    std::array<std::uint8_t,180> headers{};NotifyPut(headers,20,11);std::array<PlayerTemplateStateStream,9> streams{};std::array<std::string,9> names{};std::uint32_t depth{};bool mission{};
    RawPlayerTemplateState state(OriginalEdition::Patch1078,headers,streams,names,depth,mission,[](auto) { throw std::logic_error("빈이름 자체는 진단 대상이 아닙니다"); },[](auto) { throw std::logic_error("Notify 로딩 깊이 변경 금지"); });
    bool missing{};try { state.Changed(1); }catch (const std::logic_error& error) { missing=std::string_view(error.what())=="Player Template실제DataManager파일저장44d8d0/CD44cc40미연결"; }CHECK(missing);
    NotifyPut(headers,28,1);state.Changed(1);NotifyPut(headers,28,0);NotifyPut(headers,36,1);state.Changed(1); // 실제미선택관문은저장장치없이자연반환한다.
}
