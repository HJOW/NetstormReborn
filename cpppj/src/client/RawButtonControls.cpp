// 가상 함수의 호스트 분배만 구성하며 개별 원본 몸체는 기존 Gump/Button 모듈을 호출한다.
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawProductionControls.h"
#include "client/RawMinimapControls.h"
#include "client/StoneButtonGump.h"
#include "client/GumpFormBinding.h"
#include <algorithm>
#include <bit>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 두 PE의 초기 DWORD 사건 값/타입 BYTE다. 메뉴 명령 번호와 Gump serial은 구별한다.
constexpr std::uint32_t kValueEvent=0x02000070,kClickEvent=0x0200002c,kRightEvent=0x02000012,kRepeatEvent=0x0200002f;
constexpr std::int8_t kButtonType=56,kStoneType=49,kBackgroundType=35;
// Gump 기본/배경 할당 크기와 즉시 버튼 플래그다. 모든 실제 파생 노드는 별도로 소유한다.
constexpr std::size_t kBackgroundSize=0xb8;
constexpr std::uint32_t kInstant=4;
// 보호10.78의005427e4 초기 DWORD다. Overlay는 현재 사건 대신 이 별도 전역을 가상+28에 전달한다.
constexpr std::uint32_t kOverlayEvent=0x0200003b;
// 원본 생산 목록 갱신 문자열 ID의 초기 DWORD다(PATCH00542708/CD00512890).
constexpr std::uint32_t kRefreshProductionEvent=0x02000003;
}
// 기본·돌 버튼·배경은 같은 기본 노드 주소로 조회되며 저장소 이동은 하지 않는다.
struct RawButtonControls::Impl {
    ProductionMaintenanceGlobals productionGlobals;ProductionMaintenancePanel productionPanel; // 전역 TYPE 시계/추첨 순번과 현재 월드 패널 필드는 다시 그려도 보존한다.
    // create: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState create;/* life: 원본의 고정 크기 idle/overlay/삭제/dialog 표와 현재 삭제 전역이다 (GumpLifetimeState). */ GumpLifetimeState life;/* treeState: 원본 지도 화면 영역과 패치판의 밀어내기 링크 진단 전역이다 (GumpTreeState). */ GumpTreeState treeState;/* eventState: 원본 전체 트리 커서 선택의 현재 좌표/필터/최소 signed 깊이/선택 토큰 전역이다 (GumpEventState). */ GumpEventState eventState;/* inputState: overlay 처리기는 호출자의 사건 대신 원본의 별도 사건 코드 전역을 사용한다 (GumpInputState). */ GumpInputState inputState;
    // buttonState: 실행 중 문자열 ID 전역 네 개와 기본 버튼 색 BYTE다. 사건 ID는 호출자가 실제 초기화 값을 공급한다 (ButtonGumpState).
    ButtonGumpState buttonState{kValueEvent,kClickEvent,kRightEvent,kRepeatEvent,kButtonType};
    // stoneState: 생성 시 읽는 색 BYTE·기본 글자 문맥·팔레트와 현재 배경 타입/프레임 전역이다 (StoneButtonGumpState).
    StoneButtonGumpState stoneState;/* backgroundColor: 배경 색. */ std::int8_t backgroundColor{kBackgroundType};/* assets: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계. */ RawButtonAssets assets;/* hooks: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (RawButtonHooks). */ RawButtonHooks hooks;
    // bases: 기본노드.
    std::map<std::uint32_t,std::unique_ptr<GumpCreateNode>> bases;
    // stones: 돌버튼.
    std::map<std::uint32_t,std::unique_ptr<StoneButtonGumpNode>> stones;
    // backgrounds: 배경.
    std::map<std::uint32_t,std::unique_ptr<GumpBackgroundNode>> backgrounds;
    // controls: 입력 대상인 UI 컨트롤의 목록.
    std::map<int,std::uint32_t> controls;/* nextToken: 새 등록 대상에 부여할 다음 토큰. */ /* root: 현재 UI 트리의 루트 토큰. */ std::uint32_t nextToken{},root{};/* currentPoll: 사건 하나: 코드와 화면 좌표(창 테두리를 뺀 커서 위치) (InputEvent). */ InputEvent currentPoll{InputCode::kRelease,0,0};
    // activated: 활성화.
    std::optional<int> activated;/* changed: 상태 변경으로 후속 갱신이 필요한지. */ bool changed{};
    // backgroundAsset: 실제 타입 저장소의 주소 토큰과 +dc SHP 포인터다. frame 조회 중에도 주소가 안정되어야 한다 (GumpBackgroundType).
    GumpBackgroundType backgroundAsset;/* backgroundFrame: 실제 SHP 프레임 헤더 +18/+1a의 signed WORD를 부호 확장하기 전 값이다 (GumpBackgroundFrame). */ GumpBackgroundFrame backgroundFrame;
    // factory: 타입에 맞는 객체를 생성하는 처리기.
    std::unique_ptr<GumpCreate> factory;/* collection: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (std::unique_ptr<GumpCollection>). */ std::unique_ptr<GumpCollection> collection;/* tree: 00466750/CD 00493070의 머리 삽입과 원본의 후속 정렬/현재 자식 재배치를 담당한다 (std::unique_ptr<GumpTree>). */ std::unique_ptr<GumpTree> tree;
    // lifetime: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    std::unique_ptr<GumpLifetime> lifetime;/* events: 발생한 이벤트 또는 이벤트 처리기. */ std::unique_ptr<GumpEvents> events;/* input: 처리할 입력 또는 입력 분배기. */ std::unique_ptr<GumpInput> input;
    // frame: 사용할 그림 프레임 또는 프레임 처리 자료.
    std::unique_ptr<GumpFrame> frame;/* buttons: 돌 버튼 생성/텍스트/부착은 StoneButtonGump에 맡기는 상태 처리기다. GumpEvents/입력/프레임 훅에 연결한다 (std::unique_ptr<ButtonGump>). */ std::unique_ptr<ButtonGump> buttons;/* backgroundCreate: 기본 생성/등록은 GumpCreate, 부착/파괴는 같은 저장소의 GumpTree/GumpLifetime이 맡는다 (std::unique_ptr<GumpBackgroundCreate>). */ std::unique_ptr<GumpBackgroundCreate> backgroundCreate;
    // stone: 같은 ButtonGump/GumpCreate/GumpTree/GumpLifetime을 사용한다. 자동 Client 연결은 별도 계층이 맡는다 (std::unique_ptr<StoneButtonGump>).
    std::unique_ptr<StoneButtonGump> stone;
    std::unique_ptr<RawProductionControls> production; // 같은 등록표와 현재 Session의 생산 패널이다.
    o::MetadisplayDirtyState minimapState; // 원본전역bitmap/큐/프레임은개별Gump와월드교체보다오래살아있다.
    std::unique_ptr<RawMinimapControls> minimap; // 같은 Gump/Pool의 실제 미니맵이며 월드보다 먼저 반납한다.
    std::unique_ptr<RawMenuControls> menus; // 실제 메뉴도 같은 루트/serial/등록표/입력/수명을 공유한다.
    // 모든 분배 훅은 실제 호출 때 현재 노드와 처리기를 조회하여 순환 의존을 연결한다.
    Impl(o::OriginalEdition edition,RawButtonHooks supplied):hooks(std::move(supplied)) {
        create.collection.edition=edition;stoneState.color=kStoneType;stoneState.defaultContext={0,0,0,0,0,0,1,1};
        inputState.overlayEvent=kOverlayEvent; // 새 Client 포인터 연결의 원본 별도 사건 전역이며 caller의 raw code와 구별한다.
        factory=std::make_unique<GumpCreate>(create,GumpCreateHooks{
            [this](auto token) { return Node(token); },[this](auto bytes) { return Allocate(bytes); }});
        collection=std::make_unique<GumpCollection>(create.collection,factory->CollectionHooks());
        GumpTreeHooks treeHooks;treeHooks.node=[this](auto token) { return Node(token); };
        treeHooks.redraw=[this](auto token,auto erase) { if (menus && menus->Node(token)) menus->Redraw(token,erase);else changed=true; };treeHooks.push=[this](auto token) { tree->Push(token); };
        treeHooks.attach=[this](auto token,float x,float y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };
        treeHooks.kill=[this](auto token) { lifetime->Kill(token); };
        tree=std::make_unique<GumpTree>(create,treeState,std::move(treeHooks));
        GumpLifetimeHooks lifeHooks;lifeHooks.node=[this](auto token) { return Node(token); };
        lifeHooks.push=[this](auto token) { tree->Push(token); };
        lifeHooks.deleteForm=[this](std::uint32_t form,std::uint32_t flags) { if (!hooks.deleteForm) throw std::logic_error("현재UI Form삭제소비자미연결");hooks.deleteForm(form,flags); };
        lifeHooks.deletingDestructor=[this](auto token,auto flags) {
            if (minimap && minimap->Node(token)) minimap->DeleteScalar(token,flags);else if (production && production->Node(token)) production->DeleteScalar(token,flags);else if (menus && menus->Node(token)) menus->DeleteScalar(token,flags);else if (stones.contains(token)) stone->DeleteScalar(token,flags);else lifetime->DeleteScalar(token,flags);
        };
        lifeHooks.release=[this](auto token) { if (minimap) minimap->Release(token);if (production) production->Release(token);if (menus) menus->Release(token);bases.erase(token);stones.erase(token);backgrounds.erase(token); };
        lifetime=std::make_unique<GumpLifetime>(create,life,std::move(lifeHooks));
        GumpEventHooks eventHooks;eventHooks.node=[this](auto token) { return Node(token); };
        eventHooks.handle=[this](auto token,auto event,auto first,auto second) { return Handle(token,event,first,second); };
        events=std::make_unique<GumpEvents>(create,life,eventState,*collection,std::move(eventHooks));
        GumpInputHooks inputHooks;inputHooks.node=[this](auto token) { return Node(token); };
        inputHooks.handle=[this](auto token,auto event,auto first,auto second) { return Handle(token,event,first,second); };
        input=std::make_unique<GumpInput>(create,life,inputState,*collection,*events,std::move(inputHooks));
        GumpFrameHooks frameHooks;frameHooks.node=[this](auto token) { return Node(token); };
        frameHooks.redraw=[this](auto,bool) { changed=true; };
        frameHooks.idle=[this](auto token) { if (stones.contains(token) || (menus && menus->Button(token))) buttons->Idle(token);else if (minimap && minimap->Node(token)) minimap->Idle(token);else if (production && production->Node(token)) production->Idle(token);else if (menus && menus->Node(token)) menus->Idle(token); };
        frameHooks.broadcast=[this](auto token,auto event,auto first,auto second) {
            if (stones.contains(token) || (menus && menus->Button(token))) buttons->Broadcast(token,event,first,second);
            else if (menus && menus->Node(token)) menus->Broadcast(token,event,first,second);
        };
        frame=std::make_unique<GumpFrame>(create,life,std::move(frameHooks));
        ButtonGumpHooks buttonHooks;
        buttonHooks.node=[this](auto token)->ButtonGumpNode* { const auto found=stones.find(token);return found==stones.end() ? (menus ? menus->Button(token) : nullptr) : found->second.get(); };
        buttonHooks.baseNode=[this](auto token) { return Node(token); };
        buttonHooks.backgroundFlags=[this](auto token)->std::uint32_t* { const auto found=backgrounds.find(token);auto* background=found==backgrounds.end() ? (menus ? menus->Background(token) : nullptr) : found->second.get();return background ? &background->backgroundFlags : nullptr; };
        buttonHooks.now=[this] { return hooks.now ? hooks.now() : 0.0; };
        buttonHooks.poll=[this](auto code) { return hooks.poll ? hooks.poll(code) : currentPoll; };
        buttonHooks.redraw=[this](auto,bool) { changed=true; };
        buttonHooks.sound=[this] { if (hooks.sound) hooks.sound(); };
        buttonHooks.baseEvent=[this](auto token,auto event,auto first,auto second) { return menus ? menus->Handle(token,event,first,second) : 0U; }; // 같은 기본 GumpHelp로 위임하며 Button Handle을 다시 호출하지 않는다.
        buttons=std::make_unique<ButtonGump>(create,buttonState,*factory,*collection,*events,*input,*lifetime,std::move(buttonHooks));
        GumpBackgroundCreateHooks backgroundHooks;
        backgroundHooks.node=[this](auto token) { const auto found=backgrounds.find(token);return found==backgrounds.end() ? nullptr : found->second.get(); };
        backgroundHooks.type=[this](auto type)->const GumpBackgroundType* { return type==assets.type ? &backgroundAsset : nullptr; };
        backgroundHooks.frame=[this](auto,auto,auto number)->const GumpBackgroundFrame* { return number==assets.frame ? &backgroundFrame : nullptr; };
        backgroundCreate=std::make_unique<GumpBackgroundCreate>(create,*factory,backgroundColor,std::move(backgroundHooks));
        StoneButtonGumpHooks stoneHooks;
        stoneHooks.measure=[this](auto,auto bytes,const auto& context,auto& width,auto& height,auto limit) { if (!menus) throw std::logic_error("공통 돌 버튼의 실제 글꼴 계층 미설치");menus->MeasureText(bytes,context,width,height,limit); }; // Help/화살표 Fit도 같은 원본 측정기다.
        stoneHooks.node=[this](auto token) { const auto found=stones.find(token);return found==stones.end() ? (menus ? menus->Button(token) : nullptr) : found->second.get(); };
        stoneHooks.allocate=[this](auto bytes) { return Allocate(bytes); };stoneHooks.push=[this](auto token) { tree->Push(token); };
        stoneHooks.attach=[this](auto token,float x,float y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };
        stone=std::make_unique<StoneButtonGump>(create,stoneState,*factory,*buttons,*backgroundCreate,*tree,*lifetime,std::move(stoneHooks));
        root=factory->InitializeRoot();
    }
    // 가상 호출 동안 노드 주소는 유지되며 실제 scalar 소멸자에서만 erase한다.
    GumpCreateNode* Node(std::uint32_t token) const {
        if (const auto it=stones.find(token);it!=stones.end()) return it->second.get();
        if (const auto it=backgrounds.find(token);it!=backgrounds.end()) return it->second.get();
        if (minimap) if (auto* node=minimap->Node(token)) return node;
        if (production) if (auto* node=production->Node(token)) return node;
        if (menus) if (auto* node=menus->Node(token)) return node;
        const auto it=bases.find(token);return it==bases.end() ? nullptr : it->second.get();
    }
    // 원본 new 요청 크기에 맞는 파생 저장소를 준비한다. 호스트 토큰은 재사용하지 않는다.
    std::uint32_t Allocate(std::size_t bytes) {
        const auto token=++nextToken;
        if (bytes==kBackgroundSize) backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());
        else bases.emplace(token,std::make_unique<GumpCreateNode>());
        return token;
    }
    // 돌 버튼 가상 부착의 실제 배경 Push/Attach를 먼저 실행한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (minimap && minimap->Node(token)) minimap->Attach(token,x,y,parent,mode);else if (production && production->Node(token)) production->Attach(token,x,y,parent,mode);else if (menus && menus->Node(token)) menus->Attach(token,x,y,parent,mode);else if (stones.contains(token)) stone->Attach(token,x,y,parent,mode);else tree->Attach(token,x,y,parent,mode);
    }
    // 루트 부모 click의 serial을 화면 명령 번호로 변환한다. right/repeat는 미지원 상위 명령으로 보존한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        if (stones.contains(token) || (menus && menus->Button(token))) return buttons->Handle(token,event,first,second);
        if (minimap && minimap->Node(token)) return minimap->Handle(token,event,first,second);
        if (production && production->Node(token)) return production->Handle(token,event,first,second);
        if (menus && menus->Node(token)) return menus->Handle(token,event,first,second);
        if (token==root && (event&0x03ffffffU)==buttonState.clickEvent) {
            // ID와 serial의 크기/순서를 같다고 가정하지 않는다.
            for (const auto& [id,button]:controls) if (Node(button)->serial==first) { activated=id;return 1; }
        }
        return 0;
    }
    // 자식 먼저 Kill한 실제 큐를 비우며 루트/serial/count를 임의 초기화하지 않는다.
    void Clear(bool preserveWorldUi=false) {
        if (preserveWorldUi) {
            // 이 페이지가 만든 돌 버튼과 그 배경만 같은 자식 우선 수명으로 반납한다.
            for (const auto& control:controls) lifetime->Kill(control.second);
        } else {
            // Kill이 root.child를 바꾸므로 현재 머리를 매번 읽는다.
            while (Node(root)->child) lifetime->Kill(Node(root)->child);
        }
        lifetime->Cleanup();if (!preserveWorldUi) { minimap.reset();production.reset(); }controls.clear();activated.reset();
    }
};
// 구현 주소는 생성 이후 이동하지 않아 순환 훅이 안정된 처리기를 조회한다.
RawButtonControls::RawButtonControls(o::OriginalEdition edition,RawButtonHooks hooks):impl_(std::make_unique<Impl>(edition,std::move(hooks))) {}
// 소유자 해제 시에도 복원된 가상 소멸자가 실제 저장소/표를 정리한다.
RawButtonControls::~RawButtonControls() { impl_->Clear();impl_->lifetime->Kill(impl_->root);impl_->lifetime->Cleanup(); }
// 쓰기 전에 전체 화면 입력을 검증하고 이전 페이지를 실제 수명 경로로 정리한다.
void RawButtonControls::SetControls(std::span<const GumpControl> controls,std::span<const std::string> labels,
    int width,int height,RawButtonAssets assets,bool preserveWorldUi) {
    auto& state=*impl_;if (width<=0 || height<=0 || controls.size()*2+1>=kGumpRegistryCapacity) throw std::out_of_range("raw 버튼 화면/개수 범위 오류");
    std::map<int,bool> ids;
    // 메뉴 행은 다른 실제 처리기의 대상이므로 돌 버튼 등록에 포함하지 않는다.
    for (const auto& control:controls) if (!control.menu) {
        if (control.id<0 || static_cast<std::size_t>(control.id)>=labels.size() || !ids.emplace(control.id,true).second ||
            control.rect.right<=control.rect.left || control.rect.bottom<=control.rect.top) throw std::invalid_argument("raw 버튼 입력 오류");
    }
    state.Clear(preserveWorldUi);state.assets=assets;state.backgroundAsset={assets.type,assets.type};state.backgroundFrame={assets.width,assets.height};
    state.stoneState.palette=assets.palette;state.stoneState.backgroundType=assets.type;state.stoneState.backgroundFrame=assets.frame;
    state.create.screenWidth=width;state.create.screenHeight=height;auto& root=*state.Node(state.root);
    root.baseWidth=root.width=static_cast<float>(width);root.baseHeight=root.height=static_cast<float>(height);
    // 화면이 공급한 전체 버튼 경계를 같은 생성자/입력/Draw 저장소에 전달한다.
    for (const auto& control:controls) if (!control.menu) {
        const auto token=++state.nextToken;state.stones.emplace(token,std::make_unique<StoneButtonGumpNode>());
        state.stone->Construct(token,control.rect.right-control.rect.left,control.rect.bottom-control.rect.top,labels[static_cast<std::size_t>(control.id)].c_str());
        state.controls.emplace(control.id,token);if (control.instant) state.stones.at(token)->buttonFlags|=kInstant;
        state.buttons->SetEnabled(token,control.enabled);state.stone->Attach(token,static_cast<float>(control.rect.left),static_cast<float>(control.rect.top),state.root,1);
    }
    state.changed=true;
}
// 명령은 부모 가상 통지 시에만 생성된다. release의 화면 위치만으로 새 명령을 만들지 않는다.
std::optional<int> RawButtonControls::Event(InputEvent event) { return RouteInput(event).activated; }
// 성공 Gump 토큰을 잃지 않아 같은 사건을 UI와 월드에 두 번 적용하지 않는다.
RawButtonInputResult RawButtonControls::RouteInput(InputEvent event) {
    auto& state=*impl_;state.activated.reset();state.currentPoll=event;
    const auto consumed=state.input->Route(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y)); // 현재 공통 focus/dialog/root 순서를 그대로 쓴다.
    return {consumed,state.activated};
}
// 루트 교체 뒤에도 처음 생성한 토큰의 사본을 반환하지 않는다.
std::uint32_t RawButtonControls::RootToken() const { return impl_->create.entries[0]; }
// count/visible로 대체하지 않고 원문처럼 고정 등록칸과 현재 serial/dead만 읽는다.
std::uint32_t RawButtonControls::SerialGump(std::uint32_t serial) const {
    for (std::size_t index=1;index<impl_->create.entries.size();++index) { // 등록0칸의 루트는 검색하지 않는다.
        const auto token=impl_->create.entries[index];if (!token) continue;const auto& node=*impl_->Node(token); // 각 현재 비영 슬롯만 역참조한다.
        if (node.serial==serial && !(node.flags&0x10000U)) return token;
    }
    return 0;
}
// Kill에서 즉시 해제하지 않아 현재 메뉴 입력 callback의 나머지 실행을 보존한다.
void RawButtonControls::KillGump(std::uint32_t token) { impl_->lifetime->Kill(token);impl_->changed=true; }
// 004d68c2의 현재 focus/dialog 관문 뒤004d69f4의 raw 사건 통지를 일반 큐보다 먼저 실행한다.
RawButtonPointerResult RawButtonControls::NotifyPointer(InputEvent event,std::uint32_t mode,bool cursorActive,bool allowOverlay) {
    auto& state=*impl_;state.currentPoll=event;state.activated.reset();
    RawButtonPointerResult result; // 미호출 Overlay와 실제 반환0을 구별하여 커서 fallback을 상위에 전달한다.
    if (!(event.code&InputCode::kOutside)) {
        const bool overlayAllowed=state.life.currentDialog!=0 || (allowOverlay && mode==0 && !state.life.focus && !cursorActive); // 실제 modal OR는 host 차단/focus/활성 커서 조건보다 바깥이며 현재 Pick이 modal 앞 대상을 고른다.
        if (overlayAllowed) result.overlay=state.input->Overlay(event.code,{event.x,event.y});
        state.frame->Broadcast(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y));
    }
    // 원본 밖 포인터는 이 두 통지를 건너뛴다. 합성 release 보정은 별도 입력 준비 경계이며 여기서 캡처를 임의 해제하지 않는다.
    result.activated=std::exchange(state.activated,{});result.changed=std::exchange(state.changed,false);return result;
}
// Kill 예약을 같은 공통 표의 가상 소멸로 옮긴다. 현재 삭제 큐 재조회는 기존 GumpLifetime이 소유한다.
bool RawButtonControls::CleanupGumps() { impl_->lifetime->Cleanup();return std::exchange(impl_->changed,false); }
// 같은 idle 표에 대한 실제 가상 분배만 실행한다. Idle 중 Kill한 대상은 다음 Cleanup까지 실제 저장소에 남는다.
bool RawButtonControls::IdleGumps() { impl_->frame->Idle();return std::exchange(impl_->changed,false); }
// 기존 독립 검사 계약을 유지하는 결합 경계다. Client 프레임 연결에서는 사용하지 않는다.
bool RawButtonControls::Poll(InputEvent event) {
    auto& state=*impl_;state.currentPoll=event;
    if (event.code&InputCode::kOutside) Cancel();
    state.frame->Broadcast(0,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y));state.frame->Idle();
    const bool result=state.changed;state.changed=false;return result;
}
// 캡처/idle/누름 해제는 같은 ButtonGump의 release 몸체를 사용한다.
void RawButtonControls::Cancel() {
    auto& state=*impl_;if (state.life.focus) state.input->Route(InputCode::kLeftButton|InputCode::kRelease,0xffffffffU,0xffffffffU);
    state.activated.reset();
}
// 현재 입력 드레인의 후속 사건이 반납된 HTML 자식을 다시 pick/발화하지 못하게 한다.
void RawButtonControls::RetirePageControls() {
    auto& state=*impl_;
    for (const auto& control:state.controls) state.lifetime->Kill(control.second); // 버튼/자식 배경은 기존 가상 Kill을 따른다.
    state.controls.clear();state.activated.reset();state.changed=true;
}
// 캡처 우선 및 includeHidden=1의 실제 원본 분배이며 UI 상태를 임의 갱신하지 않는다.
void RawButtonControls::RefreshProduction() { impl_->input->Route(kRefreshProductionEvent,0,0,true); }
// 공통 등록표와 현재 파생 가상 분배를 그대로 빌려 메뉴 처리기를 생성한다.
RawMenuControls& RawButtonControls::EnableMenus(const GameAssets& assets,FontStore& fonts,RawMenuHooks hooks) {
    auto& s=*impl_;if (s.menus) throw std::logic_error("실제 메뉴 계층은 한 번만 연결 가능");
    RawMenuTree tree{s.create,s.life,*s.factory,*s.collection,*s.tree,*s.lifetime,*s.events,*s.input,*s.stone,
        [&s](auto token) { const auto it=s.backgrounds.find(token);return it==s.backgrounds.end() ? (s.production ? s.production->Background(token) : nullptr) : it->second.get(); },
        [&s] { return ++s.nextToken; },[&s](auto token) { return s.Node(token); },
        [&s](auto token,float x,float y,auto parent,auto mode) { s.Attach(token,x,y,parent,mode); },[&s] { s.changed=true; }};
    s.menus=std::make_unique<RawMenuControls>(std::move(tree),assets,fonts,std::move(hooks));return *s.menus;
}
// 별도 메뉴 트리 사본을 만들지 않고 같은 현재 확장 계층을 반환한다.
RawMenuControls* RawButtonControls::Menus() const { return impl_->menus.get(); }
// 같은 공통 참조와 가상 분배를 생산 노드에도 빌려 준다.
RawProductionControls& RawButtonControls::EnableProduction(RawCampaignSession& session,const GameAssets& assets,RawProductionHooks hooks) {
    auto& s=*impl_;if (s.production || !s.menus) throw std::logic_error("생산 패널 수명/메뉴 연결 오류");
    RawMenuTree tree{s.create,s.life,*s.factory,*s.collection,*s.tree,*s.lifetime,*s.events,*s.input,*s.stone,
        [&s](auto token) { const auto it=s.backgrounds.find(token);return it==s.backgrounds.end() ? nullptr : it->second.get(); },
        [&s] { return ++s.nextToken; },[&s](auto token) { return s.Node(token); },
        [&s](auto token,float x,float y,auto parent,auto mode) { s.Attach(token,x,y,parent,mode); },[&s] { s.changed=true; }};
    s.production=std::make_unique<RawProductionControls>(std::move(tree),*s.menus,session,assets,s.productionGlobals,s.productionPanel,std::move(hooks));s.production->Open();return *s.production;
}
RawProductionControls* RawButtonControls::Production() const { return impl_->production.get(); } // 같은 현재 처리기다.
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 게임 설정 파일에 저장하지 않는다.
void RawButtonControls::ConfigureTestBridgePatterns(std::string_view patterns) { impl_->productionGlobals.ConfigureTestBridgePatterns(patterns); }
void RawButtonControls::DisableProduction() { if (impl_->production) { impl_->production->Close();impl_->production.reset(); }impl_->productionPanel={}; } // 월드를 떠날 때 패널 필드만 새로 만들며 TYPE 시계와 순번은 전역 수명을 유지한다.

