// 로컬 상태의 다섯 실제 블록과 현재 전역/효과를 빌리는 형식이다.
namespace Netstorm.Core.Original;

/// <summary>원본 서로 다른 다섯0x90바이트 블록을 같은 물리에서 빌린다.</summary>
public sealed record LocalMissionStateStorage
{
    /// <summary>005c8630의 이전 확정 블록이다.</summary>
    public Memory<byte> Old { get; init; }
    /// <summary>005c8758의 마지막 확정 phase이다.</summary>
    public Memory<byte> Phase { get; init; }
    /// <summary>005c86c8의 같은 Run 즉시 요청이다.</summary>
    public Memory<byte> Immediate { get; init; }
    /// <summary>005c87e8의 현재 실행 명령이다.</summary>
    public Memory<byte> Current { get; init; }
    /// <summary>005c8878의 다음 Poll 요청이다.</summary>
    public Memory<byte> Next { get; init; }
}

/// <summary>원문 State7의 정리 전 실제 지역값이며 뒤 현재+84를 따로 재조회한다.</summary>
public readonly record struct LocalMissionStartingSnapshot
{
    /// <summary>정리 전005c85c0 값이다.</summary>
    public uint ZonePending { get; init; }
    /// <summary>현재+84 비영 때 보관한0055b4ac 값이다.</summary>
    public uint GameTicks { get; init; }
    /// <summary>같은 조건으로 보관한0055a420 값이다.</summary>
    public uint SourceType { get; init; }
}

/// <summary>같은 실제 Client 전역을 현재 getter/setter 또는 원문 상자로 빌린다.</summary>
public sealed record LocalMissionStateGlobals
{
    /// <summary>현재 서버 모드를 조회한다.</summary>
    public Func<bool>? Server { get; init; }
    /// <summary>현재 방장 모드를 조회한다.</summary>
    public Func<bool>? Boss { get; init; }
    /// <summary>같은 실제 전투 모드를 쓴다.</summary>
    public Action<bool>? SetInBattle { get; init; }
    /// <summary>현재 미션 후처리 모드를 조회한다.</summary>
    public Func<bool>? MissionPost { get; init; }
    /// <summary>현재 저장 게임 모드를 조회한다.</summary>
    public Func<bool>? SavedGame { get; init; }
    /// <summary>같은 실제 저장 모드를 쓴다.</summary>
    public Action<bool>? SetSavedGame { get; init; }
    /// <summary>현재 미션 월드 선택을 조회한다.</summary>
    public Func<bool>? MissionWorld { get; init; }
    /// <summary>현재 동시접속 설정을 조회한다.</summary>
    public Func<bool>? Simulconnect { get; init; }
    /// <summary>현재 메타맵 모드를 조회한다.</summary>
    public Func<bool>? Metamap { get; init; }
    /// <summary>같은 실제 두owner/known을 빌린다.</summary>
    public PlayerWorldState Player { get; init; } = new();
    /// <summary>현재005c85c0을 빌린다.</summary>
    public UIntBox ZonePending { get; init; } = new();
    /// <summary>현재0055b4ac를 빌린다.</summary>
    public UIntBox GameTicks { get; init; } = new();
    /// <summary>현재0055a420을 빌린다.</summary>
    public UIntBox SourceType { get; init; } = new();
    /// <summary>같은005c85b8 CD 확인 결과를 쓴다.</summary>
    public UIntBox CdPresent { get; init; } = new();
    /// <summary>immediate[33]과 같은005c874c DWORD를 빌린다.</summary>
    public UIntBox MenuWindow { get; init; } = new();
    /// <summary>같은005c85d8을 읽고 쓴다.</summary>
    public UIntBox MetaRefresh { get; init; } = new();
    /// <summary>같은005c85dc를 읽고 쓴다.</summary>
    public UIntBox MetaRebuild { get; init; } = new();
    /// <summary>같은 HTML 배경80바이트 중 첫BYTE를 빌린다.</summary>
    public Memory<byte> TitleInitial { get; init; }
    /// <summary>현재005c85d0 BYTE를 조회한다.</summary>
    public Func<byte>? BattleByte { get; init; }
    /// <summary>같은005c85f8 연결 벽시각 기한이다.</summary>
    public DoubleBox ConnectEnd { get; init; } = new();
    /// <summary>같은00544d58 주변음 벽시각 기한이다.</summary>
    public DoubleBox ActivityEnd { get; init; } = new();
    /// <summary>0055b4d0의 현재 게임 시각을 조회한다.</summary>
    public Func<double>? GameSeconds { get; init; }
    /// <summary>0055b4d8의 현재 프레임 벽시각을 조회한다.</summary>
    public Func<double>? WallCached { get; init; }
}

