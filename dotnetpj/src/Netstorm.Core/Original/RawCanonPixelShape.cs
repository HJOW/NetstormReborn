// 일반 타입·다리·섬·전투 패턴 전체 픽셀 getter의 부동소수 저장 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>현재 프레임 표/패턴/실제 SHP 헤더를 원문 순서로 순회하는 픽셀 범위 계산기다.</summary>
public sealed class RawCanonPixelShape
{
    /// <summary>PATCH frameCheck에서 물리 프레임 수를 두 배로 만드는 플래그다.</summary>
    private const uint DoubleFrames = 0x440000;
    /// <summary>PATCH의 첫 번째 해제된 타입 HP 표식이다.</summary>
    private const int FreedA = -0x22222223;
    /// <summary>PATCH의 두 번째 해제된 타입 HP 표식이다.</summary>
    private const int FreedB = -0x32323233;
    /// <summary>판본과 실제 어댑터 연결을 확인하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>프레임마다 현재 플래그/해제 표식을 읽는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>생성 때 기본 프레임을 저장하고 frameCheck 때 현재 논리 개수를 읽는 메타다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;
    /// <summary>현재 물리 프레임 헤더가 든 실제 SHP 표다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>프레임 기준점에서 뺄 타입의 현재 signed DWORD 좌표다.</summary>
    private readonly IReadOnlyList<PriestTypeHotspot> _hotspots;
    /// <summary>현재 패턴 타입 전역/진단 장치다.</summary>
    private readonly PriestPlacementGeometryState _geometry;
    /// <summary>매 유효 프레임의 계산 때 읽는 배율이다.</summary>
    private readonly PriestPlacementShapeState _state;
    /// <summary>같은 번호 체계의 판본별 자료 길이를 확인하고 실제 표를 대여한다.</summary>
    public RawCanonPixelShape(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, IReadOnlyList<SquidDisplayShape> shapes, IReadOnlyList<PriestTypeHotspot> hotspots, PriestPlacementGeometryState geometryState, PriestPlacementShapeState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(hotspots); ArgumentNullException.ThrowIfNull(geometryState); ArgumentNullException.ThrowIfNull(state);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || frames.Count != types.Count || shapes.Count != types.Count || hotspots.Count != types.Count) throw new ArgumentException("일반 픽셀 모양 타입/메타/SHP/기준점 자료 누락");
        _pool = pool; _types = types; _frames = frames; _shapes = shapes; _hotspots = hotspots; _geometry = geometryState; _state = state;
    }
    /// <summary>원문 low DWORD 뺄셈을 signed 넘침 예외 없이 보존한다.</summary>
    private static int Difference(short header, int type) => unchecked((int)((uint)(int)header - (uint)type));
    /// <summary>PATCH 논리 frameCheck와 해제 표식을 확인하며 CD는 물리 접근 범위만 확인한다.</summary>
    private SquidDisplayFrame Header(uint type, int frame)
    {
        SquidDisplayShape shape = _shapes[(int)type]; OriginalTypeRecord record = _types[(int)type];
        if (!shape.Loaded || frame < 0 || frame >= shape.Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame), "일반 픽셀 모양 SHP 물리 프레임 조회 실패");
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            ulong count = (ulong)_frames[(int)type].Frames.Codes.Count * ((record.Flags1 & DoubleFrames) != 0 ? 2u : 1u);
            if ((ulong)frame >= count || record.MaxHitPoints == FreedA || record.MaxHitPoints == FreedB) throw new ArgumentOutOfRangeException(nameof(frame), "일반 픽셀 모양 frameCheck 실패(00419850)");
        }
        return shape.Frames[frame];
    }
    /// <summary>decoder의 모든 유효 프레임을 순회하며 판본별 중간 float·동률 저장·기준점을 보존한다.</summary>
    public PriestPlacementPixelShape Measure(uint type, uint argument, uint direction, bool explicitFrame = false)
    {
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(type), "일반 픽셀 모양 타입 번호 오류");
        PriestPlainCanonType meta = _frames[(int)type];
        CanonDecoder decoder = CanonTypeDecoder.DecodeCanonType(meta.Frames, meta.DefaultFrame, _geometry.PatternTypes, new(type, unchecked((int)argument), unchecked((int)direction), 0, 0, explicitFrame), _pool.Edition, _geometry.DecoderDiagnostics);
        float leftBound = 1000, topBound = 1000, rightBound = 0, bottomBound = 0, anchorX = _state.ScaleX, anchorY = _state.ScaleY;
        // 빈 칸을 건너뛴 반복자의 현재 모든 프레임을 원문 Valid/Advance 순서로 합친다.
        while (decoder.Valid())
        {
            if (!float.IsFinite(_state.ScaleX) || !float.IsFinite(_state.ScaleY)) throw new ArgumentException("일반 픽셀 모양 배율의 비유한 값");
            SquidDisplayFrame header = Header(type, decoder.Frame()); PriestTypeHotspot hot = _hotspots[(int)type];
            int dx = Difference(header.HotspotX, hot.X), dy = Difference(header.HotspotY, hot.Y); bool patch = _pool.Edition == OriginalEdition.Patch1078;
            double offsetX = patch ? (double)dx : (double)(float)dx; float offsetY = dy;
            double pixelX = patch ? (double)decoder.X() * _state.ScaleX : (float)((double)decoder.X() * _state.ScaleX);
            float pixelY = (float)((double)decoder.Y() * _state.ScaleY);
            double leftWide = pixelX - offsetX, topWide = (double)pixelY - offsetY; float left = (float)leftWide, top = (float)topWide;
            if (leftWide < leftBound) anchorX = dx;
            if (topWide < topBound) anchorY = offsetY;
            if (left <= leftBound) leftBound = left;
            if (top <= topBound) topBound = top;
            double right = patch ? (double)left + header.Width : ((double)header.Width - offsetX) + pixelX;
            double bottom = patch ? topWide + header.Height : ((double)header.Height - offsetY) + pixelY;
            if (patch ? rightBound <= right : rightBound < right) rightBound = (float)right;
            if (patch ? bottomBound <= bottom : bottomBound < bottom) bottomBound = (float)bottom;
            decoder.Advance();
        }
        return new(new(leftBound, topBound, rightBound, bottomBound), anchorX, anchorY);
    }
    /// <summary>배치 접두와 같은 실제 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;
}
