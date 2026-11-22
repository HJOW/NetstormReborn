// 같은 Root의 실제 채팅 창·관계표·설정·음향과 뉴스 생성기를 수신에 연결한다.
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
}
