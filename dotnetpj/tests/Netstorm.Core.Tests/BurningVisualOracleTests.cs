using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 생성/입자/연소점과 현재TYPE HP의 독립 관찰만 기대값으로 사용한다.
namespace Netstorm.Core.Tests;

/// <summary>양판본 생성256·입자564·source652·현재HP/정수5652입력을 대조한다.</summary>
public sealed partial class BurningVisualOracleTests
{
    /// <summary>선택 판본을 원본 이름에서 구한다.</summary>
    private static OriginalEdition Edition(string name) => name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
    /// <summary>현재 입자를 padding없이 여섯DWORD/24바이트로 보관한다.</summary>
    private static byte[] ParticleBytes(BurningVisualFields fields)
    {
        byte[] bytes = new byte[fields.Particles.Length * 24];
        // source포인터의 비영만0/1로 정규화하고 다른float비트는 그대로 쓴다.
        for (int i = 0; i < fields.Particles.Length; i++)
        {
            BurningParticle p = fields.Particles[i]; Span<byte> raw = bytes.AsSpan(i * 24, 24);
            BinaryPrimitives.WriteSingleLittleEndian(raw, p.X); BinaryPrimitives.WriteSingleLittleEndian(raw[4..], p.Y);
            BinaryPrimitives.WriteSingleLittleEndian(raw[8..], p.VelocityX); BinaryPrimitives.WriteSingleLittleEndian(raw[12..], p.VelocityY);
            BinaryPrimitives.WriteSingleLittleEndian(raw[16..], p.Life); BinaryPrimitives.WriteUInt32LittleEndian(raw[20..], p.Source ? 1u : 0u);
        }
        return bytes;
    }
    /// <summary>원문 빈 바이트배열 표기를 유지한다.</summary>
    private static string Hex(byte[] bytes) => bytes.Length == 0 ? "-" : Convert.ToHexStringLower(bytes);
    /// <summary>원문 단정도 비트를 읽는다.</summary>
    private static float F(uint value) => BitConverter.UInt32BitsToSingle(value);

