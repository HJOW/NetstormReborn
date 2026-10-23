// Player의9×16BYTE Core 장부와별도Core10BYTE초기화원문을복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o {
inline constexpr std::uint32_t kPlayerCoreRecordCount=9,kPlayerCoreRecordStride=16,kPlayerCoreRecordBytes=144; // 실제00595360의9기록폭이며각기록끝2BYTE는padding이다.
// 실제물리안의장부시작을빌려주소산술을보존하고별도ID/Core배열로분리하지않는다.
struct PlayerCoreRecordsView {
    std::span<std::uint8_t> bytes; // 정상Client는144BYTE다.명시물리관측은인접주소를포함한더넓은뷰도사용할수있다.
    std::uint32_t origin{}; // bytes시작에서장부첫주소까지의원문32비트상대위치이며정상Client는0이다.
};
//0042ce70: 같은Core에DWORD0→DWORD4→WORD8을0으로쓴다.짧으면앞선store만남긴다.
void InitializePlayerCore(std::span<std::uint8_t> core);
//004feac0:9회각record+4의Core초기화뒤subscriberDWORD0을쓴다.padding2/인접lastKnown은보존한다.
void InitializePlayerCoreRecords(PlayerCoreRecordsView records);
//0048f350:owner×16주소의ID DWORD→복제된Core DWORD/DWORD/WORD를쓴다.논리owner상한관문은추가하지않는다.
//owner주소의32비트감김은보존하지만선택된store가bytes밖이면host물리계약오류다.
void WritePlayerCoreMetadata(PlayerCoreRecordsView records,std::uint32_t owner,std::uint32_t subscriber,std::array<std::uint8_t,10> core);
}
