using System.Buffers.Binary;

// 원문의 signed IDIV와 DWORD 감김·현재 실제 프레임 지정을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawDiscRegular.cpp의 실제 투사체 방향별 modulo 정기 처리기다.</summary>
public sealed class RawDiscRegular
{
    /// <summary>현재 프레임/가상 표를 읽는 같은 물리 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원문 span처럼 빌리는 현재 방향/클러스터 표다.</summary>
    private readonly IReadOnlyList<ArcherTypeData> _types;
    /// <summary>같은 표시/공간을 갱신하는 실제 지정기다.</summary>
    private readonly SquidFrame _setter;
    /// <summary>같은 Pool과 실제 두 판본의 전체 표 폭을 검사한다.</summary>
    public RawDiscRegular(RawSidPool pool, IReadOnlyList<ArcherTypeData> types, SquidFrame setter)
    {
        if (!ReferenceEquals(pool, setter.Pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)) throw new ArgumentException("디스크 정기 Pool/TYPE 판본 불일치"); _pool = pool; _types = types; _setter = setter;
    }
    /// <summary>원문 x86 IDIV의0분모/몫 넘침만 거부한다.</summary>
    private static int Divide(int value, int divisor) { if (divisor == 0 || (value == int.MinValue && divisor == -1)) throw new ArithmeticException("디스크 원문 IDIV 오류"); return value / divisor; }
    /// <summary>나머지도 같은 IDIV 오류를 먼저 판정한다.</summary>
    private static int Remainder(int value, int divisor) { _ = Divide(value, divisor); return value % divisor; }
    /// <summary>원문 정상 입력의 현재 프레임/분모 계약을 검사한다.</summary>
    public static int NextFrame(int current, int frameCount, int directions)
    {
        if (current < 0 || current == int.MaxValue || directions <= 0 || frameCount < directions) throw new ArgumentOutOfRangeException(nameof(current)); return NextFrameNative(current, frameCount, directions);
    }
    /// <summary>세 signed 나눗셈과 원문 LEA/SUB/ADD의 하위DWORD 감김을 보존한다.</summary>
    public static int NextFrameNative(int current, int frameCount, int directions) { int stride = Divide(frameCount, directions), next = unchecked(current + 1); return unchecked(Remainder(next, stride) - Remainder(current, stride) + current); }
    /// <summary>타입 번호가 아닌 실제 VT50b570/CD501440을 선택한다.</summary>
    public bool Handles(int sid) => BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid)) == (_pool.Edition == OriginalEdition.Patch1078 ? 0x50b570u : 0x501440u);
    /// <summary>실제 현재 VT를 확인하고 프레임 지정 뒤 payload 전체 float를 반환한다.</summary>
    public float Handle(int sid, uint ev, uint count, float payload)
    {
        if (!Handles(sid)) throw new InvalidOperationException("디스크 정기 현재 가상 표 미지원"); _ = ev; _ = count; return HandleIn(_pool, _setter, type => type >= _types.Count ? throw new ArgumentOutOfRangeException(nameof(type)) : _types[(int)type], sid, payload);
    }
    /// <summary>현재 TYPE getter 복귀 뒤 raw프레임을 읽고 같은 실제 Frame.Set을 실행한다.</summary>
    public static float HandleIn(RawSidPool pool, SquidFrame setter, Func<uint, ArcherTypeData>? type, int sid, float payload, bool nativeArithmetic = false)
    {
        if (type is null) throw new InvalidOperationException("디스크 정기 현재 TYPE getter 미연결"); ReadOnlySpan<byte> raw = pool.Slot(sid); ArcherTypeData data = type(raw[10]); int current = pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34], next = nativeArithmetic ? NextFrameNative(current, data.FrameCount, data.Directions) : NextFrame(current, data.FrameCount, data.Directions); setter.Set(sid, next, 0); return payload;
    }
    /// <summary>디스크의 실제 VT만 선택하고 다른 부모 입력은 그대로 보존한다.</summary>
    public RegularHandler Handler(RegularHandler? fallback = null) => (sid, ev, count, payload) => Handles(sid) ? Handle(sid, ev, count, payload) : (fallback ?? throw new InvalidOperationException("디스크 외부 정기 부모 미연결"))(sid, ev, count, payload);
}
