// 화면 사각형/표시사본/모바일 여유 대신 실제 선택용 그림의 색255를 판정한다.
#include "client/RawObjectHit.h"
#include "client/RawCameraView.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장치에 의존하지 않는 원본 선택 몸체다.
namespace { // 비정렬 읽기와 x86 signed 감김만 공용화한다.
std::uint32_t HitDword(std::span<const std::uint8_t> raw,std::size_t offset) { // 지정한 네 원시 바이트를 little endian으로 읽는다.
    std::uint32_t value{}; // 원래 DWORD의 비트다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[offset+byte])<<(8*byte); // 정렬/이웃 슬롯을 가정하지 않는다.
    return value;
}
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)-std::bit_cast<std::uint32_t>(b)); } // x86 sub의 하위DWORD를 보존한다.
template<class F> const F& Required(const F& callback) { if (!callback) throw std::logic_error("픽셀 선택의 도달한 가상/출력 하위 미연결");return callback; } // opaque/음수 분기에는 출력 경계를 요구하지 않는다.
}
RawObjectHit::RawObjectHit(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const o::SquidDisplayShape> shapes,ObjectHitHooks hooks)
    :pool_(pool),types_(types),shapes_(shapes),hooks_(std::move(hooks)) { // 같은 원문 판본/TYPE 번호만 받는다.
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188 || shapes.size()!=188) throw std::invalid_argument("픽셀 선택의10.78 TYPE/SHP 계약 오류");
}
bool RawObjectHit::Hit(o::Sid sid,std::int32_t x,std::int32_t y,const std::array<std::int32_t,2>& camera) const { // getter/그림 호출을 원문보다 앞당기지 않는다.
    const auto raw=pool_.Slot(sid);const auto type=raw[10]; // 첫 TYPE snapshot과 현재 raw를 구별한다.
    if (type>=types_.size()) throw std::out_of_range("픽셀 선택의 현재TYPE 범위");
    const auto& record=types_[type];const auto& shape=shapes_[type]; // 첫 TYPE가 공급하는 실제 헤더 목록이다.
    const auto first=Required(hooks_.frame)(sid);const auto frame=first==-1 ? std::bit_cast<std::int32_t>(HitDword(raw,36)) : hooks_.frame(sid); // 원문은−1이 아닐 때+78을 다시 호출한다.
    const auto limit=std::int64_t(shape.frameCount)*((record.flags1&0x440000U) ? 2 : 1); // 그림자/추상 물리표의 원본 허용 범위다.
    if (!shape.loaded || frame<0 || frame>=limit || static_cast<std::size_t>(frame)>=shape.frames.size() || std::bit_cast<std::uint32_t>(record.maxHitPoints)==0xddddddddU || std::bit_cast<std::uint32_t>(record.maxHitPoints)==0xcdcdcdcdU) throw std::logic_error("픽셀 선택의 frameCheck 원본 진단");
    const auto header=shape.frames[static_cast<std::size_t>(frame)]; // 추가Draw 이후에도 같은 signed 헤더/상대점을 쓴다.
    const auto point=ProjectCameraCoordinates(camera[0],camera[1],std::bit_cast<float>(HitDword(raw,14)),std::bit_cast<float>(HitDword(raw,18)));
    const auto rx=Subtract(x,Subtract(Subtract(point[0],record.hotspotX),header.hotspotX));
    const auto ry=Subtract(y,Subtract(Subtract(point[1],record.hotspotY),header.hotspotY));
    if (rx<0 || ry<0) return false;
    if (record.flags1&0x800000U) return rx<header.width && ry<header.height;
    std::array<std::uint8_t,200*200> pixels;pixels.fill(255); // 4a2b10의 고정 버퍼이며 일반 캡처 크기/색표를 재사용하지 않는다.
    if (!(raw[40]&2)) Required(hooks_.plain)(type,std::bit_cast<std::int32_t>(HitDword(raw,36)),header.hotspotX,header.hotspotY,pixels);
    else {
        const auto main=hooks_.frame(sid);if (main!=-1) Required(hooks_.plain)(type,main,header.hotspotX,header.hotspotY,pixels);
        Required(hooks_.additional)(sid,header.hotspotX,header.hotspotY,pixels);
    }
    return rx<200 && ry<200 && pixels[static_cast<std::size_t>(ry)*200+static_cast<std::size_t>(rx)]!=255; // 바깥점도 먼저 그리며 불투명색255는hit이 아니다.
}
}
