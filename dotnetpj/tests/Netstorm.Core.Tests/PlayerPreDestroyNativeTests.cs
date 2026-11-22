using System.Buffers.Binary;
using System.Globalization;
using System.Security.Cryptography;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Player 고유 Pre의 현재 주소 소비·필드 폭·공통 부모·부분 실패를 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>고정 Player 사본을 재사용하면 드러나는 세 접근 사이의 owner/base 교체를 검사한다.</summary>
public sealed class PlayerPreDestroyNativeTests
{
    /// <summary>VT/TYPE 선행 분류 없이 실제 물리 owner만 제공하는 원문 입력이다.</summary>
    private static (RawSidPool Pool, int Sid) Scene(byte owner = 1, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        var pool = new RawSidPool(32768, edition: edition); int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid)[34] = owner; return (pool, sid);
    }
    /// <summary>주변 바이트까지 비교할 수 있는 독립 기록 입력이다.</summary>
    private static byte[] Records() => Enumerable.Repeat((byte)0xa5, 9 * 180).ToArray();
    /// <summary>검사 입력의 한 DWORD만 설정한다.</summary>
    private static void Write(byte[] records, int owner, int field, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(records.AsSpan(owner * 180 + field, 4), value);
    /// <summary>검사 결과의 같은 DWORD를 읽는다.</summary>
    private static uint Read(byte[] records, int owner, int field) => BinaryPrimitives.ReadUInt32LittleEndian(records.AsSpan(owner * 180 + field, 4));

    /// <summary>원본135바이트/39명령의 독립360관측과 전체세버퍼해시/전체50바이트슬롯을비교한다.</summary>
    [Fact]
    public void OriginalX86PreMatchesAllBuffersAndOwnerTransitions()
    {
        int count = 0;
        // 원본 격리 실행이 쓴 관측 행을 읽으며 기대 해시를 C# 결과로 계산하지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "player-pre-destroy-x86.tsv")))
        {
            if (line.StartsWith('#') || line.Length == 0) continue;
            string[] row = line.Split('\t'); Assert.Equal(9, row.Length);
            byte owner = byte.Parse(row[0], CultureInfo.InvariantCulture); uint flags = uint.Parse(row[1], CultureInfo.InvariantCulture);
            bool matches = row[2] == "1"; uint recordFlags = uint.Parse(row[3], CultureInfo.InvariantCulture); int variant = int.Parse(row[4], CultureInfo.InvariantCulture);
            int nextOwner = (owner + 1) & 255, thirdOwner = (owner + 2) & 255;
            var pool = new RawSidPool(32768); const int sid = 5; // 원본 포인터 차이/50이 사용한 명시 SID다.
            pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[34] = owner;
            byte[][] buffers = [Enumerable.Repeat((byte)0xa5, 65536).ToArray(), Enumerable.Repeat((byte)0x5a, 65536).ToArray(), Enumerable.Repeat((byte)0x3c, 65536).ToArray()];
            Write(buffers[0], owner, 0x9c, matches ? sid : 0x87654321); Write(buffers[0], owner, 0x50, recordFlags);
            Write(buffers[1], nextOwner, 0x50, recordFlags ^ 0x55555555); Write(buffers[2], thirdOwner, 0x50, recordFlags ^ 0xaaaaaaaa);
            int reads = 0, bases = 0; byte[] current = buffers[0];
            RawPlayerWorld.PreDestroyNative(pool, sid, flags, new()
            {
                Current = () =>
                {
                    reads++; byte[] selected = current;
                    // 두 번째 필드는 이미 owner를 읽었으므로 반환할 같은view는B로 고정하고 다음 접근만C로 바꾼다.
                    if (reads == 2 && variant == 2) { pool.RawSlot(sid)[34] = (byte)thirdOwner; current = buffers[2]; }
                    return new(selected, selected);
                },
                Base = (actual, value) =>
                {
                    bases++; Assert.Equal(sid, actual); Assert.Equal(flags, value);
                    if (variant != 0) { pool.RawSlot(sid)[34] = (byte)nextOwner; current = buffers[1]; }
                },
            });
            Assert.Equal(3, reads); Assert.Equal(1, bases); Assert.Equal(Convert.FromHexString(row[5]), pool.RawSlot(sid).ToArray());
            // 원본이 관측한 세 전체65536바이트 각각을 비교해 주변필드/다른owner도 포함한다.
            for (int i = 0; i < buffers.Length; i++) Assert.Equal(row[6 + i], Convert.ToHexString(SHA256.HashData(buffers[i])).ToLowerInvariant());
            count++;
        }
        Assert.Equal(360, count);
    }

    /// <summary>자신SID만 정리하고 전체 flags/다른 Player/주변 바이트를 보존한다.</summary>
    [Theory]
    [InlineData(true, 0u)]
    [InlineData(false, 0u)]
    [InlineData(true, 0xffffffffu)]
    [InlineData(false, 0x81234567u)]
    public void FullRecordsKeepOtherFieldsAndPassOriginalFlags(bool matches, uint flags)
    {
        var (pool, sid) = Scene(); byte[] records = Records();
        Write(records, 1, 0x9c, matches ? (uint)sid : 0x12345678); Write(records, 1, 0x50, 0x89abcdef);
        byte[] expected = records.ToArray(); if (matches) Write(expected, 1, 0x9c, 0); Write(expected, 1, 0x50, 0x89abcdee | 0x100);
        var events = new List<string>(); object owner = new();
        RawPlayerWorld.PreDestroyNative(pool, sid, flags, new()
        {
            Current = () => { events.Add("current"); return new(owner, records); },
            Base = (actual, value) => { events.Add("base"); Assert.Equal(sid, actual); Assert.Equal(flags, value); Assert.Equal(matches ? 0u : 0x12345678u, Read(records, 1, 0x9c)); Assert.Equal(0x89abcdefu, Read(records, 1, 0x50)); },
        });
        Assert.Equal(new[] { "current", "base", "current", "current" }, events); Assert.Equal(expected, records);
    }

    /// <summary>각owner는 getter 전에 읽고 다음 필드는 새owner/새base로 주소를 다시 계산한다.</summary>
    [Fact]
    public void EachFieldReadsOwnerBeforeGetterAndUsesNewCurrentBase()
    {
        var (pool, sid) = Scene(0); byte[] first = Records(), second = Records(), third = Records();
        Write(first, 0, 0x9c, (uint)sid); Write(second, 1, 0x50, 0xffffffff); Write(third, 2, 0x50, 2);
        byte[] expectedFirst = first.ToArray(), expectedSecond = second.ToArray(), expectedThird = third.ToArray();
        Write(expectedFirst, 0, 0x9c, 0); Write(expectedSecond, 1, 0x50, 0xfffffffe); Write(expectedThird, 2, 0x50, 0x102);
        int reads = 0;
        RawPlayerWorld.PreDestroyNative(pool, sid, 7, new()
        {
            Current = () =>
            {
                reads++;
                if (reads == 1) { pool.AllocatedBytes(sid)[34] = 8; return new(first, first); }
                if (reads == 2) { pool.AllocatedBytes(sid)[34] = 2; return new(second, second); }
                Assert.Equal(3, reads); return new(third, third);
            },
            Base = (_, _) => { Assert.Equal(0u, Read(first, 0, 0x9c)); pool.AllocatedBytes(sid)[34] = 1; },
        });
        Assert.Equal(3, reads); Assert.Equal(expectedFirst, first); Assert.Equal(expectedSecond, second); Assert.Equal(expectedThird, third);
    }

    /// <summary>owner를9행으로선행제한하지않고현재실제view의물리범위로만각주소를검사한다.</summary>
    [Theory]
    [InlineData(9)]
    [InlineData(255)]
    public void WideOwnerByteUsesActualViewRangeWithoutLogicalClamp(byte owner)
    {
        var (pool, sid) = Scene(owner); byte[] records = new byte[(owner + 1) * 180];
        Write(records, owner, 0x9c, (uint)sid); Write(records, owner, 0x50, 1); int reads = 0, bases = 0;
        RawPlayerWorld.PreDestroyNative(pool, sid, uint.MaxValue, new()
        { Current = () => { reads++; return new(records, records); }, Base = (_, _) => bases++ });
        Assert.Equal(3, reads); Assert.Equal(1, bases); Assert.Equal(0u, Read(records, owner, 0x9c)); Assert.Equal(0x100u, Read(records, owner, 0x50));
    }

    /// <summary>공통 Pre 예외는 앞선SID0만 남기고 뒤flags 쓰기/조회는 실행하지 않는다.</summary>
    [Fact]
    public void BaseFailureKeepsOnlyTechnologyPrefix()
    {
        var (pool, sid) = Scene(); byte[] records = Records(); Write(records, 1, 0x9c, (uint)sid); int reads = 0;
        var failure = new IOException("현재 공통 Pre 실패"); byte[] expected = records.ToArray(); Write(expected, 1, 0x9c, 0);
        Assert.Same(failure, Assert.Throws<IOException>(() => RawPlayerWorld.PreDestroyNative(pool, sid, 0, new()
        { Current = () => { reads++; return new(records, records); }, Base = (_, _) => throw failure })));
        Assert.Equal(1, reads); Assert.Equal(expected, records);
    }

    /// <summary>세 번째 짧은현재view는 두 번째 bit0 해제까지 보존하고bit8을다른배열에쓰지않는다.</summary>
    [Fact]
    public void LastFieldRangeFailureKeepsPriorClear()
    {
        var (pool, sid) = Scene(); byte[] records = Records(); Write(records, 1, 0x9c, (uint)sid); Write(records, 1, 0x50, 1);
        byte[] expected = records.ToArray(); Write(expected, 1, 0x9c, 0); Write(expected, 1, 0x50, 0); int reads = 0, bases = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPlayerWorld.PreDestroyNative(pool, sid, 0, new()
        { Current = () => new(records, ++reads == 3 ? records.AsMemory(0, 180 + 0x50 + 3) : records), Base = (_, _) => bases++ }));
        Assert.Equal(3, reads); Assert.Equal(1, bases); Assert.Equal(expected, records);
    }

    /// <summary>판본미지원·첫범위오류와늦은미설치Base를같은실제선택순서로구분한다.</summary>
    [Fact]
    public void EditionAndMissingDevicesFailAtTheirOriginalSelection()
    {
        var (cd, cdSid) = Scene(edition: OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => RawPlayerWorld.PreDestroyNative(cd, cdSid, 0, new() { Current = () => throw new IOException("CD에서 기록 조회 금지") }));
        var (pool, sid) = Scene();
        Assert.Throws<InvalidOperationException>(() => RawPlayerWorld.PreDestroyNative(pool, sid, 0, new()));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPlayerWorld.PreDestroyNative(pool, sid, 0, new() { Current = () => new(null, Memory<byte>.Empty), Base = (_, _) => Assert.Fail("첫 범위 뒤 부모 금지") }));
        byte[] records = Records(); Write(records, 1, 0x9c, (uint)sid); byte[] expected = records.ToArray(); Write(expected, 1, 0x9c, 0);
        Assert.Throws<InvalidOperationException>(() => RawPlayerWorld.PreDestroyNative(pool, sid, 0, new() { Current = () => new(records, records) }));
        Assert.Equal(expected, records);
    }
}
