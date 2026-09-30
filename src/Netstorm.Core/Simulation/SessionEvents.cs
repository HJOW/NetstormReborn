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
    /// <summary>유닛을 놓음</summary>
    UnitPlaced,

    /// <summary>건물 건설 시작</summary>
    BuildingStarted,

    /// <summary>건물 건설 완료 (템플이면 섬 소유·다리 공급 시작, 워크샵이면 덱에 등록 가능)</summary>
    BuildingCompleted,

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
