// 아이스 타워의 파괴·재성장·주기를 같은 장기 물리/TYPE/Host와 가상 부모에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidFinder.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { //같은실제현재필드를읽고임시World나모드사본을만들지않는다.
namespace { //표면getter의plainCRT절삭은배치의거의올림과구별한다.
// 원문의signed64절삭/하위DWORD와지원범위밖현재입력의0결과를보존한다.
std::int32_t IceCell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<-9223372036854775808.0 || static_cast<double>(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 현재역할의주소만설치하며예전독립값이나Session초기값을현재전역에쓰지않는다.
o::IceTowerTypes& RawSurfaceStorage::IceTowerGlobals() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 아이스의 실제 Native 전역 미결합");
    iceTowerTypes_.sharedIce=&gameAux_->RainBlockerType();iceTowerTypes_.sharedGrowing=&gameAux_->GrowingRainBlockerType();iceTowerTypes_.sharedNoIsland=&links.noIslandType;return iceTowerTypes_;
}
// 모든큰선택효과는원문위치에서만요구하고이미있는실제장기부모를직접사용한다.
o::RawIceTower& RawSurfaceStorage::IceTower() {
    if (!iceTower_) {
        o::IceTowerHooks hooks; //원문네본문의선택순서와현재함수사본수명을보존한다.
        hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.regular=[this](auto parent,auto event,auto period) { ConstructResourceRegular(parent,event,period); };
        hooks.basePop=[this](auto parent,auto flags) { PostPop().PostPopBase(parent,flags); };
        hooks.grid=[this](float x,float y) { const auto cx=IceCell(x),cy=IceCell(y);return cx<0 || cy<0 || cx>255 || cy>255 ? o::Sid{}:o::Sid{hash.Cell(0,cx,cy)}; };
        hooks.genusAt=[this](auto x,auto y,auto genus) { return DamageablePost().GenusAt(x,y,genus); };
        hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };hooks.unpop=[this](auto parent,auto flags) { UnpopAsset(parent,flags); };
        hooks.word=[this](auto parent,std::uint16_t value) { auto raw=pool.AllocatedBytes(parent);raw[12]=static_cast<std::uint8_t>(value);raw[13]=static_cast<std::uint8_t>(value>>8);if (raw[10]==links.bridgeType) Building().NotifySurface(parent); };
        hooks.owner=[this](auto parent,auto owner) { SetAssetOwner(parent,owner); }; //48eec5/48ec34는현재생성자의가상74를호출하므로변경된TYPE의파생Owner도생략하지않는다.
        hooks.pop=[this](auto parent,auto x,auto y,auto flags) { static_cast<void>(PopAsset(parent,x,y,flags)); };
        hooks.destroy=[this](auto parent,auto flags) { static_cast<void>(DestroyAsset(parent,flags)); };
        hooks.dropIsland=[this](auto x,auto y,auto flags,auto owner) { const auto drop=damageableDevices_.dropIsland;if (!drop) throw std::logic_error("장기 아이스의 선택된 실제 받침 정리 미결합");drop(x,y,flags,owner); };
        hooks.damageablePost=[this](auto parent,auto flags) { DamageablePost().PostDestroy(parent,flags); };hooks.basePost=[this](auto parent,auto flags) { CommonDeletion().PostDestroy(parent,flags); };
        hooks.setFrame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };
        iceTower_=std::make_unique<o::RawIceTower>(pool,frames,IceTowerGlobals(),authority_->server,authority_->boss,LoadingDepth(),std::move(hooks)); //첫bool의기존battle이름은옛입력열명이며원문은540bc0서버를읽는다.594fbc전투모드와다르다.
    }
    return *iceTower_;
}
}
