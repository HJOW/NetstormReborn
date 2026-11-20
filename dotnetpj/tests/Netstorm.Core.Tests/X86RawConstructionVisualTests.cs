using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 세 실제PE의 건설 표시 초기화·버퍼·입자·난수·팔레트 캐시를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>입력 장면만 공급하고 기대값은 원본1776행에서 읽는다.</summary>
public sealed class X86RawConstructionVisualTests
{
    /// <summary>원본 실행기의 공유 색 표 토큰이다.</summary>
    private const uint Palette = 0x12000200;

    /// <summary>같은 현재 풀·난수·표시 상태와 외부 효과 기록이다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 부모50의 물리 슬롯이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>실제 공유 난수 본체다.</summary>
        public readonly GameRandom Random = new();
        /// <summary>공유 색 표·배율·역할의 현재값이다.</summary>
        public readonly ConstructionVisualState State = new();
        /// <summary>외부 효과의 실제 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>한 행의14개DWORD 입력이다.</summary>
        public uint[] Input = [];
        /// <summary>독립 예약 실패를 공급할 바이트 수다.</summary>
        public uint FailBytes;
        /// <summary>같은 장기 상태를 공유하는 실제 본체다.</summary>
        public readonly RawConstructionVisual Visual;
        /// <summary>현재 원문 판본이다.</summary>
        public bool Patch => Pool.Edition == OriginalEdition.Patch1078;

