// 같은 Client 난수 소유권과 독립 Kencloud 합성을 검사하며 실제 Client/Session/OS/GUI는 초기화하지 않는다.
#include "TestSupport.h"
#include "client/ClientRandomGlobals.h"
#include "o/RawKencloud.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>

using namespace netstorm;
// 실제 PE의 최초55와 CRT1/모드0/IDinit0을 구별하고 각 getter가 같은 실제 객체를 빌리는지 확인한다.
TEST_CASE(ClientGameRandomOwner_InitialPe55AndSeparateCrt) {
    static_assert(!std::is_copy_constructible_v<client::ClientRandomGlobals> && !std::is_move_constructible_v<client::ClientRandomGlobals>); // 참조 중인 실제 주소의 이동/복제를 허용하지 않는다.
    client::ClientRandomGlobals owner;const auto& readOnly=owner; // 같은 새 Client 수명 입력이다.
    CHECK(owner.Game().State()==55&&owner.GameMode()==0&&owner.CrtState()==1&&owner.SubscriberIdInitialized()==0);
    CHECK(&owner.Game()==&readOnly.Game()&&&owner.GameMode()==&readOnly.GameMode());
    CHECK(owner.Game().Next(510)==55&&owner.Game().State()==0x003700a8U); // 004558c0의실제초기55에서한번전진한고정산술이다.
    CHECK(owner.CrtState()==1&&owner.SubscriberIdInitialized()==0&&owner.GameMode()==0);
    owner.GameMode()=0x80000002U;owner.Crt().Next(); // 게임 모드와 CRT를 바꾸어도 다른 상태를 복제/덮어쓰지 않는다.
    CHECK(readOnly.GameMode()==0x80000002U&&readOnly.Game().State()==0x003700a8U&&owner.CrtState()==2745024U);
}
// 먼저 고정한 Kencloud v2 첫 입력의 같은 난수 일곱 호출이 장기 owner 자체에 도달한다.
TEST_CASE(ClientGameRandomOwner_KencloudBorrowsOneLongLivedObject) {
    auto owner=std::make_shared<client::ClientRandomGlobals>();auto second=owner;std::weak_ptr<client::ClientRandomGlobals> lifetime=owner; // 두 사용자와 장기 원본 한개다.
    auto* same=&owner->Game();owner->Game().SetState(0); // fixture가 명시한0입력이며 실제 PE 최초55를 재정의하지 않는다.
    std::uint32_t cache=0,end=0,next=0,last=0,phase=0,all=17,part=19,spacer=29; // 명시 장치의 현재 실제 DWORD다.
    const std::uint32_t color=228,speed=0,fury=10000,count=3; // 원문 v2 첫 입력을 그대로 공급한다.
    std::array<std::uint32_t,16> rows{};std::array<std::uint32_t,512> dirty{};std::array<std::uint32_t,3> seeds{}; // warm-up을 자연 생략하는 현재 표다.
    const o::KencloudGlobals globals{cache,end,next,last,phase,all,part,spacer,color,speed,fury,count,rows,dirty,seeds}; // 난수 상태는 이 구조체에 없다.
    std::uint32_t fileCalls=0; // 정상512×512 이미지의 명시 파일 하위다.
    o::KencloudHooks hooks;hooks.loadFile=[&] { ++fileCalls;return 0x12001000U; }; // 기존 전체 파일 관측과 구별한 필수 하위 입력이다.
    o::BuildKencloud(globals,owner->Game(),hooks);
    CHECK(&second->Game()==same&&second->Game().State()==2488585150U); // 제품 작성 전 v2 첫 행의 실제 PE 최종 난수다.
    CHECK(spacer==89&&fileCalls==1&&all==1&&part==0&&owner->CrtState()==1&&owner->GameMode()==0);
    owner.reset();CHECK(!lifetime.expired()&&&second->Game()==same&&second->Game().State()==2488585150U); // 첫 사용자의 반납이 같은 객체를 초기화하지 않는다.
    second.reset();CHECK(lifetime.expired()); // 최종 반납 뒤 same을 역참조하지 않는다.
}
