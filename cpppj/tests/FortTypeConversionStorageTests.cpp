// 실제 변환표 소유자의 현재 DWORD, 물리255BYTE 할당과 공유 수명을 검사한다.
#include "TestSupport.h"
#include "client/FortTypeConversionStorage.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>

using netstorm::client::FortTypeConversionStorage;
namespace {
// 원본004bd800의 operator new 요청 폭이며 제품 상수를 기대값으로 다시 읽지 않는다.
constexpr std::uint32_t kExpectedConversionBytes=255;

// 의도한 예외 형식만 성공으로 인정하여 다른 실패를 주소/크기 거부로 오인하지 않는다.
template<class Exception,class Operation>
bool ConversionRejects(Operation operation) {
    try { operation(); }
    catch (const Exception&) { return true; }
    catch (...) { return false; }
    return false;
}

// 전체 물리 블록을 먼저 명시적으로 작성하므로 미초기화 메모리는 한 BYTE도 읽지 않는다.
std::span<std::uint8_t> InitializeConversion(FortTypeConversionStorage& owner,std::uint32_t token,std::uint8_t value) {
    auto bytes=owner.Bytes(token); // 반복 조회와 비교할 같은 실제 할당을 빌린다.
    if (!bytes.data() || bytes.size()!=kExpectedConversionBytes) throw std::logic_error("변환표 검사에 필요한 실제255BYTE 범위");
    std::fill(bytes.begin(),bytes.end(),value);
    return bytes;
}
}

// 읽기/쓰기 참조는 같은 실제 DWORD이며 독립 소유자의 current에는 영향을 주지 않는다.
TEST_CASE(FortTypeConversionStorage_CurrentReferencesAndIndependentOwners) {
    static_assert(!std::is_copy_constructible_v<FortTypeConversionStorage> && !std::is_copy_assignable_v<FortTypeConversionStorage>); // 이미 빌린 DWORD/버퍼의 복제를 막는다.
    static_assert(!std::is_move_constructible_v<FortTypeConversionStorage> && !std::is_move_assignable_v<FortTypeConversionStorage>); // 참조 주소를 이동으로 끊지 않는다.
    FortTypeConversionStorage first,second; // 서로 다른 Client에 해당하는 독립 소유자다.
    auto& current=first.Current(); // 부모가 실제 대입할 현재 DWORD 참조다.
    const auto& constFirst=first; // 읽기 소비자도 같은 DWORD 주소를 빌린다.
    CHECK(current==0 && second.Current()==0);
    CHECK(&current==&first.Current() && &current==&constFirst.Current() && &current!=&second.Current());
    const auto token=first.Allocate(kExpectedConversionBytes); // 할당만으로는 current가 갱신되지 않아야 한다.
    CHECK(token!=0 && current==0 && constFirst.Current()==0);
    current=token;second.Current()=0x87654321U;
    CHECK(first.Current()==token && constFirst.Current()==token && second.Current()==0x87654321U);
    current=0; // 빈 TypeNames처럼 현재 포인터만 잃어도 소유한 옛 할당은 남아야 한다.
    CHECK(first.Bytes(token).size()==kExpectedConversionBytes && second.Current()==0x87654321U);
}

