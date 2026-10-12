// 원본 WORD 감김·x87 저장 위치·CRT 절삭과 그림 소멸/체인 분배를 복원한다.
#include "client/Markup.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 배율 1의 float32 비트와 VFX 16.16 배율 상수다.
constexpr std::uint32_t kUnitScale=0x3f800000U;
constexpr double kFixedScale=65536.0;
// 원본 signed DWORD 좌표 덧셈을 하위 비트로 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second));
}
// 원본 타입 기준점은 하위 WORD만 빼고 다시 signed WORD로 해석한다.
std::int16_t SubtractWord(std::int16_t value,std::int32_t offset) {
    const auto result=static_cast<std::uint16_t>(std::uint16_t(value)-std::uint16_t(offset));
    return std::bit_cast<std::int16_t>(result);
}
// 원본 36바이트 추가 헤더의 little endian WORD를 signed 비트 그대로 읽는다.
std::int16_t HeaderWord(const std::array<std::uint8_t,36>& header,std::size_t offset) {
    const auto word=static_cast<std::uint16_t>(header.at(offset)|(std::uint16_t(header.at(offset+1))<<8));
    return std::bit_cast<std::int16_t>(word);
}
// 원본 _ftol의 signed 64비트 결과에서 하위 signed DWORD를 취한다. 무효 변환 결과의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}

// 실제 도달 시 필요한 조회/출력 경계를 검사한다. 생성/색 BYTE 설정에는 자산 경계가 없어도 된다.
Markup::Markup(StyleTextState& fragments,MarkupState& state,MarkupHooks hooks,o::OriginalEdition edition)
    :fragments_(fragments),state_(state),hooks_(std::move(hooks)),edition_(edition) {}

// 같은 공통 표의 현재 조각을 읽는다. 다른 기본 저장소 복사본을 만들지 않는다.
StyleTextFragment& Markup::Fragment(std::uint32_t token) const {
    const auto it=fragments_.fragments.find(token);
    if (!token || it==fragments_.fragments.end()) throw std::out_of_range("Markup 공통 조각 저장소 없음");
    return it->second;
}

// 공통 가상 분류가 그림인 동안에만 직접 파생 처리를 허용한다.
MarkupPicture& Markup::Picture(std::uint32_t token) const {
    const auto it=state_.pictures.find(token);
    if (it==state_.pictures.end() || Fragment(token).kind!=StyleTextFragmentKind::External)
        throw std::out_of_range("Markup 그림 조각 저장소 없음");
    return it->second;
}

// SHP는 조각이 소유하지 않는다. 공통/파생 map 할당 실패에서는 새 등록만 정리한다.
std::uint32_t Markup::NewPicture(std::uint32_t type,std::uint32_t frame,std::uint32_t metadata) const {
    StyleTextFragment fragment;
    fragment.kind=StyleTextFragmentKind::External;
    fragment.metadata=metadata;
    const auto token=RegisterStyleTextFragment(fragments_,std::move(fragment));
    try {
        if (!state_.pictures.emplace(token,MarkupPicture{type,frame,1,0}).second)
            throw std::runtime_error("Markup 파생 토큰 중복");
    }
    catch (...) { fragments_.fragments.erase(token);throw; }
    return token;
}

// 원본 clone은 기존 배율과 mapped BYTE를 복사하지 않는다.
std::uint32_t Markup::ClonePicture(std::uint32_t token) const {
    const auto& picture=Picture(token);
    return NewPicture(picture.type,picture.frame,Fragment(token).metadata);
}

// 원본 소멸자는 next/타입/배율을 지우지 않고 기본 가상 표로만 전환한다.
void Markup::Destruct(std::uint32_t token) const {
    Picture(token);
    Fragment(token).kind=StyleTextFragmentKind::Base;
}

// StyleText가 공통 조각을 scalar 반납하기 직전에 파생 저장소만 제거한다.
void Markup::Release(std::uint32_t token) const { state_.pictures.erase(token); }

// 반납 전까지는 Base로 전환한 그림도 소속 표에 남아 있다.
bool Markup::Contains(std::uint32_t token) const { return state_.pictures.contains(token); }

