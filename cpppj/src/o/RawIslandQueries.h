// 실제 mutable 섬 목록에서 현재owner/원래owner/territory를 읽는 세 검색 몸체다.
#pragma once
#include "o/RawIslandRuntime.h"

namespace netstorm::o {
// 패치0046f380/0046f3f0/0046f430와 CD004bdaa0/004bdb10/004bdb60의 읽기 전용 검색이다.
class RawIslandQueries {
public:
    // 원본 두 검색 함수가 아무 섬도 찾지 못했을 때 반환하는 고정번호다.
    static constexpr std::uint32_t kNotFound=127;
    // 같은 CoreData의3204BYTE 기록과 현재fort모드를 참조하며 별도 섬 벡터를 만들지 않는다.
    RawIslandQueries(const RawIslandRuntime& islands,const FortFinishOwnerState& modes);
    // 일반모드는 signed원래ownerBYTE와 현재ownerDWORD를 모두 비교한다. fort모드는 현재owner0도 허용한다.
    std::uint32_t FindOwnerTerritory(std::int32_t owner,std::int32_t territory) const;
    // 존재하는 기록 중 signedterritoryBYTE가 같은 첫 번호를 반환한다.
    std::uint32_t FindTerritory(std::int32_t territory) const;
    // 존재하는 기록 중 현재ownerDWORD가 같은 개수를 반환한다. owner0도 비교 대상이다.
    std::uint32_t CountOwned(std::uint32_t owner) const;
private:
    // 실제 기록의BYTE를 MSVC signed char처럼32비트 부호확장해 읽는다.
    std::int32_t SignedField(std::int32_t island,std::size_t field) const;
    // islands_: 실제 currentCount/Exists/현재owner와3204BYTE를 가진 같은 월드의 목록이다.
    const RawIslandRuntime& islands_;
    // modes_: owner/territory 검색의 현재fort 플래그다. special/chaff는 이 세 원본 몸체에서 사용하지 않는다.
    const FortFinishOwnerState& modes_;
};
}
