using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 포구/방향/사정선과 기존 미사일 본문을 함께 독립 PE 자료에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>크로스보우/썬더/초기 아이스의3514관측 및 충돌명령688관측 검사다.</summary>
public sealed class RawMissileGeometryTests
{
    /// <summary>부호 입력도 포함한 하위DWORD를 읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>현재 float 비트의 십진 표현이다.</summary>
    private static uint B(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>현재 DWORD만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>현재 전체 물리 바이트를 원문 소문자hex로 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>같은 실제 geometry/모드/미사일에 독립 입력과 명시 외부 효과만 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>관측 세 자산과 같은 Pool이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 group/hotspot/발자국 표다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 사거리/음향/방향 자료다.</summary>
        internal MissileTypeData[] Data { get; } = Enumerable.Range(0, 188).Select(_ => new MissileTypeData()).ToArray();
        /// <summary>현재 SHP 포구와 실제 균등 방향 자료다.</summary>
        internal ProjectileGeometryType[] GeometryData { get; } = Enumerable.Range(0, 188).Select(_ => new ProjectileGeometryType()).ToArray();
        /// <summary>같은 현재 모드가 빌리는 코드 표다.</summary>
        internal RiftTypeFrames[] Frames { get; } = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        /// <summary>같은 현재 게임 시각/역할이다.</summary>
        internal MissileState State { get; } = new();
        /// <summary>미작성 padding까지 포함하는128BYTE다.</summary>
        internal MissileFields Fields { get; } = new();
        /// <summary>실제 외부 효과의 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>현재 독립 입력이다.</summary>
        internal string[] Row = [];
        /// <summary>0크로스보우/1썬더/2초기 아이스의 원문 파생이다.</summary>
        internal int Style { get; }
        /// <summary>원문 발사자 TYPE다.</summary>
        internal int Source => Style == 0 ? 140 : Style == 1 ? 131 : 129;
        /// <summary>원문 표시자 TYPE다.</summary>
        internal int Display => Style == 0 ? 81 : Style == 1 ? 132 : 130;
        /// <summary>기대 결과가 시작되는 실제 열이다.</summary>
        internal int Output => Style == 0 ? 21 : Style == 1 ? 23 : 24;
        /// <summary>세 관측 자산 번호를 동일 순서로 확보한다.</summary>
        internal Scene(int style)
        {
            Style = style;
            // 원문 관측이 사용한50/60/90을 같은 순서로 확보한다.
            for (int sid = 5; sid <= 100; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>원문 입력의 float 비트를 그대로 읽는다.</summary>
        internal float F(int at) => BitConverter.UInt32BitsToSingle(N(Row[at]));
        /// <summary>기대값을 사용하지 않고 세 자산/시간/표/raw를 구성한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(Output + 4, row.Length); Row = row; Events.Clear(); State.Now = N(row[0]) == 0 ? 10.25 : 10 + N(row[9]) * .5;
            // 원문이 쓰지 않는 바이트의 패턴이다.
            for (int i = 0; i < 128; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            // 세 자산의 상태와 현재 원문 좌표를 독립 입력으로 쓴다.
            foreach ((int sid, int type) in new[] { (50, Source), (60, 71), (90, Display) })
            {
                Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); Put(raw, 0, 0x11319000); raw[10] = (byte)type; raw[11] = sid == 60 ? (byte)N(row[10]) : (byte)0; raw[34] = 1; raw[40] = (sid == 60 || (Style != 0 && sid == 50)) ? (byte)N(row[19]) : (byte)0; Put(raw, 14, sid == 50 ? N(row[1]) : B(50)); Put(raw, 18, sid == 50 ? N(row[2]) : B(50));
            }
            Types[Source].Group = Style == 0 ? 0 : 1; Types[Source].HotspotX = int.Parse(row[5], CultureInfo.InvariantCulture); Types[Source].HotspotY = int.Parse(row[6], CultureInfo.InvariantCulture); Data[Source].Range = F(13); Data[Source].FiringSound = N(row[14]); Data[Display].FiringSound = N(row[15]); Data[Display].ImpactSound = N(row[16]); Data[Display].FrameCount = int.Parse(row[17], CultureInfo.InvariantCulture); Data[136].ImpactSound = N(row[16]); GeometryData[Source].DefaultFrame = 0; GeometryData[Source].FrameCount = Style == 0 ? 1 : 36; GeometryData[Source].ShotOffsets = Enumerable.Repeat((unchecked((short)int.Parse(row[3], CultureInfo.InvariantCulture)), unchecked((short)int.Parse(row[4], CultureInfo.InvariantCulture))), Style == 0 ? 1 : 36).ToArray(); GeometryData[Display].Directions = int.Parse(row[17], CultureInfo.InvariantCulture); GeometryData[Display].FrameCount = int.Parse(row[18], CultureInfo.InvariantCulture);
            if (Style != 0) { Put(Pool.RawSlot(50), 36, N(row[21])); Types[Source].FootX = Types[Source].FootY = 0; Types[71].FootX = 2; Types[71].FootY = 3; GeometryData[Source].Range = F(13); var codes = new RiftFrameCode[36]; codes[N(row[21])] = new((byte)N(row[22]), 0, 0, 0); Frames[Source] = new(codes); }
            if (N(row[0]) != 1) return;
            byte[] fields = Fields.Bytes; Put(fields, 0, 0x50d630); Put(fields, 4, 81); Put(fields, 8, 90); Put(fields, 12, 0); Put(fields, 16, N(row[20])); Put(fields, 20, 50); Put(fields, 24, 60); Put(fields, 28, N(row[12]));
            // 독립 Frame의 네 초기 DWORD와 사각형은 생성 결과에서 가져오지 않는다.
            for (int at = 32; at < 48; at += 4) Put(fields, at, 0);
            Put(fields, 48, B(Style == 0 ? 35 : 20)); Put(fields, 52, 0); Put(fields, 56, N(row[1])); Put(fields, 60, N(row[2])); Put(fields, 64, N(row[7])); Put(fields, 68, N(row[8])); Put(fields, 72, N(row[7])); Put(fields, 76, N(row[8]));
            // 원문 무효 사각형을 같은 비트로 보존한다.
            for (int at = 88; at < 104; at += 4) Put(fields, at, 0xc61c4000);
            BinaryPrimitives.WriteDoubleLittleEndian(fields.AsSpan(104), 12); BinaryPrimitives.WriteDoubleLittleEndian(fields.AsSpan(112), 10); Put(fields, 120, B(50)); Put(fields, 124, N(row[11]) != 0 ? B(89) : 0xc61c4000);
        }
        /// <summary>명시 cdecl 위치 음향의 다섯 인수를 기록한다.</summary>
        internal void Sound(float x, float y, uint token) => Events.Add($"S:{B(x)}:{B(y)}:{token}:0:0");
        /// <summary>숫자 기하는 제품 모듈을 사용하고 효과만 원문 관측 경계로 남긴다.</summary>
        internal MissileHooks Hooks(RawProjectileGeometry geometry) => new()
        {
            Muzzle = geometry.Muzzle, Hotspot = sid => { Events.Add("H:50"); return geometry.Hotspot(sid); }, TargetDimensions = sid => { if (Style != 0) return geometry.Dimensions(sid); Events.Add("D:60"); return [2, 3]; }, RayEnd = geometry.CannonRayEnd,
            DirectionFrame = (from, to, type) => { if (Style == 0) Events.Add($"F:{B(from[0])}:{B(from[1])}:{B(to[0])}:{B(to[1])}:{type}"); return geometry.DirectionFrame(from, to, type); },
            Create = (type, flags) => { Events.Add($"A:{type}:{flags}"); return 90; }, Owner = (sid, owner) => { Pool.RawSlot(sid)[34] = owner; Events.Add($"O:90:{owner}"); }, Pop = (sid, x, y, flags) => { Put(Pool.RawSlot(sid), 14, B(x)); Put(Pool.RawSlot(sid), 18, B(y)); Events.Add($"P:90:{B(x)}:{B(y)}:{flags}"); }, Unpop = (_, flags) => Events.Add($"U:90:{flags}"), FiringSound = Sound, ImpactSound = Sound,
            Center = sid => { if (Style == 0) { Events.Add("C:50"); return [50, 50]; } ReadOnlySpan<byte> raw = Pool.Slot(sid); return [BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..])), BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]))]; },
            TargetBounds = (_, point) => { Events.Add($"T:{B(point[0])}:{B(point[1])}"); return [49, 89, 51, 91]; }, Hit = (firing, target) => Events.Add($"G:{firing}:{target}"), HitEffect = (x, y) => Events.Add($"H:{B(x)}:{B(y)}"), Refresh = _ => Events.Add("Q:50"), Impact = (x, y, mode, payload) => Events.Add($"I:{B(x)}:{B(y)}:{mode}:{B(payload)}"), Destroy = (_, flags) => Events.Add($"X:90:{flags}"),
            RandomInteger = (low, high) => { Events.Add($"N:{low}:{high}"); return unchecked((int)N(Row[23])); }, ImpactName = (x, y, name) => Events.Add($"F:{B(x)}:{B(y)}:{name}"), Ricochet = fields => Events.Add($"R:{fields.Parent()}:{fields.Target()}"), QueueCommand = (command, first, second) => Events.Add($"E:{command}:{first}:{second}"),
        };
        /// <summary>실제 기하 getter와 생성/Frame을 실행하고 전체 결과를 직렬화한다.</summary>
        internal string[] Run(bool connectQueue = true)
        {
            var mode = new RawGunMode(Pool, Frames, new() { Value = 140 }); var geometry = new RawProjectileGeometry(Pool, Types, GeometryData, gunMode: mode); MissileHooks hooks = Hooks(geometry); if (!connectQueue) hooks.QueueCommand = null; var logic = new RawMissile(Pool, Types, Data, State, hooks); float[] point = [0, 0]; uint kind = N(Row[0]);
            if (kind == 0) { var request = new GunProjectileRequest { Firing = 50, Target = 60, Speed = Style == 0 ? 35 : 20, DisplayType = (uint)Display, EffectType = 0, TargetPosition = [F(7), F(8)], TargetCenter = N(Row[12]) != 0 ? [F(7), F(8)] : [-10000, -10000], FiringPosition = [-10000, -10000] }; ushort asset = logic.Construct(Fields, request); Events.Add("B:28:90:0:16"); Put(Fields.Bytes, 4, 81); Put(Fields.Bytes, 8, asset); } else if (kind == 1) logic.Frame(Fields); else point = kind == 2 ? geometry.Muzzle(50) : geometry.CannonRayEnd([F(1), F(2)], [F(7), F(8)], 50);
            byte[] output = new byte[8]; Put(output, 0, B(point[0])); Put(output, 4, B(point[1])); return [string.Join(';', Events), Hex(Fields.Bytes), string.Concat(new[] { 50, 60, 90 }.Select(sid => Hex(Pool.Slot(sid)))), Hex(output)];
        }
    }
    /// <summary>각 실제 파생의 생성/Frame/포구/사정선 전체3514행을 원문과 비교한다.</summary>
    [Theory]
    [InlineData(0, 0, 229)] [InlineData(0, 1, 466)] [InlineData(0, 2, 8)]
    [InlineData(1, 0, 320)] [InlineData(1, 1, 454)] [InlineData(1, 2, 8)] [InlineData(1, 3, 336)]
    [InlineData(2, 0, 320)] [InlineData(2, 1, 1029)] [InlineData(2, 2, 8)] [InlineData(2, 3, 336)]
    public void MatchesAllNativeInitialMissilesAndGeometry(int style, int kind, int expected)
    {
        var scene = new Scene(style); string name = style == 0 ? "crossbow-missile-x86.tsv" : style == 1 ? "thunder-missile-x86.tsv" : "ice-missile-initial-x86.tsv"; int count = 0;
        // 기대값은 네 반환 결과를 만든 뒤에만 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name))) { if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); if (N(row[0]) != kind) continue; scene.Prepare(row); Assert.Equal(row.Skip(scene.Output), scene.Run()); count++; }
        Assert.Equal(expected, count);
    }
    /// <summary>충돌 효과/입력큐/피해/삭제 순서688행과 선택 큐 누락 실패를 대조한다.</summary>
    [Fact]
    public void MatchesNativeThunderHitEventsAndRejectsMissingQueue()
    {
        var scene = new Scene(1); string[]? eventRow = null; int count = 0, events = 0;
        // 기존23입력 뒤의 세 TYPE/상태만 추가하고 기대열로 입력을 만들지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/thunder-hit-event-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); PrepareEvent(scene, row); string[] actual = scene.Run(); Assert.Equal(row.Skip(26), actual); count++; if (actual[0].Contains("E:", StringComparison.Ordinal)) { events++; eventRow = row; }
        }
        Assert.Equal(688, count); Assert.Equal(24, events); Assert.NotNull(eventRow); PrepareEvent(scene, eventRow); Assert.Throws<InvalidOperationException>(() => scene.Run(false));
    }
    /// <summary>독립 충돌명령의 추가 TYPE/상태만 같은 장면에 공급한다.</summary>
    private static void PrepareEvent(Scene scene, string[] row)
    {
        Assert.Equal(30, row.Length); scene.Prepare(row.Take(23).Concat(row.Skip(26)).ToArray()); int target = (int)N(row[23]), display = (int)N(row[24]); scene.Pool.RawSlot(60)[10] = (byte)target; scene.Pool.RawSlot(90)[10] = (byte)display; scene.Pool.RawSlot(50)[11] = (byte)N(row[25]); scene.Types[target].FootX = 2; scene.Types[target].FootY = 3; scene.Data[display].ImpactSound = N(row[16]);
    }
}
