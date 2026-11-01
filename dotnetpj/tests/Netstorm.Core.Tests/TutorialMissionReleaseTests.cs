using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>실제 Player 저장소와 튜토리얼 전역을 사용해 소멸 순서와 실패 시 정리를 검사한다.</summary>
public sealed class TutorialMissionReleaseTests
{
    /// <summary>Player brain 포인터를 원본 위치에 저장한다.</summary>
    /// <param name="players">실제 아홉 Player 바이트.</param><param name="owner">Player 번호.</param><param name="brain">원본 토큰.</param>
    private static void SetBrain(byte[] players, int owner, uint brain) =>
        BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(owner * PlayerParticipation.PlayerStride + PlayerParticipation.BrainOffset), brain);

    /// <summary>허용 표 갱신 뒤 여섯 필드를 지우고 각 현재 AI를 순서대로 반납한 뒤 기본 미션을 닫는다.</summary>
    [Fact]
    public void Release_UsesSharedGlobalsAndPlayerLifetime()
    {
        var start = new MissionStartGlobals { DenySalvage = 3, DenyAscend = 4 };
        var gate = new MissionFrameGateState { TutorialNumber = 5, Stage = 66, StageDirty = 7 };
        var globals = new TutorialMissionReleaseGlobals { MissionNumber = 8 };
        var bytes = new byte[9 * PlayerParticipation.PlayerStride];
        SetBrain(bytes, 1, 11); SetBrain(bytes, 3, 33);
        var trace = new List<string>();
        var players = new PlayerParticipation(bytes, new uint[81], new byte[9], new(), new()
        {
            DestructBrain = brain => { Assert.Equal(0, gate.Stage); Assert.Equal(0U, globals.MissionNumber); trace.Add("destroy:" + brain); },
            FreeBrain = brain => trace.Add("free:" + brain)
        });
        RawTutorialMissionRelease.Release(start, gate, globals, players,
            allow => { Assert.Equal(1U, allow); Assert.Equal(3, start.DenySalvage); trace.Add("allow"); },
            () => { Assert.Equal(0U, players.Brain(3)); trace.Add("base"); });
        Assert.Equal(new[] { "allow", "destroy:11", "free:11", "destroy:33", "free:33", "base" }, trace);
        Assert.Equal(0, start.DenySalvage); Assert.Equal(0, start.DenyAscend);
        Assert.Equal(0U, gate.TutorialNumber); Assert.Equal(0U, gate.StageDirty);
    }

    /// <summary>AI 반납 실패는 앞선 변경을 보존하고 기본 Mission을 정확히 한 번 반납한다.</summary>
    [Fact]
    public void Release_CleansBaseAfterBrainFailure()
    {
        var start = new MissionStartGlobals { DenySalvage = 3 };
        var gate = new MissionFrameGateState { Stage = 70 };
        var globals = new TutorialMissionReleaseGlobals { MissionNumber = 9 };
        var bytes = new byte[9 * PlayerParticipation.PlayerStride];
        SetBrain(bytes, 1, 123);
        var players = new PlayerParticipation(bytes, new uint[81], new byte[9], new(), new()
        {
            DestructBrain = _ => throw new InvalidOperationException("AI 실패")
        });
        int released = 0;
        var error = Assert.Throws<InvalidOperationException>(() => RawTutorialMissionRelease.Release(start, gate, globals, players, _ => { }, () => released++));
        Assert.Equal("AI 실패", error.Message); Assert.Equal(1, released);
        Assert.Equal(0, start.DenySalvage); Assert.Equal(0, gate.Stage);
        Assert.Equal(0U, globals.MissionNumber); Assert.Equal(123U, players.Brain(1));
    }
    /// <summary>허용 표 변경에서 실패하면 직접 전역 쓰기와 AI 해제를 하지 않고 기본 Mission만 반납한다.</summary>
    [Fact]
    public void Release_PreservesGlobalsWhenAllowTableFails()
    {
        var start = new MissionStartGlobals { DenyAscend = 4 };
        var gate = new MissionFrameGateState { Stage = 66 };
        var globals = new TutorialMissionReleaseGlobals { MissionNumber = 7 };
        var bytes = new byte[9 * PlayerParticipation.PlayerStride];
        SetBrain(bytes, 1, 77);
        var players = new PlayerParticipation(bytes, new uint[81], new byte[9], new(), new());
        int released = 0;
        Assert.Throws<InvalidOperationException>(() => RawTutorialMissionRelease.Release(start, gate, globals, players,
            _ => throw new InvalidOperationException("허용 표 실패"), () => released++));
        Assert.Equal(1, released); Assert.Equal(4, start.DenyAscend); Assert.Equal(66, gate.Stage);
        Assert.Equal(7U, globals.MissionNumber); Assert.Equal(77U, players.Brain(1));
    }

}