// 두 살아 있는 할당은 독립이고 반복 Bytes는 복사본이 아닌 같은 물리 메모리를 반환한다.
TEST_CASE(FortTypeConversionStorage_ExactAllocationAndPhysicalAliases) {
    FortTypeConversionStorage owner;owner.Current()=0xfedcba98U; // 임의 현재값을 할당이 덮어쓰는지 드러내는 표식이다.
    const auto first=owner.Allocate(kExpectedConversionBytes),second=owner.Allocate(kExpectedConversionBytes); // 동시에 살아 있는 두 할당을 만든다.
    CHECK(first!=0 && second!=0 && first!=second && owner.Current()==0xfedcba98U);
    auto firstBytes=InitializeConversion(owner,first,0x31); // 초기값을 읽기 전에 첫255BYTE 전체를 작성한다.
    auto secondBytes=InitializeConversion(owner,second,0x72); // 두번째255BYTE도 별도로 작성한다.
    CHECK(firstBytes.data()!=secondBytes.data() && owner.Bytes(first).data()==firstBytes.data());
    CHECK(owner.Bytes(second).data()==secondBytes.data() && firstBytes.size()==255 && secondBytes.size()==255);
    firstBytes.front()=0x19;owner.Bytes(first).back()=0xe7;
    CHECK(owner.Bytes(first).front()==0x19 && firstBytes.back()==0xe7);
    CHECK(std::all_of(secondBytes.begin(),secondBytes.end(),[](std::uint8_t byte) { return byte==0x72; })); // 다른 살아 있는 블록에는 쓰기가 전달되지 않는다.
    secondBytes[127]=0xa5;
    CHECK(owner.Bytes(second)[127]==0xa5 && firstBytes[127]==0x31 && owner.Current()==0xfedcba98U);
}

// 0/없는 주소와 이미 해제한 주소는 실제 배열처럼 읽거나 다시 반납할 수 없다.
TEST_CASE(FortTypeConversionStorage_UnknownAndFreedTokensRejected) {
    FortTypeConversionStorage owner; // 아무 할당도 없는 최초 소유자다.
    constexpr auto unknown=std::numeric_limits<std::uint32_t>::max(); // 소수의 성공 할당과 겹치지 않는 미발급 토큰이다.
    CHECK(ConversionRejects<std::out_of_range>([&] { static_cast<void>(owner.Bytes(0)); }));
    CHECK(ConversionRejects<std::out_of_range>([&] { static_cast<void>(owner.Bytes(unknown)); }));
    CHECK(ConversionRejects<std::out_of_range>([&] { owner.Release(unknown); }));
    const auto token=owner.Allocate(kExpectedConversionBytes); // 진짜 할당을 반납한 뒤와 미발급 주소를 구별해 검증한다.
    InitializeConversion(owner,token,0x4c);
    owner.Release(token);
    CHECK(ConversionRejects<std::out_of_range>([&] { static_cast<void>(owner.Bytes(token)); }));
    CHECK(ConversionRejects<std::out_of_range>([&] { owner.Release(token); }));
    CHECK(owner.Current()==0);
}

// free(nullptr), 비현재 해제와 현재 해제 모두 부모의 별도 current 쓰기를 대신하지 않는다.
TEST_CASE(FortTypeConversionStorage_ReleasePreservesCurrentAndOtherBlocks) {
    FortTypeConversionStorage owner;owner.Release(0); // 최초 free(nullptr)는 허용된다.
    const auto previous=owner.Allocate(kExpectedConversionBytes),current=owner.Allocate(kExpectedConversionBytes); // 현재와 이전 할당의 수명을 독립시킨다.
    InitializeConversion(owner,previous,0x18);
    auto live=InitializeConversion(owner,current,0x59); // 비현재 할당 반납 뒤에도 읽을 명시 초기화 블록이다.
    owner.Current()=current;owner.Release(0);
    CHECK(owner.Current()==current && owner.Bytes(current).data()==live.data());
    owner.Release(previous);
    CHECK(owner.Current()==current && owner.Bytes(current).front()==0x59 && owner.Bytes(current).back()==0x59);
    CHECK(ConversionRejects<std::out_of_range>([&] { static_cast<void>(owner.Bytes(previous)); }));
    owner.Release(current);
    CHECK(owner.Current()==current); // 해제 뒤에도 원문 부모의 무조건0쓰기 전까지 옛 DWORD가 남는다.
    CHECK(ConversionRejects<std::out_of_range>([&] { static_cast<void>(owner.Bytes(current)); }));
    owner.Release(0);CHECK(owner.Current()==current);
    owner.Current()=0;CHECK(owner.Current()==0); // 부모가 수행하는 명시0쓰기만 현재값을 바꾼다.
}

