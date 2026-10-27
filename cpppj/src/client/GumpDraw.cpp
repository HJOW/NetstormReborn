// Gump는 클립을 복구하지만 VFX 색 문맥이나 하위 객체 상태를 되돌리지 않는다.
#include "client/GumpDraw.h"
#include "client/VFXTransform.h"
#include "client/VFXLine.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <string>

namespace netstorm::client {
namespace {
// 원본 right/bottom -1은 signed overflow 없이 하위 DWORD를 보존한다.
int Previous(int value) { return std::bit_cast<std::int32_t>(std::uint32_t(value)-1U); }
}
// 화면 half-open 전역과 VFX inclusive 전역의 동시 쓰기다. 빈/역전 입력도 그대로 보존한다.
void SetGumpDrawClip(GumpDrawState& state,ScreenRect clip) {
    state.clip=clip;state.vfxClip={clip.left,clip.top,Previous(clip.right),Previous(clip.bottom)};
}
// 실제 ClipGump 임시 객체처럼 논리 영역만 저장하고 독립적으로 제한한 네 좌표를 설정한다.
GumpClipScope::GumpClipScope(GumpDrawState& state,ScreenRect bounds):state_(state),saved_(state.clip) {
    SetGumpDrawClip(state_,ClipScreenRect(bounds,saved_));
}
// 가상 Draw가 바꾼 현재 클립 대신 저장한 논리 영역으로 복구한다.
GumpClipScope::~GumpClipScope() { SetGumpDrawClip(state_,saved_); }
// 없는 가상 처리기는 생성 시 거부해 일부 클립 쓰기 뒤 실패하는 일을 막는다.
GumpDraw::GumpDraw(const GumpCollection& collection,GumpDrawState& state,GumpDrawHooks hooks)
    :collection_(collection),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.draw) throw std::invalid_argument("Gump 가상 Draw 필요");
}
// 플래그가 없는 Gump는 위치 캐시와 클립을 건드리지 않는다. 가상 처리기의 클립 변경도 그대로 남는다.
void GumpDraw::Draw(std::uint32_t token) const {
    if (collection_.Flags(token)&kGumpDrawClip) {
        const auto bounds=collection_.Bounds(token);const GumpClipScope guard(state_,bounds);
        hooks_.draw(token);return;
    }
    hooks_.draw(token);
}
// UI 토큰을 직접 받는 공통 분배에 연결한다. 이후 프레임에서도 현재 트리/상태를 조회한다.
void BindGumpDraw(DrawDispatchHooks& hooks,const GumpDraw& draw) { hooks.ui=[&draw](auto token) { draw.Draw(token); }; }
// 픽셀 표면의 용량은 64비트에서 검사하며 행 간격 밖 쓰기를 막는다.
GumpPixelTarget::GumpPixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,const GumpDrawState& state)
    :pixels_(pixels),pitch_(pitch),height_(height),state_(state) {
    if (pitch<1 || height<1 || std::uint64_t(pitch)*std::uint64_t(height)>pixels.size())
        throw std::invalid_argument("Gump 픽셀 표면 용량 오류");
}
// inclusive 끝점 +1은 int 범위에서 감기지 않게 계산한 뒤 실제 표면 안으로 제한한다.
ScreenRect GumpPixelTarget::Clip() const {
    const auto clip=state_.vfxClip;
    return {static_cast<int>(std::clamp<std::int64_t>(clip.left,0,pitch_)),
        static_cast<int>(std::clamp<std::int64_t>(clip.top,0,height_)),
        static_cast<int>(std::clamp<std::int64_t>(std::int64_t(clip.right)+1,0,pitch_)),
        static_cast<int>(std::clamp<std::int64_t>(std::int64_t(clip.bottom)+1,0,height_))};
}
// 선의 양 끝은 포함하며 색 번호는 실제 표면의 BYTE로 자른다.
void GumpPixelTarget::Lines(std::span<const GumpLine> lines) const {
    // 입력 순서를 유지해 모서리와 겹친 선을 뒤 색이 덮게 한다.
    for (const auto& line:lines) {
        if (line.first.x!=line.last.x && line.first.y!=line.last.y) {
            DrawVfxLine(pixels_,pitch_,height_,Clip(),line.first,line.last,line.color);continue;
        }
        const auto clip=Clip();
        const auto left=std::max<std::int64_t>(clip.left,std::min(line.first.x,line.last.x));
        const auto top=std::max<std::int64_t>(clip.top,std::min(line.first.y,line.last.y));
        const auto right=std::min<std::int64_t>(clip.right,std::int64_t(std::max(line.first.x,line.last.x))+1);
        const auto bottom=std::min<std::int64_t>(clip.bottom,std::int64_t(std::max(line.first.y,line.last.y))+1);
        if (left>=right || top>=bottom) continue;
        // 유효 행만 팔레트 번호로 채운다. 모든 주소가 실제 표면 범위 안에 있다.
        for (auto y=top;y<bottom;++y) std::fill(pixels_.begin()+y*pitch_+left,pixels_.begin()+y*pitch_+right,static_cast<std::uint8_t>(line.color));
    }
}
// 기존 실제 VFX 해제/투명 합성기를 같은 현재 클립에 연결한다.
void GumpPixelTarget::Image(const IndexedImage& image,ScreenPoint position,const ColorMap* colors) const {
    DrawIndexedImage(pixels_,pitch_,height_,image,position.x,position.y,Clip(),colors);
}
// 같은 표면을 임시 클립으로 빌려 bitmap보다 넓은 Gump의 외곽선도 유지한다.
void GumpPixelTarget::LinesClipped(std::span<const GumpLine> lines,ScreenRect bounds) const {
    GumpDrawState clipped=state_;SetGumpDrawClip(clipped,ClipScreenRect(bounds,Clip()));
    GumpPixelTarget(pixels_,pitch_,height_,clipped).Lines(lines);
}
// 대상 클립을 먼저 계산하므로 가려진 소스 guard는 읽지 않으며 음수 X의 유효 flat 주소도 허용한다.
void GumpPixelTarget::RawBitmap(std::span<const std::uint8_t> source,std::int32_t stride,std::int64_t sourceBase,
    ScreenPoint size,ScreenPoint destination,ScreenRect bounds,
    const std::function<std::uint8_t(std::int64_t)>& outside) const {
    if (stride<=0) throw std::invalid_argument("원시 bitmap의 양수 행 간격 필요");
    if (size.x<=0 || size.y<=0) return;
    const auto clip=ClipScreenRect(bounds,Clip()); // 실제 화면과 전체 Gump 경계를 먼저 교차한다.
    const auto left=std::max<std::int64_t>(clip.left,destination.x),top=std::max<std::int64_t>(clip.top,destination.y);
    const auto right=std::min<std::int64_t>(clip.right,std::int64_t(destination.x)+size.x);
    const auto bottom=std::min<std::int64_t>(clip.bottom,std::int64_t(destination.y)+size.y);
    // 보이는 행만 순회하며 원본 stride를 유지한다. 창의 남은 여백은 기존 픽셀을 보존한다.
    for (auto y=top;y<bottom;++y) {
        // 평면 소스의 X가 음수여도 최종 BYTE 주소가 할당 안이면 그대로 읽는다.
        for (auto x=left;x<right;++x) {
            const auto delta=(y-destination.y)*stride+(x-destination.x); // 비음수 대상 상대 좌표의 평면 이동량이다.
            if (sourceBase>std::numeric_limits<std::int64_t>::max()-delta) throw std::overflow_error("원시 bitmap 소스 주소 넘침");
            const auto offset=sourceBase+delta;std::uint8_t value{};
            if (offset>=0 && static_cast<std::uint64_t>(offset)<source.size()) value=source[static_cast<std::size_t>(offset)];
            else {
                if (!outside) throw std::out_of_range("원시 bitmap의 보이는 할당 밖 BYTE 공급 필요: 주소="+std::to_string(offset)+", 할당="+std::to_string(source.size())+", 시작="+std::to_string(sourceBase)+", 크기="+std::to_string(size.x)+"x"+std::to_string(size.y)+", 목적="+std::to_string(destination.x)+","+std::to_string(destination.y)); //실제 실패한 평면 주소와 기하를 남기며 할당 밖 값을 만들어 대체하지 않는다.
                value=outside(offset);
            }
            pixels_[static_cast<std::size_t>(y)*pitch_+static_cast<std::size_t>(x)]=value;
        }
    }
}
// 원본 변환 커널의 직교 배율 분배도 일반 그림과 같은 현재 클립/대상 저장소를 사용한다.
void GumpPixelTarget::ScaledImage(const IndexedImage& image,const ShapeFrame& frame,ScreenPoint position,
    std::int32_t scaleX,std::int32_t scaleY,const ColorMap* colors) const {
    DrawScaledVfxImage(pixels_,pitch_,height_,image,frame,position,scaleX,scaleY,Clip(),colors);
}
// 원본 004a2060/004a2180의 소프트웨어 픽셀 결과다. 빈/역전 영역은 호스트에서 쓰지 않는다.
void GumpPixelTarget::Fill(ScreenRect rect,std::uint32_t color,bool checker) const {
    const auto visible=ClipScreenRect(rect,Clip());const auto ink=static_cast<std::uint8_t>(color);
    if (visible.left>=visible.right || visible.top>=visible.bottom) return;
    // 실제 표면 범위 안의 행만 갱신하며 체크 위상은 잘린 좌표의 절대 홀짝을 따른다.
    for (int y=visible.top;y<visible.bottom;++y) {
        const auto row=static_cast<std::size_t>(y)*static_cast<std::size_t>(pitch_);
        if (!checker) std::fill(pixels_.begin()+row+visible.left,pixels_.begin()+row+visible.right,ink);
        else {
            // 체크 fill의 (~y^left)&1과 같은 화면 위상이다. 오른/아래 경계는 제외한다.
            for (int x=visible.left;x<visible.right;++x) if ((std::uint32_t(x)^std::uint32_t(y))&1U) pixels_[row+x]=ink;
        }
    }
}
}
