using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 세판본의 배치 접두·전체 픽셀 모양과 실제 사제 생성 결선을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>접두6384·모양14014개의 독립 원본 입력과 현재 자료/실제 생성 합성 검사다.</summary>
public sealed class PlacementShapeOracleTests
{
    /// <summary>원문 특수 패턴8종과 마지막 비패턴 사제의 타입 번호다.</summary>
    private static readonly uint[] PatternKinds = [107, 82, 94, 157, 131, 129, 140, 142, 158];
    /// <summary>판본별 현재 타입 표를 만든다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
    /// <summary>여섯 출력 float의 부호 있는0도 손실 없이 비교한다.</summary>
    private static uint[] Bits(PriestPlacementPixelShape shape) => [BitConverter.SingleToUInt32Bits(shape.Bounds.Left), BitConverter.SingleToUInt32Bits(shape.Bounds.Top), BitConverter.SingleToUInt32Bits(shape.Bounds.Right), BitConverter.SingleToUInt32Bits(shape.Bounds.Bottom), BitConverter.SingleToUInt32Bits(shape.AnchorX), BitConverter.SingleToUInt32Bits(shape.AnchorY)];
    /// <summary>원본 SHP 입력과 같은 signed WORD 감싸기를 적용한다.</summary>
    private static short Short(int value) => unchecked((short)value);
    /// <summary>원본 입력 생성기의 정상/역순/빈 프레임 코드만 구성한다. 기대 모양은 계산하지 않는다.</summary>
    private static RiftTypeFrames InputFrames(int profile)
    {
        var codes = new List<RiftFrameCode>();
        // 원문 코드가 참조할 전체 방향과 한 자리 번호/번호10을 공급한다.
        for (int side = 'A'; side <= 'P'; side++)
        {
            // 물리 프레임 순서가 검색의 첫 일치를 결정한다.
            for (int number = 0; number <= 10; number++) if (profile != 2) codes.Add(new((byte)side, (byte)'P', (byte)number, 0));
        }
        if (profile == 1) codes.Reverse();
        if (profile != 2) codes.Add(codes[0]);
        return new(codes);
    }
    /// <summary>전체 순회에서 프레임 선택/최솟값/동률이 드러나는 원본 입력 헤더다.</summary>
    private static SquidDisplayFrame Header(int frame, int metric) => metric switch
    {
        0 => new() { Width = Short(24 + frame % 7 * 3), Height = Short(19 + frame % 5 * 2), HotspotX = Short(12 + frame % 11), HotspotY = Short(7 - frame % 13) },
        1 => new() { Width = Short(32767 + frame * 37), Height = Short(-32768 + frame * 29), HotspotX = Short(32767 - frame * 31), HotspotY = Short(-32768 + frame * 17) },
        _ => new(),
    };