// 여러 Session과 같은 소비자들이 한 소유자를 빌리면 마지막 소비자가 놓을 때까지 할당이 유지된다.
TEST_CASE(FortTypeConversionStorage_SharedConsumersKeepOwnerAlive) {
    auto clientOwner=std::make_shared<FortTypeConversionStorage>(); // Client가 생성한 한 실제 소유자에 해당한다.
    std::weak_ptr<FortTypeConversionStorage> lifetime=clientOwner; // 수명만 관측하며 새 소유권은 추가하지 않는다.
    auto firstSession=clientOwner,secondSession=clientOwner; // Session 객체 자체를 실행하지 않는 두 공유 소비자다.
    const auto token=clientOwner->Allocate(kExpectedConversionBytes); // 두 소비자가 같은 주소로 조회할 유일한 할당이다.
    auto bytes=InitializeConversion(*clientOwner,token,0x8b); // 살아 있는 동안 읽을255BYTE 전체를 작성한다.
    auto* const currentAddress=&clientOwner->Current(); // 소유자가 살아 있는 구간에서만 주소를 비교한다.
    auto* const bufferAddress=bytes.data(); // 같은 실제 버퍼인지 비교할 주소이며 해제 뒤에는 사용하지 않는다.
    clientOwner.reset();
    CHECK(!lifetime.expired() && &firstSession->Current()==currentAddress && &secondSession->Current()==currentAddress);
    firstSession->Current()=token;firstSession->Bytes(token)[254]=0xc6;
    CHECK(secondSession->Current()==token && secondSession->Bytes(token).data()==bufferAddress && bytes.back()==0xc6);
    firstSession.reset();
    CHECK(!lifetime.expired() && secondSession->Current()==token && secondSession->Bytes(token).front()==0x8b && secondSession->Bytes(token).back()==0xc6);
    secondSession.reset();
    CHECK(lifetime.expired()); // 이 지점부터 DWORD/버퍼 주소와 span은 역참조하지 않는다.
}

// 잘못된 크기는 명시 진단하고 이미 있는 current/할당을 바꾸거나 다음 성공 할당을 막지 않는다.
TEST_CASE(FortTypeConversionStorage_WrongSizesPreserveLiveState) {
    FortTypeConversionStorage owner; // 실패 전후 비교할 하나의 실제 소유자다.
    const auto existing=owner.Allocate(kExpectedConversionBytes); // 잘못된 요청과 무관하게 살아 있어야 하는 토큰이다.
    auto bytes=InitializeConversion(owner,existing,0xd3); // 진단 뒤에도 보존할 실제 데이터다.
    owner.Current()=existing;
    constexpr std::array<std::uint32_t,5> invalidSizes{0,1,254,256,std::numeric_limits<std::uint32_t>::max()}; // 빈 요청과 양쪽 경계 및 과대 요청을 검사한다.
    for (const auto size:invalidSizes) { // 각 실패가 같은 살아 있는 할당에 부작용을 남기는지 확인한다.
        CHECK(ConversionRejects<std::invalid_argument>([&] { static_cast<void>(owner.Allocate(size)); }));
        CHECK(owner.Current()==existing && owner.Bytes(existing).data()==bytes.data());
        CHECK(std::all_of(bytes.begin(),bytes.end(),[](std::uint8_t byte) { return byte==0xd3; })); // 이 블록은 검사에서 전체 초기화한 데이터만 읽는다.
    }
    const auto next=owner.Allocate(kExpectedConversionBytes); // 진단 뒤 정상 요청도 current 자동 대입 없이 성공해야 한다.
    CHECK(next!=0 && next!=existing && owner.Current()==existing && owner.Bytes(next).size()==255);
    CHECK(owner.Bytes(existing).front()==0xd3 && owner.Bytes(existing).back()==0xd3);
}
