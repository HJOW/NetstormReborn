using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 프레임 지정(<see cref="SquidFrame"/>)의 보호 경로와 분기 계약을 확인한다.
/// 원본 기대값 재생(X86RawFrameTests)은 레벨·효과·슬롯을 대조하고,
/// 이 검사들은 합성 입력으로 분기(레벨 일치·불일치·섬·경계값)를 고정한다 (W02 Frame 조각, 2026-10-20).
/// </summary>
public sealed class RawFrameTests
{
    /// <summary>관찰 대상 타입 번호.</summary>
    private const int ObjectType = 82;

    /// <summary>검사 대상 번호.</summary>
    private const int Target = 50;

    /// <summary>번호 50 을 클라이언트 영역에 확보한 풀을 만든다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호를 클라이언트 영역에 확보한다
        for (int id = 5; id <= Target; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        return pool;
    }

    /// <summary>타입 레코드 188개를 만들고 관찰 타입의 부류를 정한다.</summary>
    /// <param name="genus">82번 타입의 플래그2.</param>
    private static OriginalTypeRecord[] Types(uint genus = 0)
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        types[ObjectType].Flags2 = genus;
        return types;
    }

    /// <summary>기본 슬롯(관찰 타입·현재 프레임·레벨 바이트)을 채운다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="frame">현재 프레임 DWORD.</param>
    /// <param name="level">표시 레벨 바이트.</param>
    private static void Fill(RawSidPool pool, uint frame, byte level)
    {
        Span<byte> raw = pool.AllocatedBytes(Target);
        raw.Clear();
        Put(raw, 0, 0x11010000);
        raw[RawSidPool.TypeOffset] = ObjectType;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
        Put(raw, 0x24, frame);
        raw[0x21] = level;
    }

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 4바이트를 쓴다
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>효과 순서와 호출 수를 기록하는 hooks 를 만든다.</summary>
    /// <param name="events">효과 순서 기록.</param>
    /// <param name="sizes">프레임 크기 조회 결과.</param>
    /// <param name="sizeCalls">크기 조회 횟수(길이 1).</param>
    private static SquidFrameHooks Hooks(List<string> events, float[] sizes, int[] sizeCalls) => new()
    {
        FrameSize = (sid, frame) => { sizeCalls[0]++; return sizes; },
        Update = (sid, flags) => events.Add($"U:{sid}:{flags}"),
        Unpop = (sid, flags) => events.Add($"N:{sid}:{flags}"),
        Pop = (sid, x, y, flags) => events.Add($"P:{sid}:{flags}"),
    };

    /// <summary>
    /// 생성자는 같은 풀·10.78 타입 표·연결된 효과를 요구한다.
    /// </summary>
    [Fact]
    public void ConstructorRejectsMismatchedPoolTypesAndHooks()
    {
        var pool = Pool();
        var types = Types();
        var hooks = Hooks([], [1f, 1f], new int[1]);
        _ = new SquidFrame(pool, types, hooks);
        // 짧은 타입 표(CD 171개)는 받지 않는다
        Assert.ThrowsAny<Exception>(() => new SquidFrame(pool, types.Take(100).ToList(), hooks));
        // 빠진 효과로는 만들지 않는다
        var broken = Hooks([], [1f, 1f], new int[1]);
        broken.Pop = null;
        Assert.ThrowsAny<Exception>(() => new SquidFrame(pool, types, broken));
    }

    /// <summary>
    /// 섬·다리 부류는 크기를 세지 않고 레벨 0이다.
    /// 그 밖은 큰 쪽 크기 2·4 경계로 1·2·3을 정한다.
    /// </summary>
    [Fact]
    public void StoreLevelFollowsGenusAndSizeThresholds()
    {
        // 섬·다리 부류는 크기 조회를 부르지 않고 0이다
        foreach (uint genus in new uint[] { 0x2u, 0x4u, 0x6u })
        {
            var pool = Pool();
            var types = Types(genus);
            int calls = 0;
            var frames = new SquidFrame(pool, types, new SquidFrameHooks
            {
                FrameSize = (sid, frame) => { calls++; return [99f, 99f]; },
                Update = (sid, flags) => { },
                Unpop = (sid, flags) => { },
                Pop = (sid, x, y, flags) => { },
            });
            Fill(pool, 0, 7);
            Assert.Equal(0, frames.StoreLevel(Target));
            Assert.Equal(0, calls);
            Assert.Equal(0, pool.Slot(Target)[0x21]);
        }
        // 경계값: 큰 쪽이 2 이하면 1, 4 이하면 2, 그 밖이면 3이다
        foreach ((float[] size, int expected) in new (float[], int)[] { ([1f, 1f], 1), ([2f, 2f], 1), ([2f, 4f], 2), ([4f, 4f], 2), ([4.25f, 1f], 3) })
        {
            var pool = Pool();
            var types = Types();
            var frames = new SquidFrame(pool, types, Hooks([], size, new int[1]));
            Fill(pool, 0, 7);
            Assert.Equal(expected, frames.StoreLevel(Target));
            Assert.Equal((byte)expected, pool.Slot(Target)[0x21]);
        }
        // 유한하지 않거나 음수 크기는 거부한다
        foreach (float[] size in new float[][] { [float.NaN, 1f], [1f, float.PositiveInfinity], [-1f, 1f] })
        {
            var pool = Pool();
            var frames = new SquidFrame(pool, Types(), Hooks([], size, new int[1]));
            Fill(pool, 0, 7);
            Assert.ThrowsAny<Exception>(() => frames.StoreLevel(Target));
        }
        // 타입 번호가 표를 벗어나면 거부한다
        {
            var pool = Pool();
            var frames = new SquidFrame(pool, Types(), Hooks([], [1f, 1f], new int[1]));
            Fill(pool, 0, 7);
            pool.AllocatedBytes(Target)[RawSidPool.TypeOffset] = 255;
            Assert.Throws<ArgumentOutOfRangeException>(() => frames.StoreLevel(Target));
        }
    }

    /// <summary>
    /// 레벨이 맞으면 갱신만 두 번 하고 표시 등록은 그대로 둔다.
    /// 레벨이 맞지 않아도 프레임이 현재와 같으면 레벨 바이트만 고친다.
    /// </summary>
    [Fact]
    public void SetUpdatesTwiceOnSameLevelAndKeepsSpotOtherwise()
    {
        // 레벨 일치: 갱신 두 번, 프레임 쓰기, Unpop·Pop 없음
        {
            var pool = Pool();
            var events = new List<string>();
            var frames = new SquidFrame(pool, Types(), Hooks(events, [1f, 1f], new int[1]));
            Fill(pool, 3, 1);
            frames.Set(Target, 5, 0);
            Assert.Equal(new[] { "U:50:0", "U:50:0" }, events);
            ReadOnlySpan<byte> slot = pool.Slot(Target);
            uint written = (uint)slot[0x24] | ((uint)slot[0x25] << 8) | ((uint)slot[0x26] << 16) | ((uint)slot[0x27] << 24);
            Assert.Equal(5u, written);
            Assert.Equal(1, slot[0x21]);
        }
        // 레벨 불일치·같은 프레임: 레벨 바이트만 고치고 효과 없음
        {
            var pool = Pool();
            var events = new List<string>();
            var frames = new SquidFrame(pool, Types(), Hooks(events, [1f, 1f], new int[1]));
            Fill(pool, 2, 9);
            frames.Set(Target, 2, 0x2000);
            Assert.Empty(events);
            Assert.Equal(1, pool.Slot(Target)[0x21]);
            ReadOnlySpan<byte> slot = pool.Slot(Target);
            uint written = (uint)slot[0x24] | ((uint)slot[0x25] << 8) | ((uint)slot[0x26] << 16) | ((uint)slot[0x27] << 24);
            Assert.Equal(2u, written);
        }
        // 레벨 불일치·다른 프레임: Unpop 뒤 현재 위치로 다시 등록한다
        {
            var pool = Pool();
            var events = new List<string>();
            var frames = new SquidFrame(pool, Types(), Hooks(events, [5f, 5f], new int[1]));
            Fill(pool, 2, 9);
            frames.Set(Target, 7, 0);
            Assert.Equal(new[] { "N:50:0", "P:50:80" }, events);
            Assert.Equal(3, pool.Slot(Target)[0x21]);
            ReadOnlySpan<byte> slot = pool.Slot(Target);
            uint written = (uint)slot[0x24] | ((uint)slot[0x25] << 8) | ((uint)slot[0x26] << 16) | ((uint)slot[0x27] << 24);
            Assert.Equal(7u, written);
        }
        // 큰 프레임 번호도 4바이트 전부 쓴다
        {
            var pool = Pool();
            var events = new List<string>();
            var frames = new SquidFrame(pool, Types(), Hooks(events, [1f, 1f], new int[1]));
            Fill(pool, 3, 9);
            frames.Set(Target, unchecked((int)0x12345678), 0);
            Assert.Equal(new[] { "N:50:0", "P:50:80" }, events);
            ReadOnlySpan<byte> slot = pool.Slot(Target);
            uint written = (uint)slot[0x24] | ((uint)slot[0x25] << 8) | ((uint)slot[0x26] << 16) | ((uint)slot[0x27] << 24);
            Assert.Equal(0x12345678u, written);
        }
    }

    /// <summary>
    /// 표시 최소 단계는 섬·다리 부류 0, 큰 쪽 크기 2·4 경계로 1·2·3이다.
    /// 유한하지 않거나 음수 크기는 거부한다(원본 SquidHash::ObjectLevel).
    /// </summary>
    [Fact]
    public void ObjectLevelFollowsGenusAndSizeThresholds()
    {
        // 섬·다리 부류는 크기와 무관하게 0이다
        foreach (uint genus in new uint[] { 0x2u, 0x4u, 0x6u, 0x20002u })
        {
            Assert.Equal(0, RawSquidHash.ObjectLevel(genus, 99f, 99f));
        }
        // 경계값: 큰 쪽이 2 이하면 1, 4 이하면 2, 그 밖이면 3이다
        Assert.Equal(1, RawSquidHash.ObjectLevel(0, 1f, 1f));
        Assert.Equal(1, RawSquidHash.ObjectLevel(0, 2f, 2f));
        Assert.Equal(2, RawSquidHash.ObjectLevel(0, 2f, 4f));
        Assert.Equal(2, RawSquidHash.ObjectLevel(0, 4f, 4f));
        Assert.Equal(3, RawSquidHash.ObjectLevel(0, 4.25f, 1f));
        // 유한하지 않거나 음수 크기는 거부한다
        Assert.ThrowsAny<Exception>(() => RawSquidHash.ObjectLevel(0, float.NaN, 1f));
        Assert.ThrowsAny<Exception>(() => RawSquidHash.ObjectLevel(0, 1f, float.PositiveInfinity));
        Assert.ThrowsAny<Exception>(() => RawSquidHash.ObjectLevel(0, -1f, 1f));
    }
}
