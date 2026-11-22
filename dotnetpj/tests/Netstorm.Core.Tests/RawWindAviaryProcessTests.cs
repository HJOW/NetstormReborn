using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 Wind 프레임/삭제/원주를 독립 원문 전체 이미지와 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>Wind 원문3039관측과 같은 실제 Host/Kernel/Flyer 수명 검사다.</summary>
public sealed class RawWindAviaryProcessTests
{
    /// <summary>부호 비트를 포함한 입력을 낮은DWORD로 읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원문 텍스트의 유한/비유한 double 입력이다.</summary>
    private static double D(string value) => value.ToLowerInvariant() switch { "nan" => double.NaN, "inf" or "+inf" => double.PositiveInfinity, "-inf" => double.NegativeInfinity, _ => double.Parse(value, CultureInfo.InvariantCulture) };
    /// <summary>지정DWORD만 little endian으로 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>지정WORD만 little endian으로 기록한다.</summary>
    private static void Word(Span<byte> bytes, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(bytes[offset..], value);
    /// <summary>raw double의8BYTE만 기록한다.</summary>
    private static void Time(Span<byte> bytes, int offset, double value) => BinaryPrimitives.WriteDoubleLittleEndian(bytes[offset..], value);
    /// <summary>전체 이미지를 독립 관측의 소문자hex로 표시한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>CPP WindScene의 독립 입력과 선택된 외부 효과만 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 Client 영역의 부모/플라이어/Form을 확보하는 같은 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 실제80BYTE 입력이다.</summary>
        internal WindAviaryProcessFields Fields { get; } = new(0xa5);
        /// <summary>현재fort owner다.</summary>
        internal bool Fort;
        /// <summary>현재friendly owner다.</summary>
        internal bool Friendly;
        /// <summary>현재server owner다.</summary>
        internal bool Server = true;
        /// <summary>현재boss owner다.</summary>
        internal bool Boss = true;
        /// <summary>현재Editor owner다.</summary>
        internal uint Editor;
        /// <summary>현재단일시각 owner다.</summary>
        internal double Now = 100;
        /// <summary>현재59 역할의 같은 주소다.</summary>
        internal UIntBox ProcessType { get; } = new() { Value = 59 };
        /// <summary>현재138 역할의 같은 주소다.</summary>
        internal UIntBox FlyerType { get; } = new() { Value = 138 };
        /// <summary>Session의 현재139 역할이다.</summary>
        internal UIntBox AviaryType { get; } = new() { Value = 139 };
        /// <summary>실제 원주 몸체를 선택할지 여부다.</summary>
        internal bool FullCircle;
        /// <summary>원주 외부 효과의 명시 변경 번호다.</summary>
        internal uint CircleMutation;
        /// <summary>현재 헤더 입력이다.</summary>
        internal WindCircleHeader CircleHeader;
        /// <summary>원주가 같은 주소에서 현재값을 읽는188TYPE다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택된 실제 효과 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>같은 입력과 장기참조를 사용하는 제품 본체다.</summary>
        internal RawWindAviaryProcess Body { get; }
        /// <summary>결과를 계산하지 않고 독립 하네스의 입력만 준비한다.</summary>
        internal Scene(string kind, string[] input)
        {
            // 원문이 비교하는70번까지 실제Client 번호를 확보한다.
            for (int sid = 5; sid <= 70; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 세50BYTE의 타입/좌표/체인/미정바이트를 같은 원문 입력으로 준비한다.
            foreach (int sid in new[] { 50, 60, 70 })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Fill(0xa5); Put(raw, 0, 0x1240f000); Word(raw, 4, 0); Word(raw, 6, sid == 50 ? (ushort)70 : (ushort)0); Word(raw, 8, 0); raw[10] = sid == 50 ? (byte)139 : sid == 60 ? (byte)138 : (byte)59; raw[11] = 0; Word(raw, 12, 0); Put(raw, 14, BitConverter.SingleToUInt32Bits(sid + .25f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(sid + .5f)); raw[34] = 1; raw[40] = 0; Put(raw, 36, 0);
            }
            Put(Fields.Bytes, 0, 0x509dfc); Fields.Bytes[8] = 0x10; Time(Fields.Bytes, 16, 100); Put(Fields.Bytes, 24, 1); Time(Fields.Bytes, 32, 99); Time(Fields.Bytes, 40, 100); Put(Fields.Bytes, 48, 0); Put(Fields.Bytes, 56, 0x509ddc); Put(Fields.Bytes, 60, 77); Put(Fields.Bytes, 64, 50); Put(Fields.Bytes, 68, 4); Put(Fields.Bytes, 72, 2);
            if (kind == "F") { Put(Fields.Bytes, 24, N(input[0])); Time(Fields.Bytes, 40, 100 - D(input[1])); Time(Fields.Bytes, 32, D(input[2])); Put(Fields.Bytes, 48, N(input[3])); Server = N(input[4]) != 0; Boss = N(input[5]) != 0; Pool.RawSlot(50)[10] = (byte)N(input[6]); Editor = N(input[7]); Pool.RawSlot(50)[40] = (byte)N(input[8]); Put(Pool.RawSlot(50), 36, N(input[9])); }
            else { Server = N(input[0]) != 0; Fort = N(input[1]) != 0; Friendly = N(input[2]) != 0; Array.Fill(Fields.Bytes, (byte)0xa5); Word(Pool.RawSlot(50), 6, 0); }
            Body = new(Pool, Globals(), Hooks());
        }
        /// <summary>같은 현재owner를 빌리는 입력 구조체다.</summary>
        internal WindAviaryGlobals Globals() => new(() => Fort, () => Server, () => Boss, () => Editor, () => Now, ProcessType, FlyerType, AviaryType) { FriendlyFire = new(() => Friendly) };
        /// <summary>독립입력의 큰효과/반환값만 제공하고 다음제품상태를 계산하지 않는다.</summary>
        internal WindAviaryHooks Hooks()
        {
            var lookup = new RawContainedLookup(Pool, (_, _) => throw new IOException("미선택 삭제"));
            return new()
            {
                FindProcess = (sid, type) => (ushort)lookup.Find(sid, type), Allocate = () => { Events.Add("Allocate,80"); return new(0xa5); },
                Attach = (fields, parent, type) => { Assert.NotNull(fields); Events.Add($"Attach,{type},{parent},8"); fields.Bytes.CopyTo(Fields.Bytes, 0); },
                Damageable = (_, flags) => Events.Add($"Base,{flags}"), Assertion = line => Events.Add($"Assert,{line}"),
                SetFrame = (sid, frame) => { Events.Add($"Frame,{sid},{frame}"); BinaryPrimitives.WriteInt32LittleEndian(Pool.RawSlot(sid)[36..], frame); },
                Create = type => { Events.Add($"Create,{type}"); return 60; },
                SetOwner = (sid, owner) => { Events.Add($"Owner,{owner}"); Pool.RawSlot(sid)[34] = (byte)owner; if (FullCircle && CircleMutation == 1) Pool.RawSlot(sid)[10] = 139; if (FullCircle && CircleMutation == 2) Pool.RawSlot(50)[10] = 139; },
                Height = (_, _) => { Events.Add("Height"); return 44; }, FootY = _ => 3,
                Pop = (_, x, y) => Events.Add($"Pop,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}"),
                CreateFlyerProcess = (sid, parent) => { Events.Add("Allocate,104"); Events.Add($"Process,{sid},{parent},1082130432,1056964608"); }, BindFlyer = (sid, home) => Events.Add($"Bind,{sid},{home}"),
                CirclePosition = (parent, flyer, current) =>
                {
                    if (FullCircle) return RawWindCircle.Position(Pool, Types, parent, flyer, current, (sid, abstractFlag, frame) =>
                    {
                        Events.Add($"Header,{Pool.Slot(sid)[10]},{abstractFlag},{frame}");
                        if (CircleMutation == 3) { Put(Fields.Bytes, 68, 7); Put(Fields.Bytes, 72, 1); Put(Fields.Bytes, 64, 51); Put(Pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(61.75f)); Put(Pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(62.5f)); } return CircleHeader;
                    });
                    Events.Add("SpecialHeight"); Events.Add("Center"); Events.Add($"Circle,{BinaryPrimitives.ReadInt32LittleEndian(current.Bytes.AsSpan(68))},{BinaryPrimitives.ReadInt32LittleEndian(current.Bytes.AsSpan(72))}"); return ((float)(61.25 - 1.399999976158142), (float)(44.0 * .09090909361839294 - 3.0 + 62.5));
                },
                Finish = _ => Events.Add("Finish,0"),
            };
        }
        /// <summary>세 객체의 실제 전체50BYTE를 원문 순서로 합친다.</summary>
        internal string Objects()
        {
            byte[] result = new byte[150]; int offset = 0;
            // 숫자정렬과 무관하게 원문 배열 순서를 유지한다.
            foreach (int sid in new[] { 50, 60, 70 }) { Pool.Slot(sid).CopyTo(result.AsSpan(offset)); offset += 50; } return Hex(result);
        }
    }

    /// <summary>전체F1607/P32의80BYTE/세슬롯/효과순서를 독립 원문과 대조한다.</summary>
    [Theory]
    [InlineData("F", 1607)] [InlineData("P", 32)]
    public void FrameAndPostPopMatchAllIndependentPeObservations(string kind, int expected)
    {
        int count = 0;
        // 비주석 머리글과 다른범위를 건너뛰고 전체 결과를 원문 그대로 대조한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/wind-aviary-frame-x86.tsv")))
        {
            if (!line.StartsWith(kind + "\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(5, row.Length); string[] input = row[1].Split(','); var scene = new Scene(kind, input);
            if (kind == "F") scene.Body.Frame(scene.Fields); else scene.Body.PostPop(50, N(input[3])); Assert.Equal(row[2], Hex(scene.Fields.Bytes)); Assert.Equal(row[3], scene.Objects()); Assert.Equal(row[4], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); count++;
        }
        Assert.Equal(expected, count);
    }
    /// <summary>실제헤더/현재TYPE/중심/원주와 효과뒤현재필드를 독립288관측으로 대조한다.</summary>
    [Fact]
    public void CircleFrameMatches288IndependentPeObservations()
    {
        int count = 0;
        // 기존명시효과 관측을 대체하지 않고 실제원주 경계의 별도 자료를 전수재생한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/wind-circle-frame-x86.tsv")))
        {
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith('#')) continue; string[] row = line.Split('\t'); string[] input = row[0].Split(','); var scene = new Scene("F", "12,0,99,0,1,1,133,0,0,0".Split(',')) { FullCircle = true, CircleMutation = N(input[9]) };
            Put(scene.Fields.Bytes, 68, N(input[0])); Put(scene.Fields.Bytes, 72, N(input[1])); Put(scene.Pool.RawSlot(50), 14, N(input[2])); Put(scene.Pool.RawSlot(50), 18, N(input[3])); scene.Pool.RawSlot(60)[40] = (byte)N(input[10]);
            scene.Types[133].FootX = unchecked((int)N(input[4])); scene.Types[133].FootY = unchecked((int)N(input[5])); scene.Types[139].FootX = scene.Types[133].FootX + 2; scene.Types[139].FootY = scene.Types[133].FootY + 1; scene.CircleHeader = new(BitConverter.UInt32BitsToSingle(N(input[6])), unchecked((short)N(input[7])), unchecked((short)N(input[8]))); scene.Body.Frame(scene.Fields);
            Assert.Equal(row[1], Hex(scene.Fields.Bytes)); Assert.Equal(row[2], scene.Objects()); Assert.Equal(row[3], string.Join(';', scene.Events)); count++;
        }
        Assert.Equal(288, count);
    }
    /// <summary>정수각도/상수/float저장 경계의 원주점232관측을 대조한다.</summary>
    [Fact]
    public void CirclePointMatches232IndependentPeObservations()
    {
        int count = 0;
        // 양수/음수count와DWORD감김index의 원문 두float 비트를 비교한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/wind-circle-point-x86.tsv")))
        {
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith('#')) continue; string[] row = line.Split('\t'); string[] input = row[0].Split(','); var point = RawWindCircle.Point(BitConverter.UInt32BitsToSingle(N(input[2])), BitConverter.UInt32BitsToSingle(N(input[3])), unchecked((int)N(input[0])), unchecked((int)N(input[1]))); Assert.Equal(row[1], $"{BitConverter.SingleToUInt32Bits(point.X)},{BitConverter.SingleToUInt32Bits(point.Y)}"); count++;
        }
        Assert.Equal(232, count); Assert.Throws<ArgumentException>(() => RawWindCircle.Point(0, 0, 0, 0));
    }
    /// <summary>전체DWORD주소/bit31별칭/첫SID/진단재진입/현재VT를 독립880관측과 대조한다.</summary>
    [Fact]
    public void FormDestroyMatches880IndependentPeObservations()
    {
        int count = 0;
        // 정상/예약/free/dead/주소끝의네실제슬롯을 각독립입력에서 다시 준비한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/wind-aviary-destroy-x86.tsv")))
        {
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith('#')) continue; string[] row = line.Split('\t'); uint[] input = row[0].Split(',').Select(N).ToArray(); var pool = new RawSidPool(24000, true); var fields = new WindAviaryProcessFields(0xa5); Put(fields.Bytes, 48, input[1]); bool boss = input[0] != 0; var events = new List<string>();
            // 원문 주소 첫/중간/끝의 전체50BYTE를 그대로 준비한다.
            foreach (int sid in new[] { 1, 60, 127, 23999 }) { Span<byte> raw = pool.RawSlot(sid); raw.Clear(); Put(raw, 0, 0x12200000); raw[10] = (byte)input[3]; raw[11] = (byte)input[2]; raw[34] = 2; }
            var body = new RawWindAviaryProcess(pool, new(() => false, () => true, () => boss, () => 0, () => 0, new() { Value = 59 }, new() { Value = input[4] }, new() { Value = 139 }), new()
            {
                Assertion = number => { events.Add($"A:{number}"); if (input[6] == 1) { Put(fields.Bytes, 48, 0); pool.RawSlot(60)[11] = 2; } if (input[6] == 2) Put(pool.RawSlot(60), 0, 0x12200100); if (input[6] == 3) boss = false; },
                Destroy = (sid, flags) => { events.Add($"{(BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)) == 0x12200100 ? "K2" : "K")}:{sid}:{flags}"); pool.RawSlot(sid)[11] |= 3; },
            });
            body.FormDestroy(fields); string raws = "";
            // 진단효과뒤네슬롯의전체바이트를 원문 배열순서로 합친다.
            foreach (int sid in new[] { 1, 60, 127, 23999 }) raws += Hex(pool.Slot(sid));
            Assert.Equal(row[1], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[2], Hex(fields.Bytes)); Assert.Equal(row[3], raws); count++;
        }
        Assert.Equal(880, count);
    }

