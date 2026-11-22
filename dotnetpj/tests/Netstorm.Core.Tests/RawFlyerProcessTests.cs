using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 원문 생성/홈 연결/전체 writer와 현재 owner·부분 실패 순서를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>flyer-postpop의 공통C/B/W1092관측만 이 단위에서 대조한다. P329는 후속이다.</summary>
public sealed class RawFlyerProcessTests
{
    /// <summary>독립 입력의 unsigned 십진수다.</summary>
    private static uint N(string value) => uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    /// <summary>실제 DWORD만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>현재8BYTE 시각의 원문 비트만 쓴다.</summary>
    private static void Time(Span<byte> bytes, int offset, double value) => BinaryPrimitives.WriteUInt64LittleEndian(bytes[offset..], BitConverter.DoubleToUInt64Bits(value));
    /// <summary>전체 이미지를 원문 소문자 hex 형식으로 변환한다.</summary>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>독립 하네스의 모든 입력만 구성하는 실제 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>원문과 같은 Client SID를 확보하는24000슬롯 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 genus가 바뀌어도 본체가 그대로 읽는188TYPE다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>부착 전후에 같은 주소로 쓰는112BYTE 입력이다.</summary>
        internal FlyerProcessFields Fields { get; } = new(0xa5);
        /// <summary>관측의 현재 서버 참조다.</summary>
        internal bool Server = true;
        /// <summary>관측의 현재 단일 게임 시각이다.</summary>
        internal double Now = 100.25;
        /// <summary>관측의 현재 공통 프로세스 TYPE다.</summary>
        internal UIntBox ProcessType { get; } = new() { Value = 31 };
        /// <summary>외부 효과가 적용할 명시 입력 변경 번호다.</summary>
        internal uint Mutation;
        /// <summary>실제 외부 호출의 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>부착 직전 실제 이미지를 기록한다.</summary>
        internal string AttachImage = "-";
        /// <summary>검사 입력보다 먼저 해제되는 실제 공통 본체다.</summary>
        internal RawFlyerProcess Body { get; }
        /// <summary>Source FlyerScene처럼 입력 객체/필드만 준비하며 기대 결과를 읽지 않는다.</summary>
        internal Scene(string kind, uint[] input)
        {
            // 원문 관측의71번까지 실제 Client 영역을 확보한다.
            for (int sid = 5; sid <= 71; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 원문이 대조하는 다섯 객체의50BYTE 전체를 같은 독립 입력으로 만든다.
            foreach (int sid in new[] { 50, 60, 61, 70, 71 })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Fill(0xa5); Put(raw, 0, 0x50a388); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], sid == 70 ? (ushort)71 : (ushort)0);
                BinaryPrimitives.WriteUInt16LittleEndian(raw[6..], sid == 50 ? (ushort)70 : (ushort)0); BinaryPrimitives.WriteUInt16LittleEndian(raw[8..], 0);
                raw[10] = sid < 70 ? (byte)144 : (byte)31; raw[11] = 0; BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], 17);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(50.25f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(50.5f)); Put(raw, 36, 2); raw[40] = 0;
            }
            // 입력에서 사용하는 실제 세 TYPE의 flags1/genus만 구성한다.
            foreach (int type in new[] { 31, 143, 144 }) { Types[type].Flags1 = 0x400000; Types[type].Flags2 = 0x100000; }
            Fields.Bytes[8] = 0x10; Put(Fields.Bytes, 52, 77); Put(Fields.Bytes, 56, 50);
            if (kind == "C") { Server = input[0] != 0; Array.Fill(Fields.Bytes, (byte)input[4]); Mutation = input[5]; }
            else if (kind == "B")
            {
                Types[144].Flags2 = input[0]; uint layout = input[1];
                if (layout == 0) BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(50)[6..], 0);
                if (layout == 2) Pool.RawSlot(70)[11] = 2; if (layout == 3) Pool.RawSlot(70)[11] = 1;
                if (layout == 4) { Pool.RawSlot(70)[10] = 32; Pool.RawSlot(71)[11] = 2; }
            }
            else
            {
                Fields.Bytes[8] = (byte)input[0]; BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(10), 65534); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(12), 65533);
                Put(Fields.Bytes, 16, 0x3f400000); Put(Fields.Bytes, 20, 0x3f000000); Put(Fields.Bytes, 24, uint.MaxValue); Time(Fields.Bytes, 32, 123.25); Time(Fields.Bytes, 40, -5.75);
            }
            var lookup = new RawContainedLookup(Pool, (_, _) => throw new IOException("홈 Bind의 미선택 가상 삭제"));
            Body = new(Pool, Types, new(() => Server, () => Now, ProcessType), new()
            {
                Assertion = line => { Events.Add($"Assert,{line}"); if (Mutation == 1) { Now = 200; ProcessType.Value = 30; Fields.Bytes[8] = 0x10; } },
                Attach = (fields, parent, type, flags) =>
                {
                    Assert.Same(Fields, fields); AttachImage = Hex(fields.Bytes); Events.Add($"Attach,{type},{parent},{flags}");
                    if (Mutation == 2) { Put(Fields.Bytes, 84, 77); Pool.RawSlot(50)[10] = 143; BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(50)[12..], 13); Now = 200; }
                },
                FindProcess = (parent, type) => (ushort)lookup.Find(parent, type), Fields = form => { Events.Add($"Fields,{form}"); return Fields; },
            });
        }
        /// <summary>원문 관측의 다섯 객체/전역16BYTE를 순서대로 직렬화한다.</summary>
        internal (string Objects, string Globals) Current()
        {
            byte[] objects = new byte[250]; int offset = 0;
            // 객체는 SID 숫자순이 아닌 원문 배열 순서를 유지한다.
            foreach (int sid in new[] { 50, 60, 61, 70, 71 }) { Pool.Slot(sid).CopyTo(objects.AsSpan(offset)); offset += 50; }
            byte[] globals = new byte[16]; Put(globals, 0, Server ? 1u : 0); Time(globals, 4, Now); Put(globals, 12, ProcessType.Value); return (Hex(objects), Hex(globals));
        }
    }

    /// <summary>기존 보호 PE의 공통 생성38/Bind30/전체 writer1024관측을 각각 전수 대조한다.</summary>
    [Theory]
    [InlineData("C", 38)] [InlineData("B", 30)] [InlineData("W", 1024)]
    public void CommonBodiesMatchIndependentPeImages(string kind, int expectedCount)
    {
        int count = 0;
        // 머리글과 후속 P329를 건너뛰고 원문 전체 출력 열을 기대값으로 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "flyer-postpop-x86.tsv")))
        {
            if (!line.StartsWith(kind + "\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(9, row.Length); uint[] input = row[1].Split(',').Select(N).ToArray(); var scene = new Scene(kind, input);
            string packet = "-", result = "-";
            if (kind == "C") scene.Body.Construct(scene.Fields, 50, (ushort)input[1], input[2], BitConverter.UInt32BitsToSingle(input[3]));
            else if (kind == "B") scene.Body.Bind(50, (ushort)input[2]);
            else { byte[] bytes = Enumerable.Repeat((byte)0xa7, 64).ToArray(); result = RawFlyerProcess.WriteProcess(scene.Fields, bytes, input[1] != 0, input[2] != 0).ToString(); packet = Hex(bytes); }
            Assert.Equal(row[2], Hex(scene.Fields.Bytes)); var current = scene.Current(); Assert.Equal(row[3], current.Objects); Assert.Equal(row[4], current.Globals);
            Assert.Equal(row[5], packet); Assert.Equal(row[6], result); Assert.Equal(row[7], scene.AttachImage); Assert.Equal(row[8], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); count++;
        }
        Assert.Equal(expectedCount, count);
    }

    /// <summary>부족한 writer는 무변경이고 read는 앞선 필드만 반영하며 미등록 observer를 거부한다.</summary>
    [Fact]
    public void CodecPreservesRawBitsTailAndPartialFailure()
    {
        var fields = new FlyerProcessFields(0xa5); RawFlyerProcess.InitializeReceived(fields, 1); fields.Bytes[8] = 0x80; Put(fields.Bytes, 52, 77); Put(fields.Bytes, 56, 50); byte[] before = fields.Bytes.ToArray();
        // 최대45BYTE보다 짧은 모든 force 버퍼에서 어떤 원문 이미지/packet도 바뀌지 않는다.
        for (int width = 0; width < 45; width++)
        {
            byte[] packet = Enumerable.Repeat((byte)0xa7, width).ToArray(); Assert.Throws<ArgumentException>(() => RawFlyerProcess.WriteProcess(fields, packet, true, true)); Assert.Equal(before, fields.Bytes); Assert.All(packet, value => Assert.Equal(0xa7, value));
        }
        byte[] full = Enumerable.Repeat((byte)0xa7, 64).ToArray(); Assert.Equal(45, RawFlyerProcess.WriteProcess(fields, full, true, true)); Assert.Equal(0, fields.Bytes[8]); Assert.Equal(0xa7, full[45]);
        var copy = new FlyerProcessFields(0x5a); RawFlyerProcess.InitializeReceived(copy, 0); copy.Bytes[8] = 0x80; Assert.Equal(50u, RawFlyerProcess.ReadData(copy, full));
        Assert.Equal(before.AsSpan(10, 4).ToArray(), copy.Bytes.AsSpan(10, 4).ToArray()); Assert.Equal(before.AsSpan(16, 12).ToArray(), copy.Bytes.AsSpan(16, 12).ToArray()); Assert.Equal(before.AsSpan(32, 16).ToArray(), copy.Bytes.AsSpan(32, 16).ToArray()); Assert.Equal(0x80, copy.Bytes[8]);
        full[8] = 0xff; BinaryPrimitives.WriteUInt16LittleEndian(full.AsSpan(13), 61); BinaryPrimitives.WriteUInt16LittleEndian(full.AsSpan(15), 62); Put(full, 17, 0x80000000);
        Assert.Throws<ArgumentException>(() => RawFlyerProcess.ReadData(copy, full.AsSpan(0, 21))); Assert.Equal(61, copy.Home()); Assert.Equal(0x80000000u, BinaryPrimitives.ReadUInt32LittleEndian(copy.Bytes.AsSpan(16))); Assert.Equal(0x80, copy.Bytes[8]);
        Put(copy.Bytes, 0, 0); byte[] invalid = copy.Bytes.ToArray(); Assert.Throws<InvalidOperationException>(() => RawFlyerProcess.ReadData(copy, full)); Assert.Equal(invalid, copy.Bytes);
    }

    /// <summary>수신 leaf는 지정한 추가 필드만 초기화하고 소멸은두−12345 전체DWORD를 보존한다.</summary>
    [Theory]
    [InlineData(0, 0x509f98u)] [InlineData(1, 0x509f60u)] [InlineData(2, 0x509f24u)]
    public void ReceivedLeavesPreserveCallerPaddingAndRealDestructionSentinels(int theme, uint table)
    {
        var fields = new FlyerProcessFields(0xa5); RawFlyerProcess.InitializeReceived(fields, theme); Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes)); Assert.Equal(0xa5a5, fields.Home()); Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(80)));
        Assert.All(fields.SunTail.AsSpan(16, 8).ToArray(), value => Assert.Equal(0xa5, value)); if (theme == 2) Assert.All(fields.SunTail.AsSpan(0, 16).ToArray(), value => Assert.Equal(0, value));
        RawFlyerProcess.DestructFields(fields); Assert.Equal(0x5019ecu, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(48))); Assert.Equal(0xffffcfc7u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(52))); Assert.Equal(0xffffcfc7u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(56)));
    }

    /// <summary>같은 실제 typed Host의 flags8 생성/Kernel 실행과40삭제가 Form/부모/깊이를 반납한다.</summary>
    [Fact]
    public void ActualTypedHostClosesFlyerKernelFormAndParentChain()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 원문 부모50까지 같은 Client 번호를 확보하고 빈 종속 체인으로 준비한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 144; var kernel = new Kernel();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); byte[] packet = new byte[64]; int sends = 0, frames = 0;
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new()
        {
            WriteData = (process, changed) => Assert.IsType<FlyerProcess>(process).WritePacket(packet, changed), Transmit = length => { Assert.Equal(45, length); sends++; },
        });
        FlyerProcess? current = null; var body = new RawFlyerProcess(pool, types, new(() => true, () => 10, new() { Value = 31 }), new()
        {
            Attach = (fields, parent, type, flags) => { Assert.Same(current!.Fields, fields); Assert.Same(current, host.Attach(current, type, parent, flags)); }, Frame = _ => frames++,
        });
        current = new(body, new FlyerProcessFields(0xa5));
        try
        {
            body.Construct(current.Fields, 50, 23, 0x3f400000, 0.5f); RawFlyerProcess.SpecializeRain(current.Fields); int form = current.Form();
            Assert.Equal(1, sends); Assert.Equal(1, kernel.Size()); Assert.Equal(form, current.Fields.Form()); Assert.Equal(50, current.Fields.Parent()); Assert.Equal(0, current.Fields.Bytes[8]); kernel.RunFrame(); Assert.Equal(1, frames);
            host.Kill(current, 0x40); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(50)[6..])); Assert.True((pool.Slot(form)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>TYPE 참조와 copied checking 뷰/현재 owner를 구별하고 큰 미복원을 조용히 생략하지 않는다.</summary>
    [Fact]
    public void CurrentTypesOwnersCopiedCheckingAndFinishRereadsArePreserved()
    {
        var pool = new RawSidPool(24000, false); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 144;
        bool server = true; double now = 100; uint checking = 0; var role = new UIntBox { Value = 31 }; var globals = new FlyerProcessGlobals(() => server, () => now, role) { Checking = new(() => checking) }; var fields = new FlyerProcessFields(); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(10), 61); Put(fields.Bytes, 56, 50);
        int findCalls = 0; ushort found = 70; var lines = new List<uint>(); var events = new List<string>();
        var body = new RawFlyerProcess(pool, types, globals, new()
        {
            Assertion = line => lines.Add(line), FindProcess = (sid, type) => { Assert.Equal(50, sid); Assert.Equal(role.Value, type); findCalls++; return found; }, Fields = _ => fields,
            Pieces = sid => { events.Add($"pieces:{sid}"); Put(fields.Bytes, 56, 60); }, Destroy = (sid, flags) => events.Add($"destroy:{sid}:{flags}"),
        });
        types[144].Flags2 = 0x100000; Assert.Equal(61, body.Home(50)); Assert.Empty(lines);
        globals.Checking = new(false); checking = 0x80000000; role.Value = 44; found = 0; Assert.Equal(0, body.Home(50)); Assert.Equal(new uint[] { 0x8f5 }, lines); Assert.Equal(2, findCalls);
        server = false; body.Finish(fields); Assert.Equal(new uint[] { 0x8f5, 0x41d }, lines); Assert.Equal(new[] { "pieces:50", "destroy:60:8" }, events);
        Assert.Throws<InvalidOperationException>(() => body.Frame(fields)); Assert.Throws<InvalidOperationException>(() => body.Notify(fields, 2, 3, 4)); Assert.Throws<InvalidOperationException>(() => body.ParentDying(fields));
        RawFlyerProcess.InitializeReceived(fields, 0); body.Command(fields, 2); Assert.Equal(125, BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(40))); now = 200; body.Command(fields, 3); Assert.Equal(0ul, BinaryPrimitives.ReadUInt64LittleEndian(fields.Bytes.AsSpan(40)));
        Time(fields.Bytes, 104, double.NaN); body.Command(fields, 1); Assert.Equal(260, BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(104)));
    }
}
