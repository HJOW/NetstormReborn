// 실제 후보 준비 함수가 동결한 전체 본체와 입력 지도 보존을 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 다리 후보를 검사하는 클래스다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.
// 근거: cpppj tests/AiBridgeCandidatesTests.cpp의 4건 전부다.
public sealed class RawAiBridgeCandidatesTests
{
    /// <summary>원본 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>원본에 공급한 지도/TYPE/슬롯 입력만 만들고 후보 결과는 제품 몸체가 계산한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    private sealed class CandidateScene
    {
        /// <summary>실제 Brain 소유 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x55000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 핸들이다.</summary>
        public readonly uint Token;

        /// <summary>명시 source50의 실제 50BYTE 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>원본 helper가 읽은 surface flag 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>실제 현재 코드 입력이다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>명시 hash0 입력이다.</summary>
        public readonly ushort[] Surfaces = new ushort[65536];

        /// <summary>명시 spot 입력이다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>같은 현재 원본 TYPE 전역이다.</summary>
        public uint Bridge = 82;

        /// <summary>같은 현재 원본 TYPE 전역이다.</summary>
        public uint Fencemark = 90;

        /// <summary>같은 현재 원본 TYPE 전역이다.</summary>
        public uint WindArcher = 140;

        /// <summary>실제 슬롯을 확보하고 caller 입력을 각 필드에만 적용한다.</summary>
        /// <param name="row">16칸 입력 행이다.</param>
        public CandidateScene(string[] row)
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            uint mode = X86Fixture.UInt(row[0]);
            uint kind = X86Fixture.UInt(row[1]);
            int x = unchecked((int)X86Fixture.UInt(row[3]));
            int y = unchecked((int)X86Fixture.UInt(row[4]));
            // 지도 생성에 쓰는 caller의 현재 시작 칸이다.
            byte[] body = Memory.Bytes(Token);
            body.AsSpan().Fill((byte)X86Fixture.UInt(row[12]));
            Field(0x5f8, X86Fixture.UInt(row[9]));
            Field(0x5fc, X86Fixture.UInt(row[10]));
            Field(0x690, X86Fixture.UInt(row[6]));
            Field(0x694, X86Fixture.UInt(row[7]));
            Field(0x698, X86Fixture.UInt(row[5]));
            Field(0x69c, X86Fixture.UInt(row[8]));
            Field(0x6a0, X86Fixture.UInt(row[3]));
            Field(0x6a4, X86Fixture.UInt(row[4]));
            // 원본 관측 도구와 같은 입력 지도만 행·열 순서로 구성하는 반복문이다.
            for (int cy = 0; cy < 256; cy++)
            {
                // 각 칸의 지형/점유 입력만 채우는 반복문이다.
                for (int cx = 0; cx < 256; cx++)
                {
                    bool present = mode != 0 && (mode != 4 || (cx == x && cy == y));
                    if (mode == 3)
                    {
                        present = (cx + cy) % 3 != 0;
                    }

                    int at = cy * 256 + cx;
                    if (present)
                    {
                        Surfaces[at] = 50;
                    }

                    if (mode == 2)
                    {
                        Spots[at] = 1;
                    }
                    else if (mode == 3)
                    {
                        Spots[at] = (cx + cy) % 3 == 0 ? (byte)0x80 : (byte)0;
                    }
                    else if (mode == 5)
                    {
                        Spots[at] = ((cx == x && cy == y - 1) || (cx == x + 1 && cy == y)) ? (byte)0x40 : (byte)0;
                    }
                }
            }

            Span<byte> raw = Pool.RawSlot(50);
            raw.Fill(0);
            raw[RawSidPool.TypeOffset] = (byte)kind;
            raw[30] = 7;
            raw[34] = 2;
            Types[(int)kind].Flags1 = 0x800;
            Frames[(int)kind] = new RiftTypeFrames([new RiftFrameCode((byte)X86Fixture.UInt(row[2]), (byte)'A', 0, 0)]);
        }

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>현재 본체 DWORD를 쓴다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>두 float의 원문 비트를 기록한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="point">쓸 좌표다.</param>
        public void Point(int at, RawPathPoint point)
        {
            Field(at, BitConverter.SingleToUInt32Bits(point.X));
            Field(at + 4, BitConverter.SingleToUInt32Bits(point.Y));
        }

