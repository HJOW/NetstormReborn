using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 두 PE의 디스크 정기와 PATCH IDIV 경계를 기대값 수정 없이 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문192정기/70정수경계와 현재 getter·fallback 검사다.</summary>
public sealed class RawDiscRegularTests
{
    /// <summary>같은 실제 Host/Kernel의 정기가 현재 자료와 Frame.Set을 호출하고 float payload를 보존한다.</summary>
    [Fact]
    public void ActualHostUsesCurrentTypeAndFrameAndKeepsPayload()
    {
        var pool = new RawSidPool(24000, true); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 실제 정기 부모50의 번호를 그대로 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        Span<byte> raw = pool.RawSlot(50); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x50b570); raw[10] = 83; raw[33] = 1; ArcherTypeData[] data = types.Select(_ => new ArcherTypeData()).ToArray(); var definition = new RiftTypeDefinition(); definition.Properties.Add(new("orientations", 1));
        // 별도 계산 상수 대신 실제 정의의 클러스터를 자료 공급기에 전달한다.
        for (int i = 0; i < 4; i++) definition.Clusters.Add(new());
        data[83] = RawArcherTypeData.Read(definition); int updates = 0; var setter = new SquidFrame(pool, types, new() { FrameSize = (_, _) => [1, 1], Update = (_, _) => updates++, Unpop = (_, _) => throw new InvalidOperationException("미선택 공간해제"), Pop = (_, _, _, _) => throw new InvalidOperationException("미선택 공간등록") }); var logic = new RawDiscRegular(pool, data, setter); var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); var clock = new SquidProcessState { Now = 10 }; var host = new SquidProcessHost(pool, types, kernel, destroy, clock, logic.Handler(), new() { Selected = () => 0 });
        try
        {
            RegularProcess process = Assert.IsType<RegularProcess>(host.AddRegular(50, 99, .25f, 0x52)); clock.Now = 11; kernel.RunFrame(); Assert.Equal(1u, process.Count()); Assert.Equal(.25f, process.Payload()); Assert.Equal(1, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(50)[36..])); definition.Clusters.Add(new()); definition.Clusters.Add(new()); data[83] = RawArcherTypeData.Read(definition); BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(50)[36..], 3); clock.Now = 12; kernel.RunFrame(); Assert.Equal(2u, process.Count()); Assert.Equal(4, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(50)[36..])); Assert.Equal(4, updates); Assert.Same(process, host.FindEvent(50, 99)); host.Kill(process, 0x10); Assert.Equal(0, kernel.Size()); Assert.Equal(0, pool.Slot(50)[11] & 3); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
    /// <summary>현재 자료 getter 복귀 뒤 raw프레임을 읽으며 IDIV오류 전 무변경·다른VT fallback을 유지한다.</summary>
    [Fact]
    public void CurrentGetterAndDivideFaultPreserveSourceBoundaries()
    {
        var pool = new RawSidPool(24000, false); int sid = pool.Allocate(SidAllocation.Client); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); ArcherTypeData[] data = types.Select(_ => new ArcherTypeData()).ToArray(); Span<byte> raw = pool.AllocatedBytes(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x50b570); raw[10] = 83; raw[33] = 1; int updates = 0; var setter = new SquidFrame(pool, types, new() { FrameSize = (_, _) => [1, 1], Update = (_, _) => updates++, Unpop = (_, _) => throw new InvalidOperationException("미선택 공간해제"), Pop = (_, _, _, _) => throw new InvalidOperationException("미선택 공간등록") }); float result = RawDiscRegular.HandleIn(pool, setter, type => { Assert.Equal(83u, type); BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(sid)[36..], 3); return new(1, 6); }, sid, -.25f, true); Assert.Equal(-.25f, result); Assert.Equal(4, BinaryPrimitives.ReadInt32LittleEndian(raw[36..])); byte[] before = raw.ToArray(); int prior = updates; Assert.Throws<ArithmeticException>(() => RawDiscRegular.HandleIn(pool, setter, _ => new(0, 4), sid, .25f, true)); Assert.Equal(before, pool.Slot(sid).ToArray()); Assert.Equal(prior, updates); var logic = new RawDiscRegular(pool, data, setter); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0); int calls = 0; RegularHandler handler = logic.Handler((parent, ev, count, payload) => { Assert.Equal(sid, parent); Assert.Equal(99u, ev); Assert.Equal(7u, count); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(payload)); calls++; return .75f; }); Assert.Equal(.75f, handler(sid, 99, 7, BitConverter.UInt32BitsToSingle(0x80000000))); Assert.Equal(1, calls); Assert.Throws<InvalidOperationException>(() => logic.Handler()(sid, 99, 7, 1)); Assert.Throws<ArgumentException>(() => new RawDiscRegular(new(24000), data, setter));
    }
    /// <summary>부호있는 입력의 원문 int32다.</summary>
    private static int I(string value) => int.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>부호비트가 있는 원문 DWORD 입력이다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>실제 Frame.Set과 같은 원문 프레임/반환float를 두 판본96행씩 비교한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllNativeRegularFramesAndPayload(bool patch)
    {
        var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); int sid = pool.Allocate(SidAllocation.Client); OriginalTypeRecord[] types = Enumerable.Range(0, patch ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); ArcherTypeData[] data = types.Select(_ => new ArcherTypeData()).ToArray(); int updates = 0; var setter = new SquidFrame(pool, types, new() { FrameSize = (_, _) => [1, 1], Update = (_, flags) => { Assert.Equal(0u, flags); updates++; }, Unpop = (_, _) => throw new InvalidOperationException("같은 단계에서 미선택 Unpop"), Pop = (_, _, _, _) => throw new InvalidOperationException("같은 단계에서 미선택 Pop") }); var logic = new RawDiscRegular(pool, data, setter); int count = 0;
        // 결과열은 실제 본문 실행 뒤 비교에만 사용한다.
        foreach (string[] row in Rows("disc-regular-x86.tsv"))
        {
            if (row[0] != (patch ? "originals" : "originalCD")) continue; Span<byte> raw = pool.AllocatedBytes(sid); BinaryPrimitives.WriteUInt32LittleEndian(raw, patch ? 0x50b570u : 0x501440u); raw[10] = 83; raw[11] = 0; raw[patch ? 33 : 31] = 1; if (patch) BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], N(row[1])); else raw[34] = (byte)N(row[1]); updates = 0; data[83].Directions = I(row[3]); data[83].FrameCount = I(row[2]); Assert.Equal(I(row[7]), RawDiscRegular.NextFrame(I(row[1]), I(row[2]), I(row[3]))); float result = logic.Handle(sid, N(row[4]), N(row[5]), BitConverter.UInt32BitsToSingle(N(row[6]))); Assert.Equal(N(row[9]), BitConverter.SingleToUInt32Bits(result)); Assert.Equal(N(row[7]) & (patch ? uint.MaxValue : 255), patch ? BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]) : raw[34]); Assert.Equal(0u, N(row[8])); Assert.Equal(2, updates); count++;
        }
        Assert.Equal(96, count);
    }
    /// <summary>음수/경계/감김의 정상47건과 실제 IDIV오류23건을 독립 원문과 대조한다.</summary>
    [Fact]
    public void MatchesAllNativeIntegerBoundaries()
    {
        int count = 0, faults = 0;
        // 정수 결과를 검사에서 재계산하지 않고 원문 DWORD에 직접 비교한다.
        foreach (string[] row in Rows("disc-regular-boundary-1219-x86.tsv")) { Assert.Equal(8, row.Length); if (row[3] == "divide-error") { Assert.Throws<ArithmeticException>(() => RawDiscRegular.NextFrameNative(I(row[0]), I(row[1]), I(row[2]))); faults++; } else { Assert.Equal(N(row[4]), unchecked((uint)RawDiscRegular.NextFrameNative(I(row[0]), I(row[1]), I(row[2])))); Assert.Equal(0u, N(row[5])); Assert.Equal(0x3d800000u, N(row[6])); } count++; }
        Assert.Equal(70, count); Assert.Equal(23, faults);
    }
}
