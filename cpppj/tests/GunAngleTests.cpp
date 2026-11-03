// 원본 Gunanim의 모든 유효 인덱스·상한·signed 나눗셈을 독립 반환값과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGunAngle.h"
#include <optional>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 음수-4..-1과0..512 및INT_MAX는 모두 원본의 유효 표 범위에 도달한다.
TEST_CASE(GunAngle_Native518) {
    const auto fixture=LoadFixture(NETSTORM_GUN_ANGLE_FIXTURE);CHECK(fixture.rows.size()==519 && fixture.rows.front()[0]=="frame");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 각 입력의 signed 값과 실제 원문 반환을 그대로 읽는다.
        const auto& row=fixture.rows[index];CHECK(row.size()==2);CHECK(PatchGunGraphFromFrame(static_cast<std::int32_t>(std::stol(row[0])))==Number(row[1]));
    }
}
// 원문이 진단 뒤 표 밖 메모리를 읽는 음수 인덱스는 정상 방향0으로 가장하지 않는다.
TEST_CASE(GunAngle_InvalidNegativeIndex) { CHECK(Throws([] { static_cast<void>(PatchGunGraphFromFrame(-5)); })); }

// signed 인덱스와 현재 진단의 앞단을384개 원문 입력/복귀/중단 경계와 대조한다.
TEST_CASE(GunAngle_NativeCheckingPrefix384) {
    std::size_t count{}; // 표를 읽기 전 고유 원문 입력 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_GUN_FRAME_CHECKING_FIXTURE).rows) { // 음수 표 밖 메모리는 실행하지 않는다.
        CHECK(row.size()==6);const auto input=Split(row[0],',');CHECK(input.size()==4);auto frame=static_cast<std::int32_t>(std::stol(input[0]));auto checking=Number(input[1]);std::string trace,outcome="body";std::optional<std::int32_t> index;
        auto assertion=[&](std::string_view expression,std::string_view file,std::uint32_t line) {
            CHECK(expression=="0 <= index && index < 20" && file=="\\Ns\\O\\Gunanim.cpp" && line==0x387);trace="D:30203c3d20696e64657820262620696e646578203c203230:5c4e735c4f5c47756e616e696d2e637070:903";
            if (!Number(input[2])) throw std::runtime_error("명시 진단 중단");if (Number(input[3])) { checking=0;frame=95; }
        }; // 진단이 원래 인자 저장소를 바꿔도 이미 계산된 지역 인덱스를 유지한다.
        try { index=PatchGunFrameIndex(frame,RawCheckingFlag{checking},assertion);if (!trace.empty()) trace+=';';trace+='B'; }
        catch (const std::runtime_error& error) { if (std::string_view(error.what())!="명시 진단 중단") throw;outcome="assertion"; }
        CHECK(trace==row[1]);if (index) CHECK(*index==static_cast<std::int32_t>(std::stol(row[2])));CHECK(checking==Number(row[3]));CHECK(std::bit_cast<std::uint32_t>(frame)==Number(row[4]));CHECK(outcome==row[5]);++count;
    }
    CHECK(count==384);
}

// 같은 현재 checking을 읽고 진단 복귀 뒤에도 음수 표 밖 역참조를 명시 범위 실패로 남긴다.
TEST_CASE(GunAngle_CurrentCheckingAndNegativeReadBoundary) {
    std::int32_t checking{};const RawCheckingFlag current{checking};std::uint32_t diagnostics{}; // 외부 owner가 바뀌면 같은 뷰가 현재 DWORD를 읽는다.
    const auto assertion=[&](auto,auto,std::uint32_t line) { CHECK(line==0x387);++diagnostics;checking=0; }; // 진단 복귀로 지역 인덱스를 정상값으로 덮어쓰지 않는다.
    CHECK(PatchGunFrameIndex(-5,current)==-1);checking=std::bit_cast<std::int32_t>(0x80000000U);CHECK(PatchGunFrameIndex(-10,current,assertion)==-2 && diagnostics==1 && checking==0);
    checking=-1;bool rejected{};try { static_cast<void>(PatchGunGraphFromFrame(-10,current,assertion)); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && diagnostics==2);
    checking=1;rejected=false;try { static_cast<void>(PatchGunFrameIndex(-5,current)); } catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="Gunanim 인덱스 원문 진단 장치 미연결"; }CHECK(rejected);
    CHECK(PatchGunGraphFromFrame(-4,current)==0 && PatchGunGraphFromFrame(2147483647,current)==0);CHECK(diagnostics==2); // 유효 인덱스는 진단 장치 없이 기존 표/방향을 사용한다.
}
