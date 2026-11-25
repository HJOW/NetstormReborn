// 디컴파일에서 누락된 점수·실제 조회 순서·첫 SID 출력의 경계를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 표적을 검사하는 클래스다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.
// 근거: cpppj tests/AiDropTargetsTests.cpp의 14건 전부다.
public sealed class RawAiDropTargetsTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>실제 본체/목록/슬롯에 명시 입력을 넣고 실제 하위의 호출 경계를 관찰한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    private sealed class AiDropScene
    {
        /// <summary>실제 AI 본체와 두 목록 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x50000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>첫 목록 backing이다.</summary>
        public readonly uint First;

        /// <summary>둘째 목록 backing이다.</summary>
        public readonly uint Second;

        /// <summary>실제 PATCH 슬롯과 현재 타입 표다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>현재 타입 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>입력 SID다.</summary>
        public readonly Sid Target = new(5);

        /// <summary>다른 입력 SID다.</summary>
        public readonly Sid Other = new(6);

        /// <summary>현재 제단 TYPE다.</summary>
        public uint Altar = 164;

        /// <summary>현재 방향 TYPE다.</summary>
        public uint Directional = 142;

        /// <summary>명시 하위 입력이다.</summary>
        public readonly float[] Rates = new float[188];

        /// <summary>현재 최대 HP다.</summary>
        public int Maximum = 1000;

        /// <summary>현재 사제 내용물이다.</summary>
        public Sid Captive;

        /// <summary>현재 검색 결과다.</summary>
        public Sid Found;

        /// <summary>현재 수집 목록이다.</summary>
        public List<uint> Candidates = [];

        /// <summary>현재 중심이다.</summary>
        public RawPathPoint Center = new(20, 29);

        /// <summary>선택된 실제 하위다.</summary>
        public AiDropTargetHooks Hooks = new();

        /// <summary>실제 호출 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>기본은 적 owner 2의 평범한 TYPE 80/81 표적과 비어 있는 원본 두 목록이다.</summary>
        public AiDropScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            First = Memory.Allocate(32);
            Second = Memory.Allocate(32);
            // 실제 할당 순서와 지정 슬롯 입력만 준비하는 반복문이다.
            for (ushort id = 5; id <= 9; id++)
            {
                Span<byte> raw = Pool.RawSlot(id);
                raw.Fill(0);
                raw[10] = (byte)(75 + id);
                raw[34] = 2;
                Put(raw, 26, 1000);
            }

            Field(0x36c, First);
            Field(0x378, Second);
            Field(0x260, 1);
            Field(0x264, 2);
            Hooks.Captive = actor => { Trace.Add("captive:" + actor.Value); return Captive; };
            Hooks.Rate = actor => { Trace.Add("rate:" + actor.Value); return Rates[Pool.Slot(actor.Value)[10]]; };
            Hooks.MaxHitPoints = actor => { Trace.Add("max:" + actor.Value); return Maximum; };
            Hooks.Gun = (x, y, type, direction, owner) =>
            {
                Assert.Equal(20f, x);
                Assert.Equal(30f, y);
                Assert.Equal(128u, type);
                Assert.Equal(2, direction);
                Assert.Equal(1u, owner);
                Trace.Add("gun");
                return Found;
            };
            Hooks.Blockers = (owner, point, capacity) =>
            {
                Assert.Equal(1u, owner);
                Assert.Equal(20f, point.X);
                Assert.Equal(30f, point.Y);
                Assert.Equal(30, capacity);
                Trace.Add("blockers");
                return Candidates;
            };
            Hooks.Center = actor => { Trace.Add("center:" + actor.Value); return Center; };
            Hooks.Assertion = (text, line) =>
            {
                Assert.Equal("sid", text);
                Assert.Equal(0xaebu, line);
                Trace.Add("assert");
            };
        }

        /// <summary>지정 본체 DWORD만 입력한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>지정 목록의 실제 backing에 중복을 그대로 입력한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
        /// <param name="pointer">목록 기록 위치다.</param>
        /// <param name="values">쓸 SID 목록이다.</param>
        public void List(int pointer, Sid[] values)
        {
            Field(pointer + 8, (uint)values.Length);
            byte[] raw = Memory.Bytes(Get(Memory.Bytes(Token), pointer));
            int at = 0;
            // 입력 등장 순서를 보존하는 반복문이다.
            foreach (Sid value in values)
            {
                Put(raw, at, value.Value);
                at += 4;
            }
        }

        /// <summary>같은 실제 저장소에 처리기를 연결하며 호출 자체는 각 검사에서 선택한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
        /// <returns>검사할 배치 표적 몸체다.</returns>
        public RawAiDropTargets Actions() => new(Memory, Pool, Types, new AiDropTargetGlobals
        {
            AltarType = () => Altar,
            DirectionalType = () => Directional,
        }, Hooks);

        /// <summary>현재 TYPE/호출 하위의 점수를 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
        /// <returns>표적 점수다.</returns>
        public int Score()
        {
            var actions = Actions();
            Assert.Same(Pool, actions.Pool);
            return actions.Score(Token, Target);
        }
    }

    /// <summary>원본 정수 기본/사제/건물/group 6/제단 가산을 각각 검사한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void NativeBasePriestBuildingGroupAndAltarBonuses()
    {
        var scene = new AiDropScene();
        Assert.Equal(10, scene.Score());
        scene.Types[80].Flags2 = 0x4000000;
        Assert.Equal(5, scene.Score());
        scene.Types[80].Flags2 = 0x200000;
        Assert.Equal(210, scene.Score());
        scene.Field(0x2a8, 9);
        Assert.Equal(15, scene.Score());
        scene.Types[80].Flags2 = 0x200;
        Assert.Equal(16, scene.Score());
        scene.Types[80].Flags2 = 0;
        scene.Types[80].Group = 6;
        Assert.Equal(17, scene.Score());
        scene.Types[80].Group = 0;
        scene.Altar = 80;
        Assert.Equal(18, scene.Score());
    }

    /// <summary>정수 가산과 현재 hpPerSec를 합한 다음 ftol하며 NaN의 하위 DWORD 0을 보존한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void RateIsAddedBeforeTruncationAndNonfinite()
    {
        var scene = new AiDropScene();
        scene.Rates[80] = -0.9f;
        Assert.Equal(9, scene.Score());
        scene.Rates[80] = float.NaN;
        Assert.Equal(0, scene.Score());
        scene.Rates[80] = 1.75f;
        Assert.Equal(11, scene.Score());
    }

    /// <summary>포획 사제 400점은 운반 genus 30000에서만 조회하며 앞선 flags/group은 보존한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void CaptiveBonusAndCurrentRateAfterLookup()
    {
        var scene = new AiDropScene();
        scene.Types[80].Flags2 = 0x10000;
        scene.Types[80].Group = 6;
        scene.Rates[81] = 23;
        scene.Hooks.Captive = actor =>
        {
            Assert.Equal(scene.Target, actor);
            scene.Trace.Add("captive");
            scene.Pool.RawSlot(actor.Value)[10] = 81;
            return scene.Other;
        };
        Assert.Equal(440, scene.Score());
        Assert.Equal(["captive", "rate:5"], scene.Trace);
        scene.Trace.Clear();
        Assert.Equal(33, scene.Score());
        Assert.Equal(["rate:5"], scene.Trace);
    }

    /// <summary>모든 실제 가산을 한 번씩 더하고 제단 TYPE는 현재 전역을 사용한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void AllNativeBonusesAreCombined()
    {
        var scene = new AiDropScene();
        scene.Types[80].Flags2 = 0x4000000 | 0x200000 | 0x200 | 0x10000;
        scene.Types[80].Group = 6;
        scene.Altar = 80;
        scene.Captive = scene.Other;
        scene.Rates[80] = 1.75f;
        Assert.Equal(627, scene.Score());
    }

    /// <summary>두 목록은 각 첫 일치에만 100을 더하며 하위 효과 뒤 현재 목록을 읽는다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void ListBonusesDuplicateAndCurrentBacking()
    {
        var scene = new AiDropScene();
        scene.List(0x36c, [scene.Target, scene.Target]);
        scene.List(0x378, [scene.Target, scene.Target]);
        Assert.Equal(210, scene.Score());
        scene.Hooks.Rate = actor =>
        {
            Assert.Equal(scene.Target, actor);
            scene.Field(0x374, 0);
            return 0.0f;
        };
        Assert.Equal(110, scene.Score());
        scene.Field(0x380, 0xffffffffu);
        Assert.Equal(10, scene.Score());
    }

    /// <summary>집중 대상의 HP는 getter 전 저장하고 signed 최대 HP/2는 0쪽 나눗셈이다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void FocusSavesHpAndUsesSignedHalf()
    {
        var scene = new AiDropScene();
        scene.Field(0x3f8, scene.Target.Value);
        Put(scene.Pool.RawSlot(scene.Target.Value), 26, 0xffffffffu);
        scene.Hooks.MaxHitPoints = actor =>
        {
            Assert.Equal(scene.Target, actor);
            scene.Trace.Add("max");
            Put(scene.Pool.RawSlot(actor.Value), 26, 0xfffffff6u);
            return -3;
        };
        Assert.Equal(60, scene.Score());
        Assert.Equal(["rate:5", "max"], scene.Trace);
    }

    /// <summary>마지막 owner BYTE는 전체 enemy DWORD와 비교하고 음수 점수도 0쪽으로 절반을 낸다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void EnemyOwnerFullDwordAndCurrentGetterEffects()
    {
        var scene = new AiDropScene();
        scene.Rates[80] = -21;
        Assert.Equal(-11, scene.Score());
        scene.Field(0x264, 0x102);
        Assert.Equal(-5, scene.Score());
        scene.Field(0x264, 2);
        scene.Field(0x3f8, scene.Target.Value);
        scene.Hooks.MaxHitPoints = actor =>
        {
            scene.Pool.RawSlot(actor.Value)[34] = 3;
            scene.Field(0x264, 3);
            return 99999;
        };
        Assert.Equal(-11, scene.Score());
    }

    /// <summary>32비트 가산은 signed overflow 대신 원본 하위 DWORD 감김을 유지한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void ListAdditionWrapsDword()
    {
        var scene = new AiDropScene();
        scene.Rates[80] = 2147483520.0f;
        scene.List(0x36c, [scene.Target]);
        scene.List(0x378, [scene.Target]);
        Assert.Equal(-2147483566, scene.Score());
    }

    /// <summary>물리 SID 0의 운반체 진단은 선택된 곳에서 호출하고 실제 내용물 조회를 계속한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void PhysicalZeroCarrierAssertionContinues()
    {
        var scene = new AiDropScene();
        byte zeroType = scene.Pool.Slot(new Sid(0).Value)[10];
        scene.Types[zeroType].Flags2 = 0x10000;
        scene.Field(0x3f8, 9);
        scene.Actions().Score(scene.Token, new Sid(0));
        Assert.Equal(["assert", "captive:0", "rate:0"], scene.Trace);
    }

    /// <summary>실제 검색 결과는 점수 계산보다 먼저 쓰고 빈 검색도 0으로 덮어쓴다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void GunWritesResultBeforeScoreAndZero()
    {
        var scene = new AiDropScene();
        Sid result = new(9);
        var actions = scene.Actions();
        Assert.Equal(0, actions.Gun(scene.Token, 128, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(new Sid(0), result);
        scene.Found = scene.Target;
        scene.Hooks.Rate = actor =>
        {
            Assert.Equal(scene.Target, actor);
            Assert.Equal(scene.Target, result);
            return 5.0f;
        };
        Assert.Equal(15, scene.Actions().Gun(scene.Token, 128, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(scene.Target, result);
        scene.Hooks.Rate = null;
        Assert.ThrowsAny<Exception>(() => scene.Actions().Gun(scene.Token, 128, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(scene.Target, result);
    }

    /// <summary>최고 점수의 항목이 둘째여도 원본은 첫 SID를 출력하며 중복 총점은 모두 센다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void BlockersSumAndNativeFirstEntryOutput()
    {
        var scene = new AiDropScene();
        scene.Rates[80] = -10;
        scene.Rates[81] = 20;
        scene.Candidates = [5, 6, 6];
        Sid result = new(9);
        Assert.Equal(60, scene.Actions().Blockers(scene.Token, 78, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(scene.Target, result);
        Assert.Equal(["blockers", "rate:5", "rate:6", "rate:6"], scene.Trace);
    }

    /// <summary>양의 최고가 없으면 출력은 보존하고 음수/0도 총점에 포함한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void BlockersNonpositivePreservesOutputAndWrapsTotal()
    {
        var scene = new AiDropScene();
        scene.Rates[80] = -11;
        scene.Candidates = [5, 5];
        Sid result = new(9);
        Assert.Equal(-2, scene.Actions().Blockers(scene.Token, 78, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(new Sid(9), result);
        scene.Rates[80] = 2147483520.0f;
        Assert.Equal(-236, scene.Actions().Blockers(scene.Token, 78, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(scene.Target, result);
        scene.Candidates.Clear();
        result = new Sid(9);
        Assert.Equal(0, scene.Actions().Blockers(scene.Token, 78, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(new Sid(9), result);
    }

    /// <summary>방향 블로커 필터는 점수 뒤 두 축 -1을 저장하고 현재 전역 TYPE에만 적용한다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void DirectionalFilterAfterScoringAndCurrentGlobal()
    {
        var scene = new AiDropScene();
        scene.Candidates = [5];
        scene.Center = new RawPathPoint(19, 29.5f);
        Sid result = new(9);
        Assert.Equal(10, scene.Actions().Blockers(scene.Token, 142, new RawPathPoint(20, 30), 4, ref result));
        Assert.Equal(scene.Target, result);
        Assert.Equal(["blockers", "rate:5", "center:5"], scene.Trace);
        scene.Trace.Clear();
        result = new Sid(9);
        Assert.Equal(0, scene.Actions().Blockers(scene.Token, 142, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(new Sid(9), result);
        scene.Types[80].Flags2 = 0x100000;
        scene.Trace.Clear();
        Assert.Equal(0, scene.Actions().Blockers(scene.Token, 142, new RawPathPoint(20, 30), 0, ref result));
        Assert.Equal(["blockers", "rate:5"], scene.Trace);
        scene.Types[80].Flags2 = 0;
        scene.Directional = 78;
        scene.Trace.Clear();
        Assert.Equal(10, scene.Actions().Blockers(scene.Token, 142, new RawPathPoint(20, 30), 2, ref result));
        Assert.Equal(["blockers", "rate:5"], scene.Trace);
    }

    /// <summary>홀수 방향도 원문대로 중심을 먼저 읽고 제외하며 대각 동률은 세로다. 2026-11-25 R4-5b 배치 표적 이식에서 추가됐다.</summary>
    [Fact]
    public void FacingOddDirectionAndVerticalTie()
    {
        var scene = new AiDropScene();
        scene.Center = new RawPathPoint(21, 29);
        Assert.True(scene.Actions().Facing(new RawPathPoint(20, 30), 0, scene.Target));
        scene.Trace.Clear();
        Assert.False(scene.Actions().Facing(new RawPathPoint(20, 30), 1, scene.Target));
        Assert.Equal(["center:5"], scene.Trace);
    }
}
