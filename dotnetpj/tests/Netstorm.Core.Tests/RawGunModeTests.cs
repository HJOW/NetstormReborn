using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 두 PE의 현재 공격 방향과 signed 문자/그래프 우선순위를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>campaign-frame의 M480행과 mutable 역할/현재 프레임 대여 검사다.</summary>
public sealed class RawGunModeTests
{
    /// <summary>독립 십진 입력을 읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>두 판본의 모드240행씩만 실제 본문과 대조한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllNativeModeRows(bool patch)
    {
        var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        // 원문 관측 SID50을 그대로 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        RiftTypeFrames[] frames = Enumerable.Range(0, patch ? 188 : 171).Select(_ => new RiftTypeFrames([])).ToArray(); var mode = new RawGunMode(pool, frames, new() { Value = 140 }); int count = 0;
        // 같은 fixture의 H/S는 별도 미이식 본문이므로 M의480개만 센다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/campaign-frame-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); if (row[0] != (patch ? "originals" : "originalCD") || row[1] != "M") continue; uint[] input = row[2].Split(',').Select(N).ToArray(); byte[] image = Convert.FromHexString(row[3]); image.CopyTo(pool.RawSlot(50)); frames[input[0]] = new(Enumerable.Repeat(new RiftFrameCode((byte)input[3], (byte)'A', 0, 0), 48)); uint result = mode.Get(50); Assert.Equal(N(row[4]), result); Assert.Equal(row[6], Convert.ToHexString(pool.Slot(50)[..image.Length]).ToLowerInvariant()); Assert.Equal("-", row[7]); count++;
        }
        Assert.Equal(240, count);
    }
    /// <summary>같은 역할이 바뀌면 graph와 현재 프레임/부호 있는 코드 선택이 즉시 바뀐다.</summary>
    [Fact]
    public void CurrentRoleGraphAndFullFrameAreReadEveryTime()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); var role = new UIntBox { Value = 140 }; var mode = new RawGunMode(pool, frames, role); Span<byte> raw = pool.AllocatedBytes(sid); raw[10] = 140; raw[30] = 3; BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], uint.MaxValue); Assert.Equal(6u, mode.Get((ushort)sid)); role.Value = 141; Assert.Throws<ArgumentOutOfRangeException>(() => mode.Get((ushort)sid)); frames[140] = new([new RiftFrameCode((byte)'O', 0, 0, 0)]); BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], 0); Assert.Equal(6u, mode.Get((ushort)sid)); frames[140] = new([new RiftFrameCode(255, 0, 0, 0)]); Assert.Equal(0u, mode.Get((ushort)sid)); role.Value = 140; raw[30] = 255; Assert.Equal(510u, mode.Get((ushort)sid)); raw[40] = 8; Assert.Equal(0u, mode.Get((ushort)sid));
    }
}
