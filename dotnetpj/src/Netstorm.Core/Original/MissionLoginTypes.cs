// 로그인 부모가 같은 현재 전역/실제 장치 주소를 빌리는 뷰다.
namespace Netstorm.Core.Original;

/// <summary>RawMissionLogin.h의 현재 참조 묶음이며 Ready 성공 사본을 두지 않는다.</summary>
public sealed record MissionLoginGlobals
{
    /// <summary>00540bc0의 현재 서버 여부를 빌린다.</summary>
    public Func<bool>? Server { get; init; }
    /// <summary>00540bc4의 현재 Ready 송신 권한을 빌린다.</summary>
    public Func<bool>? Boss { get; init; }
    /// <summary>현재 Config0054db50이며 로컬이라는 이유로 켜지지 않는 관문을 빌린다.</summary>
    public Func<bool>? Simulconnect { get; init; }
    /// <summary>00594fa0의 실제 저장 월드 선택을 빌린다.</summary>
    public Func<bool>? MissionWorld { get; init; }
    /// <summary>00594fa4의 실제 후처리 생략 관문을 빌린다.</summary>
    public Func<bool>? MissionPost { get; init; }
    /// <summary>00594fc8의 현재 Player 번호 진단 조건을 빌린다.</summary>
    public Func<bool>? Metamap { get; init; }
    /// <summary>같은 실제 두 owner와 known을 빌린다.</summary>
    public PlayerWorldState Player { get; init; } = new();
    /// <summary>00540c74의 현재 자기 구독자를 빌린다.</summary>
    public Func<uint>? MySubscriber { get; init; }
    /// <summary>005951f8의 실제 Fort 순서 수를 빌린다.</summary>
    public UIntBox FortCount { get; init; } = new();
    /// <summary>005951fc의 실제 마지막 새 Fort 소유자를 빌린다.</summary>
    public UIntBox LastLoginPlayer { get; init; } = new();
    /// <summary>005424b0의 실제 SID 결정적 난수 소비 수를 빌린다.</summary>
    public UIntBox PredictableCount { get; init; } = new();
    /// <summary>005c7a74의 실제 시작 전역을 빌린다.</summary>
    public UIntBox Begin { get; init; } = new();
    /// <summary>005409c4의 현재 시작 시드를 빌린다.</summary>
    public Func<uint>? BeginSeed { get; init; }
    /// <summary>00595088의 현재 전투 시드이며 공유 signed 주소가 없을 때 선택을 빌린다.</summary>
    public Func<uint>? BattleSeed { get; init; }
    /// <summary>존재하면 같은 실제 자원 시드 주소를 빌린다.</summary>
    public IntBox? SharedBattleSeed { get; init; }
    /// <summary>0054dbc4의 현재 디버그 묶음 송신 관문을 빌린다.</summary>
    public Func<uint>? DebugBurst { get; init; }
    /// <summary>0052ed54/55의 같은 현재2바이트를 빌린다.</summary>
    public ReadOnlyMemory<byte> DebugPacketHeader { get; init; }
    /// <summary>00544c24의 현재 디버그 생성 길이를 빌린다.</summary>
    public Func<ushort>? DebugPacketInitialLength { get; init; }
    /// <summary>00594fb0의 실제 CRT clock 저장값을 빌린다.</summary>
    public UIntBox ClockStamp { get; init; } = new();
    /// <summary>00594fb4의 현재 기준 CRT clock을 빌린다.</summary>
    public Func<uint>? BaseClock { get; init; }
    /// <summary>005c8618의 현재 벽시각 마감을 빌린다.</summary>
    public Func<double>? CaptureEnd { get; init; }
    /// <summary>00595350의 현재 선택 주소 조회를 빌린다.</summary>
    public Func<Memory<byte>>? ReadCoreData { get; init; }
    /// <summary>00595350의 같은 선택 주소 변경을 빌린다.</summary>
    public Action<Memory<byte>>? StoreCoreData { get; init; }
    /// <summary>005953f0의 같은 실제10바이트 캐시를 빌린다.</summary>
    public Memory<byte> CachedCoreData { get; init; }
    /// <summary>005c86c0의 같은 실제6바이트를 빌린다.</summary>
    public ReadOnlyMemory<byte> ZoneAddress { get; init; }
    /// <summary>0052f7e0의 같은 현재87바이트를 빌린다.</summary>
    public ReadOnlyMemory<byte> BattleOptions { get; init; }
    /// <summary>일반 초기 자원 선택 때만 요구하는 실제005c85d4을 빌린다.</summary>
    public Func<bool>? SavedGame { get; init; }
    /// <summary>일반 초기 자원 선택 때만 요구하는 실제00594fbc을 빌린다.</summary>
    public Func<bool>? InBattle { get; init; }
}

