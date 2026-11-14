using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 실제 PE의 로컬 미리보기·현재 표면/관계와 부분 배열 효과를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>일반/사제 미리보기9720입력과 보호/현재 관계/실제 기하 결선 검사다.</summary>
public sealed class PlacementPreviewOracleTests
{
    /// <summary>독립 실행기에 공급했던 표면SID/타입/genus/소유자 입력이다.</summary>
    private static readonly (int Sid, int Type, uint Genus, byte Owner)[] Surfaces = [(50, 83, 2, 1), (51, 84, 2, 2), (52, 85, 4, 0), (53, 86, 2, 8)];
    /// <summary>반복 대조의 입력 복사에만 쓰는 원문 지도5종이다.</summary>
    private static readonly (byte[] Spots, ushort[] Map)[] Maps = BuildMaps();
    /// <summary>기대 차단값 없이 원본 실행기에 넣은 비대칭 지도만 만든다.</summary>
    private static (byte[] Spots, ushort[] Map)[] BuildMaps()
    {
        var maps = new (byte[] Spots, ushort[] Map)[5]; ushort[] ids = [0, 50, 51, 52, 53];
        // 다섯 입력 변형은 원문 지도의 주기와 같다.
        for (int variant = 0; variant < maps.Length; variant++)
        {
            maps[variant] = (new byte[65536], new ushort[65536]);
            // 비대칭 x+2y와3x+y로 전치/경계를 구별한다.
            for (int index = 0; index < 65536; index++)
            {
                int x = index % 256, y = index / 256, choice = (x + 2 * y) % 5; ushort sid = 0; byte spot = 0;
                if (variant == 1) { sid = 50; spot = 6; }
                if (variant is 2 or 4) { sid = ids[choice]; spot = (byte)(variant == 4 && (3 * x + y) % 7 == 0 ? 16 : 6); }
                if (variant == 3) { sid = (ushort)(choice != 3 ? 0xffff : 52); spot = (byte)(choice != 3 ? 16 : 6); }
                maps[variant].Spots[index] = spot; maps[variant].Map[index] = sid;
            }
        }
        return maps;
    }
    /// <summary>접두→미리보기 전체 배열/현재 표면/방향 관계/지도 해시와 콜백을 세판본에 대조한다.</summary>
    [Theory]
    [InlineData(false, "originals", OriginalEdition.Patch1078)]
    [InlineData(false, "originalCD", OriginalEdition.Cd1072)]
    [InlineData(false, "original1037", OriginalEdition.Cd1072)]
    [InlineData(true, "originals", OriginalEdition.Patch1078)]
    [InlineData(true, "originalCD", OriginalEdition.Cd1072)]
    [InlineData(true, "original1037", OriginalEdition.Cd1072)]
    public void PreviewMatchesNativeCurrentMapAndRelations(bool priest, string name, OriginalEdition edition)
    {
        string[][] rows = X86Fixture.Read(priest ? "priestpreview" : "canonpreview", name); Assert.Equal(1620, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); var spots = new byte[65536]; var surfaces = new ushort[65536]; var state = new CanonPlacementPreviewState(); var placementState = new PriestPlacementState(); int ownerOffset = edition == OriginalEdition.Patch1078 ? 34 : 32;
        // 원본과 같은 표면 슬롯을 확보한다. 이후 free/void/dead 입력도 필터 없이 공급한다.
        for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        X86Fixture.CheckRows(rows, row =>
        {
            int d = priest ? 0 : 1; Assert.Equal(priest ? 36 : 37, row.Length); uint type = X86Fixture.UInt(row[1]); var query = new CanonPlacementQuery(type, priest ? type : X86Fixture.UInt(row[2]), X86Fixture.FloatBits(row[2 + d]), X86Fixture.FloatBits(row[3 + d]), X86Fixture.UInt(row[5 + d]), X86Fixture.UInt(row[4 + d]), X86Fixture.UInt(row[6 + d])); types[type].Flags2 = X86Fixture.UInt(row[7 + d]); placementState.ForcePlacement = X86Fixture.UInt(row[8 + d]); placementState.LocalPlayer = X86Fixture.Int(row[9 + d]); placementState.BlockedRelation = uint.MaxValue;
            var shape = new PriestPlacementRect(X86Fixture.FloatBits(row[10 + d]), X86Fixture.FloatBits(row[11 + d]), X86Fixture.FloatBits(row[12 + d]), X86Fixture.FloatBits(row[13 + d])); uint lower = X86Fixture.UInt(row[14 + d]); Assert.Equal("0", row[15 + d]); var bounds = new SquidSearchArea(X86Fixture.Int(row[16 + d]), X86Fixture.Int(row[17 + d]), X86Fixture.Int(row[18 + d]), X86Fixture.Int(row[19 + d])); int variant = X86Fixture.Int(row[20 + d]); Array.Copy(Maps[variant].Spots, spots, spots.Length); Array.Copy(Maps[variant].Map, surfaces, surfaces.Length);
            Array.Fill(state.Blocked, (byte)0xa5); state.Editor = X86Fixture.UInt(row[21 + d]); state.UseAlliances = X86Fixture.UInt(row[22 + d]); Array.Clear(state.Alliances); state.Alliances[19] = X86Fixture.UInt(row[23 + d]); int mutation = X86Fixture.Int(row[24 + d]); if ((query.Owner & 255) == 255) state.Alliances[8] = state.Alliances[19]; var events = new List<string>();
            // 각 행의 원래 raw/타입 입력을 복구하며 할당 상태를 별도 정상화하지 않는다.
            foreach (var entry in Surfaces) { Span<byte> raw = pool.RawSlot(entry.Sid); raw.Fill(0xcd); raw[10] = (byte)entry.Type; raw[11] = (byte)((variant + entry.Sid) % 16); raw[ownerOffset] = entry.Owner; types[entry.Type].Flags2 = entry.Genus; }
            // 범위 콜백 뒤 지도·소유자·TYPE·관계가 바뀌는 원본 입력 효과다.
            void Mutate()
            {
                if (mutation == 1) { long index = ((long)bounds.Bottom + 1) * 256 + bounds.Right + 1; if (index >= 0 && index < spots.Length) spots[(int)index] = 16; }
                if (mutation == 2) { state.Editor = 0; state.UseAlliances = 1; state.Alliances[19] = 0; types[83].Flags2 = 4; pool.RawSlot(50)[ownerOffset] = 8; }
            }
            // 실제 배열 초기화 뒤 원래 값 인자/콜백 순서를 확인하며 범위만 대체한다.
            SquidSearchArea Bounds(CanonPlacementQuery q)
            {
                Assert.Equal(query, q); Assert.All(state.Blocked, b => Assert.Equal((byte)0, b)); events.Add(priest ? $"B:{q.Type}:{BitConverter.SingleToUInt32Bits(q.X)}:{BitConverter.SingleToUInt32Bits(q.Y)}:{q.Flags}" : $"B:{q.Type}:{q.Argument}:{BitConverter.SingleToUInt32Bits(q.X)}:{BitConverter.SingleToUInt32Bits(q.Y)}:{q.Flags}"); events.Add($"R:{bounds.Left}:{bounds.Top}:{bounds.Right}:{bounds.Bottom}"); Mutate(); return bounds;
            }
            // 후보/지역 몸체는 원본 대체 기록 경계이며 결과만 입력에서 공급한다.
            bool Collisions(CanonPlacementQuery q, bool local) { Assert.Equal(query, q); events.Add($"C:{(local ? 1 : 0)}:{q.Owner}:{q.Flags}:{q.Mode}:{lower}"); return lower != 0; }
            // 픽셀 접두 경계도 입력 사각형과 원래 타입/별도 첫 인자를 기록한다.
            PriestPlacementRect Shape(uint t, uint argument, uint flags) { Assert.Equal((type, query.Argument, query.Flags), (t, argument, flags)); events.Add($"H:{t}:{argument}:{flags}"); return shape; }
            bool actual;
            if (priest)
            {
                // 사제 래퍼를 실제 연결하여 일반 경로와 같은 전역/지도 효과를 검증한다.
                CanonPlacementQuery Convert(PriestPlacementQuery q) => new(q.Type, q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode);
                var preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, state, new() { Bounds = q => Bounds(Convert(q)), InspectCollisions = (q, local) => Collisions(Convert(q), local) }); var placement = new RawPriestPlacement(pool, types, placementState, RawPriestPlacementPreview.MakePriestPlacementPreviewHooks(pool, preview, new() { Shape = Shape })); actual = placement.MayPlace(new(type, query.X, query.Y, query.Flags, query.Owner, query.Mode));
            }
            else { var preview = new RawCanonPlacementPreview(pool, types, spots, surfaces, state, new() { Bounds = Bounds, InspectCollisions = Collisions }); var placement = new RawCanonPlacement(pool, types, placementState, RawCanonPlacementPreview.MakeCanonPlacementPreviewHooks(pool, preview, new() { Shape = Shape })); actual = placement.MayPlace(query); }
            Assert.Equal(row[25 + d] != "0", actual); Assert.Equal(X86Fixture.UInt(row[26 + d]), placementState.BlockedRelation); Assert.Equal(X86Fixture.UInt(row[27 + d]), state.Editor); Assert.Equal(X86Fixture.UInt(row[28 + d]), state.UseAlliances); Assert.Equal(X86Fixture.UInt(row[29 + d]), state.Alliances[19]); Assert.Equal(X86Fixture.UInt(row[30 + d]), types[83].Flags2); Assert.Equal(X86Fixture.Bytes(row[31 + d]), state.Blocked);
            byte[] nativeRaw = X86Fixture.Bytes(row[32 + d]); Assert.Equal(nativeRaw, pool.Slot(50)[..nativeRaw.Length].ToArray()); if (nativeRaw.Length < 50) Assert.All(pool.Slot(50)[nativeRaw.Length..].ToArray(), b => Assert.Equal((byte)0xcd, b)); Assert.Equal(X86Fixture.UInt(row[33 + d]), X86Fixture.Adler32(spots)); Assert.Equal(X86Fixture.UInt(row[34 + d]), X86Fixture.Adler32(MemoryMarshal.AsBytes(surfaces.AsSpan()))); Assert.Equal(row[35 + d], string.Join(';', events)); return true;
        });
    }

    /// <summary>현재 표면의 signed 요청자 인덱스·free/dead 무필터와 원문 산술 오류/어댑터를 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void PreviewGuardsCurrentRelationsAndFunctionCopies(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); var spots = new byte[65536]; var surfaces = new ushort[65536]; var state = new CanonPlacementPreviewState(); var bounds = new SquidSearchArea(20, 21, 20, 21); int calls = 0;
        // 표면50까지 확보한 뒤 원본 입력처럼 free/dead 상태로 그대로 조회한다.
        for (int sid = 5; sid <= 50; sid++) pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.RawSlot(50); raw.Clear(); raw[10] = 83; raw[11] = 0xf; raw[edition == OriginalEdition.Patch1078 ? 34 : 32] = 1; types[83].Flags2 = 2; Array.Fill(surfaces, (ushort)50); state.UseAlliances = 1; state.Alliances[8] = 0x100;
        var hooks = new PriestPlacementPreviewHooks { Bounds = _ => bounds, InspectCollisions = (_, _) => { calls++; return false; } }; var preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, state, hooks); hooks.Bounds = null; hooks.InspectCollisions = null; PriestPlacementQuery query = new(158, 20.5f, 21.5f, 0, 255, 0);
        Assert.False(preview.Inspect(query, true)); Assert.Equal(1, calls); Assert.All(state.Blocked, b => Assert.Equal((byte)0, b)); state.Alliances[8] = 0; Assert.False(preview.Inspect(query, true)); Assert.Equal(9, state.Blocked.Count(b => b == 1));
        Array.Fill(state.Blocked, (byte)0xa5); Assert.False(preview.Inspect(query, false)); Assert.All(state.Blocked, b => Assert.Equal((byte)0xa5, b)); state.Editor = 0x100; Assert.False(preview.Inspect(query, true)); Assert.Equal(0, state.Blocked.Count(b => b != 0));
        state.Editor = 0; query = query with { Owner = 128 }; Assert.Throws<ArgumentOutOfRangeException>(() => preview.Inspect(query, true)); bounds = new(1, 1, 11, 1); state.UseAlliances = 0; Assert.Throws<ArgumentOutOfRangeException>(() => preview.Inspect(query, true)); bounds = bounds with { Right = int.MaxValue }; Assert.Throws<ArgumentOutOfRangeException>(() => preview.Inspect(query, true)); bounds = new(20, 255, 20, 255);
        if (edition == OriginalEdition.Patch1078) Assert.False(preview.Inspect(query, true)); else Assert.Throws<ArgumentOutOfRangeException>(() => preview.Inspect(query, true));
        bounds = new(5, 5, 0, 0); int before = calls; Assert.False(preview.Inspect(query, true)); Assert.Equal(before + 1, calls); Assert.All(state.Blocked, b => Assert.Equal((byte)0, b));
        var placementHooks = new PriestPlacementHooks { Shape = (_, _, _) => default }; var attached = RawPriestPlacementPreview.MakePriestPlacementPreviewHooks(pool, preview, placementHooks); Assert.Null(placementHooks.InspectGeometry); Assert.Same(placementHooks.Shape, attached.Shape); Assert.Throws<ArgumentException>(() => RawPriestPlacementPreview.MakePriestPlacementPreviewHooks(other, preview, new()));
        Assert.Throws<ArgumentException>(() => new RawPriestPlacementPreview(pool, types, spots, surfaces, state, new() { Bounds = _ => bounds })); Assert.Throws<ArgumentException>(() => new RawCanonPlacementPreview(pool, types, spots, surfaces[..1], state, new() { Bounds = _ => bounds, InspectCollisions = (_, _) => false }));
    }

    /// <summary>사제 실제 픽셀/기하/미리보기/지형 상태를 같은 표에 연결해 후반 거부를 보존한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void PriestPixelGeometryPreviewAndTerrainShareActualState(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray(); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometryState = new PriestPlacementGeometryState(); var shapeState = new PriestPlacementShapeState(); var previewState = new CanonPlacementPreviewState(); var placementState = new PriestPlacementState { LocalPlayer = 1 }; var terrainState = new PriestPlacementTerrainState(); var surfaces = new ushort[65536]; var spots = Enumerable.Repeat((byte)6, 65536).ToArray();
        types[158].Flags2 = 0x210000; types[158].FootX = types[158].FootY = 1; frames[158].Frames = new(new RiftFrameCode[1]); shapes[158].Loaded = true; shapes[158].FrameCount = 1; shapes[158].Frames.Add(new() { Width = 16, Height = 11 }); var terrain = new RawPriestPlacementTerrain(pool, types, frames, surfaces, terrainState); var events = new List<string>();
        var geometry = new RawPriestPlacementGeometry(pool, types, frames, geometryState, new() { BeginRegions = q => { terrain.Begin(q); events.Add("begin"); }, BeginShape = (q, _, x, y) => { terrain.BeginShape(q, x, y); events.Add("prepare"); }, EndShape = (q, _, _, _) => { events.Add("end"); return terrain.EndShape(q); }, FinishRegions = q => { events.Add("finish"); return terrain.Finish(q); } }); int scans = 0; bool reject = false;
        var collisionHooks = RawPriestPlacementGeometry.MakePriestGeometryCollisionHooks(pool, geometry, new());
        var preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, previewState, RawPriestPlacementGeometry.MakePriestGeometryPreviewHooks(pool, geometry, new() { InspectCollisions = (q, local) => { Assert.True(local); return collisionHooks.InspectRegions!(q, area => { events.Add("scan"); Assert.Equal(new SquidSearchArea(20, 21, 21, 22), area); scans++; return !reject; }); } }));
        var shape = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, shapeState); var placement = new RawPriestPlacement(pool, types, placementState, RawPriestPlacementShape.MakePriestShapePlacementHooks(pool, shape, RawPriestPlacementPreview.MakePriestPlacementPreviewHooks(pool, preview, new()))); PriestPlacementQuery query = new(158, 20.5f, 21.5f, 0, 1, 0);
        Assert.True(placement.MayPlace(query)); Assert.Equal(["begin", "prepare", "scan", "end", "finish"], events); Assert.Equal(1, scans); Assert.False(terrainState.GroundComplete); Assert.False(terrainState.CanPlaceGround); Assert.Equal(127, terrainState.Regions[0]); Assert.Equal(16, previewState.Blocked.Count(b => b == 1));
        reject = true; events.Clear(); Assert.False(placement.MayPlace(query)); Assert.Equal(["begin", "prepare", "scan"], events); Assert.Equal(2, scans); Assert.Equal(16, previewState.Blocked.Count(b => b == 1));
    }
}
