using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 의식 사제 wrapper의 원본 현재타입/프레임/예약 순서를 실제 같은풀 소비자와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>유효/범위밖 SID, 현재 first-table, frame callback 뒤 현재값, 확보 실패/604 요청을 실제 PE와 대조한다.</summary>
public sealed class RawPriestRitualActionsTests
{
    /// <summary>부호있는 입력의 원문 int32다.</summary>
    private static int I(string value) => int.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>부호비트가 있는 원문 DWORD 입력이다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>앞 A 둘과 K..P 각1/2/3개의 실제 LetterRun 입력을 만든다.</summary>
    private static RiftTypeFrames RitualCodes(uint profile)
    {
        var codes = new List<RiftFrameCode> { new((byte)'A', (byte)'P', 1, 0), new((byte)'A', (byte)'P', 2, 0) };
        // stage마다 현재 자산의 같은 글자 구간을 구성한다. 기대 선택값을 계산하지 않는다.
        for (uint stage = 0; stage < 6; stage++)
            for (uint index = 0; index <= profile; index++)
                codes.Add(new((byte)('K' + stage), (byte)'P', (byte)(index + 1), 0));
        return new RiftTypeFrames(codes);
    }
    /// <summary>같은 풀/표/효과로 한 행을 재생하고 원문 trace/바이트와 대조한다.</summary>
    private static string Replay(bool patch, uint mode, uint sid, uint stage, uint profile, uint type, uint reserve, uint mutation, uint rawState, List<string> effects)
    {
        var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        int count = patch ? 188 : 171;
        OriginalTypeRecord[] types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray();
        uint dataSid = sid > 0 && sid < 24000 ? sid : 5;
        for (uint number = 5; number <= 8; number++) Assert.Equal((int)number, pool.Allocate(SidAllocation.Client));
        Span<byte> Raw() => pool.RawSlot((int)dataSid);
        Raw().Clear(); BinaryPrimitives.WriteUInt32LittleEndian(Raw(), 0x11010000);
        Raw()[10] = (byte)type; Raw()[11] = (byte)rawState;
        RiftTypeFrames[] frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[133] = RitualCodes(profile); frames[158] = RitualCodes(profile);
        var hooks = new PriestRitualActionHooks
        {
            SetFrame = (current, index, flags) =>
            {
                Assert.Equal((int)dataSid, current); Assert.Equal(0u, flags);
                effects.Add("F:" + current + "," + unchecked((uint)index) + "," + flags);
                if (patch) BinaryPrimitives.WriteUInt32LittleEndian(Raw()[36..], unchecked((uint)index)); else Raw()[34] = (byte)index;
                if (mutation == 1) Raw()[10] = (byte)(type == 158 ? 133 : 158);
            },
            ReserveShared = () => { effects.Add("N:40"); if (mutation == 2) Raw()[11] = 2; return reserve != 0; },
            AddShared = (current, ev, payload) =>
            {
                Assert.Equal((ushort)sid, (ushort)current); Assert.Equal(0x25cu, ev);
                Assert.Equal(0x3c23d70au, BitConverter.SingleToUInt32Bits(payload));
                effects.Add("S:" + ev + "," + current + "," + BitConverter.SingleToUInt32Bits(payload));
            },
        };
        var body = new RawPriestRitualActions(pool, frames, hooks);
        if (mode == 0) body.Begin(sid, stage); else body.Continue(sid, stage);
        return Convert.ToHexString(pool.Slot((int)dataSid)).ToLowerInvariant();
    }
    /// <summary>원문1752행 정기/하위 입력을 두 판본에서 전수 대조한다.</summary>
    [Fact]
    public void MatchesAllNativePriestRitualRows()
    {
        int rows = 0;
        foreach (string[] row in Rows("priestritualactions-x86.tsv"))
        {
            Assert.Equal(4, row.Length); uint[] values = row[1].Split(',').Select(N).ToArray(); Assert.Equal(8, values.Length);
            bool patch = row[0] == "originals";
            var effects = new List<string>();
            string hex = Replay(patch, values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], effects);
            Assert.Equal(row[2], effects.Count == 0 ? "-" : string.Join(';', effects));
            if (patch) { Assert.Equal(row[3].Length, hex.Length); Assert.Equal(row[3].Substring(8), hex.Substring(8)); }
            else { Assert.Equal(row[3].Substring(8), hex.Substring(8, row[3].Length - 8)); }
            rows++;
        }
        Assert.True(rows > 1000);
    }
    /// <summary>같은 의식 요청/중복 예약을 별도 새 월드에서 수행한다. 팩토리 생성 한계로 CD 결합은 후속이다.</summary>
    [Fact]
    public void BindsRealFrameAndSharedConsumer()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Patch1078);
        var hash = new RawSquidHash(); byte[] spots = new byte[65536];
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[158].Flags2 = 0x210000; types[80].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, 2);
        var factory = new RawSquidFactory(pool, types, false, new RawUnpop(pool, hash, spots));
        var diagnostics = new List<string>();
        factory.BindDiagnostics(new() { Assertion = (expression, file, line) => diagnostics.Add(expression + "@" + line) });
        int parent = pool.Allocate(SidAllocation.Client); parent = factory.Construct(158, parent); Span<byte> raw = pool.AllocatedBytes(parent); raw[11] = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(raw[33..], 1);
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[158] = RitualCodes(1);
        int updates = 0, reserved = 0;
        var frame = new SquidFrame(pool, types, new()
        {
            FrameSize = (sid, index) => { Assert.Equal(parent, sid); return [1, 1]; },
            Update = (sid, flags) => { Assert.Equal(parent, sid); Assert.Equal(0u, flags); updates++; },
            Unpop = (_, _) => throw new InvalidOperationException("같은 hash단계 의식 프레임은 Unpop을 실행하지 않는다"),
            Pop = (_, _, _, _) => throw new InvalidOperationException("같은 hash단계 의식 프레임은 Pop을 실행하지 않는다"),
        });
        var unpop = new RawUnpop(pool, hash, spots);
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var kernel = new Kernel();
        var clock = new SquidProcessState { Now = 99, CurrentProcessFormType = new UIntBox { Value = 80 } };
        var host = new SquidProcessHost(pool, types, kernel, destroy, clock, null, new() { Selected = () => 0, Emit = _ => throw new InvalidOperationException("의식 프레임/예약 검사는 자산 삭제를 실행하지 않는다") });
        host.BindFormFactory(factory);
        var hooks = new PriestRitualActionHooks { ReserveShared = () => { reserved++; return true; } };
        var actions = new RawPriestRitualActions(pool, frames, RawPriestRitualActions.MakePriestRitualActionHooks(pool, frame, host, hooks));
        actions.Begin((uint)parent, 2); Assert.Equal(2, updates); Assert.Equal(0, reserved); Assert.Equal(0, kernel.Size());
        actions.Continue((uint)parent, 4); Assert.Equal(4, updates); Assert.Equal(1, reserved); Assert.Equal(1, kernel.Size());
        Assert.Empty(diagnostics);
        kernel.DrainOwnedForShutdown();
    }
}
