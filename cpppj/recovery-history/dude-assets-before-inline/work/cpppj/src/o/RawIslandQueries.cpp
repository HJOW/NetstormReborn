// 기존 섬 목록 getter를 재사용하며 검색마다 같은 raw기록을 읽고 원본 첫 일치 순서를 보존한다.
#include "o/RawIslandQueries.h"
#include <bit>

namespace netstorm::o {
namespace {
// 현재countDWORD의 signed비트 해석과100기록의 원본stride다.
constexpr std::size_t kIslandStride=32;
// 실제32바이트 기록 안의territoryBYTE와 원래ownerBYTE 위치다.
constexpr std::size_t kTerritory=12;
constexpr std::size_t kOriginalOwner=13;
// 원본의 signedcount 비교를 호스트의구현정의 unsigned변환 없이 수행한다.
std::int32_t SignedCount(std::uint32_t count) { return std::bit_cast<std::int32_t>(count); }
}
// 목록과모드의 장기소유자 수명은 호출하는 월드가 유지한다.
RawIslandQueries::RawIslandQueries(const RawIslandRuntime& islands,const FortFinishOwnerState& modes)
    :islands_(islands),modes_(modes) {}
// Exists로확인한 물리기록을 같은 span에서 직접읽는다. 별도복사나ownerBYTE/DWORD 병합은하지않는다.
std::int32_t RawIslandQueries::SignedField(std::int32_t island,std::size_t field) const {
    const auto value=islands_.Bytes()[4+static_cast<std::size_t>(island)*kIslandStride+field];
    return static_cast<std::int32_t>(std::bit_cast<std::int8_t>(value));
}
// 일반모드는 원래owner가같아도 현재owner가달라졌으면 통과하지않는다.
std::uint32_t RawIslandQueries::FindOwnerTerritory(std::int32_t owner,std::int32_t territory) const {
    const bool patch=islands_.Pool().Edition()==OriginalEdition::Patch1078;
    const bool initialFort=modes_.fort;const auto initialCount=SignedCount(islands_.Count());
    // 실제번호0부터순서대로 존재/두owner/지역을 확인해 최초일치를 반환한다.
    for (std::int32_t island=0;island<(patch ? initialCount : SignedCount(islands_.Count()));++island) {
        if (!islands_.Exists(island)) continue;
        const auto current=islands_.Owner(island);const bool fort=patch ? initialFort : modes_.fort;
        if (!fort) {
            if (SignedField(island,kOriginalOwner)!=owner || current!=std::bit_cast<std::uint32_t>(owner)) continue;
        } else if (current && current!=std::bit_cast<std::uint32_t>(owner)) continue;
        if (SignedField(island,kTerritory)==territory) return static_cast<std::uint32_t>(island);
    }
    return kNotFound;
}
// 이검색은fort/special/chaff 및소유자값을읽지않는다.
std::uint32_t RawIslandQueries::FindTerritory(std::int32_t territory) const {
    const bool patch=islands_.Pool().Edition()==OriginalEdition::Patch1078;const auto initialCount=SignedCount(islands_.Count());
    // 존재DWORD가0이아닌첫기록의 signed지역BYTE를비교한다.
    for (std::int32_t island=0;island<(patch ? initialCount : SignedCount(islands_.Count()));++island) {
        if (islands_.Exists(island) && SignedField(island,kTerritory)==territory) return static_cast<std::uint32_t>(island);
    }
    return kNotFound;
}
// 이미복원된Exists/Owner를같은raw목록에호출하며0소유섬도계수한다.
std::uint32_t RawIslandQueries::CountOwned(std::uint32_t owner) const {
    std::uint32_t count=0;
    // count가삭제뒤줄지않는원본정책을유지하며 빈기록을건너뛴다.
    for (std::int32_t island=0;island<SignedCount(islands_.Count());++island) {
        if (islands_.Exists(island) && islands_.Owner(island)==owner) ++count;
    }
    return count;
}
}
