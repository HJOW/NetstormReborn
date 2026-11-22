using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 보호 원문720관측과 같은 실제 Host 예약/공통 후처리 수명을 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>맨 오 워/훨리기그의 전체 이미지·객체·현재 전역·호출 순서를 독립 비교한다.</summary>
public sealed class RawFlyerPostPopTests
{
    /// <summary>독립 입력의 signed 표기도 낮은DWORD 비트로 보존한다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, System.Globalization.CultureInfo.InvariantCulture));
    /// <summary>현재 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>현재 WORD만 쓴다.</summary>
    private static void Word(Span<byte> bytes, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(bytes[offset..], value);
    /// <summary>전체 이미지를 원문의 소문자hex로 만든다.</summary>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>두 native 하네스의 입력을 공유하며 기대 출력은 계산하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>원문 관측과 같은 실제 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>입력의 불변 flags1/genus다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>입력의 speed/프레임/소리 보충 값이다.</summary>
        internal FlyerPostPopType[] Details { get; } = new FlyerPostPopType[188];
        /// <summary>같은 실제 공통 생성자 이미지다.</summary>
        internal FlyerProcessFields Fields { get; } = new(0xa5);
        /// <summary>실제 원문 훨리기그 장면인지 구별한다.</summary>
        internal bool Sun { get; }
        /// <summary>효과 뒤 현재 서버 owner다.</summary>
        internal bool Server;
        /// <summary>프로세스 확보의 명시 반환값이다.</summary>
        internal bool Allocated;
        /// <summary>소리 확보의 명시 반환값이다.</summary>
        internal bool SoundAllocated;
        /// <summary>현재 단일 시각이다.</summary>
        internal double Now = 100.25;
        /// <summary>현재 프로세스 TYPE 주소다.</summary>
        internal UIntBox ProcessType { get; } = new() { Value = 31 };
        /// <summary>하네스가 지정한 외부 효과 변경 번호다.</summary>
        internal uint Mutation;
        /// <summary>첫/후속 Pop 효과를 구별하는 입력 상태다.</summary>
        internal int PopCount;
        /// <summary>실제 호출 순서의 기록이다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>실제 부착 전 전체 이미지다.</summary>
        internal string AttachImage = "-";
        /// <summary>같은 이미지에 기본 생성자를 적용한다.</summary>
        internal RawFlyerProcess Logic { get; }
        /// <summary>관측 대상까지 실제 확보하고 전체 객체/TYPE 입력을 준비한다.</summary>
        internal Scene(bool sun, uint[] input)
        {
            Sun = sun; Server = input[0] != 0; Allocated = input[2] != 0; SoundAllocated = sun && input[3] != 0; Mutation = input[7];
            // 독립 관측의71번까지 같은 Client 번호를 확보한다.
            for (int sid = 5; sid <= 71; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 관측의 객체250BYTE 전체와 연결/좌표/그림자를 같은 입력으로 만든다.
            foreach (int sid in new[] { 50, 60, 61, 70, 71 })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Fill(0xa5); Put(raw, 0, sun ? 0x50a1e8u : 0x50a388u); Word(raw, 4, !sun && sid == 70 ? (ushort)71 : (ushort)0);
                Word(raw, 6, !sun && sid == 50 ? (ushort)70 : (ushort)0); Word(raw, 8, 0); raw[10] = sun ? (byte)73 : sid < 70 ? (byte)144 : (byte)31; raw[11] = 0; Word(raw, 12, sun ? (ushort)60 : (ushort)17);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(50.25f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(50.5f)); Put(raw, 36, 2); raw[40] = 0;
            }
            Put(Pool.RawSlot(50), 36, input[sun ? 4 : 3]); Word(Pool.RawSlot(50), 8, input[6] != 0 ? (ushort)60 : (ushort)0);
            if (sun)
            {
                Put(Pool.RawSlot(60), 0, 0x509930); Pool.RawSlot(60)[10] = 72;
                // 서로 다른 두 TYPE의 snapshot 차이를 원문 입력으로 구별한다.
                foreach (int type in new[] { 73, 143 })
                {
                    Types[type].Flags1 = input[5] != 0 ? 0x400000u : 0; Details[type] = new() { Speed = BitConverter.SingleToUInt32Bits(type == 73 ? 0.75f : 1.25f), Frames = type == 73 ? 12 : 18, ImpactSound = type == 73 ? 901u : 911u, ActiveSound = type == 73 ? 902u : 912u };
                }
            }
            else
            {
                Fields.SunTail.AsSpan().Clear();
                // 맨 오 워 관측의 세 TYPE/프레임/속도 입력이다.
                foreach (int type in new[] { 31, 143, 144 })
                {
                    Types[type].Flags1 = 0x400000; Types[type].Flags2 = 0x100000; Details[type] = new() { Speed = BitConverter.SingleToUInt32Bits(type == 143 ? 1.25f : 0.75f), Frames = type == 144 ? unchecked((int)input[4]) : 12 };
                }
                Types[144].Flags1 = input[5] != 0 ? 0x400000u : 0;
            }
            Logic = new(Pool, Types, new(() => Server, () => Now, ProcessType), new()
            {
                Assertion = line => { Events.Add($"Assert,{line}"); if (!Sun && Mutation == 1) { Now = 200; ProcessType.Value = 30; Fields.Bytes[8] = 0x10; } },
                Attach = (fields, parent, type, flags) =>
                {
                    Assert.Same(Fields, fields); AttachImage = Image(); Events.Add($"Attach,{type},{parent},{flags}");
                    if (Mutation == 2) { Put(Fields.Bytes, 84, 77); Pool.RawSlot(50)[10] = 143; Word(Pool.RawSlot(50), 12, 13); Now = 200; }
                },
            });
        }
        /// <summary>기본112BYTE와 Sun꼬리를 원문 장면의 폭으로 읽는다.</summary>
        internal string Image() => Hex(Fields.Bytes) + (Sun ? Hex(Fields.SunTail) : "");
        /// <summary>각 현재 경계의 인수/순서와 명시된 입력 변경만 공급한다.</summary>
        internal FlyerPostPopHooks Hooks() => new()
        {
            AllocateRainProcess = () => Allocate(112), AllocateSunProcess = () => Allocate(136),
            CreateRainProcess = (parent, home, speed, parameter) => { Logic.Construct(Fields, parent, home, speed, parameter); RawFlyerProcess.SpecializeRain(Fields); },
            CreateSunProcess = (parent, home, speed, parameter) => { Logic.Construct(Fields, parent, home, speed, parameter); RawFlyerProcess.SpecializeSun(Fields); },
            AllocateSound = () => { Events.Add("Allocate,40"); return SoundAllocated; }, CreateSound = (parent, sound, alternate, flags) => Events.Add($"Sound,{parent},{sound},{alternate},{flags}"),
            Repop = (sid, flags) => Events.Add($"Repop,{sid},{flags}"),
            Create = (type, flags) => { Events.Add($"Create,{type},{flags}"); if (Mutation == 3) { Put(Pool.RawSlot(50), 36, 7); Put(Pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(90.25f)); } return 61; },
            Pop = (sid, x, y, flags) => { Events.Add($"Pop,{sid},{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{flags}"); PopCount++; if (Mutation == 4 && PopCount == 1) { Put(Pool.RawSlot(50), 36, 9); Put(Pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(70.25f)); } },
            Unpop = (sid, flags) => { Events.Add($"Unpop,{sid},{flags}"); if (Mutation == 6) { Put(Pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(100.25f)); Put(Pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(110.25f)); } },
            InitializeUnit = sid =>
            {
                ulong bits = BitConverter.DoubleToUInt64Bits(Now); Events.Add($"SetTime,{sid},{(uint)bits},{(uint)(bits >> 32)}");
                if (Mutation == 5) { Word(Pool.RawSlot(50), 8, 60); Put(Pool.RawSlot(50), 36, 11); Put(Pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(80.25f)); }
            },
            ShadowOffset = _ => Sun ? (-0.6f, 3.5f) : (0.5f, 0.5f), Damageable = (_, flags) => Events.Add($"Base,{flags}"),
        };
        /// <summary>실제 확보 반환과 그 효과 뒤 current 조회를 구별하는 명시 입력이다.</summary>
        private bool Allocate(int bytes)
        {
            Events.Add($"Allocate,{bytes}"); if (Mutation == 7) { Pool.RawSlot(50)[10] = 143; Word(Pool.RawSlot(50), 12, 13); } if (Mutation == 8) { Server = false; ProcessType.Value = 30; } return Allocated;
        }
        /// <summary>전체 관측 객체와 현재 원문16BYTE 전역을 읽는다.</summary>
        internal (string Objects, string Globals) Current()
        {
            byte[] objects = new byte[250]; int offset = 0;
            // 원문 배열 순서대로 현재 객체를 합친다.
            foreach (int sid in new[] { 50, 60, 61, 70, 71 }) { Pool.Slot(sid).CopyTo(objects.AsSpan(offset)); offset += 50; }
            byte[] globals = new byte[16]; Put(globals, 0, Server ? 1u : 0); BinaryPrimitives.WriteDoubleLittleEndian(globals.AsSpan(4), Now); Put(globals, 12, ProcessType.Value); return (Hex(objects), Hex(globals));
        }
    }

