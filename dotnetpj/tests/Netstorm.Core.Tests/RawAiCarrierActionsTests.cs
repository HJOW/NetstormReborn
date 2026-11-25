// 실제 본체/목록으로 전체 수집·포획 순서와 genus/다이스 하위를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 운송 행동을 검사하는 클래스다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.
// 근거: cpppj tests/AiCarrierActionsTests.cpp의 15건 전부다.
public sealed class RawAiCarrierActionsTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>원본 raw 입력과 선택된 하위 효과/호출 순서를 준비한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    private sealed class AiCarrierScene
    {
        /// <summary>같은 실제 풀/할당 저장소다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>실제 본체 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4c000000u, bytes => bytes.Fill(0));

        /// <summary>현재 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Actor;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Enemy;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Dais;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Geyser;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Other;

        /// <summary>명시 입력의 역할 SID다.</summary>
        public Sid Crystal;

        /// <summary>현재 무효 그래프 값이다.</summary>
        public uint Invalid = 254;

        /// <summary>현재 진단 값이다.</summary>
        public uint Checking;

        /// <summary>현재 시각이다.</summary>
        public double Now;

        /// <summary>현재 권한이다.</summary>
        public bool Boss = true;

        /// <summary>명시 하위의 현재 결과 입력이다.</summary>
        public readonly Dictionary<ushort, Sid> Prisoners = [];

        /// <summary>명시 하위의 현재 결과 입력이다.</summary>
        public readonly Dictionary<ushort, Sid> Paths = [];

        /// <summary>명시 하위의 현재 결과 입력이다.</summary>
        public readonly Dictionary<ushort, uint> Graphs = [];

        /// <summary>선택된 실제 하위다.</summary>
        public AiCarrierActionHooks Hooks = new();

        /// <summary>원문 효과/순서 관찰이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>명시 상태 입력이다.</summary>
        public int Crystals;

        /// <summary>명시 상태 입력이다.</summary>
        public bool Available = true;

        /// <summary>명시 상태 입력이다.</summary>
        public bool Vulnerable = true;

        /// <summary>명시 상태 입력이다.</summary>
        public bool Flying;

        /// <summary>명시 상태 입력이다.</summary>
        public uint[] Targets = [0u, 0u];

        /// <summary>다음에 할당할 SID 번호다.</summary>
        private ushort _nextSid = 5;

        /// <summary>기본은 정지한 일반 수집자 한 명이며 결과 판단은 입력에 합성하지 않는다.</summary>
        public AiCarrierScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Actor = New(133);
            Enemy = New(158);
            Dais = New(165);
            Geyser = New(80);
            Other = New(133);
            Crystal = New(6);
            Types[158].Flags2 = 0x200000;
            Types[165].Flags2 = 0x400000;
            Types[80].Flags2 = 0x10000000;
            Field(0x2a0, Enemy.Value);
            Field(0x264, 2);
            Field(0x294, Dais.Value);
            Field(0x298, 7);
            List(0x2dc, [Actor]);
            List(0x384, []);
            List(0x330, []);
            Graphs[Actor.Value] = 7;
            Graphs[Enemy.Value] = 7;
            Graphs[Geyser.Value] = 7;
            Hooks.MaxHitPoints = _ =>
            {
                Trace.Add("max");
                return 100;
            };
            Hooks.Prisoner = sid =>
            {
                Trace.Add("prisoner:" + Role(sid));
                return Prisoners.TryGetValue(sid.Value, out Sid found) ? found : new Sid(0);
            };
            Hooks.Path = sid =>
            {
                Trace.Add("path:" + Role(sid));
                return Paths.TryGetValue(sid.Value, out Sid found) ? found : new Sid(0);
            };
            Hooks.DaisPoint = sid =>
            {
                Assert.Equal(Dais, sid);
                Trace.Add("dais");
                return new RawPathPoint(10, 12);
            };
            Hooks.Point = (brain, sid, point) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("point:" + Role(sid));
            };
            Hooks.Home = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("home:" + Role(sid));
            };
            Hooks.EnemyAvailable = owner =>
            {
                Assert.Equal(2u, owner);
                Trace.Add("enemy");
                return Available;
            };
            Hooks.Vulnerable = _ =>
            {
                Trace.Add("vulnerable");
                return Vulnerable;
            };
            Hooks.FlyingGetter = _ =>
            {
                Trace.Add("flying");
                return Flying;
            };
            Hooks.Graph = sid =>
            {
                Trace.Add("graph:" + Role(sid));
                return Graphs[sid.Value];
            };
            Hooks.Crystals = sid =>
            {
                Assert.Equal(Actor, sid);
                Trace.Add("crystals");
                return Crystals;
            };
            Hooks.FirstCrystal = sid =>
            {
                Assert.Equal(Actor, sid);
                Trace.Add("first");
                return Crystal;
            };
            Hooks.DropRequest = sid => Trace.Add("drop:" + Role(sid));
            Hooks.Destroy = sid =>
            {
                Assert.Equal(Crystal, sid);
                Trace.Add("destroy");
            };
            Hooks.Target = (brain, sid, target) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Actor, sid);
                Trace.Add("target:" + Role(target));
            };
            Hooks.Remember = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("remember:" + Role(sid));
            };
            Hooks.Collect = (brain, sid) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("collect:" + Role(sid));
            };
            Hooks.Targets = sid =>
            {
                Assert.Equal(Actor, sid);
                Trace.Add("targets");
                return Targets;
            };
            Hooks.Reclassify = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("reclassify");
            };
            Hooks.Stop = sid => Trace.Add("stop:" + Role(sid));
            Hooks.Assertion = (text, line) => Trace.Add("assert:" + line);
        }

        /// <summary>확보한 실제 payload만 명시 0으로 초기화한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="type">쓸 TYPE 번호다.</param>
        /// <returns>확보한 SID다.</returns>
        public Sid New(byte type)
        {
            ushort sid = _nextSid++;
            Span<byte> raw = Pool.RawSlot(sid);
            raw.Fill(0);
            raw[RawSidPool.TypeOffset] = type;
            return new Sid(sid);
        }

        /// <summary>호출 인자를 쉽게 구별하는 고정 이름이다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="sid">이름을 구할 SID다.</param>
        /// <returns>A/E/O/G 또는 other다.</returns>
        public string Role(Sid sid)
        {
            if (sid == Actor)
            {
                return "A";
            }

            if (sid == Enemy)
            {
                return "E";
            }

            if (sid == Other)
            {
                return "O";
            }

            if (sid == Geyser)
            {
                return "G";
            }

            return "other";
        }

        /// <summary>현재 본체 DWORD를 쓴다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>원본 QWORD 기한의 두 DWORD를 입력한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 기한이다.</param>
        public void Due(int at, double value)
        {
            ulong bits = BitConverter.DoubleToUInt64Bits(value);
            Field(at, (uint)bits);
            Field(at + 4, (uint)(bits >> 32));
        }

        /// <summary>실제 backing에 입력 목록을 연결한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <param name="at">목록 기록 위치다.</param>
        /// <param name="values">쓸 SID 목록이다.</param>
        /// <param name="capacity">목록 용량이다.</param>
        /// <returns>확보한 backing 토큰이다.</returns>
        public uint List(int at, Sid[] values, uint capacity = 8)
        {
            uint pointer = Memory.Allocate((int)capacity * 4);
            int offset = 0;
            // 명시 입력 순서만 기록하는 반복문이다.
            foreach (Sid value in values)
            {
                Put(Memory.Bytes(pointer), offset, value.Value);
                offset += 4;
            }

            Field(at, pointer);
            Field(at + 4, capacity);
            Field(at + 8, (uint)values.Length);
            return pointer;
        }

        /// <summary>같은 현재 전역과 실제 저장소를 빌린다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
        /// <returns>검사할 운송 행동 몸체다.</returns>
        public RawAiCarrierActions Make() => new(Memory, Pool, Types, new AiCarrierActionGlobals
        {
            Now = () => Now,
            Boss = () => Boss,
            Checking = new RawCheckingFlag(Checking),
            InvalidGraph = () => Invalid,
        }, Hooks);
    }

    /// <summary>물리 적 SID면 free/dead여도 최대 HP prefix를 실행하고 빈 일반 목록은 그 뒤 종료한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixAndEmptyListOrder()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x2e4, 0);
        scene.Pool.RawSlot(scene.Enemy.Value)[11] = 3;
        scene.Make().Act(scene.Token);
        Assert.Equal(["max"], scene.Trace);
        scene.Trace.Clear();
        scene.Field(0x2a0, 0);
        scene.Hooks = new AiCarrierActionHooks();
        scene.Make().Act(scene.Token);
        Assert.Empty(scene.Trace);
    }

    /// <summary>포획 귀환은 빈 일반 목록/미도달 기한보다 우선하고 이동 효과 뒤 현재 큐를 한 actor로 축소한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void PrisonerQueuePriorityAndCollapse()
    {
        var scene = new AiCarrierScene();
        uint backing = scene.List(0x384, [new Sid(0), scene.Actor, scene.Other]);
        scene.Prisoners[scene.Actor.Value] = scene.Crystal;
        scene.Field(0x2e4, 0);
        scene.Due(0x5a8, double.NaN);
        scene.Field(0x2a4, scene.Dais.Value);
        scene.Hooks.Point = (brain, actor, point) =>
        {
            Assert.Equal(scene.Actor, actor);
            Assert.Equal(10f, point.X);
            scene.Trace.Add("point:A");
            scene.Field(0x38c, 7);
        };
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "prisoner:A", "path:A", "dais", "point:A"], scene.Trace);
        Assert.Equal(1u, scene.Field(0x38c));
        Assert.Equal(scene.Actor.Value, Get(scene.Memory.Bytes(backing), 0));
        Assert.Equal(scene.Other.Value, Get(scene.Memory.Bytes(backing), 8));
        scene.Trace.Clear();
        scene.Field(0x388, 0);
        scene.Field(0x2a4, 0);
        scene.Make().Act(scene.Token);
        Assert.Equal("home:A", scene.Trace[^1]);
        Assert.Equal(0u, scene.Field(0x38c));
    }

    /// <summary>less/unordered 기한은 순환 인덱스도 바꾸지 않고 prefix까지만 진행한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ThinkTimerOrderedAndUnordered()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x6d0, 77);
        scene.Due(0x5a8, 1);
        scene.Make().Act(scene.Token);
        Assert.Equal(77u, scene.Field(0x6d0));
        Assert.Equal(["max"], scene.Trace);
        scene.Trace.Clear();
        scene.Due(0x5a8, double.NaN);
        scene.Make().Act(scene.Token);
        Assert.Equal(77u, scene.Field(0x6d0));
        Assert.Single(scene.Trace);
        scene.Trace.Clear();
        scene.Due(0x5a8, 0);
        scene.Now = double.NaN;
        scene.Make().Act(scene.Token);
        Assert.Equal(77u, scene.Field(0x6d0));
        Assert.Single(scene.Trace);
        scene.Now = 0;
        scene.Make().Act(scene.Token);
        Assert.Equal(0u, scene.Field(0x6d0));
        Assert.Equal("collect:A", scene.Trace[^1]);
    }

    /// <summary>순환은 다음 항목부터 시작하며 free는 모든 중복을 안정 제거하고 cursor를 보정하지 않는다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void RoundRobinAndFreeStableRemoval()
    {
        var scene = new AiCarrierScene();
        uint backing = scene.List(0x2dc, [scene.Actor, scene.Other, scene.Other]);
        scene.Pool.RawSlot(scene.Other.Value)[11] = 1;
        scene.Checking = 1;
        scene.Make().Act(scene.Token);
        Assert.Equal(1u, scene.Field(0x6d0));
        Assert.Equal(1u, scene.Field(0x2e4));
        Assert.Equal(scene.Actor.Value, Get(scene.Memory.Bytes(backing), 0));
        Assert.Equal(scene.Other.Value, Get(scene.Memory.Bytes(backing), 8));
        Assert.Equal(["max", "assert:3076"], scene.Trace);
    }

    /// <summary>free 진단이 반환하며 상태를 고치면 계속하고 priest genus 진단은 checking 0에서도 호출한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ReturningDiagnosticRereadsState()
    {
        var scene = new AiCarrierScene();
        Span<byte> raw = scene.Pool.RawSlot(scene.Actor.Value);
        scene.Checking = 1;
        raw[11] = 1;
        scene.Hooks.Assertion = (text, line) =>
        {
            Assert.Equal("!s.isFree()", text);
            Assert.Equal(0xc04u, line);
            scene.Pool.RawSlot(scene.Actor.Value)[11] = 0;
            scene.Trace.Add("fixed");
        };
        scene.Make().Act(scene.Token);
        Assert.Equal(1u, scene.Field(0x2e4));
        Assert.Equal(["max", "fixed", "path:A", "prisoner:A", "collect:A"], scene.Trace);
        scene.Trace.Clear();
        scene.Checking = 0;
        scene.Types[133].Flags2 = 0x200000;
        scene.Hooks.Assertion = (text, line) =>
        {
            Assert.Equal("!( s.getGenus() & gPRIEST )", text);
            Assert.Equal(0xc0au, line);
            scene.Trace.Add("priestAssert");
        };
        scene.Make().Act(scene.Token);
        Assert.Equal("priestAssert", scene.Trace[1]);
    }

    /// <summary>서버 포획은 첫 크리스털만 삭제하고 삭제 효과 뒤 현재 적으로 이동한 다음 등록한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ServerCaptureCrystalAndCurrentEnemy()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x248, 0x100);
        scene.Crystals = 2;
        scene.Hooks.Destroy = crystal =>
        {
            Assert.Equal(scene.Crystal, crystal);
            scene.Trace.Add("destroy");
            scene.Field(0x2a0, scene.Other.Value);
        };
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "path:A", "enemy", "vulnerable", "graph:A", "graph:E", "crystals", "first", "destroy", "target:O", "remember:A"], scene.Trace);
    }

    /// <summary>현재 boss는 첫 크리스털 조회 뒤 재판정하며 비서버 내려놓기는 이동/등록을 생략한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CaptureAuthorityAfterCrystalLookup()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x248, 0x100);
        scene.Crystals = 1;
        scene.Hooks.FirstCrystal = _ =>
        {
            scene.Trace.Add("first");
            scene.Boss = false;
            return scene.Crystal;
        };
        scene.Make().Act(scene.Token);
        Assert.Equal("drop:A", scene.Trace[^1]);
        Assert.DoesNotContain("destroy", scene.Trace);
        Assert.DoesNotContain("remember:A", scene.Trace);
        scene.Trace.Clear();
        scene.Hooks.FirstCrystal = _ =>
        {
            scene.Trace.Add("first");
            scene.Boss = true;
            return scene.Crystal;
        };
        scene.Make().Act(scene.Token);
        Assert.Equal("destroy", scene.Trace[^3]);
        Assert.Equal("remember:A", scene.Trace[^1]);
    }

    /// <summary>첫 비행 포획 운반자는 그래프를 요구하지 않으며 비행 운반자가 이미 있으면 그래프로 판단한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void FlyingCaptureAndGraphFallback()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x248, 0x100);
        scene.Types[133].Flags2 = 0x20000;
        scene.Hooks.Graph = null;
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "path:A", "enemy", "vulnerable", "flying", "crystals", "target:E", "remember:A"], scene.Trace);
        scene.Trace.Clear();
        scene.Flying = true;
        scene.Hooks.Graph = sid =>
        {
            scene.Trace.Add("graph:" + scene.Role(sid));
            return sid == scene.Actor ? 7u : 8u;
        };
        scene.Make().Act(scene.Token);
        Assert.Equal("prisoner:A", scene.Trace[^2]);
        Assert.Equal("collect:A", scene.Trace[^1]);
    }

    /// <summary>참여 효과 뒤 현재 적을 HP 판정에 전달하고 포획 기한 NaN이면 비행/그래프를 조회하지 않는다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CaptureShortCircuitAndTimerEffects()
    {
        var scene = new AiCarrierScene();
        scene.Field(0x248, 0x100);
        scene.Hooks.EnemyAvailable = _ =>
        {
            scene.Trace.Add("enemy");
            scene.Field(0x2a0, scene.Other.Value);
            return true;
        };
        scene.Hooks.Vulnerable = sid =>
        {
            Assert.Equal(scene.Other, sid);
            scene.Trace.Add("vulnerable");
            scene.Due(0x5e0, double.NaN);
            return true;
        };
        scene.Hooks.Graph = null;
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "path:A", "enemy", "vulnerable", "prisoner:A", "collect:A"], scene.Trace);
    }

    /// <summary>일반 선택의 포획 사제 귀환은 큐를 축소하지 않으며 수집 뒤에는 현재 큐의 모든 중복을 제거한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DeliveryAndCollectionCurrentQueue()
    {
        var scene = new AiCarrierScene();
        scene.Prisoners[scene.Actor.Value] = scene.Crystal;
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "path:A", "prisoner:A", "home:A"], scene.Trace);
        Assert.Equal(0u, scene.Field(0x38c));
        scene.Trace.Clear();
        scene.Prisoners.Clear();
        scene.Hooks.Collect = (brain, sid) =>
        {
            Assert.Equal(scene.Token, brain);
            scene.Trace.Add("collect:A");
            scene.List(0x384, [scene.Other, scene.Actor, scene.Actor, scene.Geyser]);
        };
        scene.Make().Act(scene.Token);
        byte[] list = scene.Memory.Bytes(scene.Field(0x384));
        Assert.Equal(2u, scene.Field(0x38c));
        Assert.Equal(scene.Other.Value, Get(list, 0));
        Assert.Equal(scene.Geyser.Value, Get(list, 4));
        Assert.Equal(scene.Geyser.Value, Get(list, 12));
    }

    /// <summary>포획 대기 중 경로는 유지하고 신전/유효 그래프 없이는 목표 조회 없이 정지하며 target A 0은 정지하지 않는다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ActivePathProtectionAndMissingTarget()
    {
        var scene = new AiCarrierScene();
        scene.Paths[scene.Actor.Value] = new Sid(100);
        scene.List(0x384, [scene.Actor]);
        scene.Make().Act(scene.Token);
        Assert.Equal("path:A", scene.Trace[^1]);
        Assert.DoesNotContain("targets", scene.Trace);
        scene.Trace.Clear();
        scene.Field(0x38c, 0);
        scene.Field(0x294, 0);
        scene.Make().Act(scene.Token);
        Assert.Equal("stop:A", scene.Trace[^1]);
        scene.Trace.Clear();
        scene.Field(0x294, scene.Dais.Value);
        scene.Make().Act(scene.Token);
        Assert.Equal("targets", scene.Trace[^1]);
    }

    /// <summary>살아 있는 등록 간헐천의 비행/같은 그래프 경로는 유지하고 다른 그래프는 재분류한 뒤 정지한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void GeyserPathRetentionAndCancellation()
    {
        var scene = new AiCarrierScene();
        scene.Paths[scene.Actor.Value] = new Sid(100);
        scene.Targets = [scene.Geyser.Value, 0xffffffffu];
        scene.List(0x330, [scene.Geyser]);
        scene.Types[133].Flags2 = 0x20000;
        scene.Make().Act(scene.Token);
        Assert.Equal("targets", scene.Trace[^1]);
        scene.Trace.Clear();
        scene.Types[133].Flags2 = 0;
        scene.Make().Act(scene.Token);
        Assert.Equal("graph:G", scene.Trace[^2]);
        Assert.Equal("graph:A", scene.Trace[^1]);
        scene.Trace.Clear();
        scene.Graphs[scene.Geyser.Value] = 8;
        scene.Make().Act(scene.Token);
        Assert.Equal("reclassify", scene.Trace[^2]);
        Assert.Equal("stop:A", scene.Trace[^1]);
        scene.Trace.Clear();
        scene.Pool.RawSlot(scene.Geyser.Value)[11] = 2;
        scene.Hooks.Graph = null;
        scene.Make().Act(scene.Token);
        Assert.Equal(["max", "path:A", "targets", "reclassify", "stop:A"], scene.Trace);
    }

    /// <summary>재분류 뒤 정지 하위 누락은 앞선 효과를 보존하며 signed 음수 cursor는 쓴 뒤 물리 접근을 거부한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectedMissingEffectAndSignedCursor()
    {
        var scene = new AiCarrierScene();
        scene.Paths[scene.Actor.Value] = new Sid(100);
        scene.Targets = [scene.Other.Value, 0];
        scene.Hooks.Stop = null;
        Assert.ThrowsAny<Exception>(() => scene.Make().Act(scene.Token));
        Assert.Equal("reclassify", scene.Trace[^1]);
        var cursor = new AiCarrierScene();
        cursor.List(0x2dc, [cursor.Actor, cursor.Other]);
        cursor.Field(0x6d0, 0xfffffffeu);
        Assert.ThrowsAny<Exception>(() => cursor.Make().Act(cursor.Token));
        Assert.Equal(0xffffffffu, cursor.Field(0x6d0));
        Assert.Equal(["max"], cursor.Trace);
    }

    /// <summary>genus는 kind 전체 WORD의 TYPE로 먼저 계산하며 부분 비트/상위 mask/순번을 원본대로 처리한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ContainedGenusWordKindAndFilterOrder()
    {
        var scene = new AiCarrierScene();
        var state = new ContainedFinderState();
        Span<byte> parent = scene.Pool.RawSlot(scene.Actor.Value);
        Span<byte> first = scene.Pool.RawSlot(scene.Crystal.Value);
        Span<byte> second = scene.Pool.RawSlot(scene.Other.Value);
        BinaryPrimitives.WriteUInt16LittleEndian(parent.Slice(6, 2), scene.Crystal.Value);
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(4, 2), scene.Other.Value);
        first[10] = 6;
        second[10] = 6;
        first[11] = 3;
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(18, 2), 158);
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(20, 2), 0x120);
        BinaryPrimitives.WriteUInt16LittleEndian(second.Slice(18, 2), 158);
        BinaryPrimitives.WriteUInt16LittleEndian(second.Slice(20, 2), 0x20);
        scene.Types[158].Flags2 = 0x200001;
        Assert.Equal(scene.Crystal, AiContainedGenus.Find(scene.Pool, state, scene.Types, scene.Actor, 0x100, 0x200000, 0));
        Assert.Equal(scene.Other, AiContainedGenus.Find(scene.Pool, state, scene.Types, scene.Actor, 0x20, 0x200000, 1));
        Assert.Equal(new Sid(0), AiContainedGenus.Find(scene.Pool, state, scene.Types, scene.Actor, 0, 0x200003, 0));
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(18, 2), 0x19e);
        BinaryPrimitives.WriteUInt16LittleEndian(first.Slice(20, 2), 0);
        Assert.ThrowsAny<Exception>(() => AiContainedGenus.Find(scene.Pool, state, scene.Types, scene.Actor, 0x20, 0x200000, 0));
    }

    /// <summary>다이스는 free/dead도 좌표-3이며 진단 효과 뒤 현재 TYPE/좌표/공유 실패점을 재조회한다. 2026-11-25 R4-5b 운송 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DaisPointCurrentDiagnosticAndFallback()
    {
        var scene = new AiCarrierScene();
        Span<byte> raw = scene.Pool.RawSlot(scene.Dais.Value);
        Put(raw, 14, BitConverter.SingleToUInt32Bits(10.0f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(12.0f));
        raw[11] = 3;
        var invalid = new RawPathPoint(-10000, -10000);
        RawPathPoint point = AiContainedGenus.DaisApproachPoint(scene.Pool, scene.Types, new RawCheckingFlag(scene.Checking), invalid, null, scene.Dais);
        Assert.Equal(7f, point.X);
        Assert.Equal(9f, point.Y);
        scene.Checking = 1;
        scene.Types[165].Flags2 = 0;
        string? seen = null;
        Action<string> assertion = text =>
        {
            Assert.Equal("dais.isValid() && dais.getGenus() & gDAIS", text);
            scene.Types[165].Flags2 = 0x400000;
            Put(scene.Pool.RawSlot(scene.Dais.Value), 14, BitConverter.SingleToUInt32Bits(20.0f));
            seen = text;
        };
        point = AiContainedGenus.DaisApproachPoint(scene.Pool, scene.Types, new RawCheckingFlag(scene.Checking), invalid, assertion, scene.Dais);
        Assert.Equal(17f, point.X);
        Assert.Equal(9f, point.Y);
        Assert.NotNull(seen);
        Action<string> fallthrough = _ => invalid = new RawPathPoint(88, 99);
        point = AiContainedGenus.DaisApproachPoint(scene.Pool, scene.Types, new RawCheckingFlag(scene.Checking), invalid, fallthrough, new Sid(0));
        Assert.Equal(88f, point.X);
        Assert.Equal(99f, point.Y);
    }
}
