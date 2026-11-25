// 실제 네 원본 하위의 두 정밀도 결과·전체 본체·난수와 현재 graph 조립을 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 다리 계획을 검사하는 클래스다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.
// 근거: cpppj tests/AiBridgePlanningTests.cpp의 5건 전부다.
public sealed class RawAiBridgePlanningTests
{
    /// <summary>원본 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>실제 8BYTE 결과의 부호 0/NaN도 원문 비트로 비교한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="point">바꿀 좌표다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string PointHex(RawPathPoint point)
    {
        byte[] raw = new byte[8];
        Put(raw, 0, BitConverter.SingleToUInt32Bits(point.X));
        Put(raw, 4, BitConverter.SingleToUInt32Bits(point.Y));
        return Hex(raw);
    }

    /// <summary>결과를 계산하지 않고 원본 caller와 동일한 현재 월드 입력만 조립한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    private sealed class PlanningScene
    {
        /// <summary>전체 실제 본체의 유일 소유자다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x55000000u, bytes => bytes.Fill(0xa5));

        /// <summary>현재 살아 있는 본체다.</summary>
        public readonly uint Token;

        /// <summary>source50/base51/target60을 가진 실제 50BYTE 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>실제 현재 TYPE flags 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>source의 실제 현재 코드다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>현재 hash0 과거의 올림 graph 조회가 공유하는 입력이다.</summary>
        public readonly ushort[] Surfaces = new ushort[65536];

        /// <summary>현재 실제 spot BYTE 입력이다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>같은 현재 전역이다.</summary>
        public uint Bridge = 82;

        /// <summary>같은 현재 전역이다.</summary>
        public uint Fencemark = 90;

        /// <summary>같은 현재 전역이다.</summary>
        public uint WindArcher = 140;

        /// <summary>실제 공용 난수의 입력 상태다.</summary>
        public readonly GameRandom Random = new();

        /// <summary>현재 QWORD 게임 시각이다.</summary>
        public double Now;

        /// <summary>실제 graph 모듈이 빌리는 장부다. 후보를 합성하지 않는다.</summary>
        public readonly RawPostPopState Bookkeeping = new();

        /// <summary>graph 조회에서 사용하지 않는 현재 목록이다.</summary>
        public readonly RawPostPopList Additional = new();

        /// <summary>같은 실제 내용물 권한 입력이다.</summary>
        public readonly ContainedFinderState Finder = new();

        /// <summary>실제 활성 graph/무효값/특수 TYPE다.</summary>
        public readonly PlayerPlacementAnchorState AnchorState = new()
        {
            GraphReady = 1,
            InvalidGraph = 254,
            IgnoreRestrictions = 0,
            FenceType = 90,
            WindArcherType = 140,
        };

        /// <summary>실제 슬롯을 확보하고 caller 본체의 두 현재 필드만 설치한다.</summary>
        public PlanningScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Put(Memory.Bytes(Token), 0x298, 7);
            Put(Memory.Bytes(Token), 0x260, 2);
        }

        /// <summary>같은 현재 원문·지도·코드의 실제 모듈을 만든다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
        /// <returns>검사할 다리 후보 몸체다.</returns>
        public RawAiBridgeCandidates Candidates() => new(Memory, Pool, Types, Frames, Surfaces, Spots, new AiBridgeCandidateTypes
        {
            Bridge = () => Bridge,
            Fencemark = () => Fencemark,
            WindArcher = () => WindArcher,
        });

        /// <summary>같은 현재 graph 몸체이며 별도 기대값을 반환하지 않는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
        /// <param name="sid">조회할 SID다.</param>
        /// <returns>현재 graph 값이다.</returns>
        public uint Graph(Sid sid) => new RawPlayerPlacementAnchor(Pool, Types, Surfaces, Bookkeeping, Additional, Finder, AnchorState).GraphFor(sid.Value);

