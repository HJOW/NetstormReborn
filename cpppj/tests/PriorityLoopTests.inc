// 원문 WinMain 국소96입력의 raw 요청/OS등급 비교를 그대로 확인한다. 실제 OS/게임/MP는 실행하지 않는다.
TEST_CASE(ConfigurationCommandInput_NativePriorityLoop96) {
    const auto rows=LoadFixture(NETSTORM_PRIORITY_LOOP_FIXTURE).rows;std::size_t count{};
    // 기대 wanted/정규화/설정 여부/값은 독립 x86 관측에서만 읽는다.
    for (const auto& row:rows) {
        if (row[0]=="name") continue;CHECK(row.size()==17);if (row.size()!=17) continue;CHECK(Number(row[1])==0);
        const auto requested=std::bit_cast<std::int32_t>(Number(row[3]));const auto wanted=Number(row[2]) ? requested : 0; //명시 비MP active 입력으로 caller가 선택한 raw값만 공급한다.
        const auto plan=PlanNativeThreadPriority(wanted,std::bit_cast<std::int32_t>(Number(row[4])));
        CHECK(std::bit_cast<std::uint32_t>(plan.wanted)==Number(row[8]) && plan.currentLevel==static_cast<std::int32_t>(Number(row[9])));
        CHECK(plan.change==(Number(row[10])!=0));if (plan.change) CHECK(plan.priority==static_cast<std::int32_t>(Number(row[11])));
        CHECK(row[13]=="1" && row[14]=="0");++count;
    }
    CHECK(count==96);
}