/// <summary>큰 부모의 자연 반환을 공통144바이트 확정과 구별한다.</summary>
public enum LocalMissionParentResult
{
    /// <summary>원문 공통부로 돌아가 전체 블록을 확정한다.</summary>
    Commit,
    /// <summary>현재 블록/phase의 공통 확정 없이 반환한다.</summary>
    ReturnWithoutCommit,
}

/// <summary>원문 실제 진행창의 문구 선택이다.</summary>
public enum LocalMissionProgress
{
    /// <summary>원문 MissionStarting 진행 안내를 선택한다.</summary>
    MissionStarting,
    /// <summary>원문 ReconnectStarting 진행 안내를 선택한다.</summary>
    ReconnectStarting,
    /// <summary>원문 ConnectingCountdown 진행 안내를 선택한다.</summary>
    ConnectingCountdown,
    /// <summary>원문 ReconnectCountdown 진행 안내를 선택한다.</summary>
    ReconnectCountdown,
    /// <summary>원문 ServerLost 진행 안내를 선택한다.</summary>
    ServerLost,
    /// <summary>원문 ConnectingStarting 진행 안내를 선택한다.</summary>
    ConnectingStarting,
    /// <summary>원문 Connecting 진행 안내를 선택한다.</summary>
    Connecting,
    /// <summary>원문 Reconnecting 진행 안내를 선택한다.</summary>
    Reconnecting,
    /// <summary>원문 Transferring 진행 안내를 선택한다.</summary>
    Transferring,
    /// <summary>원문 ReconnectTransferring 진행 안내를 선택한다.</summary>
    ReconnectTransferring,
    /// <summary>원문 MissionTransferring 진행 안내를 선택한다.</summary>
    MissionTransferring,
}

