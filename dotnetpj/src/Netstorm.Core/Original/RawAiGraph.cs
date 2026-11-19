// 원본 배치 기준점의 SID·좌표→그래프 번호 조회를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 그래프 조회 모듈을 담는 네임스페이스다. 2026-11-19 0.657에서 R4-5 AI 그래프용으로 추가했다.

// 그래프 활성·예외 타입·진단 장치를 연결한다.
public sealed class AiGraphHooks
{
    /// <summary>그래프 활성 여부다. 꺼져 있으면 좌표를 읽지 않고 무효 값을 돌려준다. 0.657에서 추가했다.</summary>
    public Func<bool>? GraphReady { get; set; }

    /// <summary>표면 이외에 허용하는 펜스 타입 번호다. 0.657에서 추가했다.</summary>
    public Func<uint>? FenceType { get; set; }

    /// <summary>표면 이외에 허용하는 윈드아처 타입 번호다. 0.657에서 추가했다.</summary>
    public Func<uint>? WindArcherType { get; set; }

    /// <summary>SID 0 조회의 원본 진단이다. 0.657에서 추가했다.</summary>
    public Action<string, string, uint>? GraphAssertion { get; set; }

    /// <summary>죽은 부모 조회 진단의 현재 전체 DWORD와 진단 소비자다. 0.657에서 추가했다.</summary>
    public ContainedFinderState? Contained { get; set; }
}

/// <summary>00462b80의 좌표→그래프와 00462bc0의 SID→그래프를 같은 순서로 수행한다.
/// SID 0은 surface raw 번호 또는 비표면 좌표 조회이며, surface 타입은 활성 여부와 무관하게 자신의 raw 번호를 쓴다.</summary>
/// <remarks>2026-11-19 0.657에서 추가했다. 근거: cpppj <c>o/RawPlayerPlacementAnchor.h/.cpp</c>의 <c>GraphAt</c>·<c>GraphFor</c>다.</remarks>
public sealed class RawAiGraph
{
    /// <summary>원본 fadd DWORD 상수다. 0.657에서 추가했다.</summary>
    private const float CoordinateBias = 0.9999f;

    /// <summary>슬롯 안 타입 번호 바이트의 위치다. 0.657에서 추가했다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 X 좌표 float의 위치다. 0.657에서 추가했다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 Y 좌표 float의 위치다. 0.657에서 추가했다.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 그래프 바이트(PATCH)의 위치다. 0.657에서 추가했다.</summary>
    private const int GraphOffset = 30;

    /// <summary>슬롯 안 그래프 바이트(CD)의 위치다. 0.657에서 추가했다.</summary>
    private const int CdGraphOffset = 28;

    /// <summary>표면 판정의 타입 플래그 비트다. 0.657에서 추가했다.</summary>
    private const uint SurfaceBit = 0x800u;

    /// <summary>그래프 무효 번호다. 0.657에서 추가했다.</summary>
    private const uint InvalidGraph = 254u;

    /// <summary>같은 실제 풀·타입 표·표면 지도를 빌린다. 0.657에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성을 조회한다. 0.657에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 hash0 원문(65536 WORD)이다. 0.657에서 추가했다.</summary>
    private readonly ushort[] _surfaces;

    /// <summary>선택 위치의 실제 중심·장치·진단 경계다. 0.657에서 추가했다.</summary>
    private readonly AiGraphHooks _hooks;

    /// <summary>AI 그래프 조회 본체를 만든다. 같은 PATCH/CD 풀·타입·표면 계약을 확인한다.</summary>
    public RawAiGraph(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ushort[] surfaces, AiGraphHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(hooks);
        if (surfaces.Length != 65536 || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("AI 그래프 조회의 같은 판본 TYPE·표면65536 누락", nameof(pool));
        }

        _pool = pool;
        _types = types;
        _surfaces = surfaces;
        _hooks = hooks;
    }

    /// <summary>00462b80: 그래프 비활성은 좌표를 읽기 전에 고정 무효 값을 돌려준다. 0.657에서 추가했다.</summary>
    public uint GraphAt(float x, float y)
    {
        if (!Need(_hooks.GraphReady)())
        {
            return InvalidGraph;
        }

        int px = Truncate(x), py = Truncate(y);
        if (px < 0 || py < 0 || px > 255 || py > 255)
        {
            return InvalidGraph;
        }

        uint sid = _surfaces[py * 256 + px];
        return sid != 0 ? RawGraphByte(sid) : InvalidGraph;
    }

    /// <summary>00462bc0: SID 0은 surface raw 번호 또는 비표면 좌표 조회다. 0.657에서 추가했다.</summary>
    public uint GraphFor(uint sid)
    {
        if (sid == 0)
        {
            if (_pool.Edition == OriginalEdition.Patch1078 && Need(_hooks.Contained).CheckingDead.IsEnabled)
            {
                Need(_hooks.GraphAssertion)("mySid", "\\Ns\\O\\Graph.cpp", 0x93);
            }

            return 0;
        }

        ReadOnlySpan<byte> raw = Raw(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 그래프 타입 오류");
        }

        return (_types[raw[TypeOffset]].Flags1 & SurfaceBit) != 0
            ? RawGraphByte(sid)
            : GraphAt(Coordinate(raw, XOffset), Coordinate(raw, YOffset));
    }

    /// <summary>후속 실제 월드 조립 확인용 동일 풀이다. 0.657에서 추가했다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>WORD SID로 좁히기 전에 실제 풀과 번호의 물리 범위를 검사한다. 0.657에서 추가했다.</summary>
    private ReadOnlySpan<byte> Raw(uint sid)
    {
        if (sid > ushort.MaxValue || sid >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 그래프 SID 오류");
        }

        return _pool.Slot(unchecked((int)sid));
    }

    /// <summary>CD getter는 타입 assert 없이 raw graph를 읽으며 패치만 현재 타입 조건을 확인한다. 0.657에서 추가했다.</summary>
    private uint RawGraphByte(uint sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch && (raw[TypeOffset] >= _types.Count
            || ((_types[raw[TypeOffset]].Flags1 & SurfaceBit) == 0
                && raw[TypeOffset] != Need(_hooks.FenceType)()
                && raw[TypeOffset] != Need(_hooks.WindArcherType)())))
        {
            throw new ArgumentException("AI 표면 graph getter 타입 오류", nameof(sid));
        }

        return raw[patch ? GraphOffset : CdGraphOffset];
    }

    /// <summary>비정렬 raw 좌표의 little endian 비트값을 읽는다. 0.657에서 추가했다.</summary>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4)));

    /// <summary>x87의 좌표+float 상수는 double에서 계산한 뒤 float으로 좁히지 않고 CRT 절삭한다. 0.657에서 추가했다.</summary>
    private static int Truncate(float value)
    {
        double result = Math.Truncate((double)value + CoordinateBias);
        if (!double.IsFinite(result) || result < int.MinValue || result > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 그래프 좌표 절삭 오류");
        }

        return (int)result;
    }

    /// <summary>선택되지 않은 하위를 미리 요구하지 않고 도달한 경계에서만 거부한다. 0.657에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 그래프 조회 실제 하위 미연결");
}