        /// <summary>입력 지도 모드와 TYPE/슬롯만 원본 도구와 같은 방식으로 구성한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
        /// <param name="mode">고를 지도 모드다.</param>
        /// <param name="kind">쓸 TYPE 번호다.</param>
        /// <param name="side">쓸 방향 글자다.</param>
        public void World(uint mode, uint kind, byte side)
        {
            // flat 행 순서로 caller 입력만 설치하는 반복문이다.
            for (uint y = 0; y < 256; y++)
            {
                for (uint x = 0; x < 256; x++)
                {
                    // 비대칭 입력으로 X/Y 전도와 전체 BYTE 검사를 검증하는 계산이다.
                    int at = (int)(y * 256 + x);
                    Surfaces[at] = mode != 0 && (mode != 3 || (x + 3 * y) % 5 != 0) ? (ushort)50 : (ushort)0;
                    Spots[at] = (byte)(mode == 2 ? 0x80 : mode == 3 && (x + 3 * y) % 5 == 0 ? 0x40 : 0);
                }
            }

            Span<byte> source = Pool.RawSlot(50);
            source.Fill(0);
            source[RawSidPool.TypeOffset] = (byte)kind;
            source[30] = 7;
            source[34] = 2;
            Types[(int)kind].Flags1 = 0x800;
            Frames[(int)kind] = new RiftTypeFrames([new RiftFrameCode(side, (byte)'A', 0, 0)]);
        }

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Token).AsSpan(at, 4));

        /// <summary>현재 본체 DWORD를 쓴다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);
    }

    /// <summary>모든 A~P/다리·육지/지도·난수와 경계에서 실제 XY/공용 난수/전체 본문을 대조한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    [Fact]
    public void Native261RandomStartsAndState()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-planning-random");
        int count = 0;
        // 독립 caller 입력만 제품에 공급하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(12, row.Length);
            var scene = new PlanningScene();
            scene.World(X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]), (byte)X86Fixture.UInt(row[2]));
            scene.Field(0x298, X86Fixture.UInt(row[7]));
            scene.Field(0x260, X86Fixture.UInt(row[8]));
            scene.Random.SetState(X86Fixture.UInt(row[9]));
            string before = Hex(scene.Memory.Bytes(scene.Token));
            var planning = new RawAiBridgePlanning(scene.Candidates(), scene.Random, () => scene.Now, null);
            var bounds = new AiBuildSearchBounds(X86Fixture.FloatBits(row[3]), X86Fixture.FloatBits(row[4]), X86Fixture.FloatBits(row[5]), X86Fixture.FloatBits(row[6]));
            RawPathPoint point = planning.RandomStart(scene.Token, bounds);
            bool same = PointHex(point) == row[10] && scene.Random.State() == X86Fixture.UInt(row[11]) && Hex(scene.Memory.Bytes(scene.Token)) == before;
            Assert.True(same);
            count++;
        }

        Assert.Equal(261, count);
    }

    /// <summary>실제 00462bc0에 대응하는 현재 anchor도 조립해 12칸과 원본 전체 DWORD 비교를 검증한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    [Fact]
    public void Native85ConnectedEdgesWithActualGraph()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-planning-connected");
        int count = 0;
        // 슬롯/지도 입력만 구성하며 연결 기대를 만들지 않는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length);
            var scene = new PlanningScene();
            scene.World(0, X86Fixture.UInt(row[1]), (byte)X86Fixture.UInt(row[2]));
            scene.Field(0x298, X86Fixture.UInt(row[3]));
            scene.Field(0x260, X86Fixture.UInt(row[4]));
            scene.Bridge = X86Fixture.UInt(row[5]);
            uint xbits = X86Fixture.UInt(row[6]);
            uint ybits = X86Fixture.UInt(row[7]);
            Span<byte> target = scene.Pool.RawSlot(60);
            target.Fill(0);
            target[RawSidPool.TypeOffset] = 98;
            Put(target, 14, xbits);
            Put(target, 18, ybits);
            scene.Types[97].Flags1 = 0x800;
            Span<byte> basis = scene.Pool.RawSlot(51);
            basis.Fill(0);
            basis[RawSidPool.TypeOffset] = 97;
            basis[30] = 7;
            basis[34] = 2;
            float xf = BitConverter.UInt32BitsToSingle(xbits);
            float yf = BitConverter.UInt32BitsToSingle(ybits);
            int x = (int)xf;
            int y = (int)yf;
            int gx = (int)((double)xf + (double)0.9999f);
            int gy = (int)((double)yf + (double)0.9999f);
            if (gx >= 0 && gx < 256 && gy >= 0 && gy < 256)
            {
                scene.Surfaces[gy * 256 + gx] = 51;
            }

            int edge = X86Fixture.Int(row[0]);
            // 실제 caller의 단일 가장자리 배치 좌표다.
            (int, int)[] edges = [(-2, -3), (-2, 1), (-1, -3), (-1, 1), (0, -3), (0, 1), (-3, -2), (1, -2), (-3, -1), (1, -1), (-3, 0), (1, 0)];
            if (edge >= 0)
            {
                int cx = x + edges[edge].Item1;
                int cy = y + edges[edge].Item2;
                if (cx >= 0 && cx < 256 && cy >= 0 && cy < 256)
                {
                    scene.Surfaces[cy * 256 + cx] = 50;
                }
            }

            string before = Hex(scene.Memory.Bytes(scene.Token));
            var planning = new RawAiBridgePlanning(scene.Candidates(), scene.Random, () => scene.Now, sid => scene.Graph(sid));
            bool result = planning.Connected(scene.Token, new Sid(60));
            bool same = result == (row[8] != "0") && Hex(scene.Memory.Bytes(scene.Token)) == before;
            Assert.True(same);
            count++;
        }

        Assert.Equal(85, count);
    }

    /// <summary>단일칸·반전 사각형·flat 행 경계의 inclusive 반환과 본문 보존을 확인한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    [Fact]
    public void Native24InclusiveRectangles()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-planning-rectangle");
        int count = 0;
        // 실제 점유 입력만 제품에 공급하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            var scene = new PlanningScene();
            scene.World(X86Fixture.UInt(row[4]), 82, (byte)'J');
            var planning = new RawAiBridgePlanning(scene.Candidates(), scene.Random, () => scene.Now, null);
            string before = Hex(scene.Memory.Bytes(scene.Token));
            Assert.Equal(row[5] != "0", planning.RectangleFree(X86Fixture.Int(row[0]), X86Fixture.Int(row[1]), X86Fixture.Int(row[2]), X86Fixture.Int(row[3])));
            Assert.Equal(before, Hex(scene.Memory.Bytes(scene.Token)));
            count++;
        }

        Assert.Equal(24, count);
    }

    /// <summary>현재 QWORD 시각과 전체 count DWORD로 원본 5d0만 쓴 전체 본문을 대조한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    [Fact]
    public void Native28RetryWholeBodies()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-planning-delay");
        int count = 0;
        // 40/120초와 큰 정밀도/무한 시각에서도 독립 원문만 기대로 쓰는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(3, row.Length);
            var scene = new PlanningScene();
            scene.Field(0x350, X86Fixture.UInt(row[0]));
            scene.Now = BitConverter.UInt64BitsToDouble(X86Fixture.ULong(row[1]));
            var planning = new RawAiBridgePlanning(scene.Candidates(), scene.Random, () => scene.Now, null);
            planning.DelayRetry(scene.Token);
            Assert.Equal(row[2], Hex(scene.Memory.Bytes(scene.Token)));
            count++;
        }

        Assert.Equal(28, count);
    }

    /// <summary>graph 하위가 현재 owner/좌표를 바꾸면 변경 후 필드로 연결을 판정하고 해제 토큰은 거부한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectedReadsCurrentStateAfterGraph()
    {
        var scene = new PlanningScene();
        scene.World(0, 82, (byte)'J');
        Span<byte> raw = scene.Pool.RawSlot(60);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = 98;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(1f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(1f));
        scene.Surfaces[60 * 256 + 48] = 50;
        int calls = 0;
        var planning = new RawAiBridgePlanning(scene.Candidates(), scene.Random, () => scene.Now, sid =>
        {
            Assert.Equal(new Sid(60), sid);
            calls++;
            Put(scene.Pool.RawSlot(60), 14, BitConverter.SingleToUInt32Bits(50.75f));
            Put(scene.Pool.RawSlot(60), 18, BitConverter.SingleToUInt32Bits(63.25f));
            scene.Field(0x298, 9);
            return 9u;
        });
        Assert.True(planning.Connected(scene.Token, new Sid(60)));
        Assert.Equal(1, calls);
        scene.Memory.Free(scene.Token);
        Assert.ThrowsAny<Exception>(() => planning.DelayRetry(scene.Token));
    }
}
