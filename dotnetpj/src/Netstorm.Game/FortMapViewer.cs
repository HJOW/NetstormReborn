using System.Diagnostics;
using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>저장된 맵 오브젝트와 영역 청크를 원본 배율로 살펴보는 개발용 뷰어.</summary>
internal sealed partial class FortMapViewer : IDisposable
{
    /// <summary>초당 카메라 이동 픽셀 수.</summary>
    private const float PanSpeed = 600f;
    /// <summary>오브젝트가 표시되는 기본 확대 배율.</summary>
    private const float DefaultZoom = 1f;
    /// <summary>화면 밖 컬링에서 작은 지면 조각(타일·절벽)에 주는 여유(논리 픽셀, 확대 배율 전). 가장 큰 조각의 기준점 오프셋보다 크게 잡는다.</summary>
    private const float TileCullMargin = 96f;
    /// <summary>화면 밖 컬링에서 건물·증기 등 큰 오브젝트에 주는 여유(논리 픽셀, 확대 배율 전).</summary>
    private const float ObjectCullMargin = 400f;
    /// <summary>월드의 한 변 청크 수 (월드 = 16×16 청크, docs/formats/fort.md).</summary>
    private const int WorldChunks = 16;
    /// <summary>월드 전체의 원본 픽셀 크기. 화면 끝 스크롤이 카메라 중심을 이 범위 안에 가둔다.</summary>
    private static readonly Vector2 WorldPixelSize = new(
        WorldChunks * FortMap.CellsPerChunk * FortMap.CellPixelWidth,
        WorldChunks * FortMap.CellsPerChunk * FortMap.CellPixelHeight);
    /// <summary>상단 안내 영역 높이 (개발용 안내 4줄). 공개 캠페인은 원본처럼 상단 막대 없이 지도를 화면 전체에 그린다.</summary>
    private int HeaderHeight => _playUi ? 0 : 128;
    /// <summary>
    /// 원본 미션 시작 화면에서 플레이어 1 사제의 칸 기준점이 화면 중심(512, 384)보다 오른쪽·아래로 떨어진 거리.
    /// 원본은 화면 왼쪽 위를 칸 경계에 맞춰 사제 칸에서 33칸 왼쪽·36칸 위에 두므로 칸 기준점이 (528, 396)에 온다.
    /// 2026-10-03 TEST01 녹화에서 건물 6종의 위치로 맞춘 값이다. 사제 그림은 hotFootRatio (0.5, 0.2) 때문에
    /// 여기서 (8, 2) 왼쪽·위인 (520, 394)에 그려지며, 이전 캡처 3장의 측정값 (525, 393) ±4px 와도 맞는다.
    /// </summary>
    private static readonly Vector2 OriginalStartOffset = new(16, 12);
    /// <summary>
    /// 소유자 번호 → 색 번호(1 파랑, 2 빨강, 3 흰색, 4 초록, 5 보라, 6 노랑, 7 연파랑, 8 주황). 원본처럼 기본은 자기 번호이고
    /// 미션 머리 값 aiNColor 가 덮어쓴다 (<see cref="PlayerColors"/>). 섬 테두리·받침·미니맵·선택 괄호 색을 고른다.
    /// </summary>
    private readonly IReadOnlyDictionary<int, int> _playerColors;
    private readonly FortMap _map;
    private readonly ShapeDatabase _shapes;
    private readonly Palette _palette;
    /// <summary>타입별 지면·유닛·건물의 소유자 색 변환표.</summary>
    private readonly ObjectColorRemap _objectColors;
    private readonly GraphicsDevice _device;
    private readonly Texture2D _pixel;
    /// <summary>메인 메뉴와 같은 원본 UI 질감·장식을 쓰는 공유 그리기 도구.</summary>
    private readonly OriginalUiSkin _uiSkin;
    /// <summary>타입마다 색 변환 대상이 다르므로 타입·프레임 위치·소유자색을 함께 캐시한다.</summary>
    private readonly Dictionary<(int Type, int Frame, int Color), (Texture2D Texture, Point Offset)> _textures = [];
    private readonly FortMapObject[] _sorted;
    private FortTerrainPreview _terrain;
    private readonly TypeInfo _terrainType;
    private readonly TypeInfo _edgeFarmType;
    private readonly TypeInfo _fringeType;
    private readonly TypeInfo _supportTopType;