// 같은 등록표/수명/입력/원본 셀 그림을 현재Session과 공유한다.
RawMinimapControls& RawButtonControls::EnableMinimap(RawCampaignSession& session,RawMinimapHooks hooks) {
    auto& s=*impl_;if (s.minimap || !s.menus) throw std::logic_error("미니맵 공통 트리/수명 오류");
    RawMenuTree tree{s.create,s.life,*s.factory,*s.collection,*s.tree,*s.lifetime,*s.events,*s.input,*s.stone,
        [&s](auto token) { const auto it=s.backgrounds.find(token);return it==s.backgrounds.end() ? nullptr : it->second.get(); },
        [&s] { return ++s.nextToken; },[&s](auto token) { return s.Node(token); },
        [&s](auto token,float x,float y,auto parent,auto mode) { s.Attach(token,x,y,parent,mode); },[&s] { s.changed=true; }};
    s.minimap=std::make_unique<RawMinimapControls>(std::move(tree),*s.menus,session,s.minimapState,std::move(hooks));s.minimap->Open();return *s.minimap;
}
// 같은 실제 미니맵의 안정된 처리기만 빌린다.
RawMinimapControls* RawButtonControls::Minimap() const { return impl_->minimap.get(); }
// Pop/Unpop이현재Session의해제된dirty를호출하지않도록노드보다먼저월드를반납하지않는다.
void RawButtonControls::DisableMinimap() { if (impl_->minimap) { impl_->minimap->Close();impl_->minimap.reset(); } }