// bool로 정규화하지 않고 원본 BYTE 비트를 보관한다.
void Markup::SetMapped(std::uint32_t token,std::uint32_t mapped) const { Picture(token).mapped=static_cast<std::uint8_t>(mapped); }

// 타입 범위/shape/현재 frame 검사를 통과한 경우에만 원본 헤더 조회에 도달한다.
MarkupFrameView Markup::Lookup(std::uint32_t token) const {
    const auto type=std::bit_cast<std::int32_t>(Picture(token).type);
    MarkupFrameView result;
    if (type<=0 || type>=state_.typeCount) return result;
    if (!hooks_.type) throw std::invalid_argument("Markup 현재 타입 조회 경계 필요");
    const auto* view=hooks_.type(static_cast<std::uint32_t>(type));
    if (!view || !view->shape) return result;
    const auto frame=std::bit_cast<std::int32_t>(Picture(token).frame);
    if (frame<0 || frame>=view->frameCount) return result;
    if (!hooks_.frame) throw std::invalid_argument("Markup 현재 SHP 헤더 조회 경계 필요");
    result.frame=hooks_.frame(view->token,0,static_cast<std::uint32_t>(frame));
    result.frame.hotspotX=SubtractWord(result.frame.hotspotX,view->hotspotX);
    result.frame.hotspotY=SubtractWord(result.frame.hotspotY,view->hotspotY);
    result.type=static_cast<std::uint32_t>(type);
    result.shape=view->shape;
    return result;
}

// 패치의 세로 float32 저장과 CD의 unordered 분기를 단순한 std::min으로 바꾸지 않는다.
void Markup::Fit(std::uint32_t token,std::int32_t width,std::int32_t height) const {
    const auto frame=Lookup(token).frame;
    const double widthRatio=static_cast<double>(width)/frame.width;
    const double heightRatio=static_cast<double>(height)/frame.height;
    double selected;
    if (edition_==o::OriginalEdition::Patch1078) {
        const auto storedHeight=static_cast<float>(heightRatio);
        selected=widthRatio<storedHeight ? widthRatio : static_cast<double>(storedHeight);
        if (selected<1.0) Picture(token).scale=static_cast<float>(selected);
    } else {
        selected=widthRatio>heightRatio ? heightRatio : widthRatio;
        if (!(selected>=1.0)) Picture(token).scale=static_cast<float>(selected);
    }
}

// 원본 가상 +4는 폭 대신 그림 높이와 보정된 y 기준점을 반환한다.
std::array<std::int32_t,2> Markup::Baseline(std::uint32_t token) const {
    const auto frame=Lookup(token).frame;
    const auto scale=Picture(token).scale;
    if (std::bit_cast<std::uint32_t>(scale)==kUnitScale) return {frame.height,frame.hotspotY};
    return {Ftol(static_cast<double>(frame.height)*scale),Ftol(static_cast<double>(frame.hotspotY)*scale)};
}

// 배율 1에서는 signed WORD를 그대로 사용하며 그 외에는 원본 CRT 절삭을 적용한다.
std::array<std::int32_t,2> Markup::Measure(std::uint32_t token) const {
    const auto frame=Lookup(token).frame;
    const auto scale=Picture(token).scale;
    if (std::bit_cast<std::uint32_t>(scale)==kUnitScale) return {frame.width,frame.height};
    const std::array<std::int32_t,2> result{Ftol(static_cast<double>(frame.width)*scale),Ftol(static_cast<double>(frame.height)*scale)};
    // CD는 두 출력을 쓴 뒤 현재 배율로 폭을 다시 절삭해 처음 계산한 폭과 비교한다.
    if (edition_==o::OriginalEdition::Cd1072 && Ftol(static_cast<double>(frame.width)*Picture(token).scale)!=result[0])
        throw std::runtime_error("Markup CD 배율 폭 재확인 오류");
    return result;
}

