using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 실제 원본 반사점/후보/후속 인자와 공유 난수 상태를 비트 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 반사1182/지역점213과 실제 Finder/Reward·부분 효과 검사다.</summary>
public sealed class RawIceRicochetTests
{
    /// <summary>원문 입력의 DWORD 비트를 읽는다.</summary>
    private static uint N(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원문 float를 불투명 DWORD로 읽는다.</summary>
    private static uint B(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>현재 DWORD만 기록한다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>전체 이미지를 원문 소문자hex로 바꾼다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();
    /// <summary>사본 없는 읽기 전용 원문 fixture 행이다.</summary>
    private static IEnumerable<string[]> Rows(string name = "ice-ricochet-x86.tsv") => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>기대열을 사용하지 않는 현재 표시/필드/후보/난수 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>저번호와 서버 후보의 같은 실제 물리다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 HP/비행 TYPE 자료다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 표시 반사 사거리다.</summary>
        internal MissileTypeData[] Data { get; } = Enumerable.Range(0, 188).Select(_ => new MissileTypeData()).ToArray();
        /// <summary>같은32비트 게임 난수다.</summary>
        internal GameRandom Random { get; } = new();
        /// <summary>원문128BYTE 필드와 padding이다.</summary>
        internal MissileFields Fields { get; } = new();
        /// <summary>현재 독립 입력의0..10열이다.</summary>
        internal string[] Row = [];
        /// <summary>명시 일반 Finder 후보의 원문 순서다.</summary>
        internal List<ushort> Candidates { get; } = [];
        /// <summary>다음 후보를 반환하는 현재 커서다.</summary>
        internal int Index;
        /// <summary>원문 외부 효과의 전체 순서다.</summary>
        internal List<string> Events { get; } = [];
        /// <summary>미작성 지역점의 선택된 실제 입력이다.</summary>
        internal Action<byte[]>? InitialPointBytes;
        /// <summary>현재 입력만으로 동일 raw/타입/후보/난수를 준비한다.</summary>
        internal void Prepare(string[] row)
        {
            Assert.Equal(15, row.Length); Row = row; Events.Clear(); Candidates.Clear(); Index = 0; Random.SetState(N(row[1])); InitialPointBytes = null;
            // 원문이 쓰지 않은128BYTE 패딩도 보존한다.
            for (int i = 0; i < 128; i++) Fields.Bytes[i] = unchecked((byte)(i * 13 + 7));
            Put(Fields.Bytes, 4, 81); Put(Fields.Bytes, 8, 90); Put(Fields.Bytes, 24, N(row[7])); Put(Fields.Bytes, 48, N(row[8])); Put(Fields.Bytes, 56, N(row[2])); Put(Fields.Bytes, 60, N(row[3]));
            // 앞선 행의 TYPE 자료가 다음 행으로 새지 않게 한다.
            for (int i = 0; i < Types.Length; i++) { Types[i] = new(); Data[i] = new(); }
            Data[130].Range = BitConverter.UInt32BitsToSingle(N(row[6]));
            // 최초 표시와 탐색 중 바뀌는 표시를 서로 다른 주소에 둔다.
            foreach (int sid in new[] { 90, 91 }) { Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt16LittleEndian(raw[6..], 81); raw[10] = 130; Put(raw, 14, N(row[4])); Put(raw, 18, N(row[5])); }
            if (row[9] == "-") return;
            // 각 후보는 별도 TYPE라서 HP/비행 입력을 덮지 않는다.
            foreach (string text in row[9].Split(';'))
            {
                string[] node = text.Split(','); Assert.Equal(7, node.Length); int type = 70 + Candidates.Count; ushort sid = checked((ushort)N(node[0])); Candidates.Add(sid); Span<byte> raw = Pool.RawSlot(sid); raw.Clear(); raw[10] = (byte)type; raw[11] = (byte)N(node[6]); raw[40] = (byte)N(node[3]); Put(raw, 14, N(node[4])); Put(raw, 18, N(node[5])); Types[type].Flags1 = N(node[1]); Types[type].Flags2 = N(node[2]);
            }
        }
        /// <summary>다음 후보 경계에서 원문 지정 필드만 바꾼다.</summary>
        internal ushort Next()
        {
            Events.Add("N"); Index++; if (Index == 1) { uint mutation = N(Row[10]); if (mutation == 1) Put(Fields.Bytes, 24, 15001); if (mutation == 2) Put(Fields.Bytes, 48, B(37.25f)); if (mutation == 3) Put(Fields.Bytes, 8, 91); if (mutation == 4) { Pool.RawSlot(90)[10] = 132; Put(Pool.RawSlot(90), 14, B(81.25f)); Put(Pool.RawSlot(90), 18, B(82.5f)); } } return Index < Candidates.Count ? Candidates[Index] : (ushort)0;
        }
        /// <summary>요청의 열두DWORD를 실제 호출 순서대로 기록한다.</summary>
        internal void Create(GunProjectileRequest request)
        {
            uint[] args = [request.Firing, request.Target, B(request.Speed), request.DisplayType, request.EffectType, B(request.TargetPosition[0]), B(request.TargetPosition[1]), B(request.TargetCenter[0]), B(request.TargetCenter[1]), request.Ricochet, B(request.FiringPosition[0]), B(request.FiringPosition[1])]; Events.Add("J:" + string.Join(',', args.Select(value => value.ToString(CultureInfo.InvariantCulture))));
        }
        /// <summary>실제 HP 본문을 사용하며 SP 난수 저장소는 초기화하지 않는다.</summary>
        internal RawSquidReward Reward() => new(Pool, Types, new(), new(), new RawSpStore(Random, () => 1));
        /// <summary>같은 입력에서 일반 탐색 순서와 현재 HP/후속 인자를 관찰한다.</summary>
        internal IceRicochetHooks Hooks()
        {
            RawSquidReward reward = Reward(); return new() { Begin = (area, flags) => { Assert.Equal(0u, flags); Index = 0; Events.Add($"B:{area.Left},{area.Top},{area.Right},{area.Bottom}:0:0"); return Candidates.Count == 0 ? (ushort)0 : Candidates[0]; }, Next = Next, HasHitPoints = sid => reward.HasHitPoints(sid), CreateProjectile = Create, InitialPointBytes = InitialPointBytes };
        }
        /// <summary>원문 Point/Generate 실행 뒤 출력점/필드/난수/사건을 직렬화한다.</summary>
        internal string[] Run()
        {
            var logic = new RawIceRicochet(Pool, Types, Data, Random, Hooks()); float[] point = [123.25f, 124.5f]; if (N(Row[0]) == 0) logic.Point(Fields, point); else logic.Generate(Fields); byte[] bytes = new byte[8]; Put(bytes, 0, B(point[0])); Put(bytes, 4, B(point[1])); return [string.Join(';', Events), Hex(bytes), Hex(Fields.Bytes), Random.State().ToString(CultureInfo.InvariantCulture)];
        }
    }
    /// <summary>변위/확률/현재 필드의 모든 독립1182행을 비교한다.</summary>
    [Theory]
    [InlineData(0, 486)] [InlineData(1, 696)]
    public void MatchesAllNativePointsCandidatesAndRandom(int kind, int expected)
    {
        var scene = new Scene(); int count = 0;
        // 기대열은 실행 뒤 실제 네 결과와만 비교한다.
        foreach (string[] row in Rows()) { if (N(row[0]) != kind) continue; scene.Prepare(row); Assert.Equal(row.Skip(11), scene.Run()); count++; }
        Assert.Equal(expected, count);
    }
    /// <summary>미작성 지역8BYTE의 NaN/모서리/유효점213행을 원문 그대로 공급한다.</summary>
    [Fact]
    public void MatchesAllNativeScratchBytesAndSelectedFill()
    {
        var scene = new Scene(); int count = 0;
        // 보존된 원문8BYTE를 결과와 별개 입력으로 전달한다.
        foreach (string[] row in Rows("ice-ricochet-scratch-x86.tsv"))
        {
            Assert.Equal(20, row.Length); scene.Prepare(row.Take(11).Concat(row.Skip(13).Take(4)).ToArray()); int fills = 0; scene.InitialPointBytes = bytes => { Assert.Equal(8, bytes.Length); fills++; Put(bytes, 0, N(row[11])); Put(bytes, 4, N(row[12])); }; Assert.Equal(row.Skip(13).Take(4), scene.Run()); Assert.Equal(row[4] == row[2] && row[5] == row[3] ? (int)N(row[18]) : 0, fills); if (fills != 0) Assert.Equal("1", row[19]); count++;
        }
        Assert.Equal(213, count);
    }
    /// <summary>선택된 생성/지역점 누락만 거부하며 앞선 확률 난수 소비는 보존한다.</summary>
    [Fact]
    public void MissingSelectedLifetimeOrScratchPreservesPriorRandom()
    {
        var scene = new Scene(); string[] row = Rows().First(); row[0] = "1"; row[1] = "20"; row[6] = "0"; scene.Prepare(row); var hooks = new IceRicochetHooks { Begin = (_, _) => 0, Next = () => 0, HasHitPoints = _ => false }; var logic = new RawIceRicochet(scene.Pool, scene.Types, scene.Data, scene.Random, hooks); Assert.Throws<InvalidOperationException>(() => logic.Generate(scene.Fields)); Assert.Empty(scene.Events); row[2] = row[4]; row[3] = row[5]; scene.Prepare(row); Assert.Throws<InvalidOperationException>(() => scene.Run()); Assert.Empty(scene.Events); Assert.Equal(0x0014003fu, scene.Random.State()); row[1] = "1"; scene.Prepare(row); scene.Run(); Assert.Empty(scene.Events); Assert.Equal(0x00010006u, scene.Random.State());
    }
    /// <summary>같은 실제 해시 체인/HP가 native와 같은 후속 요청과 난수 상태를 만든다.</summary>
    [Fact]
    public void ActualFinderAndRewardShareNativeRequest()
    {
        var scene = new Scene(); string[] row = Rows().Single(row => row[0] == "1" && row[1] == "20" && row[6] == "0" && row[7] == "60" && row[10] == "0" && row[9] == "15000,16,0,0,1114636288,1112014848,0"); scene.Prepare(row); var hash = new RawSquidHash(); hash.Bucket(0, 60, 50) = 15000; BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(15000)[4..], 0); var store = new RawSpStore(scene.Random, () => 1); var reward = new RawSquidReward(scene.Pool, scene.Types, new(), new(), store); IceRicochetHooks hooks = RawIceRicochet.MakeWorldHooks(scene.Pool, hash, scene.Types, reward, new() { CreateProjectile = scene.Create }); new RawIceRicochet(scene.Pool, scene.Types, scene.Data, scene.Random, hooks).Generate(scene.Fields); Assert.Single(scene.Events); Assert.Equal(row[11][row[11].LastIndexOf("J:", StringComparison.Ordinal)..], scene.Events[0]); Assert.Equal(row[13], Hex(scene.Fields.Bytes)); Assert.Equal(N(row[14]), scene.Random.State()); Assert.False(store.Initialized); Assert.Throws<ArgumentException>(() => RawIceRicochet.MakeWorldHooks(new(24000), hash, scene.Types, reward, hooks));
    }
    /// <summary>HP 효과 전 flags2와 이후 raw/표적·부모/속도 및 최초 표시 주소를 구별한다.</summary>
    [Fact]
    public void HpCallbackUsesCapturedFlagsAndCurrentRawAndFields()
    {
        var scene = new Scene(); string[] row = Rows().First(); row[0] = "1"; row[1] = "20"; row[6] = "0"; row[9] = "15000,16,0,0,1114636288,1112014848,0"; scene.Prepare(row); IceRicochetHooks hooks = scene.Hooks(); GunProjectileRequest? request = null; hooks.HasHitPoints = sid => { scene.Types[70].Flags2 = 0x120000; Put(scene.Fields.Bytes, 8, 91); Put(scene.Fields.Bytes, 48, B(37.25f)); scene.Pool.RawSlot(90)[10] = 132; Put(scene.Pool.RawSlot(90), 14, B(81.25f)); return true; }; hooks.CreateProjectile = value => request = value; new RawIceRicochet(scene.Pool, scene.Types, scene.Data, scene.Random, hooks).Generate(scene.Fields); Assert.NotNull(request); Assert.Equal((ushort)15000, request.Target); Assert.Equal((ushort)91, request.Firing); Assert.Equal(37.25f, request.Speed); Assert.Equal(132u, request.DisplayType); Assert.Equal(81.25f, request.FiringPosition[0]); Assert.Equal(60, request.TargetPosition[0]);
    }
}
