using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Tests; // 원본 fixture와 장치 독립 표시 계약을 대조하는 검사 영역이다.

/// <summary>SpriteDraw의 원본 명령열·현재 전역 부작용·실제 8비트 픽셀 경계를 검증한다.
/// 근거: cpppj/tests/SpriteDrawTests.cpp. 월드 수집/정렬/실자산 어댑터의 통합 검사는 R4·R5에서 잇는다.</summary>
public sealed class SpriteDrawTests
{
    /// <summary>원본 fixture에서 SID 5를 가리키는 raw 주소 토큰.</summary>
    private const uint SpriteRaw = 0x16001000;

    /// <summary>packed 필드에 지정 폭의 하위 바이트만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 주변 packed 필드를 건드리지 않고 low byte부터 쓴다.
        for (int i = 0; i < width; i++) raw[offset + i] = unchecked((byte)(value >> (i * 8)));
    }

    /// <summary>원본 이벤트의 모든 인자를 DWORD 십진수로 손실 없이 직렬화한다.</summary>
    private static string Event(string name, params uint[] values) => name + "," + string.Join(',', values.Select(v => v.ToString(CultureInfo.InvariantCulture)));

    /// <summary>부호 있는 화면 좌표를 같은 DWORD 비트로 직렬화한다.</summary>
    private static uint Bits(int value) => unchecked((uint)value);

    /// <summary>실제 raw 슬롯 5~8을 확보한다. CD 관측도 필드 입력만 공통 50바이트 저장소에 적재하며 CD 풀 자체 검증으로 세지 않는다.</summary>
    private static RawSidPool Pool(bool patch = true)
    {
        var pool = new RawSidPool(24000, server: false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        // 본체5·표면8의 주소를 원본 fixture SID와 일치시킨다.
        for (int i = 5; i <= 8; i++) Assert.Equal(i, pool.Allocate(SidAllocation.Client));
        return pool;
    }

    /// <summary>세 판본 각각 463행의 전체 이벤트·선택 SID·alias 쓰기를 원본 그대로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeCommands(string edition)
    {
        bool patch = edition == "originals";
        RawSidPool pool = Pool(patch);
        string[][] rows = X86Fixture.Read("spritedraw", edition);
        Assert.Equal(463, rows.Length);
        // 기대 분기/픽셀을 C#으로 재계산하지 않고 원본이 관측한 호출열과 비교한다.
        foreach (string[] row in rows)
        {
            uint[] v = row[2].Split(',').Select(X86Fixture.UInt).ToArray();
            // signed DWORD 입력을 같은 비트로 복원한다.
            int Integer(int i) => unchecked((int)v[i]);
            var s = new SpriteDrawState
            {
                CameraX = Integer(5), CameraY = Integer(6), LocalPlayer = v[15], SpecialMode = v[17], FilteredFlags = v[18],
                WideOwnerType = v[14] != 0 ? 70U : 90U, UnfilteredType = v[21] != 0 ? 70U : 90U,
                ShadowAliasType = v[36], ShadowAliasPinned = v[37] != 0, QuarterHitPoints = v[35] != 0, SelectionDisabled = v[28] != 0,
                MarkerEnabled = v[24] != 0, MarkerVisible = v[25] != 0, MarkerType = 71, MarkerExcludeType = 73, StatusType = 75, AbstractStatusType = 76,
                CostType = v[46] != 0 ? 70U : 90U, TimerProcessType = 40, StatusProcessType = 41, Now = 100.25,
                OwnerMaps = new byte[91], OwnerPalettes = new int[301], SelectionColors = [180, 181, 182, 183], HitPointColors = [100, 101, 102], BarBackground = 9,
            };
            // 서로 다른 타이머 전역은 같은 타입 번호도 허용한다.
            for (int i = 0; i < 3; i++) s.TimerTypes[i] = v[44] == i + 1 || v[44] == 4 ? 70U : 90U;
            s.OwnerMaps[70] = unchecked((byte)v[16]);
            // 원본 입력과 같은 플레이어별 팔레트 번호다.
            for (int i = 0; i < s.OwnerPalettes.Length; i++) s.OwnerPalettes[i] = i % 3 + 1;
            OriginalTypeRecord[] types = new OriginalTypeRecord[91];
            SpriteTypeView[] views = new SpriteTypeView[91];
            // 물리 프레임 64개에 동일한 원본 측정 헤더를 준비한다.
            for (int i = 0; i < views.Length; i++)
            {
                types[i] = new OriginalTypeRecord();
                views[i] = new SpriteTypeView { GumpFrame = v[23], Shape = new() { Loaded = true, FrameCount = Integer(12) } };
                // 헤더 입력의 signed16 폭을 유지한다.
                for (int frame = 0; frame < 64; frame++) views[i].Shape.Frames.Add(new SquidDisplayFrame
                {
                    Width = unchecked((short)Integer(29)), Height = unchecked((short)Integer(30)),
                    HotspotX = unchecked((short)Integer(31)), HotspotY = unchecked((short)Integer(32)),
                });
            }
            views[70].Shape.Loaded = v[11] != 0;
            views[74].Capacity = Integer(42); views[74].ConstructionTime = Integer(39); views[75].Duration = 30; views[76].Duration = 20;
            types[70] = new OriginalTypeRecord
            {
                HotspotX = Integer(7), HotspotY = Integer(8), FootX = Integer(9), FootY = Integer(10), Flags1 = v[19], Flags2 = v[20],
                MaxHitPoints = Integer(34), Cost = BitConverter.UInt32BitsToSingle(v[47]),
            };
            Span<byte> raw = pool.AllocatedBytes(5);
            raw.Clear(); raw[10] = 70; Put(raw, 14, v[3]); Put(raw, 18, v[4]);
            Put(raw, 22, Bits((int)((double)BitConverter.UInt32BitsToSingle(v[3]) * 16 + .5)) & 0xffff, 2);
            Put(raw, 24, Bits((int)((double)BitConverter.UInt32BitsToSingle(v[4]) * 11 + .5)) & 0xffff, 2);
            Put(raw, patch ? 36 : 34, v[2], patch ? 4 : 1); raw[patch ? 40 : 35] = unchecked((byte)v[1]);
            raw[patch ? 34 : 32] = unchecked((byte)v[13]); Put(raw, 12, v[14] != 0 ? v[13] : v[48], 2); Put(raw, 26, v[33], patch ? 4 : 2);
            if (patch) { Put(raw, 42, 74); Put(raw, 46, v[40]); }
            pool.AllocatedBytes(8)[10] = (byte)(v[27] != 0 ? 73 : 72);
            byte[] before = raw.ToArray();
            if (v[49] != 0)
            {
                var processes = new SpriteStatusProcess[40000];
                Array.Fill(processes, new SpriteStatusProcess());
                processes[0] = new() { Type = 41, Flags = 1, Members = [5], EndTime = 999 };
                processes[1] = new() { Type = 41, Flags = 3, Members = [6, 5, 5], EndTime = 111.9 };
                processes[2] = new() { Type = 41, Flags = 2, Members = [5], EndTime = 122.8 };
                processes[3] = new() { Type = 41, Flags = 1, Members = [5], EndTime = 99 };
                processes[4] = new() { Type = 42, Flags = 1, Members = [5], EndTime = 900 };
                processes[5] = new() { Type = 41, Flags = 2, Members = [6], EndTime = 900 };
                processes[39999] = new() { Type = 41, Flags = 1, Members = [5], EndTime = v[49] == 2 ? 120.6 : 104 };
                s.Processes = processes;
            }
            var events = new List<string>();
            var hash = new RawSquidHash();
            int ix = (int)((double)BitConverter.UInt32BitsToSingle(v[3]) + (double).9999F);
            int iy = (int)((double)BitConverter.UInt32BitsToSingle(v[4]) + (double).9999F);
            if (ix >= 0 && ix < 256 && iy >= 0 && iy < 256) hash.Cell(0, ix, iy) = (ushort)(v[26] != 0 ? 8 : 0);
            var hooks = new SpriteDrawHooks
            {
                SetOwnerColors = (t, owner) => events.Add(Event("M", t, owner)),
                Shape = r => events.Add(Event("S", r.Type, r.Frame, Bits(r.Position.X), Bits(r.Position.Y), (uint)r.Mode)),
                Line = (a, b, color) => events.Add(Event("L", Bits(a.X), Bits(a.Y), Bits(b.X), Bits(b.Y), color)),
                Fill = (r, color) => events.Add(Event("R", Bits(r.Left), Bits(r.Top), Bits(r.Right), Bits(r.Bottom), color)),
                VirtualFrame = sid => { events.Add(Event("F", (uint)sid)); return Integer(22); },
                VirtualDraw = (sid, p) => events.Add(Event("V", (uint)sid, Bits(p.X), Bits(p.Y))),
                Surface = SpriteDraw.MakeSpriteSurfaceLookup(hash),
                ConstructionActive = sid => { Assert.Equal(5, sid); return v[38] != 0; },
                Resource = sid => { Assert.Equal(5, sid); return v[41] != 0 ? new(74, Integer(43)) : null; },
                Timers = (sid, t) => { Assert.Equal(5, sid); Assert.Equal(40U, t); return v[45] != 0 ? [119.99, 160.99, 151.01] : null; },
            };
            var draw = new SpriteDraw(pool, types, views, s, hooks, SpriteRaw - 5 * RawSidPool.Stride);
            draw.Draw(new(BitConverter.UInt32BitsToSingle(v[3]), BitConverter.UInt32BitsToSingle(v[4]), SpriteRaw, 0, 0, unchecked((byte)v[0])));
            string observed = events.Count == 0 ? "-" : string.Join(';', events);
            Assert.True(row[3] == observed, $"{edition}/{row[1]}\n실제: {observed}\n원본: {row[3]}");
            Assert.Equal(X86Fixture.UInt(row[4]), s.SelectedSid);
            Assert.Equal(X86Fixture.UInt(row[5]), unchecked(0x17000000U + (s.ShadowShapeType ?? s.ShadowAliasType) * 0x1000U));
            Assert.Equal(before, pool.Slot(5).ToArray());
        }
    }

    /// <summary>현재 상태의 최소 합성 raw/SHP 장면. 모든 기본 전역은 출력 대상70과 충돌하지 않게 둔다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 SID5를 유지하는 실제 raw 저장소.</summary>
        public RawSidPool Pool { get; } = SpriteDrawTests.Pool();
        /// <summary>전체 타입 속성 표.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 71).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>전체 표시 헤더 표.</summary>
        public SpriteTypeView[] Views { get; } = Enumerable.Range(0, 71).Select(_ => new SpriteTypeView()).ToArray();
        /// <summary>기본 전역 입력.</summary>
        public SpriteDrawState State { get; } = new() { OwnerMaps = new byte[71], OwnerPalettes = [0], SelectionColors = [7], ShadowAliasType = 69, TimerTypes = [68, 68, 68], CostType = 68 };
        /// <summary>월드 .25/.25를 화면4/3에 투영하는 기본 슬롯을 채운다.</summary>
        public Scene()
        {
            Span<byte> raw = Pool.AllocatedBytes(5); raw.Clear(); raw[10] = 70;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(.25F)); Put(raw, 18, BitConverter.SingleToUInt32Bits(.25F)); Put(raw, 22, 4, 2); Put(raw, 24, 3, 2);
            Views[70].Shape.Loaded = true; Views[70].Shape.FrameCount = 1;
            Views[70].Shape.Frames.Add(new() { Width = 4, Height = 4, HotspotX = 1, HotspotY = 1 });
        }
        /// <summary>같은 저장소/전역으로 그리기 본체를 만든다.</summary>
        public SpriteDraw Draw(SpriteDrawHooks hooks) => new(Pool, Types, Views, State, hooks, SpriteRaw - 250);
    }

    /// <summary>투명·불투명0/255를 포함하는 실제 VFX RLE 입력.</summary>
    private static ShapeDatabase PixelShape()
    {
        byte[] raw = new byte[46];
        Put(raw, 0, 0x30312e31); Put(raw, 4, 1); Put(raw, 8, 16); Put(raw, 32, 2);
        new byte[] { 1, 1, 5, 0, 255, 0 }.CopyTo(raw, 40);
        return new ShapeDatabase(raw);
    }

    /// <summary>실제 raw→Draw→SHP 디코딩→픽셀 합성을 하나의 경로로 검사한다.</summary>
    [Fact]
    public void RawDrawWritesPixels()
    {
        var scene = new Scene();
        ShapeDatabase database = PixelShape();
        byte[] pixels = Enumerable.Repeat((byte)9, 64).ToArray();
        var target = new SpritePixelTarget(pixels, 8, 8, new(0, 0, 8, 8), type => { Assert.Equal(70U, type); return new(database, 0); }, null);
        scene.Draw(target.Hooks()).Draw(new(.25F, .25F, SpriteRaw, 0, 0, 0));
        Assert.Equal(9, pixels[28]); Assert.Equal(0, pixels[29]); Assert.Equal(255, pixels[30]);
    }

    /// <summary>소유자 색·명시 그림자는 소스 색을 치환하며 투명·클립·소유자 문맥 오류를 유지한다.</summary>
    [Fact]
    public void PixelModesTransparencyClip()
    {
        ShapeDatabase database = PixelShape();
        byte[] pixels = Enumerable.Repeat((byte)9, 32).ToArray();
        byte[]? returnedMap = null;
        var target = new SpritePixelTarget(pixels, 8, 4, new(1, 1, 5, 3), _ => new(database, 0), (mode, _, _) =>
        {
            var map = new byte[256];
            // 불투명 0/255도 같은 변환 표를 통과한다.
            for (int i = 0; i < map.Length; i++) map[i] = unchecked((byte)(i + (int)mode * 10));
            returnedMap = map;
            return map;
        });
        SpriteDrawHooks hooks = target.Hooks();
        hooks.Shape!(new(70, 0, new(1, 1), SpriteShapeMode.Plain));
        Assert.Equal(9, pixels[9]); Assert.Equal(0, pixels[10]); Assert.Equal(255, pixels[11]);
        Assert.Throws<InvalidOperationException>(() => hooks.Shape(new(70, 0, new(1, 1), SpriteShapeMode.Owner)));
        hooks.SetOwnerColors!(70, 2); returnedMap![0] = 77;
        hooks.Shape(new(70, 0, new(1, 1), SpriteShapeMode.Owner));
        Assert.Equal(10, pixels[10]); Assert.Equal(9, pixels[11]);
        hooks.Shape(new(70, 0, new(1, 1), SpriteShapeMode.Shadow));
        Assert.Equal(40, pixels[10]); Assert.Equal(39, pixels[11]); Assert.Equal(9, pixels[9]);
        hooks.Shape(new(70, 0, new(4, 2), SpriteShapeMode.Dead)); Assert.Equal(9, pixels[21]);
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Shape(new(70, 1, new(1, 1), SpriteShapeMode.Plain)));
    }

    /// <summary>선 끝점·역순·클립 및 half-open fill의 빈/역전 영역을 검사한다.</summary>
    [Fact]
    public void PixelLinesAndBars()
    {
        byte[] pixels = Enumerable.Repeat((byte)9, 64).ToArray();
        SpriteDrawHooks hooks = new SpritePixelTarget(pixels, 8, 8, new(2, 2, 6, 6), null, null).Hooks();
        hooks.Line!(new(7, 3), new(1, 3), 17);
        Assert.Equal(17, pixels[26]); Assert.Equal(17, pixels[29]); Assert.Equal(9, pixels[25]); Assert.Equal(9, pixels[30]);
        hooks.Line(new(4, 7), new(4, 1), 18);
        Assert.Equal(18, pixels[20]); Assert.Equal(18, pixels[44]); Assert.Equal(9, pixels[52]);
        hooks.Fill!(new(3, 4, 5, 6), 19);
        Assert.Equal(19, pixels[35]); Assert.Equal(19, pixels[44]); Assert.Equal(9, pixels[45]);
        byte[] before = (byte[])pixels.Clone();
        hooks.Fill(new(5, 4, 3, 6), 99); hooks.Fill(new(3, 5, 5, 5), 99);
        Assert.Equal(before, pixels);
        Assert.Throws<ArgumentException>(() => hooks.Line(new(2, 2), new(3, 3), 4));
    }

    /// <summary>null shape의 조기 반환·좌표·token·빠진 출력 경계·그림자 alias 선행 쓰기를 검사한다.</summary>
    [Fact]
    public void ContractsAndEarlyReturns()
    {
        var scene = new Scene();
        SpriteDraw draw = scene.Draw(new());
        var entry = new RenderCollectionEntry(0, 0, SpriteRaw, 0, 0, 0);
        scene.Views[70].Shape.Loaded = false; draw.Draw(entry);
        scene.Views[70].Shape.Loaded = true;
        Assert.Throws<ArgumentException>(() => draw.Draw(entry));
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Draw(entry with { Token = SpriteRaw + 1 }));
        Put(scene.Pool.AllocatedBytes(5), 22, 5, 2); Assert.Throws<ArgumentException>(() => draw.Draw(entry)); Put(scene.Pool.AllocatedBytes(5), 22, 4, 2);
        entry = entry with { Tail = 4 }; scene.State.SpecialMode = 1; draw.Draw(entry); Assert.Equal(70U, scene.State.ShadowShapeType);
        scene.State.ShadowShapeType = null; scene.State.ShadowAliasPinned = true; draw.Draw(entry); Assert.Null(scene.State.ShadowShapeType);
        scene.State.SpecialMode = 0; scene.Pool.AllocatedBytes(5)[40] = 0x10; draw.Draw(entry); Assert.Null(scene.State.ShadowShapeType);
    }

    /// <summary>가상 Draw가 쓴 선택 비트를 후속 선택선이 현재 raw에서 읽는다.</summary>
    [Fact]
    public void CurrentRawAfterVirtualDraw()
    {
        var scene = new Scene(); scene.Pool.AllocatedBytes(5)[40] = 2;
        int lines = 0;
        SpriteDraw draw = scene.Draw(new()
        {
            VirtualFrame = _ => -1, VirtualDraw = (_, _) => scene.Pool.AllocatedBytes(5)[40] = 4,
            Line = (_, _, _) => lines++, Resource = _ => null,
        });
        draw.Draw(new(0, 0, SpriteRaw, 0, 0, 0)); Assert.Equal(8, lines); Assert.Equal(5U, scene.State.SelectedSid);
    }

    /// <summary>표면 조회의 음수 절삭과 월드 경계, 유한값 계약을 고정한다.</summary>
    [Fact]
    public void SurfaceLookupBoundaries()
    {
        var hash = new RawSquidHash(); hash.Cell(0, 0, 0) = 5; hash.Cell(0, 1, 1) = 6;
        Func<float, float, int> lookup = SpriteDraw.MakeSpriteSurfaceLookup(hash);
        Assert.Equal(5, lookup(0, 0)); Assert.Equal(6, lookup(.25F, .25F)); Assert.Equal(5, lookup(-.25F, -.25F));
        Assert.Equal(0, lookup(-2, -2)); Assert.Equal(0, lookup(256, 256));
        Assert.Throws<ArgumentOutOfRangeException>(() => lookup(float.PositiveInfinity, 0));
    }

    /// <summary>가상 frame의 효과가 특수 필터의 다음 분기에 즉시 반영된다.</summary>
    [Fact]
    public void CurrentFilterAfterVirtualFrame()
    {
        var scene = new Scene(); scene.Pool.AllocatedBytes(5)[40] = 2; scene.Types[70].Flags2 = 8; scene.Views[70].GumpFrame = 7;
        var drawn = new List<SpriteShapeRequest>();
        scene.Draw(new()
        {
            VirtualFrame = _ => { scene.State.SpecialMode = 1; scene.State.FilteredFlags = 8; return -1; }, Shape = drawn.Add,
        }).Draw(new(0, 0, SpriteRaw, 0, 0, 0));
        SpriteShapeRequest shape = Assert.Single(drawn); Assert.Equal(7U, shape.Frame); Assert.Equal(SpriteShapeMode.Abstract, shape.Mode);
    }

    /// <summary>공유 진단은 독립 bool을 대신하며 정확0일 때만 매 호출의 alias를 갱신한다.</summary>
    [Fact]
    public void SharedDiagnosticControlsCurrentShadowAlias()
    {
        var scene = new Scene(); uint diagnostic = 6; int shapes = 0;
        scene.State.SharedPatchDiagnostic = () => diagnostic; scene.State.ShadowAliasPinned = true;
        SpriteDraw draw = scene.Draw(new() { Shape = _ => shapes++ });
        // 실행 중 바뀐 동일 DWORD를 매 호출 읽는다.
        foreach (uint value in new uint[] { 6, 0, 1, uint.MaxValue, 0 })
        {
            diagnostic = value; scene.State.ShadowShapeType = null;
            draw.Draw(new(0, 0, SpriteRaw, 0, 0, 4));
            Assert.Equal(value == 0, scene.State.ShadowShapeType.HasValue);
        }
        Assert.Equal(5, shapes); Assert.True(scene.State.ShadowAliasPinned);
    }

    /// <summary>alias는 물리 헤더/loaded를 빌리지만 원래 타입의 논리 frameCount 선택 검사는 유지한다.</summary>
    [Fact]
    public void ShadowAliasUsesCurrentPhysicalShape()
    {
        var scene = new Scene(); scene.Views[69].Shape.FrameCount = 1;
        var lines = new List<ScreenPoint>(); var drawn = new List<SpriteShapeRequest>();
        SpriteDraw draw = scene.Draw(new()
        {
            Shape = drawn.Add, Line = (first, _, _) => lines.Add(first), ConstructionActive = _ => false, Resource = _ => null,
        });
        draw.Draw(new(0, 0, SpriteRaw, 0, 0, 4)); Assert.Equal(70U, scene.State.ShadowShapeType);
        scene.Pool.Slot(5).CopyTo(scene.Pool.AllocatedBytes(6));
        Span<byte> alias = scene.Pool.AllocatedBytes(6); alias[10] = 69; alias[40] = 4; Put(alias, 42, 70);
        draw.Draw(new(0, 0, SpriteRaw + 50, 0, 0, 0));
        Assert.Equal(2, drawn.Count); Assert.Equal(69U, drawn[1].Type);
        Assert.Equal(8, lines.Count); Assert.Equal(3, lines[0].X); Assert.Equal(6U, scene.State.SelectedSid);
        scene.Views[69].Shape.FrameCount = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => draw.Draw(new(0, 0, SpriteRaw + 50, 0, 0, 0)));
    }

    /// <summary>가상 frame 뒤 공유 소유자/표식 옵션을 다시 읽고 지역 초기값보다 우선한다.</summary>
    [Fact]
    public void SharedOwnerTablesAndBannerAfterVirtualFrame()
    {
        var scene = new Scene(); scene.Pool.AllocatedBytes(5)[34] = 1; scene.Pool.AllocatedBytes(5)[40] = 2;
        scene.Types[70].Flags1 = 0x400; scene.Pool.AllocatedBytes(6)[10] = 69;
        scene.State.MarkerType = 96; scene.State.MarkerExcludeType = 157; scene.State.MarkerVisible = true; scene.State.MarkerEnabled = true;
        scene.State.OwnerPalettes = [0, 1]; scene.State.OwnerMaps[70] = 1;
        uint[] tables = new uint[71]; uint banners = 0; uint requested = 0; int ownerCalls = 0;
        scene.State.SharedOwnerMaps = tables; scene.State.SharedUseBanners = () => banners;
        var drawn = new List<SpriteShapeRequest>();
        SpriteDraw draw = scene.Draw(new()
        {
            VirtualFrame = _ => { tables[70] = banners = requested; return 0; }, VirtualDraw = (_, _) => { },
            Shape = drawn.Add, SetOwnerColors = (_, _) => ownerCalls++, Surface = (_, _) => 6,
        });
        // 지역 입력과 반대인 공유 값을 가상 호출이 다시 바꾸는 경우를 순차 재생한다.
        foreach (uint value in new uint[] { 0, 1, 0, 1 })
        {
            requested = value; tables[70] = banners = 1 - value; drawn.Clear(); ownerCalls = 0;
            draw.Draw(new(0, 0, SpriteRaw, 0, 0, 0));
            Assert.Equal(1 + (int)value, drawn.Count); Assert.Equal((int)value, ownerCalls);
            Assert.Equal(value != 0 ? SpriteShapeMode.Owner : SpriteShapeMode.Plain, drawn[0].Mode);
            if (value != 0) { Assert.Equal(96U, drawn[1].Type); Assert.Equal(0U, drawn[1].Frame); }
        }
    }
}
