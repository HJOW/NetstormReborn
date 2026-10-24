// 실제 할당 소유권·현재 Player factory 헤더·고정뷰의 세션 수명을 함께 검사한다.
#include "TestSupport.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientMain.h"
#include "o/RawPlayerParticipation.h"
#include <algorithm>
#include <stdexcept>

namespace {
using namespace netstorm::client;
using namespace netstorm::o;
// 기대하는 보호 경계의 실제 예외만 확인한다.
template<class Function> bool PlayerOwnerThrows(Function function) { try { function();return false; }catch (const std::exception&) { return true; } }
// raw 헤더의4BYTE를 fixture 입력/결과 확인에만 쓴다.
void OwnerPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 정확한 DWORD 폭이다.
}
std::uint32_t OwnerRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; // little endian 결과다.
}
// 원문236BYTE의 동맹 자식은 합성 행 쓰기 대신 기존 실제00490ea0 몸체로 실행한다.
void OwnerInitialize(ClientPlayerRecords& records,ClientPlayerRecords::Use& use) {
    PlayerParticipationState state;std::array<std::uint8_t,9> hidden{}; // 최초 known0의 실제 입력이다.
    use.Initialize([&](auto owner,auto mask) {
        RawPlayerParticipation initial(OriginalEdition::Patch1078,records.Records(),records.Alliances(),hidden,state,{});initial.SetAlliance(owner,mask);
    });
}
}

// 실제 +0/+4/+8과 factory 배열이 추가/중복 삭제/포인터 변경에 같은 값을 읽고 쓰는지 검사한다.
TEST_CASE(ClientPlayerRecords_CurrentFactoryHeadersAndSingleStorage) {
    std::uint32_t allocations{};ClientPlayerRecords records([&](auto bytes) { CHECK(bytes.size()==120);++allocations;std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x79}); }); // 명시 CRT 할당 입력이다.
    CHECK(records.Base()==0 && records.Records().empty());auto use=records.Acquire();OwnerInitialize(records,*use);CHECK(allocations==9);
    SquidPostPopState books;auto bytes=records.Records(); // 같은 실제 Player 기록과 외부 목록 뷰다.
    for (std::uint32_t owner=0;owner<9;++owner) { // 모든 원문 행의 초기화와 비초기화 factory 입력을 확인한다.
        use->BindFactory(books.ownerFactories[owner],owner);const auto& list=books.ownerFactories[owner];
        CHECK(OwnerRead(bytes,owner*180)!=0 && OwnerRead(bytes,owner*180+4)==30 && OwnerRead(bytes,owner*180+8)==0);
        CHECK(OwnerRead(bytes,owner*180+0x78)==127 && OwnerRead(bytes,owner*180+0x54)==(1U<<owner));
        CHECK(list.entries.empty() && list.count==0 && list.Capacity()==30 && list.Count()==0);
        CHECK(std::all_of(list.Storage().begin(),list.Storage().end(),[](auto value) { return value==0x79797979; }));
        for (std::uint32_t target=0;target<9;++target) CHECK(records.Alliances()[owner*9+target]==(owner==target ? 1U : 0U)); // 방향표의 실제 자식 결과다.
    }
    auto& list=books.ownerFactories[1];list.Append(51);list.Append(52);list.Append(51);list.Append(53);list.RemoveAll(51);
    CHECK(OwnerRead(bytes,180+8)==2 && list.Items()[0]==52 && list.Items()[1]==53 && list.Storage()[2]==51 && list.Storage()[3]==53);
    const auto original=OwnerRead(bytes,180);const auto other=OwnerRead(bytes,360); // 현재 포인터 변경으로 두 목록이 같은 배열을 읽는지 검사한다.
    OwnerPut(bytes,180,other);OwnerPut(bytes,180+4,2);OwnerPut(bytes,180+8,1);list.Append(54);list.Append(55);
    CHECK(list.Count()==2 && OwnerRead(bytes,180+8)==2 && list.Storage().data()==books.ownerFactories[2].Storage().data());
    CHECK(books.ownerFactories[2].Storage()[1]==54 && books.ownerFactories[2].Count()==0);
    OwnerPut(bytes,180,original);OwnerPut(bytes,180+4,30);OwnerPut(bytes,180+8,2); // 원문 다음 free가 같은 할당을 두 번 지우지 않도록 입력 포인터를 복원한다.
    OwnerPut(bytes,180+4,0);OwnerPut(bytes,180,0);list.Append(99);CHECK(list.Count()==2); // full이면 NULL 배열을 읽지 않는다.
    CHECK(PlayerOwnerThrows([&] { static_cast<void>(list.Items()); }));
    OwnerPut(bytes,180,original);OwnerPut(bytes,180+4,30);
}

