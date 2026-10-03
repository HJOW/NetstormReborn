using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>명령·판정이 실패한 이유. None 이면 성공이다.</summary>
public enum CommandFailure
{
    /// <summary>성공</summary>
    None,

    /// <summary>알 수 없는 명령 종류</summary>
    UnknownCommand,

    /// <summary>세션에 없는 플레이어</summary>
    UnknownPlayer,

    /// <summary>타입 이름을 찾을 수 없음</summary>
    UnknownType,

    /// <summary>명령이 다룰 수 없는 종류의 타입 (예: 건물을 유닛으로 놓기)</summary>
    WrongKind,

    /// <summary>기술 허용 표가 막은 타입 (Construct 메뉴·생산 목록·덱)</summary>
    TechDenied,

    /// <summary>생산 창(덱)에 등록되어 있지 않은 유닛</summary>
    NotInDeck,

    /// <summary>방금 배치해 아직 재충전 중인 유닛</summary>
    NotReady,

    /// <summary>위치 조건(섬·자리·Storm Power·에너지)을 만족하지 못함 — PlacementProblem 참고</summary>
    Placement,

    /// <summary>워크샵 등록 실패 — RegisterResult 참고</summary>
    RegisterFailed,

    /// <summary>해당 번호의 오브젝트가 없음</summary>
    NoSuchEntity,

    /// <summary>다른 플레이어의 오브젝트</summary>
    NotOwner,

    /// <summary>아직 건설 중인 오브젝트</summary>
    NotComplete,

    /// <summary>회수 금지 상태 (미션 denySalvage 가 켜져 있음)</summary>
    SalvageDenied,

    /// <summary>회수할 수 없는 종류 (사제·가이저·지형 등)</summary>
    CannotSalvage,

    /// <summary>다리 칸의 조각 순번이 잘못됨</summary>
    NoBridgePiece,

    /// <summary>이미 다리 조각을 집고 있음</summary>
    AlreadyHolding,

    /// <summary>집고 있는 다리 조각이 없음</summary>
    NotHolding,

    /// <summary>다리 칸이 가득 차서 되돌릴 수 없음</summary>
    TrayFull,

    /// <summary>다리 조각을 놓을 수 없는 위치 — BridgePlacementProblem 참고</summary>
    BridgeBlocked,

    /// <summary>플레이어가 조종할 수 있는 사제가 없음.</summary>
    NoPriest,

    /// <summary>결정을 전달할 완성된 신전이 없음.</summary>
    NoTemple,

    /// <summary>가이저가 비어 더 채집할 수 없음</summary>
    GeyserEmpty,

    /// <summary>현재 섬·다리 칸에서 가이저까지 걸어갈 경로가 없음.</summary>
    NoRoute,

    /// <summary>집을 수 없는 사제 (기절하지 않음·동맹·이미 포획됨, allowAnyCapture 미션 제외).</summary>
    NotCapturable,

    /// <summary>제단에 이미 다른 사제가 묶여 있음.</summary>
    AltarOccupied,

    /// <summary>수송 유닛이 사제를 운반하고 있지 않음.</summary>
    NotCarrying,

    /// <summary>수송 유닛이 이미 사제를 운반하고 있음.</summary>
    AlreadyCarrying,
}

/// <summary>명령 실행 결과.</summary>
/// <param name="Failure">실패 이유 (성공이면 None)</param>
/// <param name="Detail">사람이 읽는 설명 (개발용 한국어 문구)</param>
public sealed record CommandResult(CommandFailure Failure, string Detail = "")
{
    /// <summary>성공했는지</summary>
    public bool Accepted => Failure == CommandFailure.None;

    /// <summary>성공 결과</summary>
    /// <param name="detail">설명</param>
    public static CommandResult Ok(string detail = "") => new(CommandFailure.None, detail);
}

/// <summary>
/// 배치 시험·판정 결과: 생산 규칙(기술 허용·덱 등록·재충전) 실패가 있으면 그것이 먼저이고, 없으면 위치 판정 결과다.
/// </summary>
/// <param name="Failure">실패 이유 (Placement 이면 Site.Problem 이 원인)</param>
/// <param name="Site">위치 판정 (타입을 찾지 못하는 등 판정 전에 실패하면 null)</param>
public sealed record SessionPlacementCheck(CommandFailure Failure, PlacementCheck? Site)
{
    /// <summary>놓거나 지을 수 있는지</summary>
    public bool Allowed => Failure == CommandFailure.None;

