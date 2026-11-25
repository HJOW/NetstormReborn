// 원본 기술 순환/회전·직전 후보 재사용/현재 상태·unordered 기한을 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 선택을 검사하는 클래스다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.
// 근거: cpppj tests/AiDropSelectionTests.cpp의 15건 전부다.
public sealed class RawAiDropSelectionTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 실제 본체/후보 압축·공용 난수와 명시 기술 getter를 준비한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    private sealed class DropSelectionScene
    {
        /// <summary>본체와 직전 표적 목록의 실제 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x53000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>직전 표적 목록 backing이다.</summary>
        public readonly uint List;

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

        /// <summary>현재 CRT 스캔 경계다.</summary>
        public AiBuildSearchBounds Bounds = new(10, 10, 246, 246);

        /// <summary>현재 다리 TYPE다.</summary>
        public uint Bridge = 82;

        /// <summary>현재 공용 난수다.</summary>
        public readonly GameRandom Random = new(1u);

        /// <summary>현재 시각이다.</summary>
        public double Now = 100;

        /// <summary>네 TYPE 전역이다.</summary>
        public readonly uint[] Directional = [131, 129, 140, 142];

        /// <summary>기술 부모다.</summary>
        public Sid Parent = new(5);

        /// <summary>현재 블로커 조각이다.</summary>
        public Sid Blocker;

        /// <summary>기술 개수다.</summary>
        public int Count = 2;

        /// <summary>실제 ordinal 반환 목록이다.</summary>
        public List<uint> Products = [128, 129];

        /// <summary>선택된 실제 하위다.</summary>
        public AiDropSelectionHooks Hooks = new();

        /// <summary>같은 후보 압축기다.</summary>
        public readonly RawAiDropScan Scan;

        /// <summary>선택된 하위의 호출 기록이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>실제 승인 쿼리다.</summary>
        public readonly List<CanonPlacementQuery> Queries = [];

        /// <summary>실제 승인 허용 목록이다.</summary>
        public List<bool> Legal = [];

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action? DuringTechnology;

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action? DuringCount;

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action? DuringBlocker;

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action? DuringNth;

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action<AiDropSelectionLog>? DuringLog;

        /// <summary>각 원문 하위의 명시 현재 상태 변경이다.</summary>
        public Action<int>? DuringPlace;

        /// <summary>기본은 새 순환 선택/빈 후보/직전 점수 123이며 최초 ordinal 0에서 1로 진행한다.</summary>
        public DropSelectionScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            List = Memory.Allocate(32);
            Pool.RawSlot(5).Fill(0);
            Field(0x260, 1);
            Field(0x36c, List);
            Field(0x3e8, 123);
            Field(0x3f0, 9);
            Field(0x3ec, 7);
            Field(0xaf8, 8);
            Field(0xafc, 9);
            Scan = new RawAiDropScan(Memory, Pool, Types, Frames, Spots, Surfaces, Allies, Random, new AiDropScanGlobals
            {
                BridgeType = () => Bridge,
                Bounds = () => Bounds,
            }, new AiDropScanHooks
            {
                Assertion = (_, _) => throw new InvalidOperationException("명시 후보 압축 진단"),
            });
            Hooks.Technology = owner =>
            {
                Assert.Equal(10u, Field(0xaf8));
                Assert.Equal(10u, Field(0xafc));
                Assert.Equal(0u, Field(0x3ec));
                Trace.Add("tech:" + owner);
                Sid saved = Parent;
                DuringTechnology?.Invoke();
                return saved;
            };
            Hooks.Count = technology =>
            {
                Trace.Add("count:" + technology.Value);
                int saved = Count;
                DuringCount?.Invoke();
                return saved;
            };
            Hooks.Blocker = technology =>
            {
                Trace.Add("blocker:" + technology.Value);
                DuringBlocker?.Invoke();
                return Blocker;
            };
            Hooks.Nth = (technology, index) =>
            {
                Trace.Add("nth:" + technology.Value + ":" + index);
                DuringNth?.Invoke();
                return index < Products.Count ? Products[(int)index] : 0u;
            };
            Hooks.MayPlace = query =>
            {
                int index = Queries.Count;
                Queries.Add(query);
                Trace.Add("place:" + (int)query.X);
                DuringPlace?.Invoke(index);
                return Legal.Count == 0 || Legal[(int)Math.Min(index, Legal.Count - 1)];
            };
            Hooks.Remove = (brain, index) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("remove:" + index);
                Scan.Remove(brain, index);
            };
            Hooks.Log = (brain, action, type, direction) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add(action == AiDropSelectionLog.Hated ? "hate" : action == AiDropSelectionLog.Retried ? "retry" : "scan:" + type + ":" + direction);
                DuringLog?.Invoke(action);
            };
        }

        /// <summary>지정 본체 DWORD만 입력한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>원본 QWORD 기한을 축소 없이 입력한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 기한이다.</param>
        public void Time(int at, double value)
        {
            ulong raw = BitConverter.DoubleToUInt64Bits(value);
            Field(at, (uint)raw);
            Field(at + 4, (uint)(raw >> 32));
        }

        /// <summary>직전 표적 목록의 순서/중복을 그대로 기록한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="values">쓸 표적 목록이다.</param>
        public void SetList(uint[] values)
        {
            Field(0x374, (uint)values.Length);
            int at = 0;
            // 입력 목록 순서를 보존하는 반복문이다.
            foreach (uint value in values)
            {
                Put(Memory.Bytes(List), at, value);
                at += 4;
            }
        }

        /// <summary>실제 같은 후보 XY/표적 배열의 명시 입력이다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="index">쓸 후보 번호다.</param>
        /// <param name="x">쓸 X 좌표다.</param>
        /// <param name="y">쓸 Y 좌표다.</param>
        /// <param name="target">쓸 표적 번호다.</param>
        public void Choice(int index, float x, float y, uint target)
        {
            Field(0x408 + index * 8, BitConverter.SingleToUInt32Bits(x));
            Field(0x40c + index * 8, BitConverter.SingleToUInt32Bits(y));
            Field(0x4f8 + index * 4, target);
        }

        /// <summary>원본 반복 관문을 통과할 명시 직전 상태를 준비한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        /// <param name="type">쓸 직전 TYPE다.</param>
        public void Repeat(uint type = 128)
        {
            Field(0x248, 0x80);
            Field(0x3f4, 1);
            Field(0x3e0, type);
            Field(0x3fc, 77);
            Field(0xb00, 1);
            SetList([77]);
        }

        /// <summary>현재 실제 저장소와 하위로 한 번 선택한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
        public void Select()
        {
            var body = new RawAiDropSelection(Memory, Pool, Types, Directional, Random, () => Now, Hooks);
            Assert.Same(Pool, body.Pool);
            body.Select(Token);
        }
    }

    /// <summary>기술 0개는 진행/완료 표식·TYPE/재선택만 쓰고 후보·점수·직전 상태는 보존한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void EmptyTechnologyOnlyWritesNativePrefix()
    {
        var scene = new DropSelectionScene();
        scene.Count = 0;
        scene.Repeat();
        scene.Field(0x3e4, 6);
        scene.Field(0x570, 2);
        scene.Field(0x404, 1);
        scene.Select();
        Assert.Equal(["tech:1", "count:5"], scene.Trace);
        Assert.Equal(0u, scene.Field(0x3e0));
        Assert.Equal(1u, scene.Field(0x3f0));
        Assert.Equal(6u, scene.Field(0x3e4));
        Assert.Equal(2u, scene.Field(0x570));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.Equal(1u, scene.Field(0x3f4));
        Assert.Equal(77u, scene.Field(0x3fc));
        Assert.Equal(1u, scene.Field(0x404));
        Assert.Equal(1u, scene.Random.State());
    }

    /// <summary>현재 owner 조회는 한 번이고 나머지는 최초 기술 부모/개수를 계속 사용한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CachesActualTechnologyAndCount()
    {
        var scene = new DropSelectionScene();
        scene.Field(0xb00, 1);
        scene.DuringTechnology = () => { scene.Field(0x260, 2); scene.Parent = new Sid(6); };
        scene.DuringCount = () => scene.Count = 0;
        scene.Select();
        Assert.Equal(["tech:1", "count:5", "nth:5:1", "scan:129:0"], scene.Trace);
        Assert.Equal(129u, scene.Field(0x3e0));
        Assert.Equal(1u, scene.Field(0x3dc));
        Assert.Equal(0u, scene.Field(0x3e8));
        Assert.Equal(0u, scene.Field(0x3f0));
        Assert.Equal(1u, scene.Field(0x400));
    }

    /// <summary>네 현재 전역만 전체 DWORD로 비교하고 6 미만 회전은 2씩 증가한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentDirectionalTypesAndRotation()
    {
        var scene = new DropSelectionScene();
        var body = new RawAiDropSelection(scene.Memory, scene.Pool, scene.Types, scene.Directional, scene.Random, () => scene.Now, scene.Hooks);
        Assert.True(body.Directional(131));
        Assert.False(body.Directional(0x10083));
        scene.Directional[0] = 130;
        Assert.True(body.Directional(130));
        Assert.False(body.Directional(131));
        scene.Field(0x3e0, 130);
        scene.Field(0x3e4, 4);
        scene.Select();
        Assert.Equal(130u, scene.Field(0x3e0));
        Assert.Equal(6u, scene.Field(0x3e4));
        Assert.Equal(["tech:1", "count:5"], scene.Trace);
    }

    /// <summary>방향은 signed 비교라 홀수/음수도 원본 그대로 2를 더하고 6 이상은 기술을 순환한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void SignedOddRotationAndOrdinalAdvance()
    {
        var scene = new DropSelectionScene();
        scene.Field(0x3e0, 131);
        scene.Field(0x3e4, 5);
        scene.Select();
        Assert.Equal(7u, scene.Field(0x3e4));
        Assert.Equal(131u, scene.Field(0x3e0));
        scene.Trace.Clear();
        scene.Field(0x3e4, 0xffffffffu);
        scene.Select();
        Assert.Equal(1u, scene.Field(0x3e4));
        scene.Field(0x3e4, 6);
        scene.Select();
        Assert.Equal(129u, scene.Field(0x3e0));
        Assert.Equal(0u, scene.Field(0x3e4));
        Assert.Equal(1u, scene.Field(0x3dc));
    }

    /// <summary>ordinal은 DWORD 증가 뒤 signed 나머지이며 음수는 다른 양수 인덱스로 고치지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void OrdinalWrapAndSignedRemainder()
    {
        var scene = new DropSelectionScene();
        scene.Field(0x3dc, 0xffffffffu);
        scene.Select();
        Assert.Equal(0u, scene.Field(0x3dc));
        Assert.Equal(128u, scene.Field(0x3e0));
        scene.Field(0x3e0, 0);
        scene.Field(0x3dc, 0xfffffffeu);
        scene.Select();
        Assert.Equal(0xffffffffu, scene.Field(0x3dc));
        Assert.Equal(0u, scene.Field(0x3e0));
        Assert.Equal("nth:5:4294967295", scene.Trace[^1]);
    }

    /// <summary>하위 BYTE 80h는 직전 배치가 없어도 난수 100을 쓰며 상위 BYTE 80h는 쓰지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void RepeatByteConsumesRandomBeforeOtherGates()
    {
        var scene = new DropSelectionScene();
        scene.Field(0x248, 0x80);
        var expected = new GameRandom(1u);
        expected.Next(100u);
        scene.Select();
        Assert.Equal(expected.State(), scene.Random.State());
        var higher = new DropSelectionScene();
        higher.Field(0x248, 0x8000);
        higher.Select();
        Assert.Equal(1u, higher.Random.State());
    }

    /// <summary>실제 목록 압축 뒤 같은 index에서 다음 후보를 재검사하고 직전 점수·회전을 유지한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void RepeatCompactsAndRechecksSameIndex()
    {
        var scene = new DropSelectionScene();
        scene.Repeat();
        scene.Field(0x3e4, 4);
        scene.Field(0x570, 3);
        scene.Choice(0, 11, 12, 7);
        scene.Choice(1, 21, 22, 8);
        scene.Choice(2, 31, 32, 9);
        scene.Legal = [false, false, true];
        scene.Select();
        Assert.Equal(["tech:1", "count:5", "hate", "place:11", "remove:0", "place:21", "remove:0", "place:31", "retry", "scan:128:4"], scene.Trace);
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.Equal(0u, scene.Field(0x400));
        Assert.Equal(0u, scene.Field(0x3f4));
        Assert.Equal(0u, scene.Field(0x3fc));
        Assert.Equal(BitConverter.SingleToUInt32Bits(31f), scene.Field(0x408));
        Assert.Equal(9u, scene.Field(0x4f8));
    }

    /// <summary>직전 표적 목록의 80/30 확률 경계와 비어 있지 않은 비일치 목록을 구분한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void RepeatProbabilityBoundaries()
    {
        // 작은 원본 시드는 첫 Next100이 시드와 같으므로 실제 경계 입력으로 쓴다.
        foreach (uint seed in new uint[] { 79, 80, 29, 30 })
        {
            var scene = new DropSelectionScene();
            scene.Repeat();
            scene.Random.SetState(seed);
            if (seed < 40)
            {
                scene.SetList([]);
            }

            scene.Select();
            bool repeated = seed == 79 || seed == 29;
            Assert.Equal(repeated, scene.Trace.Contains("retry"));
        }

        var nonempty = new DropSelectionScene();
        nonempty.Repeat();
        nonempty.SetList([88]);
        nonempty.Select();
        Assert.DoesNotContain("retry", nonempty.Trace);
    }

    /// <summary>재사용 승인 쿼리는 현재 TYPE/owner BYTE이며 회전값은 flags에 넣지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void RepeatUsesCurrentTypeOwnerAndCanonicalFlags()
    {
        var scene = new DropSelectionScene();
        scene.Repeat();
        scene.Field(0x570, 2);
        scene.Choice(0, 11, 12, 7);
        scene.Choice(1, 21, 22, 8);
        scene.DuringLog = action =>
        {
            if (action == AiDropSelectionLog.Hated)
            {
                scene.Field(0x3e0, 129);
                scene.Field(0x260, 0x102);
            }
        };
        scene.Select();
        Assert.Equal(2, scene.Queries.Count);
        // 반복문은 각 현재 승인 입력을 원본 계약과 비교한다.
        foreach (var query in scene.Queries)
        {
            Assert.Equal(129u, query.Type);
            Assert.Equal(2u, query.Owner);
            Assert.Equal(0u, query.Argument);
            Assert.Equal(0u, query.Flags);
            Assert.Equal(1u, query.Mode);
        }
    }

    /// <summary>블로커가 있으면 반복보다 우선하고 원본 순환 예산 안에서 찾은 TYPE만 선택한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void BlockerPriorityOverridesRepeat()
    {
        var scene = new DropSelectionScene();
        scene.Repeat();
        scene.Field(0x404, 1);
        scene.Count = 3;
        scene.Products = [128, 136, 142];
        scene.Types[142].Flags2 = 0x4000000;
        scene.Blocker = new Sid(6);
        scene.Select();
        Assert.Equal(142u, scene.Field(0x3e0));
        Assert.Equal(0u, scene.Field(0x404));
        Assert.Equal(0u, scene.Field(0x570));
        Assert.Equal(0u, scene.Field(0x3e8));
        Assert.Equal(["tech:1", "count:5", "hate", "blocker:5", "nth:5:1", "nth:5:2", "scan:142:0"], scene.Trace);
    }

    /// <summary>현재 TYPE가 블로커면 조회를 생략하고 재사용 경로의 404를 강제로 지우지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentBlockerRepeatKeepsPriorityFlag()
    {
        var scene = new DropSelectionScene();
        scene.Repeat(142);
        scene.Types[142].Flags2 = 0x4000000;
        scene.Field(0x404, 1);
        scene.Field(0x570, 1);
        scene.Choice(0, 11, 12, 7);
        scene.Select();
        Assert.Equal(1u, scene.Field(0x404));
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.DoesNotContain("blocker:5", scene.Trace);
    }

    /// <summary>블로커 부재는 404만 지우고 원본 반복을 허용한다. 기술 개수+2 회전 예산도 그대로다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void BlockerAbsentAndRotationBudget()
    {
        var absent = new DropSelectionScene();
        absent.Repeat();
        absent.Field(0x404, 1);
        absent.Select();
        Assert.Equal(0u, absent.Field(0x404));
        Assert.Contains("retry", absent.Trace);
        var bounded = new DropSelectionScene();
        bounded.Field(0x404, 1);
        bounded.Field(0x3e0, 131);
        bounded.Count = 1;
        bounded.Products = [128];
        bounded.Blocker = new Sid(6);
        bounded.Select();
        Assert.Equal(131u, bounded.Field(0x3e0));
        Assert.Equal(6u, bounded.Field(0x3e4));
        Assert.Equal(0u, bounded.Field(0x404));
        Assert.Equal(["tech:1", "count:5", "blocker:5"], bounded.Trace);
    }

    /// <summary>최고 점수는 count==0에서만 0으로 만들고 signed 음수 count는 점수를 보존한다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void ExactZeroScoreResetAndSignedFirstAttempt()
    {
        var scene = new DropSelectionScene();
        scene.Repeat();
        scene.Field(0x570, 0xffffffffu);
        scene.Select();
        Assert.Equal(0xffffffffu, scene.Field(0x570));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.Equal(1u, scene.Field(0x400));
        var empty = new DropSelectionScene();
        empty.Repeat();
        empty.Select();
        Assert.Equal(0u, empty.Field(0x570));
        Assert.Equal(0u, empty.Field(0x3e8));
        Assert.Equal(1u, empty.Field(0x400));
    }

    /// <summary>운반체는 선택을 지우고 공급 TYPE의 less/unordered 기한도 재선택 상태로 만든다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void CarrierAndOrderedSupplyCooldown()
    {
        var carrier = new DropSelectionScene();
        carrier.Products[1] = 133;
        carrier.Types[133].Flags2 = 0x10000;
        carrier.Select();
        Assert.Equal(0u, carrier.Field(0x3e0));
        Assert.Equal(1u, carrier.Field(0x3f0));
        // NaN도 미도달이며 equality는 도달이다. 저장 QWORD를 float로 줄이지 않는다.
        foreach (double due in new double[] { 100.0, 100.00000001, double.NaN })
        {
            var scene = new DropSelectionScene();
            scene.Products[1] = 136;
            scene.Types[136].Flags2 = 0x800;
            scene.Time(0x5b8, due);
            scene.Select();
            Assert.Equal(136u == scene.Field(0x3e0), due == 100.0);
            Assert.Equal(due == 100.0 ? 0u : 1u, scene.Field(0x3f0));
        }

        var nan = new DropSelectionScene();
        nan.Products[1] = 136;
        nan.Types[136].Flags2 = 0x800;
        nan.Now = double.NaN;
        nan.Select();
        Assert.Equal(0u, nan.Field(0x3e0));
        Assert.Equal(1u, nan.Field(0x3f0));
    }

    /// <summary>선택된 nth 누락은 이미 기록한 순번/진행을 보존하고 뒤 정리를 수행하지 않는다. 2026-11-25 R4-5b 배치 선택 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingNthPreservesPartialWrites()
    {
        var scene = new DropSelectionScene();
        scene.Field(0x3e0, 128);
        scene.Field(0x3e4, 4);
        scene.Hooks.Nth = null;
        Assert.ThrowsAny<Exception>(() => scene.Select());
        Assert.Equal(1u, scene.Field(0x3dc));
        Assert.Equal(128u, scene.Field(0x3e0));
        Assert.Equal(4u, scene.Field(0x3e4));
        Assert.Equal(123u, scene.Field(0x3e8));
        Assert.Equal(9u, scene.Field(0x3f0));
        Assert.Equal(10u, scene.Field(0xaf8));
    }
}