/// <summary>선택한 실제 큰 하위만 요구하며 TConnection은 같은 실제 노드 정체성을 유지한다.</summary>
public sealed record MissionLoginHooks<TConnection> where TConnection : class
{
    /// <summary>실제 현재 연결 해제를 호출한다.</summary>
    public Action<int>? Disconnect { get; init; }
    /// <summary>전체 mutable 예약을 현재 실제 연결 gate에 전달하는 송신을 호출한다.</summary>
    public Action<int, Memory<byte>>? Send { get; init; }
    /// <summary>대상0 Ready의 실제 전체분배를 호출한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; init; }
    /// <summary>실제 미발견 거부 응답의 같은 연결 가상 송신을 호출한다.</summary>
    public Action<TConnection, ReadOnlyMemory<byte>>? PendingReply { get; init; }
    /// <summary>004e2920의 현재 전체 flush을 호출한다.</summary>
    public Action? Flush { get; init; }
    /// <summary>00460d70의 현재 벽시각을 호출한다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>CRT clock의 현재32비트를 호출한다.</summary>
    public Func<uint>? Clock { get; init; }
    /// <summary>같은 실제 분산SP의 현재float32 비트를 조회한다.</summary>
    public Func<uint, uint>? SpBits { get; init; }
    /// <summary>004b4900의 실제 전체 옵션 수신을 호출한다.</summary>
    public Action<ReadOnlyMemory<byte>>? ApplyOptions { get; init; }
    /// <summary>0041c320의 같은 실제256바이트 사용표 초기화를 호출한다.</summary>
    public Action? ClearTypes { get; init; }
    /// <summary>0048f9f0의 실제 owner/target Player writer를 호출한다.</summary>
    public Action<uint, uint>? SendPlayer { get; init; }
    /// <summary>004a6b40의 현재 중앙 Fort metadata 전체를 호출한다.</summary>
    public Action<uint>? SendFortMetadata { get; init; }
    /// <summary>자기 server 파일을 같은 Template에 읽는 전체 부모를 호출한다.</summary>
    public Action<uint>? LoadCurrentFort { get; init; }
    /// <summary>다른 owner의 실제 준비/이미지 읽기 전체를 호출한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? LoadFortImage { get; init; }
    /// <summary>현재 실제 peer의 좁은 주소를 호출한다.</summary>
    public Func<uint, string>? PeerAddress { get; init; }
    /// <summary>현재 Template 직렬화와 실제 owner/target/count 전송을 호출한다.</summary>
    public Action<uint, uint, uint>? SendFort { get; init; }
    /// <summary>다른 owner의 실제 전체 월드 동기화를 호출한다.</summary>
    public Action<uint>? SynchronizePeer { get; init; }
    /// <summary>새 미션 로그인의 같은 Session.Load를 호출한다.</summary>
    public Action? LoadWorld { get; init; }
    /// <summary>현재 실제7DWORD 설정 전송을 호출한다.</summary>
    public Action<uint>? SendSettings { get; init; }
    /// <summary>원문 진단 뒤 복귀하는 실제 장치를 호출한다.</summary>
    public Action<string>? Assertion { get; init; }
    /// <summary>previous0에서만 선택되는 실제0048fb30 전체를 호출한다.</summary>
    public Action<uint>? RestoreCoreData { get; init; }
    /// <summary>현재 owner/정수SP 지정과 실제 HUD/편집 처리 전체를 호출한다.</summary>
    public Action<uint, int>? SetSp { get; init; }
    /// <summary>일반 전투의 실제 섬 생성/배치 전체를 호출한다.</summary>
    public Action<uint>? MakeBattleIsland { get; init; }
    /// <summary>63h의 같은 실제 SID/가상/환불 정리를 호출한다.</summary>
    public Action? CleanupReconnectClients { get; init; }
    /// <summary>subscriber 미발견 꼬리의 현재005e4794을 호출한다.</summary>
    public Func<bool>? CheckingAssertions { get; init; }
    /// <summary>004a68e0의 실제 현재 로그를 호출한다.</summary>
    public Action<string>? Log { get; init; }
}
