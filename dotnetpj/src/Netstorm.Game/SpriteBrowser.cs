using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>
/// 원본 셰이프의 타입·프레임·클러스터 정보를 한 화면에서 탐색하는 개발용 뷰어.
/// 선택 프레임이 속한 동작(원본 프레임 코드의 측면·변형이 같은 클러스터)을 재생하고,
/// 팔레트를 바꾸거나 .type 속성 전체를 볼 수 있다.
/// </summary>
internal sealed class SpriteBrowser : IDisposable
{
    /// <summary>한 페이지에 표시하는 프레임 열 수.</summary>
    private const int Columns = 5;
    /// <summary>한 페이지에 표시하는 프레임 행 수.</summary>
    private const int Rows = 4;
    /// <summary>프레임 격자 위 안내 영역의 높이.</summary>
    private const int HeaderHeight = 100;
    /// <summary>선택 프레임 정보와 큰 그림에 배정하는 폭.</summary>
    private const int DetailsWidth = 270;
    /// <summary>격자 칸 안의 여백.</summary>
    private const int CellPadding = 8;
    /// <summary>정보 패널·속성 목록의 줄 간격.</summary>
    private const int LineHeight = 26;
    /// <summary>작은 프레임이 지나치게 커지지 않도록 제한하는 격자 배율.</summary>
    private const float GridScaleLimit = 4f;
    /// <summary>선택 프레임의 확대 배율 상한.</summary>
    private const float PreviewScaleLimit = 8f;
    /// <summary>한 페이지의 프레임 수.</summary>
    private const int PageSize = Columns * Rows;
    /// <summary>재생 속도 기본값(초당 프레임). 원본 틱·동작별 속도는 아직 확인되지 않은 개발용 값이다.</summary>
    private const int DefaultFramesPerSecond = 10;
    /// <summary>재생 속도 하한.</summary>
    private const int MinFramesPerSecond = 1;
    /// <summary>재생 속도 상한.</summary>
    private const int MaxFramesPerSecond = 30;
    /// <summary>속성 목록의 열 수.</summary>
    private const int PropertyColumns = 2;

    private readonly GraphicsDevice _device;
    private readonly ShapeDatabase _shapes;
    private readonly TypeCatalog _catalog;
    private readonly IReadOnlyList<string> _paletteNames;
    private readonly Func<string, Palette> _loadPalette;
    private readonly Texture2D _pixel;
    private readonly Dictionary<int, Texture2D> _textures = [];
    private Palette _palette;
    private int _paletteIndex;
    private string? _paletteError;
    private KeyboardState _previousKeyboard;
    private MouseState _previousMouse;
    private int _typeIndex;
    private int _frameIndex;
    private int _framesPerSecond = DefaultFramesPerSecond;
    private double _elapsed;

    /// <summary>타입 이름을 확인하고 첫 프레임을 선택한다.</summary>
    /// <param name="device">텍스처를 만들 그래픽 장치</param>
    /// <param name="shapes">셰이프 데이터베이스</param>
    /// <param name="catalog">로딩 순서의 타입 목록</param>
    /// <param name="paletteNames">교체할 수 있는 팔레트 이름 목록</param>
    /// <param name="loadPalette">팔레트 이름으로 팔레트를 읽는 함수</param>
    /// <param name="paletteName">처음 적용할 팔레트 이름 (대소문자 무시)</param>
    /// <param name="typeName">처음 선택할 타입 이름</param>
    public SpriteBrowser(GraphicsDevice device, ShapeDatabase shapes, TypeCatalog catalog,
        IReadOnlyList<string> paletteNames, Func<string, Palette> loadPalette, string paletteName, string typeName)
    {
        _device = device;
        _shapes = shapes;
        _catalog = catalog;
        _loadPalette = loadPalette;
        _typeIndex = TypeLoadOrder.IndexOf(typeName);
        if (_typeIndex < 0 || _typeIndex >= shapes.Blocks.Count)
        {
            throw new ArgumentException($"셰이프 타입이 없습니다: {typeName}", nameof(typeName));
        }
        // 지정 팔레트가 목록에 없으면(대소문자·아카이브 차이) 목록 맨 앞에 넣는다
        var names = paletteNames.ToList();
        _paletteIndex = names.FindIndex(n => n.Equals(paletteName, StringComparison.OrdinalIgnoreCase));
        if (_paletteIndex < 0)
        {
            names.Insert(0, paletteName);
            _paletteIndex = 0;
        }
        _paletteNames = names;
        _palette = loadPalette(names[_paletteIndex]);
        _pixel = new Texture2D(device, 1, 1);
        _pixel.SetData(new[] { Color.White });
    }