    /// <summary>사람이 읽는 실패 설명 (가능하면 "배치 가능")</summary>
    public string Describe() => Failure switch
    {
        CommandFailure.None => "배치 가능",
        CommandFailure.Placement when Site != null => PlacementRules.Describe(Site.Problem),
        _ => SessionText.Describe(Failure),
    };
}

/// <summary>게임 세션에서 일어난 일의 종류 (화면 알림·소리·기록용).</summary>
public enum SessionEventKind
{
    /// <summary>포대가 탄을 발사함 (EntityId = 발사자).</summary>
    ShotFired,

    /// <summary>탄이 맞아 체력이 줄어듦 (EntityId = 피해 대상).</summary>
    EntityDamaged,

    /// <summary>체력을 모두 잃어 오브젝트가 파괴됨.</summary>
    EntityDestroyed,

    /// <summary>전투로 파괴된 템플·포대가 주위 1칸에 폭발 피해를 줌 (Player = 처치한 플레이어, EntityId = 파괴된 오브젝트).</summary>
    EntityExploded,

    /// <summary>사제가 체력 절반에서 기절해 보호막을 얻음.</summary>
    PriestStunned,

    /// <summary>신전이 있는 사제가 체력을 회복해 기절에서 깨어남.</summary>
    PriestRecovered,

    /// <summary>유닛을 놓음</summary>
    UnitPlaced,

    /// <summary>건물 건설 시작</summary>
    BuildingStarted,

    /// <summary>건물 건설 완료 (템플이면 섬 소유·다리 공급 시작, 워크샵이면 덱에 등록 가능)</summary>
    BuildingCompleted,

    /// <summary>사제가 가이저에서 Storm Crystal 하나를 가져옴.</summary>
    CrystalCollected,

    /// <summary>사제가 신전에 Storm Crystal 하나를 전달해 Storm Power를 얻음.</summary>
    CrystalDelivered,

    /// <summary>가이저의 Storm Power 가 바닥나 빈 가이저가 됨 (EntityId = 가이저). 원본 도움말: 수송 유닛은 가이저가 고갈될 때까지 채집을 반복한다.</summary>
    GeyserDepleted,

    /// <summary>튜토리얼의 F4 화면 복귀 입력.</summary>
    ReturnedHome,

    /// <summary>워크샵에 지식을 등록함</summary>
    Registered,

    /// <summary>오브젝트를 회수함</summary>
    Salvaged,

    /// <summary>다리 칸에 새 조각이 들어옴</summary>
    BridgePieceAdded,

    /// <summary>다리 조각을 집음</summary>
    BridgePicked,

    /// <summary>집었던 다리 조각을 되돌림</summary>
    BridgeReturned,

    /// <summary>다리 조각을 놓음</summary>
    BridgePlaced,

    /// <summary>다리 칸이 금 감 (원본은 bridgeCrack.wav 재생)</summary>
    BridgeCracked,

    /// <summary>다리 칸이 무너져 사라짐</summary>
    BridgeCollapsed,

    /// <summary>명령이 거부됨</summary>
    CommandRejected,

    /// <summary>
    /// 튜토리얼이 미션 스크립트의 섹션을 알림 (Text = 섹션 이름: 단계 "A." "B." … 또는 보정 안내 "NotVortex").
    /// 화면은 그 섹션 본문을 안내 창으로 띄운다 (원본 FUN_004cf960 = 스크립트 섹션 Tell).
    /// </summary>
    TutorialTell,

    /// <summary>공중 기지가 새 비행체를 만들었다.</summary>
    FlyerLaunched,

    /// <summary>비행체가 기지에 돌아와 연료를 보충한다.</summary>
    FlyerRefuelling,

    /// <summary>귀환할 기지가 없는 비행체가 종료됐다.</summary>
    FlyerExpired,

    /// <summary>수송 유닛이 사제를 집음 (Player = 수송 유닛 소유자, EntityId = 사제). 원본 golemPickUp·ourPriestCaptured/enemyPriestCaptured.</summary>
    PriestCaptured,

