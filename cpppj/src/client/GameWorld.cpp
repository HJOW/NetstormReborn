#include "client/GameWorld.h"
#include "client/ClientColorSortInput.h"
#include "client/RawCampaignFactoryDisplay.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCursorPosition.h"
#include <bit>
#include "client/SquidRenderer.h"
#include "client/OwnerColorMaps.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawPathAnimation.h"
#include "o/OriginalText.h"
#include "o/RawRainVortexDraw.h"
#include "o/RawTempleDraw.h"
#include "o/RawFactoryDraw.h"
#include "o/RawThunderCannonCapture.h"
#include "o/CanonDecoder.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 월드 칸의 화면 픽셀 크기와 현재 메뉴 막대가 차지하는 높이.
constexpr int kCellPixelsX=16,kCellPixelsY=11,kMenuHeight=28;
// 현재 연결한 지형/받침/물체의 상대 깊이. 전체 원본 zorder 표는 후속 복원 대상이다.
constexpr std::int16_t kLandDepth=30,kCliffDepth=40,kObjectDepth=0;
constexpr std::uint32_t kHideBuildingsMask=2421574144U,kHideWideOwnerType=164,kHideUnfilteredType=84; // PATCH005424b4/5412e8/5411a8이며기존SpriteDraw와같은현재TYPE분류다.
// 00531a08의 소유자 색 후보 표.
constexpr std::array<std::array<std::uint8_t,8>,8> kOwnerColors{{
    {81,80,76,145,146,147,4,10},{94,100,170,169,204,249,217,227},{195,93,185,35,31,149,70,15},{207,131,133,128,136,2,142,143},
    {189,190,253,150,148,70,14,10},{113,218,219,3,52,59,67,213},{9,85,155,156,6,72,144,143},{220,221,222,223,224,63,227,213}}};
