// 모양 기반 flag 8 표면 finder와 주변 권한 구간을 독립 원본 관찰로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// RawCanonPlacementSurrounding의 표면 순회·주변 권한을 검증하는 테스트 클래스다. 2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
public sealed class CanonPlacementSurroundingTests
{
    /// <summary>판본별 원본 관찰 행 수다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Rows = 524;

    /// <summary>같은 가상 슬롯 범위다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Slots = 128;

    /// <summary>원본이 기록한 기대 행을 읽는다. C++ 판단을 다시 계산하지 않는다.</summary>
    /// <returns>판본 이름이 첫 칸인 행 목록이다.</returns>
    private static string[][] Fixture() => X86Fixture.Read("canonsurrounding");

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>쉼표 DWORD 목록을 읽는다.</summary>
    /// <param name="text">쉼표로 나눈 십진수 목록이다.</param>
    private static List<uint> Numbers(string text)
    {
        var result = new List<uint>();
        if (string.IsNullOrEmpty(text) || text == "-")
        {
            return result;
        }

        // 순서·중복을 지우지 않고 입력 번호만 옮기는 반복문이다.
        foreach (string part in text.Split(','))
        {
            if (part.Length != 0)
            {
                result.Add(Number(part));
            }
        }

        return result;
    }

    /// <summary>원본 little endian 슬롯 필드에 값을 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 너비만큼 쓰는 반복문이다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>판본별 +34/+68..+7c/+64/WORD cache 서순을 C++ 커서와 비교 가능한 목록으로 만든다.</summary>
    /// <param name="walk">상태를 읽을 표면 순회다.</param>
    /// <returns>현재·좌표·모서리·캐시 크기와 반환 순서다.</returns>
    private static List<uint> Snapshot(RawCanonSurfaceWalk walk)
    {
        CanonSurfaceCursor state = walk.State();
        var result = new List<uint>
        {
            (uint)state.Current,
            (uint)state.X,
            (uint)state.Y,
            (uint)state.Right,
            (uint)state.Top,
            (uint)state.Left,
            (uint)state.Bottom,
            (uint)state.Returned.Count,
        };
        result.AddRange(state.Returned.Select(v => (uint)v));
        return result;
    }

    /// <summary>각 판본의 실제 함수/구간 관찰을 같은 raw/타입/지도 자료로 대조한다.</summary>
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

        ushort[] islands = new ushort[65536];
        byte[] spots = new byte[65536];
        byte[] mapBytes = new byte[131072];
        var anchors = new List<List<uint>>();
        uint selected = 0, mutation = 0;
        // 미래 후보/지만 바꾸는 원본 반환과 현재 번호·캐시를 되돌리는 변이 동작이다.
        void Mutate()
        {
            if (mutation == 1)
            {
                pool.RawSlot(53)[11] = 2;
            }
            else if (mutation == 2)
            {
                islands[22 * 256 + 20] = 0;
            }
            else if (mutation == 3)
            {
                pool.RawSlot(53)[patch ? 34 : 32] = 2;
            }
            else if (mutation == 4)
            {
                var codes = new List<RiftFrameCode> { new( (byte)'P', (byte)'P', 1, 0), new((byte)'P', (byte)'P', 2, 0), new((byte)'J', (byte)'P', 3, 0) };
                frames[82].Frames = frames[83].Frames = new RiftTypeFrames(codes);
            }
        }

