// 원본 fulltime epoch와 서로 다른 프로세스 원점 사이의 실제 시작 복구 관문을 검사한다.
#include "TestSupport.h"
#include "o/GameClock.h"
#include "o/RawStartupStateCheck.h"
#include <cmath>

// 00460cd0/00460d70의 초 변환 이전 DWORD 감김을 고정 계산값과 대조한다.
TEST_CASE(GameClockEpoch_UsesNativeDwordWrapBeforeSecondsAndSynchronization) {
    netstorm::o::GameClock clock(1000,943722600U);std::uint32_t adjustment=1500; // 2026-11-27 time32에서 원본 epoch를 뺀 명시 입력과 실제 보정 주소다.
    clock.BindSynchronizationTicks(adjustment);
    CHECK(std::abs(clock.WallSeconds(3500)-3124764.676)<1e-9);
    CHECK(std::abs(clock.GameSeconds(3500)-3124763.176)<1e-9);
    clock.Pause(3500);CHECK(clock.GameSeconds(7000)==clock.GameSeconds(3500));clock.Resume(7500);
    CHECK(std::abs(clock.GameSeconds(8500)-3124764.176)<1e-9);
    clock.Reset();CHECK(std::abs(clock.GameSeconds(8500)-3124768.176)<1e-9);
    netstorm::o::GameClock wrapped(0xfffffff0U,4294967U); // OS 밀리초와 epoch 밀리초의 두 감김을 함께 통과한다.
    CHECK(std::abs(wrapped.WallSeconds(0x200U)-0.232)<1e-12);
}

// 이전 실행이 길었어도 새 실행의 작은 경과 시간이 저장 시각보다 과거로 돌아가지 않아야 한다.
TEST_CASE(GameClockEpoch_RestartedClockPassesActualStartupAgeGate) {
    netstorm::o::GameClock before(1000,943722600U),after(900000,943722630U); // 별도 실행의 원점과30초 뒤 epoch 초다.
    const auto saved=static_cast<std::int32_t>(before.WallSeconds(3500));std::size_t questions{}; // 실제 writer처럼 초를 절삭하고 실제 관문의 안내 횟수를 관측한다.
    netstorm::o::StartupStateCheckHooks hooks; // 계산과45분 분기는 제품 부모에 맡긴다.
    hooks.configInt=[](auto) { return 1; };hooks.readSavedState=[saved] { return saved; };
    hooks.wallSeconds=[&after] { return after.WallSeconds(901500); };
    hooks.translate=[](auto key) { return key; };
    hooks.formatButtons=[](auto,auto,auto)->std::string_view { return "buttons"; };
    hooks.formatWarning=[](auto,double minutes,auto)->std::string_view { CHECK(minutes>0.49 && minutes<0.50);return "question"; };
    hooks.tell=[&questions](auto,auto,auto) { ++questions; };
    CHECK(netstorm::o::CheckStartupState(hooks)==1 && questions==1);
    hooks.wallSeconds=[&after] { return after.WallSeconds(3600000); }; // 같은 저장에서45분을 넘기면 기존 복구 제한이 유지된다.
    CHECK(netstorm::o::CheckStartupState(hooks)==0 && questions==1);
}
