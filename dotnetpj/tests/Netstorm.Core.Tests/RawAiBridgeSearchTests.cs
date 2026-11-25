// 실제 원본 모양/점유/끝점 점수/8개 예산의 독립 전체 바이트를 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 다리 검색을 검사하는 클래스다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.
// 근거: cpppj tests/AiBridgeSearchTests.cpp의 5건 전부다.
public sealed class RawAiBridgeSearchTests
{
    /// <summary>보고서의 실제 bridge.type 코드 입력 240BYTE이며 모양/점수 결과가 아니다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private const string BridgeCodes = "4150010041500b2041501440425001004250020042500b2042500c2042501440435001004350020043500b2043500c2043501440445001004450020044500b2044500c2044501440455001004550020045500b2045500c20455014404650010046500b20465014404750010047500b20475014404850010048500b20485014404950010049500b20495014404a5001004a5002004a500b204a500b204a500c204a5014404b5001004b5002004b500b204b500b204b500c204b5014404c5001004c500b204c5014404d5001004d500b204d5014404e5001004e500b204e5014404f5001004f500b204f50144050500000";

    /// <summary>원문 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>fixture의 XY 두 float 비트는 NaN/음수 0도 바꾸지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="text">자를 16진 문자열이다.</param>
    /// <returns>읽은 좌표다.</returns>
    private static RawPathPoint Point(string text)
    {
        byte[] raw = Convert.FromHexString(text);
        return new RawPathPoint(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0, 4))), BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(4, 4))));
    }

    /// <summary>임시 XY 출력 전체 비트를 원문과 비교한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    /// <param name="point">바꿀 좌표다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string PointHex(RawPathPoint point)
    {
        byte[] raw = new byte[8];
        Put(raw, 0, BitConverter.SingleToUInt32Bits(point.X));
        Put(raw, 4, BitConverter.SingleToUInt32Bits(point.Y));
        return Hex(raw);
    }

    /// <summary>결과를 계산하지 않고 현재 caller의 모양 프레임/spot 입력만 조립한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    private sealed class SearchScene
    {
        /// <summary>실제 본체의 유일 소유자다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x55000000u, bytes => bytes.Fill(0));

        /// <summary>실제 0xb08 본체다.</summary>
        public readonly uint Token;

        /// <summary>현재 TYPE별 실제 코드 입력이다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>현재 점유 BYTE 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>원본과 같은 bridge 선택이며 타입 번호 자체는 코드 결과에 관여하지 않는다.</summary>
        public uint Bridge = 82;

        /// <summary>한 장면의 실제 공용 난수다.</summary>
        public readonly GameRandom Random = new();

        /// <summary>보호 원본에서 추출한 프레임 코드 입력만 구성한다.</summary>
        public SearchScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            byte[] bytes = Convert.FromHexString(BridgeCodes);
            var codes = new List<RiftFrameCode>();
            // 실제 코드 순서/중복을 유지하는 반복문이다.
            for (int index = 0; index < bytes.Length; index += 4)
            {
                codes.Add(new RiftFrameCode(bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3]));
            }

            Frames[Bridge] = new RiftTypeFrames(codes);
        }

        /// <summary>제품과 검사 모두 같은 현재 저장소를 참조한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
        /// <returns>검사할 다리 검색 몸체다.</returns>
        public RawAiBridgeSearch Search() => new(Memory, Frames, Spots, () => Bridge, Random);

        /// <summary>원본 관측에 쓴 지도 입력이며 후보 점수/배치를 만들지 않는다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
        /// <param name="mode">고를 지도 모드다.</param>
        /// <param name="search">검색 입력이면 참이다.</param>
        public void SpotsFill(uint mode, bool search)
        {
            // 행 순서를 원본 flat 주소와 같게 하는 반복문이다.
            for (uint y = 0; y < 256; y++)
            {
                for (uint x = 0; x < 256; x++)
                {
                    // 비대칭 입력으로 x/y 전도를 검출하는 계산이다.
                    Spots[(int)(y * 256 + x)] = (byte)(mode == 1 ? (search ? 0x80 : 1) : mode == 2 ? ((x + 3 * y) % 11 == 0 ? (search ? 0x40 : 0x80) : 0) : mode == 3 && x == 50 ? 0x40 : 0);
                }
            }
        }
    }

    /// <summary>26개 패턴×4회전에서 grid/끝점 여백 두 초기값을 포함한 0xbc 전체를 대조한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    [Fact]
    public void Native208ShapeBuffers()
    {
        var scene = new SearchScene();
        string[][] rows = X86Fixture.Read("ai-bridge-shape");
        int count = 0;
        // 원본 입력만 제품에 공급하며 끝점 기대를 재계산하지 않는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            byte[] raw = new byte[RawAiBridgeSearch.ShapeBytes];
            raw.AsSpan().Fill((byte)X86Fixture.UInt(row[2]));
            scene.Search().Shape(X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]), raw);
            Assert.Equal(row[3], Hex(raw));
            count++;
        }

        Assert.Equal(208, count);
    }

    /// <summary>실제 decoder 순서와 비대칭 점유/소수 좌표에서 첫 차단 반환을 대조한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    [Fact]
    public void Native416ClearReturns()
    {
        var scene = new SearchScene();
        string[][] rows = X86Fixture.Read("ai-bridge-clear");
        int count = 0;
        // 모든 26개 조각/4회전/4지도 입력을 실제 계산하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            scene.SpotsFill(X86Fixture.UInt(row[4]), false);
            var placement = new AiBridgePlacement(new RawPathPoint(X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3])), X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]));
            Assert.Equal(row[5] != "0", scene.Search().Clear(placement));
            count++;
        }

        Assert.Equal(416, count);
    }

    /// <summary>x87의 X 보존/Y float 저장·signed 감김·금지/벌점·미갱신 결과를 대조한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    [Fact]
    public void Native640EndpointScores()
    {
        var scene = new SearchScene();
        string[][] rows = X86Fixture.Read("ai-bridge-score");
        int count = 0;
        // shape는 검증된 실제 제품 helper로 만들고 독립 점수/출력과 비교하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            byte[] shape = new byte[RawAiBridgeSearch.ShapeBytes];
            scene.Search().Shape(X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]), shape);
            var placement = new AiBridgePlacement(new RawPathPoint(X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3])), X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]));
            byte fill = (byte)X86Fixture.UInt(row[7]);
            uint bits = unchecked((uint)fill * 0x01010101u);
            var scratch = new RawPathPoint(BitConverter.UInt32BitsToSingle(bits), BitConverter.UInt32BitsToSingle(bits));
            int score = scene.Search().Score(placement, unchecked((int)X86Fixture.UInt(row[4])), unchecked((int)X86Fixture.UInt(row[5])), shape, X86Fixture.UInt(row[6]), ref scratch);
            Assert.Equal(X86Fixture.UInt(row[8]), unchecked((uint)score));
            Assert.Equal(row[9], PointHex(scratch));
            count++;
        }

        Assert.Equal(640, count);
    }

    /// <summary>72장면을 각 4번 재개하며 본체 0xb08/공용 난수/임시 XY/완료값을 모두 비교한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    [Fact]
    public void Native288BudgetedFullBodies()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-search");
        int count = 0;
        int returns = 0;
        // native 입력의 실제 초기 본체를 그대로 설치하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            var scene = new SearchScene();
            scene.SpotsFill(X86Fixture.UInt(row[1]), true);
            scene.Random.SetState(X86Fixture.UInt(row[2]));
            RawPathPoint scratch = Point(row[3]);
            byte[] body = Convert.FromHexString(row[4]);
            body.CopyTo(scene.Memory.Bytes(scene.Token), 0);
            // 변경된 실제 본체/난수를 다음 호출에 이어 쓰며 결과를 덮어주지 않는 반복문이다.
            foreach (string observation in row[5].Split(';'))
            {
                string[] expected = observation.Split(',');
                Assert.Equal(4, expected.Length);
                bool complete = scene.Search().Search(scene.Token, ref scratch);
                bool same = complete == (expected[0] != "0") && scene.Random.State() == X86Fixture.UInt(expected[1]) && PointHex(scratch) == expected[2] && Hex(scene.Memory.Bytes(scene.Token)) == expected[3];
                Assert.True(same);
                returns++;
            }

            count++;
        }

        Assert.Equal(72, count);
        Assert.Equal(288, returns);
    }

    /// <summary>실제 초기화→24칸 후보→8개 검색→대기→결과 반환을 원본 Step과 함께 수행한다. 2026-11-25 R4-5b 다리 검색 이식에서 추가됐다.</summary>
    [Fact]
    public void ComposesActualPreparationAndStep()
    {
        var scene = new SearchScene();
        var pool = new RawSidPool(32768);
        Span<byte> slot = pool.RawSlot(50);
        slot.Fill(0);
        slot[RawSidPool.TypeOffset] = 97;
        slot[30] = 7;
        slot[34] = 2;
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[97].Flags1 = 0x800;
        scene.Frames[97] = new RiftTypeFrames([new RiftFrameCode((byte)'P', (byte)'A', 0, 0)]);
        var surfaces = new ushort[65536];
        Array.Fill(surfaces, (ushort)50);
        var candidates = new RawAiBridgeCandidates(scene.Memory, pool, types, scene.Frames, surfaces, scene.Spots, new AiBridgeCandidateTypes
        {
            Bridge = () => scene.Bridge,
            Fencemark = () => 90,
            WindArcher = () => 140,
        });
        var finder = new ContainedFinderState();
        double now = 0;
        int limit = 256;
        uint logKind = 0;
        uint marker = 0;
        var scratch = new RawPathPoint(-10000, -10000);
        var hooks = new AiBridgePathHooks();
        hooks.Initialize = (token, source, target) =>
        {
            var path = new RawAiBridgePath(scene.Memory, pool, finder, new AiBridgePathGlobals
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
        hooks.Prepare = token => candidates.Prepare(token);
        hooks.Search = token => scene.Search().Search(token, ref scratch);
        var path = new RawAiBridgePath(scene.Memory, pool, finder, new AiBridgePathGlobals
        {
            Now = () => now,
            MapLimit = () => limit,
            BridgeType = () => scene.Bridge,
            LogKind = () => logKind,
            StartMarker = () => marker,
            EndMarker = () => marker,
            TargetMarker = () => marker,
        }, hooks);
        byte[] body = scene.Memory.Bytes(scene.Token);
        Put(body, 0x5f0, 1);
        Put(body, 0x5f8, 7);
        Put(body, 0x5fc, 2);
        Put(body, 0x604, 8);
        Put(body, 0x628, 8);
        // 현재 재고 8개를 설치하며 결과 조각을 미리 고르지 않는 반복문이다.
        for (uint index = 0; index < 8; index++)
        {
            Put(body, 0x608 + (int)index * 4, index);
        }

        var source = new RawPathPoint(50, 63);
        bool complete = false;
        bool pending = false;
        // 원본 단계마다 한 번 처리하며 실제 예산 중단을 확인하는 반복문이다.
        for (int call = 0; call < 100 && !complete; call++)
        {
            uint stage = Get(body, 0x5f0);
            complete = path.Step(scene.Token, ref source, new RawPathPoint(80, 90));
            if (stage == 3 && Get(body, 0x5f0) == 3)
            {
                pending = true;
            }
        }

        Assert.True(complete && pending && Get(body, 0x5f0) == 0);
        Assert.True(source.X > 0 && source.Y > 0 && (source.X != 50 || source.Y != 63));
        Assert.True(Get(body, 0x6a8) < 65536 && Get(body, 0x6b4) < 8 && Get(body, 0x6b8) <= 6);
    }
}