    /// <summary>두 보호 원문의 전체P329/Sun391을 결과 재생성 없이 비교한다.</summary>
    [Theory]
    [InlineData(false, 329)] [InlineData(true, 391)]
    public void WholePostPopMatchesIndependentPeImages(bool sun, int expectedCount)
    {
        int count = 0; string file = sun ? "sun-flyer-postpop-x86.tsv" : "flyer-postpop-x86.tsv";
        // 머리글/다른 진입점을 제외하고 해당 원문 정상 입력을 전수 재생한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", file)))
        {
            if (line.Length == 0 || line.StartsWith('#') || (!sun && !line.StartsWith("P\t", StringComparison.Ordinal))) continue;
            string[] row = line.Split('\t'); uint[] input = row[sun ? 0 : 1].Split(',').Select(N).ToArray(); var scene = new Scene(sun, input);
            new RawFlyerPostPop(scene.Pool, scene.Types, scene.Details, () => scene.Server, scene.Hooks()).PostPop(50, input[1]); var current = scene.Current(); string trace = scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events);
            if (sun)
            {
                Assert.Equal(6, row.Length); Assert.Equal(row[1], scene.Image()); Assert.Equal(row[2], current.Objects); Assert.Equal(row[3], current.Globals); Assert.Equal(row[4], scene.AttachImage); Assert.Equal(row[5], trace);
            }
            else
            {
                Assert.Equal(9, row.Length); Assert.Equal(row[2], scene.AttachImage == "-" ? "-" : scene.Image()); Assert.Equal(row[3], current.Objects); Assert.Equal(row[4], current.Globals); Assert.Equal("-", row[5]); Assert.Equal("-", row[6]); Assert.Equal(row[7], scene.AttachImage); Assert.Equal(row[8], trace);
            }
            count++;
        }
        Assert.Equal(expectedCount, count);
    }

    /// <summary>선택 누락의 앞선 생성/전문화 효과와 TYPE/details/함수 값 복사를 보존한다.</summary>
    [Fact]
    public void MissingSelectedEffectPreservesCompletedPrefixAndCopiedInputs()
    {
        var scene = new Scene(false, [1, 1, 1, 2, 12, 1, 1, 0]); FlyerPostPopHooks hooks = scene.Hooks(); hooks.Create = null;
        var body = new RawFlyerPostPop(scene.Pool, scene.Types, scene.Details, () => scene.Server, hooks); hooks.Create = (_, _) => throw new IOException("복사 뒤 함수 변경"); scene.Types[144].Flags1 = 0; scene.Details[144] = new();
        Assert.Throws<InvalidOperationException>(() => body.PostPop(50, 1)); Assert.Equal(new[] { "Allocate,112", "Attach,31,50,8" }, scene.Events); Assert.Equal(0x509f60u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes)); Assert.NotEqual("-", scene.AttachImage);
        Put(scene.Pool.RawSlot(50), 0, 0x1234); Assert.False(body.Handles(50)); Assert.Throws<ArgumentException>(() => body.PostPop(50, 0));
    }

    /// <summary>공통 가상 분배는 같은 풀만 허용하고 기본 장부/깊이를 한 번 처리하며 대여를 반납한다.</summary>
    [Fact]
    public void CommonDispatcherOwnsOneBaseCallAndReleasesFlyerBinding()
    {
        var pool = new RawSidPool(24000, false); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var state = new RawPostPopState { Suppressed = true }; var common = new RawPostPop(pool, types, state); int calls = 0;
        var body = new RawFlyerPostPop(pool, types, new FlyerPostPopType[188], () => false, new() { Damageable = (sid, flags) => { calls++; common.PostPop(sid, flags); } }); common.SetFlyerPostPop(body);
        pool.RawSlot(50).Clear(); pool.RawSlot(50)[10] = 144;
        // 실제 등록된 세 VT만 전체 몸체로 분배하고 기본 공통 깊이는 한 번 복귀한다.
        foreach (uint table in new uint[] { 0x50a388, 0x50a1e8, 0x50a050 }) { Put(pool.RawSlot(50), 0, table); Assert.True(common.HandlesDerived(50)); common.Activate(50, 0); Assert.Equal(0u, state.Depth); }
        Assert.Equal(3, calls); var foreign = new RawFlyerPostPop(new(24000), types, new FlyerPostPopType[188], () => true, new()); Assert.Throws<ArgumentException>(() => common.SetFlyerPostPop(foreign));
        Assert.True(common.HandlesFlyer(50)); common.ReleaseRuntimeBindings(); Assert.False(common.HandlesFlyer(50)); Assert.False(common.HandlesDerived(50));
    }

    /// <summary>Rain 할당 예약의 같은 객체/이미지를 실제 flags8 Host에 넘긴 뒤 전용 표를 쓴다.</summary>
    [Fact]
    public void RainPostPopConsumesSameActualFlyerReservationBeforeSpecialization()
    {
        var pool = new RawSidPool(24000); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 실제 부모와 홈 입력까지 같은 Client 번호를 확보한다.
        for (int sid = 5; sid <= 60; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); Put(pool.RawSlot(50), 0, 0x50a388); pool.RawSlot(50)[10] = 144; Word(pool.RawSlot(50), 12, 60);
        var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); byte[] packet = new byte[64]; int writes = 0, allocations = 0, bases = 0;
        var host = new SquidProcessHost(pool, types, kernel, destroy, new(), transmit: new()
        {
            WriteData = (process, changed) => { var current = Assert.IsType<FlyerProcess>(process); Assert.Equal(0x509cc8u, BinaryPrimitives.ReadUInt32LittleEndian(current.Fields.Bytes)); writes++; return current.WritePacket(packet, changed); }, Transmit = length => Assert.Equal(45, length),
        });
        FlyerProcess? pending = null, live = null;
        var logic = new RawFlyerProcess(pool, types, new(() => true, () => 10, new() { Value = 31 }), new()
        {
            Attach = (fields, parent, type, flags) => { Assert.NotNull(pending); FlyerProcess current = pending; Assert.Same(current.Fields, fields); pending = null; live = current; Assert.Same(current, host.Attach(current, type, parent, flags)); },
        });
        var body = new RawFlyerPostPop(pool, types, new FlyerPostPopType[188], () => true, new()
        {
            AllocateRainProcess = () => { Assert.Null(pending); allocations++; pending = new(logic, new FlyerProcessFields(0xa5)); return true; },
            CreateRainProcess = (parent, home, speed, parameter) => { Assert.NotNull(pending); FlyerProcess current = pending; logic.Construct(current.Fields, parent, home, speed, parameter); RawFlyerProcess.SpecializeRain(current.Fields); },
            InitializeUnit = _ => { }, Damageable = (_, _) => bases++,
        });
        try
        {
            body.PostPop(50, 1); Assert.Null(pending); Assert.NotNull(live); Assert.Equal(1, allocations); Assert.Equal(1, writes); Assert.Equal(1, bases); Assert.Equal(1, kernel.Size());
            Assert.Same(live, host.FromForm(live.Form())); Assert.Equal(60, live.Fields.Home()); Assert.Equal(50, live.Parent()); Assert.Equal(0x509f60u, BinaryPrimitives.ReadUInt32LittleEndian(live.Fields.Bytes)); Assert.Equal(0, live.Fields.Bytes[8]);
        }
        finally { pending?.Dispose(); kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>Wind 최초 정기는 확보한 같은 Regular를 실제 Host에 부착하며 이미 있으면 다시 만들지 않는다.</summary>
    [Fact]
    public void WindFirstPopAttachesSameReservedRegularOnce()
    {
        var pool = new RawSidPool(24000); var types = Enumerable.Range(0, 188).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToArray();
        // 실제 부모50까지 같은 Client 번호를 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        pool.RawSlot(50).Clear(); Put(pool.RawSlot(50), 0, 0x50a050); pool.RawSlot(50)[10] = 138; var kernel = new Kernel(); var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new RawSquidHash(), new byte[65536]), types); var host = new SquidProcessHost(pool, types, kernel, destroy, new());
        RegularProcess? pending = null; int allocations = 0, bases = 0;
        var body = new RawFlyerPostPop(pool, types, new FlyerPostPopType[188], () => false, new()
        {
            HasRegular = (sid, ev) => host.FindEvent(sid, ev) is not null, AllocateWindRegular = () => { allocations++; pending = new(0, 0.25f); return true; },
            CreateWindRegular = (sid, ev, period) => { Assert.Equal(0u, ev); Assert.Equal(0.25f, period); Assert.NotNull(pending); Assert.Same(pending, host.AttachRegular(pending, sid, false)); },
            InitializeUnit = _ => { }, Damageable = (_, _) => bases++,
        });
        try
        {
            body.PostPop(50, 1); RegularProcess process = Assert.IsType<RegularProcess>(host.FindEvent(50, 0)); Assert.Same(pending, process); Assert.Equal(0.25f, process.Payload()); Assert.Equal(1, kernel.Size()); Assert.Equal(46, pool.Slot(process.Form())[10]);
            body.PostPop(50, 1); Assert.Equal(1, allocations); Assert.Equal(2, bases); Assert.Equal(1, kernel.Size());
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}
