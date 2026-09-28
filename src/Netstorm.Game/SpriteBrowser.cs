using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>원본 셰이프의 타입·프레임·클러스터 정보를 한 화면에서 탐색하는 개발용 뷰어.</summary>
internal sealed class SpriteBrowser : IDisposable
{
    /// <summary>한 페이지에 표시하는 프레임 열 수.</summary>
    private const int Columns = 5;
    /// <summary>한 페이지에 표시하는 프레임 행 수.</summary>
    private const int Rows = 4;
    /// <summary>프레임 격자 위 안내 영역의 높이.</summary>
    private const int HeaderHeight = 82;
    /// <summary>선택 프레임 정보와 큰 그림에 배정하는 폭.</summary>
    private const int DetailsWidth = 270;
    /// <summary>격자 칸 안의 여백.</summary>
    private const int CellPadding = 8;
    /// <summary>작은 프레임이 지나치게 커지지 않도록 제한하는 격자 배율.</summary>
    private const float GridScaleLimit = 4f;
    /// <summary>선택 프레임의 확대 배율 상한.</summary>
    private const float PreviewScaleLimit = 8f;
    /// <summary>한 페이지의 프레임 수.</summary>
    private const int PageSize = Columns * Rows;

    private readonly GraphicsDevice _device;
    private readonly ShapeDatabase _shapes;
    private readonly Palette _palette;
    private readonly TypeCatalog _catalog;
    private readonly Texture2D _pixel;
    private readonly Dictionary<int, Texture2D> _textures = [];
    private KeyboardState _previousKeyboard;
    private MouseState _previousMouse;
    private int _typeIndex;
    private int _frameIndex;

    /// <summary>타입 이름을 확인하고 첫 프레임을 선택한다.</summary>
    public SpriteBrowser(GraphicsDevice device, ShapeDatabase shapes, Palette palette, TypeCatalog catalog, string typeName)
    {
        _device = device;
        _shapes = shapes;
        _palette = palette;
        _catalog = catalog;
        _typeIndex = TypeLoadOrder.IndexOf(typeName);
        if (_typeIndex < 0 || _typeIndex >= shapes.Blocks.Count)
        {
            throw new ArgumentException($"셰이프 타입이 없습니다: {typeName}", nameof(typeName));
        }
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
    }

    /// <summary>현재 선택한 타입의 이름.</summary>
    public string TypeName => _catalog.Types[_typeIndex].Name;