    /// <summary>다 쓴 가이저를 그릴 원본 emptyGeyser 타입 (없으면 원래 가이저 그림을 유지).</summary>
    private readonly TypeInfo? _emptyGeyserType;
    private readonly TypeInfo _supportBottomType;
    private IReadOnlyList<FortTerrainFringeSprite> _fringes;
    private IReadOnlyList<FortEdgeFarmTile> _edgeFarms;
    private readonly HashSet<(int X, int Y)> _edgeFarmCells;
    private bool _showChunks;
    private Vector2 _camera;
    private float _zoom = DefaultZoom;
    private KeyboardState _previousKeyboard;
    private MouseState _previousMouse;

    /// <summary>미션으로 연 뷰어의 시작 조건 (맵 시험 모드면 null).</summary>
    public MissionStart? Mission => _mission;

    /// <summary>로드한 맵 파일의 표시 이름.</summary>
    public string Name { get; }

    /// <summary>미션으로 연 뷰어인지. 맵 시험 화면에는 미션 메뉴를 표시하지 않는다.</summary>
    public bool IsMissionMode => _mission != null;

    /// <summary>실행 설정에서 선택한 언어. 개발용 안내 문구의 번역 여부와는 별개다.</summary>
    private string Language { get; }

    /// <summary>오브젝트 위치를 계산하고 게임 세션을 만들며 카메라를 플레이어 사제에 맞춘다.</summary>
    /// <param name="mission">미션 시작 조건 (없으면 맵만 보는 시험 모드)</param>
    public FortMapViewer(GraphicsDevice device, ShapeDatabase shapes, Palette palette, FortFile fort, string name,
        TypeCatalog catalog, string language, OriginalUiSkin uiSkin, MissionStart? mission = null,
        MissionScript? tutorialScript = null, ConfigStore? tutorialSettings = null, HelpTopics? help = null,
        MissionScript? commonScript = null)
    {
        _device = device;
        _uiSkin = uiSkin;
        _shapes = shapes;
        _palette = palette;
        _objectColors = new ObjectColorRemap(palette);
        _playerColors = PlayerColors.Table(mission?.AiColors);
        _map = new FortMap(fort);
        _terrainType = catalog.Find("isle") ?? throw new InvalidDataException("isle 타입이 없습니다.");
        _edgeFarmType = catalog.Find("edgeFarm") ?? throw new InvalidDataException("edgeFarm 타입이 없습니다.");
        _fringeType = catalog.Find("fringe") ?? throw new InvalidDataException("fringe 타입이 없습니다.");
        _supportTopType = catalog.Find("island") ?? throw new InvalidDataException("island 타입이 없습니다.");
        _emptyGeyserType = catalog.Find("emptyGeyser");
        _supportBottomType = catalog.Find("islandStalag") ?? throw new InvalidDataException("islandStalag 타입이 없습니다.");
        _terrain = new FortTerrainPreview(_map, _terrainType.Definition);
        _fringes = FortTerrainFringe.Create(_terrain, _terrainType.Definition, _fringeType.Definition);
        _edgeFarms = FortEdgeFarmPreview.Create(_map, _terrain, _terrainType.Definition, _edgeFarmType.Definition);
        _edgeFarmCells = _edgeFarms.Select(tile => (tile.X, tile.Y)).ToHashSet();
        Name = name;
        Language = language;
        _sorted = _map.Objects.OrderBy(o => o.Object.Type.Definition.HasFlag("surface") ? 0 : 1)
            .ThenBy(o => o.Y).ThenBy(o => o.X).ToArray();
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
        InitializeSession(fort, catalog, mission);
        InitializeKnowledge(fort, catalog, help);
        InitializeTutorialDialog(tutorialScript, tutorialSettings, commonScript);
        InitializePlacement(catalog);
        InitializeBridges(catalog);
        CenterOnPriest();
        // 첫 그리기에서 신전 테마로 지면을 다시 만드는 일(약 80ms)과 그 그림의 텍스처 만들기를 미션 로딩 중에 끝낸다.
        RefreshTerritoryAppearance();
        WarmTextures();
    }

