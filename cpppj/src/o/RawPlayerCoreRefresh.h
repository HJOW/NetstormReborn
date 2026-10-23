// 현재Core의기술bit와자기소유다이스의count하한을0044fc00순서로갱신한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 현재rawPool뷰와원문32비트주소역할을묶으며별도객체목록을만들지않는다.
struct PlayerCoreRefreshPool {
    std::span<const std::uint8_t> bytes; // callback변경후현재물리를다시빌리는원본50BYTE슬롯표다.
    std::uint32_t address{}; // host포인터가아닌원문주소산술/유효관문의식별값이다.
    std::uint32_t capacity{}; //005c847c의현재DWORD이며iteratorcached상한과구별한다.
};
// 가상level호출에넘길같은현재객체물리와주소역할이다.
struct PlayerCoreRefreshForm {
    std::span<const std::uint8_t> bytes; // 현재Pool의같은객체슬롯이며복사버퍼가아니다.
    std::uint32_t address{}; // 같은가상호출대상을식별하는원문32비트주소다.
};
// Begin마다따로살아있는ContainedFinder를빌려중첩/재진입의현재와next를분리한다.
struct PlayerCoreRefreshCursor {
    std::function<std::uint32_t()> current; // 실제현재후보SID이며0이순회종료다.
    std::function<void()> next; //004b1b20의이미저장한다음링크진행이다.
};
// 현재전역/기존실제자식은호출위치에서만요구하며사본상태나성공기본값을추가하지않는다.
struct PlayerCoreRefreshHooks {
    std::function<std::span<std::uint8_t>()> core; //0048f0d0의진단/현재재조회를포함한실제Core뷰다.
    std::function<std::uint32_t()> technologyParent; //0048fc60의현재known/owner/Player기록조회다.
    std::function<PlayerCoreRefreshCursor(std::uint32_t)> begin; //004b2270의실제독립Finder와기본true필터다.
    std::function<PlayerCoreRefreshPool()> pool; //현재Pool/base/capacity를직접조회하는순수provider다.
    std::function<std::int32_t(std::uint32_t)> technologyBit; //0049a840이반환한현재TYPE+1ec의signedDWORD다.종류WORDffff를부모가미리거부하지않는다.
    std::function<std::uint32_t()> playerType; //005412ec의현재다이스타입DWORD(정상165)를순수조회한다.Player152전역005412b8과다르다.
    std::function<std::uint32_t()> owner; //00540c70의현재DWORD를순수조회한다.
    std::function<std::int32_t(PlayerCoreRefreshForm)> level; //선택한같은객체의가상+80이다.정상VT507950은004478f0다이스count이며첫째/둘째반환을합치지않는다.
    std::function<void(std::uint32_t)> assertion; //Ch.cpp346의기술bit범위진단이며복귀후같은Core물리에쓴다.
};
// 원문357BYTE부모다.짧은span/원시무효포인터는선택접근의물리계약오류로거부한다.
void RefreshPlayerCore(const PlayerCoreRefreshHooks& hooks);
}
