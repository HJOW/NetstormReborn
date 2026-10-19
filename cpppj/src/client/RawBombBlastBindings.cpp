// 기존 피해/지면/끝 칸/가상 삭제 몸체를 재구현하지 않고 동일 월드에 전달한다.
#include "client/RawBombBlastBindings.h"
#include "client/RawCampaignHitBindings.h"
#include "o/RawBridgeEvents.h"
#include <stdexcept>
namespace netstorm::client {
// 같은Pool을 구성 시 확인하고 실제 큰 효과는 요청 시점에만 실행한다.
o::BombBlastHooks MakeCampaignBombBlastHooks(RawSurfaceWorld& world,const RawCampaignHitBindings& hits,const o::RawBombTargets& coordinates) {
    const auto core=world.Core();
    if (&core.pool!=&hits.Pool() || &core.pool!=&coordinates.Pool()) throw std::invalid_argument("폭발 실제 월드 Pool 불일치");
    o::BombBlastHooks hooks;
    // 이 계열502370의 실제 가상b4는41e3c0 기본반경이다.
    hooks.radius=[&coordinates](o::Sid parent,std::uint32_t table) { if (table!=0x502370U) throw std::invalid_argument("폭발 반경 가상 표 불일치");return coordinates.DefaultRadius(parent); };
    // 기존 현재 가상98 분배가 사제/일반/신전 실제 피해를 선택한다.
    hooks.damage=[damage=hits.DamageHooks({}).damage](o::Sid target,std::uint32_t,std::int32_t amount,std::uint8_t owner,o::Sid parent) {
        if (!damage) throw std::logic_error("폭발 실제 가상 피해 미연결");damage(target,amount,owner,parent);
    };
    // 원본이 무시하는 가상 삭제 반환값을 새 성공/실패 관문으로 사용하지 않는다.
    hooks.destroy=[&world](o::Sid target,std::uint32_t,std::uint32_t flags) { static_cast<void>(world.Destroy(target,flags)); };
    // 실제470ac0의Dude/공유난수/효과 생성은 기존 단일 인스턴스를 쓴다.
    hooks.ground=[&hits](float x,float y) { hits.Impact().Ground(x,y); };
    // 실제421f70이 감싸는4215d0의 끝 칸 전환을 같은 표면 처리기에 전달한다.
    hooks.edge=[bridge=&core.bridgeEvents](o::Sid target,float x,float y) { bridge->ConvertEnd(target,x,y); };
    return hooks;
}
}
