using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// RU-4a 실제 Root 미니맵 설치다. 같은 지도 getter와 미니맵 전역을 공유한다. 0.991에서 추가됐다.
public sealed partial class ClientHostGame
{
    /// <summary>같은 실제 트리에 설치한 장기 미니맵 소유자다. 미션보다 오래 산다.</summary>
    private RawMinimapControls? _nativeMinimap;
    /// <summary>같은 실제 공유 bitmap과 큐다. 미션 교체 뒤에도 유지한다.</summary>
    private readonly MetadisplayDirtyState _minimapState = new();
    /// <summary>같은 실제 전용 팔레트와 현재 토큰과 serial이다. 미션 교체 뒤에도 유지한다.</summary>
    private readonly RawMinimapGlobals _minimapGlobals = new();

    /// <summary>같은 실제 메뉴/장기 저장소/미니맵 전역 소유자다. 초기 미니맵 공장은 Open이 아닌 Construct를 쓴다.</summary>
    public RawMinimapControls NativeMinimap => _nativeMinimap ?? throw new InvalidOperationException("실제 미니맵 공장 미설치");
    /// <summary>실제 메뉴 트리와 장기 저장소로 미니맵을 만들고 현재 bitmap 폭으로 생성한다. 전체 viewport로 지도 영역을 대신하지 않는다.</summary>
    /// <remarks>2026-11-25 0.991에서 RU-4a용으로 추가했다. 근거: cpppj RawButtonControls.cpp의 초기 미니맵 공장(Construct)과 0.822의 공유 MapRevision 공급. 1.118에서 일반 중심 이동·현재 선택 모드·포인터 폴링을 연결했다. 선택 유닛 이동은 R6에서 잇는다.</remarks>
    private void InitializeNativeMinimap()
    {
        if (_nativeMinimap is not null) throw new InvalidOperationException("미니맵 공장 중복 설치");
        var menu = _mainMenu ?? throw new InvalidOperationException("미니맵 공장의 실제 공통 트리 없음");
        var baseHandle = menu.Tree.EventHooks.Handle;
        var minimap = new RawMinimapControls(menu.Tree, _surfaceGlobals.Storage, _minimapState, _minimapGlobals, _gameplay.MapRevision, new()
        {
            // 원본 미니맵 기하는 화면 픽셀이 아닌 카메라의 월드 좌표와 현재 칸 배율을 사용한다.
            View = () => new MinimapGeometryView(
                _gameplay.Camera.Snapshot.WorldBits[0],
                _gameplay.Camera.Snapshot.WorldBits[1],
                menu.CurrentMapView,
                _gameplay.Camera.Snapshot.CellPixels[0],
                _gameplay.Camera.Snapshot.CellPixels[1]),
            Center = (x, y) => (_missionSession ?? throw new InvalidOperationException("미니맵 중심의 현재 미션 없음"))
                .RequestMinimapCenter(x, y, _driver.Screen.Width, _driver.Screen.Height),
            Move = (_, _) => throw new InvalidOperationException("미니맵 선택 이동은 R5 입력 연결에서 잇는다"),
            InputMode = () => _gameplay.Selection.Mode.Value,
            Poll = () => Poll(0xffff),
            BaseHandle = (token, code, first, second) => (baseHandle ?? throw new InvalidOperationException("미니맵 실제 기본 입력 미설치"))(token, code, first, second),
            Invalidate = (_, _) => _menuDirty = true,
            Changed = () => _menuDirty = true,
        });
        minimap.Construct();
        minimap.SetPalette(_driver.Screen.Colors());
        _nativeMinimap = minimap;
    }

    /// <summary>미션 진입에 미니맵을 실제 HUD 왼쪽 아래에 부착하고 현재 지형으로 갱신해 그린다.</summary>
    /// <remarks>2026-11-26 1.080에서 R5-4c3용으로 추가했다. 근거: cpppj RawMinimapControls.cpp의 Open 뒤 80×75 부착과 HudDock.cpp의 왼쪽 아래 도킹이다. 처음 부착은 Push 없이 Attach하며(Construct 직후 void), 이후 크기 변경은 Resize(Push→부착)를 쓴다. 선택 이동은 R6에서 잇는다. 시작 덤프에는 닿지 않는다.</remarks>
    private void EnsureMissionMinimap()
    {
        var minimap = _nativeMinimap ?? throw new InvalidOperationException("미션 미니맵의 실제 공장 없음");
        minimap.SetPalette(_driver.Screen.Colors());
        var node = minimap.Node(minimap.RootToken) ?? throw new InvalidOperationException("미션 미니맵 실제 노드 없음");
        if (node.Parent == 0)
        {
            node.BaseWidth = node.Width = 80;
            node.BaseHeight = node.Height = 75;
            minimap.Attach(minimap.RootToken, 0, _driver.Screen.Height - 75, _mainMenu?.Tree.Create.Entries[0] ?? throw new InvalidOperationException("미니맵 부착의 실제 루트 없음"), 0);
        }
        else
        {
            minimap.Resize(_driver.Screen.Height);
        }

        minimap.Refresh();
        _menuDirty = true;
    }
}
