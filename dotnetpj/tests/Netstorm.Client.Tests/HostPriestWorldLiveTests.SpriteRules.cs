using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 미션 세션의 객체 표시 조회 장치를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 사제·공장·프로세스 호스트를 객체 표시에서 사용한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>현재 HP·건설/자원 정기 프로세스와 실제 공간 해시를 조회한다.</summary>
    [Fact]
    public void SpriteRulesUseActualSessionFactoryAndProcesses()
    {
        var (session, types) = Scene();
        int priest = PlacePriest(session, types);
        SpriteDrawHooks rules = session.BuildSpriteWorldRules(session.TerrainPopHash!, types);
        Assert.True(rules.TypeHitPoints!(158) > 0);
        Assert.False(rules.ConstructionActive!(priest));
        Assert.Null(rules.Resource!(priest));
        Assert.Null(rules.Timers!(priest, 31));
        Assert.Single(rules.StatusProcesses!());
        RegularProcess construction = Assert.IsType<RegularProcess>(session.PlayerProcesses.AddRegular(priest, 0x25d, 0));
        Assert.True(rules.ConstructionActive(priest));
        RegularProcess resource = Assert.IsType<RegularProcess>(session.PlayerProcesses.AddRegular(priest, 0x493, 0));
        uint emptyKind = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(0)[18..]);
        Assert.Equal(new SpriteResourceStatus(emptyKind, 0), rules.Resource(priest));
        ushort previous = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(priest)[6..]);
        int content = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> child = session.Pool.AllocatedBytes(content);
        child[10] = 6;
        BinaryPrimitives.WriteUInt16LittleEndian(child[4..], previous);
        BinaryPrimitives.WriteUInt16LittleEndian(child[18..], 84);
        BinaryPrimitives.WriteUInt16LittleEndian(child[20..], 1);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(priest)[6..], (ushort)content);
        Assert.Equal(new SpriteResourceStatus(84, 0), rules.Resource(priest));
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(priest)[6..], previous);
        session.PlayerProcesses.Kill(resource, 0x40);
        session.PlayerProcesses.Kill(construction, 0x40);
        Assert.False(rules.ConstructionActive(priest));
        Assert.Null(rules.Resource(priest));
        session.ReleasePlayerProcesses();
    }

    /// <summary>실제 커널에 상태 효과 Form이 있으면 미이식 타이머를 빈 목록으로 숨기지 않는다.</summary>
    [Fact]
    public void SpriteRulesRejectUnportedActualStatusTimer()
    {
        var (session, types) = Scene();
        int priest = PlacePriest(session, types);
        SpriteDrawHooks rules = session.BuildSpriteWorldRules(session.TerrainPopHash!, types);
        SquidProcess status = Assert.IsType<SquidProcess>(session.PlayerProcesses.Attach(new SquidProcess(), 57, priest, 0x50));
        Assert.Throws<InvalidOperationException>(() => rules.StatusProcesses!());
        session.PlayerProcesses.Kill(status, 0x40);
        Assert.Single(rules.StatusProcesses!());
        session.ReleasePlayerProcesses();
    }
}
