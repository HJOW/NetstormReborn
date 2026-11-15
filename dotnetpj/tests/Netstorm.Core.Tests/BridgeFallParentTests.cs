using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 현재 checking과 실제 다리 낙하 예약을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj/tests/BridgeFallParentTests.cpp의 원본 부모·실제 Host 예약 검사를 옮긴다.</summary>
public sealed class BridgeFallParentTests
{
    /// <summary>원본 관측에 공급한 좌표의 binary32 입력이다.</summary>
    private static readonly uint[][] Coordinates = [[0x80000000, 0x41780000], [0x42fa8000, 0xc0600000], [0x7fc12345, 0x7f800000]];

    /// <summary>원문200개 checking·genus·복귀 진단·원래 좌표 비트를 최초 관측에 대조한다.</summary>
    [Fact]
    public void X86Native200CurrentCheckingArgumentsAndResume()
    {
        int count = 0;
        // 각 고유 입력을 같은 현재 DWORD owner와 명시 장치로 재생한다.
        X86Fixture.CheckRows(X86Fixture.Read("bridge-fall-parent"), row =>
        {
            uint[] input = row[0].Split(',').Select(X86Fixture.UInt).ToArray();
            int sid = (ushort)input[0]; uint word = input[1], genus = input[2];
            var checking = new RawCheckingFlag(() => word);
            var events = new List<string>();
            string outcome = "ok";
            var hooks = new BridgeFallParentHooks
            {
                Genus = current => { Assert.Equal(sid, current); events.Add($"G:{0x12000000u + (uint)current * 50}:{genus}"); if (input[5] != 0) word = 0; return genus; },
                Assertion = (condition, source, line) =>
                {
                    events.Add($"D:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(condition))}:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(source))}:{line}");
                    if (input[5] != 0) word = 0xabcdef10;
                    if (input[4] == 0) throw new InvalidOperationException("assertion");
                },
                Schedule = (current, x, y) => { Assert.Equal(sid, current); events.Add($"F:{0x12000000u + (uint)current * 50}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}"); },
            };
            try { RawBridgeFallParent.Run(sid, BitConverter.UInt32BitsToSingle(Coordinates[input[3]][0]), BitConverter.UInt32BitsToSingle(Coordinates[input[3]][1]), checking, hooks); }
            catch (InvalidOperationException error) { outcome = error.Message; }
            Assert.Equal(row[1], outcome); Assert.Equal(X86Fixture.UInt(row[2]), word); Assert.Equal(row[3], string.Join(';', events));
            count++; return true;
        });
        Assert.Equal(200, count);
    }

    /// <summary>실제 Regular 등록 앞에서 진단하고 복귀 후 숫자5396의 낙하 자식을 같은 Kernel에 붙인다.</summary>
    [Fact]
    public void ActualHostChecksBeforeRegistrationAndKeepsChild()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[2].ConstructorAddress = OriginalConstructors.Address(2);
        var kernel = new Kernel();
        var state = new SquidProcessState { Now = 9 };
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new(), new byte[65536]), types);
        var host = new SquidProcessHost(pool, types, kernel, destroy, state);
        int parent = pool.Allocate(); pool.RawSlot(parent).Clear(); pool.RawSlot(parent)[10] = 82;
        uint word = 0; var checking = new RawCheckingFlag(() => word);
        RegularProcess? first = RawBridgeFallParent.ScheduleChecked(host, pool, types, parent, 20.75f, 21.9f, checking);
        Assert.NotNull(first); Assert.Equal(RawBridgeEvents.FallEvent, first.Event()); Assert.Equal(5396f, first.Payload()); Assert.Equal(9, first.Time()); Assert.Equal(0u, first.Count()); Assert.Equal(1, kernel.Size());
        word = 1; uint free = pool.FreeCount;
        Assert.Throws<InvalidOperationException>(() => RawBridgeFallParent.ScheduleChecked(host, pool, types, parent, 20, 21, checking));
        Assert.Equal(1, kernel.Size()); Assert.Equal(free, pool.FreeCount);
        types[82].Flags2 = TypeFlag2.Bridge;
        Assert.Equal(5396f, RawBridgeFallParent.ScheduleChecked(host, pool, types, parent, 20.75f, 21.9f, checking)!.Payload());
        Assert.Equal(2, kernel.Size());
        types[82].Flags2 = 0; word = 0x80000000; int assertions = 0;
        RegularProcess? resumed = RawBridgeFallParent.ScheduleChecked(host, pool, types, parent, 20.75f, 21.9f, checking, (condition, source, line) =>
        {
            Assert.Equal("s.getGenus() & gBRIDGE", condition); Assert.Equal(@"\Ns\O\Bridge.cpp", source); Assert.Equal(0x323u, line);
            types[82].Flags2 = TypeFlag2.Bridge; assertions++;
        });
        Assert.NotNull(resumed); Assert.Equal(5396f, resumed.Payload()); Assert.Equal(1, assertions); Assert.Equal(3, kernel.Size());
        Assert.Throws<ArgumentException>(() => RawBridgeFallParent.ScheduleChecked(host, new(32768), types, parent, 20, 21, checking));
        kernel.Clear();
    }

    /// <summary>checking0은 진단과 타입 장치를 미리 요구하지 않으며 선택된 예약 부재는 명시 실패한다.</summary>
    [Fact]
    public void OnlyReachedDevicesAreRequired()
    {
        int scheduled = 0;
        RawBridgeFallParent.Run(0, float.NaN, float.PositiveInfinity, new(false), new() { Schedule = (_, _, _) => scheduled++ });
        Assert.Equal(1, scheduled);
        Assert.Throws<InvalidOperationException>(() => RawBridgeFallParent.Run(0, 0, 0, new(false), new()));
        Assert.Throws<InvalidOperationException>(() => RawBridgeFallParent.Run(0, 0, 0, new(true), new() { Schedule = (_, _, _) => scheduled++ }));
        Assert.Equal(1, scheduled);
    }
}
