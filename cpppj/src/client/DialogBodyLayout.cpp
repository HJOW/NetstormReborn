// 임의 word-wrap 대신 기존 원본 StyleTextBuffer를 재사용하며 변환 효과와 반복 출력을 분리한다.
#include "client/DialogBodyLayout.h"
#include "client/StyleTextBuffer.h"
#include "client/ScrollingTextGumpMarkup.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 좌표/높이의 signed 합산은 하위 DWORD를 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second));
}
// GumpTextDraw와 같은 슬롯/불법 스타일 fallback/빈 조합 규칙으로 실제 출력 폰트를 고른다.
const BitmapFont* Font(FontStore& fonts,const std::array<std::uint32_t,8>& context,o::OriginalEdition edition) {
    auto slot=std::bit_cast<std::int32_t>(context[1]);FontStore::LogicalHeight(slot);
    auto style=context[3];if (style>(edition==o::OriginalEdition::Patch1078 ? 4U : 2U)) { slot=0;style=0; }
    const auto selected=static_cast<FontStyle>(style);
    if (!FontStore::Available(slot,selected) || (edition!=o::OriginalEdition::Patch1078 && slot==6 && selected==FontStyle::Bold)) return nullptr;
    return &fonts.Get(slot,selected);
}
// 한 번 설정한 본문의 원문 조각과 표시 복제는 같은 StyleText 저장소를 공유한다.
struct BodyContent {
    FontStore& fonts; // RenderText의 실제 BitmapFont 포인터 소유자는 호출자의 FontStore다.
    o::OriginalEdition edition; // 실제 측정/스타일/빈 슬롯 차이를 고른다.
    GumpText metrics; // 첫 폭 표와 논리 높이·캐시 기준선을 실제 폰트에서 읽는다.
    StyleTextState fragments; // 파서/입력 줄/표시 줄의 별도 소유 조각을 한 표에 둔다.
    StyleTextBufferState storage; // 실제 완료 줄·자동 줄바꿈·높이·폭·센티널 저장소다.
    std::unique_ptr<StyleText> style; // 원본 짧은 명령·측정·기준선 두 번 순회와 반납을 처리한다.
    std::unique_ptr<StyleTextBuffer> buffer; // 기존 버퍼의 AppendText/Layout/TotalHeight를 재사용한다.
    std::unique_ptr<MarkupAssets> assets; // 같은 GameAssets의 원본 타입/물리 SHP 연결이며 문자 전용이면 없다.
    MarkupState pictureState; // 입력/표시 그림의 현재 실제 파생 필드 저장소다.
    StyleTextMarkupState markupState; // 원본 다중 줄 그림 맞춤/옵션 상태다.
    std::unique_ptr<Markup> pictures; // 실제 그림 생성/기준선/폭/외부 복제/출력/해제를 처리한다.
    std::unique_ptr<StyleTextMarkup> markup; // 실제 이름/프레임 선택과 다중 줄 # 맞춤 규칙을 처리한다.
    GumpCreateState create; // 기존 다중 줄 파서가 같은 판본을 조회하는 최소 생성 상태다.
    ScrollingTextGumpNode node; // 본문의 실제 문맥/metadata/dirty와 연결 버퍼 토큰을 공유하는 노드다.
    GumpDrawState drawClip; // 기존 다중 줄 처리기의 clip 저장소다. 배치 출력은 요청 clip을 사용한다.
    std::unique_ptr<GumpCollection> collection; // 같은 노드를 제공하는 기존 수집/좌표 처리기다.
    std::unique_ptr<ScrollingTextGump> scrolling; // 기존 AppendText/AppendPicture와 dirty/표시 통지를 사용한다.
    std::uint64_t changes{}; // 본문을 추가하며 발생한 실제 Redraw 요청 횟수다. 임의 UI 이벤트를 만들지 않는다.
    std::int32_t width{}; // 현재 표시 복제를 만든 양수 폭이다. 0이면 아직 배치하지 않았다.
    ScreenRect clip{}; // 이번 출력 요청의 실제 내부 clip이다.
    std::vector<RenderText> output; // 이번 StyleText 가상 출력이 제출한 조각 목록이다.
    std::vector<RenderSprite> sprites; // 이번 Markup 가상 출력이 제출한 실제 SHP 그림이다.

