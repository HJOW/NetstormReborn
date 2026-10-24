// Warning/Pause의 실제 파생 노드를 Client의 단일 Gump/StyleText에 연결한다.
#include "client/WarningMessages.h"
#include "client/RawMenuControls.h"
#include <cmath>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kWarningVtable=0x517b60,kDead=0x10000; // 원본 Warning 가상표와 공통 dead 비트다.
constexpr std::uint32_t kDrawSlot=2; // 생산0/미니맵1과 겹치지 않는 공통 Draw 공급자다.
// 실제 MessageGump/TextGump 주소에 보고용 원문만 덧붙인다. 출력은 공통 StyleText 체인을 읽는다.
struct WarningNode:MessageGumpNode {
    std::string source; // 실제 창 관찰의 원문이며 별도 표시/측정 저장소가 아니다.
};
}
// 등록표/루트/수명/글자는 빌리고 원본20칸과 파생 필드만 소유한다.
struct WarningMessages::Impl {
    RawMenuTree common; // 같은 Client의 현재 등록표·토큰·트리·삭제 큐다.
    RawMenuControls& menus; // 같은 StyleText 조각·글꼴·픽셀 클립을 제공한다.
    std::function<double()> now; // 원본처럼 매 가상 Idle/생성에서 읽는 벽 시각이다.
    std::function<void()> sound; // 실제 SoundList의 newWarnMessage.wav 재생이다.
    MessageBoardState boardState; // 원본20칸 serial 표이며 루트 교체 때 임의 초기화하지 않는다.
    std::map<std::uint32_t,std::unique_ptr<WarningNode>> messages; // 공통 토큰의 안정된 파생 저장소다.
    std::unique_ptr<MessageBoard> board; // 같은 현재 루트의 검색/생성/부착 순서다.
    std::unique_ptr<MessageGump> pauseMessage; // 같은 TextGump를 쓰는 원본 종류11 생성자다.

