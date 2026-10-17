// 기존 원본 몸체들의 계산을 재구현하지 않고 실제 월드의 단일 상태로 연결한다.
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/RawCursorPreviews.h"
#include "client/RawCursorActions.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 장치 교체 후에도 현재 저장된 콜백을 다시 읽으며 누락을 명시한다.
template<class Result,class... Arguments>
std::function<Result(Arguments...)> Current(std::function<Result(Arguments...)>& callback,const char* name) {
    return [&callback,name](Arguments... arguments)->Result {
        if (!callback) throw std::logic_error(std::string("캠페인 커서 실제 경계 미연결: ")+name);
        return callback(std::forward<Arguments>(arguments)...);
    };
}
}
// 참조한 게임 몸체는 세션 소유이며 커서의 상태/모듈만 여기서 소유한다.
struct RawCampaignCursor::Impl {
    CampaignCursorData data; // 같은 월드/현재 커서/카메라의 참조다.
    RawWorldCore core; // 같은 풀/TYPE/지도/장부의 참조다.
    CampaignCursorGame game; // 실제 세션의 게임 계산과 현재 전역 갱신이다.
    CampaignCursorEffects effects; // 교체 가능한 현재 UI/장치/송신 경계다.
    CursorLifecycleState lifecycleState; // 현재 임시30SID와 글자 측정 상태다.
    CursorPlacementState placementState; // 현재 위치 허용/표시 범위다.
    CursorReturnPieceState returnState; // 현재 선택 타입/유지/소멸 옵션이다.
    o::ConstructionLocalState localState; // 실제 로컬 소유자/Canon 패턴이다.
    std::uint32_t snapToGrid{}; // 실제 위치 갱신이 읽는 snapToGrid DWORD다.
    std::unique_ptr<RawCampaignCursorDisplay> display; // 같은 현재 목록/격자/SP의 실제 수집/표시기다.
    std::unique_ptr<RawCursorPlacement> placement; // 실제 Query/Prepare 몸체다.
    std::unique_ptr<RawCursorPreviews> previews; // 실제 생성/재사용/연결 미리보기다.
    std::unique_ptr<RawCursorLifecycle> lifecycle; // 실제 Take/Cancel/Activate 몸체다.
    std::unique_ptr<RawCursorPosition> position; // 같은 카메라로 마우스를 변환한다.
    std::unique_ptr<o::RegularProcess> pendingTimer; // 실제 할당 성공부터 반환 조각에 타이머를 부착할 때까지 예약한다.
    std::unique_ptr<RawCursorReturnPiece> returned; // 같은 난수/contained의 조각 반환이다.
    std::unique_ptr<o::RawConstructionLocal> local; // 실제 예측 배치/Builder/요청 송신이다.
    std::unique_ptr<RawCursorActions> actions; // 같은 잠금/방향/Range의 실제 세 부모 몸체다.
    std::unique_ptr<RawCursorDrop> drop; // 실제 상위 비용/잠금/정리 순서다.

