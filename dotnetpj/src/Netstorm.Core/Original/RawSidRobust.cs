// 원문 SID 번호 반환과 선택된 상세 raw 진단을 구별한다.
namespace Netstorm.Core.Original;

/// <summary>보호PE의 DWORD 토큰과64비트 호스트 주소 계산의 폭을 구별한다.</summary>
public enum SidRobustAddressMode
{
    /// <summary>원문32비트 주소 감김이다.</summary>
    Original32,
    /// <summary>실제64비트 호스트 주소의 덧셈 범위를 검사한다.</summary>
    HostPointer,
}

/// <summary>같은 Client가 소유하는 원문 두 진단DWORD다.</summary>
public sealed class RawSidRobustDiagnostics
{
    /// <summary>상세 몸체 진입 때만0으로 바꾸는 최근 실패 비트다.</summary>
    public uint LastRobust { get; set; }
    /// <summary>논리 주소 범위보다 먼저 object+10에서 읽는 현재TYPE다.</summary>
    public uint Type { get; set; }
}

/// <summary>선택된 시점에만 요구하는 현재 주소/메모리/진단 장치다.</summary>
public sealed class SidRobustHooks
{
    /// <summary>현재 주소 산술의 폭이다.</summary>
    public SidRobustAddressMode AddressMode { get; set; }
    /// <summary>현재 풀 base를 소비 때 다시 조회한다.</summary>
    public Func<ulong>? PoolBase { get; set; }
    /// <summary>하한 통과 뒤에만 조회하는 현재용량DWORD다.</summary>
    public Func<uint>? Capacity { get; set; }
    /// <summary>선택한 주소/폭의 실제 바이트만 빌린다. 없는 물리는 그 지점에서 실패한다.</summary>
    public Func<ulong, int, ReadOnlyMemory<byte>>? Memory { get; set; }
    /// <summary>유효SID/live 분기 뒤 읽는 현재 전체checkingDWORD다.</summary>
    public Func<uint>? Checking { get; set; }
    /// <summary>원문 식/줄0x218을 보고하고 복귀하면 번호 부모는1을 반환한다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

/// <summary>PATCH004ac040/004ac140의 현재 raw 진단과 번호 부모다. 세대/정렬/void/TYPE상한 필터를 추가하지 않는다.</summary>
public static class RawSidRobust
{
    /// <summary>원문ADD32와 실제64비트 호스트 덧셈을 구별한다.</summary>
    private static ulong Address(ulong address, uint delta, SidRobustAddressMode mode)
    {
        if (mode == SidRobustAddressMode.Original32) return unchecked((uint)address + delta);
        if (delta > ulong.MaxValue - address) throw new ArgumentOutOfRangeException(nameof(address), "SID 실제 호스트 주소 범위");
        return address + delta;
    }
    /// <summary>현재 선택필드1/4바이트만 little endian으로 읽으며 사전 주소 보정을 하지 않는다.</summary>
    private static uint Read(ulong address, int width, SidRobustHooks hooks)
    {
        ReadOnlySpan<byte> raw = (hooks.Memory ?? throw new InvalidOperationException("SID 원문 진단의 선택 메모리 미연결"))(address, width).Span;
        if (raw.Length < width) throw new ArgumentOutOfRangeException(nameof(width), "SID 원문 진단의 선택 물리 범위");
        uint value = 0;
        // 실제 선택 폭만 합치며 현재 원문의 선행/중복 읽기 순서를 보존한다.
        for (int i = 0; i < width; i++) value |= (uint)raw[i] << (i * 8);
        return value;
    }
    /// <summary>최근진단0→TYPE선행저장→같은base하한/상한→각 현재필드의 실패bit 순서다.</summary>
    public static bool SquidIsRobust(ulong address, int live, RawSidRobustDiagnostics diagnostics, SidRobustHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(diagnostics); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.AddressMode == SidRobustAddressMode.Original32) address = (uint)address;
        uint failures = 0; diagnostics.LastRobust = 0; diagnostics.Type = Read(Address(address, 10, hooks.AddressMode), 1, hooks);
        ulong start = (hooks.PoolBase ?? throw new InvalidOperationException("SID 원문 진단의 선택 Pool 미연결"))();
        if (address < Address(start, 50, hooks.AddressMode)) { diagnostics.LastRobust = 1; return false; }
        uint limit = unchecked((hooks.Capacity ?? throw new InvalidOperationException("SID 원문 진단의 선택 용량 미연결"))() * 50);
        if (address >= Address(start, limit, hooks.AddressMode)) { diagnostics.LastRobust = 1; return false; }
        if (live != 0)
        {
            uint table = Read(address, 4, hooks);
            if (table is 0 or 0xdddddddd or 0xcdcdcdcd) { diagnostics.LastRobust = 2; failures = 1; }
            if ((Read(Address(address, 11, hooks.AddressMode), 1, hooks) & 1) != 0) { diagnostics.LastRobust |= 4; failures++; }
            if ((Read(Address(address, 11, hooks.AddressMode), 1, hooks) & 2) != 0) { diagnostics.LastRobust |= 8; failures++; }
            if (Read(Address(address, 10, hooks.AddressMode), 1, hooks) == 0) { diagnostics.LastRobust |= 0x10; failures++; }
        }
        else
        {
            if (Read(address, 4, hooks) == 0) { diagnostics.LastRobust = 2; failures = 1; }
            if ((Read(Address(address, 11, hooks.AddressMode), 1, hooks) & 1) == 0) { diagnostics.LastRobust |= 4; failures++; }
        }
        return failures == 0;
    }
    /// <summary>signed1..65534만 판정하고 live0/checking0은 Pool을 읽지 않는다. 상세진단 복귀 뒤에는1이다.</summary>
    public static bool IsSidRobust(int sid, int live, RawSidRobustDiagnostics diagnostics, SidRobustHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(diagnostics); ArgumentNullException.ThrowIfNull(hooks);
        if (sid < 1 || sid > 65534) return false;
        if (live == 0) return true;
        if ((hooks.Checking ?? throw new InvalidOperationException("SID 원문 진단의 선택 checking 미연결"))() != 0)
        {
            ulong address = Address((hooks.PoolBase ?? throw new InvalidOperationException("SID 원문 진단의 선택 Pool 미연결"))(), unchecked((uint)sid * 50), hooks.AddressMode);
            if (!SquidIsRobust(address, 1, diagnostics, hooks))
                (hooks.Assertion ?? throw new InvalidOperationException("SID 원문 진단의 선택 assert 미연결"))("resolve( sid ).isRobust()", 0x218);
        }
        return true;
    }
}
