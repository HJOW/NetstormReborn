// 최종 표면 소유 관계·특수 지역·최종 거부의 독립 원본 관찰을 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// RawCanonPlacementRelations의 최종 관계·특수 지역·외부 거부를 검증하는 테스트 클래스다. 2026-11-18 0.609에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
public sealed class CanonPlacementRelationsTests
{
    /// <summary>판본별 원본 관찰 행 수다. 2026-11-18 0.609에서 추가했다.</summary>
    private const int Rows = 2307;

    /// <summary>원본 checksum 대상 슬롯 수다. 2026-11-18 0.609에서 추가했다.</summary>
    private const int Slots = 128;

    /// <summary>특수 지역 레코드 수다. 2026-11-18 0.609에서 추가했다.</summary>
    private const int Regions = 128;

    /// <summary>원본이 기록한 기대 행을 읽는다. C++ 판단을 다시 계산하지 않는다.</summary>
    /// <returns>판본 이름이 첫 칸인 행 목록이다.</returns>
    private static string[][] Fixture() => X86Fixture.Read("canonrelations");

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>원본 little endian 슬롯 필드에 WORD를 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutWord(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 두 바이트만 쓰는 반복문이다.
        for (int i = 0; i < 2; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>원본 little endian 필드에 DWORD를 쓴다.</summary>
    /// <param name="raw">쓸 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트만 쓰는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>원본 little endian 바이트 목록 끝에 DWORD를 덧붙인다.</summary>
    /// <param name="trace">쓸 기록이다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutDword(List<byte> trace, uint value)
    {
        // 낮은 바이트부터 네 개를 잇는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            trace.Add((byte)(value >> (i * 8)));
        }
    }

    /// <summary>슬롯 전체를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>원본 최종 관계·특수 지역·외부 거부의 전체 정상 반환을 대조한다.</summary>
    /// <param name="name">판본 이름이다. originals·originalCD·original1037 중 하나다.</param>
    private static void Replay(string name)
    {
        OriginalEdition edition = name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        bool patch = edition == OriginalEdition.Patch1078;
        var pool = new RawSidPool(24000, false, null, edition);
        var types = new OriginalTypeRecord[patch ? 188 : 171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        var frames = new PriestPlainCanonType[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new PriestPlainCanonType();
        }

        ushort[] surfaces = new ushort[65536];
        ushort[] islands = new ushort[65536];
        var regions = new CanonPlacementIslandRegion[Regions];
        byte[] mapBytes = new byte[131072];
        byte[] relationBytes = new byte[324];
        var permissionState = new CanonPlacementPermissionState();
        var permission = new RawCanonPlacementPermission(pool, permissionState, (_, _, _, _) => { Assert.Fail("호출하지 않아야 한다"); return 0u; });
        var state = new CanonPlacementRelationsState { IslandCount = (uint)Regions };
        var placement = new PriestPlacementState();
        var relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, state, placement, permission);
        string[][] rows = Fixture();
        Assert.Equal(3 * Rows, rows.Length);
        int count = 0;
        // 이전 입력의 raw·지도·관계 표·이전 누적 상태가 다음 입력에 남지 않게 초기화한다.
        foreach (string[] row in rows)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(31, row.Length);
            int stride = pool.SlotStride;
            int ownerOffset = patch ? 34 : 32;
            // 첫 128슬롯을 원본 관측의 0xCD로 채우는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.RawSlot(sid).Fill(0xcd);
            }

            pool.RawSlot(50)[ownerOffset] = (byte)Number(row[8]);
            pool.RawSlot(51)[ownerOffset] = (byte)((Number(row[8]) + 1) % 3);
            uint surface = Number(row[13]);
            Array.Fill(surfaces, (ushort)surface);
            surfaces[21 * 256 + 20] = (ushort)surface;
            surfaces[21 * 256 + 21] = surface == 50 ? (ushort)51 : (ushort)surface;
            surfaces[22 * 256 + 20] = surface == 50 ? (ushort)51 : (ushort)surface;
            surfaces[22 * 256 + 21] = (ushort)surface;
            types[82].Flags2 = Number(row[1]);
            types[82].Flags1 = Number(row[2]);
            types[82].FootX = types[82].FootY = 1;
            permissionState.Editor = Number(row[10]);
            permissionState.UseAlliances = Number(row[11]);
            Array.Clear(permissionState.Alliances);
            // 요청 owner 하위 바이트를 부호 확장한 signed BYTE 계약으로 읽는다.
            uint owner = unchecked((uint)(int)(sbyte)(byte)Number(row[9]));
            uint index = Number(row[8]) * 9u + owner;
            if (index < (uint)permissionState.Alliances.Length)
            {
                permissionState.Alliances[(int)index] = Number(row[12]);
            }

            state.BypassGroundPermission = Number(row[7]);
            placement.BlockedRelation = Number(row[19]);
            // 실제 특수 지역 레코드의 제한·존재 값만 명시적으로 입력하는 반복문이다.
            for (int i = 0; i < regions.Length; ++i)
            {
                regions[i] = new CanonPlacementIslandRegion { Restricted = Number(row[18]), Exists = 1 };
            }

            var query = new CanonPlacementQuery(82, 0, BitConverter.UInt32BitsToSingle(Number(row[14])), BitConverter.UInt32BitsToSingle(Number(row[15])), 0, Number(row[9]), 0);
            var terrain = new CanonPlacementTerrainState
            {
                GroundComplete = Number(row[3]) != 0,
                NoIslandOnly = Number(row[4]) != 0,
                BridgeOverlap = Number(row[5]) != 0,
                Permission = Number(row[6]) != 0,
                EmptyRegion = Number(row[17]),
            };
            terrain.Regions.AsSpan().Fill((byte)Number(row[16]));
            CanonPlacementRelationResult observed = new();
            var pipeline = new RawCanonPlacementTerrain(pool, types, frames, islands, terrain, new CanonPlacementTerrainHooks
            {
                CandidatePermission = (_, _) => { Assert.Fail("호출하지 않아야 한다"); return false; },
                ShapePermission = (_, _) => { Assert.Fail("호출하지 않아야 한다"); return false; },
                FinishRelations = (q, current) => { observed = relations.Inspect(q, current, Number(row[20])); return observed.Allowed; },
            });
            pipeline.Begin(query);
            // Begin의 초기값 대신 실제 앞부분 누적 상태를 회복하고 빈 모양의 Finish만 호출한다.
            terrain.GroundComplete = Number(row[3]) != 0;
            terrain.NoIslandOnly = Number(row[4]) != 0;
            terrain.BridgeOverlap = Number(row[5]) != 0;
            terrain.Permission = Number(row[6]) != 0;
            bool allowed = pipeline.Finish(query);
            // 물리 지도·관계 표도 원본 checksum과 대조하여 입력 자료의 뜻하지 않은 변경을 찾는다.
            for (int i = 0; i < surfaces.Length; ++i)
            {
                mapBytes[2 * i] = (byte)surfaces[i];
                mapBytes[2 * i + 1] = (byte)(surfaces[i] >> 8);
            }

            var relationList = new List<byte>();
            // 직접 DWORD 관계 표의 행 방향과 비트값을 유지하는 반복문이다.
            for (int i = 0; i < permissionState.Alliances.Length; ++i)
            {
                PutDword(relationList, permissionState.Alliances[i]);
            }

            relationList.CopyTo(relationBytes);
            // 첫 128슬롯의 물리 바이트를 원본 checksum과 같은 순서로 모으는 반복문이다. CD는 36바이트 관측에 맞춰 앞 36바이트만 packing한다.
            int slotBytes = patch ? 50 : 36;
            byte[] rawBytes = new byte[Slots * slotBytes];
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.Slot(sid).Slice(0, slotBytes).CopyTo(rawBytes.AsSpan(sid * slotBytes, slotBytes));
            }

