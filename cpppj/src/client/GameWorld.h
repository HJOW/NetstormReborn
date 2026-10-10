// 현재 복원한 저장 미션·지면·Squid 필드를 클라이언트에 연결하는 새 월드 어댑터.
// 원본 Mission/CoreData/타입별 생성자 전체의 복원과는 구분한다.
#pragma once
#include "client/GameAssets.h"
#include "client/Renderer.h"
#include "client/RawSurfaceWorld.h"
#include "client/Sound.h"
#include "o/BaseProcess.h"
#include "o/Player.h"
#include "o/Squid.h"
#include "o/Islandbuilder.h"
#include <map>

namespace netstorm::client {
class GameWorld : public o::BaseProcess {
public:
    // 저장 위치를 쓰는 Tutorial 미션의 초기 월드를 만든다. Battle 재배치는 아직 지원하지 않는다.
    GameWorld(const GameAssets& assets,const o::FortTemplate& fort,o::MissionPlayers players);
    // Kernel에서 호출할 실제 게임 시간과 정지 상태의 공급자를 연결한다.
    std::function<double()> elapsed;
    std::function<bool()> paused;
    // 클라이언트 GameClock이 고정한 절대 게임 시각/프레임 번호를 raw Kernel과 공유한다.
    std::function<o::FrameTime()> frameTime;
    SurfaceDisplayHooks surfaceDisplay;
    // 현재 카메라와 실제 월드 표시 영역을 위치 효과음에 전달한다. Resize·Scroll 및 raw 프레임 처리 전에 호출한다.
    // 수신 객체는 월드보다 오래 살아야 한다. 비어 있으면 소리 없이 같은 월드 경로를 사용한다.
    std::function<void(SoundView)> soundViewChanged;
    // 원본 클라이언트 커널의 갱신 단계에서 이동을 진행한다.
    void RunFrame() override;
    // 검사에서는 OS 시계 없이 같은 이동 경로를 진행한다.
    void Step(double seconds);
    // 화면 크기를 갱신하고 최초에는 내 신전/사제에 카메라를 둔다.
    void Resize(int width,int height);
    // 카메라를 월드 픽셀 범위 안에서 옮긴다.
    void Scroll(int dx,int dy);
    // F4/H·F5의 원본 홈 신전/사제 보기를 연결한다.
    void Home(bool priest);
    // 화면 좌표와 실제 렌더링 그림으로 선택 가능한 오브젝트를 찾는다.
    o::SquidId Pick(ScreenPoint point) const;
    // 선택 번호를 현재 월드에서 검증하고 표시를 갱신한다.
    void Select(o::SquidId id);
    // 선택한 내 이동 유닛에 땅 좌클릭 명령을 내린다. 실패해도 선택은 해제한다.
    bool MoveSelected(ScreenPoint point);
    // 현재 스프라이트·지형의 깊이 순 목록을 수집한다. 정적 InspectView를 거치지 않는다.
    std::vector<RenderSprite> Sprites() const;
    // 선택 괄호와 체력 막대를 원본 팔레트 색으로 합성한다.
    std::shared_ptr<IndexedImage> SelectionImage() const;
    // 입력/검사에서 쓰는 동일 화면 좌표 변환.
    ScreenPoint ToScreen(double x,double y) const;
    o::CellPoint ToCell(ScreenPoint point) const;
    // 자동 검사의 실제 그림/칸 좌표. 명령 실행에는 사용하지 않는다.
    std::optional<ScreenPoint> Point(std::string_view label) const;
    // 미션과 객체 수명 안에서만 유효한 런타임 번호를 조회한다.
    const o::Squid* Object(o::SquidId id) const;
    // 모드 변경·선택·이동·카메라 변경이 표시를 요구했는지 읽고 지운다.
    bool TakeChanged();
    // 원본 파일 없이도 스모크가 초기 상태/이동 결과를 관찰하도록 TSV를 만든다.
    std::string Report(bool mask=false) const;
    // 현재 미션의 시작 자원·기술·동맹 상태를 읽기 전용으로 조회한다.
    const o::MissionPlayers& Players() const;
    // 검사/후속 게임 명령이 사용하는 실제 raw 표면 월드다. 표시도 같은 슬롯을 읽는다.
    RawSurfaceWorld& Surfaces();
private:
    struct Tile { const TypeAsset* type{}; std::size_t frame{}; int x{},y{},owner{}; std::int16_t depth{}; };
    // 청크 순서의 저장 객체를 월드 좌표에 생성한다.
    void Place(const o::FortChunkSection& section,std::span<const o::ChunkCoordinate> chunks,int territory);
    // 지면·받침·절벽 프레임과 정지 점유 격자를 만든다.
    void BuildTerrain();
    // 지형/저장 입력을 실제 표면 SID로 등록한다. 비표면 유닛 이동은 기존 어댑터에 남긴다.
    void BuildSurfaces();
    // 기존 비표면 유닛의 관찰 기반 이동을 진행한다.
    void AdvanceObjects(double seconds);
    // 현재 카메라/장치 변경을 raw 표시 경계에 전달한다.
    void SurfaceView();
    // 임시 보행 격자의 표면/다리 상태도 현재 raw 슬롯에서 갱신한다.
    void RefreshSurfaceGround();
    // 지면 방향·원소와 조명 조건으로 허용 프레임을 선택한다. 변형 선택은 고정 시드의 어댑터다.
    std::size_t TerrainFrame(const TypeAsset& type,char a,char b,int theme,int x,int y,bool core) const;
    // 클러스터 기준 프레임과 보행 상태에서 실제 표시 프레임을 결정한다.
    std::size_t Frame(const o::Squid& object) const;
    // 그림 사각형을 원본 SHP의 기준점으로 변환한다.
    ScreenRect Bounds(const o::Squid& object) const;
    // 선택·소유자 표시에 쓰는 가장 가까운 팔레트 색을 찾는다.
    std::uint8_t Color(int r,int g,int b) const;
    // 원본 기본 색 띠 및 지면 명도 표 중 연결한 타입의 변환표.
    std::optional<ColorMap> Remap(const TypeAsset& type,int owner) const;
    const GameAssets& assets_;
    o::ChunkMap chunks_;
    o::IslandList islands_;
    std::unique_ptr<o::TerrainBuilder> terrain_;
    std::unique_ptr<o::GroundGrid> ground_;
    o::MissionPlayers players_;
    std::vector<o::Squid> objects_;
    std::vector<Tile> tiles_;
    std::unique_ptr<RawSurfaceWorld> surfaces_;
    o::FrameTime surfaceTime_{};
    std::vector<o::CellPoint> surfaceCells_;
    std::uint32_t bridgeType_{},noIslandType_{};
    std::array<int,o::kTerritoryCount> regionOwners_{},regionThemes_{};
    std::array<int,99> coreVariants_{};
    std::array<PaletteColor,256> palette_{};
    mutable std::map<std::pair<std::size_t,int>,ColorMap> remaps_;
    int width_{},height_{},scrollX_{},scrollY_{};
    o::SquidId selected_{};
    bool changed_{true};
};
}
