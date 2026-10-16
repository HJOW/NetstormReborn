// DWORD 후보의 높은 바이트를 색 번호에 섞지 않고 원본 아홉 표를 생성한다.
#include "client/OwnerColorMaps.h"

namespace netstorm::client {
namespace {
// 보호10.78 PE의005319e8..00531b07 원문이다. 첫8개0은 색0 후보이며 변환표0 자체는 항등이다.
constexpr std::array<std::uint32_t,72> kCandidates{
    0,0,0,0,0,0,0,0,81,80,76,145,146,147,4,10,94,100,170,169,204,249,217,227,
    195,93,185,35,31,149,70,15,207,131,133,128,136,2,142,143,189,190,253,150,148,70,14,10,
    113,218,219,3,52,59,67,213,9,85,155,156,6,72,144,143,220,221,222,223,224,63,227,213};
}
// 원본 정적 DWORD 표의 읽기 전용 view다.
std::span<const std::uint32_t,72> OriginalOwnerColorCandidates() { return kCandidates; }
// 원문에서 색0 행은 변경하지 않고1..8행만 실제 띠를 바꾼다.
OwnerColorMaps BuildStandardOwnerColorMaps(std::uint32_t type,std::uint32_t bridgeType,std::span<const std::uint32_t,72> candidates) {
    OwnerColorMaps result;
    // 모든 비대상 색/색0의 원래 번호를 유지한다.
    for (auto& row:result) {
        // 원본256바이트 항등 표 하나다.
        for (std::size_t color=0;color<row.size();++color) row[color]=static_cast<std::uint8_t>(color);
    }
    // 플레이어 색 번호1..8 각각의 명도 후보를 사용한다.
    for (std::size_t owner=1;owner<result.size();++owner) {
        //228..237은 밝은0,238..245는 역순7..0이며 다리의 색2/7은 후반 명도를 고정한다.
        for (std::size_t color=228;color<=245;++color) {
            auto shade=color<238 ? std::size_t{} : 245-color;
            if (color>=238 && type==bridgeType && (owner==2 || owner==7)) shade=owner==2 ? 4 : 3;
            result[owner][color]=static_cast<std::uint8_t>(candidates[owner*8+shade]);
        }
    }
    return result;
}
}