    /// <summary>수송 유닛이 도착했지만 사제가 회복해 포획에 저항함 (EntityId = 사제).</summary>
    PriestResisted,

    /// <summary>운반한 사제를 제단의 희생의 원에 묶음 (EntityId = 사제).</summary>
    PriestBound,

    /// <summary>포획된 사제가 풀려나 완전히 회복함 (내려놓음·운반 유닛 파괴·판매, 제단 파괴·판매, EntityId = 사제).</summary>
    PriestReleased,

    /// <summary>제단에서 희생 의식이 시작됨 (Player = 제단 소유자, EntityId = 제단). 원본은 이때 희생 음악을 튼다.</summary>
    SacrificeStarted,

    /// <summary>의식의 룬 하나를 지키기 시작함 (Text = 룬 이름 Wind/Sun/Rain/Thunder/Storm, 원본 forWind2.wav 등).</summary>
    SacrificeRune,

    /// <summary>룬 하나가 타서 사라짐 (원본 altarBurnCollapse.wav).</summary>
    SacrificeRuneBurned,

    /// <summary>다섯 룬을 모두 지켜 의식이 완료됨 (원본 itIsDone2.wav).</summary>
    SacrificeCompleted,

    /// <summary>묶인 사제가 희생됨 (원본 priestSacrifice2.wav, EntityId = 희생된 사제).</summary>
    PriestSacrificed,

    /// <summary>제단이 파괴·판매돼 의식이 깨지고 포로가 풀림 (EntityId = 제단). 의식 사제의 이탈·기절·포획은 깨지지 않고 멈춘다.</summary>
    SacrificeBroken,

    /// <summary>의식 사제가 제단 옆을 떠나거나 기절·포획돼 의식이 멈춤 (EntityId = 제단). 포로는 묶인 채 남는다(2026-10-01 녹화·사용자 확인).</summary>
    SacrificePaused,

    /// <summary>마크가 나타나기 전에 사제가 떠나 그 룬이 취소됨 (Text = 룬 이름, EntityId = 제단). 복귀하면 음성부터 다시 한다.</summary>
    SacrificeRuneCancelled,

    /// <summary>의식 사제가 움직일 수 있는 상태로 제단 옆에 돌아와 의식이 이어짐 (EntityId = 제단).</summary>
    SacrificeResumed,

    /// <summary>적 사제를 의식으로 완전히 처리해 제단 주인이 스톰 파워 5,000을 받음 (EntityId = 희생된 사제).</summary>
    SacrificeRewarded,

    /// <summary>의식이 끝난 제단이 소멸함 (EntityId = 제단).</summary>
    AltarConsumed,

    /// <summary>
    /// 미션 스크립트 섹션을 알림 (Text = "BadTeamDead"·"Failed"·"ai2PriestCaptured" 등). 원본 FUN_004c36c0 의 승패·AI 이벤트이며
    /// 각 이름은 미션당 한 번만 나온다. 화면은 그 섹션이 스크립트에 있으면 창을 띄운다(게임 시간 정지).
    /// </summary>
    MissionTell,

    /// <summary>발판을 잃은 지상 이동체가 낙하해 제거됐다.</summary>
    UnitFell,

    /// <summary>발판을 잃은 사제가 그 자리의 허공에서 기절했다.</summary>
    PriestSuspended,

    /// <summary>남은 목표까지 길이 없어 이동/수확이 대기 상태가 됐다.</summary>
    MoveBlocked,

    /// <summary>지형이 다시 이어져 기존 이동/수확을 재개했다.</summary>
    MoveResumed,

    /// <summary>
    /// 룬 음성이 끝나 제단에 그 룬의 마크가 나타났다 (Text = 룬 이름). 이 뒤에는 사제가 떠나도 룬이 탄다.
    /// 원본은 이때부터 룬이 탈 때까지 jimbuild.wav 를 1.14초마다 반복한다(2026-10-02 녹음 판독).
    /// </summary>
    SacrificeRuneMarked,

    /// <summary>워크샵을 한 단계 업그레이드했다 (원본 완료음 upgradeComplete.wav, exe FUN_004545e0).</summary>
    WorkshopUpgraded,

