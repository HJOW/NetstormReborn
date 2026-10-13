// 실제 배경 출력 분기와 현재 자산/클립/색을 보존하고 원본 호출 수명을 같은 저장소로 연결한다.
#include "client/GumpBackgroundDraw.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 좌표/디버그 색 덧셈의 하위 DWORD 감김을 유지한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 실제 도달한 분기의 빠진 경계를 명확한 호스트 오류로 거부한다.
template<class Function> const Function& Require(const Function& function) {
    if (!function) throw std::invalid_argument("배경 Draw 출력/자산 경계 필요");return function;
}
// 질감의 논리 클립만 복구한다. 색 문맥과 이미 출력한 픽셀은 복구하지 않는다.
class TextureClipGuard {
public:
    // 초기 설정이 실패할 때도 원래 클립을 복구한다. 실제 clip 콜백은 예외 없이 두 전역을 쓰는 것이 계약이다.
    TextureClipGuard(const std::function<void(ScreenRect)>& set,ScreenRect saved,ScreenRect next):set_(set),saved_(saved) {
        try { set_(next); } catch (...) { set_(saved_);throw; }
    }
    // 출력 경계가 실패해도 같은 현재 저장소의 half-open/VFX 클립을 복구한다.
    ~TextureClipGuard() { set_(saved_); }
private:
    // set_: 현재 클리핑 영역을 출력 대상에 적용하는 콜백.
    const std::function<void(ScreenRect)>& set_;
    // saved_: 화면 좌표의 사각형. right·bottom은 포함하지 않는다(원본 004a1800의 인자) (ScreenRect).
    ScreenRect saved_;
};
}
// 원본의 흰색(+7)/회색(+8) 전역을 그대로 연결하고 debug 상태/색 번호는 보존한다.
void SetGumpBackgroundColors(GumpBackgroundDrawState& state,const PaletteColorTable& palette) {
    state.nearColor=palette.basic[7];state.farColor=palette.basic[8];
}
// 노드 조회만 생성 시 필수다. 단색/체크/무배경은 타입/프레임 경계 없이 실행할 수 있다.
GumpBackgroundDraw::GumpBackgroundDraw(const GumpCollection& collection,GumpDrawState& clip,GumpBackgroundDrawState& state,GumpBackgroundDrawHooks hooks)
    :collection_(collection),clip_(clip),state_(state),hooks_(std::move(hooks)) {
    Require(hooks_.node);
}
// Bounds는 같은 Position 캐시/현재 배율 크기와 CRT 절삭을 사용한다. debugColor는 fill 호출 전에 감겨 증가한다.
void GumpBackgroundDraw::Draw(std::uint32_t token) const {
    const auto rect=collection_.Bounds(token);
    if (state_.debug) { const auto color=state_.debugColor++;Require(hooks_.fill)(rect,color,true);return; }
    const auto* node=Require(hooks_.node)(token);
    if (!node) throw std::runtime_error("배경 Draw 토큰/저장소 없음");
    Background(rect,node->backgroundFlags,node->type,node->frame);
}
// 단색/체크/무배경의 선 순서는 왼쪽→위→오른쪽→아래다. pressed는 이 경로의 선 색을 바꾸지 않는다.
void GumpBackgroundDraw::Background(ScreenRect rect,std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const {
    using namespace GumpBackgroundFlags;
    if (flags&kChecker) Require(hooks_.fill)(rect,flags&255U,true);
    else if (flags&kSolid) Require(hooks_.fill)(rect,flags&255U,false);
    else if (flags&kTexture) {
        Texture(rect,flags,type,frame);const auto saved=clip_.clip;
        const auto nearShade=(flags&kPressed) ? kDark : kBright,farShade=(flags&kPressed) ? kBright : kDark;
        if (flags&kLeft) Texture(ClipScreenRect({rect.left,rect.top,Add(rect.left,2),rect.bottom},clip_.clip),flags|nearShade,type,frame);
        if (flags&kRight) Texture(ClipScreenRect({Add(rect.right,-2),rect.top,rect.right,rect.bottom},saved),flags|farShade,type,frame);
        if (flags&kTop) Texture(ClipScreenRect({rect.left,rect.top,rect.right,Add(rect.top,2)},saved),flags|nearShade,type,frame);
        if (flags&kBottom) Texture(ClipScreenRect({rect.left,Add(rect.bottom,-2),rect.right,rect.bottom},saved),flags|farShade,type,frame);
        return;
    }
    if (flags&kLeft) Require(hooks_.line)({{rect.left,rect.top},{rect.left,Add(rect.bottom,-1)},state_.nearColor});
    if (flags&kTop) Require(hooks_.line)({{rect.left,rect.top},{Add(rect.right,-1),rect.top},state_.nearColor});
    if (flags&kRight) Require(hooks_.line)({{Add(rect.right,-1),rect.top},{Add(rect.right,-1),Add(rect.bottom,-1)},state_.farColor});
    if (flags&kBottom) Require(hooks_.line)({{rect.left,Add(rect.bottom,-1)},{Add(rect.right,-1),Add(rect.bottom,-1)},state_.farColor});
}
// asset 조회가 현재 클립을 바꿀 수 있으므로 조회 뒤 저장한다. 잘못된 타일 크기는 clip/map/shape를 건드리지 않는다.
void GumpBackgroundDraw::Texture(ScreenRect rect,std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const {
    const auto metrics=Require(hooks_.frame)(type,frame);const auto saved=clip_.clip;
    const auto plan=PlanGumpTexture(rect,saved,metrics.width,metrics.height,flags);
    if (!plan.validTile) return;
    const TextureClipGuard guard(Require(hooks_.clip),saved,plan.clip);
    if (plan.shade) Require(hooks_.shade)(plan.shade);
    // 빈 클립에서도 원본의 모든 여분 타일 호출을 전달한다. 원색은 현재 설치된 색 표를 무시한다.
    for (const auto origin:plan.origins) Require(hooks_.shape)({type,frame,origin,plan.shade!=0});
}
// 기존 돌 버튼/다른 UI 가상 처리기를 보존하면서 같은 배경 저장소만 선택한다.
void BindGumpBackgroundDraw(GumpDrawHooks& hooks,const GumpBackgroundDraw& draw,std::function<bool(std::uint32_t)> isBackground) {
    if (!isBackground) throw std::invalid_argument("배경 Draw 분배 식별기 필요");const auto previous=hooks.draw;
    hooks.draw=[&draw,isBackground,previous](std::uint32_t token) {
        if (isBackground(token)) draw.Draw(token);else Require(previous)(token);
    };
}
// 원본 전체 타입 번호와 물리 SHP 로딩 순서를 유지한다. 논리 프레임 수는 클러스터 수다.
GumpBackgroundAssets::GumpBackgroundAssets(const GameAssets& assets):shapes_(assets.Shapes()) {
    const auto types=assets.TypeTable().Types();
    // 내장 타입은 SHP 자산이 없으므로 생성 경계에서 실제 자산 타입만 제공한다.
    for (std::size_t i=0;i<assets.Types().size();++i) {
        const auto& asset=assets.Types()[i];const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+i);
        if (asset.definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("배경 논리 프레임 수 범위 오류");
        Add({type,asset.block,static_cast<std::int32_t>(asset.definition.clusters.size()),types[type].flags1});
    }
}
// 독립 데이터베이스도 실제 GameAssets와 같은 헤더/프레임 검증 경로를 사용한다.
GumpBackgroundAssets::GumpBackgroundAssets(const ShapeDatabase& shapes,std::span<const GumpBackgroundAssetBinding> bindings):shapes_(shapes) {
    // 바인딩 순서를 바꿔도 타입 번호/블록 대응은 입력대로 유지한다.
    for (const auto binding:bindings) Add(binding);
}
// 메타데이터는 안정된 자체 표로 복사하지만 VFX 픽셀은 원본 데이터베이스에서 조회한다.
void GumpBackgroundAssets::Add(GumpBackgroundAssetBinding binding) {
    if (!binding.type || views_.contains(binding.type) || binding.block>=shapes_.Blocks().size() || binding.frameCount<1 ||
        static_cast<std::size_t>(binding.frameCount)>shapes_.Blocks()[binding.block].frames.size()) throw std::invalid_argument("배경 자산 바인딩 오류");
    View view{{binding.type,binding.type},binding.block,binding.frameCount,binding.flags,{}};
    // 추가 레이어/공유 프레임을 포함한 실제 물리 헤더를 읽어 signed WORD 폭/높이를 보존한다.
    for (std::size_t frame=0;frame<shapes_.Blocks()[binding.block].frames.size();++frame) {
        const auto metrics=shapes_.SquidMetrics(binding.block,frame);view.frames.push_back({metrics.width,metrics.height});
    }
    views_.emplace(binding.type,std::move(view));
}
// 없는 타입의 raw/null shape 접근을 호스트 예외로 바꾼다. 기존 원본 파일에는 쓰지 않는다.
const GumpBackgroundType& GumpBackgroundAssets::Type(std::uint32_t type) const {
    const auto found=views_.find(type);if (found==views_.end()) throw std::out_of_range("배경 자산 타입 없음");return found->second.type;
}
// background는 abstract=0만 사용한다. signed frame 범위와 원본의 추가 레이어 허용 비트를 먼저 검사한다.
const GumpBackgroundFrame& GumpBackgroundAssets::Frame(std::uint32_t type,std::uint32_t abstract,std::uint32_t frame) const {
    if (abstract) throw std::invalid_argument("배경 자산 abstract 인자는 0이어야 함");
    Type(type);const auto& view=views_.at(type);
    const auto maximum=std::uint64_t(view.count)*((view.flags&0x440000U) ? 2U : 1U);
    if (frame>=maximum || frame>=view.frames.size()) throw std::out_of_range("배경 SHP 프레임 범위 오류");return view.frames[frame];
}
// 프레임 계약은 모든 실제 출력에서 다시 검사한다. 노드에 저장한 이전 shape 토큰은 사용하지 않는다.
std::pair<const ShapeDatabase*,std::size_t> GumpBackgroundAssets::Shape(std::uint32_t type,std::uint32_t frame) const {
    Frame(type,0,frame);return {&shapes_,views_.at(type).block};
}
// 생성과 출력 모두 같은 타입/헤더를 읽되 raw 생성자는 판본별 필드 쓰기 순서를 그대로 맡는다.
void BindGumpBackgroundAssets(GumpBackgroundCreateHooks& create,GumpBackgroundDrawHooks& draw,const GumpBackgroundAssets& assets) {
    create.type=[&assets](auto type) { return &assets.Type(type); };
    create.frame=[&assets](auto type,auto abstract,auto frame) { return &assets.Frame(type,abstract,frame); };
    draw.frame=[&assets](auto type,auto frame) { return assets.Frame(type,0,frame); };
}
// 질감/글자는 같은 현재 VFX 색 표를 공유한다. 명암 표는 Screen의 마지막 파일 팔레트 표를 참조할 수 있다.
GumpBackgroundPixelTarget::GumpBackgroundPixelTarget(const GumpPixelTarget& target,const GumpBackgroundAssets& assets,const GumpShadeMaps& maps,ColorMap& colors)
    :target_(target),assets_(assets),maps_(maps),colors_(colors) {}
// 노드/프레임 경계는 보존하고 같은 실제 표면/현재 상태를 사용하는 출력 콜백만 바꾼다.
GumpBackgroundDrawHooks GumpBackgroundPixelTarget::Hooks(GumpDrawState& state,GumpBackgroundDrawHooks hooks) {
    hooks.clip=[&state](auto clip) { SetGumpDrawClip(state,clip); };
    hooks.shade=[this](unsigned shade) { if (shade!=1 && shade!=2) throw std::invalid_argument("배경 명암 표 선택 오류");colors_=shade==1 ? maps_.bright : maps_.dark; };
    hooks.shape=[this](auto request) { Shape(request); };
    hooks.fill=[this](auto rect,auto color,auto checker) { target_.Fill(rect,color,checker); };
    hooks.line=[this](auto line) { target_.Lines(std::span(&line,1)); };return hooks;
}
// 빈 특별 프레임은 픽셀을 쓰지 않는다. 명암은 소스 색 치환이며 대상 화면 색 치환이 아니다.
void GumpBackgroundPixelTarget::Shape(GumpBackgroundShapeRequest request) {
    const auto [database,block]=assets_.Shape(request.type,request.frame);
    const auto& frame=database->Blocks()[block].frames[request.frame];if (frame.IsSpecial()) return;
    const auto key=std::make_tuple(database,block,static_cast<std::size_t>(request.frame));auto found=images_.find(key);
    if (found==images_.end()) found=images_.emplace(key,database->Decode(block,request.frame)).first;
    target_.Image(found->second,{Add(request.position.x,frame.rect.left),Add(request.position.y,frame.rect.top)},request.mapped ? &colors_ : nullptr);
}
}
