// 실제 Client가 소유한 저장 관문 DWORD와 설정/스크립트 연결을 Run 없이 검사한다. 창/장치/파일쓰기는 실행하지 않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ScriptCommand.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawFortBadgesSave.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
// 기존 ClientPause 검사처럼 실제 보호 자산을 읽기만 하는 Client 구성값이다.
netstorm::client::ClientOptions Options() {
    netstorm::client::ClientOptions options; // Run을 호출하지 않아 시작 설정 저장/Win32 창을 만들지 않는다.
    options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";
    return options;
}
// 파일 저장에 빌릴 API가 임시값이나 bool이 아니라 실제 const DWORD 참조인지 컴파일 때 확인한다.
static_assert(std::is_same_v<decltype(std::declval<const netstorm::client::Client&>().ReadOnlyForts()),const std::uint32_t&>);
}

// 실제 Set/SetInt가 반환되기 전에 빌린 같은 DWORD가 전체 signed 비트로 바뀌어야 한다.
TEST_CASE(ClientFortFileOptions_LiveDwordAndSynchronousSet) {
    netstorm::client::Client client(Options()); // 전체 Client의 실제 생성/소멸을 사용하며 추출 어댑터가 아니다.
    auto& config=client.Configuration(); // Client가 가진 동일 설정 객체를 직접 사용한다.
    const auto* address=&client.ReadOnlyForts(); // Session 저장 소비자가 빌릴 실제 한 DWORD 주소다.
    CHECK(*address==0 && static_cast<bool>(config.onChanged));
    config.Configuration().AppendSection("END");
    const std::array<std::pair<const char*,std::uint32_t>,10> cases{{
        {"0",0},{"1",1},{"-1",0xffffffffU},{"2",2},{"2147483648",0x80000000U},
        {"4294967295",0xffffffffU},{"4294967296",0},{"-2147483649",0x7fffffffU},{"",0},{"abc",0}}}; // 원본 atol의 비0/음수/넘침/존재하는 빈 값 입력이다.
    for (const auto& [text,expected]:cases) { // callback이 현재 설정을 읽은 결과를 Set 직후 관찰한다.
        config.Set("readOnlyForts",text);
        CHECK(*address==expected && address==&client.ReadOnlyForts());
    }
    config.SetInt("readOnlyForts",-1);
    CHECK(*address==0xffffffffU && config.Configuration().changed);
    config.SetInt("unrelatedOption",17);
    CHECK(*address==0xffffffffU && address==&client.ReadOnlyForts());
}

// 실제 문자열 고리/Client 입력 큐의 Config를 순서대로 소비하며 예약 자체는 장기 DWORD를 바꾸지 않는다.
TEST_CASE(ClientFortFileOptions_ActualQueuedScriptConfig) {
    netstorm::client::Client client(Options()); // 실제 Client의 입력/설정/전역 소유권을 함께 사용한다.
    auto& config=client.Configuration();auto& queue=client.Input(); // 저장 관문과 명령이 같은 Client 수명에 속한다.
    config.Configuration().AppendSection("END");
    const auto& live=client.ReadOnlyForts(); // 값이 아니라 장기 필드를 빌려 후속 변경을 그대로 관찰한다.
    netstorm::client::ScriptStringRing strings; // 실제 Config 인자를 유지하는80칸 문자열 고리다.
    netstorm::client::ScriptCommandHooks commands; // 실제 복원 Config 명령만 입력 큐에 등록한다.
    commands.lookup=[](std::string_view name) { CHECK(name=="Config");return netstorm::client::kScriptConfig; };
    netstorm::client::QueueScriptCommand("Config,readOnlyForts=-1",queue,strings,commands);
    netstorm::client::QueueScriptCommand("Config,readOnlyForts=0",queue,strings,commands);
    CHECK(live==0 && !queue.Empty());
    const auto first=queue.Pop(false); // 첫 실제 Config를 적용하기 전 둘째 예약을 소비하지 않는다.
    CHECK(first.code==netstorm::client::kScriptConfig && first.x==first.y);
    netstorm::client::ApplyScriptConfig(strings.Resolve(std::bit_cast<std::uint32_t>(first.x)),config);
    CHECK(live==0xffffffffU && !queue.Empty());
    const auto second=queue.Pop(false); // 같은 Client의 두번째 실제 Set도 동일 DWORD를 동기 갱신한다.
    CHECK(second.code==netstorm::client::kScriptConfig && second.x==second.y);
    netstorm::client::ApplyScriptConfig(strings.Resolve(std::bit_cast<std::uint32_t>(second.x)),config);
    CHECK(live==0 && queue.Empty() && config.Configuration().changed);
}