/// <summary>선택된 실제 UI/파일/전송/정리 부모만 도달 위치에서 요구한다.</summary>
public sealed record LocalMissionStateHooks
{
    /// <summary>0045ad00의 현재 상태 진입 로그를 기록한다.</summary>
    public Action<uint>? LogState { get; init; }
    /// <summary>004b2bc0에서 이미 요청된 immediate를 진단하고 복귀할 수 있다.</summary>
    public Action? Assertion { get; init; }
    /// <summary>원문 default의 현재 미지원 상태 진단을 실행한다.</summary>
    public Action<uint>? InvalidState { get; init; }
    /// <summary>큰 메뉴/메타 상태 부모 전체를 실행하고 공통 확정 여부를 반환한다.</summary>
    public Func<uint, LocalMissionParentResult>? ParentState { get; init; }
    /// <summary>004978d0(0)의 기본 배경 시차 분모를 선택한다.</summary>
    public Action? SelectDefaultDenominator { get; init; }
    /// <summary>004b2df0의 실제 전투 정리를 인자1로 실행한다.</summary>
    public Action<uint>? CleanupBattle { get; init; }
    /// <summary>00440c60의 현재 CD 확인 결과를 반환한다. 네트워크 검증으로 대체하지 않는다.</summary>
    public Func<uint>? CdPresent { get; init; }
    /// <summary>0042e440의 현재 명령창 숨김/갱신을 실행한다.</summary>
    public Action? RefreshCommands { get; init; }
    /// <summary>00469fc0의 실제 음악 정리를 실행한다.</summary>
    public Action? StopMusic { get; init; }
    /// <summary>missionWorld와 missionPost가 참일 때00463a40 가상+28을 호출한다.</summary>
    public Action? RefreshMission { get; init; }
    /// <summary>동시 접속에서 현재 포트의 실제 listener를 연다.</summary>
    public Action? ListenServer { get; init; }
    /// <summary>일반 서버 전투의 같은 난수 밀도 writer를 실행한다.</summary>
    public Action? InitializeDensity { get; init; }
    /// <summary>일반 상태7의 채널40h와80h를 순서대로 지운다.</summary>
    public Action<uint>? ClearChat { get; init; }
    /// <summary>004b4be0으로 현재 Zone 핸들을 설치한다. 핸들 해석은 같은 소유자가 맡는다.</summary>
    public Action<uint>? InstallZone { get; init; }
    /// <summary>설치 뒤 현재 블록에서 다시 읽은 Zone 핸들을 실제 반납한다.</summary>
    public Action<uint>? FreeZone { get; init; }
    /// <summary>004c1e70의 실제 맵 처리기를 등록한다.</summary>
    public Action? InstallMapHandlers { get; init; }
    /// <summary>004993c0의 실제 Renderer를 초기화한다.</summary>
    public Action? InitializeRenderer { get; init; }
    /// <summary>00466280(mode,0,1)의 실제 UI 모드를 설정한다.</summary>
    public Action<uint>? UiMode { get; init; }
    /// <summary>00460d70의 현재 벽시각을 조회한다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>00460980의 pending Fort 이름/manager snapshot과 Notify 부모를 실행한다.</summary>
    public Action? LoadReconnectFort { get; init; }
    /// <summary>선택된 원문 진행 문구의 실제 창을 생성하거나 갱신한다.</summary>
    public Action<LocalMissionProgress, int>? Progress { get; init; }
    /// <summary>004410f0으로 battlePal의 실제 핸들을 조회한다.</summary>
    public Func<uint>? BattlePalette { get; init; }
    /// <summary>004b6100으로 조회한 실제 팔레트를 적용한다.</summary>
    public Action<uint>? ApplyPalette { get; init; }
    /// <summary>004e2620(9,current+54,current+7c,0)의 실제 연결을 시작한다. 주소는 현재 블록의 같은20바이트다.</summary>
    public Action<ReadOnlyMemory<byte>, uint>? ConnectServer { get; init; }
    /// <summary>004e2b70의 현재 연결 상태를 조회한다.</summary>
    public Func<int, int>? ConnectionStatus { get; init; }
    /// <summary>004e2b10의 현재 연결 열림 여부를 조회한다.</summary>
    public Func<int, bool>? ConnectionOpen { get; init; }
    /// <summary>004e2940으로 실제 지정 노드를 닫는다.</summary>
    public Action<int>? CloseConnection { get; init; }
    /// <summary>004e27b0으로 같은 실제 노드의 연결 번호를 변경한다.</summary>
    public Action<int, int>? RenameConnection { get; init; }
    /// <summary>401h/40fh 분기의 현재 연결 진단을 기록한다.</summary>
    public Action<int, int>? LogConnection { get; init; }
    /// <summary>004b63d0의 실제 로그인 송신을 실행한다. 수신 처리는 공통 확정 뒤 큐로 분배한다.</summary>
    public Action? SendLogin { get; init; }
    /// <summary>004b6090의 실제 로그인 전송 진행 안내를 실행한다.</summary>
    public Action? ConnectedProgress { get; init; }
    /// <summary>004c8dc0의 현재 진행창 전체를 해제한다.</summary>
    public Action? CloseProgress { get; init; }
    /// <summary>004e2220(9)의 같은 실제 서버 연결 핸들을 조회한다.</summary>
    public Func<uint>? ServerConnection { get; init; }
    /// <summary>004c1eb0에 현재 서버 연결을 전달한다.</summary>
    public Action<uint>? AttachCommander { get; init; }
    /// <summary>현재 Template owner의 실제 Fort wrapper 존재 여부를 조회한다.</summary>
    public Func<uint, bool>? FortAvailable { get; init; }
    /// <summary>현재 Fort 이름과 파일 읽기의 필수 부모를 실행한다.</summary>
    public Action? LoadCurrentFort { get; init; }
    /// <summary>004bd550으로 같은 실제 CoreData를 최소24바이트 빌린다. 주소 조회 전 선택한 메모리를 유지한다.</summary>
    public Func<uint, Memory<byte>>? FortCore { get; init; }
    /// <summary>004e2a50(out,9)의 실제20바이트 주소를 반환한다.</summary>
    public Func<ReadOnlyMemory<byte>>? LocalAddress { get; init; }
    /// <summary>004bd540으로 현재 Template owner의 Fort를 갱신한다.</summary>
    public Action<uint>? TouchFort { get; init; }
    /// <summary>00460730의 실제 Fort 설정 갱신을 실행한다.</summary>
    public Action? RefreshFort { get; init; }
    /// <summary>004477d0으로 실제 재접속 SP를 지정한다.</summary>
    public Action<float>? SetReconnectSp { get; init; }
    /// <summary>004b67c0(1)의 실제 로그인 타이머를 시작한다.</summary>
    public Action<bool>? LoginTimer { get; init; }
    /// <summary>0042f270(0)의 실제 입력 모드로 전환한다.</summary>
    public Action<bool>? InputMode { get; init; }
    /// <summary>Poll의00479760 메타맵 선행 효과를 실행한다.</summary>
    public Action? MetaFirst { get; init; }
    /// <summary>Poll의0047dd60 메타맵 갱신을 실행한다.</summary>
    public Action? MetaSecond { get; init; }
    /// <summary>Poll의0047cab0 메타맵 후처리를 실행한다.</summary>
    public Action? MetaThird { get; init; }
    /// <summary>상태15에서 같은 월드/Player/전송/시계 부모를 실행한다. 즉시16 요청도 공통 확정한다.</summary>
    public Action? TakeServer { get; init; }
}
