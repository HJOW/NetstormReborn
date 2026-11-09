// 공통 주문의 실제 정기 본문과 현재 TYPE/권한을 Native Client 수명으로 보유한다.
#include "client/RawSurfaceWorld.h"
#include "client/GameAssets.h"
#include "o/RawFrameAdvance.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장기 본문은 World/Session을 만들거나 현재 게임 상태를 초기화하지 않는다.
namespace { // 기존 전체 자격/효과가 연결된 실제 열두 VT만 Native 정기 분배에 등록한다.
constexpr std::array<std::uint32_t,12> kConnectedBombTables{0x502908U,0x502b70U,0x5029d0U,0x502aa8U,0x5030c0U,0x502448U,0x502370U,0x502510U,0x5025d8U,0x502e68U,0x502f30U,0x502ff8U}; // 나머지4VT는 별도 복원 대상이다.
}
// 큰 후단은 같은 World가 실제 외곽 pin과 함께 공개하며 Storage는 약하게만 빌린다.
void RawSurfaceStorage::BindBombRuntime(const std::shared_ptr<const RawSurfaceBombBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !bombRuntime_.expired()) throw std::invalid_argument("현재 Bomb 후단 대여 중복/누락");bombRuntime_=runtime;
}
// 호출 전에 현재 권한/owner 값을 복사하지 않고 원문이 소비하는 위치에서 같은 실제 주소를 읽는다.
o::BombRegularState& RawSurfaceStorage::BombState() {
    if (!UsesNativeTransmissionDevices() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::logic_error("장기 Bomb의 실제 PATCH Client 장치 미연결");
    bombState_.sharedBoss=&authority_->boss;
    if (!bombState_.currentLocalOwner) bombState_.currentLocalOwner=[this] { const auto read=postPopDevices_.localOwner;if (!read) throw std::logic_error("장기 Bomb의 실제 현재 Player getter 미연결");return read(); };
    return bombState_;
}
// 최초 자산 로드의 TYPE+1b8만 보충하고 이후 Session/World 교체에서는 같은 현재 값을 유지한다.
std::span<std::int32_t> RawSurfaceStorage::BombCastTimes() {
    if (!UsesNativeTransmissionDevices() || !Source() || types.size()!=bombCastTimes_.size()) throw std::logic_error("장기 Bomb의 같은 실제 TYPE 자산 owner 미연결");
    if (!bombCastTimesLoaded_) {
        for (const auto& asset:Source()->Types()) { // 원본 자산 block 번호로 TYPE70 이후의 실제 casttime을 한 번 읽는다.
            const auto cast=asset.definition.Number("casttime").value_or(0.0);const auto type=asset.block+70U;
            if (type>=bombCastTimes_.size() || !std::isfinite(cast) || cast<std::numeric_limits<std::int32_t>::min() || cast>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("장기 Bomb 현재 casttime TYPE/정수 범위");
            bombCastTimes_[type]=static_cast<std::int32_t>(cast);
        }
        bombCastTimesLoaded_=true;
    }
    return bombCastTimes_;
}
// 실제 현재 VT의 DWORD를 읽으며 TYPE 번호/사건만으로 미연결 주문을 다른 효과에 보내지 않는다.
bool RawSurfaceStorage::HasBombRegular(o::Sid sid) const {
    if (pool.Edition()!=o::OriginalEdition::Patch1078) return false;const auto raw=pool.Slot(sid);
    const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
    return std::find(kConnectedBombTables.begin(),kConnectedBombTables.end(),table)!=kConnectedBombTables.end();
}
// 실제 확보 메모리의 개수만 노출하여 누락 후단/예외 뒤 해제 여부를 진단한다.
std::size_t RawSurfaceStorage::BombSearchReservations() const { return bombSearchMemory_.size(); }
// 같은 Pool/현재 프레임/시전 시간·권한과 실제 Client 방송을 기존 전체 Handle에 한 번 연결한다.
o::RawBombRegular& RawSurfaceStorage::BombRegular() {
    if (!bombRegular_) {
        static_cast<void>(BombCastTimes());o::BombRegularHooks hooks;
        hooks.notice=[this](auto parent,auto count,auto event) {
            const auto current=bombRuntime_.lock();if (!current) throw std::logic_error("장기 Bomb의 실제 현재 시전 플레어 Runtime 미연결");
            const auto pin=current->pin;const auto owner=pin ? pin():std::shared_ptr<void>{};const auto call=current->notice;
            if (!call) throw std::logic_error("장기 Bomb의 실제 시전 플레어 본문 미연결");call(parent,count,event);
        };
        hooks.frame=[this](auto sid,auto frame,auto flags) { Frame().Set(sid,std::bit_cast<std::int32_t>(frame),flags); };
        hooks.animate=[this](auto sid,auto delta,auto flags) { o::RawFrameAdvance(pool,frames,Frame()).Advance(sid,std::bit_cast<std::int32_t>(delta),flags); };
        hooks.allocate=[this](auto bytes) {
            if (bytes!=400) throw std::logic_error("장기 Bomb의 원문 검색 할당 크기 불일치");
            try { bombSearchMemory_.push_back(std::make_unique<std::uint32_t[]>(bytes/sizeof(std::uint32_t)));return true; }catch (const std::bad_alloc&) { return false; }
        };
        hooks.targetBuffer=[this]()->std::span<std::uint32_t> { if (bombSearchMemory_.empty()) throw std::logic_error("장기 Bomb의 실제 확보 검색 배열 미연결");return {bombSearchMemory_.back().get(),100}; }; // 검색과 패킷 대상 복사는 같은 실제400BYTE를 읽고 쓴다.
        hooks.targets=[this](auto parent,auto output) {
            const auto current=bombRuntime_.lock();if (!current) throw std::logic_error("장기 Bomb의 실제 현재 대상 Runtime 미연결");
            const auto pin=current->pin;const auto owner=pin ? pin():std::shared_ptr<void>{};const auto call=current->targets;
            if (!call) throw std::logic_error("장기 Bomb의 실제 전체 대상 검색 미연결");return call(parent,output);
        };
        hooks.broadcast=[this](auto packet) { BroadcastAssetPacket(packet); }; // 각 현재 Source의 실제 observer/송신만 길이를 마무리한다.
        hooks.receive=[this](auto player,auto connection,auto packet) {
            const auto current=bombRuntime_.lock();if (!current) throw std::logic_error("장기 Bomb의 실제 현재 효과 Runtime 미연결");
            const auto pin=current->pin;const auto owner=pin ? pin():std::shared_ptr<void>{};const auto call=current->receive;
            if (!call) throw std::logic_error("장기 Bomb의 실제 전체 효과 수신 미연결");call(player,connection,packet);
        };
        hooks.release=[this](auto bytes) { if (bytes!=400 || bombSearchMemory_.empty()) throw std::logic_error("장기 Bomb의 실제 검색 메모리 해제 순서 오류");bombSearchMemory_.pop_back(); };
        auto body=std::make_unique<o::RawBombRegular>(pool,BombState(),std::move(hooks),[this](auto type) { if (type>=frames.size()) throw std::out_of_range("장기 Bomb의 현재 TYPE 물리 범위");return o::BombRegularType{frames[type].Run('A').first,BombCastTimes()[type]}; });
        bombRegular_=std::move(body);
    }
    return *bombRegular_;
}
}