    /// <summary>현재 선택한 타입의 이름.</summary>
    public string TypeName => _catalog.Types[_typeIndex].Name;

    /// <summary>선택 프레임이 속한 동작을 재생 중인지 여부.</summary>
    public bool Playing { get; set; }

    /// <summary>프레임 격자 대신 .type 속성 목록을 보여 줄지 여부.</summary>
    public bool ShowProperties { get; set; }

    /// <summary>현재 타입의 프레임 하나를 선택한다 (명령줄 --frame 용).</summary>
    public void SelectFrameAt(int index) => SelectFrame(index);

    /// <summary>누름 전환으로 타입·프레임·보기 모드를 바꾸고 재생 중이면 동작을 진행한다.</summary>
    /// <param name="deltaSeconds">지난 갱신 이후 경과 시간(초)</param>
    /// <param name="mouse">논리 화면 좌표로 바꾼 마우스 상태</param>
    /// <param name="viewWidth">논리 화면 폭</param>
    /// <param name="viewHeight">논리 화면 높이</param>
    public void Update(double deltaSeconds, MouseState mouse, int viewWidth, int viewHeight)
    {
        KeyboardState keyboard = Keyboard.GetState();
        bool shift = keyboard.IsKeyDown(Keys.LeftShift) || keyboard.IsKeyDown(Keys.RightShift);
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
        if (Pressed(keyboard, Keys.Space))
        {
            Playing = !Playing;
            _elapsed = 0;
        }
        if (Pressed(keyboard, Keys.OemPlus) || Pressed(keyboard, Keys.Add))
        {
            _framesPerSecond = Math.Min(MaxFramesPerSecond, _framesPerSecond + 1);
        }
        if (Pressed(keyboard, Keys.OemMinus) || Pressed(keyboard, Keys.Subtract))
        {
            _framesPerSecond = Math.Max(MinFramesPerSecond, _framesPerSecond - 1);
        }
        if (Pressed(keyboard, Keys.P))
        {
            SelectPalette(_paletteIndex + (shift ? -1 : 1));
        }
        if (Pressed(keyboard, Keys.Tab))
        {
            ShowProperties = !ShowProperties;
        }
        if (!ShowProperties && mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            int cellWidth = GridWidth(viewWidth) / Columns;
            int cellHeight = GridHeight(viewHeight) / Rows;
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
        Advance(deltaSeconds);
    }

    /// <summary>재생 중이면 누적 시간에 맞춰 같은 동작의 다음 프레임으로 넘긴다.</summary>
    private void Advance(double deltaSeconds)
    {
        if (!Playing)
        {
            return;
        }
        _elapsed += deltaSeconds;
        double step = 1.0 / _framesPerSecond;
        // 한 번의 갱신이 길어도 프레임 길이만큼씩 여러 번 진행한다
        while (_elapsed >= step)
        {
            _elapsed -= step;
            int next = NextFrameInSequence();
            if (next < 0)
            {
                Playing = false;
                return;
            }
            SelectFrame(next);
        }
    }

    /// <summary>
    /// 선택 프레임과 같은 레이어·동작에서 다음 프레임 번호. 셰이프 블록은 레이어별로 클러스터 전체를
    /// 차례로 담으므로 (레이어 × 클러스터 수 + 클러스터)로 계산한다. 동작이 없거나 범위를 벗어나면 -1.
    /// </summary>
    private int NextFrameInSequence()
    {
        TypeDefinition definition = _catalog.Types[_typeIndex].Definition;
        int clusters = definition.Clusters.Count;
        if (clusters == 0)
        {
            return -1;
        }
        int layer = _frameIndex / clusters;
        int cluster = _frameIndex % clusters;
        IReadOnlyList<int> sequence = definition.Frames.SequenceOf(cluster);
        int position = IndexOf(sequence, cluster);
        int next = layer * clusters + sequence[(position + 1) % sequence.Count];
        return next < _shapes.Blocks[_typeIndex].Frames.Count ? next : -1;
    }

    /// <summary>목록에서 값의 위치 (없으면 -1).</summary>
    private static int IndexOf(IReadOnlyList<int> list, int value)
    {
        // 동작 목록은 짧으므로 순차 검색한다
        for (int i = 0; i < list.Count; i++)
        {
            if (list[i] == value)
            {
                return i;
            }
        }
        return -1;
    }

    /// <summary>눌린 순간만 처리해 게임 프레임 속도에 따른 반복 이동을 막는다.</summary>
    private bool Pressed(KeyboardState keyboard, Keys key) =>
        keyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key);