// 색 설치로 바뀐 현재 frame은 다시 읽지만 이미 계산한 기준점/16.16 배율과 현재 SHP는 유지한다.
void Markup::Draw(std::uint32_t token,ScreenPoint position) const {
    const auto view=Lookup(token);
    if (!view.shape) return;
    const auto scale=Picture(token).scale;
    MarkupShapeRequest request;
    request.type=view.type;
    request.shape=view.shape;
    request.scaled=std::bit_cast<std::uint32_t>(scale)!=kUnitScale;
    if (request.scaled) {
        request.position={Ftol(static_cast<double>(view.frame.hotspotX)*scale+position.x),
            Ftol(static_cast<double>(view.frame.hotspotY)*scale+position.y)};
        request.scaleX=Ftol(static_cast<double>(scale)*kFixedScale);
        request.scaleY=request.scaleX;
    } else request.position={Add(position.x,view.frame.hotspotX),Add(position.y,view.frame.hotspotY)};
    request.mapped=Picture(token).mapped!=0;
    if (request.mapped) {
        if (!hooks_.color) throw std::invalid_argument("Markup 그림 변환 색 표 설치 경계 필요");
        hooks_.color();
    }
    request.frame=Picture(token).frame;
    if (!hooks_.shape) throw std::invalid_argument("Markup VFX 그림 출력 경계 필요");
    hooks_.shape(request);
}

// 기존 다른 외부 조각 분배는 해당 경계에 실제 도달할 때만 요구한다.
void BindMarkup(StyleTextHooks& hooks,const Markup& markup) {
    const auto metrics=hooks.externalMetrics;
    const auto measure=hooks.externalMeasure;
    const auto draw=hooks.externalDraw;
    const auto destroy=hooks.externalDestroy;
    const auto release=hooks.release;
    // 현재 그림 토큰이면 원본 +4를 처리하고 다른 파생 토큰은 기존 분배를 사용한다.
    hooks.externalMetrics=[&markup,metrics](std::uint32_t token) {
        if (markup.Contains(token)) return markup.Baseline(token);
        if (!metrics) throw std::invalid_argument("Markup 외 조각 메트릭 경계 필요");
        return metrics(token);
    };
    // 현재 그림의 원본 +8을 전체 StyleText 측정에 공급한다.
    hooks.externalMeasure=[&markup,measure](std::uint32_t token) {
        if (markup.Contains(token)) return markup.Measure(token);
        if (!measure) throw std::invalid_argument("Markup 외 조각 측정 경계 필요");
        return measure(token);
    };
    // 그림 가상 출력은 전달된 텍스트 override 문맥을 사용하지 않는다.
    hooks.externalDraw=[&markup,draw](std::uint32_t token,ScreenPoint position,const std::array<std::uint32_t,8>* context) {
        if (markup.Contains(token)) markup.Draw(token,position);
        else {
            if (!draw) throw std::invalid_argument("Markup 외 조각 출력 경계 필요");
            draw(token,position,context);
        }
    };
    // 그림의 Base 전환 이후 scalar 반납은 원래 StyleText 처리기에 남긴다.
    hooks.externalDestroy=[&markup,destroy](std::uint32_t token,std::uint8_t flags) {
        if (markup.Contains(token)) markup.Destruct(token);
        else {
            if (!destroy) throw std::invalid_argument("Markup 외 조각 삭제 경계 필요");
            destroy(token,flags);
        }
    };
    // 기존 반납 통지를 먼저 호출해 소멸 후 필드를 관찰할 수 있도록 한다.
    hooks.release=[&markup,release](std::uint32_t token) {
        if (release) release(token);
        markup.Release(token);
    };
}

