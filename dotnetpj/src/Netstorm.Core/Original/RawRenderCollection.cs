using System.Buffers.Binary;

// 공통 표시 수집은 같은 목록/부모 raw/배율의 현재 값을 읽는다.
namespace Netstorm.Core.Original;

/// <summary>화면 픽셀과 구별하는 월드 사각형이다.</summary>
/// <param name="Left">왼쪽 좌표다.</param>
/// <param name="Top">위쪽 좌표다.</param>
/// <param name="Right">오른쪽 좌표다.</param>
/// <param name="Bottom">아래쪽 좌표다.</param>
public readonly record struct RenderWorldRect(float Left, float Top, float Right, float Bottom);

/// <summary>원문 표시 영역 환산·엄격 교차·kind3 출력 수집기다. 정렬/그리기는 소비자가 맡는다.</summary>
public sealed class RawRenderCollection
{
    /// <summary>현재 판본과 부모 바이트를 공유한다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 좌표 배율 owner다.</summary>
    private readonly ConstructionVisualState _state;
    /// <summary>빈 칸을 보존하는 공통 등록 목록이다.</summary>
    private readonly RawRenderRegistry _registry;
    /// <summary>생성 중이면 null을 돌려주는 현재 토큰 조회다.</summary>
    private readonly Func<uint, RenderProcessFields?> _lookup;

    /// <summary>현재 목록/부모가 같은 풀인지 확인하고 함수 사본을 빌린다.</summary>
    public RawRenderCollection(RawSidPool pool, ConstructionVisualState state, RawRenderRegistry registry, Func<uint, RenderProcessFields?> lookup)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(lookup);
        if (!ReferenceEquals(pool, registry.Pool)) throw new ArgumentException("표시 수집 연결 오류");
        _pool = pool; _state = state; _registry = registry; _lookup = lookup;
    }

    /// <summary>부모 범위/head만 검사하며 free/dead/void 등 추가 필터는 만들지 않는다.</summary>
    private ReadOnlySpan<byte> Parent(RenderProcessFields fields)
    {
        if (fields.Parent < 0 || fields.Parent >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields), "표시 수집 부모 범위 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(fields.Parent);
        if ((raw[6] | raw[7]) == 0) throw new InvalidOperationException("표시 수집 부모 head가 비었습니다");
        return raw;
    }

    /// <summary>PATCH는 반올림 전 double역수 곱셈, CD는 직접 나눗셈 뒤 float로 저장한다.</summary>
    public RenderWorldRect Bounds(RenderProcessFields fields)
    {
        _ = Parent(fields);
        if (!float.IsFinite(_state.ScaleX) || !float.IsFinite(_state.ScaleY) || _state.ScaleX == 0 || _state.ScaleY == 0)
            throw new ArgumentException("표시 수집 배율 오류");
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        double x = 1.0 / _state.ScaleX, y = 1.0 / _state.ScaleY;
        return new((float)(patch ? x * fields.Left : (double)fields.Left / _state.ScaleX),
            (float)(patch ? y * fields.Top : (double)fields.Top / _state.ScaleY),
            (float)(patch ? x * fields.Right : (double)fields.Right / _state.ScaleX),
            (float)(patch ? y * fields.Bottom : (double)fields.Bottom / _state.ScaleY));
    }

    /// <summary>경계만 닿으면 제외한다. CD는 원문 x87 unordered 분기 차이를 유지한다.</summary>
    public static bool Intersects(RenderWorldRect view, RenderWorldRect bounds, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        if (edition != OriginalEdition.Patch1078)
            return view.Left <= view.Right && !(view.Top > view.Bottom) && bounds.Left <= bounds.Right && !(bounds.Top > bounds.Bottom)
                && !(view.Left >= bounds.Right) && bounds.Left < view.Right && !(view.Top >= bounds.Bottom) && bounds.Top < view.Bottom;
        return view.Left <= view.Right && view.Top <= view.Bottom && bounds.Left <= bounds.Right && bounds.Top <= bounds.Bottom
            && view.Left < bounds.Right && bounds.Left < view.Right && view.Top < bounds.Bottom && bounds.Top < view.Bottom;
    }

    /// <summary>목록 순서로 limit까지 쓰며 미사용 항목/마지막 Tail은 보존하고 limit이하0이면 조회하지 않는다.</summary>
    public int Collect(RenderWorldRect view, Span<RenderCollectionEntry> output, int limit)
    {
        if (limit > output.Length) throw new ArgumentException("표시 수집 출력 용량 오류");
        int count = 0;
        // 수집은 등록 배열의 원문 순서이며 콜백은 배열을 변경하지 않아야 한다.
        foreach (uint token in _registry.Slots)
        {
            if (count >= limit) return count;
            if (token == 0) continue;
            RenderProcessFields? fields = _lookup(token);
            if (fields is null || !Intersects(view, Bounds(fields), _pool.Edition)) continue;
            ReadOnlySpan<byte> parent = Parent(fields);
            RenderCollectionEntry previous = output[count];
            output[count++] = new(BinaryPrimitives.ReadSingleLittleEndian(parent[14..]),
                (float)((double)BinaryPrimitives.ReadSingleLittleEndian(parent[18..]) + BitConverter.UInt32BitsToSingle(0x3a83126f)),
                token, unchecked((short)fields.ZOrder), 3, previous.Tail);
        }
        return count;
    }
}
