#include "client/WarningMessages.h"
#include "client/MessageBoard.h"
#include "client/TextGump.h"
#include "client/GumpFrame.h"
#include "client/GumpTree.h"
#include "client/Renderer.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 Warning 파생 가상표와 dead 비트다. 다른 메시지 생성자는 이 어댑터에서 선택하지 않는다.
constexpr std::uint32_t kWarningVtable=0x517b60,kDead=0x10000;
// TextGump와 같은 실제 노드에 종류/검색 키/기한을 보관한다.
struct WarningNode:MessageGumpNode {
    std::string source; // 실제 창 보고에만 사용하는 원문 사본이며 출력은 소유 StyleText 체인을 읽는다.
};
}
// 실제 Gump 생성/트리/수명/StyleText 저장소를 하나의 안정된 주소에서 소유한다.
struct WarningMessages::Impl {
    FontStore& fonts; // 실제 원본 비트맵 글꼴 저장소를 공유한다.
    PaletteColorTable palette; // 색 조회 callback이 참조하는 현재 파일 팔레트의 안정된 저장소다.
    std::function<double()> now; // 원본처럼 매 조회마다 읽는 벽 시각이다.
    std::function<void()> sound; // 같은 실제 SoundList/SoundPlayer의 newWarnMessage.wav 재생이다.
    GumpText metrics; // StyleText의 실제 캐시 측정/기준선 조회기다.
    StyleTextState fragments; // 메시지들이 소유하는 실제 문자열 조각 표다.
    std::unique_ptr<StyleText> style; // 기존 파서/측정/출력/삭제 처리기다.
    GumpCreateState create; // 같은 노드의 등록표와 루트다.
    GumpLifetimeState life; // 실제 idle 표와 지연 삭제 큐다.
    GumpTreeState treeState; // 실제 부착/깊이 처리의 현재 전역이다.
    GumpDrawState clip; // TextGump 내부 클립을 실제 글자 제출에 전달한다.
    TextGumpState textState; // 원본 정렬 BYTE 초기값45다.
    MessageBoardState boardState; // 원본20칸 메시지 serial 표다.
    std::map<std::uint32_t,std::unique_ptr<GumpCreateNode>> bases; // 실제 루트 저장소다.
    std::map<std::uint32_t,std::unique_ptr<WarningNode>> messages; // 같은 원본 메시지와 TextGump의 공유 노드다.
    std::uint32_t nextToken{},root{}; // 재사용하지 않는 호스트 토큰과 실제 루트 식별자다.
    bool changed{}; // 부착/삭제가 화면 갱신을 필요로 하는지 기록한다.
    std::vector<RenderText> output; // TextGump의 현재 Draw 콜백이 제출하는 글자 목록이다.
    std::unique_ptr<GumpCreate> factory; // 같은 저장소의 실제 생성/등록 처리기다.
    std::unique_ptr<GumpCollection> collection; // 같은 노드의 위치/경계/클립 계산이다.
    std::unique_ptr<GumpTree> tree; // 실제 루트 부착과 분리를 처리한다.
    std::unique_ptr<GumpLifetime> lifetime; // 같은 노드의 Kill/Cleanup/파생 소멸을 처리한다.
    std::unique_ptr<GumpFrame> frame; // 실제 idle 표를 순회한다.
    std::unique_ptr<TextGump> text; // 원본 TextGump의 생성/측정/출력을 재사용한다.
    std::unique_ptr<MessageBoard> board; // 원본20칸 검색/생성/부착 순서를 처리한다.
    std::unique_ptr<MessageGump> pauseMessage; // 같은 파생 노드와 TextGump를 사용하는 원본 종류11 생성자다.

