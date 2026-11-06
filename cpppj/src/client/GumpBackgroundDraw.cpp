// 실제 배경 출력 분기와 현재 자산/클립/색을 보존하고 원본 호출 수명을 같은 저장소로 연결한다.
#include "client/GumpBackgroundDraw.h"
#include <algorithm>
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
    const auto bodyStart=hooks_.repeatBody ? hooks_.repeatBody(type,frame):0;
    const std::int32_t firstPeriod=metrics.height-1,bodyRows=firstPeriod-bodyStart;
    const auto extra=ClipScreenRect(plan.clip,{plan.clip.left,firstPeriod,plan.clip.right,plan.clip.bottom});
    if (bodyStart>0 && bodyRows>0 && extra.left<extra.right && extra.top<extra.bottom) {
        // 첫 타일은 원본774행을 그대로 보존한다. 새 높이에 드러나는 반복에서만 예산 아래 숨었던25행을 제외한다.
        const auto legacy=ScreenRect{plan.clip.left,plan.clip.top,plan.clip.right,std::min(plan.clip.bottom,firstPeriod)};
        if (legacy.top<legacy.bottom) {
            const TextureClipGuard guard(Require(hooks_.clip),saved,legacy);if (plan.shade) Require(hooks_.shade)(plan.shade);
            for (const auto origin:plan.origins) Require(hooks_.shape)({type,frame,origin,plan.shade!=0}); // 기존 타일/위상은 최초 원문 영역 안에서만 출력한다.
        }
        if (plan.origins.empty()) return;const auto firstY=plan.origins.front().y;
        const auto first=std::int64_t(firstPeriod)+(std::int64_t(extra.top)-firstPeriod)/bodyRows*bodyRows;
        const auto columns=std::count_if(plan.origins.begin(),plan.origins.end(),[firstY](auto origin) { return origin.y==firstY; }); // 기존 가로 타일 개수를 그대로 사용한다.
        const auto rows=(std::int64_t(extra.bottom)-first+bodyRows-1)/bodyRows;
        if (std::uint64_t(columns)*static_cast<std::uint64_t>(rows)>1000000) throw std::length_error("와이드 패널 본문 반복 범위 초과");
        for (auto top=first;top<extra.bottom;top+=bodyRows) { // 원래의 첫 전체 타일 뒤에는 실제 본문25..773만749행 간격으로 반복한다.
            const ScreenRect cut{extra.left,static_cast<std::int32_t>(std::max<std::int64_t>(extra.top,top)),extra.right,static_cast<std::int32_t>(std::min<std::int64_t>(extra.bottom,top+bodyRows))};
            const TextureClipGuard guard(Require(hooks_.clip),saved,cut);if (plan.shade) Require(hooks_.shade)(plan.shade);
            for (const auto origin:plan.origins) { // 원문의 가로 타일 간격과 현재 프레임/색 표는 그대로 사용한다.
                if (origin.y==firstY) Require(hooks_.shape)({type,frame,{origin.x,static_cast<std::int32_t>(top-bodyStart)},plan.shade!=0});
            }
        }
        return;
    }
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
    const auto panelBlock=assets.Edition()==o::OriginalEdition::Patch1078 ? assets.Find("fortGump").block : std::numeric_limits<std::size_t>::max(); // 원문 CD/다른 자산에는 호환 정책을 설치하지 않는다.
    // 내장 타입은 SHP 자산이 없으므로 생성 경계에서 실제 자산 타입만 제공한다.
    for (std::size_t i=0;i<assets.Types().size();++i) {
        const auto& asset=assets.Types()[i];const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+i);
        if (asset.definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("배경 논리 프레임 수 범위 오류");
        const bool widePanel=asset.block==panelBlock; // 다른 자산/판본의 검정 픽셀이나 투명을 바꾸지 않는다.
        Add({type,asset.block,static_cast<std::int32_t>(asset.definition.clusters.size()),types[type].flags1,widePanel ? 2:-1});
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
    if (binding.repeatFrame>=0) {
        if (binding.repeatFrame>=binding.frameCount) throw std::invalid_argument("와이드 패널의 실제 논리 프레임 범위 오류");const auto index=static_cast<std::size_t>(binding.repeatFrame);const auto& source=shapes_.Blocks()[binding.block].frames[index];const auto image=shapes_.Decode(binding.block,index);const auto metrics=view.frames[index];
        if (!source.IsSpecial() && source.rect.left==0 && source.rect.top==0 && image.width==static_cast<std::size_t>(metrics.width-1) && image.height==static_cast<std::size_t>(metrics.height-1)) {
            std::size_t row{};
            for (;row<image.height;++row) { // 명시한A02 패널에서만 첫 연속 불투명 검정 행을 실제 RLE 결과로 센다.
                const auto first=row*image.width;bool black=true;
                for (std::size_t x=0;x<image.width;++x) { // 팔레트0을 투명으로 취급하지 않고 실제 opacity도 함께 확인한다.
                    if (image.indices[first+x]!=0 || image.opacity[first+x]!=255) { black=false;break; }
                }
                if (!black) break;
            }
            if (row>0 && row<image.height) { view.repeatFrame=binding.repeatFrame;view.repeatBodyStart=static_cast<std::int32_t>(row); }
        }
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
// 실제 파일에서 명시 선택한 패널만 확장하고 다른 프레임의 검정/투명 픽셀 의미는 보존한다.
std::int32_t GumpBackgroundAssets::RepeatBodyStart(std::uint32_t type,std::uint32_t frame) const { Frame(type,0,frame);const auto& view=views_.at(type);return view.repeatFrame>=0 && frame==static_cast<std::uint32_t>(view.repeatFrame) ? view.repeatBodyStart:0; }
// 생성과 출력 모두 같은 타입/헤더를 읽되 raw 생성자는 판본별 필드 쓰기 순서를 그대로 맡는다.
void BindGumpBackgroundAssets(GumpBackgroundCreateHooks& create,GumpBackgroundDrawHooks& draw,const GumpBackgroundAssets& assets) {
    create.type=[&assets](auto type) { return &assets.Type(type); };
    create.frame=[&assets](auto type,auto abstract,auto frame) { return &assets.Frame(type,abstract,frame); };
    draw.frame=[&assets](auto type,auto frame) { return assets.Frame(type,0,frame); };
    draw.repeatBody=[&assets](auto type,auto frame) { return assets.RepeatBodyStart(type,frame); }; // 10.78 A02의 새 높이에서만 원문 선두 행 재반복을 제외한다.
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