    /// <summary>일반과 사제 접두의 반환·전체 전역·인자와 콜백 순서를 세 원본의 기록에 대조한다.</summary>
    [Theory]
    [InlineData(false, "originals", OriginalEdition.Patch1078, 1120)]
    [InlineData(false, "originalCD", OriginalEdition.Cd1072, 1120)]
    [InlineData(false, "original1037", OriginalEdition.Cd1072, 1120)]
    [InlineData(true, "originals", OriginalEdition.Patch1078, 1008)]
    [InlineData(true, "originalCD", OriginalEdition.Cd1072, 1008)]
    [InlineData(true, "original1037", OriginalEdition.Cd1072, 1008)]
    public void PlacementPrefixMatchesNative(bool priest, string editionName, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read(priest ? "priestplacement" : "canonplacement", editionName); Assert.Equal(expectedRows, rows.Length);
        var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var state = new PriestPlacementState();
        X86Fixture.CheckRows(rows, row =>
        {
            int d = priest ? 0 : 1; Assert.Equal(priest ? 23 : 24, row.Length); uint type = X86Fixture.UInt(row[1]);
            var query = new CanonPlacementQuery(type, priest ? type : X86Fixture.UInt(row[2]), X86Fixture.FloatBits(row[2 + d]), X86Fixture.FloatBits(row[3 + d]), X86Fixture.UInt(row[5 + d]), X86Fixture.UInt(row[4 + d]), X86Fixture.UInt(row[6 + d]));
            types[(int)type].Flags2 = X86Fixture.UInt(row[7 + d]); state.ForcePlacement = X86Fixture.UInt(row[8 + d]); state.LocalPlayer = X86Fixture.Int(row[9 + d]); state.BlockedRelation = uint.MaxValue;
            var rect = new PriestPlacementRect(X86Fixture.FloatBits(row[10 + d]), X86Fixture.FloatBits(row[11 + d]), X86Fixture.FloatBits(row[12 + d]), X86Fixture.FloatBits(row[13 + d]));
            uint lower = X86Fixture.UInt(row[14 + d]), change = X86Fixture.UInt(row[15 + d]); var events = new List<string>();
            // 모양 진입의 상태/인자와 원문 대체 효과를 기록한다.
            PriestPlacementRect Shape(uint suppliedType, uint argument, uint flags)
            {
                Assert.Equal((type, query.Argument, query.Flags, 0u), (suppliedType, argument, flags, state.BlockedRelation)); events.Add($"H:{suppliedType}:{argument}:{flags}");
                if (change == 1) { state.LocalPlayer = 17; state.ForcePlacement = 1; types[(int)type].Flags2 = 0; } return rect;
            }
            // 모양 조회의 전역 변경 뒤에도 원래 요청·로컬 비교를 받아 후반 원문 효과만 공급한다.
            bool Geometry(CanonPlacementQuery input, bool local)
            {
                Assert.Equal(query, input); Assert.Equal(0u, state.BlockedRelation); events.Add($"G:{(local ? 1 : 0)}:{input.Owner}:{input.Flags}:{input.Mode}:{lower}");
                state.BlockedRelation = 0x77; if (change == 2) { state.ForcePlacement = 9; state.LocalPlayer = -7; } return lower != 0;
            }
            bool actual;
            if (priest)
            {
                var input = new PriestPlacementQuery(type, query.X, query.Y, query.Flags, query.Owner, query.Mode);
                actual = new RawPriestPlacement(pool, types, state, new() { Shape = Shape, InspectGeometry = (q, local) => Geometry(new(q.Type, q.Type, q.X, q.Y, q.Flags, q.Owner, q.Mode), local) }).MayPlace(input);
            }
            else actual = new RawCanonPlacement(pool, types, state, new() { Shape = Shape, InspectGeometry = Geometry }).MayPlace(query);
            Assert.Equal(row[16 + d] != "0", actual); Assert.Equal(X86Fixture.UInt(row[18 + d]), state.BlockedRelation); Assert.Equal(X86Fixture.UInt(row[19 + d]), state.ForcePlacement); Assert.Equal(X86Fixture.UInt(row[20 + d]), unchecked((uint)state.LocalPlayer)); Assert.Equal(X86Fixture.UInt(row[21 + d]), types[(int)type].Flags2); Assert.Equal(row[22 + d], string.Join(';', events)); return true;
        });
    }

