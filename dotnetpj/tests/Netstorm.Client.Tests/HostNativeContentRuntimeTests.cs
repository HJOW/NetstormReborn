using System.Buffers.Binary;
using Netstorm.Core.Original;

// 장기 Native 내용물 통지가 실제 미션 세션과 수명을 공유하는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 생산·연구 부모 통지와 현재 세션 대여의 결합을 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>공통 분리에서 실제 연구 본문까지 전달하고 세션 반납 뒤 새 대여를 허용한다.</summary>
    [Fact]
    public void NativeContentDetachUsesProductionSessionAndReleasesItsLease()
    {
        var (session, _, surface) = WorldDeletionRootScene(sharedNative: true);
        session.BuildFlareWorldDevices((_, _, _, _) => throw new InvalidOperationException("선택되지 않은 플레어 소리"));
        session.BuildProductionWorldDevices(ProductionDevices());
        int parent = MakeWorldDeletionSlot(session, 74, 0, 0, 1, 0x509148u);
        int form = surface.Storage.FactoryExecutor().CreateContained(2);
        // 외곽 Runtime이 공개되기 전에는 실제 파생 통지를 실행할 수 없다.
        Assert.Throws<InvalidOperationException>(() => surface.Storage.NoticeContent(parent, form, 2));
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            surface.Storage.ContentForms().Attach(form, parent, 0);
            session.Pool.AllocatedBytes(form)[20] = 16;
            uint before = session.ProductionDirty;
            surface.Storage.ContainedUnpopExecutor().Detach(form);
            Assert.Equal(before + 1, session.ProductionDirty);
            Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]));
            Assert.False(session.Gameplay.RuntimeCallActive(session));
            session.ReleasePlayerProcesses();
        }
        Assert.Throws<InvalidOperationException>(() => surface.Storage.NoticeContent(parent, form, 2));
        var next = new RawSurfaceContentBindings();
        surface.Storage.BindContentRuntime(next);
        Assert.Throws<ArgumentException>(() => surface.Storage.ReleaseContentRuntime(new RawSurfaceContentBindings()));
        surface.Storage.ReleaseContentRuntime(next);
    }
}
