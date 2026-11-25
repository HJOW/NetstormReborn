// 원본 공용 난수/다리 주변 spot·지도/진행 예산·30후보 압축을 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 스캔을 검사하는 클래스다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.
// 근거: cpppj tests/AiDropScanTests.cpp의 16건 전부다.
public sealed class RawAiDropScanTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>실제 본체/슬롯·전체 지도와 원본 공용 난수에 명시 하위 효과를 연결한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    private sealed class DropScanScene
    {
        /// <summary>같은 실제 본체 핸들이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x52000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>현재 frame 표다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>같은 현재 원본 spot이다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>같은 현재 원본 표면이다.</summary>
        public readonly ushort[] Surfaces = new ushort[65536];

        /// <summary>같은 현재 원본 동맹이다.</summary>
        public readonly uint[] Allies = new uint[81];

        /// <summary>현재 공용 난수다.</summary>
        public readonly GameRandom Random = new(0x12345678u);

        /// <summary>현재 다리 TYPE다.</summary>
        public uint Bridge = 82;

        /// <summary>현재 CRT 스캔 경계다.</summary>
        public AiBuildSearchBounds Bounds = new(10, 10, 246, 246);

        /// <summary>명시 실제 하위다.</summary>
        public AiDropScanHooks Hooks = new();

        /// <summary>실제 free 반환값이다.</summary>
        public bool Free = true;

        /// <summary>실제 승인 여부다.</summary>
        public bool Accepted;

        /// <summary>명시 점수 목록이다.</summary>
        public List<int> Values = [];

        /// <summary>각 하위에서 달라지는 명시 효과다.</summary>
        public Action<ulong>? During;

        /// <summary>실제 free 호출 수다.</summary>
        public int Frees;

        /// <summary>실제 승인 호출 수다.</summary>
        public int Considers;

        /// <summary>실제 승인 좌표다.</summary>
        public readonly List<RawPathPoint> Points = [];

        /// <summary>실제 진단 기록이다.</summary>
        public readonly List<uint> Diagnostics = [];

        /// <summary>SID 5는 J/K 다리, SID 6은 일반 자기 표면이며 원본 진행은 10/10이다.</summary>
        public DropScanScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            // 실제 슬롯을 확보하고 입력 바이트만 준비하는 반복문이다.
            for (ushort id = 5; id <= 6; id++)
            {
                Span<byte> raw = Pool.RawSlot(id);
                raw.Fill(0);
                raw[10] = id == 5 ? (byte)82 : (byte)94;
                raw[34] = 1;
            }

            Frames[82] = new RiftTypeFrames([new RiftFrameCode((byte)'J', 0, 0, 0), new RiftFrameCode((byte)'K', 0, 0, 0), new RiftFrameCode((byte)'A', 0, 0, 0)]);
            Field(0x260, 1);
            Field(0x2c8, 1);
            Field(0xaf8, 10);
            Field(0xafc, 10);
            Allies[1 * 9 + 1] = 1;
            Hooks.Free = (type, x, y) =>
            {
                Assert.Equal(80u, type);
                Frees++;
                return Free;
            };
            Hooks.Consider = (uint brain, uint type, RawPathPoint point, int direction, ref RawPathPoint best, ref Sid target) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(80u, type);
                Assert.Equal(2, direction);
                Assert.Equal(new Sid(0), target);
                Points.Add(point);
                ulong call = (ulong)Considers++;
                During?.Invoke(call);
                if (!Accepted)
                {
                    return false;
                }

                best = point;
                target = new Sid(9);
                int value = Values.Count == 0 ? 10 : Values[(int)Math.Min(call, (ulong)(Values.Count - 1))];
                Field(0x3e8, unchecked((uint)value));
                return true;
            };
            Hooks.Assertion = (text, line) =>
            {
                Assert.Equal("totalChoices > 0", text);
                Assert.Equal(0x1acu, line);
                Diagnostics.Add(line);
            };
        }

        /// <summary>같은 현재 본체 DWORD만 입력한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>현재 다리의 물리 프레임을 선택한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <param name="value">쓸 프레임 번호다.</param>
        public void Frame(uint value) => Put(Pool.RawSlot(5), 36, value);

        /// <summary>명시 좌표의 실제 surface/spot을 입력한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <param name="x">쓸 가로 칸이다.</param>
        /// <param name="y">쓸 세로 칸이다.</param>
        /// <param name="surface">쓸 표면 SID다.</param>
        /// <param name="spot">쓸 spot이다.</param>
        public void Cell(int x, int y, Sid surface, byte spot = 2)
        {
            int at = y * 256 + x;
            Surfaces[at] = surface.Value;
            Spots[at] = spot;
        }

        /// <summary>현재 선택 배열의 한 항목을 명시 입력한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <param name="index">쓸 후보 번호다.</param>
        /// <param name="x">쓸 X 좌표다.</param>
        /// <param name="y">쓸 Y 좌표다.</param>
        /// <param name="target">쓸 표적 SID 번호다.</param>
        public void Choice(int index, float x, float y, uint target)
        {
            Field(0x408 + index * 8, BitConverter.SingleToUInt32Bits(x));
            Field(0x40c + index * 8, BitConverter.SingleToUInt32Bits(y));
            Field(0x4f8 + index * 4, target);
        }

        /// <summary>같은 실제 저장소·난수를 빌린 처리기를 만든다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
        /// <returns>검사할 배치 스캔 몸체다.</returns>
        public RawAiDropScan Actions() => new(Memory, Pool, Types, Frames, Spots, Surfaces, Allies, Random, new AiDropScanGlobals
        {
            BridgeType = () => Bridge,
            Bounds = () => Bounds,
        }, Hooks);
    }

    /// <summary>J는 위쪽 세 칸을 우선하며 기준 Y 쓰기 뒤 X에 공용 난수 3을 더한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeJPreferredSideAndSharedRandom()
    {
        var scene = new DropScanScene();
        var expected = new GameRandom(scene.Random.State());
        uint draw = expected.Next(3u);
        int x = 20;
        int y = 30;
        Assert.True(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(20 + (int)draw, x);
        Assert.Equal(29, y);
        Assert.Equal(expected.State(), scene.Random.State());
    }

    /// <summary>전체 BYTE 0을 검사하므로 고위 spot 비트도 위쪽을 막고 아래쪽을 선택한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeJWholeByteFallbackAndFailureNoRandom()
    {
        var scene = new DropScanScene();
        scene.Spots[29 * 256 + 20] = 0x40;
        var expected = new GameRandom(scene.Random.State());
        uint draw = expected.Next(3u);
        int x = 20;
        int y = 30;
        Assert.True(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(20 + (int)draw, x);
        Assert.Equal(33, y);
        uint state = scene.Random.State();
        scene.Spots[31 * 256 + 20] = 1;
        x = 20;
        y = 30;
        Assert.False(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(20, x);
        Assert.Equal(30, y);
        Assert.Equal(state, scene.Random.State());
    }

    /// <summary>K는 왼쪽 우선/오른쪽 대체와 세로 난수이며 선형 행 이웃을 좌표 clamp하지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeKAndPhysicalRowBoundary()
    {
        var scene = new DropScanScene();
        scene.Frame(1);
        var expected = new GameRandom(scene.Random.State());
        uint draw = expected.Next(3u);
        int x = 0;
        int y = 30;
        Assert.True(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(-1, x);
        Assert.Equal(30 + (int)draw, y);
        scene.Spots[30 * 256 + 19] = 1;
        x = 20;
        y = 30;
        uint next = expected.Next(3u);
        Assert.True(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(23, x);
        Assert.Equal(30 + (int)next, y);
    }

    /// <summary>다른 TYPE는 프레임을 읽지 않고 J/K 이외 코드는 난수를 쓰지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeCurrentTypeAndActualFrameCode()
    {
        var scene = new DropScanScene();
        int x = 20;
        int y = 30;
        Put(scene.Pool.RawSlot(new Sid(6).Value), 36, 0xffffffffu);
        Assert.False(scene.Actions().AdjustBridge(new Sid(6), ref x, ref y));
        scene.Frame(2);
        uint state = scene.Random.State();
        Assert.False(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(state, scene.Random.State());
        scene.Frame(0);
        scene.Bridge = 83;
        Assert.False(scene.Actions().AdjustBridge(new Sid(5), ref x, ref y));
        Assert.Equal(state, scene.Random.State());
    }

    /// <summary>스캔은 X를 먼저 증가시키고 inclusive 우측/하단 소진 뒤 X/Y를 좌상단으로 되돌린다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void CursorPreincrementAndInclusiveCompletion()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 13, 10);
        scene.Cell(10, 10, new Sid(5));
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(10u, scene.Field(0xaf8));
        Assert.Equal(10u, scene.Field(0xafc));
        Assert.Equal(0, scene.Considers);
        scene.Field(0xaf8, 12);
        scene.Cell(13, 10, new Sid(5));
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(1, scene.Considers);
        Assert.Equal(9f, scene.Points[0].Y);
    }

    /// <summary>빈 지도에서 일반 1000/공급 500 예산을 정확히 차감하며 재개 위치를 보존한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void EmptyMapNormalAndSupplyBudgets()
    {
        var normal = new DropScanScene();
        Assert.True(normal.Actions().Scan(normal.Token, 80, 2));
        Assert.Equal(62u, normal.Field(0xaf8));
        Assert.Equal(14u, normal.Field(0xafc));
        var supply = new DropScanScene();
        supply.Types[80].Flags2 = 0x800;
        Assert.True(supply.Actions().Scan(supply.Token, 80, 2));
        Assert.Equal(36u, supply.Field(0xaf8));
        Assert.Equal(12u, supply.Field(0xafc));
    }

    /// <summary>다리 후보 한 번은 5+125+1을 쓰며 승인 실패도 125를 차감한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeBudgetConsumesSharedRandomOnRejectedCandidates()
    {
        // 일반은 8회/공급은 4회 뒤 예산이 소진되며 같은 공용 난수도 그 횟수만큼 전진한다.
        foreach (bool supply in new[] { false, true })
        {
            var scene = new DropScanScene();
            Array.Fill(scene.Surfaces, (ushort)5);
            if (supply)
            {
                scene.Types[80].Flags2 = 0x800;
            }

            var expected = new GameRandom(scene.Random.State());
            uint count = supply ? 4u : 8u;
            for (uint index = 0; index < count; index++)
            {
                expected.Next(3u);
            }

            Assert.True(scene.Actions().Scan(scene.Token, 80, 2));
            Assert.Equal((int)count, scene.Considers);
            Assert.Equal(0, scene.Frees);
            Assert.Equal(10 + count, scene.Field(0xaf8));
            Assert.Equal(expected.State(), scene.Random.State());
        }
    }

    /// <summary>동맹 행은 surfaceOwner*9+owner이며 자기 발자국 실패도 추가 10을 쓴다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void DirectedAllianceAndOwnFootprintBudget()
    {
        var own = new DropScanScene();
        Array.Fill(own.Surfaces, (ushort)6);
        Array.Fill(own.Spots, (byte)2);
        own.Types[80].Flags2 = 0x800;
        own.Free = false;
        Assert.True(own.Actions().Scan(own.Token, 80, 2));
        Assert.Equal(32, own.Frees);
        Assert.Equal(0, own.Considers);
        Assert.Equal(42u, own.Field(0xaf8));
        var ally = new DropScanScene();
        Array.Fill(ally.Surfaces, (ushort)6);
        Array.Fill(ally.Spots, (byte)2);
        ally.Types[80].Flags2 = 0x800;
        ally.Pool.RawSlot(new Sid(6).Value)[34] = 2;
        ally.Allies[2 * 9 + 1] = 1;
        Assert.True(ally.Actions().Scan(ally.Token, 80, 2));
        Assert.Equal(0, ally.Frees);
        Assert.Equal(0, ally.Considers);
        Assert.Equal(42u, ally.Field(0xaf8));
        var reverse = new DropScanScene();
        Array.Fill(reverse.Surfaces, (ushort)6);
        Array.Fill(reverse.Spots, (byte)2);
        reverse.Types[80].Flags2 = 0x800;
        reverse.Pool.RawSlot(new Sid(6).Value)[34] = 2;
        reverse.Allies[1 * 9 + 2] = 1;
        Assert.True(reverse.Actions().Scan(reverse.Token, 80, 2));
        Assert.Equal(0, reverse.Frees);
        Assert.Equal(94u, reverse.Field(0xaf8));
    }

    /// <summary>flags/spot/2c8 관문이 막히면 전체 발자국과 후보 승인을 호출하지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void NonbridgeEligibilityGatesAndMaskedSpot()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 13, 10);
        scene.Types[80].Flags2 = 0x800;
        scene.Cell(11, 10, new Sid(6), 0x82);
        scene.Cell(12, 10, new Sid(6), 0x42);
        scene.Cell(13, 10, new Sid(6), 0x40);
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(2, scene.Frees);
        Assert.Equal(2, scene.Considers);
        scene.Field(0x2c8, 0);
        scene.Frees = 0;
        scene.Considers = 0;
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(0, scene.Frees);
        Assert.Equal(0, scene.Considers);
    }

    /// <summary>같은 호출의 경계/시설 flags는 하위 효과 뒤에도 최초값이며 진행은 본체에 남는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void BoundsAndFlagsArePerCallSnapshot()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 13, 10);
        scene.Types[80].Flags2 = 0x800;
        scene.Cell(11, 10, new Sid(6));
        scene.Cell(12, 10, new Sid(6));
        scene.Cell(13, 10, new Sid(6));
        scene.During = call =>
        {
            if (call == 0)
            {
                scene.Bounds = new AiBuildSearchBounds(100, 100, 102, 102);
                scene.Types[80].Flags2 = 0;
            }
        };
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(3, scene.Frees);
        Assert.Equal(3, scene.Considers);
        Assert.Equal(10u, scene.Field(0xaf8));
        Assert.Equal(10u, scene.Field(0xafc));
    }

    /// <summary>엄격한 점수 개선은 목록을 다시 시작하고 동률은 추가한다. 미사용 꼬리는 지우지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void ImprovementResetsAndEqualScoreAppends()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 13, 10);
        scene.Types[80].Flags2 = 0x800;
        scene.Accepted = true;
        scene.Values = [20, 20, 30];
        scene.Field(0x570, 2);
        scene.Field(0x3e8, 10);
        scene.Choice(0, 1, 2, 7);
        scene.Choice(1, 3, 4, 8);
        scene.Cell(11, 10, new Sid(6));
        scene.Cell(12, 10, new Sid(6));
        scene.Cell(13, 10, new Sid(6));
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(30u, scene.Field(0x3e8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(13f), scene.Field(0x408));
        Assert.Equal(9u, scene.Field(0x4f8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(12f), scene.Field(0x410));
        Assert.Equal(9u, scene.Field(0x4fc));
    }

    /// <summary>가득찬 30개 동률 목록은 같은 공용 난수로 한 칸만 교체하고 count를 증가시키지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FullThirtyChoicesRandomReplacement()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 11, 10);
        scene.Types[80].Flags2 = 0x800;
        scene.Cell(11, 10, new Sid(6));
        scene.Accepted = true;
        scene.Field(0x570, 30);
        scene.Field(0x3e8, 10);
        // 미선택 29칸의 원문도 보존되는지 각각 비교하는 반복문이다.
        for (int index = 0; index < 30; index++)
        {
            scene.Choice(index, 100 + index, 200, (uint)(1000 + index));
        }

        var expected = new GameRandom(scene.Random.State());
        uint selected = expected.Next(30u);
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(30u, scene.Field(0x570));
        Assert.Equal(expected.State(), scene.Random.State());
        // 선택된 칸 이외에는 좌표와 표적 어느 것도 재작성하지 않는 반복문이다.
        for (int index = 0; index < 30; index++)
        {
            Assert.Equal(BitConverter.SingleToUInt32Bits(index == selected ? 11.0f : 100 + index), scene.Field(0x408 + index * 8));
            Assert.Equal(index == selected ? 9u : (uint)(1000 + index), scene.Field(0x4f8 + index * 4));
        }
    }

    /// <summary>승인 거부 하위가 변경한 실제 점수/count도 사본으로 되돌리지 않는다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectedCandidateKeepsCurrentStoredEffects()
    {
        var scene = new DropScanScene();
        scene.Bounds = new AiBuildSearchBounds(10, 10, 11, 10);
        scene.Types[80].Flags2 = 0x800;
        scene.Cell(11, 10, new Sid(6));
        scene.During = _ =>
        {
            scene.Field(0x3e8, 123);
            scene.Field(0x570, 3);
        };
        Assert.False(scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.Equal(3u, scene.Field(0x570));
    }

    /// <summary>후보 하위가 없으면 선증가된 진행과 이미 소비한 다리 난수는 보존한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingConsiderKeepsCursorAndBridgeRandom()
    {
        var scene = new DropScanScene();
        scene.Cell(11, 10, new Sid(5));
        scene.Hooks.Consider = null;
        var expected = new GameRandom(scene.Random.State());
        expected.Next(3u);
        Assert.ThrowsAny<Exception>(() => scene.Actions().Scan(scene.Token, 80, 2));
        Assert.Equal(11u, scene.Field(0xaf8));
        Assert.Equal(expected.State(), scene.Random.State());
        Assert.Equal(0u, scene.Field(0x570));
    }

    /// <summary>선택 제거는 count 선감소 후 XY/표적을 안정 압축하고 꼬리/점수/진행을 보존한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void RemoveStableCompactionAndUntouchedTail()
    {
        var scene = new DropScanScene();
        scene.Field(0x570, 3);
        scene.Field(0x3e8, 99);
        scene.Choice(0, 1, 2, 7);
        scene.Choice(1, 3, 4, 8);
        scene.Choice(2, 5, 6, 9);
        scene.Actions().Remove(scene.Token, 1);
        Assert.Equal(2u, scene.Field(0x570));
        Assert.Equal(BitConverter.SingleToUInt32Bits(5f), scene.Field(0x410));
        Assert.Equal(9u, scene.Field(0x4fc));
        Assert.Equal(BitConverter.SingleToUInt32Bits(5f), scene.Field(0x418));
        Assert.Equal(9u, scene.Field(0x500));
        Assert.Equal(99u, scene.Field(0x3e8));
        Assert.Equal(10u, scene.Field(0xaf8));
        scene.Actions().Remove(scene.Token, 999);
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1f), scene.Field(0x408));
    }

    /// <summary>count 0 진단이 돌아오면 원본처럼 감소를 계속하며 누락 진단은 앞에서 실패한다. 2026-11-25 R4-5b 배치 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void RemoveDiagnosticContinuationAndPrefix()
    {
        var scene = new DropScanScene();
        scene.Actions().Remove(scene.Token, 0);
        Assert.Equal(0xffffffffu, scene.Field(0x570));
        Assert.Equal(new List<uint> { 428u }, scene.Diagnostics);
        scene.Field(0x570, 0);
        scene.Hooks.Assertion = null;
        Assert.ThrowsAny<Exception>(() => scene.Actions().Remove(scene.Token, 0));
        Assert.Equal(0u, scene.Field(0x570));
        scene.Field(0x570, 2);
        Assert.ThrowsAny<Exception>(() => scene.Actions().Remove(scene.Token, -1));
        Assert.Equal(1u, scene.Field(0x570));
    }
}
