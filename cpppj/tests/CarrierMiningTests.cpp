// 전체 원본 도착의 명시 채취 계약에서 관측한 통지 조각864행을 독립 재생한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierMining.h"
using namespace netstorm::test::rawscene;
namespace {
// 원본의 헤더/지면 WORD/송신 효과 입력을 읽는다.
std::vector<std::uint32_t> MiningNumbers(std::string_view text) {
    std::vector<std::uint32_t> result;
    // 모든 DWORD 입력을 현재 원문 순서로 읽는다.
    for (const auto& part:Split(text,',')) result.push_back(Number(part));return result;
}
}
// 헤더 길이 비보정/bit7 제거/송신 전후 동일 버퍼/현재 지면 상태를 두 정밀도의 원문과 대조한다.
TEST_CASE(CarrierMining_NativePacketAndSendBeforeReceive) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::size_t count{}; // 독립 원문의 실제 대조 수다.
    // 각 행의 값은 관측기에 제공한 입력이며 효과나 예상 패킷을 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_CARRIER_MINING_FIXTURE).rows) {
        CHECK(row.size()==7);if (row.size()!=7) continue;const auto c=MiningNumbers(row[0]);CHECK(c.size()==5);if (c.size()!=5) continue;
        auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size());raw[c[3]*50+11]=0;
        FormPacketHeader header{static_cast<std::uint8_t>(c[1]),static_cast<std::uint8_t>(c[0]),static_cast<std::uint16_t>(c[2])};std::string events,packet;CarrierMiningHooks hooks;
        hooks.broadcast=[&](std::span<const std::uint8_t> bytes) {
            events="S:"+Hex(bytes);CHECK(bytes.size()==6);
            if (c[4]==1) header={0x23,0x81,7};
            if (c[4]==2) { auto writable=std::span<std::uint8_t>(const_cast<std::uint8_t*>(bytes.data()),bytes.size());Put(writable,4,61,2); }
            if (c[4]==3) raw[c[3]*50+11]=1;
        };
        hooks.receive=[&](std::span<const std::uint8_t> bytes) { packet=Hex(bytes);events+=";L:"+packet; };
        RawCarrierMining mining(pool,header,std::move(hooks));CHECK(&mining.Pool()==&pool);mining.Notify(Sid{static_cast<std::uint16_t>(c[3])});
        const auto current=std::to_string(header.protocol)+','+std::to_string(header.message)+','+std::to_string(header.length)+','+std::to_string(raw[c[3]*50+11]);
        CHECK(events==row[1] && packet==row[2] && current==row[3]);CHECK(std::equal(row.begin()+1,row.begin()+4,row.begin()+4));++count;
    }
    CHECK(count==864);
}
// 송신 뒤의 미연결 수신 경계와 지면 SID의 전체 WORD 폭을 확인한다.
TEST_CASE(CarrierMining_LateBoundariesAndEdition) {
    SidPool pool(OriginalEdition::Patch1078,24000);FormPacketHeader header{0x5b,1,6};std::size_t sends{};CarrierMiningHooks hooks;
    hooks.broadcast=[&](std::span<const std::uint8_t> bytes) { ++sends;CHECK(bytes[4]==0xff && bytes[5]==0xff); };
    RawCarrierMining mining(pool,header,hooks);CHECK(Throws([&] { mining.Notify(Sid{0xffff}); }));CHECK(sends==1);
    CHECK(Throws([&] { SidPool cd(OriginalEdition::Cd1072,24000);RawCarrierMining bad(cd,header,{}); }));
}
