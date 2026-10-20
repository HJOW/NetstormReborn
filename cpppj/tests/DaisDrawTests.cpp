// 제품 구현 전에 동결한 다이스 추가 그림768입력을 전체 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDaisDraw.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 실제 idiv의 부호/감김과 함수 자체의 dead/추상 필터 부재를 원본 요청에 대조한다.
TEST_CASE(DaisDraw_Native768) {
    std::size_t rows{};
    // 두 원본 정밀도의 동일 결과를 한 행도 생략하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_DAIS_DRAW_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;const auto input=Split(row[0],',');CHECK(input.size()==6);if (input.size()!=6) return;
        std::array<std::uint8_t,50> raw{};Put(raw,0,0x507950);raw[10]=165;raw[11]=static_cast<std::uint8_t>(Number(input[2]));raw[40]=static_cast<std::uint8_t>(Number(input[2])>>8);Put(raw,36,Number(input[0]));
        const auto before=raw;std::string trace; // 원본 함수에서 상태가 보존된 같은 입력이다.
        DrawDaisAdditional(raw,std::bit_cast<std::int32_t>(Number(input[1])),[&](std::uint32_t type,std::int32_t frame) {
            CHECK(type==169);trace="draw:50:"+input[3]+':'+input[4]+':'+input[5]+':'+std::to_string(std::bit_cast<std::uint32_t>(frame));
        });
        CHECK(trace==row[1] && row[1]==row[2] && raw==before);++rows;
    }
    CHECK(rows==768);
}

// 다른 VT나 손상된 TYPE 프레임 수를 일반 다이스 효과로 처리하지 않는다.
TEST_CASE(DaisDraw_RejectsUnboundContext) {
    std::array<std::uint8_t,50> raw{};Put(raw,0,0x507950);raw[10]=165;std::size_t draws{};const auto draw=[&](std::uint32_t,std::int32_t) { ++draws; };
    CHECK(IsPatchDais(raw));CHECK(Throws([&] { DrawDaisAdditional(raw,0,draw); }));CHECK(Throws([&] { DrawDaisAdditional(raw,180,{}); }));
    raw[10]=164;CHECK(!IsPatchDais(raw));CHECK(Throws([&] { DrawDaisAdditional(raw,180,draw); }));CHECK(draws==0);
}
