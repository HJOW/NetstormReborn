// LAN 시계 보정의 부호 있는 경과를 같은 실제 Archer/Missile 상태로 전달하는지 확인한다.
#include "CampaignInstallSupport.h"
#include "o/GameClock.h"
#include <cmath>
#include <limits>

// 원문00460e90의 차분과00467c80의 비양수 분기를 위해 음수 경과도 그대로 보존한다.
TEST_CASE(CampaignClock_SynchronizationRewindsActualArcherFrame) {
    InstallScene scene;GameClock clock(1000);std::uint32_t ticks{}; // 같은 주소를 보정 writer와 시계가 공유한다.
    clock.BindSynchronizationTicks(ticks);
    const auto prepare=[&](std::uint32_t milliseconds) { // 실제 시계 결과를 조립 부모에 전달하고 외부 원본 상태를 관측한다.
        const auto frame=clock.Capture(milliseconds);scene.combat->PrepareFrame(frame);scene.world.PrepareFrame(frame);
        CHECK(scene.archerState.now==frame.game && scene.archerState.delta==frame.delta);
        CHECK(scene.missileState.now==frame.game && scene.core.processState.Now()==frame.game);return frame;
    };
    const auto first=prepare(11000);CHECK(first.game==10 && first.delta==10);
    ticks=3000;const auto rewind=prepare(12000);CHECK(rewind.game==8 && rewind.delta==-2 && rewind.wall==11);
    const auto same=prepare(12000);CHECK(same.game==8 && same.delta==0);
    ticks=1000;const auto forward=prepare(13000);CHECK(forward.game==11 && forward.delta==3);
    clock.Pause(13000);ticks=4000;const auto paused=prepare(16000);CHECK(paused.game==11 && paused.delta==0);
    clock.Resume(17000);const auto resumed=prepare(18000);CHECK(resumed.game==12 && resumed.delta==1);
    CHECK(resumed.number==6 && !clock.IsPaused());
}

// 장기 Client 시각을 빌리는 월드도 독립 스냅샷 대신 보정된 현재 주소를 사용한다.
TEST_CASE(CampaignClock_WorldBorrowsRewoundSharedTimeWithoutWritingIt) {
    InstallScene scene;double current=10;scene.core.processState.sharedNow=&current; // 제품과 같은 현재 시각 주소를 대여한다.
    scene.world.PrepareFrame({999,10,1,1});CHECK(scene.world.Report().starts_with("raw_world\t10\t1\t"));
    current=8;scene.world.PrepareFrame({999,11,-2,2});CHECK(current==8 && scene.world.Report().starts_with("raw_world\t8\t2\t"));
    const auto before=scene.world.Report();current=std::numeric_limits<double>::quiet_NaN();bool rejected{}; // 같은 대여 주소의 비유한 값도 쓰기 전에 거부한다.
    try { scene.world.PrepareFrame({999,12,1,3}); }catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && std::isnan(current) && scene.world.Report()==before);scene.core.processState.sharedNow=nullptr;
}

// NaN/무한대 거부는 모든 상태 쓰기 전에 수행하며 부호 있는 유한 경과 허용과 구별한다.
TEST_CASE(CampaignClock_NonfiniteFrameRejectedBeforeStateMutation) {
    InstallScene scene;scene.combat->PrepareFrame({7,10,-0.5,1}); // 최초 정상 입력부터 음수 경과를 허용한다.
    const std::array<double,3> invalid{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}; // 유한성 검사의 세 경계다.
    for (const auto value:invalid) { // 현재 시각과 경과의 잘못된 입력을 각각 독립 실행한다.
        for (const bool badGame:{false,true}) { // 같은 상태에 쓰기 전 예외가 나는지 확인한다.
            bool rejected{};try { scene.combat->PrepareFrame({badGame ? value : 12,20,badGame ? 0.25 : value,2}); }
            catch (const std::invalid_argument&) { rejected=true; }
            CHECK(rejected && scene.archerState.now==7 && scene.archerState.delta==-0.5 && scene.missileState.now==7);
        }
    }
}
