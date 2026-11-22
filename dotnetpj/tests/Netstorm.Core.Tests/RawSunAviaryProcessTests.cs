using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 독립 PE의 전체 이미지/현재 전역/효과 순서와 실제 Host 수명을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>훨리베이스1734관측과 홈 통지128관측의 원문 입력을 대조한다.</summary>
public sealed class RawSunAviaryProcessTests
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
    /// <summary>Source AviaryScene의 입력과 외부 효과만 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>다섯 관측 객체를 실제 Client 번호로 확보하는 같은 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 실제48BYTE 입력이다.</summary>
        internal SunAviaryProcessFields Fields { get; } = new(0xa5);
        /// <summary>현재 서버 전역이다.</summary>
        internal bool Server = true;
        /// <summary>현재 Editor 전역이다.</summary>
        internal uint Editor;
        /// <summary>현재 게임 시각이다.</summary>
        internal double Now = 100;
        /// <summary>현재 프로세스 역할의 같은 주소다.</summary>
        internal UIntBox ProcessType { get; } = new() { Value = 32 };
        /// <summary>현재 플라이어 역할의 같은 주소다.</summary>
        internal UIntBox FlyerType { get; } = new() { Value = 73 };
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
        internal SunAviaryProcessFields? Attached;
        /// <summary>같은 입력을 사용하는 제품 본체다.</summary>
        internal RawSunAviaryProcess Body { get; }
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
                raw[10] = sid is 50 or 51 ? (byte)72 : sid == 60 ? (byte)73 : sid == 70 ? (byte)32 : (byte)31; raw[11] = 0; Word(raw, 12, sid == 50 ? (ushort)150 : sid == 51 ? (ushort)151 : sid == 60 ? (ushort)9 : (ushort)0);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(sid + .25f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(sid + .5f)); raw[34] = sid == 50 ? (byte)1 : sid == 51 ? (byte)2 : (byte)0; Put(raw, 36, sid == 51 ? 7u : sid == 60 ? 42u : 0); raw[40] = 0;
            }
            Put(Fields.Bytes, 0, 0x509dd8); Fields.Bytes[8] = 0x10; Time(Fields.Bytes, 16, 100); Put(Fields.Bytes, 24, 1); Put(Fields.Bytes, 32, 0x509db8); Put(Fields.Bytes, 36, 77); Put(Fields.Bytes, 40, 50); Put(Fields.Bytes, 44, 0);
            if (kind == "C") { Server = input[0] != 0; Now = new[] { 0d, 100.25, -5.25 }[input[1]]; Array.Fill(Fields.Bytes, (byte)input[2]); }
            else if (kind == "P")
            {
                Server = input[0] != 0; Allocated = input[2] != 0; Mutation = input[4]; uint layout = input[3];
                if (layout == 0) Word(Pool.RawSlot(50), 6, 0); if (layout == 2) { Pool.RawSlot(70)[11] = 2; Pool.RawSlot(71)[10] = 32; } if (layout == 3) Pool.RawSlot(70)[11] = 1;
            }
            else if (kind == "F")
            {
                Server = input[0] != 0; Editor = input[1]; Pool.RawSlot(50)[40] = (byte)input[2]; Put(Fields.Bytes, 24, input[3]); Put(Pool.RawSlot(50), 36, input[4]); Put(Fields.Bytes, 44, input[5]); Time(Fields.Bytes, 16, new[] { 99d, 100, 101 }[input[6]]); Mutation = input[7];
            }
            else if (kind == "W") { Fields.Bytes[8] = (byte)input[0]; Time(Fields.Bytes, 16, 123.25); Put(Fields.Bytes, 24, 7); Put(Fields.Bytes, 44, 5); }
            Body = new(Pool, Globals(), Hooks());
        }
        /// <summary>현재 owner 참조를 빌리는 입력 구조체다.</summary>
        internal SunAviaryProcessGlobals Globals() => new(() => Server, () => Editor, () => Now, ProcessType, FlyerType);
        /// <summary>독립 관측의 외부 반환값과 변경 입력만 제공한다.</summary>
        internal SunAviaryProcessHooks Hooks()
        {
            var lookup = new RawContainedLookup(Pool, (_, _) => throw new IOException("미선택 삭제"));
            return new()
            {
                FindProcess = (sid, type) => { Events.Add($"Find,{sid},{type}"); if (Kind == "P" && Mutation == 1) Server = false; if (Kind == "P" && Mutation == 2) ProcessType.Value = 59; return (ushort)lookup.Find(sid, type); },
                Allocate = size => { Events.Add($"Allocate,{size}"); return Allocated ? new(0xa5) : null; },
                Attach = (fields, sid, type, flags) => { Events.Add($"Attach,{type},{sid},{flags},{(fields is null ? 0 : 1)}"); Attached = fields; },
                Damageable = (_, flags) => Events.Add($"Base,{flags}"), Assertion = line => Events.Add($"Assert,{line}"),
                SetFrame = (sid, frame) => { Events.Add($"Frame,{sid},{frame}"); Put(Pool.RawSlot(sid), 36, unchecked((uint)frame)); FrameCalls++; if (FrameCalls == 1 && Mutation == 1) { Put(Fields.Bytes, 40, 51); Now = 200; } if (FrameCalls == 1 && Mutation == 5) Put(Fields.Bytes, 24, 9); },
                Sound = (x, y, name) => { Assert.Equal("apStart.wav", name); Events.Add($"Sound,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}"); if (Mutation == 2) { Put(Fields.Bytes, 24, 9); Put(Fields.Bytes, 44, 10); } },
                CheckFrame = (_, frame, line) => Events.Add($"Check,{frame},{line}"),
                Create = (type, flags) => { Events.Add($"Create,{type},{flags}"); return 60; },
                SetOwner = (sid, owner) => { Events.Add($"Owner,{sid},{owner}"); Pool.RawSlot(sid)[34] = (byte)owner; if (Mutation == 3) { Put(Fields.Bytes, 40, 51); Now = 200; } },
                Pop = (sid, x, y, flags) => { Events.Add($"Pop,{sid},{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{flags}"); if (Mutation == 4) { Put(Fields.Bytes, 40, 51); Now = 200; } },
                BindFlyer = (flyer, sid) => Events.Add($"Bind,{flyer},{sid}"), RefreshForm = _ => { Events.Add("Refresh"); if (Mutation == 6) Fields.Bytes[8] = 0; },
            };
        }
        /// <summary>다섯 객체의 실제 전체 바이트와 현재 전역20BYTE다.</summary>
        internal (string Objects, string Globals) Current()
        {
            byte[] objects = new byte[250]; int offset = 0;
            // 원문 배열 순서대로 실제50BYTE를 읽는다.
            foreach (int sid in new[] { 50, 51, 60, 70, 71 }) { Pool.Slot(sid).CopyTo(objects.AsSpan(offset)); offset += 50; }
            byte[] globals = new byte[20]; Put(globals, 0, Server ? 1u : 0); Put(globals, 4, Editor); Time(globals, 8, Now); Put(globals, 16, ProcessType.Value); return (Hex(objects), Hex(globals));
        }
    }

    /// <summary>원문 C/P/F/W의 전체 이미지/객체/전역/패킷/효과 순서를 각각 전수 대조한다.</summary>
    [Theory]
    [InlineData("C", 18)] [InlineData("P", 98)] [InlineData("F", 594)] [InlineData("W", 1024)]
    public void BodiesMatchAllIndependentPeObservations(string kind, int expected)
    {
        int count = 0;
        // 비주석 머리글과 다른 범위를 건너뛰며 기대값은 원문 자료를 그대로 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/sun-aviary-x86.tsv")))
        {
            if (!line.StartsWith(kind + "\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(8, row.Length); uint[] input = row[1].Split(',').Select(N).ToArray(); var scene = new Scene(kind, input); string packet = "-", result = "-";
            if (kind == "C") scene.Body.Initialize(scene.Fields); else if (kind == "P") scene.Body.PostPop(50, input[1]); else if (kind == "F") scene.Body.Frame(scene.Fields);
            else { byte[] bytes = Enumerable.Repeat((byte)0xa7, 64).ToArray(); result = RawSunAviaryProcess.WriteProcess(scene.Fields, bytes, input[1] != 0, input[2] != 0).ToString(CultureInfo.InvariantCulture); packet = Hex(bytes); }
            Assert.Equal(row[2], kind == "P" ? scene.Attached is null ? "-" : Hex(scene.Attached.Bytes) : Hex(scene.Fields.Bytes)); var current = scene.Current(); Assert.Equal(row[3], kind is "P" or "F" ? current.Objects : "-"); Assert.Equal(row[4], current.Globals);
            Assert.Equal(row[5], packet); Assert.Equal(row[6], result); Assert.Equal(row[7], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); count++;
        }
        Assert.Equal(expected, count);
    }

    /// <summary>홈 통지의 첫 조회와 공간 효과 이후 현재 이미지 쓰기를 독립128입력과 대조한다.</summary>
    [Fact]
    public void HomeNoticeMatches128IndependentPeObservations()
    {
        int count = 0;
        // 두 정밀도 실행 결과가 같은 동결 자료의 전체 바이트를 검증한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/sun-aviary-notice-x86.tsv")))
        {
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith('#')) continue; string[] row = line.Split('\t'); Assert.Equal(9, row.Length); uint[] input = row[0].Split(',').Select(N).ToArray(); var scene = new Scene("N", input) { Server = input[0] != 0, Mutation = input[3] };
            Span<byte> raw = scene.Pool.RawSlot(50); raw.Fill(0xa5); Put(raw, 0, 0x12200000); raw[10] = 72; raw[11] = 0; Put(raw, 36, 7);
            Array.Fill(scene.Fields.Bytes, (byte)0xa5); scene.Fields.Bytes[8] = (byte)input[1]; Time(scene.Fields.Bytes, 16, 100.25); Put(scene.Fields.Bytes, 24, input[2]); Put(scene.Fields.Bytes, 32, 0x509db8); Put(scene.Fields.Bytes, 36, 70); Put(scene.Fields.Bytes, 40, 50);
            var body = new RawSunAviaryProcess(scene.Pool, scene.Globals(), new()
            {
                FindProcess = (sid, type) => { Assert.Equal(50, sid); Assert.Equal(32u, type); scene.Events.Add("Find:50:32"); if (scene.Mutation == 3) { scene.Server = false; scene.ProcessType.Value = 99; } return 70; },
                ProcessFields = sid => { Assert.Equal(70, sid); scene.Events.Add("Resolve:70"); return scene.Fields; },
                Unpop = (sid, flags) => { Assert.Equal(50, sid); Assert.Equal(0u, flags); scene.Events.Add("Unpop:50:0"); scene.Pool.RawSlot(50)[11] = 4; if (scene.Mutation == 1) { Put(scene.Pool.RawSlot(50), 36, 99); scene.Fields.Bytes[8] = 0x10; Put(scene.Fields.Bytes, 24, 5); Put(scene.Fields.Bytes, 40, 51); } },
                Repop = (sid, flags) => { Assert.Equal(50, sid); Assert.Equal(0u, flags); scene.Events.Add($"Repop:50:0:{BinaryPrimitives.ReadUInt32LittleEndian(scene.Pool.Slot(50)[36..])}"); scene.Pool.RawSlot(50)[11] = 0; if (scene.Mutation == 2) { Put(scene.Pool.RawSlot(50), 36, 77); scene.Fields.Bytes[8] = 0x40; Put(scene.Fields.Bytes, 24, 9); } },
                RefreshForm = fields => { Assert.Same(scene.Fields, fields); scene.Events.Add("Refresh"); },
            });
            body.FlyerDied(50); Assert.Equal(row[1], row[5]); Assert.Equal(row[2], row[6]); Assert.Equal(row[3], row[7]); Assert.Equal(row[4], row[8]);
            Assert.Equal(row[1], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(row[2], Hex(raw)); Assert.Equal(row[3], Hex(scene.Fields.Bytes)); Assert.Equal(row[4], $"{(scene.Server ? 1 : 0)},{scene.ProcessType.Value}"); count++;
        }
        Assert.Equal(128, count);
    }

    /// <summary>미선택 장치는 요구하지 않으며 누락/NULL 앞의 실제 부분 효과를 보존한다.</summary>
    [Fact]
    public void SelectedMissingEffectsAndNullHomeKeepSourcePrefix()
    {
        var scene = new Scene("P", [1, 1, 1, 0, 0]); SunAviaryProcessHooks hooks = scene.Hooks(); hooks.Allocate = null; var body = new RawSunAviaryProcess(scene.Pool, scene.Globals(), hooks);
        Assert.Throws<InvalidOperationException>(() => body.PostPop(50, 1)); Assert.Equal(new[] { "Find,50,32" }, scene.Events); Assert.Null(scene.Attached);
        scene.Server = false; scene.Events.Clear(); body.PostPop(50, 1); Assert.Equal(new[] { "Base,1" }, scene.Events);
        scene.Server = true; scene.Events.Clear(); hooks = new()
        {
            FindProcess = (_, _) => 0, Unpop = (_, _) => scene.Events.Add("Unpop"), Repop = (sid, _) => { Assert.Equal(27u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Pool.Slot(sid)[36..])); scene.Events.Add("Repop"); },
        };
        body = new(scene.Pool, scene.Globals(), hooks); Assert.Throws<InvalidOperationException>(() => body.FlyerDied(50)); Assert.Equal(new[] { "Unpop", "Repop" }, scene.Events);
        scene.Server = false; new RawSunAviaryProcess(scene.Pool, scene.Globals(), new()).FlyerDied(50);
        var frame = new Scene("F", [1, 0, 0, 2, 11, 0, 1, 0]); var globals = frame.Globals(); globals.Checking = new(true); hooks = frame.Hooks(); hooks.CheckFrame = null; body = new(frame.Pool, globals, hooks); byte[] before = frame.Fields.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => body.Frame(frame.Fields)); Assert.Equal(before, frame.Fields.Bytes); Assert.Empty(frame.Events);
    }

    /// <summary>고정 writer의 짧은 버퍼/lowWORD 식별자와 공통 수신의 부분 쓰기/선택 폭 순서를 검증한다.</summary>
    [Fact]
    public void CodecKeepsRawTimeLowWordIdsTailAndPartialReadEffects()
    {
        var fields = new SunAviaryProcessFields(0xa5); fields.Bytes[8] = 0x80; Put(fields.Bytes, 36, 0xabcd004d); Put(fields.Bytes, 40, 0xdcba0032); BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(16), 0x7ff8000012345678); Put(fields.Bytes, 24, uint.MaxValue); byte[] before = fields.Bytes.ToArray();
        // 최대25BYTE보다 짧은 모든 force 버퍼는 필드와 패킷을 바꾸지 않는다.
        for (int size = 0; size < 25; size++)
        {
            byte[] small = Enumerable.Repeat((byte)0xa7, size).ToArray(); Assert.Throws<ArgumentException>(() => RawSunAviaryProcess.WriteProcess(fields, small, true, true)); Assert.Equal(before, fields.Bytes); Assert.All(small, b => Assert.Equal(0xa7, b));
        }
        byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); Assert.Equal(25, RawSunAviaryProcess.WriteProcess(fields, packet, true, true)); Assert.Equal(77u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4))); Assert.Equal(50u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(9))); Assert.Equal(0xa7, packet[25]);
        var copy = new SunAviaryProcessFields(0x5a); copy.Bytes[8] = 0x80; Assert.Equal(50u, RawAviaryProcessData.Read(copy.Bytes, packet)); Assert.Equal(before.AsSpan(16, 12).ToArray(), copy.Bytes.AsSpan(16, 12).ToArray()); Assert.Equal(0x80, copy.Dirty());
        Put(packet, 21, 7); Assert.Throws<ArgumentException>(() => RawAviaryProcessData.Read(copy.Bytes, packet.AsSpan(0, 24))); Assert.Equal(0xffffffffu, copy.Stage());
        byte[] prefix = new byte[9]; Assert.Equal(0u, RawAviaryProcessData.Read([], prefix)); prefix[8] = 1; byte[] parentOnly = new byte[13]; prefix.CopyTo(parentOnly, 0); Put(parentOnly, 9, 0xfedcba98); Assert.Equal(0xfedcba98u, RawAviaryProcessData.Read([], parentOnly));
        byte[] shortFields = new byte[24]; packet[8] = 7; ArgumentException missingPacket = Assert.Throws<ArgumentException>(() => RawAviaryProcessData.Read(shortFields, packet.AsSpan(0, 24))); Assert.Contains("선택 필드", missingPacket.Message); Assert.Equal(before.AsSpan(16, 8).ToArray(), shortFields.AsSpan(16, 8).ToArray());
        ArgumentException missingObject = Assert.Throws<ArgumentException>(() => RawAviaryProcessData.Read(shortFields, packet)); Assert.Contains("객체 필드", missingObject.Message);
    }

    /// <summary>현재 전역/함수 값 복사/진단 뷰와 ordered NaN·현재시각 두 읽기를 보존한다.</summary>
    [Fact]
    public void CurrentOwnersCopiedSlotsAndOrderedTimeArePreserved()
    {
        var scene = new Scene("F", [1, 0, 0, 2, 25, 0, 1, 0]); uint checking = 0; var globals = scene.Globals(); globals.Checking = new(() => checking); var hooks = scene.Hooks(); var body = new RawSunAviaryProcess(scene.Pool, globals, hooks);
        hooks.SetFrame = null; globals.Checking = new(false); checking = 0x80000000; body.Frame(scene.Fields); Assert.Equal(new[] { "Check,26,1828", "Frame,50,26" }, scene.Events);
        Put(scene.Fields.Bytes, 24, 2); Put(scene.Pool.RawSlot(50), 36, 25); Time(scene.Fields.Bytes, 16, double.NaN); byte[] before = scene.Fields.Bytes.ToArray(); scene.Events.Clear(); body.Frame(scene.Fields); Assert.Equal(before, scene.Fields.Bytes); Assert.Empty(scene.Events);
        Put(scene.Fields.Bytes, 24, 6); scene.FlyerType.Value = 111; body.Frame(scene.Fields); Assert.Equal("Create,111,0", scene.Events[0]);
        var pool = new RawSidPool(24000, false); pool.RawSlot(50).Clear(); var fields = new SunAviaryProcessFields(); Put(fields.Bytes, 40, 50); Put(fields.Bytes, 24, 2); Time(fields.Bytes, 16, -1); int reads = 0;
        body = new(pool, new(() => true, () => 0, () => ++reads, new(), new()), new() { SetFrame = (_, _) => { } }); body.Frame(fields); Assert.Equal(3, reads); Assert.Equal(3 + 0.20000000298023224, fields.Time());
    }

    /// <summary>같은 실제 typed Host/Kernel에서 flags8 부착·상태 프레임·40삭제와 깊이 복귀를 검증한다.</summary>
    [Fact]
    public void ActualHostUsesSameReservationKernelWriterAndDeletion()
    {
        var pool = new RawSidPool(24000, true); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 부착할 부모50까지 같은 실제 Client 영역을 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 72; var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); int sends = 0; byte[] packet = new byte[64];
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new() { WriteData = (process, changed) => Assert.IsType<SunAviaryProcess>(process).WritePacket(packet, changed), Transmit = length => { Assert.Equal(25, length); sends++; } });
        SunAviaryProcess? process = null; var caller = new SunAviaryProcessFields(0xa5); RawSunAviaryProcess body = null!;
        body = new(pool, new(() => true, () => 0, () => 100, new() { Value = 32 }, new() { Value = 73 }), new()
        {
            FindProcess = (_, _) => 0, Allocate = size => { Assert.Equal(48, size); return caller; }, Damageable = (_, _) => { }, SetFrame = (sid, frame) => Put(pool.RawSlot(sid), 36, unchecked((uint)frame)), Sound = (_, _, _) => { },
            Attach = (fields, parent, type, flags) => { Assert.NotNull(fields); Assert.NotSame(caller, fields); Assert.Equal(8u, flags); process = new(body, fields); Assert.Same(process, host.Attach(process, type, parent, flags)); },
        });
        try
        {
            body.PostPop(50, 1); Assert.NotNull(process); int form = process.Form(); Assert.Equal(1, sends); Assert.Equal(32, pool.Slot(form)[10]); Assert.Equal(form, process.Fields.Form()); Assert.Equal(50, process.Fields.Parent()); Assert.Equal(0xa5, caller.Bytes[8]); Assert.Equal(0, process.Fields.Dirty()); Assert.Same(process, host.FromForm(form));
            kernel.RunFrame(); Assert.Equal(8u, process.Fields.Stage()); Assert.Equal(27u, BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(50)[36..])); byte[] before = process.Fields.Bytes.ToArray(); process.Notify(1, 2, 3); Assert.Equal(before, process.Fields.Bytes);
            host.Kill(process, 0x40); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(50)[6..])); Assert.True((pool.Slot(form)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}
