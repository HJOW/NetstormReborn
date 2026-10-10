#include "client/GameWorld.h"
#include "client/SquidRenderer.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawPathAnimation.h"
#include "o/OriginalText.h"
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
}
// 저장 위치와 파일 필드를 유지하여 표시와 이동이 공유하는 월드를 만든다.
GameWorld::GameWorld(const GameAssets& assets,const o::FortTemplate& fort,o::MissionPlayers players)
    : assets_(assets),ground_(std::make_unique<o::GroundGrid>()),players_(std::move(players)) {
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
        surfaceTime_=frameTime();SurfaceView();surfaces_->RunFrame(surfaceTime_,stopped);RefreshSurfaceGround();
        if (!stopped) AdvanceObjects(surfaceTime_.delta);
    } else if (elapsed && !stopped) Step(elapsed());
}
// 콘솔에서는 같은 실행 경로에 누적된 게임 시각을 공급한다.
void GameWorld::Step(double seconds) {
    if (!std::isfinite(seconds) || seconds<0) throw std::invalid_argument("월드 시간차 범위 오류");
    surfaceTime_.game+=seconds;surfaceTime_.delta=seconds;++surfaceTime_.number;SurfaceView();surfaces_->RunFrame(surfaceTime_,false);RefreshSurfaceGround();AdvanceObjects(seconds);
}
// 비표면 유닛의 관찰 기반 이동은 raw 표면 렌더링과 구별한다.
void GameWorld::AdvanceObjects(double seconds) {
    // 객체 번호 순서로 갱신한다.
    for (auto& object:objects_) if (object.owner>0 && object.owner<=o::kPlayerCount && object.Advance(seconds,*ground_,players_.players[static_cast<std::size_t>(object.owner)].allies)) { object.frame=Frame(object); changed_=true; }
}
// 현재 카메라·월드 표시 영역을 raw 표시와 위치 효과음에 함께 전달한다. 메뉴 줄을 제외한 현재 GUI 영역을 그대로 쓴다.
// 원본 가변 생산/상태 패널의 영역 계산은 아직 복원하지 않았으며, 위치 계산 함수 자체는 SoundPlayer의 원본 규칙을 쓴다.
void GameWorld::SurfaceView() {
    const o::SquidDisplayRect clip{0,std::min(kMenuHeight,height_),width_,height_};
    surfaces_->SetDisplay({scrollX_,scrollY_,65536,clip},surfaceDisplay);
    if (soundViewChanged) soundViewChanged({scrollX_,scrollY_,clip.left,clip.top,clip.right,clip.bottom});
}
// 카메라 크기만 바뀌어도 다시 표시하고 위치 효과음의 화면 기준을 갱신한다. 같은 크기로 불러도 새 수신자를 갱신한다.
void GameWorld::Resize(int width,int height) {
    const bool first=width_==0; if (width_==width && height_==height) { SurfaceView();return; }
    width_=width; height_=height; if (first) Home(false); else Scroll(0,0); changed_=true;
}
// 월드 테두리 밖으로 카메라가 나가지 않게 제한하고, 같은 입력 단계의 위치 효과음에 새 원점을 전달한다.
void GameWorld::Scroll(int dx,int dy) {
    const int x=std::clamp(scrollX_+dx,0,std::max(0,o::kWorldCells*kCellPixelsX-width_));
    const int y=std::clamp(scrollY_+dy,0,std::max(0,o::kWorldCells*kCellPixelsY-height_));
    if (x!=scrollX_ || y!=scrollY_) { scrollX_=x; scrollY_=y; changed_=true; }
    SurfaceView();
}
// 처음에는 내 신전, 신전이 없으면 내 사제를 가운데에 둔다.
void GameWorld::Home(bool priest) {
    const o::Squid* target=nullptr;
    // 내 객체만 홈 대상으로 고른다.
    for (const auto& object:objects_) if (object.owner==1) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        if ((priest && Has(type,"priest")) || (!priest && Has(type,"vortex"))) { target=&object; break; }
        if (!target && Has(type,"priest")) target=&object;
    }
    if (target) { const auto size=assets_.Types()[static_cast<std::size_t>(target->type-o::kFirstAssetTypeNumber)].definition.Footprint(); scrollX_=static_cast<int>((target->x-(size[0]-1)*0.5)*16)-width_/2; scrollY_=static_cast<int>((target->y-(size[1]-1)*0.5)*11)-height_/2; Scroll(0,0); changed_=true; }
}
// 월드 기준점은 원본 SHP 사각형의 원점과 동일하다.
ScreenPoint GameWorld::ToScreen(double x,double y) const { return {static_cast<int>(std::lround(x*16))-scrollX_,static_cast<int>(std::lround(y*11))-scrollY_}; }
// 이동 목표는 가장 가까운 칸이다. 월드 밖은 경로 탐색에서 거부한다.
o::CellPoint GameWorld::ToCell(ScreenPoint p) const { return {static_cast<int>(std::lround((p.x+scrollX_)/16.0)),static_cast<int>(std::lround((p.y+scrollY_)/11.0))}; }
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
    const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; const auto& frame=assets_.Shapes().Blocks()[type.block].frames[object.frame];
    const auto p=ToScreen(object.x,object.y); return {p.x+frame.rect.left,p.y+frame.rect.top,p.x+frame.rect.right+1,p.y+frame.rect.bottom+1};
}
// 가장 앞에 가까운 실제 그림을 선택한다. 모바일 그림에는 녹화 기반 4픽셀 여유가 있다.
o::SquidId GameWorld::Pick(ScreenPoint point) const {
    if (point.y<kMenuHeight || point.x<0 || point.y>=height_ || point.x>=width_) return 0;
    o::SquidId picked=0; double best=std::numeric_limits<double>::max();
    // 겹친 그림은 클릭과 중심점의 거리가 가장 가까운 객체를 고른다.
    for (const auto& object:objects_) if (object.selectable) {
        const auto r=Bounds(object); const int margin=object.mobile ? 4 : 0;
        if (point.x<r.left-margin || point.x>=r.right+margin || point.y<r.top-margin || point.y>=r.bottom+margin) continue;
        const double dx=point.x-(r.left+r.right)*0.5,dy=point.y-(r.top+r.bottom)*0.5,distance=dx*dx+dy*dy;
        if (distance<best) { picked=object.id; best=distance; }
    }
    return picked;
}
// 없는 객체 또는 비선택 타입을 선택하지 않는다.
void GameWorld::Select(o::SquidId id) { const auto* object=Object(id); selected_=object && object->selectable ? id : 0; changed_=true; }
// 선택한 유닛의 주인이 나일 때만 명령을 적용한다.
bool GameWorld::MoveSelected(ScreenPoint point) {
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
const o::Squid* GameWorld::Object(o::SquidId id) const { return id>0 && id<=objects_.size() ? &objects_[id-1] : nullptr; }
// 화면 경계 밖 그림은 제출하지 않아 지형 전체를 매번 그리지 않는다.
std::vector<RenderSprite> GameWorld::Sprites() const {
    std::vector<RenderSprite> result; const ScreenRect clip{0,kMenuHeight,width_,height_};
    // 현재 프레임을 화면 범위와 소유자 색에 맞춰 장면에 추가한다.
    const auto add=[&](const TypeAsset& type,std::size_t frame,double x,double y,int owner,std::int16_t depth,bool shadow) {
        const auto& frames=assets_.Shapes().Blocks()[type.block].frames; if (frame>=frames.size() || frames[frame].IsSpecial()) return;
        const auto p=ToScreen(x,y); const auto r=frames[frame].rect;
        if (p.x+r.right<clip.left || p.x+r.left>=clip.right || p.y+r.bottom<clip.top || p.y+r.top>=clip.bottom) return;
        result.push_back({&assets_.Shapes(),type.block,frame,p.x,p.y,clip,{static_cast<float>(x),static_cast<float>(y),depth},shadow ? std::optional<ColorMap>{} : Remap(type,owner),shadow});
    };
    // 받침·지면·절벽은 원본 이미지 기준점으로 제출한다.
    for (const auto& tile:tiles_) if (o::AsciiLower(tile.type->assetName)=="fringe") add(*tile.type,tile.frame,tile.x,tile.y,tile.owner,tile.depth,false);
    // 지면·받침·종유석·다리·연결 조각은 현재 raw SID의 프레임/좌표/소유자를 직접 제출한다.
    for (const auto& snapshot:surfaces_->Objects()) {
        const auto& object=snapshot.object;const auto& type=assets_.Types()[object.type-o::kFirstAssetTypeNumber];
        const auto name=o::AsciiLower(type.assetName);if (name=="noisland") continue;
        const auto depth=name=="islandstalag" ? kCliffDepth : (name=="isle" || name=="island" ? kLandDepth : kObjectDepth);
        add(type,static_cast<std::size_t>(object.frame),object.x,object.y,static_cast<int>(object.owner),depth,false);
    }
    // 살아 있는 객체의 현재 좌표와 현재 프레임을 제출한다.
    for (const auto& object:objects_) if (object.visible && !Has(assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)],"bridge")) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)];
        add(type,object.frame,object.x,object.y,object.owner,kObjectDepth,false);
    }
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
    if (!selected_) return {}; const auto* object=Object(selected_); if (!object) return {};
    auto image=std::make_shared<IndexedImage>(); image->width=width_; image->height=height_; image->indices.resize(static_cast<std::size_t>(width_)*height_); image->opacity.resize(image->indices.size());
    const auto cyan=Color(0,255,255),green=Color(0,255,0); const auto p=ToScreen(object->x,object->y); const auto r=Bounds(*object);
    // 게임 영역 안의 한 픽셀만 투명 선택 표시 위에 기록한다.
    const auto dot=[&](int x,int y,std::uint8_t color) { if (x>=0 && x<width_ && y>=kMenuHeight && y<height_) { const auto i=static_cast<std::size_t>(y*width_+x); image->indices[i]=color; image->opacity[i]=255; } };
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
    const int color=players_.players[static_cast<std::size_t>(owner)].color; if (color<=0 || color>8) return {};
    const auto key=std::pair{type.block,color}; if (const auto found=remaps_.find(key);found!=remaps_.end()) return found->second;
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
    if (label.starts_with("cell:")) { const auto comma=label.find(','); if (comma==std::string_view::npos) return {}; const int x=std::stoi(std::string(label.substr(5,comma-5))),y=std::stoi(std::string(label.substr(comma+1))); return ToScreen(x,y); }
    if (!label.starts_with("world:")) return {}; const auto wanted=o::AsciiLower(label.substr(6));
    // 원본 타입 파일 이름의 첫 내 객체를 고른다.
    for (const auto& object:objects_) if (object.owner==1 && object.selectable) {
        const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; if (o::AsciiLower(type.assetName)!=wanted) continue;
        const auto r=Bounds(object); return ScreenPoint{(r.left+r.right)/2,(r.top+r.bottom)/2};
    }
    return {};
}
// 표시 요청을 한 번만 소비한다.
bool GameWorld::TakeChanged() { const bool rawChanged=surfaces_->TakeChanged();return std::exchange(changed_,false) || rawChanged; }
// 순수한 플레이어 초기 상태 조회다.
const o::MissionPlayers& GameWorld::Players() const { return players_; }
// 게임 명령/검사와 렌더링은 같은 raw 상태를 참조한다.
RawSurfaceWorld& GameWorld::Surfaces() { return *surfaces_; }
// 스모크 관찰용이며 실제 게임 화면에는 구현 세부 정보를 표시하지 않는다.
std::string GameWorld::Report(bool mask) const {
    std::ostringstream out; out.precision(12);
    out<<"world\t"<<objects_.size()<<'\t'<<tiles_.size()<<"\nselected\t"<<selected_<<"\ncamera\t"<<scrollX_<<'\t'<<scrollY_<<"\n";
    // 활성 플레이어와 머리 값/저장 상태를 서로 다른 열에 기록한다.
    for (const auto& p:players_.players) if (p.active) out<<"player\t"<<p.number<<'\t'<<p.stormPower<<'\t'<<p.color<<'\t'<<p.allies<<'\t'<<Field(p.name)<<'\t'<<Field(p.startingTech)<<'\t'<<p.storedTechnology.items.size()<<'\t'<<p.storedDeck.size()<<"\n";
    // 원본 저장 레코드와 이동 중의 좌표를 대조한다.
    for (const auto& object:objects_) { const auto& type=assets_.Types()[static_cast<std::size_t>(object.type-o::kFirstAssetTypeNumber)]; out<<"object\t"<<object.id<<'\t'<<type.assetName<<'\t'<<object.owner<<'\t'<<object.territory<<'\t'<<object.cell.x<<'\t'<<object.cell.y<<'\t'<<object.x<<'\t'<<object.y<<'\t'<<object.frame<<'\t'<<object.walking<<'\t'<<object.goal.x<<'\t'<<object.goal.y<<'\t'<<object.quantity<<'\t'<<object.hitPoints<<'\t'<<object.contents.items.size()<<"\n"; }
    if (mask) {
        out<<"land\t";
        // 마스크 바이트를 표현할 16진수 숫자 표다.
        constexpr char digits[]="0123456789abcdef";
        // 각 바이트를 두 자리 16진수로 기록한다.
        for (const auto value:terrain_->Mask()) out<<digits[value>>4]<<digits[value&15];
        out<<"\n";
    }
    out<<surfaces_->Report();
    return out.str();
}
}
