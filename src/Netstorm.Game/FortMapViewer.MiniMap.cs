using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 원본 방식의 미니맵: 검은 상자에 섬과 다리를 소유자 색으로 칠하고, 흰 사각형으로 지금 보이는 범위를 표시한다.
/// 좌표 규칙은 <see cref="MiniMapLayout"/> (1픽셀 = 2칸, 화면 중심을 따라 움직이는 창)이다.
/// 근거: 2026-10-03 TEST01 녹화 37.5·275.4·345.0초 프레임과 1-1 시작 캡처(docs/screens/the-war-begins-start.md 2.3절).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>미니맵 상자의 논리 화면 폭. 녹화(125% 배율)의 검은 상자 x 1~98 → 논리 약 1~78.</summary>
    private const int MiniMapWidth = 78;

    /// <summary>미니맵 상자의 논리 화면 높이. 녹화의 검은 상자 y 866~958 → 논리 약 693~767.</summary>
    private const int MiniMapHeight = 74;

    /// <summary>소유자가 없는 섬(신전 없는 섬·중립 받침)의 미니맵 색. 녹화 표본 (64,86,40) → 팔레트 140번 (63,88,40).</summary>
    private static readonly Color MiniMapNeutral = new(63, 88, 40);

    /// <summary>가이저를 나타내는 노란 점의 색 (1-1 시작 캡처의 2×2 노란 점). 정확한 팔레트 번호는 확인하지 못해 순색을 쓴다.</summary>
    private static readonly Color MiniMapGeyser = new(255, 255, 0);

    /// <summary>현재 화면 범위를 나타내는 사각형 테두리 색 (녹화의 흰 1픽셀 선).</summary>
    private static readonly Color MiniMapViewRect = Color.White;

    /// <summary>
    /// 색 번호(0~8)별 미니맵 색. 파랑(1)=(155,210,206)·빨강(2)=(255,0,0)·흰색(3)=(255,255,255)은 TEST01 녹화에서 잰 값이고
    /// (팔레트 88·254·195번), 초록·보라·노랑·연파랑·주황은 같은 방식의 순색으로 둔 추정값이다.
    /// </summary>
    private static readonly Color[] MiniMapPlayerColors =
    [
        MiniMapNeutral, new(155, 210, 206), new(255, 0, 0), new(255, 255, 255), new(0, 255, 0),
        new(255, 0, 255), new(255, 255, 0), new(0, 255, 255), new(255, 128, 0),
    ];

    /// <summary>월드 전체를 2칸 = 1픽셀로 그려 둔 미니맵 그림 (128×128). 섬 소유·다리·오브젝트 수가 바뀔 때만 다시 만든다.</summary>
    private Texture2D? _miniMapTexture;

    /// <summary>직전에 미니맵 그림을 만들 때의 상태 서명.</summary>
    private string? _miniMapSignature;

    /// <summary>미니맵 안에서 왼쪽 버튼을 누른 뒤 아직 떼지 않았는지 (누른 채 끌면 화면이 계속 따라온다).</summary>
    private bool _miniMapDragging;

    /// <summary>원본처럼 사이드바 맨 아래의 검은 상자 (1024×768 에서 x 1~78, y 693~766).</summary>
    private static Rectangle MiniMap(int width, int height) => new(1, height - MiniMapHeight - 1, MiniMapWidth, MiniMapHeight);

    /// <summary>지도가 보이는 영역(사이드바 오른쪽)의 화면 중심을 월드 칸 좌표로 구한다.</summary>
    private Vector2 ViewCenterCells(int width, int height)
    {
        Vector2 world = _camera + MapAreaOffset(width, height);
        return new Vector2(world.X / FortMap.CellPixelWidth, world.Y / FortMap.CellPixelHeight);
    }

    /// <summary>카메라 기준점에서 지도 영역 중심까지의 월드 픽셀 거리 (사이드바 때문에 창 중심과 다르다).</summary>
    private Vector2 MapAreaOffset(int width, int height)
    {
        var center = new Vector2(width / 2f, (height + HeaderHeight) / 2f);
        return (new Vector2((PlaySidebarWidth + width) / 2f, height / 2f) - center) / _zoom;
    }

    /// <summary>미니맵 창의 왼쪽 위 월드 칸 (화면 중심을 가운데 두고 월드 끝에서 멈춘다).</summary>
    private Vector2 MiniMapOrigin(int width, int height)
    {
        Vector2 view = ViewCenterCells(width, height);
        return new Vector2((float)MiniMapLayout.Origin(view.X, MiniMapWidth, BridgeGrid.WorldSize),
            (float)MiniMapLayout.Origin(view.Y, MiniMapHeight, BridgeGrid.WorldSize));
    }

    /// <summary>
    /// 미니맵 누르기·끌기: 누른 지점의 월드 칸이 화면 중심이 되게 카메라를 옮긴다. 누르고 있는 동안 매 갱신마다 되풀이하므로
    /// 커서가 상자 중심에서 벗어나 있으면 화면이 그 방향으로 계속 흘러간다(녹화에서 0.2초마다 전혀 다른 곳이 보였다).
    /// 커서가 상자 밖으로 나가도 상자 끝 값으로 계속 따라간다.
    /// </summary>
    /// <param name="clicked">이번 갱신에 왼쪽 버튼을 새로 눌렀는지</param>
    /// <returns>미니맵이 입력을 썼으면 true</returns>
    private bool UpdateMiniMap(MouseState mouse, int width, int height, bool clicked)
    {
        if (mouse.LeftButton != ButtonState.Pressed)
        {
            _miniMapDragging = false;
            return false;
        }
        Rectangle mini = MiniMap(width, height);
        if (clicked && mini.Contains(mouse.X, mouse.Y)) _miniMapDragging = true;
        if (!_miniMapDragging) return false;
        Vector2 origin = MiniMapOrigin(width, height);
        var cell = new Vector2((float)MiniMapLayout.ToCell(mouse.X - mini.X, origin.X, mini.Width),
            (float)MiniMapLayout.ToCell(mouse.Y - mini.Y, origin.Y, mini.Height));
        _camera = Vector2.Clamp(new Vector2(cell.X * FortMap.CellPixelWidth, cell.Y * FortMap.CellPixelHeight)
            - MapAreaOffset(width, height), Vector2.Zero, WorldPixelSize);
        return true;
    }

    /// <summary>색 번호 표로 소유자의 미니맵 색을 고른다 (소유자 0·표에 없는 번호는 중립색).</summary>
    private Color MiniMapColor(int owner)
    {
        int color = _playerColors.GetValueOrDefault(owner);
        return color > 0 && color < MiniMapPlayerColors.Length ? MiniMapPlayerColors[color] : MiniMapNeutral;
    }

    /// <summary>
    /// 월드 전체 미니맵 그림을 필요할 때만 다시 만든다. 한 픽셀(2×2칸)에 소유자가 있는 땅이 하나라도 있으면 그 색,
    /// 중립 땅만 있으면 중립색이다. 다리는 놓은 플레이어의 색으로 덮고, 가이저는 소유자 색이 없는 자리에만 노란 점으로 찍는다
    /// (1-1 캡처에서 적 섬 위 가이저는 빨간색에 가려 보이지 않았다).
    /// </summary>
    private void RefreshMiniMapTexture()
    {
        int geysers = _session.Entities.Count(e => e.Kind == ObjectKind.Geyser && !e.IsDepletedGeyser);
        string signature = $"{_terrainTempleSignature}|{_session.Bridges.Version}|{_session.Entities.Count}|{geysers}";
        if (_miniMapTexture != null && signature == _miniMapSignature) return;
        _miniMapSignature = signature;
        int size = BridgeGrid.WorldSize / MiniMapLayout.CellsPerPixel;
        var pixels = new Color[size * size];
        var owned = new bool[size * size];
        // 칸 → 땅 소유자: 본섬·불완전 받침 타일, 저장된 3×3 받침, 게임 중 유닛이 만든 받침 순서로 모은다.
        var owners = new Dictionary<(int X, int Y), int>();
        foreach (FortTerrainTile tile in _terrain.Tiles) owners[(tile.X, tile.Y)] = tile.Owner;
        foreach (FortIslandSupport support in _terrain.Supports) FillSupport(owners, support.X, support.Y, support.Owner);
        foreach (GameEntity entity in _session.Entities.Where(e => e.Source == null && e.Type.Definition.HasFlag("createsisland")))
            FillSupport(owners, entity.Footprint.AnchorX, entity.Footprint.AnchorY, entity.Owner);
        // 월드의 모든 칸을 훑어 땅을 칠한다
        for (int y = 0; y < BridgeGrid.WorldSize; y++)
        {
            // 같은 행의 칸을 왼쪽에서 오른쪽으로 본다
            for (int x = 0; x < BridgeGrid.WorldSize; x++)
            {
                if (!_session.Bridges.IsIsland(x, y)) continue;
                int owner = owners.GetValueOrDefault((x, y));
                int index = y / MiniMapLayout.CellsPerPixel * size + x / MiniMapLayout.CellsPerPixel;
                if (owned[index]) continue;
                pixels[index] = MiniMapColor(owner);
                owned[index] = _playerColors.GetValueOrDefault(owner) > 0;
            }
        }
        // 다리 칸은 놓은 플레이어의 색으로 덮는다
        foreach (BridgeCellState cell in _session.Bridges.Cells)
        {
            if ((uint)cell.X >= BridgeGrid.WorldSize || (uint)cell.Y >= BridgeGrid.WorldSize) continue;
            int index = cell.Y / MiniMapLayout.CellsPerPixel * size + cell.X / MiniMapLayout.CellsPerPixel;
            if (owned[index]) continue;
            pixels[index] = MiniMapColor(cell.Owner);
            owned[index] = _playerColors.GetValueOrDefault(cell.Owner) > 0;
        }
        // 남은 가이저를 2×2 노란 점으로 찍는다 (소유자 색 위에는 찍지 않는다)
        foreach (GameEntity geyser in _session.Entities.Where(e => e.Kind == ObjectKind.Geyser && !e.IsDepletedGeyser))
        {
            int px = geyser.Footprint.AnchorX / MiniMapLayout.CellsPerPixel;
            int py = geyser.Footprint.AnchorY / MiniMapLayout.CellsPerPixel;
            // 점의 네 픽셀을 차례로 칠한다
            for (int i = 0; i < 4; i++)
            {
                int x = px - i % 2, y = py - i / 2;
                if ((uint)x >= size || (uint)y >= size || owned[y * size + x]) continue;
                pixels[y * size + x] = MiniMapGeyser;
            }
        }
        _miniMapTexture ??= new Texture2D(_device, size, size);
        _miniMapTexture.SetData(pixels);
    }

    /// <summary>오른쪽 아래 칸이 (x, y)인 3×3 받침의 아홉 칸에 소유자를 적는다.</summary>
    private static void FillSupport(Dictionary<(int X, int Y), int> owners, int x, int y, int owner)
    {
        // 받침의 3×3 칸을 차례로 채운다
        for (int i = 0; i < FortIslandSupports.Size * FortIslandSupports.Size; i++)
            owners[(x - i % FortIslandSupports.Size, y - i / FortIslandSupports.Size)] = owner;
    }

    /// <summary>검은 상자, 소유자 색 섬·다리, 현재 화면 범위의 흰 사각형을 그린다.</summary>
    private void DrawMiniMap(SpriteBatch batch, int width, int height)
    {
        Rectangle mini = MiniMap(width, height);
        batch.Draw(_pixel, mini, Color.Black);
        RefreshMiniMapTexture();
        Vector2 origin = MiniMapOrigin(width, height);
        var source = new Rectangle((int)MathF.Round(origin.X / MiniMapLayout.CellsPerPixel),
            (int)MathF.Round(origin.Y / MiniMapLayout.CellsPerPixel), mini.Width, mini.Height);
        batch.Draw(_miniMapTexture, mini, source, Color.White);
        // 지도가 보이는 범위(사이드바 오른쪽 ~ 창 오른쪽 끝, 위 ~ 아래)를 칸으로 바꿔 사각형으로 표시한다.
        Vector2 view = ViewCenterCells(width, height);
        float halfWidth = (width - PlaySidebarWidth) / 2f / _zoom / FortMap.CellPixelWidth;
        float halfHeight = height / 2f / _zoom / FortMap.CellPixelHeight;
        int left = mini.X + (int)MathF.Round((float)MiniMapLayout.ToPixel(view.X - halfWidth, origin.X));
        int right = mini.X + (int)MathF.Round((float)MiniMapLayout.ToPixel(view.X + halfWidth, origin.X));
        int top = mini.Y + (int)MathF.Round((float)MiniMapLayout.ToPixel(view.Y - halfHeight, origin.Y));
        int bottom = mini.Y + (int)MathF.Round((float)MiniMapLayout.ToPixel(view.Y + halfHeight, origin.Y));
        Rectangle rect = Rectangle.Intersect(new Rectangle(left, top, right - left, bottom - top), mini);
        if (rect.Width > 1 && rect.Height > 1) Outline(batch, rect, MiniMapViewRect);
    }
}
