// 사제 비패턴 모양 계약과 현재 타입 기준점·SHP·진단을 일반 픽셀 getter에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>타입의 원문 signed DWORD 기준점이다.</summary>
/// <param name="X">타입 가로 기준점이다.</param>
/// <param name="Y">타입 세로 기준점이다.</param>
public readonly record struct PriestTypeHotspot(int X, int Y);

/// <summary>일반/사제 픽셀 모양이 매 유효 프레임에서 읽는 현재 배율이다.</summary>
public sealed class PriestPlacementShapeState
{
    /// <summary>지도 셀 가로를 픽셀로 바꾸는 현재 배율이다.</summary>
    public float ScaleX { get; set; } = 16;
    /// <summary>지도 셀 세로를 픽셀로 바꾸는 현재 배율이다.</summary>
    public float ScaleY { get; set; } = 11;
}

/// <summary>픽셀 getter의 사각형과 최솟값 갱신 프레임 기준점이다.</summary>
/// <param name="Bounds">빈 decoder면 (1000,1000,0,0)을 보존한다.</param>
/// <param name="AnchorX">가로 최솟값을 엄격히 갱신한 기준점 또는 초기 가로 배율이다.</param>
/// <param name="AnchorY">세로 최솟값을 엄격히 갱신한 기준점 또는 초기 세로 배율이다.</param>
public readonly record struct PriestPlacementPixelShape(PriestPlacementRect Bounds, float AnchorX, float AnchorY);

/// <summary>기하판정과 픽셀 모양에 공유하는 현재 타입 전역과 decoder 진단이다. 기하 본체는 별도다.</summary>
public sealed class PriestPlacementGeometryState
{
    /// <summary>puzzlePiece/bridge/island/noIsland/네 전투 타입 순서인 고정8 현재 전역이다.</summary>
    public uint[] PatternTypes { get; } = new uint[8];
    /// <summary>반복자 생성이 빌리는 같은 현재 checking/단언 장치다.</summary>
    public CanonDecoderDiagnostics DecoderDiagnostics { get; } = new();
}

/// <summary>사제 genus/비패턴 계약을 유지하며 일반 전체 픽셀 getter를 호출한다.</summary>
public sealed class RawPriestPlacementShape
{
    /// <summary>판본과 다른 장치의 동일성을 확인하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>각 호출 때 사제 genus를 다시 읽는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 메타/SHP/배율을 빌리는 일반 계산기다.</summary>
    private readonly RawCanonPixelShape _shape;
    /// <summary>사제 비패턴 계약을 확인하는 현재 타입 전역이다.</summary>
    private readonly PriestPlacementGeometryState _geometry;
    /// <summary>타입 번호 체계가 같은 메타/모양/기준점을 실제 수명 동안 빌린다.</summary>
    public RawPriestPlacementShape(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, IReadOnlyList<SquidDisplayShape> shapes, IReadOnlyList<PriestTypeHotspot> hotspots, PriestPlacementGeometryState geometryState, PriestPlacementShapeState state)
    {
        _shape = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, state); _pool = pool; _types = types; _geometry = geometryState;
    }
    /// <summary>현재 사제 genus와 패턴 별칭을 확인하고 원래 값 인자로 픽셀 범위를 계산한다.</summary>
    public PriestPlacementPixelShape Measure(uint type, uint argument, uint direction, bool explicitFrame = false)
    {
        if (type >= _types.Count || (_types[(int)type].Flags2 & 0x200000) == 0 || Array.IndexOf(_geometry.PatternTypes, type) >= 0) throw new ArgumentException("사제 픽셀 모양 비패턴 타입 계약 오류");
        return _shape.Measure(type, argument, direction, explicitFrame);
    }
    /// <summary>배치 접두와 동일 풀 연결을 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>사제 모양 경계만 명시 프레임 없는 실제 getter로 바꾸고 후반 사본은 보존한다.</summary>
    public static PriestPlacementHooks MakePriestShapePlacementHooks(RawSidPool pool, RawPriestPlacementShape shape, PriestPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(shape); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, shape.Pool)) throw new ArgumentException("사제 픽셀 모양/배치의 SID 풀이 다릅니다");
        PriestPlacementHooks result = hooks.Copy(); result.Shape = (type, argument, direction) => shape.Measure(type, argument, direction).Bounds; return result;
    }
}
