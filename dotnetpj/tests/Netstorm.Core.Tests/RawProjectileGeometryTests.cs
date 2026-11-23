using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 포구/치수/충돌 상자와 사정선을 독립 PE의 전체 float 비트로 비교한다.
namespace Netstorm.Core.Tests;

/// <summary>두 판본240기하·PATCH4040사정선과 현재 대여 계약 검사다.</summary>
public sealed class RawProjectileGeometryTests
{
    /// <summary>원문 DWORD 입력의 비트를 읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>독립 입력의 float 비트를 읽는다.</summary>
    private static float F(string value) => BitConverter.UInt32BitsToSingle(N(value));
    /// <summary>현재 raw DWORD만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>실제 원본 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>두 판본의 포구/치수/상자120건씩을 허용오차 없이 비교한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllNativeMuzzlesDimensionsAndBounds(bool patch)
    {
        var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); int sid = pool.Allocate(SidAllocation.Client); OriginalTypeRecord[] types = Enumerable.Range(0, patch ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); ProjectileGeometryType[] geometry = types.Select(_ => new ProjectileGeometryType()).ToArray(); geometry[71].DefaultFrame = 1; geometry[71].FrameCount = 4; geometry[71].ShotOffsets = [(-35, 91), (20, -7), (32767, -32768), (-127, 43)]; int count = 0;
        // 기대 열은 실제 호출 뒤에만 읽는다.
        foreach (string[] row in Rows("projectile-geom-x86.tsv"))
        {
            if (row[0] != (patch ? "originals" : "originalCD")) continue; Span<byte> raw = pool.AllocatedBytes(sid); raw[10] = 71; Put(raw, 14, N(row[6])); Put(raw, 18, N(row[7])); if (patch) Put(raw, 36, N(row[2])); else raw[34] = (byte)N(row[2]); raw[patch ? 40 : 35] = (byte)N(row[3]); types[71].FootX = int.Parse(row[8], CultureInfo.InvariantCulture); types[71].FootY = int.Parse(row[9], CultureInfo.InvariantCulture); geometry[71].HeightExtension = int.Parse(row[10], CultureInfo.InvariantCulture); var logic = new RawProjectileGeometry(pool, types, geometry, float.Parse(row[4], CultureInfo.InvariantCulture), float.Parse(row[5], CultureInfo.InvariantCulture)); uint[] actual = row[1] == "M" ? logic.ArcherMuzzle((ushort)sid).Select(BitConverter.SingleToUInt32Bits).ToArray() : row[1] == "D" ? logic.Dimensions((ushort)sid).Select(v => unchecked((uint)v)).ToArray() : logic.TargetBounds((ushort)sid, [F(row[6]), F(row[7])]).Select(BitConverter.SingleToUInt32Bits).ToArray(); Assert.Equal(row[11].Split(',').Select(value => uint.Parse(value, NumberStyles.HexNumber, CultureInfo.InvariantCulture)), actual); count++;
        }
        Assert.Equal(120, count);
    }
    /// <summary>원본 썬 캐논의 모든 방향·사거리 절삭·범위 모서리를 대조한다.</summary>
    [Fact]
    public void MatchesAllNativeSunCannonRayEnds()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); ProjectileGeometryType[] geometry = types.Select(_ => new ProjectileGeometryType()).ToArray(); var logic = new RawProjectileGeometry(pool, types, geometry); pool.AllocatedBytes(sid)[10] = 128; int count = 0;
        // 4040고유 입력의 두 반환 DWORD를 원문 그대로 비교한다.
        foreach (string[] row in Rows("cannon-rayend-x86.tsv")) { Assert.Equal(8, row.Length); Put(pool.AllocatedBytes(sid), 36, N(row[0])); geometry[128].Range = F(row[5]); float[] actual = logic.CannonRayEnd([F(row[1]), F(row[2])], [F(row[3]), F(row[4])], (ushort)sid); Assert.Equal(N(row[6]), BitConverter.SingleToUInt32Bits(actual[0])); Assert.Equal(N(row[7]), BitConverter.SingleToUInt32Bits(actual[1])); count++; }
        Assert.Equal(4040, count);
    }
    /// <summary>현재 TYPE/SHP를 대여하며 반환 좌표의 별칭과 다른 Pool 혼용을 막는다.</summary>
    [Fact]
    public void BorrowsCurrentTablesAndRejectsDifferentDirectionPool()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); ProjectileGeometryType[] geometry = types.Select(_ => new ProjectileGeometryType()).ToArray(); RiftTypeFrames[] frames = types.Select(_ => new RiftTypeFrames([])).ToArray(); var logic = new RawProjectileGeometry(pool, types, geometry); Span<byte> raw = pool.AllocatedBytes(sid); raw[10] = 71; Put(raw, 14, BitConverter.SingleToUInt32Bits(50)); Put(raw, 18, BitConverter.SingleToUInt32Bits(60)); Put(raw, 36, 0); geometry[71].ShotOffsets = [(16, 11)]; Assert.Equal(new float[] { 52, 62 }, logic.ArcherMuzzle((ushort)sid)); geometry[71] = new() { ShotOffsets = [(32, 22)] }; Assert.Equal(new float[] { 54, 64 }, logic.ArcherMuzzle((ushort)sid)); types[71].FootX = 4; types[71].FootY = 5; geometry[71].HeightExtension = 2; Assert.Equal(new[] { 4, 5 }, logic.Dimensions((ushort)sid)); Assert.Equal(new float[] { 46, 53, 50, 60 }, logic.TargetBounds((ushort)sid, [50, 60])); float[] first = logic.Muzzle((ushort)sid); first[0] = 99; Assert.Equal(54, logic.Muzzle((ushort)sid)[0]); var other = new RawSidPool(24000); var mode = new RawGunMode(other, frames, new() { Value = 140 }); Assert.Throws<ArgumentException>(() => new RawProjectileGeometry(pool, types, geometry, gunMode: mode)); raw[10] = 131; Assert.Throws<InvalidOperationException>(() => logic.CannonRayEnd([10, 20], [11, 30], (ushort)sid));
    }
}
