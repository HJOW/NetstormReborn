using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 독립 PE의 전체 이미지/현재 전역/효과 순서와 실제 Host 수명을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>맨 오 워 풀1602관측과 홈 통지36관측의 원문 입력을 대조한다.</summary>
public sealed class RawRainAviaryProcessTests
{
    /// <summary>음수 signedDWORD 입력도 낮은32비트로 보존한다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>다른 padding을 보존하고 실제DWORD만 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>실제WORD만 little endian으로 기록한다.</summary>
    private static void Word(Span<byte> bytes, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(bytes[offset..], value);
    /// <summary>raw double의8BYTE만 기록한다.</summary>
    private static void Time(Span<byte> bytes, int offset, double value) => BinaryPrimitives.WriteDoubleLittleEndian(bytes[offset..], value);
    /// <summary>전체 이미지를 독립 관측의 소문자 hex로 표시한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>Source Rain AviaryScene의 입력과 외부 효과만 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>다섯 관측 객체를 실제 Client 번호로 확보하는 같은 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 실제48BYTE 입력이다.</summary>
        internal RainAviaryProcessFields Fields { get; } = new(0xa5);
        /// <summary>현재 서버 전역이다.</summary>
        internal bool Server = true;
        /// <summary>현재fort 입력이다.</summary>
        internal bool Fort;
        /// <summary>현재friendlyFire 입력이다.</summary>
        internal bool FriendlyFire;
        /// <summary>현재 Editor 전역이다.</summary>
        internal uint Editor;
        /// <summary>현재 게임 시각이다.</summary>
        internal double Now = 100;
        /// <summary>현재60 프로세스 역할의 같은 주소다.</summary>
        internal UIntBox ProcessType { get; } = new() { Value = 60 };
        /// <summary>현재144 플라이어 역할의 같은 주소다.</summary>
        internal UIntBox FlyerType { get; } = new() { Value = 144 };
        /// <summary>외부 효과의 명시 변경 입력이다.</summary>
        internal uint Mutation;
        /// <summary>프레임 효과의 실제 호출 횟수다.</summary>
        internal int FrameCalls;
        /// <summary>관측이 선택한 할당 성공 입력이다.</summary>
        internal bool Allocated = true;
        /// <summary>관측 범위 문자다.</summary>
        internal string Kind { get; }
        /// <summary>실제 효과 호출 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>실제 부착 경계가 받은 선택 이미지다.</summary>
        internal RainAviaryProcessFields? Attached;
        /// <summary>같은 입력을 사용하는 제품 본체다.</summary>
        internal RawRainAviaryProcess Body { get; }
        /// <summary>결과를 계산하지 않고 독립 PE 하네스의 입력만 재현한다.</summary>
        internal Scene(string kind, uint[] input)
        {
            Kind = kind;
            // 원문이 비교하는71번까지 실제 Client SID를 확보한다.
            for (int sid = 5; sid <= 71; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 원문 다섯50BYTE 전체를 같은 입력/padding으로 준비한다.
            foreach (int sid in new[] { 50, 51, 60, 70, 71 })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Fill(0xa5); Put(raw, 0, 0x1240f000); Word(raw, 4, sid == 70 ? (ushort)71 : (ushort)0); Word(raw, 6, sid is 50 or 51 ? (ushort)70 : (ushort)0); Word(raw, 8, 127);
                raw[10] = sid is 50 or 51 ? (byte)143 : sid == 60 ? (byte)144 : sid == 70 ? (byte)60 : (byte)31; raw[11] = 0; Word(raw, 12, sid == 50 ? (ushort)150 : sid == 51 ? (ushort)151 : sid == 60 ? (ushort)9 : (ushort)0);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(sid + .25f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(sid + .5f)); raw[34] = sid == 50 ? (byte)1 : sid == 51 ? (byte)2 : (byte)0; Put(raw, 36, sid == 51 ? 7u : sid == 60 ? 42u : 0); raw[40] = 0;
            }
            Put(Fields.Bytes, 0, 0x509e20); Fields.Bytes[8] = 0x10; Time(Fields.Bytes, 16, 100); Put(Fields.Bytes, 24, 1); Put(Fields.Bytes, 32, 0x509e00); Put(Fields.Bytes, 36, 77); Put(Fields.Bytes, 40, 50); Put(Fields.Bytes, 44, 0);
            if (kind == "C") { Server = input[0] != 0; Now = new[] { 0d, 100.25, -5.25 }[input[1]]; Array.Fill(Fields.Bytes, (byte)input[2]); }
            else if (kind == "P")
            {
                Server = input[0] != 0; Fort = input[1] != 0; FriendlyFire = input[2] != 0; Allocated = input[4] != 0; Mutation = input[6]; uint layout = input[5];
                if (layout == 0) Word(Pool.RawSlot(50), 6, 0); if (layout == 2) { Pool.RawSlot(70)[11] = 2; Pool.RawSlot(71)[10] = 60; } if (layout == 3) Pool.RawSlot(70)[11] = 1;
            }
            else if (kind == "F")
            {
                Server = input[0] != 0; Editor = input[1]; Pool.RawSlot(50)[40] = (byte)input[2]; Put(Fields.Bytes, 24, input[3]); Put(Pool.RawSlot(50), 36, input[4]); Put(Fields.Bytes, 44, input[5]); Time(Fields.Bytes, 16, new[] { 99d, 100, 101 }[input[6]]); Mutation = input[7];
            }
            else if (kind == "W") { Fields.Bytes[8] = (byte)input[0]; Time(Fields.Bytes, 16, 123.25); Put(Fields.Bytes, 24, 7); Put(Fields.Bytes, 44, 5); }
            Body = new(Pool, Globals(), Hooks());
        }
        /// <summary>현재 owner 참조를 빌리는 입력 구조체다.</summary>
        internal RainAviaryProcessGlobals Globals() => new(() => Fort, () => Server, () => Editor, () => Now, ProcessType, FlyerType) { FriendlyFire = new(() => FriendlyFire) };
        /// <summary>독립 관측의 외부 반환값과 변경 입력만 제공한다.</summary>
        internal RainAviaryProcessHooks Hooks()
        {
            var lookup = new RawContainedLookup(Pool, (_, _) => throw new IOException("미선택 삭제"));
            return new()
            {
                FindProcess = (sid, type) => { Events.Add($"Find,{sid},{type}"); if (Kind == "P" && Mutation == 1) Server = false; if (Kind == "P" && Mutation == 2) ProcessType.Value = 59; return (ushort)lookup.Find(sid, type); },
                Allocate = size => { Events.Add($"Allocate,{size}"); return Allocated ? new(0xa5) : null; },
                Attach = (fields, sid, type, flags) => { Events.Add($"Attach,{type},{sid},{flags},{(fields is null ? 0 : 1)}"); Attached = fields; },
                Damageable = (_, flags) => Events.Add($"Base,{flags}"), Assertion = line => Events.Add($"Assert,{line}"),
                SetFrame = (sid, frame) => { Events.Add($"Frame,{sid},{frame}"); Put(Pool.RawSlot(sid), 36, unchecked((uint)frame)); FrameCalls++; if (FrameCalls == 1 && Mutation == 1) { Put(Fields.Bytes, 40, 51); Now = 200; } if (FrameCalls == 1 && Mutation == 5) Put(Fields.Bytes, 24, 9); },
                Sound = (x, y, name) => { Assert.Equal("manOWarBuild.wav", name); Events.Add($"Sound,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}"); if (Mutation == 2) { Put(Fields.Bytes, 24, 9); Put(Fields.Bytes, 44, 10); } },
                CheckFrame = (_, frame, line) => Events.Add($"Check,{frame},{line}"),
                Create = (type, flags) => { Events.Add($"Create,{type},{flags}"); return 60; },
                SetOwner = (sid, owner) => { Events.Add($"Owner,{sid},{owner}"); Pool.RawSlot(sid)[34] = (byte)owner; if (Mutation == 3) { Put(Fields.Bytes, 40, 51); Now = 200; } },
                Pop = (sid, x, y, flags) => { Events.Add($"Pop,{sid},{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{flags}"); if (Mutation == 4) { Put(Fields.Bytes, 40, 51); Now = 200; } },
                BindFlyer = (flyer, sid) => Events.Add($"Bind,{flyer},{sid}"),
            };
        }
        /// <summary>다섯 객체의 실제 전체 바이트와 현재 전역28BYTE다.</summary>
        internal (string Objects, string Globals) Current()
        {
            byte[] objects = new byte[250]; int offset = 0;
            // 원문 배열 순서대로 실제50BYTE를 읽는다.
            foreach (int sid in new[] { 50, 51, 60, 70, 71 }) { Pool.Slot(sid).CopyTo(objects.AsSpan(offset)); offset += 50; }
            byte[] globals = new byte[28]; Put(globals, 0, Server ? 1u : 0); Put(globals, 4, Editor); Time(globals, 8, Now); Put(globals, 16, ProcessType.Value); Put(globals, 20, Fort ? 1u : 0); Put(globals, 24, FriendlyFire ? 1u : 0); return (Hex(objects), Hex(globals));
        }
    }

