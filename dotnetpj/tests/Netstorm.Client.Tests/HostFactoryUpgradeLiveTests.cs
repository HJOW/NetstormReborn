using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 공장 정기·업그레이드 그림·건설·감시 쌍·완료·AI 통지를 전투 없이 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 자산/장치/Host를 사용하는 업그레이드 세션 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>먼저 실제 건설을 완료해 정상 공장을 만들고 기존 정기 예약은 그대로 둔다.</summary>
    private static int UpgradeFactory(BurningContext scene)
    {
        HostSession session = scene.Session; int parent = ProgressFactory(scene);
        session.WorldConstructionProgress.Start(parent, .01f);
        scene.Visuals.PrepareFrame(.01, 1); session.AdvanceMissionFrame(.01); session.Kernel.RunFrame();
        Assert.Equal(0, session.Pool.Slot(parent)[40] & 1); return parent;
    }

    /// <summary>실제 부모의 현재감시Form에서원본조건의클라이언트그림을찾는다.</summary>
    private static int UpgradePicture(HostSession session, int parent)
    {
        int form = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]);
        // 다른정기Form은건너뛰고현재역할의감시양방향연결만조회한다.
        while (form != 0)
        {
            ReadOnlySpan<byte> raw = session.Pool.Slot(form);
            if (raw[10] == session.GameAux.DependFormTypeBox.Value)
            {
                int watcher = session.WorldDependencies.Watcher(form);
                if (watcher > 4 && watcher < 15000 && session.Pool.Slot(watcher)[10] == session.Pool.Slot(parent)[10]
                    && (session.Pool.Slot(watcher)[40] & 8) != 0) return watcher;
            }
            form = BinaryPrimitives.ReadUInt16LittleEndian(raw[4..]);
        }
        throw new InvalidOperationException("검사 업그레이드 그림 없음");
    }

    /// <summary>실제사건3첫호출과다음호출이그림/건설/감시를만들고레벨/HP/프레임/플레어/전체정리를수행한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductFactoryUpgradeRegularCreatesPictureAndCompletes()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        _ = session.PlayerProcesses.AddSharedRegular(parent, 4, 20) ?? throw new InvalidOperationException("검사 사건4 부착 실패");
        RegularProcess command = session.PlayerProcesses.AddSharedRegular(parent, 3, .2f) ?? throw new InvalidOperationException("검사 사건3 부착 실패");
        Assert.True(session.WorldFactoryUpgrade.IsUpgrading(parent)); session.Kernel.RunFrame();
        int picture = UpgradePicture(session, parent); Assert.True(session.WorldDependencies.Has(parent, picture));
        Assert.Equal(1, session.Pool.Slot(picture)[11] >> 5); Assert.Equal(1, session.Pool.Slot(picture)[34]);
        Assert.NotEqual(0, new RawContainedLookup(session.Pool, (_, _) => throw new InvalidOperationException("조회 삭제" )).Find(picture, 30));
        Assert.Equal(1u, command.Count());
        scene.Visuals.PrepareFrame(.3, 2); session.AdvanceMissionFrame(.29);
        Assert.Equal(1, session.Pool.Slot(parent)[11] >> 5); Assert.Equal(session.Reward.MaxHitPoints(parent), BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[26..]));
        Assert.Equal(session.WorldFrames[85].Run((byte)'B').First, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(parent)[36..]));
        Assert.Equal(1, session.Pool.Slot(picture)[11] & 1); Assert.False(session.WorldDependencies.Has(parent, picture));
        Assert.False(session.WorldFactoryUpgrade.IsUpgrading(parent)); Assert.Null(session.PlayerProcesses.FindEvent(parent, 3)); Assert.Null(session.PlayerProcesses.FindEvent(parent, 4));
    }

    /// <summary>복수예측그림을저장next순서로정리하고현재아웃포스트역할관문은모든완료효과를생략한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductFactoryUpgradeClearsMultiplePicturesAndReadsCurrentOutpost()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        session.WorldFactoryUpgrade.Begin(parent, .1f); int first = UpgradePicture(session, parent);
        session.WorldFactoryUpgrade.Begin(parent, .1f); int second = UpgradePicture(session, parent); Assert.NotEqual(first, second);
        session.Gameplay.OutpostTypeBox.Value = 85; session.WorldFactoryUpgrade.Complete(parent);
        Assert.Equal(0, session.Pool.Slot(parent)[11] >> 5); Assert.True(session.WorldFactoryUpgrade.HasPicture(parent));
        session.Gameplay.OutpostTypeBox.Value = 126; session.WorldFactoryUpgrade.Complete(parent);
        Assert.Equal(1, session.Pool.Slot(first)[11] & 1); Assert.Equal(1, session.Pool.Slot(second)[11] & 1);
        Assert.False(session.WorldFactoryUpgrade.HasPicture(parent));
    }

    /// <summary>실제감시생성/검색/쌍삭제는다른감시와Kernel 객체를보존한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDependencyPairsPreserveOtherPairsAndForeignProcesses()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int first = session.BuildPlayerWorldHooks().Create!(70, 2), second = session.BuildPlayerWorldHooks().Create!(70, 2), third = session.BuildPlayerWorldHooks().Create!(70, 2);
        DependFormPair pair = session.WorldDependencies.Create(first, second); DependFormPair other = session.WorldDependencies.Create(first, third);
        uint foreign = session.Kernel.Add(new BaseProcess()); Assert.True(session.WorldDependencies.Has(first, second));
        session.WorldDependencies.Remove(second, first);
        Assert.Equal(1, session.Pool.Slot(pair.WatchedForm)[11] & 1); Assert.Equal(1, session.Pool.Slot(pair.WatcherForm)[11] & 1);
        Assert.True(session.WorldDependencies.Has(first, third)); Assert.NotNull(session.Kernel.Get(foreign));
        session.ReleasePlayerProcesses(); Assert.Equal(1, session.Pool.Slot(other.WatchedForm)[11] & 1); Assert.Equal(1, session.Pool.Slot(other.WatcherForm)[11] & 1);
        Assert.Throws<InvalidOperationException>(() => session.WorldDependencies); session.Kernel.Remove(foreign);
    }

    /// <summary>원본ProcessForm 가상통지가전달할사건과인자를관측한다.</summary>
    private sealed class UpgradeDependencyObserver : SquidProcess
    {
        /// <summary>실제Host가전달한원본사건/감시Form/부모다.</summary>
        public List<(uint Event, uint Form, uint Parent)> Notices { get; } = [];
        /// <summary>가상2 통지를관측하며수명/인자를바꾸지않는다.</summary>
        public override void Notify(uint ev, uint info, uint value) => Notices.Add((ev, info, value));
    }

    /// <summary>부모의실제공통삭제가감시Form을통해현재ProcessForm 가상통지를전달하고양쪽을정리한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDependencyParentDeathUsesActualProcessNotify()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        int watched = session.BuildPlayerWorldHooks().Create!(70, 2), anchor = session.BuildPlayerWorldHooks().Create!(70, 2);
        var observer = new UpgradeDependencyObserver(); _ = session.PlayerProcesses.Attach(observer, 47, anchor, 0x10);
        DependFormPair pair = session.WorldDependencies.Create(watched, observer.Form());
        session.PlayerProcesses.Destroy().Destroy(watched, 0, session.WorldDependencies.Hooks());
        Assert.Equal(new[] { (2u, (uint)pair.WatchedForm, (uint)watched) }, observer.Notices);
        Assert.Equal(1, session.Pool.Slot(pair.WatchedForm)[11] & 1); Assert.Equal(1, session.Pool.Slot(pair.WatcherForm)[11] & 1);
        Assert.Same(observer, session.PlayerProcesses.FromForm(observer.Form()));
    }

    /// <summary>빈자산가상통지는현재VT만읽으며잘못된TYPE/owner/상태와통지인자를미리검사하지않는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductDependencyEmptyAssetNoticePreservesNativeArgumentSemantics()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int sid = session.BuildPlayerWorldHooks().Create!(85, 2);
        Span<byte> raw = session.Pool.AllocatedBytes(sid); raw[10] = 255; raw[34] = 255; raw[11] |= 3;
        session.NotifyWorldDependency(sid, int.MaxValue, int.MinValue);
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x12345678);
        Assert.Throws<InvalidOperationException>(() => session.NotifyWorldDependency(sid, 0, 0));
    }

    /// <summary>완료뒤현재Player brain을읽어실제AI 시설목록/지갑에현재소유자통지를전달한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductFactoryUpgradeNotifiesCurrentAiFactoryMemory()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        uint brain = session.AiBrainMemory.Allocate(RawAiBrainConstructor.BytesCount), list = session.AiBrainMemory.Allocate(16);
        byte[] body = session.AiBrainMemory.Bytes(brain); BinaryPrimitives.WriteUInt32LittleEndian(body.AsSpan(0x260), 2);
        BinaryPrimitives.WriteUInt32LittleEndian(body.AsSpan(0x30c), list); BinaryPrimitives.WriteUInt32LittleEndian(body.AsSpan(0x310), 4);
        BinaryPrimitives.WriteUInt32LittleEndian(body.AsSpan(0x2d8), 10000);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(2 * 0xb4 + 0xb0), brain);
        session.SetFortObjectOwner(parent, 2); Assert.Equal(-1, session.WorldFactoryUpgrade.Finish(parent));
        Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(body.AsSpan(0x314)));
        Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(session.AiBrainMemory.Bytes(list)));
        Assert.Equal(unchecked(10000u - (uint)(int)scene.Types[85].Cost), BinaryPrimitives.ReadUInt32LittleEndian(body.AsSpan(0x2d8)));
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(2 * 0xb4 + 0xb0), 0);
        session.AiBrainMemory.Free(list); session.AiBrainMemory.Free(brain);
    }
}
