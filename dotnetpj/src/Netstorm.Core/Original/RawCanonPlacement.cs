// 일반 타입/패턴의 즉시 허용·현재 소유자 캡처·판본별 픽셀 여백을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>일반 배치의 모양 조회와 전체 후반 정책 경계다.</summary>
public sealed class CanonPlacementHooks
{
    /// <summary>별도 타입/패턴 인자/방향을 실제 전체 픽셀 getter에 전달한다.</summary>
    public Func<uint, uint, uint, PriestPlacementRect>? Shape { get; set; }
    /// <summary>지도 여백을 통과한 원래 요청과 저장한 로컬 비교로 후반 정책을 처리한다.</summary>
    public Func<CanonPlacementQuery, bool, bool>? InspectGeometry { get; set; }
    /// <summary>원문 함수 객체의 값 사본을 만든다.</summary>
    internal CanonPlacementHooks Copy() => new() { Shape = Shape, InspectGeometry = InspectGeometry };
}

/// <summary>0049b510/CD00445200의 일반 배치 접두와 실제 픽셀 모양 어댑터다.</summary>
public sealed class RawCanonPlacement
{
    /// <summary>현재 TYPE에서 즉시 허용하는 비트다.</summary>
    private const uint Anywhere = 0x02000000;
    /// <summary>픽셀 가로 차이를 지도 여백으로 바꾸는 원문 float다.</summary>
    private const float HorizontalScale = 0.0625f;
    /// <summary>픽셀 세로 차이를 지도 여백으로 바꾸는 원문 float다.</summary>
    private const float VerticalScale = 0.09090909361839294f;
    /// <summary>오른쪽/아래쪽 경계를 검사하는 지도 한 변이다.</summary>
    private const float MapSide = 256;
    /// <summary>판본과 실제 장치 연결을 판별하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>각 진입 때 읽는 현재 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>사제 배치와 공유하는 현재 전역이다.</summary>
    private readonly PriestPlacementState _state;
    /// <summary>구성 때 복사한 모양/후반 함수다.</summary>
    private readonly CanonPlacementHooks _hooks;
    /// <summary>판본별 타입 길이와 필수 두 효과를 먼저 확인한다.</summary>
    public RawCanonPlacement(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, PriestPlacementState state, CanonPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.Shape is null || hooks.InspectGeometry is null) throw new ArgumentException("일반 배치 자료/하위 경계 누락");
        _pool = pool; _types = types; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>차단 관계 초기화→현재 강제 허용→소유자 캡처→픽셀 여백→후반 정책 순서다.</summary>
    public bool MayPlace(CanonPlacementQuery query)
    {
        if (query.Type < OriginalConstructors.FirstAssetType || query.Type >= _types.Count) throw new ArgumentException("일반 배치 타입 범위 오류");
        if (!float.IsFinite(query.X) || !float.IsFinite(query.Y)) throw new ArgumentException("일반 배치의 비유한 지도 좌표");
        _state.BlockedRelation = 0;
        if (_state.ReadForcePlacement() != 0 || (_types[(int)query.Type].Flags2 & Anywhere) != 0) return true;
        int owner = unchecked((sbyte)(byte)query.Owner); bool localOwner = owner == _state.LocalPlayer;
        float margin = Margin(_pool.Edition, _hooks.Shape!(query.Type, query.Argument, query.Flags));
        if (query.X < margin || (double)MapSide - query.X < margin || query.Y < margin || (double)MapSide - query.Y < margin) return false;
        return _hooks.InspectGeometry!(query, localOwner);
    }
    /// <summary>PATCH는 가로 차의 double 정밀도를 유지하고 CD는 두 합을 각각 float로 저장한다.</summary>
    private static float Margin(OriginalEdition edition, PriestPlacementRect rect)
    {
        if (!float.IsFinite(rect.Left) || !float.IsFinite(rect.Top) || !float.IsFinite(rect.Right) || !float.IsFinite(rect.Bottom)) throw new ArgumentException("일반 배치 모양의 비유한 좌표");
        double horizontal; float vertical;
        if (edition == OriginalEdition.Patch1078) { horizontal = ((double)rect.Right - rect.Left) * HorizontalScale; vertical = (float)(((double)rect.Bottom - rect.Top) * VerticalScale); }
        else { vertical = (float)((double)rect.Top * -VerticalScale + (double)rect.Bottom * VerticalScale); horizontal = (float)((double)rect.Left * -HorizontalScale + (double)rect.Right * HorizontalScale); }
        double value = horizontal > vertical ? horizontal : vertical;
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(rect), "일반 배치 여백의 정수 범위 오류");
        return (int)value;
    }
    /// <summary>픽셀 getter와 동일한 실제 풀을 확인하는 참조다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>명시 프레임을 끈 실제 전체 픽셀 getter만 연결하고 후반 정책 사본은 보존한다.</summary>
    public static CanonPlacementHooks MakeCanonShapePlacementHooks(RawSidPool pool, RawCanonPixelShape shape, CanonPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(shape); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, shape.Pool)) throw new ArgumentException("일반 픽셀 모양/배치의 SID 풀이 다릅니다");
        CanonPlacementHooks result = hooks.Copy(); result.Shape = (type, argument, direction) => shape.Measure(type, argument, direction).Bounds; return result;
    }
}