            bool same = allowed == (Number(row[21]) != 0)
                && terrain.Permission == (Number(row[22]) != 0)
                && terrain.CanPlaceGround == (Number(row[23]) != 0)
                && placement.BlockedRelation == Number(row[24])
                && observed.RelationRejected == (Number(row[25]) != 0)
                && observed.ForeignRejected == (Number(row[26]) != 0)
                && observed.PriorRejection == (Number(row[27]) != 0)
                && X86Fixture.Adler32(rawBytes) == Number(row[28])
                && X86Fixture.Adler32(mapBytes) == Number(row[29])
                && X86Fixture.Adler32(relationBytes) == Number(row[30]);
            if (!same)
            {
                string detail = $"allowed {allowed}/{row[21]} perm {terrain.Permission}/{row[22]} ground {terrain.CanPlaceGround}/{row[23]} blocked {placement.BlockedRelation}/{row[24]} rel {observed.RelationRejected}/{row[25]} foreign {observed.ForeignRejected}/{row[26]} prior {observed.PriorRejection}/{row[27]} raw {X86Fixture.Adler32(rawBytes)}/{row[28]} map {X86Fixture.Adler32(mapBytes)}/{row[29]} relAd {X86Fixture.Adler32(relationBytes)}/{row[30]}";
                Assert.True(false, $"최종 관계 {name} 행 {count} 불일치: {detail}");
            }
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>패치의 실제 특수 지역·표면 helper와 최종 정상 반환을 모두 재생한다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD의 실제 인라인 판정·이전 거부 저장 차이를 대조한다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 모든 독립 관찰을 재생한다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>정상 범위 검사와 실제 읽기 전의 생략·표시 차단·내부 거부 구별을 검사한다.</summary>
    [Fact]
    public void RequiredDataAndLazyGuards()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var other = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        ushort[] surfaces = new ushort[65536];
        var regions = new CanonPlacementIslandRegion[10];
        // 모든 지역 레코드를 존재 상태로 채우는 반복문이다.
        for (int i = 0; i < regions.Length; ++i)
        {
            regions[i] = new CanonPlacementIslandRegion { Restricted = 0, Exists = 1 };
        }