    // 다른 풀을 섞지 않고 실제 하위들을 먼저 만든 뒤 수명 몸체로 순환 연결한다.
    Impl(CampaignCursorData source,CampaignCursorGame operations):data(source),core(data.world.Core()),game(std::move(operations)) {
        if (&data.forms.Pool()!=&core.pool || &data.contained.Pool()!=&core.pool || &data.reward.Pool()!=&core.pool ||
            &data.construction.Pool()!=&core.pool || !game.prepare || !game.moveBuilder || !game.playDrop)
            throw std::invalid_argument("실제 캠페인 커서 단일 월드/게임 연결 오류");
        auto placementHooks=game.placement; // 게임 판정은 세션의 같은 실제 몸체를 사용한다.
        BindCursorPlacementAssets(placementHooks,data.assets,data.geometry);
        placementHooks.gumpAt=Current(effects.placement.gumpAt,"Gump 위치 조회");
        placementHooks.accept=Current(effects.placement.accept,"Gump 수신 판정");
        placementHooks.parent=Current(effects.placement.parent,"Gump 부모 조회");
        placementHooks.invalidate=Current(effects.placement.invalidate,"화면 dirty");
        placement=std::make_unique<RawCursorPlacement>(core.pool,core.types,core.hash.Entries(0),data.drop,lifecycleState,placementState,placementHooks);
        CursorPreviewHooks previewHooks; // 실제 Factory/HP/연결/가상 소유자를 공급한다.
        BindCursorPreviewPlacement(previewHooks,core.pool,*placement,placementHooks);
        BindCursorPreviewWorld(previewHooks,core.pool,core.factory,data.reward,data.world.BridgeConnections(),data.frames,core.bridgeState,
            [this](o::Sid sid) { core.building.NotifySurface(sid); });
        previewHooks.setOwner=o::MakeOwnerDispatch(core.pool,core.bridgeState,[this](o::Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); });
        previews=std::make_unique<RawCursorPreviews>(core.pool,core.types,data.drop,lifecycleState,core.bridgeState,std::move(previewHooks));
        auto lifecycleHooks=game.lifecycle;CursorDropHooks dropHooks; // 장치만 현재 콜백을 통해 호출한다.
        lifecycleHooks.highlight=dropHooks.highlight=Current(effects.lifecycle.highlight,"강조 해제");
        lifecycleHooks.selectCursor=dropHooks.selectCursor=Current(effects.lifecycle.selectCursor,"하드웨어 커서 선택");
        lifecycleHooks.closePopups=Current(effects.lifecycle.closePopups,"팝업 닫기");
        lifecycleHooks.translate=Current(effects.lifecycle.translate,"현재 번역");
        lifecycleHooks.measure=Current(effects.lifecycle.measure,"현재 글자 측정");
        lifecycleHooks.mouse=Current(effects.lifecycle.mouse,"현재 마우스");
        BindCursorLifecycleAssets(lifecycleHooks,data.assets);BindCursorPlacement(lifecycleHooks,dropHooks,*placement);
        BindCursorPreviews(lifecycleHooks,*previews);
        lifecycleHooks.updatePosition=[this](std::int32_t x,std::int32_t y) { position->Update(x,y); };
        lifecycle=std::make_unique<RawCursorLifecycle>(core.pool,core.types,data.drop,lifecycleState,std::move(lifecycleHooks));
        position=std::make_unique<RawCursorPosition>(data.drop,placementState,snapToGrid,data.camera,
            MakeCursorPositionHooks(*lifecycle,*previews,dropHooks.selectCursor));
        CursorReturnPieceHooks returnHooks; // 실제 메모리 예약과 같은 Host의 Regular 부착을 사용한다.
        returnHooks.allocateTimer=[this](std::uint32_t bytes) {
            if (bytes!=0x28) throw std::logic_error("조각 반환 원본 타이머 할당 크기 오류");
            try { pendingTimer=std::make_unique<o::RegularProcess>(0U,0.0f);return true; }catch (const std::bad_alloc&) { return false; }
        };
        returnHooks.createTimer=[this](o::Sid parent,std::uint32_t event,float duration) {
            if (!pendingTimer) throw std::logic_error("조각 반환 실제 타이머 예약 누락");
            auto* memory=pendingTimer.release();std::destroy_at(memory);std::construct_at(memory,event,duration);
            auto* attached=static_cast<o::RegularProcess*>(core.processes.Attach(std::unique_ptr<o::RegularProcess>(memory),46,parent,0x50));
            if (!attached) throw std::logic_error("조각 반환 실제 Regular 부착 실패");attached->ResetTask(event,core.processState.now);
        };
        returned=std::make_unique<RawCursorReturnPiece>(data.contained,returnState,data.random,std::move(returnHooks));
        o::ConstructionLocalHooks localHooks; // Form/반환/실제 Place/Builder/직렬화를 같은 게임에 연결한다.
        localHooks.previewSource=[this](o::Sid sid) { return o::SquidForms(core.pool).Parent(sid).value; };
        localHooks.destroyPreview=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        localHooks=MakeReturnPieceHooks(core.pool,*returned,std::move(localHooks));localHooks.moveBuilder=game.moveBuilder;
        localHooks.send=[this](std::uint32_t destination,const o::ConstructionLocalMessage& message) {
            std::array<std::uint8_t,25> packet{};o::WriteConstructionCommand(message,packet);
            Current(effects.send,"로컬 건설 송신")(destination,packet);
        };
        local=std::make_unique<o::RawConstructionLocal>(core.pool,core.types,data.frames,core.spots,localState,data.construction.BindLocal(std::move(localHooks)));
        dropHooks.playDrop=game.playDrop;
        dropHooks.previewSource=[this](o::Sid sid) { return o::SquidForms(core.pool).Parent(sid).value; };
        dropHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        dropHooks.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { data.world.Pop(sid,x,y,flags); };
        dropHooks.record=Current(effects.drop.record,"선택 기록기");
        dropHooks.placeLocal=[this](const o::ConstructionLocalRequest& request) { returnState.type=data.drop.type;local->Place(request); };
        dropHooks.addLocalSp=game.lifecycle.addLocalSp;
        dropHooks.showSp=Current(effects.drop.showSp,"비용 표시");
        dropHooks.battleRefresh=Current(effects.drop.battleRefresh,"전투 표시 갱신");
        dropHooks.invalidGui=[this] { lifecycle->Cancel(); };
        dropHooks.guiFlags=Current(effects.drop.guiFlags,"Gump 허용 DWORD");
        dropHooks.guiParent=Current(effects.drop.guiParent,"Gump 부착 부모");
        dropHooks.detach=[this](o::Sid sid,std::uint32_t flags) { game.lifecycle.detach(sid,flags); };
        dropHooks.attach=[this](o::Sid sid,std::uint32_t parent,std::uint32_t flags) {
            if (!parent || parent>=core.pool.Capacity()) throw std::out_of_range("커서 Gump 부모 SID 범위");
            data.forms.Attach(sid,o::Sid{static_cast<std::uint16_t>(parent)},flags);
        };
        dropHooks.guiReceive=Current(effects.drop.guiReceive,"Gump 조각 수신");
        dropHooks.returnPiece=[this](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { returnState.type=data.drop.type;returned->Return(flags,source,quality); };
        dropHooks.activatePreview=[this](std::uint32_t force) { lifecycle->Activate(force); };
        dropHooks.refresh=Current(effects.drop.refresh,"커서 변경 갱신");
        // 원본 cursorManaProcess의 가상+1c다. UI 토큰으로 바꾸지 않고 같은 Host의 현재 프로세스에 보낸다.
        dropHooks.closeGump=Current(game.lifecycle.processEvent,"현재 커서 프로세스 통지");
        drop=std::make_unique<RawCursorDrop>(core.pool,core.types,data.drop,std::move(dropHooks));
        CursorActionHooks actionHooks; // 기존 실제 하위 몸체만 같은 커서에 연결한다.
        actionHooks.activate=[this](std::uint32_t force) { lifecycle->Activate(force); };actionHooks.createPreviews=[this] { previews->Create(); };
        actionHooks.selectCursor=Current(effects.lifecycle.selectCursor,"하드웨어 커서 선택");actionHooks.sound=game.lifecycle.sound;
        actionHooks.removeRange=game.removeRange;actionHooks.createRange=game.lifecycle.createRange;
        actions=std::make_unique<RawCursorActions>(core.types,data.drop,lifecycleState,data.windType,data.rangeProcessType,std::move(actionHooks));
        display=std::make_unique<RawCampaignCursorDisplay>(core,CursorPreviewSource{core.pool,data.drop,lifecycleState,placementState,core.bridgeState,data.grid},data.assets,data.sp,data.playerKnown);
    }
    // 실제 가상 삭제의 거부를 성공한 임시 정리로 처리하지 않는다.
    void Destroy(o::Sid sid,std::uint32_t flags) { if (!data.world.Destroy(sid,flags)) throw std::logic_error("실제 커서 가상 삭제 거부"); }
    // 공개 명령마다 현재 전역을 반영한다. 취소 잠금 자체는 이곳에서 해석하지 않는다.
    void Prepare() { game.prepare();localState.localPlayer=data.drop.localPlayer;localState.patternTypes=data.geometry.patternTypes;returnState.type=data.drop.type;returnState.bridgeType=core.bridgeState.bridgeType;returnState.fort=core.ownerMode.fort; }
};
// 장기 참조의 주소를 바꾸지 않고 한 번 조립한다.
RawCampaignCursor::RawCampaignCursor(CampaignCursorData data,CampaignCursorGame game):impl_(std::make_unique<Impl>(data,std::move(game))) {}
RawCampaignCursor::~RawCampaignCursor()=default; // 상위 세션이 이미 Kernel을 비운 뒤 실제 몸체를 해제한다.
// 현재 callback 저장소만 바꾸므로 이미 구성한 몸체가 다음 호출에 새 장치를 읽는다.
void RawCampaignCursor::SetEffects(CampaignCursorEffects effects) { impl_->effects=std::move(effects); }
// 현재 표시기의 주소는 장치/메뉴 교체 동안 바뀌지 않는다.
RawCampaignCursorDisplay& RawCampaignCursor::Display() { return *impl_->display; }
// Native Take의 words[2]는 글꼴 번호가 아닌 팔레트 색이다. 실제 흰색/노랑 전역을 공급한다.
void RawCampaignCursor::SetPalette(std::span<const ScreenColor> palette) { impl_->display->SetPalette(palette);const auto colors=impl_->display->TextColors();impl_->lifecycleState.warningFont=colors[0];impl_->lifecycleState.costFont=colors[1]; }
// 같은 화면 원점을 마우스의 카메라 비트와 별도로 원본 Draw의 signed 픽셀 빼기에 공급한다.
void RawCampaignCursor::SetView(SoundView view) { impl_->placementState.screenOrigin={view.cameraX,view.cameraY};impl_->display->SetView(view); }
// 실제 취득의 Cancel/Query/생성/측정/위치 꼬리를 그대로 실행한다.
void RawCampaignCursor::Take(o::Sid form,std::uint32_t flags) { impl_->Prepare();impl_->lifecycle->Take(form,flags); }
// 잠긴 상태의 꼬리 정리도 원본 몸체가 결정한다.
void RawCampaignCursor::Cancel() { impl_->Prepare();impl_->lifecycle->Cancel(); }
// 같은 현재 카메라 비트를 읽는 실제 위치 몸체를 호출한다.
void RawCampaignCursor::Update(std::int32_t x,std::int32_t y) { impl_->Prepare();impl_->position->Update(x,y); }
// 원본의 장기 UI 포인터 대신 교체되는 호스트 토큰이므로 이전 깊이를 다음 수집에 넘기지 않는다.
void RawCampaignCursor::GumpsChanged() {
    if (impl_->data.drop.lockDepth || !impl_->data.drop.preview.value) return;
    impl_->Prepare();impl_->placement->Prepare();impl_->placement->Query();impl_->placement->Prepare();
    Current(impl_->effects.lifecycle.selectCursor,"하드웨어 커서 선택")(impl_->data.drop.legalGump ? 3U : 2U);
}
// 실제 로컬 예측 이후 상위 비용과 임시 객체 정리를 수행한다.
// 현재 전역을 준비한 뒤 원본 중첩 잠금/해제/회전을 같은 상태에 실행한다.
void RawCampaignCursor::Lock(std::uint32_t skipCursor) { impl_->Prepare();impl_->actions->Lock(skipCursor); }
void RawCampaignCursor::Unlock() { impl_->Prepare();impl_->actions->Unlock(); } // 깊이0 전이에서만 실제 목록을 다시 만든다.
void RawCampaignCursor::Rotate(std::int32_t step) { impl_->Prepare();impl_->actions->Rotate(step); } // 현재 Range/TYPE도 같은 몸체다.
// 원본4d9bbb..4d9c2e의 활성 커서 분기이며 release는 그 앞의 공통 필터에서 제외된다.
bool RawCampaignCursor::HandleInput(InputEvent event,std::uint32_t rotateMode) {
    if (impl_->data.drop.lockDepth || (event.code&(InputCode::kRelease|InputCode::kOutside))) return false;
    switch (event.code&0x03ffffffU) {
    case InputCode::kLeftButton:Drop();return true;
    case InputCode::kRightButton:Rotate(rotateMode ? -2 : 2);return true;
    case 0x250000:Rotate(-2);return true;
    case 0x270000:Rotate(2);return true;
    case 0x1b:Cancel();Current(impl_->effects.lifecycle.closePopups,"팝업 닫기")();return true;
    default:return false;
    }
}
// 실제 내려놓기는 기존 Local/비용/송신과 같은 상태를 사용한다.
void RawCampaignCursor::Drop() { impl_->Prepare();impl_->drop->Drop(); }
// 메뉴의 전체 DWORD 플래그와 원래 Form을 그대로 전달한다.
void RawCampaignCursor::Bind(PriestMenuHooks& hooks) { hooks.takePreview=[this](o::Sid form,std::uint32_t flags) { Take(form,flags); }; }
// 현재 상태를 별도 복사하지 않고 직접 빌린다.
CursorLifecycleState& RawCampaignCursor::Lifecycle() { return impl_->lifecycleState; }
CursorPlacementState& RawCampaignCursor::Placement() { return impl_->placementState; }
// 실제 GUI용 반환 Form도 같은 Factory/정렬 부착/부모 통지를 사용한다.
const o::RawContainedCreate& RawCampaignCursor::Contents() const { return impl_->data.contained; }
CursorReturnPieceState& RawCampaignCursor::ReturnSettings() { return impl_->returnState; }
std::uint32_t& RawCampaignCursor::SnapToGrid() { return impl_->snapToGrid; } // 다음 실제 위치 명령에서 현재 DWORD를 읽는다.
}