// 살아 있는 대여의 재할당은 막고 이전 대여 반납 뒤에는 현재 목록을 새 Player로 재조회한다.
TEST_CASE(ClientPlayerRecords_LeaseAndReallocationRebindCurrentViews) {
    ClientPlayerRecords records([](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x43}); });auto first=records.Acquire();OwnerInitialize(records,*first);
    SquidPostPopList list;first->BindFactory(list,3);list.Append(88);const auto previous=records.Base(); // 실제 이전 행과 목록의 관측값이다.
    records.FortCount()=7;records.LastLoginPlayer()=8;
    CHECK(PlayerOwnerThrows([&] { static_cast<void>(records.Acquire()); }));
    CHECK(PlayerOwnerThrows([&] { OwnerInitialize(records,*first); }));CHECK(records.Base()==previous && list.Items()[0]==88 && records.FortCount()==7);
    first.reset();auto second=records.Acquire();OwnerInitialize(records,*second);
    CHECK(records.Base()!=previous && records.FortCount()==0 && records.LastLoginPlayer()==0);
    CHECK(list.Count()==0 && list.Capacity()==30 && list.Items().empty() && list.Storage()[0]==0x43434343); // 보관한 목록은 새 현재base를 재조회하며 옛 span을 읽지 않는다.
    CHECK(PlayerOwnerThrows([&] { second->BindFactory(list,3); }));
    SquidPostPopList populated{{1},1};CHECK(PlayerOwnerThrows([&] { second->BindFactory(populated,2); }));
}

// 두 번째 factory 할당 실패가 원문의 앞선 행과 동맹 효과를 되돌리지 않는지 검사한다.
TEST_CASE(ClientPlayerRecords_InitializationFailureKeepsReachedPrefix) {
    std::uint32_t allocations{};ClientPlayerRecords records([&](auto bytes) { // 두 번째 new에서 실제 외부 할당 실패를 발생시킨다.
        if (++allocations==2) throw std::bad_alloc();std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x29});
    });
    auto use=records.Acquire();CHECK(PlayerOwnerThrows([&] { OwnerInitialize(records,*use); }));const auto raw=records.Records();
    CHECK(raw.size()==1620 && records.Base()!=0 && OwnerRead(raw,0)!=0 && OwnerRead(raw,4)==30 && OwnerRead(raw,0x78)==127);
    CHECK(OwnerRead(raw,180)==0 && OwnerRead(raw,180+4)==0 && OwnerRead(raw,180+0x78)==0 && allocations==2);
    CHECK(records.Alliances()[0]==1 && records.Alliances()[10]==0);CHECK(PlayerOwnerThrows([&] { OwnerInitialize(records,*use); }));
    SquidPostPopList list;CHECK(PlayerOwnerThrows([&] { use->BindFactory(list,0); }));
}

// 창 없는 실제 Client의 getter는 같은 owner를 반환하며 조회만으로 Player를 할당하지 않는다.
TEST_CASE(ClientPlayerRecords_ActualClientOwnerStartsWithoutAllocation) {
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";Client client(options);const auto owner=client.PlayerRecords(); // 창/게임 실행 없이 실제 Client 생성자를 확인한다.
    CHECK(owner && owner==client.PlayerRecords() && owner->Base()==0 && owner->Records().empty());
    auto use=owner->Acquire();OwnerInitialize(*owner,*use);CHECK(client.PlayerRecords()->Records().data()==owner->Records().data());
}
