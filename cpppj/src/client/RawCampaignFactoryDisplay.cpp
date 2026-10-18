// 원본433660→498220→Factory +78/+a4를 실제8비트 장면의 동일 정렬 위치에서 실행한다.
#include "client/RawCampaignFactoryDisplay.h"
#include "client/ObjectCollection.h"
#include "client/SpriteDraw.h"
#include "client/SpriteColorMaps.h"
#include "client/PaletteColors.h"
#include "client/OwnerColorMaps.h"
#include "o/RawFactoryDraw.h"
#include "o/RawBridgeConnect.h"
#include <stdexcept>
namespace netstorm::client {
// 같은 월드만 참조하고 게임 진행 상태는 복제하지 않는 표시 전용 저장소다.
struct RawCampaignFactoryDisplay::Impl {
    RawWorldCore core; // 실제 현재 풀/해시/TYPE/시계다.
    const GameAssets& assets; // 장면보다 오래 사는 실제 SHP다.
    o::RawFactoryDraw factory; // 두 실제 가상 표의 기본·추가 프레임 몸체다.
    ObjectCollection collection; // 같은 공간 해시의 원본 단계/깊이/그림자 수집기다.
    std::vector<SpriteTypeView> views; // 현재 SHP 선택 헤더와 기본 Gump 프레임이다.
    SpriteDrawState state; // 원본 표시 전역과 현재 카메라/로컬 소유자다.
    SpriteColorMaps colors; // 실제 파일 팔레트의 세 소스 치환 표다.
    OwnerColorMaps outpost; // 원본43be30의 아웃포스트 일반9색 표다.
    bool ready{}; // 파일 팔레트가 설치되기 전에 출력하지 않는다.

