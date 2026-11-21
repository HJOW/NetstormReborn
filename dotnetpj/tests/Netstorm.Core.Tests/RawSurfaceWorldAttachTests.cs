using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 World의 자산 부착이 같은 슬롯/종속 UI/링크/후처리/전송을 사용하는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>RawSurfaceWorld.cpp의 자산 가상50 부모와 실제 공통 자식 연결 검사다.</summary>
public sealed class RawSurfaceWorldAttachTests
{
    /// <summary>같은188TYPE/실제 전체 World와 선택 진단 장치를 구성한다.</summary>
    private static RawSurfaceWorld World(Action<string, string, uint> assertion)
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes); storage.BindDisplayDiagnostics(new(), assertion);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames); storage.PostPopState.Suppressed = true;
        storage.GameAux().SquidTransmission.Compress = 0; return world;
    }
    /// <summary>풀 할당 후 지정된 실제 자산 VT와 SID/TYPE/void를 기록한다. 생성자 일치 주장을 위한 입력은 아니다.</summary>
    private static int Object(RawSurfaceWorld world, byte type = 70, bool server = false)
    {
        RawSidPool pool = world.Storage().Pool; int sid = pool.Allocate(server ? SidAllocation.Server : SidAllocation.Client); Span<byte> raw = pool.RawSlot(sid); raw.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501dd0); BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(8, 2), (ushort)sid); raw[10] = type; raw[11] = 4; return sid;
    }
    /// <summary>자산 부착의 무조건 진단은 슬롯 쓰기와 변경 표식보다 먼저 중단할 수 있다.</summary>
    [Fact]
    public void DiagnosticFailurePrecedesAllWritesAndChangedFlag()
    {
        using var world = World((text, file, line) => { Assert.Equal("0", text); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x72au, line); throw new InvalidOperationException("stop"); });
        int sid = Object(world); byte[] before = world.Storage().Pool.Slot(sid).ToArray(); _ = world.TakeChanged();
        Assert.Throws<InvalidOperationException>(() => world.AttachBaseAsset(sid, 0, 0)); Assert.Equal(before, world.Storage().Pool.Slot(sid).ToArray()); Assert.False(world.TakeChanged());
    }
    /// <summary>parent0은 진단 뒤 좌표 접두/UI를 실행하고 공통 부착이 반환한 다음 변경만 기록한다.</summary>
    [Fact]
    public void NullParentKeepsPrefixWritesWithoutPop()
    {
        var trace = new List<uint>(); using var world = World((_, _, line) => trace.Add(line)); int sid = Object(world); RawSidPool pool = world.Storage().Pool;
        pool.RawSlot(sid).Slice(14, 8).Fill(0xa5); uint depth = world.Core().PostPop.ActivationDepth; _ = world.TakeChanged();
        world.AttachBaseAsset(sid, 0, 0); Assert.Equal(new uint[] { 0x72a }, trace); Assert.Equal(new byte[8], pool.Slot(sid).Slice(14, 8).ToArray());
        Assert.Equal(4, pool.Slot(sid)[11]); Assert.Equal(depth, world.Core().PostPop.ActivationDepth); Assert.True(world.TakeChanged());
    }
    /// <summary>실제 링크/공통 Pop이 같은 깊이를 복원하고 기존 부모 머리의 이전 링크만 바꾼다.</summary>
    [Fact]
    public void ActualAttachLinksCurrentHeadAndRunsSharedPostPop()
    {
        var trace = new List<uint>(); using var world = World((_, _, line) => trace.Add(line)); RawWorldCore core = world.Core(); RawSidPool pool = core.Pool;
        int parent = Object(world), old = Object(world), sid = Object(world); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent).Slice(6, 2), (ushort)old);
        pool.RawSlot(sid).Slice(14, 8).Fill(0xa5); pool.RawSlot(old).Slice(14, 4).Fill(0xb6); core.PostPop.ActivationDepth = 7; _ = world.TakeChanged();
        world.AttachBaseAsset(sid, parent, 0x10);
        Assert.Equal(new uint[] { 0x72a, 0x3e3 }, trace); Assert.Equal(parent, RawContainedLink.CommonContainedParent(pool, sid));
        Assert.Equal(0, RawContainedLink.CommonContainedPrevious(pool, sid)); Assert.Equal(sid, RawContainedLink.CommonContainedPrevious(pool, old));
        Assert.Equal(old, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(sid).Slice(4, 2))); Assert.Equal(sid, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent).Slice(6, 2)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid).Slice(18, 4))); Assert.Equal(8, pool.Slot(sid)[11]);
        Assert.Equal(7u, core.PostPop.ActivationDepth); Assert.Equal(new byte[] { 0xb6, 0xb6 }, pool.Slot(old).Slice(14, 2).ToArray()); Assert.True(world.TakeChanged());
    }
    /// <summary>현재 Session 종속 UI가 먼저 실행되며 그 효과 뒤 TYPE/genus/content 전역을 다시 읽는다.</summary>
    [Fact]
    public void SessionUiRunsBeforeCurrentGenusAndContentChecks()
    {
        var trace = new List<string>(); using var world = World((_, _, line) => trace.Add(line.ToString("x"))); RawSidPool pool = world.Storage().Pool;
        int parent = Object(world), sid = Object(world), ui = Object(world, 3); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid).Slice(6, 2), (ushort)ui);
        var content = new UIntBox { Value = 6 }; var detach = new RawSquidContainedUnpop(pool,
            new() { ContentFormType = content, GumpFormType = new() { Value = 3 }, UiEvent = new() { Value = 0x77 } }, new()
            {
                Command = (code, first, second) =>
                {
                    Assert.Equal(0x77u, code); Assert.Equal((uint)sid, first); Assert.Equal(0u, second); trace.Add("ui");
                    Assert.Equal(BitConverter.SingleToUInt32Bits(parent), BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid).Slice(14, 4)));
                    pool.RawSlot(sid)[10] = 71; world.Types()[71].Flags2 = 0x44200; content.Value = 71;
                },
            });
        world.BindContainedUnpop(detach); world.AttachBaseAsset(sid, parent, 0x10);
        Assert.Equal(new[] { "72a", "ui", "3d0", "3e0", "3e3" }, trace); Assert.Equal(8, pool.Slot(sid)[11]); GC.KeepAlive(detach);
    }
    /// <summary>같은 실제 Squid writer가 링크와void 변경 뒤의35BYTE를 장기 버퍼에 기록한다.</summary>
    [Fact]
    public void CommonPopWritesCurrentAttachedAssetThroughActualTransmission()
    {
        var trace = new List<uint>(); using var world = World((_, _, line) => trace.Add(line)); world.Storage().Authority().Server = true;
        int parent = Object(world), sid = Object(world, server: true); RawSidPool pool = world.Storage().Pool;
        world.AttachBaseAsset(sid, parent, 0); byte[] packet = world.Storage().GameAux().SquidTransmission.Buffer;
        Assert.Equal(0x2c, packet[1]); Assert.Equal(sid, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(4, 2))); Assert.Equal(pool.Slot(sid).Slice(14, 8).ToArray(), packet.AsSpan(6, 8).ToArray());
        Assert.Equal(8, packet[17]); Assert.Equal(0u, world.Core().PostPop.ActivationDepth); Assert.Equal(new uint[] { 0x72a, 0x3e3 }, trace);
    }
}
