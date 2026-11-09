// 썬 펜스의 전체 이웃·연결선·표식·재생성·수명을 같은 장기 물리와 Host에 연결한다.
#include "client/RawSurfaceWorld.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //임시World/Player/사거리사본없이같은현재전역을사용한다.
// 현재 Player를 실제 선택 호출 동안만 빌리는 getter이며 결합 시 조회/초기화를 하지 않는다.
void RawSurfaceStorage::BindFenceAlliances(std::function<std::uint32_t(std::uint32_t,std::uint32_t)> read) {
    if (!UsesNativeTransmissionDevices() || borrowed_ || fenceAlliances_ || !read) throw std::invalid_argument("장기 펜스의 실제 최초 동맹 장치 중복/누락");
    fenceAlliances_=std::move(read);
}
// TYPE+44와 Player의 현재81DWORD를 원문 선택 위치에서만 읽는다.
o::RawFenceNeighbors& RawSurfaceStorage::FenceNeighbors() {
    if (!fenceNeighbors_) {
        fenceNeighbors_=std::make_unique<o::RawFenceNeighbors>(pool,hash,types,[this](auto type) { return types.at(type).range; },[this](auto row,auto column) {
            const auto read=fenceAlliances_;if (!read) throw std::logic_error("장기 펜스의 선택된 실제 Player 동맹 미결합");return read(row,column);
        });
    }return *fenceNeighbors_;
}
// 삭제는 현재 자산의 실제 가상10에 전달하며 연결선 배열을 별도로 복제하지 않는다.
o::RawFenceLine& RawSurfaceStorage::FenceLine() {
    if (!fenceLine_) fenceLine_=std::make_unique<o::RawFenceLine>(pool,hash,types,frames,gameAux_->FenceShieldType(),[this](auto sid,auto flags) { DestroyAsset(sid,flags); });return *fenceLine_;
}
// 표식의 중심만 기존4adea0 본문에서 읽고 나머지는 실제Factory/가상Owner/Pop를 사용한다.
o::RawFenceMarkerCreate& RawSurfaceStorage::FenceMarkers() {
    if (!fenceMarkers_) {
        o::FenceCreateHooks hooks;hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };hooks.notifySurface=[this](auto sid) { Building().NotifySurface(sid); };
        hooks.setOwner=[this](auto sid,auto player) { SetAssetOwner(sid,player); };hooks.pop=[this](auto sid,auto x,auto y,auto flags) { PopAsset(sid,x,y,flags); };
        fenceMarkers_=std::make_unique<o::RawFenceMarkerCreate>(pool,gameAux_->FenceMarkerType(),std::move(hooks),[this](auto sid) { return FenceNeighbors().Center(sid); });
    }return *fenceMarkers_;
}
// 화면 포스 필드도 같은 현재 역할과 실제188프레임에서 생성한다.
o::RawFenceShieldCreate& RawSurfaceStorage::FenceShields() {
    if (!fenceShields_) {
        o::FenceCreateHooks hooks;hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };hooks.setOwner=[this](auto sid,auto player) { SetAssetOwner(sid,player); };
        hooks.pop=[this](auto sid,auto x,auto y,auto flags) { PopAsset(sid,x,y,flags); };
        fenceShields_=std::make_unique<o::RawFenceShieldCreate>(pool,frames,gameAux_->FenceShieldType(),std::move(hooks));
    }return *fenceShields_;
}
// 일반finder가 저장한next와 현재 태그를 그대로 사용한다.
o::RawFenceMarkerClear& RawSurfaceStorage::FenceMarkerClear() {
    if (!fenceMarkerClear_) fenceMarkerClear_=std::make_unique<o::RawFenceMarkerClear>(pool,hash,types,gameAux_->FenceMarkerType(),[this](auto sid,auto flags) { DestroyAsset(sid,flags); },[this](auto sid) { return FenceNeighbors().Center(sid); });return *fenceMarkerClear_;
}
// 원문 동맹 이웃/사거리 이후의 두 생성기를 같은 본체로 연결한다.
o::RawFenceRebuild& RawSurfaceStorage::FenceRebuild() {
    if (!fenceRebuild_) {
        o::FenceRebuildHooks hooks;hooks.neighbor=[this](auto sid,auto direction) { return FenceNeighbors().Neighbor(sid,direction,true); };
        hooks.withinRange=[this](auto sid,auto points) { return FenceNeighbors().WithinRange(sid,points); };hooks.markers=[this](auto sid,auto other) { FenceMarkers().Create(sid,other); };
        hooks.shields=[this](auto points,auto first,auto second,auto a,auto b) { FenceShields().Create(points,first,second,a,b); };
        fenceRebuild_=std::make_unique<o::RawFenceRebuild>(pool,std::move(hooks));
    }return *fenceRebuild_;
}
// 각new40 성공 후 같은 확보 객체를 생성/부착하고 공통Damageable장부를 재사용한다.
o::RawFencePostPop& RawSurfaceStorage::FencePostPop() {
    if (!fencePostPop_) {
        o::FencePostPopHooks hooks;hooks.theme=[this](auto sid) { return std::bit_cast<std::uint32_t>(types.at(pool.Slot(sid)[10]).theme); };hooks.neighbor=[this](auto sid,auto direction) { return FenceNeighbors().Neighbor(sid,direction); };
        hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto sid,auto event,auto payload) { ConstructResourceRegular(sid,event,payload); };
        hooks.clearLine=[this](auto points) { FenceLine().Clear(points); };hooks.connections=[this](auto sid) { return FenceNeighbors().Connections(sid); };
        hooks.validateBase=[this](auto sid,auto flags,const auto* projected) { PostPop().ValidateBase(sid,flags,projected); };hooks.damageable=[this](auto sid,auto flags) { ResourceCarrierPostPop().DamageablePostPop(sid,flags); };
        fencePostPop_=std::make_unique<o::RawFencePostPop>(pool,std::move(hooks));
    }return *fencePostPop_;
}
// Host재진입도현재Frame/역할/동맹과같은전처리/재생성을사용한다.
o::RawFenceRegular& RawSurfaceStorage::FenceRegular() {
    if (!fenceRegular_) {
        o::FenceRegularHooks hooks;hooks.neighbor=[this](auto sid,auto direction) { return FenceNeighbors().Neighbor(sid,direction); };
        hooks.clearMarkers=[this](auto sid,auto other) { FenceMarkerClear().Clear(sid,other); };hooks.clearLine=[this](auto points) { FenceLine().Clear(points); };hooks.connections=[this](auto sid) { return FenceNeighbors().Connections(sid); };
        hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto sid,auto event,auto payload) { ConstructResourceRegular(sid,event,payload); };
        hooks.refreshNeighbors=[this](auto sid) { FenceLifetime().Refresh(sid); };hooks.rebuild=[this](auto sid) { FenceRebuild().Rebuild(sid); };
        hooks.damageable=[this](auto sid,auto event,auto count,auto payload) { return ResourceDamageableRegular(sid,event,count,payload); };
        fenceRegular_=std::make_unique<o::RawFenceRegular>(pool,frames,Frame(),std::move(hooks));
    }return *fenceRegular_;
}
// 현재 스냅샷/실제 정리/처음 두 이웃 예약 후 같은Damageable전처리를 호출한다.
o::RawFenceLifetime& RawSurfaceStorage::FenceLifetime() {
    if (!fenceLifetime_) {
        o::FenceLifetimeHooks hooks;hooks.neighbor=[this](auto sid,auto direction,auto allied) { return FenceNeighbors().Neighbor(sid,direction,allied); };
        hooks.clearMarkers=[this](auto sid,auto other) { FenceMarkerClear().Clear(sid,other); };hooks.clearLine=[this](auto points,auto,auto,auto,auto) { FenceLine().Clear(points); };
        hooks.unpop=[this](auto sid,auto flags) { UnpopAsset(sid,flags); };hooks.repop=[this](auto sid,auto flags) { RepopAsset(sid,flags); };hooks.rebuild=[this](auto sid) { FenceRebuild().Rebuild(sid); };
        hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto sid,auto event,auto payload) { ConstructResourceRegular(sid,event,payload); };
        hooks.damageable=[this](auto sid,auto flags) { DamageablePre().PreDestroy(sid,flags); };fenceLifetime_=std::make_unique<o::RawFenceLifetime>(pool,std::move(hooks));
    }return *fenceLifetime_;
}
}
