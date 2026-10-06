// 패치 004ad500/004990d0/00498ff0 ↔ CD 004ae380/00456a00/00456930.
#include "o/SquidDisplay.h"
#include "o/SquidUnpop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 표시 레이어와 extra의 선택/표식 비트다.
constexpr std::uint32_t kDoubleFrames=0x440000,kShadow=0x40000,kMarker=0x10;
// little endian raw 값을 읽으며 부족한 슬롯을 먼저 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("표시 raw 슬롯 범위 오류");
    std::uint32_t value=0;
    // CD의 1바이트 frame 옆 extra를 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 새 구현은 큰/손상된 입력의 정수 좌표 넘침을 거부한다.
int Coordinate(std::int64_t value) {
    if (value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::overflow_error("표시 좌표 넘침");
    return static_cast<int>(value);
}
// x87의 x*16/y*11+0.5 뒤 절삭이다. float 곱셈으로 먼저 반올림하지 않는다.
int Project(float value,double scale,int camera) {
    const auto pixel=static_cast<double>(value)*scale+0.5;
    if (!std::isfinite(pixel) || pixel<std::numeric_limits<int>::min() || pixel>std::numeric_limits<int>::max())
        throw std::out_of_range("표시 좌표 범위 오류");
    return Coordinate(static_cast<std::int64_t>(pixel)-camera);
}
// 원본 imul low DWORD와 산술 우측 이동을 정의된 비트 연산으로 보존한다.
int Scale(std::int16_t value,int zoom) {
    const auto product=static_cast<std::uint32_t>(static_cast<std::int64_t>(value)*zoom);
    return std::bit_cast<std::int32_t>(product)>>16;
}
// 원본은 교집합 계산 대신 각 모서리를 독립 clamp한다.
SquidDisplayRect Clip(SquidDisplayRect r,SquidDisplayRect v) {
    return {std::clamp(r.left,v.left,v.right),std::clamp(r.top,v.top,v.bottom),
        std::clamp(r.right,v.left,v.right),std::clamp(r.bottom,v.top,v.bottom)};
}
// 양의 면적만 변경 표에 보낸다.
bool Nonempty(SquidDisplayRect r) { return r.left<r.right && r.top<r.bottom; }
}
// 타입 번호 체계가 다른 목록의 혼용을 막는다.
SquidDisplay::SquidDisplay(OriginalEdition edition,std::span<const RiftTypeRecord> types,
    std::span<const SquidDisplayShape> shapes,SquidDisplaySink& sink,SquidDisplayView view)
    :edition_(edition),types_(types),shapes_(shapes),sink_(sink) {
    if (types.size()!=shapes.size()) throw std::invalid_argument("표시 타입/SHP 목록 크기 오류");
    SetView(view);
}
// 역전 viewport와 양수 아닌 확대율을 장치 호출 전에 거부한다.
void SquidDisplay::SetView(SquidDisplayView view) {
    if (view.zoom<=0 || view.viewport.left>view.viewport.right || view.viewport.top>view.viewport.bottom)
        throw std::invalid_argument("표시 카메라/viewport 오류");
    view_=view;
}
// 패치의 일반 경로에도 남아 있는 frameCheck 조건이다. debug/assert UI는 복원 범위 밖이다.
bool SquidDisplay::PatchFrameValid(std::size_t type,int frame) const {
    const auto& record=types_[type]; const auto& shape=shapes_[type];
    const auto count=static_cast<std::int64_t>(shape.frameCount)*((record.flags1&kDoubleFrames)!=0 ? 2 : 1);
    return frame>=0 && frame<count && shape.loaded && record.maxHitPoints!=-0x22222223 && record.maxHitPoints!=-0x32323233;
}
// 미복원 파생 update88/update8c는 표시가 꺼져도 지원한다고 가장하지 않는다.
void SquidDisplay::Validate(OriginalEdition edition,std::span<const std::uint8_t> bytes,std::uint32_t flags) const {
    if (edition!=edition_) throw std::invalid_argument("표시 판본 불일치");
    if (!SquidUnpop::SupportsDisplay(edition_,Read(bytes,0),flags)) throw std::logic_error("파생 표시 갱신 미복원");
    const auto type=Read(bytes,10,1);
    if (type>=types_.size()) throw std::out_of_range("표시 타입 번호 범위 오류");
    const bool patch=edition_==OriginalEdition::Patch1078;
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(bytes,36)) : static_cast<int>(Read(bytes,34,1));
    static_cast<void>(Read(bytes,patch ? 40 : 35,1));
    if ((patch && !PatchFrameValid(type,frame)) || sink_.Suppressed()) return;
    const auto& shape=shapes_[type];
    if (!shape.loaded) return;
    const auto last=static_cast<std::int64_t>(frame)+((types_[type].flags1&kShadow)!=0 ? shape.frameCount : 0);
    if (frame<0 || shape.frameCount<0 || last<0 || last>=static_cast<std::int64_t>(shape.frames.size()))
        throw std::out_of_range("표시 물리 프레임 범위 오류");
}
// VFX 압축 사각형 대신 별도 Squid 헤더의 pixel 크기와 hotspot을 사용한다.
SquidDisplayRect SquidDisplay::Bounds(std::size_t type,int frame,float x,float y) const {
    if (type>=types_.size()) throw std::out_of_range("표시 타입 번호 범위 오류");
    const auto& shape=shapes_[type];
    if (!shape.loaded) return {};
    if (frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("표시 프레임 범위 오류");
    const auto& header=shape.frames[static_cast<std::size_t>(frame)];
    const auto left=Coordinate(static_cast<std::int64_t>(Project(x,16,view_.cameraX))-Scale(header.hotspotX,view_.zoom));
    const auto top=Coordinate(static_cast<std::int64_t>(Project(y,11,view_.cameraY))-Scale(header.hotspotY,view_.zoom));
    return {left,top,Coordinate(static_cast<std::int64_t>(left)+Scale(header.width,view_.zoom)+1),
        Coordinate(static_cast<std::int64_t>(top)+Scale(header.height,view_.zoom)+1)};
}
// update8c는 update88로 가상 꼬리 호출하므로 지원 표에서 확인한 공통 경로를 공유한다.
void SquidDisplay::Update(std::span<const std::uint8_t> bytes,std::uint32_t flags) {
    Validate(edition_,bytes,flags);
    const bool patch=edition_==OriginalEdition::Patch1078; const auto type=Read(bytes,10,1);
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(bytes,36)) : static_cast<int>(Read(bytes,34,1));
    if ((patch && !PatchFrameValid(type,frame)) || sink_.Suppressed()) return;
    const auto x=std::bit_cast<float>(Read(bytes,14)),y=std::bit_cast<float>(Read(bytes,18));
    const auto marker=(Read(bytes,patch ? 40 : 35,1)&4)!=0 ? kMarker : 0;
    auto main=Clip(Bounds(type,frame,x,y),view_.viewport);
    if (marker) {
        if (static_cast<std::int64_t>(main.right)-main.left<19) { main.left=Coordinate(static_cast<std::int64_t>(main.left)-9); main.right=Coordinate(static_cast<std::int64_t>(main.right)+9); }
        main.top=Coordinate(static_cast<std::int64_t>(main.top)-(patch ? 15 : 9));
        main.left=Coordinate(static_cast<std::int64_t>(main.left)-3); main.right=Coordinate(static_cast<std::int64_t>(main.right)+3);
    }
    // clamp 뒤 확장한 범위는 다시 viewport로 자르지 않는다. Dirty/Draw 단계가 화면을 처리한다.
    if (Nonempty(main)) sink_.Invalidate(main,marker);
    if ((types_[type].flags1&kShadow)!=0) {
        const auto shadow=Clip(Bounds(type,frame+shapes_[type].frameCount,x,y),view_.viewport);
        if (Nonempty(shadow)) sink_.Invalidate(shadow,marker);
    }
}
}