// 0043bc40의 지면 명도 보정값.
constexpr std::array<double,8> kIsleOffsets{-0.1,-0.17,0.15,0,0,0.05,-0.2,0};
// .type 플래그는 대소문자를 구분하지 않는다.
bool Has(const TypeAsset& type,std::string_view flag) {
    return std::any_of(type.definition.flags.begin(),type.definition.flags.end(),[flag](const std::string& value) { return o::AsciiLower(value)==flag; });
}
// 원소별 isle 클러스터 순서: 해·천둥·바람·비.
int Theme(std::string_view theme) { return theme=="thunder" ? 1 : theme=="wind" ? 2 : theme=="rain" ? 3 : 0; }
// TSV의 한 필드 안에 줄바꿈/탭을 넣지 않는다.
std::string Field(std::string value) { std::replace(value.begin(),value.end(),'\t',' '); std::replace(value.begin(),value.end(),'\n',' '); std::replace(value.begin(),value.end(),'\r',' '); return value; }
// 원본10.78 골렘 생성자가 설치하는 실제133/VT50e568 쌍에만 발 기준점을 적용한다.
bool IsGolem(std::span<const std::uint8_t> raw) { return raw[10]==133 && raw[0]==0x68 && raw[1]==0xe5 && raw[2]==0x50 && raw[3]==0; }
}
#include "client/GameWorldInput.inc"
// 저장 위치와 파일 필드를 유지하여 표시와 이동이 공유하는 월드를 만든다.
GameWorld::GameWorld(const GameAssets& assets,const o::FortTemplate& fort,o::MissionPlayers players)
    : gameplayGlobals_(std::make_shared<ClientGameplayGlobals>()),assets_(assets),ground_(std::make_unique<o::GroundGrid>()),players_(std::move(players)) {
    // 표시 팔레트는 원본 번호를 그대로 캐시한다.
    for (std::size_t i=0;i<palette_.size();++i) palette_[i]=assets_.Palette().Color(static_cast<std::uint8_t>(i));
    const auto frames=assets.Find("puzzlePiece").definition.FrameTable();
    o::IslandBuilder builder(chunks_,islands_,frames,1); if (!fort.territory.empty()) builder.PlaceAll(fort.territory);
    terrain_=std::make_unique<o::TerrainBuilder>(chunks_,islands_);
    std::vector<o::ChunkCoordinate> all;
    // Chaff는 월드 y·x 순서로 모든 청크를 담는다.
    for (int i=0;i<o::kChunkMapSide*o::kChunkMapSide;++i) all.push_back({i%o::kChunkMapSide,i/o::kChunkMapSide});
    Place(fort.chaff,all,-1);
    // TerrNN의 레코드는 같은 영역 청크의 y·x 순서다.
    for (int region=0;region<o::kTerritoryCount;++region) Place(fort.territories[static_cast<std::size_t>(region)],o::TerritoryChunks(chunks_,islands_,1,region),region);
    // 원본 고정 시드 경로를 사용하는 재현 가능한 3×3 변형표. 실시간 원본의 전역 난수 소비와는 구분한다.
    std::uint32_t state=0x38d535; const auto next=[&state]() { state=state*0x343fdu+0x269ec3u; return static_cast<int>((state>>16)&0x7fff); };
    // 00455970의 시드 직후 난수 소비를 적용한다.
    for (int i=0;i<103;++i) next();
    // 004c04b0의 연속 하위 2비트 중복 보정을 적용한다.
    for (std::size_t i=0;i<coreVariants_.size();++i) { int value=next(); if (i>0 && (value&3)==(coreVariants_[i-1]&3)) ++value; coreVariants_[i]=value; }
    BuildTerrain();
    BuildSurfaces();
}
// 실제 캠페인에서는 이전 관찰기반 객체/지형 어댑터를 생성하지 않고 같은 raw 세션을 설치한다.
GameWorld::GameWorld(const GameAssets& assets,const o::BaseFileSystem& files,o::FortTemplate fort,o::MissionPlayers players,
    std::string mission,RawCampaignSessionServices services,ScreenPoint viewport,bool deferConnection):gameplayGlobals_(services.gameplayGlobals ? services.gameplayGlobals : std::make_shared<ClientGameplayGlobals>(services.subscriberGlobals,services.surfaceStorage ? services.surfaceStorage->Authority() : nullptr,services.surfaceStorage ? services.surfaceStorage->GameAux() : nullptr)),assets_(assets),players_(std::move(players)) {
    if (viewport.x<=0 || viewport.y<=0) throw std::invalid_argument("실제 캠페인 설치 전 화면 크기 누락");
    width_=viewport.x;height_=viewport.y;
    // 현재 보호 팔레트 번호만 UI 색 조회용으로 보관한다.
    for (std::size_t i=0;i<palette_.size();++i) palette_[i]=assets_.Palette().Color(static_cast<std::uint8_t>(i));
    // 현재 카메라는 Client owner를 직접 빌리며 로더의 같은 호출 위치에서 목표/두setter를 적용한다.
    services.install.resetCamera=[this] { ResetCampaignCamera(); };
    // 선택도 같은 GameWorld가 소유한다. 설치 중에는0이며 삭제의 실제 ClearSelection에서 같은 표시를 갱신한다.
    services.deletion.selected=[this] { return o::Sid{static_cast<std::uint16_t>(selected_)}; };
    services.deletion.emit=[this,other=std::move(services.deletion.emit)](const o::SquidDestroyEvent& event) {
        if (event.effect==o::SquidDestroyEffect::ClearSelection) { Select(0);return; }
        if (!other) throw std::logic_error("GameWorld의 다른 파생 삭제 효과 미연결");
        other(event);
    };
    const bool connect=services.localLogin.has_value(); //실제Client는로그인서버가Load를호출한다.
    const bool existingEditor=services.installMode==CampaignInstallMode::ExistingEditor; //Editor는 전체 외곽/State가 끝나기 전 Campaign.Load나 로그인으로 우회하지 않는다.
    if (existingEditor && !deferConnection) throw std::logic_error("기존 Editor는 실제 외곽의 명시 지연 설치가 필요함");
    services.gameplayGlobals=gameplayGlobals_; // 독립 호출자도 GameWorld와 Session이 서로 다른 선택 전역을 만들지 않는다.
    campaign_=std::make_unique<RawCampaignSession>(assets,files,std::move(fort),std::move(mission),std::move(services));
    if (assets_.Edition()==o::OriginalEdition::Patch1078) {
        factoryDisplay_=std::make_unique<RawCampaignFactoryDisplay>(campaign_->Core(),assets_,[this](o::Sid sid,const auto& frame) { campaign_->DrawTemple(sid,frame); },&gameplayGlobals_->PatchDiagnostic());std::array<ScreenColor,256> palette{};
        // Client가 파일 팔레트를 설치하기 전에도 실제 자산의 동일256색을 사용한다.
        for (std::size_t i=0;i<palette.size();++i) { const auto color=assets_.Palette().Color(static_cast<std::uint8_t>(i));palette[i]={color.blue,color.green,color.red,0}; }
        factoryDisplay_->SetPalette(palette);
    }
    if (deferConnection) { if (!existingEditor && !connect) throw std::logic_error("실제Client 지연로그인 장치 누락");return; } //기존 Campaign의 장치 조건은 유지하며 Editor는 외곽이 설치한다.
    if (connect) campaign_->LoadLocalConnection();else campaign_->Load();RefreshCampaignObjects();
}
//Client.CurrentSubscriber가현재Session을반환하는등록이끝난뒤에만실제로그인을시작한다.
void GameWorld::BeginLocalConnection() { if (!campaign_) throw std::logic_error("실제Session 없는 로그인 요청");campaign_->LoadLocalConnection();RefreshCampaignObjects(); }
// 세션의 실제 Kernel/음향/Form 콜백은 나머지 월드 멤버가 살아 있는 동안 정리한다.
GameWorld::~GameWorld() { factoryDisplay_.reset();campaign_.reset(); }
// 초기 카메라의 원본 좌표를 clamp나 신전 우선 선택 없이 현재 raw 표시와 음향 원점에 전달한다.
void GameWorld::ResetCampaignCamera() {
    cameraHome_.width=width_;cameraHome_.height=height_;
    campaign_->ResetCamera(cameraHome_,[this](std::int32_t x,std::int32_t y) { ApplyCameraPixels(x,y);SurfaceView(); });
}
// 실제 첫두 world float비트는 픽셀에서 재계산하지 않고 같은 현재 snapshot에서 읽는다.
MinimapGeometryView GameWorld::MinimapView() const { return {CameraGlobals().Snapshot().worldBits,MapView(),campaign_ ? CameraGlobals().Snapshot().cellPixels[0] : 16,campaign_ ? CameraGlobals().Snapshot().cellPixels[1] : 11}; }
// 현재 mapRect의 같은 장기 소유자를 연결한 뒤 표시/음향에 즉시 공급한다.
void GameWorld::BindMapView(std::function<ScreenRect()> current) { if (!current) throw std::invalid_argument("월드의 실제 지도 영역 조회 필요");mapView_=std::move(current);SurfaceView();changed_=true; }
ScreenRect GameWorld::MapView() const { return mapView_ ? mapView_() : ScreenRect{0,std::min(kMenuHeight,height_),width_,height_}; } // 원문 dock 미설치의 독립 검사 계약을 보존한다.
// 홈 선택과 미니맵 클릭은 동일한497f20을 사용하며 임의 월드 가장자리clamp를 추가하지 않는다.
void GameWorld::CenterCamera(float x,float y) {
    if (!campaign_) throw std::logic_error("실제 미니맵 카메라의 Session 없음");cameraHome_.width=width_;cameraHome_.height=height_;const auto core=campaign_->Core(); // 현재 공유 풀/TYPE다.
    CameraHome(core.pool,core.types,cameraHome_,[this](auto px,auto py) { ApplyCameraPixels(px,py);SurfaceView(); }).CenterPoint(x,y);
}
// 원본 미니맵은 일반화면의4971e0/41d770 변환을 다시 거치지 않는다.
void GameWorld::MoveFromMinimap(float x,float y) { if (!input_) throw std::logic_error("실제 미니맵 선택 장치 없음");input_->carrier->Commit(selected_,{x,y});input_->highlight->Highlight(0);changed_=true; }
// 같은 실제 슬롯의 표시/선택용 뷰다. 이 목록에는 이동/HP 계산이나 별도 경로를 저장하지 않는다.
void GameWorld::RefreshCampaignObjects() {
    if (!campaign_) return;objects_.clear();const auto core=campaign_->Core();
    // 현재 live 자산의 실제 SID/타입/좌표/프레임/HP를 조회한다.
    for (const auto& snapshot:campaign_->World().Objects()) {
        const auto& rawObject=snapshot.object;
        if (rawObject.type<o::kFirstAssetTypeNumber || rawObject.type-o::kFirstAssetTypeNumber>=assets_.Types().size()) continue;
        const auto& type=assets_.Types()[rawObject.type-o::kFirstAssetTypeNumber];const auto slot=core.pool.Slot(snapshot.sid);
        o::Squid view;view.id=snapshot.sid.value;view.type=static_cast<int>(rawObject.type);view.owner=static_cast<int>(rawObject.owner);
        view.x=rawObject.x;view.y=rawObject.y;view.cell={static_cast<int>(view.x),static_cast<int>(view.y)};view.frame=static_cast<std::size_t>(std::max(rawObject.frame,0));
        view.initialFrame=view.frame;view.mobile=(core.types[rawObject.type].flags2&o::TypeFlag2::kWalker)!=0;
        const auto& pictures=assets_.Shapes().Blocks()[type.block].frames;
        view.visible=o::AsciiLower(type.assetName)!="noisland" && view.frame<pictures.size() && !pictures[view.frame].IsSpecial();
        view.selectable=view.visible && !Has(type,"not_selectable") && !Has(type,"bridge");
        if (campaign_->HitPoints().HasHitPoints(snapshot.sid)) {
            view.maxHitPoints=campaign_->HitPoints().MaxHitPoints(snapshot.sid);
            view.hitPoints=static_cast<std::int32_t>(static_cast<std::uint32_t>(slot[26])|(std::uint32_t(slot[27])<<8)|(std::uint32_t(slot[28])<<16)|(std::uint32_t(slot[29])<<24));
        }
        objects_.push_back(std::move(view));
    }
    if (selected_ && !Object(selected_)) selected_=0;
}
// 섬 타입의 저장 레코드는 원본처럼 건너뛰고 다른 자산 객체에 런타임 번호를 배정한다.
void GameWorld::Place(const o::FortChunkSection& section,std::span<const o::ChunkCoordinate> chunks,int territory) {
    if (!section.present) return;
    if (section.chunks.size()>chunks.size()) throw std::runtime_error("Fort chunks exceed placed territory");
    // 저장 청크 순서의 좌표를 사용한다.
    for (std::size_t i=0;i<section.chunks.size();++i)
        // 타입별 저장 필드를 손실 없이 보존한다.
        for (const auto& stored:section.chunks[i].objects) {
            if (stored.type==0 || stored.type==assets_.TypeTable().IslandType()) continue;
            if (stored.type<o::kFirstAssetTypeNumber || static_cast<std::size_t>(stored.type-o::kFirstAssetTypeNumber)>=assets_.Types().size())
                throw std::runtime_error("Unrestored fort object constructor: "+std::to_string(stored.type));
            const auto& type=assets_.Types()[static_cast<std::size_t>(stored.type-o::kFirstAssetTypeNumber)];
            const auto& record=assets_.TypeTable().Types()[static_cast<std::size_t>(stored.type)]; o::Squid object;
            object.id=static_cast<o::SquidId>(objects_.size()+1); object.type=stored.type; object.territory=territory;
            object.cell={chunks[i].x*16+stored.CellX(),chunks[i].y*16+stored.CellY()}; object.x=object.cell.x; object.y=object.cell.y;
            object.owner=stored.shorthandOwner.value_or(stored.NormalizedOwner().value_or(1));
            if ((record.flags2 & (o::TypeFlag2::kOwnerClearedMask|o::TypeFlag2::kIsland))!=0) object.owner=0;
            object.frame=static_cast<std::size_t>(std::max(type.definition.specialFrames.defaultFrame,0));
            if (stored.frame) object.frame=*stored.frame; if (stored.bridgeFrame) object.frame=*stored.bridgeFrame;
            if (object.frame>=type.definition.clusters.size()) throw std::runtime_error("Invalid stored world frame");
            object.initialFrame=object.frame; object.maxHitPoints=type.definition.Number("maxHitPoints").value_or(0); object.hitPoints=object.maxHitPoints;
            object.quantity=stored.quantityWord.value_or(stored.quantityByte.value_or(0)); object.factoryState=stored.factoryState;
            if (stored.contents) object.contents=*stored.contents;
            object.mobile=(record.flags2 & o::TypeFlag2::kWalker)!=0; object.speed=type.definition.Number("speed").value_or(0);
            object.visible=o::AsciiLower(type.assetName)!="noisland"; object.selectable=object.visible && !Has(type,"not_selectable") && !Has(type,"bridge");
            if (object.owner>0 && object.owner<=o::kPlayerCount) players_.players[static_cast<std::size_t>(object.owner)].active=true;
            if (territory>=0 && Has(type,"vortex")) { regionOwners_[static_cast<std::size_t>(territory)]=object.owner; regionThemes_[static_cast<std::size_t>(territory)]=Theme(type.definition.String("theme").value_or("sun")); }
            objects_.push_back(std::move(object));
        }
}
// 기존 월드 어댑터의 색 검색·소유자별 지면 명도도 현재 화면 팔레트를 사용한다.
void GameWorld::SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical) {
    if (palette.size()!=palette_.size()) throw std::invalid_argument("World palette needs 256 colors");
    // 색 번호는 유지하고 RGB만 교체한다.
    for (std::size_t i=0;i<palette_.size();++i) palette_[i]={palette[i].red,palette[i].green,palette[i].blue};
    if (!colorSortLogical.empty() && colorSortLogical.size()!=256) throw std::invalid_argument("World 고정ColorSort256색 계약");
    colorSortLogical_=colorSortLogical;
    if (campaign_) campaign_->Cursor().SetPalette(palette,colorSortLogical_); // 실제 커서도 같은 파일 팔레트의 원본 색표를 사용한다.
    if (factoryDisplay_) factoryDisplay_->SetPalette(palette,colorSortLogical_); // 원본 SpriteDraw도 같은 파일 팔레트에서만 색표를 만든다.
    remaps_.clear();changed_=true;
}
// 메뉴명령의같은DWORD를복사하지않아열린메뉴뒤의표시/선택도현재상태를읽는다.
void GameWorld::BindHideBuildings(std::function<std::uint32_t()> current) {
    if (!current) throw std::invalid_argument("건물숨김의현재0059a8ac조회기누락");
    hideBuildings_=std::move(current);changed_=true;
}
// 연결 방향 후보와 원소별 묶음에서 지면/절벽의 실제 클러스터를 고른다.
std::size_t GameWorld::TerrainFrame(const TypeAsset& type,char a,char b,int theme,int x,int y,bool core) const {
    const auto& clusters=type.definition.clusters;
    if (core && a=='A' && b=='A') {
        const auto it=std::find_if(clusters.begin(),clusters.end(),[](const o::TypeCluster& value) { return value.name=="JJ00"; });
        if (it!=clusters.end()) {
            const int bx=x/3,by=y/3,index=(coreVariants_[static_cast<std::size_t>(by%99)]+coreVariants_[static_cast<std::size_t>(bx%99)])%99;
            const int variant=(coreVariants_[static_cast<std::size_t>(index)]+by)%4;
            const auto selected=static_cast<std::size_t>(it-clusters.begin()+1+theme*36+variant*9+(y%3)*3+x%3);
            if (selected<clusters.size()) return selected;
        }
    }
    const auto normalized=o::TerrainBuilder::Normalize(a,b);
    const std::array<std::string,4> prefixes{std::string{a,b},std::string{normalized.first,normalized.second},std::string{a,'A'},std::string(1,a)};
    // 원본 프레임 검색의 방향 폴백 순서다.
    for (const auto& prefix:prefixes) {
        std::vector<std::size_t> candidates;
        // 모든 일치 프레임을 원본 클러스터 순서로 모은다.
        for (std::size_t i=0;i<clusters.size();++i) if (clusters[i].name.starts_with(prefix) && (core || (clusters[i].code.flags & 0x10)==0)) candidates.push_back(i);
        if (candidates.empty()) continue;
        if (core && candidates.size()>=4) {
            const auto perTheme=candidates.size()/4,first=static_cast<std::size_t>(theme)*perTheme;
            const auto skip=perTheme>1 ? 1u : 0u; return candidates[first+skip+static_cast<std::size_t>(x*31+y*17)%(perTheme-skip)];
        }
        return candidates[static_cast<std::size_t>(x*31+y*17)%candidates.size()];
    }
    throw std::runtime_error("Missing terrain cluster: "+type.assetName+"/"+std::string{a,b});
}
// 본섬과 저장 받침은 같은 격자에 등록하되 서로 다른 스프라이트로 그린다.
void GameWorld::BuildTerrain() {
    const auto& isle=assets_.Find("isle"),&fringe=assets_.Find("fringe");
    std::set<std::pair<int,int>> padCells,claimed;
    // noIsland 칸은 받침의 논리 지면이다.
    for (const auto& object:objects_) if (o::AsciiLower(assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)].assetName)=="noisland") padCells.emplace(object.cell.x,object.cell.y);
    std::vector<o::CellPoint> anchors;
    // 건물 기준점을 먼저 사용해 서로 붙은 3×3 받침의 경계를 보존한다.
    for (const auto& object:objects_) { const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; if (Has(type,"createsisland") || Has(type,"geyser")) anchors.push_back(object.cell); }
    // 건물이 없는 완전한 저장 받침도 오른쪽 아래 칸을 기준점으로 조사한다.
    for (const auto& [x,y]:padCells) anchors.push_back({x,y});
    const auto& island=assets_.Find("island"),&stalag=assets_.Find("islandStalag");
    // 완전한 9칸만 전용 받침으로 바꾼다.
    for (const auto anchor:anchors) {
        bool complete=true;
        // 받침의 세 행을 검사한다.
        for (int dy=-2;dy<=0;++dy)
            // 이미 사용된 칸은 다시 받침으로 합치지 않는다.
            for (int dx=-2;dx<=0;++dx) if (!padCells.contains({anchor.x+dx,anchor.y+dy}) || claimed.contains({anchor.x+dx,anchor.y+dy})) complete=false;
        if (!complete) continue; int owner=0;
        // 받침 위 건물의 소유자 색을 사용한다. 동적 Islanddropper 소유자 전파는 후속이다.
        for (const auto& object:objects_) if (object.cell==anchor && object.visible && object.owner>0) { owner=object.owner; break; }
        const int color=owner>0 ? players_.players[static_cast<std::size_t>(owner)].color : 0; const auto frame=static_cast<std::size_t>(color>0 ? color-1 : 8);
        tiles_.push_back({&stalag,frame,anchor.x,anchor.y,0,kCliffDepth}); tiles_.push_back({&island,frame,anchor.x,anchor.y,0,kLandDepth});
        // 전용 스프라이트로 표시한 칸은 일반 지면과 중복하지 않는다.
        for (int dy=-2;dy<=0;++dy)
            // 같은 받침의 세 열을 등록한다.
            for (int dx=-2;dx<=0;++dx) claimed.emplace(anchor.x+dx,anchor.y+dy);
    }
    // 표시 프레임과 경로 격자를 월드 y·x 순서로 만든다.
    for (int y=0;y<o::kWorldCells;++y)
        // 65,536칸의 본섬 영역과 저장 받침을 공유한다.
        for (int x=0;x<o::kWorldCells;++x) {
            const auto region=terrain_->At(x,y); auto& cell=*ground_->At(x,y); cell.land=region!=o::kEmptyTerrain || padCells.contains({x,y});
            if (region==o::kEmptyTerrain || claimed.contains({x,y})) continue;
            int cardinal=0,diagonal=0;
            // 직선/대각선 비트는 원본의 서로 다른 시작 방향을 사용한다.
            for (int i=0;i<4;++i) { const auto [dx,dy]=o::kNeighborCells[static_cast<std::size_t>(i*2)]; if (terrain_->At(x+dx,y+dy)==region) cardinal|=1<<i; const auto [qx,qy]=o::kNeighborCells[static_cast<std::size_t>((7+i*2)&7)]; if (terrain_->At(x+qx,y+qy)==region) diagonal|=1<<i; }
            const char a=o::TerrainBuilder::Orientation(cardinal),b=o::TerrainBuilder::Orientation(diagonal);
            const auto frame=TerrainFrame(isle,a,b,regionThemes_[region],x,y,true); const int owner=regionOwners_[region];
            tiles_.push_back({&isle,frame,x,y,owner,kLandDepth});
            if ((isle.definition.clusters[frame].code.flags & 1)!=0) tiles_.push_back({&fringe,TerrainFrame(fringe,a,b,0,x,y,false),x,y+4,0,kCliffDepth});
        }
    // 지면 이외의 저장 객체는 다리 또는 발자국 점유를 등록한다.
    for (const auto& object:objects_) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        if (Has(type,"bridge")) continue; // 현재 다리 격자는 raw 등록 후 공급한다.
        if (!object.visible || Has(type,"balloon") || Has(type,"flyer") || Has(type,"not_real")) continue;
        const auto footprint=type.definition.Footprint();
        // 기준점은 발자국의 오른쪽 아래 칸이다(0049ae80).
        for (int y=object.cell.y-footprint[1]+1;y<=object.cell.y;++y)
            // 같은 행의 발자국 점유를 등록한다.
            for (int x=object.cell.x-footprint[0]+1;x<=object.cell.x;++x) if (auto* cell=ground_->At(x,y)) { cell->occupant=object.id; if (Has(type,"createsisland")) cell->land=true; }
    }
}
// 표면 객체는 raw 풀에서 소유한다. 건물/사제 등 비표면 객체와 땅의 생성 입력만 기존 어댑터가 공급한다.
void GameWorld::BuildSurfaces() {
    const auto types=assets_.TypeTable().Types();PriestPlacementAssets placement(assets_);std::vector<o::RiftTypeFrames> frames;frames.reserve(types.size());
    // 사제 배치와 표면 월드는 같은 로더의 실제 프레임/SHP 자료를 사용한다.
    for (auto& meta:placement.frames) frames.push_back(std::move(meta.frames));
    // 자산 이름을 판본별 타입 번호로 변환한다.
    const auto number=[this](std::string_view name) { return static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+assets_.Find(name).block); };
    o::BridgeConnectState links;links.bridgeType=number("bridge");links.noIslandType=number("noIsland");links.islandType=number("island");
    links.stalagType=number("islandStalag");links.connectorType=number("bridgeConnector");links.battle=true;
    bridgeType_=links.bridgeType;noIslandType_=links.noIslandType;
    // 기존 미션 플레이어의 색 번호를 복원된 받침 소유자 재정의에 공급한다.
    for (std::size_t i=0;i<links.ownerColors.size();++i) links.ownerColors[i]=players_.players[i].color;
    const auto terrainType=number("isle");std::vector<SurfaceSeed> seeds;
    // 본섬의 원본 지면 프레임을 같은 위치의 일반 raw 표면으로 등록한다. 절벽은 표시 레이어로 남긴다.
    for (const auto& tile:tiles_) if (tile.type->block+o::kFirstAssetTypeNumber==terrainType)
        seeds.push_back({terrainType,static_cast<std::uint32_t>(tile.owner),static_cast<std::int32_t>(tile.frame),static_cast<float>(tile.x),static_cast<float>(tile.y)});
    // 저장 noIsland와 다리만 raw 객체가 된다. 비표면 선택/이동의 번호는 기존대로 유지한다.
    for (const auto& object:objects_) if (static_cast<std::uint32_t>(object.type)==links.bridgeType || static_cast<std::uint32_t>(object.type)==links.noIslandType)
        seeds.push_back({static_cast<std::uint32_t>(object.type),static_cast<std::uint32_t>(object.owner),static_cast<std::int32_t>(object.frame),static_cast<float>(object.x),static_cast<float>(object.y)});
    surfaces_=std::make_unique<RawSurfaceWorld>(assets_.Edition(),types,frames,placement.shapes,links,terrainType);
    surfaces_->Load(seeds);
    // 생성 명령 전체 복원 전까지 저장 건물의 받침 소유자 공급은 기존 월드 초기화 경계로 유지한다.
    for (const auto& object:objects_) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        if (object.owner>0 && (Has(type,"createsisland") || Has(type,"geyser"))) surfaces_->SetSupportOwner(static_cast<float>(object.x),static_cast<float>(object.y),static_cast<std::uint32_t>(object.owner));
    }
    RefreshSurfaceGround();
}
// 삭제된 다리가 예전 저장 목록 때문에 보행 가능한 칸으로 남지 않게 한다.
void GameWorld::RefreshSurfaceGround() {
    // 이전 raw 칸의 다리/받침 지면만 지우고 본섬과 비표면 점유자는 보존한다.
    for (const auto point:surfaceCells_) {
        auto& cell=*ground_->At(point.x,point.y);cell.bridge=false;cell.owner=0;cell.land=terrain_->At(point.x,point.y)!=o::kEmptyTerrain;
    }
    surfaceCells_.clear();
    // 표면 칸 삭제와 다리 끝 칸 교체를 실제 현재 위치/소유자로 반영한다.
    for (const auto& snapshot:surfaces_->Objects()) {
        const auto& object=snapshot.object;if (object.type!=bridgeType_ && object.type!=noIslandType_) continue;
        const o::CellPoint point{static_cast<int>(object.x),static_cast<int>(object.y)};auto& cell=*ground_->At(point.x,point.y);
        if (object.type==bridgeType_) { cell.bridge=true;cell.owner=static_cast<int>(object.owner); } else cell.land=true;
        surfaceCells_.push_back(point);
    }
}
// 절대 시각을 먼저 전달하고 raw Kernel과 기존 비표면 이동을 같은 프레임에 진행한다.
void GameWorld::RunFrame() {
    const bool stopped=paused && paused();
    if (frameTime) {
        if (campaign_) { surfaceTime_=frameTime();SurfaceView();campaign_->PumpLocalConnection();campaign_->RunFrame(surfaceTime_,stopped);RefreshCampaignObjects();return; }
        surfaceTime_=frameTime();SurfaceView();surfaces_->RunFrame(surfaceTime_,stopped);RefreshSurfaceGround();
        if (!stopped) AdvanceObjects(surfaceTime_.delta);
    } else if (elapsed && !stopped) Step(elapsed());
}
// 실제 GUI는 GameWorld를 Kernel에 등록하지 않으며 전체 프로세스 실행 전 이 준비만 요청한다.
void GameWorld::PrepareKernelFrame() {
    if (!campaign_ || !frameTime) throw std::logic_error("실제 Client Kernel 프레임의 Session/시각 공급자 누락");
    surfaceTime_=frameTime();SurfaceView();campaign_->PumpLocalConnection();campaign_->PrepareFrame(surfaceTime_,paused && paused());
}
// 원문 단일 Kernel이 끝난 뒤에만 이동/생성/삭제된 실제 객체를 화면용 뷰에 반영한다.
void GameWorld::FinishKernelFrame() {
    if (!campaign_) throw std::logic_error("실제 Client Kernel 후처리의 Session 누락");RefreshCampaignObjects();
}
// 콘솔에서는 같은 실행 경로에 누적된 게임 시각을 공급한다.
void GameWorld::Step(double seconds) {
    if (!std::isfinite(seconds) || seconds<0) throw std::invalid_argument("월드 시간차 범위 오류");
    surfaceTime_.game+=seconds;surfaceTime_.delta=seconds;++surfaceTime_.number;SurfaceView();
    if (campaign_) { surfaceTime_.wall+=seconds;campaign_->PumpLocalConnection();campaign_->RunFrame(surfaceTime_,false);RefreshCampaignObjects();return; }
    surfaces_->RunFrame(surfaceTime_,false);RefreshSurfaceGround();AdvanceObjects(seconds);
}
// 비표면 유닛의 관찰 기반 이동은 raw 표면 렌더링과 구별한다.
void GameWorld::AdvanceObjects(double seconds) {
    // 객체 번호 순서로 갱신한다.
    for (auto& object:objects_) if (object.owner>0 && object.owner<=o::kPlayerCount && object.Advance(seconds,*ground_,players_.players[static_cast<std::size_t>(object.owner)].allies)) { object.frame=Frame(object); changed_=true; }
}
// 현재 카메라·월드 표시 영역을 raw 표시와 위치 효과음에 함께 전달한다. 메뉴 줄을 제외한 현재 GUI 영역을 그대로 쓴다.
// 원본 가변 생산/상태 패널의 영역 계산은 아직 복원하지 않았으며, 위치 계산 함수 자체는 SoundPlayer의 원본 규칙을 쓴다.
void GameWorld::SurfaceView() {
    const auto view=MapView();const o::SquidDisplayRect clip{view.left,view.top,view.right,view.bottom}; // 실제 도킹 결과를 표면/입자/위치 음향이 함께 읽는다.
    Surfaces().SetDisplay({CameraX(),CameraY(),65536,clip},surfaceDisplay);
    if (campaign_) campaign_->SetParticleView({CameraX(),CameraY(),clip.left,clip.top,clip.right,clip.bottom},surfaceDisplay,false);
    if (soundViewChanged) soundViewChanged({CameraX(),CameraY(),clip.left,clip.top,clip.right,clip.bottom});
}
// 카메라 크기만 바뀌어도 다시 표시하고 위치 효과음의 화면 기준을 갱신한다. 같은 크기로 불러도 새 수신자를 갱신한다.
void GameWorld::Resize(int width,int height) {
    const bool first=width_==0; if (width_==width && height_==height) { SurfaceView();return; }
    width_=width; height_=height; CameraGlobals().UpdateTargetLimits(width,height,256.0F,256.0F);if (first) Home(false); else Scroll(0,0); changed_=true;
}
// 월드 테두리 밖으로 카메라가 나가지 않게 제한하고, 같은 입력 단계의 위치 효과음에 새 원점을 전달한다.
void GameWorld::Scroll(int dx,int dy) {
    const int x=std::clamp(CameraX()+dx,0,std::max(0,o::kWorldCells*kCellPixelsX-width_));
    const int y=std::clamp(CameraY()+dy,0,std::max(0,o::kWorldCells*kCellPixelsY-height_));
    if (x!=CameraX() || y!=CameraY()) { ApplyCameraPixels(x,y); }
    SurfaceView();
}
// 실제 캠페인은 현재 Player의 두 Home SID 또는 실제 사제 참여 조회를 사용한다.
void GameWorld::Home(bool priest) {
    if (campaign_) {
        cameraHome_.width=width_;cameraHome_.height=height_;const auto apply=[this](std::int32_t x,std::int32_t y) { ApplyCameraPixels(x,y);SurfaceView(); }; // 같은 원점/표시/위치 음향을 한 변경 지점에 갱신한다.
        if (priest) campaign_->ViewYourPriest(campaign_->LocalOwner(),cameraHome_,apply);else campaign_->ViewHome(cameraHome_,apply);return;
    }
    const o::Squid* target=nullptr;
    // 내 객체만 홈 대상으로 고른다.
    for (const auto& object:objects_) if (object.owner==1) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        if ((priest && Has(type,"priest")) || (!priest && Has(type,"vortex"))) { target=&object; break; }
        if (!target && Has(type,"priest")) target=&object;
    }
    if (target) { const auto size=assets_.Types()[static_cast<std::size_t>(target->type-o::kFirstAssetTypeNumber)].definition.Footprint(); ApplyCameraPixels(static_cast<int>((target->x-(size[0]-1)*0.5)*16)-width_/2,static_cast<int>((target->y-(size[1]-1)*0.5)*11)-height_/2); Scroll(0,0); changed_=true; }
}
// 메뉴가 이미 선택한 DWORD SID도 같은 raw Session에서 현재 TYPE/좌표/내용물을 다시 읽는다.
void GameWorld::CenterOnObject(std::uint32_t sid) {
    if (campaign_) {
        cameraHome_.width=width_;cameraHome_.height=height_;
        campaign_->CenterCameraOn(sid,cameraHome_,[this](std::int32_t x,std::int32_t y) { ApplyCameraPixels(x,y);SurfaceView(); });return;
    }
    const auto* target=Object(sid);if (!target) return;const auto size=assets_.Types()[static_cast<std::size_t>(target->type-o::kFirstAssetTypeNumber)].definition.Footprint(); // 이전 값형 월드의 기존 가운데/테두리 동작을 유지한다.
    ApplyCameraPixels(static_cast<int>((target->x-(size[0]-1)*0.5)*16)-width_/2,static_cast<int>((target->y-(size[1]-1)*0.5)*11)-height_/2);Scroll(0,0);changed_=true;
}
// Players 명령의 명시 소유자는 같은 현재 Session의 참여 조회로 전달한다.
void GameWorld::ViewPriest(std::uint32_t owner) {
    if (!campaign_) { Home(true);return; }
    cameraHome_.width=width_;cameraHome_.height=height_;
    campaign_->ViewYourPriest(owner,cameraHome_,[this](std::int32_t x,std::int32_t y) { ApplyCameraPixels(x,y);SurfaceView(); });
}
// 월드 기준점은 원본 SHP 사각형의 원점과 동일하다.
ScreenPoint GameWorld::ToScreen(double x,double y) const { if (campaign_) { const auto point=ProjectCameraView(CameraGlobals().Snapshot(),static_cast<float>(x),static_cast<float>(y));return {point[0],point[1]}; }return {static_cast<int>(std::lround(x*16))-CameraX(),static_cast<int>(std::lround(y*11))-CameraY()}; }
// 이동 목표는 가장 가까운 칸이다. 월드 밖은 경로 탐색에서 거부한다.
o::CellPoint GameWorld::ToCell(ScreenPoint p) const { if (campaign_) { const auto point=RawCursorPosition::TruncatePoint(RawCursorPosition::ScreenToWorld(p.x,p.y,CameraGlobals().Snapshot().worldBits));return {static_cast<int>(point[0]),static_cast<int>(point[1])}; }return {static_cast<int>(std::lround((p.x+CameraX())/16.0)),static_cast<int>(std::lround((p.y+CameraY())/11.0))}; }
// 보행은 A~H 각 방향의 8개 프레임을 현재 시간으로 선택한다.
std::size_t GameWorld::Frame(const o::Squid& object) const {
    if (object.heading<0) return object.initialFrame;
    const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
    // 도착/보행 중단 시 현재 프레임의 글자 구간 첫 물리 번호 +1을 사용한다.
    if (!object.walking) return static_cast<std::size_t>(o::RawPathAnimation::RestFrame(type.definition.FrameTable(),static_cast<std::int32_t>(object.frame)));
    // 보행 중 12Hz 임시 표시는 원본 PathProcess의 프레임 진행 복원까지 유지한다.
    const int number=static_cast<int>(object.animation*12)%8;
    const int frame=type.definition.FrameTable().FindNumber(static_cast<std::uint8_t>('A'+object.heading),'P',static_cast<std::uint8_t>(number));
    return frame>=0 ? static_cast<std::size_t>(frame) : object.initialFrame;
}
// 현재 그림의 불투명 프레임 사각형으로 몸통 클릭을 판정한다.
ScreenRect GameWorld::Bounds(const o::Squid& object) const {
    // Factory의 현재 프레임은 작은 추가 그림일 수 있다. 실제 +78(454cd0)의 본체 프레임으로 몸통 클릭 영역을 구한다.
    if (campaign_ && assets_.Edition()==o::OriginalEdition::Patch1078) {
        const auto core=campaign_->Core();const o::Sid sid{static_cast<std::uint16_t>(object.id)}; // 렌더링과 같은 현재 풀/실제 SID다.
        const auto current=core.pool.Slot(sid);const bool geyser=current[10]==122 && current[0]==0x88 && current[1]==0xda && current[2]==0x50 && current[3]==0; // 간헐천도 내부 효과 대신 실제 고정 외형을 클릭한다.
        if (o::IsThunderCannonCapture(current) || geyser) {
            const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
            const auto frame=geyser ? type.definition.specialFrames.baseFrame : o::ThunderCannonCaptureMainFrame(current,core.frames[static_cast<std::size_t>(object.type)]); // 실제 가상 getter의 고정 외형/방향별 본체 번호다.
            const auto& rect=assets_.Shapes().Blocks()[type.block].frames[static_cast<std::size_t>(frame)].rect;
            const auto& record=core.types[static_cast<std::size_t>(object.type)];const auto point=ToScreen(object.x,object.y);
            return {point.x-record.hotspotX+rect.left,point.y-record.hotspotY+rect.top,point.x-record.hotspotX+rect.right+1,point.y-record.hotspotY+rect.bottom+1};
        }
        const o::RawFactoryDraw factory(core.pool,core.frames); // 원본 두 가상 표의 단계별 본체 프레임 선택을 재사용한다.
        if (factory.Handles(sid)) {
            const auto rect=core.display.Bounds(static_cast<std::size_t>(object.type),factory.MainFrame(sid),static_cast<float>(object.x),static_cast<float>(object.y)); // 같은 SHP 헤더/카메라/배율의 전체 본체 영역이다.
            return {rect.left,rect.top,rect.right,rect.bottom};
        }
    }
    // 템플의 현재 애니메이션 압축 영역과 별개로 원본 Squid 표시 헤더의 전체 영역을 사용한다.
    if (campaign_ && (o::IsRainVortex(assets_.Edition(),campaign_->Core().pool.Slot(o::Sid{static_cast<std::uint16_t>(object.id)})) || (assets_.Edition()==o::OriginalEdition::Patch1078 && o::IsPatchTemple(campaign_->Core().pool.Slot(o::Sid{static_cast<std::uint16_t>(object.id)}))))) {
        const auto rect=campaign_->Core().display.Bounds(static_cast<std::size_t>(object.type),static_cast<int>(object.frame),static_cast<float>(object.x),static_cast<float>(object.y));
        return {rect.left,rect.top,rect.right,rect.bottom};
    }
    const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; const auto& frame=assets_.Shapes().Blocks()[type.block].frames[object.frame];
    const auto& record=assets_.TypeTable().Types()[static_cast<std::size_t>(object.type)];
    const bool native=factoryDisplay_ && (factoryDisplay_->Handles(o::Sid{static_cast<std::uint16_t>(object.id)}) || IsGolem(campaign_->Core().pool.Slot(o::Sid{static_cast<std::uint16_t>(object.id)}))); // 실제 원본 Draw와 골렘의 같은 TYPE 기준점을 클릭 영역에도 차감한다.
    const ScreenPoint offset=native ? ScreenPoint{record.hotspotX,record.hotspotY} : ScreenPoint{}; // 일반 출력에 남은 타입의 기존 그림/클릭 정합성을 유지한다.
    const auto p=ToScreen(object.x,object.y); return {p.x-offset.x+frame.rect.left,p.y-offset.y+frame.rect.top,p.x-offset.x+frame.rect.right+1,p.y-offset.y+frame.rect.bottom+1};
}
// 가장 앞에 가까운 실제 그림을 선택한다. 모바일 그림에는 녹화 기반 4픽셀 여유가 있다.
o::SquidId GameWorld::Pick(ScreenPoint point) const {
    const auto view=MapView();if (point.y<view.top || point.x<view.left || point.y>=view.bottom || point.x>=view.right) return 0;
    o::SquidId picked=0; double best=std::numeric_limits<double>::max();
    // 겹친 그림은 클릭과 중심점의 거리가 가장 가까운 객체를 고른다.
    for (const auto& object:objects_) if (object.selectable) {
        if (campaign_ && hideBuildings_()) { // 원본004d5840의현재숨김관문만기존근사선택앞에연결한다.
            const auto core=campaign_->Core();const auto type=core.pool.Slot(o::Sid{static_cast<std::uint16_t>(object.id)})[10]; // 표시사본대신현재raw TYPE를읽는다.
            if (type>=core.types.size()) throw std::out_of_range("건물숨김선택의현재TYPE범위오류");
            if ((core.types[type].flags2&kHideBuildingsMask) || type==kHideWideOwnerType || type==kHideUnfilteredType) continue;
        }
        const auto r=Bounds(object); const int margin=object.mobile ? 4 : 0;
        if (point.x<r.left-margin || point.x>=r.right+margin || point.y<r.top-margin || point.y>=r.bottom+margin) continue;
        const double dx=point.x-(r.left+r.right)*0.5,dy=point.y-(r.top+r.bottom)*0.5,distance=dx*dx+dy*dy;
        if (distance<best) { picked=object.id; best=distance; }
    }
    return picked;
}
// 없는 객체 또는 비선택 타입을 선택하지 않는다.
void GameWorld::Select(o::SquidId id) {
    if (campaign_) { if (!input_) throw std::logic_error("실제 선택 장치 미설치");static_cast<void>(input_->selection->Select(id,0,1));changed_=true;return; }
    const auto* object=Object(id);selected_=object && object->selectable ? id : 0;changed_=true;
}
// 처리기 수명 동안 같은 선택/마우스 전역을 공유하며 선택 번호를 복사하지 않는다.
UserHighlightState GameWorld::HighlightSettings() { return {highlightMode_,selected_,mouseType_,mousePoint_.x,mousePoint_.y}; }
// 입력 모듈이 교체되어도 같은 실제 선택 저장소를 읽고 쓴다.
ClientUserSelectionGlobals& GameWorld::SelectionGlobals() { return gameplayGlobals_->selection; }
// 읽기 전용 caller도 같은 owner를 빌린다.
const ClientUserSelectionGlobals& GameWorld::SelectionGlobals() const { return gameplayGlobals_->selection; }
// 폴링 후 키/마우스 사건이 함께 조회할 현재 화면 위치다.
void GameWorld::SetMousePoint(ScreenPoint point) { mousePoint_=point; }
// 선택한 유닛의 주인이 나일 때만 명령을 적용한다.
bool GameWorld::MoveSelected(ScreenPoint point) {
    if (campaign_) {
        if (!input_) throw std::logic_error("실제 클릭 명령 장치 미설치");const auto number=selected_;const auto projected=input_->Project(point);const auto truncated=RawCursorPosition::TruncatePoint({projected.x,projected.y}); //원본 Userinput.cpp는4971e0→41d770 뒤42a360을 호출한다. 호버의 거의 올림과 구별한다.
        input_->carrier->Commit(number,{truncated[0],truncated[1]});changed_=true;return number!=0;
    }
    RefreshSurfaceGround();
    const auto id=selected_; selected_=0; changed_=true; if (id==0 || id>objects_.size()) return false;
    auto& object=objects_[id-1]; if (object.owner!=1 || !object.mobile || object.speed<=0) return false;
    const auto goal=ToCell(point); auto route=ground_->Path(object.cell,goal,object.id,players_.players[1].allies);
    if (route.empty()) return false;
    // 이동 도중 새 명령을 내리면 현재 걸음을 완료한 뒤 같은 연속 위치에서 새 길로 이어 간다.
    if (object.walking && object.next<object.route.size() && object.progress>0) {
        const auto pending=object.route[object.next]; auto tail=ground_->Path(pending,goal,object.id,players_.players[1].allies);
        if (pending!=goal && tail.empty()) return false; tail.insert(tail.begin(),pending); route=std::move(tail);
    } else object.progress=0;
    object.goal=goal; object.route=std::move(route); object.next=0; object.walking=true; object.Face(object.route[0]); object.frame=Frame(object); return true;
}
// 존재하지 않는 번호와 0은 널이다.
const o::Squid* GameWorld::Object(o::SquidId id) const {
    if (campaign_) {
        const auto found=std::find_if(objects_.begin(),objects_.end(),[id](const o::Squid& object) { return object.id==id; });
        return found==objects_.end() ? nullptr : &*found;
    }
    return id>0 && id<=objects_.size() ? &objects_[id-1] : nullptr;
}
// 화면 경계 밖 그림은 제출하지 않아 지형 전체를 매번 그리지 않는다.
std::vector<RenderSprite> GameWorld::Sprites() const {
    std::vector<RenderSprite> result;const auto clip=MapView(); // 실제 표면/모바일/입자 수집도 같은 현재 지도 경계를 사용한다.
    // 현재 프레임을 화면 범위와 소유자 색에 맞춰 장면에 추가한다.
    const auto add=[&](const TypeAsset& type,std::size_t frame,double x,double y,int owner,std::int16_t depth,bool shadow,std::vector<std::size_t> following=std::vector<std::size_t>{},ScreenPoint hotspot=ScreenPoint{}) {
        const auto& frames=assets_.Shapes().Blocks()[type.block].frames; if (frame>=frames.size() || frames[frame].IsSpecial()) return;
        const auto projected=ToScreen(x,y);const ScreenPoint p{projected.x-hotspot.x,projected.y-hotspot.y};auto r=frames[frame].rect; // 화면 위치만 보정하고 원본 정렬용 월드 좌표는 유지한다.
        // 같은 객체 내부의 후속 그림까지 포함해 화면 밖 판정을 한다.
        for (const auto number:following) {
            if (number>=frames.size()) throw std::out_of_range("후속 그림의 물리 프레임 범위 오류");
            const auto& layer=frames[number];if (layer.IsSpecial()) continue;
            r={std::min(r.left,layer.rect.left),std::min(r.top,layer.rect.top),std::max(r.right,layer.rect.right),std::max(r.bottom,layer.rect.bottom)};
        }
        if (p.x+r.right<clip.left || p.x+r.left>=clip.right || p.y+r.bottom<clip.top || p.y+r.top>=clip.bottom) return;
        result.push_back({&assets_.Shapes(),type.block,frame,p.x,p.y,clip,{static_cast<float>(x),static_cast<float>(y),depth},shadow ? std::optional<ColorMap>{} : Remap(type,owner),shadow,std::move(following)});
    };
    // 받침·지면·절벽은 원본 이미지 기준점으로 제출한다.
    for (const auto& tile:tiles_) if (o::AsciiLower(tile.type->assetName)=="fringe") add(*tile.type,tile.frame,tile.x,tile.y,tile.owner,tile.depth,false);
    // 지면·받침·종유석·다리·연결 조각은 현재 raw SID의 프레임/좌표/소유자를 직접 제출한다.
    for (const auto& snapshot:(campaign_ ? campaign_->World() : *surfaces_).Objects()) {
        const auto& object=snapshot.object;if (object.type<o::kFirstAssetTypeNumber || object.type-o::kFirstAssetTypeNumber>=assets_.Types().size()) continue;
        const auto& type=assets_.Types()[object.type-o::kFirstAssetTypeNumber];
        if (factoryDisplay_ && factoryDisplay_->Handles(snapshot.sid)) continue; // 원본 수집/가상 Draw가 제출한 같은 본체를 중복 출력하지 않는다.
        const auto name=o::AsciiLower(type.assetName);if (name=="noisland") continue;
        const auto raw=(campaign_ ? campaign_->Core() : surfaces_->Core()).pool.Slot(snapshot.sid); // 같은 실제 슬롯의 signed 깊이를 수집기와 동일하게 보존한다.
        const std::int16_t depth=std::bit_cast<std::int8_t>(raw[assets_.Edition()==o::OriginalEdition::Patch1078 ? 35 : 33]);
        if (campaign_) {
            // alive·일반 색의 실제 가상 Draw만 연결한다. abstract/dead 색 분기는 기존 SpriteDraw 통합의 후속이다.
            if (o::IsRainVortex(assets_.Edition(),raw) && (raw[40]&0x13U)==2U) {
                const auto visible=campaign_->Core().display.Bounds(object.type,object.frame,object.x,object.y);
                if (visible.right<=clip.left || visible.left>=clip.right || visible.bottom<=clip.top || visible.top>=clip.bottom) continue;
                std::vector<std::size_t> layers;
                // 원본 가상 Draw의 호출 순서를 한 Renderer 항목 안에 보관한다.
                campaign_->DrawRainTemple(snapshot.sid,[&](std::int32_t number) {
                    if (number<0) throw std::out_of_range("레인 신전 음수 그림 번호");layers.push_back(static_cast<std::size_t>(number));
                });
                if (layers.empty()) throw std::logic_error("레인 신전 기본 그림 제출 누락");
                const auto first=layers.front();layers.erase(layers.begin());
                add(type,first,object.x,object.y,static_cast<int>(object.owner),depth,false,std::move(layers));continue;
            }
        }
        ScreenPoint hotspot{}; // 실제 다리/골렘과 여섯 flyershadow 부모의 본체도 그림자와 같은 TYPE 발 기준점으로 표시한다.
        if (assets_.Edition()==o::OriginalEdition::Patch1078 && (IsGolem(raw) || (assets_.TypeTable().Types()[raw[10]].flags1&0x400000U) || (raw[10]==82 && raw[0]==0xc8 && raw[1]==0x34 && raw[2]==0x50 && raw[3]==0))) {
            const auto& record=assets_.TypeTable().Types()[raw[10]];hotspot={record.hotspotX,record.hotspotY};
        }
        add(type,static_cast<std::size_t>(object.frame),object.x,object.y,static_cast<int>(object.owner),depth,false,{},hotspot);
    }
    // 살아 있는 객체의 현재 좌표와 현재 프레임을 제출한다.
    for (const auto& object:objects_) if (!campaign_ && object.visible && !Has(assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)],"bridge")) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        add(type,object.frame,object.x,object.y,object.owner,kObjectDepth,false);
    }
    // Factory 본체/그림자는 원본 수집 깊이와 y보정을 유지하고 공통 장면 정렬에 넣는다.
    if (factoryDisplay_) for (auto& sprite:factoryDisplay_->Sprites(clip,CameraX(),CameraY(),campaign_->LocalOwner(),hideBuildings_())) result.push_back(std::move(sprite));
    // 실제 입자도 같은 Renderer 정렬에 넣으며 화면의 뒤에 별도 overlay로 덮지 않는다.
    if (campaign_) for (auto& sprite:campaign_->ParticleSprites(clip)) result.push_back(std::move(sprite));
    return result;
}
// 현재 표시 팔레트에서 가까운 색 번호를 찾는다.
std::uint8_t GameWorld::Color(int r,int g,int b) const {
    const auto& colors=palette_; int best=std::numeric_limits<int>::max(); std::uint8_t chosen=0;
    // 첫 동률을 유지한다.
    for (std::size_t i=0;i<colors.size();++i) { const int dr=r-colors[i].red,dg=g-colors[i].green,db=b-colors[i].blue,delta=dr*dr+dg*dg+db*db; if (delta<best) { best=delta; chosen=static_cast<std::uint8_t>(i); } }
    return chosen;
}
// 선택 표시의 비례와 사각형 위치는 영상 근거의 어댑터이며 Gump 원본 함수 전체는 후속이다.
std::shared_ptr<IndexedImage> GameWorld::SelectionImage() const {
    if (!selected_) return {};
    if (factoryDisplay_ && factoryDisplay_->Handles(o::Sid{static_cast<std::uint16_t>(selected_)})) return {}; // 원본 Draw의 괄호/체력 막대와 중복하지 않는다.
    const auto* object=Object(selected_); if (!object) return {};
    auto image=std::make_shared<IndexedImage>(); image->width=width_; image->height=height_; image->indices.resize(static_cast<std::size_t>(width_)*height_); image->opacity.resize(image->indices.size());
    const auto cyan=Color(0,255,255),green=Color(0,255,0); const auto p=ToScreen(object->x,object->y); const auto r=Bounds(*object);
    // 게임 영역 안의 한 픽셀만 투명 선택 표시 위에 기록한다.
    const auto view=MapView();const auto dot=[&](int x,int y,std::uint8_t color) { if (x>=view.left && x<view.right && y>=view.top && y<view.bottom && x>=0 && x<width_ && y>=0 && y<height_) { const auto i=static_cast<std::size_t>(y*width_+x); image->indices[i]=color; image->opacity[i]=255; } }; // 같은 실제 지도 안의 한 픽셀만 쓴다.
    // 발밑 네 모서리에 원본과 같은 청록 괄호를 표시한다.
    for (int side=-1;side<=1;side+=2)
        // 위와 아래 모서리를 각각 그린다.
        for (int dy=-1;dy<=1;dy+=2)
            // 모서리의 가로·세로 네 픽셀을 채운다.
            for (int i=0;i<4;++i) { dot(p.x+side*(10-i),p.y+dy*4,cyan); dot(p.x+side*10,p.y+dy*(4-i),cyan); }
    if (object->maxHitPoints>0) {
        const int length=static_cast<int>(20*std::clamp(object->hitPoints/object->maxHitPoints,0.0,1.0));
        // 그림 머리 위 체력 막대의 배경과 현재 체력을 그린다.
        for (int x=0;x<22;++x)
            // 위아래 테두리를 포함한 네 행을 채운다.
            for (int y=0;y<4;++y) dot((r.left+r.right)/2-11+x,r.top-6+y,x>0 && x<=length && y>0 && y<3 ? green : 0);
    }
    return image;
}
// 타입별 색 띠 중 실제 월드에서 필요한 지면·사제·다리·일부 유닛을 연결한다.
std::optional<ColorMap> GameWorld::Remap(const TypeAsset& type,int owner) const {
    if (owner<=0 || owner>o::kPlayerCount) return {}; const auto name=o::AsciiLower(type.assetName);
    const bool support=assets_.Edition()==o::OriginalEdition::Patch1078 && type.block+o::kFirstAssetTypeNumber==94; // 원본43bc40의 작은 섬 윗면이며 하단95는 전용 색표 대상이 아니다.
    const int color=campaign_ && (name=="bridge" || support) ? campaign_->Core().bridgeState.ownerColors[static_cast<std::size_t>(owner)] : players_.players[static_cast<std::size_t>(owner)].color; // 실제 다리와 받침은 초기 입력 사본 대신 현재 원본 소유자 색을 읽는다.
    if (color<=0 || color>8) return {};
    const auto key=std::pair{type.block,color}; if (const auto found=remaps_.find(key);found!=remaps_.end()) return found->second;
    if (support) {
        std::array<std::uint32_t,256> logical{}; // 원본 ColorSort와 같은 R/G/B 순서의 파일 팔레트다.
        // 현재 팔레트의 각 색 번호를 유지하며 장치 BGR로 바꾸지 않는다.
        for (std::size_t index=0;index<logical.size();++index) { const auto& value=palette_[index];logical[index]=std::uint32_t(value.red)|(std::uint32_t(value.green)<<8)|(std::uint32_t(value.blue)<<16); }
        const auto result=BuildSpecialOwnerColorMaps(ColorSortLogicalInput(colorSortLogical_,logical),94)[static_cast<std::size_t>(color)]; // 지면 소스33색에 보정0을 쓰는 기존 독립 검증 몸체다.
        remaps_.emplace(key,result);return result;
    }
    const bool isle=name=="isle" || name=="edgefarm";
    const bool band=name=="priest" || name=="bridge" || name=="sunwalker" || name=="rainwalker" || name=="altar" || name=="bulf" || name=="outpost";
    if (!isle && !band) return {}; ColorMap result{};
    // 변환 대상이 아닌 색은 항등 표로 남긴다.
    for (std::size_t i=0;i<result.size();++i) result[i]=static_cast<std::uint8_t>(i);
    const auto& candidates=kOwnerColors[static_cast<std::size_t>(color-1)];
    if (band) {
        // 0043be30: 228~237은 가장 밝은 색이고 238~245는 역순의 여덟 명도다.
        for (int source=228;source<=245;++source) { int shade=source<238 ? 0 : 245-source; if (name=="bridge" && source>=238 && (color==2 || color==7)) shade=color==2 ? 4 : 3; result[static_cast<std::size_t>(source)]=candidates[static_cast<std::size_t>(shade)]; }
    } else {
        std::vector<int> sources{3,177,200,203,224,220,221,222,223,114,201,226};
        // 0043bc40의 연속 지면 색 범위를 더한다.
        for (int source=44;source<=64;++source) sources.push_back(source);
        const auto& colors=palette_;
        // RGB의 최댓값과 최솟값으로 원본 HSL 명도를 계산한다.
        const auto lightness=[&](int index) { const auto& c=colors[static_cast<std::size_t>(index)]; return (std::max({c.red,c.green,c.blue})+std::min({c.red,c.green,c.blue}))/510.0; };
        // 원본 HSL 명도에 소유자 보정값을 더하고 첫 최소 후보를 선택한다.
        for (const int source:sources) {
            const double desired=lightness(source)+kIsleOffsets[static_cast<std::size_t>(color-1)]; auto best=candidates[0]; double distance=std::abs(desired-lightness(best));
            // 뒤 후보는 엄격히 가까울 때만 교체한다.
            for (std::size_t i=1;i<candidates.size();++i) { const double d=std::abs(desired-lightness(candidates[i])); if (d<distance) { best=candidates[i]; distance=d; } }
            result[static_cast<std::size_t>(source)]=best;
        }
    }
    remaps_.emplace(key,result); return result;
}
// 같은 변환을 사용하여 자동 입력의 그림 중심점/월드 칸을 찾는다.
std::optional<ScreenPoint> GameWorld::Point(std::string_view label) const {
    // 자동 입력 전용: 실제 다리 패턴의 새 칸이 목표에 가장 가까워지는 합법 마우스 위치만 찾는다.
    if (label.starts_with("placement-reach:")) {
        if (!campaign_) return {};
        const auto comma=label.find(',');if (comma==std::string_view::npos) return {}; // 검사 목표의 원본 월드 좌표다.
        const auto targetX=std::stoi(std::string(label.substr(16,comma-16))),targetY=std::stoi(std::string(label.substr(comma+1))); // 검사자가 선택한 두 칸이다.
        if (targetX<1 || targetX>255 || targetY<1 || targetY>255) return {};
        const auto& bridge=assets_.Find("bridge");const auto bridgeType=static_cast<std::uint32_t>(bridge.block+o::kFirstAssetTypeNumber); // 현재 다리 타입이다.
        const auto& drop=campaign_->CursorDropSettings();const auto& placement=campaign_->Cursor().Placement(); // 취득한 실제 조각과 미리보기다.
        if (drop.lockDepth || !drop.preview || drop.type!=bridgeType || placement.imageWidth<=0 || placement.imageHeight<=0) return {};
        const auto patterns=o::BridgePatterns(assets_.Edition());const auto frames=bridge.definition.FrameTable(); // 보호 원문 패턴/프레임 코드다.
        if (drop.argument>=patterns.size()) throw std::out_of_range("검사 다리 패턴 번호 범위");
        constexpr int radius=24; // 목표 주변에서만 조회한다.
        std::optional<ScreenPoint> nearest;double bestReach=std::numeric_limits<double>::infinity(),bestOrigin=bestReach; // 새 칸 거리/기준점 거리 순서다.
        // 실제 화면 안에서만 현재 조각의 허용을 조회한다.
        for (int x=std::max(1,targetX-radius);x<=std::min(255,targetX+radius);++x) {
            // 실제 커서/원본 배치 규칙을 통과한 현재 방향만 측정하며 결과를 만들지 않는다.
            for (int y=std::max(1,targetY-radius);y<=std::min(255,targetY+radius);++y) {
                const ScreenPoint mouse{x*16-CameraX()-placement.hotspotX+placement.imageWidth/2,y*11-CameraY()-placement.hotspotY+placement.imageHeight/2}; // 실제 기준점의 역산이다.
                if (mouse.x<80 || mouse.x>=width_ || mouse.y<28 || mouse.y>=height_) continue;
                campaign_->Cursor().Update(mouse.x,mouse.y);if (!drop.legal) continue;
                o::CanonDecoder decoder(frames,patterns[drop.argument],std::bit_cast<std::int32_t>(drop.direction),drop.x,drop.y); // 현재 조각을 원본 반복자로 펼친다.
                double reach=std::numeric_limits<double>::infinity(); // 이미 있는 다리와 겹치는 칸은 전진으로 세지 않는다.
                // 유효한 Canon 칸만 읽으며 현재 월드를 변경하지 않는다.
                while (decoder.Valid()) {
                    if (!campaign_->World().Find(decoder.X(),decoder.Y(),bridgeType).value) {
                        const double dx=static_cast<double>(decoder.X())-targetX,dy=static_cast<double>(decoder.Y())-targetY; // 목표와 새 칸 거리다.
                        reach=std::min(reach,dx*dx+dy*dy);
                    }
                    decoder.Advance();
                }
                const double dx=static_cast<double>(drop.x)-targetX,dy=static_cast<double>(drop.y)-targetY,origin=dx*dx+dy*dy; // 동률의 기준점 거리다.
                if (std::isfinite(reach) && (reach<bestReach || (reach==bestReach && origin<bestOrigin))) { nearest=mouse;bestReach=reach;bestOrigin=origin; }
            }
        }
        if (nearest) campaign_->Cursor().Update(nearest->x,nearest->y); // 마우스만 복원하고 실제 건설은 후속 좌클릭이 수행한다.
        return nearest;
    }
    // 자동 입력 전용: 실제 커서의 공간/공급 판정을 통과한 가까운 화면 좌표를 찾는다. 건설은 이후 실제 클릭이 수행한다.
    if (label.starts_with("placement-near:") || label.starts_with("placement-fire:")) {
        if (!campaign_) return {};const auto comma=label.find(',');if (comma==std::string_view::npos) return {};
        const bool firing=label.starts_with("placement-fire:");const auto second=firing ? label.find(',',comma+1) : std::string_view::npos;if (firing && second==std::string_view::npos) return {};
        const auto targetX=std::stoi(std::string(label.substr(15,comma-15))),targetY=std::stoi(std::string(label.substr(comma+1,firing ? second-comma-1 : std::string_view::npos)));if (targetX<1 || targetX>255 || targetY<1 || targetY>255) return {};
        const auto number=firing ? std::stoul(std::string(label.substr(second+1))) : 0UL; // 테스트 전용 입력의 실제 적 SID이며 원본 분석 결과가 아니다.
        const auto firingType=campaign_->CursorDropSettings().type; // 현재 미리보기 타입으로 썬 캐논/썬 디스크 발사기의 실제 사거리를 각각 질의한다.
        if (number>65535UL || (firing && (!number || (firingType!=128 && firingType!=71)))) return {};const o::Sid target{static_cast<std::uint16_t>(number)};const auto core=campaign_->Core();
        const auto& placement=campaign_->Cursor().Placement();if (placement.imageWidth<=0 || placement.imageHeight<=0) return {};
        constexpr int radius=24; // 목표 주변24칸에서만 찾으며 다른 지역으로 임의 이동하지 않는다.
        std::optional<ScreenPoint> nearest;double distanceLimit=1e9;std::size_t legalCount{},firingCount{};std::array<float,7> requested{}; // 허용/사격 후보와 실패 시 요청 칸의 실제 진단이다.
        std::vector<std::array<float,2>> legalPoints; // 자동 입력 실패 때만 조회할 실제 허용 위치이며 게임 결과로 사용하지 않는다.
        for (int x=std::max(1,targetX-radius);x<=std::min(255,targetX+radius);++x) { // 현재 보드 안의 후보만 질의한다.
            for (int y=std::max(1,targetY-radius);y<=std::min(255,targetY+radius);++y) { // 미리보기 전체가 실제 허용되는지 검사한다.
                const double distance=(x-targetX)*(x-targetX)+(y-targetY)*(y-targetY);if (distance>=distanceLimit) continue;
                const ScreenPoint mouse{x*16-CameraX()-placement.hotspotX+placement.imageWidth/2,y*11-CameraY()-placement.hotspotY+placement.imageHeight/2};
                if (mouse.x<80 || mouse.x>=width_ || mouse.y<28 || mouse.y>=height_) continue; // 고정 사이드바/상단 메뉴와 화면 밖은 클릭하지 않는다.
                campaign_->Cursor().Update(mouse.x,mouse.y);const auto& drop=campaign_->CursorDropSettings();
                if (firing && x==targetX && y==targetY) requested={drop.x,drop.y,static_cast<float>(drop.legal),static_cast<float>(placement.descriptionAllowed),static_cast<float>(placement.placementAllowed),static_cast<float>(placement.sourceAllowed),static_cast<float>(drop.localPlayer)};
                if (!drop.legal) continue;++legalCount;
                if (firing) legalPoints.push_back({drop.x,drop.y}); // 기존 탐색 순서와 선택 판정을 유지하고 관측만 보관한다.
                if (firing) { // 테스트 전용 좌표 탐색이다. 원본 분석 결과가 아니며 실제 MayTarget을 통과하는 자리만 클릭 후보로 삼는다.
                    const auto& type=core.types[firingType];const o::GunTargetQuery query{drop.x,drop.y,static_cast<float>(drop.x-type.footX*.5),static_cast<float>(drop.y-type.footY*.5),firingType,0,drop.localPlayer,0};if ((core.pool.Slot(target)[11]&3U) || !campaign_->GunTargeting().MayTarget(query,target)) continue;
                }
                ++firingCount;
                nearest=mouse;distanceLimit=distance;
            }
        }
        if (nearest) campaign_->Cursor().Update(nearest->x,nearest->y);
        else if (firing) {
            std::fprintf(stderr,"자동 사격 자리 없음: legal%zu fire%zu 요청(%g,%g) legal%g desc%g place%g source%g owner%g snap%u camera%d,%d\n",legalCount,firingCount,requested[0],requested[1],requested[2],requested[3],requested[4],requested[5],requested[6],campaign_->Cursor().SnapToGrid(),CameraX(),CameraY()); // 자동 입력 실패의 실제 판정을 기록하며 성공으로 대체하지 않는다.
            const auto raw=core.pool.Slot(target);const auto footprint=campaign_->GunTargeting().Footprint(target); // 같은 실제 표적 상태와 확장 전 발자국이다.
            std::fprintf(stderr,"자동 사격 표적: sid%u type%u state%u owner%u extra%u foot(%g,%g,%g,%g)\n",target.value,raw[10],raw[11],raw[34],raw[40],footprint.left,footprint.top,footprint.right,footprint.bottom);
            constexpr std::size_t kDiagnosticLimit=64; // 실패 로그만 제한하며 탐색 후보/성공 조건은 제한하지 않는다.
            // 허용된 실제 후보의 공간 조건과 발사 판정을 구별한다.
            for (std::size_t index=0;index<std::min(legalPoints.size(),kDiagnosticLimit);++index) {
                const auto point=legalPoints[index];const auto& type=core.types[firingType]; // 같은 실제 후보/현재 무기 발자국으로 기존 문맥을 재구성한다.
                const o::GunTargetQuery query{point[0],point[1],static_cast<float>(point[0]-type.footX*.5),static_cast<float>(point[1]-type.footY*.5),firingType,0,campaign_->CursorDropSettings().localPlayer,0}; // 기존 자동 입력 판정과 동일한 조회 인수다.
                std::fprintf(stderr,"자동 사격 허용 후보: (%g,%g) shoot%d target%d\n",point[0],point[1],campaign_->GunTargeting().MayShoot(query,target),campaign_->GunTargeting().MayTarget(query,target));
            }
            const auto& type=core.types[firingType];const o::GunTargetQuery query{static_cast<float>(targetX),static_cast<float>(targetY),static_cast<float>(targetX-type.footX*.5),static_cast<float>(targetY-type.footY*.5),firingType,0,campaign_->CursorDropSettings().localPlayer,0}; // 요청 위치의 방향별 사정선을 읽기만 한다.
            // 네 사정선을 기록해 단순 사거리/정렬과 표적 필터를 분리한다.
            for (int direction=0;type.group==1 && direction<8;direction+=2) {
                const auto ray=campaign_->GunTargeting().CannonRay(query,direction); // 실제 TYPE/SHP 포구와 현재 배율이다.
                std::fprintf(stderr,"자동 사격 요청 ray%d: (%g,%g,%g,%g)\n",direction,ray.left,ray.top,ray.right,ray.bottom);
            }
        }
        return nearest;
    }
    // 자동 입력의 건설 목표는 현재 미리보기 치수/기준점으로 마우스 위치만 역산한다. 배치 판정/생성은 실제 클릭 경로를 사용한다.
    if (label.starts_with("placement:")) {
        if (!campaign_) return {};const auto comma=label.find(',');if (comma==std::string_view::npos) return {};const auto x=std::stoi(std::string(label.substr(10,comma-10))),y=std::stoi(std::string(label.substr(comma+1)));if (x<0 || x>=256 || y<0 || y>=256) return {};
        const auto& p=campaign_->Cursor().Placement();if (p.imageWidth<=0 || p.imageHeight<=0) return {};return ScreenPoint{x*16-CameraX()-p.hotspotX+p.imageWidth/2,y*11-CameraY()-p.hotspotY+p.imageHeight/2};
    }
    if (label.starts_with("cell:")) { const auto comma=label.find(','); if (comma==std::string_view::npos) return {}; const int x=std::stoi(std::string(label.substr(5,comma-5))),y=std::stoi(std::string(label.substr(comma+1))); return ToScreen(x-.5,y-.5); } //원본4971e0의+16/+11 역투영에서 논리 칸(x,y)의 내부를 클릭한다. 경계점은 다음 칸이다.
    if (!label.starts_with("world:")) return {}; const auto wanted=o::AsciiLower(label.substr(6));
    // 원본 타입 파일 이름의 첫 내 객체를 고른다.
    for (const auto& object:objects_) if (object.owner==1 && object.selectable) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; if (o::AsciiLower(type.assetName)!=wanted) continue;
        const auto r=Bounds(object); return ScreenPoint{(r.left+r.right)/2,(r.top+r.bottom)/2};
    }
    return {};
}
// 표시 요청을 한 번만 소비한다.
bool GameWorld::TakeChanged() { const bool rawChanged=Surfaces().TakeChanged();return std::exchange(changed_,false) || rawChanged; }
// 순수한 플레이어 초기 상태 조회다.
const o::MissionPlayers& GameWorld::Players() const { return players_; }
// 생성 중 아직 Session이 없으면 0이며 기존 MissionPlayers의 localPlayer로 대체하지 않는다.
std::uint32_t GameWorld::CurrentSubscriber() const { return campaign_ ? campaign_->CurrentSubscriber() : 0U; }
// 게임 명령/검사와 렌더링은 같은 raw 상태를 참조한다.
// 같은 표시/선택/명령의 실제 캠페인 수명만 반환한다.
RawCampaignSession& GameWorld::Session() {
    if (!campaign_) throw std::logic_error("값형 GameWorld에는 실제 캠페인 Session이 없습니다");
    return *campaign_;
}
RawSurfaceWorld& GameWorld::Surfaces() { return campaign_ ? campaign_->World() : *surfaces_; }
// 구 어댑터의 미션 상태를 실제 판정으로 가장하지 않고 동일 raw 세션이 있을 때만 실행한다.
void GameWorld::CheckMissionOutcome() {
    if (!campaign_) throw std::logic_error("실제 캠페인 설치 전 미션 승패 판정");
    campaign_->CheckMissionOutcome();
}
// 스모크 관찰용이며 실제 게임 화면에는 구현 세부 정보를 표시하지 않는다.
std::string GameWorld::Report(bool mask) const {
    std::ostringstream out; out.precision(12);
    out<<"world\t"<<objects_.size()<<'\t'<<tiles_.size()<<"\nselected\t"<<selected_<<"\ncamera\t"<<CameraX()<<'\t'<<CameraY()<<"\n";
    if (campaign_) {
        out<<"raw_input\t"<<selected_<<'\t'<<highlightMode_<<'\t'<<campaign_->LocalConnectionPhase()<<'\t'<<campaign_->LocalConnectionPackets().size()<<"\nraw_packets";
        // 실제 소비된 종류만 진단하며 별도 명령 성공 플래그를 만들지 않는다.
        for (const auto kind:campaign_->LocalConnectionPackets()) out<<'\t'<<std::uint32_t(kind);out<<'\n';
        out<<"raw_session\t"<<campaign_->LocalOwner()<<'\t'<<campaign_->PlayerKnown();
        const auto& drop=campaign_->CursorDropSettings();const auto& placement=campaign_->Cursor().Placement(); // 실제 배치/원본 입력과 같은 저장소다.
        out<<"\nraw_piece\t"<<drop.argument<<'\t'<<drop.direction; // 실제 무작위 조각 번호/현재 회전을 관찰하며 취득 상태를 변경하지 않는다.
        out<<"\nraw_cursor\t"<<drop.lockDepth<<'\t'<<drop.legal<<'\t'<<drop.type<<'\t'<<drop.preview<<'\t'<<drop.x<<'\t'<<drop.y<<'\t'<<drop.inputFlags<<'\t'<<drop.costFlags<<'\t'<<placement.placementAllowed<<'\t'<<placement.spaceAllowed<<'\t'<<placement.sourceAllowed<<"\nraw_sp";
        // 화면용 MissionPlayers 사본과 구별하여 실제 분산 SP의 모든 owner를 읽는다.
        for (std::uint32_t owner=0;owner<9;++owner) out<<'\t'<<campaign_->Sp(owner);
        out<<"\nraw_brains";
        // 실제 Player9에 부착된 AI 포인터만 기록하며 aiName에서 추정하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) out<<'\t'<<campaign_->Participation().Brain(owner);
        out<<"\nraw_technology";
        // 실제 로컬 Player의 같은 기술 내용물 체인을 원본 타입 번호 순서로 조회한다.
        for (std::size_t index=0;index<assets_.Types().size();++index) {
            const auto type=static_cast<std::int32_t>(index+o::kFirstAssetTypeNumber); // 목록의 실제 원본 TYPE 번호다.
            if (campaign_->FindTechnology(static_cast<std::int32_t>(campaign_->LocalOwner()),type).value) out<<'\t'<<assets_.Types()[index].assetName;
        }
        out<<'\n';
        const auto parent=campaign_->CurrentTechnologyParent();out<<"raw_production_parent\t"<<parent<<'\n';
        if (parent) {
            auto core=campaign_->Core();const auto owner=core.pool.Slot(o::Sid{static_cast<std::uint16_t>(parent)});std::size_t visited{};
            const auto word=[](std::span<const std::uint8_t> bytes,std::size_t offset) { return static_cast<std::uint16_t>(bytes[offset]|(std::uint16_t(bytes[offset+1])<<8)); }; // 원본 비정렬WORD만 읽는다.
            // 원래 체인의flags/quality/slot을 읽으며 내용물을 만들거나 정렬하지 않는다.
            for (o::Sid form{word(owner,6)};form.value;) {
                if (++visited>core.pool.Capacity()) throw std::logic_error("생산 진단의 내용물 체인 순환");const auto content=core.pool.Slot(form);
                if (content[10]==6) out<<"raw_production_content\t"<<form.value<<'\t'<<word(content,18)<<'\t'<<word(content,20)<<'\t'<<std::uint32_t(content[30])<<'\t'<<std::uint32_t(content[23])<<'\n';
                form={word(content,4)};
            }
        }

    }
    // 활성 플레이어와 머리 값/저장 상태를 서로 다른 열에 기록한다.
    for (const auto& p:players_.players) if (p.active) out<<"player\t"<<p.number<<'\t'<<p.stormPower<<'\t'<<p.color<<'\t'<<p.allies<<'\t'<<Field(p.name)<<'\t'<<Field(p.startingTech)<<'\t'<<p.storedTechnology.items.size()<<'\t'<<p.storedDeck.size()<<"\n";
    // 원본 저장 레코드와 이동 중의 좌표를 대조한다.
    for (const auto& object:objects_) { const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; out<<"object\t"<<object.id<<'\t'<<type.assetName<<'\t'<<object.owner<<'\t'<<object.territory<<'\t'<<object.cell.x<<'\t'<<object.cell.y<<'\t'<<object.x<<'\t'<<object.y<<'\t'<<object.frame<<'\t'<<object.walking<<'\t'<<object.goal.x<<'\t'<<object.goal.y<<'\t'<<object.quantity<<'\t'<<object.hitPoints<<'\t'<<object.contents.items.size()<<"\n"; }
    if (mask) {
        out<<"land\t";
        // 마스크 바이트를 표현할 16진수 숫자 표다.
        constexpr char digits[]="0123456789abcdef";
        // 각 바이트를 두 자리 16진수로 기록한다.
        if (campaign_) { for (const auto value:campaign_->Core().spots) out<<digits[value>>4]<<digits[value&15]; }
        else { for (const auto value:terrain_->Mask()) out<<digits[value>>4]<<digits[value&15]; }
        out<<"\n";
    }
    out<<(campaign_ ? campaign_->World() : *surfaces_).Report();
    return out.str();
}
}

namespace netstorm::client { // 실제Session과 기존 값형World의 카메라 수명을 구별한다.
ClientCameraGlobals& GameWorld::CameraGlobals() { return gameplayGlobals_->Camera(); }
const ClientCameraGlobals& GameWorld::CameraGlobals() const { return gameplayGlobals_->Camera(); }
void GameWorld::ApplyCameraPixels(std::int32_t x,std::int32_t y) { auto& camera=CameraGlobals();camera.RequestPixels(x,y);camera.ApplyPixelsToRendererAndSnapshot(x,y);changed_=true; }
}
