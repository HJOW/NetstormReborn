// kind4 쌍의 현재 좌표/깊이/tail을 보존하고 실제8비트 dirty 표면에 그린다.
#include "client/ClientColorSortInput.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/RawCursorPlacement.h"
#include "client/PreviewColorMaps.h"
#include "client/OwnerColorMaps.h"
#include "client/OwnerColorTableState.h"
#include "client/PaletteColors.h"
#include "client/GumpTextDraw.h"
#include "o/RawBridgeConnect.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
constexpr std::array<std::string_view,5> kMessages{"Priest can't get here.","Must be placed over constructing artifact","Cannot place here","Energy not satisfied","Only on Controlled Island"}; // 실제5325cc의 다섯 번역 키다.
constexpr std::array<std::uint32_t,12> kStandard{82,155,158,164,135,133,156,73,170,146,144,126}; // 실제43be30의 일반 색표 대상이다.
constexpr std::array<std::uint32_t,5> kSpecial{111,141,137,94,97}; // 원본 전용 HSL 색표를 생성할 다섯 타입이다.
}
// 실제 수집/출력이 읽는 게임 상태와 표시 전용 상태를 고정 주소에서 소유한다.
struct RawCampaignCursorDisplay::Impl {
    RawWorldCore core; // 같은 현재 TYPE/spot/시간의 실제 월드 참조다.
    std::array<std::uint32_t,2> textColors{}; // Native Take가 복사할 경고 흰색과 비용 노랑이다.
    CursorPreviewSource source; // 현재 잠금/30SID/TYPE/판정/격자의 참조다.
    const GameAssets& assets; // 현재 SHP/파일 팔레트의 장기 저장소다.
    o::ScrambledSpStore& sp; // 게임과 같은32×9 분산 SP 저장소다.
    const o::SquidProcessState& time; // 같은 현재 게임 시각을 읽는다.
    const std::uint32_t& playerKnown; // 실제595344 DWORD다.
    PreviewCollectionState feedback; // 원본 강조/카운터의 쓰기 저장소다.
    PreviewDrawState settings; // 원본 옵션/팔레트/문자 프레임 표다.
    ScreenRect viewport{}; // 같은 장면 캐시는 마지막 dirty가 아닌 전체 월드 clip을 유지한다.
    bool paletteReady{}; // 실제 파일 팔레트 없이 초기화되지 않은 색을 사용하지 않는다.
    PreviewColorMaps colors; // 현재 파일 팔레트의 파괴/불법/문자 그림자 표다.
    std::map<std::uint32_t,OwnerColorMaps> ownerColors; // 일반/전용 타입의 실제9색 표를 타입별로 캐시한다.
    std::shared_ptr<OwnerColorTableState> sharedOwnerColors; // 실제 Client의 현재557940이다. 지역 색표는 기존 독립 입력에만 사용한다.
    std::function<std::uint32_t(std::uint32_t)> depth; // 실제 Gump 부모 깊이다.
    std::function<ColorMap(std::uint32_t,std::uint32_t)> specializedOwner; // 기본17종 외 확장 소유자 색표의 실제 공급자다.
    FontStore* fonts{}; // 실제 Client의 장기 원본 비트맵 글꼴 저장소다.
    std::function<std::string(std::string_view)> translate; // 현재 언어의 원본 바이트 번역이다.
    std::unique_ptr<PreviewCollection> collection; // 같은 원본 수집/부분 기록/피드백 몸체다.

