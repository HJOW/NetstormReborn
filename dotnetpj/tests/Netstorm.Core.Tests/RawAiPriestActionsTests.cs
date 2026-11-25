// 사제 행동의 원본 관문·현재 상태 재조회·요청 순서를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 사제 행동을 검사하는 클래스다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.
// 근거: cpppj tests/AiPriestActionsTests.cpp의 20건 전부다.
public sealed class RawAiPriestActionsTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>실제 본체와 슬롯에 명시 입력을 넣고 선택된 하위 호출만 관찰한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    private sealed class AiPriestScene
    {
        /// <summary>같은 본체/목록의 실제 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4f000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>본체와 명시 건물 목록의 핸들이다.</summary>
        public readonly uint List;

        /// <summary>원본 PATCH 물리 슬롯을 사용한다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Priest = new(5);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Self = new(6);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Dais = new(7);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Home = new(8);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Building = new(9);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Other = new(10);

        /// <summary>명시 입력의 역할별 SID다.</summary>
        public readonly Sid Altar = new(11);

        /// <summary>현재 전역의 참조 입력이다.</summary>
        public double Now = 100;

        /// <summary>현재 전역의 참조 입력이다.</summary>
        public int MapLimit = 256;

        /// <summary>현재 전역의 참조 입력이다.</summary>
        public uint DaisType = 165;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public bool Vulnerable;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public bool Busy;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public bool Ritual;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public bool AltarEnemy;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public bool LowFunds;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public Sid Path;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public int Maximum = 1000;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public double Distance = 2;

        /// <summary>선택된 하위의 명시 반환값이다.</summary>
        public uint Selected = 86;

        /// <summary>선택된 실제 하위다.</summary>
        public AiPriestActionHooks Hooks = new();

        /// <summary>호출 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>값으로 전달한 초기화 입력을 보존한다.</summary>
        public readonly List<AiPriestSearchRequest> Requests = [];

        /// <summary>기본은 건강한 사제/신전/충분한 지갑/미정 목표/완료된 빈 탐색이다.</summary>
        public AiPriestScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            List = Memory.Allocate(32);
            // 실제 할당은 SID 5부터 순서대로 배정하며 각 입력의 주변 바이트를 초기화하는 반복문이다.
            for (ushort id = 5; id <= 11; id++)
            {
                Span<byte> raw = Pool.RawSlot(id);
                raw.Fill(0);
                raw[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.FreeBit);
                raw[10] = 158;
                Put(raw, 26, 1000);
            }

            Position(Priest, new RawPathPoint(20.2f, 30.7f));
            Position(Dais, new RawPathPoint(40, 50));
            Field(0x29c, Priest.Value);
            Field(0x294, Home.Value);
            Field(0x264, 2);
            Field(0x260, 1);
            Field(0x2d8, 1000);
            Field(0x300, List);
            Target(new RawPathPoint(-10000, -10000));
            Field(0x270, BitConverter.SingleToUInt32Bits(10.0f));
            Field(0x274, BitConverter.SingleToUInt32Bits(20.0f));
            Field(0x278, BitConverter.SingleToUInt32Bits(50.0f));
            Field(0x27c, BitConverter.SingleToUInt32Bits(70.0f));
            Field(0x2b0, BitConverter.SingleToUInt32Bits(25.0f));
            Field(0x2b4, BitConverter.SingleToUInt32Bits(26.0f));
            Hooks.Scan = (brain, budget) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("scan:" + budget);
                Field(0x5a4, Field(0x5a0));
            };
            Hooks.Vulnerable = actor =>
            {
                Assert.Equal(Priest, actor);
                Trace.Add("vulnerable");
                return Vulnerable;
            };
            Hooks.Busy = actor =>
            {
                Assert.Equal(Priest, actor);
                Trace.Add("busy");
                return Busy;
            };
            Hooks.Path = actor =>
            {
                Assert.Equal(Priest, actor);
                Trace.Add("path");
                return Path;
            };
            Hooks.MaxHitPoints = actor =>
            {
                Assert.Equal(Priest, actor);
                Trace.Add("max");
                return Maximum;
            };
            Hooks.Ritual = actor =>
            {
                Trace.Add("ritual:" + actor.Value);
                return Ritual;
            };
            Hooks.AltarEnemy = (enemy, target) =>
            {
                Trace.Add("altar:" + enemy + ":" + target.Value);
                return AltarEnemy;
            };
            Hooks.Target = (brain, actor, target, sacrifice) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("target:" + actor.Value + ":" + target.Value + ":" + sacrifice);
            };
            Hooks.Distance = (point, target) =>
            {
                Assert.Equal(20.2f, point.X);
                Assert.Equal(30.7f, point.Y);
                Trace.Add("distance:" + target.Value);
                return Distance;
            };
            Hooks.Select = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("select");
                return Selected;
            };
            Hooks.Home = (brain, actor) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Priest, actor);
                Trace.Add("home");
            };
            Hooks.Collect = (brain, actor) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Priest, actor);
                Trace.Add("collect");
            };
            Hooks.Deploy = (brain, type, point) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Selected, type);
                Assert.Equal(20.2f, point.X);
                Assert.Equal(30.7f, point.Y);
                Assert.Equal(Now + 20, Time(0x5c0));
                Trace.Add("deploy");
            };
            Hooks.Begin = (brain, request) =>
            {
                Assert.Equal(Token, brain);
                Requests.Add(request);
                Trace.Add("begin");
                Field(0x574, request.Type);
                Field(0x5a4, 2);
                Field(0x5a0, 4);
            };
            Hooks.Point = (brain, actor, point) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Priest, actor);
                Assert.Equal(21f, point.X);
                Assert.Equal(31f, point.Y);
                Trace.Add("point");
            };
            Hooks.EnemyLowFunds = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("funds");
                return LowFunds;
            };
            Hooks.Salvage = target => Trace.Add("salvage:" + target.Value);
        }

        /// <summary>현재 본체의 지정 DWORD를 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>현재 본체의 지정 DWORD만 입력한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>현재 QWORD 기한을 정확한 double로 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 기한이다.</returns>
        public double Time(int at) => BitConverter.UInt64BitsToDouble(((ulong)Field(at + 4) << 32) | Field(at));

        /// <summary>명시 기한 입력이며 두 DWORD 이외를 바꾸지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 기한이다.</param>
        public void Time(int at, double value)
        {
            ulong bits = BitConverter.DoubleToUInt64Bits(value);
            Field(at, (uint)bits);
            Field(at + 4, (uint)(bits >> 32));
        }

        /// <summary>목표 좌표 입력을 같은 ad0/ad4에 쓴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="point">쓸 목표 좌표다.</param>
        public void Target(RawPathPoint point)
        {
            Field(0xad0, BitConverter.SingleToUInt32Bits(point.X));
            Field(0xad4, BitConverter.SingleToUInt32Bits(point.Y));
        }

        /// <summary>물리 슬롯의 현재 두 좌표만 입력한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="actor">쓸 SID다.</param>
        /// <param name="point">쓸 좌표다.</param>
        public void Position(Sid actor, RawPathPoint point)
        {
            Put(Pool.RawSlot(actor.Value), 14, BitConverter.SingleToUInt32Bits(point.X));
            Put(Pool.RawSlot(actor.Value), 18, BitConverter.SingleToUInt32Bits(point.Y));
        }

        /// <summary>명시 건물 목록을 실제 backing에 기록한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="values">쓸 SID 목록이다.</param>
        public void Buildings(Sid[] values)
        {
            Field(0x308, (uint)values.Length);
            int at = 0;
            // 명시 목록 순서를 보존하는 반복문이다.
            foreach (Sid sid in values)
            {
                Put(Memory.Bytes(List), at, sid.Value);
                at += 4;
            }
        }

        /// <summary>실제 처리기에 현재 입력을 빌려 한 번 호출한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="actor">행동할 사제 SID다.</param>
        public void Act(Sid actor)
        {
            var body = new RawAiPriestActions(Memory, Pool, new AiPriestActionGlobals
            {
                Now = () => Now,
                MapLimit = () => MapLimit,
                DaisType = () => DaisType,
            }, Hooks);
            Assert.Same(Pool, body.Pool);
            body.Act(Token, actor);
        }

        /// <summary>앞선 공통 관문 뒤에 기대하는 정확한 호출 목록을 만든다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
        /// <param name="tail">뒤에 붙일 호출 목록이다.</param>
        /// <returns>앞선 공통 관문과 이은 전체 목록이다.</returns>
        public List<string> AfterGates(string[] tail)
        {
            var result = new List<string> { "vulnerable", "busy", "path" };
            result.AddRange(tail);
            return result;
        }
    }

    /// <summary>진행 중 탐색은 사제 0/생각 기한/TYPE 0보다 먼저 처리한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void SearchPrefixPrecedesActorAndDeadlines()
    {
        var scene = new AiPriestScene();
        scene.Field(0x5a4, 0xffffffffu);
        scene.Field(0x5a0, 0);
        scene.Time(0x5a8, 999);
        scene.Act(new Sid(0));
        Assert.Equal(["scan:0"], scene.Trace);
        Assert.Equal(0u, scene.Field(0x574));
        scene.Trace.Clear();
        scene.Act(new Sid(0));
        Assert.Empty(scene.Trace);
    }

    /// <summary>생각 관문의 NaN·상처·예약·이동 중 최대 HP 조회·건설 기한을 구분한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void OrderedReadinessAndEarlyGates()
    {
        var scene = new AiPriestScene();
        scene.Time(0x5a8, double.NaN);
        scene.Act(scene.Priest);
        Assert.Empty(scene.Trace);
        scene.Time(0x5a8, 100);
        scene.Vulnerable = true;
        scene.Act(scene.Priest);
        Assert.Equal(["vulnerable"], scene.Trace);
        scene.Trace.Clear();
        scene.Vulnerable = false;
        scene.Busy = true;
        scene.Act(scene.Priest);
        Assert.Equal(["vulnerable", "busy"], scene.Trace);
        scene.Trace.Clear();
        scene.Busy = false;
        scene.Path = scene.Other;
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["max"]), scene.Trace);
        scene.Trace.Clear();
        scene.Path = new Sid(0);
        scene.Time(0x5c0, double.NaN);
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates([]), scene.Trace);
    }

    /// <summary>의식 구간은 명시 사제 인자가 아닌 본체 29c의 현재 사제를 묻는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void RitualUsesStoredSelfAndStops()
    {
        var scene = new AiPriestScene();
        scene.Field(0x248, 0x400);
        scene.Field(0x2a4, scene.Dais.Value);
        scene.Field(0x29c, scene.Self.Value);
        scene.Ritual = true;
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["ritual:6"]), scene.Trace);
    }

    /// <summary>제단 하위 효과 뒤 실제 희생 요청의 두 SID를 다시 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void AltarRequeriesSelfAndDaisBeforeSacrifice()
    {
        var scene = new AiPriestScene();
        scene.Field(0x248, 0x400);
        scene.Field(0x2a4, scene.Dais.Value);
        scene.Hooks.AltarEnemy = (enemy, dais) =>
        {
            Assert.Equal(2u, enemy);
            Assert.Equal(scene.Dais, dais);
            scene.Trace.Add("altar");
            scene.Field(0x29c, scene.Self.Value);
            scene.Field(0x2a4, scene.Other.Value);
            return true;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["ritual:5", "altar", "target:6:10:1"]), scene.Trace);
    }

    /// <summary>거리 2와 NaN은 접근하지 않으며 2 초과만 현재 다이스에 이동한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisStrictDistanceAndCurrentTarget()
    {
        var scene = new AiPriestScene();
        scene.Field(0x248, 0x400);
        scene.Field(0x2a4, scene.Dais.Value);
        scene.Hooks.Distance = (point, target) =>
        {
            Assert.Equal(scene.Dais, target);
            scene.Trace.Add("distance");
            scene.Field(0x2a4, scene.Other.Value);
            return 2.1;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["ritual:5", "altar:2:7", "distance", "target:5:10:0"]), scene.Trace);
        // 두 비접근 경계는 이후 TYPE 선택과 새 탐색까지 진행해야 하는 반복문이다.
        foreach (double distance in new double[] { 2.0, double.NaN })
        {
            var boundary = new AiPriestScene();
            boundary.Field(0x248, 0x400);
            boundary.Field(0x2a4, boundary.Dais.Value);
            boundary.Distance = distance;
            boundary.Act(boundary.Priest);
            Assert.Equal(boundary.AfterGates(["ritual:5", "altar:2:7", "distance:7", "select", "begin"]), boundary.Trace);
        }
    }

    /// <summary>TYPE 선택이 저자금보다 먼저이며 HP 선저장·현재 800h 능력으로 수집을 고른다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void LowFundsSelectionAndSavedHpFallback()
    {
        var scene = new AiPriestScene();
        scene.Field(0x2d8, 399);
        scene.Hooks.MaxHitPoints = actor =>
        {
            Assert.Equal(scene.Priest, actor);
            scene.Trace.Add("max");
            Put(scene.Pool.RawSlot(actor.Value), 26, 0);
            scene.Field(0x248, 0x800);
            return 1000;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "max", "collect"]), scene.Trace);
        Assert.Equal(0.0, scene.Time(0x5c8));
        var flying = new AiPriestScene();
        flying.Selected = 0;
        flying.Field(0x2cc, 1);
        flying.Act(flying.Priest);
        Assert.Equal(flying.AfterGates(["select", "home"]), flying.Trace);
    }

    /// <summary>TYPE 0은 유효 목표만 지우고 미정 목표에서는 귀환/수집으로 넘어간다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void NoTypeValidTargetOnlyClears()
    {
        var scene = new AiPriestScene();
        scene.Selected = 0;
        scene.Target(new RawPathPoint(50, 60));
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select"]), scene.Trace);
        Assert.Equal(0xc61c4000u, scene.Field(0xad0));
        Assert.Equal(0xc61c4000u, scene.Field(0xad4));
        scene.Trace.Clear();
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "max", "home"]), scene.Trace);
    }

    /// <summary>좌표 일치 배치는 계획 기한보다 먼저 20초를 쓰고 raw 좌표를 넘긴 뒤 목표를 지운다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DeployUsesRawPositionAndPriorTimer()
    {
        var scene = new AiPriestScene();
        scene.Target(new RawPathPoint(21, 31));
        scene.Time(0x5c8, 999);
        scene.Hooks.Deploy = (brain, type, point) =>
        {
            Assert.Equal(scene.Token, brain);
            Assert.Equal(86u, type);
            Assert.Equal(20.2f, point.X);
            Assert.Equal(30.7f, point.Y);
            Assert.Equal(120.0, scene.Time(0x5c0));
            scene.Trace.Add("deploy");
            scene.Target(new RawPathPoint(90, 91));
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "deploy"]), scene.Trace);
        Assert.Equal(0xc61c4000u, scene.Field(0xad0));
        Assert.Equal(0xc61c4000u, scene.Field(0xad4));
        Assert.Equal(999.0, scene.Time(0x5c8));
    }

    /// <summary>실제 배치 하위 누락은 앞서 쓴 20초 기한을 보존하며 목표를 지우지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingDeployPreservesPrefix()
    {
        var scene = new AiPriestScene();
        scene.Target(new RawPathPoint(21, 31));
        scene.Hooks.Deploy = null;
        Assert.ThrowsAny<Exception>(() => scene.Act(scene.Priest));
        Assert.Equal(120.0, scene.Time(0x5c0));
        Assert.Equal(21f, BitConverter.UInt32BitsToSingle(scene.Field(0xad0)));
    }

    /// <summary>계획 기한의 less/unordered는 새 탐색·후보 소비를 수행하지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void SearchDeadlineIsOrdered()
    {
        // 명시 두 기한만 바꾸고 같은 관문까지 도달하는 반복문이다.
        foreach (double due in new double[] { 101.0, double.NaN })
        {
            var scene = new AiPriestScene();
            scene.Time(0x5c8, due);
            scene.Act(scene.Priest);
            Assert.Equal(scene.AfterGates(["select"]), scene.Trace);
            Assert.Empty(scene.Requests);
        }
    }

    /// <summary>완료된 같은 TYPE 탐색은 실제 최적 좌표를 보정하고 이동하며 3초를 쓰지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CompletedSearchConsumesSnapsAndMoves()
    {
        var scene = new AiPriestScene();
        scene.Field(0x574, 86);
        scene.Field(0x57c, BitConverter.SingleToUInt32Bits(20.2f));
        scene.Field(0x580, BitConverter.SingleToUInt32Bits(30.7f));
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "point"]), scene.Trace);
        Assert.Equal(0u, scene.Field(0x574));
        Assert.Equal(0.0, scene.Time(0x5c8));
        Assert.Equal(21f, BitConverter.UInt32BitsToSingle(scene.Field(0xad0)));
        var empty = new AiPriestScene();
        empty.Field(0x574, 86);
        empty.Field(0x57c, 0xc61c4000u);
        empty.Field(0x580, 0xc61c4000u);
        empty.Act(empty.Priest);
        Assert.Equal(empty.AfterGates(["select"]), empty.Trace);
        Assert.Equal(103.0, empty.Time(0x5c8));
        Assert.Equal(-9999f, BitConverter.UInt32BitsToSingle(empty.Field(0xad0)));
        Assert.Equal(0u, empty.Field(0x574));
    }

    /// <summary>새 탐색은 그 시점의 8인자를 전달하고 하위 효과 뒤 현재 시각+3을 쓴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void NewSearchSnapshotsArgumentsAndCurrentClock()
    {
        var scene = new AiPriestScene();
        scene.Hooks.Begin = (brain, request) =>
        {
            Assert.Equal(scene.Token, brain);
            scene.Trace.Add("begin");
            scene.Requests.Add(request);
            scene.Field(0x270, 0);
            scene.Field(0x260, 3);
            scene.Now = 125;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "begin"]), scene.Trace);
        Assert.Single(scene.Requests);
        Assert.Equal(128.0, scene.Time(0x5c8));
        var request = scene.Requests[0];
        Assert.Equal(86u, request.Type);
        Assert.Equal(1u, request.Owner);
        Assert.Equal(10f, request.Bounds.Left);
        Assert.Equal(20f, request.Bounds.Top);
        Assert.Equal(50f, request.Bounds.Right);
        Assert.Equal(70f, request.Bounds.Bottom);
        Assert.Equal(25f, request.Avoid.X);
        Assert.Equal(26f, request.Avoid.Y);
    }

    /// <summary>선택 하위가 탐색을 다시 진행 상태로 바꾸면 완료 후보를 소비하지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentSearchProgressAfterSelect()
    {
        var scene = new AiPriestScene();
        scene.Field(0x574, 86);
        scene.Hooks.Select = brain =>
        {
            Assert.Equal(scene.Token, brain);
            scene.Trace.Add("select");
            scene.Field(0x5a4, 2);
            scene.Field(0x5a0, 4);
            return 86u;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select"]), scene.Trace);
        Assert.Equal(86u, scene.Field(0x574));
        Assert.Equal(103.0, scene.Time(0x5c8));
    }

    /// <summary>회수 대상은 signed HP 최대 첫 객체이며 free/dead 추가 필터 없이 원본 요청 순서를 따른다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisReclaimHighestSignedHpAndRequestOrder()
    {
        var scene = new AiPriestScene();
        scene.Selected = 165;
        scene.Field(0x574, 165);
        scene.Field(0x57c, 0xc61c4000u);
        scene.Field(0x580, 0xc61c4000u);
        scene.Buildings([scene.Building, scene.Other, scene.Home]);
        Put(scene.Pool.RawSlot(scene.Building.Value), 26, 2000);
        Put(scene.Pool.RawSlot(scene.Other.Value), 26, 2000);
        Put(scene.Pool.RawSlot(scene.Home.Value), 26, 0xffffffffu);
        scene.Pool.RawSlot(scene.Building.Value)[11] = 3;
        scene.Hooks.Salvage = target =>
        {
            Assert.Equal(scene.Building, target);
            scene.Trace.Add("salvage:9");
            Assert.Equal(0.0, scene.Time(0x5e8));
            scene.Now = 120;
            scene.Field(0x260, 3);
            scene.Field(0x270, BitConverter.SingleToUInt32Bits(11.0f));
        };
        scene.Hooks.Scan = (brain, budget) =>
        {
            Assert.Equal(scene.Token, brain);
            Assert.Equal(1u, budget);
            Assert.Equal(130.0, scene.Time(0x5e8));
            scene.Trace.Add("scan:1");
            scene.Now = 140;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "target:5:9:0", "salvage:9", "begin", "scan:1"]), scene.Trace);
        Assert.Equal(130.0, scene.Time(0x5e8));
        Assert.Equal(143.0, scene.Time(0x5c8));
        Assert.Single(scene.Requests);
        Assert.Equal(3u, scene.Requests[0].Owner);
        Assert.Equal(11f, scene.Requests[0].Bounds.Left);
    }

    /// <summary>건물 부재 시 적 저자금 확인 뒤 현재 신전을 읽으며 부재/거짓은 기한만 쓴다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ReclaimFallbackUsesCurrentTempleAndFunds()
    {
        var scene = new AiPriestScene();
        scene.Selected = 165;
        scene.Field(0x574, 165);
        scene.Field(0x57c, 0xc61c4000u);
        scene.Field(0x580, 0xc61c4000u);
        scene.Hooks.EnemyLowFunds = brain =>
        {
            Assert.Equal(scene.Token, brain);
            scene.Trace.Add("funds");
            scene.Field(0x294, scene.Other.Value);
            return true;
        };
        scene.Act(scene.Priest);
        Assert.Equal(scene.AfterGates(["select", "funds", "target:5:10:0", "salvage:10", "begin", "scan:1"]), scene.Trace);
        // 자금 조건 거짓과 신전 부재를 각각 같은 완료 탐색으로 검사하는 반복문이다.
        foreach (bool funds in new bool[] { false, true })
        {
            var empty = new AiPriestScene();
            empty.Selected = 165;
            empty.Field(0x574, 165);
            empty.Field(0x57c, 0xc61c4000u);
            empty.Field(0x580, 0xc61c4000u);
            empty.Field(0x294, 0);
            empty.LowFunds = funds;
            empty.Act(empty.Priest);
            Assert.Equal(empty.AfterGates(["select", "funds"]), empty.Trace);
            Assert.Equal(103.0, empty.Time(0x5c8));
            Assert.Equal(0.0, empty.Time(0x5e8));
        }
    }

    /// <summary>회수 기한은 경과 판정이 아닌 0 비교다. 과거 기한과 NaN도 재회수를 막는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ReclaimTimerNonzeroAndNan()
    {
        // 이미 경과한 값/NaN은 요청을 막고 +0/-0만 처음 회수를 허용하는 반복문이다.
        foreach (double due in new double[] { 1.0, double.NaN, 0.0, -0.0 })
        {
            var scene = new AiPriestScene();
            scene.Selected = 165;
            scene.Field(0x574, 165);
            scene.Field(0x57c, 0xc61c4000u);
            scene.Field(0x580, 0xc61c4000u);
            scene.Buildings([scene.Building]);
            scene.Time(0x5e8, due);
            scene.Act(scene.Priest);
            if (due == 0.0)
            {
                Assert.Equal(scene.AfterGates(["select", "target:5:9:0", "salvage:9", "begin", "scan:1"]), scene.Trace);
            }
            else
            {
                Assert.Equal(scene.AfterGates(["select"]), scene.Trace);
            }
        }
    }

    /// <summary>회수 하위 누락 시 대상 이동 요청 뒤 기한/재탐색/3초 쓰기는 발생하지 않는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingSalvagePreservesTargetRequest()
    {
        var scene = new AiPriestScene();
        scene.Selected = 165;
        scene.Field(0x574, 165);
        scene.Field(0x57c, 0xc61c4000u);
        scene.Field(0x580, 0xc61c4000u);
        scene.Buildings([scene.Building]);
        scene.Hooks.Salvage = null;
        Assert.ThrowsAny<Exception>(() => scene.Act(scene.Priest));
        Assert.Equal(scene.AfterGates(["select", "target:5:9:0"]), scene.Trace);
        Assert.Equal(0.0, scene.Time(0x5e8));
        Assert.Equal(0.0, scene.Time(0x5c8));
        Assert.Empty(scene.Requests);
    }

    /// <summary>원본 0.99999 보정은 ceil/반올림과 다르며 mapLimit은 float로 저장한 값을 사용한다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void PointSnapAndValidityNativeBoundaries()
    {
        RawPathPoint point = AiPriestPoints.Snap(new RawPathPoint(1.000001f, -2.9f));
        Assert.Equal(1f, point.X);
        Assert.Equal(-1f, point.Y);
        point = AiPriestPoints.Snap(new RawPathPoint(-10000, float.NaN));
        Assert.Equal(-9999f, point.X);
        Assert.Equal(0f, point.Y);
        Assert.True(AiPriestPoints.Valid(new RawPathPoint(0.1f, 255.9f), 256));
        Assert.False(AiPriestPoints.Valid(new RawPathPoint(0, 1), 256));
        Assert.False(AiPriestPoints.Valid(new RawPathPoint(1, 256), 256));
        Assert.False(AiPriestPoints.Valid(new RawPathPoint(float.NaN, 1), 256));
        Assert.False(AiPriestPoints.Valid(new RawPathPoint(16777216.0f, 1), 16777217));
    }

    /// <summary>제단의 owner BYTE가 아닌 argument WORD를 전체 enemy DWORD와 비교하고 찾은 SID 0도 읽는다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void AltarArgumentWordAndPhysicalZero()
    {
        var scene = new AiPriestScene();
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(scene.Altar.Value).Slice(12, 2), 2);
        scene.Pool.RawSlot(scene.Altar.Value)[34] = 3;
        Sid Find(float x, float y, uint type)
        {
            Assert.Equal(40f, x);
            Assert.Equal(50f, y);
            Assert.Equal(164u, type);
            return scene.Altar;
        }

        Assert.True(AiPriestPoints.AltarContainsEnemy(scene.Pool, scene.Dais, 2, 164, Find));
        Assert.False(AiPriestPoints.AltarContainsEnemy(scene.Pool, scene.Dais, 0x10002, 164, Find));
        uint argument = BinaryPrimitives.ReadUInt16LittleEndian(scene.Pool.Slot(new Sid(0).Value).Slice(12, 2));
        Assert.True(AiPriestPoints.AltarContainsEnemy(scene.Pool, scene.Dais, argument, 164, (x, y, type) => new Sid(0)));
    }

    /// <summary>의식 구간 10..14는 물리 frame의 첫 signed 문자에서 구하며 상태 BYTE와 무관하다. 2026-11-25 R4-5b 사제 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void RitualUsesActualFrameCode()
    {
        var scene = new AiPriestScene();
        var frames = Enumerable.Range(0, 159).Select(_ => new RiftTypeFrames([])).ToList();
        frames[158] = new RiftTypeFrames([new RiftFrameCode((byte)'J', 0, 0, 0), new RiftFrameCode((byte)'K', 0, 0, 0), new RiftFrameCode((byte)'O', 0, 0, 0), new RiftFrameCode((byte)'P', 0, 0, 0), new RiftFrameCode(0xff, 0, 0, 0)]);
        scene.Pool.RawSlot(scene.Priest.Value)[40] = 0xe0;
        // 각 물리 프레임을 그대로 사용하며 state 상위 비트로 의식 여부를 만들지 않는 반복문이다.
        for (uint frame = 0; frame < 5; frame++)
        {
            Put(scene.Pool.RawSlot(scene.Priest.Value), 36, frame);
            Assert.Equal(frame == 1 || frame == 2, AiPriestPoints.InRitual(scene.Pool, frames, scene.Priest));
        }
    }
}
