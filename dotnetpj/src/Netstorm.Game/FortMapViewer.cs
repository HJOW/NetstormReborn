using System.Diagnostics;
using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Display;
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
    /// <summary>_map: 뷰어가 표시하고 세션을 구성할 .fort 지도.</summary>
    private readonly FortMap _map;
    /// <summary>_shapes: 타입 프레임의 크기와 픽셀을 조회하는 셰이프 DB.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>_palette: 스프라이트 인덱스를 실제 색으로 바꿀 현재 팔레트.</summary>
    private readonly Palette _palette;
    /// <summary>타입별 지면·유닛·건물의 소유자 색 변환표.</summary>
    private readonly ObjectColorRemap _objectColors;
    /// <summary>_device: 텍스처 생성과 화면 출력을 수행할 그래픽 장치.</summary>
    private readonly GraphicsDevice _device;
    /// <summary>_pixel: 사각형·선·단색 배경을 그릴 때 늘려 쓰는 1×1 텍스처.</summary>
    private readonly Texture2D _pixel;
    /// <summary>메인 메뉴와 같은 원본 UI 질감·장식을 쓰는 공유 그리기 도구.</summary>
    private readonly OriginalUiSkin _uiSkin;
    /// <summary>타입마다 색 변환 대상이 다르므로 타입·프레임 위치·소유자색을 함께 캐시한다.</summary>
    private readonly Dictionary<(int Type, int Frame, int Color), (Texture2D Texture, Point Offset)> _textures = [];
    /// <summary>_terrain: 지도 지형을 펼쳐 만든 타일과 지역 정보.</summary>
    private FortTerrainPreview _terrain;
    /// <summary>_terrainType: 지면 프레임을 조회할 isle 타입.</summary>
    private readonly TypeInfo _terrainType;
    /// <summary>_edgeFarmType: 가장자리 장식을 그릴 edgeFarm 타입.</summary>
    private readonly TypeInfo _edgeFarmType;
    /// <summary>_fringeType: 절벽 장식을 그릴 fringe 타입.</summary>
    private readonly TypeInfo _fringeType;
    /// <summary>_supportTopType: 섬 받침 윗부분을 그릴 타입.</summary>
    private readonly TypeInfo _supportTopType;

    /// <summary>다 쓴 간헐천을 그릴 원본 emptyGeyser 타입 (없으면 원래 간헐천 그림을 유지).</summary>
    private readonly TypeInfo? _emptyGeyserType;
    /// <summary>_supportBottomType: 섬 받침 아랫부분을 그릴 타입.</summary>
    private readonly TypeInfo _supportBottomType;
    /// <summary>_fringes: 지도에서 계산한 절벽 장식의 출력 목록.</summary>
    private IReadOnlyList<FortTerrainFringeSprite> _fringes;
    /// <summary>_edgeFarms: 지도 가장자리에 배치한 edgeFarm 타일 목록.</summary>
    private IReadOnlyList<FortEdgeFarmTile> _edgeFarms;
    /// <summary>_edgeFarmCells: edgeFarm 출력 위치가 중복되지 않도록 기록한 칸 좌표.</summary>
    private readonly HashSet<(int X, int Y)> _edgeFarmCells;
    /// <summary>_showChunks: 청크 경계의 개발용 표시를 켤지.</summary>
    private bool _showChunks;
    /// <summary>_camera: 월드에서 화면 좌상단에 놓을 카메라 좌표.</summary>
    private Vector2 _camera;
    /// <summary>_zoom: 월드 좌표를 화면 픽셀로 바꾸는 확대 배율.</summary>
    private float _zoom = DefaultZoom;
    /// <summary>_previousKeyboard: 새 키 입력과 눌림 유지를 구별할 이전 프레임 키보드 상태.</summary>
    private KeyboardState _previousKeyboard;
    /// <summary>_previousMouse: 클릭·해제·휠 변화를 검출할 이전 프레임 마우스 상태.</summary>
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
    /// <param name="gameClock">앱 화면 루프와 공유하는 원본 숫자 게임 시계.</param>
    /// <param name="readFrameMilliseconds">앱이 이번 화면 시작에 고정한 단조 밀리초를 반환하는 공급자.</param>
    public FortMapViewer(GraphicsDevice device, ShapeDatabase shapes, Palette palette, FortFile fort, string name,
        TypeCatalog catalog, string language, OriginalUiSkin uiSkin, OriginalGameClock gameClock,
        Func<long> readFrameMilliseconds, MissionStart? mission = null,
        MissionScript? tutorialScript = null, ConfigStore? tutorialSettings = null, HelpTopics? help = null,
        MissionScript? commonScript = null, Action<string, string>? scriptConfigChanged = null)
    {
        ArgumentNullException.ThrowIfNull(gameClock);
        ArgumentNullException.ThrowIfNull(readFrameMilliseconds);
        _gameClock = gameClock;
        _readFrameMilliseconds = readFrameMilliseconds;
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
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
        InitializeSession(fort, catalog, mission);
        InitializeKnowledge(fort, catalog, help);
        InitializeTutorialDialog(tutorialScript, tutorialSettings, commonScript, scriptConfigChanged);
        InitializePlacement(catalog);
        InitializeBridges(catalog);
        CenterOnPriest();
        // 첫 그리기에서 신전 테마로 지면을 다시 만드는 일(약 80ms)과 그 그림의 텍스처 만들기를 미션 로딩 중에 끝낸다.
        RefreshTerritoryAppearance();
        WarmTextures();
        SynchronizePauseReasons();
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

    /// <summary>카메라·지도 입력을 처리한 뒤 같은 화면 프레임의 명령·시간을 갱신한다.</summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초)</param>
    /// <param name="mouse">논리 화면 좌표로 바꾼 마우스 상태</param>
    /// <param name="edgeScroll">가장자리 스크롤이 이번 갱신에서 옮길 논리 픽셀 (양수 = 오른쪽·아래)</param>
    /// <param name="externalDialogOpen">입력 처리 뒤에도 남은 외부 도움말/옵션 창. 브리핑/재개 프레임 판정에 사용한다.</param>
    public void Update(double seconds, MouseState mouse, Vector2 edgeScroll, int width, int height,
        KeyboardState? input = null, bool inputBlocked = false, bool externalDialogOpen = false)
    {
        SynchronizePauseReasons();
        try
        {
            UpdateMap(seconds, mouse, edgeScroll, width, height, input, inputBlocked);
        }
        finally
        {
            // 안내/지식창·메뉴의 조기 반환에서도 안내/재개/튜토리얼 판정은 화면당 한 번만 진행한다.
            UpdateMissionDialogFrame(externalDialogOpen);
            SynchronizePauseReasons();
        }
    }

    /// <summary>기존 지도 입력·표시 갱신 본체. 외부 Update가 진입/반환 시 정지 소유권을 동기화한다.</summary>
    private void UpdateMap(double seconds, MouseState mouse, Vector2 edgeScroll, int width, int height,
        KeyboardState? input, bool inputBlocked)
    {
        if (width < 320 || height < 320) return;
        _viewSize = new Point(width, height);
        _lastCenter = new Vector2(width / 2f, (height + HeaderHeight) / 2f);
        UpdateEffects(seconds);
        UpdateWalkClock(seconds);
        KeyboardState keyboard = input ?? Keyboard.GetState();
        if (inputBlocked)
        {
            if (!TutorialDialogOpen) UpdateSession(new KeyboardState(), mouse);
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
            UpdateSession(new KeyboardState(), mouse);
            _previousKeyboard = keyboard; _previousMouse = mouse; return;
        }
        if (UpdateMissionMenu(keyboard, mouse, width, height))
        {
            // 메뉴가 열린 동안에도 원본처럼 미션 시간은 흐르되 지도 조작은 받지 않는다.
            UpdateSession(new KeyboardState(), mouse);
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
        if (!TutorialDialogOpen)
        {
            if (!uiConsumed)
            {
                UpdatePlacement(keyboard, mouse);
            }
            // 다리 키(Q W A S Z X·E·C·Backspace)는 커서가 생산 창 위에 있어도 받고, 지도 클릭·커서 칸만 막는다
            UpdateBridges(keyboard, mouse, mapInput: !uiConsumed);
        }
        // 배치·다리 입력까지 모은 뒤 처리하여 그 프레임의 그리기에 명령 결과가 반영되게 한다.
        UpdateSession(keyboard, mouse);
        _previousKeyboard = keyboard;
        _previousMouse = mouse;
    }

    /// <summary>현재 월드 표시 목록을 정렬해 그리고, 화면 입력과 안내·미리보기를 겹쳐 표시한다.</summary>
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
        long perfWorld = Stopwatch.GetTimestamp();
        _lastCenter = center;
        DrawWorldDisplay(batch, center);
        FortMapObject? hovered = _worldDisplay.LastOrDefault(item => item.Stored != null
            && Vector2.DistanceSquared(Screen(new Vector2(item.Order.X * FortMap.CellPixelWidth, item.Order.Y * FortMap.CellPixelHeight), center),
                new Vector2(_previousMouse.X, _previousMouse.Y)) < 20 * 20)?.Stored;
        DrawHeldBridgePreview(batch, center);
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

    /// <summary>현재 뷰어가 소유한 게임 정지를 풀고 모든 GPU 텍스처를 해제한다.</summary>
    public void Dispose()
    {
        _sessionFrameClock.ReleasePauseReasons(_readFrameMilliseconds());
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
