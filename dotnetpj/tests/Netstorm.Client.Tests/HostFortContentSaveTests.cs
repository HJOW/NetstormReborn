using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

// 실제 Native 생산 내용물의 동일 Pool/Template와 대여 수명을 저장 어댑터에 연결한다.
namespace Netstorm.Client.Tests;

/// <summary>호스트 내용물 저장이 실제 Client/Runtime을 교체하거나 loaded를 합성하지 않는지 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 생성/부착된 Content를 현재 템플릿에 저장하고 Pool·Runtime·설치 관문을 보존한다.</summary>
    [Fact]
    public void FortContentSaveBorrowsActualNativeProductionPoolAndTemplate()
    {
        var (session, types, surface) = WorldDeletionRootScene(sharedNative: true);
        session.BuildFlareWorldDevices((_, _, _, _) => throw new InvalidOperationException("저장 중 선택되지 않은 소리"));
        session.BuildProductionWorldDevices(ProductionDevices());
        session.Templates.Install(1, EmptyFort(), "native-contents.fort");
        int parent = MakeWorldDeletionSlot(session, 74, 0, 0, 1, 0x509148);
        int form = surface.Storage.FactoryExecutor().CreateContained(2);
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            surface.Storage.ContentForms().Attach(form, parent, 0);
            // 원문 byte 쓰기와 부모 fallback을 독립 입력으로 고정하되 생성/종속 수명은 실제 장치를 쓴다.
            BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(form)[18..], 71);
            session.Pool.AllocatedBytes(form)[20] = 16;
            types[71].Flags1 = 0; types[71].ContentListFlags = 0; types[74].ContainerListFlags = 0;
            byte[] before = session.Pool.Slot(form).ToArray(), parentBefore = session.Pool.Slot(parent).ToArray();
            TemplateWriteStream stream = session.Templates.CreateSection(1, "Technology");
            Assert.False(session.NativeLoaded);
            NativeFortContentSave.Save(surface, session.Templates, (uint)parent, stream);
            Assert.Equal(new byte[] { 1, 71, 16 }, session.Templates.SectionBuffer(stream).ToArray());
            Assert.Equal(before, session.Pool.Slot(form).ToArray()); Assert.Equal(parentBefore, session.Pool.Slot(parent).ToArray());
            Assert.False(session.NativeLoaded); Assert.True(session.Gameplay.Borrowed()); Assert.True(surface.Storage.Borrowed());
            Assert.False(session.Gameplay.RuntimeCallActive(session));
            session.ReleasePlayerProcesses();
        }
        Assert.False(session.Gameplay.Borrowed()); Assert.False(surface.Storage.Borrowed());
    }
}
