using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>저장된 맵 오브젝트와 영역 청크를 원본 배율로 살펴보는 개발용 뷰어.</summary>
internal sealed class FortMapViewer : IDisposable
{
    /// <summary>초당 카메라 이동 픽셀 수.</summary>
    private const float PanSpeed = 600f;
    /// <summary>오브젝트가 표시되는 기본 확대 배율.</summary>
    private const float DefaultZoom = 1f;
    /// <summary>상단 안내 영역 높이.</summary>
    private const int HeaderHeight = 100;
    /// <summary>제공된 공식 미션 캡처의 플레이어 1 청록·플레이어 2 빨강을 사용하는 개발용 색상표.</summary>
    private static readonly IReadOnlyDictionary<int, int> PreviewPlayerColors = new Dictionary<int, int> { [1] = 7, [2] = 2 };
    private readonly FortMap _map;
    private readonly ShapeDatabase _shapes;
    private readonly Palette _palette;
    private readonly GraphicsDevice _device;
    private readonly Texture2D _pixel;
    private readonly Dictionary<int, (Texture2D Texture, Point Offset)> _textures = [];
    private readonly FortMapObject[] _sorted;
    private readonly FortTerrainPreview _terrain;
    private readonly TypeInfo _terrainType;
    private readonly TypeInfo _fringeType;
    private readonly TypeInfo _supportTopType;
    private readonly TypeInfo _supportBottomType;
    private readonly IReadOnlyList<FortTerrainFringeSprite> _fringes;
    private bool _showChunks;
    private Vector2 _camera;
    private float _zoom = DefaultZoom;
    private KeyboardState _previousKeyboard;
    private MouseState _previousMouse;

    /// <summary>로드한 맵 파일의 표시 이름.</summary>
    public string Name { get; }

    /// <summary>실행 설정에서 선택한 언어. 개발용 안내 문구의 번역 여부와는 별개다.</summary>
    private string Language { get; }

    /// <summary>오브젝트 위치를 계산하고 카메라를 플레이어 사제에 맞춘다.</summary>
    public FortMapViewer(GraphicsDevice device, ShapeDatabase shapes, Palette palette, FortFile fort, string name,
        TypeCatalog catalog, string language)
    {
        _device = device;
        _shapes = shapes;
        _palette = palette;
        _map = new FortMap(fort);
        _terrainType = catalog.Find("isle") ?? throw new InvalidDataException("isle 타입이 없습니다.");
        _fringeType = catalog.Find("fringe") ?? throw new InvalidDataException("fringe 타입이 없습니다.");
        _supportTopType = catalog.Find("island") ?? throw new InvalidDataException("island 타입이 없습니다.");
        _supportBottomType = catalog.Find("islandStalag") ?? throw new InvalidDataException("islandStalag 타입이 없습니다.");
        _terrain = new FortTerrainPreview(_map, _terrainType.Definition);
        _fringes = FortTerrainFringe.Create(_terrain, _terrainType.Definition, _fringeType.Definition);
        Name = name;
        Language = language;
        _sorted = _map.Objects.OrderBy(o => o.Object.Type.Definition.HasFlag("surface") ? 0 : 1)
            .ThenBy(o => o.Y).ThenBy(o => o.X).ToArray();
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
        CenterOnPriest();
    }

    /// <summary>플레이어 1의 사제를 중심으로 맞추며 없으면 첫 오브젝트를 사용한다.</summary>
    private void CenterOnPriest()
    {
        FortMapObject? focus = _map.Objects.FirstOrDefault(o => o.Object.Type.Name == "priest" && o.Object.Owner == 1)
            ?? _map.Objects.FirstOrDefault();
        _camera = focus == null ? Vector2.Zero : WorldPixels(focus.X, focus.Y);
    }

