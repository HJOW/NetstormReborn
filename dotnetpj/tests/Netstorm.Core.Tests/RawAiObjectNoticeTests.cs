// AI 생성·삭제 통지(타입 수·목록·목표·타이머)의 분기·순서를 실제 메모리·슬롯으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 생성·삭제 통지를 검사하는 클래스다. 2026-11-19 0.648에서 R4-5 AI 통지용으로 추가했다.
// 근거: cpppj o/RawAiObjectNotice.cpp의 Notice·Update(원문 004166e0·00414de0)다.
public sealed class RawAiObjectNoticeTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.648에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>본체 필드를 읽는다. 0.648에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.648에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.648에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>통지 장면을 만든다. 0.648에서 추가했다.</summary>
    private static (RawSidPool Pool, RawAiBrainMemory Memory, uint Token, byte[] Players, OriginalTypeRecord[] Types) BuildScene()
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

        return (pool, memory, token, new byte[9 * 180], types);
    }

    /// <summary>슬롯을 할당하고 0으로 채운다. 0.648에서 추가했다.</summary>
    private static int AllocateZero(RawSidPool pool)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid).Fill(0);
        return sid;
    }

    /// <summary>목록 블록을 할당하고 본체에 연결한다. 0.648에서 추가했다.</summary>
    private static void BindList(RawAiBrainMemory memory, uint token, uint offset, uint capacity, uint count = 0)
    {
        uint block = memory.Allocate(unchecked((int)capacity) * 4);
        Set(memory, token, offset, block);
        Set(memory, token, offset + 4, capacity);
        Set(memory, token, offset + 8, count);
    }

    /// <summary>통지 본체를 만든다. 0.648에서 추가했다.</summary>
    private static RawAiObjectNotice BuildNotice(RawAiBrainMemory memory, RawSidPool pool, OriginalTypeRecord[] types, byte[] players,
        List<(uint Token, AiObjectLog Log)>? logs = null, Func<int, uint[]>? targets = null) => new(
        memory, pool, types, players, () => 10.0, () => 999u, () => false,
        new AiObjectNoticeHooks
        {
            CarrierTargets = targets ?? (_ => new uint[2]),
            DistanceSquared = (_, _) => 1e9,
            Log = (brain, entry) => logs?.Add((brain, entry)),
        });

    /// <summary>비활성·0번 소유자·brain 없음은 본문을 부르지 않는다. 0.648에서 추가했다.</summary>
    [Fact]
    public void NoticeSkipsInactiveCases()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 20;
        var logs = new List<(uint Token, AiObjectLog Log)>();
        var disabled = new RawAiObjectNotice(memory, pool, types, players, () => 10.0, () => 999u, () => true,
            new AiObjectNoticeHooks { CarrierTargets = _ => new uint[2], DistanceSquared = (_, _) => 0.0, Log = (b, e) => logs.Add((b, e)) });
        disabled.Notice(2, sid, true);
        Assert.Empty(logs);
        var body = BuildNotice(memory, pool, types, players, logs);
        body.Notice(0, sid, true);
        Assert.Empty(logs);
        body.Notice(2, sid, true);
        Assert.Empty(logs);
    }

    /// <summary>brain이 있으면 타입 수가 증감한다. 0.648에서 추가했다.</summary>
    [Fact]
    public void NoticeUpdatesTypeCounter()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 20;
        types[20].Flags2 = 0u;
        Put(players, 2 * 180 + 0xb0, token);
        var logs = new List<(uint Token, AiObjectLog Log)>();
        var body = BuildNotice(memory, pool, types, players, logs);
        body.Notice(2, sid, true);
        Assert.Equal(1u, Get(memory, token, 0x6d4 + 20 * 4));
        body.Notice(2, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x6d4 + 20 * 4));
        Assert.Empty(logs);
    }

    /// <summary>사제 삭제는 같은 brain의 사제 기록을 지운다. 0.648에서 추가했다.</summary>
    [Fact]
    public void PriestRemovalClearsRecords()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 21;
        types[21].Flags2 = 0x200000u;
        var body = BuildNotice(memory, pool, types, players);
        Set(memory, token, 0x29c, unchecked((uint)sid));
        Set(memory, token, 0x2a0, unchecked((uint)sid));
        body.Update(token, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x29c));
        Assert.Equal(0u, Get(memory, token, 0x2a0));
    }

    /// <summary>운반자 삭제는 간헐천 제외·수집 목록·타이머를 갱신한다. 0.648에서 추가했다.</summary>
    [Fact]
    public void CarrierRemovalExcludesGeyser()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 22;
        types[22].Flags2 = 0x30000u;
        int geyser = AllocateZero(pool);
        BindList(memory, token, 0x330, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x330)), 0, unchecked((uint)geyser));
        BindList(memory, token, 0x33c, 8);
        BindList(memory, token, 0x2dc, 8);
        BindList(memory, token, 0x384, 8);
        BindList(memory, token, 0x378, 8);
        var logs = new List<(uint Token, AiObjectLog Log)>();
        var body = BuildNotice(memory, pool, types, players, logs,
            targets: _ => [unchecked((uint)geyser), 0u]);
        Set(memory, token, 0x248, 8u);
        body.Update(token, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x330 + 8));
        Assert.Equal(1u, Get(memory, token, 0x33c + 8));
        // 운반자 삭제도 건설 로그를 남기므로 두 건이다.
        Assert.Equal(2, logs.Count);
        Assert.Equal(AiObjectLogKind.MadeGeyserIneligible, logs[0].Log.Kind);
        Assert.Equal(unchecked((uint)geyser), logs[0].Log.Object);
        Assert.Equal(AiObjectLogKind.Built, logs[1].Log.Kind);
    }

    /// <summary>받침 생성·삭제는 다이스 기록을 쓰고 지운다. 0.648에서 추가했다.</summary>
    [Fact]
    public void DaisTracksSid()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 23;
        types[23].Flags2 = 0x400000u;
        var body = BuildNotice(memory, pool, types, players);
        body.Update(token, sid, true);
        Assert.Equal(unchecked((uint)sid), Get(memory, token, 0x2a4));
        body.Update(token, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x2a4));
    }

    /// <summary>근접 수호 생성은 목록에 넣고 삭제는 뺀다. 0.648에서 추가했다.</summary>
    [Fact]
    public void GuardTracksNearby()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 24;
        types[24].Flags2 = 0x4000000u;
        BindList(memory, token, 0x360, 8);
        BindList(memory, token, 0x300, 8);
        // 근접 판정은 신전 본체(0x294)나 0x300 목록을 보므로 신전을 둔다.
        int temple = AllocateZero(pool);
        Set(memory, token, 0x294, unchecked((uint)temple));
        var logs = new List<(uint Token, AiObjectLog Log)>();
        var body = new RawAiObjectNotice(memory, pool, types, players, () => 10.0, () => 999u, () => false,
            new AiObjectNoticeHooks
            {
                CarrierTargets = _ => new uint[2],
                DistanceSquared = (_, _) => 100.0,
                Log = (b, e) => logs.Add((b, e)),
            });
        body.Update(token, sid, true);
        Assert.Equal(1u, Get(memory, token, 0x360 + 8));
        body.Update(token, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x360 + 8));
    }

    /// <summary>신전 생성·삭제는 알림 목록과 0 시각을 갱신한다. 0.648에서 추가했다.</summary>
    [Fact]
    public void TempleTracksAndZeroesTime()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 25;
        types[25].Flags2 = 0x800u;
        BindList(memory, token, 0x2f4, 8);
        // 신전 종류도 원본처럼 포탑 분기(0x40800)를 타므로 0x2e8 목록을 묶는다.
        BindList(memory, token, 0x2e8, 8);
        var body = BuildNotice(memory, pool, types, players);
        body.Update(token, sid, true);
        Assert.Equal(1u, Get(memory, token, 0x2f4 + 8));
        Assert.Equal(0u, Get(memory, token, 0x5b8));
        Assert.Equal(0u, Get(memory, token, 0x5b8 + 4));
        body.Update(token, sid, false);
        Assert.Equal(0u, Get(memory, token, 0x2f4 + 8));
    }

    /// <summary>시설 삭제는 대기 목록을 재구성하고 관련 필드를 초기화한다. 0.648에서 추가했다.</summary>
    [Fact]
    public void FactoryRemovalRebuildsQueue()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 26;
        types[26].Flags2 = 0x4000u;
        BindList(memory, token, 0x300, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x300)), 0, unchecked((uint)sid));
        BindList(memory, token, 0x30c, 8);
        BindList(memory, token, 0x318, 8);
        var body = BuildNotice(memory, pool, types, players);
        body.Update(token, sid, true);
        Assert.Equal(1u, Get(memory, token, 0x30c + 8));
        body.Update(token, sid, false);
        Assert.Equal(6u, Get(memory, token, 0x3e4));
        Assert.Equal(1u, Get(memory, token, 0x3f0));
    }

    /// <summary>탑은 용량이 차면 맨 앞을 밀어내고 개수를 셋 올린다. 0.648에서 추가했다.</summary>
    [Fact]
    public void TowerRotatesWhenFull()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int first = AllocateZero(pool);
        int second = AllocateZero(pool);
        pool.AllocatedBytes(first)[10] = 27;
        pool.AllocatedBytes(second)[10] = 27;
        types[27].Flags2 = 0x40800u;
        BindList(memory, token, 0x2e8, 1, 1);
        Put(memory.Bytes(Get(memory, token, 0x2e8)), 0, unchecked((uint)first));
        // 0x2e8 목록의 용량 칸이 곧 부호 제한이므로 1로 둔다.
        Set(memory, token, 0x2ec, 1u);
        var body = BuildNotice(memory, pool, types, players);
        body.Update(token, second, true);
        Assert.Equal(1u, Get(memory, token, 0x2e8 + 8));
        Assert.Equal(unchecked((uint)second), BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(Get(memory, token, 0x2e8)).AsSpan(0, 4)));
        Assert.Equal(3u, Get(memory, token, 0x3c8));
    }

    /// <summary>의식 생성은 0 시각을 쓴다. 0.648에서 추가했다.</summary>
    [Fact]
    public void RitualMarksZeroTime()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 28;
        types[28].Flags2 = 0x4200u;
        var body = BuildNotice(memory, pool, types, players);
        body.Update(token, sid, true);
        Assert.Equal(0u, Get(memory, token, 0x5c0));
        Assert.Equal(0u, Get(memory, token, 0x5c0 + 4));
    }

    /// <summary>건설 생성은 로그에 좌표·타입을 남긴다. 0.648에서 추가했다.</summary>
    [Fact]
    public void BuiltLogsCoordinates()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        int sid = AllocateZero(pool);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[10] = 29;
        types[29].Flags2 = 0x30000u;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(12.7f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(34.2f));
        BindList(memory, token, 0x2dc, 8);
        var logs = new List<(uint Token, AiObjectLog Log)>();
        var body = BuildNotice(memory, pool, types, players, logs);
        Set(memory, token, 0x228, BitConverter.SingleToUInt32Bits(2.0f));
        body.Update(token, sid, true);
        Assert.Single(logs);
        Assert.Equal(AiObjectLogKind.Built, logs[0].Log.Kind);
        Assert.Equal(12, logs[0].Log.X);
        Assert.Equal(34, logs[0].Log.Y);
        Assert.Equal(12.0, BitConverter.UInt64BitsToDouble(Get(memory, token, 0x5a8) | ((ulong)Get(memory, token, 0x5a8 + 4) << 32)), 9);
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.648에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, OriginalTypeRecord[] types) = BuildScene();
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => new RawAiObjectNotice(memory, cdPool, types, players, () => 10.0, () => 999u, () => false,
            new AiObjectNoticeHooks { CarrierTargets = _ => new uint[2], DistanceSquared = (_, _) => 0.0, Log = (_, _) => { } }));
        Assert.Throws<ArgumentException>(() => new RawAiObjectNotice(memory, pool, Array.Empty<OriginalTypeRecord>(), players, () => 10.0, () => 999u, () => false,
            new AiObjectNoticeHooks { CarrierTargets = _ => new uint[2], DistanceSquared = (_, _) => 0.0, Log = (_, _) => { } }));
        Assert.Throws<ArgumentNullException>(() => new RawAiObjectNotice(memory, pool, types, players, () => 10.0, () => 999u, () => false, null!));
        var body = BuildNotice(memory, pool, types, players);
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 30;
        types[30].Flags2 = 0x30000u;
        // 운반자 삭제는 이동 목표 조회를 요구한다.
        var bare = new RawAiObjectNotice(memory, pool, types, players, () => 10.0, () => 999u, () => false, new AiObjectNoticeHooks());
        Assert.Throws<InvalidOperationException>(() => bare.Update(token, sid, false));
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => body.Update(small, sid, true));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Notice(9, sid, true));
    }
}
