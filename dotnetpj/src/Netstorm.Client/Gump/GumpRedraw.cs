using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 같은 현재 UI의 기본 갱신 영역을 실제 Renderer에 전달한다.

/// <summary>현재 노드/지도/영역 요청의 실제 장치다.</summary>
public sealed class GumpRedrawHooks
{
    /// <summary>같은 공통 트리의 현재 기본 주소다.</summary>
    public required Func<uint, GumpCollectionNode?> Node { get; init; }
    /// <summary>위치 조회 뒤에 읽는 실제 현재 지도 영역이다.</summary>
    public required Func<ScreenRect> Map { get; init; }
    /// <summary>원래 사각형/flags를 받는 실제 변경 영역 장치다.</summary>
    public required Action<ScreenRect, uint> Invalidate { get; init; }
}

/// <summary>cpppj/src/client/GumpRedraw.cpp의 Whole/Map과 원본 좌표 제한 본체다.</summary>
public sealed class GumpRedraw
{
    /// <summary>손상 조상 순환을 거부하는 원본 공통 등록 한도다.</summary>
    private const int ParentLimit = 1000;
    /// <summary>같은 현재 위치/경계와 캐시를 사용하는 수집기다.</summary>
    private readonly GumpCollection _collection;
    /// <summary>현재 외부 장치의 함수 슬롯이다.</summary>
    private readonly GumpRedrawHooks _hooks;
    /// <summary>현재 실제 장치 누락을 빈 성공으로 바꾸지 않는다.</summary>
    public GumpRedraw(GumpCollection collection, GumpRedrawHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null || hooks.Map is null || hooks.Invalidate is null) throw new ArgumentException("Gump 갱신 현재 장치 누락", nameof(hooks));
        _collection = collection; _hooks = hooks;
    }
    /// <summary>원본 signed64 ftol의 하위DWORD이며 integer-indefinite의 하위값은0이다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>역전 지도도 lower를 남기는 원문 비교 순서로 각 좌표를 제한한다.</summary>
    public static ScreenRect ClampRectangle(ScreenRect rectangle, ScreenRect bounds)
    {
        return new(Math.Max(bounds.Left, Math.Min(rectangle.Left, bounds.Right)), Math.Max(bounds.Top, Math.Min(rectangle.Top, bounds.Bottom)), Math.Max(bounds.Left, Math.Min(rectangle.Right, bounds.Right)), Math.Max(bounds.Top, Math.Min(rectangle.Bottom, bounds.Bottom)));
    }
    /// <summary>현재 살아 있는 같은 기본 주소를 요구한다.</summary>
    private GumpCollectionNode Node(uint token) => (token != 0 ? _hooks.Node(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "Gump 갱신 노드 부재");
    /// <summary>자기/모든 조상의 bit1만 확인하며 숨은 조상에서는 위치와 캐시를 조회하지 않는다.</summary>
    private bool Visible(uint token)
    {
        var seen = new HashSet<uint>();
        // 매번 현재 parent를 읽고 정상 방문 순서는 보존한다.
        while (true)
        {
            if (seen.Count >= ParentLimit || !seen.Add(token)) throw new InvalidOperationException("Gump 갱신 조상 순환/한도 초과");
            var node = Node(token); if ((node.Flags & 1) == 0) return false; if (node.Parent == 0) return true; token = node.Parent;
        }
    }
    /// <summary>00464a50→00499370. Position 뒤 실제 폭/높이를 각각 ftol해 양수 XYWH만 flags0으로 요청한다. 인수는 원문처럼 미사용이다.</summary>
    public void Whole(uint token, uint argument = 0)
    {
        if (!Visible(token)) return;
        var position = _collection.Position(token); var node = Node(token); int width = Ftol(node.Width), height = Ftol(node.Height);
        if (width > 0 && height > 0) _hooks.Invalidate(new(position.X, position.Y, unchecked(position.X + width), unchecked(position.Y + height)), 0);
    }
    /// <summary>00464ab0→00499310. 실제 Bounds 뒤 현재 지도를 읽고 제한한 양수 영역만 요청한다.</summary>
    public void Map(uint token, uint argument = 0)
    {
        if (!Visible(token)) return;
        var bounds = _collection.Bounds(token); var rectangle = ClampRectangle(bounds, _hooks.Map());
        if (rectangle.Right != rectangle.Left && unchecked(rectangle.Right - rectangle.Left) >= 0 && rectangle.Bottom != rectangle.Top && unchecked(rectangle.Bottom - rectangle.Top) >= 0) _hooks.Invalidate(rectangle, 0);
    }
}