    /// <summary>
    /// 원본 미션 시작 화면처럼 플레이어 1 사제를 창 중심에서 OriginalStartOffset 만큼 떨어진 곳에 둔다.
    /// 1024×768 창이면 사제 칸 기준점이 원본과 같은 (528, 396)에 놓여 캡처와 바로 겹쳐 볼 수 있다. 사제가 없으면 첫 오브젝트를 사용한다.
    /// </summary>
    private void CenterOnPriest()
    {
        GameEntity? priest = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Priest);
        if (priest != null)
        {
            _camera = WorldPixels(priest.Footprint.AnchorX, priest.Footprint.AnchorY) - OriginalStartOffset + new Vector2(0, HeaderHeight / 2f);
            return;
        }
        FortMapObject? focus = _map.Objects.FirstOrDefault(o => o.Object.Type.Name == "priest" && o.Object.Owner == 1)
            ?? _map.Objects.FirstOrDefault();
        // Draw 의 중심점은 안내 영역 때문에 창 중심보다 HeaderHeight/2 아래에 있으므로 그만큼 보정한다.
        _camera = focus == null ? Vector2.Zero
            : WorldPixels(focus.X, focus.Y) - OriginalStartOffset + new Vector2(0, HeaderHeight / 2f);
    }

    /// <summary>키보드·마우스·화면 끝 스크롤로 카메라를 이동하고 확대 배율을 변경한다.</summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    /// <param name="mouse">논리 화면 좌표로 바꾼 마우스 상태</param>
    /// <param name="edgeScroll">가장자리 스크롤이 이번 갱신에서 옮길 논리 픽셀 (양수 = 오른쪽·아래)</param>
    public void Update(double seconds, MouseState mouse, Vector2 edgeScroll, int width, int height,
        KeyboardState? input = null, bool inputBlocked = false)
    {
        if (width < 320 || height < 320) return;
        UpdateEffects(seconds);
        UpdateWalkClock(seconds);
        KeyboardState keyboard = input ?? Keyboard.GetState();
        if (inputBlocked)
        {
            if (!TutorialDialogOpen) UpdateSession(seconds, new KeyboardState(), mouse);
            _previousKeyboard = keyboard; _previousMouse = mouse; return;
        }
        if (_knowledgeOpen)
        {
            UpdateKnowledge(keyboard, mouse, width, height, seconds);
            _previousKeyboard = keyboard;
            _previousMouse = mouse;
            return;
        }
        if (TutorialDialogOpen)
        {
            UpdateTutorialDialog(keyboard, mouse, width, height);
            _previousKeyboard = keyboard;
            _previousMouse = mouse;
            return;
        }
        if (ContextMenuOpen)
        {
            UpdateContextMenu(mouse, width, height);
            UpdateSession(seconds, new KeyboardState(), mouse);
            _previousKeyboard = keyboard; _previousMouse = mouse; return;
        }
        if (UpdateMissionMenu(keyboard, mouse, width, height))
        {
            // 메뉴가 열린 동안에도 원본처럼 미션 시간은 흐르되 지도 조작은 받지 않는다.
            UpdateSession(seconds, new KeyboardState(), mouse);
            _previousKeyboard = keyboard;
            _previousMouse = mouse;
            return;
        }
        if (_tutorialDialog != null && Pressed(keyboard, Keys.F8) && _tutorialDialog.Review())
        {
            ResetTutorialPage();
            _previousKeyboard = keyboard;
            _previousMouse = mouse;
            return;
        }
        if (IsMissionMode && Pressed(keyboard, Keys.F6))
        {
            OpenKnowledge();
            _previousKeyboard = keyboard;
            _previousMouse = mouse;
            return;
        }
        bool uiConsumed = _playUi && UpdatePlayUi(mouse, width, height);
        var direction = new Vector2(
            (keyboard.IsKeyDown(Keys.Right) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Left) ? 1 : 0),
            (keyboard.IsKeyDown(Keys.Down) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Up) ? 1 : 0));
        _camera += direction * PanSpeed * (float)seconds / _zoom;
        if (edgeScroll != Vector2.Zero)
        {
            // 화면 끝 스크롤은 확대 배율과 상관없이 화면 기준 픽셀로 이동하고, 월드(16×16 청크) 밖으로는 나가지 않는다.
            _camera = Vector2.Clamp(_camera + edgeScroll / _zoom, Vector2.Zero, WorldPixelSize);
        }
        if (!_playUi && mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Pressed && !(_placementMode || _bridgeMode))
        {
            _camera -= new Vector2(mouse.X - _previousMouse.X, mouse.Y - _previousMouse.Y) / _zoom;
        }
        int wheel = mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue;
        if (!_playUi && wheel != 0)
        {
            _zoom = Math.Clamp(_zoom * (wheel > 0 ? 1.25f : 0.8f), 0.25f, 4f);
        }
        if (keyboard.IsKeyDown(Keys.Home) && !_previousKeyboard.IsKeyDown(Keys.Home))
        {
            _zoom = DefaultZoom;
            CenterOnPriest();
        }
        bool control = keyboard.IsKeyDown(Keys.LeftControl) || keyboard.IsKeyDown(Keys.RightControl);
        if (Pressed(keyboard, _playUi ? Keys.F5 : Keys.F4) && (!_playUi || !control))
        {
            CenterOnPriest();
            if (!_playUi) _session.Submit(new ReturnHomeCommand(1));
        }
        if (_playUi) UpdateOriginalControls(seconds, keyboard, mouse);
        if (keyboard.IsKeyDown(Keys.G) && !_previousKeyboard.IsKeyDown(Keys.G))
        {
            _showChunks = !_showChunks;
        }
        if (_playUi && !uiConsumed) UpdatePlayOrders(mouse);
        UpdateSession(seconds, keyboard, mouse);
        if (!TutorialDialogOpen)
        {
            if (!uiConsumed)
            {
                UpdatePlacement(keyboard, mouse);
            }
            // 다리 키(Q W A S Z X·E·C·Backspace)는 커서가 생산 창 위에 있어도 받고, 지도 클릭·커서 칸만 막는다
            UpdateBridges(keyboard, mouse, mapInput: !uiConsumed);
        }
        _previousKeyboard = keyboard;
        _previousMouse = mouse;
    }

    /// <summary>영역 청크 윤곽과 정적 스프라이트를 그리고 마우스 가까운 오브젝트를 설명한다.</summary>
    /// <param name="batch">스프라이트 배치</param>
    /// <param name="font">안내 글꼴</param>
    /// <param name="width">논리 화면 폭</param>
    /// <param name="height">논리 화면 높이</param>
    /// <param name="displayInfo">화면 배치 설명 문구 (안내 4번째 줄)</param>
    /// <param name="small">작은 글꼴 (지식 창 카드 이름·수치)</param>
    public void Draw(SpriteBatch batch, SpriteFontBase font, int width, int height, string displayInfo, SpriteFontBase small)
    {
        var center = new Vector2(width / 2f, (height + HeaderHeight) / 2f);
        _viewSize = new Point(width, height);
        if (_sky != null)
        {
            // 원본의 회보라 구름 배경을 지도 전체에 반복한다.
            for (int y = 0; y < height; y += _sky.Height)
            {
                // 가로 방향도 같은 크기로 반복해 와이드 화면을 채운다.
                for (int x = 0; x < width; x += _sky.Width)
                    batch.Draw(_sky, new Vector2(x, y), Color.White);
            }
        }
        // 진단용 청크 윤곽은 지면 아래에 선택적으로 표시한다.
        if (_showChunks)
        {
            // 활성 영역의 모든 청크 윤곽을 그린다.
            foreach (FortTerritory territory in _map.Territories.Where(t => t.IsActive))
            {
                // 원본 모양에서 생성되는 각 청크의 월드 범위를 그린다.
                foreach ((int x, int y) in FortMap.TerritoryChunks(territory))
                {
                    Vector2 topLeft = Screen(WorldPixels(x * FortMap.CellsPerChunk, y * FortMap.CellsPerChunk), center);
                    int w = (int)(FortMap.CellsPerChunk * FortMap.CellPixelWidth * _zoom);
                    int h = (int)(FortMap.CellsPerChunk * FortMap.CellPixelHeight * _zoom);
                    var rect = new Rectangle((int)topLeft.X, (int)topLeft.Y, w, h);
                    batch.Draw(_pixel, rect, new Color(32, 56, 70));
                    Outline(batch, rect, new Color(62, 92, 110));
                }
            }
        }
        // 화면 밖 그림은 건너뛴다. 월드 전체의 지면·오브젝트를 매 프레임 그리면 보이지 않는 수천 개의 그리기 호출이 프레임을 깎는다.
        // 여유는 그림이 기준점에서 뻗는 최대 크기(작은 지면 조각 96px, 건물·증기 등 큰 오브젝트 400px)를 확대 배율에 맞춰 잡는다.
        float tileMargin = TileCullMargin * _zoom;
        float objectMargin = ObjectCullMargin * _zoom;
        bool Visible(Vector2 point, float margin) =>
            point.X > -margin && point.X < width + margin && point.Y > -margin && point.Y < height + margin;
        long perfTerrain = Stopwatch.GetTimestamp();
        // 시드로 생성한 지면을 원본 isle 타일로 그린다.
        RefreshTerritoryAppearance();
        foreach (FortTerrainTile tile in _terrain.Tiles)
        {
            Vector2 tilePosition = Screen(WorldPixels(tile.X, tile.Y), center);
            if (!Visible(tilePosition, tileMargin)) continue;
            // 건물 소멸로 사라진 개발용 받침 타일은 현재 지지 판정에 맞춰 숨긴다.
            if (tile.Region < 0 && !_session.Bridges.IsIsland(tile.X, tile.Y)) continue;
            if (_edgeFarmCells.Contains((tile.X, tile.Y)))
            {
                continue;
            }
            int color = _islandColors ? _playerColors.GetValueOrDefault(tile.Owner) : 0;
            DrawSprite(batch, _terrainType.LoadIndex, MapSpriteFrames.BodyFrame(_terrainType.Definition, tile.Cluster),
                tilePosition, color);
        }
        // 절벽은 별도 기준점을 사용하며 본체 지면 위·건물 아래에 표시한다. 원본의 깊이 정렬은 추가 검증 대상이다.
        foreach (FortTerrainFringeSprite fringe in _fringes)
        {
            Vector2 fringePosition = Screen(WorldPixels(fringe.X, fringe.Y), center);
            if (!Visible(fringePosition, tileMargin)) continue;
            // 절벽의 원본 지면 기준점은 표시 기준점보다 OffsetY만큼 위에 있다.
            if (!_session.Bridges.IsIsland(fringe.X, fringe.Y - FortTerrainFringe.OffsetY)) continue;
            DrawSprite(batch, _fringeType.LoadIndex, MapSpriteFrames.BodyFrame(_fringeType.Definition, fringe.Cluster), fringePosition);
        }
        // edgeFarm은 원본의 matchframe으로 해당 isle을 대체한다. 원본과 같은 소유자 색상표를 적용한다.
        foreach (FortEdgeFarmTile tile in _edgeFarms)
        {
            Vector2 farmPosition = Screen(WorldPixels(tile.X, tile.Y), center);
            if (!Visible(farmPosition, tileMargin)) continue;
            int color = _islandColors ? _playerColors.GetValueOrDefault(tile.Owner) : 0;
            DrawSprite(batch, _edgeFarmType.LoadIndex, MapSpriteFrames.BodyFrame(_edgeFarmType.Definition, tile.Cluster),
                farmPosition, color);
        }
        // 확인된 3×3 받침은 같은 기준점에서 전용 하단 바위와 윗면을 그린다.
        foreach (FortIslandSupport support in _terrain.Supports)
        {
            Vector2 anchor = Screen(WorldPixels(support.X, support.Y), center);
            if (!Visible(anchor, objectMargin)) continue;
            // 저장된 받침도 건물 회수·파괴로 지지를 잃으면 화면에서 함께 사라진다.
            if (!_session.Bridges.IsIsland(support.X, support.Y)) continue;
            int cluster = FortIslandSupports.ColorCluster(support.Owner, _playerColors);
            // 받침의 주황 테두리도 본섬처럼 소유자 색으로 바뀐다 (TEST01 녹화: 내 탑의 받침 테두리가 파랑).
            int color = _islandColors ? _playerColors.GetValueOrDefault(support.Owner) : 0;
            DrawSprite(batch, _supportBottomType.LoadIndex, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, cluster), anchor, color);
            DrawSprite(batch, _supportTopType.LoadIndex, MapSpriteFrames.BodyFrame(_supportTopType.Definition, cluster), anchor, color);
        }
        // 세션에서 새로 배치한 건물형 유닛의 받침도 저장된 받침과 같은 원본 그림으로 표시한다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Source == null && e.IsComplete &&
                     e.Type.Definition.HasFlag("createsisland") && _session.Map.TerritoryAt(e.Footprint.AnchorX, e.Footprint.AnchorY) == null))
        {
            if (_terrain.Supports.Any(s => s.X == entity.Footprint.AnchorX && s.Y == entity.Footprint.AnchorY)) continue;
            int cluster = FortIslandSupports.ColorCluster(entity.Owner, _playerColors);
            int color = _islandColors ? _playerColors.GetValueOrDefault(entity.Owner) : 0;
            Vector2 anchor = Screen(WorldPixels(entity.Footprint.AnchorX, entity.Footprint.AnchorY), center);
            DrawSprite(batch, _supportBottomType.LoadIndex, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, cluster), anchor, color);
            DrawSprite(batch, _supportTopType.LoadIndex, MapSpriteFrames.BodyFrame(_supportTopType.Definition, cluster), anchor, color);
        }
        PerfMeter.Current?.Section("terrain", perfTerrain);
        long perfObjects = Stopwatch.GetTimestamp();
        DrawSunForceFields(batch, center);
        FortMapObject? hovered = null;
        float nearest = 20f * 20f;
        var mousePosition = new Vector2(_previousMouse.X, _previousMouse.Y);
        // 건물은 발자국 크기를 추가 보정하지 않고 프레임 xmin/ymin을 기준점에 더한다.
        foreach (FortMapObject item in _sorted)
        {
            // noIsland는 투명한 논리 지면이다. 미리보기 지면에 반영했으므로 표식을 그리지 않는다.
            // 무너진 저장 다리와 회수되어 사라진 저장 오브젝트는 그리지 않는다.
            if (item.Object.Type.Name == "noIsland" || ObjectKinds.Of(item.Object.Type) == ObjectKind.Flyer || IsCrumbledStoredBridge(item) || IsRemovedInitialObject(item))
            {
                continue;
            }
            GameEntity? live = _session.EntityForInitial(item);
            if (HiddenByBuildingToggle(item.Object.Type)) continue;
            // 사제뿐 아니라 저장된 수송 유닛도 현재 이동 좌표로 그린다. 걷는 유닛은 칸 사이 보간 위치와 걷는 방향 그림을 쓴다.
            bool mobile = live != null && IsMobile(live);
            int itemX = live?.Footprint.AnchorX ?? item.X;
            int itemY = live?.Footprint.AnchorY ?? item.Y;
            Vector2 anchor = Screen(mobile ? MobileWorldPixels(live!) : WorldPixels(itemX, itemY), center);
            if (!Visible(anchor, objectMargin)) continue;
            // 가이저 증기·워크샵 레벨·신전 회오리·풍선·걷기 그림을 고르고 그림자 → 본체 → 겹침 순서로 그린다.
            (TypeInfo drawn, StructureFrames frames, Vector2 shift) = ObjectSprite(item.Object.Type, live, item);
            if (!DrawObjectSprite(batch, drawn, frames, anchor + shift * _zoom,
                color: _playerColors.GetValueOrDefault(live?.Owner ?? item.Object.Owner ?? 0)))
            {
                // 이미지가 없는 특수 프레임은 소유자색 표식으로 위치만 표시한다.
                batch.Draw(_pixel, new Rectangle((int)anchor.X - 3, (int)anchor.Y - 3, 6, 6),
                    item.Object.Owner == 2 ? Color.OrangeRed : Color.Turquoise);
            }
            float distance = Vector2.DistanceSquared(anchor, mousePosition);
            if (distance < nearest)
            {
                nearest = distance;
                hovered = item;
            }
        }
        PerfMeter.Current?.Section("objects", perfObjects);
        long perfWorld = Stopwatch.GetTimestamp();
        DrawPlacedUnits(batch, center);
        DrawProductionDeliveries(batch, center);
        DrawBridgeWorld(batch, center);
        DrawFalling(batch, center);
        DrawFlyers(batch, center);
        DrawCombat(batch, font, center);
        DrawSacrificeStatus(batch, font, center);
        PerfMeter.Current?.Section("world", perfWorld);
        long perfUi = Stopwatch.GetTimestamp();
        if (!_playUi)
        {
            batch.Draw(_pixel, new Rectangle(0, 0, width, HeaderHeight), new Color(18, 24, 38));
            batch.DrawString(font, $"맵: {Name} | 오브젝트 {_map.Objects.Count}개 | 확대 {_zoom:0.##}배 | 언어: {Language}", new Vector2(16, 10), Color.Gold);
            batch.DrawString(font, $"방향키 / 우클릭 / 화면 끝: 이동 · 휠: 확대 · Home: 사제 위치 · G: 청크 윤곽 · P: 배치 시험 · B: 다리 조각 · Esc: {(IsMissionMode ? "메뉴" : "종료")}", new Vector2(16, 38), Color.White);
            batch.DrawString(font, "F4: 사제 섬으로 · F11: 전체화면 · F10: 와이드 처리 · F9: 원본 해상도 높이 · F7: 가장자리 스크롤 · T: 선택→대상 클릭 · D+클릭: 내려놓기", new Vector2(16, 66), Color.White);
            batch.DrawString(font, displayInfo, new Vector2(16, 94), Color.LightGray);
            DrawSessionHud(batch, font, width);
        }
        else DrawPlayUi(batch, font, small, width, height);
        DrawPlacementOverlay(batch, font, center, width, height);
        DrawBridgeOverlay(batch, font, width, height);
        if (!_playUi && hovered != null && !_placementMode && !_bridgeMode)
        {
            string region = hovered.Territory.HasValue ? $"Terr{hovered.Territory:00}" : "Chaff";
            string text = $"{hovered.Object.Type.Name} ({hovered.X}, {hovered.Y}) | {region} | 소유자 {hovered.Object.Owner} | 다리 {hovered.Object.BridgeShape}";
            batch.Draw(_pixel, new Rectangle(0, height - 34, width, 34), new Color(18, 24, 38));
            batch.DrawString(font, text, new Vector2(16, height - 30), Color.White);
        }
        PerfMeter.Current?.Section("ui", perfUi);
        DrawTutorialDialog(batch, font, width, height);
        DrawKnowledge(batch, font, small, width, height);
        DrawContextMenu(batch, width, height);
        DrawMissionMenu(batch, font, width, height);
    }

    /// <summary>지면 타일을 기준점과 프레임 오프셋에 맞춰 그린다.</summary>
    /// <param name="scale">배율 (없으면 현재 확대 배율)</param>
    /// <param name="alpha">불투명도 (0~1, 들고 있는 다리 조각 미리보기용)</param>
    /// <param name="tint">곱할 색 (놓을 수 없는 조각의 빨강 표시용, 없으면 흰색)</param>
    private void DrawSprite(SpriteBatch batch, int typeIndex, int frameIndex, Vector2 anchor, int color = 0,
        float? scale = null, float alpha = 1f, Color? tint = null)
    {
        var sprite = GetTexture(typeIndex, frameIndex, color);
        if (sprite.HasValue)
        {
            var (texture, offset) = sprite.Value;
            float s = scale ?? _zoom;
            batch.Draw(texture, anchor + offset.ToVector2() * s, null, (tint ?? Color.White) * alpha,
                0f, Vector2.Zero, s, SpriteEffects.None, 0f);
        }
    }

    /// <summary>본체 프레임을 텍스처로 변환하고 캐시한다.</summary>
    private (Texture2D Texture, Point Offset)? GetTexture(int typeIndex, int index, int color = 0)
    {
        ShapeBlock block = _shapes.Blocks[typeIndex];
        if (index >= block.Frames.Count || block.Frames[index].IsSpecial)
        {
            return null;
        }
        ShapeFrame frame = block.Frames[index];
        var key = (typeIndex, frame.Offset, color);
        if (!_textures.TryGetValue(key, out var sprite))
        {
            // 0 = 원래 색, 음수 = 생산 창 어둡게·빨갛게 표, 양수 = 타입별 소유자 색.
            ReadOnlyMemory<byte> table = color == 0 ? default : DeckRemap(color) is { } deck ? deck : _objectColors.Table(typeIndex, color);
            sprite = (SpriteAnimation.ToTexture(_device, _shapes.Decode(frame), _palette, table), new Point(frame.XMin, frame.YMin));
            _textures.Add(key, sprite);
        }
        return sprite;
    }

    /// <summary>칸 좌표를 카메라 보정 전의 원본 화면 픽셀 좌표로 옮긴다.</summary>
    private static Vector2 WorldPixels(int x, int y) => new(x * FortMap.CellPixelWidth, y * FortMap.CellPixelHeight);

    /// <summary>월드 픽셀 좌표를 카메라 중심·확대 배율에 따라 화면 좌표로 옮긴다.</summary>
    private Vector2 Screen(Vector2 position, Vector2 center) => (position - _camera) * _zoom + center;

    /// <summary>청크 사각형의 네 변을 1픽셀 선으로 그린다.</summary>
    private void Outline(SpriteBatch batch, Rectangle rect, Color color)
    {
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Y, rect.Width, 1), color);
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Bottom - 1, rect.Width, 1), color);
        batch.Draw(_pixel, new Rectangle(rect.X, rect.Y, 1, rect.Height), color);
        batch.Draw(_pixel, new Rectangle(rect.Right - 1, rect.Y, 1, rect.Height), color);
    }

    /// <summary>맵 뷰어가 만든 모든 GPU 텍스처를 해제한다.</summary>
    public void Dispose()
    {
        // 캐시된 텍스처를 각각 한 번씩 해제한다.
        foreach (var sprite in _textures.Values)
        {
            sprite.Texture.Dispose();
        }
        // 그림자 텍스처도 함께 해제한다.
        foreach (var shadow in _shadowTextures.Values)
        {
            shadow?.Texture.Dispose();
        }
        _pixel.Dispose();
        _miniMapTexture?.Dispose();
        _sky?.Dispose();
    }
}
