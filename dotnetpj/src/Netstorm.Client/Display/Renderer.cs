using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 같은 dirty 표·자산 캐시·장면과 실제 그리기/출력 경계를 장기 소유한다.
namespace Netstorm.Client.Display;

/// <summary>Renderer.cpp의 장면/dirty 합성·출력 관리자다. OS 장치는 실제 caller가 제공한다.</summary>
public sealed class Renderer
{
    /// <summary>원문 signed byte 거리로 표현 가능한 최대 정렬 항목이다.</summary>
    private const int MaxSortEntries = int.MaxValue / 16;
    /// <summary>변경표와 같은 양수 화면 가로다.</summary>
    private readonly int _width;
    /// <summary>변경표와 같은 양수 화면 세로다.</summary>
    private readonly int _height;
    /// <summary>같은 원문 CRT 정렬을 선택하는 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>이미 검증된 원문100항목/첫 flags 저장소다.</summary>
    private readonly DirtyRegions _dirty;
    /// <summary>정렬 뒤 현재 장면의 값형 사본이다.</summary>
    private List<RenderSprite> _sprites = [];
    /// <summary>dirty별 재정렬이 시작할 원래 제출 순서다.</summary>
    private List<RenderSprite> _sourceSprites = [];
    /// <summary>현재 BYTE 글자 장면이다.</summary>
    private List<RenderText> _text = [];
    /// <summary>실제 현재 dirty별 추가 수집기다.</summary>
    private Func<ScreenRect, uint, IReadOnlyList<RenderSprite>>? _extraCollection;
    /// <summary>정지 장면은 SP/효과 재수집 없이 마지막 추가 목록을 사용한다.</summary>
    private List<RenderSprite> _extraCache = [];
    /// <summary>같은 팔레트 번호의 공유 배경이다.</summary>
    private IndexedImage? _background;
    /// <summary>월드 뒤의 공유 선택/UI 그림이다.</summary>
    private IndexedImage? _overlay;
    /// <summary>현재 실제 UI 트리의 직접 출력이다.</summary>
    private RenderProcedure? _overlayProcedure;
    /// <summary>현재 Draw가 실제 합성한 사각형이다.</summary>
    private readonly List<ScreenRect> _painted = [];
    /// <summary>각 합성 사각형과 같은 순서의 flags다.</summary>
    private readonly List<uint> _paintFlags = [];
    /// <summary>빌린 현재 소프트웨어 커서 그림이다.</summary>
    private IndexedImage? _cursor;
    /// <summary>현재 커서의 signed 화면 위치다.</summary>
    private ScreenPoint _cursorPosition;
    /// <summary>실제 DB 정체성/블록/프레임별 압축 해제 캐시다.</summary>
    private readonly Dictionary<(ShapeDatabase Database, int Block, int Frame), IndexedImage> _images = [];
    /// <summary>Native+5c의 현재 전체 출력 요청이며 Draw 범위와 구분한다.</summary>
    public UIntBox CameraRedraw { get; } = new();
    /// <summary>Native+60의 별도 시차 표면 출력 요청이다.</summary>
    public UIntBox BackgroundRedraw { get; } = new();
    /// <summary>497960이 이동/확장할 같은 실제 변경 목록이다.</summary>
    public List<DirtyRegion> CameraDirtyEntries => _dirty.CameraEntries;
    /// <summary>첫 화면/넘침의 현재 전체 재합성 상태다.</summary>
    public bool FullRedrawPending => _dirty.FullRedraw;
    /// <summary>실제 추가 수집기가 설치됐는지다.</summary>
    public bool HasExtraCollection => _extraCollection is not null;
    /// <summary>변경 영역을 그린 실제 프레임 수다.</summary>
    public ulong DrawCount { get; private set; }
    /// <summary>성공적으로 출력한 사각형 수다.</summary>
    public ulong PresentCount { get; private set; }
    /// <summary>같은 변경표를 만들고 첫 전체 갱신을 요청한다. 카메라 초기화는 별도다.</summary>
    public Renderer(int width, int height, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        _dirty = new(width, height); _width = width; _height = height; _edition = edition; _dirty.InvalidateAll();
    }
    /// <summary>양끝을 포함하지 않는 교집합 크기다.</summary>
    private static bool Nonempty(ScreenRect rect) => rect.Left < rect.Right && rect.Top < rect.Bottom;
    /// <summary>검증된 원문 좌표 제한 본체다.</summary>
    private static ScreenRect Clip(ScreenRect a, ScreenRect b) => ScreenGeometry.ClipScreenRect(a, b);
    /// <summary>상대 VFX 영역의 범위 밖 합은 signed 감김 대신 호스트 경계에서 거부한다.</summary>
    private static int Position(int origin, int offset, int edge = 0) => checked((int)((long)origin + offset + edge));
    /// <summary>CPP 값형 장면 사본처럼 색/후속 프레임 배열도 복사한다.</summary>
    private static List<RenderSprite> CopySprites(IEnumerable<RenderSprite> sprites) => sprites.Select(sprite => sprite.Copy()).ToList();
    /// <summary>기존 Native CRT 본체로 키/원래 인덱스를 정렬한 뒤 전체 항목을 재배치한다.</summary>
    private void SortSprites(List<RenderSprite> sprites)
    {
        if (sprites.Count > MaxSortEntries) throw new ArgumentOutOfRangeException(nameof(sprites)); var entries = new RenderCollectionEntry[sprites.Count];
        // 원문 정렬 키와 원래 번호만 만들고 본문 배열은 교환 전에 유지한다.
        for (int index = 0; index < sprites.Count; index++) { DrawOrder order = sprites[index].Order; entries[index] = new(order.X, order.Y, (uint)index, order.Depth, 0, 0); }
        DrawSort.Sort(entries, _edition); RenderSprite[] original = sprites.ToArray();
        // Native 정렬의 원래 번호로 각 값형 스프라이트를 한 번씩 이동한다.
        for (int index = 0; index < entries.Length; index++) sprites[index] = original[(int)entries[index].Token];
    }
    /// <summary>직접 출력/첫 그림/모든 후속 그림의 현재 합집합 범위다.</summary>
    private ScreenRect Bounds(RenderSprite sprite)
    {
        if (sprite.Procedure is { } procedure)
        {
            if (sprite.Database is not null || procedure.Draw is null) throw new ArgumentException("직접표시실제출력경계오류"); return Clip(procedure.Bounds, sprite.Clip);
        }
        ShapeDatabase database = sprite.Database ?? throw new ArgumentException("실제스프라이트DB누락");
        if (sprite.Block < 0 || sprite.Block >= database.Blocks().Count) throw new ArgumentOutOfRangeException(nameof(sprite.Block));
        ScreenRect bounds = default; bool present = false;
        // 같은 기준점에 옮긴 유효 물리 프레임만 전체 범위에 포함한다.
        void Include(int number)
        {
            ShapeFrame frame = database.Blocks()[sprite.Block].Frames[number]; if (frame.IsSpecial()) return;
            ScreenRect rect = Clip(new(Position(sprite.X, frame.Rect.Left), Position(sprite.Y, frame.Rect.Top), Position(sprite.X, frame.Rect.Right, 1), Position(sprite.Y, frame.Rect.Bottom, 1)), sprite.Clip);
            if (!Nonempty(rect)) return;
            bounds = present ? new(Math.Min(bounds.Left, rect.Left), Math.Min(bounds.Top, rect.Top), Math.Max(bounds.Right, rect.Right), Math.Max(bounds.Bottom, rect.Bottom)) : rect; present = true;
        }
        Include(sprite.Frame);
        // 후속 그림은 별도 정렬하지 않고 객체 내부 범위만 넓힌다.
        foreach (int number in sprite.FollowingFrames) Include(number); return bounds;
    }
    /// <summary>같은 실제 DB/블록/프레임의 이미지 압축을 한 번만 푼다.</summary>
    private IndexedImage Image(ShapeDatabase database, int block, int frame)
    {
        var key = (database, block, frame); if (!_images.TryGetValue(key, out var image)) _images.Add(key, image = database.Decode(block, frame)); return image;
    }
    /// <summary>한 유효 프레임을 같은 원문 치환/그림자 본체로 합성한다.</summary>
    private void DrawFrame(Span<byte> pixels, int pitch, ScreenRect clip, RenderSprite sprite, int number)
    {
        ShapeDatabase database = sprite.Database ?? throw new ArgumentException("실제스프라이트DB누락"); ShapeFrame frame = database.Blocks()[sprite.Block].Frames[number]; if (frame.IsSpecial()) return;
        RendererDraw.DrawIndexedImage(pixels, pitch, _height, Image(database, sprite.Block, number), Position(sprite.X, frame.Rect.Left), Position(sprite.Y, frame.Rect.Top), clip, sprite.Colors, sprite.Shadow);
    }
    /// <summary>직접 출력 또는 본 그림 뒤 후속 그림을 같은 항목 안에서 순서대로 합성한다.</summary>
    private void DrawSprite(Span<byte> pixels, int pitch, ScreenRect clip, RenderSprite sprite)
    {
        ScreenRect cut = Clip(clip, sprite.Clip);
        if (sprite.Procedure is { } procedure) { (procedure.Draw ?? throw new InvalidOperationException("직접출력장치누락"))(pixels, pitch, _height, cut); return; }
        DrawFrame(pixels, pitch, cut, sprite, sprite.Frame);
        // 본 그림 뒤 물결/상태 그림을 원래 제출 순서로 덮는다.
        foreach (int number in sprite.FollowingFrames) DrawFrame(pixels, pitch, cut, sprite, number);
    }
    /// <summary>이전/새 범위를 먼저 무효화하고 원래 제출 순서와 정렬된 장면을 각각 보관한다.</summary>
    public void SetScene(IReadOnlyList<RenderSprite> sprites, IReadOnlyList<RenderText>? text = null, bool sort = true)
    {
        ArgumentNullException.ThrowIfNull(sprites); List<RenderSprite> input = CopySprites(sprites); List<RenderText> words = text?.Select(item => item.Copy()).ToList() ?? [];
        // 사라지는 객체의 이전 범위도 다시 그려야 한다.
        foreach (RenderSprite sprite in _sprites) _dirty.Add(Bounds(sprite));
        // 실패한 뒤에도 선행 dirty 등록을 보존하며 현재 키를 검사한다.
        foreach (RenderSprite sprite in input) { if (!float.IsFinite(sprite.Order.X) || !float.IsFinite(sprite.Order.Y)) throw new ArgumentException("표시정렬비유한좌표"); _dirty.Add(Bounds(sprite)); }
        _sourceSprites = CopySprites(input); if (sort) SortSprites(input); if (_text.Count != 0 || words.Count != 0) _dirty.InvalidateAll(); _sprites = input; _text = words;
    }
    /// <summary>자산/글꼴 소유자가 반납할 때만 압축 해제 캐시를 비운다.</summary>
    public void ClearDecodedImages() => _images.Clear();
    /// <summary>화면 모드 변경 등 전체 재합성을 요청한다.</summary>
    public void InvalidateAll() => _dirty.InvalidateAll();
    /// <summary>현재 count/전체 플래그만 지우고 첫 flags 물리 잔여는 보존한다.</summary>
    public void ClearDirty() => _dirty.Clear();
    /// <summary>현재 같은 dirty 표에 객체 변경을 추가한다.</summary>
    public void Invalidate(ScreenRect rect, uint flags = 0) => _dirty.Add(rect, flags);
    /// <summary>수집기 교체/해제 때 옛 캐시와 callback을 반납하고 전체를 다시 그린다.</summary>
    public void SetExtraCollection(Func<ScreenRect, uint, IReadOnlyList<RenderSprite>>? collect) { _extraCollection = collect; _extraCache.Clear(); _dirty.InvalidateAll(); }
    /// <summary>공유 배경/overlay의 실제 화면 크기와 두 픽셀 표를 검사한다.</summary>
    private void ValidateImage(IndexedImage? image)
    {
        if (image is not null && (image.Width != _width || image.Height != _height || (long)image.Width * image.Height != image.Indices.Length || image.Opacity.Length != image.Indices.Length)) throw new ArgumentException("Renderer공유그림크기오류");
    }
    /// <summary>같은 팔레트 번호의 배경을 빌리고 전체를 다시 그린다.</summary>
    public void SetBackground(IndexedImage? image) { ValidateImage(image); _background = image; _dirty.InvalidateAll(); }
    /// <summary>월드 뒤의 선택/UI 그림을 빌리고 전체를 다시 그린다.</summary>
    public void SetOverlay(IndexedImage? image) { ValidateImage(image); _overlay = image; _dirty.InvalidateAll(); }
    /// <summary>실제 UI 직접 출력의 누락을 거부하며 이미 없는 장치를 다시 없앨 때는 쓰지 않는다.</summary>
    public void SetOverlayProcedure(RenderProcedure? procedure)
    {
        if (procedure is { Draw: null }) throw new ArgumentException("현재UI표면출력경계누락"); if (procedure is null && _overlayProcedure is null) return; _overlayProcedure = procedure; _dirty.InvalidateAll();
    }
    /// <summary>커서 정체성/위치가 같으면 보존하고 force일 때는 이전/새 범위를 모두 등록한다.</summary>
    public void SetSoftwareCursor(IndexedImage? image, ScreenPoint position, bool force = false)
    {
        if (!force && ReferenceEquals(_cursor, image) && position == _cursorPosition) return;
        // 빌린 그림의 실제 범위만 같은 dirty 표에 추가한다.
        void Add(IndexedImage? item, ScreenPoint at)
        {
            if (item is null) return; if (item.Width < 0 || item.Height < 0) throw new ArgumentException("커서그림크기오류"); _dirty.Add(new(at.X, at.Y, Position(at.X, item.Width), Position(at.Y, item.Height)));
        }
        Add(_cursor, _cursorPosition); Add(image, position); _cursor = image; _cursorPosition = position;
    }
    /// <summary>새 수집/피드백 없이 마지막 월드와 overlay만 별도 그림에 합성한다.</summary>
    public IndexedImage SceneImage()
    {
        var result = new IndexedImage { Width = _width, Height = _height, Indices = new byte[checked(_width * _height)], Opacity = Enumerable.Repeat((byte)255, checked(_width * _height)).ToArray() }; ScreenRect all = new(0, 0, _width, _height);
        if (_background is not null) RendererDraw.DrawIndexedImage(result.Indices, _width, _height, _background, 0, 0, all);
        List<RenderSprite> combined = CopySprites(_extraCache.Count == 0 ? _sprites : _sourceSprites); combined.AddRange(CopySprites(_extraCache)); if (_extraCache.Count != 0) SortSprites(combined);
        // UI 글자/커서는 제외하고 마지막 장면 순서만 합성한다.
        foreach (RenderSprite sprite in combined) { ScreenRect cut = Clip(all, sprite.Clip); if (Nonempty(Clip(Bounds(sprite), cut))) DrawSprite(result.Indices, _width, cut, sprite); }
        if (_overlay is not null) RendererDraw.DrawIndexedImage(result.Indices, _width, _height, _overlay, 0, 0, all); return result;
    }
    /// <summary>기존 실제 글리프 반복기를 사용하며 Memory는 동기 callback에서 같은 버퍼를 빌리기 위한 참조다.</summary>
    private void Text(Memory<byte> pixels, int pitch, ScreenRect clip, RenderText text)
    {
        if (text.Clip is { } inner) clip = Clip(clip, inner); BitmapFont font = text.Font ?? throw new ArgumentException("실제비트맵글꼴누락"); byte[] colors = new byte[256];
        var hooks = new GumpGlyphHooks
        {
            Color = color => colors[100] = color,
            Glyph = (selected, code, at) =>
            {
                ShapeDatabase shape = selected.Glyph(code).Shape ?? throw new InvalidOperationException("실제글리프포인터누락"); ShapeFrame frame = shape.Blocks()[0].Frames[0];
                if (!frame.IsSpecial()) RendererDraw.DrawIndexedImage(pixels.Span, pitch, _height, Image(shape, 0, 0), unchecked(at.X + frame.Rect.Left), unchecked(at.Y + frame.Rect.Top), clip, colors);
            },
        };
        var context = new GumpTextContext((text.Shadow ? 2u : 0u) | (text.Outline ? 4u : 0u), 0, text.Color, 0, 0, text.ShadowColor, text.ShadowOffset.X, text.ShadowOffset.Y);
        GumpTextDraw.DrawBitmapGlyphText(text.Bytes, font, new(text.X, text.Y), context, hooks, text.Bytes.Length);
    }
    /// <summary>현재 dirty별 배경→월드/미리보기→UI→커서를 합성하고 실제 표를 자른다. Present 전에는 재호출할 수 없다.</summary>
    public IReadOnlyList<ScreenRect> Draw(Memory<byte> pixels, int pitch, byte background = 0, RendererWorldDrawing? world = null)
    {
        if (_painted.Count != 0) throw new InvalidOperationException("Draw뒤Present필요"); if (pitch < _width || (long)pitch * _height > pixels.Length) throw new ArgumentException("실제Renderer표면범위오류");
        if (_dirty.FullRedraw) { _dirty.Clear(); _dirty.Add(new(0, 0, _width, _height)); }
        // 실제 표를 제자리로 자르며 빈 유효 항목도 Present 표에는 남긴다.
        for (int index = 0; index < _dirty.CameraEntries.Count; index++)
        {
            DirtyRegion entry = _dirty.CameraEntries[index]; if ((entry.Flags & 1) != 0) continue; entry = entry with { Rect = Clip(entry.Rect, new(0, 0, _width, _height)) }; _dirty.CameraEntries[index] = entry;
            if (Nonempty(entry.Rect)) { _painted.Add(entry.Rect); _paintFlags.Add(entry.Flags); }
        }
        // 합성 범위와 전체 출력 요청을 구분하고 원문 현재 callback을 매번 재조회한다.
        for (int index = 0; index < _painted.Count; index++)
        {
            ScreenRect clip = _painted[index]; uint flags = _paintFlags[index]; if (world is not null && (world.Collect is null || world.Background is null)) throw new InvalidOperationException("NativeRenderer현재지형배경장치누락");
            uint covered = world is null ? 0 : world.Collect!(clip, flags);
            if (world is null)
            {
                // 독립 표시의 불투명 전용 항목은 기존 배경 픽셀을 보존한다.
                if ((flags & 4) == 0) for (int y = clip.Top; y < clip.Bottom; y++) pixels.Span.Slice(y * pitch + clip.Left, clip.Right - clip.Left).Fill(background);
                if (_background is not null) RendererDraw.DrawIndexedImage(pixels.Span, pitch, _height, _background, 0, 0, clip);
            }
            List<RenderSprite> combined = CopySprites(_extraCollection is not null ? _sourceSprites : _sprites);
            if (_extraCollection is not null && (world is null || world.Submit is null))
            {
                List<RenderSprite> extra = CopySprites(_extraCollection(clip, flags)); if ((flags & 4) == 0) _extraCache = CopySprites(extra); combined.AddRange(extra); SortSprites(combined);
            }
            if (world is not null) { (world.Background ?? throw new InvalidOperationException("현재월드배경장치누락"))(clip, flags, covered); if ((flags & 4) != 0) combined.Clear(); }
            if (world?.Submit is { } submit) { submit(pixels.Span, pitch, _height, clip); if (_cursor is not null) RendererDraw.DrawIndexedImage(pixels.Span, pitch, _height, _cursor, _cursorPosition.X, _cursorPosition.Y, clip); continue; }
            // 모든 월드/미리보기 항목을 같은 원문 정렬과 객체 내부 순서로 그린다.
            foreach (RenderSprite sprite in combined) { ScreenRect cut = Clip(clip, sprite.Clip); if (Nonempty(Clip(Bounds(sprite), cut))) DrawSprite(pixels.Span, pitch, cut, sprite); }
            if (_overlay is not null && (world is null || (flags & 4) == 0)) RendererDraw.DrawIndexedImage(pixels.Span, pitch, _height, _overlay, 0, 0, clip);
            if (_overlayProcedure is { } procedure) { ScreenRect cut = Clip(clip, procedure.Bounds); if (Nonempty(cut)) (procedure.Draw ?? throw new InvalidOperationException("현재UI직접출력누락"))(pixels.Span, pitch, _height, cut); }
            // 실제 BYTE 글자 패스 뒤 소프트웨어 커서를 마지막으로 합성한다.
            foreach (RenderText text in _text) Text(pixels, pitch, clip, text);
            if (_cursor is not null) RendererDraw.DrawIndexedImage(pixels.Span, pitch, _height, _cursor, _cursorPosition.X, _cursorPosition.Y, clip);
        }
        if (_painted.Count != 0) DrawCount = unchecked(DrawCount + 1); return _painted;
    }
    /// <summary>두 출력 요청을 먼저 소비한 뒤 현재 표를 출력하고 정상 복귀했을 때만 비운다.</summary>
    public void Present(Action<ScreenRect> update)
    {
        if (CameraRedraw.Value != 0 || BackgroundRedraw.Value != 0)
        {
            CameraRedraw.Value = BackgroundRedraw.Value = 0; uint flags = _dirty.FirstFlags; _dirty.CameraEntries.Clear(); _dirty.CameraEntries.Add(new(new(0, 0, _width, _height), flags));
        }
        // 출력 실패 시 선행 카운터/현재 표를 보존해 원문 중단 위치를 유지한다.
        foreach (DirtyRegion entry in _dirty.Entries) if ((entry.Flags & 1) == 0) { update(entry.Rect); PresentCount = unchecked(PresentCount + 1); }
        _painted.Clear(); _paintFlags.Clear(); _dirty.Clear();
    }
}
