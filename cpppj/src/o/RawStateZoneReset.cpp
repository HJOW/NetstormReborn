// 원문9BYTE의 실제 store 두 개를 같은 물리에 적용하고 인접WORD는 보존한다.
#include "o/RawStateZoneReset.h"
#include <cstring>
#include <stdexcept>

namespace netstorm::o { //각 store 폭을 구별해 전체8BYTE 초기화로 대체하지 않는다.
void ResetStateZone(std::span<std::uint8_t> zone) {
    if (zone.size()<4) throw std::out_of_range("State Zone의 첫 DWORD 물리 부족");
    const std::uint32_t first{};std::memcpy(zone.data(),&first,4); //첫 원문 DWORD store다.
    if (zone.size()<6) throw std::out_of_range("State Zone의 마지막 WORD 물리 부족");
    const std::uint16_t last{};std::memcpy(zone.data()+4,&last,2); //+4의 WORD만 쓰고 인접+6/+7은 읽거나 쓰지 않는다.
}
}