        /// <summary>실제 장면의 수명 동안 같은 본체와 입력 저장소를 유지한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Pool = new(24000, false, null, edition); Visual = new(Pool, State, Random, Hooks());
        }
        /// <summary>행마다 부모와 현재 전역만 공급하며 결과 입자는 계산하지 않는다.</summary>
        public void Prepare(uint[] input)
        {
            Input = input; Assert.Equal(14, input.Length); Events.Clear(); FailBytes = 0; Random.SetState(input[4]);
            State.ProcessType = 17; State.Palette = input[5] != 0 ? 0 : Palette;
            State.ScaleX = Float(input[10]); State.ScaleY = Float(input[11]);
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); Put(50, 0, 0x11010000); raw[10] = 100;
            raw[Patch ? 34 : 32] = 1; raw[Patch ? 35 : 33] = (byte)input[7]; Put(50, 14, input[8]); Put(50, 18, input[9]);
        }
        /// <summary>관측 입력의DWORD만 변경한다.</summary>
        public void Put(int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid).Slice(offset, 4), value);
        /// <summary>원본PE에 공급했던 일곱 입력 픽셀 패턴이다.</summary>
        public ConstructionVisualImage Image()
        {
            var image = new ConstructionVisualImage { Width = (int)Input[1], Height = (int)Input[2], HotspotX = -3, HotspotY = 2 };
            image.Pixels = new byte[image.Width * image.Height];
            // 하단·구멍·중앙 열·마지막 픽셀·색0을 구별하는 명시 입력이다.
            for (int index = 0; index < image.Pixels.Length; index++)
            {
                byte color = (byte)((index * 13 + 17) % 255); uint pattern = Input[3];
                if (pattern == 1) color = 255;
                if (pattern == 2 && index % 3 == 0) color = 255;
                if (pattern == 3 && index / Input[1] > Input[2] / 3) color = 255;
                if (pattern == 4 && index % Input[1] != Input[1] / 2) color = 255;
                if (pattern == 5 && index + 1 != image.Pixels.Length) color = 255;
                if (pattern == 6) color = 0;
                image.Pixels[index] = color;
            }
            return image;
        }
        /// <summary>원문 실행기의 효과 응답·경계 변동만 공급한다.</summary>
        public ConstructionVisualHooks Hooks() => new()
        {
            Reserve = size => { Events.Add($"A:{size}"); return size != FailBytes; },
            Attach = (fields, type) =>
            {
                Events.Add($"B:{fields.Parent}:{type}");
                if (Input[6] == 1) State.Palette = Palette;
                if (Input[6] == 2) Put(50, 14, Bits(7.25f));
            },
            CreatePalette = () => { Events.Add("P"); return Palette; },
            Gradient = (count, from, to, noise) =>
            {
                Assert.Equal(2.4, noise); Assert.True(count is 8 or 3);
                Assert.Equal(count == 8 ? new[] { 0d, 0d, 1d } : new[] { 1d, 1d, 1d }, from);
                Assert.Equal(count == 8 ? new[] { 1d, 1d, 1d } : new[] { 128.0 / 255, 128.0 / 255, 128.0 / 255 }, to);
                Events.Add($"G:{count}");
            },
            BuildPalette = palette => { Assert.Equal(Palette, palette); Events.Add("J"); },
            Capture = parent =>
            {
                Events.Add($"C:{parent}");
                if (Input[6] == 3) { State.ScaleX = -4.5f; Put(50, 14, Bits(-1.25f)); }
                return Image();
            },
            ReleaseParticles = () => Events.Add("F"),
        };
        /// <summary>직접Seed/동기화가 받는 기존 필드를 입력으로 공급한다.</summary>
        public ConstructionVisualFields Initial()
        {
            bool instant = Input[0] != 0; ConstructionVisualImage image = Image();
            var fields = new ConstructionVisualFields
            {
                Parent = 50, Instant = instant, ZOrder = 123, Left = 1, Top = 2, Right = 3, Bottom = 4,
                Palette = Palette, Image = image, Output = Enumerable.Repeat((byte)255, image.Pixels.Length).ToArray(),
                LastRow = (image.Height - 1) * image.Width, Minimum = instant ? 2 : Float(0x3fa66666), Maximum = instant ? 9 : 2,
                Selected = Input[12], FirstDraw = true, Active = 3,
            };
            if (Input[13] != 0) { fields.Capacity = 9; fields.Particles = new ConstructionVisualParticle[9]; }
            return fields;
        }
    }

    /// <summary>입력float 비트를 복원한다.</summary>
    private static float Float(uint bits) => BitConverter.UInt32BitsToSingle(bits);
    /// <summary>실제float 저장 결과의DWORD 비트를 읽는다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>계약 검사의 독립 입력이며 결과 계산을 포함하지 않는다.</summary>
    private static uint[] Default() => [0, 7, 9, 0, 0, 0, 0, 128, Bits(20.5f), Bits(-3.25f), Bits(16), Bits(11), 0, 0];
    /// <summary>원본 의미 필드21개를 같은DWORD 순서로 직렬화한다.</summary>
    private static string Fields(ConstructionVisualFields fields, bool patch)
    {
        uint table = patch ? (fields.Instant ? 0x503680u : 0x5036acu) : (fields.Instant ? 0x507410u : 0x507440u);
        ConstructionVisualImage image = fields.Image;
        uint[] values = [table, (uint)fields.Parent, unchecked((uint)fields.ZOrder), Bits(fields.Left), Bits(fields.Top), Bits(fields.Right), Bits(fields.Bottom),
            fields.Palette, (uint)image.Width, (uint)image.Height, unchecked((uint)image.HotspotX), unchecked((uint)image.HotspotY), (uint)fields.LastRow,
            Bits(fields.Minimum), Bits(fields.Maximum), fields.Selected, fields.Opaque, fields.FirstDraw ? 1u : 0u, fields.Active, fields.Capacity, fields.Total];
        return string.Join(':', values);
    }
    /// <summary>미사용 입자가0인지 확인하고 사용 입자의 전체 값을 같은 순서로 적는다.</summary>
    private static string Particles(ConstructionVisualFields fields)
    {
        Assert.Equal(fields.Capacity, (uint)fields.Particles.Length); var values = new List<string>();
        // 목적지는 원문과 같은 출력 버퍼 번호로 정규화되어 있다.
        for (int index = 0; index < fields.Particles.Length; index++)
        {
            ConstructionVisualParticle particle = fields.Particles[index];
            if (index >= fields.Total) { Assert.Equal(default, particle); continue; }
            values.Add($"{particle.Active}:{particle.Color}:{Bits(particle.Life)}:{particle.Pixel}");
        }
        return values.Count == 0 ? "-" : string.Join(';', values);
    }

    /// <summary>각PE592행의 호출·필드·두버퍼·입자·난수·팔레트 캐시를 전부 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void Original592(string edition)
    {
        string[][] all = X86Fixture.Read("constructionvisual"); Assert.Equal(1776, all.Length);
        string[][] rows = X86Fixture.Section(all, edition); Assert.Equal(592, rows.Length);
        var scene = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        X86Fixture.CheckRows(rows, row =>
        {
            scene.Prepare(row[2].Split(',').Select(X86Fixture.UInt).ToArray()); ConstructionVisualFields fields;
            if (row[1] == "Create") fields = scene.Visual.Create(50, scene.Input[0] != 0);
            else { fields = scene.Initial(); if (row[1] == "Seed") scene.Visual.Seed(fields); else { Assert.Equal("Sync", row[1]); scene.Visual.Synchronize(fields); } }
            Assert.Equal(row[3], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); Assert.Equal(row[4], Fields(fields, scene.Patch));
            Assert.Equal(row[5], Convert.ToHexString(fields.Image.Pixels).ToLowerInvariant()); Assert.Equal(row[6], Convert.ToHexString(fields.Output).ToLowerInvariant());
            Assert.Equal(row[7], Particles(fields)); Assert.Equal(X86Fixture.UInt(row[8]), scene.Random.State()); Assert.Equal(X86Fixture.UInt(row[9]), scene.State.Palette);
            return true;
        });
    }

    /// <summary>확보 실패는 앞선 효과만 남기고 잘못된 이미지·출력은 첫 쓰기 전에 거부한다.</summary>
    [Fact]
    public void RejectsAllocationAndInvalidBuffers()
    {
        var scene = new Scene(); scene.Prepare(Default()); scene.FailBytes = 136;
        Assert.Throws<OutOfMemoryException>(() => scene.Visual.Create(50, false)); Assert.Equal("A:136", Assert.Single(scene.Events));
        uint[] cold = Default(); cold[5] = 1; scene.Prepare(cold); scene.FailBytes = 8;
        Assert.Throws<OutOfMemoryException>(() => scene.Visual.Create(50, false)); Assert.Equal("A:136;B:50:17;A:8", string.Join(';', scene.Events)); Assert.Equal(0u, scene.State.Palette);
        scene.Prepare(Default()); scene.FailBytes = 63; Assert.Throws<OutOfMemoryException>(() => scene.Visual.Create(50, false)); Assert.Equal(0u, scene.Random.State());
        scene.Prepare(Default()); scene.FailBytes = 6000; ConstructionVisualFields fields = scene.Initial();
        Assert.Throws<OutOfMemoryException>(() => scene.Visual.Seed(fields)); Assert.Equal(scene.Image().Pixels, fields.Image.Pixels); Assert.Equal(0u, scene.Random.State());
        fields = scene.Initial(); fields.Image.Height = 0; Assert.Throws<ArgumentException>(() => scene.Visual.Seed(fields));
        fields = scene.Initial(); fields.Output = fields.Output[..^1]; Assert.Throws<ArgumentException>(() => scene.Visual.Seed(fields));
    }

    /// <summary>캡처는진입부모·동기화는부착뒤현재부모를읽으며공통필드는부착뒤초기화한다.</summary>
    [Fact]
    public void CaptureKeepsEntryParentAndSynchronizeUsesAttachedParent()
    {
        var scene = new Scene(); scene.Prepare(Default()); scene.Pool.RawSlot(51).Clear(); scene.Put(51, 14, Bits(3)); scene.Put(51, 18, Bits(4));
        ConstructionVisualHooks hooks = scene.Hooks(); hooks.Attach = (fields, _) =>
        {
            fields.Parent = 51; fields.LastTime = fields.Elapsed = 99; fields.FirstDraw = true; fields.Active = fields.Selected = 5;
        };
        hooks.Capture = parent => { Assert.Equal(50, parent); return scene.Image(); };
        var visual = new RawConstructionVisual(scene.Pool, scene.State, scene.Random, hooks); ConstructionVisualFields fields = visual.Create(50, false);
        Assert.Equal(51, fields.Parent); Assert.Equal(48, fields.Left); Assert.Equal(44, fields.Top);
        Assert.Equal(0, fields.LastTime); Assert.Equal(0, fields.Elapsed); Assert.False(fields.FirstDraw); Assert.Equal(0u, fields.Active);
    }

    /// <summary>PATCH의부모·종속검사는첫예약전에실행하며dead Form만으로는거부하지않는다.</summary>
    [Fact]
    public void ValidatesParentChainBeforeAllocation()
    {
        var scene = new Scene(); scene.Prepare(Default()); scene.Pool.RawSlot(50)[11] = 4;
        Assert.Throws<InvalidOperationException>(() => scene.Visual.Create(50, false)); Assert.Empty(scene.Events);
        scene.Prepare(Default()); BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(50).Slice(6, 2), 51);
        scene.Pool.RawSlot(51).Clear(); scene.Pool.RawSlot(51)[10] = 30; scene.Pool.RawSlot(51)[11] = 2;
        _ = scene.Visual.Create(50, false); Assert.NotEmpty(scene.Events);
        scene.Events.Clear(); BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(51).Slice(4, 2), 51);
        Assert.Throws<InvalidOperationException>(() => scene.Visual.Create(50, false)); Assert.Empty(scene.Events);
    }

    /// <summary>실제건설시작은표시생성이끝난뒤현재시각으로필드를붙이며팔레트를재사용한다.</summary>
    [Fact]
    public void ProcessAdapterCreatesVisualBeforeStartTime()
    {
        var scene = new Scene(); scene.Prepare(Default()); var state = new ConstructionProcessState(); var visuals = new List<ConstructionVisualFields>();
        ConstructionProcessFields? attached = null; ConstructionVisualHooks visualHooks = scene.Hooks();
        visualHooks.Capture = _ => { state.Now = 5.5; return scene.Image(); };
        var visual = new RawConstructionVisual(scene.Pool, scene.State, scene.Random, visualHooks);
        var hooks = new ConstructionProcessHooks
        {
            Reserve = () => true, LoadSound = _ => 77, ReserveSound = () => false, AttachSound = (_, _, _, _) => { }, Refresh = () => { },
            MaxHitPoints = _ => 100, UpdateDisplay = _ => { }, RemoveContained = (_, _) => { }, PlayAt = (_, _, _, _) => { },
            ReadySound = _ => 0, Play = _ => { }, Unpop = (_, _) => { }, Repop = (_, _) => { }, Push = (_, _, _, _) => { },
            Attach = (fields, _, _, _) => { Assert.Single(visuals); attached = fields.Copy(); return attached; },
        };
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var process = new RawConstructionProcess(scene.Pool, types, state, visual.MakeProcessHooks(scene.Pool, visuals.Add, hooks)); process.Start(50, 0);
        Assert.True(Assert.Single(visuals).Instant); Assert.NotNull(attached); Assert.Equal(5.5, attached.Start); Assert.Equal(15.5, attached.End);
        scene.State.Palette = 0; ConstructionVisualFields first = visual.Create(50, false); int count = scene.Events.Count;
        ConstructionVisualFields second = visual.Create(50, false); Assert.Equal(first.Palette, second.Palette); Assert.Equal(count + 4, scene.Events.Count);
        Assert.Throws<ArgumentException>(() => visual.MakeProcessHooks(new RawSidPool(24000), visuals.Add, hooks));
    }
}
