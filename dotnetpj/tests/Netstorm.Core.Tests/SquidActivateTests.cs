using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 물리 슬롯·참조 깊이에서 원문 진단 복귀와 부분 실패를 관측한다.
namespace Netstorm.Core.Tests;

/// <summary>cpp SquidActivateTests의 비멀티 부모 계약과 실제 공통 장부의 동일 깊이를 대조한다.</summary>
public sealed class SquidActivateTests
{
    /// <summary>같은 raw·깊이와 현재 대리자 교체를 소유하는 독립 입력이다.</summary>
    private sealed class Scene
    {
        /// <summary>원본 PATCH 물리 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>현재 진단 대상 SID다.</summary>
        public int Source = 50;
        /// <summary>부모와 후처리가 참조로 공유하는 DWORD다.</summary>
        public uint Depth = 2;
        /// <summary>후처리 호출 횟수다.</summary>
        public int Posts;
        /// <summary>후처리가 받은 실제 flags다.</summary>
        public uint Flags;
        /// <summary>진단의 실제 선택 순서다.</summary>
        public readonly List<uint> Lines = [];
        /// <summary>진단에서 요청하는 현재 물리 변경이다.</summary>
        public Action<uint>? OnAssertion;
        /// <summary>후처리에서 요청하는 현재 변경·명시 실패다.</summary>
        public Action? OnPost;
        /// <summary>부모가 매 시점 다시 선택하는 현재 효과다.</summary>
        public readonly SquidActivateHooks Hooks;

        /// <summary>client SID까지 실제로 확보하고 비멀티 권한·관측 경계를 설치한다.</summary>
        public Scene()
        {
            // 원본 입력 대상까지 차례로 확보하고 기본 void 상태를 유지한다.
            for (int sid = 5; sid <= Source; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Hooks = new SquidActivateHooks
            {
                Server = () => false,
                Transmit = (_, _) => throw new InvalidOperationException("비멀티 입력에서 미도달 송신"),
                PostPop = (sid, flags) => { Assert.Equal(Source, sid); Posts++; Flags = flags; OnPost?.Invoke(); Depth = unchecked(Depth - 1); },
                Assertion = (_, file, line) => { Assert.Equal(@"\Ns\O\Squid.cpp", file); Lines.Add(line); OnAssertion?.Invoke(line); },
            };
        }
        /// <summary>free·예약 번호도 원문처럼 같은 물리 슬롯을 읽고 쓴다.</summary>
        public Span<byte> Raw() => Pool.RawSlot(Source);
        /// <summary>원문 부모에 사본 없는 같은 깊이를 전달한다.</summary>
        public void Run(uint flags = 0) => RawSquidActivate.Run(Pool, Source, flags, ref Depth, Hooks);
    }

    /// <summary>35c 진단이 바꾼 현재 BYTE로 void를 끄고 저장한 전송중 비트를 검사한다.</summary>
    [Fact]
    public void VoidRepairAndTransmittingDiagnosticResumeCurrentState()
    {
        var scene = new Scene(); scene.Raw()[11] = 0;
        scene.OnAssertion = line =>
        {
            if (line == 0x35c) scene.Raw()[11] = 0x14;
            else { Assert.Equal(0x35fu, line); Assert.Equal(0x10, scene.Raw()[11]); scene.Raw()[11] = 0; }
        };
        scene.Run(); Assert.Equal(new uint[] { 0x35c, 0x35f }, scene.Lines); Assert.Equal(0, scene.Raw()[11]);
        Assert.Equal(2u, scene.Depth); Assert.Equal(1, scene.Posts); Assert.Equal(0x10u, scene.Flags);
    }

    /// <summary>SID0 진단이 정상 복귀하면 같은 물리0번을 사용하며 범위 밖은 진단 뒤 실패한다.</summary>
    [Fact]
    public void InvalidSidDiagnosticsPrecedePhysicalAccess()
    {
        var zero = new Scene { Source = 0 }; zero.OnAssertion = line => { Assert.Equal(0x35bu, line); zero.Raw()[11] = 4; };
        zero.Run(); Assert.Equal(new uint[] { 0x35b }, zero.Lines); Assert.Equal(0, zero.Raw()[11]); Assert.Equal(1, zero.Posts); Assert.Equal(2u, zero.Depth);
        var outside = new Scene { Source = 24000 }; Assert.Throws<ArgumentOutOfRangeException>(() => outside.Run());
        Assert.Equal(new uint[] { 0x35b }, outside.Lines); Assert.Equal(0, outside.Posts); Assert.Equal(2u, outside.Depth);
    }