    /// <summary>두 판본의 warm팔레트 생성자를 전체등록/의미필드/입자로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void WarmConstructorMatchesOriginal(string edition)
    {
        string[][] rows = X86Fixture.Read("burningprocess-constructor", edition); Assert.Equal(128, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var pool = new RawSidPool(24000, true, edition: Edition(edition)); int depth = edition == "originals" ? 35 : 33;
            pool.RawSlot(0)[depth] = unchecked((byte)X86Fixture.Int(row[5])); pool.RawSlot(50)[depth] = unchecked((byte)X86Fixture.Int(row[4])); var events = new List<string>();
            uint[][] patterns = [[], [0], [111, 0, 222], [111, 222]];
            var registry = new RawRenderRegistry(pool, new()
            {
                Reserve = amount => { events.Add($"N:{amount}"); return true; }, ReleaseArray = () => events.Add("L:registry"),
                Attach = (fields, type, parent, flags) => { events.Add($"B:{type}:{parent}:0:{flags}"); fields.Form = 83; fields.Parent = parent; },
            }, patterns[X86Fixture.Int(row[3])]);
            var state = new BurningVisualState { Palette = 1 }; var visual = new RawBurningVisual(pool, registry, state, new() { Reserve = amount => { events.Add($"N:{amount}"); return true; } });
            var fields = new BurningVisualFields(); visual.Initialize(fields, 777, (ushort)X86Fixture.UInt(row[1]), X86Fixture.UInt(row[2])); RenderProcessFields r = fields.Registration;
            ulong last = BitConverter.DoubleToUInt64Bits(fields.LastTime), elapsed = BitConverter.DoubleToUInt64Bits(fields.Elapsed);
            uint[] values = [unchecked((uint)r.Form), unchecked((uint)r.Parent), BitConverter.SingleToUInt32Bits(r.Left), BitConverter.SingleToUInt32Bits(r.Top), BitConverter.SingleToUInt32Bits(r.Right), BitConverter.SingleToUInt32Bits(r.Bottom), unchecked((uint)r.Slot), unchecked((uint)r.ZOrder), fields.Capacity, fields.Total,
                (uint)last, (uint)(last >> 32), (uint)elapsed, (uint)(elapsed >> 32), fields.Palette, fields.Sound, fields.Stage, fields.Image.Pixels.Length == 0 ? 0u : 1u,
                unchecked((uint)fields.Image.Width), unchecked((uint)fields.Image.Height), unchecked((uint)fields.Image.HotspotX), unchecked((uint)fields.Image.HotspotY), (uint)fields.Victim, fields.Mode, fields.Rows.Length == 0 ? 0u : 1u, unchecked((uint)fields.Selected), fields.Off ? 1u : 0u];
            byte[] particles = ParticleBytes(fields);
            Assert.Equal(row[7], string.Join(',', values)); Assert.Equal(row[8], string.Join(',', registry.Slots.ToArray())); Assert.Equal(row[9], string.Join(';', events));
            Assert.Equal(X86Fixture.UInt(row[10]), X86Fixture.Adler32(particles)); Assert.Equal(X86Fixture.UInt(row[11]), (uint)particles.Sum(b => (int)b)); return true;
        });
    }

    /// <summary>Append/source만 실행하며 생성장치 선택은 명시 실패시키는 실제 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 풀이다.</summary>
        public RawSidPool Pool { get; }
        /// <summary>판본의 현재TYPE표다.</summary>
        public OriginalTypeRecord[] Types { get; }
        /// <summary>실제 공유 난수다.</summary>
        public GameRandom Random { get; } = new();
        /// <summary>현재 약화/역할/진단 상태다.</summary>
        public BurningSourcesState State { get; } = new();
        /// <summary>실제 입자추가 몸체다.</summary>
        public RawBurningVisual Visual { get; }
        /// <summary>실제 최대HP/현재대상/선택기다.</summary>
        public RawBurningSources Sources { get; }
        /// <summary>현재 장기 게임보조 owner다.</summary>
        public RawGameAuxState Owner { get; } = new();
        /// <summary>같은 현재50/51 raw와 실제 선택기·장기 owner를 연결한다.</summary>
        public Scene(string edition)
        {
            Pool = new(24000, true, edition: Edition(edition)); Types = Enumerable.Range(0, edition == "originals" ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            // 두 실제 피해 입력까지 같은 client번호로 확보한다.
            for (int sid = 5; sid <= 51; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            var registry = new RawRenderRegistry(Pool, new() { Reserve = _ => throw new InvalidOperationException("미선택 표시 확보"), ReleaseArray = () => Assert.Fail("미선택 표시반납"), Attach = (_, _, _, _) => Assert.Fail("미선택 부착") });
            Visual = new(Pool, registry, new(), new() { Reserve = _ => throw new InvalidOperationException("미선택 입자 확보") }); Sources = new(Pool, Types, Visual, Random, State);
        }
    }
    /// <summary>양판본의 여유/첫만료/NaN/24바이트쓰기와 현재total을 원본과 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void AppendMatchesOriginalFloatAndSignedBitPolicies(string edition)
    {
        var s = new Scene(edition); string[][] rows = X86Fixture.Read("burningparticle", edition); Assert.Equal(282, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var fields = new BurningVisualFields { Capacity = X86Fixture.UInt(row[1]), Total = X86Fixture.UInt(row[2]) }; fields.Particles = new BurningParticle[fields.Capacity];
            uint[] lives = row[3] == "-" ? [] : row[3].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(fields.Particles.Length, lives.Length);
            // 각 기존 입자의 원문 입력6필드만 준비한다.
            for (int i = 0; i < fields.Particles.Length; i++) fields.Particles[i] = new(10.5f + i, 20f + i, i - 1f, i + 1f, F(lives[i]), (i & 1) != 0);
            bool made = s.Visual.Append(fields, new(100.5f, 50.75f, -.25f, 1.5f, X86Fixture.FloatBits(row[4]), row[5] != "0"));
            Assert.Equal(row[6] != "0", made); Assert.Equal(X86Fixture.UInt(row[7]), fields.Total); Assert.Equal(row[8], Hex(ParticleBytes(fields))); return true;
        });
    }
    /// <summary>양판본의 원본 source행/300시도/폴백/제거와 공유 약화 owner를 대조한다.</summary>
    [Theory]
    [InlineData("originals", false)]
    [InlineData("originalCD", false)]
    [InlineData("originals", true)]
    [InlineData("originalCD", true)]
    public void SourceSelectionMatchesOriginalParticlesRowsAndRandom(string edition, bool shared)
    {
        var s = new Scene(edition); bool weakened = false; if (shared) s.State.SharedWeakenedMana = () => weakened;
        string[][] rows = X86Fixture.Read("burningsources", edition); Assert.Equal(326, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint kind = X86Fixture.UInt(row[5]); int type = kind == 2 ? 158 : kind == 1 ? 70 : 89;
            s.Types[type].Flags1 = 16; s.Types[type].Flags2 = kind == 1 ? 0x400u : 0; s.Types[type].MaxHitPoints = unchecked((int)X86Fixture.UInt(row[4]));
            s.Pool.RawSlot(50).Clear(); s.Pool.RawSlot(50)[10] = (byte)type; uint hp = X86Fixture.UInt(row[3]);
            if (edition == "originals") BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(50)[26..], hp); else BinaryPrimitives.WriteUInt16LittleEndian(s.Pool.RawSlot(50)[26..], (ushort)hp);
            s.Random.SetState(X86Fixture.UInt(row[8])); weakened = row[11] != "0"; s.State.WeakenedMana = shared ? !weakened : weakened;
            var fields = new BurningVisualFields { Victim = (ushort)X86Fixture.UInt(row[12]), Capacity = X86Fixture.UInt(row[9]), Selected = X86Fixture.Int(row[6]) };
            fields.Particles = new BurningParticle[fields.Capacity]; fields.Image.Width = X86Fixture.Int(row[1]); fields.Image.Height = X86Fixture.Int(row[2]);
            fields.Image.Pixels = Enumerable.Repeat(row[7] == "0" ? (byte)0 : (byte)255, fields.Image.Width * fields.Image.Height).ToArray(); fields.Rows = new byte[(fields.Image.Width + fields.Image.Height) / 5];
            uint pattern = X86Fixture.UInt(row[10]); fields.Total = pattern == 2 ? fields.Capacity : pattern == 1 ? Math.Min(3u, fields.Capacity) : 0;
            (float X, float Y)[] points = fields.Image.Width >= 8 && fields.Image.Height >= 8 ? [(2, 3), (7, 3), (0, 0)] : [(0, 0), (0, 0), (0, 0)];
            // 기존 source좌표와 행 표식은 원문 입력만 준비한다.
            for (int i = 0; i < fields.Particles.Length; i++)
            {
                bool source = pattern == 1 && i < 3; float x = source ? points[i].X : 10f + i, y = source ? points[i].Y : 20f + i;
                fields.Particles[i] = new(x, y, 0, 0, source ? 9999 : pattern == 2 ? 1.25f : 0, source);
                if (source) { int group = ((int)x + (int)y) / 5; if (group < fields.Rows.Length) fields.Rows[group] = 1; }
            }
            s.Sources.Select(fields);
            Assert.Equal("return", row[13]); Assert.Equal(X86Fixture.Int(row[14]), fields.Selected); Assert.Equal(X86Fixture.UInt(row[15]), fields.Total);
            Assert.Equal(row[16], Hex(fields.Rows)); Assert.Equal(row[17], Hex(ParticleBytes(fields))); Assert.Equal(X86Fixture.UInt(row[18]), s.Random.State()); return true;
        });
    }
    /// <summary>양판본5652관찰의 실제TYPE getter/복귀진단/IDIV예외/원래HP 저장을 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void CurrentHpGetterAndIntegerParentMatchOriginal(string edition)
    {
        var s = new Scene(edition); string[][] rows = X86Fixture.Read("burning-source-count-current", edition); Assert.Equal(2826, rows.Length);
        s.State.SharedWeakenedMana = () => s.Owner.PriestQuarterHp; s.State.SharedManaType = s.Owner.PriestTypeBox; s.State.SharedDaisType = s.Owner.DaisTypeBox;
        X86Fixture.CheckRows(rows, row =>
        {
            string[] input = row[1].Split(','); uint checking = X86Fixture.UInt(input[3]), profile = X86Fixture.UInt(input[7]), calls = 0; var diagnostics = new List<string>();
            // 같은 물리TYPE표를 유지하면서 각 입력 기록만 새 값으로 준비한다.
            for (int i = 0; i < s.Types.Length; i++) s.Types[i] = new();
            s.Types[74].MaxHitPoints = X86Fixture.Int(input[1]); s.Types[74].Flags1 = checking == 0 ? 16u : 0;
            s.Types[74].Flags2 = 0x400000u | (input[4] != "0" ? 0x400u : 0); s.Types[75].MaxHitPoints = 999;
            s.Owner.PriestQuarterHp = input[2] != "0"; s.Owner.PriestTypeBox.Value = X86Fixture.UInt(input[6]); s.Owner.DaisTypeBox.Value = 165;
            var fields = new BurningVisualFields { Victim = 50 }; fields.Image.Width = X86Fixture.Int(input[5]); fields.Image.Height = 2;
            s.Pool.RawSlot(50).Clear(); s.Pool.RawSlot(50)[10] = 74; s.Pool.RawSlot(50)[edition == "originals" ? 40 : 35] = 1;
            if (edition == "originals") BinaryPrimitives.WriteInt32LittleEndian(s.Pool.RawSlot(50)[26..], X86Fixture.Int(input[0])); else BinaryPrimitives.WriteInt16LittleEndian(s.Pool.RawSlot(50)[26..], unchecked((short)X86Fixture.Int(input[0])));
            s.State.Victim.Checking = new(() => checking); s.State.Victim.Robust = (_, _) => { calls++; return true; };
            s.State.Victim.Assertion = (expression, file, line) =>
            {
                diagnostics.Add($"{expression}|{file}|{line}"); Assert.Equal(0x1e3u, line);
                if (profile == 0) return;
                BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(50)[26..], profile == 2 ? 5u : 0u); s.Types[74].MaxHitPoints = 200;
                s.Owner.PriestQuarterHp = profile != 3; s.Owner.PriestTypeBox.Value = profile == 3 ? 75u : 74u;
                if (profile is 2 or 3) { s.Pool.RawSlot(50)[10] = 75; fields.Victim = 51; s.Types[75].Flags2 = profile == 2 ? 0x400u : 0; }
                if (profile == 3) s.Owner.DaisTypeBox.Value = 75;
            };
            uint seed = s.Random.State(), free = s.Pool.FreeCount; string outcome = "ok";
            try { s.Sources.Select(fields); } catch (DivideByZeroException) { outcome = "zero"; } catch (OverflowException) { outcome = "overflow"; }
            int currentHp = edition == "originals" ? BinaryPrimitives.ReadInt32LittleEndian(s.Pool.Slot(50)[26..]) : BinaryPrimitives.ReadInt16LittleEndian(s.Pool.Slot(50)[26..]);
            string post = string.Join(',', new long[] { fields.Selected, currentHp, s.Pool.Slot(50)[10], fields.Victim, checking, s.Owner.PriestQuarterHp ? 1 : 0, s.Owner.PriestTypeBox.Value, s.Owner.DaisTypeBox.Value, s.Types[74].MaxHitPoints });
            Assert.Equal(row[2], outcome); Assert.Equal(row[3], post); Assert.Equal(row[4], diagnostics.Count == 0 ? "-" : string.Join(';', diagnostics));
            if (edition == "originals") Assert.Equal(X86Fixture.UInt(row[6]), calls);
            Assert.Equal(seed, s.Random.State()); Assert.Equal(free, s.Pool.FreeCount); Assert.Equal(0u, fields.Total); Assert.Empty(fields.Particles); return true;
        });
    }
}
