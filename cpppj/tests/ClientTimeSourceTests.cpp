// Client 시각 공급원(client/ClientTimeSource)의 기본 OS 시계와 검사 전용 고정 시각 모드 계약을 확인한다.
// 2026-11-09: webpj/dotnetpj L2·L3 결정론 대조용 --fixed-frame-ms 추가와 함께 작성했다.
#include "TestSupport.h"
#include "client/ClientTimeSource.h"
#include <ctime>
#include <stdexcept>

using namespace netstorm::client;
namespace {
// 호출이 표준 예외로 거부되는지 확인한다(다른 시험 파일과 같은 지역 도우미). (2026-11-09 추가)
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
}

// 기본 상태는 꺼져 있고 전진 호출이 아무것도 하지 않으며 유닉스 초는 OS 시계와 같은 범위다.
TEST_CASE(ClientTimeSource_DefaultUsesOperatingSystemClock) {
    DisableFixedClientTimeForTests();
    CHECK(!FixedClientTimeEnabled());
    const auto before=std::time(nullptr);AdvanceFixedClientTime();const auto seconds=ClientUnixSeconds();const auto after=std::time(nullptr);
    CHECK(seconds>=before && seconds<=after);
}

// 고정 모드는 원점에서 시작해 프레임마다 정확히 같은 밀리초만 전진하고, 유닉스 초는 경과 초를 내림해 더한다.
TEST_CASE(ClientTimeSource_FixedModeAdvancesOnlyPerFrame) {
    DisableFixedClientTimeForTests();
    EnableFixedClientTime({25,4294967000U,946684800});
    CHECK(FixedClientTimeEnabled() && ClientMilliseconds()==4294967000U && ClientMilliseconds()==4294967000U && ClientUnixSeconds()==946684800);
    // 40프레임(1000ms) 전진 뒤 값과 unsigned 32비트 감김을 확인한다.
    for (int frame=0;frame<40;++frame) AdvanceFixedClientTime();
    CHECK(ClientMilliseconds()==static_cast<std::uint32_t>(4294967000ULL+1000ULL) && ClientUnixSeconds()==946684801);
    CHECK(Throws([] { EnableFixedClientTime({25}); }));
    DisableFixedClientTimeForTests();
    CHECK(Throws([] { EnableFixedClientTime({0}); }) && Throws([] { EnableFixedClientTime({60001}); }) && Throws([] { EnableFixedClientTime({25,0,-1}); }) && !FixedClientTimeEnabled());
}
