using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 두 실제PE의2560관측을 입력으로 재생하고 기대 raw/효과/프레임을 그대로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>RawGunAnimation.cpp의 프레임/launch접두·실제Host와 codec 실패 순서를 검증한다.</summary>
public sealed class RawGunAnimationTests
{
    /// <summary>독립 입력의 십진DWORD를 읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>주변패딩을보존하고DWORD만쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>기대값과같은 소문자 전체raw표시다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>실제 Missile 생성자 진입에서 멈춘 독립 관측 경계다.</summary>
    private sealed class MissilePrefix : Exception { }
    /// <summary>기대 열을 읽지 않고 독립 입력만 준비하는 같은 두 판본 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>원문 SID를 확보한 실제 풀이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>부모/표적의 HP/역할 입력이다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>매 입력의 현재 시각/역할이다.</summary>
        internal GunAnimationState State { get; } = new();
        /// <summary>원문64BYTE와모든패딩이다.</summary>
        internal GunAnimationFields Fields { get; } = new();
        /// <summary>실제 효과의 순서/인자 기록이다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>현재 중심 효과의 명시 반환좌표다.</summary>
        internal float[] Point = [0, 0];
        /// <summary>현재 판본의 프레임 폭이다.</summary>
        internal bool Patch { get; }
        /// <summary>원문 client5~100을 동일 순서로 확보한다.</summary>
        internal Scene(bool patch, bool server = false)
        {
            Patch = patch; OriginalEdition edition = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; Pool = new(24000, server, edition: edition); Types = Enumerable.Range(0, patch ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(edition, i) }).ToArray();
            // 독립 관측에사용된 명시SID를 실제할당자로 확보한다.
            for (int sid = 5; sid <= 100; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>kind/frame/direction/target상태/기한만 읽고 출력열은사용하지않는다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(13, row.Length); Events.Clear(); State.Now = 10; uint kind = N(row[1]); int index = kind == 3 ? 1 : (int)kind;
            // 미작성 패딩은native 입력의 바이트패턴을그대로쓴다.
            for (int i = 0; i < Fields.Bytes.Length; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            uint[] outer = Patch ? [0x50b260,0x50b318,0x50b2c8] : [0x505520,0x505480,0x505660], inner = Patch ? [0x50b258,0x50b310,0x50b2c4] : [0x505518,0x505478,0x505658];
            Put(Fields.Bytes, 0, outer[index]); Put(Fields.Bytes, 4, 81); Put(Fields.Bytes, 8, 50); Put(Fields.Bytes, 12, 0x42700000); Put(Fields.Bytes, 16, 0x42700000); Put(Fields.Bytes, 20, 0x426e0000); Put(Fields.Bytes, 24, 0x426e0000); Put(Fields.Bytes, 32, inner[index]); Fields.Bytes[40] = (byte)N(row[8]); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42), 70); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44), 60); BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48), N(row[7]) != 0 ? 10 : 10.25); Put(Fields.Bytes, 56, N(row[4]));
            Span<byte> parent = Pool.RawSlot(50); parent.Clear(); Put(parent, 0, 0x1131a000); parent[10] = 128; Put(parent, 14, 0x424a0000); Put(parent, 18, 0x424a0000); parent[Patch ? 34 : 32] = 1; if (Patch) Put(parent, 36, N(row[2])); else parent[34] = (byte)N(row[2]);
            Span<byte> target = Pool.RawSlot(60); target.Clear(); target[10] = 71; target[11] = (byte)N(row[5]); Put(target, 14, 0x42700000); Put(target, 18, 0x42700000); Types[71].Flags1 = N(row[6]) != 0 ? 0x10u : 0;
            float[][] directions = [[50,40],[60,50],[50,60],[40,50]]; Point = directions[N(row[3]) % 4];
        }
        /// <summary>외부효과를명시적으로관측하며 Missile생성접두에서만중단한다.</summary>
        internal GunAnimationHooks Hooks() => new()
        {
            Center = sid => { Events.Add($"C:{sid}"); return sid == 50 ? [50,50] : Point; }, TargetPoint = (firing, target) => { Events.Add($"T:{firing}:{target}"); return [52.25f,52.75f]; },
            Unpop = (sid, flags) => Events.Add($"U:{sid}:{flags}"), Repop = (sid, flags) => Events.Add($"P:{sid}:{flags}"), RandomInteger = (low, high) => { Events.Add($"N:{low}:{high}"); return 3000; },
            Sound = (_, name) => { Events.Add("F:3"); Events.Add("S:" + name); }, Notify = (_, ev, info, value) => Events.Add($"M:{ev}:{info}:{value}"), Kill = (sid, flags) => Events.Add($"K:{sid}:{flags}"),
            CreateProjectile = request =>
            {
                Events.Add(Patch ? "A:128" : "A:104"); uint[] args = [request.Firing,request.Target,BitConverter.SingleToUInt32Bits(request.Speed),request.DisplayType,request.EffectType,BitConverter.SingleToUInt32Bits(request.TargetPosition[0]),BitConverter.SingleToUInt32Bits(request.TargetPosition[1]),BitConverter.SingleToUInt32Bits(request.TargetCenter[0]),BitConverter.SingleToUInt32Bits(request.TargetCenter[1]),request.Ricochet,BitConverter.SingleToUInt32Bits(request.FiringPosition[0]),BitConverter.SingleToUInt32Bits(request.FiringPosition[1])]; Events.Add("J:" + string.Join(',', args.Select(value => value.ToString(CultureInfo.InvariantCulture)))); throw new MissilePrefix();
            },
        };
    }
    /// <summary>두 PE의 회전224/발사768/원위치280/launch8을 각각 전부 비교한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllIndependentPeFramesAndLaunchPrefixes(bool patch)
    {
        var scene = new Scene(patch); int count = 0; int[] kinds = new int[4];
        // 원문 입력만 재생하고 정상/접두상태·효과·전체raw·부모프레임 기대열을 직접 대조한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunanimation-x86.tsv")))
        {
            if (!line.StartsWith(patch ? "originals\t" : "originalCD\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); scene.Prepare(row); var logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, scene.Hooks()); bool prefix = false;
            try { if (N(row[1]) == 3) logic.Launch(scene.Fields); else logic.Frame(scene.Fields, (GunAnimationKind)N(row[1])); } catch (MissilePrefix) { prefix = true; }
            Assert.Equal(row[9], prefix ? "prefix" : "return"); Assert.Equal(row[10], string.Join(';', scene.Events)); Assert.Equal(row[11], Hex(scene.Fields.Bytes)); Assert.Equal(N(row[12]), patch ? BinaryPrimitives.ReadUInt32LittleEndian(scene.Pool.Slot(50)[36..]) : scene.Pool.Slot(50)[34]); kinds[N(row[1])]++; count++;
        }
        Assert.Equal(1280, count); Assert.Equal(new[] { 224,768,280,8 }, kinds);
    }
    /// <summary>뒤packet 부족은앞선WORD저장을유지하며 NULL객체는미선택저장에서만자연반환한다.</summary>
    [Fact]
    public void ReceivePreservesSelectedPartialWritesAndNullDecoder()
    {
        byte[] packet = new byte[25]; packet[8] = 15; Put(packet, 9, 0xfedcba98); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(13), 0x9876); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(15), 0x5432); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(17), 0x7ff0000000000001); var fields = new GunAnimationFields(0xa5);
        Assert.Throws<ArgumentException>(() => RawGunAnimation.ReadProcess(fields.Bytes, packet.AsSpan(0, 17))); Assert.Equal(0x9876, fields.GunForm()); Assert.Equal(0x5432, fields.Target()); Assert.Equal(0xa5a5a5a5a5a5a5a5ul, BinaryPrimitives.ReadUInt64LittleEndian(fields.Bytes.AsSpan(48))); Assert.Equal(0xa5, fields.Bytes[40]);
        Assert.Equal(0xfedcba98u, RawGunAnimation.ReadProcess(fields.Bytes, packet)); Assert.Equal(0x7ff0000000000001ul, BinaryPrimitives.ReadUInt64LittleEndian(fields.Bytes.AsSpan(48)));
        packet[8] = 0xf0; Assert.Equal(0u, RawGunAnimation.ReadProcess(Span<byte>.Empty, packet.AsSpan(0, 9))); packet[8] = 1; Assert.Equal(0xfedcba98u, RawGunAnimation.ReadProcess(Span<byte>.Empty, packet.AsSpan(0, 13))); packet[8] = 2; Assert.Throws<InvalidOperationException>(() => RawGunAnimation.ReadProcess(Span<byte>.Empty, packet));
    }
    /// <summary>수신기본생성의네지정필드만바뀌고미작성좌표/WORD/패딩은그대로다.</summary>
    [Theory]
    [InlineData(GunAnimationReceiveKind.SunFire, 0x50b318u, 0x50b310u)]
    [InlineData(GunAnimationReceiveKind.SunRotate, 0x50b260u, 0x50b258u)]
    [InlineData(GunAnimationReceiveKind.ArcherFire, 0x50b1c0u, 0x50b1bcu)]
    [InlineData(GunAnimationReceiveKind.ArcherRotate, 0x50b450u, 0x50b448u)]
    public void ReceivedConstructorsPreserveEveryUnwrittenByte(GunAnimationReceiveKind kind, uint outer, uint inner)
    {
        byte[] raw = Enumerable.Repeat((byte)0xa5, 88).ToArray(); byte[] expected = raw.ToArray(); Put(expected, 0, outer); Put(expected, 4, 0); Put(expected, 8, 0); Put(expected, 32, inner); Put(expected, 48, 0); Put(expected, 52, 0); expected[40] = 0xff;
        if (kind == GunAnimationReceiveKind.ArcherFire) { Put(expected, 64, 0); Put(expected, 68, 0x43340000); Put(expected, 72, 0x43340000); Put(expected, 76, 0); Put(expected, 80, 0); }
        RawGunAnimation.InitializeReceived(raw, kind); Assert.Equal(expected, raw); byte[] shortRaw = Enumerable.Repeat((byte)0xa5, kind == GunAnimationReceiveKind.ArcherFire ? 87 : 55).ToArray(); byte[] before = shortRaw.ToArray(); Assert.Throws<ArgumentException>(() => RawGunAnimation.InitializeReceived(shortRaw, kind)); Assert.Equal(before, shortRaw);
    }
    /// <summary>writer는전체DWORD 식별자/두WORD/원문64비트를쓰고tail·실패전이미지를보존한다.</summary>
    [Fact]
    public void WriterPreservesFullIdentityBitsTailAndAtomicBounds()
    {
        var fields = new GunAnimationFields(0xa5); Put(fields.Bytes, 0, 0x50b318); Put(fields.Bytes, 4, 0x12345678); Put(fields.Bytes, 8, 0xfedcba98); BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(48), 0x8000000000000000); byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(), before = fields.Bytes.ToArray();
        Assert.Throws<ArgumentException>(() => RawGunAnimation.WriteProcess(fields.Bytes, packet.AsSpan(0, 24), true, true, 0x73, 0x81)); Assert.Equal(before, fields.Bytes); Assert.All(packet, value => Assert.Equal(0xa7, value));
        Assert.Equal(25, RawGunAnimation.WriteProcess(fields.Bytes, packet, true, true, 0x73, 0x81)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xff, packet[8]); Assert.Equal(0x12345678u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4))); Assert.Equal(0xfedcba98u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(9))); Assert.Equal(0x8000000000000000ul, BinaryPrimitives.ReadUInt64LittleEndian(packet.AsSpan(17))); Assert.Equal(0xa7, packet[25]); Assert.Equal(0, fields.Bytes[40]);
        fields.Bytes[40] = 1; Put(fields.Bytes, 8, 0); Assert.Equal(9, RawGunAnimation.WritePacket(fields, packet, true)); Assert.Equal(1, packet[8]); Assert.Throws<InvalidOperationException>(() => RawGunAnimation.UsesFirePacket(new byte[4]));
    }
    /// <summary>효과부족/현재역할과함수값복사·부분효과순서를명시적으로검사한다.</summary>
    [Fact]
    public void CurrentRoleAndEffectFailuresPreserveSourceOrder()
    {
        var scene = new Scene(true); string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunanimation-x86.tsv")).First(l => l.StartsWith("originals\t", StringComparison.Ordinal)).Split('\t'); scene.Prepare(row); var role = new UIntBox { Value = 129 }; scene.State.CurrentSunCannonType = role; var hooks = scene.Hooks(); var logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, hooks);
        Assert.Throws<InvalidOperationException>(() => logic.Frame(scene.Fields, GunAnimationKind.SunRotate)); role.Value = 128; BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48), 10); hooks.Center = _ => throw new IOException("외부슬롯교체"); logic.Frame(scene.Fields, GunAnimationKind.SunRotate); Assert.Contains("C:60", scene.Events);
        scene.Prepare(row); scene.Pool.RawSlot(60)[11] = 2; Assert.False(new RawGunAnimation(scene.Pool, scene.Types, scene.State, new()).Launch(scene.Fields)); scene.Pool.RawSlot(60)[11] = 0; Assert.Throws<InvalidOperationException>(() => new RawGunAnimation(scene.Pool, scene.Types, scene.State, new()).Launch(scene.Fields));
    }
    /// <summary>실제PATCH Host의Gun통지/flags10 애니메이션삭제와깊이/Kernel소유권을닫는다.</summary>
    [Fact]
    public void ActualHostNotifiesGunAndDeletesSameAnimation()
    {
        var scene = new Scene(true, true); scene.Pool.RawSlot(50).Clear(); scene.Pool.RawSlot(50)[10] = 128; scene.Types[128].Group = 1; var kernel = new Kernel(); var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types); byte[] buffer = new byte[64]; var packets = new List<byte[]>(); var rotate = new UIntBox { Value = 36 }; var clock = new SquidProcessState { Now = 10, CurrentRotateAnimationType = rotate };
        var transmit = new ProcessFormTransmission { WriteData = (process, changed) => process switch { GunProcess gun => gun.WritePacket(buffer, changed, false), GunAnimation animation => animation.WritePacket(buffer, changed), _ => throw new InvalidOperationException("시험의미지파생") }, Transmit = length => packets.Add(buffer.AsSpan(0, length).ToArray()) };
        var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, clock, transmit: transmit); var gunLogic = new RawGunProcess(scene.Pool, scene.Types, new() { Now = 10, Boss = false }, new());
        try
        {
            GunProcess gun = Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host, gunLogic, 50)); Put(gun.Fields.Bytes, 28, 3); var state = new GunAnimationState { Now = 10 }; var logic = new RawGunAnimation(scene.Pool, scene.Types, state, RawGunAnimation.MakeHostHooks(host, new() { Center = sid => sid == 50 ? [50,50] : [50,60] })); rotate.Value = 44;
            GunAnimation animation = Assert.IsType<GunAnimation>(RawGunAnimation.Create(host, logic, 2, (ushort)gun.Form(), 60)); int form = animation.Form(); Assert.Equal(44, scene.Pool.Slot(form)[10]); Assert.Equal(2, kernel.Size()); Assert.Equal(gun.Form(), animation.Fields.GunForm()); Assert.Equal(50, animation.Fields.Parent()); Assert.Equal(10, animation.Fields.Time()); animation.RunFrame(); Assert.Equal(4u, gun.Fields.Stage()); Assert.Null(host.FromForm(form)); Assert.Equal(1, kernel.Size()); Assert.True((scene.Pool.Slot(form)[11] & 3) != 0); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth); Assert.NotEmpty(packets); host.Kill(gun, 0x40); Assert.Equal(0, kernel.Size());
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }

    /// <summary>중심효과실패는앞선표적위치를보존하고공간재등록미설치는Unpop전에거부한다.</summary>
    [Fact]
    public void SelectedEffectFailurePreservesEarlierWritesAndPairedGuards()
    {
        var scene = new Scene(true); string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunanimation-x86.tsv")).First(l => l.StartsWith("originals\t", StringComparison.Ordinal)).Split('\t'); scene.Prepare(row); Put(scene.Pool.RawSlot(60), 14, 0x7fc12345); Put(scene.Pool.RawSlot(60), 18, 0x80000000); var failure = new IOException("현재중심실패");
        var logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, new() { Center = _ => throw failure }); Assert.Same(failure, Assert.Throws<IOException>(() => logic.Frame(scene.Fields, GunAnimationKind.SunFire))); Assert.Equal(0x7fc12345u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(12))); Assert.Equal(0x80000000u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(16))); Assert.Equal(0x426e0000u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(20)));
        scene.Pool.RawSlot(60)[11] = 2; BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48), 10); int unpopped = 0; logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, new() { Unpop = (_, _) => unpopped++ }); Assert.Throws<InvalidOperationException>(() => logic.Frame(scene.Fields, GunAnimationKind.SunFire)); Assert.Equal(0, unpopped); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Pool.Slot(50)[36..]));
    }

    /// <summary>외부가재사용하는중심/코너배열을Source std::array의반환값처럼복사한다.</summary>
    [Fact]
    public void CoordinatesKeepValueCopiesAcrossLaterCallbacks()
    {
        var scene = new Scene(true); string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunanimation-x86.tsv")).First(l => l.StartsWith("originals\t", StringComparison.Ordinal)).Split('\t'); scene.Prepare(row); BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48), 10); float[] shared = [50,60]; int finished = 0;
        var logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, new()
        {
            Center = sid => { if (sid == 50) shared[1] = 50; return shared; }, Notify = (_, ev, info, value) => { Assert.Equal(1u, ev); Assert.Equal(3u, info); Assert.Equal(4u, value); finished++; }, Kill = (_, flags) => Assert.Equal(0x10u, flags),
        });
        logic.Frame(scene.Fields, GunAnimationKind.SunRotate); Assert.Equal(1, finished); float[] corner = [52,53];
        logic = new RawGunAnimation(scene.Pool, scene.Types, scene.State, new() { TargetPoint = (_, _) => corner, CreateProjectile = request => { corner[0] = 99; Assert.Equal(52, request.TargetCenter[0]); } }); Assert.True(logic.Launch(scene.Fields));
    }
}