    /// <summary>팔레트 목록을 순환하며 읽고, 실패하면 이전 팔레트를 유지한다.</summary>
    private void SelectPalette(int index)
    {
        int selected = ((index % _paletteNames.Count) + _paletteNames.Count) % _paletteNames.Count;
        try
        {
            _palette = _loadPalette(_paletteNames[selected]);
            _paletteError = null;
        }
        catch (Exception error) when (error is IOException or InvalidDataException or ArgumentException)
        {
            _paletteError = $"{_paletteNames[selected]}: {error.Message}";
        }
        // 읽지 못한 팔레트도 목록 위치는 옮겨 다음 팔레트로 넘어갈 수 있게 한다
        _paletteIndex = selected;
        ClearTextures();
    }

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

    /// <summary>머리말, 프레임 격자(또는 속성 목록), 선택 프레임 정보를 표시한다.</summary>
    public void Draw(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        TypeInfo type = _catalog.Types[_typeIndex];
        int gridWidth = GridWidth(width);
        string state = Playing ? $"재생 {_framesPerSecond}fps" : $"정지 {_framesPerSecond}fps";
        batch.DrawString(font, $"스프라이트: {type.Name} ({_typeIndex + 1}/{_catalog.Types.Count})  " +
            $"팔레트: {_paletteNames[_paletteIndex]} ({_paletteIndex + 1}/{_paletteNames.Count})  {state}",
            new Vector2(14, 8), Color.Gold);
        batch.DrawString(font, "↑↓ 타입  ←→ 프레임  PgUp/PgDn·휠 페이지  Home/End 처음·끝  클릭 선택",
            new Vector2(14, 38), Color.White);
        batch.DrawString(font, _paletteError ?? "Space 동작 재생/정지  +/- 속도  P/Shift+P 팔레트  Tab 속성 목록  Esc 종료",
            new Vector2(14, 64), _paletteError == null ? Color.White : Color.OrangeRed);
        if (ShowProperties)
        {
            DrawProperties(batch, font, type.Definition, new Rectangle(14, HeaderHeight, gridWidth - 28, height - HeaderHeight));
        }
        else
        {
            DrawGrid(batch, font, gridWidth, height);
        }
        DrawDetails(batch, font, type.Definition, gridWidth, width, height);
    }