    // callback 대상은 이 구현보다 오래 사는 Client 글꼴/음향/벽 시각이다.
    Impl(FontStore& suppliedFonts,const PaletteColorTable& suppliedPalette,std::function<double()> suppliedNow,std::function<void()> suppliedSound,MessageGumpHooks pauseContext)
        :fonts(suppliedFonts),palette(suppliedPalette),now(std::move(suppliedNow)),sound(std::move(suppliedSound)),metrics(fonts) {
        if (!now || !sound) throw std::invalid_argument("Warning 실제 벽 시각/음향 연결 누락");
        StyleTextHooks styleHooks;BindStyleTextFonts(styleHooks,metrics);BindStyleTextPalette(styleHooks,palette);
        styleHooks.textDraw=[this](std::span<const char> bytes,ScreenPoint point,const std::array<std::uint32_t,8>& context) {
            const auto end=std::find(bytes.begin(),bytes.end(),'\0');
            output.push_back({&fonts.Get(static_cast<int>(context[1]),static_cast<FontStyle>(context[3])),std::string(bytes.begin(),end),point.x,point.y,
                static_cast<std::uint8_t>(context[2]),static_cast<std::uint8_t>(context[5]),{std::bit_cast<std::int32_t>(context[6]),std::bit_cast<std::int32_t>(context[7])},
                (context[0]&2U)!=0,(context[0]&4U)!=0,clip.clip});
        };
        style=std::make_unique<StyleText>(fragments,std::move(styleHooks));
        create.screenWidth=create.screenHeight=1;
        factory=std::make_unique<GumpCreate>(create,GumpCreateHooks{[this](auto token) { return Node(token); },[this](auto) {
            const auto token=++nextToken;bases.emplace(token,std::make_unique<GumpCreateNode>());return token;
        }});
        collection=std::make_unique<GumpCollection>(create.collection,factory->CollectionHooks());
        GumpTreeHooks treeHooks;treeHooks.node=[this](auto token) { return Node(token); };
        treeHooks.redraw=[this](auto,bool) { changed=true; };treeHooks.push=[this](auto token) { tree->Push(token); };
        treeHooks.attach=[this](auto token,float x,float y,auto parent,auto mode) { tree->Attach(token,x,y,parent,mode); };
        treeHooks.kill=[this](auto token) { lifetime->Kill(token); };tree=std::make_unique<GumpTree>(create,treeState,std::move(treeHooks));
        GumpLifetimeHooks lifeHooks;lifeHooks.node=[this](auto token) { return Node(token); };lifeHooks.push=[this](auto token) { tree->Push(token); };
        lifeHooks.deletingDestructor=[this](auto token,auto flags) { if (messages.contains(token)) text->DeleteScalar(token,flags);else lifetime->DeleteScalar(token,flags); };
        lifeHooks.release=[this](auto token) { messages.erase(token);bases.erase(token); };
        lifetime=std::make_unique<GumpLifetime>(create,life,std::move(lifeHooks));
        text=std::make_unique<TextGump>(create,textState,*factory,*lifetime,*collection,clip,*style,TextGumpHooks{
            [this](auto token)->TextGumpNode* { const auto it=messages.find(token);return it==messages.end() ? nullptr : it->second.get(); },[this](auto token) { return Node(token); }});
        // 외부 소유자는 현재 모드/이름/번역만 공급하고 실제 노드·문자열·측정은 같은 저장소를 쓴다.
        if (pauseContext.challenge) {
            pauseContext.node=[this](auto token)->MessageGumpNode* { const auto it=messages.find(token);return it==messages.end() ? nullptr : it->second.get(); };
            pauseContext.constructText=[this](auto token) { text->Construct(token,0,0,0,0); };
            pauseContext.style=[this](auto token,auto value) { auto& node=*messages.at(token);node.source=value;style->Parse(node.head,value,node.textContext); };
            pauseContext.fit=[this](auto token,auto x,auto y) { text->Fit(token,x,y); };
            pauseMessage=std::make_unique<MessageGump>(create,std::move(pauseContext));
        }
        GumpFrameHooks frameHooks;frameHooks.node=[this](auto token) { return Node(token); };
        frameHooks.idle=[this](auto token) {
            const auto deadline=messages.at(token)->deadline;
            if (deadline!=0.0 && WarningExpired(deadline,now())) lifetime->Kill(token);
        };
        frame=std::make_unique<GumpFrame>(create,life,std::move(frameHooks));root=factory->InitializeRoot();
        MessageBoardHooks boardHooks;boardHooks.lookup=[this](auto serial) { return Lookup(serial); };
        boardHooks.kill=[this](auto token) { lifetime->Kill(token); };
        boardHooks.create=[this](auto kind,auto player,auto parameter,std::string_view source) {
            if (kind!=17 && kind!=11) throw std::logic_error("다른 원본 메시지 생성자 미연결");
            const auto token=++nextToken;auto& node=*messages.emplace(token,std::make_unique<WarningNode>()).first->second;
            if (kind==11) {
                if (!pauseMessage) throw std::logic_error("Pause 메시지의 현재 모드/이름/번역 미연결");
                pauseMessage->Construct(token,kind,player,parameter);return MessageBoardRecord{token,node.serial,kind,player,parameter};
            }
            text->Construct(token,0,0,0,0);node.vtable=kWarningVtable;node.kind=kind;node.player=player;node.parameter=parameter;node.source=source;
            sound();style->Parse(node.head,source,node.textContext);node.deadline=WarningDeadline(now());lifetime->SetIdle(token,true);text->Fit(token);
            return MessageBoardRecord{token,node.serial,kind,player,parameter};
        };
        boardHooks.width=[this](auto token) {
            const auto width=Node(token)->baseWidth;
            if (!std::isfinite(width) || width<0 || static_cast<double>(width)>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Warning 원본 ftol 폭 범위");
            return static_cast<std::int32_t>(width);
        };
        boardHooks.attach=[this](auto token,auto x,auto y) { tree->Attach(token,static_cast<float>(x),static_cast<float>(y),root,0); };
        board=std::make_unique<MessageBoard>(boardState,std::move(boardHooks));
    }
    // 실제 처리기 해제 전에 메시지의 소유 체인을 원래 파생 소멸 순서로 해제한다.
    ~Impl() { while (Node(root)->child) lifetime->Kill(Node(root)->child);lifetime->Cleanup(); }
    // 기본/파생 노드는 같은 GumpCreateNode 주소를 제공한다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (const auto it=messages.find(token);it!=messages.end()) return it->second.get();
        const auto it=bases.find(token);return it==bases.end() ? nullptr : it->second.get();
    }
    // 원본00464060은 등록표 전체가 아니라 현재 루트의 직계 자식만 읽고 dead를 제외한다.
    std::optional<MessageBoardRecord> Lookup(std::uint32_t serial) const {
        std::size_t visited=0;
        // 호스트에서는 손상된 순환만 거부한다. 정상 형제 순서는 그대로 따른다.
        for (auto token=Node(root)->child;token;token=Node(token)->next) {
            if (++visited>kGumpRegistryCapacity) throw std::logic_error("Warning 직계 자식 순환");
            const auto& node=*messages.at(token);
            if (node.serial==serial && !(node.flags&kDead)) return MessageBoardRecord{token,serial,node.kind,node.player,node.parameter};
        }
        return std::nullopt;
    }
};
// 글꼴/색/벽 시각/음향의 실제 공급자를 같은 구현에 보관한다.
WarningMessages::WarningMessages(FontStore& fonts,const PaletteColorTable& palette,std::function<double()> now,std::function<void()> sound,MessageGumpHooks pauseContext)
    :impl_(std::make_unique<Impl>(fonts,palette,std::move(now),std::move(sound),std::move(pauseContext))) {}
