// 종속 Form 분리와 UI 통지의 체인·관문을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 종속 해제를 검증하는 테스트 클래스다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.
public sealed class RawSquidContainedUnpopTests
{
    /// <summary>들어 있는 내용물 비트다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private const byte ContainedBit = 8;

    /// <summary>같은 풀·상태·효과로 종속 해제를 만든다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    private static RawSquidContainedUnpop Make(RawSidPool pool, SquidContainedUnpopHooks hooks) =>
        new(pool, new SquidContainedUnpopState
        {
            ContentFormType = new UIntBox { Value = 6u },
            GumpFormType = new UIntBox { Value = 3u },
            UiEvent = new UIntBox { Value = 0x02000039u },
        }, hooks);

    /// <summary>살아 있는 빈 슬롯을 하나 할당한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="pool">쓸 풀이다.</param>
    /// <param name="type">쓸 타입 번호다.</param>
    /// <returns>할당한 번호다.</returns>
    private static int Live(RawSidPool pool, byte type)
    {
        // 할당 뒤 void 상태를 지워 살아 있는 자산으로 둔다.
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = type;
        return sid;
    }

    /// <summary>슬롯 바이트에 원문 WORD 하나를 쓴다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    /// <param name="pool">쓸 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void WriteWord(RawSidPool pool, int sid, int offset, ushort value)
    {
        // 인접 payload를 바꾸지 않는다.
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[offset] = unchecked((byte)value);
        raw[offset + 1] = unchecked((byte)(value >> 8));
    }

    /// <summary>분리가 체인·부모 머리를 정리하는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void DetachUnlinksChain()
    {
        // 부모 머리·이전·다음 잇기와 비트 지우기·부모 알림 순서를 검증한다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        int parent = Live(pool, 70);
        int child = Live(pool, 0x46);
        pool.AllocatedBytes(child)[RawSidPool.StateOffset] = ContainedBit;
        WriteWord(pool, parent, 6, unchecked((ushort)child));
        var notices = new List<(int Parent, int Sid, uint Reason)>();
        var unpop = Make(pool, new SquidContainedUnpopHooks
        {
            Parent = _ => parent,
            Previous = _ => 0,
            SetPrevious = (_, _) => Assert.Fail("다음이 없는데 이전 쓰기를 불렀다."),
            ParentNotice = (p, s, r) => notices.Add((p, s, r)),
            Assertion = (_, _, _) => Assert.Fail("정상 분리에 진단을 불렀다."),
        });
        unpop.Detach(child);
        Assert.Equal(0, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
        Assert.Equal(0, pool.Slot(child)[RawSidPool.StateOffset] & ContainedBit);
        Assert.Single(notices);
        Assert.Equal((parent, child, 2u), notices[0]);
    }

    /// <summary>UI 통지가 Gump 명령을 큐에 넣는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void NotifyUiQueuesGumpCommand()
    {
        // Form 체인 걷기와 Gump 일치·명령 기록을 검증한다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        int root = Live(pool, 70);
        int form = Live(pool, 3);
        WriteWord(pool, root, 6, unchecked((ushort)form));
        var commands = new List<(uint Event, uint Sid, uint Flags)>();
        var unpop = Make(pool, new SquidContainedUnpopHooks
        {
            Command = (e, s, f) => commands.Add((e, s, f)),
            Assertion = (_, _, _) => Assert.Fail("정상 통지에 진단을 불렀다."),
        });
        unpop.NotifyUi(root);
        Assert.Single(commands);
        Assert.Equal((0x02000039u, unchecked((uint)root), 0u), commands[0]);
    }

    /// <summary>비운 분리 장치를 거부하는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void DetachRejectsMissingHooks()
    {
        // 부모 getter 누락은 원본처럼 명시 오류로 멈춘다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        int child = Live(pool, 0x46);
        pool.AllocatedBytes(child)[RawSidPool.StateOffset] = ContainedBit;
        var unpop = Make(pool, new SquidContainedUnpopHooks
        {
            Assertion = (_, _, _) => { },
        });
        Assert.Throws<InvalidOperationException>(() => unpop.Detach(child));
    }

    /// <summary>해제가 분리와 통지를 순서대로 잇는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopComposesDetachAndNotify()
    {
        // Detach 뒤 NotifyUi 호출 순서를 검증한다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        int parent = Live(pool, 70);
        int child = Live(pool, 70);
        int form = Live(pool, 3);
        pool.AllocatedBytes(child)[RawSidPool.StateOffset] = ContainedBit;
        WriteWord(pool, parent, 6, unchecked((ushort)child));
        WriteWord(pool, child, 4, unchecked((ushort)form));
        WriteWord(pool, child, 6, unchecked((ushort)form));
        var sets = new List<(int Sid, int Previous)>();
        var notices = new List<(int Parent, int Sid, uint Reason)>();
        var commands = new List<(uint Event, uint Sid, uint Flags)>();
        var unpop = Make(pool, new SquidContainedUnpopHooks
        {
            Parent = _ => parent,
            Previous = _ => 0,
            SetPrevious = (s, p) => sets.Add((s, p)),
            ParentNotice = (p, s, r) => notices.Add((p, s, r)),
            Command = (e, s, f) => commands.Add((e, s, f)),
            Assertion = (_, _, _) => Assert.Fail("정상 해제에 진단을 불렀다."),
        });
        unpop.Unpop(child);
        Assert.Equal(0, pool.Slot(child)[RawSidPool.StateOffset] & ContainedBit);
        Assert.Equal(form, pool.Slot(parent)[6] | (pool.Slot(parent)[7] << 8));
        Assert.Single(sets);
        Assert.Equal((form, 0), sets[0]);
        Assert.Single(notices);
        Assert.Single(commands);
    }

    /// <summary>부모 탐색 번호와 같은 풀을 돌려주는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void ContentFormTypeAndPool()
    {
        // 같은 상자 값 읽기와 풀 정체성을 검증한다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        var unpop = Make(pool, new SquidContainedUnpopHooks());
        Assert.Equal(6u, unpop.ContentFormType());
        Assert.Same(pool, unpop.Pool());
    }

    /// <summary>UI 탐색 순환을 거부하는지 검증한다. 2026-11-07 F2d 종속 해제 이식에서 추가됐다.</summary>
    [Fact]
    public void NotifyUiRejectsCycle()
    {
        // 순환 체인은 원본처럼 명시 오류로 멈춘다.
        var pool = new RawSurfaceStorage(OriginalEdition.Patch1078).Pool;
        int root = Live(pool, 70);
        int first = Live(pool, 70);
        int second = Live(pool, 70);
        WriteWord(pool, root, 6, unchecked((ushort)first));
        WriteWord(pool, first, 4, unchecked((ushort)second));
        WriteWord(pool, second, 4, unchecked((ushort)first));
        var unpop = Make(pool, new SquidContainedUnpopHooks());
        Assert.Throws<InvalidOperationException>(() => unpop.NotifyUi(root));
    }
}
