// 같은 Root의 실제 채팅 창·관계표·설정·음향과 뉴스 생성기를 수신에 연결한다.
using Netstorm.Net;

namespace Netstorm.Client.Host;

/// <summary>UberHud.inc의 실제 UI 수신 소비자와 Root 수명 결선이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>Session 교체 뒤에도 같은 현재 장기 소유자를 조회하는 실제 UI 수신 부모다.</summary>
    private HostChatReceive? _nativeChatReceive;
    /// <summary>이미 생성한 실제 창/설정/Identity/HTML 장치를 연결한다. 공통 패킷 표의 설치는 별도 부모다.</summary>
    private void InitializeNativeChatReceive()
    {
        var startup = _startup ?? throw new InvalidOperationException("Chat 수신 실제 시작 설정 없음");
        var menu = _mainMenu ?? throw new InvalidOperationException("Chat 수신 실제 공통 트리 없음");
        _nativeChatReceive = new(_gameplay, _surfaceGlobals, _subscriberGlobals, _chatWindowGlobals, startup.Configuration,
            subscriber => (_nativeChatIdentity ?? throw new InvalidOperationException("Chat 수신 실제 Identity 없음")).FindSenderOwner(subscriber), menu.Tree, _driver.Screen, NativeChatWindows,
            () => (_missionSession ?? throw new InvalidOperationException("Chat 관계 관전자의 현재 월드 없음")).HiddenPlayers, new()
            {
                Sound = name => (_missionAudio ?? throw new InvalidOperationException("Chat 실제 장기 음향 장치 없음")).PlayGlobal(name),
                SaveNewsTime = SaveNativeChatNewsTime,
                News = (body, width, height) =>
                {
                    _ = NativeMapView();
                    var windows = _htmlWindows ?? throw new InvalidOperationException("Chat 뉴스 실제 HTML 소유자 없음");
                    var factory = _htmlFactory ?? throw new InvalidOperationException("Chat 뉴스 실제 HTML 생성기 없음");
                    windows.SetMapView(NativeMapView); factory.Factory.Show(body, 0, width, height); _menuDirty = true;
                },
                Log = (level, text) => ErrorLog.Note("Chat 원문 진단 " + level.ToString(System.Globalization.CultureInfo.InvariantCulture) + ": " + text),
            });
    }
    /// <summary>공통41h 수신기와 로컬 fallback이 같은 현재 UI/패킷 물리를 사용한다.</summary>
    public void ReceiveNativePacketChat(Memory<byte> packet) => (_nativeChatReceive ?? throw new InvalidOperationException("실제 Root Chat 수신 미설치")).Receive(packet);
    /// <summary>R11 송수신 펌프가 소비하는 실제 패킷 분배기다.</summary>
    public RawPacketDispatch PacketDispatch => _packetDispatch ?? throw new InvalidOperationException("패킷 분배기 미설치");
    /// <summary>공통 패킷 분배기에 41h·로그인·설정·버전·기술·변수 묶음을 설치한다. 전송 설치는 R4-6, 실제 송수신 펌프는 R11에서 잇는다.</summary>
    /// <remarks>2026-11-23 0.897·0.898에서 R4-6 전송·수신 묶음용으로 추가했다. 근거: R4-6 남음의 "공통 41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치"와 "NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출"이다. 0.970에서 실제 기록 조회·열기를 묶어 1a/1b의 기록 갈래를 Root에 연결했고, 0.973에서 실제 설정 조회·진단 로그를 묶어 0x1d의 진단·버전 갈래를 Root에 연결했으며, 0.974에서 실제 송신 기록·Player 조회·지연 분포를 묶어 수송 팩토리와 같은 후크 모양으로 맞췄고, 0.981에서 실제 NSInfo 서버 목록·저장을 묶어 0x0f의 서버 목록 갈래를 Root에 연결했다(Tell·Run·메뉴는 후속). 종료·닫기·반납은 수송이 없어 명시 거부한다.</remarks>
    private void InstallClientPacketDispatch()
    {
        _packetDispatch = ClientPacketDispatch.Build(
            new PacketDispatchHooks
            {
                Now = () => unchecked((uint)_clientClock.Frame.Value.Number),
                Record = (connection, packet, length) => PacketRecordOwner.Write(false, connection, packet, length),
                Terminate = _ => throw new InvalidOperationException("패킷 종료 미연결(R11)"),
                Close = _ => throw new InvalidOperationException("패킷 닫기 미연결(R11)"),
                Release = _ => throw new InvalidOperationException("패킷 반납 미연결(R11)"),
                // 실제 송신 복사/쓰기 뒤 방향1 기록이다. 수송 backend가 붙기 전까지 호출되지 않는다.
                Sent = (connection, packet, length) => PacketRecordOwner.Write(true, connection, packet, length),
                // 재할당 뒤에도 같은 현재 Player9 기록을 조회한다.
                Players = () => _gameplay.PlayerRecords.Records,
                // 실제 Client 장기 분포/벽시각/CRT를 TCP/UDP Poll만 선택한다. 수송 backend가 붙기 전까지 호출되지 않는다.
                Delay = _connections.PacketDelays(),
            },
            _gameplay, _connections, _surfaceGlobals, _chatWindowGlobals, ReceiveNativePacketChat,
            (expression, file, line) => _surfaceGlobals.Storage.AssertDisplay(expression, file, line),
            new ConnectionActivityHooks
            {
                WallSeconds = () => _clock.NowSeconds,
                FrameNumber = () => unchecked((uint)_clientClock.Frame.Value.Number),
                Server = () => _gameplay.Authority.Server,
                EraseStatus = (_, _) => throw new InvalidOperationException("연결 활성 정리 미연결(R11)"),
            },
            _challenge,
            () => PacketRecordOwner.Requested.Value,
            PacketRecordOwner.Open,
            key => (_startup ?? throw new InvalidOperationException("Chat 수신 실제 시작 설정 없음")).Configuration.GetInt(key),
            (code, text) => ErrorLog.Note($"원문 진단 0x{code:X}: {text}"),
            null,
            _nsInfoGlobals,
            SaveRootServerList);
    }
}
