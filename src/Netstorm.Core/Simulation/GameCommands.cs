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
/// <param name="TrayIndex">칸의 조각 순번 (BridgeTray.Pieces 기준)</param>
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
