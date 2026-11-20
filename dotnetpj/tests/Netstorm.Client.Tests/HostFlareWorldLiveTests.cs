using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 전투 없이 현재 역할·로딩깊이·실제Form/정기/공간/표시·소리의 플레어 결선을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>원본 대조 플레어 본체를 실제 미션 장치로 호출하는 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>현재 억제·로딩 관문과 장기TYPE를 읽고 실제Regular 부착부터 마지막삭제까지 수행한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductFlareUsesCurrentGatesRoleAndHostLifecycle()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ConstructionParent(scene);
        session.Pool.AllocatedBytes(parent)[35] = 7;
        session.GameAux.BombFlareTypeBox.Value = uint.MaxValue; session.WorldLoginEnd.Value = 1;
        session.WorldFlare.Spawn(parent, 2, 1); Assert.Equal(0, session.Kernel.Size());
        session.WorldLoginEnd.Value = 0; session.SetWorldLoadingDepth(1);
        session.WorldFlare.Spawn(parent, 2, 1); Assert.Equal(0, session.Kernel.Size());
        session.SetWorldLoadingDepth(0); session.GameAux.BombFlareTypeBox.Value = 106;
        session.ProcessTime.Value = 2; session.WorldFlare.Spawn(parent, 2, 1);
        Assert.Equal(2, session.Kernel.Size()); var forms = new List<int>();
        // 실제 두 정기는 같은Host에 붙은client Form을 부모로 사용한다.
        for (uint pid = 1; pid <= 2; pid++)
        {
            var process = Assert.IsType<RegularProcess>(session.Kernel.Get(pid)); int effect = process.Parent(); forms.Add(effect);
            Assert.True(session.WorldFlare.Handles(effect)); Assert.Equal(1u, process.Event()); Assert.Equal(2, process.Time());
            Assert.InRange(process.Payload(), .05f, .12f); Assert.Equal(0u, process.Count());
            ReadOnlySpan<byte> raw = session.Pool.Slot(effect); Assert.Equal(106, raw[10]); Assert.Equal(0, raw[34]); Assert.Equal(6, raw[35]); Assert.Equal(8, raw[40] & 8);
            Assert.InRange(effect, 5, 14999);
            Assert.Throws<InvalidOperationException>(() => process.InitializeReserved(99, 99));
            Assert.Equal(1u, process.Event());
        }
        // 실제 정기의 재예약/현재프레임/끝표시/공통삭제로 효과와Form이 모두 반납된다.
        for (int frame = 0; frame < 200 && session.Kernel.Size() != 0; frame++) session.AdvanceMissionFrame(.15);
        Assert.Equal(0, session.Kernel.Size());
        // 풀 반납은 원본 free 비트로 확인하며 임의의 새 게임 객체를 만들지 않는다.
        foreach (int form in forms) Assert.Equal(1, session.Pool.Slot(form)[11] & 1);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.WorldFlare);
    }

    /// <summary>B구간 중간의 현재 위치소리와 후속플레어가 실제 장치/음소거 관문을 통과해 완전히 종료한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductFlareChainsThroughActualMutedAudioAndWorld()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ConstructionParent(scene);
        session.WorldFlare.SpawnDirection(parent, 1, 2, 106, 1); Assert.Equal(1, session.Kernel.Size());
        var root = Assert.IsType<RegularProcess>(session.Kernel.Get(1)); int first = root.Parent(); bool chained = false;
        // 사건2의 끝 전단계는새효과/사건1을만들며 원문 정기순서로각각끝난다.
        for (int frame = 0; frame < 200 && session.Kernel.Size() != 0; frame++)
        {
            session.AdvanceMissionFrame(.15); chained |= session.Kernel.Size() > 1;
        }
        Assert.True(chained); Assert.Equal(0, session.Kernel.Size()); Assert.Equal(1, session.Pool.Slot(first)[11] & 1);
    }

    /// <summary>현재SHP 기준점으로생성한B구간이원본 powerup 이름·위치·flags를실제소리경계에보낸다.</summary>
    [Fact]
    public void FlareCurrentShapeAndSoundConsumerAreConnected()
    {
        var (session, _) = Scene(); _ = session.BuildPlayerWorldHooks();
        var sounds = new List<(float, float, string, uint)>();
        session.BuildFlareWorldDevices((x, y, name, flags) => sounds.Add((x, y, name, flags)));
        try
        {
            int parent = session.BuildPlayerWorldHooks().Create!(85, 2); Span<byte> raw = session.Pool.AllocatedBytes(parent);
            BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 20); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 20);
            session.WorldFlare.SpawnDirection(parent, 1, 1, 106, 1);
            var root = Assert.IsType<RegularProcess>(session.Kernel.Get(1)); int effect = root.Parent();
            float x = BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(effect)[14..]);
            float y = BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(effect)[18..]);
            // 같은B구간정기를현재시계로진행시켜중간소리를관찰한다.
            for (int frame = 0; frame < 200 && session.Kernel.Size() != 0; frame++) session.AdvanceMissionFrame(.15);
            Assert.Equal(new[] { (x, y, "powerup.wav", 0u) }, sounds); Assert.Equal(0, session.Kernel.Size());
        }
        finally { session.ReleasePlayerProcesses(); }
    }
}
