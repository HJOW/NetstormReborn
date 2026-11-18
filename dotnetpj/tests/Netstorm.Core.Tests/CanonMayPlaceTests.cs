// 전체 MayPlace 독립 원본 관찰을 저장소를 공유하는 실제 파이프라인과 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// RawCanonPlacementPipeline의 전체 MayPlace를 검증하는 테스트 클래스다. 2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
public sealed class CanonMayPlaceTests
{
    /// <summary>독립 관찰의 판본별 행 수다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Rows = 144;

    /// <summary>checksum 대상 슬롯 수다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Slots = 128;

    /// <summary>원본이 기록한 기대 행을 읽는다. C++ 판단을 다시 계산하지 않는다.</summary>
    /// <returns>판본 이름이 첫 칸인 행 목록이다.</returns>
    private static string[][] Fixture() => X86Fixture.Read("canonmayplace");

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

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

    /// <summary>슬롯 전체를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>반환·표시 배열·지역 배열과 원본 입력 불변성을 대조한다.</summary>
    /// <param name="name">판본 이름이다. originals·originalCD·original1037 중 하나다.</param>
    private static void Replay(string name)
    {
        OriginalEdition edition = name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        bool patch = edition == OriginalEdition.Patch1078;
        var pool = new RawSidPool(24000, false, null, edition);
        var hash = new RawSquidHash();
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

        var shapes = new SquidDisplayShape[types.Length];
        // 모든 타입 번호마다 빈 셰이프를 넣는 반복문이다.
        for (int i = 0; i < shapes.Length; ++i)
        {
            shapes[i] = new SquidDisplayShape();
        }

        var hotspots = new PriestTypeHotspot[types.Length];
        // 모든 타입 번호마다 빈 핫스폿을 넣는 반복문이다.
        for (int i = 0; i < hotspots.Length; ++i)
        {
            hotspots[i] = new PriestTypeHotspot(0, 0);
        }

        ushort[] islands = new ushort[65536];
        byte[] spots = new byte[65536];
        Array.Fill(spots, (byte)6);
        var regions = new CanonPlacementIslandRegion[128];
        // 모든 지역 레코드를 존재 상태로 채우는 반복문이다.
        for (int i = 0; i < regions.Length; ++i)
        {
            regions[i] = new CanonPlacementIslandRegion { Restricted = 0, Exists = 1 };
        }

        var state = new CanonPlacementPipelineState();
        Array.Copy(new uint[] { 107, 82, 94, 157, 131, 129, 140, 142 }, state.Geometry.PatternTypes, 8);
        state.Relations.IslandCount = 128;
        var books = new RawPostPopState();
        var additional = new RawPostPopList();
        var contained = new ContainedFinderState();
        var data = new CanonPlacementPipelineData
        {
            Types = types,
            Frames = frames,
            Shapes = shapes,
            Hotspots = hotspots,
            Spots = spots,
            Islands = islands,
            Regions = regions,
        };
        var pipeline = new RawCanonPlacementPipeline(pool, hash, data, books, additional, contained, state);
        byte[] hashBytes = [];
        byte[] fixedBytes = new byte[131072];
        string[][] rows = Fixture();
        Assert.Equal(3 * Rows, rows.Length);
        int count = 0;
        // 같은 슬롯/타입/목록/공간 자료를 판본별 원본 입력 순서로 재생하는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(26, row.Length);
            int stride = pool.SlotStride;
            // 첫 128슬롯을 원본 실행기와 같은 0으로 만드는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.RawSlot(sid).Fill(0);
            }

            hash.Reset();
            Array.Clear(islands);
            bool pattern = Number(row[1]) != 0;
            uint placing = pattern ? 157u : 80u;
            types[(int)placing].Group = 0;
            types[(int)placing].Flags1 = Number(row[7]);
            types[(int)placing].Flags2 = Number(row[6]);
            types[(int)placing].FootX = types[(int)placing].FootY = 1;
            types[83].Flags1 = 0x800;
            types[83].Flags2 = 2;
            types[83].FootX = types[83].FootY = 1;
            types[84].Flags1 = 0x800;
            types[84].Flags2 = 6;
            types[84].FootX = types[84].FootY = 1;
            types[86].Flags1 = 2;
            types[86].Flags2 = 0;
            types[86].FootX = types[86].FootY = 1;
            var codes = new List<RiftFrameCode>();
            // 정상 원본 받침/일반 자산과 동일한 논리 프레임 코드를 넣는 반복문이다.
            for (byte side = (byte)'A'; side <= (pattern ? (byte)'I' : (byte)'A'); ++side)
            {
                codes.Add(new RiftFrameCode(side, (byte)'P', 1, 0));
            }

            frames[(int)placing].Frames = new RiftTypeFrames(codes);
            frames[83].Frames = new RiftTypeFrames(new List<RiftFrameCode> { new((byte)'P', (byte)'P', 1, 0) });
            var shapeFrames = new List<SquidDisplayFrame>();
            // 원본과 같은 논리 프레임 수만큼 16×11 더미 헤더를 넣는 반복문이다.
            for (int i = 0; i < codes.Count; ++i)
            {
                shapeFrames.Add(new SquidDisplayFrame { Width = 16, Height = 11, HotspotX = 8, HotspotY = 5 });
            }

