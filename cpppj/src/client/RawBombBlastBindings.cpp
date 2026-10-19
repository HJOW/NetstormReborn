// 기존 피해/지면/끝 칸/가상 삭제 몸체를 재구현하지 않고 동일 월드에 전달한다.
#include "client/RawBombBlastBindings.h"
#include "client/RawCampaignHitBindings.h"
#include "o/RawBridgeEvents.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client {
// 다른 주문의 추가 그림은 아직 포함하지 않고 실제502370의 요청만 전달한다.
void DrawDevastationAdditional(const o::SidPool& pool,o::Sid sid,std::int32_t x,std::int32_t y,const std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::int32_t)>& draw) {
    const auto raw=pool.Slot(sid);std::uint32_t table{},frame{};
    // 원본 비정렬 가상 표와 프레임의 전체DWORD 비트를 읽는다.
    for (std::size_t index=0;index<4;++index) { table|=std::uint32_t(raw[index])<<(8*index);frame|=std::uint32_t(raw[36+index])<<(8*index); }
    if (table!=0x502370U || !draw) throw std::invalid_argument("Devastation 추가 그림 표/소비자 오류");draw(raw[10],std::bit_cast<std::int32_t>(frame),x,y);
}
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