    /// <summary>현재 페이지의 모든 프레임을 축소해 격자로 그린다.</summary>
    private void DrawGrid(SpriteBatch batch, SpriteFontBase font, int gridWidth, int height)
    {
        ShapeBlock block = _shapes.Blocks[_typeIndex];
        int cellWidth = gridWidth / Columns;
        int cellHeight = GridHeight(height) / Rows;
        int page = _frameIndex / PageSize;
        // 현재 페이지의 프레임을 선택 팔레트로 표시한다.
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
    }

    /// <summary>오른쪽 패널: 프레임 번호·원본 프레임 코드·특수 프레임·동작 위치와 확대 그림.</summary>
    private void DrawDetails(SpriteBatch batch, SpriteFontBase font, TypeDefinition definition,
        int gridWidth, int width, int height)
    {
        ShapeBlock block = _shapes.Blocks[_typeIndex];
        int detailsX = gridWidth + 8;
        batch.Draw(_pixel, new Rectangle(gridWidth, HeaderHeight, width - gridWidth, height - HeaderHeight),
            new Color(31, 38, 53));
        float y = HeaderHeight + 9;
        batch.DrawString(font, $"프레임 #{_frameIndex} ({_frameIndex + 1}/{block.Frames.Count})", new Vector2(detailsX, y), Color.Gold);
        if (block.Frames.Count == 0)
        {
            return;
        }
        ShapeFrame frame = block.Frames[_frameIndex];
        int clusters = definition.Clusters.Count;
        Cluster? cluster = clusters == 0 ? null : definition.Clusters[_frameIndex % clusters];
        int layer = clusters == 0 ? 0 : _frameIndex / clusters;
        // 표시할 정보 줄 목록 (글자, 색)
        var lines = new List<(string Text, Color Color)>();
        if (cluster == null)
        {
            lines.Add(("클러스터 없음", Color.White));
        }
        else
        {
            int index = _frameIndex % clusters;
            FrameCode code = definition.Frames.Codes[index];
            IReadOnlyList<int> sequence = definition.Frames.SequenceOf(index);
            lines.Add(($"클러스터: {cluster.Name} (레이어 {layer})", Color.White));
            lines.Add(($"코드: {code.Side} {code.Variant} {code.Number}", Color.LightGray));
            lines.Add(($"속성: {string.Join(" ", cluster.Flags)}", Color.LightGray));
            lines.Add(($"동작: {IndexOf(sequence, index) + 1}/{sequence.Count}", Color.LightGray));
            lines.Add(($"특수: {SpecialFrameNames(definition.Frames, index)}", Color.LightGray));
        }
        lines.Add((frame.IsSpecial ? "특수 레코드" : $"크기: {frame.Width} × {frame.Height}", Color.White));
        if (!frame.IsSpecial)
        {
            lines.Add(($"기준점: {frame.XMin}, {frame.YMin}", Color.LightGray));
        }
        // 정보 줄을 차례로 출력한다
        foreach ((string text, Color color) in lines)
        {
            y += LineHeight;
            batch.DrawString(font, text, new Vector2(detailsX, y), color);
        }
        y += LineHeight + 12;
        if (!frame.IsSpecial)
        {
            DrawFrame(batch, _frameIndex, new Rectangle(detailsX, (int)y, width - detailsX - CellPadding,
                Math.Max(60, height - (int)y - 60)), PreviewScaleLimit);
        }
        if (cluster != null && layer < cluster.Layers.Count)
        {
            batch.DrawString(font, $"그림: {cluster.Layers[layer].Image} #{cluster.Layers[layer].Frame}",
                new Vector2(detailsX, height - 40), Color.LightGray);
        }
    }

    /// <summary>원본 타입 구조체의 특수 프레임 중 지정 클러스터에 해당하는 이름들.</summary>
    private static string SpecialFrameNames(TypeFrameTable table, int index)
    {
        var names = new List<string>();
        if (table.DefaultFrame == index)
        {
            names.Add("기본");
        }
        if (table.HelpFrame == index)
        {
            names.Add("도움말");
        }
        if (table.GumpFrame == index)
        {
            names.Add("gump");
        }
        if (table.BaseFrame == index)
        {
            names.Add("base");
        }
        return names.Count == 0 ? "-" : string.Join(" ", names);
    }

