// 너겟485230과왕복프레임/지면검사/낙하의실제하위몸체를복원한다.
#pragma once
#include "o/SquidProcess.h"
#include <span>
namespace netstorm::o {
// 기존공간/프레임/현재가상효과와같은Regular확보메모리를연결한다.
struct NuggetRegularHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 4acee0(frame,0)의실제쓰기/표시/필요한재부착이다.
    std::function<void(Sid)> refresh; // 증가프레임뒤추가현재가상88이다. 감소에는이추가호출이없다.
    std::function<bool()> allocate; // 왕복끝점의new28이며실패하면난수를소비하지않는다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInteger; // 4558f0(2,8)의상한제외정수난수다.
    std::function<void(Sid,std::uint32_t,float)> construct; // 확보한같은메모리의496e80이다. payload와현재time을구별한다.
    std::function<void(Sid,std::uint32_t)> destroy,unpop; // 현재가상10/48의실제효과다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 낙하가캡처한x/y와flags40의현재가상90이다.
    RegularHandler damageable; // 마지막44b900의실제처리기다.
};
// 너겟의기존두예약과끝점에서생긴후속예약이같은현재상태를읽는다.
class RawNuggetRegular {
public:
    RawNuggetRegular(SidPool& pool,std::span<const std::int32_t> aFrameCounts,std::span<const std::uint8_t> spots,const bool& boss,NuggetRegularHooks hooks); // PATCH188TYPE/65536spot과현재권한을받는다.
    bool Handles(Sid sid) const; // 실제154/50dc30쌍의485230만자동분배한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const; // count0초기지연뒤실제0/1/2/3사건을처리한다.
    bool CanFall(Sid sid) const; // 485fe0: 실제ftol하위DWORD/감긴지도색인에서spot&6이없으면참이다.
    const SidPool& Pool() const; // 다른Session의상태와실제예약을혼합하지않게한다.
private:
    float Animate(Sid sid,bool grow) const; // 4850a0/485170의증가/감소/끝점예약이다.
    float Fall(Sid sid,std::int32_t count) const; // 484ec0의죽음검사/가속낙하/상한/권한삭제다.
    SidPool& pool_; // 프레임/좌표/상태/가상표의단일물리풀이다.
    std::span<const std::int32_t> aFrameCounts_; // TYPE+170의A구간프레임수다.
    std::span<const std::uint8_t> spots_; // 실제x+256*y의동일점유지도다.
    const bool& boss_; // 낙하100회뒤실제삭제를결정하는현재권한이다.
    NuggetRegularHooks hooks_; // 원본큰하위효과와기존장부/Host의경계다.
};
}
