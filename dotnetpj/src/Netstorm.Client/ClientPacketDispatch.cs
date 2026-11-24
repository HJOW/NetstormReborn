using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay·Carrier/Construction·장기 CS·연결 종료·전투 시작·서버 발견·메타 geometry·챌린지 응답·Root 로그인·전투 설명·참가자 제거·참가자 이름·전투 옵션 묶음을 같은 분배기에 설치한다. 전송 설치는 R4-6, 실제 송수신 펌프는 R11에서 잇는다.
/// <summary>공통 패킷 분배기의 실제 소유자 조립이다. 장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay·Carrier/Construction·장기 CS·연결 종료·전투 시작·서버 발견·메타 geometry·챌린지 응답·Root 로그인·전투 설명·참가자 제거·참가자 이름·전투 옵션 묶음을 같은 분배기에 설치한다.</summary>
public static class ClientPacketDispatch
{
    /// <summary>같은 장기 Client 장치만 읽으며 실제 수신 UI/진단/활동 장치를 묶는다.</summary>
    /// <param name="dispatch">공통 분배기의 실제 시계/기록/종료 장치다.</param>
    /// <param name="game">같은 실제 Player/권한/현재 세션 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/Source 장부다.</param>
    /// <param name="surface">같은 실제 표면/시계/커널이다.</param>
    /// <param name="chat">창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</param>
    /// <param name="receive">실제 Chat UI 수신 입구다. 41h가 도착하면 호출된다.</param>
    /// <param name="assertion">원문 진단의 실제 선택 줄 소비자다.</param>
    /// <param name="activity">장기 연결 활성의 실제 벽시계/프레임/서버 조회다.</param>
    /// <param name="challenge">같은 Client 수명으로 유지하는 실제 CS 슬롯·할당 owner다. 0x7b·0x7c 후크 결합 수신기가 공유한다.</param>
    /// <returns>장기 기본 표·41h·로그인·설정·버전·기술·변수·Native gameplay 묶음이 설치된 실제 분배기다.</returns>
    /// <remarks>2026-11-23 0.897에서 R4-6 전송·수신 묶음용으로 추가하고 0.898에서 설정·버전·기술·변수 묶음을, 0.899에서 Native gameplay 6종 묶음을, 0.934에서 장기 기본 18행을, 0.964에서 Carrier/Construction 묶음을, 0.966에서 장기 CS 묶음(0x7b·0x7c)을, 0.969에서 연결 종료 2번을, 0.970에서 전투 시작 1a/1b를, 0.971에서 로컬 서버 발견 0x66·메타 geometry 0x26을, 0.973에서 챌린지 로그인 응답 0x1d를, 0.974에서 Root 로그인 응답 0x0f를, 0.975에서 전투 설명 0x71·참가자 제거 0x21을, 0.977에서 참가자 이름 0x88·전투 옵션 0x67을 이었다. 근거: R4-6 남음의 "공통 41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치"와 "NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출"과 InstallClientPacketDefaults의 기본 18행·Carrier/Construction 고정 F·장기 CS·2번·1a/1b·0x66·0x26·0x1d·0x0f·0x71·0x21·0x88·0x67 수신기다. 호스트는 10.78만 다루므로 판본을 고정한다. 종료/닫기/반납은 R11 수송에서 잇는다.</remarks>
    public static RawPacketDispatch Build(
        PacketDispatchHooks dispatch,
        ClientGameplayGlobals game,
        ClientConnectionGlobals connections,
        ClientSurfaceGlobals surface,
        ClientChatWindowGlobals chat,
        Action<Memory<byte>> receive,
        Action<string, string, uint> assertion,
        ConnectionActivityHooks activity,
        ClientChallengeGlobals challenge,
        Func<uint>? recordRequested = null,
        Action<string>? openRecord = null,
        Func<string, int>? config = null,
        Action<uint, string>? diagnosticLog = null,
        Action<string>? tell = null)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(chat);
        ArgumentNullException.ThrowIfNull(receive);
        ArgumentNullException.ThrowIfNull(assertion);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(challenge);
        var packet = new RawPacketDispatch(OriginalEdition.Patch1078, dispatch);
        InstallDefaults(packet, game, connections, surface, chat, receive, assertion, activity, challenge, recordRequested, openRecord, config, diagnosticLog, tell);
        return packet;
    }

    /// <summary>같은 장기 기본 표를 채우는 원문 순서로 전체 설치를 한 부모에서 호출한다. 수송 팩토리와 공유한다. 0.967에서 추가됐다.</summary>
    /// <param name="packet">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="game">같은 실제 Player/권한/현재 세션 소유자다.</param>
    /// <param name="connections">같은 실제 연결 Registry/Source 장부다.</param>
    /// <param name="surface">같은 실제 표면/시계/커널이다.</param>
    /// <param name="chat">창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</param>
    /// <param name="receive">실제 Chat UI 수신 입구다. 41h가 도착하면 호출된다.</param>
    /// <param name="assertion">원문 진단의 실제 선택 줄 소비자다.</param>
    /// <param name="activity">장기 연결 활성의 실제 벽시계/프레임/서버 조회다.</param>
    /// <param name="challenge">같은 Client 수명으로 유지하는 실제 CS 슬롯·할당 owner다.</param>
    /// <param name="recordRequested">현재 요청 DWORD를 읽는다. 없으면 기록을 열지 않는다.</param>
    /// <param name="openRecord">현재 이름으로 같은 실제 기록을 닫고 새로 연다. 없으면 명시 실패한다.</param>
    /// <param name="config">현재 설정 정수를 읽는다. suppressVersionMessage 조회에 사용한다. 없으면 명시 실패한다.</param>
    /// <param name="diagnosticLog">원문 진단 로그에 쓴다. 없으면 명시 실패한다.</param>
    /// <param name="tell">버전 차이 Tell을 실제 UI에 알린다. 없으면 명시 실패한다.</param>
    internal static void InstallDefaults(
        RawPacketDispatch packet,
        ClientGameplayGlobals game,
        ClientConnectionGlobals connections,
        ClientSurfaceGlobals surface,
        ClientChatWindowGlobals chat,
        Action<Memory<byte>> receive,
        Action<string, string, uint> assertion,
        ConnectionActivityHooks activity,
        ClientChallengeGlobals challenge,
        Func<uint>? recordRequested = null,
        Action<string>? openRecord = null,
        Func<string, int>? config = null,
        Action<uint, string>? diagnosticLog = null,
        Action<string>? tell = null)
    {
        ArgumentNullException.ThrowIfNull(packet);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(connections);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(chat);
        ArgumentNullException.ThrowIfNull(receive);
        ArgumentNullException.ThrowIfNull(assertion);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(challenge);
        ClientPacketDefaultObservers.Install18(packet, surface.Storage);
        ClientChatPacketReceiver.Install(game, surface, connections, chat, packet, receive);
        NativeLoginPacketReceivers.Install(game, connections, packet, assertion, activity);
        ClientMissionPacketDefaults.InstallMissionSendObservers(packet, surface.Storage);
        ClientMissionPacketDefaults.InstallSettings(packet, game);
        ClientMissionPacketDefaults.InstallVersionReply(packet, game, connections, surface);
        ClientTechnologyPacketDefaults.Install(packet, game, surface.Storage);
        ClientVariablePacketObservers.Install(packet, surface.Storage);
        NativeGameplayPacketReceivers.InstallWindAviary(game, packet);
        NativeGameplayPacketReceivers.InstallSunAviary(game, packet);
        NativeGameplayPacketReceivers.InstallRainAviary(game, packet);
        NativeGameplayPacketReceivers.InstallGun(game, packet);
        NativeGameplayPacketReceivers.InstallFlyer(game, packet);
        NativeGameplayPacketReceivers.InstallArcherAnimation(game, packet);
        // 원문 InstallClientPacketDefaults 순서(gameplay26F→carrier15F→construction8F→chat41h…)에서
        // carrier/construction은 chat 뒤가 아닌 gameplay 직후에 둔다. DN 기본 18행·41h 선행 순서는 유지한다.
        // 0x3f/0x5f/0x7d 송신 관찰기는 ClientVariablePacketObservers와 식이 같아 마지막 쓰기가 이긴다.
        NativeCarrierPacketReceivers.Install(packet, game, surface.Storage);
        NativeConstructionPacketReceivers.Install(packet, game, surface.Storage);
        // 같은 장기 CS만으로 묶이는 후크 결합 수신기(0x7b·0x7c)를 잇는다. 세션·메뉴·월드가 필요한 나머지 중 2번을 먼저 잇는다.
        ClientChallengePacketDefaults.Install(packet, challenge, surface.Storage);
        ClientMetaGameLogin.Install(packet, game, connections, challenge, surface.Storage);
        // 원문 앱 전역 2번 연결 종료를 같은 기본 표에 잇는다. 세션 효과는 실제 세션에 묶고 R6 본체까지 명시 실패한다. 0.969에서 추가됐다.
        ClientConnectionEndedReceiver.Install(
            packet,
            game,
            connections,
            connections.Activity,
            () => game.PlayerRecords.Records.ToArray(),
            () =>
            {
                // 실제 9번 장애 저장은 선택된 현재 세션에서만 실제 전체 owner를 보유한다. R6 본체까지 명시 실패한다.
                var session = game.RequireSession();
                using var pin = game.PinCurrentRuntime(session);
                throw new InvalidOperationException("연결 종료 9번 장애 저장 미연결(R6)");
            },
            owner =>
            {
                // 선행 flags 저장 뒤 선택된 현재 세션에서만 실제 방송 하위를 보유한다. R6 본체까지 명시 실패한다.
                var session = game.RequireSession();
                using var pin = game.PinCurrentRuntime(session);
                throw new InvalidOperationException("연결 종료 소유자 방송 미연결(R6)");
            });
        // 1a/1b 전투 시작을 같은 기본 표에 잇는다. 상태·예약·포트·구독자·검사·진단·복제·연결은 실제 장기 장치이며 기록 조회·열기는 호출자가 묶는다. 0.970에서 추가됐다.
        var battleHooks = new ClientStateBattleStartHooks
        {
            // 같은 실제 로컬 상태 블록 owner다.
            State = connections.LocalState.State,
            // 같은 실제 예약 수 owner다.
            ZonePending = connections.LocalState.ZonePending,
            // 현재 게임 포트를 읽는다.
            GamePort = () => game.GameServerPort.Value,
            // 같은 실제 자기 구독자 번호 owner다.
            Subscriber = game.Subscribers?.MySubscriber ?? throw new InvalidOperationException("전투 시작의 실제 구독자 미연결"),
            // 현재 요청 DWORD를 읽는다. 기록 owner가 없으면 열지 않는다.
            RecordRequested = recordRequested ?? (() => 0u),
            // 같은 실제 현재 검사 뷰다.
            Checking = surface.Storage.Checking(),
            // 현재 서버 여부를 읽는다.
            Server = () => game.Authority.Server,
            // 원문 진단의 실제 선택 줄 소비자다.
            Assertion = assertion,
            // 같은 실제 가변 복제 장부 토큰을 반환한다.
            Duplicate = buffer => connections.LocalState.ZonePackets.Duplicate(buffer, packet),
            // 현재 이름으로 같은 실제 기록을 닫고 새로 연다. 기록 owner가 없으면 명시 실패한다.
            OpenRecord = openRecord ?? (_ => throw new InvalidOperationException("전투 시작 기록 열기 미연결(R7)")),
            // 실제 현재 연결을 순서대로 닫는다.
            Disconnect = connections.Registry.Disconnect,
            // 1b의 현재 쓰기 뒤 같은 실제 상태 처리기를 동기 실행한다. 실제 UI State 부모가 없으므로 설치 후크에서 명시 실패한다.
            Run = () => throw new InvalidOperationException("전투 시작의 실제 UI State 부모 미연결"),
        };
        // 실제 배송 connection의 Source 선택 장치다. 비어 있으면 명시 실패한다.
        NativeBattleSource SelectSource(PacketConnection? connection) =>
            connection is null
                ? throw new InvalidOperationException("전투 시작의 실제 Source 미연결")
                : connections.PacketSources.Resolve(connection) ?? throw new InvalidOperationException("전투 시작의 실제 Source 미연결");
        // 실제 UI State 부모의 배송 분배 장치다. 메뉴가 없으므로 대여를 반납하고 명시 실패한다.
        void DispatchBattle(NativeBattleSource source, bool peer)
        {
            _ = peer;
            source.Dispose();
            throw new InvalidOperationException("전투 시작의 실제 UI State 부모 미연결");
        }
        ClientStateBattleStart.Install(packet, battleHooks, SelectSource, DispatchBattle);
        // 0x66 로컬 서버 발견을 같은 기본 표에 잇는다. 상태·토큰·기한·벽시각·주소 조회는 실제 장기 장치이며 현재 36 뒤 동기 Run만 메뉴에 묶는다. 0.971에서 추가됐다.
        ClientLocalServerDiscovery.Install(packet, new ClientLocalServerDiscoveryHooks
        {
            // 같은 실제 로컬 상태 블록 owner다.
            State = connections.LocalState.State,
            // 같은 실제 검색 토큰 owner다.
            Token = connections.LocalState.LocalServerToken,
            // 같은 실제 검색 만료 기한 owner다.
            SearchEnd = connections.LocalState.LocalServerSearchEnd,
            // 경쟁 검색의 지연 선택에서만 호출하는 현재 벽시각이다.
            Wall = activity.WallSeconds,
            // 현재 sender의 실제 주소 바이트다.
            PeerAddress = sender => connections.Registry.Address(unchecked((int)sender)),
            // 현재 36 작성 뒤에 호출하는 실제 부모다. 실제 UI State 부모가 없으므로 명시 실패한다.
            ProcessState = () => throw new InvalidOperationException("로컬 서버 발견의 실제 UI State 부모 미연결"),
        });
        // 0x26 메타 geometry를 같은 기본 표에 잇는다. 좌표·모드·진단은 실제 장기 장치이며 화면 한계만 후속에 묶는다. 0.971에서 추가됐다.
        ClientMetaGeometry.Install(packet, new ClientMetaGeometryHooks
        {
            // 같은 실제 최초 배치 좌표 owner다.
            Geometry = challenge.Geometry,
            // 005467d4의 같은 현재 모드 owner다.
            Mode = challenge.GeometryMode,
            // 실제 화면 논리 크기와 메타 크기로 현재 한계를 둔다. 화면 장치가 없으므로 명시 실패한다.
            Limits = (_, _) => throw new InvalidOperationException("메타 geometry의 실제 화면 한계 미연결(R5)"),
            // 원문 진단의 실제 선택 줄 소비자다.
            Assertion = assertion,
        });
        // 0x1d 챌린지 로그인 응답을 같은 기본 표에 잇는다. 상태·구독자는 실제 장기 장치이며 진단·설정·알림·Run은 호출자가 묶는다. 0.973에서 추가됐다.
        // 원문 진단 로그·설정 조회가 없으면 받는 즉시 명시 실패한다.
        Action<uint, string> challengeLog = diagnosticLog ?? ((_, _) => throw new InvalidOperationException("챌린지 로그인 응답의 실제 진단 로그 미연결"));
        // 버전 차이 확인의 실제 설정·진단·알림 장치다. 없으면 확인 즉시 명시 실패한다.
        ClientMetaVersionHooks versionHooks = new()
        {
            Config = config ?? (_ => throw new InvalidOperationException("챌린지 로그인 응답의 실제 설정 미연결")),
            Log = challengeLog,
            Tell = tell ?? (_ => throw new InvalidOperationException("챌린지 로그인 응답의 실제 Tell 미연결")),
        };
        ClientChallengeLoginReply.Install(packet, new ClientChallengeLoginReplyHooks
        {
            // 원문 수준 1dh의 현재 응답코드 문구를 기록한다.
            Log = text => challengeLog(0x1d, text),
            // 004b4160의 실제 메타 버전 검사다.
            Version = (major, minor) => ClientMetaVersion.Check(major, minor, versionHooks),
            // 현재 블록을 직접 28/12로 쓴 뒤 실제 동기 Run을 호출한다. 실제 UI State 부모가 없으므로 명시 실패한다.
            Run = () => throw new InvalidOperationException("챌린지 로그인 응답의 실제 UI State 부모 미연결"),
            // 같은 실제 로컬 상태 블록 owner다.
            State = connections.LocalState.State,
            // 같은 실제 구독자 번호 owner다.
            Subscriber = game.Subscribers?.MySubscriber ?? throw new InvalidOperationException("챌린지 로그인 응답의 실제 구독자 미연결"),
        });
        // 0x0f Root 로그인을 같은 기본 표에 잇는다. 상태·기록·갱신·정리·진단은 실제 장기 장치이며 버전 확인은 0x1d와 같은 호출자 장치를 공유하고 서버 값·목록·저장은 후속에 묶는다. 0.974에서 추가됐다.
        ClientRootLoginReply.Install(packet, new ClientRootLoginReplyHooks
        {
            // 같은 실제 로컬 상태 블록 owner다.
            State = connections.LocalState.State,
            // 00546908의 같은 실제 51×144BYTE 기록이다.
            Zones = connections.LocalState.MetaZoneRecords,
            // 같은 실제 현재 갱신 예약 owner다.
            Refresh = connections.LocalState.MetaRefresh,
            // 004b4160의 실제 버전 확인이다. 0x1d와 같은 호출자 장치를 공유한다.
            Version = (major, minor) => ClientMetaVersion.Check(major, minor, versionHooks),
            // 현재 마커 삭제/표 정리 부모다. 비영 마커 삭제에서만 실제 세션을 pin한다.
            ClearZones = () => RawMetaZoneCleanup.ClearRecords(
                connections.LocalState.MetaFirstReset,
                connections.LocalState.MetaZoneRecords,
                index => RawMetaZoneCleanup.ClearMarker(
                    connections.LocalState.MetaZoneRecords,
                    index,
                    (sid, flags) =>
                    {
                        // 비영 마커의 실제 가상 삭제에서만 현재 전체 Runtime을 선택한다.
                        var session = game.RequireSession();
                        using var pin = game.PinCurrentRuntime(session);
                        if (!ReferenceEquals(session.Pool, surface.Storage.Pool) || !ReferenceEquals(session.LocalState, connections.LocalState))
                        {
                            throw new InvalidOperationException("메타구역 삭제의 현재 Session/Pool/Client 불일치");
                        }

                        if (sid >= surface.Storage.Pool.Capacity || sid > ushort.MaxValue)
                        {
                            throw new ArgumentOutOfRangeException(nameof(sid), "메타구역 DWORD SID의 현재 Pool 물리 범위");
                        }

                        _ = session.DestroyForState((int)sid, flags);
                    })),
            // phase+44 서버 행 값을 갱신한다. 실제 NsInfo 저장소가 없으므로 명시 실패한다.
            Checksum = _ => throw new InvalidOperationException("Root 로그인 응답의 실제 서버 값 미연결(R6)"),
            // 현재 서버 목록에 행을 추가한다. 실제 NsInfo 저장소가 없으므로 명시 실패한다.
            Server = _ => throw new InvalidOperationException("Root 로그인 응답의 실제 서버 목록 미연결(R6)"),
            // 현재 서버 목록을 사용자 홈에 저장한다. 실제 저장 부모가 없으므로 명시 실패한다.
            SaveServers = () => throw new InvalidOperationException("Root 로그인 응답의 실제 서버 목록 저장 미연결(R7)"),
            // 원문 진단의 실제 선택 줄 소비자다.
            Assertion = assertion,
        });
        // 0x71 전투 설명을 같은 기본 표에 잇는다. 분배·CS·진단은 실제 장기 장치이며 설명 본문의 표시 장치는 후속에 묶는다. 0.975에서 추가됐다.
        ClientMetaBattleDescriptionReceiver.Install(
            packet,
            challenge,
            surface.Storage,
            (sender, body) => NativeMetaBattleDescription.Receive(challenge, body, new NativeMetaBattleDescriptionHooks
            {
                // 수준9의 무효 전투·설명 길이 진단이다.
                Log = text => challengeLog(9, text),
                // 현재 전투 기록+20H SID의 실제 가상88이며 복사·정제 뒤에 호출한다. 실제 표시 장치가 없으므로 명시 실패한다.
                RefreshObject = _ => throw new InvalidOperationException("전투 설명의 실제 표시 갱신 미연결(R6)"),
                // 00479c40의 현재 packet 전투 번호 갱신이다. 실제 표시 장치가 없으므로 명시 실패한다.
                UpdateBattle = _ => throw new InvalidOperationException("전투 설명의 실제 전투 번호 갱신 미연결(R6)"),
            }));
        // 0x21 메타 참가자 제거를 같은 기본 표에 잇는다. CS·모드·검사·진단·로그는 실제 장기 장치이며 전투·Chat·하차·NSInfo 표시는 후속에 묶는다. 0.975에서 추가됐다.
        ClientMetaParticipantRemove.Install(packet, new ClientMetaParticipantRemoveHooks
        {
            // 같은 실제 CS 참가자40 기록 owner다.
            Challenge = challenge,
            // 같은 실제 메타맵 모드 owner다. 거짓이면 수신 전체를 건너뛴다.
            OwnerModeChallenge = () => surface.Storage.OwnerMode.Challenge,
            // 같은 실제 현재 검사 뷰다.
            Checking = surface.Storage.Checking(),
            // 원문 진단의 실제 선택 줄 소비자다.
            Assertion = assertion,
            // 수준9의 실제 진단 로그 장치다.
            Log = text => challengeLog(9, text),
            // 같은 실제 전투 표시 갱신 장치다. 실제 표시 장치가 없으므로 명시 실패한다.
            Battle = _ => throw new InvalidOperationException("메타 참가자 제거의 실제 전투 표시 미연결(R6)"),
            // 실제 Chat 채널 망각 장치다. 실제 Chat 장치가 없으므로 명시 실패한다.
            Chat = _ => throw new InvalidOperationException("메타 참가자 제거의 실제 Chat 망각 미연결(R6)"),
            // 같은 실제 섬 장비 하차 장치다. 실제 월드 장치가 없으므로 명시 실패한다.
            Leave = (_, _) => throw new InvalidOperationException("메타 참가자 제거의 실제 섬 장비 하차 미연결(R6)"),
            // 같은 실제 NSInfo 갱신 장치다. 실제 NSInfo 장치가 없으므로 명시 실패한다.
            RefreshInfo = () => throw new InvalidOperationException("메타 참가자 제거의 실제 NSInfo 갱신 미연결(R6)"),
        });
        // 0x88 참가자 이름을 같은 기본 표에 잇는다. CS 조회·복사는 실제 장기 장치이며 번역·등급·표시·NSInfo는 후속에 묶는다. 0.977에서 추가됐다.
        ClientMetaParticipantName.Install(packet, new ClientMetaParticipantNameHooks
        {
            // 같은 실제 CS 참가자40 기록 owner다.
            Challenge = challenge,
            // 원문 표시 부호화의 실제 번역 장치다. 실제 번역 장치가 없으므로 명시 실패한다.
            Translate = _ => throw new InvalidOperationException("메타 참가자 이름의 실제 번역 미연결(R6)"),
            // 같은 Core 10BYTE의 실제 등급 합계 장치다. 실제 등급 장치가 없으므로 명시 실패한다.
            Weight = _ => throw new InvalidOperationException("메타 참가자 이름의 실제 등급 합계 미연결(R6)"),
            // 같은 실제 참가자 표시 갱신 장치다. 실제 표시 장치가 없으므로 명시 실패한다.
            Update = (_, _) => throw new InvalidOperationException("메타 참가자 이름의 실제 표시 갱신 미연결(R6)"),
            // 같은 실제 NSInfo 갱신 장치다. 실제 NSInfo 장치가 없으므로 명시 실패한다.
            RefreshInfo = () => throw new InvalidOperationException("메타 참가자 이름의 실제 NSInfo 갱신 미연결(R6)"),
        });
        // 0x67 전투 옵션을 같은 기본 표에 잇는다. 게임·CS·저장소·진단·로그는 실제 장기 장치이며 음향·NSInfo·재생성은 후속에 묶는다. 0.977에서 추가됐다.
        ClientMetaBattleOptions.Install(packet, new ClientMetaBattleOptionsHooks
        {
            // 같은 실제 장기 게임 전역 owner다.
            Game = game,
            // 같은 실제 CS owner다.
            Challenge = challenge,
            // 같은 실제 표면 저장소 owner다.
            Storage = surface.Storage,
            // 같은 실제 로컬 상태 owner다.
            Local = connections.LocalState,
            // 원문 진단의 실제 선택 줄 소비자다.
            Assertion = assertion,
            // 원문 수준9의 진단 문자열 장치다.
            Log = text => challengeLog(9, text),
            // 실제 guy.wav 재생 장치다. 실제 음향 장치가 없으므로 명시 실패한다.
            GuySound = () => throw new InvalidOperationException("메타 전투 옵션의 실제 음향 미연결(R6)"),
            // 현재 NSInfo 갱신 장치다. 실제 NSInfo 장치가 없으므로 명시 실패한다.
            RefreshInfo = () => throw new InvalidOperationException("메타 전투 옵션의 실제 NSInfo 갱신 미연결(R6)"),
            // 최종 전투·비미션·저장 관문을 모두 통과할 때 실제 Uber 재생성 장치다. 실제 메뉴 장치가 없으므로 명시 실패한다.
            Reconnect = () => throw new InvalidOperationException("메타 전투 옵션의 실제 Uber 재생성 미연결(R11)"),
        });
    }
}
