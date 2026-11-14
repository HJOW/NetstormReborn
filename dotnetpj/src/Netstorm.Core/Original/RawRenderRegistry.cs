// 공통 표시 등록은 파생 효과와 같은 raw 부모/슬롯 순서를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>원문 RenderProcess의 등록 필드다. 파생 객체가 소유하고 등록기는 같은 참조를 빌린다.</summary>
public sealed class RenderProcessFields
{
    /// <summary>원본 객체 포인터 대신 쓰는 비영 토큰이다.</summary>
    public uint Token { get; set; }
    /// <summary>현재 부착 Form이며 base 소멸 뒤 -12345다.</summary>
    public int Form { get; set; }
    /// <summary>현재 부착 부모이며 진입 부모와 다를 수 있다.</summary>
    public int Parent { get; set; }
    /// <summary>화면 표시 영역의 왼쪽 좌표다.</summary>
    public float Left { get; set; }
    /// <summary>화면 표시 영역의 위쪽 좌표다.</summary>
    public float Top { get; set; }
    /// <summary>화면 표시 영역의 오른쪽 좌표다.</summary>
    public float Right { get; set; }
    /// <summary>화면 표시 영역의 아래쪽 좌표다.</summary>
    public float Bottom { get; set; }
    /// <summary>구멍을 포함한 원문 목록 번호이며 미등록은 -1이다.</summary>
    public int Slot { get; set; } = -1;
    /// <summary>진입 부모의 현재 signed BYTE 깊이를 확장한 값이다.</summary>
    public int ZOrder { get; set; }
}

/// <summary>표시 종류가 공유하는 원문 배율·생성 역할·팔레트 owner다.</summary>
public sealed class ConstructionVisualState
{
    /// <summary>건설 표시의 현재 프로세스 TYPE다.</summary>
    public uint ProcessType { get; set; } = 17;
    /// <summary>월드 수명 동안 공유하는 건설 색 표 토큰이다.</summary>
    public uint Palette { get; set; }
    /// <summary>월드에서 화면으로 변환하는 현재 가로 배율이다.</summary>
    public float ScaleX { get; set; } = 16;
    /// <summary>월드에서 화면으로 변환하는 현재 세로 배율이다.</summary>
    public float ScaleY { get; set; } = 11;
}

/// <summary>원문 목록 확보/반납과 실제 BaseProcess 부착 경계다.</summary>
public sealed class RenderRegistryHooks
{
    /// <summary>원본32비트 포인터 배열의 확보 바이트 수를 받는다.</summary>
    public Func<uint, bool>? Reserve { get; set; }
    /// <summary>확장 시 기존 내용을 복사한 뒤 기존 배열을 반납한다.</summary>
    public Action? ReleaseArray { get; set; }
    /// <summary>이미 등록한 필드와 type/부모/flags를 같은 실제 Host에 부착한다.</summary>
    public Action<RenderProcessFields, uint, int, uint>? Attach { get; set; }
    /// <summary>함수 객체는 값 복사하고 캡처한 현재 월드의 수명은 빌린다.</summary>
    internal RenderRegistryHooks Copy() => new() { Reserve = Reserve, ReleaseArray = ReleaseArray, Attach = Attach };
}

/// <summary>RawConstructionLifetime의 공통 표시 등록/삭제다. 파생 건설 버퍼 소멸은 별도 몸체다.</summary>
public sealed class RawRenderRegistry
{
    /// <summary>원문 한 번의 목록 확장 칸 수다.</summary>
    private const int Growth = 40;
    /// <summary>목록/필드가 읽는 같은 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 원문 배열이다. 확장 뒤 과거 span은 새 슬롯을 반영하지 않는다.</summary>
    public ReadOnlySpan<uint> Slots => _slots;
    /// <summary>파생 소유권과 구별하는 비소유 토큰 배열이다.</summary>
    private uint[] _slots;
    /// <summary>구성 시 복사한 메모리/부착 함수다.</summary>
    private readonly RenderRegistryHooks _hooks;

    /// <summary>같은 풀과 필수 장치를 보관하며 초기 목록은 원문 vector 값처럼 복사한다.</summary>
    public RawRenderRegistry(RawSidPool pool, RenderRegistryHooks hooks, ReadOnlySpan<uint> initial = default)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Reserve is null || hooks.ReleaseArray is null || hooks.Attach is null) throw new ArgumentException("표시 목록 경계 누락");
        if (initial.Length > int.MaxValue / 4) throw new ArgumentException("표시 목록 크기 오류");
        Pool = pool; _hooks = hooks.Copy(); _slots = initial.ToArray();
    }

    /// <summary>SID0도 원문대로 조회하며 현재 판본의 signed 깊이 BYTE를 읽는다.</summary>
    private int Depth(int parent) => unchecked((sbyte)Pool.Slot(parent)[Pool.Edition == OriginalEdition.Patch1078 ? 35 : 33]);

    /// <summary>초기화·첫 빈 칸/40칸 확장·SID0깊이·부착·진입 부모 재조회 순서로 등록한다.</summary>
    public void Construct(RenderProcessFields fields, uint token, int parent, uint type)
    {
        ArgumentNullException.ThrowIfNull(fields);
        if (token == 0 || Array.IndexOf(_slots, token) >= 0) throw new ArgumentException("표시 목록 토큰 중복/0");
        if (fields.Token != 0 && fields.Slot >= 0 && fields.Slot < _slots.Length && _slots[fields.Slot] == fields.Token)
            throw new InvalidOperationException("살아 있는 표시 등록 필드 재사용");
        _ = Pool.Slot(parent); fields.Token = token; fields.Form = fields.Parent = 0;
        fields.Left = fields.Top = fields.Right = fields.Bottom = 0;
        int free = Array.IndexOf(_slots, 0u);
        if (free < 0)
        {
            int old = _slots.Length;
            if (old > int.MaxValue / 4 - Growth) throw new InvalidOperationException("표시 목록 확장 범위 오류");
            int size = old + Growth;
            if (!_hooks.Reserve!((uint)(size * 4))) throw new OutOfMemoryException("표시 목록 확보 실패");
            uint[] next = new uint[size]; _slots.CopyTo(next, 0);
            if (old != 0) _hooks.ReleaseArray!();
            _slots = next; free = old;
        }
        fields.Slot = free; _slots[free] = token; fields.ZOrder = Depth(0);
        if (parent != 0) { _hooks.Attach!(fields, type, parent, 0x10); fields.ZOrder = Depth(parent); }
    }

    /// <summary>정확히 일치하는 등록만 지우고 영역/깊이/slot은 보존한다.</summary>
    public void Destroy(RenderProcessFields fields)
    {
        ArgumentNullException.ThrowIfNull(fields);
        if (fields.Slot < 0 || fields.Slot >= _slots.Length || fields.Token == 0 || _slots[fields.Slot] != fields.Token)
            throw new InvalidOperationException("표시 목록 삭제 등록 불일치");
        _slots[fields.Slot] = 0; fields.Form = fields.Parent = -12345;
    }
}