    // 문자 효과는 현재 실제 팔레트에서 읽고 그림은 공급한 GameAssets만 사용한다.
    BodyContent(FontStore& suppliedFonts,const PaletteColorTable& palette,o::OriginalEdition suppliedEdition,const GameAssets* suppliedAssets)
        :fonts(suppliedFonts),edition(suppliedEdition),metrics(fonts,edition) {
        MarkupHooks pictureHooks;
        if (suppliedAssets) {
            if (suppliedAssets->Edition()!=edition) throw std::invalid_argument("Tell 본문/그림 판본 불일치");
            assets=std::make_unique<MarkupAssets>(*suppliedAssets);BindMarkupAssets(pictureState,pictureHooks,*assets);
        }
        // 원본 다중 줄 그림은 색 치환을 설정하지 않는다. 미연결 다른 경로의 mapped 요청은 거부한다.
        pictureHooks.shape=[this](MarkupShapeRequest request) {
            if (!assets) throw std::logic_error("Tell 본문 실제 그림 자산 미연결");
            if (request.scaled || request.angle || request.mapped) throw std::logic_error("Tell 본문 배율/색치환 VFX 출력 경계 미연결");
            const auto [database,block]=assets->Shape(request.shape,request.frame);RenderSprite sprite;
            sprite.database=database;sprite.block=block;sprite.frame=request.frame;sprite.x=request.position.x;sprite.y=request.position.y;sprite.clip=clip;
            sprites.push_back(std::move(sprite));
        };
        pictures=std::make_unique<Markup>(fragments,pictureState,std::move(pictureHooks),edition);
        StyleTextHooks hooks;BindStyleTextFonts(hooks,metrics);BindStyleTextPalette(hooks,palette);
        BindMarkup(hooks,*pictures);
        // 실제 조각의 가상 Draw가 선택한 현재 문맥/기준선/클립을 렌더러 입력으로 보관한다.
        hooks.textDraw=[this](std::span<const char> bytes,ScreenPoint point,const std::array<std::uint32_t,8>& context) {
            const auto* font=Font(fonts,context,edition);if (!font) return;
            const auto end=std::find(bytes.begin(),bytes.end(),'\0');
            output.push_back({font,std::string(bytes.begin(),end),point.x,point.y,static_cast<std::uint8_t>(context[2]),static_cast<std::uint8_t>(context[5]),
                {std::bit_cast<std::int32_t>(context[6]),std::bit_cast<std::int32_t>(context[7])},(context[0]&2U)!=0,(context[0]&4U)!=0,clip});
        };
        style=std::make_unique<StyleText>(fragments,std::move(hooks),edition);
        StyleTextMarkupHooks markupHooks;if (suppliedAssets) BindStyleTextMarkupAssets(markupHooks,*suppliedAssets);BindStyleTextMarkupPalette(markupHooks,palette);
        markup=std::make_unique<StyleTextMarkup>(fragments,*pictures,markupState,std::move(markupHooks),edition);
        create.collection.edition=edition;node.buffer=1;GumpCollectionHooks collectionHooks;
        collectionHooks.node=[this](auto token)->GumpCollectionNode* { return token==1 ? &node : nullptr; }; // 본문 파생/기본 노드는 동일 저장소다.
        collection=std::make_unique<GumpCollection>(create.collection,std::move(collectionHooks));
    }
    // 원본 Destroy가 일부러 남기는 미완료 끝 칸도 이 독립 소유자가 먼저 정리한다.
    ~BodyContent() {
        if (buffer && storage.initialized) {
            if (storage.lineCount>=0 && storage.lineCount<storage.maxLines) style->Clear(storage.hardLines[static_cast<std::size_t>(storage.lineCount)]);
            buffer->Destroy();
        }
    }
    // 완성된 StyleText 문자열을 실제 조각별 문맥/태그와 함께 입력 버퍼에 이전한다.
    void Load(std::string_view bytes,const std::array<std::uint32_t,8>& context) {
        const auto lines=std::count(bytes.begin(),bytes.end(),'\n');
        if (lines>=std::numeric_limits<std::int32_t>::max()) throw std::length_error("Tell 본문 원본 signed 줄 수 초과");
        StyleTextBufferHooks hooks;BindStyleTextBufferFonts(hooks,metrics);BindStyleTextBufferMarkup(hooks,*pictures);
        buffer=std::make_unique<StyleTextBuffer>(storage,fragments,*style,static_cast<std::int32_t>(lines)+1,std::move(hooks));
        node.textContext=context;ScrollingTextGumpHooks textHooks;
        textHooks.node=[this](auto token) { return token==1 ? &node : nullptr; }; // 파서/입력은 같은 현재 문맥/태그를 사용한다.
        textHooks.baseNode=[this](auto token)->GumpCreateNode* { return token==1 ? &node : nullptr; }; // 별도 기본 객체 복사본을 만들지 않는다.
        textHooks.buffer=[this](auto token) { return token==1 ? TextGumpBufferBinding{&storage,buffer.get()} : TextGumpBufferBinding{}; }; // 실제 Append/그림/복제 저장소다.
        textHooks.redraw=[this](auto,bool) { ++changes; }; // 원본 추가 뒤 표시 통지는 이 페이지의 변경 요청으로 기록한다.
        scrolling=std::make_unique<ScrollingTextGump>(create,*collection,drawClip,fragments,*style,std::move(textHooks));
        ScrollingTextGumpMarkupHooks parserHooks;
        parserHooks.node=[this](auto token) { return token==1 ? &node : nullptr; }; // 원본 다중 줄 파서가 현재 callback/metadata를 재조회한다.
        parserHooks.base=[this](auto token)->GumpCreateNode* { return token==1 ? &node : nullptr; }; // 실제 파생 노드와 같은 주소다.
        ScrollingTextGumpMarkup(*scrolling,*style,*pictures,*markup,std::move(parserHooks)).Parse(1,bytes);
    }
    // 같은 폭이면 표시 복제를 재사용하고 origin/clip이 바뀌면 실제 출력 좌표만 다시 만든다.
    DialogBodyLayoutResult Layout(std::int32_t requestedWidth,ScreenPoint origin,ScreenRect requestedClip) {
        if (requestedWidth<=0) throw std::invalid_argument("Tell 본문 배치에는 양수 폭 필요");
        if (width!=requestedWidth) { buffer->Layout(requestedWidth);width=requestedWidth; }
        clip=requestedClip;output.clear();sprites.clear();auto y=origin.y;std::size_t visited=0;
        // ScrollingTextGump와 같은 각 줄 최대 상승/조각 기준선을 기존 StyleText::Draw에 맡긴다.
        for (auto token=storage.sentinel.next;token;token=storage.softLines.at(token).next) {
            if (++visited>storage.softLines.size()) throw std::logic_error("Tell 본문 표시 줄 순환");
            const auto& line=storage.softLines.at(token);style->Draw(line.head,{origin.x,y});y=Add(y,line.height);
        }
        return {output,buffer->MaximumWidth(),buffer->TotalHeight(),storage.softCount,sprites};
    }
};
}
// FontStore와 현재 팔레트는 외부 소유이며 페이지 내용만 교체한다.
struct DialogBodyLayout::Impl {
    FontStore& fonts; // 결과 폰트 포인터와 실제 캐시의 소유자다.
    const PaletteColorTable& palette; // SetBody가 현재 원본 색 번호를 읽는 안정된 표다.
    o::OriginalEdition edition; // 모든 파서/버퍼/출력 처리기가 공유하는 원본 판본이다.
    const GameAssets* assets{}; // 그림 지원 생성자만 실제 자산을 공급한다. 문자 전용은 선택 시 명시 오류다.
    std::unique_ptr<BodyContent> content; // 가장 최근 성공한 본문의 조각/줄/표시 복제 소유자다.
    // 실제 자산의 수명을 빌리고 본문/효과는 아직 생성하지 않는다.
    Impl(FontStore& suppliedFonts,const PaletteColorTable& suppliedPalette,o::OriginalEdition suppliedEdition)
        :fonts(suppliedFonts),palette(suppliedPalette),edition(suppliedEdition) {}
};
// 실제 글꼴/색 조회 경계만 보관하므로 생성 시 HTML 명령을 실행하지 않는다.
DialogBodyLayout::DialogBodyLayout(FontStore& fonts,const PaletteColorTable& palette,o::OriginalEdition edition)
    :impl_(std::make_unique<Impl>(fonts,palette,edition)) {}
