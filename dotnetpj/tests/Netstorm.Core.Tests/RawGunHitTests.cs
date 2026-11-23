using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 두 PE의 압축 총 시각/DPS/가상 피해와 현재 상태 복귀 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립2058행과 선택된 실제 피해/시간 통지 경계 검사다.</summary>
public sealed class RawGunHitTests
{
    /// <summary>원문 입력의 하위DWORD다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>현재 원문 DWORD만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>전체 바이트의 원문 소문자hex다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>실제 원문 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows() => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/gunhit-x86.tsv")).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>입력만으로 실제 raw/TYPE/DPS와 명시 피해/통지를 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 원문50/60의 실제 풀이다.</summary>
        internal RawSidPool Pool { get; }
        /// <summary>현재group/비행 표다.</summary>
        internal OriginalTypeRecord[] Types { get; }
        /// <summary>현재 실제 DPS 자료다.</summary>
        internal GunHitType[] Data { get; }
        /// <summary>외부 반환 double 버퍼다.</summary>
        internal byte[] Output { get; } = new byte[8];
        /// <summary>실제 피해/통지 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>현재 판본의owner/슬롯 폭이다.</summary>
        internal bool Patch { get; }
        /// <summary>현재 독립 double 시각이다.</summary>
        internal double Now;
        /// <summary>동일 관측 번호를 실제 할당한다.</summary>
        internal Scene(bool patch)
        {
            Patch = patch; Pool = new(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072); Types = Enumerable.Range(0, patch ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); Data = Types.Select(_ => new GunHitType()).ToArray();
            // 반복 행에서 같은50/60을 사용한다.
            for (int sid = 5; sid <= 60; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>원문 숫자 입력만으로 두 슬롯과 외부 버퍼를 재초기화한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(12, row.Length); Events.Clear(); Now = BitConverter.Int64BitsToDouble(unchecked((long)ulong.Parse(row[3], CultureInfo.InvariantCulture)));
            // helper가 쓰지 않은 반환8BYTE도 원문 패턴이다.
            for (int i = 0; i < Output.Length; i++) Output[i] = unchecked((byte)(i * 7 + 3));
            // free 상태와 padding도 매 행 독립적으로 다시 쓴다.
            foreach ((int sid, int type) in new[] { (50, 128), (60, 71) })
            {
                Span<byte> raw = Pool.RawSlot(sid);
                // CD의 논리 호스트 바깥14BYTE도 제품이 건드리지 않아야 한다.
                for (int i = 0; i < raw.Length; i++) raw[i] = unchecked((byte)(i * 11 + 5));
                Put(raw, 0, sid == 50 ? 0x11318000u : 0x11318100u); raw[10] = (byte)type; raw[11] = sid == 60 ? (byte)N(row[6]) : (byte)0; raw[Patch ? 34 : 32] = 1; if (sid == 50) BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], (ushort)N(row[2]));
            }
            Types[128].Group = 1; Types[128].Flags2 = 0; Types[71].Flags2 = N(row[8]); Data[128].DamagePerSecond = BitConverter.UInt32BitsToSingle(N(row[4])); Data[128].SpecialFlyerDamage = N(row[7]) != 0; Data[128].FlyerDamagePerSecond = 16;
        }
        /// <summary>실제 외부 가상 피해와 시간 쓰기 뒤 통지의 명시 관측이다.</summary>
        internal GunHitHooks Hooks() => new() { NotifySurface = firing => Events.Add($"N:{firing}"), Damage = (target, damage, owner, firing) => Events.Add($"D:{target}:{damage}:{owner}:{firing}") };
    }
    /// <summary>두 판본의 전체 시간/감김/DPS/피해2058행을 허용오차 없이 비교한다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void MatchesAllNativePackedTimeAndDamage(bool patch)
    {
        var scene = new Scene(patch); int count = 0;
        // 기대열은 실제 본문 실행 뒤에만 비교한다.
        foreach (string[] row in Rows())
        {
            if (row[0] != (patch ? "originals" : "originalCD")) continue; scene.Prepare(row); var logic = new RawGunHit(scene.Pool, scene.Types, scene.Data, scene.Hooks());
            switch (N(row[1])) { case 0: BinaryPrimitives.WriteDoubleLittleEndian(scene.Output, logic.Time(50)); break; case 1: case 4: logic.SetTime(50, scene.Now); break; case 2: logic.Hit(50, (ushort)N(row[5]), scene.Now); break; case 3: logic.Apply(50, (ushort)N(row[5]), 17); break; default: throw new InvalidOperationException("미지 원문 총 피해 본문"); }
            Assert.Equal(row[9], string.Join(';', scene.Events)); Assert.Equal(row[10], Hex(scene.Pool.Slot(50)[..(patch ? 50 : 36)])); Assert.Equal(row[11], Hex(scene.Output)); if (!patch) Assert.Equal(Enumerable.Range(36, 14).Select(i => unchecked((byte)(i * 11 + 5))), scene.Pool.Slot(50)[36..].ToArray()); count++;
        }
        Assert.Equal(1029, count);
    }
    /// <summary>전용 source+c0 입구는 generic 범위/target0/free/dead 선관문보다 먼저다.</summary>
    [Fact]
    public void OverrideRunsBeforeGenericGuardsAndFunctionsAreCopied()
    {
        var scene = new Scene(true); scene.Prepare(Rows().First()); bool handled = true; int calls = 0; GunHitHooks hooks = scene.Hooks(); hooks.ApplyOverride = (firing, target, damage) => { calls++; return handled; }; var logic = new RawGunHit(scene.Pool, scene.Types, scene.Data, hooks); hooks.ApplyOverride = null; logic.Apply(65535, 65535, 17); handled = false; logic.Apply(65535, 0, 17); scene.Pool.RawSlot(60)[11] = 2; logic.Apply(65535, 60, 17); scene.Pool.RawSlot(60)[11] = 0; Assert.Throws<ArgumentOutOfRangeException>(() => logic.Apply(65535, 60, 17)); Assert.Equal(4, calls); Assert.Empty(scene.Events);
    }
    /// <summary>선택 피해 누락은 시각을 갱신하지 않고 후반 통지 누락은 앞선 피해를 보존한다.</summary>
    [Fact]
    public void SelectedMissingDamageAndLateNotifyRetainSourceEffects()
    {
        var scene = new Scene(true); scene.Prepare(Rows().First()); scene.Pool.RawSlot(60)[11] = 0; ushort before = BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(50)[12..]); var missing = new RawGunHit(scene.Pool, scene.Types, scene.Data, new()); Assert.Throws<InvalidOperationException>(() => missing.Apply(50, 60, 17)); Assert.Equal(before, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(50)[12..])); BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(50)[12..], 0); scene.Data[128].DamagePerSecond = 10; var hooks = scene.Hooks(); hooks.NotifySurface = null; hooks.Damage = (_, amount, _, _) => { Assert.Equal(10, amount); scene.Pool.RawSlot(50)[12] = 1; scene.Events.Add("D"); }; var logic = new RawGunHit(scene.Pool, scene.Types, scene.Data, hooks); Assert.Throws<InvalidOperationException>(() => logic.Hit(50, 60, 1)); Assert.Equal(new[] { "D" }, scene.Events); Assert.Equal((ushort)1, BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(50)[12..]));
    }
    /// <summary>미사일이 같은 현재 시각/DPS/소유자와 bit0을 보존하는 실제 피해/갱신을 호출한다.</summary>
    [Fact]
    public void MissileBindingsReadCurrentTimeAndDpsAfterDamage()
    {
        var scene = new Scene(true); scene.Prepare(Rows().First()); scene.Pool.RawSlot(60)[11] = 0; BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(50)[12..], 1); scene.Data[128].DamagePerSecond = 10; var state = new MissileState { Now = 1 }; GunHitHooks effects = scene.Hooks(); effects.Damage = (target, amount, owner, firing) => { scene.Events.Add($"D:{target}:{amount}:{owner}:{firing}"); scene.Pool.RawSlot(50)[12] &= 0xfe; scene.Data[128].DamagePerSecond = 20; }; var hit = new RawGunHit(scene.Pool, scene.Types, scene.Data, effects); MissileHooks hooks = RawGunHit.MakeMissileHooks(hit, state, new()); hooks.Hit!(50, 60); Assert.Equal(1, hit.Time(50)); state.Now = 2; scene.Pool.RawSlot(50)[34] = 3; hooks.Hit(50, 60); state.Now = 4.25; hooks.Refresh!(50); Assert.Equal(4.25, hit.Time(50)); Assert.Equal(new[] { "D:60:10:1:50", "N:50", "D:60:20:3:50", "N:50", "N:50" }, scene.Events); Assert.Equal(0, scene.Pool.Slot(50)[12] & 1);
    }
}
