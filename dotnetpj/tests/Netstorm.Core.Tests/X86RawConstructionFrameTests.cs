using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 건설 표시의 프레임 전체·부분·연속 중간 상태를 세 원본PE의 불변 관측과 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>PATCH435·CD441·별도10.37의441행과 생성 이후 실제 연속 갱신을 확인한다.</summary>
public sealed class X86RawConstructionFrameTests
{
    /// <summary>생성과 프레임이 공유하는 원문 팔레트 토큰이다.</summary>
    private const uint PaletteToken = 0x12000200;

    /// <summary>원본과 같은 물리 입력·시간·색 표·효과 응답만 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 부모50의 실제 물리 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>현재 공유 난수다.</summary>
        public readonly GameRandom Random = new();
        /// <summary>현재 표시 배율이다.</summary>
        public readonly ConstructionVisualState VisualState = new();
        /// <summary>현재 시각·카메라 입력이다.</summary>
        public readonly ConstructionFrameState State = new();
        /// <summary>현재 색수와 인접DWORD 입력이다.</summary>
        public readonly ConstructionFramePalette Palette = new();
        /// <summary>28개의DWORD/시각64비트 입력이다.</summary>
        public ulong[] Input = [];
        /// <summary>외부 효과의 실제 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>첫dirty 요청의 전역 변경을 구별한다.</summary>
        private int _dirtyCount;
        /// <summary>동일 입력 owner를 읽는 실제 프레임 본체다.</summary>
        public readonly RawConstructionFrame Frame;
        /// <summary>현재 원문 판본이다.</summary>
        public bool Patch => Pool.Edition == OriginalEdition.Patch1078;

