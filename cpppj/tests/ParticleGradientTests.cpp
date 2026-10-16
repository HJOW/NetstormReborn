// 실제10.78의 색 띠 의미 필드와1/2/3색 띠 병합을 전체 원본 비트로 대조한다.
#include "TestSupport.h"
#include "o/ParticleGradient.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace netstorm::o;
namespace {
// 실제 원본 입력의 고유 팔레트/두 색 띠 조합 수다. 같은 입력의037f 관찰은 중복 가산하지 않는다.
constexpr std::size_t kNativeGradientCases=24;
// 마지막 빈 열도 보존하면서 fixture의 구분자 필드를 읽는다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t first=0;
    // 구분자가 없으면 나머지 필드를 넣고 종료한다.
    for (;;) {
        const auto end=text.find(delimiter,first);result.emplace_back(text.substr(first,end==std::string_view::npos ? end : end-first));
        if (end==std::string_view::npos) return result;first=end+1;
    }
}
// 순서를 바꾸지 않고 원본 little endian 메모리의 hex 바이트를 복구한다.
std::vector<std::uint8_t> Hex(std::string_view text) {
    if (text.size()%2) throw std::runtime_error("입자 색 hex 입력 길이 오류");
    std::vector<std::uint8_t> result;result.reserve(text.size()/2);
    // 두 hex 문자마다 원본 바이트 하나다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// 비정렬4/8바이트 값을 호스트 구조체로 덮어씌우지 않고 읽는다.
std::uint64_t Number(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset+width>bytes.size() || width>8) throw std::runtime_error("입자 색 비트 읽기 범위 오류");
    std::uint64_t result=0;
    // 각 바이트의 원래 위치를 유지한다.
    for (std::size_t index=0;index<width;++index) result|=static_cast<std::uint64_t>(bytes[offset+index])<<(index*8);
    return result;
}
// 실제 COL에서 공급한256개 RGBX 입력을 원래 색 번호 순서로 복구한다.
std::array<std::uint32_t,256> Palette(std::string_view text) {
    const auto raw=Hex(text);if (raw.size()!=1024) throw std::runtime_error("입자 색 입력 팔레트 크기 오류");
    std::array<std::uint32_t,256> result{};
    // 네 바이트마다 색 하나이며 예약 BYTE도 입력에 보존한다.
    for (std::size_t index=0;index<result.size();++index) result[index]=static_cast<std::uint32_t>(Number(raw,index*4,4));
    return result;
}
// 오직 fixture 입력의 RGB/요청 수/폭을 제품 함수에 공급한다. 기대값 계산에는 사용하지 않는다.
ParticleGradient Make(std::span<const std::uint32_t,256> palette,const std::vector<std::string>& row,std::size_t column) {
    const auto rgb=Split(row[column],',');
    if (rgb.size()!=6) throw std::runtime_error("입자 색 RGB 입력 열 수 오류");
    std::array<double,6> values{};
    // 실제 binary64 입력을 반올림 없이 복원한다.
    for (std::size_t index=0;index<values.size();++index) values[index]=std::bit_cast<double>(std::stoull(rgb[index]));
    return BuildParticleGradient(palette,{values[0],values[1],values[2]},{values[3],values[4],values[5]},
        std::stoi(row[column+1]),std::bit_cast<double>(std::stoull(row[column+2])));
}
// 첫 차이를 입력 이름/색 띠/항목/필드와 함께 반환한다. 원본 출력에서 기대 키를 다시 계산하지 않는다.
std::string Difference(const ParticleGradient& actual,std::string_view header,std::string_view payload,const std::string& context) {
    const auto expected=Split(header,',');const auto bytes=Hex(payload);
    if (expected.size()!=4 || bytes.size()!=256*48) throw std::runtime_error("입자 색 기대값 크기 오류");
    if (actual.from!=std::stoi(expected[0]) || actual.to!=std::stoi(expected[1]) || actual.selected!=std::stoi(expected[2]) ||
        std::bit_cast<std::uint64_t>(actual.threshold)!=std::stoull(expected[3])) return context+" 머리 값 불일치";
    // 의미48바이트의 실제 필드 위치/폭이며 x86 HSV/패딩은 비교 범위가 아니다.
    constexpr std::array<std::size_t,7> kOffsets{0,8,16,24,28,36,44},kWidths{8,8,8,4,8,8,4};
    // 전체256항목의 RGB 차/번호/거리/축 위치/선택 표식을 빠짐없이 비교한다.
    for (std::size_t index=0;index<actual.entries.size();++index) {
        const auto& entry=actual.entries[index];
        const std::array<std::uint64_t,7> values{std::bit_cast<std::uint64_t>(entry.translated[0]),std::bit_cast<std::uint64_t>(entry.translated[1]),
            std::bit_cast<std::uint64_t>(entry.translated[2]),entry.index,std::bit_cast<std::uint64_t>(entry.distance),
            std::bit_cast<std::uint64_t>(entry.position),entry.selected};
        // 부동소수점 허용오차를 쓰지 않고 DWORD/QWORD 전체를 확인한다.
        for (std::size_t field=0;field<values.size();++field)
            if (values[field]!=Number(bytes,index*48+kOffsets[field],kWidths[field]))
                return context+" 항목"+std::to_string(index)+" 필드"+std::to_string(field)+" 불일치";
    }
    return {};
}
// 원본 병합 결과의 DWORD 번호 전체와 요청 길이를 확인한다.
std::string MergeDifference(std::span<const ParticleGradient> gradients,std::string_view payload,const std::string& context) {
    const auto expected=Hex(payload);const auto actual=MergeParticleGradients(gradients);
    if (actual.size()*4!=expected.size()) return context+" 병합 길이 불일치";
    // 선택 수가 요청 수보다 작을 때의 뒤쪽 항목도 비교한다.
    for (std::size_t index=0;index<actual.size();++index)
        if (actual[index]!=Number(expected,index*4,4)) return context+" 병합 색"+std::to_string(index)+" 불일치";
    return {};
}
// 한 입력의 차이는 한 번 보고하고 다른 팔레트 입력도 계속 확인한다.
void Report(std::string difference) {
    if (!difference.empty()) netstorm::test::ReportFailure(__FILE__,__LINE__,difference.c_str());
}
}
// 기본027f의24입력/48색 띠와72병합을 원본 실제 결과와 비교한다. 037f 원문도 fixture에 보존한다.
TEST_CASE(ParticleGradient_Native027f) {
    std::ifstream input(NETSTORM_PARTICLE_GRADIENT_FIXTURE);
    if (!input) throw std::runtime_error("입자 색 띠 원본 fixture 없음");
    std::string line;std::size_t compared=0,extended=0;
    // 기본/비교 정밀도의 독립 행을 읽되 제품 기본 정밀도만 완료로 센다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=16) throw std::runtime_error("입자 색 띠 원본 열 수 오류");
        const auto precision=std::stoul(row[1]);
        if (precision==0x037f) { ++extended;continue; }
        if (precision!=0x027f) throw std::runtime_error("입자 색 띠 x87 입력 오류");
        const auto palette=Palette(row[2]);
        const auto a=Make(palette,row,3),b=Make(palette,row,6);
        Report(Difference(a,row[9],row[10],row[0]+" A"));
        Report(Difference(b,row[11],row[12],row[0]+" B"));
        const std::array<ParticleGradient,2> pair{a,b};
        const std::array<ParticleGradient,3> triple{a,b,a};
        Report(MergeDifference(pair,row[13],row[0]+" 2색 띠"));
        Report(MergeDifference(triple,row[14],row[0]+" 3색 띠"));
        Report(MergeDifference(std::span<const ParticleGradient>(&a,1),row[15],row[0]+" 1색 띠"));
        ++compared;
    }
    CHECK(compared==kNativeGradientCases);
    CHECK(extended==kNativeGradientCases);
}
// 실제 생성자 관찰을 사용해 두 RGB 끝점의 순서와8+3 구성을3팔레트에서 검증한다.
TEST_CASE(ParticleGradient_FieryTailPeConstants) {
    std::ifstream input(NETSTORM_FIERYTAIL_LIFETIME_FIXTURE);
    if (!input) throw std::runtime_error("Fiery Tail 색 표 원본 fixture 없음");
    std::string line;std::size_t count=0;
    // 단위99 생성자의 cold 기본 입력만 고른다. 명시 RGB helper 입력을 생성자의 증거로 쓰지 않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=12) throw std::runtime_error("Fiery Tail 생성자 색 표 원본 열 수 오류");
        if (std::stoul(row[1])!=0x027f || row[2].find(".COL,0,0,-1,50,127,0,1")==std::string::npos) continue;
        const auto actual=BuildFieryTailPalette(Palette(row[3]));const auto expected=Hex(row[6]);
        CHECK(actual.size()==11 && expected.size()==44);
        // 생성된11개 색 번호를 원본 병합 결과와 직접 비교한다.
        for (std::size_t index=0;index<actual.size();++index) CHECK(actual[index]==Number(expected,index*4,4));
        ++count;
    }
    CHECK(count==3);
}
