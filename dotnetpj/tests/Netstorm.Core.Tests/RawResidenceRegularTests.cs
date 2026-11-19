// 거주 구역 정기(주민·장식·지면 테마)의 분기·난수·프레임을 실제 슬롯으로 대조한다.
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// 거주 구역 정기를 검사하는 클래스다. 2026-11-19 0.635에서 R4-4 정기 처리용으로 추가했다.
// 근거: cpppj o/RawResidenceRegular.cpp의 RunResidenceRegular(원문 0049a4e0)다.
public sealed class RawResidenceRegularTests
{
    /// <summary>실제 풀의 슬롯 수다. 2026-11-19 0.635에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.635에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다.
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>거주 구역 슬롯 장면을 만든다. 2026-11-19 0.635에서 추가했다.</summary>
    private static (RawSidPool Pool, RawSquidHash Hash, OriginalTypeRecord[] Types, List<RiftTypeFrames> Frames, int Sid) BuildScene()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        var frames = new List<RiftTypeFrames>(188);
        // 모든 타입 번호마다 빈 코드 표를 넣는 반복문이다.
        for (int i = 0; i < 188; ++i)
        {
            frames.Add(new RiftTypeFrames([]));
        }

        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        Put(raw, 0, 0x0050f540u);
        raw[RawSidPool.TypeOffset] = 89;
        return (pool, new RawSquidHash(), types, frames, sid);
    }

    /// <summary>사건 0은 주민 nibble을 뒤집고 한 번 뽑아 15~35초 뒤로 재예약한다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void EventZeroTogglesResidentAndRearms()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        pool.AllocatedBytes(sid)[12] = 0x23;
        Put(pool.AllocatedBytes(sid), 36, 10u);
        var temples = new RawPostPopList { Entries = new uint[20] };
        int notifies = 0, framed = 0;
        var random = new GameRandom(0u);
        float result = RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 0, 0, 0.1f, new ResidenceRegularHooks
        {
            Random = () => random,
            Notify = _ => notifies++,
            Frame = (_, _, _) => framed++,
            Surface = (_, _) => 0,
            SetOwner = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Dude = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Element = _ => 0,
            ThemeMode = () => 0u,
            DecorationType = () => 102u,
            RecordScan = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(0x20, pool.Slot(sid)[12]);
        Assert.Equal(1, notifies);
        Assert.Equal(1, framed);
        Assert.Equal(BitConverter.UInt32BitsToSingle(0x41953080u), result);
        Assert.Equal(0x2eb4230au, random.State());
    }

    /// <summary>사건 1은 두 난수를 뽑고 빈 지도에서는 조용히 1초 뒤로 재예약한다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void EventOneScansEmptyMapQuietly()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(pool.AllocatedBytes(sid), 18, BitConverter.SingleToUInt32Bits(52.5f));
        types[89].FootX = 1;
        types[89].FootY = 1;
        var temples = new RawPostPopList { Entries = new uint[20] };
        int scanned = -1;
        var random = new GameRandom(0u);
        float result = RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 1, 0, 0.2f, new ResidenceRegularHooks
        {
            Random = () => random,
            Notify = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Frame = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Surface = (_, _) => 0,
            SetOwner = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Dude = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Element = _ => 0,
            ThemeMode = () => 0u,
            DecorationType = () => 102u,
            RecordScan = value => scanned = value,
        });
        Assert.Equal(1.0f, result);
        Assert.Equal(0, scanned);
        Assert.Equal(0xaf266921u, random.State());
    }

    /// <summary>사건 3은 소유자 일치 신전의 원소로 테마를 정하고 0.1초 뒤로 재예약한다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void EventThreeAppliesThemeFromMatchingTemple()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        pool.AllocatedBytes(sid)[12] = 0x07;
        pool.AllocatedBytes(sid)[34] = 2;
        Put(pool.AllocatedBytes(sid), 36, 10u);
        Put(pool.AllocatedBytes(sid), 42, 20u);
        int temple = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(temple)[10] = 122;
        pool.AllocatedBytes(temple)[34] = 2;
        var temples = new RawPostPopList { Entries = new uint[20] };
        temples.Entries[0] = unchecked((uint)temple);
        temples.Count = 1;
        types[122].Theme = 2;
        int framesSet = -1;
        float result = RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 3, 4, 0.5f, new ResidenceRegularHooks
        {
            Random = () => throw new InvalidOperationException("호출되면 안 된다"),
            Notify = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Frame = (target, value, _) => { framesSet = value; Put(pool.AllocatedBytes(target), 36, unchecked((uint)value)); },
            Surface = (_, _) => 0,
            SetOwner = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Dude = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Element = _ => 2,
            ThemeMode = () => 1u,
            DecorationType = () => 102u,
            RecordScan = _ => throw new InvalidOperationException("호출되면 안 된다"),
        });
        Assert.Equal(0.1f, result);
        Assert.Equal(33, framesSet);
        Assert.Equal(33, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4)));
    }

    /// <summary>사건 2는 R6 주민 생성에서 잇는다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void EventTwoNeedsDudeRuntime()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        var temples = new RawPostPopList { Entries = new uint[20] };
        Assert.Throws<InvalidOperationException>(() => RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 2, 0, 0.1f, new ResidenceRegularHooks
        {
            Random = () => throw new InvalidOperationException("호출되면 안 된다"),
            Notify = _ => throw new InvalidOperationException("호출되면 안 된다"),
            Frame = (_, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Surface = (_, _) => 0,
            SetOwner = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
            Dude = _ => throw new InvalidOperationException("거주 구역 주민 생성 미연결(R6)"),
            Element = _ => 0,
            ThemeMode = () => 0u,
            DecorationType = () => 102u,
            RecordScan = _ => throw new InvalidOperationException("호출되면 안 된다"),
        }));
    }

    /// <summary>거주 구역 가상표가 아니면 거부한다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void RejectsNonResidenceVtable()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        Put(pool.AllocatedBytes(sid), 0, 0x00509148u);
        var temples = new RawPostPopList { Entries = new uint[20] };
        Assert.Throws<ArgumentException>(() => RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 0, 0, 0.1f, new ResidenceRegularHooks
        {
            Random = () => new GameRandom(1u),
            Notify = _ => { },
            Frame = (_, _, _) => { },
            Surface = (_, _) => 0,
            SetOwner = (_, _) => { },
            Dude = _ => { },
            Element = _ => 0,
            ThemeMode = () => 0u,
            DecorationType = () => 102u,
            RecordScan = _ => { },
        }));
        Assert.False(RawResidenceRegular.Handles(pool, sid));
    }

    /// <summary>없는 풀·해시·장치는 실제 호출 전에 거부한다. 2026-11-19 0.635에서 추가했다.</summary>
    [Fact]
    public void RejectsMissingDevices()
    {
        (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, List<RiftTypeFrames> frames, int sid) = BuildScene();
        var temples = new RawPostPopList { Entries = new uint[20] };
        var hooks = new ResidenceRegularHooks
        {
            Random = () => new GameRandom(1u),
            Notify = _ => { },
            Frame = (_, _, _) => { },
            Surface = (_, _) => 0,
            SetOwner = (_, _) => { },
            Dude = _ => { },
            Element = _ => 0,
            ThemeMode = () => 0u,
            DecorationType = () => 102u,
            RecordScan = _ => { },
        };
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(null!, hash, types, frames, temples, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(pool, null!, types, frames, temples, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(pool, hash, null!, frames, temples, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(pool, hash, types, null!, temples, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(pool, hash, types, frames, null!, sid, 0, 0, 0.1f, hooks));
        Assert.Throws<ArgumentNullException>(() => RawResidenceRegular.Run(pool, hash, types, frames, temples, sid, 0, 0, 0.1f, null!));
    }
}
