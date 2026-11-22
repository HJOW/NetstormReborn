using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제두PE의회전/각상태발사/helper/생성접두1690관측을그대로대조한다.
namespace Netstorm.Core.Tests;

/// <summary>RawArcherAnimation.cpp의현재상태/88BYTE/효과와실제Host수명을검사한다.</summary>
public sealed class RawArcherAnimationTests
{
    /// <summary>독립입력의십진DWORD를읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문DWORD폭으로기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>기대전체이미지와같은소문자표시다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>실제Missile생성진입에서멈춘명시관측경계다.</summary>
    private sealed class PrefixStop : Exception { }
    /// <summary>원문입력만작성하고기대열을읽지않는두판본장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>독립관측의실제SID풀이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>현재HP/역할을바꾸는원문표다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재방향수/프레임수의별개표다.</summary>
        internal ArcherTypeData[] Data { get; }
        /// <summary>매행의시각/delta/역할/각상태다.</summary>
        internal ArcherAnimationState State { get; } = new();
        /// <summary>원문88BYTE와패딩을소유한다.</summary>
        internal ArcherAnimationFields Fields { get; } = new();
        /// <summary>기하helper의4BYTE반환버퍼다.</summary>
        internal byte[] Output { get; } = new byte[4];
        /// <summary>현재입력행이며0~17열만입력으로쓴다.</summary>
        internal string[] Row = [];
        /// <summary>같은효과의실제순서/인자기록이다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>현재판본의raw폭을선택한다.</summary>
        internal bool Patch { get; }
        /// <summary>독립입력SID5~70을같은할당자로확보한다.</summary>
        internal Scene(bool patch, bool server = false)
        {
            Patch = patch; OriginalEdition edition = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; Pool = new(24000, server, edition: edition); Types = Enumerable.Range(0, patch ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(edition, i) }).ToArray(); Data = Enumerable.Range(0, Types.Length).Select(_ => new ArcherTypeData()).ToArray();
            // native의명시세슬롯번호를동일순서로준비한다.
            for (int sid = 5; sid <= 70; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>입력DWORD의float비트를계산없이읽는다.</summary>
        internal float Float(int index) => BitConverter.UInt32BitsToSingle(N(Row[index]));
        /// <summary>판본/각상태/HP/방향자료를원문입력열로초기화한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(23, row.Length); Row = row; Events.Clear(); Array.Clear(Output); State.Now = 10; State.Delta = BitConverter.UInt64BitsToDouble(ulong.Parse(row[3], CultureInfo.InvariantCulture)); Data[71].Directions = (int)N(row[15]); Data[71].FrameCount = (int)N(row[16]); Types[158].Flags1 = N(row[17]) != 0 ? 0x10u : 0; uint kind = N(row[1]); bool fire = kind == 1 || kind >= 3;
            // 생성자가쓰지않는패딩도독립입력의원문패턴을유지한다.
            for (int i = 0; i < Fields.Bytes.Length; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            Put(Fields.Bytes, 0, fire ? (Patch ? 0x50b1c0u : 0x5054d0u) : (Patch ? 0x50b450u : 0x505570u)); Put(Fields.Bytes, 4, 81); Put(Fields.Bytes, 8, 50); Put(Fields.Bytes, 12, 0x42700000); Put(Fields.Bytes, 16, 0x42700000); Put(Fields.Bytes, 20, 0x426e0000); Put(Fields.Bytes, 24, 0x426e0000); Put(Fields.Bytes, 32, fire ? (Patch ? 0x50b1bcu : 0x5054ccu) : (Patch ? 0x50b448u : 0x50556cu)); Fields.Bytes[40] = 0x20; BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(42), 70); BinaryPrimitives.WriteUInt16LittleEndian(Fields.Bytes.AsSpan(44), N(row[17]) != 0 || fire ? (ushort)60 : (ushort)0); BinaryPrimitives.WriteDoubleLittleEndian(Fields.Bytes.AsSpan(48), N(row[11]) != 0 ? 10 : 10.25); Put(Fields.Bytes, 56, 0); Put(Fields.Bytes, 64, N(row[4])); Put(Fields.Bytes, 68, N(row[5])); Put(Fields.Bytes, 72, N(row[6])); Put(Fields.Bytes, 76, N(row[8])); Put(Fields.Bytes, 80, N(row[7]));
            // free/dead표적을포함한세raw슬롯을매입력마다재구성한다.
            foreach ((int sid, byte type) in new[] { (50,(byte)71),(60,(byte)158),(70,(byte)34) })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); Put(raw, 0, 0x1131a000); BinaryPrimitives.WriteUInt16LittleEndian(raw[6..], sid == 50 ? (ushort)81 : (ushort)0); raw[10] = type; raw[11] = sid == 60 ? (byte)N(row[9]) : (byte)0; raw[12] = 0xa5; raw[Patch ? 34 : 32] = 1; Put(raw, 14, BitConverter.SingleToUInt32Bits(sid == 50 ? 50.5f : 60)); Put(raw, 18, BitConverter.SingleToUInt32Bits(sid == 50 ? 50.5f : 60)); if (Patch) Put(raw, 36, N(row[2])); else raw[34] = (byte)N(row[2]);
            }
            if (kind == 0 && N(row[17]) == 0) Pool.RawSlot(50)[Patch ? 30 : 28] = (byte)N(row[12]);
        }
        /// <summary>정상/접두생성인자와실제같은하위효과를관측한다.</summary>
        internal ArcherAnimationHooks Hooks() => new()
        {
            Common = new()
            {
                Center = sid => { Events.Add($"C:{sid}"); return sid == 50 ? [50,50] : [Float(13),Float(14)]; }, Unpop = (sid, flags) => Events.Add($"U:{sid}:{flags}"), Repop = (sid, flags) => Events.Add($"P:{sid}:{flags}"), Sound = (_, name) => Events.Add("S:" + name), RandomInteger = (low, high) => { Events.Add($"N:{low}:{high}"); Events.Add("F:3"); return 3000; },
                Notify = (_, ev, info, value) => Events.Add($"M:{ev}:{info}:{value}"), Kill = (sid, flags) => Events.Add($"K:{sid}:{flags}"), TargetPoint = (firing, target) => { Events.Add($"T:{firing}:{target}"); return [52.25f,52.75f]; },
                CreateProjectile = request =>
                {
                    Events.Add(Patch ? "A:128" : "A:104"); uint[] args = [request.Firing,request.Target,BitConverter.SingleToUInt32Bits(request.Speed),request.DisplayType,request.EffectType,BitConverter.SingleToUInt32Bits(request.TargetPosition[0]),BitConverter.SingleToUInt32Bits(request.TargetPosition[1]),BitConverter.SingleToUInt32Bits(request.TargetCenter[0]),BitConverter.SingleToUInt32Bits(request.TargetCenter[1]),request.Ricochet,BitConverter.SingleToUInt32Bits(request.FiringPosition[0]),BitConverter.SingleToUInt32Bits(request.FiringPosition[1])]; Events.Add("J:" + string.Join(',', args.Select(v => v.ToString(CultureInfo.InvariantCulture)))); if (N(Row[1]) == 6) throw new PrefixStop();
                },
            },
            AimDirection = sid => Pool.Slot(sid)[Patch ? 30 : 28], NotifySurface = sid => Events.Add($"Q:{sid}"),
        };
    }
    /// <summary>두PE각845건의일곱본문/기하/helper/생성접두를모두대조한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllIndependentPeRotateFireHelpersAndConstructors(bool patch)
    {
        var scene = new Scene(patch); int count = 0; int[] kinds = new int[7];
        // 실제원본기대열18~22는결과대조에만쓰며입력/복원계산에사용하지않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunarcher-x86.tsv")))
        {
            if (!line.StartsWith(patch ? "originals\t" : "originalCD\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); scene.Prepare(row); var logic = new RawArcherAnimation(scene.Pool, scene.Types, scene.Data, scene.State, scene.Hooks()); string result = "return"; uint kind = N(row[1]);
            try
            {
                switch (kind)
                {
                    case 0: logic.Rotate(scene.Fields); break;
                    case 1: logic.Fire(scene.Fields); break;
                    case 2: result = RawArcherAnimation.NearestDirection(scene.Float(4), scene.Pool.Edition).ToString(CultureInfo.InvariantCulture); break;
                    case 3: Put(scene.Output, 0, BitConverter.SingleToUInt32Bits(RawArcherAnimation.InitialAngle(unchecked((int)N(row[2]))))); break;
                    case 4: result = RawArcherAnimation.FireFrame(scene.Float(4), N(row[8]) != 0).ToString(CultureInfo.InvariantCulture); break;
                    case 5: logic.Initialize(scene.Fields, true, 70, 60); scene.Events.Add("B:35:50:0:16"); Put(scene.Fields.Bytes, 4, 81); Put(scene.Fields.Bytes, 8, 50); logic.InitializeFire(scene.Fields); break;
                    case 6: result = logic.Launch(scene.Fields) ? "1" : "0"; break;
                    default: throw new InvalidOperationException("독립관측의미지본문");
                }
            }
            catch (PrefixStop) { result = "prefix"; }
            Assert.Equal(row[18], result); Assert.Equal(row[19], string.Join(';', scene.Events)); Assert.Equal(row[20], Hex(scene.Fields.Bytes)); Assert.Equal(row[21], Hex(scene.Pool.Slot(50)[..(patch ? RawSidPool.Stride : RawSidPool.CdStride)])); if (!patch) Assert.All(scene.Pool.Slot(50)[RawSidPool.CdStride..].ToArray(), value => Assert.Equal(0, value)); Assert.Equal(row[22], Hex(scene.Output)); count++; kinds[kind]++;
        }
        Assert.Equal(845, count); Assert.Equal(new[] { 164,576,13,33,18,33,8 }, kinds);
    }
    /// <summary>현재중심효과뒤raw좌표를재조회하고상위함수슬롯교체와현재TYPE표를구별한다.</summary>
    [Fact]
    public void TrackReadsAfterCenterAndBorrowsCurrentTypeWhileCopyingHooks()
    {
        var scene = new Scene(true); string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunarcher-x86.tsv")).First(l => l.StartsWith("originals\t", StringComparison.Ordinal)).Split('\t'); scene.Prepare(row); BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Bytes.AsSpan(44), 60); BinaryPrimitives.WriteDoubleLittleEndian(scene.Fields.Bytes.AsSpan(48), 100); int centers = 0; var hooks = new ArcherAnimationHooks { Common = new() { Center = sid => { centers++; if (sid == 60) { Put(scene.Pool.RawSlot(60), 14, 0x7fc12345); Put(scene.Pool.RawSlot(60), 18, 0x80000000); } return sid == 60 ? [60,60] : [50,50]; } } }; var logic = new RawArcherAnimation(scene.Pool, scene.Types, scene.Data, scene.State, hooks); hooks.Common.Center = _ => throw new IOException("외부슬롯교체"); logic.Rotate(scene.Fields); Assert.Equal(2, centers); Assert.Equal(0x7fc12345u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(12))); Assert.Equal(0x80000000u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(16)));
        scene.Types[158].Flags1 = 0; Assert.False(logic.Launch(scene.Fields)); scene.Types[158].Flags1 = 0x10; Assert.Throws<InvalidOperationException>(() => logic.Launch(scene.Fields)); scene.Data[71].Directions = 0; Assert.Throws<InvalidOperationException>(() => logic.Rotate(scene.Fields));
    }
    /// <summary>중심실패는추적쓰기전에발생하고비유한/범위밖기하는명시거부한다.</summary>
    [Fact]
    public void FailedCenterPreservesPreviousCoordinatesAndRejectsInvalidGeometry()
    {
        var scene = new Scene(true); string[] row = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunarcher-x86.tsv")).First(l => l.StartsWith("originals\t", StringComparison.Ordinal)).Split('\t'); scene.Prepare(row); BinaryPrimitives.WriteUInt16LittleEndian(scene.Fields.Bytes.AsSpan(44), 60); byte[] before = scene.Fields.Bytes.ToArray(); var failure = new IOException("중심실패"); var logic = new RawArcherAnimation(scene.Pool, scene.Types, scene.Data, scene.State, new() { Common = new() { Center = _ => throw failure } }); Assert.Same(failure, Assert.Throws<IOException>(() => logic.Rotate(scene.Fields))); Assert.Equal(before, scene.Fields.Bytes);
        Assert.Throws<ArgumentException>(() => RawArcherAnimation.NearestDirection(float.NaN, OriginalEdition.Patch1078)); Assert.Throws<ArgumentException>(() => RawArcherAnimation.FireFrame(float.PositiveInfinity, false)); Assert.Throws<ArgumentOutOfRangeException>(() => RawArcherAnimation.InitialAngle(33)); Assert.Throws<ArgumentOutOfRangeException>(() => RawArcherAnimation.FireFrame(-1, false));
    }
    /// <summary>실제PATCH Host가회전/발사완료통지와원문부착후출력변경·같은Kernel삭제를수행한다.</summary>
    [Fact]
    public void ActualHostInitializesAfterAttachAndClosesBothAnimationLifetimes()
    {
        var scene = new Scene(true, true); scene.Pool.RawSlot(50).Clear(); scene.Pool.RawSlot(50)[10] = 71; scene.Pool.RawSlot(50)[12] = 0xa5; scene.Pool.RawSlot(60)[10] = 158; scene.Types[71].Group = 0; scene.Types[158].Flags1 = 0x10; scene.Data[71] = new(20,40); var kernel = new Kernel(); var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types); byte[] packet = new byte[64]; var packets = new List<byte[]>(); var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new() { Now = 10 }, transmit: new() { WriteData = (process, changed) => Assert.IsType<GunProcess>(process).WritePacket(packet, changed, false), Transmit = length => packets.Add(packet.AsSpan(0, length).ToArray()) }); var gunLogic = new RawGunProcess(scene.Pool, scene.Types, new() { Now = 10, Boss = false }, new());
        try
        {
            GunProcess gun = Assert.IsType<GunProcess>(RawGunProcess.CreateGunProcess(host, gunLogic, 50)); scene.State.Now = 10; scene.State.Delta = 0; int surfaces = 0; var logic = new RawArcherAnimation(scene.Pool, scene.Types, scene.Data, scene.State, RawArcherAnimation.MakeHostHooks(host, new() { Common = new() { Center = sid => sid == 50 ? [50,50] : [50,40] }, NotifySurface = _ => { Assert.Equal(2, kernel.Size()); surfaces++; } })); Put(gun.Fields.Bytes, 28, 3);
            ArcherAnimation rotate = Assert.IsType<ArcherAnimation>(RawArcherAnimation.Create(host, logic, 2, (ushort)gun.Form(), 60)); int rotateForm = rotate.Form(); Assert.Equal(gun.Form(), rotate.Fields.GunForm()); Assert.Equal(50, rotate.Fields.Parent()); Assert.Equal(2, kernel.Size()); rotate.RunFrame(); Assert.Equal(4u, gun.Fields.Stage()); Assert.Null(host.FromForm(rotateForm)); Assert.Equal(1, kernel.Size());
            Put(gun.Fields.Bytes, 28, 6); ArcherAnimation fire = Assert.IsType<ArcherAnimation>(RawArcherAnimation.Create(host, logic, 5, (ushort)gun.Form(), 60)); int fireForm = fire.Form(); Assert.Equal(1, surfaces); Assert.Equal(0xa4, scene.Pool.Slot(50)[12]); Assert.Equal(BitConverter.SingleToUInt32Bits(360), BinaryPrimitives.ReadUInt32LittleEndian(fire.Fields.Bytes.AsSpan(64))); Put(fire.Fields.Bytes, 64, BitConverter.SingleToUInt32Bits(270)); Put(fire.Fields.Bytes, 68, 0); Put(fire.Fields.Bytes, 80, 1); fire.RunFrame(); Assert.Equal(7u, gun.Fields.Stage()); Assert.Null(host.FromForm(fireForm)); Assert.Equal(1, kernel.Size()); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth); Assert.NotEmpty(packets); host.Kill(gun, 0x40); Assert.Equal(0, kernel.Size());
        }
        finally { kernel.DrainOwnedForShutdown(); }
    }
}
