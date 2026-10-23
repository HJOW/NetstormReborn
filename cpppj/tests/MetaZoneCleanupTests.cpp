// 실제 부모38관측과 필수 자식·고정 물리의 부분효과를 별도 장치로 검사한다.
#include "TestSupport.h"
#include "o/RawMetaZoneCleanup.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 원시 TSV·입력 칼럼을 빈 칸까지 같은 순서로 읽는다.
std::vector<std::string> MetaFields(std::string_view text,char delimiter) {
    std::vector<std::string> result; // 원문 기대를 재계산하지 않는 파서 결과다.
    std::size_t first{}; // 아직 읽지 않은 칼럼의 시작 위치다.
    for (;;) { // 마지막 칼럼까지 한 번씩 분리한다.
        const auto next=text.find(delimiter,first); // 다음 구분자의 위치다.
        result.emplace_back(text.substr(first,next==text.npos?text.npos:next-first));
        if (next==text.npos) return result;
        first=next+1;
    }
}
// 실제 표 전체와 뒤16BYTE canary를 NUL과 무관하게 비교한다.
std::string MetaHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; // 원시 BYTE의 고정 hex 문자다.
    std::string result; // 미작성 인접 BYTE도 포함한 출력이다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; } // 실제 낮은 주소 순서를 보존한다.
    return result;
}
// Native 필수 자식 장치의 명시 중단이며 물리 오류와 구별한다.
struct MetaStop {};
// 부모 계산을 대체하지 않고 초기 실제 표·선택 자식 효과만 제공한다.
struct MetaDevice {
    std::array<std::uint8_t,kMetaZoneCleanupBytes+16> records{}; // 전체 고정 표와 인접 canary다.
    std::uint32_t flag{},stop{},mutationAt{}; // 실제 flag와 명시 자식 입력이다.
    std::string mutation,calls,trace; // 자식 변경과 호출 순서를 Native 문자열과 그대로 비교한다.
    std::size_t count{}; // 선택한 자식 호출의 수다.

    // 최초 Native와 같은 초기 입력만 구성하며 원문 출력은 계산하지 않는다.
    explicit MetaDevice(const std::vector<std::string>& fields) {
        if (fields.size()!=4) throw std::logic_error("메타구역 Native 입력 칼럼 수");
        const auto seed=std::stoul(fields[0]); // 명시 비영 메모리 입력이다.
        flag=static_cast<std::uint32_t>(std::stoull(fields[1]));stop=static_cast<std::uint32_t>(std::stoul(fields[3]));
        const auto effect=MetaFields(fields[2],':'); // 한 자식 callback의 선택 변경이다.
        if (effect.size()!=2) throw std::logic_error("메타구역 명시 효과 칼럼 수");
        mutationAt=static_cast<std::uint32_t>(std::stoul(effect[0]));mutation=effect[1];
        for (std::size_t byte=0;byte<records.size();++byte) records[byte]=static_cast<std::uint8_t>(seed+byte*17); // 실제7344BYTE 밖도 원시 입력과 같다.
    }
    // 실제00479520 본문은 대체하지 않으며 관측에서 명시한 변경·중단 장치만 제공한다.
    void Child(std::uint32_t index) {
        if (!calls.empty()) calls+=',';
        calls+=std::to_string(index);
        if (!trace.empty()) trace+=';';
        trace+="C:"+std::to_string(index);++count;
        if (count==stop) throw MetaStop{};
        if (count!=mutationAt) return;
        trace+=";M:"+mutation;
        if (mutation=="flag1") flag=1;
        else if (mutation=="flagMax") flag=0xffffffffU;
        else if (mutation=="touch") {
            constexpr std::uint32_t value=0x89abcdefU; // Native가 같은record+4c에 쓰는 명시 입력이다.
            for (std::size_t byte=0;byte<4;++byte) records[index*kMetaZoneCleanupStride+76+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 실제 child 본문의 가상소멸 완료를 주장하지 않는다.
        } else throw std::logic_error("메타구역 미해석 명시 효과");
    }
};
}