// 같은 실제 GameAssets를 그림 이름/프레임/폭/기준점과 출력에 공유한다.
DialogBodyLayout::DialogBodyLayout(FontStore& fonts,const PaletteColorTable& palette,const GameAssets& assets,o::OriginalEdition edition)
    :DialogBodyLayout(fonts,palette,edition) { impl_->assets=&assets; }
// 현재 페이지와 표시 복제의 소유 체인을 해제한다.
DialogBodyLayout::~DialogBodyLayout()=default;
// 효과 실행과 소유 본문 작성이 모두 성공한 뒤에만 이전 본문을 교체한다.
void DialogBodyLayout::SetBody(std::string_view html,const std::array<std::uint32_t,8>& context,HtmlTextParserState& state,
    HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks) {
    const auto bytes=HtmlTextParser(state,globals,std::move(hooks),impl_->edition).Parse(html);
    auto content=std::make_unique<BodyContent>(impl_->fonts,impl_->palette,impl_->edition,impl_->assets);content->Load(bytes,context);impl_->content=std::move(content);
}
// 본문이 아직 없으면 빈 측정을 제공하되 잘못된 양수 폭 계약은 동일하게 검사한다.
DialogBodyLayoutResult DialogBodyLayout::Layout(std::int32_t width,ScreenPoint origin,ScreenRect clip) const {
    if (width<=0) throw std::invalid_argument("Tell 본문 배치에는 양수 폭 필요");
    return impl_->content ? impl_->content->Layout(width,origin,clip) : DialogBodyLayoutResult{};
}
}
