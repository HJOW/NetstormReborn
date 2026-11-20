using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Client;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;
using Xunit;

// CPP 도구와 같은 독립 경계 입력으로 원문 위치 판정의 모든 상태/효과를 비교한다.
namespace Netstorm.Client.Tests;

/// <summary>위치/연관 지역/dirty573입력과 callback 재조회·누락 경계 검사다.</summary>
public sealed class CursorPlacementTests
{
    /// <summary>음수 십진 입력도 하위DWORD 그대로 읽는다.</summary>
    private static uint[] Numbers(string value) => value.Split(',').Select(cell => unchecked((uint)long.Parse(cell, CultureInfo.InvariantCulture))).ToArray();
    /// <summary>명시 DWORD를 원래 단정도 비트로 읽는다.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>외부 decoder에 제공하는 독립 좌표다.</summary>
    private static CursorCanonCell[] Cells(uint profile) => profile switch
    {
        2 => [], 4 => [new(0, Float(0x7fc00000), 1.1f)], 5 => [new(0, -1.9998f, -0.9999f), new(1, 254.0002f, 255.0002f)],
        0 => [new(0, 0, 0)], _ => [new(0, 0, 0), new(1, 1.25f, -0.5f), new(2, -2.75f, 3.5f)],
    };
    /// <summary>독립 SHP signed WORD 입력이다.</summary>
    private static CursorCostImage Image(uint profile, int frame) => profile == 3 && frame == 0 ? new(1, -11, -17, 23, -7) : frame switch
    { 0 => new(1, 31, 23, 8, 17), 1 => new(1, 19, 29, -3, 31), 2 => new(1, 47, 13, 7, -5), _ => throw new ArgumentOutOfRangeException(nameof(frame)) };
    /// <summary>하나의 물리 풀과 세 현재 상태를 모든 행에서 공유한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제10.78 client 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>현재 TYPE100/101 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>별도 byte spot 지도가 아닌 실제 ushort SID 지도다.</summary>
        public readonly ushort[] Surface = new ushort[65536];
        /// <summary>같은 내려놓기 전역이다.</summary>
        public readonly CursorDropState Drop = new();
        /// <summary>같은 수명 전역이다.</summary>
        public readonly CursorLifecycleState Life = new();
        /// <summary>같은 위치 전역이다.</summary>
        public readonly CursorPlacementState State = new();
        /// <summary>원문23개 명시 입력이다.</summary>
        public uint[] Input = [];
        /// <summary>전체 실제 경계의 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>독립 투영 결과다.</summary>
        public ScreenPoint Projected;
        /// <summary>같은 client 풀에서100번까지 확보한다.</summary>
        public Scene()
        {
            // 기대값과 독립적으로 실제 슬롯 번호만 준비한다.
            for (int sid = 5; sid <= 100; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
        }
        /// <summary>부호를 DWORD로 저장하는 실제 경계 기록이다.</summary>
        public void Event(string name, params uint[] values) => Events.Add(name + (values.Length == 0 ? "" : ":" + string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)))));
        /// <summary>이전 결과를 제거하고 독립 입력만 초기화한다.</summary>
        public void Prepare(uint[] v)
        {
            Input = v; Drop.Reset(); Life.Reset(); State.Reset(); Events.Clear(); Array.Clear(Surface);
            Surface[0] = 80; Surface[22 * 256 + 21] = 81; Surface[21 * 256 + 20] = 82; Surface[255 * 256 + 255] = 83;
            Drop.Legal = 5; State.PlacementAllowed = v[12]; State.DescriptionAllowed = v[7]; State.SpaceAllowed = unchecked(1 - v[13]); State.SourceAllowed = v[20];
            Life.CurrentGump = v[9] != 0 ? 1u : 0u; Drop.LegalGump = 2; new[] { 3, 7, 43, 51 }.CopyTo(State.Bounds, 0); State.HotspotX = 44; State.HotspotY = 45; State.ImageWidth = 91; State.ImageHeight = 93;
            Drop.Type = Life.AbstractType = 100; Drop.Argument = 7; Drop.Direction = 3; Drop.Preview = 50; Drop.CostFlags = v[15]; State.PlacementFlags = v[16]; Drop.CursorGump = v[17] != 0 ? 1u : 0u;
            Drop.X = Float(v[18]); Drop.Y = Float(v[19]); Drop.LocalPlayer = 0x123401; Drop.ScreenX = -3; Drop.ScreenY = 47;
            State.LabelSpacing = unchecked((int)v[6]); State.Obstruction = v[13]; State.AcceptEvent = 0x123456; State.ScreenOriginX = -17; State.ScreenOriginY = 31; State.Description = "old"; Life.Previews[0] = 100;
            Types[100].FootX = unchecked((int)v[4]); Types[100].FootY = unchecked((int)v[5]); Types[100].Flags1 = v[2]; Types[100].Flags2 = v[3]; Types[101].FootX = 7; Types[101].FootY = 9; Types[101].Flags1 = v[2]; Types[101].Flags2 = 4;
            int[][] sizes = [[35, 17, 49, 23], [-3, -7, -11, -13], [int.MaxValue, unchecked((int)0x80000001), 0x7ffffffd, unchecked((int)0x80000003)]];
            int[] size = sizes[v[22]]; Life.WarningWidth = size[0]; Life.WarningHeight = size[1]; Drop.CursorWidth = size[2]; Life.CostHeight = size[3];
            ScreenPoint[] points = [new(101, 203), new(-3, -7), new(0x7ffffffe, unchecked((int)0x80000002))]; Projected = points[v[22]];
            BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(100)[22..], 0xfff9); BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(100)[24..], 0x8001);
        }
        /// <summary>CPP 어댑터와 같은 외부 decoder/GUI/공간/표시 입력만 공급한다.</summary>
        public CursorPlacementHooks Hooks() => new()
        {
            Describe = (ref string text, float x, float y, uint type, uint owner) => { Event("D", BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), type, owner); text = Input[8] == 0 ? "AB" : Input[8] == 1 ? "" : "ABCDE"; if (Input[21] == 1) Life.AbstractType = 101; return Input[7]; },
            Walk = (query, visit) =>
            {
                Event("W", query.Type, unchecked((uint)query.Argument), unchecked((uint)query.Direction), BitConverter.SingleToUInt32Bits(query.X), BitConverter.SingleToUInt32Bits(query.Y), query.ExplicitFrame ? 1u : 0u);
                // false에서 다음 칸과 N 사건을 생략하는 원문 반복 경계다.
                foreach (var cell in Cells(Input[1])) { if (!visit(new(cell.Frame, cell.X + query.X, cell.Y + query.Y))) return; Event("N"); }
            },
            Image = (type, frame) => { Event("I", type, 0, unchecked((uint)frame)); return Image(Input[1], frame); },
            GumpAt = (x, y) => { Event("G", unchecked((uint)x), unchecked((uint)y), Life.CurrentGump, Drop.LegalGump); return Input[10] != 0 ? 1u : 0u; },
            Accept = (gump, ev, first, second, third) => { Event("A", gump, ev, first, second, third); return Input[11] != 0 ? 2u : 0u; },
            Parent = gump => { Assert.Equal(2u, gump); return 77; },
            MayPlace = query => { Event("P", query.Type, query.Argument, BitConverter.SingleToUInt32Bits(query.X), BitConverter.SingleToUInt32Bits(query.Y), query.Flags, query.Owner, query.Mode); if (Input[21] == 2) State.Obstruction = 1; return Input[12]; },
            Source = sid => { Event("U", (uint)sid); return 77; }, Region = sid => { Event("R", (uint)sid); return unchecked((int)(sid == 77 || Input[20] != 0 ? Input[14] : Input[14] + 1)); },
            ProcessEvent = (process, ev, first, second) => Event("E", process, ev, first, second),
            Project = (x, y) => { Event("J", BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y)); return Projected; },
            Invalidate = (rect, flags) => Event("V", unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), flags),
        };
        /// <summary>17DWORD 전체를 기대값과 같은 순서로 직렬화한다.</summary>
        public uint[] Snapshot(uint result)
        {
            var values = new List<uint> { Drop.Legal, State.PlacementAllowed, State.DescriptionAllowed, State.SpaceAllowed, State.SourceAllowed, Life.CurrentGump, Drop.LegalGump };
            values.AddRange(State.Bounds.Select(value => unchecked((uint)value))); values.AddRange([unchecked((uint)State.HotspotX), unchecked((uint)State.HotspotY), unchecked((uint)State.ImageWidth), unchecked((uint)State.ImageHeight), Life.AbstractType, result]); return values.ToArray();
        }
        /// <summary>현재 하나의 실제 위치 본체를 생성한다.</summary>
        public RawCursorPlacement Logic(CursorPlacementHooks? hooks = null) => new(Pool, Types, Surface, Drop, Life, State, hooks ?? Hooks());
    }
    /// <summary>원문573입력의17DWORD·전체 사건·문자열을 전수 대조한다.</summary>
    [Fact]
    public void EveryNativePlacementSequenceMatches()
    {
        string[][] rows = X86Fixture.Read("cursor-placement"); Assert.Equal(573, rows.Length); var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] values = Numbers(row[0]); scene.Prepare(values); RawCursorPlacement logic = scene.Logic(); uint result = 0;
            if (values[0] == 0) logic.Query(); else if (values[0] == 1) logic.Prepare(); else result = logic.SourceAllowed();
            return scene.Snapshot(result).SequenceEqual(Numbers(row[2])) && (scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)) == row[1] && scene.State.Description == row[3];
        });
    }
    /// <summary>설명은 실제 저장 위치를 참조하며 callback 중 직접 변경도 되돌리지 않는다.</summary>
    [Fact]
    public void DescriptionUsesActualReferenceAndNulPrefix()
    {
        var scene = new Scene(); scene.Prepare(Numbers(X86Fixture.Read("cursor-placement")[1][0])); CursorPlacementHooks hooks = scene.Hooks();
        hooks.Describe = (ref string text, float x, float y, uint type, uint owner) => { text = "old"; scene.State.Description = "A\0ignored"; Assert.Equal("A\0ignored", text); return 7; };
        scene.Logic(hooks).Query(); Assert.Equal("A\0ignored", scene.State.Description); Assert.Equal(7u, scene.State.DescriptionAllowed); Assert.Equal(1u, scene.Drop.Legal);
        int[] first = scene.State.Bounds.ToArray(); hooks.Describe = (ref string text, float x, float y, uint type, uint owner) => { text = "A"; return 7; }; scene.Logic(hooks).Query(); Assert.Equal(first, scene.State.Bounds);
    }
    /// <summary>도달하지 않은 지도는 생략하고 선택된 누락은 이미 실행한 접두 뒤 실패한다.</summary>
    [Fact]
    public void DependenciesFailAtSelectedBoundaryAndKeepPrefix()
    {
        var scene = new Scene(); var hooks = new CursorPlacementHooks(); var logic = new RawCursorPlacement(scene.Pool, scene.Types, ReadOnlyMemory<ushort>.Empty, scene.Drop, scene.Life, scene.State, hooks);
        Assert.Equal(1u, logic.SourceAllowed()); Assert.Throws<InvalidOperationException>(logic.Prepare); scene.Drop.CostFlags = 0x20; Assert.Throws<InvalidOperationException>(() => logic.SourceAllowed());
        scene.Prepare(Numbers(X86Fixture.Read("cursor-placement")[0][0])); hooks = scene.Hooks(); hooks.GumpAt = null;
        Assert.Throws<InvalidOperationException>(scene.Logic(hooks).Query); Assert.Equal(0u, scene.Drop.LegalGump); Assert.Equal(0u, scene.Life.CurrentGump); Assert.Equal("AB", scene.State.Description); Assert.NotEqual(43, scene.State.Bounds[2]);
        hooks = scene.Hooks(); hooks.Accept = (_, _, _, _, _) => 2; hooks.GumpAt = (_, _) => 1; hooks.Parent = _ => 0;
        Assert.Throws<InvalidOperationException>(scene.Logic(hooks).Query); Assert.Equal(1u, scene.Drop.Legal); Assert.Equal(1u, scene.State.PlacementAllowed); Assert.Equal(2u, scene.Drop.LegalGump);
    }
    /// <summary>빈 잠금 취소도 실제 준비를 실행하고 같은 현재 카메라 원점을 다시 읽는다.</summary>
    [Fact]
    public void LifecycleBindingUsesActualPrepareAndCurrentOrigin()
    {
        var scene = new Scene(); scene.Prepare(Numbers(X86Fixture.Read("cursor-placement")[0][0])); scene.Drop.LockDepth = 1; scene.Drop.CostFlags = 0x40; scene.Drop.CursorGump = 0; scene.Life.PreviewCount = 0; int[] pixels = [10, 20]; scene.State.SharedScreenOrigin = pixels;
        CursorPlacementHooks hooks = scene.Hooks(); Action<ScreenRect, uint> record = hooks.Invalidate!; hooks.Invalidate = (rect, flags) => { record(rect, flags); if (flags == 2) { pixels[0] = 70; pixels[1] = 90; } };
        var life = new CursorLifecycleHooks(); scene.Logic(hooks).BindLifecycle(life); new RawCursorLifecycle(scene.Pool, scene.Types, scene.Drop, scene.Life, life).Cancel();
        Assert.StartsWith("J:", scene.Events[0]); Assert.Contains("V:4294967179:4294934439:4294967230:4294934464:0", scene.Events); Assert.Equal(1u, scene.Drop.LockDepth);
    }
    /// <summary>원문 자산 Canon/SHP를 사용하며 false 중단과 잘못된 프레임을 구분한다.</summary>
    [Fact]
    public void AssetBindingUsesActualDecoderAndShapeHeaders()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var assets = new HostAssetTables(new(data), new()); var geometry = new PriestPlacementGeometryState(); var hooks = new CursorPlacementHooks(); HostCursorPlacementAssets.Bind(hooks, assets, geometry);
        int visits = 0; hooks.Walk!(new(92, 0, 0, 1.25f, -0.5f), cell => { visits++; Assert.Equal(1.25f, cell.X); Assert.Equal(-0.5f, cell.Y); Assert.Equal(23u, hooks.Image!(92, cell.Frame).Token); return false; }); Assert.Equal(1, visits);
        Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Walk!(new(69, 0, 0, 0, 0), _ => true)); Assert.Throws<InvalidOperationException>(() => hooks.Image!(92, -1)); Assert.Throws<InvalidOperationException>(() => hooks.Image!(92, 10000));
    }
}
