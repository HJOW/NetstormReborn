// 실제 PE의 비교 호출 순서와 후보 배열 전체를 재생한다. 표준 정렬 결과를 기대값으로 사용하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawCarrierCandidateSort.h"
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// TSV의 명시 DWORD 입력/배열/비교 주소를 읽는다.
std::vector<std::uint32_t> SortNumbers(std::string_view text) {
    std::vector<std::uint32_t> result;if (text=="-") return result;
    // 값은 SID로 축소하지 않는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
// 보호 PE에 제공한 물리 바이트와 동일한 입력만 생성한다.
std::vector<std::uint8_t> SortPool(std::uint32_t pattern) {
    std::vector<std::uint8_t> result(24000*50);
    // 슬롯 경계의 WORD와 감김으로 생기는 비정렬 WORD를 구분하는 입력이다.
    for (std::size_t i=0;i<result.size();++i) result[i]=pattern ? static_cast<std::uint8_t>(i*37+(i/50)*11+pattern*19) : 0;return result;
}
// 정렬하지 않은 원래 입력이며 일부 값은 유효 SID 범위를 벗어난다.
std::vector<std::uint32_t> SortValues(std::uint32_t pattern) {
    std::vector<std::uint32_t> result(500);
    // 실제 정렬은 이 값의 크기나 유효성에 의존하지 않아야 한다.
    for (std::uint32_t i=0;i<500;++i) result[i]=pattern==0 ? i+1 : pattern==1 ? 500-i : 0xf0000000U+i*0x10203U;return result;
}
}
// 작은 선택 정렬/큰 분할/동률/pivot 이동/주소 감김을 원본의 전체 배열·호출 인자로 대조한다.
TEST_CASE(CarrierCandidateSort_NativeComparatorAndLegacyQsort) {
    SidPool pool(OriginalEdition::Patch1078,24000);const std::array<std::vector<std::uint8_t>,2> inputs{SortPool(0),SortPool(1)};
    std::size_t count{},diagnostics{}; // 전체 재생 수와 불일치 출력 한도다.
    // 두 정밀도 결과도 같아야 하며 C++는 정수 경로 하나로 같은 원문을 재생한다.
    for (const auto& fields:LoadFixture(NETSTORM_CARRIER_CANDIDATE_SORT_FIXTURE).rows) {
        CHECK(fields.size()==9);if (fields.size()!=9) continue;
        const auto row=SortNumbers(fields[0]);CHECK(row.size()==8);if (row.size()!=8) continue;
        const auto& input=inputs.at(row[3]);auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size());std::copy(input.begin(),input.end(),raw.begin());
        auto candidates=SortValues(row[4]);const auto initial=candidates;std::vector<std::uint32_t> calls;
        // 원래 주소로 재배치할 뿐 비교 결과/키/교환은 공급하지 않는다.
        RawCarrierCandidateSort sorter(pool,[&](const void* pointer) {
            if (pointer==pool.Bytes().data()) return row[7];
            const auto address=reinterpret_cast<std::uintptr_t>(pointer),base=reinterpret_cast<std::uintptr_t>(candidates.data());
            if (address<base || address-base>=candidates.size()*4) throw std::out_of_range("후보 정렬 관측의 배열 주소 범위 오류");
            const auto mapped=row[1]+static_cast<std::uint32_t>(address-base);calls.push_back(mapped);return mapped;
        });
        std::int32_t result{};if (row[0]) result=sorter.Compare(row[5],row[6]);else sorter.Sort(std::span(candidates).first(row[2]));
        if (row[0]) calls={row[5],row[6]}; // 직접 호출에서는 thunk가 원본 비교기에 전달한 두 주소다.
        const auto expected=SortNumbers(fields[2]),expectedCalls=SortNumbers(fields[3]);const auto matches=std::equal(candidates.begin(),candidates.begin()+row[2],expected.begin(),expected.end());
        if ((!matches || calls!=expectedCalls || result!=std::stoi(fields[1])) && diagnostics++<5) std::cout<<"후보 정렬 입력 "<<fields[0]<<" 결과 "<<result<<" 비교호출 "<<calls.size()/2<<" 원본 "<<expectedCalls.size()/2<<'\n';
        CHECK(result==std::stoi(fields[1]));CHECK(matches);CHECK(calls==expectedCalls);
        CHECK(std::equal(candidates.begin()+row[2],candidates.end(),initial.begin()+row[2]));CHECK(std::equal(raw.begin(),raw.end(),input.begin()));
        CHECK(Adler(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(candidates.data()),row[2]*4))==Number(fields[4]));
        CHECK(std::equal(fields.begin()+1,fields.begin()+5,fields.begin()+5));++count;
    }
    CHECK(count==513);
}
// 기본 포인터 경로의 무효 주소와 CD 오연결을 확인한다.
TEST_CASE(CarrierCandidateSort_HostAddressAndEdition) {
    SidPool pool(OriginalEdition::Patch1078,24000);RawCarrierCandidateSort sorter(pool);CHECK(sorter.Compare(0,0)==0);
    RawCarrierCandidateSort explicitAddress(pool,[](const void* pointer) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer)); });
    // 기본 변환도 실제 하위32비트를 사용하며 할당 주소가 상한에서 감기는 경우를 가정으로 제외하지 않는다.
    for (const auto address:std::array<std::uint32_t,5>{0,1,23999,0x051eb854,0xffffffff}) CHECK(sorter.Compare(address,1)==explicitAddress.Compare(address,1));
    std::array<std::uint32_t,0> empty{};sorter.Sort(empty);std::array<std::uint32_t,1> single{42};sorter.Sort(single);CHECK(single[0]==42);
    bool rejected{};try { SidPool cd(OriginalEdition::Cd1072,24000);RawCarrierCandidateSort bad(cd); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
