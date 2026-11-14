using System.Buffers.Binary;

// 보호막의 두 최초 예약과 현재 공간·사제 조회·프레임/삭제 Regular를 원문 순서로 실행한다.
namespace Netstorm.Core.Original;

/// <summary>보호막 타입·가상 표·원본 주기와 선택 장치의 공통 계약이다.</summary>
public static class ForcefieldLifecycle
{
    /// <summary>실제 보호막 TYPE BYTE다.</summary>
    public const byte Type = 167;
    /// <summary>PATCH 보호막의 실제 가상 표다.</summary>
    public const uint PatchVtable = 0x00507770;
    /// <summary>CD 보호막의 실제 가상 표다.</summary>
    public const uint CdVtable = 0x005042b8;
    /// <summary>프레임 진행 Regular의 사건 번호다.</summary>
    public const uint FrameEvent = 1;
    /// <summary>사제 존재 검사 Regular의 사건 번호다.</summary>
    public const uint CheckEvent = 2;
    /// <summary>원본 단정도0.08의 정확한 비트다.</summary>
    public static float FrameInterval => BitConverter.UInt32BitsToSingle(0x3da3d70a);
    /// <summary>원본 단정도0.01의 정확한 비트다.</summary>
    public static float CheckInterval => BitConverter.UInt32BitsToSingle(0x3c23d70a);
    /// <summary>선택 시점의 대리자를 값으로 빌려 자기 연결 반납 중에도 실행을 끝낸다.</summary>
    internal static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("보호막 선택 효과 미연결");
    /// <summary>명시 사전 검사/CD 경로에서만 실제 클래스·상태를 검증한다.</summary>
    internal static void Validate(RawSidPool pool, int sid)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        if (sid < 5 || (raw[11] & 9) != 0 || raw[10] != Type || BinaryPrimitives.ReadUInt32LittleEndian(raw) != (pool.Edition == OriginalEdition.Patch1078 ? PatchVtable : CdVtable))
            throw new ArgumentException("보호막 타입/가상 표 불일치");
    }
    /// <summary>확보 경계 뒤 같은 실제 Host의 타입46 Form·부모 체인·Kernel을 사용한다.</summary>
    public static ForcefieldPostPopHooks MakeForcefieldPostPopProcessHooks(RawSidPool pool, SquidProcessHost host, Func<bool> reserve)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(reserve);
        if (!ReferenceEquals(pool, host.Pool())) throw new ArgumentException("보호막/프로세스 풀 불일치");
        return new ForcefieldPostPopHooks { ReserveRegular = reserve, AddRegular = (sid, ev, payload) => host.AddRegular(sid, ev, payload) };
    }
    /// <summary>같은 실제 진행기에 프레임 효과만 연결하고 다른 효과 사본은 유지한다.</summary>
    public static ForcefieldRegularHooks MakeForcefieldFrameHooks(RawSidPool pool, RawFrameAdvance advance, ForcefieldRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(advance); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, advance.Pool)) throw new ArgumentException("보호막/프레임 진행 풀 불일치");
        ForcefieldRegularHooks result = hooks.Copy(); result.Advance = (sid, delta, flags) => advance.Advance(sid, delta, flags); return result;
    }
}

/// <summary>두 원본 new(0x28) 확보와 실제 Regular 등록 경계다.</summary>
public sealed class ForcefieldPostPopHooks
{
    /// <summary>현재 확보 결과를 매번 따로 읽는다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>확보 성공 뒤만 같은 보호막에 사건·payload를 등록한다.</summary>
    public Action<int, uint, float>? AddRegular { get; set; }
    /// <summary>함수 객체는 복사하고 캡처 장기 owner는 공유한다.</summary>
    internal ForcefieldPostPopHooks Copy() => new() { ReserveRegular = ReserveRegular, AddRegular = AddRegular };
}

/// <summary>보호막 최초 Pop의 두 예약 접두다. 공통 장부/깊이는 호출자가 한 번 처리한다.</summary>
public sealed class RawForcefieldPostPop
{
    /// <summary>명시 검사와 일반 Pop이 공유하는 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원문 함수 객체처럼 값 복사한 확보·생성 장치다.</summary>
    private readonly ForcefieldPostPopHooks _hooks;
    /// <summary>PATCH는 선택된 장치만 요구하며 CD는 기존 구성 사전 계약을 유지한다.</summary>
    public RawForcefieldPostPop(RawSidPool pool, ForcefieldPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 && (hooks.ReserveRegular is null || hooks.AddRegular is null)) throw new ArgumentException("보호막 예약 효과 누락");
        _pool = pool; _hooks = hooks.Copy();
    }
    /// <summary>연결 시 다른 월드 풀을 혼용하지 않도록 같은 풀을 돌려준다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>raw/공간을 바꾸지 않는 명시 사전 검사다.</summary>
    public void Validate(int sid) => ForcefieldLifecycle.Validate(_pool, sid);
    /// <summary>flags bit0에서만 두 확보를 독립적으로 수행한다. 첫 실패여도 두 번째를 선택한다.</summary>
    public void Prefix(int sid, uint flags)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) Validate(sid);
        if ((flags & 1) == 0) return;
        if (ForcefieldLifecycle.Need(_hooks.ReserveRegular)()) ForcefieldLifecycle.Need(_hooks.AddRegular)(sid, ForcefieldLifecycle.FrameEvent, ForcefieldLifecycle.FrameInterval);
        if (ForcefieldLifecycle.Need(_hooks.ReserveRegular)()) ForcefieldLifecycle.Need(_hooks.AddRegular)(sid, ForcefieldLifecycle.CheckEvent, ForcefieldLifecycle.CheckInterval);
    }
}

