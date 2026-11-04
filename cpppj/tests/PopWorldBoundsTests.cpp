// 원문Point.isValid 전체와 Pop 입구의 현재 signed크기/float저장/NaN 단락을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSquidPopParent.h"
#include <filesystem>
#include <limits>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
TEST_CASE(PopWorldBounds_Native260OrderedPointAndPopFrontFloatSnapshot) { // 53/64비트 원문이 같은260입력의 비트/메모리 조회 횟수다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/pop-world-bounds-x86.tsv").string().c_str());std::size_t count=0; // 원문 독립 입력 재생 수다.
    for (const auto& row:fixture.rows) { CHECK(row.size()==8);if (row.size()!=8) return;const auto limit=static_cast<std::int32_t>(std::stoll(row[0]));const auto x=std::bit_cast<float>(Number(row[1])),y=std::bit_cast<float>(Number(row[2]));std::uint32_t reads=0;const std::function<std::int32_t()> current=[&] { ++reads;return limit; };const auto valid=IsCurrentPopPointValid(x,y,current);CHECK(valid==(Number(row[3])!=0) && reads==Number(row[4])); // 기대값은 Native40e800의 반환/읽기다.
        reads=0;float mappedX=x,mappedY=y;if (!IsCurrentPopPointValid(mappedX,mappedY,current)) mappedX=mappedY=10.0F;CHECK(std::bit_cast<std::uint32_t>(mappedX)==Number(row[5]) && std::bit_cast<std::uint32_t>(mappedY)==Number(row[6]) && reads==Number(row[7]));++count; // 같은 비교를 사용하는 부모 입구의 원문SetPosition 인자와 대조한다.
    }
    CHECK(count==260);
}
TEST_CASE(PopWorldBounds_SelectedGetterSurvivesSelfUnbindingAndInvalidXSkipsIt) { // 선택 함수 소유권과x단락은 Native읽기0/1의 C++ 장치 계약이다.
    std::function<std::int32_t()> getter;auto keep=std::make_shared<int>(32);std::weak_ptr<int> lifetime=keep;std::uint32_t calls=0; // 자기 연결을 반납할 getter의 유일한 장기 캡처다.
    getter=[&,keep] { getter={};++calls;CHECK(!lifetime.expired());return *keep; };keep.reset();CHECK(!lifetime.expired());CHECK(IsCurrentPopPointValid(31,20,getter));CHECK(calls==1 && lifetime.expired() && !getter);
    getter=[&]()->std::int32_t { ++calls;throw std::runtime_error("선택 세계 크기 조회 중단"); };CHECK(!IsCurrentPopPointValid(std::numeric_limits<float>::quiet_NaN(),10,getter) && !IsCurrentPopPointValid(-1,10,getter) && calls==1);CHECK(Throws([&] { static_cast<void>(IsCurrentPopPointValid(1,std::numeric_limits<float>::quiet_NaN(),getter)); }) && calls==2); // y검사 전에 읽는 현재 크기의 실패는 숨기지 않는다.
    CHECK(IsCurrentPopPointValid(255,255) && !IsCurrentPopPointValid(256,10)); // 기존 독립 미연결256 입력만 유지한다.
}
