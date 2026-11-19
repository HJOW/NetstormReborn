// AI 월드 등록(전체 스캔·간헐천 분류)의 분기·순서를 실제 메모리·슬롯으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 월드 등록을 검사하는 클래스다. 2026-11-19 0.649에서 R4-5 AI 등록용으로 추가했다.
// 근거: cpppj o/RawAiWorldRegistration.cpp의 FullScan·Geyser·CloserToAlly·ClassifySurface다.
public sealed class RawAiWorldRegistrationTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.649에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>본체 필드를 읽는다. 0.649에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.649에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.649에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>등록 장면을 만든다. 0.649에서 추가했다.</summary>
    private static (RawSidPool Pool, RawAiBrainMemory Memory, uint Token, byte[] Players, uint[] Allies, ushort[] Surface, OriginalTypeRecord[] Types) BuildScene()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }

        return (pool, memory, token, new byte[9 * 180], new uint[81], new ushort[65536], types);
    }

    /// <summary>슬롯을 할당하고 0으로 채운다. 0.649에서 추가했다.</summary>
    private static int AllocateZero(RawSidPool pool)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid).Fill(0);
        return sid;
    }

    /// <summary>좌표·타입·소유자를 가진 자산 슬롯을 만든다. 0.649에서 추가했다.</summary>
    private static int PrepareAsset(RawSidPool pool, byte type, uint owner, float x, float y)
    {
        int sid = AllocateZero(pool);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[10] = type;
        raw[34] = unchecked((byte)owner);
        Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        return sid;
    }

    /// <summary>목록 블록을 본체에 연결한다. 0.649에서 추가했다.</summary>
    private static void BindList(RawAiBrainMemory memory, uint token, uint offset, uint capacity, uint count = 0)
    {
        uint block = memory.Allocate(unchecked((int)capacity) * 4);
        Set(memory, token, offset, block);
        Set(memory, token, offset + 4, capacity);
        Set(memory, token, offset + 8, count);
    }

    /// <summary>통지 본체를 만든다. 0.649에서 추가했다.</summary>
    private static RawAiObjectNotice BuildNotice(RawAiBrainMemory memory, RawSidPool pool, OriginalTypeRecord[] types, byte[] players,
        List<(uint Token, AiObjectLog Log)>? logs = null) => new(
        memory, pool, types, players, () => 10.0, () => 999u, () => false,
        new AiObjectNoticeHooks
        {
            CarrierTargets = _ => new uint[2],
            DistanceSquared = (_, _) => 1e9,
            Log = (brain, entry) => logs?.Add((brain, entry)),
        });

    /// <summary>등록 본체를 만든다. 0.649에서 추가했다.</summary>
    private static RawAiWorldRegistration BuildRegistration(RawAiBrainMemory memory, RawSidPool pool, OriginalTypeRecord[] types,
        byte[] players, uint[] allies, ushort[] surface, RawAiObjectNotice objects,
        List<(uint Token, uint Sid)>? farther = null, List<uint>? missing = null) => new(
        memory, pool, types, players, allies, surface, objects,
        new AiWorldRegistrationHooks
        {
            Center = _ => new float[] { 0, 0 },
            FartherGeyser = (brain, sid) => farther?.Add((brain, sid)),
            MissingSurface = sid => missing?.Add(sid),
        });

    /// <summary>상태 0은 다섯 목록에서 모두 제거한다. 0.649에서 추가했다.</summary>
    [Fact]
    public void GeyserZeroRemovesAllLists()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        var notice = BuildNotice(memory, pool, types, players);
        var body = BuildRegistration(memory, pool, types, players, allies, surface, notice);
        foreach (uint at in new uint[] { 0x324u, 0x330u, 0x33cu, 0x348u, 0x354u })
        {
            BindList(memory, token, at, 8, 1);
            Put(memory.Bytes(Get(memory, token, at)), 0, 77u);
        }

        body.Geyser(token, 77u, 0);
        foreach (uint at in new uint[] { 0x324u, 0x330u, 0x33cu, 0x348u, 0x354u })
        {
            Assert.Equal(0u, Get(memory, token, at + 8));
        }
    }

    /// <summary>가깝고 표면이 있으면 324·348 목록에 넣는다. 0.649에서 추가했다.</summary>
    [Fact]
    public void GeyserNearClassifiesSurface()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        types[70].Flags2 = 0x10000000u;
        int geyser = PrepareAsset(pool, 70, 1, 10.5f, 20.5f);
        int ground = AllocateZero(pool);
        surface[20 * 256 + 10] = unchecked((ushort)ground);
        var notice = BuildNotice(memory, pool, types, players);
        var farther = new List<(uint Token, uint Sid)>();
        var body = BuildRegistration(memory, pool, types, players, allies, surface, notice, farther);
        BindList(memory, token, 0x324, 8);
        BindList(memory, token, 0x348, 8);
        Set(memory, token, 0x260, 1u);
        body.Geyser(token, unchecked((uint)geyser), 1);
        Assert.Equal(1u, Get(memory, token, 0x324 + 8));
        Assert.Equal(1u, Get(memory, token, 0x348 + 8));
        Assert.Empty(farther);
    }

    /// <summary>동맹 쪽이 가까우면 후보에 넣지 않고 로그만 남긴다. 0.649에서 추가했다.</summary>
    [Fact]
    public void GeyserCloserToAllyLogsOnly()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        types[70].Flags2 = 0x10000000u;
        int geyser = PrepareAsset(pool, 70, 1, 10.0f, 10.0f);
        allies[1 * 9 + 2] = 1u;
        Put(players, 2 * 180 + 0x7c, 5u);
        var notice = BuildNotice(memory, pool, types, players);
        var farther = new List<(uint Token, uint Sid)>();
        var body = new RawAiWorldRegistration(memory, pool, types, players, allies, surface, notice,
            new AiWorldRegistrationHooks
            {
                Center = sid => sid == 5u ? new float[] { 11.0f, 10.0f } : new float[] { 100.0f, 100.0f },
                FartherGeyser = (brain, sid) => farther.Add((brain, sid)),
                MissingSurface = _ => { },
            });
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x268, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x26c, BitConverter.SingleToUInt32Bits(0.0f));
        body.Geyser(token, unchecked((uint)geyser), 1);
        Assert.Single(farther);
        Assert.Equal(0u, Get(memory, token, 0x324 + 8));
    }

    /// <summary>표면 0은 진단 뒤 0슬롯 분류로 348에 넣는다. 0.649에서 추가했다.</summary>
    [Fact]
    public void MissingSurfaceClassifiesZero()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        types[70].Flags2 = 0x10000000u;
        int geyser = PrepareAsset(pool, 70, 1, 300.0f, 300.0f);
        var notice = BuildNotice(memory, pool, types, players);
        var missing = new List<uint>();
        var body = BuildRegistration(memory, pool, types, players, allies, surface, notice, missing: missing);
        BindList(memory, token, 0x324, 8);
        BindList(memory, token, 0x348, 8);
        Set(memory, token, 0x260, 1u);
        body.Geyser(token, unchecked((uint)geyser), 1);
        Assert.Equal([unchecked((uint)geyser)], missing);
        Assert.Equal(1u, Get(memory, token, 0x348 + 8));
    }

    /// <summary>전체 스캔은 간헐천·자산·방어를 등록하고 3c8=3을 쓴다. 0.649에서 추가했다.</summary>
    [Fact]
    public void FullScanRegistersWorld()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        types[70].Flags2 = 0x10000000u;
        int geyser = PrepareAsset(pool, 70, 1, 10.5f, 20.5f);
        int asset = PrepareAsset(pool, 71, 1, 30.0f, 30.0f);
        int foreign = PrepareAsset(pool, 72, 2, 40.0f, 40.0f);
        int defense = PrepareAsset(pool, 73, 1, 50.0f, 50.0f);
        types[73].Flags2 = 0x4000000u;
        int ground = AllocateZero(pool);
        surface[20 * 256 + 10] = unchecked((ushort)ground);
        var notice = new RawAiObjectNotice(memory, pool, types, players, () => 10.0, () => 999u, () => false,
            new AiObjectNoticeHooks
            {
                CarrierTargets = _ => new uint[2],
                DistanceSquared = (_, _) => 1e9,
                Log = (_, _) => { },
            });
        var body = BuildRegistration(memory, pool, types, players, allies, surface, notice);
        foreach (uint at in new uint[] { 0x324u, 0x330u, 0x348u })
        {
            BindList(memory, token, at, 8);
        }

        Set(memory, token, 0x260, 1u);
        // FullScan은 통지 본체를 직접 부르므로 목록·3c8을 대조한다.
        body.FullScan(token);
        Assert.Equal(3u, Get(memory, token, 0x3c8));
        Assert.Equal(1u, Get(memory, token, 0x324 + 8));
        Assert.Equal(1u, Get(memory, token, 0x348 + 8));
        Assert.Equal(unchecked((uint)geyser), BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(Get(memory, token, 0x324)).AsSpan(0, 4)));
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.649에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, ushort[] surface, OriginalTypeRecord[] types) = BuildScene();
        var notice = BuildNotice(memory, pool, types, players);
        var hooks = new AiWorldRegistrationHooks { Center = _ => new float[] { 0, 0 }, FartherGeyser = (_, _) => { }, MissingSurface = _ => { } };
        Assert.Throws<ArgumentException>(() => new RawAiWorldRegistration(memory, pool, new OriginalTypeRecord[10], players, allies, surface, notice, hooks));
        Assert.Throws<ArgumentException>(() => new RawAiWorldRegistration(memory, pool, types, new byte[10], allies, surface, notice, hooks));
        Assert.Throws<ArgumentException>(() => new RawAiWorldRegistration(memory, pool, types, players, allies, new ushort[10], notice, hooks));
        Assert.Throws<ArgumentNullException>(() => new RawAiWorldRegistration(memory, pool, types, players, allies, surface, notice, null!));
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => new RawAiWorldRegistration(memory, cdPool, types, players, allies, surface, notice, hooks));
        var body = BuildRegistration(memory, pool, types, players, allies, surface, notice);
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => body.FullScan(small));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Geyser(token, unchecked((uint)PoolCapacity) + 1, 1));
        var bare = new RawAiWorldRegistration(memory, pool, types, players, allies, surface, notice, new AiWorldRegistrationHooks());
        int geyser = PrepareAsset(pool, 70, 1, 300.0f, 300.0f);
        Set(memory, token, 0x260, 1u);
        Assert.Throws<InvalidOperationException>(() => bare.Geyser(token, unchecked((uint)geyser), 1));
    }
}