    /// <summary>원문 C/P/F/W의 전체 이미지/객체/전역/패킷/효과 순서를 각각 전수 대조한다.</summary>
    [Theory]
    [InlineData("C", 18)] [InlineData("P", 386)] [InlineData("F", 174)] [InlineData("W", 1024)]
    public void BodiesMatchAllIndependentPeObservations(string kind, int expected)
    {
        int count = 0;
        // 비주석 머리글과 다른 범위를 건너뛰며 기대값은 원문 자료를 그대로 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/rain-aviary-x86.tsv")))
        {
            if (!line.StartsWith(kind + "\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(8, row.Length); uint[] input = row[1].Split(',').Select(N).ToArray(); var scene = new Scene(kind, input); string packet = "-", result = "-";
            if (kind == "C") scene.Body.Initialize(scene.Fields); else if (kind == "P") scene.Body.PostPop(50, input[3]); else if (kind == "F") scene.Body.Frame(scene.Fields);
            else { byte[] bytes = Enumerable.Repeat((byte)0xa7, 64).ToArray(); result = RawRainAviaryProcess.WriteProcess(scene.Fields, bytes, input[1] != 0, input[2] != 0).ToString(CultureInfo.InvariantCulture); packet = Hex(bytes); }
            Assert.Equal(row[2], kind == "P" ? scene.Attached is null ? "-" : Hex(scene.Attached.Bytes) : Hex(scene.Fields.Bytes)); var current = scene.Current(); Assert.Equal(row[3], kind is "P" or "F" ? current.Objects : "-"); Assert.Equal(row[4], current.Globals);
            Assert.Equal(row[5], packet); Assert.Equal(row[6], result); Assert.Equal(row[7], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); count++;
        }
        Assert.Equal(expected, count);
    }

    /// <summary>독립36입력의 전체48BYTE/난수상태와 같은 실제 갱신 이미지를 대조한다.</summary>
    [Fact]
    public void HomeNoticeMatches36IndependentPeObservations()
    {
        var pool = new RawSidPool(24000, false); bool server = false; double now = 0; var body = new RawRainAviaryProcess(pool, new(() => false, () => server, () => 0, () => now, new() { Value = 60 }, new() { Value = 144 }), new()); int count = 0;
        // 현재 서버/시각/raw 입력만 공급하고 기대값은 동결한 실제 PE 출력으로 유지한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/rain-home-notice-x86.tsv")))
        {
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith('#')) continue; string[] row = line.Split('\t'); Assert.Equal(8, row.Length); server = N(row[0]) != 0; var random = new GameRandom(N(row[1])); now = BitConverter.UInt64BitsToDouble(ulong.Parse(row[2], CultureInfo.InvariantCulture)); var fields = new RainAviaryProcessFields(); Convert.FromHexString(row[4]).CopyTo(fields.Bytes, 0); string actual = ""; int refreshed = 0;
            body.FlyerDied(fields, random, current => { Assert.Same(fields, current); Assert.Equal(80, current.Form()); Assert.Equal(50, current.Parent()); actual = "R:" + Hex(current.Bytes); refreshed++; });
            Assert.Equal(row[5], Hex(fields.Bytes)); Assert.Equal(N(row[6]), random.State()); Assert.Equal(server ? 1 : 0, refreshed);
            if (server) { int offset = row[7].IndexOf(";R:", StringComparison.Ordinal); Assert.True(offset >= 0); Assert.Equal(row[7][(offset + 1)..], actual); } else { Assert.Equal("-", row[7]); Assert.Empty(actual); } count++;
        }
        Assert.Equal(36, count);
    }

    /// <summary>현재fort/friendly 참조와 값 복사한 뷰/함수 슬롯·비서버stage6 공통후단을 검증한다.</summary>
    [Fact]
    public void CurrentOwnersCopiedFlagsAndNonServerLaunchTailMatchSource()
    {
        var scene = new Scene("P", [1, 1, 0, 1, 1, 0, 0]); uint friendly = 0, checking = 0; var globals = scene.Globals(); globals.FriendlyFire = new(() => friendly); globals.Checking = new(() => checking); var hooks = scene.Hooks(); var body = new RawRainAviaryProcess(scene.Pool, globals, hooks);
        hooks.Allocate = null; globals.FriendlyFire = new(false); body.PostPop(50, 1); Assert.Equal(new[] { "Base,1" }, scene.Events); scene.Events.Clear(); friendly = uint.MaxValue; scene.ProcessType.Value = 59; body.PostPop(50, 1); Assert.Equal("Find,50,59", scene.Events[0]); Assert.NotNull(scene.Attached);
        scene.Events.Clear(); scene.Server = false; Put(scene.Fields.Bytes, 24, 6); Put(scene.Fields.Bytes, 40, 50); scene.Now = 200; body.Frame(scene.Fields); Assert.Equal(new[] { "Frame,50,0" }, scene.Events); Assert.Equal(5u, scene.Fields.Stage()); Assert.Equal(203, scene.Fields.Time());
        scene.Events.Clear(); Put(scene.Fields.Bytes, 24, 2); Time(scene.Fields.Bytes, 16, 0); checking = 0x80000000; globals.Checking = new(false); body.Frame(scene.Fields); Assert.Equal("Check,1,2042", scene.Events[0]);
        scene.Events.Clear(); Put(scene.Fields.Bytes, 24, 10); Time(scene.Fields.Bytes, 16, double.NaN); byte[] before = scene.Fields.Bytes.ToArray(); body.Frame(scene.Fields); Assert.Equal(before, scene.Fields.Bytes); Assert.Empty(scene.Events);
    }

    /// <summary>홈 통지의 최초dirty/시각·정수 지연과 선택누락 앞의 부분 쓰기를 보존한다.</summary>
    [Fact]
    public void HomeNoticePartialFailureAndCurrentReadOrderArePreserved()
    {
        var fields = new RainAviaryProcessFields(0xa5); fields.Bytes[8] = 0x80; Time(fields.Bytes, 16, 77);
        Assert.Throws<InvalidOperationException>(() => RawRainAviaryProcess.FlyerDiedIn(fields, null, null, null)); Assert.Equal(10u, fields.Stage()); Assert.Equal(0x84, fields.Dirty()); Assert.Equal(77, fields.Time());
        fields.Bytes[8] = 0x80; int reads = 0; var random = new GameRandom(1);
        RawRainAviaryProcess.FlyerDiedIn(fields, () => { reads++; if (reads == 1) { fields.Bytes[8] = 0x20; Put(fields.Bytes, 24, 5); return 100; } return 200; }, () => { Assert.Equal(0x86, fields.Dirty()); Assert.Equal(100, fields.Time()); fields.Bytes[8] = 0x40; return random; }, current => Assert.Same(fields, current));
        Assert.Equal(2, reads); Assert.Equal(210, fields.Time()); Assert.Equal(5u, fields.Stage()); Assert.Equal(0x40, fields.Dirty()); Assert.Equal(65542u, random.State());
        fields.Bytes[8] = 0x80; Assert.Throws<InvalidOperationException>(() => RawRainAviaryProcess.FlyerDiedIn(fields, () => 50, null, null)); Assert.Equal(0x86, fields.Dirty()); Assert.Equal(10u, fields.Stage()); Assert.Equal(50, fields.Time());
        Assert.Throws<InvalidOperationException>(() => RawRainAviaryProcess.FlyerDiedIn(fields, () => 100, () => random, null)); Assert.Equal(10u, fields.Stage()); Assert.InRange(fields.Time(), 110, 114);
    }

    /// <summary>Rain 고유 padding/lowWORD식별자/raw시각과 짧은force 버퍼의 무변경을 확인한다.</summary>
    [Fact]
    public void RainWriterAndInitializationPreservePaddingAndRawBits()
    {
        var scene = new Scene("C", [0, 1, 165]); scene.Body.Initialize(scene.Fields); Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(44))); Assert.Equal(0xa5a5a5a5u, scene.Fields.Stage()); Put(scene.Fields.Bytes, 36, 0xabcd004d); Put(scene.Fields.Bytes, 40, 0xdcba0032); BinaryPrimitives.WriteUInt64LittleEndian(scene.Fields.Bytes.AsSpan(16), 0x7ff8000012345678); byte[] before = scene.Fields.Bytes.ToArray();
        // 최대25BYTE보다 짧은force 버퍼는 어떤 원문 쓰기도 허용하지 않는다.
        for (int size = 0; size < 25; size++)
        {
            byte[] small = Enumerable.Repeat((byte)0xa7, size).ToArray(); Assert.Throws<ArgumentException>(() => RawRainAviaryProcess.WriteProcess(scene.Fields, small, true, true)); Assert.Equal(before, scene.Fields.Bytes); Assert.All(small, b => Assert.Equal(0xa7, b));
        }
        byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); Assert.Equal(25, RawRainAviaryProcess.WriteProcess(scene.Fields, packet, true, true)); Assert.Equal(77u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4))); Assert.Equal(50u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(9))); Assert.Equal(0xa7, packet[25]); var received = new RainAviaryProcessFields(0x5a); Assert.Equal(50u, RawAviaryProcessData.Read(received.Bytes, packet)); Assert.Equal(before.AsSpan(16, 12).ToArray(), received.Bytes.AsSpan(16, 12).ToArray());
    }

    /// <summary>실제typed Host의 같은 flags8 예약/Kernel/40삭제와 부모체인/깊이 복귀를 검증한다.</summary>
    [Fact]
    public void ActualHostOwnsRainReservationKernelWriterAndDeletion()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 실제부모50까지 같은Client 번호를 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 143; var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); byte[] packet = new byte[64]; int sends = 0;
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new() { WriteData = (process, changed) => Assert.IsType<RainAviaryProcess>(process).WritePacket(packet, changed), Transmit = length => { Assert.Equal(25, length); sends++; } });
        RainAviaryProcess? process = null; var caller = new RainAviaryProcessFields(0xa5); RawRainAviaryProcess body = null!; var globals = new RainAviaryProcessGlobals(() => false, () => true, () => 0, () => 100, new() { Value = 60 }, new() { Value = 144 });
        body = new(pool, globals, new()
        {
            FindProcess = (_, _) => 0, Allocate = size => { Assert.Equal(48, size); return caller; }, Damageable = (_, _) => { }, SetFrame = (sid, frame) => BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(sid)[36..], frame), Sound = (_, _, name) => Assert.Equal("manOWarBuild.wav", name),
            Attach = (fields, parent, type, flags) => { Assert.NotNull(fields); Assert.NotSame(caller, fields); Assert.Equal(8u, flags); process = new(body, fields); Assert.Same(process, host.Attach(process, type, parent, flags)); },
        });
        try
        {
            body.PostPop(50, 1); Assert.NotNull(process); int form = process.Form(); Assert.Equal(1, sends); Assert.Equal(60, pool.Slot(form)[10]); Assert.Equal(form, process.Fields.Form()); Assert.Equal(50, process.Fields.Parent()); Assert.Equal(0, process.Fields.Dirty()); Assert.Equal(0xa5, caller.Dirty()); kernel.RunFrame(); Assert.Equal(2u, process.Fields.Stage());
            byte[] before = process.Fields.Bytes.ToArray(); process.Notify(1, 2, 3); Assert.Equal(before, process.Fields.Bytes); host.Kill(process, 0x40); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(50)[6..])); Assert.True((pool.Slot(form)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}

