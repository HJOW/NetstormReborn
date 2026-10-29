namespace Netstorm.Client.Display; // 원본 인덱스 화면의 픽셀 출력 경계다.

/// <summary>현재 타입에 연결한 SHP 저장소/블록. alias는 호출 시점에 조회한다.</summary>
/// <param name="Database">현재 셰이프 저장소.</param>
/// <param name="Block">실제 블록 번호.</param>
public readonly record struct PreviewShapeBinding(ShapeDatabase Database, int Block);

/// <summary>실제 SHP 합성·직교 선택선·막대를 잘린 8비트 화면에 출력한다.
/// 근거: cpppj/src/client/SpriteDraw.cpp 의 SpritePixelTarget. 그림자도 대상이 아닌 소스 색을 치환한다.</summary>
public sealed class SpritePixelTarget
{
    /// <summary>원본 팔레트 인덱스 출력 버퍼.</summary>
    private readonly byte[] _pixels;
    /// <summary>한 행의 실제 바이트 간격.</summary>
    private readonly int _pitch;
    /// <summary>실제 행 수.</summary>
    private readonly int _height;
    /// <summary>표면 경계와 교차한 half-open 클립.</summary>
    private readonly ScreenRect _clip;
    /// <summary>alias를 포함한 현재 타입 바인딩.</summary>
    private readonly Func<uint, PreviewShapeBinding>? _shape;
    /// <summary>모드·타입·소유자의 실제 변환 표.</summary>
    private readonly Func<SpriteShapeMode, uint, uint, byte[]>? _colors;
    /// <summary>마지막으로 설정한 소유자 색 문맥.</summary>
    private byte[]? _current;
    /// <summary>저장소/블록/프레임별 디코딩 그림 캐시.</summary>
    private readonly Dictionary<(ShapeDatabase, int, uint), IndexedImage> _images = [];

    /// <summary>표면/클립을 검증하고 현재 그림·색 조회 경계를 빌린다. 저장소는 출력기보다 오래 살아야 한다.</summary>
    public SpritePixelTarget(byte[] pixels, int pitch, int height, ScreenRect clip,
        Func<uint, PreviewShapeBinding>? shape, Func<SpriteShapeMode, uint, uint, byte[]>? colors)
    {
        ArgumentNullException.ThrowIfNull(pixels);
        if (pitch <= 0 || height <= 0 || (long)pitch * height > pixels.Length || clip.Left > clip.Right || clip.Top > clip.Bottom)
            throw new ArgumentException("Sprite Draw 표면/클립 오류");
        _pixels = pixels; _pitch = pitch; _height = height;
        _clip = ScreenGeometry.ClipScreenRect(clip, new(0, 0, pitch, height));
        _shape = shape; _colors = colors;
    }

    /// <summary>게임 조회와 가상 처리기는 보존하고 그림·선·막대 경계만 이 표면에 연결한 사본을 만든다.</summary>
    public SpriteDrawHooks Hooks(SpriteDrawHooks? rules = null) => new()
    {
        SetOwnerColors = (type, owner) => _current = (byte[])(_colors ?? throw new ArgumentException("Sprite Draw 색 경계 누락"))(SpriteShapeMode.Owner, type, owner).Clone(),
        Shape = Shape,
        Line = Line,
        Fill = Fill,
        VirtualFrame = rules?.VirtualFrame,
        VirtualDraw = rules?.VirtualDraw,
        Surface = rules?.Surface,
        ConstructionActive = rules?.ConstructionActive,
        Resource = rules?.Resource,
        Timers = rules?.Timers,
        StatusProcesses = rules?.StatusProcesses,
    };

    /// <summary>투명 RLE 마스크를 보존하며 원색 또는 명시/현재 소스 색 표로 합성한다.</summary>
    private void Shape(SpriteShapeRequest request)
    {
        PreviewShapeBinding binding = (_shape ?? throw new ArgumentException("Sprite Draw SHP 경계 누락"))(request.Type);
        if (binding.Database is null) throw new ArgumentException("Sprite Draw SHP 바인딩 누락");
        IReadOnlyList<ShapeBlock> blocks = binding.Database.Blocks();
        if (binding.Block < 0 || binding.Block >= blocks.Count || request.Frame >= blocks[binding.Block].Frames.Count)
            throw new ArgumentOutOfRangeException(nameof(request), "Sprite Draw 프레임 오류");
        ShapeFrame frame = blocks[binding.Block].Frames[(int)request.Frame];
        if (frame.IsSpecial()) return;
        var key = (binding.Database, binding.Block, request.Frame);
        if (!_images.TryGetValue(key, out IndexedImage? image))
        {
            image = binding.Database.Decode(binding.Block, (int)request.Frame);
            _images.Add(key, image);
        }
        byte[]? colors = null;
        if (request.Mode == SpriteShapeMode.Owner) colors = _current ?? throw new InvalidOperationException("Sprite Draw 소유자 색 미설정");
        else if (request.Mode != SpriteShapeMode.Plain) colors = (_colors ?? throw new ArgumentException("Sprite Draw 색 경계 누락"))(request.Mode, request.Type, 0);
        RendererDraw.DrawIndexedImage(_pixels, _pitch, _height, image,
            unchecked(request.Position.X + frame.Rect.Left), unchecked(request.Position.Y + frame.Rect.Top), _clip, colors);
    }

    /// <summary>양 끝 포함 직교선을 원래 끝점에서 정규화하고 자른다. 대각선은 이 출력의 계약 밖이다.</summary>
    private void Line(ScreenPoint first, ScreenPoint last, uint color)
    {
        if (first.X != last.X && first.Y != last.Y) throw new ArgumentException("Sprite Draw 직교선 오류");
        if (first.Y == last.Y)
        {
            if (first.Y < _clip.Top || first.Y >= _clip.Bottom) return;
            int low = Math.Max(Math.Min(first.X, last.X), _clip.Left);
            int high = Math.Min(Math.Max(first.X, last.X), _clip.Right - 1);
            // 양 끝을 포함하는 가로선의 실제 남은 구간만 쓴다.
            for (int x = low; x <= high; x++) _pixels[first.Y * _pitch + x] = unchecked((byte)color);
        }
        else
        {
            if (first.X < _clip.Left || first.X >= _clip.Right) return;
            int low = Math.Max(Math.Min(first.Y, last.Y), _clip.Top);
            int high = Math.Min(Math.Max(first.Y, last.Y), _clip.Bottom - 1);
            // 양 끝을 포함하는 세로선의 실제 남은 구간만 쓴다.
            for (int y = low; y <= high; y++) _pixels[y * _pitch + first.X] = unchecked((byte)color);
        }
    }

    /// <summary>끝 제외 fill로 막대/배경을 채운다. 빈 영역과 역전 영역에는 쓰지 않는다.</summary>
    private void Fill(ScreenRect rect, uint color)
    {
        ScreenRect visible = ScreenGeometry.ClipScreenRect(rect, _clip);
        // 교차 영역의 실제 행만 방문한다.
        for (int y = visible.Top; y < visible.Bottom; y++)
        {
            // 각 행의 끝 좌표를 제외한 픽셀만 쓴다.
            for (int x = visible.Left; x < visible.Right; x++) _pixels[y * _pitch + x] = unchecked((byte)color);
        }
    }
}
