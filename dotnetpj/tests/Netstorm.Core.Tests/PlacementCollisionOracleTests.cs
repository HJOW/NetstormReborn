using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;
using Netstorm.Core.Simulation;

// 원본 후보 인라인 구간과 전체 무시 비트값을 실제 finder/기하/미리보기 합성에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>세판본 일반22680·사제11520입력과 현재 공유 전역·실제 후보 조립 검사다.</summary>
public sealed class PlacementCollisionOracleTests
{
    /// <summary>같은 판본의 현재 타입 표를 만든다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>원문 비정렬 DWORD를 입력 폭 그대로 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>실제 raw 후보의 타입·원래 float 위치만 구성한다.</summary>
    private static void Position(RawSidPool pool, int sid, float x, float y) { Span<byte> raw = pool.AllocatedBytes(sid); raw[10] = 83; Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); }
    /// <summary>세 실제 PE의 무시 반환DWORD/후보 결과·전체 배열·raw 보존을 독립 관찰과 비교한다.</summary>
    [Theory]
    [InlineData(false, "originals", OriginalEdition.Patch1078, 7560)]
    [InlineData(false, "originalCD", OriginalEdition.Cd1072, 7560)]
    [InlineData(false, "original1037", OriginalEdition.Cd1072, 7560)]
    [InlineData(true, "originals", OriginalEdition.Patch1078, 3840)]
    [InlineData(true, "originalCD", OriginalEdition.Cd1072, 3840)]
    [InlineData(true, "original1037", OriginalEdition.Cd1072, 3840)]
    public void CollisionAndFullIgnoreBitsMatchNative(bool priest, string name, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read(priest ? "priestcollision" : "canoncollision", name); Assert.Equal(expectedRows, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); var state = new CanonPlacementCollisionState(); var preview = new CanonPlacementPreviewState();
        var general = new RawCanonPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, _) => throw new InvalidOperationException("한 후보 원본은 지역 미선택"), InspectCandidateTerrain = (_, _) => throw new InvalidOperationException("한 후보 원본은 지형 미선택") });
        var priests = new RawPriestPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, _) => throw new InvalidOperationException("한 후보 원본은 지역 미선택"), InspectCandidateTerrain = (_, _) => throw new InvalidOperationException("한 후보 원본은 지형 미선택") });
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(23, row.Length); OriginalTypeRecord own = types[82], other = types[83]; own.Flags2 = X86Fixture.UInt(row[2]); own.Flags1 = X86Fixture.UInt(row[3]); other.Flags2 = X86Fixture.UInt(row[4]); uint mode = X86Fixture.UInt(row[5]); state.IgnoredGenus = X86Fixture.UInt(row[6]); state.ClientMode = X86Fixture.UInt(row[7]); int sid = X86Fixture.Int(row[8]); own.FootX = X86Fixture.Int(row[14]); own.FootY = X86Fixture.Int(row[15]); other.FootX = X86Fixture.Int(row[16]); other.FootY = X86Fixture.Int(row[17]); Span<byte> raw = pool.RawSlot(sid); raw.Fill(0xcd); raw[10] = 83; Put(raw, 14, X86Fixture.UInt(row[12])); Put(raw, 18, X86Fixture.UInt(row[13])); raw[edition == OriginalEdition.Patch1078 ? 40 : 35] = (byte)X86Fixture.UInt(row[18]); Array.Fill(preview.Blocked, row[19] != "0" ? (byte)0xa5 : (byte)0);
            uint result;
            if (row[1] == "I") result = RawCanonPlacementCollision.PlacementIgnoreValue(edition, own, other, mode, state.IgnoredGenus);
            else if (priest) result = priests.InspectCandidate(new(82, X86Fixture.FloatBits(row[10]), X86Fixture.FloatBits(row[11]), 0, 1, mode), row[9] != "0", sid) ? 1u : 0u;
            else result = general.InspectCandidate(new(82, 25, X86Fixture.FloatBits(row[10]), X86Fixture.FloatBits(row[11]), 0, 1, mode), row[9] != "0", sid) ? 1u : 0u;
            Assert.Equal(X86Fixture.UInt(row[20]), result); Assert.Equal(X86Fixture.Bytes(row[21]), preview.Blocked); byte[] native = X86Fixture.Bytes(row[22]); Assert.Equal(native, pool.Slot(sid)[..native.Length].ToArray()); if (native.Length < 50) Assert.All(pool.Slot(sid)[native.Length..].ToArray(), b => Assert.Equal((byte)0xcd, b)); return true;
        });
    }

    /// <summary>실제 finder의 buried 제외와 첫 거부 뒤 Next/지형·재scan 중단을 두 경로에서 검증한다.</summary>
    [Theory]
    [InlineData(false, OriginalEdition.Patch1078)]
    [InlineData(false, OriginalEdition.Cd1072)]
    [InlineData(true, OriginalEdition.Patch1078)]
    [InlineData(true, OriginalEdition.Cd1072)]
    public void ActualFinderStopsAtRejectionAndSkipsBuried(bool priest, OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, true, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); types[82].Flags2 = priest ? 0x200000u : 0x8000u; types[82].FootX = types[82].FootY = types[83].FootX = types[83].FootY = 1; int sid = pool.Allocate(); Position(pool, sid, 20, 21); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid)[4..], 0xffff); hash.Bucket(1, 20, 21) = (ushort)sid; var state = new CanonPlacementCollisionState(); var preview = new CanonPlacementPreviewState(); int requests = 0; var area = new SquidSearchArea(20, 21, 20, 21);
        // 첫 거부를 무시하는 악성 상위 정책도 다음 탐색을 강제할 수 없어야 한다.
        bool Regions(Func<SquidSearchArea, bool> scan) { requests++; Assert.False(scan(area)); Assert.False(scan(area)); return true; }
        var general = new RawCanonPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, scan) => Regions(scan), InspectCandidateTerrain = (_, _) => Assert.Fail("거부 후보는 지형 미선택") }); var priests = new RawPriestPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, scan) => Regions(scan), InspectCandidateTerrain = (_, _) => Assert.Fail("거부 후보는 지형 미선택") });
        Assert.False(priest ? priests.Inspect(new(82, 20, 21, 0, 1, 1), true) : general.Inspect(new(82, 25, 20, 21, 0, 1, 1), true)); Assert.Equal(1, requests); Assert.Equal((byte)1, preview.Blocked[13]);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(sid)[4..], 0); pool.RawSlot(sid)[edition == OriginalEdition.Patch1078 ? 40 : 35] = 8; Array.Clear(preview.Blocked); Assert.True(priest ? priests.InspectArea(new(82, 20, 21, 0, 1, 1), true, area) : general.InspectArea(new(82, 25, 20, 21, 0, 1, 1), true, area)); Assert.All(preview.Blocked, b => Assert.Equal((byte)0, b));
    }

    /// <summary>무시 후보도 지형 효과를 받고 실제 finder가 미리 저장한next/현재 마스크로 다음 후보를 읽는다.</summary>
    [Theory]
    [InlineData(false, OriginalEdition.Patch1078)]
    [InlineData(false, OriginalEdition.Cd1072)]
    [InlineData(true, OriginalEdition.Patch1078)]
    [InlineData(true, OriginalEdition.Cd1072)]
    public void TerrainMutationKeepsSavedFinderNextAndCurrentMask(bool priest, OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, true, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); types[82].Flags2 = priest ? 0x210000u : 0x10000u; types[82].FootX = types[82].FootY = types[83].FootX = types[83].FootY = 1; int first = pool.Allocate(), second = pool.Allocate(); Position(pool, first, 20, 21); Position(pool, second, 20, 21); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(first)[4..], (ushort)second); hash.Bucket(1, 20, 21) = (ushort)first; var state = new CanonPlacementCollisionState(); var preview = new CanonPlacementPreviewState(); var visited = new List<int>(); uint currentMask = 0; state.SharedIgnoredGenus = () => currentMask;
        // 첫 무시 후보의 지형 효과는 원문 체인 저장값을 무효화하지 못하며 다음 후보의 마스크만 바꾼다.
        void Terrain(int sid) { visited.Add(sid); Assert.Equal(first, sid); BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(first)[4..], 0); currentMask = uint.MaxValue; }
        var hooks = new CanonPlacementCollisionHooks { InspectRegions = (q, scan) => { Assert.Equal(25u, q.Argument); return scan(new(20, 21, 20, 21)); }, InspectCandidateTerrain = (_, sid) => Terrain(sid) }; var general = new RawCanonPlacementCollision(pool, hash, types, state, preview, hooks); hooks.InspectRegions = null; hooks.InspectCandidateTerrain = null;
        var priestHooks = new PriestPlacementCollisionHooks { InspectRegions = (_, scan) => scan(new(20, 21, 20, 21)), InspectCandidateTerrain = (_, sid) => Terrain(sid) }; var priests = new RawPriestPlacementCollision(pool, hash, types, state, preview, priestHooks); priestHooks.InspectRegions = null; priestHooks.InspectCandidateTerrain = null;
        Assert.False(priest ? priests.Inspect(new(82, 20, 21, 0, 1, 0), true) : general.Inspect(new(82, 25, 20, 21, 0, 1, 0), true)); Assert.Equal([first], visited); Assert.Equal((byte)1, preview.Blocked[13]); Assert.Equal((ushort)0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(first)[4..])); Assert.Equal(uint.MaxValue, currentMask);
    }

    /// <summary>실제 패턴/기하/미리보기/충돌을 연결해 지형 효과 뒤 두 번째 모양 거부를 보존한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualPatternPreviewStopsAtSecondShape(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, true, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var geometryState = new PriestPlacementGeometryState(); uint[] kinds = [107, 82, 94, 157, 131, 129, 140, 142]; Array.Copy(kinds, geometryState.PatternTypes, 8); types[157].Flags2 = 0x10000; types[157].FootX = types[157].FootY = types[83].FootX = types[83].FootY = 1; frames[157].Frames = new(Enumerable.Range('A', 9).Select(side => new RiftFrameCode((byte)side, (byte)'P', 1, 0))); var geometry = new RawCanonPlacementGeometry(pool, types, frames, geometryState);
        int first = pool.Allocate(), second = pool.Allocate(); Position(pool, first, 20, 21); Position(pool, second, 22, 21); hash.Bucket(1, 20, 21) = (ushort)first; hash.Bucket(1, 22, 21) = (ushort)second; var state = new CanonPlacementCollisionState(); var previewState = new CanonPlacementPreviewState(); var placementState = new PriestPlacementState { LocalPlayer = 1 }; var spots = Enumerable.Repeat((byte)6, 65536).ToArray(); var surfaces = new ushort[65536]; var events = new List<string>(); CanonPlacementQuery expected = new(157, 0, 20.5f, 21.5f, 0, 1, 0);
        var original = new CanonPlacementCollisionHooks { InspectCandidateTerrain = (q, sid) => { Assert.Equal(expected, q); Assert.Equal(first, sid); events.Add("terrain"); state.IgnoredGenus = uint.MaxValue; } };
        var attached = RawCanonPlacementCollision.MakeCanonGeometryCollisionHooks(pool, geometry, q => { Assert.Equal(expected, q); return new() { BeginRegions = _ => events.Add("begin"), BeginShape = (_, _) => events.Add("shape"), EndShape = (_, _) => { events.Add("end"); return true; }, FinishRegions = _ => { events.Add("finish"); return true; } }; }, original); Assert.Null(original.InspectRegions); Assert.Same(original.InspectCandidateTerrain, attached.InspectCandidateTerrain);
        var collision = new RawCanonPlacementCollision(pool, hash, types, state, previewState, attached); var previewHooks = new CanonPlacementPreviewHooks(); var collisionAttached = RawCanonPlacementCollision.MakeCanonPlacementCollisionHooks(pool, collision, previewHooks); Assert.Null(previewHooks.InspectCollisions); var preview = new RawCanonPlacementPreview(pool, types, spots, surfaces, previewState, RawCanonPlacementPreview.MakeCanonGeometryPreviewHooks(pool, geometry, collisionAttached)); var placement = new RawCanonPlacement(pool, types, placementState, RawCanonPlacementPreview.MakeCanonPlacementPreviewHooks(pool, preview, new() { Shape = (type, arg, _) => { Assert.Equal((157u, 0u), (type, arg)); return new(0, 0, 48, 33); } }));
        Assert.False(placement.MayPlace(expected)); Assert.Equal(["begin", "shape", "terrain", "end", "shape"], events); Assert.Equal(36, previewState.Blocked.Count(b => b == 1));
    }

    /// <summary>전체 실제 사제 배치 부품→막힌 첫 나선/빈 둘째→공장/실제 소유자/Carrier를 같은 풀에 조립한다.</summary>
    [Fact]
    public void ActualPriestPlacementSpawnsAfterBlockingFirstCandidate()
    {
        var pool = new RawSidPool(24000, true); var hash = new RawSquidHash(); var types = Types(OriginalEdition.Patch1078); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometryState = new PriestPlacementGeometryState(); var shapeState = new PriestPlacementShapeState(); var previewState = new CanonPlacementPreviewState(); var state = new CanonPlacementCollisionState { IgnoredGenus = 0x200000 }; var placementState = new PriestPlacementState { LocalPlayer = 1 }; var terrainState = new PriestPlacementTerrainState(); var surfaces = new ushort[65536]; var spots = Enumerable.Repeat((byte)6, 65536).ToArray();
        types[158].ConstructorAddress = OriginalConstructors.Address(158); types[158].Flags1 = 0x69012; types[158].Flags2 = 0x210000; types[158].MaxHitPoints = 200; types[158].FootX = types[158].FootY = types[83].FootX = types[83].FootY = 1; frames[158].Frames = new(new RiftFrameCode[1]); shapes[158].Loaded = true; shapes[158].FrameCount = 1; shapes[158].Frames.Add(new() { Width = 16, Height = 11 }); int blocker = pool.Allocate(); Position(pool, blocker, 21, 22); hash.Bucket(1, 21, 22) = (ushort)blocker; var terrain = new RawPriestPlacementTerrain(pool, types, frames, surfaces, terrainState); var events = new List<string>();
        var geometry = new RawPriestPlacementGeometry(pool, types, frames, geometryState, new() { BeginRegions = q => { terrain.Begin(q); events.Add("begin"); }, BeginShape = (q, _, x, y) => { terrain.BeginShape(q, x, y); events.Add("shape"); }, EndShape = (q, _, _, _) => { events.Add("end"); return terrain.EndShape(q); }, FinishRegions = q => { events.Add("finish"); return terrain.Finish(q); } });
        var collision = new RawPriestPlacementCollision(pool, hash, types, state, previewState, RawPriestPlacementGeometry.MakePriestGeometryCollisionHooks(pool, geometry, new() { InspectCandidateTerrain = terrain.Candidate })); var previewHooks = new PriestPlacementPreviewHooks(); var attached = RawPriestPlacementCollision.MakePriestPlacementCollisionHooks(pool, collision, previewHooks); Assert.Null(previewHooks.InspectCollisions); var preview = new RawPriestPlacementPreview(pool, types, spots, surfaces, previewState, RawPriestPlacementGeometry.MakePriestGeometryPreviewHooks(pool, geometry, attached)); var shape = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometryState, shapeState); var placement = new RawPriestPlacement(pool, types, placementState, RawPriestPlacementShape.MakePriestShapePlacementHooks(pool, shape, RawPriestPlacementPreview.MakePriestPlacementPreviewHooks(pool, preview, new())));
        var factory = new RawSquidFactory(pool, types); var mode = new SquidOwnerMode { Battle = true }; var commonOwner = new SquidOwner(pool, types, new RawPostPopState(), mode); int notices = 0, pops = 0, born = 0; var owner = new RawPriestOwner(pool, mode, new PriestOwnerState { BattlePlayer = 1 }, new() { Base = commonOwner.Set, Notify = _ => notices++ }); var carrier = new RawCarrierCheck(pool, types, spots, _ => throw new InvalidOperationException("사제는 Walker 낙하 미선택"));
        var spawn = new RawPriestSpawn(pool, types, spots, new(), RawPriestPlacement.MakePriestPlacementHooks(pool, placement, RawCarrierCheck.MakeCarrierCheckSpawnHooks(pool, carrier, new() { Create = (type, flags) => { Assert.Equal(0u, flags); return born = factory.Create(type); }, SetOwner = RawPriestOwner.MakePriestOwnerDispatch(owner, commonOwner.Set), Pop = (sid, x, y, flags) => { Assert.Equal((born, 21f, 21f, 0u), (sid, x, y, flags)); pops++; }, NotifySurface = _ => Assert.Fail("사제는 다리 알림 미선택") })));
        spawn.Spawn(20.75f, 21.9f, 158, 0x8101); Assert.Equal(15001, born); Assert.Equal((1, 1), (pops, notices)); Assert.Equal((ushort)0x8181, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(born)[12..])); Assert.Equal(1, pool.Slot(born)[34]); Assert.Equal(["begin", "shape", "begin", "shape", "end", "finish"], events); Assert.Equal(16, previewState.Blocked.Count(b => b == 1)); Assert.False(terrainState.GroundComplete); Assert.False(terrainState.CanPlaceGround);
    }

    /// <summary>현재 공유 역할/전체 마스크와 genus/자료/다른풀 진단을 선택 지점에서 검증한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentSharedStateAndGuards(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); types[82].Flags2 = 0x200000; types[82].FootX = types[82].FootY = types[83].FootX = types[83].FootY = 1; int sid = pool.Allocate(SidAllocation.Client); Position(pool, sid, 20, 21); pool.RawSlot(sid).Clear(); pool.RawSlot(sid)[10] = 83; Put(pool.RawSlot(sid), 14, BitConverter.SingleToUInt32Bits(20)); Put(pool.RawSlot(sid), 18, BitConverter.SingleToUInt32Bits(21)); bool server = true; uint mask = 0x100; int maskReads = 0; var state = new CanonPlacementCollisionState { IgnoredGenus = uint.MaxValue, SharedServer = () => server, SharedIgnoredGenus = () => { maskReads++; return mask; } }; var preview = new CanonPlacementPreviewState();
        var collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, _) => false, InspectCandidateTerrain = (_, _) => { } }); CanonPlacementQuery q = new(82, 25, 20, 21, 0, 1, 1); Assert.True(collision.InspectCandidate(q, true, sid)); Assert.Equal(0, maskReads); server = false; Assert.False(collision.InspectCandidate(q, true, sid)); Assert.Equal(1, maskReads); Assert.Equal((byte)1, preview.Blocked[13]); types[83].Flags2 = 0x8000; mask = 0x200000; Assert.Equal(edition == OriginalEdition.Patch1078, collision.InspectCandidate(q, false, sid));
        pool.RawSlot(sid)[10] = 255; server = true; Assert.Throws<ArgumentOutOfRangeException>(() => collision.InspectCandidate(q, false, sid)); pool.RawSlot(sid)[10] = 83; server = false; types[83].Flags2 = 0; mask = 0; Put(pool.RawSlot(sid), 14, BitConverter.SingleToUInt32Bits(float.NaN)); Assert.Throws<ArgumentOutOfRangeException>(() => collision.InspectCandidate(q, true, sid)); Assert.False(collision.InspectCandidate(q, false, sid));
        Assert.Throws<ArgumentException>(() => collision.Inspect(q with { Type = 188 }, true)); Assert.Throws<ArgumentException>(() => collision.Inspect(q with { X = float.PositiveInfinity }, true)); Assert.Throws<ArgumentException>(() => new RawCanonPlacementCollision(pool, hash, types, state, preview, new())); Assert.Throws<ArgumentException>(() => RawCanonPlacementCollision.MakeCanonPlacementCollisionHooks(other, collision, new()));
        var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var geometry = new RawCanonPlacementGeometry(pool, types, frames, new()); Assert.Throws<ArgumentException>(() => RawCanonPlacementCollision.MakeCanonGeometryCollisionHooks(pool, geometry, null!, new())); Assert.Throws<ArgumentException>(() => RawCanonPlacementCollision.MakeCanonGeometryCollisionHooks(other, geometry, _ => new(), new()));
        var priest = new RawPriestPlacementCollision(pool, hash, types, state, preview, new() { InspectRegions = (_, _) => false, InspectCandidateTerrain = (_, _) => { } }); types[82].Flags2 = 0; Assert.Throws<ArgumentException>(() => priest.Inspect(new(82, 20, 21, 0, 1, 0), true)); types[82].Flags2 = 0x200000; Assert.False(priest.Inspect(new(82, 20, 21, 0, 1, 0), true)); Assert.Throws<ArgumentException>(() => RawPriestPlacementCollision.MakePriestPlacementCollisionHooks(other, priest, new())); Assert.Throws<ArgumentException>(() => new RawPriestPlacementCollision(pool, hash, types, state, preview, new()));
    }
}
