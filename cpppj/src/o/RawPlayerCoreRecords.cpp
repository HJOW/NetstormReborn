// 실제부분store순서와14BYTE기록/padding2의경계를보존한다.
#include "o/RawPlayerCoreRecords.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 단일원문store의전체물리폭을확인한뒤같은폭만쓴다.
void Store(std::span<std::uint8_t> bytes,std::uint32_t offset,std::uint32_t width,std::uint32_t value) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("Core장부의실제store물리범위");
    for (std::uint32_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 이웃필드와padding은보존한다.
}
// 호출인자로이미복제된10BYTE의DWORD/WORD를원문레지스터값으로읽는다.
std::uint32_t Value(const std::array<std::uint8_t,10>& core,std::uint32_t offset,std::uint32_t width) {
    std::uint32_t value{};
    for (std::uint32_t byte=0;byte<width;++byte) value|=std::uint32_t(core[offset+byte])<<(byte*8); // 실제인자폭만읽는다.
    return value;
}
// 최초자식호출의주소만고정하며그자식의각store가짧은물리를따로판정한다.
std::span<std::uint8_t> At(std::span<std::uint8_t> bytes,std::uint32_t offset) {
    if (offset>bytes.size()) throw std::out_of_range("Core장부초기화자식의실제시작물리");
    return bytes.subspan(offset);
}
}
// lastKnown/지역Core에도재사용하며10BYTE전체선행검사를추가하지않는다.
void InitializePlayerCore(std::span<std::uint8_t> core) {
    Store(core,0,4,0);Store(core,4,4,0);Store(core,8,2,0);
}
// 실제CRT는Core+4를먼저초기화하고그뒤같은record의ID에0을쓴다.
void InitializePlayerCoreRecords(PlayerCoreRecordsView records) {
    auto offset=records.origin;
    for (std::uint32_t record=0;record<kPlayerCoreRecordCount;++record) { // 정상9회만수행하며매기록끝padding2는건드리지않는다.
        InitializePlayerCore(At(records.bytes,offset+4U));Store(records.bytes,offset,4,0);offset+=kPlayerCoreRecordStride;
    }
}
// 입력Core는원문값인자와같은복제본이며기존장부와겹치더라도도중에다시읽지않는다.
void WritePlayerCoreMetadata(PlayerCoreRecordsView records,std::uint32_t owner,std::uint32_t subscriber,std::array<std::uint8_t,10> core) {
    const auto offset=records.origin+owner*kPlayerCoreRecordStride;
    Store(records.bytes,offset,4,subscriber);Store(records.bytes,offset+4U,4,Value(core,0,4));Store(records.bytes,offset+8U,4,Value(core,4,4));Store(records.bytes,offset+12U,2,Value(core,8,2));
}
}