    /// <summary>누름 전환으로 타입·프레임을 움직이고 마우스로 격자 프레임을 선택한다.</summary>
    public void Update()
    {
        KeyboardState keyboard = Keyboard.GetState();
        MouseState mouse = Mouse.GetState();
        if (Pressed(keyboard, Keys.Up))
        {
            SelectType(_typeIndex - 1);
        }
        if (Pressed(keyboard, Keys.Down))
        {
            SelectType(_typeIndex + 1);
        }
        if (Pressed(keyboard, Keys.Left))
        {
            SelectFrame(_frameIndex - 1);
        }
        if (Pressed(keyboard, Keys.Right))
        {
            SelectFrame(_frameIndex + 1);
        }
        if (Pressed(keyboard, Keys.PageUp) || mouse.ScrollWheelValue > _previousMouse.ScrollWheelValue)
        {
            SelectFrame((_frameIndex / PageSize - 1) * PageSize);
        }
        if (Pressed(keyboard, Keys.PageDown) || mouse.ScrollWheelValue < _previousMouse.ScrollWheelValue)
        {
            SelectFrame((_frameIndex / PageSize + 1) * PageSize);
        }
        if (Pressed(keyboard, Keys.Home))
        {
            SelectFrame(0);
        }
        if (Pressed(keyboard, Keys.End))
        {
            SelectFrame(_shapes.Blocks[_typeIndex].Frames.Count - 1);
        }
        if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            int cellWidth = GridWidth(_device.Viewport.Width) / Columns;
            int cellHeight = GridHeight(_device.Viewport.Height) / Rows;
            int x = mouse.X / cellWidth;
            int y = (mouse.Y - HeaderHeight) / cellHeight;
            if (mouse.X >= 0 && mouse.X < cellWidth * Columns && mouse.Y >= HeaderHeight &&
                x < Columns && y >= 0 && y < Rows)
            {
                SelectFrame((_frameIndex / PageSize) * PageSize + y * Columns + x);
            }
        }
        _previousKeyboard = keyboard;
        _previousMouse = mouse;
    }

    /// <summary>눌린 순간만 처리해 게임 프레임 속도에 따른 반복 이동을 막는다.</summary>
    private bool Pressed(KeyboardState keyboard, Keys key) =>
        keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);

    /// <summary>다른 타입으로 옮기고 이전 타입에서 만든 GPU 텍스처를 해제한다.</summary>
    private void SelectType(int index)
    {
        int selected = Math.Clamp(index, 0, _catalog.Types.Count - 1);
        if (selected == _typeIndex)
        {
            return;
        }
        ClearTextures();
        _typeIndex = selected;
        _frameIndex = 0;
    }

    /// <summary>프레임 범위를 확인하고 페이지가 바뀌면 캐시를 비운다.</summary>
    private void SelectFrame(int index)
    {
        int count = _shapes.Blocks[_typeIndex].Frames.Count;
        if (count == 0)
        {
            return;
        }
        int selected = Math.Clamp(index, 0, count - 1);
        if (selected / PageSize != _frameIndex / PageSize)
        {
            ClearTextures();
        }
        _frameIndex = selected;
    }

    /// <summary>한 페이지의 축소 그림과 선택 프레임의 원본 정보를 표시한다.</summary>
    public void Draw(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        TypeInfo type = _catalog.Types[_typeIndex];
        ShapeBlock block = _shapes.Blocks[_typeIndex];
        int gridWidth = GridWidth(width);
        int cellWidth = gridWidth / Columns;
        int cellHeight = GridHeight(height) / Rows;
        int page = _frameIndex / PageSize;
        batch.DrawString(font, $"스프라이트: {type.Name} ({_typeIndex + 1}/{_catalog.Types.Count})", new Vector2(14, 8), Color.Gold);
        batch.DrawString(font, "↑↓ 타입  ←→ 프레임  PgUp/PgDn·휠 페이지  Home/End 처음·끝  클릭 선택  Esc 종료",
            new Vector2(14, 38), Color.White);
        // 현재 페이지의 모든 프레임을 원본 팔레트로 표시한다.
        for (int position = 0; position < PageSize; position++)
        {
            int index = page * PageSize + position;
            if (index >= block.Frames.Count)
            {
                break;
            }
            int x = position % Columns * cellWidth;
            int y = HeaderHeight + position / Columns * cellHeight;
            var cell = new Rectangle(x + 2, y + 2, cellWidth - 4, cellHeight - 4);
            batch.Draw(_pixel, cell, index == _frameIndex ? new Color(59, 85, 95) : new Color(40, 48, 65));
            batch.DrawString(font, $"#{index}", new Vector2(x + CellPadding, y + 3), Color.White);
            DrawFrame(batch, index, new Rectangle(x + CellPadding, y + 30,
                cellWidth - CellPadding * 2, cellHeight - 38), GridScaleLimit);
        }
        int detailsX = gridWidth + 8;
        batch.Draw(_pixel, new Rectangle(gridWidth, HeaderHeight, width - gridWidth, height - HeaderHeight),
            new Color(31, 38, 53));
        batch.DrawString(font, $"프레임 {_frameIndex + 1}/{block.Frames.Count}", new Vector2(detailsX, 91), Color.Gold);
        if (block.Frames.Count == 0)
        {
            return;
        }
        ShapeFrame frame = block.Frames[_frameIndex];
        Cluster? cluster = _frameIndex < type.Definition.Clusters.Count ? type.Definition.Clusters[_frameIndex] : null;
        batch.DrawString(font, cluster == null ? "클러스터: 추가 레이어" : $"클러스터: {cluster.Name}",
            new Vector2(detailsX, 121), Color.White);
        batch.DrawString(font, cluster == null ? "" : $"속성: {string.Join(" ", cluster.Flags)}",
            new Vector2(detailsX, 151), Color.LightGray);
        batch.DrawString(font, frame.IsSpecial ? "특수 레코드" : $"크기: {frame.Width} × {frame.Height}",
            new Vector2(detailsX, 181), Color.White);
        if (!frame.IsSpecial)
        {
            batch.DrawString(font, $"기준점: {frame.XMin}, {frame.YMin}", new Vector2(detailsX, 211), Color.LightGray);
            DrawFrame(batch, _frameIndex, new Rectangle(detailsX, 260, width - detailsX - CellPadding,
                Math.Max(60, height - 340)), PreviewScaleLimit);
        }
        if (cluster?.Layers.Count > 0)
        {
            batch.DrawString(font, $"그림: {cluster.Layers[0].Image}",
                new Vector2(detailsX, height - 52), Color.LightGray);
        }
    }

    /// <summary>프레임을 지정한 칸에 비율을 유지해 그리고 특수 레코드는 글자로 표시한다.</summary>
    private void DrawFrame(SpriteBatch batch, int index, Rectangle area, float scaleLimit)
    {
        ShapeFrame frame = _shapes.Blocks[_typeIndex].Frames[index];
        if (frame.IsSpecial)
        {
            return;
        }
        float scale = Math.Min(scaleLimit, Math.Min(area.Width / (float)frame.Width, area.Height / (float)frame.Height));
        Vector2 position = new(area.X + (area.Width - frame.Width * scale) / 2,
            area.Y + (area.Height - frame.Height * scale) / 2);
        batch.Draw(GetTexture(index), position, null, Color.White, 0f, Vector2.Zero, scale, SpriteEffects.None, 0f);
    }

    /// <summary>보이는 프레임만 디코딩해 텍스처로 캐시한다.</summary>
    private Texture2D GetTexture(int index)
    {
        if (!_textures.TryGetValue(index, out Texture2D? texture))
        {
            ShapeFrame frame = _shapes.Blocks[_typeIndex].Frames[index];
            texture = SpriteAnimation.ToTexture(_device, _shapes.Decode(frame), _palette);
            _textures.Add(index, texture);
        }
        return texture;
    }

    /// <summary>창 폭에서 정보 패널을 제외한 프레임 격자 폭.</summary>
    private static int GridWidth(int width) => Math.Max(Columns * 60, width - DetailsWidth);

    /// <summary>창 높이에서 머리말과 아래 여백을 제외한 프레임 격자 높이.</summary>
    private static int GridHeight(int height) => Math.Max(Rows * 60, height - HeaderHeight - 18);

    /// <summary>페이지 이동·타입 변경 전에 생성한 GPU 텍스처를 모두 해제한다.</summary>
    private void ClearTextures()
    {
        // 현재 페이지에서 만든 모든 프레임 텍스처를 해제한다.
        foreach (Texture2D texture in _textures.Values)
        {
            texture.Dispose();
        }
        _textures.Clear();
    }

    /// <summary>뷰어에서 소유한 GPU 텍스처를 해제한다.</summary>
    public void Dispose()
    {
        ClearTextures();
        _pixel.Dispose();
    }
}
