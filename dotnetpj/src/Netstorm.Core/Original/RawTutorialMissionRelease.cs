using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>Guide 수명과 공유하는 현재 미션 번호다.</summary>
public sealed class TutorialMissionReleaseGlobals
{
    /// <summary>현재 미션 번호(005ca8e0).</summary>
    public uint MissionNumber { get; set; }
}

/// <summary>튜토리얼 소멸 시 현재 전역, 실제 Player와 기본 Mission을 원본 순서로 반납한다.
/// 근거: cpppj/src/o/RawTutorialMissionRelease.cpp, docs/exe/cpp-guide-release-20261101.md.</summary>
public static class RawTutorialMissionRelease
{
    /// <summary>허용 표를 연 뒤 전역 여섯 값을 지우고 AI를 해제한다. 본문이 실패해도 기본 Mission을 반납한다.</summary>
    /// <param name="start">실제 회수·승급 제한 전역.</param>
    /// <param name="gate">현재 튜토리얼 번호와 단계 저장소.</param>
    /// <param name="globals">현재 미션 번호 저장소.</param>
    /// <param name="players">실제 Player 및 AI 수명 소유자.</param>
    /// <param name="setAllowAll">실제 기술 허용 표를 변경하는 효과.</param>
    /// <param name="releaseBaseMission">실제 기본 Mission 반납 효과.</param>
    public static void Release(MissionStartGlobals start, MissionFrameGateState gate, TutorialMissionReleaseGlobals globals,
        PlayerParticipation players, Action<uint>? setAllowAll, Action? releaseBaseMission)
    {
        ArgumentNullException.ThrowIfNull(start);
        ArgumentNullException.ThrowIfNull(gate);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(players);
        try
        {
            (setAllowAll ?? throw new InvalidOperationException("튜토리얼 반납의 실제 허용 표 미연결"))(1);
            start.DenySalvage = 0;
            start.DenyAscend = 0;
            globals.MissionNumber = 0;
            gate.TutorialNumber = 0;
            gate.Stage = 0;
            gate.StageDirty = 0;
            players.RemoveAllBrains();
        }
        finally
        {
            (releaseBaseMission ?? throw new InvalidOperationException("튜토리얼 반납의 기본 Mission 미연결"))();
        }
    }
}
