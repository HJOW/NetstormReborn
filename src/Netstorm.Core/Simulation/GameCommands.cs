namespace Netstorm.Core.Simulation;

/// <summary>
/// 플레이어가 게임 세션에 내리는 명령. 게임 상태는 이 명령으로만 바뀐다 (멀티플레이 락스텝·리플레이는 명령만 주고받으면 되도록).
/// 명령은 <see cref="BattleSession.Submit"/> 으로 넣고, 다음 틱 처음에 넣은 순서대로 실행된다.
/// </summary>
/// <param name="Player">명령을 내린 플레이어 번호</param>
public abstract record GameCommand(int Player);

/// <summary>생산 창(덱)의 유닛을 기준점(오른쪽 아래 칸)에 놓는다. 비용·에너지·자리 조건은 BattleMap 규칙을 따른다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TypeName">유닛 타입 이름</param>
/// <param name="X">기준점 칸 x</param>
/// <param name="Y">기준점 칸 y</param>
public sealed record PlaceUnitCommand(int Player, string TypeName, int X, int Y) : GameCommand(Player);

/// <summary>사제의 Construct 로 건물(템플·워크샵·알타·아웃포스트)을 짓기 시작한다. 건설이 끝나야 규칙 효과가 생긴다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TypeName">건물 타입 이름</param>
/// <param name="X">기준점 칸 x</param>
/// <param name="Y">기준점 칸 y</param>
public sealed record ConstructBuildingCommand(int Player, string TypeName, int X, int Y) : GameCommand(Player);

/// <summary>지식(유닛)을 워크샵에 등록해 생산 창에 올린다 ("Put Knowledge into Production").</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="WorkshopId">워크샵 오브젝트 번호</param>
/// <param name="TypeName">등록할 유닛 타입 이름</param>
public sealed record RegisterKnowledgeCommand(int Player, int WorkshopId, string TypeName) : GameCommand(Player);

/// <summary>자기 오브젝트를 회수한다 ("Salvage"). 비용의 25% 를 돌려받는다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="EntityId">회수할 오브젝트 번호</param>
public sealed record SalvageCommand(int Player, int EntityId) : GameCommand(Player);

/// <summary>생산 창 다리 칸의 조각을 커서로 집는다 (칸에서 빠지므로 빈 칸에 새 조각이 채워질 수 있다).</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TrayIndex">칸 자리 번호 (BridgeTray.Slots 기준, 2열 행 우선)</param>
public sealed record PickBridgePieceCommand(int Player, int TrayIndex) : GameCommand(Player);

/// <summary>집고 있던 다리 조각을 칸으로 되돌린다.</summary>
/// <param name="Player">플레이어 번호</param>
public sealed record ReturnBridgePieceCommand(int Player) : GameCommand(Player);

/// <summary>집고 있는 다리 조각을 돌린 상태로 왼쪽 위 칸 기준으로 놓는다. 회전은 커서(화면)가 관리하고 놓을 때 값만 전달한다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="Rotation">놓을 회전 번호 (0~3, 1 = 시계 방향 90°)</param>
/// <param name="X">조각 왼쪽 위 칸 x</param>
/// <param name="Y">조각 왼쪽 위 칸 y</param>
public sealed record PlaceBridgeCommand(int Player, int Rotation, int X, int Y) : GameCommand(Player);

/// <summary>
/// 오브젝트를 선택하거나(번호) 선택을 푼다(0). 선택은 화면 조작이지만 튜토리얼 단계 처리가 읽는 규칙 상태이므로 명령으로 둔다
/// (원본은 단계 C·F 에서 선택한 오브젝트가 템플인지 확인한다).
/// </summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="EntityId">선택할 오브젝트 번호 (0 이면 선택 해제)</param>
public sealed record SelectEntityCommand(int Player, int EntityId) : GameCommand(Player);

/// <summary>내 사제 또는 수송 유닛에게 지정 가이저에서 결정을 반복 수확해 신전으로 가져오게 한다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="GeyserId">수확할 가이저 오브젝트 번호</param>
/// <param name="CollectorId">수집자 번호. 0이면 기존 사제 자동 선택을 유지한다.</param>
public sealed record HarvestGeyserCommand(int Player, int GeyserId, int CollectorId = 0) : GameCommand(Player);

/// <summary>내 사제·수송 유닛을 지정한 칸으로 이동한다. 진행 중인 수확을 취소한다.</summary>
public sealed record MoveEntityCommand(int Player, int EntityId, int X, int Y) : GameCommand(Player);

/// <summary>내 사제·수송 유닛의 이동·수확을 중지한다. 운반 중인 결정·사제는 유지한다.</summary>
public sealed record StopEntityCommand(int Player, int EntityId) : GameCommand(Player);

/// <summary>완공된 내 워크샵을 1,000 SP로 한 단계 올려 생산 칸을 늘린다.</summary>
public sealed record UpgradeWorkshopCommand(int Player, int WorkshopId) : GameCommand(Player);

/// <summary>튜토리얼 1에서 F4로 자기 섬 화면에 복귀했음을 규칙 세션에 알린다.</summary>
public sealed record ReturnHomeCommand(int Player) : GameCommand(Player);

/// <summary>
/// 수송 유닛을 기절한 적 사제에게 보내 집게 한다 (도움말 "Bring the Enemy Priest to your Altar").
/// 제단 번호를 주면 집은 뒤 곧바로 그 제단으로 운반해 희생의 원에 묶는다.
/// </summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TransportId">내 수송 유닛(골렘·게·풍선 등) 번호</param>
/// <param name="PriestId">집을 사제 번호</param>
/// <param name="AltarId">이어서 운반할 내 제단 번호 (0 이면 집은 채로 대기)</param>
public sealed record CapturePriestCommand(int Player, int TransportId, int PriestId, int AltarId = 0) : GameCommand(Player);

/// <summary>사제를 운반 중인 수송 유닛을 내 제단으로 보내 사제를 희생의 원에 묶는다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TransportId">사제를 운반 중인 수송 유닛 번호</param>
/// <param name="AltarId">내 제단 번호</param>
public sealed record DeliverPriestCommand(int Player, int TransportId, int AltarId) : GameCommand(Player);

/// <summary>
/// 사제를 운반 중인 수송 유닛을 칸으로 보내 그곳에 사제를 내려놓는다. 풀려난 사제는 완전히 회복한다
/// (구출 미션: 동맹 사제를 내 섬에 내려놓으면 aiNPriestSaved).
/// </summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="TransportId">사제를 운반 중인 수송 유닛 번호</param>
/// <param name="X">내려놓을 칸 x</param>
/// <param name="Y">내려놓을 칸 y</param>
public sealed record DropPriestCommand(int Player, int TransportId, int X, int Y) : GameCommand(Player);

/// <summary>내 사제를 내 제단으로 보낸다. PriestId 가 0 이면 첫 자유 사제를 고른다.</summary>
/// <param name="Player">플레이어 번호</param>
/// <param name="AltarId">내 제단 번호</param>
/// <param name="PriestId">이동할 사제 번호 (0 이면 자동 선택)</param>
public sealed record MovePriestToAltarCommand(int Player, int AltarId, int PriestId = 0) : GameCommand(Player);
