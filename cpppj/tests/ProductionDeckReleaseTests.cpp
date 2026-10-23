// 실제 Deck 단일 소유자의 개별 반납과 원문 Player 정리 부모의 순서를 검사한다.
#include "TestSupport.h"
#include "o/RawProductionDeck.h"
#include "o/RawPlayerLinksCleanup.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace netstorm::o;
// 이번 검사에서 선택하지 않는 실제 Player 생성/공간 자식은 성공값으로 대체하지 않는다.
[[noreturn]] void UnselectedDeckPlayerEffect() { throw std::logic_error("Deck반납검사의미선택Player효과"); }
// 실제 RawPlayerWorld 생성 계약은 충족하되 이 경계가 선택되면 명시 중단한다.
PlayerWorldHooks DeckReleasePlayerHooks() {
    PlayerWorldHooks hooks; // 같은 실제 Player 객체의 필수 생성 경계다.
    hooks.assertion=[](PlayerWorldAssertion) { UnselectedDeckPlayerEffect(); };
    hooks.find=[](float,float,std::uint32_t)->Sid { UnselectedDeckPlayerEffect(); };
    hooks.create=[](std::uint32_t,std::uint32_t)->Sid { UnselectedDeckPlayerEffect(); };
    hooks.setOwner=[](Sid,std::uint32_t) { UnselectedDeckPlayerEffect(); };
    hooks.pop=[](Sid,float,float,std::uint32_t) { UnselectedDeckPlayerEffect(); };
    hooks.reserveRegular=[]()->bool { UnselectedDeckPlayerEffect(); };
    hooks.addRegular=[](Sid,std::uint32_t,float) { UnselectedDeckPlayerEffect(); };
    return hooks;
}
// 비정렬 Player DWORD를 직접 읽고 인접 필드는 그대로 둔다.
std::uint32_t DeckReleaseRead(std::span<const std::uint8_t> records,std::size_t at) {
    std::uint32_t value{}; // 원래 필드폭의 조립 결과다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(records[at+byte])<<(byte*8); // 낮은 BYTE부터 읽는다.
    return value;
}
// 테스트가 명시한 실제 Player 입력만 변경한다.
void DeckReleasePut(std::span<std::uint8_t> records,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) records[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 정확한 네 BYTE만 쓴다.
}
// 원본 범위 밖의 미등록/중복 토큰 오류를 확인한다.
template<class Function> bool DeckReleaseRejected(Function&& function) {
    try { function(); }catch (const std::out_of_range&) { return true; }
    return false;
}
// 실제 SidPool/Player/Deck 단일 소유자를 구성하며 객체 생성·공간·생산 몸체는 선택하지 않는다.
struct DeckReleaseScene {
    SidPool pool{OriginalEdition::Patch1078,24000}; // 실제 PATCH 풀의 최소 유효 물리다.
    std::array<RiftTypeRecord,188> types{}; // 이 반납 경로는 TYPE 항목을 읽지 않는다.
    SquidPostPopState postState; // 실제 공통 Post 객체의 미선택 장부다.
    SquidPostPop post{pool,types,postState}; // 실제 PlayerWorld가 요구하는 동일 풀의 객체다.
    std::array<std::uint8_t,9*180> records{}; // 같은 Player+68을 Deck와 부모가 함께 본다.
    PlayerWorldState state; // 원본 두 owner와 현재 모드를 가진 실제 객체다.
    RawPlayerWorld player{pool,post,records,state,DeckReleasePlayerHooks()}; // 가짜 Player 또는 두 번째 저장소를 만들지 않는다.
    std::uint32_t fills{}; // 실제 새 블록의 명시 바이트 공급 호출 수다.
    std::unique_ptr<RawProductionDeck> deck; // Player보다 먼저 반납되는 유일한 Deck owner다.
    // 각 새352BYTE 블록을 다른 입력으로 채워 다른 할당 보존을 확인한다.
    DeckReleaseScene() {
        deck=std::make_unique<RawProductionDeck>(player,[this](std::span<std::uint8_t> bytes) { ++fills;std::fill(bytes.begin(),bytes.end(),static_cast<std::uint8_t>(0x40U+fills)); });
    }
};
}

