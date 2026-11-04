// 양판본 실제 Form 생성자의 물리 슬롯/단일DWORD 저장을 격리 원문과 대조한다.
#include "TestSupport.h"
#include "o/SquidForms.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace { // 입력/기대 BYTE를 읽을 뿐 생성 결과를 계산하지 않는다.
std::vector<std::string> Split(std::string_view text,char delimiter) { // 마지막 빈 열도 보존한다.
    std::vector<std::string> result;std::size_t start=0; // 읽은 열과 현재 시작이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 각 구분자를 한 번씩 소비한다.
}
std::vector<std::uint8_t> Hex(std::string_view text) { // 실제 원문 슬롯 전체를 BYTE로 복원한다.
    if (text.size()%2) throw std::runtime_error("Form 생성자 hex 폭 오류");std::vector<std::uint8_t> result; // 원문 바이트 순서다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 두 문자마다 한 BYTE다.
    return result;
}
}
TEST_CASE(FormConstructorCurrent_Native56PhysicalSlotsOnlyStoreVtableDword) { //0/예약/free/dead/끝 슬롯의 나머지 payload·풀 카운터를 원문과 대조한다.
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/form-constructor-x86.tsv");CHECK(input.good());std::string line;std::size_t count=0; // Native 관측 재생 수다.
    while (std::getline(input,line)) { // 각 독립 입력에 새 풀을 구성하여 SID0 머리 덮어쓰기를 다음 사례에 전파하지 않는다.
        if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==6);if (row.size()!=6) return;const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(edition,65535,false);const auto sid=Sid{static_cast<std::uint16_t>(std::stoul(row[2]))};const auto before=Hex(row[3]),expected=Hex(row[4]);auto raw=pool.Slot(sid);CHECK(raw.size()==before.size() && raw.size()==expected.size());if (raw.size()!=before.size() || raw.size()!=expected.size()) return;std::copy(before.begin(),before.end(),const_cast<std::uint8_t*>(raw.data()));const auto free=pool.FreeCount();SquidForms forms(pool);
        forms.ConstructVtableAt(static_cast<std::uint32_t>(std::stoul(row[1])),sid);CHECK(std::equal(raw.begin(),raw.end(),expected.begin()) && pool.FreeCount()==free && raw[10]==74 && std::equal(raw.begin()+4,raw.end(),before.begin()+4));CHECK(std::stoul(row[5])==static_cast<std::uint32_t>(sid.value)*pool.Layout().stride);++count; // 반환 포인터는 원문 물리 상대 주소이며 현재 caller는 쓰지 않는다.
    }
    CHECK(count==56);
}
