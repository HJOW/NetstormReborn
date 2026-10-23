// 원본004b5ec0의지형회복과004bd600의같은manager지형기록쓰기를복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 전체20개×6BYTE Territory를고정길이로빌려원본120BYTE읽기/쓰기를보존한다.
using FortTerritoryRecords=std::span<const std::uint8_t,120>;
// manager/current와일반writer를실제수명소유자에연결하는004bd600의하위경계다.
struct FortTerritoryWriteHooks {
    std::function<std::uint32_t(std::uint32_t)> manager; //고정this슬롯의현재manager를첫진단전/Create전/current조회전에각각읽는다.
    std::function<void(std::uint32_t)> assertion; //Template.cpp521/522다.복귀하면현재값으로계속하며자동성공/반환하지않는다.
    std::function<void(std::uint32_t,std::string_view)> create; //현재manager로Create("Territory")한다.원문의반환stream은사용하지않는다.
    std::function<std::uint32_t(std::uint32_t)> current; //Create뒤다시읽은manager+8의실제현재stream토큰이다.0주소의물리거부도장치가소유한다.
    std::function<void(std::uint32_t,std::optional<FortTerritoryRecords>,std::uint32_t)> write; //같은현재stream에일반0044cce0을호출한다.NULL입력도진단뒤120BYTE길이로전달한다.
};
// 같은현재owner의getter/기록writer/State와29번로그만분리한004b5ec0의실제하위경계다.
struct FortTerritoryRecoverHooks {
    std::function<std::uint32_t()> owner; //00540cac현재DWORD다.읽기전/쓰기전/State전의세시점에다시읽는다.
    std::function<void(std::uint32_t,std::span<std::uint8_t,120>)> read; //004bd660의실제120BYTE읽기다.부모가먼저비운같은지역버퍼를빌리며수명밖에보관하지않는다.
    std::function<void(std::uint32_t,std::string_view)> log; //bit4가있을때29번로그를호출한다.복귀뒤같은flags BYTE를다시읽는다.
    std::function<void(std::uint32_t,FortTerritoryRecords)> write; //004bd600실제본문으로연결한다.현재owner는이경계직전에다시읽는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; //004bd550가반환한같은State주소다.부모는첫BYTE만읽고bit4를지운다.
};
// manager521→NULL입력522→현재manager Create→현재manager.current→일반Write120의순서를보존한다.
void WriteFortTerritoryRecords(std::uint32_t slot,std::optional<FortTerritoryRecords> records,const FortTerritoryWriteHooks& hooks);
// 0초기화120→읽기→20flags/조건부로그→현재owner쓰기→현재owner State첫BYTE회복을실행한다.
void RecoverFortTerritory(const FortTerritoryRecoverHooks& hooks);
}
