// 경로 cdecl의 전체 원문 바이트와 실제 Session의 부착 없는 고정 클래스 생성을 대조한다.
#include "TestSupport.h"
#include "o/RawPathRuntime.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace netstorm::o;
namespace { // 원문 입력/출력 해석만 담당하며 생성 결과를 계산하지 않는다.
std::vector<std::string> SplitPathCtor(std::string_view text,char delimiter) { // 마지막 열까지 읽는다.
    std::vector<std::string> result;std::size_t start=0; // 현재 열의 시작이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 각 구분자를 한 번 소비한다.
}
std::vector<std::vector<std::string>> PathCtorRows() { // 실제 PE가 출력한32행만 읽는다.
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/path-constructor-x86.tsv");if (!input) throw std::runtime_error("경로 생성자 fixture 없음");std::vector<std::vector<std::string>> rows;std::string line; // 원문 행/열이다.
    while (std::getline(input,line)) { if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;auto row=SplitPathCtor(line,'\t');if (row.size()!=5) throw std::runtime_error("경로 생성자 fixture 열 오류");rows.push_back(std::move(row)); } // 파일 전체를 순서대로 읽는다.
    return rows;
}
std::vector<std::uint8_t> PathCtorExpected(const std::string& rle) { // Native 전체 객체의 반복 BYTE를 그대로 푼다.
    std::vector<std::uint8_t> result; // 누락된 바이트를 임의0으로 보충하지 않는다.
    for (const auto& run:SplitPathCtor(rle,',')) { const auto pair=SplitPathCtor(run,':');if (pair.size()!=2) throw std::runtime_error("경로 생성자 RLE 오류");const auto count=std::stoul(pair[0]),byte=std::stoul(pair[1]);if (count>0x186f0 || byte>255 || result.size()+count>0x186f0) throw std::runtime_error("경로 생성자 RLE 범위 오류");result.insert(result.end(),count,static_cast<std::uint8_t>(byte)); } // 모든 Native run을 한 번 소비한다.
    if (result.size()!=0x186f0) throw std::runtime_error("경로 생성자 전체 객체 폭 오류");return result;
}
}
TEST_CASE(PathConstructorCurrent_Native32WholeObjectsPreserveUninitializedHeap) { // PATCH8종의0/A5 heap과 잔여 caller 인자를 원문 전체 바이트와 대조한다.
    const auto rows=PathCtorRows();CHECK(rows.size()==32);
    for (const auto& row:rows) { const auto kind=std::stoul(row[0]);RawPathRuntime runtime(OriginalEdition::Patch1078,kind==54,static_cast<std::uint8_t>(std::stoul(row[2])),kind==39,kind==40,kind==42,kind==24,kind==41,kind==38);const auto expected=PathCtorExpected(row[4]);CHECK(std::equal(runtime.Bytes().begin(),runtime.Bytes().end(),expected.begin())); } // 각 생성 결과의100080BYTE를 대조한다.
}
