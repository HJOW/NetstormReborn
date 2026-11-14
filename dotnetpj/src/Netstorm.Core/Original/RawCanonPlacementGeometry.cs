// 일반 타입/패턴 배치의 현재 발자국·실제 decoder 순회와 finder 사각형을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>칸 순회 전후의 실제 지역 정책을 연결하는 함수 묶음이다.</summary>
public sealed class CanonPlacementGeometryHooks
{
    /// <summary>decoder 생성/첫 선택 뒤 한 번 수행하는 지역 초기화다.</summary>
    public Action<CanonTypeQuery>? BeginRegions { get; set; }
    /// <summary>각 finder 호출 직전 수행하는 선택적 현재 모양 준비다.</summary>
    public Action<CanonTypeQuery, CanonPlacementCell>? BeginShape { get; set; }
    /// <summary>후보 소진 뒤 현재 모양의 지형/관계를 판정한다.</summary>
    public Func<CanonTypeQuery, CanonPlacementCell, bool>? EndShape { get; set; }
    /// <summary>빈 decoder를 포함해 모든 칸 소진 뒤 최종 관계를 판정한다.</summary>
    public Func<CanonTypeQuery, bool>? FinishRegions { get; set; }
}

/// <summary>현재 타입/프레임과 실제 CanonDecoder로 정수 범위와 칸별 finder 인자를 계산한다.</summary>
public sealed class RawCanonPlacementGeometry
{
    /// <summary>오른쪽/아래쪽 finder 인자에 더하는 원문 단정도 보정값이다.</summary>
    private const float NearCeil = 0.99999f;
    /// <summary>판본과 어댑터 동일성을 확인하는 원래 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>칸마다 현재 발자국을 읽는 실제 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>생성 시 첫 프레임과 칸별 현재 방향 코드를 읽는 메타 표다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;
    /// <summary>현재 패턴 타입 전역과 공유 checking 진단이다.</summary>
    private readonly PriestPlacementGeometryState _state;
    /// <summary>같은 번호 체계의 판본별 타입/프레임 자료를 빌린다.</summary>
    public RawCanonPlacementGeometry(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, PriestPlacementGeometryState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(state);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || frames.Count != types.Count) throw new ArgumentException("일반 배치 모양 타입/프레임 자료 누락");
        _pool = pool; _types = types; _frames = frames; _state = state;
    }
    /// <summary>현재 메타와 패턴 타입의 우선순위를 원문 생성자에 전달한다.</summary>
    private CanonDecoder Decode(CanonTypeQuery query)
    {
        if (query.Type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(query), "일반 배치 모양 타입 번호 오류");
        PriestPlainCanonType meta = _frames[(int)query.Type]; return CanonTypeDecoder.DecodeCanonType(meta.Frames, meta.DefaultFrame, _state.PatternTypes, query, _pool.Edition, _state.DecoderDiagnostics);
    }
    /// <summary>빈 decoder에서도 첫 현재 좌표와 전체 칸 수로 원문 미리보기 범위를 계산한다.</summary>
    public SquidSearchArea Bounds(CanonTypeQuery query)
    {
        CanonDecoder decoder = Decode(query); OriginalTypeRecord type = _types[(int)query.Type]; int[] area = decoder.Bounds(type.FootX, type.FootY); return new(area[0], area[1], area[2], area[3]);
    }
    /// <summary>CRT처럼 먼저 절삭한 뒤 정수 범위를 확인해 비유한/범위 밖 입력을 진단한다.</summary>
    private static int Truncate(double value)
    {
        value = Math.Truncate(value);
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(value), "배치 모양 좌표 절삭 범위 오류");
        return (int)value;
    }
    /// <summary>왼쪽 위는 snapped 좌표, 오른쪽 아래는 원래 소수 좌표와 넓은 정밀도 합으로 계산한다.</summary>
    public static SquidSearchArea CollisionArea(float x, float y, int footX, int footY)
    {
        if (footX < 0 || footY < 0) throw new ArgumentOutOfRangeException(nameof(footX), "배치 모양 발자국 오류");
        float snappedX = Truncate(x), snappedY = Truncate(y);
        return new(Truncate((double)snappedX - ((long)footX - 1)), Truncate((double)snappedY - ((long)footY - 1)), Truncate((double)x + NearCeil), Truncate((double)y + NearCeil));
    }
    /// <summary>생성→초기화→각 칸 준비/scan/종료→Advance→최종 정책이며 거부 즉시 중단한다.</summary>
    public bool Inspect(CanonTypeQuery query, Func<CanonPlacementCell, bool> scan, CanonPlacementGeometryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (scan is null || hooks.BeginRegions is null || hooks.EndShape is null || hooks.FinishRegions is null) throw new ArgumentException("일반 배치 모양 finder/지역 경계 누락");
        CanonDecoder decoder = Decode(query); hooks.BeginRegions(query);
        // 각 칸에서 현재 발자국/코드를 다시 읽되 decoder 생성 때 저장한 첫 선택은 보존한다.
        while (decoder.Valid())
        {
            OriginalTypeRecord type = _types[(int)query.Type]; float x = decoder.X(), y = decoder.Y(); int frame = decoder.Frame(); IReadOnlyList<RiftFrameCode> codes = _frames[(int)query.Type].Frames.Codes;
            byte side = frame >= 0 && frame < codes.Count ? codes[frame].Side : (byte)0;
            var cell = new CanonPlacementCell(frame, decoder.Label(), side, x, y, Truncate(x), Truncate(y), CollisionArea(x, y, type.FootX, type.FootY));
            hooks.BeginShape?.Invoke(query, cell);
            if (!scan(cell) || !hooks.EndShape(query, cell)) return false;
            decoder.Advance();
        }
        return hooks.FinishRegions(query);
    }
    /// <summary>동일 판본/풀에 실제 장치를 연결하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
}
