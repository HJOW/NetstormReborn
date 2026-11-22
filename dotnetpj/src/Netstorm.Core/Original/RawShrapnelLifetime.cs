using System.Buffers.Binary;

// 부모0등록→더미/부착→현재캡처/준비와반납순서를공유한다.
namespace Netstorm.Core.Original;

/// <summary>실제 소멸자가 구별하는 소유 메모리다.</summary>
public enum ShrapnelRelease
{
    /// <summary>캡처한 원본 bitmap이다.</summary>
    Source,
    /// <summary>용량 전체 입자 저장소다.</summary>
    Particles,
    /// <summary>flags bit0이 요청한 객체다.</summary>
    Object,
}

/// <summary>같은 월드·Host·캡처·계산을 잇는 실제 외부 경계다.</summary>
public sealed class ShrapnelLifetimeHooks
{
    /// <summary>wrapper의104바이트 메모리 요청이다.</summary>
    public Func<uint, bool>? Reserve { get; set; }
    /// <summary>TYPE110/flags2의client더미를생성한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>생성뒤현재source좌표로더미를Pop한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>부모0등록뒤실제process16을부착한다.</summary>
    public Action<RenderProcessFields, uint, int, uint>? Attach { get; set; }
    /// <summary>같은Form/더미의상호감시쌍을만든다.</summary>
    public Action<int, int>? Depend { get; set; }
    /// <summary>현재source의plain/mapped색그림을캡처한다.</summary>
    public Func<int, ConstructionVisualImage>? Capture { get; set; }
    /// <summary>같은두난수와시각의실제Prepare를호출한다.</summary>
    public Action<ShrapnelFrameFields, ShrapnelKind>? Prepare { get; set; }
    /// <summary>경계함수는값복사하고같은장치의수명은빌린다.</summary>
    public ShrapnelLifetimeHooks Copy() => new() { Reserve = Reserve, Create = Create, Pop = Pop, Attach = Attach, Depend = Depend, Capture = Capture, Prepare = Prepare };
}

/// <summary>PATCH004604a0·세파생/공통소멸의실제몸체다.</summary>
public sealed class RawShrapnelLifetime
{
    /// <summary>현재source좌표/깊이를재조회하는같은풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>모든효과가공유하는같은원본목록이다.</summary>
    private readonly RawRenderRegistry _registry;
    /// <summary>실제외부경계의함수사본이다.</summary>
    private readonly ShrapnelLifetimeHooks _hooks;
    /// <summary>세가상생성자가저장하는정확한binary32중력이다.</summary>
    private static readonly uint[] GravityBits = [0x40c00000, 0x3e75c28f, 0x3f99999a];
    /// <summary>같은PATCH풀/목록과모든장치를등록전에확인한다.</summary>
    public RawShrapnelLifetime(RawSidPool pool, RawRenderRegistry registry, ShrapnelLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, registry.Pool) || hooks.Reserve is null || hooks.Create is null || hooks.Pop is null || hooks.Attach is null || hooks.Depend is null || hooks.Capture is null || hooks.Prepare is null)
            throw new ArgumentException("파편 수명 판본/실제 경계 누락");
        _pool = pool; _registry = registry; _hooks = hooks.Copy();
    }
    /// <summary>실제wrapper의new104실패는생성/등록에진입하지않는다.</summary>
    public bool ReserveObject() => _hooks.Reserve!(104);
    /// <summary>현재비정렬좌표의float비트다.</summary>
    private static float Float(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4)));
    /// <summary>더미출력은생성직후설정한다. 초기SID0깊이와준비뒤현재source깊이를구별한다.</summary>
    public void Initialize(ShrapnelFrameFields fields, uint token, int source, ShrapnelKind kind, ref int dummy)
    {
        int variant = (int)kind; if ((uint)variant >= GravityBits.Length) throw new ArgumentOutOfRangeException(nameof(kind), "파편 가상 생성기 범위 오류");
        _ = _pool.Slot(source); _registry.Construct(fields.Registration, token, 0, 16);
        fields.Particles = []; fields.Total = 0; fields.LastTime = fields.Elapsed = 0; fields.First = false;
        dummy = _hooks.Create!(110, 2); ReadOnlySpan<byte> raw = _pool.Slot(source);
        _hooks.Pop!(dummy, Float(raw, 14), Float(raw, 18), 16); _hooks.Attach!(fields.Registration, 16, dummy, 16);
        if (fields.Registration.Form < 1 || fields.Registration.Form > 65535) throw new ArgumentOutOfRangeException(nameof(fields), "파편 현재 Form 번호 오류");
        _hooks.Depend!(fields.Registration.Form, dummy);
        if (kind == ShrapnelKind.Variant2) fields.Registration.ZOrder = unchecked(fields.Registration.ZOrder + 1);
        fields.Image.Pixels = []; fields.Gravity = BitConverter.UInt32BitsToSingle(GravityBits[variant]);
        fields.Image = _hooks.Capture!(source); _hooks.Prepare!(fields, kind); fields.Registration.ZOrder = unchecked((sbyte)_pool.Slot(source)[35]);
    }
    /// <summary>bitmap→입자→등록/base→선택객체순서다. raw포인터대신실제관리저장소를반납한다.</summary>
    public void Destroy(ShrapnelFrameFields fields, uint flags, Action<ShrapnelRelease> release)
    {
        ArgumentNullException.ThrowIfNull(release);
        if (fields.Image.Pixels.Length != 0) { release(ShrapnelRelease.Source); fields.Image.Pixels = []; }
        if (fields.Particles.Length != 0) { release(ShrapnelRelease.Particles); fields.Particles = []; }
        _registry.Destroy(fields.Registration); if ((flags & 1) != 0) release(ShrapnelRelease.Object);
    }
}
