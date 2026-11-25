// 부족 좌표 점수/시설별 분배/후보 승인과 실제 목록 효과를 검사한다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 평가를 검사하는 클래스다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.
// 근거: cpppj tests/AiDropEvaluationTests.cpp의 17건 전부다.
public sealed class RawAiDropEvaluationTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 본체/실제 부족 좌표 backing/실제 표적 평가를 연결한 명시 입력 장면이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    private sealed class EvaluationScene
    {
        /// <summary>본체/12 BYTE 기록의 실제 핸들이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x51000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>기록 backing이다.</summary>
        public readonly uint Records;

        /// <summary>실제 물리 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 시설/표적 TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>현재 제단 TYPE다.</summary>
        public uint Altar = 164;

        /// <summary>현재 방향 TYPE다.</summary>
        public uint Directional = 142;

        /// <summary>현재 조회 TYPE다.</summary>
        public uint Probe = 71;

        /// <summary>현재 공급 반경이다.</summary>
        public float Radius = 22;

        /// <summary>명시 표적 비율이다.</summary>
        public float Rate;

        /// <summary>현재 검색 결과다.</summary>
        public Sid Found = new(5);

        /// <summary>실제 배치 허용 여부다.</summary>
        public bool Placement = true;

        /// <summary>실제 에너지 승인 값이다.</summary>
        public uint Energy = 1;

        /// <summary>실제 공급 문자열이다.</summary>
        public string Supply = "s";

        /// <summary>선택된 평가 하위다.</summary>
        public AiDropEvaluationHooks Hooks = new();

        /// <summary>같은 실제 표적 계산기다.</summary>
        public RawAiDropTargets Targets;

        /// <summary>호출 기록이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>각 하위에서 현재 본체/출력 참조가 달라지는 명시 효과다.</summary>
        public Action? DuringPlace;

        /// <summary>각 하위에서 현재 본체/출력 참조가 달라지는 명시 효과다.</summary>
        public Action? DuringRate;

        /// <summary>각 하위에서 현재 본체/출력 참조가 달라지는 명시 효과다.</summary>
        public Action? DuringCollect;

        /// <summary>각 하위에서 현재 본체/출력 참조가 달라지는 명시 효과다.</summary>
        public Action? DuringEnergy;

        /// <summary>값으로 전달한 원본 배치 인자의 관찰값이다.</summary>
        public CanonPlacementQuery LastPlacement;

        /// <summary>값으로 전달한 원본 수집 인자의 관찰값이다.</summary>
        public uint LastCollectOwner;

        /// <summary>값으로 전달한 원본 수집 인자의 관찰값이다.</summary>
        public uint LastCollectType;

        /// <summary>값으로 전달한 원본 에너지 인자의 관찰값이다.</summary>
        public uint LastEnergyOwner;

        /// <summary>기본은 같은 적 소유자 2의 10점 표적, TYPE 90 Gun과 near dry spot 한 개다.</summary>
        public EvaluationScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Records = Memory.Allocate(120);
            // 실제 슬롯을 확보하고 입력 바이트만 준비하는 반복문이다.
            for (ushort id = 5; id <= 9; id++)
            {
                Span<byte> raw = Pool.RawSlot(id);
                raw.Fill(0);
                raw[10] = 80;
                raw[34] = 2;
                Put(raw, 26, 1000);
            }

            Field(0x260, 1);
            Field(0x264, 2);
            Field(0xad8, 2);
            Field(0xadc, 10);
            Field(0xae4, Records);
            Field(0xaec, 1);
            Types[90].Group = 0;
            Types[91].Group = 3;
            Types[92].Flags2 = 0x4000000;
            Types[93].Flags2 = 0x800;
            Types[93].Group = 3;
            Types[94].Group = 6;
            Types[95].Group = 7;
            var target = new AiDropTargetHooks
            {
                Captive = _ => new Sid(0),
                MaxHitPoints = _ => 1000,
                Center = _ => new RawPathPoint(20, 29),
                Assertion = (_, _) => { },
            };
            target.Rate = actor =>
            {
                Assert.Equal(new Sid(5), actor);
                Trace.Add("rate");
                DuringRate?.Invoke();
                return Rate;
            };
            target.Gun = (x, y, type, direction, owner) =>
            {
                Assert.Equal(2, direction);
                Trace.Add("gun:" + owner);
                return Found;
            };
            target.Ground = (x, y, type, owner) =>
            {
                Trace.Add("ground:" + owner);
                return Found;
            };
            target.Blockers = (owner, point, capacity) =>
            {
                Assert.Equal(30, capacity);
                Trace.Add("blockers");
                return [5u];
            };
            Targets = new RawAiDropTargets(Memory, Pool, Types, new AiDropTargetGlobals
            {
                AltarType = () => Altar,
                DirectionalType = () => Directional,
            }, target);
            Hooks.Collect = (text, x, y, type, owner, theme) =>
            {
                Assert.Equal(0, text.Length);
                Assert.Equal(-1, theme);
                Trace.Add("collect");
                LastCollectOwner = owner;
                LastCollectType = type;
                DuringCollect?.Invoke();
                text.Append(Supply);
            };
            Hooks.MayPlace = query =>
            {
                Trace.Add("place");
                LastPlacement = query;
                DuringPlace?.Invoke();
                return Placement;
            };
            Hooks.Energy = (text, x, y, type, owner) =>
            {
                Assert.Equal(0, text.Length);
                Trace.Add("energy");
                LastEnergyOwner = owner;
                DuringEnergy?.Invoke();
                return Energy;
            };
            Hooks.Dry = (brain, x, y, type) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("dry");
                return new RawAiDrySpots(Memory, Pool, new AiDrySpotHooks()).Add(brain, x, y, type);
            };
            Hooks.Unsupported = (brain, type) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("unknown:" + type);
            };
            Hooks.DryLog = (brain, x, y) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("drylog:" + x + ":" + y);
            };
            Dry([new RawPathPoint(21.25f, 30.75f)]);
        }

        /// <summary>현재 본체 DWORD만 입력한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>지정 본체 DWORD를 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>기록 TYPE와 무관한 명시 XY 입력을 현재 backing에 쓴다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
        /// <param name="values">쓸 좌표 목록이다.</param>
        public void Dry(RawPathPoint[] values)
        {
            Field(0xae0, (uint)values.Length);
            int at = 0;
            // 원본 기록 순서를 유지하는 반복문이다.
            foreach (var point in values)
            {
                Put(Memory.Bytes(Records), at, BitConverter.SingleToUInt32Bits(point.X));
                Put(Memory.Bytes(Records), at + 4, BitConverter.SingleToUInt32Bits(point.Y));
                Put(Memory.Bytes(Records), at + 8, 0xdeadbeefu);
                at += 12;
            }
        }

        /// <summary>같은 현재 실제 표적 계산기를 평가/승인 하위와 연결한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
        /// <returns>검사할 배치 평가 몸체다.</returns>
        public RawAiDropEvaluation Actions() => new(Memory, Pool, Types, Targets, new AiDropEvaluationGlobals
        {
            ManaRadius = () => Radius,
            ProbeType = () => Probe,
        }, Hooks);
    }

    /// <summary>빈/음수 count는 포인터를 읽지 않고 출력 index를 보존한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void DryEmptySignedCountPreservesIndex()
    {
        var scene = new EvaluationScene();
        scene.Field(0xae0, 0);
        scene.Field(0xae4, 0);
        int index = 99;
        Assert.Equal(0, scene.Actions().DryDistance(scene.Token, new RawPathPoint(20.25f, 30.75f), ref index));
        Assert.Equal(99, index);
        scene.Field(0xae0, 0xffffffffu);
        Assert.Equal(0, scene.Actions().DryDistance(scene.Token, new RawPathPoint(20.25f, 30.75f), ref index));
        Assert.Equal(99, index);
    }

    /// <summary>정확한 0거리는 제외하고 NaN/거리 1000은 건너뛰며 같은 최고는 첫 index다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void DryNonzeroManhattanFirstTieAndNan()
    {
        var scene = new EvaluationScene();
        scene.Dry([new RawPathPoint(20.25f, 30.75f), new RawPathPoint(21.25f, 30.75f), new RawPathPoint(19.25f, 30.75f), new RawPathPoint(1020.25f, 30.75f), new RawPathPoint(float.NaN, 30.75f)]);
        int index = 99;
        Assert.Equal(999, scene.Actions().DryDistance(scene.Token, new RawPathPoint(20.25f, 30.75f), ref index));
        Assert.Equal(1, index);
        Assert.Empty(scene.Trace);
    }

    /// <summary>최고값 float 저장 때문에 더 가까운 둘째 기록도 축소 전 값이 저장 최고보다 작으면 유지한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void DryComparesExtendedThenStoresFloat()
    {
        var scene = new EvaluationScene();
        scene.Dry([new RawPathPoint(0.000002f, 0), new RawPathPoint(0.000001f, 0)]);
        int index = 99;
        Assert.Equal(1000, scene.Actions().DryDistance(scene.Token, new RawPathPoint(0, 0), ref index));
        Assert.Equal(0, index);
    }

    /// <summary>반경 22의 거리 44 경계는 제외하며 점수는 float 저장 후 절삭/길이+1로 나눈다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void SupplyStrictRadiusAndStoredScore()
    {
        var scene = new EvaluationScene();
        scene.Dry([new RawPathPoint(64.25f, 30.75f)]);
        Assert.Equal(0, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        Assert.Empty(scene.Trace);
        scene.Dry([new RawPathPoint(63.75f, 30.75f)]);
        Assert.Equal(478, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        Assert.Equal(["collect"], scene.Trace);
        scene.Trace.Clear();
        scene.Radius = float.NaN;
        Assert.Equal(0, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        Assert.Empty(scene.Trace);
    }

    /// <summary>공급 문자열은 첫 NUL까지 세며 빈 문자열과 signed 길이 한도를 원본대로 처리한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void SupplyCstrlenAndCurrentSignedLimit()
    {
        var scene = new EvaluationScene();
        scene.Supply = "s\0sss";
        scene.Probe = 75;
        scene.Field(0x260, 0x101);
        scene.Field(0xad8, 0);
        scene.DuringCollect = () => { scene.Field(0xad8, 2); scene.Field(0x260, 2); scene.Probe = 76; };
        Assert.Equal(499, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        Assert.Equal(0x101u, scene.LastCollectOwner);
        Assert.Equal(75u, scene.LastCollectType);
        scene.DuringCollect = null;
        scene.Field(0xad8, 1);
        Assert.Equal(0, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        scene.Field(0xad8, 0xffffffffu);
        Assert.Equal(0, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
        scene.Supply = "";
        scene.Field(0xad8, 10);
        Assert.Equal(0, scene.Actions().SupplyScore(scene.Token, new RawPathPoint(20.25f, 30.75f)));
    }

    /// <summary>flags 800은 블로커/group 3보다 우선하며 공급 평가는 표적 출력을 바꾸지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void DispatchFlagsPrecedeGroupsAndPreserveOutput()
    {
        var scene = new EvaluationScene();
        scene.Types[93].Flags2 |= 0x4000000;
        Sid target = new(9);
        Assert.Equal(499, scene.Actions().Evaluate(scene.Token, 93, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(9), target);
        Assert.Equal(["collect"], scene.Trace);
        scene.Types[93].Flags2 = 0x4000000;
        scene.Trace.Clear();
        Assert.Equal(10, scene.Actions().Evaluate(scene.Token, 93, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(5), target);
        Assert.Equal(["blockers", "rate"], scene.Trace);
    }

    /// <summary>group 3은 별도 지상 검색이고 0/1은 실제 Gun 경로이며 6은 진단 없이 0이다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void DispatchGroundGunAndSilentGroupSix()
    {
        var scene = new EvaluationScene();
        Sid target = new(9);
        Assert.Equal(10, scene.Actions().Evaluate(scene.Token, 91, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(["ground:1", "rate"], scene.Trace);
        scene.Trace.Clear();
        Assert.Equal(10, scene.Actions().Evaluate(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(["gun:1", "rate"], scene.Trace);
        scene.Types[90].Group = 1;
        scene.Trace.Clear();
        Assert.Equal(10, scene.Actions().Evaluate(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(["gun:1", "rate"], scene.Trace);
        scene.Trace.Clear();
        scene.Field(0xb00, 1);
        target = new Sid(9);
        Assert.Equal(0, scene.Actions().Evaluate(scene.Token, 94, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(9), target);
        Assert.Empty(scene.Trace);
    }

    /// <summary>미지원 group은 출력 보존/현재 b00 진단만 수행하며 잘못된 TYPE는 명시 거부한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void UnknownGroupDiagnosticAndTypeBounds()
    {
        var scene = new EvaluationScene();
        Sid target = new(9);
        Assert.Equal(0, scene.Actions().Evaluate(scene.Token, 95, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(9), target);
        Assert.Empty(scene.Trace);
        scene.Field(0xb00, 1);
        Assert.Equal(0, scene.Actions().Evaluate(scene.Token, 95, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(["unknown:95"], scene.Trace);
        Assert.ThrowsAny<Exception>(() => scene.Actions().Evaluate(scene.Token, 188, new RawPathPoint(20.25f, 30.75f), 2, ref target));
    }

    /// <summary>별도 검색은 0도 출력하고 점수 하위 실패보다 먼저 실제 SID를 쓴다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void GroundResultWrittenBeforeScoreFailure()
    {
        var scene = new EvaluationScene();
        Sid target = new(9);
        scene.Found = new Sid(0);
        Assert.Equal(0, scene.Actions().Evaluate(scene.Token, 91, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(0), target);
        scene.Found = new Sid(5);
        scene.Targets = new RawAiDropTargets(scene.Memory, scene.Pool, scene.Types, new AiDropTargetGlobals
        {
            AltarType = () => scene.Altar,
            DirectionalType = () => scene.Directional,
        }, new AiDropTargetHooks
        {
            Rate = actor =>
            {
                Assert.Equal(new Sid(5), target);
                throw new InvalidOperationException("명시 점수 getter 실패");
            },
            Ground = (x, y, type, owner) => scene.Found,
        });
        Assert.ThrowsAny<Exception>(() => scene.Actions().Evaluate(scene.Token, 91, new RawPathPoint(20.25f, 30.75f), 2, ref target));
        Assert.Equal(new Sid(5), target);
    }

    /// <summary>전체 배치 검사는 방향 0/소유자 BYTE/mode 1이며 거부 시 이후 평가·출력도 없다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void PlacementGateCanonicalArguments()
    {
        var scene = new EvaluationScene();
        scene.Field(0x260, 0x101);
        scene.Placement = false;
        var best = new RawPathPoint(7, 8);
        int score = 3;
        Sid target = new(9);
        Assert.False(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        var query = scene.LastPlacement;
        Assert.Equal(90u, query.Type);
        Assert.Equal(0u, query.Argument);
        Assert.Equal(0u, query.Flags);
        Assert.Equal(1u, query.Owner);
        Assert.Equal(1u, query.Mode);
        Assert.Equal(20.25f, query.X);
        Assert.Equal(30.75f, query.Y);
        Assert.Equal(7f, best.X);
        Assert.Equal(8f, best.Y);
        Assert.Equal(3, score);
        Assert.Equal(new Sid(9), target);
        Assert.Equal(["place"], scene.Trace);
    }

    /// <summary>평가/에너지는 앞선 하위 효과 뒤 현재 full owner를 다시 읽는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentOwnerBetweenPlacementScoreAndEnergy()
    {
        var scene = new EvaluationScene();
        scene.Field(0x260, 0x101);
        scene.DuringPlace = () => scene.Field(0x260, 0x102);
        scene.DuringRate = () => scene.Field(0x260, 0x103);
        var best = new RawPathPoint(7, 8);
        int score = 0;
        Sid target = new(9);
        scene.Energy = 0x80000000u;
        Assert.True(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(1u, scene.LastPlacement.Owner);
        Assert.Equal(0x103u, scene.LastEnergyOwner);
        Assert.Equal(20.25f, best.X);
        Assert.Equal(30.75f, best.Y);
        Assert.Equal(10, score);
        Assert.Equal(new Sid(5), target);
        Assert.Equal(["place", "gun:258", "rate", "energy"], scene.Trace);
    }

    /// <summary>양수 동률은 승인하고 0/음수 동률은 승인하지 않는다. 음수도 개선이면 승인할 수 있다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void PositiveTieAndSignedImprovement()
    {
        var scene = new EvaluationScene();
        var best = new RawPathPoint(7, 8);
        int score = 10;
        Sid target = new(9);
        Assert.True(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(10, score);
        Assert.Equal(20.25f, best.X);
        scene.Trace.Clear();
        score = 0;
        best = new RawPathPoint(7, 8);
        Assert.False(scene.Actions().Consider(scene.Token, 94, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(["place"], scene.Trace);
        scene.Rate = -11;
        scene.Trace.Clear();
        score = -1;
        Assert.False(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(["place", "gun:1", "rate"], scene.Trace);
        score = -2;
        Assert.True(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(-1, score);
    }

    /// <summary>후보를 채택하지 않아도 실제 표적 SID의 평가 효과를 보존하고 최고 좌표/점수는 유지한다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void WorseScoreKeepsTargetSideEffect()
    {
        var scene = new EvaluationScene();
        var best = new RawPathPoint(7, 8);
        int score = 20;
        Sid target = new(9);
        Assert.False(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(new Sid(5), target);
        Assert.Equal(7f, best.X);
        Assert.Equal(8f, best.Y);
        Assert.Equal(20, score);
        Assert.Equal(["place", "gun:1", "rate"], scene.Trace);
    }

    /// <summary>실제 부족 좌표 추가는 에너지 거부 뒤이며 중복 실패는 로그도 반복하지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void EnergyFailureUsesActualDryBackingAndCurrentLog()
    {
        var scene = new EvaluationScene();
        scene.Field(0xae0, 0);
        scene.Energy = 0;
        scene.DuringEnergy = () => scene.Field(0xb00, 1);
        var best = new RawPathPoint(7, 8);
        int score = 0;
        Sid target = new(9);
        Assert.False(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(1u, scene.Field(0xae0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.25f), Get(scene.Memory.Bytes(scene.Records), 0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(30.75f), Get(scene.Memory.Bytes(scene.Records), 4));
        Assert.Equal(90u, Get(scene.Memory.Bytes(scene.Records), 8));
        Assert.Equal(7f, best.X);
        Assert.Equal(0, score);
        Assert.Equal(new Sid(5), target);
        Assert.Equal(["place", "gun:1", "rate", "energy", "dry", "drylog:20:30"], scene.Trace);
        scene.Trace.Clear();
        Assert.False(scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(1u, scene.Field(0xae0));
        Assert.Equal(["place", "gun:1", "rate", "energy", "dry"], scene.Trace);
    }

    /// <summary>선택된 하위 미연결은 평가가 이미 쓴 표적을 보존하고 최고 좌표/점수는 바꾸지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingHooksPreservePrefix()
    {
        var scene = new EvaluationScene();
        scene.Hooks.Energy = null;
        var best = new RawPathPoint(7, 8);
        int score = 0;
        Sid target = new(9);
        Assert.ThrowsAny<Exception>(() => scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(new Sid(5), target);
        Assert.Equal(7f, best.X);
        Assert.Equal(0, score);
        scene.Energy = 0;
        scene.Hooks.Energy = (text, x, y, type, owner) => 0u;
        scene.Hooks.Dry = null;
        Assert.ThrowsAny<Exception>(() => scene.Actions().Consider(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref score, ref target));
        Assert.Equal(0u, scene.Field(0xb00));
    }

    /// <summary>스캔의 실제 3e8은 평가 뒤 읽고 승인 뒤 그 자리에서 쓰며 임시 사본을 되돌리지 않는다. 2026-11-25 R4-5b 배치 평가 이식에서 추가됐다.</summary>
    [Fact]
    public void StoredScoreReadsCurrentBodyAtNativeSites()
    {
        var scene = new EvaluationScene();
        scene.Field(0x3e8, 0);
        scene.DuringRate = () => scene.Field(0x3e8, 100);
        var best = new RawPathPoint(7, 8);
        Sid target = new(9);
        Assert.False(scene.Actions().ConsiderStored(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref target));
        Assert.Equal(100u, scene.Field(0x3e8));
        Assert.Equal(new Sid(5), target);
        Assert.Equal(7f, best.X);
        scene.DuringRate = () => scene.Field(0x3e8, 5);
        scene.DuringEnergy = () => Assert.Equal(5u, scene.Field(0x3e8));
        Assert.True(scene.Actions().ConsiderStored(scene.Token, 90, new RawPathPoint(20.25f, 30.75f), 2, ref best, ref target));
        Assert.Equal(10u, scene.Field(0x3e8));
        Assert.Equal(20.25f, best.X);
        Assert.Equal(30.75f, best.Y);
    }
}
