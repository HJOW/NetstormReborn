// 값형 장면 항목과 실제 버퍼 출력 장치를 Renderer 수명에서 구분한다.
namespace Netstorm.Client.Display;

/// <summary>깊이 내림차순과 Y/X 오름차순의 원문 정렬 키다.</summary>
/// <param name="X">현재 단정도 X다.</param>
/// <param name="Y">현재 단정도 Y다.</param>
/// <param name="Depth">현재 signed WORD 깊이다.</param>
public readonly record struct DrawOrder(float X, float Y, short Depth);

/// <summary>실제8비트 표면을 현재 dirty clip 안에 그리는 동기 장치다.</summary>
public delegate void RenderSurfaceDraw(Span<byte> pixels, int pitch, int height, ScreenRect clip);

/// <summary>SHP 없이 직접 그리는 한 표시 항목이다.</summary>
/// <param name="Bounds">현재 화면 범위다.</param>
/// <param name="Draw">현재 실제 표면 소비자다.</param>
public readonly record struct RenderProcedure(ScreenRect Bounds, RenderSurfaceDraw? Draw);

/// <summary>실제 Native Frame의 지형 수집/배경/동일목록 전체 출력 경계다.</summary>
public sealed class RendererWorldDrawing
{
    /// <summary>현재 해시에서 지형과 덮임을 먼저 수집한다.</summary>
    public Func<ScreenRect, uint, uint>? Collect { get; set; }
    /// <summary>수집 뒤 현재 flags/덮임으로 실제 배경을 그린다.</summary>
    public Action<ScreenRect, uint, uint>? Background { get; set; }
    /// <summary>지정된 Native 경로는 한 원문 목록 전체를 출력하고 분리 장면을 중복 출력하지 않는다.</summary>
    public RenderSurfaceDraw? Submit { get; set; }
}

/// <summary>Renderer.h의 값형 프레임·정렬·색·후속 그림 정보다.</summary>
public sealed record RenderSprite
{
    /// <summary>Renderer보다 오래 사는 실제 셰이프 DB다.</summary>
    public ShapeDatabase? Database { get; init; }
    /// <summary>실제 SHP 블록 번호다.</summary>
    public int Block { get; init; }
    /// <summary>첫 물리 프레임 번호다.</summary>
    public int Frame { get; init; }
    /// <summary>화면 X 기준점이다.</summary>
    public int X { get; init; }
    /// <summary>화면 Y 기준점이다.</summary>
    public int Y { get; init; }
    /// <summary>객체 내부 출력 clip이다.</summary>
    public ScreenRect Clip { get; init; }
    /// <summary>원문 깊이/좌표 키다.</summary>
    public DrawOrder Order { get; init; }
    /// <summary>선택적256BYTE 값형 색 치환표다.</summary>
    public byte[]? Colors { get; init; }
    /// <summary>대상 화면 색을 치환하는 그림자 표식이다.</summary>
    public bool Shadow { get; init; }
    /// <summary>별도 정렬 없이 같은 항목 안에서 이어 그릴 프레임들이다.</summary>
    public IReadOnlyList<int> FollowingFrames { get; init; } = [];
    /// <summary>SHP 대신 같은 정렬 위치에서 실행하는 직접 출력이다.</summary>
    public RenderProcedure? Procedure { get; init; }
    /// <summary>실제 raw 자산의 신원이며 다른 출력은0이다.</summary>
    public uint SourceSid { get; init; }
    /// <summary>CPP vector/optional array 값 복사처럼 현재 가변 배열을 별도 보관한다.</summary>
    internal RenderSprite Copy() => this with { Colors = Colors?.ToArray(), FollowingFrames = FollowingFrames.ToArray() };
}

/// <summary>원문 코드페이지 BYTE 글자와 그림자/외곽선/클립 문맥이다.</summary>
public sealed record RenderText
{
    /// <summary>Renderer보다 오래 사는 현재 실제 글꼴이다.</summary>
    public BitmapFont? Font { get; init; }
    /// <summary>명시 길이로 그리므로 중간 NUL도 포함하는 원문 BYTE다.</summary>
    public byte[] Bytes { get; init; } = [];
    /// <summary>화면 X 기준점이다.</summary>
    public int X { get; init; }
    /// <summary>화면 Y 기준점이다.</summary>
    public int Y { get; init; }
    /// <summary>현재 본문 색이며 최초255다.</summary>
    public byte Color { get; init; } = 255;
    /// <summary>그림자와 외곽선의 현재 색이다.</summary>
    public byte ShadowColor { get; init; }
    /// <summary>본문으로부터 그림자를 이동할 signed 픽셀이다.</summary>
    public ScreenPoint ShadowOffset { get; init; }
    /// <summary>그림자 패스를 실행할지다.</summary>
    public bool Shadow { get; init; }
    /// <summary>원문 고정4방향 외곽선 패스를 실행할지다.</summary>
    public bool Outline { get; init; }
    /// <summary>파생 글자 내부 clip이며 미지정이면 화면 clip만 쓴다.</summary>
    public ScreenRect? Clip { get; init; }
    /// <summary>CPP string 값 복사처럼 현재 BYTE를 별도 보관한다.</summary>
    internal RenderText Copy() => this with { Bytes = Bytes.ToArray() };
}