    // 실제 같은 풀/판본과 원본 기본 타입/문자 표를 한 번 연결한다.
    Impl(RawWorldCore world,CursorPreviewSource input,const GameAssets& data,o::ScrambledSpStore& store,const std::uint32_t& known)
        :core(world),source(input),assets(data),sp(store),time(core.processState),playerKnown(known) {
        if (&core.pool!=&source.pool || source.pool.Edition()!=o::OriginalEdition::Patch1078 || data.Edition()!=source.pool.Edition()) throw std::invalid_argument("실제 커서 표시 판본/풀 계약 오류");
        settings.ownerMaps.resize(data.TypeTable().Types().size());settings.overlayTrigger=99;settings.overlayType=123;settings.labelType=103;settings.priceType=source.lifecycle.costType;
        // 원본472e60의 문자 표는 BSS0을 유지하고 지정11문자만 물리 프레임 순서로 쓴다.
        const std::string_view letters="wrtsWRTS.?>";for (std::size_t index=0;index<letters.size();++index) settings.labelFrames[static_cast<std::uint8_t>(letters[index])]=static_cast<std::uint32_t>(index);
        // 일반12종은 고정 후보로 즉시 만들고 전용5종은 파일 팔레트에서 만든다.
        for (const auto type:kStandard) { settings.ownerMaps.at(type)=true;ownerColors.emplace(type,BuildStandardOwnerColorMaps(type,source.surfaces.bridgeType)); }
        // 전용 등록도 원본 존재 표를 유지하며 SetPalette에서 실제 HSL 표를 채운다.
        for (const auto type:kSpecial) settings.ownerMaps.at(type)=true;
        PreviewCollectionHooks hooks;hooks.gumpDepth=[this](std::uint32_t parent) { if (!depth) throw std::logic_error("실제 커서 부모 Gump 깊이 미연결");return depth(parent); };
        hooks=MakePreviewSpHooks(sp,std::move(hooks));hooks.now=[this] { return std::bit_cast<std::uint32_t>(static_cast<float>(time.Now())); };hooks.querySp=[this] { return playerKnown; };
        hooks.scrambledOffset=[this] { return sp.OffsetCurrent(); };hooks.copiedOffset=[this] { return std::bit_cast<std::uint32_t>(sp.TrackingOffsetCurrent()); };
        hooks.firstPool=[this] { return sp.FirstPoolCurrent()!=nullptr; };hooks.firstIndex=[this] { return sp.Permutation()[0]; };
        collection=std::make_unique<PreviewCollection>(core.pool,core.types,core.spots,feedback,std::move(hooks));collection->BindCursor(source);
    }
    // 같은 현재 TYPE/owner 색 번호의 원본 표 또는 연결한 확장 표만 반환한다.
    ColorMap Color(PreviewColorRequest request) {
        if (request.colors==PreviewColors::Destroy) return colors.destroy;if (request.colors==PreviewColors::Illegal) return colors.illegal;if (request.colors==PreviewColors::LabelShadow) return colors.labelShadow;
        if (sharedOwnerColors) { // 현재 Player 색을 같은 원본 번호로 바꾼 뒤 실제 공통 표를 읽는다.
            if (request.owner>=source.surfaces.ownerColors.size()) throw std::out_of_range("커서 현재 owner 색 번호 범위");
            const auto color=source.surfaces.ownerColors[request.owner];if (color<0 || color>8) throw std::out_of_range("커서 현재 owner 색표 범위");return sharedOwnerColors->Color(request.type,static_cast<std::uint32_t>(color));
        }
        const auto found=ownerColors.find(request.type);
        if (found==ownerColors.end()) { if (!specializedOwner) throw std::logic_error("커서 확장 소유자 색표 미연결 TYPE"+std::to_string(request.type));return specializedOwner(request.type,request.owner); }
        if (request.owner>=source.surfaces.ownerColors.size()) throw std::out_of_range("커서 현재 owner 색 번호 범위");
        const auto color=source.surfaces.ownerColors[request.owner];if (color<0 || color>8) throw std::out_of_range("커서 현재 owner 색표 범위");return found->second[static_cast<std::size_t>(color)];
    }
    // 실제 글자 출력은 같은 현재 문맥/원본 바이트/FontStore와 dirty clip을 사용한다.
    void Text(PreviewTextRequest request,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        if (!fonts || !translate) throw std::logic_error("실제 커서 글꼴/번역 미연결");std::string text;const auto& life=source.lifecycle;
        const auto context=DecodeGumpTextContext(request.index==5 ? life.costContext : life.warningContext);
        if (request.index==5) text=life.costText;else { if (request.index>=kMessages.size()) throw std::out_of_range("커서 경고 문구 번호");text=translate(kMessages[request.index]); }
        text.push_back(0);GumpDrawState state;SetGumpDrawClip(state,clip);GumpPixelTarget target(pixels,pitch,height,state);GumpGlyphColorState glyph;
        GumpTextDraw draw(*fonts,MakeGumpGlyphPixelHooks(target,glyph),assets.Edition());draw.Draw(text,request.position,context);
    }
    // Native 목록을 가져온 뒤 삭제/잠금이 변한 캐시 항목을 실제 슬롯으로 다시 실행하지 않는다.
    void Draw(o::RenderCollectionEntry entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        if (source.drop.lockDepth) return;if (!paletteReady) throw std::logic_error("실제 커서 파일 팔레트 미설치");const o::Sid sid{static_cast<std::uint16_t>(entry.token/source.pool.Layout().stride)};
        if (std::find(source.lifecycle.previews.begin(),source.lifecycle.previews.end(),sid.value)==source.lifecycle.previews.end()) return;
        PreviewPixelTarget target(pixels,pitch,height,clip,MakeCursorPreviewAssetShapes(assets,settings.priceType,source.lifecycle),[this](PreviewColorRequest request) { return Color(request); },[&](PreviewTextRequest request) { Text(request,pixels,pitch,height,clip); });
        PreviewDraw draw(core.pool,core.types,feedback,settings,target.Hooks());draw.BindCursor(source);draw.Draw(entry);
    }
};
// 참조 입력만 보관하고 실제 표시 몸체를 생성한다.
RawCampaignCursorDisplay::RawCampaignCursorDisplay(RawWorldCore core,CursorPreviewSource source,const GameAssets& assets,o::ScrambledSpStore& sp,const std::uint32_t& known):impl_(std::make_unique<Impl>(core,source,assets,sp,known)) {}
RawCampaignCursorDisplay::~RawCampaignCursorDisplay()=default; // 상위 커서가 현재 표시 callback보다 오래 살아야 한다.
// 파일 팔레트가 바뀌면 원본 세 표와 흰색/빨강 윤곽 번호를 같은 논리 RGB로 다시 계산한다.
void RawCampaignCursorDisplay::SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical) {
    if (palette.size()!=256) throw std::invalid_argument("커서 실제 파일 팔레트256색 계약");std::array<std::uint32_t,256> logical;
    // BGRX 호스트 구조와 별개로 원본 R/G/B 순 DWORD를 만든다.
    for (std::size_t index=0;index<logical.size();++index) logical[index]=std::uint32_t(palette[index].red)|(std::uint32_t(palette[index].green)<<8)|(std::uint32_t(palette[index].blue)<<16)|(std::uint32_t(palette[index].reserved)<<24);
    const auto named=BuildPaletteColorTable(logical,impl_->assets.Edition());impl_->textColors={named.basic[7],named.basic[5]};impl_->colors=BuildPreviewColorMaps(ColorSortLogicalInput(colorSortLogical,logical),named.basic[1]);impl_->settings.goodColor=static_cast<std::uint8_t>(named.basic[7]);impl_->settings.badColor=static_cast<std::uint8_t>(named.basic[1]);
    // 실제 전용5종의 현재 파일 명도 표도 함께 다시 만든다.
    if (!impl_->sharedOwnerColors) for (const auto type:kSpecial) impl_->ownerColors[type]=BuildSpecialOwnerColorMaps(ColorSortLogicalInput(colorSortLogical,logical),type); // 독립 입력만 지역 표를 갱신한다.
    impl_->paletteReady=true;
}
// 실제 월드 clip은 마지막 dirty 영역과 별도로 유지한다.
void RawCampaignCursorDisplay::SetView(SoundView view) { impl_->viewport={view.left,view.top,view.right,view.bottom}; }
void RawCampaignCursorDisplay::BindOwnerColors(std::shared_ptr<OwnerColorTableState> owner) { // 테마와 무관한 실제 현재 배너 표를 빌린다.
    if (!owner || !owner->Ready()) throw std::invalid_argument("커서 공통 소유자 색표 최초 등록 누락");
    impl_->sharedOwnerColors=std::move(owner);impl_->ownerColors.clear();impl_->settings.sharedOwnerMaps=impl_->sharedOwnerColors->Tokens();
}
// 같은 장기 실제 글꼴/번역기를 다음 Draw부터 사용한다.
void RawCampaignCursorDisplay::SetText(FontStore& fonts,std::function<std::string(std::string_view)> translate) { impl_->fonts=&fonts;impl_->translate=std::move(translate); }
// 실제 부모 Gump 및 확장 색표 처리를 같은 표시기에 연결한다.
void RawCampaignCursorDisplay::SetExternal(std::function<std::uint32_t(std::uint32_t)> depth,std::function<ColorMap(std::uint32_t,std::uint32_t)> specializedOwner) { impl_->depth=std::move(depth);impl_->specializedOwner=std::move(specializedOwner); }
// 수집은 dirty마다 한 번이며 본체/보조를 나누어 현재 Renderer의 공통 깊이 정렬에 넣는다.
std::vector<RenderSprite> RawCampaignCursorDisplay::Sprites(ScreenRect clip,std::uint32_t dirtyFlags) {
    std::array<o::RenderCollectionEntry,60> entries{};DrawCollectionHooks hooks;BindPreviewCollection(hooks,*impl_->collection);
    const auto count=CollectDrawEntries({clip,{},{},dirtyFlags,5000},entries,hooks).count;std::vector<RenderSprite> result;
    // 원본 kind4 쌍의 순서와 개별 깊이를 유지한다. 실제 Draw는 현재 dirty 표면에서 실행한다.
    for (std::int32_t index=0;index<count;++index) { const auto entry=entries[static_cast<std::size_t>(index)];RenderSprite sprite;sprite.clip=impl_->viewport;sprite.order={entry.x,entry.y,entry.depth};sprite.procedure=RenderProcedure{impl_->viewport,[this,entry](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect cut) { impl_->Draw(entry,pixels,pitch,height,cut); }};result.push_back(std::move(sprite)); }
    return result;
}
std::int32_t RawCampaignCursorDisplay::CollectDrawing(o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const { return impl_->collection->Collect(view,output,limit); } //현재30SID/판정/남은 횟수의 실제 원문 수집이다.
void RawCampaignCursorDisplay::DrawEntry(const o::RenderCollectionEntry& entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) { impl_->Draw(entry,pixels,pitch,height,clip); } //별도 스프라이트나 두 번째 수집을 만들지 않는다.
std::array<std::uint32_t,2> RawCampaignCursorDisplay::TextColors() const { return impl_->textColors; } // Take의 두 현재 색만 반환한다.
PreviewDrawState& RawCampaignCursorDisplay::Settings() { return impl_->settings; } // 현재 표시 옵션을 직접 빌린다.
PreviewCollectionState& RawCampaignCursorDisplay::Feedback() { return impl_->feedback; } // 현재 피드백 쓰기 상태를 직접 빌린다.
}