// 원본 타입 표/자산 로딩 순서와 공유 SHP의 대응을 읽기 전용으로 준비한다.
MarkupAssets::MarkupAssets(const GameAssets& assets):shapes_(assets.Shapes()) {
    const auto types=assets.TypeTable().Types();
    if (types.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::length_error("Markup 타입 수 범위 초과");
    typeCount_=static_cast<std::int32_t>(types.size());
    // 자산이 없는 내장 타입은 type 조회 시 null로 공급한다.
    for (std::size_t i=0;i<assets.Types().size();++i) {
        const auto token=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+i);
        const auto& asset=assets.Types()[i];
        if (token>=types.size() || asset.block>=shapes_.Blocks().size() ||
            asset.definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw std::out_of_range("Markup 실제 자산 바인딩 범위 오류");
        const auto count=static_cast<std::int32_t>(asset.definition.clusters.size());
        views_.emplace(token,View{{token,token,count,types[token].hotspotX,types[token].hotspotY},asset.block});
    }
}

// 원본과 같은 signed 타입 수를 반환한다.
std::int32_t MarkupAssets::TypeCount() const { return typeCount_; }

// 안정된 타입 주소와 현재 실제 자산 유무를 공급한다.
const MarkupType* MarkupAssets::Type(std::uint32_t token) const {
    const auto it=views_.find(token);
    return it==views_.end() ? nullptr : &it->second.type;
}

// 현재 논리 프레임 헤더만 읽으며 추가 레이어로 자동 전환하지 않는다.
MarkupFrame MarkupAssets::Frame(std::uint32_t type,std::uint32_t abstract,std::uint32_t frame) const {
    const auto* view=Type(type);
    if (abstract || !view || frame>=static_cast<std::uint32_t>(view->frameCount)) throw std::out_of_range("Markup 논리 프레임 범위 오류");
    const auto header=shapes_.SquidHeader(views_.at(type).block,frame);
    return {HeaderWord(header,0x18),HeaderWord(header,0x1a),HeaderWord(header,0x1c),HeaderWord(header,0x1e),
        HeaderWord(header,0x20),HeaderWord(header,0x22)};
}

// VFX 출력은 SHP의 실제 물리 프레임 인덱스로 조회한다.
std::pair<const ShapeDatabase*,std::size_t> MarkupAssets::Shape(std::uint32_t shape,std::uint32_t frame) const {
    const auto it=views_.find(shape);
    if (it==views_.end() || frame>=shapes_.Blocks()[it->second.block].frames.size()) throw std::out_of_range("Markup VFX 물리 프레임 범위 오류");
    return {&shapes_,it->second.block};
}

// 생성 후에도 각 Lookup에서 동일한 현재 타입/헤더 조회를 사용한다.
void BindMarkupAssets(MarkupState& state,MarkupHooks& hooks,const MarkupAssets& assets) {
    state.typeCount=assets.TypeCount();
    // 현재 타입의 안정된 주소를 공급한다.
    hooks.type=[&assets](std::uint32_t type) { return assets.Type(type); };
    // abstract=0 및 현재 논리 프레임을 같은 SHP 메타데이터로 연결한다.
    hooks.frame=[&assets](std::uint32_t type,std::uint32_t abstract,std::uint32_t frame) { return assets.Frame(type,abstract,frame); };
}

// 그림은 SHP를 소유하지 않고 해제한 일반 픽셀만 캐시한다.
MarkupPixelTarget::MarkupPixelTarget(const GumpPixelTarget& target,const MarkupAssets& assets,ColorMap& currentColors,const ColorMap& pictureColors)
    :target_(target),assets_(assets),colors_(currentColors),pictureColors_(pictureColors) {}

// 일반 원색/색 치환은 현재 UI 표면에 연결하고 scaled 경계는 호출자가 공급한 원본 커널 분배를 보존한다.
MarkupHooks MarkupPixelTarget::Hooks(MarkupHooks hooks) {
    const auto scaled=hooks.shape;
    // 그림에 지정된 원본 256바이트 변환 표를 현재 공유 VFX 표에 설치한다.
    hooks.color=[this] { colors_=pictureColors_; };
    // 확대/축소의 픽셀 선택을 임의 보간으로 대체하지 않고 원본 요청을 기존 경계에 전달한다.
    hooks.shape=[this,scaled](MarkupShapeRequest request) {
        if (!request.scaled) Image(request);
        else {
            if (!scaled) throw std::invalid_argument("Markup 확대/축소 VFX 커널 경계 필요");
            scaled(request);
        }
    };
    return hooks;
}

// 보정된 표시 기준점 뒤에 실제 VFX 프레임의 상대 픽셀 좌표를 더한다.
void MarkupPixelTarget::Image(MarkupShapeRequest request) {
    const auto [database,block]=assets_.Shape(request.shape,request.frame);
    const auto& frame=database->Blocks()[block].frames[request.frame];
    if (frame.IsSpecial()) return;
    const auto key=std::make_tuple(database,block,static_cast<std::size_t>(request.frame));
    auto it=images_.find(key);
    if (it==images_.end()) it=images_.emplace(key,database->Decode(block,request.frame)).first;
    target_.Image(it->second,{Add(request.position.x,frame.rect.left),Add(request.position.y,frame.rect.top)},request.mapped ? &colors_ : nullptr);
}
}
