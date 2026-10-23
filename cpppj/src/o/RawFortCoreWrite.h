// 원본004beb30의CoreData저장부모와선택된실제하위경계만복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// Template.cpp의실제진단이며복귀하면그뒤현재전역을다시읽는다.
enum class FortCoreWriteAssertion : std::uint32_t {
    MissingTemplate=0x45f, //진입의고정this첫DWORD가0이다.Editor에서도먼저진단한다.
    InMetamap=0x460, //현재00594fc8이비영이다.진단후post/editor자연관문을계속한다.
    WrongPlayer=0x467 //clamp한고정this의Player번호와현재00540c70이다르다.
};
// 같은Client/Session의현재전역을빌린다.이모듈은독립전역이나난수를소유하지않는다.
struct FortCoreWriteGlobals {
    std::function<std::uint32_t()> metamap; //00594fc8이며manager진단복귀뒤조회한다.
    std::function<std::uint32_t()> missionPost; //00594fa4이며metamap진단뒤조회한다.비영이면editor도읽지않는다.
    std::function<std::uint32_t()> editor; //005c85a4이며post0일때만조회한다.비영이면하위writer를선택하지않는다.
    std::function<std::span<std::uint8_t>()> players; //0059534c의현재Player배열이다.Create복귀뒤한번받은물리를본문끝까지유지한다.
    std::function<std::uint32_t()> playerOwner; //00540c70이며고정Player주소계산뒤진단에사용한다.Template owner와구별한다.
    std::function<std::uint32_t&()> seed; //005c89b4의같은DWORD참조를첫10BYTEwrite뒤에만빌린다.자연ret에는미요구이며선택시실제고정owner가필수다.
};
// 도달하지않은하위는요구하지않는다.선택된장치의실제본문은빈성공으로대체하지않는다.
struct FortCoreWriteHooks {
    std::function<std::uint32_t(std::int32_t)> manager; //고정this의현재첫DWORD를두원문읽기위치에서조회한다.
    std::function<void(FortCoreWriteAssertion)> assertion; //실제Template.cpp행번호진단이다.부분중단은그대로전파한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; //0044d1b0의현재manager/CoreData이며반환한같은stream을모든child에넘긴다.
    std::function<void()> refresh; //0044fc00의실제선행효과다.직렬화보다먼저호출하며부재를성공으로채우지않는다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> write; //0044cce0의일반writer다.고정Player+6c10BYTE다음로컬seed2BYTE를쓴다.
    std::function<void()> prepareRandom; //현재seed.low16이0일때한번실행하는004559d0이다.
    std::function<std::uint32_t(std::uint32_t)> nextRandom; //004558c0(65536)의실제공유난수다.반환low16이0이면반복하며GameRandom의새owner를만들지않는다.
    std::function<void(std::uint32_t,float)> sp; //004be960의실제SP writer다.각호출직전고정Player의byte+70/word+6c/word+6e를다시읽는다.
};
// 004beb30:manager/metamap진단→post/editor자연ret→실제CoreData생성/쓰기순서다.
// slot1..8만같은Player번호이고그밖slot은Player0이다.범위밖물리는명시실패하며callback부분효과를복구하지않는다.
void WriteFortCoreData(std::int32_t slot,const FortCoreWriteGlobals& globals,const FortCoreWriteHooks& hooks);
}
