using Netstorm.Core.Original;
using Netstorm.Tests;
using System.Buffers.Binary;

// 세판본 기하 관찰과 실제 정책 호출 순서/현재 발자국/미리보기 결선을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>일반5967·사제4352입력과 기하/미리보기 실제 합성 검사다.</summary>
public sealed class PlacementGeometryOracleTests
{
    /// <summary>원문 특수 타입8개와 비패턴 사제 번호다.</summary>
    private static readonly uint[] Kinds = [107, 82, 94, 157, 131, 129, 140, 142, 158];
    /// <summary>현재 번호 체계의 타입 표다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>기대 좌표를 계산하지 않는 정상/역순/빈 메타 입력이다.</summary>
    private static RiftTypeFrames InputFrames(int profile)
    {
        var codes = new List<RiftFrameCode>();
        // 실제 패턴이 찾는 전체 방향/번호를 물리 순서로 구성한다.
        for (int side = 'A'; side <= 'P'; side++)
        {
            // 번호10을 포함하며 profile2는 빈 표다.
            for (int number = 0; number <= 10; number++) if (profile != 2) codes.Add(new((byte)side, (byte)'P', (byte)number, 0));
        }
        if (profile == 1) codes.Reverse(); if (profile != 2) codes.Add(codes[0]); return new(codes);
    }
    /// <summary>원문 네 정수의 순서를 저장한다.</summary>
    private static string Area(SquidSearchArea a) => $"{a.Left}:{a.Top}:{a.Right}:{a.Bottom}";
    /// <summary>기존 decoder 배열 결과를 같은 원문 순서로 읽는다.</summary>
    private static string Area(int[] a) => string.Join(':', a);
    /// <summary>기본 비교에서는 지역 몸체를 명시한 기록 경계로 허용한다.</summary>
    private static CanonPlacementGeometryHooks Allow() => new() { BeginRegions = _ => { }, EndShape = (_, _) => true, FinishRegions = _ => true };
    /// <summary>실제 전체 패턴의 모든 칸·범위·현재 발자국 변화를 원본 관찰과 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 3159)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1404)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1404)]
    public void GeneralGeometryMatchesNative(string name, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("canongeometry", name); Assert.Equal(expectedRows, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); var geometry = new RawCanonPlacementGeometry(pool, types, frames, state);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(15, row.Length); uint type = Kinds[X86Fixture.Int(row[1])]; Array.Copy(Kinds, state.PatternTypes, 8); if (row[10] == "1") state.PatternTypes[0] = state.PatternTypes[1]; if (row[10] == "2") state.PatternTypes[2] = state.PatternTypes[4];
            frames[type].Frames = InputFrames(X86Fixture.Int(row[4])); frames[type].DefaultFrame = X86Fixture.Int(row[11]); int wx = X86Fixture.Int(row[7]), wy = X86Fixture.Int(row[8]); types[type].FootX = wx; types[type].FootY = wy;
            var query = new CanonTypeQuery(type, X86Fixture.Int(row[2]), X86Fixture.Int(row[3]), X86Fixture.FloatBits(row[5]), X86Fixture.FloatBits(row[6]), row[9] != "0"); Assert.Equal(row[13].Replace(',', ':'), Area(geometry.Bounds(query)));
            string[] expected = row[14] == "-" ? [] : row[14].Split(';'); int ordinal = 0, begun = 0, ended = 0, finished = 0; var hooks = new CanonPlacementGeometryHooks { BeginRegions = q => { Assert.Equal(query, q); begun++; }, EndShape = (_, _) => { ended++; return true; }, FinishRegions = _ => { finished++; return true; } };
            Assert.True(geometry.Inspect(query, cell =>
            {
                Assert.True(ordinal < expected.Length); string[] fields = expected[ordinal].Split(','); Assert.Equal(11, fields.Length); Assert.Equal(X86Fixture.Int(fields[0]), cell.Frame); Assert.Equal(X86Fixture.UInt(fields[1]), BitConverter.SingleToUInt32Bits(cell.X)); Assert.Equal(X86Fixture.UInt(fields[2]), BitConverter.SingleToUInt32Bits(cell.Y)); Assert.Equal(X86Fixture.Int(fields[3]), cell.Label); Assert.Equal(X86Fixture.UInt(fields[4]), (uint)cell.Side); Assert.Equal(X86Fixture.UInt(fields[5]), BitConverter.SingleToUInt32Bits(cell.SnappedX)); Assert.Equal(X86Fixture.UInt(fields[6]), BitConverter.SingleToUInt32Bits(cell.SnappedY)); Assert.Equal(string.Join(':', fields[7..]), Area(cell.Area)); ordinal++;
                types[type].FootX = wx + (row[12] != "0" ? ordinal % 3 : 0); types[type].FootY = wy + (row[12] != "0" ? (ordinal + 1) % 3 : 0); return true;
            }, hooks));
            Assert.Equal((expected.Length, 1, expected.Length, 1), (ordinal, begun, ended, finished)); return true;
        });
    }
    /// <summary>사제 fixture의 직접 decoder·첫/끝 상태·충돌범위와 명시 프레임 비트를 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 2304)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1024)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1024)]
    public void PriestDecoderAndCollisionAreaMatchNative(string name, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("priestgeometry", name); Assert.Equal(expectedRows, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(22, row.Length); int direction = unchecked((int)X86Fixture.UInt(row[3])), footX = X86Fixture.Int(row[9]), footY = X86Fixture.Int(row[10]); var frames = new RiftTypeFrames(new RiftFrameCode[X86Fixture.Int(row[7])]); var decoder = new CanonDecoder(frames, X86Fixture.Int(row[8]), X86Fixture.Int(row[2]), direction, X86Fixture.FloatBits(row[4]), X86Fixture.FloatBits(row[5]), row[6] != "0", edition);
            Assert.Equal(X86Fixture.Int(row[11]), decoder.Frame()); Assert.Equal(row[12] != "0", decoder.Valid()); Assert.Equal(X86Fixture.UInt(row[13]), BitConverter.SingleToUInt32Bits(decoder.X())); Assert.Equal(X86Fixture.UInt(row[14]), BitConverter.SingleToUInt32Bits(decoder.Y())); Assert.Equal(row[15], Area(decoder.Bounds(footX, footY))); Assert.Equal(row[16], decoder.Valid() ? Area(RawPriestPlacementGeometry.CollisionArea(decoder.X(), decoder.Y(), footX, footY)) : "none");
            decoder.Advance(); Assert.Equal(X86Fixture.Int(row[17]), decoder.Frame()); Assert.Equal(row[18] != "0", decoder.Valid()); Assert.Equal(X86Fixture.UInt(row[19]), BitConverter.SingleToUInt32Bits(decoder.X())); Assert.Equal(X86Fixture.UInt(row[20]), BitConverter.SingleToUInt32Bits(decoder.Y())); Assert.Equal(row[21], Area(decoder.Bounds(footX, footY))); return true;
        });
    }
    /// <summary>다중 칸 정책 순서·scan/종료 거부와 빈 decoder·생성시 첫 프레임 저장을 확인한다.</summary>
    [Fact]
    public void GeneralGeometryStopsAtRejectionAndKeepsInitialFrame()
    {
        var pool = new RawSidPool(24000, false); var types = Types(OriginalEdition.Patch1078); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); Array.Copy(Kinds, state.PatternTypes, 8); types[157].FootX = types[157].FootY = 1; frames[157].Frames = InputFrames(0); var geometry = new RawCanonPlacementGeometry(pool, types, frames, state); var query = new CanonTypeQuery(157, 0, 0, 20.5f, 21.5f); var expected = query; var events = new List<string>(); int scanned = 0;
        var hooks = new CanonPlacementGeometryHooks { BeginRegions = q => { Assert.Equal(expected, q); events.Add("begin"); query = query with { X = 99 }; }, BeginShape = (q, _) => { Assert.Equal(expected, q); events.Add("prepare"); }, EndShape = (_, _) => { events.Add("end"); return true; }, FinishRegions = _ => { events.Add("finish"); return true; } };
        Assert.False(geometry.Inspect(query, _ => { events.Add("scan"); return ++scanned != 2; }, hooks)); Assert.Equal(["begin", "prepare", "scan", "end", "prepare", "scan"], events); events.Clear(); scanned = 0; query = expected; hooks.EndShape = (_, _) => { events.Add("end"); return false; };
        Assert.False(geometry.Inspect(query, _ => { scanned++; events.Add("scan"); return true; }, hooks)); Assert.Equal(1, scanned); Assert.Equal(["begin", "prepare", "scan", "end"], events);
        query = expected = new(70, 70, 0, 20.5f, 21.5f); types[70].FootX = types[70].FootY = 1; frames[70].DefaultFrame = -1; events.Clear(); Assert.True(geometry.Inspect(query, _ => throw new InvalidOperationException("빈 모양 scan 미선택"), hooks)); Assert.Equal(["begin", "finish"], events);
        frames[70].Frames = InputFrames(0); frames[70].DefaultFrame = 2; hooks = Allow(); hooks.BeginRegions = _ => frames[70].DefaultFrame = -1; Assert.True(geometry.Inspect(query, cell => { Assert.Equal(2, cell.Frame); return true; }, hooks));
    }
    /// <summary>사제의 현재 발자국·요청 사본·선택 모양 준비와 기하 어댑터 복사를 검증한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void PriestGeometryUsesCurrentFootprintAndOriginalQuery(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); types[158].Flags2 = 0x210000; types[158].FootX = types[158].FootY = 1; frames[158].DefaultFrame = 4; frames[158].Frames = new(new RiftFrameCode[5]); PriestPlacementQuery query = new(158, 20.5f, 21.5f, 0, 1, 0), expected = query; var events = new List<string>();
        var hooks = new PriestPlacementGeometryHooks { BeginRegions = q => { Assert.Equal(expected, q); events.Add("begin"); query = query with { X = 99 }; types[158].FootX = 3; types[158].FootY = 2; frames[158].DefaultFrame = -1; }, BeginShape = (q, frame, x, y) => { Assert.Equal(expected, q); Assert.Equal((4, 20f, 21f), (frame, x, y)); events.Add("prepare"); }, EndShape = (q, frame, x, y) => { Assert.Equal(expected, q); Assert.Equal((4, 20f, 21f), (frame, x, y)); events.Add("end"); return true; }, FinishRegions = q => { Assert.Equal(expected, q); events.Add("finish"); return true; } };
        var geometry = new RawPriestPlacementGeometry(pool, types, frames, state, hooks); hooks.BeginRegions = null; hooks.EndShape = null; hooks.FinishRegions = null;
        Assert.True(geometry.Inspect(query, area => { events.Add("scan"); Assert.Equal("18:20:21:22", Area(area)); return true; })); Assert.Equal(["begin", "prepare", "scan", "end", "finish"], events); query = expected; events.Clear(); Assert.Equal("17:19:20:21", Area(geometry.Bounds(query))); Assert.True(geometry.Inspect(query, _ => throw new InvalidOperationException("빈 모양 미선택"))); Assert.Equal(["begin", "finish"], events);
        frames[158].DefaultFrame = 4; query = expected; events.Clear(); Assert.False(geometry.Inspect(query, _ => false)); Assert.Equal(["begin", "prepare"], events);
        var original = new PriestPlacementPreviewHooks { InspectCollisions = (_, _) => false }; var attached = RawPriestPlacementGeometry.MakePriestGeometryPreviewHooks(pool, geometry, original); Assert.Null(original.Bounds); Assert.Same(original.InspectCollisions, attached.InspectCollisions);
        var collisionHooks = new PriestPlacementCollisionHooks { InspectCandidateTerrain = (_, _) => { } }; var collision = RawPriestPlacementGeometry.MakePriestGeometryCollisionHooks(pool, geometry, collisionHooks); Assert.Null(collisionHooks.InspectRegions); Assert.Same(collisionHooks.InspectCandidateTerrain, collision.InspectCandidateTerrain);
        Assert.Throws<ArgumentException>(() => RawPriestPlacementGeometry.MakePriestGeometryPreviewHooks(other, geometry, new())); Assert.Throws<ArgumentException>(() => RawPriestPlacementGeometry.MakePriestGeometryCollisionHooks(other, geometry, new())); Assert.Throws<ArgumentException>(() => geometry.Bounds(expected with { Type = 70 })); state.PatternTypes[4] = 158; Assert.Throws<ArgumentException>(() => geometry.Bounds(expected)); state.PatternTypes[4] = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPriestPlacementGeometry.CollisionArea(20, 21, 0, 1)); Assert.Throws<ArgumentException>(() => new RawPriestPlacementGeometry(pool, types, frames, state, new())); Assert.Throws<ArgumentException>(() => geometry.Bounds(expected with { X = float.PositiveInfinity }));
    }
    /// <summary>실제 패턴 기하→미리보기→접두가 같은 배열을 갱신하며 후반 거부와 표시를 구별한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualPatternGeometryAndPreviewShareState(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); Array.Copy(Kinds, state.PatternTypes, 8); types[157].FootX = types[157].FootY = 1; frames[157].Frames = InputFrames(0); var geometry = new RawCanonPlacementGeometry(pool, types, frames, state); byte[] spots = Enumerable.Repeat((byte)16, 65536).ToArray(); var surfaces = new ushort[65536]; var previewState = new CanonPlacementPreviewState(); var placementState = new PriestPlacementState { LocalPlayer = 1 }; int collisions = 0; CanonPlacementQuery query = new(157, 0, 20.5f, 21.5f, 0, 1, 0);
        var original = new CanonPlacementPreviewHooks { InspectCollisions = (q, local) => { Assert.Equal(query, q); Assert.Equal(q.Owner == 1, local); collisions++; return false; } }; var attached = RawCanonPlacementPreview.MakeCanonGeometryPreviewHooks(pool, geometry, original); Assert.Null(original.Bounds); Assert.Same(original.InspectCollisions, attached.InspectCollisions); var preview = new RawCanonPlacementPreview(pool, types, spots, surfaces, previewState, attached); attached.Bounds = null; attached.InspectCollisions = null;
        var placementHooks = new CanonPlacementHooks { Shape = (type, argument, _) => { Assert.Equal((157u, 0u), (type, argument)); return new(0, 0, 48, 33); } }; var connected = RawCanonPlacementPreview.MakeCanonPlacementPreviewHooks(pool, preview, placementHooks); Assert.Null(placementHooks.InspectGeometry); Assert.Same(placementHooks.Shape, connected.Shape); var placement = new RawCanonPlacement(pool, types, placementState, connected);
        Assert.False(placement.MayPlace(query)); Assert.Equal(1, collisions); Assert.Equal("17:18:20:21", Area(geometry.Bounds(new(157, 0, 0, 20.5f, 21.5f)))); Assert.Equal(36, previewState.Blocked.Count(v => v == 1));
        byte[] before = previewState.Blocked.ToArray(); query = query with { Owner = 2 }; Assert.False(placement.MayPlace(query)); Assert.Equal(2, collisions); Assert.Equal(before, previewState.Blocked);
        Assert.Throws<ArgumentException>(() => RawCanonPlacementPreview.MakeCanonGeometryPreviewHooks(other, geometry, new())); Assert.Throws<ArgumentException>(() => RawCanonPlacementPreview.MakeCanonPlacementPreviewHooks(other, preview, new()));
    }

    /// <summary>다중 칸이 실제 해시/finder 후보를 찾으며 첫 거부 뒤 지형/최종 호출을 생략한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void PatternGeometryUsesActualFinderAndStopsAfterBlocker(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, true, edition: edition); var hash = new RawSquidHash(); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); Array.Copy(Kinds, state.PatternTypes, 8); types[157].FootX = types[157].FootY = types[83].FootX = types[83].FootY = 1; frames[157].Frames = InputFrames(0);
        int blocker = pool.Allocate(); Span<byte> raw = pool.AllocatedBytes(blocker); raw[10] = 83; BinaryPrimitives.WriteUInt32LittleEndian(raw[14..], BitConverter.SingleToUInt32Bits(21)); BinaryPrimitives.WriteUInt32LittleEndian(raw[18..], BitConverter.SingleToUInt32Bits(22)); hash.Bucket(1, 21, 22) = (ushort)blocker;
        var geometry = new RawCanonPlacementGeometry(pool, types, frames, state); var finder = new RawSquidFinder(pool, hash, types); int scanned = 0, ended = 0, finished = 0; var hooks = Allow(); hooks.EndShape = (_, _) => { ended++; return true; }; hooks.FinishRegions = _ => { finished++; return true; };
        Assert.False(geometry.Inspect(new(157, 0, 0, 20.5f, 21.5f), cell => { scanned++; return finder.Begin(cell.Area) == 0; }, hooks)); Assert.InRange(scanned, 1, 9); Assert.Equal(scanned - 1, ended); Assert.Equal(0, finished); Assert.Equal(blocker, finder.State.Current);
    }

    /// <summary>기본 프레임의 개수 검사 생략과 실제 범위·필수 정책 오류를 구별한다.</summary>
    [Fact]
    public void GeneralGeometryKeepsDefaultFrameWithoutCodeAndDiagnosesInputs()
    {
        var pool = new RawSidPool(24000, false); var types = Types(OriginalEdition.Patch1078); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var state = new PriestPlacementGeometryState(); var geometry = new RawCanonPlacementGeometry(pool, types, frames, state); var query = new CanonTypeQuery(70, 70, 0, 20, 21); var hooks = Allow();
        frames[70].DefaultFrame = 4; types[70].FootX = types[70].FootY = 1; Assert.True(geometry.Inspect(query, cell => { Assert.Equal(4, cell.Frame); Assert.Equal((byte)0, cell.Side); return true; }, hooks)); Assert.Equal(new SquidSearchArea(21, 22, 20, 21), RawCanonPlacementGeometry.CollisionArea(20, 21, 0, 0));
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementGeometry(pool, types, [], state)); Assert.Throws<ArgumentOutOfRangeException>(() => geometry.Bounds(query with { Type = 188 })); Assert.Throws<ArgumentException>(() => geometry.Inspect(query, null!, hooks)); Assert.Throws<ArgumentException>(() => geometry.Inspect(query, _ => true, new()));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawCanonPlacementGeometry.CollisionArea(20, 21, -1, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => RawCanonPlacementGeometry.CollisionArea(float.PositiveInfinity, 21, 1, 1)); Assert.Throws<ArgumentOutOfRangeException>(() => RawCanonPlacementGeometry.CollisionArea(float.MaxValue, 21, 1, 1));
    }
}