    /// <summary>후처리 뒤 현재 깊이를 진단하고 정상 복귀·중단 모두 외부 변경만 남긴다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void CurrentDepthDiagnosticPreservesCompletedEffects(bool abort)
    {
        var scene = new Scene(); scene.OnPost = () => { Assert.Equal(3u, scene.Depth); scene.Depth = 9; scene.Raw()[40] = 0xa5; };
        scene.OnAssertion = line =>
        {
            Assert.Equal(0x392u, line); Assert.Equal(8u, scene.Depth); Assert.Equal(1, scene.Posts);
            if (abort) throw new InvalidOperationException("명시 후단 진단 중단");
            scene.Depth = 7;
        };
        if (abort) Assert.Throws<InvalidOperationException>(() => scene.Run(8)); else scene.Run(8);
        Assert.Equal(new uint[] { 0x392 }, scene.Lines); Assert.Equal(abort ? 8u : 7u, scene.Depth);
        Assert.Equal(8u, scene.Flags); Assert.Equal(0, scene.Raw()[11]); Assert.Equal(0xa5, scene.Raw()[40]);
    }

    /// <summary>균형 잡힌 DWORD 감김은 진단을 요구하지 않고 하위 실패는 증가한 깊이를 보존한다.</summary>
    [Fact]
    public void BalancedWrapAndPostFailureDoNotInventFinalDiagnostics()
    {
        var balanced = new Scene { Depth = uint.MaxValue }; balanced.Hooks.Assertion = null;
        balanced.OnPost = () => Assert.Equal(0u, balanced.Depth); balanced.Run(); Assert.Equal(uint.MaxValue, balanced.Depth); Assert.Equal(1, balanced.Posts);
        var failed = new Scene(); failed.OnPost = () => throw new InvalidOperationException("명시 가상몸체 실패");
        Assert.Throws<InvalidOperationException>(() => failed.Run()); Assert.Equal(3u, failed.Depth); Assert.Equal(1, failed.Posts); Assert.Empty(failed.Lines); Assert.Equal(0, failed.Raw()[11]);
    }

    /// <summary>자기 연결을 바꾼 진단 다음에는 새 장치를 읽으며 현재 서버 장치 실패 앞의 void 해제도 보존한다.</summary>
    [Fact]
    public void SelfRebindingAndReachedDeviceFailureUseCurrentHooks()
    {
        var scene = new Scene(); scene.Raw()[11] = 0; var calls = new List<uint>();
        scene.Hooks.Assertion = (expression, _, line) =>
        {
            Assert.Equal("isVoid()", expression); Assert.Equal(0x35cu, line); calls.Add(line); scene.Raw()[11] = 0x14;
            scene.Hooks.Assertion = (expression, _, line) => { Assert.Equal("!xmitJustCreatedFlag", expression); Assert.Equal(0x35fu, line); Assert.Equal(0x10, scene.Raw()[11]); calls.Add(line); scene.Raw()[11] = 0; };
        };
        scene.Run(); Assert.Equal(new uint[] { 0x35c, 0x35f }, calls); Assert.Equal(2u, scene.Depth);
        var missing = new Scene(); missing.Hooks.Server = null;
        Assert.Throws<InvalidOperationException>(() => missing.Run()); Assert.Equal(0, missing.Raw()[11]); Assert.Equal(2u, missing.Depth); Assert.Equal(0, missing.Posts);
    }

    /// <summary>실제 공통 PostPop의 통계와 깊이가 같은 참조이고 후처리 뒤 변경도 부모 진단에 보인다.</summary>
    [Fact]
    public void ActualCommonPostPopSharesDepthAndStatistics()
    {
        var scene = new Scene(); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); types[74].Cost = 3;
        Span<byte> raw = scene.Raw(); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x501dd0); raw[10] = 74; raw[11] = 4; raw[34] = 1;
        var state = new RawPostPopState { Depth = 2, LocalOwner = 1 }; var post = new RawPostPop(scene.Pool, types, state); int diagnoses = 0;
        scene.Hooks.PostPop = (sid, flags) => { Assert.Equal(3u, state.Depth); post.PostPopVirtual(sid, flags); Assert.Equal(2u, state.Depth); state.Depth = 9; };
        scene.Hooks.Assertion = (expression, _, line) =>
        {
            Assert.Equal("curPostPop == basePostPop", expression); Assert.Equal(0x392u, line); Assert.Equal(9u, state.Depth);
            Assert.Equal(3, state.TotalCost); Assert.Equal(1u, state.GlobalCounts[74]); Assert.Equal(1u, state.LocalCounts[74]); state.Depth = 2; diagnoses++;
        };
        RawSquidActivate.Run(scene.Pool, scene.Source, 1, ref state.ActivationDepth, scene.Hooks);
        Assert.Equal(1, diagnoses); Assert.Equal(2u, state.Depth); Assert.Equal(0, scene.Raw()[11]);
    }

    /// <summary>CD 판본은 동일 raw를 바꾸거나 현재 효과를 조회하기 전에 거부한다.</summary>
    [Fact]
    public void EditionGuardPrecedesEffects()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072); uint depth = 7;
        Assert.Throws<ArgumentException>(() => RawSquidActivate.Run(pool, 50, 0, ref depth, new SquidActivateHooks())); Assert.Equal(7u, depth);
    }
}
