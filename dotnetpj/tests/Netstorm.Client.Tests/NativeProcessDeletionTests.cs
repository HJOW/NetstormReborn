using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Native 기본 수명이 있는 실제 Session에서 정기 종료가 같은 가상 Form/Kernel 수명을 닫는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 Runtime의 삭제 결선 누락으로 실제 미션이 종료되던 경로를 창 없이 재현한다.</summary>
public sealed class NativeProcessDeletionTests
{
    /// <summary>같은 현재 선택/전송·실제 Host에 정기를 붙이고 반환0 삭제와 미이식 자산 거부를 확인한다.</summary>
    [Fact]
    public void ActualNativeSessionDeletesCompletedRegularThroughCurrentFormHooks()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var connections = new ClientConnectionGlobals(local); var templates = new CampaignTemplateStorage();
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 1000, installDevices: owner =>
        {
            owner.Storage.BindSerializationGlobals(templates.SerializationGlobals); owner.BindNativeTransmission(connections, () => game.Selection.Selected.Value = 0);
            owner.Storage.BindGraphTrace((_, _) => { }); owner.Storage.BindDisplayDiagnostics(new StringBox(), (_, _, _) => { });
            owner.Storage.BindDisplayDevices(new() { View = () => new(), Suppressed = () => true, Invalidate = (_, _) => throw new IOException("미선택 dirty") });
        });
        var session = new HostSession(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates);
        _ = HostPostPopRuntimeTests.Build(session, surface.Storage, true); _ = session.BuildPlayerWorldHooks();
        int parent = session.Pool.Allocate(SidAllocation.Client); Span<byte> raw = session.Pool.AllocatedBytes(parent); raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501dd0); raw[10] = 74;
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            SquidProcessHost host = session.PlayerProcesses; host.SetRegularHandler((_, _, _, _) => 0);
            RegularProcess process = Assert.IsType<RegularProcess>(host.AddRegular(parent, 99, 1));
            int form = process.Form(); uint pid = surface.Kernel.Find(process); game.Selection.Selected.Value = 0xffff0005;
            Assert.Equal(0xffff0005u, host.Asset()!.SelectedRaw!());
            session.RunMissionFrame(); Assert.Null(surface.Kernel.Get(pid)); Assert.Equal(1, session.Pool.Slot(form)[11] & 1);
            Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]));
            Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth); Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
            Assert.Throws<InvalidOperationException>(() => host.Asset()!.Emit!(new(SquidDestroyEffect.DestroyDependent, parent, 0, 0)));
            Assert.Equal(0x501dd0u, BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(parent)));
        }
        session.ReleasePlayerRecordViews(); templates.ClearAll();
    }
}