// 실제 Startup의 Load/InstallDir/CDDir callback과 누락 키 재적재가 같은 현재 DWORD를 보존한다.
TEST_CASE(ClientFortFileOptions_StartupAndMissingReloadPreserveCurrent) {
    netstorm::client::Client client(Options()); // Run의 디스크 저장 없이 실제 Startup 설정 읽기만 실행한다.
    auto& config=client.Configuration(); // 생성자에 설치된 callback을 바꾸거나 검사 callback으로 대체하지 않는다.
    netstorm::o::ConfigInterface::StartupOptions startup; // 명시 인자 첫 정의가 뒤의 읽기 전용 파일보다 우선한다.
    startup.commandLine="readOnlyForts=-1";
    config.Startup(startup);
    const auto* address=&client.ReadOnlyForts(); // Startup 뒤에도 실제 필드 주소를 빌린다.
    CHECK(*address==0xffffffffU);
    CHECK(config.Configuration().RemoveKey("readOnlyForts"));
    config.SetInt("unrelatedOption",1);
    CHECK(*address==0xffffffffU);
    const std::array<std::string,1> missing{""}; // 실제 Load로 모든 readOnly 정의가 없는 설정을 다시 만든다.
    config.Load(missing);
    config.SetInt("unrelatedOption",2);
    CHECK(*address==0xffffffffU && address==&client.ReadOnlyForts());
    config.Set("readOnlyForts",""); // 누락과 달리 존재하는 빈 문자열은 원본 atol0으로 바꾼다.
    CHECK(*address==0);
}

// 중복 Client 구성 실패가 기존 callback을 바꾸지 않으며 실제 소멸 뒤 새 Client는 BSS0부터 시작한다.
TEST_CASE(ClientFortFileOptions_ExclusiveClientAndFreshLifetime) {
    {
        netstorm::client::Client client(Options()); // 첫 실제 수명에서 비0 값을 유지한다.
        client.Configuration().Configuration().AppendSection("END");
        client.Configuration().SetInt("readOnlyForts",-1);
        bool rejected=false; // 두번째 실제 생성이 Client 단일 소유 가드에서 거부됐는지 확인한다.
        try { netstorm::client::Client duplicate(Options()); }
        catch (const std::logic_error&) { rejected=true; }
        CHECK(rejected && client.ReadOnlyForts()==0xffffffffU);
        client.Configuration().SetInt("readOnlyForts",2);
        CHECK(client.ReadOnlyForts()==2 && static_cast<bool>(client.Configuration().onChanged));
    }
    netstorm::client::Client fresh(Options()); // 앞 Client의 실제 소멸과 전역 등록 반납 이후 독립 수명이다.
    CHECK(fresh.ReadOnlyForts()==0 && static_cast<bool>(fresh.Configuration().onChanged));
    fresh.Configuration().Configuration().AppendSection("END");
    fresh.Configuration().SetInt("readOnlyForts",1);
    CHECK(fresh.ReadOnlyForts()==1);
}

// 실제 Client가 전달한 한 장부는 Config로 다시 초기화되지 않으며 다음 Client는 새 CRT/초기 count0을 가진다.
TEST_CASE(ClientFortFileOptions_SameBadgesSurviveConfigAndNewClientStartsZero) {
    std::shared_ptr<netstorm::o::RawFortBadgesGlobals> previous; // 첫 Client의 실제 장부 수명을 관측 끝까지 유지하여 주소 재사용과 혼동하지 않는다.
    {
        netstorm::client::Client client(Options()); // Run을 하지 않아 시작 reset 호출 자체는 이 검사의 범위가 아니다.
        const auto& owner=client.FortBadges(); // 복제 장부가 아닌 Client의 실제 shared_ptr 슬롯을 빌린다.
        CHECK(owner && &owner==&client.FortBadges());
        previous=owner;
        auto records=owner->Records(); // MetaLoader/getter/writer가 빌릴 실제 전체6160BYTE다.
        CHECK(records.size()==netstorm::o::kFortBadgesGlobalsBytes && records.data()==client.FortBadges()->Records().data());
        CHECK(netstorm::o::ReadFortBadgeCount(records,71)==0);
        const std::array<std::uint8_t,4> countBytes{0x78,0x56,0x34,0x12}; // 첫 type71의 현재 count를12345678DWORD로 정하는 명시 검사 입력이다.
        std::copy(countBytes.begin(),countBytes.end(),records.begin()+netstorm::o::kFortBadgesCountOffset);
        const std::vector<std::uint8_t> before(records.begin(),records.end()); // Config가 count 이외 CRT 자료까지 보존하는지 전수를 비교한다.
        client.Configuration().Configuration().AppendSection("END");
        client.Configuration().SetInt("readOnlyForts",-1);
        netstorm::client::ApplyScriptConfig("unrelatedOption=1",client.Configuration());
        CHECK(client.ReadOnlyForts()==0xffffffffU && client.FortBadges().get()==previous.get());
        CHECK(client.FortBadges()->Records().data()==records.data() && std::equal(before.begin(),before.end(),records.begin()));
        CHECK(netstorm::o::ReadFortBadgeCount(records,71)==0x12345678U);
    }
    netstorm::client::Client fresh(Options()); // 이전 장부를 보존한 채 다음 실제 Client의 독립 생성을 검사한다.
    CHECK(fresh.FortBadges() && fresh.FortBadges().get()!=previous.get());
    CHECK(fresh.FortBadges()->Records().data()!=previous->Records().data());
    CHECK(netstorm::o::ReadFortBadgeCount(fresh.FortBadges()->Records(),71)==0);
    CHECK(netstorm::o::ReadFortBadgeCount(previous->Records(),71)==0x12345678U);
}