    /// <summary>일반 패턴 전체 getter의 범위/기준점 float 비트를 독립 기계어 기록과 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 3150)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1400)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1400)]
    public void CanonPixelShapeMatchesNative(string editionName, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("canonshape", editionName); Assert.Equal(expectedRows, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition);
        var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var state = new PriestPlacementShapeState(); var getter = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometry, state);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(19, row.Length); uint type = PatternKinds[X86Fixture.Int(row[1])]; Array.Copy(PatternKinds, geometry.PatternTypes, 8);
            if (row[6] == "1") geometry.PatternTypes[0] = geometry.PatternTypes[1]; if (row[6] == "2") geometry.PatternTypes[2] = geometry.PatternTypes[4];
            frames[type].Frames = InputFrames(X86Fixture.Int(row[4])); frames[type].DefaultFrame = X86Fixture.Int(row[7]); hotspots[type] = new(X86Fixture.Int(row[11]), X86Fixture.Int(row[12])); state.ScaleX = X86Fixture.FloatBits(row[9]); state.ScaleY = X86Fixture.FloatBits(row[10]);
            SquidDisplayShape shape = shapes[type]; shape.Loaded = true; shape.FrameCount = frames[type].Frames.Codes.Count; shape.Frames.Clear();
            // 기대 결과 없이 각 원본 입력의 물리 헤더만 원래 순서로 준비한다.
            for (int frame = 0; frame < shape.FrameCount; frame++) shape.Frames.Add(Header(frame, X86Fixture.Int(row[8])));
            Assert.Equal(row.Skip(13).Select(X86Fixture.UInt).ToArray(), Bits(getter.Measure(type, unchecked((uint)X86Fixture.Int(row[2])), unchecked((uint)X86Fixture.Int(row[3])), row[5] != "0"))); return true;
        });
    }

    /// <summary>비패턴 사제 getter의 실제 프레임/현재 기준점과 판본별 저장을 원본 비트에 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 4032)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 2016)]
    [InlineData("original1037", OriginalEdition.Cd1072, 2016)]
    public void PriestPixelShapeMatchesNative(string editionName, OriginalEdition edition, int expectedRows)
    {
        string[][] rows = X86Fixture.Read("priestshape", editionName); Assert.Equal(expectedRows, rows.Length); var pool = new RawSidPool(24000, false, edition: edition); var types = Types(edition); types[158].Flags2 = 0x210000;
        var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var state = new PriestPlacementShapeState(); var getter = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometry, state);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(22, row.Length); uint type = X86Fixture.UInt(row[1]); frames[type].Frames = new(new RiftFrameCode[X86Fixture.Int(row[5])]); frames[type].DefaultFrame = X86Fixture.Int(row[6]); types[type].Flags1 = X86Fixture.UInt(row[7]); hotspots[type] = new(X86Fixture.Int(row[12]), X86Fixture.Int(row[13])); state.ScaleX = X86Fixture.FloatBits(row[14]); state.ScaleY = X86Fixture.FloatBits(row[15]);
            SquidDisplayShape shape = shapes[type]; shape.Loaded = true; shape.FrameCount = X86Fixture.Int(row[5]); shape.Frames.Clear();
            // 원본 비연속 SHP 표와 같은 물리 순서를 구성하며 헤더 값은 입력에서만 읽는다.
            for (int frame = 0; frame < 8; frame++) shape.Frames.Add(new() { Width = Short(X86Fixture.Int(row[8]) + frame * 2), Height = Short(X86Fixture.Int(row[9]) - frame), HotspotX = Short(X86Fixture.Int(row[10]) + frame), HotspotY = Short(X86Fixture.Int(row[11]) - frame) });
            Assert.Equal(row.Skip(16).Select(X86Fixture.UInt).ToArray(), Bits(getter.Measure(type, X86Fixture.UInt(row[2]), X86Fixture.UInt(row[3]), row[4] != "0"))); return true;
        });
    }

    /// <summary>픽셀 getter→여백을 연결하고 현재 메타 변경과 함수 사본을 두 판본에서 검증한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualPixelGetterPlacementAndCurrentMetadata(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, true, edition: edition); var types = Types(edition); types[158].ConstructorAddress = OriginalConstructors.Address(edition, 158); types[158].Flags2 = 0x210000; types[158].MaxHitPoints = 200;
        var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var state = new PriestPlacementShapeState(); var placementState = new PriestPlacementState { LocalPlayer = 1, BlockedRelation = 99 };
        frames[158].Frames = new(new RiftFrameCode[2]); shapes[158].Loaded = true; shapes[158].FrameCount = 2; shapes[158].Frames.AddRange([new() { Width = 160, Height = 110, HotspotX = 80, HotspotY = 55 }, new() { Width = 16, Height = 11, HotspotX = 8, HotspotY = 5 }]); hotspots[158] = new(8, 5);
        var getter = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometry, state); int inspections = 0; var originalHooks = new PriestPlacementHooks { InspectGeometry = (_, local) => { Assert.True(local); inspections++; return true; } };
        PriestPlacementHooks hooks = RawPriestPlacementShape.MakePriestShapePlacementHooks(pool, getter, originalHooks); Assert.Null(originalHooks.Shape); Assert.Same(originalHooks.InspectGeometry, hooks.InspectGeometry); var placement = new RawPriestPlacement(pool, types, placementState, hooks); hooks.Shape = null; hooks.InspectGeometry = null;
        Assert.False(placement.MayPlace(new(158, 9, 9, 0, 1, 0))); Assert.Equal(0, inspections); Assert.Equal(0u, placementState.BlockedRelation);
        frames[158].DefaultFrame = 1; Assert.True(placement.MayPlace(new(158, 9, 9, 0, 1, 0))); Assert.Equal(1, inspections); Assert.Equal(160f, getter.Measure(158, 0, 0, true).Bounds.Right - getter.Measure(158, 0, 0, true).Bounds.Left);
        hotspots[158] = new(0, 0); Assert.Equal(-8f, getter.Measure(158, 158, 0).Bounds.Left); shapes[158].Frames[1].Width = 32; Assert.Equal(24f, getter.Measure(158, 158, 0).Bounds.Right);
        frames[158].DefaultFrame = -1; shapes[158].Loaded = false; state.ScaleX = 0.5f; state.ScaleY = 1.25f; Assert.Equal(new PriestPlacementPixelShape(new(1000, 1000, 0, 0), 0.5f, 1.25f), getter.Measure(158, 158, 0));
    }

    /// <summary>10.78의 실제 모양/배치/나선/서버 사제 생성자를 같은 풀에 연결한다. CD 실제 공장은 미이식이다.</summary>
    [Fact]
    public void ActualPixelPlacementAndSpawnUsesPatchFactory()
    {
        var pool = new RawSidPool(24000, true); var types = Types(OriginalEdition.Patch1078); types[158].ConstructorAddress = OriginalConstructors.Address(158); types[158].Flags2 = 0x210000; types[158].MaxHitPoints = 200;
        var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var state = new PriestPlacementShapeState(); var placementState = new PriestPlacementState { LocalPlayer = 1, BlockedRelation = 99 };
        frames[158].Frames = new(new RiftFrameCode[1]); shapes[158].Loaded = true; shapes[158].FrameCount = 1; shapes[158].Frames.Add(new() { Width = 16, Height = 11, HotspotX = 8, HotspotY = 5 });
        var getter = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometry, state); int born = 0, pops = 0, checks = 0, inspections = 0; var factory = new RawSquidFactory(pool, types); var spots = Enumerable.Repeat((byte)6, 65536).ToArray(); int before = 0;
        var rejecting = new RawPriestPlacement(pool, types, placementState, RawPriestPlacementShape.MakePriestShapePlacementHooks(pool, getter, new() { InspectGeometry = (q, local) => { Assert.True(local); Assert.Equal(new PriestPlacementQuery(158, 21, inspections == before ? 22 : 21, 0, 1, 0), q); return ++inspections == before + 2; } }));
        var spawnHooks = new PriestSpawnHooks { Create = (type, flags) => { Assert.Equal(0u, flags); return born = factory.Create(type); }, SetOwner = (sid, owner) => { Assert.Equal(born, sid); Assert.Equal(1u, owner); Assert.Equal((ushort)0x8101, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(sid)[12..])); }, Pop = (sid, x, y, flags) => { Assert.Equal((born, 21f, 21f, 0u), (sid, x, y, flags)); pops++; }, NotifySurface = _ => Assert.Fail("사제는 다리 알림을 선택하지 않는다"), CheckCarrier = sid => { Assert.Equal(born, sid); checks++; } };
        PriestSpawnHooks connected = RawPriestPlacement.MakePriestPlacementHooks(pool, rejecting, spawnHooks); Assert.Null(spawnHooks.MayPlace); Assert.Same(spawnHooks.Create, connected.Create); new RawPriestSpawn(pool, types, spots, new(), connected).Spawn(20.75f, 21.9f, 158, 0x8101);
        Assert.Equal(15000, born); Assert.Equal((1, 1, before + 2), (pops, checks, inspections));
    }

    /// <summary>현재 공유 전체DWORD·소유자와 요청 스냅샷·선택 진단의 부분 효과를 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentForcePlacementSnapshotAndGuards(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Types(edition); types[158].Flags2 = 0x210000;
        uint force = 0x100; int reads = 0, shapes = 0, geometries = 0; var state = new PriestPlacementState { ForcePlacement = 0, LocalPlayer = -1, BlockedRelation = 99 };
        state.SharedForcePlacement = () => { Assert.Equal(0u, state.BlockedRelation); reads++; return force; };
        PriestPlacementQuery submitted = new(158, 20, 21, 6, 0x1ff, uint.MaxValue); PriestPlacementQuery expected = submitted;
        var hooks = new PriestPlacementHooks { Shape = (type, argument, flags) => { Assert.Equal((158u, 158u, 6u), (type, argument, flags)); shapes++; state.LocalPlayer = 17; submitted = new(70, 0, 0, 0, 0, 0); force = 0x100; return new(0, 0, 16, 11); }, InspectGeometry = (q, local) => { Assert.Equal(expected, q); Assert.True(local); geometries++; return false; } };
        var placement = new RawPriestPlacement(pool, types, state, hooks); hooks.Shape = null; hooks.InspectGeometry = null;
        Assert.True(placement.MayPlace(submitted)); Assert.Equal((1, 0, 0), (reads, shapes, geometries));
        force = 0; Assert.False(placement.MayPlace(submitted)); Assert.Equal((2, 1, 1), (reads, shapes, geometries)); Assert.Equal(70u, submitted.Type);
        state.BlockedRelation = 99; Assert.Throws<ArgumentException>(() => placement.MayPlace(expected with { Type = 69 })); Assert.Equal(99u, state.BlockedRelation);
        Assert.Throws<ArgumentException>(() => placement.MayPlace(expected with { X = float.NaN })); types[158].Flags2 = 0; Assert.Throws<ArgumentException>(() => placement.MayPlace(expected)); types[158].Flags2 = 0x210000;
        Assert.Throws<ArgumentException>(() => new RawPriestPlacement(pool, types[..1], state, new() { Shape = (_, _, _) => default, InspectGeometry = (_, _) => false })); Assert.Throws<ArgumentException>(() => new RawPriestPlacement(pool, types, state, new() { Shape = (_, _, _) => default })); Assert.Throws<ArgumentException>(() => RawPriestPlacement.MakePriestPlacementHooks(other, placement, new()));
        force = 0; var badShape = new RawCanonPlacement(pool, types, state, new() { Shape = (_, _, _) => new(float.NaN, 0, 0, 0), InspectGeometry = (_, _) => throw new InvalidOperationException("잘못된 모양은 후반에 진입하지 않는다") }); state.BlockedRelation = 99; Assert.Throws<ArgumentException>(() => badShape.MayPlace(new(70, 0, 20, 21, 0, 1, 0))); Assert.Equal(0u, state.BlockedRelation);
        var wide = new RawCanonPlacement(pool, types, state, new() { Shape = (_, _, _) => new(-float.MaxValue, 0, float.MaxValue, 0), InspectGeometry = (_, _) => false }); Assert.Throws<ArgumentOutOfRangeException>(() => wide.MayPlace(new(70, 0, 20, 21, 0, 1, 0)));
    }

    /// <summary>실제 다리 패턴 getter에 타입과 별도 인자/방향을 전하고 지도 경계를 적용한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void GeneralPlacementUsesActualPatternPixelShape(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Types(edition); var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var pixelState = new PriestPlacementShapeState(); var state = new PriestPlacementState { LocalPlayer = 8, BlockedRelation = 99 };
        Array.Copy(PatternKinds, geometry.PatternTypes, 8); frames[82].Frames = InputFrames(0); shapes[82].FrameCount = frames[82].Frames.Codes.Count; shapes[82].Loaded = true;
        // 정상 메타 입력만 공급하고 실제 패턴/회전 순회는 제품 decoder에 맡긴다.
        for (int i = 0; i < shapes[82].FrameCount; i++) shapes[82].Frames.Add(new() { Width = 16, Height = 11, HotspotX = 8, HotspotY = 5 });
        var shape = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometry, pixelState); int calls = 0; CanonPlacementQuery expected = new(82, 25, 20, 21, 6, 0x108, 0);
        var hooks = new CanonPlacementHooks { InspectGeometry = (q, local) => { Assert.Equal(expected, q); Assert.True(local); calls++; return true; } }; var attached = RawCanonPlacement.MakeCanonShapePlacementHooks(pool, shape, hooks); Assert.Null(hooks.Shape); Assert.Same(hooks.InspectGeometry, attached.InspectGeometry); var placement = new RawCanonPlacement(pool, types, state, attached); attached.Shape = null; attached.InspectGeometry = null;
        Assert.True(placement.MayPlace(expected)); Assert.Equal(1, calls); Assert.False(placement.MayPlace(expected with { X = 0, Y = 0 })); Assert.Equal(1, calls); Assert.Equal(0u, state.BlockedRelation);
        Assert.Throws<ArgumentException>(() => RawCanonPlacement.MakeCanonShapePlacementHooks(other, shape, new())); Assert.Throws<ArgumentException>(() => new RawCanonPlacement(pool, types, state, new() { InspectGeometry = (_, _) => false })); Assert.Throws<ArgumentException>(() => placement.MayPlace(expected with { Type = 255 })); Assert.Throws<ArgumentException>(() => placement.MayPlace(expected with { Y = float.PositiveInfinity }));
    }

    /// <summary>PATCH 논리 프레임/해제 표식과 CD 직접 물리 프레임, 사제 비패턴 계약을 구별한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void PixelShapeValidatesCurrentFramesAndPriestContract(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition); var other = new RawSidPool(24000, false, edition: edition); var types = Types(edition); types[158].Flags2 = 0x200000; var frames = types.Select(_ => new PriestPlainCanonType()).ToArray(); var shapes = types.Select(_ => new SquidDisplayShape()).ToArray(); var hotspots = new PriestTypeHotspot[types.Length]; var geometry = new PriestPlacementGeometryState(); var state = new PriestPlacementShapeState();
        frames[158].Frames = new(new RiftFrameCode[1]); frames[158].DefaultFrame = 1; shapes[158].FrameCount = 1; shapes[158].Loaded = true; shapes[158].Frames.AddRange([new(), new()]);
        var getter = new RawPriestPlacementShape(pool, types, frames, shapes, hotspots, geometry, state);
        if (edition == OriginalEdition.Patch1078) Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 158, 0)); else Assert.Equal(0f, getter.Measure(158, 158, 0).Bounds.Right);
        types[158].Flags1 = 0x40000; Assert.Equal(0f, getter.Measure(158, 158, 0).Bounds.Right); types[158].MaxHitPoints = -0x22222223;
        if (edition == OriginalEdition.Patch1078) Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 158, 0)); else Assert.Equal(0f, getter.Measure(158, 158, 0).Bounds.Right);
        types[158].MaxHitPoints = -0x32323233;
        if (edition == OriginalEdition.Patch1078) Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 158, 0)); else Assert.Equal(0f, getter.Measure(158, 158, 0).Bounds.Right);
        types[158].MaxHitPoints = 0; frames[158].DefaultFrame = 2; Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 158, 0)); frames[158].DefaultFrame = 0; shapes[158].Loaded = false; Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 158, 0)); shapes[158].Loaded = true;
        Assert.Throws<ArgumentOutOfRangeException>(() => getter.Measure(158, 1, 0, true)); geometry.PatternTypes[2] = 158; Assert.Throws<ArgumentException>(() => getter.Measure(158, 158, 0)); geometry.PatternTypes[2] = 0;
        Assert.Throws<ArgumentException>(() => RawPriestPlacementShape.MakePriestShapePlacementHooks(other, getter, new())); Assert.Throws<ArgumentException>(() => new RawCanonPixelShape(pool, types, frames, [], hotspots, geometry, state)); Assert.Throws<ArgumentException>(() => new RawPriestPlacementShape(pool, types, frames, [], hotspots, geometry, state));
        state.ScaleX = float.PositiveInfinity; Assert.Throws<ArgumentException>(() => getter.Measure(158, 158, 0)); types[158].Flags2 = 0; Assert.Throws<ArgumentException>(() => getter.Measure(158, 158, 0)); Assert.Throws<ArgumentException>(() => getter.Measure(255, 158, 0));
        var canon = new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometry, state); Assert.Throws<ArgumentOutOfRangeException>(() => canon.Measure(255, 0, 0));
    }
}
