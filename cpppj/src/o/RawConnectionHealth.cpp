// 원문의 현재값 재조회·cached wall·signed 활동 초와 unsigned 프레임 차를 유지한다.
#include "o/RawConnectionHealth.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 게임 정지와 별개로 연결 활동의 실제 벽시각을 사용한다.
namespace { // 선택된 외부 장치와 실제 Player 바이트 범위만 확인한다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("연결 감시 실제 장치 미연결");return effect; } // 자연 반환 가지에서는 사용하지 않는 장치를 요구하지 않는다.
}
bool PollConnectionHealth(ConnectionHealthGlobals g,const ConnectionHealthHooks& h) {
    if ((g.phase[0]==29 || g.phase[0]==33) && !Need(h.connectionExists)(11)) return false;
    if (g.phase[0]!=14) { g.heartbeatEnd=Need(h.wallSeconds)()+1.0;return false; }
    if (g.heartbeatEnd<=Need(h.wallSeconds)()) { Need(h.sendHeartbeat)(g.server);const bool server=g.server;g.heartbeatEnd=Need(h.wallSeconds)()+(server ? 3.0:10.0); } // 송신 효과 뒤 서버 권한을 재조회하고 다음 wall 호출 전에 간격을 선택한다.
    const auto owner=g.localOwner;const auto wall=Need(h.frameWallSeconds)(); // 원문은 heartbeat 뒤 현재owner/프레임wall을 이 순서로 캡처한다.
    const auto elapsed=[&](std::size_t id) { return wall-static_cast<double>(std::bit_cast<std::int32_t>(g.activity.wallSeconds[id])); }; // 원문 signed DWORD 활동 초다.
    if (g.server) {
        for (std::uint32_t player=1;player<9;++player) { // 현재flags1/활동 프레임·로컬owner/전투 종료를 원문 순서대로 검사한다.
            const auto rows=Need(g.players)();if (rows.size()!=9*180) throw std::out_of_range("연결 감시 Player9 물리 누락");if ((rows[player*180+0x50]&1U) && elapsed(player)>15.0 && g.activity.frames[player] && player!=owner && !Need(h.battleFinished)()) Need(h.status)(2,player);
        }return false;
    }
    if (!g.phase[33]) {
        if (elapsed(9)>8.0 && g.activity.frames[9] && !Need(h.battleFinished)()) Need(h.status)(1,0);
        const bool exists=Need(h.connectionExists)(9);if ((!exists || (elapsed(9)>20.0 && Need(h.frameNumber)()-g.activity.frames[9]>20U)) && g.activity.frames[9] && g.autoRestart) { Need(h.saveServerFailure)();return true; }
    }return false;
}
}