    /// <summary>현재owner/복사한뷰·함수슬롯/읽기순서와 선택누락의 부분효과를 보존한다.</summary>
    [Fact]
    public void CurrentOwnersCopiedSlotsReadOrderAndMissingEffectsMatchSource()
    {
        var scene = new Scene("P", "1,1,0,1".Split(',')); uint friendly = 0, checking = 0; int serverReads = 0; var globals = new WindAviaryGlobals(() => scene.Fort, () => { serverReads++; return scene.Server; }, () => scene.Boss, () => scene.Editor, () => scene.Now, scene.ProcessType, scene.FlyerType, scene.AviaryType) { FriendlyFire = new(() => friendly), Checking = new(() => checking) };
        WindAviaryHooks hooks = scene.Hooks(); hooks.Allocate = () => { scene.Events.Add("null"); return null; }; hooks.Attach = (fields, parent, type) => { Assert.Null(fields); Assert.Equal(50, parent); Assert.Equal(scene.ProcessType.Value, type); scene.Events.Add("attach-null"); }; var body = new RawWindAviaryProcess(scene.Pool, globals, hooks);
        hooks.Allocate = null; globals.FriendlyFire = new(false); body.PostPop(50, 0); body.PostPop(50, 1); Assert.Equal(0, serverReads); Assert.Equal(new[] { "Base,0", "Base,1" }, scene.Events); scene.Events.Clear(); friendly = 0x80000000; scene.ProcessType.Value = 44; body.PostPop(50, 1); Assert.Equal(new[] { "null", "attach-null", "Base,1" }, scene.Events);
        Put(scene.Fields.Bytes, 64, 50); Put(scene.Fields.Bytes, 24, 14); checking = 0x80000000; globals.Checking = new(false); scene.Events.Clear(); body.Frame(scene.Fields); Assert.Equal(new[] { "Assert,2266" }, scene.Events);
        scene.Events.Clear(); scene.Server = false; scene.Fields.Bytes[8] = 0x80; byte[] before = scene.Fields.Bytes.ToArray(); body.FlyerDied(scene.Fields, null); Assert.Equal(before, scene.Fields.Bytes); scene.Server = true; Assert.Throws<InvalidOperationException>(() => body.FlyerDied(scene.Fields, null)); Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(24))); Assert.Equal(0x84, scene.Fields.Bytes[8]);
        Put(scene.Fields.Bytes, 48, 60); scene.Pool.RawSlot(60)[10] = (byte)scene.FlyerType.Value; Assert.Throws<InvalidOperationException>(() => body.FormDestroy(scene.Fields));
        var fields = new WindAviaryProcessFields(); Put(fields.Bytes, 64, 50); Put(fields.Bytes, 24, 1); scene.Pool.RawSlot(50)[10] = 139; int nowReads = 0;
        body = new(scene.Pool, new(() => false, () => true, () => true, () => 0, () => ++nowReads, new(), new(), new() { Value = 139 }), new()); body.Frame(fields); Assert.Equal(4, nowReads); Assert.Equal(3, BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(16))); Assert.Equal(3, BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(32))); Assert.Equal(4, BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(40)));
    }

    /// <summary>실제Wind Host/Kernel/홈갱신과 가상Form삭제가 같은다른실제Form을 중첩삭제한다.</summary>
    [Fact]
    public void ActualTypedHostClosesWindKernelHomeWriterAndVirtualFormDeletion()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 부모50과 실제플라이어60까지 같은Client 영역을 확보한다.
        for (int sid = 5; sid <= 60; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 139; pool.RawSlot(60).Clear(); pool.RawSlot(60)[10] = 138; var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); var lengths = new List<int>(); byte[] packet = new byte[64];
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new() { WriteData = (process, changed) => Assert.IsType<WindAviaryProcess>(process).WritePacket(packet, changed), Transmit = size => lengths.Add(size) }); var flyerType = new UIntBox { Value = 138 }; var deleted = new List<ushort>();
        var body = new RawWindAviaryProcess(pool, new(() => false, () => true, () => true, () => 0, () => 100, new() { Value = 59 }, flyerType, new() { Value = 139 }), new() { Destroy = (sid, flags) => { deleted.Add(sid); Assert.Equal(0u, flags); host.Kill(host.FromForm(sid)!, flags); } });
        var fields = new WindAviaryProcessFields(0xa5); RawWindAviaryState.Initialize(fields, true, 100); var process = new WindAviaryProcess(body, fields);
        try
        {
            Assert.Same(process, host.Attach(process, 59, 50, 8)); int form = process.Form(); Assert.Equal(59, pool.Slot(form)[10]); Assert.Equal((uint)form, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(60))); Assert.Equal(50u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(64))); Assert.Equal(new[] { 45 }, lengths); Assert.Equal(0, process.Fields.Bytes[8]); kernel.RunFrame(); Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(24))); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(48)));
            body.FlyerDied(process.Fields, current => { Assert.Same(process.Fields, current); host.RefreshForm(process); }); Assert.Equal(new[] { 45, 33 }, lengths); Assert.Equal(0, process.Fields.Bytes[8]); byte[] before = process.Fields.Bytes.ToArray(); process.Notify(1, 2, 3); Assert.Equal(before, process.Fields.Bytes);
            var flyerBody = new RawFlyerProcess(pool, types, new(() => true, () => 100, new() { Value = 31 }), new()); var flyerFields = new FlyerProcessFields(); RawFlyerProcess.InitializeReceived(flyerFields, 0); var flying = new FlyerProcess(flyerBody, flyerFields); Assert.Same(flying, host.Attach(flying, 31, 60, 0x10)); int targetForm = flying.Form(); flyerType.Value = 31; Put(process.Fields.Bytes, 48, (uint)targetForm);
            host.Kill(process, 0x40); Assert.Equal(new[] { (ushort)targetForm }, deleted); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Null(host.FromForm(targetForm)); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(50)[6..])); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(60)[6..])); Assert.True((pool.Slot(form)[11] & 3) != 0); Assert.True((pool.Slot(targetForm)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>같은실제Flyer 예약이 기본flags8부착 뒤Wind전문화를 받고 특별상태12가한번만 소비한다.</summary>
    [Fact]
    public void ActualWindLaunchUsesSameFlyerReservationBeforeSpecialization()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 실제부모/플라이어의60번까지 원문Client 슬롯을 확보한다.
        for (int sid = 5; sid <= 60; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 133; pool.RawSlot(60).Clear(); pool.RawSlot(60)[10] = 138; var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); FlyerProcess? reserved = null; byte[] packet = new byte[64]; int attached = 0, launches = 0;
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new() { WriteData = (process, changed) => Assert.IsType<FlyerProcess>(process).WritePacket(packet, changed), Transmit = size => { Assert.Equal(45, size); Assert.Equal(0x509cc8u, BinaryPrimitives.ReadUInt32LittleEndian(reserved!.Fields.Bytes)); attached++; } });
        var flyerBody = new RawFlyerProcess(pool, types, new(() => true, () => 100, new() { Value = 31 }), new() { Attach = (fields, parent, type, flags) => { Assert.Same(reserved!.Fields, fields); Assert.Equal(8u, flags); Assert.Same(reserved, host.Attach(reserved, type, parent, flags)); } });
        var fields = new WindAviaryProcessFields(); Put(fields.Bytes, 64, 50); Put(fields.Bytes, 24, 12); Put(fields.Bytes, 48, 60); var body = new RawWindAviaryProcess(pool, new(() => false, () => true, () => true, () => 0, () => 100, new() { Value = 59 }, new() { Value = 138 }, new() { Value = 139 }), new()
        {
            CreateFlyerProcess = (sid, home) => { launches++; Assert.Equal(60, sid); Assert.Equal(50, home); reserved = new(flyerBody, new FlyerProcessFields(0xa5)); flyerBody.Construct(reserved.Fields, sid, home, 0x40800000, .5f); RawFlyerProcess.SpecializeWind(reserved.Fields); },
        });
        try
        {
            body.Frame(fields); Assert.NotNull(reserved); Assert.Equal(1, launches); Assert.Equal(1, attached); Assert.Equal(1, kernel.Size()); Assert.Same(reserved, host.FromForm(reserved.Form())); Assert.Equal(50, reserved.Fields.Home()); Assert.Equal(0x509f98u, BinaryPrimitives.ReadUInt32LittleEndian(reserved.Fields.Bytes)); Assert.Equal(0x40800000u, BinaryPrimitives.ReadUInt32LittleEndian(reserved.Fields.Bytes.AsSpan(16))); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(48))); Assert.Equal(13u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(24)));
            Assert.Throws<InvalidOperationException>(() => body.Frame(fields)); Assert.Equal(1, launches); host.Kill(reserved, 0x40); Assert.Equal(0, kernel.Size()); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}
