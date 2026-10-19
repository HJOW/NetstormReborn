// 폭발 주문의 큰 효과를 같은 실제 월드/피격/다리/지면 효과 몸체에 연결한다.
#pragma once
#include "o/RawBombBlast.h"
namespace netstorm::client {
// 원본41e3d0의 현재 TYPE/프레임/입력 좌표를 실제 평문 RLE 그리기에 넘긴다. 기본+a0는 별도로 빈 그림이다.
void DrawDevastationAdditional(const o::SidPool& pool,o::Sid sid,std::int32_t x,std::int32_t y,const std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::int32_t)>& draw);
class RawSurfaceWorld; // 실제 공간/가상 삭제와 다리 처리기를 소유하는 현재 월드다.
class RawCampaignHitBindings; // 실제 사제/일반/신전 피해와 지면 효과를 소유하는 현재 피격 체인이다.
// 반환 콜백은 세 인자를 빌리므로 같은 세션의 설치/해제 순서를 유지해야 한다.
o::BombBlastHooks MakeCampaignBombBlastHooks(RawSurfaceWorld& world,const RawCampaignHitBindings& hits,const o::RawBombTargets& coordinates);
}
