using System.Buffers.Binary;

// 세 연소 소비자의 현재SID 조회와 진단 후 고정 슬롯을 같은 몸체로 공유한다.
namespace Netstorm.Core.Original;

/// <summary>같은 원문 조건의 부모별 줄 번호를 선택한다.</summary>
public enum BurningVictimEntry
{
    /// <summary>00423d50의 프레임 캡처 부모다.</summary>
    Capture,
    /// <summary>00424110의 연소점 부모다.</summary>
    Sources,
    /// <summary>00424530의 실제 프레임 부모다.</summary>
    Frame,
}
/// <summary>선택한 실제조회/진단만 요구하는 함수 묶음이다.</summary>
public sealed class BurningVictimHooks
{
    /// <summary>현재SID의 live1을 실제로 조회한다.</summary>
    public Func<int, bool, bool>? Robust { get; set; }
    /// <summary>캡처 부모만 고정슬롯의 실제VT 비영 여부를 읽는다.</summary>
    public Func<int, bool>? Vtable { get; set; }
    /// <summary>두 번째 robust 뒤 고정한 슬롯의 현재HP보유표식을 읽는다.</summary>
    public Func<int, bool>? HasHitPoints { get; set; }
    /// <summary>원문 식/파일/줄을 보고하며 복귀하면 이후 앞단을 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}
/// <summary>같은 Client checking/상세조회/진단 owner를 빌리는 연소 부모의 상태다.</summary>
public sealed class BurningVictimState
{
    /// <summary>현재 전체DWORD owner의 값사본 뷰다.</summary>
    private RawCheckingFlag _checking = new();
    /// <summary>독립값은0이며 실제월드에서는 같은 현재주소를 빌린다.</summary>
    public RawCheckingFlag Checking { get => _checking; set => _checking = new(value); }
    /// <summary>실제 Client의 현재 상세SID진단이다.</summary>
    public Func<int, bool, bool>? Robust { get; set; }
    /// <summary>실제 원문 진단UI 또는 명시실패 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}
/// <summary>RunBurningVictimParent/MakeBurningVictimHooks의 같은 현재대상 앞단이다.</summary>
public static class RawBurningVictim
{
    /// <summary>current는 const Sid&amp; 대신 쓰는 현재주소 조회다. 두 번째robust 뒤에만 슬롯을 고정한다.</summary>
    public static int? Run(Func<int> current, BurningVictimEntry entry, RawCheckingFlag checking, BurningVictimHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(current); ArgumentNullException.ThrowIfNull(checking); ArgumentNullException.ThrowIfNull(hooks);
        var view = new RawCheckingFlag(checking);
        uint line = entry == BurningVictimEntry.Capture ? 0x6fu : entry == BurningVictimEntry.Sources ? 0x91u : 0x11fu;
        if (view && !(hooks.Robust ?? throw new InvalidOperationException("연소 피해 대상의 선택 robust 미연결"))(current(), true))
            (hooks.Assertion ?? throw new InvalidOperationException("연소 피해 대상의 선택 진단 미연결"))("isSidRobust(burningSid)", @".\Burning.cpp", line);
        if (!(hooks.Robust ?? throw new InvalidOperationException("연소 피해 대상의 선택 robust 미연결"))(current(), true)) return null;
        int selected = current();
        if (entry == BurningVictimEntry.Capture && !(hooks.Vtable ?? throw new InvalidOperationException("연소 피해 대상의 선택 VT 미연결"))(selected))
            (hooks.Assertion ?? throw new InvalidOperationException("연소 피해 대상의 선택 진단 미연결"))("*((long**)&s)!=0", @".\Burning.cpp", 0x77);
        if (!(hooks.HasHitPoints ?? throw new InvalidOperationException("연소 피해 대상의 선택 HP 미연결"))(selected))
            (hooks.Assertion ?? throw new InvalidOperationException("연소 피해 대상의 선택 진단 미연결"))(entry == BurningVictimEntry.Sources ? "burningVictim.hasHitPoints()" : "s.hasHitPoints()", @".\Burning.cpp", entry == BurningVictimEntry.Capture ? 0x7fu : entry == BurningVictimEntry.Sources ? 0x98u : 0x126u);
        return selected;
    }
    /// <summary>같은 raw의 비가상HP/VT와 현재상태를 빌린다. 번호/진단을 선행 필터로 바꾸지 않는다.</summary>
    public static BurningVictimHooks MakeHooks(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, BurningVictimState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state);
        return new()
        {
            Robust = (sid, live) => state.Robust is { } robust ? robust(sid, live) : RawSidRobust.IsSidRobust(sid, live ? 1 : 0, new(), new() { Checking = () => state.Checking.Read() }),
            Vtable = sid => BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)) != 0,
            HasHitPoints = sid =>
            {
                ReadOnlySpan<byte> raw = pool.Slot(sid); int type = raw[10];
                if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "연소 피해 대상의 현재TYPE 범위");
                return (types[type].Flags2 & 0x400000) != 0 ? (raw[pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] & 1) != 0 : (types[type].Flags1 & 0x10) != 0;
            },
            Assertion = (expression, file, line) => (state.Assertion ?? throw new InvalidOperationException($"원문 연소 진단 {file}:{line}: {expression}"))(expression, file, line),
        };
    }
}
