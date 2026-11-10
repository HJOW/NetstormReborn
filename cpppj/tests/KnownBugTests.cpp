// 10차 알려진 버그(NB01~NB34) 중 순수 함수/작은 경계로 확인할 수 있는 교정의 회귀 검사다. 게임/창/MP를 실행하지 않는다.
#include "TestSupport.h"
#include "client/NativeMetaParticipantName.h"
#include "client/StyleTextMarkup.h"

// NB02: 채팅 글자의 엠보스 오프셋 ~[E.x# y#]는 두 좌표가 모두 -4..4 안일 때만 적용한다.
// 원문은 한 좌표만 범위 안이면 다른 좌표를 임의로 키워 글자를 뒤로 밀 수 있었다.
TEST_CASE(KnownBug_NB02_EmbossOffsetRequiresBothCoordinatesInRange) {
    using netstorm::client::StyleTextOffsetAllowed;
    CHECK(StyleTextOffsetAllowed(0,0) && StyleTextOffsetAllowed(-4,4) && StyleTextOffsetAllowed(4,-4)); // 경계값은 허용한다.
    CHECK(!StyleTextOffsetAllowed(5,0) && !StyleTextOffsetAllowed(0,-5)); // 한 좌표가 범위를 벗어나면 거부한다.
    CHECK(!StyleTextOffsetAllowed(0,1000) && !StyleTextOffsetAllowed(1000,0)); // 원문 악용: 한 좌표만 정상이고 다른 좌표가 매우 큰 경우다.
    CHECK(!StyleTextOffsetAllowed(-1000,1000)); // 둘 다 범위 밖이면 당연히 거부한다.
}

// NB09: 최대 길이를 넘는 길드 태그는 이름표 버퍼를 넘어 종료하지 않고 태그를 줄여 서식에 맞춘다. 정상 길이는 그대로다.
TEST_CASE(KnownBug_NB09_LongGuildTagIsShortenedInsteadOfFailing) {
    using namespace netstorm::client;
    NativeMetaParticipantNameHooks hooks;hooks.translate=[](auto text) { return std::string(text); };hooks.weight=[](auto) { return std::uint8_t{3}; };
    std::vector<std::uint8_t> record(0xd8+80,0); // 이름표 목적 버퍼는 80바이트다.
    const char name[]="Bob";std::copy_n(name,sizeof name,record.begin()+0x26);
    WriteNativeMetaParticipantLabel(record,"~aABC:",hooks);
    const std::string normal(reinterpret_cast<const char*>(record.data()+0xd8));
    CHECK(normal.starts_with("~j~aABC:~wBob\n") && normal.size()<80); // 정상 길이는 그대로 쓴다.
    WriteNativeMetaParticipantLabel(record,"~a"+std::string(200,'T')+":",hooks); // 원문 악용/실수: 200자 길드 태그다.
    const std::string shortened(reinterpret_cast<const char*>(record.data()+0xd8));
    CHECK(shortened.size()<80 && shortened.starts_with("~j~aTTT") && shortened.find(":~wBob")!=std::string::npos); // 이름·서식은 보존하고 태그만 줄었다.
    bool failed{};try { WriteNativeMetaParticipantLabel(std::span<std::uint8_t>(record).first(0xd8+20),"",hooks); }catch (const std::out_of_range&) { failed=true; }
    CHECK(failed); // 길드 없이도 버퍼가 모자라면 원문처럼 명시 거부한다.
}
