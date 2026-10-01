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
    /// <summary>월드의 한 변 청크 수 (월드 = 16×16 청크, docs/formats/fort.md).</summary>
    private const int WorldChunks = 16;
    /// <summary>월드 전체의 원본 픽셀 크기. 화면 끝 스크롤이 카메라 중심을 이 범위 안에 가둔다.</summary>
    private static readonly Vector2 WorldPixelSize = new(
        WorldChunks * FortMap.CellsPerChunk * FortMap.CellPixelWidth,
        WorldChunks * FortMap.CellsPerChunk * FortMap.CellPixelHeight);
    /// <summary>상단 안내 영역 높이 (안내 4줄).</summary>
    private const int HeaderHeight = 128;
    /// <summary>
    /// 원본 미션 시작 화면에서 플레이어 1 사제 칸 기준점이 화면 중심(512, 384)보다 오른쪽·아래로 떨어진 거리.
    /// 원본 캡처 3장(The War Begins!·Save the Island!·Dissolved Alliance!)에서 (525, 393) ±4px 로 측정했다.
    /// </summary>
    private static readonly Vector2 OriginalStartOffset = new(13, 9);
    /// <summary>제공된 공식 미션 캡처의 플레이어 1 청록·플레이어 2 빨강을 사용하는 개발용 색상표.</summary>
    private static readonly IReadOnlyDictionary<int, int> PreviewPlayerColors = new Dictionary<int, int> { [1] = 7, [2] = 2 };
    private readonly FortMap _map;
    private readonly ShapeDatabase _shapes;
    private readonly Palette _palette;
    private readonly IsleColorRemap _isleColors;
    private readonly GraphicsDevice _device;
    private readonly Texture2D _pixel;
    private readonly Dictionary<(int Frame, int Color), (Texture2D Texture, Point Offset)> _textures = [];
    private readonly FortMapObject[] _sorted;
    private FortTerrainPreview _terrain;
    private readonly TypeInfo _terrainType;
    private readonly TypeInfo _edgeFarmType;
    private readonly TypeInfo _fringeType;
    private readonly TypeInfo _supportTopType;
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
        TypeCatalog catalog, string language, MissionStart? mission = null,
        MissionScript? tutorialScript = null, ConfigStore? tutorialSettings = null, HelpTopics? help = null,
        MissionScript? commonScript = null)
    {
        _device = device;
        _shapes = shapes;
        _palette = palette;
        _isleColors = new IsleColorRemap(palette);
        _map = new FortMap(fort);
        _terrainType = catalog.Find("isle") ?? throw new InvalidDataException("isle 타입이 없습니다.");
        _edgeFarmType = catalog.Find("edgeFarm") ?? throw new InvalidDataException("edgeFarm 타입이 없습니다.");
        _fringeType = catalog.Find("fringe") ?? throw new InvalidDataException("fringe 타입이 없습니다.");
        _supportTopType = catalog.Find("island") ?? throw new InvalidDataException("island 타입이 없습니다.");
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
    }

    /// <summary>
    /// 원본 미션 시작 화면처럼 플레이어 1 사제를 창 중심에서 OriginalStartOffset 만큼 떨어진 곳에 둔다.
    /// 1024×768 창이면 사제가 원본 캡처와 같은 (525, 393)에 그려져 캡처와 바로 겹쳐 볼 수 있다. 사제가 없으면 첫 오브젝트를 사용한다.
    /// </summary>
    private void CenterOnPriest()
    {
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
    public void Update(double seconds, MouseState mouse, Vector2 edgeScroll, int width, int height)
    {
        KeyboardState keyboard = Keyboard.GetState();
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
        var direction = new Vector2(
            (keyboard.IsKeyDown(Keys.Right) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Left) ? 1 : 0),
            (keyboard.IsKeyDown(Keys.Down) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Up) ? 1 : 0));
        _camera += direction * PanSpeed * (float)seconds / _zoom;
        if (edgeScroll != Vector2.Zero)
        {
            // 화면 끝 스크롤은 확대 배율과 상관없이 화면 기준 픽셀로 이동하고, 월드(16×16 청크) 밖으로는 나가지 않는다.
            _camera = Vector2.Clamp(_camera + edgeScroll / _zoom, Vector2.Zero, WorldPixelSize);
        }
        if (mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Pressed)
        {
            _camera -= new Vector2(mouse.X - _previousMouse.X, mouse.Y - _previousMouse.Y) / _zoom;
        }
        int wheel = mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue;
        if (wheel != 0)
        {
            _zoom = Math.Clamp(_zoom * (wheel > 0 ? 1.25f : 0.8f), 0.25f, 4f);
        }
        if (keyboard.IsKeyDown(Keys.Home) && !_previousKeyboard.IsKeyDown(Keys.Home))
        {
            _zoom = DefaultZoom;
            CenterOnPriest();
        }
        if (Pressed(keyboard, Keys.F4))
        {
            CenterOnPriest();
            _session.Submit(new ReturnHomeCommand(1));
        }
        if (keyboard.IsKeyDown(Keys.G) && !_previousKeyboard.IsKeyDown(Keys.G))
        {
            _showChunks = !_showChunks;
        }
        UpdateSession(seconds, keyboard, mouse);
        if (!TutorialDialogOpen)
        {
            UpdatePlacement(keyboard, mouse);
            UpdateBridges(keyboard, mouse);
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
        // 시드로 생성한 지면을 원본 isle 타일로 그린다.
        RefreshTerritoryAppearance();
        foreach (FortTerrainTile tile in _terrain.Tiles)
        {
            if (_edgeFarmCells.Contains((tile.X, tile.Y)))
            {
                continue;
            }
            int color = PreviewPlayerColors.GetValueOrDefault(tile.Owner);
            DrawSprite(batch, _terrainType.LoadIndex, MapSpriteFrames.BodyFrame(_terrainType.Definition, tile.Cluster),
                Screen(WorldPixels(tile.X, tile.Y), center), color);
        }
        // 절벽은 별도 기준점을 사용하며 본체 지면 위·건물 아래에 표시한다. 원본의 깊이 정렬은 추가 검증 대상이다.
        foreach (FortTerrainFringeSprite fringe in _fringes)
        {
            DrawSprite(batch, _fringeType.LoadIndex, MapSpriteFrames.BodyFrame(_fringeType.Definition, fringe.Cluster),
                Screen(WorldPixels(fringe.X, fringe.Y), center));
        }
        // edgeFarm은 원본의 matchframe으로 해당 isle을 대체한다. 원본과 같은 소유자 색상표를 적용한다.
        foreach (FortEdgeFarmTile tile in _edgeFarms)
        {
            int color = PreviewPlayerColors.GetValueOrDefault(tile.Owner);
            DrawSprite(batch, _edgeFarmType.LoadIndex, MapSpriteFrames.BodyFrame(_edgeFarmType.Definition, tile.Cluster),
                Screen(WorldPixels(tile.X, tile.Y), center), color);
        }
        // 확인된 3×3 받침은 같은 기준점에서 전용 하단 바위와 윗면을 그린다.
        foreach (FortIslandSupport support in _terrain.Supports)
        {
            int cluster = FortIslandSupports.ColorCluster(support.Owner, PreviewPlayerColors);
            Vector2 anchor = Screen(WorldPixels(support.X, support.Y), center);
            DrawSprite(batch, _supportBottomType.LoadIndex, MapSpriteFrames.BodyFrame(_supportBottomType.Definition, cluster), anchor);
            DrawSprite(batch, _supportTopType.LoadIndex, MapSpriteFrames.BodyFrame(_supportTopType.Definition, cluster), anchor);
        }
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
            int itemX = live?.Kind == ObjectKind.Priest ? live.Footprint.AnchorX : item.X;
            int itemY = live?.Kind == ObjectKind.Priest ? live.Footprint.AnchorY : item.Y;
            Vector2 anchor = Screen(WorldPixels(itemX, itemY), center);
            var sprite = GetSprite(item);
            if (sprite.HasValue)
            {
                var (texture, offset) = sprite.Value;
                batch.Draw(texture, anchor + offset.ToVector2() * _zoom, null, Color.White,
                    0f, Vector2.Zero, _zoom, SpriteEffects.None, 0f);
            }
            else
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
        DrawPlacedUnits(batch, center);
        DrawBridgeWorld(batch, center);
        DrawFlyers(batch, center);
        DrawCombat(batch, font, center);
        DrawSacrificeStatus(batch, font, center);
        batch.Draw(_pixel, new Rectangle(0, 0, width, HeaderHeight), new Color(18, 24, 38));
        batch.DrawString(font, $"맵: {Name} | 오브젝트 {_map.Objects.Count}개 | 확대 {_zoom:0.##}배 | 언어: {Language}", new Vector2(16, 10), Color.Gold);
        batch.DrawString(font, $"방향키 / 우클릭 / 화면 끝: 이동 · 휠: 확대 · Home: 사제 위치 · G: 청크 윤곽 · P: 배치 시험 · B: 다리 조각 · Esc: {(IsMissionMode ? "메뉴" : "종료")}", new Vector2(16, 38), Color.White);
        batch.DrawString(font, "F4: 사제 섬으로 · F11: 전체화면 · F10: 와이드 처리 · F9: 원본 해상도 높이 · F7: 가장자리 스크롤 · T: 선택→대상 클릭 · D+클릭: 내려놓기", new Vector2(16, 66), Color.White);
        batch.DrawString(font, displayInfo, new Vector2(16, 94), Color.LightGray);
        DrawSessionHud(batch, font, width);
        DrawPlacementOverlay(batch, font, center, width, height);
        DrawBridgeOverlay(batch, font, width, height);
        if (hovered != null && !_placementMode && !_bridgeMode)
        {
            string region = hovered.Territory.HasValue ? $"Terr{hovered.Territory:00}" : "Chaff";
            string text = $"{hovered.Object.Type.Name} ({hovered.X}, {hovered.Y}) | {region} | 소유자 {hovered.Object.Owner} | 다리 {hovered.Object.BridgeShape}";
            batch.Draw(_pixel, new Rectangle(0, height - 34, width, 34), new Color(18, 24, 38));
            batch.DrawString(font, text, new Vector2(16, height - 30), Color.White);
        }
        DrawTutorialDialog(batch, font, width, height);
        DrawKnowledge(batch, font, small, width, height);
        DrawMissionMenu(batch, font, width, height);
    }

    /// <summary>클러스터 번호에서 본체 레이어 프레임을 찾아 필요한 텍스처만 캐시한다. 거주지는 영역 원소 그림을 쓴다.</summary>
    private (Texture2D Texture, Point Offset)? GetSprite(FortMapObject item)
    {
        return GetTexture(item.Object.Type.LoadIndex,
            MapSpriteFrames.BodyFrame(item, _terrain.TerritoryTheme(item.Territory)));
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
        var key = (frame.Offset, color);
        if (!_textures.TryGetValue(key, out var sprite))
        {
            ReadOnlyMemory<byte> table = color == 0 ? default : _isleColors.Table(color);
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
        _pixel.Dispose();
    }
}