        /// <summary>한 판본의 같은 자료를 모든 프레임 경계와 공유한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            Pool = new(32768, false, null, edition); Frame = new(Pool, VisualState, State, Palette, Random, Hooks());
        }
        /// <summary>예상 결과를 계산하지 않고 현재 전역·슬롯·팔레트 입력을 공급한다.</summary>
        public void Prepare(string text)
        {
            Input = text.Split(',').Select(value => value.StartsWith('-') ? unchecked((ulong)X86Fixture.Long(value)) : X86Fixture.ULong(value)).ToArray();
            Assert.Equal(28, Input.Length); Events.Clear(); _dirtyCount = 0;
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); Put(0, Patch ? 0x501dd0u : 0x5058d8u); raw[10] = 100;
            raw[11] = (byte)Input[24]; raw[Patch ? 40 : 35] = 1; Put(14, (uint)Input[14]); Put(18, (uint)Input[15]); Put(26, (uint)Input[7]);
            Random.SetState((uint)Input[2]); VisualState.ScaleX = F(Input[16]); VisualState.ScaleY = F(Input[17]);
            State.Now = BitConverter.UInt64BitsToDouble(Input[10]); State.Rate = F(Input[13]); State.CameraX = F(Input[18]); State.CameraY = F(Input[19]);
            Palette.Token = PaletteToken; Palette.Count = (int)Input[22];
            Palette.Entries = Enumerable.Range(0, Palette.Count + 1).Select(index => 0x81000000u + (uint)index * 257 + 7).ToArray();
        }
        /// <summary>원문의 입력DWORD만 실제 부모 슬롯에 쓴다.</summary>
        public void Put(int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(50).Slice(offset, 4), value);
        /// <summary>명시 최대HP·dirty·장치·Kill 인자와경계변동만 공급한다.</summary>
        public ConstructionFrameHooks Hooks() => new()
        {
            MaxHitPoints = parent =>
            {
                Assert.Equal(50, parent); Events.Add("H:50");
                if (Input[23] == 1) Put(26, 1); if (Input[23] == 2) Put(26, 100);
                if (Input[23] == 3) { Put(14, Bits(-7.75f)); State.Now = 30; }
                return unchecked((int)Input[8]);
            },
            Invalidate = rectangle =>
            {
                Events.Add($"I:{rectangle.Left}:{rectangle.Top}:{rectangle.Right}:{rectangle.Bottom}");
                if (_dirtyCount == 0 && Input[23] == 4) State.CameraX = 100.25f;
                if (_dirtyCount == 0 && Input[23] == 5) Pool.RawSlot(50)[Patch ? 40 : 35] = 0;
                _dirtyCount++;
            },
            Draw = request =>
            {
                Assert.Equal(64, request.Pixels.Length); Assert.Equal(8, request.Pitch); Assert.Equal(new(0, 0, 8, 8), request.Clip);
                Events.Add($"D:{request.X}:{request.Y}:{request.Pitch}:0:0:8:8");
            },
            Kill = flags => { Assert.Equal(0u, flags); Events.Add("K:0"); },
        };
        /// <summary>8×8 소스 패턴과이미 선택된입자만 입력으로 공급한다.</summary>
        public ConstructionFrameFields Initial()
        {
            var fields = new ConstructionFrameFields { Delta = 13, ScaledDelta = 17 }; ConstructionVisualFields v = fields.Visual;
            v.Parent = (int)Input[25]; v.Instant = Input[0] != 0; v.Left = 1.1f; v.Top = -2.1f; v.Right = 3.1f; v.Bottom = 4.1f;
            v.LastTime = BitConverter.UInt64BitsToDouble(Input[11]); v.Elapsed = BitConverter.UInt64BitsToDouble(Input[12]);
            v.Palette = PaletteToken; v.Image = new() { Width = 8, Height = 8, HotspotX = -3, HotspotY = 2, Pixels = new byte[64] };
            v.Output = Enumerable.Repeat((byte)255, 64).ToArray();
            // 관측 실행기의 선택 전 입력 픽셀 패턴이다.
            for (int index = 0; index < 64; index++) v.Image.Pixels[index] = Input[1] == 2 && index % 3 == 0 ? (byte)255 : (byte)((index * 13 + 17) % 255);
            v.LastRow = 56; v.Minimum = v.Instant ? 2 : F(0x3fa66666); v.Maximum = F(Input[27]); v.Selected = (uint)Input[6];
            v.Opaque = 64; v.FirstDraw = Input[9] != 0; v.Active = (uint)Input[5]; v.Capacity = (uint)Input[3]; v.Total = (uint)Input[4];
            v.Particles = new ConstructionVisualParticle[v.Capacity];
            // 이미 선택된 입자의 위치/수명/색은 원본의 명시 입력이다.
            for (uint index = 0; index < v.Total; index++)
            {
                uint pixel = 27 + index * 8;
                v.Particles[index] = new() { Active = index < v.Active ? (byte)1 : (byte)0, Color = v.Image.Pixels[pixel], Life = F(Input[21]) * (index + 1), Pixel = pixel };
                v.Image.Pixels[pixel] = 255;
            }
            if (Input[26] == 0) v.Image.Pixels = [];
            return fields;
        }
    }

    /// <summary>입력의하위DWORD를float로복원한다.</summary>
    private static float F(ulong value) => BitConverter.UInt32BitsToSingle((uint)value);
    /// <summary>현재float저장결과의비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>빈관측열은원문표기인대시로출력한다.</summary>
    private static string Join(IEnumerable<string> values, char separator) { string value = string.Join(separator, values); return value.Length == 0 ? "-" : value; }
    /// <summary>빈버퍼표기까지원본과같은16진문자열로바꾼다.</summary>
    private static string Hex(byte[] bytes) => bytes.Length == 0 ? "-" : Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>전체필드·미사용입자잔여값·두버퍼·난수상태를직렬화한다.</summary>
    private static string[] Snapshot(ConstructionFrameFields fields, GameRandom random)
    {
        ConstructionVisualFields v = fields.Visual;
        uint[] words = [Bits(v.Left), Bits(v.Top), Bits(v.Right), Bits(v.Bottom), Bits(fields.Delta), Bits(fields.ScaledDelta), v.Selected, v.Opaque, v.FirstDraw ? 1u : 0u, v.Active, v.Capacity, v.Total];
        string text = string.Join(':', words) + $":{BitConverter.DoubleToUInt64Bits(v.LastTime)}:{BitConverter.DoubleToUInt64Bits(v.Elapsed)}";
        return [text, Hex(v.Image.Pixels), Hex(v.Output), Join(v.Particles.Select(p => $"{p.Active}:{p.Color}:{Bits(p.Life)}:{p.Pixel}"), ';'), random.State().ToString()];
    }
    /// <summary>계약검사에쓰는유한입력이며프레임결과계산은포함하지않는다.</summary>
    private static string Default() => string.Join(',', new ulong[] { 0, 0, 17, 12, 3, 0, 3, 50, 101, 1, BitConverter.DoubleToUInt64Bits(.025), 0,
        BitConverter.DoubleToUInt64Bits(1000000.125), Bits(.7f), Bits(20.1f), Bits(-3.2f), Bits(16), Bits(11), Bits(.75f), Bits(-.25f), 27, Bits(.25f), 11, 0, 0, 50, 1, Bits(2) });

    /// <summary>세PE의전체·부분명령과연속5단계모든중간관측을대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void OriginalWholeBody(string edition)
    {
        string[][] all = X86Fixture.Read("constructionframe"); Assert.Equal(1317, all.Length);
        string[][] rows = X86Fixture.Section(all, edition); Assert.Equal(edition == "originals" ? 435 : 441, rows.Length);
        var scene = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        X86Fixture.CheckRows(rows, row =>
        {
            scene.Prepare(row[2]); ConstructionFrameFields fields = scene.Initial(); uint result = 0; var history = new List<string>();
            if (row[1] == "Run") scene.Frame.Run(fields);
            else if (row[1] == "Expand") scene.Frame.Expand(fields, (uint)scene.Input[20]);
            else if (row[1] == "Append") { if (scene.Frame.Append(fields, unchecked((int)scene.Input[20]))) result = fields.Visual.Total; }
            else if (row[1] == "Color") result = scene.Frame.Color(F(scene.Input[21]), F(scene.Input[27]));
            else if (row[1] == "Clock") scene.Frame.Clock(fields);
            else if (row[1] == "Draw") scene.Frame.Draw(fields);
            else
            {
                // 원본과동일한HP/시각입력의다섯중간상태를전부기록한다.
                for (int step = 0; step < 5; step++)
                {
                    scene.State.Now = step * .0625; scene.Put(26, (uint)(step * 25)); if (step == 4) scene.Pool.RawSlot(50)[scene.Patch ? 40 : 35] = 0;
                    scene.Frame.Run(fields); scene.Frame.Draw(fields); history.Add(Join(Snapshot(fields, scene.Random), '|'));
                }
            }
            string[] observed = Snapshot(fields, scene.Random);
            if (scene.Input[26] == 0) { scene.Input[26] = 1; observed[1] = Hex(scene.Initial().Visual.Image.Pixels); scene.Input[26] = 0; }
            Assert.Equal(row[3], Join(scene.Events, ';')); Assert.Equal(row.Skip(4).Take(5).ToArray(), observed);
            Assert.Equal(X86Fixture.UInt(row[9]), result); Assert.Equal(row[10], Join(history, '~')); return true;
        });
    }

    /// <summary>원본의잘못된저장소/팔레트는실제쓰기전에호스트예외로거부한다.</summary>
    [Fact]
    public void RejectsInvalidStorageAndPalette()
    {
        var scene = new Scene(); scene.Prepare(Default()); ConstructionFrameFields fields = scene.Initial(); byte[] source = fields.Visual.Image.Pixels.ToArray();
        fields.Visual.LastRow = 63; Assert.Throws<ArgumentException>(() => scene.Frame.Run(fields)); Assert.Equal(source, fields.Visual.Image.Pixels); Assert.Empty(scene.Events);
        fields = scene.Initial(); fields.Visual.Particles[0].Pixel = 64; Assert.Throws<ArgumentException>(() => scene.Frame.Expand(fields, 27));
        fields = scene.Initial(); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Frame.Append(fields, 64)); Assert.Equal(3u, fields.Visual.Total); Assert.Equal(17u, scene.Random.State());
        scene.Palette.Entries = scene.Palette.Entries.Take(11).ToArray(); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Frame.Color(2, 2));
        Assert.Throws<ArgumentException>(() => scene.Frame.Color(1, 0));
    }

    /// <summary>필수장치누락을거부하고capacity0 Draw는부모/장치를조회하지않는다.</summary>
    [Fact]
    public void RequiresHooksAndSkipsEmptyCapacity()
    {
        var scene = new Scene(); scene.Prepare(Default()); ConstructionFrameHooks hooks = scene.Hooks(); hooks.Kill = null;
        Assert.Throws<ArgumentException>(() => new RawConstructionFrame(scene.Pool, scene.VisualState, scene.State, scene.Palette, scene.Random, hooks));
        var fields = new ConstructionFrameFields(); scene.Frame.Draw(fields); Assert.Empty(scene.Events);
        fields.Visual.Capacity = 1; fields.Visual.Total = 2; Assert.Throws<ArgumentException>(() => scene.Frame.Draw(fields));
    }

    /// <summary>실제생성의소유버퍼를연속프레임에넘겨원래픽셀복구와abstract종료를확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, false)]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, false)]
    [InlineData(OriginalEdition.Cd1072, true)]
    public void CreateThenRunDrawAndFinish(OriginalEdition edition, bool instant)
    {
        var scene = new Scene(edition); scene.Prepare(Default()); scene.Pool.RawSlot(50)[scene.Patch ? 34 : 32] = 1; scene.VisualState.Palette = PaletteToken;
        ConstructionVisualImage input = scene.Initial().Visual.Image;
        // 실제SHP처럼투명테두리만추가한선택전캡처입력이다.
        for (int index = 0; index < 64; index++) input.Pixels[index] = index / 8 is 0 or 7 || index % 8 is 0 or 7 ? (byte)255 : (byte)((index * 13 + 17) % 255);
        var hooks = new ConstructionVisualHooks
        {
            Reserve = _ => true, Attach = (_, _) => { }, CreatePalette = () => PaletteToken, Gradient = (_, _, _, _) => { }, BuildPalette = _ => { },
            Capture = _ => new() { Width = 8, Height = 8, HotspotX = -3, HotspotY = 2, Pixels = input.Pixels.ToArray() }, ReleaseParticles = () => { },
        };
        var visual = new RawConstructionVisual(scene.Pool, scene.VisualState, scene.Random, hooks); scene.Input[8] = 21; scene.Put(26, 0);
        var fields = new ConstructionFrameFields { Visual = visual.Create(50, instant) }; scene.Events.Clear(); scene.State.Now = 0;
        uint opaque = fields.Visual.Opaque; Assert.True(fields.Visual.Total > 0); scene.Frame.Run(fields); scene.Frame.Draw(fields); Assert.Equal(0, fields.Delta);
        scene.Put(26, 21);
        // 앞프레임에서추가한이웃도이후프레임에활성화/만료시킨다.
        for (int index = 1; index <= 20; index++) { scene.State.Now = index; scene.Frame.Run(fields); }
        Assert.Equal(0u, fields.Visual.Total); Assert.Equal(0u, fields.Visual.Active); Assert.Equal(opaque, fields.Visual.Selected); Assert.Equal(input.Pixels, fields.Visual.Output);
        int count = scene.Events.Count; scene.Frame.Draw(fields); Assert.Equal(count + 1, scene.Events.Count);
        scene.Pool.RawSlot(50)[scene.Patch ? 40 : 35] = 0; scene.Frame.Run(fields); Assert.Equal("K:0", scene.Events[^1]);
    }
}