    /// <summary>키보드와 마우스로 카메라를 이동하고 확대 배율을 변경한다.</summary>
    public void Update(double seconds)
    {
        KeyboardState keyboard = Keyboard.GetState();
        MouseState mouse = Mouse.GetState();
        var direction = new Vector2(
            (keyboard.IsKeyDown(Keys.Right) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Left) ? 1 : 0),
            (keyboard.IsKeyDown(Keys.Down) ? 1 : 0) - (keyboard.IsKeyDown(Keys.Up) ? 1 : 0));
        _camera += direction * PanSpeed * (float)seconds / _zoom;
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
        if (keyboard.IsKeyDown(Keys.G) && !_previousKeyboard.IsKeyDown(Keys.G))
        {
            _showChunks = !_showChunks;
        }
        _previousKeyboard = keyboard;
        _previousMouse = mouse;
    }

    /// <summary>영역 청크 윤곽과 정적 스프라이트를 그리고 마우스 가까운 오브젝트를 설명한다.</summary>
    public void Draw(SpriteBatch batch, SpriteFontBase font, int width, int height)
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
        foreach (FortTerrainTile tile in _terrain.Tiles)
        {
            DrawSprite(batch, _terrainType.LoadIndex, MapSpriteFrames.BodyFrame(_terrainType.Definition, tile.Cluster),
                Screen(WorldPixels(tile.X, tile.Y), center));
        }
        // 절벽은 별도 기준점을 사용하며 본체 지면 위·건물 아래에 표시한다. 원본의 깊이 정렬은 추가 검증 대상이다.
        foreach (FortTerrainFringeSprite fringe in _fringes)
        {
            DrawSprite(batch, _fringeType.LoadIndex, MapSpriteFrames.BodyFrame(_fringeType.Definition, fringe.Cluster),
                Screen(WorldPixels(fringe.X, fringe.Y), center));
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
            if (item.Object.Type.Name == "noIsland")
            {
                continue;
            }
            Vector2 anchor = Screen(WorldPixels(item.X, item.Y), center);
            var sprite = GetSprite(item.Object);
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
        batch.Draw(_pixel, new Rectangle(0, 0, width, HeaderHeight), new Color(18, 24, 38));
        batch.DrawString(font, $"맵: {Name} | 오브젝트 {_map.Objects.Count}개 | 확대 {_zoom:0.##}배 | 언어: {Language}", new Vector2(16, 10), Color.Gold);
        batch.DrawString(font, "방향키 / 우클릭: 이동 · 휠: 확대 · Home: 사제 위치 · G: 청크 윤곽 · Esc: 종료", new Vector2(16, 38), Color.White);
        batch.DrawString(font, "다리: 저장 프레임 · 지면: 생성 미리보기 · 지면 변형/그림자/플레이어색은 검증 전", new Vector2(16, 66), Color.LightGray);
        if (hovered != null)
        {
            string region = hovered.Territory.HasValue ? $"Terr{hovered.Territory:00}" : "Chaff";
            string text = $"{hovered.Object.Type.Name} ({hovered.X}, {hovered.Y}) | {region} | 소유자 {hovered.Object.Owner} | 다리 {hovered.Object.BridgeShape}";
            batch.Draw(_pixel, new Rectangle(0, height - 34, width, 34), new Color(18, 24, 38));
            batch.DrawString(font, text, new Vector2(16, height - 30), Color.White);
        }
    }

    /// <summary>클러스터 번호에서 본체 레이어 프레임을 찾아 필요한 텍스처만 캐시한다.</summary>
    private (Texture2D Texture, Point Offset)? GetSprite(FortObject obj)
    {
        return GetTexture(obj.Type.LoadIndex, MapSpriteFrames.BodyFrame(obj));
    }

    /// <summary>지면 타일을 기준점과 프레임 오프셋에 맞춰 그린다.</summary>
    private void DrawSprite(SpriteBatch batch, int typeIndex, int frameIndex, Vector2 anchor)
    {
        var sprite = GetTexture(typeIndex, frameIndex);
        if (sprite.HasValue)
        {
            var (texture, offset) = sprite.Value;
            batch.Draw(texture, anchor + offset.ToVector2() * _zoom, null, Color.White,
                0f, Vector2.Zero, _zoom, SpriteEffects.None, 0f);
        }
    }

    /// <summary>본체 프레임을 텍스처로 변환하고 캐시한다.</summary>
    private (Texture2D Texture, Point Offset)? GetTexture(int typeIndex, int index)
    {
        ShapeBlock block = _shapes.Blocks[typeIndex];
        if (index >= block.Frames.Count || block.Frames[index].IsSpecial)
        {
            return null;
        }
        ShapeFrame frame = block.Frames[index];
        if (!_textures.TryGetValue(frame.Offset, out var sprite))
        {
            sprite = (SpriteAnimation.ToTexture(_device, _shapes.Decode(frame), _palette), new Point(frame.XMin, frame.YMin));
            _textures.Add(frame.Offset, sprite);
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
