using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
// 구 근사 모델과 구별되는 실제 Host 파생이다.
using Missile = Netstorm.Core.Original.Missile;

// 두 보호 PE의 미사일 생성/이동/충돌 raw와 외부 효과 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>공통 썬 캐논/썬 아처의 독립2184관측과 현재 값·수명 계약 검사다.</summary>
public sealed class RawMissileTests
{
    /// <summary>부호 비트도 포함한 입력을 하위DWORD로 읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>관측의 float 비트를 그대로 읽는다.</summary>
    private static float V(string value) => BitConverter.UInt32BitsToSingle(N(value));
    /// <summary>관측의 float 비트를 십진수로 표시한다.</summary>
    private static uint B(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>원문 DWORD 폭만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>원문 전체 이미지를 소문자hex로 표시한다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>원문 입력과 명시 외부 효과만 준비하며 기대열을 읽지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 판본과 같은 물리 슬롯이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>원문 span처럼 빌리는 현재 TYPE 자료다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재 사거리/방향/음향 자료다.</summary>
        internal MissileTypeData[] Data { get; }
        /// <summary>원문 현재 시각과 역할이다.</summary>
        internal MissileState State { get; } = new();
        /// <summary>판본 밖과 padding도 보존하는 원문 이미지다.</summary>
        internal MissileFields Fields { get; } = new();
        /// <summary>현재 독립 입력 행이다.</summary>
        internal string[] Row = [];
        /// <summary>관측 대상 외부 효과의 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>PATCH 필드 배치를 선택한다.</summary>
        internal bool Patch { get; }
        /// <summary>아처71/표시83의 별도 원문 경로다.</summary>
        internal bool Archer { get; }
        /// <summary>원문5~100 자산 번호를 그대로 확보한다.</summary>
        internal Scene(bool patch, bool archer, bool server = false)
        {
            Patch = patch; Archer = archer; Pool = new(24000, server, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); Types = Enumerable.Range(0, patch ? 188 : 171).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, i) }).ToArray(); Data = Types.Select(_ => new MissileTypeData()).ToArray();
            // 세 관측 슬롯을 원문과 같은 번호로 확보한다.
            for (int sid = 5; sid <= 100; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>독립 숫자 입력만으로 raw/시간/표를 매번 다시 구성한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(Archer ? 20 : 17, row.Length); Row = row; Events.Clear(); State.Now = N(row[1]) == 0 ? 10.25 : 10 + N(row[10]) * .5;
            // 원문 생성자가 쓰지 않은 바이트도 같은 패턴이다.
            for (int i = 0; i < 128; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            // 세 자산의 상태/소유자/좌표를 독립 입력으로 다시 쓴다.
            foreach ((int sid, int type) in new[] { (50, Archer ? 71 : 128), (60, 71), (90, Archer ? 83 : 80) })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); Put(raw, 0, 0x11319000); raw[10] = (byte)type; raw[11] = sid == 60 ? (byte)N(row[11]) : (byte)0; raw[Patch ? 34 : 32] = 1; Put(raw, 14, 0x42480000); Put(raw, 18, 0x42480000);
            }
            int firing = Archer ? 71 : 128; Types[firing].Group = Archer ? 0 : 1; Types[firing].HotspotX = int.Parse(row[6], CultureInfo.InvariantCulture); Types[firing].HotspotY = int.Parse(row[7], CultureInfo.InvariantCulture); Data[firing].Range = Archer ? V(row[14]) : 40;
            if (Archer) { Data[71].FiringSound = N(row[15]); Data[83].FrameCount = 1; Data[83].ImpactSound = N(row[16]); }
            if (N(row[1]) == 0) return;
            byte[] fields = Fields.Bytes; Put(fields, 0, Patch ? 0x50d630u : 0x501ca0u); Put(fields, 4, 81); Put(fields, 8, 90); Put(fields, 12, 0); Put(fields, 16, 0); Put(fields, 20, 50); Put(fields, 24, 60); Put(fields, 28, N(row[13])); Put(fields, Patch ? 48 : 32, Archer ? 0x420c0000u : 0x41a00000u); Put(fields, Patch ? 52 : 36, 0); Put(fields, Patch ? 56 : 40, N(row[2])); Put(fields, Patch ? 60 : 44, N(row[3])); Put(fields, Patch ? 64 : 48, N(row[8])); Put(fields, Patch ? 68 : 52, N(row[9])); Put(fields, Patch ? 72 : 56, N(row[8])); Put(fields, Patch ? 76 : 60, N(row[9]));
            // 독립 Frame 입력의 초기 사각형과 두 double 시각이다.
            for (int at = Patch ? 88 : 64; at < (Patch ? 104 : 80); at += 4) Put(fields, at, 0xc61c4000);
            BinaryPrimitives.WriteDoubleLittleEndian(fields.AsSpan(Patch ? 104 : 80), 12); BinaryPrimitives.WriteDoubleLittleEndian(fields.AsSpan(Patch ? 112 : 88), 10); Put(fields, Patch ? 120 : 96, 0x42480000); Put(fields, Patch ? 124 : 100, N(row[12]) != 0 ? B(89) : 0xc61c4000);
            if (Patch) { Put(fields, 32, 0); Put(fields, 36, 0); Put(fields, 40, 0); Put(fields, 44, 0); }
        }
        /// <summary>원문 관측과 같은 인수/순서만 기록하는 외부 월드 경계다.</summary>
        internal MissileHooks Hooks() => new()
        {
            Muzzle = _ => { Events.Add("M:50"); return [V(Row[2]), V(Row[3])]; },
            Hotspot = _ => { if (Patch) Events.Add("H:50"); return [int.Parse(Row[4], CultureInfo.InvariantCulture), int.Parse(Row[5], CultureInfo.InvariantCulture)]; },
            TargetDimensions = _ => { Events.Add("D:60"); return [2, 3]; },
            RayEnd = (from, to, firing) => { Events.Add($"R:{B(from[0])},{B(from[1])},{B(to[0])},{B(to[1])},{firing}"); return [V(Row[8]), V(Row[9])]; },
            Create = (type, flags) => { Events.Add($"A:{type}:{flags}"); return 90; },
            Owner = (_, owner) => { Pool.AllocatedBytes(90)[Patch ? 34 : 32] = owner; Events.Add($"O:90:{owner}"); },
            Pop = (_, x, y, flags) => { Put(Pool.AllocatedBytes(90), 14, B(x)); Put(Pool.AllocatedBytes(90), 18, B(y)); Events.Add($"P:90:{B(x)}:{B(y)}:{flags}"); },
            Unpop = (_, flags) => Events.Add($"U:90:{flags}"), FlightEffect = (type, asset) => Events.Add($"E:{type}:{asset}:0"),
            FiringSound = (x, y, token) => Events.Add($"S:{B(x)}:{B(y)}:{token}:0:0"),
            Center = _ => { Events.Add("C:50"); return [50, 50]; },
            TargetBounds = (_, point) => { Events.Add($"T:{B(point[0])}:{B(point[1])}"); return [49, 89, 51, 91]; },
            Hit = (firing, target) => Events.Add($"G:{firing}:{target}"), HitEffect = (x, y) => Events.Add($"H:{B(x)}:{B(y)}"), Refresh = _ => Events.Add("Q:50"),
            Impact = (x, y, mode, payload) => Events.Add($"I:{B(x)}:{B(y)}:{mode}:{B(payload)}"), ImpactSound = (x, y, token) => Events.Add($"S:{B(x)}:{B(y)}:{token}:0:0"), Destroy = (_, flags) => Events.Add($"X:90:{flags}"),
        };
        /// <summary>현재 행의 생성 요청이며 기대열은 참조하지 않는다.</summary>
        internal GunProjectileRequest Request() => new() { Firing = 50, Target = 60, TargetPosition = [100, 90], TargetCenter = N(Row[13]) != 0 ? [100, 90] : [-10000, -10000], Speed = Archer ? 35 : 20, DisplayType = Archer ? 83u : 80u, EffectType = Archer ? 0u : 11u };
    }
    /// <summary>두 판본의 전체 raw/자산/사건 순서를 원본2184행과 비교한다.</summary>
    [Theory]
    [InlineData(true, false, 148)] [InlineData(false, false, 148)] [InlineData(true, true, 944)] [InlineData(false, true, 944)]
    public void MatchesIndependentGunAndArcherMissiles(bool patch, bool archer, int expected)
    {
        var scene = new Scene(patch, archer); int count = 0;
        // 결과열은 실제 실행이 끝난 뒤 비교에만 쓴다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, archer ? "X86/archermissile-x86.tsv" : "X86/gunmissile-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); if (row[0] != (patch ? "originals" : "originalCD")) continue; scene.Prepare(row); var logic = new RawMissile(scene.Pool, scene.Types, scene.Data, scene.State, scene.Hooks());
            if (N(row[1]) == 0) { ushort asset = logic.Construct(scene.Fields, scene.Request()); scene.Events.Add("B:28:90:0:16"); Put(scene.Fields.Bytes, 4, 81); Put(scene.Fields.Bytes, 8, asset); } else logic.Frame(scene.Fields);
            int at = archer ? 17 : 14; Assert.Equal(row[at], string.Join(';', scene.Events)); Assert.Equal(row[at + 1], Hex(scene.Fields.Bytes.AsSpan(0, patch ? 128 : 104))); Assert.Equal(row[at + 2], Hex(scene.Pool.Slot(90)[..(patch ? 50 : 36)]));
            if (!patch) Assert.Equal(Enumerable.Range(104, 24).Select(i => unchecked((byte)(i * 13 + 7))), scene.Fields.Bytes.Skip(104)); count++;
        }
        Assert.Equal(expected, count);
    }
    /// <summary>선택된 실제 비행 효과 누락은 raw/공간 변경 전에 거부한다.</summary>
    [Fact]
    public void MissingFlightEffectRejectsBeforeAnyRawMutation()
    {
        var scene = new Scene(true, false); scene.Prepare(File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunmissile-x86.tsv")).First(line => line.Length != 0 && !line.StartsWith('#')).Split('\t')); MissileHooks hooks = scene.Hooks(); hooks.FlightEffect = null; var logic = new RawMissile(scene.Pool, scene.Types, scene.Data, scene.State, hooks); byte[] before = scene.Fields.Bytes.ToArray(); Assert.Throws<InvalidOperationException>(() => logic.Construct(scene.Fields, scene.Request())); Assert.Equal(before, scene.Fields.Bytes); Assert.Empty(scene.Events);
    }
    /// <summary>실제 공장 자산과 같은 Pool/Kernel의 flags10 부착 및 즉시 수명 종료를 확인한다.</summary>
    [Fact]
    public void ActualFactoryAttachesToSameHostAndPreservesAssetOnKill()
    {
        var scene = new Scene(true, false); scene.Prepare(File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunmissile-x86.tsv")).First(line => line.Length != 0 && !line.StartsWith('#')).Split('\t')); var kernel = new Kernel(); var unpop = new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]); var factory = new RawSquidFactory(scene.Pool, scene.Types, unpop: unpop); var destroy = new RawSquidDestroy(scene.Pool, unpop, scene.Types); var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new() { Now = 10 }, asset: new() { Selected = () => 0 }); MissileHooks hooks = scene.Hooks(); ushort created = 0; int pops = 0;
        hooks.Owner = (sid, owner) => { created = sid; scene.Pool.AllocatedBytes(sid)[34] = owner; }; hooks.Pop = (sid, x, y, flags) => { Assert.Equal(created, sid); Assert.Equal(0u, flags); Put(scene.Pool.AllocatedBytes(sid), 14, B(x)); Put(scene.Pool.AllocatedBytes(sid), 18, B(y)); pops++; }; hooks = RawMissile.MakeFactoryHooks(scene.Pool, factory, hooks); var logic = new RawMissile(scene.Pool, scene.Types, scene.Data, scene.State, hooks); GunProjectileRequest request = scene.Request(); request.EffectType = 0;
        try
        {
            Missile process = Assert.IsType<Missile>(RawMissile.Create(host, logic, request)); int form = process.Form(); Assert.Equal(101, created); Assert.Equal(1, pops); Assert.Equal(1, kernel.Size()); Assert.Same(process, host.FromForm(form)); Assert.Equal(created, process.Parent()); Assert.Equal((ushort)form, process.Fields.Form()); Assert.Equal(created, process.Fields.Parent()); Assert.Equal((ushort)50, process.Fields.Firing()); Assert.Equal((ushort)60, process.Fields.Target()); Assert.Equal(80, scene.Pool.Slot(created)[10]); Assert.Equal(1, scene.Pool.Slot(created)[34]); Assert.Equal(form, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(created)[6..])); host.Kill(process, 0x10); Assert.Equal(0, kernel.Size()); Assert.Null(host.FromForm(form)); Assert.Equal(0, scene.Pool.Slot(created)[11] & 3); Assert.Equal(0u, destroy.PreDepth); Assert.Equal(0u, destroy.PostDepth);
        }
        finally { kernel.DrainOwnedForShutdown(); }
        var other = new RawSidPool(24000); Assert.Throws<ArgumentException>(() => RawMissile.MakeFactoryHooks(other, factory, hooks));
    }
    /// <summary>표/전역은 현재값을 빌리고 함수 선언은 복사하며 반환 좌표는 값으로 보존한다.</summary>
    [Fact]
    public void ConstructorKeepsValueCoordinatesAndBorrowsCurrentTypeAndRequest()
    {
        var scene = new Scene(true, false); scene.Prepare(File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunmissile-x86.tsv")).First(line => line.Length != 0 && !line.StartsWith('#')).Split('\t')); GunProjectileRequest request = scene.Request(); MissileHooks hooks = scene.Hooks(); float[] shared = [50, 50]; int[] hotspot = [0, 0]; hooks.Muzzle = _ => shared; hooks.Hotspot = _ => { shared[0] = 200; scene.Types[128].HotspotX = 16; request.TargetPosition = [120, 100]; return hotspot; }; hooks.TargetDimensions = _ => { hotspot[0] = 900; request.TargetPosition = [122, 102]; return [2, 3]; }; hooks.RayEnd = (from, to, firing) => { Assert.Equal(49, from[0]); Assert.Equal(50, from[1]); Assert.Equal(121, to[0]); Assert.Equal(100.5f, to[1]); Assert.Equal((ushort)50, firing); from[0] = 201; return [100, 90]; }; var logic = new RawMissile(scene.Pool, scene.Types, scene.Data, scene.State, hooks); hooks.Muzzle = _ => throw new InvalidOperationException("생성 뒤 외부 함수 교체는 원문 함수값에 영향 없음"); Assert.Equal((ushort)90, logic.Construct(scene.Fields, request)); Assert.Equal(49, BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(56)))); Assert.Equal(50, BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(60)))); Assert.Equal(122, BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(scene.Fields.Bytes.AsSpan(64))));
    }
}
