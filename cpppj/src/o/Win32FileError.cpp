// 원본004ed5ee의 중복 키와 표 우선순위를 보존한다.
#include "o/Win32FileError.h"
#include <array>
#include <utility>

namespace netstorm::o { // Win32 오류 번호와 원본 MSVCRT errno 번호를 직접 대응시킨다.
namespace { // 실제 PE00544080의45쌍이며 중복6을 정렬/병합하지 않는다.
constexpr std::array<std::pair<std::uint32_t,std::int32_t>,45> kErrors{{
    {1,22},{2,2},{3,2},{4,24},{5,13},{6,9},{7,12},{8,12},{9,12},{10,7},{11,8},{12,22},{13,22},
    {15,2},{16,13},{17,18},{18,2},{33,13},{53,2},{65,13},{67,2},{80,17},{82,13},{83,13},{87,22},
    {89,11},{108,13},{109,32},{112,28},{114,9},{6,22},{128,10},{129,10},{130,9},{131,22},{132,13},
    {145,41},{158,13},{161,2},{164,11},{167,13},{183,17},{206,2},{215,11},{1816,12}
}}; // 각 항목은 원문 unsigned Win32 번호와 signed errno DWORD다.
}
// table에서 첫 키를 찾은 뒤에만 원문의 포함 구간13..24/BC..CA(16진수)를 적용한다.
std::int32_t Win32FileErrorCode(std::uint32_t error) {
    for (const auto& [code,value]:kErrors) { if (error==code) return value; } // 실제45개 원문 순서다.
    if (error>=0x13 && error<=0x24) return 13;
    if (error>=0xbc && error<=0xca) return 8;
    return 22;
}
}
