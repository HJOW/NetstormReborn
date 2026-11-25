using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 같은 세션 풀·공급 목록·동맹에서 배치 에너지 조립·실행을 검증한다.
// 2026-11-26 1.055에서 SL-3b 에너지 결선용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>세션 에너지 장치 조립과 실제 공급 판정 진입을 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 세션 풀에 공급원 SID를 할당하고 DWORD(가상 표·타입·좌표·소유자)를 쓴다.</summary>
    private static int PlaceSessionProvider(HostSession session, uint vtable, int type, float x, float y, uint owner)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, vtable);
        raw[10] = unchecked((byte)type);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14), x);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18), y);
        raw[34] = unchecked((byte)owner);
        return sid;
    }

    /// <summary>에너지 조립은 같은 세션 공급·동맹·반경을 빌리며 미조립 실행은 거부된다.</summary>
    [Fact]
    public void EnergyDevicesRequireRealSessionTableAndSingleBinding()
    {
        var (session, types) = Scene();
        Assert.Throws<InvalidOperationException>(() => session.EnergySupply);
        Assert.Throws<InvalidOperationException>(() => session.CollectEnergy(0f, 0f, 50, 1));
        session.BuildEnergyDevices(types, _ => "");
        Assert.Equal(484f, session.EnergyState.RadiusSquared);
        Assert.Equal(1u, session.EnergyState.Allied);
        Assert.Equal(session.Gameplay.UserCheat.Value, session.EnergyState.UserCheat);
        Assert.Throws<InvalidOperationException>(() => session.BuildEnergyDevices(types, _ => ""));
    }

    /// <summary>주입 요구 문자열로 수집·평가·소비·정렬이 같은 세션 본체에 닿는다.</summary>
    [Fact]
    public void InjectedRequirementDrivesSessionEnergyRunners()
    {
        var (session, types) = Scene();
        // 같은 내용의 복제 표에서 50번만 거푸집·원소 고정값으로 둔다. 실제 표는 건드리지 않는다.
        var energyTypes = types.Select(record => record.Clone()).ToList();
        energyTypes[50].FootX = 2; energyTypes[50].FootY = 2; energyTypes[50].Theme = 0;
        session.Gameplay.PlayerRecords.Alliances[2 * 9 + 1] = 1;
        session.BuildEnergyDevices(energyTypes, _ => "wr");
        int first = PlaceSessionProvider(session, 0x00501ea0u, 50, 60f, 60f, 1);
        int second = PlaceSessionProvider(session, 0x00501ea0u, 50, 70f, 60f, 2);
        session.SidLists.Providers.Append(first); session.SidLists.Providers.Append(second);
        // 자신·동맹이 수집되고 선택 목록에 남는다.
        Assert.Equal("ww", session.CollectEnergy(65f, 60f, 50, 1));
        Assert.Equal(new[] { unchecked((uint)first), unchecked((uint)second) }, session.EnergySelected.Items().ToArray());
        // 요구 "wr"는 "w" 하나로 못 채우므로 0과 "Wr"이 된다.
        Assert.Equal(0u, session.EvaluateEnergy(65f, 60f, 50, 1, out string output));
        Assert.Equal("Wr", output);
        // 가까운 순(둘째·첫째)으로 다시 보관하고 앞에서부터 소비한다.
        session.SortEnergySources("ww", 69f, 60f);
        Assert.Equal(new[] { unchecked((uint)second), unchecked((uint)first) }, session.EnergySelected.Items().ToArray());
        Assert.Equal(unchecked((uint)second), session.TakeEnergySource());
        Assert.Equal(unchecked((uint)first), session.TakeEnergySource());
        Assert.Equal(0u, session.TakeEnergySource());
    }

    /// <summary>우회가 켜지면 수집 없이 성공한다.</summary>
    [Fact]
    public void EnergyOverrideSkipsCollection()
    {
        var (session, types) = Scene();
        session.BuildEnergyDevices(types, _ => "wr");
        session.EnergyState.Special = 1; session.EnergyState.Ignore = 1;
        Assert.Equal(1u, session.EvaluateEnergy(60f, 60f, 50, 1, out string output));
        Assert.Equal("", output);
    }
}