    /// <summary>.type 머리·typeflags·속성 전체를 여러 열로 표시한다 (칸을 넘는 값은 줄임).</summary>
    private static void DrawProperties(SpriteBatch batch, SpriteFontBase font, TypeDefinition definition, Rectangle area)
    {
        var lines = new List<(string Text, Color Color)>
        {
            ($"typename {definition.Name} {definition.Modifier}".TrimEnd(), Color.Gold),
        };
        int columnWidth = area.Width / PropertyColumns - 12;
        lines.AddRange(Wrap(font, "typeflags: " + string.Join(" ", definition.Flags), columnWidth)
            .Select(l => (l, Color.LightGreen)));
        TypeFrameTable frames = definition.Frames;
        lines.Add(($"클러스터 {definition.Clusters.Count}개, 레이어 {definition.LayerCount}", Color.LightGreen));
        lines.Add(($"기본 {frames.DefaultFrame}  도움말 {frames.HelpFrame}  gump {frames.GumpFrame}  base {frames.BaseFrame}",
            Color.LightGreen));
        // 속성은 파일에 나온 키 이름 순서와 무관하게 이름순으로 정렬해 찾기 쉽게 한다
        foreach ((string key, string value) in definition.Properties.OrderBy(p => p.Key, StringComparer.OrdinalIgnoreCase))
        {
            lines.Add((Fit(font, $"{key} = {value}", columnWidth), Color.White));
        }
        int rowsPerColumn = Math.Max(1, (area.Height - 10) / LineHeight);
        // 열이 가득 차면 다음 열로 넘기고, 모든 열이 차면 생략 표시를 남긴다
        for (int i = 0; i < lines.Count; i++)
        {
            int column = i / rowsPerColumn;
            if (column >= PropertyColumns)
            {
                batch.DrawString(font, $"… {lines.Count - i}줄 생략", new Vector2(area.Right - 160, area.Bottom - LineHeight), Color.OrangeRed);
                break;
            }
            var position = new Vector2(area.X + column * (area.Width / PropertyColumns), area.Y + 8 + i % rowsPerColumn * LineHeight);
            batch.DrawString(font, lines[i].Text, position, lines[i].Color);
        }
    }

    /// <summary>단어 단위로 폭에 맞게 줄을 나눈다.</summary>
    private static IEnumerable<string> Wrap(SpriteFontBase font, string text, int maxWidth)
    {
        string line = "";
        // 단어를 붙여 가다 폭을 넘으면 줄을 끊는다
        foreach (string word in text.Split(' ', StringSplitOptions.RemoveEmptyEntries))
        {
            string candidate = line.Length == 0 ? word : $"{line} {word}";
            if (line.Length > 0 && font.MeasureString(candidate).X > maxWidth)
            {
                yield return line;
                candidate = "  " + word;
            }
            line = candidate;
        }
        if (line.Length > 0)
        {
            yield return line;
        }
    }

    /// <summary>폭을 넘는 문자열 끝을 잘라 "…"를 붙인다.</summary>
    private static string Fit(SpriteFontBase font, string text, int maxWidth)
    {
        if (font.MeasureString(text).X <= maxWidth)
        {
            return text;
        }
        // 한 글자씩 줄이며 폭에 맞는 길이를 찾는다
        for (int length = text.Length - 1; length > 0; length--)
        {
            string cut = text[..length] + "…";
            if (font.MeasureString(cut).X <= maxWidth)
            {
                return cut;
            }
        }
        return "…";
    }

    /// <summary>프레임을 지정한 칸에 비율을 유지해 그리고 특수 레코드는 건너뛴다.</summary>
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

    /// <summary>페이지 이동·타입·팔레트 변경 전에 생성한 GPU 텍스처를 모두 해제한다.</summary>
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