        var permissionState = new CanonPlacementPermissionState();
        var permission = new RawCanonPlacementPermission(pool, permissionState, (owner, type, x, y) =>
        {
            anchors.Add(new List<uint> { owner, type, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y) });
            if (anchors.Count == 1)
            {
                Mutate();
            }

            return selected;
        });
        var surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
        string[][] rows = Fixture();
        Assert.Equal(3 * Rows, rows.Length);
        int count = 0;
        // 판본 기계어에 기록된 같은 입력을 자료·조회 상태에 준비하는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(28, row.Length);
            int stride = pool.SlotStride;
            // 첫 128슬롯·섬·spot을 원본 실행기와 같은 0으로 만드는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.RawSlot(sid).Fill(0);
            }

            Array.Clear(islands);
            Array.Clear(spots);
            anchors.Clear();
            types[82].Group = 0;
            types[82].FootX = int.Parse(row[2], CultureInfo.InvariantCulture);
            types[82].FootY = int.Parse(row[3], CultureInfo.InvariantCulture);
            types[82].Flags1 = 0x800;
            types[82].Flags2 = Number(row[7]);
            types[83].FootX = types[83].FootY = 1;
            types[83].Flags1 = Number(row[9]);
            types[83].Flags2 = Number(row[8]);
            var baseCodes = new List<RiftFrameCode> { new((byte)'P', (byte)'P', 1, 0), new((byte)'A', (byte)'B', 2, 0), new((byte)'J', (byte)'P', 3, 0) };
            frames[82].Frames = frames[83].Frames = new RiftTypeFrames(baseCodes);
            permissionState.GraphReady = Number(row[10]);
            permissionState.Editor = Number(row[11]);
            permissionState.UseAlliances = Number(row[12]);
            permissionState.AllowOtherOwners = Number(row[13]);
            permissionState.Alliances.AsSpan().Fill(Number(row[15]));
            selected = Number(row[16]);
            mutation = Number(row[18]);
            // 입력 슬롯의 판본별 BYTE/DWORD 필드를 실행기와 같은 순서로 쓰는 반복문이다.
            foreach (string part in row[19].Split(';', StringSplitOptions.RemoveEmptyEntries))
            {
                List<uint> node = Numbers(part);
                Assert.Equal(7, node.Count);
                Span<byte> slot = pool.RawSlot((int)node[0]);
                slot[10] = 83;
                slot[11] = (byte)node[5];
                slot[patch ? 34 : 32] = (byte)node[1];
                slot[patch ? 40 : 35] = (byte)node[6];
                Put(slot, 14, node[2], 4);
                Put(slot, 18, node[3], 4);
                Put(slot, patch ? 36 : 34, node[4], patch ? 4 : 1);
            }

            // 지도는 현재 등록 머리 번호를 그대로 보존하는 반복문이다.
            foreach (string part in row[20].Split(';', StringSplitOptions.RemoveEmptyEntries))
            {
                List<uint> cell = Numbers(part);
                islands[cell[1] * 256 + cell[0]] = (ushort)cell[2];
            }

            if (row[21] != "-")
            {
                // 기준점 및 전체 발자국의 내부 spot 입력을 옮기는 반복문이다.
                foreach (string part in row[21].Split(';', StringSplitOptions.RemoveEmptyEntries))
                {
                    List<uint> cell = Numbers(part);
                    spots[cell[1] * 256 + cell[0]] = (byte)cell[2];
                }
            }

            float x = BitConverter.UInt32BitsToSingle(Number(row[4])), y = BitConverter.UInt32BitsToSingle(Number(row[5]));
            var query = new CanonPlacementQuery(82, 0, x, y, 0, Number(row[14]), 0);
            var cellInfo = new CanonPlacementCell(int.Parse(row[6], CultureInfo.InvariantCulture), 0, 0, x, y, (float)Math.Truncate(x), (float)Math.Truncate(y), default);
            bool same = true;
            if (row[1] == "W")
            {
                var walk = new RawCanonSurfaceWalk(pool, types, frames, islands, spots, query, cellInfo);
                var snapshots = new List<List<uint>> { Snapshot(walk) };
                if (walk.Current() != 0)
                {
                    Mutate();
                }

                // 마지막 0 관찰까지 비교하며 그 이후에는 원본 함수를 재호출하지 않는 반복문이다.
                while (walk.Current() != 0)
                {
                    walk.Next();
                    snapshots.Add(Snapshot(walk));
                }

                string[] expected = row[23].Split(';', StringSplitOptions.RemoveEmptyEntries);
                same = snapshots.Count == expected.Length;
                // 행마다 실제 커서/캐시가 같아야 반환 SID 순서의 우연한 일치를 배제한다.
                for (int i = 0; same && i < snapshots.Count; ++i)
                {
                    same = snapshots[i].SequenceEqual(Numbers(expected[i]));
                }
            }
            else
            {
                var terrainState = new CanonPlacementTerrainState();
                CanonPlacementTerrainHooks hooks = RawCanonPlacementSurrounding.MakeCanonSurroundingTerrainHooks(pool, surrounding,
                    RawCanonPlacementPermission.MakeCanonPermissionTerrainHooks(pool, permission, new CanonPlacementTerrainHooks
                    {
                        CandidatePermission = (_, _) => false,
                        ShapePermission = (_, _) => false,
                        FinishRelations = (_, current) => current.Permission,
                    }));
                var terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, terrainState, hooks);
                terrain.Begin(query);
                terrain.BeginShape(query, cellInfo);
                terrainState.Permission = Number(row[17]) != 0;
                terrain.EndShape(query);
                same = terrain.Finish(query) == (Number(row[22]) != 0);
            }

            if (row[24] == "-")
            {
                same = same && anchors.Count == 0;
            }
            else
            {
                string[] expected = row[24].Split(';', StringSplitOptions.RemoveEmptyEntries);
                same = same && anchors.Count == expected.Length;
                // 실제 Player 경계의 타입/owner/원본 좌표와 계속 순회한 호출 횟수를 확인하는 반복문이다.
                for (int i = 0; same && i < anchors.Count; ++i)
                {
                    same = anchors[i].SequenceEqual(Numbers(expected[i]));
                }
            }

            // 지도 번호를 little endian으로 만들어 전체 입력/변화의 checksum도 비교하는 반복문이다.
            for (int i = 0; i < islands.Length; ++i)
            {
                mapBytes[2 * i] = (byte)islands[i];
                mapBytes[2 * i + 1] = (byte)(islands[i] >> 8);
            }

            int slotBytes = patch ? 50 : 36;
            byte[] rawBytes = new byte[Slots * slotBytes];
            // 첫 128슬롯의 물리 바이트를 원본 checksum 순서로 모으는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.Slot(sid).Slice(0, slotBytes).CopyTo(rawBytes.AsSpan(sid * slotBytes, slotBytes));
            }

            same = same && X86Fixture.Adler32(rawBytes) == Number(row[25]) && X86Fixture.Adler32(mapBytes) == Number(row[26]) && X86Fixture.Adler32(spots) == Number(row[27]);
            Assert.True(same, $"주변 표면 {name} 행 {count}({row[1]}) 불일치");
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>패치의 전체 표면 finder와 실제 주변 구간의 모든 입력을 재생한다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD의 다른 구조체/프레임/루프를 실제 원본 관찰로 대조한다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 10.37 PE에서 기록한 독립 관찰을 재생한다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>새로운 자료 경계는 필수이며 잘못된 지도/프레임/발자국을 암묵 허용하지 않는다.</summary>
    [Fact]
    public void RequiredDataGuards()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var other = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
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

        ushort[] islands = new ushort[65536];
        byte[] spots = new byte[65536];
        var state = new CanonPlacementPermissionState();
        var permission = new RawCanonPlacementPermission(pool, state, (_, _, _, _) => 0u);
        var surrounding = new RawCanonPlacementSurrounding(pool, types, frames, islands, spots, permission);
        Assert.Throws<ArgumentException>(() => RawCanonPlacementSurrounding.MakeCanonSurroundingTerrainHooks(other, surrounding, new CanonPlacementTerrainHooks()));
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementSurrounding(other, types, frames, islands, spots, permission));
        var query = new CanonPlacementQuery(82, 0, 20f, 21f, 0, 1, 0);
        var cell = new CanonPlacementCell(0, 0, 0, 20f, 21f, 20f, 21f, default);
        Assert.Throws<ArgumentOutOfRangeException>(() => surrounding.Inspect(query, cell));
        types[82].FootX = types[82].FootY = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => surrounding.Inspect(query, cell));
        frames[82].Frames = new RiftTypeFrames(new List<RiftFrameCode> { new((byte)'P', (byte)'P', 1, 0) });
        islands = new ushort[1];
        Assert.Throws<ArgumentException>(() => new RawCanonSurfaceWalk(pool, types, frames, islands, spots, query, cell));
    }
}
