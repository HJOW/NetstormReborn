// Badges의원본28개기록을한배열로소유하고로더/조회/저장이같은span을빌리게한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace netstorm::o {
inline constexpr std::size_t kFortBadgesRecordCount=28,kFortBadgesRecordBytes=220; // 10.78의실제기록수와원본stride다.
inline constexpr std::size_t kFortBadgesGlobalsBytes=kFortBadgesRecordCount*kFortBadgesRecordBytes; // type/ASCII자료/padding/count를포함한전체6160BYTE다.
inline constexpr std::size_t kFortBadgesCountOffset=216; // 각기록에서현재수량DWORD가놓이는위치다.
// 004fcbe0: CRT가지정한6152BYTE만구성한다. 첫기록+4..11의8BYTE는입력그대로보존한다.
void InitializeFortBadgesRecords(std::span<std::uint8_t> records);
// 0047d860:28개countDWORD만0으로만들고다른6048BYTE를보존한다.
void ResetFortBadgeCounts(std::span<std::uint8_t> records);
// 한Client수명에BSS0→CRT를한번적용하는실제6160BYTE소유자다. Session/Storage생성에묶지않는다.
class RawFortBadgesGlobals {
public:
    RawFortBadgesGlobals(); // 새배열을최초0으로준비한뒤원본CRT구성을한번적용한다.
    RawFortBadgesGlobals(const RawFortBadgesGlobals&)=delete; // 빌린span뒤에별도장부복제를만들지않는다.
    RawFortBadgesGlobals& operator=(const RawFortBadgesGlobals&)=delete; // 같은Client의장부주소를대입으로바꾸지않는다.
    RawFortBadgesGlobals(RawFortBadgesGlobals&&)=delete; // 이미빌린배열의주소를이동으로무효화하지않는다.
    RawFortBadgesGlobals& operator=(RawFortBadgesGlobals&&)=delete; // span소비자가살아있는장부의이동대입을막는다.
    std::span<std::uint8_t,kFortBadgesGlobalsBytes> Records(); // 기존MetaLoader와같은배열의mutable뷰다.
    std::span<const std::uint8_t,kFortBadgesGlobalsBytes> Records() const; // 새getter/writer가같은배열을읽기만빌리는뷰다.
    void ResetCounts(); // 원본시작reset을명시호출하며전체CRT초기화는반복하지않는다.
private:
    std::array<std::uint8_t,kFortBadgesGlobalsBytes> records_{}; // 원본BSS처럼처음0인단일고정장부이며타입/ASCII자료도함께소유한다.
};
}