/// <summary>사제 존재 검사가 소비 시점에 읽는 현재 전역이다.</summary>
public sealed class ForcefieldRegularState
{
    /// <summary>독립 자료 재생의 현재 사제 TYPE DWORD다.</summary>
    public uint PriestType { get; set; } = 158;
    /// <summary>미지원 PATCH 사건 진단이 읽는 현재 checking DWORD다.</summary>
    public RawCheckingFlag Debug { get; set; } = new();
    /// <summary>실제 월드가 빌리는 장기 사제 TYPE owner다.</summary>
    public UIntBox? SharedPriestType { get; set; }
    /// <summary>존재 검사 때마다 현재 전체 DWORD를 읽으며 BYTE로 줄이지 않는다.</summary>
    public uint CurrentPriestType() => SharedPriestType?.Value ?? PriestType;
}

/// <summary>현재 Regular가 도달한 프레임·가상 삭제·진단·typed finder 효과다.</summary>
public sealed class ForcefieldRegularHooks
{
    /// <summary>같은 현재 프레임을 지정/공간 모듈로 진행한다.</summary>
    public Action<int, int, uint>? Advance { get; set; }
    /// <summary>현재 삭제 분배기를 실행하며 자기 Regular도 삭제할 수 있다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>원문 식/파일/줄 진단이며 정상 복귀할 수 있다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>실제 타입별 단계 finder에 raw float 좌표·현재 TYPE를 전달한다.</summary>
    public Func<float, float, uint, int>? FindTypeAt { get; set; }
    /// <summary>효과 객체만 값 복사하며 현재 캡처 owner는 같은 수명으로 공유한다.</summary>
    internal ForcefieldRegularHooks Copy() => new() { Advance = Advance, Destroy = Destroy, Assertion = Assertion, FindTypeAt = FindTypeAt };
}

/// <summary>보호막 프레임/사제 존재 Regular 본체다. 선택 효과 이후 부모를 다시 쓰지 않는다.</summary>
public sealed class RawForcefieldRegular
{
    /// <summary>현재 raw 부모/후보가 공유하는 같은 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>일반 finder가 읽는 실제 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>후보 발자국·genus를 매번 읽는 같은 현재 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>매 사건 현재 전역을 빌리는 상태 owner다.</summary>
    private readonly ForcefieldRegularState _state;
    /// <summary>생성 시 값 복사한 현재 장치 묶음이다.</summary>
    private readonly ForcefieldRegularHooks _hooks;
    /// <summary>판본별 TYPE 길이를 확인하며 PATCH는 선택된 장치만 요구한다.</summary>
    public RawForcefieldRegular(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, ForcefieldRegularState state, ForcefieldRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || (pool.Edition != OriginalEdition.Patch1078 && (hooks.Advance is null || hooks.Destroy is null)))
            throw new ArgumentException("보호막 Regular 타입/효과 연결 오류");
        _pool = pool; _hash = hash; _types = types; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>진행/공간 어댑터가 같은 풀을 사용하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>원문 float→signed int 정상 범위를 검사하고0방향으로 절삭한다.</summary>
    private static int Cell(float value)
    {
        if (!float.IsFinite(value) || (double)value < int.MinValue || (double)value > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(value), "보호막 사제 검사 좌표 범위 오류");
        return (int)value;
    }
    /// <summary>소유자/HP/state 별도 필터 없이 실제 finder 순서의 첫 현재 타입 일치를 찾는다.</summary>
    private int FindPriest(int sid)
    {
        Func<float, float, uint, int>? find = _hooks.FindTypeAt;
        if (find is not null)
        {
            uint type = _state.CurrentPriestType(); ReadOnlySpan<byte> raw = _pool.Slot(sid);
            float y = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]), x = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
            return find(x, y, type);
        }
        ReadOnlySpan<byte> slot = _pool.Slot(sid);
        int cx = Cell(BinaryPrimitives.ReadSingleLittleEndian(slot[14..])), cy = Cell(BinaryPrimitives.ReadSingleLittleEndian(slot[18..]));
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 현재 해시 체인 순서를 그대로 유지하며 매 후보에서 현재 DWORD를 다시 비교한다.
        for (int current = finder.Begin(new SquidSearchArea(cx, cy, cx, cy)); current != 0; current = finder.Next())
            if (_pool.Slot(current)[10] == _state.CurrentPriestType()) return current;
        return 0;
    }
    /// <summary>사건1은 진행 후0.08, 사건2는 존재하면0.01/없으면 삭제 후-1, 그 밖은 선택 진단 후0이다.</summary>
    public float Handle(int sid, uint ev, uint count, float payload)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) ForcefieldLifecycle.Validate(_pool, sid);
        if (ev == ForcefieldLifecycle.FrameEvent) { ForcefieldLifecycle.Need(_hooks.Advance)(sid, 1, 0); return ForcefieldLifecycle.FrameInterval; }
        if (ev == ForcefieldLifecycle.CheckEvent)
        {
            if (FindPriest(sid) != 0) return ForcefieldLifecycle.CheckInterval;
            ForcefieldLifecycle.Need(_hooks.Destroy)(sid, 0); return -1;
        }
        if (_pool.Edition == OriginalEdition.Patch1078 && _state.Debug.Read() != 0) ForcefieldLifecycle.Need(_hooks.Assertion)("0", @".\Dais.cpp", 0x479);
        return 0;
    }
}