    // 원본 PE 초기값만 설치하고 각 수집에서 현재 게임 값으로 갱신한다.
    Impl(RawWorldCore world,const GameAssets& data):core(world),assets(data),factory(core.pool,core.frames),
        collection(core.hash,core.pool,core.types,core.shapes),views(MakeSpriteAssetViews(data)),outpost(BuildStandardOwnerColorMaps(126,82)) {
        state.filteredFlags=2421574144U;state.wideOwnerType=164;state.unfilteredType=84;state.shadowAliasType=158;
        state.shadowAliasPinned=true; // 실제5318ec의 초기DWORD6이다.0일 때만 alias를 바꾼다.
        state.markerEnabled=true;state.markerExcludeType=157;state.markerType=96;state.statusType=118;state.abstractStatusType=117;
        state.timerProcessType=31;state.statusProcessType=57;state.costType=122;state.timerTypes={138,144,73};
        state.selectionColors={84,84,249,184,140,150,112,156,222}; // 원본5319c4의 고정9DWORD다.
        state.ownerMaps.resize(core.types.size());state.ownerMaps.at(126)=1;
    }
    // 건설/추상/그림자는 명시 표, 아웃포스트는 같은 현재 소유자 색 번호로 변환한다.
    ColorMap Color(SpriteShapeMode mode,std::uint32_t type,std::uint32_t owner) const {
        if (mode==SpriteShapeMode::Abstract) return colors.bright;if (mode==SpriteShapeMode::Dead) return colors.dead;if (mode==SpriteShapeMode::Shadow) return colors.shadow;
        if (mode!=SpriteShapeMode::Owner || type!=126 || owner>=core.bridgeState.ownerColors.size()) throw std::logic_error("Factory 원본 소유자 색표 미연결");
        const auto index=core.bridgeState.ownerColors[owner];if (index<0 || index>8) throw std::out_of_range("Factory 현재 소유자 색 번호");return outpost[static_cast<std::size_t>(index)];
    }
    // 원본 현재 슬롯과 실제 dirty clip을 읽고 일반 Draw의 모든 픽셀을 같은 표면에 쓴다.
    void Draw(o::RenderCollectionEntry entry,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        const auto sid=collection.Object(entry.token);if (!factory.Handles(sid)) return;if (!ready) throw std::logic_error("Factory 파일 팔레트 미설치");
        const auto raw=core.pool.Slot(sid);
        if ((raw[40]&4U) && (raw[40]&0x30U)) throw std::logic_error("Factory 선택 상태 프로세스 목록 표시 미연결");
        state.now=core.processState.now;state.ownerPalettes.assign(core.bridgeState.ownerColors.begin(),core.bridgeState.ownerColors.end());
        SpritePixelTarget target(pixels,pitch,height,clip,MakeSpriteAssetShapes(assets,state),[this](auto mode,auto type,auto owner) { return Color(mode,type,owner); });
        SpriteDrawHooks rules;rules.virtualFrame=[this](o::Sid object) { return factory.MainFrame(object); };rules.surface=MakeSpriteSurfaceLookup(core.hash);
        rules.resource=[this](o::Sid object)->std::optional<SpriteResourceStatus> {
            if (core.processes.FindEvent(object,0x493)) throw std::logic_error("Factory 자원 내용물 막대 표시 미연결");return std::nullopt;
        };
        auto hooks=target.Hooks(std::move(rules));
        // +a4의0044c180은 같은 기준점/단위 배율의 plain SHP다. 일반 owner 문맥으로 바꾸지 않는다.
        hooks.virtualDraw=[this,shape=hooks.shape](o::Sid object,ScreenPoint point) {
            factory.Additional(object,point.x,point.y,[&shape](auto x,auto y,auto type,auto frame) { if (frame<0) throw std::out_of_range("Factory 추가 그림 음수 프레임");shape({type,static_cast<std::uint32_t>(frame),{x,y},SpriteShapeMode::Plain}); });
        };
        SpriteDraw draw(core.pool,core.types,views,state,std::move(hooks),0);draw.Draw(entry);
    }
};
// 같은 Session의 수명이 끝나기 전에 표시기를 해제한다.
RawCampaignFactoryDisplay::RawCampaignFactoryDisplay(RawWorldCore core,const GameAssets& assets):impl_(std::make_unique<Impl>(core,assets)) {}
RawCampaignFactoryDisplay::~RawCampaignFactoryDisplay()=default; // procedure가 더 이상 호출되지 않을 때 반납한다.
bool RawCampaignFactoryDisplay::Handles(o::Sid sid) const { return impl_->factory.Handles(sid); } // 별도 TYPE 이름 휴리스틱을 쓰지 않는다.
// 장치 BGRX와 별개로 원본 R/G/B/예약 DWORD를 만들어 실제 파일 색 표를 계산한다.
void RawCampaignFactoryDisplay::SetPalette(std::span<const ScreenColor> palette) {
    if (palette.size()!=256) throw std::invalid_argument("Factory 파일 팔레트256색 계약");std::array<std::uint32_t,256> logical{};
    // 실제 현재 파일 팔레트의 모든 번호를 보존한다.
    for (std::size_t i=0;i<logical.size();++i) logical[i]=std::uint32_t(palette[i].red)|(std::uint32_t(palette[i].green)<<8)|(std::uint32_t(palette[i].blue)<<16)|(std::uint32_t(palette[i].reserved)<<24);
    const auto named=BuildPaletteColorTable(logical,impl_->assets.Edition());impl_->colors=BuildSpriteColorMaps(logical);impl_->state.barBackground=named.basic[0];
    impl_->state.hitPointColors={named.named.at(10).index,named.basic[5],named.basic[1]};impl_->ready=true;
}
// 원본 수집기의 전체 순서/한 번의 그림자 보정/해시2단계 반환을 유지한다.
std::vector<RenderSprite> RawCampaignFactoryDisplay::Sprites(ScreenRect clip,int cameraX,int cameraY,std::uint32_t localPlayer) {
    auto& self=*impl_;self.state.cameraX=cameraX;self.state.cameraY=cameraY;self.state.localPlayer=localPlayer;
    const o::RenderWorldRect view{static_cast<float>((static_cast<double>(clip.left)+cameraX)/16),static_cast<float>((static_cast<double>(clip.top)+cameraY)/11),
        static_cast<float>((static_cast<double>(clip.right)+cameraX)/16),static_cast<float>((static_cast<double>(clip.bottom)+cameraY)/11)};
    std::array<o::RenderCollectionEntry,kDrawSubmissionCapacity> entries{};const auto count=self.collection.Collect(view,entries,kDrawSubmissionCapacity);std::vector<RenderSprite> result;
    // 같은 실제 공간 항목 중 두 가상 표만 제출하고 나머지는 기존 월드 출력에 남긴다.
    for (std::int32_t i=0;i<count;++i) {
        const auto entry=entries[static_cast<std::size_t>(i)];if (!self.factory.Handles(self.collection.Object(entry.token))) continue;
        RenderSprite sprite;sprite.clip=clip;sprite.order={entry.x,entry.y,entry.depth};
        sprite.procedure=RenderProcedure{clip,[this,entry](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect cut) { impl_->Draw(entry,pixels,pitch,height,cut); }};result.push_back(std::move(sprite));
    }
    return result;
}
}
