// 원본 전체 가짜 부족 좌표의 현재 상태/선행 제거/순서·일부 효과를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 가짜 부족 좌표를 검사하는 클래스다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.
// 근거: cpppj tests/AiFakeDrySpotsTests.cpp의 14건 전부다.
public sealed class RawAiFakeDrySpotsTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>실제 본체/12바이트 목록과 기존 실제 Add/Remove/Seed를 같은 저장소에서 사용한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    private sealed class FakeDryScene
    {
        /// <summary>실제 본체와 두 목록 backing이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x54000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>기록 backing이다.</summary>
        public readonly uint Records;

        /// <summary>공급 backing이다.</summary>
        public readonly uint Suppliers;

        /// <summary>같은 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>같은 공용 난수다.</summary>
        public readonly GameRandom Random = new(8u);

        /// <summary>현재 공급 반경이다.</summary>
        public float Radius = 22;

        /// <summary>현재 지도 상한이다.</summary>
        public int Limit = 256;

        /// <summary>현재 디버그 종류다.</summary>
        public uint Debug = 99;

        /// <summary>명시 중심이다.</summary>
        public RawPathPoint Center = new(40, 60);

        /// <summary>명시 경로다.</summary>
        public RawPathPoint Candidate = new(100, 100);

        /// <summary>명시 점수 목록이다.</summary>
        public List<float> Scores = [100];

        /// <summary>명시 index 목록이다.</summary>
        public List<int> Indices = [0];

        /// <summary>선택된 실제 하위다.</summary>
        public AiFakeDryHooks Hooks = new();

        /// <summary>원문 호출 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<RawPathPoint> ScorePoints = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<RawPathPoint> Sources = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<RawPathPoint> Targets = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<Sid> Centers = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<AiFakeDryLog> Logs = [];

        /// <summary>원문 현재 인자다.</summary>
        public readonly List<(int X, int Y)> LogPoints = [];

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action<ulong>? DuringScore;

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action? DuringCenter;

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action? DuringCandidate;

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action? DuringAdd;

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action? DuringMark;

        /// <summary>실제 효과 뒤의 명시 상태 변경이다.</summary>
        public Action? DuringLog;

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>기본은 유효한 현재 가짜 좌표와 한 부족 기록이며 공급 시설/신전은 caller가 선택한다.</summary>
        public FakeDryScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Records = Memory.Allocate(120);
            Suppliers = Memory.Allocate(32);
            Field(0x280, 73);
            Field(0xadc, 10);
            Field(0xae0, 0);
            Field(0xae4, Records);
            Field(0xaec, 2);
            Field(0x2f4, Suppliers);
            Field(0x2f8, 8);
            Field(0x2fc, 0);
            Field(0xaf0, BitConverter.SingleToUInt32Bits(90.0f));
            Field(0xaf4, BitConverter.SingleToUInt32Bits(90.0f));
            Field(0x260, 2);
            Field(0xb00, 1);
            Assert.True(Dry().Add(Token, 150, 160, 71));
            Hooks.Score = (uint brain, RawPathPoint point, ref int index) =>
            {
                Assert.Equal(Token, brain);
                ulong call = (ulong)ScorePoints.Count;
                ScorePoints.Add(point);
                Trace.Add("score");
                index = Indices[(int)Math.Min(call, (ulong)(Indices.Count - 1))];
                float value = Scores[(int)Math.Min(call, (ulong)(Scores.Count - 1))];
                DuringScore?.Invoke(call);
                return value;
            };
            Hooks.Center = sid =>
            {
                Centers.Add(sid);
                Trace.Add("center");
                DuringCenter?.Invoke();
                return Center;
            };
            Hooks.Candidate = (source, target) =>
            {
                Sources.Add(source);
                Targets.Add(target);
                Trace.Add("candidate");
                DuringCandidate?.Invoke();
                return Candidate;
            };
            Hooks.Add = (brain, point, type) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("add");
                bool result = Dry().Add(brain, point.X, point.Y, type);
                DuringAdd?.Invoke();
                return result;
            };
            Hooks.Remove = (brain, index) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(0, index);
                Trace.Add("remove");
                Dry().Remove(brain, index);
            };
            Hooks.Seed = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("seed");
                Dry().Seed(brain, Random);
            };
            Hooks.Mark = point =>
            {
                Assert.Equal(Candidate.X, point.X);
                Assert.Equal(Candidate.Y, point.Y);
                Trace.Add("mark");
                DuringMark?.Invoke();
            };
            Hooks.Log = (brain, action, x, y) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add(action == AiFakeDryLog.Added ? "added" : "discarded");
                Logs.Add(action);
                LogPoints.Add((x, y));
                DuringLog?.Invoke();
            };
        }

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>지정 본체 DWORD를 쓴다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>기존 실제 Dry Spot 모듈을 같은 저장소에 연결한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <returns>검사할 부족 좌표 몸체다.</returns>
        public RawAiDrySpots Dry() => new(Memory, Pool, new AiDrySpotHooks());

        /// <summary>같은 실제 풀/본체와 명시 하위로 전체 루틴을 조립한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <returns>검사할 가짜 부족 좌표 몸체다.</returns>
        public RawAiFakeDrySpots Make() => new(Memory, Pool, new AiFakeDryGlobals
        {
            ManaRadius = () => Radius,
            MapLimit = () => Limit,
            Debug = () => Debug,
        }, Hooks);

        /// <summary>원래 공급 시설 목록에 사용할 실제 raw 객체를 만든다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <param name="x">쓸 X 좌표다.</param>
        /// <param name="y">쓸 Y 좌표다.</param>
        /// <returns>만든 SID다.</returns>
        public Sid Object(float x, float y)
        {
            ushort sid = _nextSid++;
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Fill(0);
            raw[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.FreeBit);
            raw[10] = 136;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
            return new Sid(sid);
        }

        /// <summary>현재 공급 시설의 SID DWORD 목록을 설정한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
        /// <param name="values">쓸 SID 목록이다.</param>
        public void Supply(Sid[] values)
        {
            Field(0x2fc, (uint)values.Length);
            int index = 0;
            // 지정한 SID 순서만 기록하는 반복문이다.
            foreach (Sid sid in values)
            {
                Put(Memory.Bytes(Suppliers), index * 4, sid.Value);
                index++;
            }
        }
    }

    /// <summary>fake TYPE 0이면 다른 관문/목록/실제 효과를 전혀 요청하지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void ZeroTypeSkipsFullAndAllEffects()
    {
        var input = new FakeDryScene();
        input.Field(0x280, 0);
        input.Field(0xadc, 0);
        input.Field(0xae4, 0);
        input.Hooks = new AiFakeDryHooks();
        input.Make().Extend(input.Token);
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Empty(input.Trace);
        Assert.Equal(8u, input.Random.State());
    }

    /// <summary>full 제거는 신전이 없는 반환보다 먼저이며 cursor/가짜 좌표를 보정하지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void FullRemovalPrecedesMissingTemple()
    {
        var input = new FakeDryScene();
        input.Field(0xadc, 1);
        input.Field(0xae8, 7);
        input.Make().Extend(input.Token);
        Assert.Equal(["remove"], input.Trace);
        Assert.Equal(0u, input.Field(0xae0));
        Assert.Equal(7u, input.Field(0xae8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(90.0f), input.Field(0xaf0));
    }

    /// <summary>공급 시설 0은 중심 대신 신전 raw 좌표를 사용하며 중복 실패에서도 원래 Added 로그를 남긴다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void TempleUsesRawPointAndIgnoresDuplicate()
    {
        var input = new FakeDryScene();
        Sid temple = input.Object(150, 160);
        input.Field(0x294, temple.Value);
        input.Make().Extend(input.Token);
        Assert.Equal(["add", "added"], input.Trace);
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Empty(input.Centers);
        Assert.Empty(input.Sources);
        Assert.Equal(BitConverter.SingleToUInt32Bits(90.0f), input.Field(0xaf0));
        Assert.Equal([AiFakeDryLog.Added], input.Logs);
        Assert.Equal([(150, 160)], input.LogPoints);
    }

    /// <summary>공급 목록 포인터/count는 진입 cache이며 각 객체의 raw 좌표와 첫 최고 점수 규칙은 유지한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void CachedSupplyListAndFirstMaximum()
    {
        var input = new FakeDryScene();
        Sid first = input.Object(20, 30);
        Sid second = input.Object(50, 60);
        input.Supply([first, second]);
        input.Scores = [10, 10];
        uint other = input.Memory.Allocate(4);
        Put(input.Memory.Bytes(other), 0, 65535);
        input.DuringScore = call =>
        {
            if (call == 0)
            {
                input.Field(0x2f4, other);
                input.Field(0x2fc, 0);
                Put(input.Pool.RawSlot(second.Value), 14, BitConverter.SingleToUInt32Bits(55.0f));
            }
        };
        input.Make().Extend(input.Token);
        Assert.Equal(2, input.ScorePoints.Count);
        Assert.Equal(55f, input.ScorePoints[1].X);
        Assert.Equal([first], input.Centers);
        Assert.Equal(60f, input.Sources[0].X);
        Assert.Equal(60f, input.Sources[0].Y);
        Assert.Equal(149f, input.Targets[0].X);
        Assert.Equal(159f, input.Targets[0].Y);
    }

    /// <summary>중심 효과 뒤 현재 Dry Spot pointer를 사용하되 점수에서 선택한 index는 보존한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentDryPointerAfterCenter()
    {
        var input = new FakeDryScene();
        Assert.True(input.Dry().Add(input.Token, 180, 190, 71));
        input.Supply([input.Object(179, 190)]);
        input.Scores = [999];
        input.Indices = [1];
        uint replacement = input.Memory.Allocate(24);
        Put(input.Memory.Bytes(replacement), 12, BitConverter.SingleToUInt32Bits(181.0f));
        Put(input.Memory.Bytes(replacement), 16, BitConverter.SingleToUInt32Bits(191.0f));
        input.DuringCenter = () =>
        {
            input.Field(0xae4, replacement);
            input.Field(0xae0, 1);
            input.Field(0xadc, 2);
        };
        input.Make().Extend(input.Token);
        Assert.Equal(180f, input.Targets[0].X);
        Assert.Equal(190f, input.Targets[0].Y);
        Assert.Equal(2u, input.Field(0xae0));
    }

    /// <summary>현재 공급 반경의 0.7배보다 커야 채택하며 NaN 반경도 폐기한다. 거리 중심은 X/Y 원래 쌍이다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void RadiusOrderedGateUsesOriginalCenter()
    {
        // 원본 float 0.7을 넓힌 임계값과 거리의 양쪽을 서로 다른 실제 반경으로 지정하는 반복문이다.
        foreach (float radius in new float[] { 20.0f, 30.0f, float.NaN })
        {
            var input = new FakeDryScene();
            input.Supply([input.Object(20, 30)]);
            input.Center = new RawPathPoint(40, 60);
            input.Candidate = new RawPathPoint(60, 60);
            input.Radius = radius;
            input.Make().Extend(input.Token);
            bool accepted = radius == 20.0f;
            Assert.Equal(accepted ? 2u : 1u, input.Field(0xae0));
            Assert.Equal(accepted ? AiFakeDryLog.Added : AiFakeDryLog.Discarded, input.Logs[^1]);
        }
    }

    /// <summary>유효성 경계/NaN 후보에서는 거리/등록/폐기 로그 없이 현재 가짜 좌표만 확인한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void InvalidCandidateHasNoDiscardLog()
    {
        // 축 0/상한/NaN은 원본 strict 좌표 검사의 서로 다른 거부 입력이다.
        foreach (float coordinate in new float[] { 0.0f, 256.0f, float.NaN })
        {
            var input = new FakeDryScene();
            input.Supply([input.Object(20, 30)]);
            input.Candidate = new RawPathPoint(coordinate, 100);
            input.Make().Extend(input.Token);
            Assert.Equal(1u, input.Field(0xae0));
            Assert.Empty(input.Logs);
            Assert.Equal(["score", "center", "candidate"], input.Trace);
        }
    }

    /// <summary>signed 음수 공급 count는 탐색을 건너뛰고 유효하지 않은 현재 가짜 좌표만 실제 Seed로 보충한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void NegativeCountFallsBackWithoutLookup()
    {
        var input = new FakeDryScene();
        input.Field(0x2fc, 0xffffffffu);
        input.Field(0x2f4, 0);
        input.Field(0xaf0, 0);
        input.Field(0x248, 0);
        input.Make().Extend(input.Token);
        Assert.Equal(["seed"], input.Trace);
        Assert.Equal(8u, input.Random.State());
        Assert.Empty(input.Logs);
    }

    /// <summary>실제 Add가 false여도 선택 좌표/전략 4를 갱신하고 현재 디버그 표식/로그를 원래 순서로 실행한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void AddFailureStillUpdatesStrategyAndMarks()
    {
        var scene = new FakeDryScene();
        scene.Supply([scene.Object(20, 30)]);
        Assert.True(scene.Dry().Add(scene.Token, 100, 100, 99));
        scene.Field(0x2d4, 4);
        scene.Field(0x3cc, 7);
        scene.Field(0x3d0, 9);
        scene.Field(0x3d4, 11);
        scene.DuringAdd = () =>
        {
            scene.Field(0x280, 77);
            scene.Debug = 29;
        };
        scene.Make().Extend(scene.Token);
        Assert.Equal(2u, scene.Field(0xae0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(100.0f), scene.Field(0xaf0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(100.0f), scene.Field(0xaf4));
        Assert.Equal(0u, scene.Field(0x3cc));
        Assert.Equal(0xc61c4000u, scene.Field(0x3d0));
        Assert.Equal(0xc61c4000u, scene.Field(0x3d4));
        Assert.Equal(1u, scene.Field(0x3d8));
        Assert.Equal(["score", "center", "candidate", "add", "mark", "added"], scene.Trace);
        Assert.Equal(99u, Get(scene.Memory.Bytes(scene.Records), 20));
    }

    /// <summary>채택 뒤 표식이 현재 가짜 좌표를 무효화하면 실제 Seed가 뒤따르지만 최종 로그는 원래 후보다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void SeedAfterMarkKeepsOriginalLogPoint()
    {
        var scene = new FakeDryScene();
        scene.Supply([scene.Object(20, 30)]);
        scene.Debug = 29;
        scene.Field(0x248, 0x20);
        scene.Field(0x270, BitConverter.SingleToUInt32Bits(10.0f));
        scene.Field(0x274, BitConverter.SingleToUInt32Bits(20.0f));
        scene.Field(0x278, BitConverter.SingleToUInt32Bits(30.0f));
        scene.Field(0x27c, BitConverter.SingleToUInt32Bits(60.0f));
        scene.DuringMark = () => scene.Field(0xaf0, 0);
        scene.Make().Extend(scene.Token);
        Assert.Equal(["score", "center", "candidate", "add", "mark", "seed", "added"], scene.Trace);
        Assert.Equal([(100, 100)], scene.LogPoints);
        Assert.Equal(BitConverter.SingleToUInt32Bits(10.0f), scene.Field(0xaf0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.0f), scene.Field(0xaf4));
        Assert.Equal(3u, scene.Field(0xae0));
        Assert.NotEqual(8u, scene.Random.State());
    }

    /// <summary>후보 계산 뒤 현재 TYPE/전략과 b00을 읽고 출력이 꺼진 경우 실제 로그를 요구하지 않는다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentTypeAndDebugAfterCandidate()
    {
        var scene = new FakeDryScene();
        scene.Supply([scene.Object(20, 30)]);
        scene.DuringCandidate = () =>
        {
            scene.Field(0x280, 0xdeadbeefu);
            scene.Field(0xb00, 0);
            scene.Field(0x2d4, 3);
        };
        scene.Hooks.Log = null;
        scene.Make().Extend(scene.Token);
        Assert.Equal(2u, scene.Field(0xae0));
        Assert.Equal(0xdeadbeefu, Get(scene.Memory.Bytes(scene.Records), 20));
        Assert.Equal(0u, scene.Field(0x3d8));
        Assert.Empty(scene.Logs);
    }

    /// <summary>선택된 실제 경로/표식 누락에서도 이미 완료한 full 제거·좌표/전략 쓰기는 유지한다. 2026-11-25 R4-5b 가짜 부족 좌표 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingPreservesPartialWrites()
    {
        var scene = new FakeDryScene();
        Assert.True(scene.Dry().Add(scene.Token, 180, 190, 71));
        scene.Field(0xadc, 2);
        scene.Supply([scene.Object(20, 30)]);
        scene.Hooks.Candidate = null;
        Assert.ThrowsAny<Exception>(() => scene.Make().Extend(scene.Token));
        Assert.Equal(1u, scene.Field(0xae0));
        var marked = new FakeDryScene();
        marked.Supply([marked.Object(20, 30)]);
        marked.Debug = 29;
        marked.Field(0x2d4, 4);
        marked.Hooks.Mark = null;
        Assert.ThrowsAny<Exception>(() => marked.Make().Extend(marked.Token));
        Assert.Equal(2u, marked.Field(0xae0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(100.0f), marked.Field(0xaf0));
        Assert.Equal(1u, marked.Field(0x3d8));
        Assert.Empty(marked.Logs);
    }
}
