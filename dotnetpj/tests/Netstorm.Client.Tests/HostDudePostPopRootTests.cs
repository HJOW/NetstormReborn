using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 같은 Host의 Dude 전용 후처리가 최초 idle·등록·I 프레임을 수행하는지 검증한다.
// 2026-11-26 1.038에서 WP-4b Dude 전용 PostPop용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>Dude 전용 후처리 설치와 최초·재Pop 효과를 같은 세션에서 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>제품 흐름과 같은 순서로 지형·섬·지역·패킷 장치를 연결한 세션을 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface) DudePostPopRootScene()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        session.BuildRegionOwnershipDevices(types);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = _ => { } });
        return (session, types, surface);
    }

    /// <summary>실제 지형 도장으로 지면을 만들고 Dude 슬롯을 할당해 가상 표·타입·소유자·좌표를 채운다.</summary>
    private static int PlaceDude(HostSession session, IReadOnlyList<OriginalTypeRecord> types, uint owner = 1, float x = 50, float y = 60)
    {
        TerrainStampHooks stamp = session.BuildTerrainStampHooks(types);
        int ground = stamp.Create!(97, 1);
        stamp.Pop!(ground, 50, 60, 0x02000061);
        // 공장 Create는 Dude 가상 표를 쓰지 않으므로 슬롯을 직접 채운다. 근거: HostSessionDudeTests.PrepareDudeSid다.
        // Pop 진입에 void 비트가 필요하므로(원본 부모의 Unchanged 관문) free·contained 없이 void만 둔다.
        int dude = session.Pool.Allocate(SidAllocation.Client);
        Span<byte> raw = session.Pool.AllocatedBytes(dude);
        raw.Fill(0);
        raw[RawSidPool.StateOffset] = 4;
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x00508cc8u);
        raw[RawSidPool.TypeOffset] = 70;
        raw[34] = unchecked((byte)owner);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(14, 4), x);
        BinaryPrimitives.WriteSingleLittleEndian(raw.Slice(18, 4), y);
        return dude;
    }

    /// <summary>최초 Pop이 idle 작업 번호·population 등록·I 프레임을 쓰고 공통 깊이를 복원한다.</summary>
    [Fact]
    public void DudeFirstPopRegistersPopulationIdleAndIFrame()
    {
        var (session, types, _) = DudePostPopRootScene();
        Assert.Equal(0u, session.PostPopDepth);
        int dude = PlaceDude(session, types);
        int expected = session.WorldFrames[70].Run((byte)'I').First;
        Assert.NotEqual(-1, expected);
        session.PopWorldObject(dude, 50, 60, 1);
        Assert.Equal(0u, session.PostPopDepth);
        Span<byte> raw = session.Pool.AllocatedBytes(dude);
        Assert.Equal(0x01, raw[12]);
        Assert.Equal(0x51, raw[13]);
        Assert.Contains(unchecked((uint)dude), session.SidLists.Dudes.Items().ToArray());
        Assert.Equal(expected, BinaryPrimitives.ReadInt32LittleEndian(raw[36..]));
        session.ReleasePlayerProcesses();
    }

    /// <summary>최초 비트 없는 재Pop은 등록을 늘리지 않고 공통 몸체만 실행한다.</summary>
    [Fact]
    public void DudeRepopRunsBaseWithoutDuplicateRegistration()
    {
        var (session, types, _) = DudePostPopRootScene();
        int first = PlaceDude(session, types);
        uint before = session.SidLists.Dudes.Count;
        session.PopWorldObject(first, 50, 60, 1);
        Assert.Equal(before + 1, session.SidLists.Dudes.Count);
        Assert.Contains(unchecked((uint)first), session.SidLists.Dudes.Items().ToArray());
        // 해제 뒤 다시 등록하면 최초 비트가 없으므로 등록을 늘리지 않고 공통 몸체만 실행한다.
        // 근거: RawSquidPopParent.Run 198행은 raw[40] 0x80이 있으면 FirstPop을 합치지 않는다.
        session.UnpopWorldObject(first, 0);
        session.PopWorldObject(first, 51, 60, 0);
        Assert.Equal(before + 1, session.SidLists.Dudes.Count);
        Assert.Equal(0u, session.PostPopDepth);
        session.ReleasePlayerProcesses();
    }

    /// <summary>전용 몸체 미설치 Dude Pop은 공통 성공으로 바꾸지 않고 명시 실패한다.</summary>
    [Fact]
    public void DudePopWithoutBodyFailsExplicitly()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        int dude = PlaceDude(session, types);
        var failure = Assert.Throws<InvalidOperationException>(() => session.PopWorldObject(dude, 50, 60, 1));
        Assert.Contains("미복원", failure.Message);
        session.ReleasePlayerProcesses();
    }

    /// <summary>전용 후처리 설치는 한 번만 허용하고 장치 없이는 거부한다.</summary>
    [Fact]
    public void DudePostPopInstallIsSingleAndRequiresDevices()
    {
        var (session, types, _) = DudePostPopRootScene();
        Assert.Throws<InvalidOperationException>(() => session.BuildDudePostPopDevices(types));
        session.ReleasePlayerProcesses();
        var bare = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildDudePostPopDevices(types));
        bare.ReleasePlayerRecordViews();
    }

    /// <summary>몸체 단위에서 타입·가상 표를 가리고 가득 찬 목록에서도 프레임·공통 호출을 유지한다.</summary>
    [Fact]
    public void DudePostPopBodyRejectsWrongTypeAndCapsPopulation()
    {
        var pool = new RawSidPool(32768);
        var frames = new List<RiftTypeFrames>();
        for (int index = 0; index < 188; index++)
        {
            frames.Add(new RiftTypeFrames(index == 70 ? [new RiftFrameCode((byte)'I', (byte)'A', 0, 0)] : []));
        }

        var population = new RawPostPopList { Entries = [7u], Count = 1 };
        int validated = 0, based = 0, framed = 0, notified = 0;
        var hooks = new DudePostPopHooks
        {
            SetFrame = (_, _, _) => framed++,
            ValidateBase = (_, _) => validated++,
            Base = (_, _) => based++,
            NotifySurface = _ => notified++,
        };
        var body = new RawDudePostPop(pool, frames, new DudeSpawnState { Population = population }, hooks, () => 82u);
        int dude = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(dude);
        raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x00508cc8u);
        raw[RawSidPool.TypeOffset] = 70;
        Assert.True(body.Handles(dude));
        body.Validate(dude, 1);
        Assert.Equal(1, validated);
        body.PostPop(dude, 1);
        // 가득 찬 목록에는 추가하지 않지만 I 프레임·공통 호출은 유지한다.
        Assert.Equal([7u], population.Items().ToArray());
        Assert.Equal(1u, population.Count);
        Assert.Equal(1, framed);
        Assert.Equal(1, based);
        Assert.Equal(0x01, pool.AllocatedBytes(dude)[12]);
        Assert.Equal(0x51, pool.AllocatedBytes(dude)[13]);
        Assert.Equal(0, notified);
        raw[RawSidPool.TypeOffset] = 71;
        // PATCH 분기는 가상 표만 보므로 Handles는 유지되고 Validate가 타입을 거부한다.
        Assert.True(body.Handles(dude));
        Assert.Throws<ArgumentException>(() => body.Validate(dude, 1));
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0xDEADBEEFu);
        Assert.False(body.Handles(dude));
        var other = new RawDudePostPop(new RawSidPool(32768), frames, new DudeSpawnState(), hooks);
        Assert.Throws<ArgumentException>(() => new RawPostPop(pool, [], new RawPostPopState()).SetDudePostPop(other));
    }
}