// 제품 전에 고정한 부모 정상28/명시중단10의 표·flag·자식호출 순서를 모두 대조한다.
TEST_CASE(MetaZoneCleanup_ReplaysIndependentParent38) {
#ifdef NETSTORM_META_ZONE_CLEANUP_FIXTURE
    const std::filesystem::path path=NETSTORM_META_ZONE_CLEANUP_FIXTURE; // 공용 등록은 최초 TSV 절대 경로를 제공한다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/meta-zone-cleanup-x86.tsv"; // 격리 검사도 동일 최초 기대를 읽는다.
#endif
    std::ifstream file(path); // 최초 독립 CPU 관측 파일이다.
    if (!file) throw std::runtime_error("메타구역 최초 fixture 없음");
    std::string line; // 현재 원시 행이다.
    std::size_t rows{},normal{},stopped{}; // 실제 분류 누락을 검출한다.
    while (std::getline(file,line)) { // 두 정밀도 전체38관측을 재생한다.
        if (line.empty() || line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=MetaFields(line,'\t'); // 원시 입력·출력을 분리한다.
        if (fields.size()!=10) throw std::logic_error("메타구역 TSV 칼럼 수");
        MetaDevice device(MetaFields(fields[1],',')); // 부모 기대와 별도인 외부 장치다.
        std::string status="normal"; // 명시 자식 중단만 별도 분류한다.
        try { ClearMetaZoneRecords({device.flag,device.records},{[&](auto index) { device.Child(index); }}); }
        catch (const MetaStop&) { status="stop"; }
        CHECK(status==fields[2] && std::to_string(device.flag)==fields[3] && MetaHex(device.records)==fields[4]);
        CHECK((device.calls.empty()?"-":device.calls)==fields[5] && (device.trace.empty()?"-":device.trace)==fields[6]);
        CHECK(fields[7]==(status=="normal"?"1836":"0") && std::stoul(fields[9])>0); // Native store 수 감사이며 C++ 쓰기를 계측했다고 가장하지 않는다.
        ++rows;if (status=="normal") ++normal;else ++stopped;
    }
    CHECK(rows==38 && normal==28 && stopped==10);
}

// 최초 비영 관문만 자식을 생략하고 이후 미연결 자식은 전체0쓰기보다 먼저 중단한다.
TEST_CASE(MetaZoneCleanup_FirstBranchAndRequiredChildKeepPartialEffects) {
    std::array<std::uint8_t,kMetaZoneCleanupBytes+16> records;records.fill(0xa5); // 인접 canary를 포함한 실제 고정 표다.
    std::uint32_t flag=0xffffffffU; // bool로 축소하지 않는 최초 현재 DWORD다.
    ClearMetaZoneRecords({flag,records},{});
    CHECK(flag==0 && std::all_of(records.begin(),records.begin()+kMetaZoneCleanupBytes,[](auto byte) { return byte==0; }) && records.back()==0xa5);
    records[0]=0x7a;bool missing{}; // 두 번째 호출은 실제 marker 자식 없이는 계속할 수 없다.
    try { ClearMetaZoneRecords({flag,records},{}); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && flag==0 && records[0]==0x7a && records.back()==0xa5);
    bool stopped{}; // 자식의 부분 효과가 예외 뒤에도 남는지 확인한다.
    try { ClearMetaZoneRecords({flag,records},{[&](auto index) { CHECK(index==0);flag=1;records[1]=0x6b;throw MetaStop{}; }}); }
    catch (const MetaStop&) { stopped=true; }
    CHECK(stopped && flag==1 && records[0]==0x7a && records[1]==0x6b);
}

// 짧은 물리는 단일DWORD 경계에서만 거부하며 마지막 flag쓰기도 전체 표 완료 뒤에 수행한다.
TEST_CASE(MetaZoneCleanup_ShortPhysicalPreservesStorePrefixAndChildTiming) {
    constexpr std::array<std::size_t,15> lengths{0,1,3,4,5,7,8,142,143,144,7339,7340,7343,7344,7348}; // 첫/중간/마지막 DWORD와 인접 물리의 경계 입력이다.
    for (const auto length:lengths) { // 전체 크기를 선행 검사하거나 부분 BYTE를 쓰는 오류를 구별한다.
        std::array<std::uint8_t,kMetaZoneCleanupBytes+16> records;records.fill(0xa5); // 호출보다 오래 사는 실제 물리다.
        std::uint32_t flag=1;bool rejected{}; // 실패에서는 최초 플래그가 유지돼야 한다.
        try { ClearMetaZoneRecords({flag,std::span(records).first(length)},{}); }catch (const std::out_of_range&) { rejected=true; }
        const auto written=std::min(length/4,kMetaZoneCleanupBytes/4)*4; // 성공 가능한 원문 DWORD의 범위다.
        CHECK(rejected==(length<kMetaZoneCleanupBytes) && flag==(rejected?1U:0U));
        for (std::size_t byte=0;byte<records.size();++byte) CHECK(records[byte]==(byte<written?0:0xa5)); // 완료된 prefix와 뒤 canary를 한 번에 확인한다.
    }
    std::array<std::uint8_t,kMetaZoneCleanupBytes> records;records.fill(0xa5); // 자식 앞 선행 물리 검사를 구별하는 추가 입력이다.
    std::uint32_t flag{},calls{};bool rejected{}; // 진입0 분기의 callback 변경은 실패 뒤도 남는다.
    try { ClearMetaZoneRecords({flag,std::span(records).first(8)},{[&](auto index) { CHECK(index==calls);++calls;if (index==50) flag=0x12345678U; }}); }
    catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && calls==51 && flag==0x12345678U && records[0]==0 && records[7]==0 && records[8]==0xa5);
}