        var state = new CanonPlacementRelationsState { IslandCount = 10 };
        var placement = new PriestPlacementState();
        var permissions = new CanonPlacementPermissionState();
        var permission = new RawCanonPlacementPermission(pool, permissions, (_, _, _, _) => { Assert.Fail("호출하지 않아야 한다"); return 0u; });
        var relations = new RawCanonPlacementRelations(pool, types, surfaces, regions, state, placement, permission);
        var terrain = new CanonPlacementTerrainState { GroundComplete = true, Permission = true, CanPlaceGround = true };
        terrain.Regions.AsSpan().Fill(9);
        var query = new CanonPlacementQuery(82, 0, 20f, 21f, 0, 1, 0);
        Assert.Throws<ArgumentException>(() => RawCanonPlacementRelations.MakeCanonRelationsTerrainHooks(other, relations, new CanonPlacementTerrainHooks()));
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementRelations(other, types, surfaces, regions, state, placement, permission));
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementRelations(pool, types, new ushort[0], regions, state, placement, permission));
        types[82].Flags2 = 0x200;
        regions[9] = new CanonPlacementIslandRegion { Restricted = 0, Exists = 0 };
        Assert.Throws<ArgumentOutOfRangeException>(() => relations.Inspect(query, terrain));
        regions[9] = new CanonPlacementIslandRegion { Restricted = 0, Exists = 1 };
        state.IslandCount = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => relations.Inspect(query, terrain));
        state.IslandCount = 10;
        terrain.Regions.AsSpan().Fill(255);
        terrain.EmptyRegion = 0xffffffff;
        Assert.True(relations.Inspect(query, terrain).Allowed);
        terrain.EmptyRegion = 127;
        Assert.Throws<ArgumentOutOfRangeException>(() => relations.Inspect(query, terrain));
        types[82].Flags2 = 0x200000;
        query = new CanonPlacementQuery(82, 0, float.NaN, float.PositiveInfinity, 0, 1, 0);
        Assert.True(relations.Inspect(query, terrain, 99).Allowed);
        Assert.Throws<ArgumentOutOfRangeException>(() => relations.SurfaceAt(query.X, query.Y));
        query = new CanonPlacementQuery(82, 0, 20f, 21f, 0, 1, 0);
        types[82].Flags2 = 0x4200;
        terrain.Regions.AsSpan().Fill(9);
        Assert.False(relations.Inspect(query, terrain).Allowed);
        Assert.Equal(0u, placement.BlockedRelation);
        types[82].Flags2 = 0;
        int surface = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        pool.AllocatedBytes(surface)[34] = 255;
        surfaces[21 * 256 + 20] = (ushort)surface;
        permissions.Editor = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => relations.Inspect(query, terrain));
    }
}
