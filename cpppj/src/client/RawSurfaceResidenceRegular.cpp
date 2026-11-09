// Native 거주 구역은 같은 실제 지형 역할/신전/난수와 선택한 Dude Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "o/RawResidenceRegular.h"
#include "o/RawTerrainBitmap.h"
#include "o/RawPathSearch.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::client { // 상태 복제나 Session 생성 없이 장기 저장소에 연결한다.
namespace { // 현재 선택한 함수와 실제 전체 Runtime만 이 호출 동안 유지한다.
template<class Function,class... Args>
decltype(auto) ResidenceCall(const std::weak_ptr<const RawSurfaceResidenceBindings>& weak,Function RawSurfaceResidenceBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("거주 구역 Dude의 실제 현재 Runtime 미연결");const auto pin=current->pin;const auto hold=pin ? pin():std::shared_ptr<void>{};const auto call=current.get()->*member;if (!call) throw std::logic_error("거주 구역 Dude의 선택된 실제 본문 미연결");return call(std::forward<Args>(args)...);
}
}
void RawSurfaceStorage::BindResidenceDevices(const std::shared_ptr<const o::TerrainBitmapState>& terrain,std::function<std::uint32_t()> theme) { // 첫 메뉴 전 실제 현재 owner만 약하게 공개한다.
    if (!UsesNativeTransmissionDevices() || !terrain || !theme || !residenceTerrain_.expired() || residenceTheme_) throw std::invalid_argument("거주 구역의 실제 최초 장치 계약");residenceTerrain_=terrain;residenceTheme_=std::move(theme);
}
void RawSurfaceStorage::BindResidenceRuntime(const std::shared_ptr<const RawSurfaceResidenceBindings>& runtime) { // 선택된 생성 효과만 현재 World 수명으로 빌린다.
    if (!UsesNativeTransmissionDevices() || !runtime || !residenceRuntime_.expired()) throw std::invalid_argument("거주 구역 현재 Runtime 대여 계약");residenceRuntime_=runtime;
}
float RawSurfaceStorage::ResidenceRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { // 실제49a4e0의 상태/표/Host를 같은 장기 본문에 연결한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("거주 구역의 실제 Native 장치 미연결");o::ResidenceRegularHooks hooks;
    hooks.random=[this]()->o::GameRandom& { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("거주 구역 같은 현재 난수 미연결");return get(); };
    hooks.notify=[this](auto parent) { Building().NotifySurface(parent); };hooks.frame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };
    hooks.surface=[this](auto x,auto y) { return o::RawPathSurfaceAt(hash,{x,y}); };hooks.owner=[this](auto parent,auto owner) { SetAssetOwner(parent,owner); };
    hooks.element=[this](auto temple) { return types.at(pool.Slot(temple)[10]).theme; };hooks.themeMode=[this] { const auto get=residenceTheme_;if (!get) throw std::logic_error("거주 구역 현재 테마 장치 미연결");return get(); };
    hooks.decorationType=[this] { const auto terrain=residenceTerrain_.lock();if (!terrain) throw std::logic_error("거주 구역 현재 지형 TYPE owner 미연결");return terrain->fringeType; };
    hooks.recordScan=[this](auto count) { auto& maximum=gameAux_->ResidenceMaximumScan();if (maximum<=count) maximum=count; };
    hooks.dude=[this](auto parent) {
        o::ResidenceDudeHooks effects;effects.entrance=[this](auto source) { return ResidenceCall(residenceRuntime_,&RawSurfaceResidenceBindings::entrance,source); };effects.spawn=[this](auto x,auto y,auto owner) { return ResidenceCall(residenceRuntime_,&RawSurfaceResidenceBindings::spawn,x,y,owner); };effects.task=[this](auto born,auto event,auto delay) { ResidenceCall(residenceRuntime_,&RawSurfaceResidenceBindings::task,born,event,delay); };effects.assertion=[this] { AssertDisplay("srcSid",".\\Dude.cpp",0x1b5); };
        o::RunResidenceDudeSpawn(pool,ownerMode.fort,sidLists.dudes,sidLists.residences,parent,effects);
    };
    return o::RunResidenceRegular(pool,hash,types,frames,sidLists.temples,sid,event,count,payload,hooks);
}
}