            shapes[(int)placing] = new SquidDisplayShape { FrameCount = codes.Count, Loaded = true };
            shapes[(int)placing].Frames.AddRange(shapeFrames);
            uint scene = Number(row[8]), owner = Number(row[9]);
            ushort sidCursor = 50;
            // 원본 장면과 같은 6×6 칸의 raw/고정 섬/현재 해시를 준비하는 반복문이다.
            for (int y = 19; y < 25; ++y)
            {
                // 현재 표면과 finder의 0단계는 같은 hash 저장소에 쓰는 반복문이다.
                for (int x = 18; x < 24; ++x)
                {
                    Span<byte> slot = pool.RawSlot(sidCursor);
                    slot[10] = 83;
                    slot[patch ? 34 : 32] = (byte)(scene == 2 ? 2 : 1);
                    slot[patch ? 30 : 28] = 7;
                    slot[patch ? 40 : 35] = 1;
                    Put(slot, 8, 9, 2);
                    Put(slot, 14, BitConverter.SingleToUInt32Bits(x), 4);
                    Put(slot, 18, BitConverter.SingleToUInt32Bits(y), 4);
                    if (scene != 3 || x != 20 || y != 21)
                    {
                        hash.Bucket(0, x, y) = sidCursor;
                        islands[y * 256 + x] = sidCursor;
                    }

                    ++sidCursor;
                }
            }

            Span<byte> work = pool.RawSlot(90);
            work[10] = 84;
            work[patch ? 34 : 32] = 1;
            work[patch ? 30 : 28] = 7;
            Put(work, 14, BitConverter.SingleToUInt32Bits(18f), 4);
            Put(work, 18, BitConverter.SingleToUInt32Bits(20f), 4);
            books.OwnerFactories[owner].Entries = new uint[] { 90 };
            books.OwnerFactories[owner].Count = scene == 1 ? 0u : 1u;
            if (scene == 4)
            {
                Span<byte> obstacle = pool.RawSlot(91);
                obstacle[10] = 86;
                Put(obstacle, 14, BitConverter.SingleToUInt32Bits(20f), 4);
                Put(obstacle, 18, BitConverter.SingleToUInt32Bits(21f), 4);
                hash.Bucket(1, 20f, 21f) = 91;
            }

            if (scene == 5)
            {
                hash.Reset();
                Array.Clear(islands);
            }

            state.Placement.ForcePlacement = Number(row[18]);
            state.Placement.LocalPlayer = (int)Number(row[10]);
            state.Placement.BlockedRelation = 73;
            state.Permission.GraphReady = Number(row[11]);
            state.Permission.Editor = Number(row[12]);
            state.Permission.UseAlliances = Number(row[13]);
            Array.Clear(state.Permission.Alliances);
            state.Permission.Alliances[9 + (int)owner] = state.Permission.Alliances[18 + (int)owner] = Number(row[14]);
            state.Relations.BypassGroundPermission = Number(row[15]);
            Array.Fill(state.Preview.Blocked, (byte)0x55);
            Array.Fill(state.Terrain.Regions, (byte)0x55);
            // 패치 전용 목록의 제한 값과 정상 존재 비트도 원본과 같이 쓰는 반복문이다.
            for (int i = 0; i < regions.Length; ++i)
            {
                regions[i] = new CanonPlacementIslandRegion { Restricted = Number(row[16]), Exists = 1 };
            }

            var query = new CanonPlacementQuery(placing, Number(row[2]), BitConverter.UInt32BitsToSingle(Number(row[4])), BitConverter.UInt32BitsToSingle(Number(row[5])), Number(row[3]), owner, Number(row[17]));
            bool allowed = pipeline.MayPlace(query);
            hashBytes = new byte[0];
            // 네 단계 배열을 원본 메모리 순서로 직렬화하는 반복문이다.
            for (int level = 0; level < 4; ++level)
            {
                foreach (ushort head in hash.Entries(level))
                {
                    Array.Resize(ref hashBytes, hashBytes.Length + 2);
                    hashBytes[^2] = (byte)head;
                    hashBytes[^1] = (byte)(head >> 8);
                }
            }

            // 별도 고정 섬 지도도 little endian checksum으로 검사하는 반복문이다.
            for (int i = 0; i < islands.Length; ++i)
            {
                fixedBytes[2 * i] = (byte)islands[i];
                fixedBytes[2 * i + 1] = (byte)(islands[i] >> 8);
            }

            int slotBytes = patch ? 50 : 36;
            byte[] rawBytes = new byte[Slots * slotBytes];
            // 첫 128슬롯의 물리 바이트를 원본 checksum 순서로 모으는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.Slot(sid).Slice(0, slotBytes).CopyTo(rawBytes.AsSpan(sid * slotBytes, slotBytes));
            }

            bool same = allowed == (Number(row[19]) != 0) && state.Placement.BlockedRelation == Number(row[20])
                && Hex(state.Preview.Blocked) == row[21] && Hex(state.Terrain.Regions) == row[22]
                && X86Fixture.Adler32(rawBytes) == Number(row[23]) && X86Fixture.Adler32(hashBytes) == Number(row[24]) && X86Fixture.Adler32(fixedBytes) == Number(row[25]);
            if (!same)
            {
                string detail = $"허용 {allowed}/{row[19]} 차단 {state.Placement.BlockedRelation}/{row[20]} 미리보기 {Hex(state.Preview.Blocked)}/{row[21]} 지역 {Hex(state.Terrain.Regions)}/{row[22]} raw {X86Fixture.Adler32(rawBytes)}/{row[23]} hash {X86Fixture.Adler32(hashBytes)}/{row[24]} fixed {X86Fixture.Adler32(fixedBytes)}/{row[25]}";
                Assert.True(false, $"전체 MayPlace {name} 행 {count} 불일치: {detail}");
            }
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>세 판본 전체 진입/실제 helper/정상 반환의 독립 관찰을 비교한다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD의 미리보기/지역/반환 차이를 전체 호출 결과로 검사한다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 10.37 PE의 전체 함수 관찰도 따로 재생한다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }
}
