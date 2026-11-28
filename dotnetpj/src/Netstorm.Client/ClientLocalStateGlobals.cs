using Netstorm.Core.Original;
using Netstorm.Client.Chat;
using Netstorm.Net;

// 같은 Client 수명에서 State/로그인/내용물/주변음/시계가 현재 저장소를 공유한다.
namespace Netstorm.Client;

/// <summary>다섯 실제 상태 블록과 로그인/시계 전역의 장기 소유자다.</summary>
public sealed class ClientLocalStateGlobals
{
    /// <summary>옵션/주 프레임이 공유하는 현재 시간 알림의 초기화·기한·두 signed DWORD다.</summary>
    public MonitorTimeState MonitorTime { get; } = new();
    /// <summary>옵션 해석과 메뉴가 공유하는 실제 helpWord 문구다.</summary>
    public ClientHelpWord HelpWord { get; } = new();
    /// <summary>원본 다섯0x90바이트 블록의 유일한 실제720바이트다.</summary>
    private readonly byte[] _blocks = new byte[720];
    /// <summary>실제8바이트 Zone이며 첫6바이트만 원문 Zone이다.</summary>
    private readonly byte[] _zone = new byte[8];
    /// <summary>같은 기존 ClientGameplay의 저장 모드다.</summary>
    private readonly ClientGameplayGlobals _gameplay;
    /// <summary>같은 실제 게임 전역 정체성을 HostSession 결선에서 확인한다.</summary>
    public ClientGameplayGlobals Gameplay => _gameplay;
    /// <summary>같은 기존 Host 주변음 기한/종류/강제값이다.</summary>
    public AmbientSoundState AmbientSound { get; }
    /// <summary>상태7과 주변음이 같은 실제 기한을 읽고 쓰는 상자다.</summary>
    private sealed class AmbientDeadline : DoubleBox
    {
        /// <summary>이미 실제 프레임이 갱신하는 동일 주변음 소유자다.</summary>
        private readonly AmbientSoundState _state;
        /// <summary>바인딩 자체는 기한을 변경하지 않는다.</summary>
        public AmbientDeadline(AmbientSoundState state) => _state = state;
        /// <summary>현재 실제 주변음 기한을 즉시 읽고 쓴다.</summary>
        public override double Value { get => _state.Deadline; set => _state.Deadline = value; }
    }
    /// <summary>서로 다른 같은144바이트 뷰다. getter가 새 원문 저장소를 만들지 않는다.</summary>
    public LocalMissionStateStorage State { get; }
    /// <summary>같은 immediate+84의 실제005c874c다.</summary>
    public UIntBox MenuWindow { get; }
    /// <summary>005c85d4는 기존 내용물 통지와 같은 BoolBox다.</summary>
    public BoolBox SavedGame => _gameplay.SavedGameBox;
    /// <summary>상태/편집/주변음이 공유하는 같은00544d58이다.</summary>
    public DoubleBox ActivityEnd { get; }
    /// <summary>같은 Client의 실제 세 동기화 DWORD다.</summary>
    public ClockSyncResetGlobals ClockSynchronization { get; } = new();
    /// <summary>같은 실제70표본과 진단값이다.</summary>
    public ClockSynchronizationData ClockSamples { get; } = new();
    /// <summary>State/저장/시계의 같은0055b4ac다.</summary>
    public UIntBox GameTicks => ClockSynchronization.GameTicks;
    /// <summary>전체 Territory DWORD와 첫BYTE가 공유하는 실제4바이트다.</summary>
    private readonly byte[] _battleTerritory = new byte[4];
    /// <summary>005c85d0의 전체DWORD다.</summary>
    public UIntBox BattleTerritory { get; }
    /// <summary>같은 DWORD의 첫BYTE만 읽고 쓰며 상위3바이트를 보존한다.</summary>
    public byte BattleByte { get => _battleTerritory[0]; set => _battleTerritory[0] = value; }
    /// <summary>원문 Zone의 같은 앞6바이트다.</summary>
    public Memory<byte> Zone => _zone.AsMemory(0, 6);
    /// <summary>인접 보존WORD를 포함하는 실제8바이트다.</summary>
    public Memory<byte> ZoneStorage => _zone;
    /// <summary>State+80이 가리키는 실제 가변 Zone 할당 장부다.</summary>
    public RawPacketCopies ZonePackets { get; } = new();
    /// <summary>이전 handle의 NUL을 포함하는 고정64바이트다.</summary>
    public Memory<byte> PreviousHandle { get; } = new byte[64];
    /// <summary>설정이 읽고 쓰는 현재 Test Battle 주소64바이트다.</summary>
    public Memory<byte> TestBattleAddress { get; } = new byte[64];
    /// <summary>00546908의 실제51×144바이트 기록이다.</summary>
    public Memory<byte> MetaZoneRecords { get; } = new byte[7344];
    /// <summary>0054d4e8의 현재 TYPE별 경고255DWORD다.</summary>
    public uint[] PlayerNoMakeWarnings { get; } = new uint[255];
    /// <summary>00540a30의 실제 최초127이다.</summary>
    public IntBox InvalidTerritory { get; } = new() { Value = 127 };
    /// <summary>현재 동시 접속 모드이며 Session 교체가 초기화하지 않는다.</summary>
    public BoolBox Simulconnect { get; } = new();
    /// <summary>0054dbc4의 최초 BSS0이며 옵션 재해석만 변경한다.</summary>
    public UIntBox DebugBurst { get; } = new() { Value = 0 };
    /// <summary>005427a4의 현재 개발자 명령이다.</summary>
    public UIntBox JoinTestBattleCommand { get; } = new() { Value = 0x0200002b };
    /// <summary>00542744의 현재 ViewMeta 명령이다.</summary>
    public UIntBox ViewMetaCommand { get; } = new() { Value = 0x02000013 };
    /// <summary>00542530의 최초1인 Welcome 선택 표식이다.</summary>
    public UIntBox FirstMenuPending { get; } = new() { Value = 1 };
    /// <summary>0054da8c의 BSS0이다. 서비스 종료 검증 writer는 연결하지 않는다.</summary>
    public UIntBox UserUpdated { get; } = new() { Value = 0 };
    /// <summary>0053190c의 최초1인 현재 안내 옵션이다.</summary>
    public UIntBox TellTips { get; } = new() { Value = 1 };
    /// <summary>00531910의 최초1인 현재 안내 순번이다.</summary>
    public UIntBox TipNumber { get; } = new() { Value = 1 };
    /// <summary>0054db98의 실제 파일 감지 결과다.</summary>
    public UIntBox IntroAvailable { get; } = new() { Value = 0 };
    /// <summary>0054db90의 현재 옵션이며 Session 교체 때 초기화하지 않는다.</summary>
    public UIntBox AutoDemo { get; } = new() { Value = 0 };
    /// <summary>005c85c0의 현재 Zone 요청 표식이다.</summary>
    public UIntBox ZonePending { get; } = new() { Value = 0 };
    /// <summary>0055a420의 현재 시작 출처다.</summary>
    public UIntBox SourceType { get; } = new() { Value = 0 };
    /// <summary>005c7a74의 Login/Ready/Timer 공유 DWORD다.</summary>
    public UIntBox Begin { get; } = new() { Value = 0 };
    /// <summary>005c85a0의 metadata/cleanup 공유 표식이다.</summary>
    public UIntBox CentralCreated { get; } = new() { Value = 0 };
    /// <summary>005c85b0의 실제 진행 창 serial이다.</summary>
    public UIntBox LoginProgressSerial { get; } = new() { Value = 0 };
    /// <summary>00542528의 최초ffffffff 복구 owner다.</summary>
    public UIntBox RestoreOwner { get; } = new() { Value = uint.MaxValue };
    /// <summary>0054252c의 최초ffffffff 복구 subscriber다.</summary>
    public UIntBox RestoreSubscriber { get; } = new() { Value = uint.MaxValue };
    /// <summary>005c893c의 자동 재접속 실패 안내 표식이다.</summary>
    public UIntBox NoRestartSerial { get; } = new() { Value = 0 };
    /// <summary>00542544의 실제 최초1이며 타이머 완료/중단이 함께 쓴다.</summary>
    public UIntBox ScriptBreak { get; } = new() { Value = 1 };
    /// <summary>005c85e0의 별도 중단 예약이다.</summary>
    public UIntBox AbortPending { get; } = new() { Value = 0 };
    /// <summary>005c85cc의 독립4바이트이며 인접 BattleTerritory와 구별한다.</summary>
    public UIntBox StateAuxCC { get; } = new() { Value = 0 };
    /// <summary>005c85c8의 현재 메타 부모 모드다.</summary>
    public UIntBox MetadataMode { get; } = new() { Value = 0 };
    /// <summary>005c85c4의 실제 메타 연결 표시다.</summary>
    public UIntBox MetaConnected { get; } = new() { Value = 0 };
    /// <summary>005c85b4의 별도 연결 통지 표식이다.</summary>
    public UIntBox ConnectionNotified { get; } = new() { Value = 0 };
    /// <summary>00542534의 최초ffffffff인 선택 인덱스 캐시다.</summary>
    public UIntBox CachedPeerIndex { get; } = new() { Value = uint.MaxValue };
    /// <summary>00542538의 별도 선택 값 캐시다.</summary>
    public UIntBox CachedPeerValue { get; } = new() { Value = uint.MaxValue };
    /// <summary>0054253c의 독립a4 비교 캐시다.</summary>
    public UIntBox CachedPeerFieldA4 { get; } = new() { Value = uint.MaxValue };
    /// <summary>00542540의 독립a8 비교 캐시다.</summary>
    public UIntBox CachedPeerFieldA8 { get; } = new() { Value = uint.MaxValue };
    /// <summary>005427ec의 현재 HUD 모드 명령이다.</summary>
    public UIntBox HudModeCommand { get; } = new() { Value = 0x0200003d };
    /// <summary>00542780의 현재 빈 섬 명령이다.</summary>
    public UIntBox AddIslandCommand { get; } = new() { Value = 0x02000022 };
    /// <summary>005c85b8의 State7 CD 확인 결과다. 난수 시드가 아니다.</summary>
    public UIntBox CdPresent { get; } = new() { Value = 0 };
    /// <summary>005c85bc의 State19 별도 CD 결과다.</summary>
    public UIntBox MetaCdPresent { get; } = new() { Value = 0 };
    /// <summary>005c85e8의 별도 메타 예약이다.</summary>
    public UIntBox MetaPending { get; } = new() { Value = 0 };
    /// <summary>005c85a8의 현재 로컬 서버 검색 토큰이다.</summary>
    public UIntBox LocalServerToken { get; } = new() { Value = 0 };
    /// <summary>005c85d8의 현재 메타 갱신 예약이다.</summary>
    public UIntBox MetaRefresh { get; } = new() { Value = 0 };
    /// <summary>005c85dc의 별도 메타 재구성 예약이다.</summary>
    public UIntBox MetaRebuild { get; } = new() { Value = 0 };
    /// <summary>00531760의 실제 최초1이며 Meta 기록 정리가 읽고 쓴다.</summary>
    public UIntBox MetaFirstReset { get; } = new() { Value = 1 };
    /// <summary>005c8618의 독립 캡처/Reply 기한이다.</summary>
    public DoubleBox CaptureEnd { get; } = new();
    /// <summary>005c85f8의 연결 만료 벽시각이다.</summary>
    public DoubleBox ConnectEnd { get; } = new();
    /// <summary>005c85f0의 로그인/플레어 억제 기한이다.</summary>
    public DoubleBox LoginEnd { get; } = new();
    /// <summary>005c8610의 재접속 저장 기한이다.</summary>
    public DoubleBox ReconnectSaveEnd { get; } = new();
    /// <summary>005c8600의 별도 로컬 검색 기한이다.</summary>
    public DoubleBox LocalServerSearchEnd { get; } = new();
    /// <summary>005c8608의 별도 재송신 기한이다.</summary>
    public DoubleBox LocalServerRetryEnd { get; } = new();
    /// <summary>실제 Moviegump가 소유하는 0054dc58의 현재 영화 갱신 잠금 깊이이며 BSS0에서 시작한다. 게임 시계 정지와 구별하며 영상 중에도 idle은 실행한다.</summary>
    public int MovieUpdatePauseDepth { get; private set; }
    /// <summary>양수 깊이에서만 갱신 블록을 건너뛴다. 0 이하는 잠그지 않는다.</summary>
    public bool IsMovieUpdateLocked => MovieUpdatePauseDepth > 0;
    /// <summary>원본 00435dc0의 같은 갱신 깊이를 32비트로 가감하며 게임 시계와 합치지 않는다.</summary>
    /// <param name="paused">잠그면 참, 풀면 거짓이다.</param>
    /// <remarks>2026-11-26 1.060에서 SL-7 영화 갱신 잠금용으로 추가했다. 근거: cpppj ClientMain.cpp의 PauseMovieUpdates다. 영상 재생(R6) 없이는 잠기지 않는다.</remarks>
    public void PauseMovieUpdates(bool paused) => MovieUpdatePauseDepth = unchecked(MovieUpdatePauseDepth + (paused ? 1 : -1));
    /// <summary>기존 실제 저장 모드/주변음을 빌리고 새로운 블록/최초 PE값만 한 번 구성한다.</summary>
    public ClientLocalStateGlobals(ClientGameplayGlobals gameplay, AmbientSoundState ambient)
    {
        ArgumentNullException.ThrowIfNull(gameplay); ArgumentNullException.ThrowIfNull(ambient); _gameplay = gameplay; AmbientSound = ambient;
        State = new() { Old = _blocks.AsMemory(0, 144), Phase = _blocks.AsMemory(144, 144), Immediate = _blocks.AsMemory(288, 144), Current = _blocks.AsMemory(432, 144), Next = _blocks.AsMemory(576, 144) };
        MenuWindow = new(State.Immediate.Slice(132, 4)); BattleTerritory = new(_battleTerritory); ActivityEnd = new AmbientDeadline(ambient);
    }
}