// 한 블록만 실제 map에서 제거하고 Player/다른 블록/할당 순서는 보존한다.
TEST_CASE(ProductionDeckRelease_OnlySelectedAllocationAndNoPlayerWrites) {
    DeckReleaseScene scene; // 외부 수명과 동일한 실제 소유자다.
    const auto first=scene.deck->Get(1),other=scene.deck->Get(8); // 같은 owner의 서로 다른 두 할당이다.
    const auto records=scene.records; // 개별 free가 쓰면 안 되는 실제 Player 전체다.
    const auto otherBytes=std::vector<std::uint8_t>(scene.deck->Bytes(other).begin(),scene.deck->Bytes(other).end()); // 살아 있어야 할 두 번째 블록이다.
    scene.deck->Release(first);
    CHECK(scene.records==records && scene.fills==2);
    CHECK(DeckReleaseRejected([&] { static_cast<void>(scene.deck->Bytes(first)); }));
    CHECK(std::equal(otherBytes.begin(),otherBytes.end(),scene.deck->Bytes(other).begin()));
    CHECK(DeckReleaseRejected([&] { static_cast<void>(scene.deck->Get(1)); })); // 부모의0쓰기 전 stale Player 포인터를 새 할당으로 숨기지 않는다.
    DeckReleasePut(scene.records,180+0x68,0); // 실제 부모의 후속0쓰기를 별도 명시한다.
    const auto replacement=scene.deck->Get(1); // 다음 토큰을 되감거나 해제 토큰을 다른 블록에 가장하지 않는다.
    CHECK(replacement==other+kProductionDeckBytes && replacement!=first && scene.fills==3);
    CHECK(scene.deck->Bytes(replacement)[0]==0x43 && scene.deck->Bytes(other)[0]==0x42);
}

// Native162로 고정한 부모가 실제 같은 할당 owner를 호출한 뒤 현재 Player 슬롯을0으로 만든다.
TEST_CASE(ProductionDeckRelease_RealPlayerLinksParentReleasesSameOwner) {
    DeckReleaseScene scene; // 새 map이나 대체 free registry를 만들지 않는다.
    std::array<std::uint32_t,9> tokens{}; // 실제 Get으로 얻은 각 Player의 할당 토큰이다.
    for (std::uint32_t owner=0;owner<9;++owner) { // 모든 실제 Player 기록에 서로 다른 블록을 부착한다.
        tokens[owner]=scene.deck->Get(owner);DeckReleasePut(scene.records,owner*180+0x9c,owner+20);DeckReleasePut(scene.records,owner*180+8,owner+30);
    }
    std::size_t calls{}; // 원문에 선택된 실제 free 호출 수다.
    PlayerLinksCleanupHooks hooks; // 같은 현재 Player와 같은 Deck owner만 빌린다.
    hooks.players=[&] { return std::span(scene.records); };
    hooks.release=[&](std::uint32_t token) {
        CHECK(calls<tokens.size() && token==tokens[calls]);
        const auto offset=calls*180; // 원문 루프의 해당 Player 위치다.
        CHECK(DeckReleaseRead(scene.records,offset+0x9c)==0 && DeckReleaseRead(scene.records,offset+8)==0);
        CHECK(DeckReleaseRead(scene.records,offset+0x68)==token); // free 전후 포인터0쓰기는 이 API의 효과가 아니다.
        const auto before=scene.records;scene.deck->Release(token);CHECK(scene.records==before);
        CHECK(DeckReleaseRejected([&] { static_cast<void>(scene.deck->Bytes(token)); }));++calls;
    };
    CleanupPlayerLinks(hooks);
    CHECK(calls==9 && scene.fills==9);
    for (std::uint32_t owner=0;owner<9;++owner) CHECK(DeckReleaseRead(scene.records,owner*180+0x68)==0); // 실제 부모가 후속0쓰기를 끝냈다.
    CleanupPlayerLinks({hooks.players,{}}); // 모두0이면 실제 free 장치를 선택하지 않는다.
    CHECK(calls==9 && scene.fills==9);
}

// CRT NULL 무효과와 호스트의 외부/중복 토큰 계약, 소멸자의 별도 Player 정리를 구별한다.
TEST_CASE(ProductionDeckRelease_NullUnknownDuplicateAndDestructorLifetime) {
    DeckReleaseScene scene; // Player가 Deck 소멸자보다 오래 살아 있는 실제 순서다.
    const auto first=scene.deck->Get(1),other=scene.deck->Get(8); // 서로 다른 실제 할당이다.
    const auto before=scene.records; // 실패/NULL에서 보존할 입력이다.
    scene.deck->Release(0);
    CHECK(DeckReleaseRejected([&] { scene.deck->Release(0xffffffffU); }) && scene.records==before);
    scene.deck->Release(first);
    CHECK(DeckReleaseRejected([&] { scene.deck->Release(first); }) && scene.records==before && scene.fills==2);
    CHECK(scene.deck->Bytes(other).size()==kProductionDeckBytes);
    scene.deck.reset(); // 살아 있는 map 항목에 매인 현재 Player 포인터만 기존 소멸자가0으로 만든다.
    CHECK(DeckReleaseRead(scene.records,180+0x68)==first && DeckReleaseRead(scene.records,8*180+0x68)==0);
    // 해제된 토큰은 부모가0으로 만들어야 하며 소멸자로 그 순서를 대신하지 않는다.
}
