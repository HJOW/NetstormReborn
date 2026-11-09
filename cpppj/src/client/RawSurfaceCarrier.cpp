// Carrier의 내용물/현재 TYPE와 미리보기 수명을 실제 Client 물리에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawCarrierFalling.h"
#include "o/RawCarrierPreDestroy.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 메뉴·NoWorld·Session이 같은 장기 Carrier 본문을 사용한다.
namespace { // 불변 원문 가상 표와 선택 호출의 수명 규칙만 이 파일에 한정한다.
constexpr std::array<std::uint32_t,6> kCarrierTables{0x503d80U,0x503e50U,0x503f20U,0x503ff0U,0x5040c0U,0x50e568U}; // PE의 실제+5c=42a750 여섯 직접 가상 표다.
// 정렬되지 않은 슬롯의 실제 현재 VT만 읽는다.
std::uint32_t CarrierTable(std::span<const std::uint8_t> bytes) {
    std::uint32_t value{};
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[byte])<<(byte*8); // 원문 DWORD 폭만 결합한다.
    return value;
}
// 선택 후단을 약하게 빌리고 실제 전체 Runtime pin 뒤 함수 사본을 호출한다.
template<class Function,class... Args>
decltype(auto) CarrierCall(const std::weak_ptr<const RawSurfaceCarrierBindings>& weak,Function RawSurfaceCarrierBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("Carrier의 현재 실제 큰 Runtime 미연결");
    const auto pin=current->pin;const auto owner=pin ? pin():std::shared_ptr<void>{};const auto call=current.get()->*member;
    if (!call) throw std::logic_error("Carrier의 선택된 실제 큰 본문 미연결");
    return call(std::forward<Args>(args)...);
}
}
// 현재 큰 하위만 weak로 공개하며 권한/타입/목록/Host를 재초기화하지 않는다.
void RawSurfaceStorage::BindCarrierRuntime(const std::shared_ptr<const RawSurfaceCarrierBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !carrierRuntime_.expired()) throw std::invalid_argument("Carrier 실제 후단 대여 중복/누락");carrierRuntime_=runtime;
}
bool RawSurfaceStorage::HasCarrierRegular(o::Sid sid) const { const auto table=CarrierTable(pool.Slot(sid));return std::find(kCarrierTables.begin(),kCarrierTables.end(),table)!=kCarrierTables.end(); }
// 일반 내용물 표시/기한/위험 칸은 장기 현재 상태로 실행하고 큰 이동/낙하만 현재 전체 Runtime을 요구한다.
o::RawCarrierRegular& RawSurfaceStorage::CarrierRegular() {
    if (!UsesNativeTransmissionDevices() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::logic_error("Carrier 실제 PATCH Client 장치 미연결");
    if (!carrierRegular_) {
        o::CarrierRegularHooks hooks;
        hooks.find=[this](auto parent,auto mask,auto kind,auto index) { return CarrierPre().FindContained(parent,mask,kind,index); };
        hooks.defaultFrame=[this](auto type) { return types.at(type).defaultFrame; };
        hooks.castTime=[this](auto type) { return BombCastTimes()[type]; }; // 공통 TYPE+1b8의 현재188행을 그대로 공유한다.
        hooks.create=[this](auto type,auto flags) { return Factory().Create(type,flags); }; // 원문4af530에 World 변경 관찰을 앞붙이지 않는다.
        hooks.setWord=[this](auto sid,auto value) { auto raw=pool.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(value);raw[13]=static_cast<std::uint8_t>(value>>8);if (raw[10]==links.bridgeType) Building().NotifySurface(sid); };
        hooks.setOwner=[this](auto sid,auto owner) {
            const auto table=CarrierTable(pool.Slot(sid));
            if (table==0x50f210U || table==0x5196f0U || table==0x5197a8U || table==0x519860U || table==0x519918U) { CarrierCall(carrierRuntime_,&RawSurfaceCarrierBindings::setOwner,sid,owner);return; } // Priest/메타의 실제 전용74만 기존 전체 분배기로 보낸다.
            SetAssetOwner(sid,owner);
        };
        hooks.pop=[this](auto sid,auto x,auto y,auto flags) { static_cast<void>(PopAsset(sid,x,y,flags)); };
        hooks.refresh=[this](auto sid) { Display().Update(pool.Slot(sid)); };
        hooks.needsEscape=[this](auto x,auto y) { return o::RawCarrierFalling::NeedsEscapeIn(pool,types,frames,hash.Entries(0),x,y)!=0; };
        hooks.movementType=[this](auto type) { return CarrierCall(carrierRuntime_,&RawSurfaceCarrierBindings::movementType,type); };
        hooks.hasPath=[this](auto sid,auto type) { return CarrierCall(carrierRuntime_,&RawSurfaceCarrierBindings::hasPath,sid,type); };
        hooks.escape=[this](auto sid,auto delay,auto event) { CarrierCall(carrierRuntime_,&RawSurfaceCarrierBindings::escape,sid,delay,event); };
        hooks.falling=[this](auto sid,auto count) { return CarrierCall(carrierRuntime_,&RawSurfaceCarrierBindings::falling,sid,count); };
        hooks.damageable=[this](auto sid,auto event,auto count,auto payload) { return ResourceDamageableRegular(sid,event,count,payload); };
        hooks.allocateTimer=[this] {
            if (pendingContentPreview_) throw std::logic_error("장기 내용물 타이머 확보 메모리 중복");
            if (!contentPreviewLogic_) {
                o::ContentPreviewHooks preview;
                preview.attach=[this](auto& fields,auto parent,auto type,auto flags) {
                    if (!pendingContentPreview_ || &pendingContentPreview_->Fields()!=&fields) throw std::logic_error("장기 내용물 타이머 실제 확보 메모리 불일치");
                    static_cast<void>(Processes().Host().Attach(std::move(pendingContentPreview_),type,parent,flags));
                };
                preview.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
                preview.assertion=[this] { AssertDisplay("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4); };
                contentPreviewLogic_=std::make_unique<o::RawContentPreviewProcess>(pool,Processes().State().Now(),gameAux_->SoundProcessType(),std::move(preview));
            }
            // 원문new18의 미작성+c는 현재 호스트 할당 바이트를 보존하고 생성자 쓰기로만 초기화한다.
            const std::unique_ptr<void,decltype(&std::free)> memory(std::malloc(24),&std::free);if (!memory) return false;
            o::ContentPreviewFields fields;std::memcpy(fields.bytes.data(),memory.get(),fields.bytes.size());
            try { pendingContentPreview_=std::make_unique<o::ContentPreviewProcess>(*contentPreviewLogic_,fields); }catch (const std::bad_alloc&) { return false; }return true;
        };
        hooks.constructTimer=[this](auto parent,auto duration) { if (!pendingContentPreview_ || !contentPreviewLogic_) throw std::logic_error("장기 내용물 타이머 생성 전 확보 메모리 누락");contentPreviewLogic_->Construct(pendingContentPreview_->Fields(),parent,duration); };
        carrierRegular_=std::make_unique<o::RawCarrierRegular>(pool,types,authority_->boss,std::move(hooks));
    }
    return *carrierRegular_;
}
}
