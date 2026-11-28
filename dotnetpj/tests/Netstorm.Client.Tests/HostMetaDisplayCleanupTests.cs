using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 메타 이탈의 실제 가상 삭제가 현재 Runtime을 빌리는 동안 공통 Form 수명을 끝내는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 Native 세션/Pool/분배기의 구역·참가자·전투 정리를 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>구역→참가자→전투의 세 실제 Form을 pin된 가상 삭제로 반납하고 부모와 다른 슬롯을 보존한다.</summary>
    [Fact]
    public void MetaDisplayCleanupUsesActualCurrentRuntimeAndNativeFormDispatch()
    {
        var (session, _, surface) = WorldDeletionRootScene(sharedNative: true);
        session.BuildWorldDeletionRootDevices();
        var challenge = new ClientChallengeGlobals(); Assert.True(challenge.Prepare());
        var calls = new List<int>();
        using (var runtime = HostMissionRuntime.Create(session, surface.Storage))
        {
            int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
            var forms = new List<int>();
            // 세 실제 프로세스/Form은 같은 부모에 붙지만 각 정리 단계의 SID를 독립 관측한다.
            for (int index = 0; index < 3; index++)
            {
                int selected = index;
                var process = new ObservedWorldDeletionProcess(_ =>
                {
                    Assert.True(session.Gameplay.RuntimeCallActive(session)); calls.Add(selected);
                });
                session.PlayerProcesses.Attach(process, 10, parent, 0x50); forms.Add(process.Form());
            }
            BinaryPrimitives.WriteUInt32LittleEndian(session.LocalState.MetaZoneRecords.Span, 1);
            BinaryPrimitives.WriteUInt32LittleEndian(session.LocalState.MetaZoneRecords.Span.Slice(76, 4), (uint)forms[0]);
            BinaryPrimitives.WriteUInt32LittleEndian(challenge.Participant(0).Span.Slice(0xb8, 4), (uint)forms[1]);
            BinaryPrimitives.WriteUInt32LittleEndian(challenge.Battle(0).Span.Slice(0x20, 4), (uint)forms[2]);
            NativeMetaDisplayCleanup.LeaveCurrent(challenge, surface, session.LocalState);
            Assert.Equal(new[] { 0, 1, 2 }, calls); Assert.False(session.Gameplay.RuntimeCallActive(session));
            // 삭제된 Form만 free이고 같은 부모의 할당과 Runtime 대여는 보존되어야 한다.
            foreach (int form in forms) Assert.NotEqual(0, session.Pool.Slot(form)[11] & RawSidPool.FreeBit);
            Assert.Equal(0, session.Pool.Slot(parent)[11] & RawSidPool.FreeBit);
            Assert.True(session.Gameplay.Borrowed()); Assert.True(surface.Storage.Borrowed());
            Assert.Equal(0u, surface.Storage.BaseDestroy().PreDepth); Assert.Equal(0u, surface.Storage.BaseDestroy().PostDepth);
            session.ReleasePlayerProcesses();
        }
    }
}
