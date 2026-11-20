using System.Reflection;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Host의기존공장Form장치와직접외곽호출대여를검사한다.
namespace Netstorm.Client.Tests;

/// <summary>준비/공개/예외/최종정리의실제공장효과수명을대조한다.</summary>
public sealed class HostFactoryRuntimeTests
{
    /// <summary>제품진단API없이실제로연결된기존공장만빌린다.</summary>
    private static RawSquidFactory Factory(HostSession session) => Assert.IsType<RawSquidFactory>(typeof(HostSession).GetField("_fortFactory", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
    /// <summary>실제생성자의같은Form에명시contained/부모상태입력을공급한다.</summary>
    private static int Form(HostSession session, RawSquidFactory factory)
    {
        int parent = factory.Create(152, SidAllocation.Client), form = factory.Create(2, SidAllocation.Client); Span<byte> raw = session.Pool.AllocatedBytes(form);
        raw[11] = 8; raw[16] = (byte)parent; raw[17] = (byte)(parent >> 8); return form;
    }
    /// <summary>최종정리뒤실제기존공장의직접pin이분리됐는지확인한다.</summary>
    private static void AssertUnpinned(RawSquidFactory factory) => Assert.Null(typeof(RawSquidFactory).GetField("_directRuntimePin", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(factory));

    /// <summary>직접기존분리장치를유지하고공개전실행수/공개후외곽대여로반납을거부한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void ExistingFactoryEffectsStayConnectedAcrossRuntimePublication(bool publishFirst)
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool); HostMissionRuntime? owner = publishFirst ? HostMissionRuntime.Create(session, storage, []) : null;
        _ = HostPostPopRuntimeTests.Build(session); RawSquidFactory factory = Factory(session); int form = Form(session, factory); int calls = 0;
        factory.BindFormDetach(_ => { calls++; Assert.Equal(1ul, factory.ActiveRuntimeCalls); Assert.Equal(owner is not null, session.Gameplay.RuntimeCallActive(session)); Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); });
        factory.UnpopForm(form); Assert.Equal(1, calls); Assert.Equal(0ul, factory.ActiveRuntimeCalls);
        if (owner is null) { owner = HostMissionRuntime.Create(session, storage, []); session.Pool.AllocatedBytes(form)[11] = 8; factory.UnpopForm(form); Assert.Equal(2, calls); }
        Assert.False(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); session.ReleasePlayerRecordViews(); AssertUnpinned(factory);
    }

    /// <summary>실제공장분리실패시외곽종료는호출계수반납뒤완료하고자식정리가가능하다.</summary>
    [Fact]
    public void ActualFactoryFailureUnwindsWholeOwnerAndCounter()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool); _ = HostPostPopRuntimeTests.Build(session);
        RawSquidFactory factory = Factory(session); int form = Form(session, factory); var owner = HostMissionRuntime.Create(session, storage, []); var failure = new IOException("실제Host공장분리실패");
        factory.BindFormDetach(_ => { Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => factory.UnpopForm(form))); Assert.Equal(0ul, factory.ActiveRuntimeCalls); Assert.True(owner.Released); Assert.False(session.Gameplay.Borrowed());
        session.ReleasePlayerRecordViews(); AssertUnpinned(factory);
    }
}
