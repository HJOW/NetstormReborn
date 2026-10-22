// 네 다리 전략의 실제 연결 판정·무작위 시작점·빈 사각형·재시도 기한이다.
#pragma once
#include "o/RawAiBridgeCandidates.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 실제 후보 모듈이 소유한 같은 월드와 현재 난수/시각만 참조한다.
class RawAiBridgePlanning {
public:
    RawAiBridgePlanning(const RawAiBridgeCandidates& candidates,GameRandom& random,const double& now,std::function<std::uint32_t(Sid)> graphFor); //graphFor는같은현재00462bc0이며필요한연결판정에서만호출한다.
    bool Connected(std::uint32_t token,Sid geyser) const; //00410d40:현재목표graph가같을때가장자리12칸의자기J/K다리를확인한다.
    void DelayRetry(std::uint32_t token) const; //00411310:현재350 count0이면40초,그외120초를현재QWORD시각에더해5d0에쓴다.
    RawPathPoint RandomStart(std::uint32_t token,const AiBuildSearchBounds& bounds) const; //00411470:20회y→x난수로같은graph/owner의첫열린지면을선택한다.
    bool RectangleFree(std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom) const; //004119b0:inclusive사각형의flat spot BYTE0을X바깥/Y안쪽순서로검사한다.
private:
    bool Edge(std::uint32_t token,std::uint32_t x,std::uint32_t y,std::uint8_t side) const; //현재map좌표0..255와전체bridge/owner/현재첫코드만판정한다.
    const RawAiBridgeCandidates& candidates_; //같은실제본체/풀/TYPE/코드/지도getter의유일한소유모듈이다.
    GameRandom& random_; //다른게임계산과같은00532710난수상태다.
    const double& now_; //현재0055b4d0QWORD를기한을쓸때읽는다.
    std::function<std::uint32_t(Sid)> graphFor_; //현재실제object graph이며호출뒤본체/좌표를다시읽는다.
};
}