// Form소유ID는알림시점의등록표에서해석하고기본가상+4의지연Kill만요청한다.
o::GumpFormNotice RawButtonControls::ObjectNotice() const {
    auto* current=impl_.get();return MakeGumpFormNotice(current->create,[current](std::uint32_t token,o::Sid object) {
        if (current->production && current->production->Node(token)) current->production->ParentGone(token,object.value);else ReleaseGumpObject(*current->lifetime,token,object);current->changed=true;
    });
}
// 원본00463ff0과 같은 실제 등록 배열을 요청 시 읽는다. 등록 없는0은 그대로 반환한다.
std::uint32_t RawButtonControls::RegisteredGump(std::uint32_t id) const {
    if (id>=impl_->create.entries.size()) throw std::out_of_range("현재 Gump 등록ID 물리 범위 오류");
    return impl_->create.entries[id];
}
// 실제 노드가 존재하는 대상의 가상 처리기에만 직접 보낸다.
std::uint32_t RawButtonControls::HandleGump(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
    if (!token || !impl_->Node(token)) throw std::out_of_range("현재 직접 Gump 가상 처리 대상 없음");
    return impl_->Handle(token,event,first,second);
}
// 커서 Query의465ed0은 전체 자식 선택466020과 달리 루트 직계 자식만 검사한다.
std::uint32_t RawButtonControls::PickGump(ScreenPoint point) const { return impl_->events->PickTopLevel(point); }
// 같은 등록표/수명/현재 노드로4651b0을 호출하며 성공 처리기의 DWORD를 토큰으로 대체하는 원본 계약을 유지한다.
std::uint32_t RawButtonControls::DispatchGump(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden) const {
    if (!token || !impl_->Node(token)) throw std::out_of_range("현재 재귀 Gump 처리 대상 없음");
    return impl_->events->Dispatch(token,event,first,second,includeHidden);
}
// 원본 커서의cursorLegalGump->parentSid는 UI 트리 부모가 아니라+24의게임 SID다.
std::uint32_t RawButtonControls::GumpParentSid(std::uint32_t token) const {
    const auto* node=token ? impl_->Node(token) : nullptr;
    if (!node) throw std::out_of_range("현재 Gump 부모 SID 조회 대상 없음");
    return node->object;
}
// 캐시를 만들지 않아 파생 부착/깊이 갱신 직후에도 현재 저장 필드를 읽는다.
std::uint32_t RawButtonControls::GumpDepth(std::uint32_t token) const {
    const auto* node=token ? impl_->Node(token) : nullptr;
    if (!node) throw std::out_of_range("현재 Gump 깊이 조회 대상 없음");
    return node->depth;
}
// 같은 메뉴 처리기가 사용하는 현재 루트/수명에 Kill을 요청하고 실제 해제는 기존 후속 정리에 맡긴다.
void RawButtonControls::RequestClosePopups() {
    if (!impl_->menus) throw std::logic_error("실제 Gump 팝업 수명 처리기 미연결");
    impl_->menus->RequestClosePopups();
}
// 원본처럼 현재 직계 연결을 순회한다. dead/visible 필터를 추가하지 않는다.
bool RawButtonControls::BlockingGumpOpen() const {
    // 콜백 없이 읽기만 하므로 현재 자식의 다음 링크를 순서대로 읽는다.
    for (auto token=impl_->Node(impl_->root)->child;token;token=impl_->Node(token)->next)
        if ((impl_->Node(token)->flags&0x4000U)!=0) return true;
    return false;
}
// 렌더링이 이전 어댑터의 캡처 사본을 조회하지 않게 현재 raw 상태만 읽는다.
std::optional<int> RawButtonControls::Pressed() const {
    // 표시 명령 번호 순서는 부모 트리 방문 순서와 구별한다.
    for (const auto& [id,token]:impl_->controls) { const auto& node=*impl_->stones.at(token);if (node.value || node.pressed) return id; }
    return {};
}
// 기존 별도 사각형 캡처 사본이 아닌 실제 GumpInput focus 전역이다.
bool RawButtonControls::HasCapture() const { return impl_->life.focus!=0; }
// 원본 Draw가 현재 edge 전역을 읽는 반면 잉크/그림자는 생성 시 복사된 문맥을 읽는다.
void RawButtonControls::SetPalette(ButtonPalette palette) { impl_->assets.palette=palette;impl_->stoneState.palette=palette;impl_->changed=true; }
// 새 호스트 노드의 마지막 바이트는 0이며 원본 생성자의 보존/복사 범위는 StoneButtonGump가 처리한다.
std::string RawButtonControls::Label(int id) const {
    const auto& bytes=impl_->stones.at(impl_->controls.at(id))->label;
    return {bytes.begin(),std::find(bytes.begin(),bytes.end(),'\0')};
}
// 글자 측정은 기존 Renderer가 공급하되 좌표/문맥/상태는 실제 돌 버튼 저장소에서 읽는다.
ButtonDrawPlan RawButtonControls::DrawPlan(int id,std::int32_t textWidth,std::int32_t textHeight) const {
    auto& state=*impl_;const auto found=state.controls.find(id);if (found==state.controls.end()) throw std::out_of_range("raw 출력 버튼 ID 없음");
    auto& node=*state.stones.at(found->second);node.textWidth=textWidth;node.textHeight=textHeight;return state.stone->DrawPlan(found->second);
}
// 부착 생성자가 만든 실제 자식의 현재 raw 값만 사용한다.
std::vector<GumpTexturePass> RawButtonControls::BackgroundPlan(int id,ScreenRect clip) const {
    const auto& state=*impl_;const auto button=state.controls.at(id);const auto token=state.stones.at(button)->background;
    const auto& background=*state.backgrounds.at(token);
    return PlanGumpBackground(state.collection->Bounds(token),clip,static_cast<std::int16_t>(background.tileWidth),
        static_cast<std::int16_t>(background.tileHeight),background.backgroundFlags);
}
// 검사 보고는 UI 실행 상태를 드러내며 게임 플레이 완료를 뜻하지 않는다.
std::string RawButtonControls::Report() const {
    const auto& state=*impl_;std::ostringstream out;
    out<<"RawButtons\t"<<state.controls.size()<<'\t'<<state.create.count<<'\t'<<state.create.serial<<'\t'<<state.life.focus<<'\t'
       <<std::count_if(state.life.idle.begin(),state.life.idle.end(),[](auto token) { return token!=0; })<<'\t'
       <<state.bases.size()+state.stones.size()+state.backgrounds.size();if (state.production) out<<'\n'<<state.production->Report();if (state.minimap) out<<'\n'<<state.minimap->Report();return out.str();
}
}