        /// <summary>같은 현재 월드/본체를 참조하는 실제 준비 모듈이다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
        /// <returns>검사할 다리 후보 몸체다.</returns>
        public RawAiBridgeCandidates Candidates() => new(Memory, Pool, Types, Frames, Surfaces, Spots, new AiBridgeCandidateTypes
        {
            Bridge = () => Bridge,
            Fencemark = () => Fencemark,
            WindArcher = () => WindArcher,
        });
    }

    /// <summary>원본 210장면/220반환의 24칸 예산·최대 6개·연결비트·재개·전체 바이트를 대조한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    [Fact]
    public void Native210FullBodies()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-candidates");
        int count = 0;
        int returns = 0;
        // 독립 caller 입력만 설치하고 원문 결과를 재계산하지 않는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(16, row.Length);
            var scene = new CandidateScene(row);
            var candidates = scene.Candidates();
            bool result = false;
            for (uint repeat = 0; repeat < X86Fixture.UInt(row[11]); repeat++)
            {
                // 같은 상태를 실제 다음 호출에 이어 재개한다.
                result = candidates.Prepare(scene.Token);
                returns++;
            }

            bool same = result == (row[13] != "0") && Hex(scene.Memory.Bytes(scene.Token)) == row[14];
            Assert.True(same);
            count++;
        }

        Assert.Equal(210, count);
        Assert.Equal(220, returns);
    }

    /// <summary>실제 초기화와 준비를 Step에 연결하면 예산 중단 동안 상태 2를 유지한 뒤에만 검색 3으로 진행한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    [Fact]
    public void ComposesInitializationAndBudgetedStep()
    {
        var scene = new CandidateScene(["0", "97", "65", "50", "50", "2", "0", "0", "1", "7", "2", "1", "165", "0", "", "0"]);
        var finder = new ContainedFinderState();
        double now = 0;
        int limit = 256;
        uint logKind = 0;
        uint marker = 0;
        var hooks = new AiBridgePathHooks();
        hooks.Initialize = (token, source, target) =>
        {
            var path = new RawAiBridgePath(scene.Memory, scene.Pool, finder, new AiBridgePathGlobals
            {
                Now = () => now,
                MapLimit = () => limit,
                BridgeType = () => scene.Bridge,
                LogKind = () => logKind,
                StartMarker = () => marker,
                EndMarker = () => marker,
                TargetMarker = () => marker,
            }, new AiBridgePathHooks());
            Assert.True(path.Initialize(token, source, target));
        };
        hooks.Prepare = token => scene.Candidates().Prepare(token);
        var path = new RawAiBridgePath(scene.Memory, scene.Pool, finder, new AiBridgePathGlobals
        {
            Now = () => now,
            MapLimit = () => limit,
            BridgeType = () => scene.Bridge,
            LogKind = () => logKind,
            StartMarker = () => marker,
            EndMarker = () => marker,
            TargetMarker = () => marker,
        }, hooks);
        scene.Field(0x5f0, 1);
        var source = new RawPathPoint(50, 50);
        Assert.False(path.Step(scene.Token, ref source, new RawPathPoint(80, 80)));
        Assert.Equal(2u, scene.Field(0x5f0));
        Assert.False(path.Step(scene.Token, ref source, new RawPathPoint(80, 80)));
        Assert.Equal(2u, scene.Field(0x5f0));
        // 실제 나선 상태가 완료할 때까지 동일 Step을 호출하는 반복문이다.
        for (int call = 0; call < 16 && scene.Field(0x5f0) == 2; call++)
        {
            Assert.False(path.Step(scene.Token, ref source, new RawPathPoint(80, 80)));
        }

        Assert.Equal(3u, scene.Field(0x5f0));
        Assert.Equal(50f, source.X);
        Assert.Equal(50f, source.Y);
        Assert.Same(scene.Pool, scene.Candidates().Pool);
    }

    /// <summary>원본 시작점 399개를 caller 입력만으로 재실행하고 결과 XY/본문 불변을 검증한다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    [Fact]
    public void Native399StartPoints()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-start");
        int count = 0;
        // 같은 거리 동률/분수 경계/현재 전체 DWORD 비교도 원본 결과와 대조하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length);
            var scene = new CandidateScene(["0", row[1], row[2], "50", "50", "2", "0", "0", "1", "7", "2", "1", "165", "0", "", "0"]);
            uint mode = X86Fixture.UInt(row[0]);
            // 원본 관측의 현재 지도 입력만 행 순서로 구성하는 반복문이다.
            for (uint y = 0; y < 256; y++)
            {
                // 비대칭 지형/점유로 X/Y 전도를 검출하는 반복문이다.
                for (uint x = 0; x < 256; x++)
                {
                    int at = (int)(y * 256 + x);
                    scene.Surfaces[at] = mode != 0 && (mode != 3 || (x + 3 * y) % 5 != 0) ? (ushort)50 : (ushort)0;
                    scene.Spots[at] = (byte)(mode == 2 ? 0x80 : mode == 3 && (x + 3 * y) % 5 == 0 ? 0x40 : 0);
                }
            }

            scene.Bridge = X86Fixture.UInt(row[11]);
            byte[] body = scene.Memory.Bytes(scene.Token);
            scene.Field(0x298, X86Fixture.UInt(row[9]));
            scene.Field(0x260, X86Fixture.UInt(row[10]));
            string before = Hex(body);
            var bounds = new AiBuildSearchBounds(X86Fixture.FloatBits(row[3]), X86Fixture.FloatBits(row[4]), X86Fixture.FloatBits(row[5]), X86Fixture.FloatBits(row[6]));
            RawPathPoint point = scene.Candidates().Start(scene.Token, new RawPathPoint(X86Fixture.FloatBits(row[7]), X86Fixture.FloatBits(row[8])), bounds);
            byte[] result = new byte[8];
            Put(result, 0, BitConverter.SingleToUInt32Bits(point.X));
            Put(result, 4, BitConverter.SingleToUInt32Bits(point.Y));
            Assert.Equal(row[12], Hex(result));
            Assert.Equal(before, Hex(body));
            count++;
        }

        Assert.Equal(399, count);
    }

    /// <summary>실제 현재 graph/owner를 고르고 시작점 없음은 기존 timeout을 변경하지 않는다. 2026-11-25 R4-5b 다리 후보 이식에서 추가됐다.</summary>
    [Fact]
    public void ComposesStartAndAdvance()
    {
        var scene = new CandidateScene(["1", "97", "80", "50", "50", "2", "0", "0", "1", "7", "2", "1", "165", "0", "", "0"]);
        byte[] body = scene.Memory.Bytes(scene.Token);
        scene.Field(0x298, 7);
        scene.Field(0x260, 2);
        scene.Field(0x3d0, BitConverter.SingleToUInt32Bits(-10000f));
        scene.Field(0x3d4, BitConverter.SingleToUInt32Bits(-10000f));
        scene.Field(0x3d8, 9);
        var finder = new ContainedFinderState();
        double now = 0;
        int limit = 256;
        uint logKind = 0;
        uint marker = 0;
        var bounds = new AiBuildSearchBounds(49.75f, 61.5f, 55.25f, 67.5f);
        var hooks = new AiBridgePathHooks();
        hooks.Start = (token, target) => scene.Candidates().Start(token, target, bounds);
        var path = new RawAiBridgePath(scene.Memory, scene.Pool, finder, new AiBridgePathGlobals
        {
            Now = () => now,
            MapLimit = () => limit,
            BridgeType = () => scene.Bridge,
            LogKind = () => logKind,
            StartMarker = () => marker,
            EndMarker = () => marker,
            TargetMarker = () => marker,
        }, hooks);
        Assert.True(path.Advance(scene.Token, new RawPathPoint(52.9f, 64.1f)));
        Assert.Equal(52f, BitConverter.UInt32BitsToSingle(Get(body, 0x3d0)));
        Assert.Equal(64f, BitConverter.UInt32BitsToSingle(Get(body, 0x3d4)));
        Assert.Equal(9u, Get(body, 0x3d8));
        scene.Field(0x3d0, BitConverter.SingleToUInt32Bits(-10000f));
        scene.Field(0x3d4, BitConverter.SingleToUInt32Bits(-10000f));
        scene.Field(0x298, 0x107);
        Assert.True(path.Advance(scene.Token, new RawPathPoint(52.9f, 64.1f)));
        Assert.Equal(-10000f, BitConverter.UInt32BitsToSingle(Get(body, 0x3d0)));
        Assert.Equal(-10000f, BitConverter.UInt32BitsToSingle(Get(body, 0x3d4)));
        Assert.Equal(9u, Get(body, 0x3d8));
    }
}
