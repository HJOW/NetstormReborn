using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 미션 시작 때 플레이어 참여와 방향별 동맹을 원본(004c2b20 → 004c2d40 → 00417e50 → 00416860/004911a0) 순서로 구성한다.
/// 로컬 사람은 로그인하고, 로컬이 아닌 소유자 가운데 실제 사제가 있는 소유자마다 AI 를 만들어 자기 방향의 동맹 행을 설정한다.
/// </summary>
/// <remarks>
/// 원본 규칙(docs/exe/cpp-player-participation-20261015.md, cpp-mission-start-20261015.md):
/// AI 생성 조건은 <c>owner != local || demo</c> 이며 실제 사제가 존재해야 한다(aiName 의 유무는 조건이 아니다).
/// AI 의 AllyList 마스크를 <see cref="PlayerParticipation.SetAlliance"/> 로 그 AI 의 행에만 쓰고, <c>allies[AI][local]</c> 이 참일 때만
/// 로컬의 반대 방향(local → AI)을 더한다. 모든 AI 사이의 관계를 대칭화하지 않고 사람 쪽 목록(myAllyList)은 읽지 않는다.
/// 2026-10-15 감사 D03 에서 추가.
/// </remarks>
public static class MissionParticipation
{
    /// <summary>임시 AI 의 불투명 brain 토큰 기준값. 번호를 더해 0이 아닌 값으로 만든다.</summary>
    /// <remarks>
    /// 클론의 AI 는 원본 생성자(00416860)가 만드는 brain 객체가 없다(전략 미복원). 이 토큰은 "AI 가 부착됐다"는 참여 비트(0x20)만
    /// 얻기 위한 표식이며 원본 AI 생성·전략이 복원됐다는 뜻이 아니다.
    /// </remarks>
    public const uint TemporaryBrainTokenBase = 0xA1000000;

    /// <summary>
    /// 미션 시작 조건과 사제가 있는 소유자 목록으로 참여·동맹 상태를 만든다.
    /// 미션이 없으면(맵만 연 시험 모드) 자기 동맹과 사람 로그인만 있고 AI 는 없다.
    /// </summary>
    /// <param name="mission">미션 시작 조건(없으면 null).</param>
    /// <param name="humanPlayer">로컬 사람 플레이어 번호.</param>
    /// <param name="priestOwners">시작 월드에 실제 사제가 있는 소유자 번호들.</param>
    /// <returns>초기화된 참여 상태.</returns>
    public static PlayerParticipation Create(MissionStart? mission, int humanPlayer, IEnumerable<int> priestOwners)
    {
        ArgumentNullException.ThrowIfNull(priestOwners);
        bool humanValid = humanPlayer is > 0 and < PlayerParticipation.PlayerCount;
        var state = new PlayerParticipationState
        {
            LocalOwner = humanValid ? (uint)humanPlayer : 0,
            // 전체 초기화 동안은 Chat 재구성을 하지 않고, 끝난 뒤 Player 번호를 알게 된 상태로 둔다.
            PlayerKnown = 0,
            LocalTransport = true,
        };
        var hooks = new PlayerParticipationHooks
        {
            // 진단만 남기고 계속 진행하는 assertion. 단일 플레이 초기화에서는 도달하지 않는다.
            Assertion = _ => { },
            // 채팅 UI 가 없어 채널 등록의 소비자가 없다. 동맹 표 자체는 위 SetAlliance 가 이미 썼다.
            ClearChat = _ => { },
            AddChat = (_, _, _) => { },
        };
        var participation = new PlayerParticipation(
            new byte[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerStride],
            new uint[PlayerParticipation.PlayerCount * PlayerParticipation.PlayerCount],
            new byte[PlayerParticipation.PlayerCount], state, hooks);
        // Player 초기화(004910a0): 모든 Player(중립 0 포함)가 자기 자신을 동맹으로 둔다.
        for (uint owner = 0; owner < PlayerParticipation.PlayerCount; owner++) participation.SetAlliance(owner, 0);
        state.PlayerKnown = 1;
        if (humanValid) participation.CheckLogin(humanPlayer, humanPlayer);
        if (mission == null) return participation;
        var priests = priestOwners.ToHashSet();
        // 원본은 owner 1~8 을 차례로 방문하며 로컬이 아니고 실제 사제가 있는 소유자마다 AI 를 만든다.
        for (int owner = 1; owner < PlayerParticipation.PlayerCount; owner++)
        {
            if (owner == humanPlayer || !priests.Contains(owner)) continue;
            // AI 생성자(00416860): 선언된 AllyList 를 이 AI 의 행에만 쓴다.
            participation.SetAlliance((uint)owner, mission.AllyMask(owner));
            // AI 가 로컬을 동맹으로 선언한 경우에만 로컬의 반대 방향을 더한다. 다른 AI 사이는 대칭화하지 않는다.
            if (humanValid && participation.Allies(owner, humanPlayer)) participation.AddAlly((uint)humanPlayer, (uint)owner);
            // 부착(004911a0): 참여 비트 0x20 을 켠다.
            participation.AttachBrain((uint)owner, TemporaryBrainTokenBase + (uint)owner);
        }
        return participation;
    }
}
