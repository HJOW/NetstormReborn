using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Tests;
using VfxDatabase = Netstorm.Client.Display.ShapeDatabase;

// 실제 보호TYPE/SHP 캡처부터 source/입자/원문dirty까지 같은 사슬로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>양판본426입력과 공유 현재바람의 같은 전체 프레임 대조다.</summary>
public sealed class BurningFrameOracleTests
{
    /// <summary>창/장치 없이 같은 실제 판본의 보호 자산을 읽는다.</summary>
    private sealed class Assets
    {
        /// <summary>현재 판본TYPE표다.</summary>
        public IReadOnlyList<OriginalTypeRecord> Types { get; }
        /// <summary>실제36바이트 헤더로 구성한 표시 셰이프다.</summary>
        public SquidDisplayShape[] Shapes { get; }
        /// <summary>기존 실제VFX RLE 데이터베이스다.</summary>
        public VfxDatabase Database { get; }
        /// <summary>원본과 같은 로딩순서/클러스터/실제SHP 헤더를 연결한다.</summary>
        public Assets(OriginalEdition edition)
        {
            DirectoryInfo? directory = new(AppContext.BaseDirectory);
            // 보호 원본의 저장소 루트만 찾으며 파일/설정을 쓰거나 게임을 실행하지 않는다.
            while (directory is not null && !File.Exists(Path.Combine(directory.FullName, "AGENTS.md"))) directory = directory.Parent;
            Assert.NotNull(directory); string root = Path.Combine(directory!.FullName, edition == OriginalEdition.Patch1078 ? "originals" : "originalCD");
            GameFileSystem files = GameFileSystem.Open(root); IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition); var sources = new RiftTypeSource[order.Count];
            // 원본 로딩목록의 실제 정의만 읽어 같은TYPE표를 만든다.
            for (int i = 0; i < order.Count; i++)
            {
                byte[] raw = files.ReadAllBytes("d/" + order[i] + ".type");
                string text = raw.AsSpan().StartsWith((ReadOnlySpan<byte>)[0xef, 0xbb, 0xbf]) ? Encoding.UTF8.GetString(raw, 3, raw.Length - 3) : OriginalText.DecodeWindows1252(raw);
                sources[i] = new(order[i], RiftTypeDefinition.Parse(text));
            }
            Types = new RiftTypeTable(edition, sources).Types; Assert.Equal(20, Types[70].MaxHitPoints); Assert.NotEqual(0u, Types[70].Flags1 & 0x10); Assert.NotEqual(0u, Types[70].Flags2 & 0x400);
            Database = new(files.ReadAllBytes("d/_shapes.shp")); Shapes = Enumerable.Range(0, Types.Count).Select(_ => new SquidDisplayShape()).ToArray();
            // SquidRenderer.Shapes와 같은 클러스터 개수/물리 프레임 헤더순서다.
            for (int asset = 0; asset < sources.Length; asset++)
            {
                SquidDisplayShape shape = Shapes[70 + asset]; shape.FrameCount = sources[asset].Definition!.Clusters.Count; shape.Loaded = true;
                // 공유/그림자 레이어도 실제 물리프레임 번호를 유지한다.
                for (int frame = 0; frame < Database.Blocks()[asset].Frames.Count; frame++)
                {
                    Netstorm.Client.Display.SquidFrameMetrics m = Database.SquidMetrics(asset, frame);
                    shape.Frames.Add(new() { Width = m.Width, Height = m.Height, HotspotX = m.HotspotX, HotspotY = m.HotspotY, CellWidth = m.CellWidth, CellHeight = m.CellHeight });
                }
            }
        }
    }
    /// <summary>독립 판본 자산은 불변으로 공유한다.</summary>
    private static readonly Lazy<Assets> PatchAssets = new(() => new(OriginalEdition.Patch1078));
    /// <summary>CD도 실제 별도TYPE/SHP만 읽는다.</summary>
    private static readonly Lazy<Assets> CdAssets = new(() => new(OriginalEdition.Cd1072));
    /// <summary>기존 원문 변경표 병합을 같은1024×768 화면에 연결한다.</summary>
    private sealed class Sink : SquidDisplaySink
    {
        /// <summary>출력까지 원문순서로 보존하는 실제 변경표다.</summary>
        public DirtyRegions Regions { get; } = new(1024, 768);
        /// <summary>변경표의 현재 넘침/전체표식이다.</summary>
        public override bool Suppressed() => Regions.FullRedraw;
        /// <summary>실제 공통 변경표로 원문영역/flags를 넘긴다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => Regions.Add(new(rect.Left, rect.Top, rect.Right, rect.Bottom), flags);
    }
    /// <summary>현재 입자의 여섯DWORD를 원문24바이트로 직렬화한다.</summary>
    private static string Particles(BurningVisualFields fields)
    {
        byte[] bytes = new byte[fields.Particles.Length * 24];
        // 호스트padding 없이 실제 의미필드/source표식을 보존한다.
        for (int i = 0; i < fields.Particles.Length; i++)
        {
            BurningParticle p = fields.Particles[i]; Span<byte> raw = bytes.AsSpan(i * 24, 24); BinaryPrimitives.WriteSingleLittleEndian(raw, p.X); BinaryPrimitives.WriteSingleLittleEndian(raw[4..], p.Y);
            BinaryPrimitives.WriteSingleLittleEndian(raw[8..], p.VelocityX); BinaryPrimitives.WriteSingleLittleEndian(raw[12..], p.VelocityY); BinaryPrimitives.WriteSingleLittleEndian(raw[16..], p.Life); BinaryPrimitives.WriteUInt32LittleEndian(raw[20..], p.Source ? 1u : 0u);
        }
        return Hex(bytes);
    }
    /// <summary>빈 버퍼의 원문 표기를 유지한다.</summary>
    private static string Hex(byte[] bytes) => bytes.Length == 0 ? "-" : Convert.ToHexStringLower(bytes);
    /// <summary>원문 단정도 입력을 비트 그대로 읽는다.</summary>
    private static float F(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>현재16바이트 영역/메타데이터·24바이트timer·dirty표 전체를 원문 순서로 반환한다.</summary>
    private static string[] Output(BurningVisualFields f, BurningFrameFields frame, GameRandom random, Sink sink, bool captured)
    {
        byte[] rect = new byte[16], metadata = new byte[16], timer = new byte[24];
        BinaryPrimitives.WriteSingleLittleEndian(rect, f.Registration.Left); BinaryPrimitives.WriteSingleLittleEndian(rect.AsSpan(4), f.Registration.Top); BinaryPrimitives.WriteSingleLittleEndian(rect.AsSpan(8), f.Registration.Right); BinaryPrimitives.WriteSingleLittleEndian(rect.AsSpan(12), f.Registration.Bottom);
        BinaryPrimitives.WriteDoubleLittleEndian(timer, f.LastTime); BinaryPrimitives.WriteDoubleLittleEndian(timer.AsSpan(8), f.Elapsed); BinaryPrimitives.WriteSingleLittleEndian(timer.AsSpan(16), frame.Delta); BinaryPrimitives.WriteSingleLittleEndian(timer.AsSpan(20), frame.ScaledDelta);
        BinaryPrimitives.WriteInt32LittleEndian(metadata, f.Image.Width); BinaryPrimitives.WriteInt32LittleEndian(metadata.AsSpan(4), f.Image.Height); BinaryPrimitives.WriteInt32LittleEndian(metadata.AsSpan(8), f.Image.HotspotX); BinaryPrimitives.WriteInt32LittleEndian(metadata.AsSpan(12), f.Image.HotspotY);
        string dirty = string.Join(';', sink.Regions.Entries.Select(e => $"{e.Rect.Left},{e.Rect.Top},{e.Rect.Right},{e.Rect.Bottom},{e.Flags}"));
        return [Hex(rect), f.Registration.ZOrder.ToString(), f.Total.ToString(), Hex(timer), f.Stage.ToString(), Hex(metadata), Hex(f.Image.Pixels), Hex(f.Rows), f.Selected.ToString(), f.Off ? "1" : "0", Particles(f), random.State().ToString(), sink.Regions.FullRedraw ? "1" : "0", dirty.Length == 0 ? "-" : dirty, captured ? $"{f.Image.Pixels.Length},{f.Rows.Length}" : "-"];
    }
    /// <summary>현재 원본 입력만 준비하며 결과열을 실제캡처 입력으로 쓰지 않는다.</summary>
    private static string[] Run(string[] row, bool sharedWind, BurningVictimState? victim = null)
    {
        bool patch = row[0] == "originals"; OriginalEdition edition = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; Assets assets = patch ? PatchAssets.Value : CdAssets.Value;
        var pool = new RawSidPool(32768, true, edition: edition);
        // 원문 피해50까지 같은 실제 client번호로 확보한다.
        for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client));
        pool.RawSlot(50).Clear(); BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(50), patch ? 0x508cc8u : 0x5072f0u); pool.RawSlot(50)[10] = 70;
        BinaryPrimitives.WriteSingleLittleEndian(pool.RawSlot(50)[14..], 20.75f); BinaryPrimitives.WriteSingleLittleEndian(pool.RawSlot(50)[18..], 21.9f);
        if (patch) BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(50)[26..], 10); else BinaryPrimitives.WriteInt16LittleEndian(pool.RawSlot(50)[26..], 10);
        pool.RawSlot(50)[patch ? 35 : 33] = unchecked((byte)X86Fixture.Int(row[9]));
        if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(50)[36..], X86Fixture.UInt(row[10])); else pool.RawSlot(50)[34] = (byte)X86Fixture.UInt(row[10]);
        var capture = new RawShapeToBuffer(pool, assets.Types, assets.Shapes, assets.Database, new() { Zoom = unchecked((int)X86Fixture.UInt(row[11])) });
        var random = new GameRandom(X86Fixture.UInt(row[6])); var registry = new RawRenderRegistry(pool, new() { Reserve = _ => throw new InvalidOperationException("프레임 미선택 등록확보"), ReleaseArray = () => Assert.Fail("프레임 미선택 반납"), Attach = (_, _, _, _) => Assert.Fail("프레임 미선택 부착") });
        var visual = new RawBurningVisual(pool, registry, new(), new() { Reserve = _ => throw new InvalidOperationException("프레임 미선택 입자확보") }); var sourceState = new BurningSourcesState(); var sources = new RawBurningSources(pool, assets.Types, visual, random, sourceState);
        var state = new BurningFrameState { Now = X86Fixture.DoubleBits(row[7]), PixelScaleX = X86Fixture.FloatBits(row[12]), PixelScaleY = X86Fixture.FloatBits(row[12]), CameraX = .25f, CameraY = .5f, Wind = X86Fixture.FloatBits(row[8]), DeltaMultiplier = .3f, Viewport = new(0, 0, 1024, 768) };
        float originalWind = state.Wind, liveWind = 7; if (sharedWind) { state.Wind = -13; state.SharedWind = () => liveWind; }
        var sink = new Sink(); bool captured = false; BurningFrameHooks hooks = BurningFrameCapture.Make(capture, pool); Func<int, ConstructionVisualImage> actual = hooks.Capture!; hooks.Capture = sid => { captured = true; return actual(sid); };
        var logic = new RawBurningFrame(pool, assets.Types, visual, sources, random, state, sink, hooks); if (sharedWind) liveWind = originalWind;
        var fields = new BurningVisualFields { Capacity = X86Fixture.UInt(row[1]), LastTime = 1, Elapsed = .123456789, Off = row[3] != "0", Victim = 50 }; fields.Particles = new BurningParticle[fields.Capacity]; fields.Registration.ZOrder = 1234;
        bool visible = row[4] != "0"; fields.Registration.Left = visible ? 12.25f : 1100.25f; fields.Registration.Top = visible ? 13.75f : 1100.75f; fields.Registration.Right = visible ? 30.5f : 1120.5f; fields.Registration.Bottom = visible ? 35.5f : 1125.5f;
        var frame = new BurningFrameFields { Delta = 123, ScaledDelta = -456 }; uint stage = X86Fixture.UInt(row[2]);
        if (fields.Capacity != 0 && stage != 0) logic.Capture(fields);
        fields.Stage = stage; fields.Victim = (ushort)X86Fixture.UInt(row[13]); uint pattern = X86Fixture.UInt(row[5]);
        if (fields.Capacity != 0 && stage != 0)
        {
            fields.Total = pattern == 1 ? Math.Min(fields.Capacity, 2u) : pattern == 2 ? Math.Min(fields.Capacity, 3u) : pattern == 3 ? Math.Min(fields.Capacity, 4u) : 0; fields.Selected = pattern != 0 ? 1 : 0;
            uint[] exceptional = [0x7fc00001, 0xffc00001, 0x7f800000, 0x80000000];
            // source좌표/이동/예외life는 기대값과 구별하는 원문 입력이다.
            for (int i = 0; i < fields.Particles.Length; i++)
            {
                bool source = pattern == 1 && i < 2; float x = source ? (i != 0 ? 7 : 2) : 25.125f + i, y = source ? 3 : 30.375f + i;
                float life = source ? 9999 : pattern == 2 ? (i == 0 ? 5.25f : i == 1 ? .1f : 0) : pattern == 3 && i < 4 ? F(exceptional[i]) : 0;
                fields.Particles[i] = new(x, y, .1f, -1.25f, life, source); if (source) { int group = ((int)x + (int)y) / 5; if (group < fields.Rows.Length) fields.Rows[group] = 1; }
            }
        }
        if (victim is not null)
        {
            state.Victim.Checking = victim.Checking; state.Victim.Robust = victim.Robust; state.Victim.Assertion = victim.Assertion;
            sourceState.Victim.Checking = victim.Checking; sourceState.Victim.Robust = victim.Robust; sourceState.Victim.Assertion = victim.Assertion;
        }
        logic.Step(fields, frame); return Output(fields, frame, random, sink, captured);
    }
    /// <summary>두 판본의 실제 캡처/프레임213행씩을 공유바람이 바뀐 별도실행에서도 대조한다.</summary>
    [Theory]
    [InlineData("originals", false)]
    [InlineData("originalCD", false)]
    [InlineData("originals", true)]
    [InlineData("originalCD", true)]
    public void ActualProtectedCaptureAndFullFrameMatchOriginal(string edition, bool sharedWind)
    {
        string[][] rows = X86Fixture.Read("burningframe", edition); Assert.Equal(213, rows.Length);
        X86Fixture.CheckRows(rows, row => { Assert.Equal(row[14..29], Run(row, sharedWind)); return true; });
    }

    /// <summary>같은현재진단을 실제Capture/Frame/Sources 소비자에 연결해 원본전체출력과 복귀순서를 함께 대조한다.</summary>
    [Theory]
    [InlineData(0u)]
    [InlineData(1u)]
    [InlineData(2u)]
    public void CurrentVictimDiagnosticsReachAllThreeActualFrameConsumers(uint stage)
    {
        string[] row = X86Fixture.Read("burningframe", "originals").First(r => r[1] != "0" && X86Fixture.UInt(r[2]) == stage && (stage != 2 || (r[4] == "1" && r[5] == "0")));
        uint checking = 0x80000000, calls = 0; var lines = new List<uint>(); var victim = new BurningVictimState { Checking = new(() => checking) };
        victim.Robust = (sid, live) => { Assert.Equal(50, sid); Assert.True(live); return (++calls % 2) == 0; };
        victim.Assertion = (expression, file, line) => { Assert.Equal("isSidRobust(burningSid)", expression); Assert.Equal(@".\Burning.cpp", file); lines.Add(line); };
        Assert.Equal(row[14..29], Run(row, false, victim));
        uint[] expected = stage == 0 ? [0x6f] : stage == 1 ? [0x11f] : [0x11f, 0x91]; Assert.Equal(expected, lines); Assert.Equal((uint)expected.Length * 2, calls); Assert.Equal(0x80000000u, victim.Checking.Read());
    }

    /// <summary>실제 보호캡처 어댑터는 같은풀만 받으며 입력의기존함수/소리장치를 값사본으로 보존한다.</summary>
    [Fact]
    public void ActualCaptureAdapterCopiesSoundAndRejectsDifferentPool()
    {
        Assets assets = PatchAssets.Value; var pool = new RawSidPool(32768, false); int sid = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 70;
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0x508cc8); var capture = new RawShapeToBuffer(pool, assets.Types, assets.Shapes, assets.Database, new()); int stopped = 0;
        Func<int, ConstructionVisualImage> old = _ => throw new InvalidOperationException("교체 전 캡처"); var input = new BurningFrameHooks { Capture = old, StopSound = _ => stopped++ };
        BurningFrameHooks bound = BurningFrameCapture.Make(capture, pool, input); Assert.Same(old, input.Capture); Assert.Same(input.StopSound, bound.StopSound); input.StopSound = null;
        Assert.NotEmpty(bound.Capture!(sid).Pixels); bound.StopSound!(55); Assert.Equal(1, stopped);
        Assert.Throws<ArgumentException>(() => BurningFrameCapture.Make(capture, new RawSidPool(32768, false), input));
    }
}