    // 모든 callback은 호출 시 현재 공통 루트/노드/등록표를 읽는다.
    Impl(RawMenuTree tree,RawMenuControls& menu,std::function<double()> suppliedNow,std::function<void()> suppliedSound,MessageGumpHooks pauseContext)
        :common(std::move(tree)),menus(menu),now(std::move(suppliedNow)),sound(std::move(suppliedSound)) {
        if (!now || !sound) throw std::invalid_argument("Warning 실제 벽 시각/음향 연결 누락");
        if (pauseContext.challenge) {
            pauseContext.node=[this](auto token)->MessageGumpNode* { return Node(token); };
            pauseContext.constructText=[this](auto token) { menus.TextBackend().Construct(token,0,0,0,0); };
            pauseContext.style=[this](auto token,auto value) { Style(token,value); };
            pauseContext.fit=[this](auto token,auto x,auto y) { menus.TextBackend().Fit(token,x,y); };
            pauseMessage=std::make_unique<MessageGump>(common.create,std::move(pauseContext));
        }
        MessageBoardHooks hooks;hooks.lookup=[this](auto serial) { return Lookup(serial); };
        hooks.kill=[this](auto token) { common.lifetime.Kill(token);common.changed(); };
        hooks.create=[this](auto kind,auto player,auto parameter,std::string_view source) {
            if (kind!=17 && kind!=11) throw std::logic_error("다른 원본 메시지 생성자 미연결");
            const auto token=common.token();auto& node=*messages.emplace(token,std::make_unique<WarningNode>()).first->second;
            if (kind==11) {
                if (!pauseMessage) throw std::logic_error("Pause 메시지의 현재 모드/이름/번역 미연결");
                pauseMessage->Construct(token,kind,player,parameter);return MessageBoardRecord{token,node.serial,kind,player,parameter};
            }
            menus.TextBackend().Construct(token,0,0,0,0);node.vtable=kWarningVtable;node.kind=kind;node.player=player;node.parameter=parameter;
            sound();Style(token,source);node.deadline=WarningDeadline(now());common.lifetime.SetIdle(token,true);menus.TextBackend().Fit(token);
            return MessageBoardRecord{token,node.serial,kind,player,parameter};
        };
        hooks.width=[this](auto token) {
            const auto width=common.node(token)->baseWidth;
            if (!std::isfinite(width) || width<0 || static_cast<double>(width)>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Warning 원본 ftol 폭 범위");
            return static_cast<std::int32_t>(width);
        };
        hooks.attach=[this](auto token,auto x,auto y) { common.tree.Attach(token,static_cast<float>(x),static_cast<float>(y),common.create.entries[0],0); };
        board=std::make_unique<MessageBoard>(boardState,std::move(hooks));
        menus.SetExternalGumps(kDrawSlot,{[this](auto token) { return Node(token); },[this](auto token) { return Node(token)!=nullptr; },
            [this](auto token,auto&,auto&,auto&) { menus.TextBackend().Draw(token); }});
    }
    // 파생 주소는 공통 기본 주소와 같으며 실제 free 전까지 보존한다.
    WarningNode* Node(std::uint32_t token) const {
        const auto it=messages.find(token);return it==messages.end() ? nullptr : it->second.get();
    }
    // 실제 소유 조각을 공통 파서에 만든다. 별도 문자열 조각 표는 없다.
    void Style(std::uint32_t token,std::string_view value) {
        auto& node=*messages.at(token);node.source=value;menus.StyleBackend().Parse(node.head,value,node.textContext);
    }
    // 등록표 전체나 예전 루트 토큰을 쓰지 않고 현재 루트의 직계 자식을 읽는다.
    std::optional<MessageBoardRecord> Lookup(std::uint32_t serial) const {
        const auto root=common.create.entries[0];if (!root) return std::nullopt;std::size_t visited=0;
        // 메뉴/생산/미니맵 형제도 같은 링크를 따라가며 메시지로 잘못 역참조하지 않는다.
        for (auto token=common.node(root)->child;token;token=common.node(token)->next) {
            if (++visited>kGumpRegistryCapacity) throw std::logic_error("Warning 직계 자식 순환");
            const auto& base=*common.node(token);
            if (base.serial==serial && !(base.flags&kDead)) if (const auto* node=Node(token)) return MessageBoardRecord{token,serial,node->kind,node->player,node->parameter};
        }
        return std::nullopt;
    }
};
// 소유자는 RawButtonControls이며 가상 분배 설치 후에만 생성/Idle을 실행한다.
WarningMessages::WarningMessages(RawMenuTree tree,RawMenuControls& menus,std::function<double()> now,std::function<void()> sound,MessageGumpHooks pauseContext)
    :impl_(std::make_unique<Impl>(std::move(tree),menus,std::move(now),std::move(sound),std::move(pauseContext))) {}
// 부모가 공통 큐를 먼저 비운 뒤 빌린 Draw callback만 해제한다.
WarningMessages::~WarningMessages() { impl_->menus.SetExternalGumps(kDrawSlot,{}); }
// 화면/팔레트도 동일한 실제 루트와 글자 전역에 전달한다. 기존 문맥 사본은 유지한다.
void WarningMessages::SetView(int width,int height,const PaletteColorTable& palette) {
    if (width<=0 || height<=0) throw std::invalid_argument("Warning 화면 크기 오류");
    auto& s=*impl_;s.menus.SetPalette(palette);s.boardState.screenWidth=width;s.common.create.screenWidth=width;s.common.create.screenHeight=height;
    if (const auto root=s.common.create.entries[0]) { auto& node=*s.common.node(root);node.baseWidth=node.width=static_cast<float>(width);node.baseHeight=node.height=static_cast<float>(height); }
}
// 원본 명령과 같은 종류17/플레이어0/표시1/매개변수0이다.
void WarningMessages::Add(std::string_view styledText) { impl_->board->Show(17,0,true,0,styledText); }
// 시계/음향은 Pause 명령 소유자가 처리하며 이 경로는 같은20칸만 변경한다.
void WarningMessages::PauseStatus(bool enabled) { impl_->board->Show(11,0,enabled,0,""); }
// 같은 현재 직계 자식 검색 뒤 공통 지연 Kill을 실행한다.
void WarningMessages::Remove(std::int32_t kind,std::int32_t player,std::int32_t parameter) { impl_->board->Show(kind,player,false,parameter,""); }
// 기본/텍스트 가상 분배에 같은 안정된 파생 주소를 빌린다.
MessageGumpNode* WarningMessages::Node(std::uint32_t token) const { return impl_->Node(token); }
// 게임 시계 정지와 별개로 공통 idle 표에서 한 번 호출한다. 기한0은 벽 시각도 조회하지 않는다.
void WarningMessages::Idle(std::uint32_t token) {
    const auto deadline=impl_->messages.at(token)->deadline;
    if (deadline!=0.0 && WarningExpired(deadline,impl_->now())) { impl_->common.lifetime.Kill(token);impl_->common.changed(); }
}
// TextGump의 실제 체인 정리와 기본 scalar 소멸 순서를 그대로 사용한다.
void WarningMessages::DeleteScalar(std::uint32_t token,std::uint32_t flags) { impl_->menus.TextBackend().DeleteScalar(token,flags); }
// 공통 수명 관리자의 free 위치에서만 저장소를 반납한다.
void WarningMessages::Release(std::uint32_t token) { impl_->messages.erase(token); }
// 원본 MessageBoard와 같은 현재 루트 직계 자식 검색이다.
std::optional<MessageBoardRecord> WarningMessages::Lookup(std::uint32_t serial) const { return impl_->Lookup(serial); }
// dead/stale serial은 별도 표시 상태 없이 같은 실제 검색에서 제외한다.
bool WarningMessages::HasMessages() const {
    for (const auto serial:impl_->boardState.serials) if (impl_->Lookup(serial)) return true; // 원본20칸 순서를 유지한다.
    return false;
}
// 기존 원시 관측 형식은 유지하고 공유 등록표/루트/삭제 큐를 함께 보고한다.
std::string WarningMessages::Report() const {
    std::ostringstream out;out.precision(17);std::size_t count=0;
    // 실제20칸의 현재 직계 생존 노드만 출력한다.
    for (std::size_t i=0;i<impl_->boardState.serials.size();++i) if (const auto record=impl_->Lookup(impl_->boardState.serials[i])) {
        const auto& node=*impl_->messages.at(record->token);const auto position=impl_->common.collection.Position(record->token);++count;
        out<<"warning\t"<<i<<'\t'<<node.serial<<'\t'<<position.x<<'\t'<<position.y<<'\t'<<node.width<<'\t'<<node.height<<'\t'<<node.deadline<<'\t'<<node.source<<'\n';
    }
    out<<"warning_count\t"<<count<<'\n'<<"warning_registry\t"<<impl_->common.create.entries[0]<<'\t'<<impl_->common.create.count<<'\t'<<impl_->common.life.killCount<<'\n';return out.str();
}
}
