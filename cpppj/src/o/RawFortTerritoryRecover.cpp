// 원문의callback뒤현재값재조회와명시중단전부분효과를보존한다.
#include "o/RawFortTerritoryRecover.h"
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kTerritoryBytes=120; //004bd600이NULL입력이라도일반writer에전달하는고정길이다.
constexpr std::size_t kTerritoryCount=20; //004b5ec0이모든6BYTE기록의flags를앞에서부터확인하는개수다.
constexpr std::string_view kRecoveredMessage="Territory recovered from Battle!\n"; //원문29번로그의NUL제외문자열이다.
// 진단장치를임의생략하지않고실제원문조건에서만호출한다.
void TerritoryAssert(const FortTerritoryWriteHooks& hooks,std::uint32_t line) {
    if (!hooks.assertion) throw std::logic_error("Territory writer 실제진단미연결");hooks.assertion(line);
}
}
// Create반환을선택한stream으로대체하지않고같은this의현재manager를다시읽는다.
void WriteFortTerritoryRecords(std::uint32_t slot,std::optional<FortTerritoryRecords> records,const FortTerritoryWriteHooks& hooks) {
    if (!hooks.manager) throw std::logic_error("Territory writer 현재manager미연결");
    if (!hooks.manager(slot)) TerritoryAssert(hooks,521);
    if (!records) TerritoryAssert(hooks,522);
    if (!hooks.create) throw std::logic_error("Territory writer 실제Create미연결");hooks.create(hooks.manager(slot),"Territory");
    if (!hooks.current) throw std::logic_error("Territory writer 현재stream미연결");const auto stream=hooks.current(hooks.manager(slot)); //고정this는유지하고Create뒤현재manager만재조회한다.
    if (!hooks.write) throw std::logic_error("Territory writer 실제일반쓰기미연결");hooks.write(stream,records,kTerritoryBytes);
}
// 변경flags가없어도같은기록을항상새로쓰고State의첫BYTE를회복한다.
void RecoverFortTerritory(const FortTerritoryRecoverHooks& hooks) {
    std::array<std::uint8_t,kTerritoryBytes> records{}; //원문지역120BYTE를읽기전모두0으로채운다.
    if (!hooks.owner) throw std::logic_error("Territory 회복현재owner미연결");
    if (!hooks.read) throw std::logic_error("Territory 회복실제getter미연결");hooks.read(hooks.owner(),records);
    // 각6BYTE의두번째BYTE만읽는다.로그가버퍼를바꾸면복귀뒤같은BYTE의현재값에서bit4를지운다.
    for (std::size_t index=0;index<kTerritoryCount;++index) {
        const auto offset=index*6+1; //다른다섯BYTE와다른flags비트는보존한다.
        if (records[offset]&4U) {
            if (!hooks.log) throw std::logic_error("Territory 회복실제로그미연결");hooks.log(29,kRecoveredMessage);records[offset]&=0xfbU;
        }
    }
    if (!hooks.write) throw std::logic_error("Territory 회복실제writer미연결");hooks.write(hooks.owner(),records);
    if (!hooks.state) throw std::logic_error("Territory 회복실제State미연결");auto state=hooks.state(hooks.owner()); //반환주소는State callback뒤다른owner가되어도그대로쓴다.
    if (state.empty()) throw std::out_of_range("Territory 회복State첫BYTE물리범위");state[0]&=0xfbU;
}
}