    /// <summary>아이스 타워가 깨져 같은 소유자·장소에서 재성장을 시작했다. 처치 보상은 없다.</summary>
    IceTowerShattered,

    /// <summary>아이스 타워가 재성장을 끝내 최대 체력으로 돌아왔다.</summary>
    IceTowerRegrown,

    /// <summary>적의 탄을 흡수한 썬 바리케이트 방어선. EntityId는 선의 시작 기둥 번호다.</summary>
    ShotBlocked,

    /// <summary>조준·착탄 동작의 원본 효과음 요청. Text는 sound/ 파일 이름이다.</summary>
    CombatSound,
}

/// <summary>세션 이벤트 한 건.</summary>
/// <param name="Tick">일어난 틱</param>
/// <param name="Kind">종류</param>
/// <param name="Player">관련 플레이어 (없으면 0)</param>
/// <param name="EntityId">관련 오브젝트 번호 (없으면 0)</param>
/// <param name="Text">개발용 한국어 설명</param>
/// <param name="Failure">명령이 거부된 이유 (거부 이벤트가 아니면 None)</param>
public sealed record SessionEvent(long Tick, SessionEventKind Kind, int Player, int EntityId, string Text,
    CommandFailure Failure = CommandFailure.None);

/// <summary>실패 이유의 개발용 한국어 문구.</summary>
public static class SessionText
{
    /// <summary>실패 이유 설명</summary>
    /// <param name="failure">실패 이유</param>
    public static string Describe(CommandFailure failure) => failure switch
    {
        CommandFailure.None => "성공",
        CommandFailure.UnknownCommand => "알 수 없는 명령",
        CommandFailure.UnknownPlayer => "없는 플레이어",
        CommandFailure.UnknownType => "없는 타입",
        CommandFailure.WrongKind => "이 명령으로 다룰 수 없는 종류",
        CommandFailure.TechDenied => "미션이 막은 기술",
        CommandFailure.NotInDeck => "생산 창에 등록되지 않은 유닛",
        CommandFailure.NotReady => "재충전 중",
        CommandFailure.Placement => "위치 조건 불만족",
        CommandFailure.RegisterFailed => "워크샵에 등록할 수 없음",
        CommandFailure.NoSuchEntity => "없는 오브젝트",
        CommandFailure.NotOwner => "내 오브젝트가 아님",
        CommandFailure.NotComplete => "아직 건설 중",
        CommandFailure.SalvageDenied => "회수 금지 상태",
        CommandFailure.CannotSalvage => "회수할 수 없는 종류",
        CommandFailure.NoBridgePiece => "그 칸에 다리 조각이 없음",
        CommandFailure.AlreadyHolding => "이미 다리 조각을 집고 있음",
        CommandFailure.NotHolding => "집고 있는 다리 조각이 없음",
        CommandFailure.TrayFull => "다리 칸이 가득 참",
        CommandFailure.BridgeBlocked => "다리를 놓을 수 없는 위치",
        CommandFailure.NoPriest => "움직일 사제가 없음",
        CommandFailure.NoTemple => "결정을 전달할 신전이 없음",
        CommandFailure.GeyserEmpty => "가이저가 비어 있음",
        CommandFailure.NoRoute => "목적지까지 이어진 길이 없음",
        CommandFailure.NotCapturable => "집을 수 없는 사제 (기절하지 않았거나 동맹)",
        CommandFailure.AltarOccupied => "제단에 이미 사제가 묶여 있음",
        CommandFailure.NotCarrying => "운반 중인 사제가 없음",
        CommandFailure.AlreadyCarrying => "이미 사제를 운반 중",
        _ => failure.ToString(),
    };

    /// <summary>다리 배치 불가 이유 설명</summary>
    /// <param name="problem">다리 배치 판정의 문제</param>
    public static string Describe(BridgePlacementProblem problem) => problem switch
    {
        BridgePlacementProblem.None => "놓을 수 있음",
        BridgePlacementProblem.OutOfWorld => "월드 밖",
        BridgePlacementProblem.Blocked => "섬·다리·오브젝트와 겹침",
        BridgePlacementProblem.NotAttached => "섬 가장자리(초목 없는 곳)나 내 다리 끝에 이어지지 않음",
        _ => problem.ToString(),
    };
}