// 구현 소멸자가 파생 텍스트의 실제 수명을 먼저 정리한다.
WarningMessages::~WarningMessages()=default;
// 화면 경계는 실제 루트/표시 클립과 메시지 배치 전역이 같은 값을 사용한다.
void WarningMessages::SetView(int width,int height,const PaletteColorTable& palette) {
    if (width<=0 || height<=0) throw std::invalid_argument("Warning 화면 크기 오류");
    impl_->palette=palette;impl_->create.screenWidth=width;impl_->create.screenHeight=height;impl_->boardState.screenWidth=width;
    auto& root=*impl_->Node(impl_->root);root.baseWidth=root.width=static_cast<float>(width);root.baseHeight=root.height=static_cast<float>(height);
    SetGumpDrawClip(impl_->clip,{0,0,width,height});
}
// Warning은 기존 대화상자 상태/일시정지/입력을 바꾸지 않는다.
void WarningMessages::Add(std::string_view styledText) { impl_->board->Show(17,0,true,0,styledText); }
// 시계/음향은 명령 소유자가 처리하며 이 경로는 실제 상태 메시지만 변경한다.
void WarningMessages::PauseStatus(bool enabled) { impl_->board->Show(11,0,enabled,0,""); }
// 새 메시지를 만들지 않고 기존 MessageBoard의 실제 검색·Kill 경로를 사용한다.
void WarningMessages::Remove(std::int32_t kind,std::int32_t player,std::int32_t parameter) { impl_->board->Show(kind,player,false,parameter,""); }
// 정상 프레임의 삭제 정리는 Idle보다 먼저 호출한다. 현재 별도 Warning 저장소의 순서 어댑터다.
bool WarningMessages::CleanupGumps() { impl_->lifetime->Cleanup();return std::exchange(impl_->changed,false); }
// 만료한 Warning은 Kill만 한다. 다음 정상 Cleanup의 실제 반납까지 dead 저장소를 보존한다.
bool WarningMessages::IdleGumps() { impl_->frame->Idle();return std::exchange(impl_->changed,false); }
// 기존 독립 검사에서 사용한 결합 순서는 유지하되 실제 Client는 이를 호출하지 않는다.
bool WarningMessages::Idle() {
    const bool changed=IdleGumps();return CleanupGumps() || changed;
}
// 같은 TextGump 소유 체인의 실제 Draw가 Renderer에 줄 글자를 만든다.
std::vector<RenderText> WarningMessages::Text() const {
    impl_->output.clear();
    // 원본 표시 정렬의 y 오름차순과 같은 행 순서로 제출한다.
    for (const auto serial:impl_->boardState.serials) if (serial && serial!=0xffffffffU) if (const auto record=impl_->Lookup(serial)) impl_->text->Draw(record->token);
    return impl_->output;
}
// 추측한 표시 상태 대신 실제 노드와 소유 원문을 보고한다.
std::string WarningMessages::Report() const {
    std::ostringstream out;out.precision(17);std::size_t count=0;
    // 실제20칸의 현재 유효 노드만 출력한다.
    for (std::size_t i=0;i<impl_->boardState.serials.size();++i) if (const auto record=impl_->Lookup(impl_->boardState.serials[i])) {
        const auto& node=*impl_->messages.at(record->token);const auto position=impl_->collection->Position(record->token);++count;
        out<<"warning\t"<<i<<'\t'<<node.serial<<'\t'<<position.x<<'\t'<<position.y<<'\t'<<node.width<<'\t'<<node.height<<'\t'<<node.deadline<<'\t'<<node.source<<'\n';
    }
    out<<"warning_count\t"<<count<<'\n';return out.str();
}
}
