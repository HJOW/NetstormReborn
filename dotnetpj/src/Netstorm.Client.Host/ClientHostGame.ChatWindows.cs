using System.Globalization;
using System.Text;

// 같은 장기 Root에 전체 실제 Chat 공장을 설치한다.
namespace Netstorm.Client.Host;

/// <summary>UberHud의 실제 현재 Client/TYPE/자식 공장 연결이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>메뉴/State 교체가 공유하는 동일 실제 Chat UI 소유자다.</summary>
    private HostChatWindows? _nativeChatWindows;
    /// <summary>현재 같은 Console/로그/controlled 메뉴의 실제 공장이다.</summary>
    public HostChatWindows NativeChatWindows => _nativeChatWindows ?? throw new InvalidOperationException("실제 Chat UI 공장 미설치");
    /// <summary>현재 Root/표면/manager/커서 주소만 사용 시점에 조회한다.</summary>
    private uint ReadNativeChatWindowGlobal(uint address) => address switch
    {
        0x541348 => (uint)AssetTables.TypeTable.Types.Count,
        0x594fc8 => _surfaceGlobals.Storage.OwnerMode.Challenge ? 1u : 0,
        0x594fbc => _missionSession?.BattleActive() ?? 0,
        0x548714 => _chatManagerGlobals.Current.Value,
        0x563e38 => (_mainMenu ?? throw new InvalidOperationException("Chat 실제 Root 트리 없음")).Tree.Create.Entries[0],
        0x5cae94 => unchecked((uint)_gameplay.Cursor.ScreenX), 0x5cae98 => unchecked((uint)_gameplay.Cursor.ScreenY),
        0x542944 => 0x02000094,
        _ => throw new InvalidOperationException("Chat 실제 Client 전역 미연결:" + address.ToString("x", CultureInfo.InvariantCulture)),
    };
    /// <summary>이미 연결한 같은 장기 장치에 공장을 설치한다.표시 창 생성/Root 입력 호출자는 별도다.</summary>
    private void InitializeNativeChatWindows()
    {
        var menu = _mainMenu ?? throw new InvalidOperationException("Chat 공장의 실제 공통 트리 없음");
        var text = _htmlText ?? throw new InvalidOperationException("Chat 공장의 실제 조각 저장소 없음");
        var entry = _textEntry ?? throw new InvalidOperationException("Chat 공장의 실제 Console 없음");
        var menus = _htmlMenus ?? throw new InvalidOperationException("Chat 공장의 실제 controlled 메뉴 없음");
        var runtime = _nativeChatEditing ?? throw new InvalidOperationException("Chat 공장의 실제 장기 편집/로그 없음");
        var startup = _startup ?? throw new InvalidOperationException("Chat 공장의 실제 시작 설정 없음");
        var baseHandle = menu.Tree.EventHooks.Handle;
        _nativeChatWindows = new(menu, _htmlWindows ?? throw new InvalidOperationException("Chat 공장의 HTML 수명 없음"), text, entry, menus, _driver.Screen,
            _chatWindowGlobals, _dotCommandGlobals, runtime, NativeChatGeometry, new()
            {
                Read = ReadNativeChatWindowGlobal, Translate = TranslateNativeChatText, Text = token => _chatWindowGlobals.Strings[token],
                FrameCount = type => NativeTextPictures.Type(unchecked((int)type)).FrameCount,
                FrameHeight = (type, frame) => NativeTextPictures.FrameHeight(unchecked((int)type), unchecked((int)frame)) ?? throw new InvalidOperationException("Chat 현재 실제 SHP 프레임 없음"),
                Recipient = CheckNativeChatRecipients, Resolve = ResolveNativeChatIdentity,
                SendChat = (manager, value) => SubmitNativeChat(manager, Encoding.Latin1.GetBytes(value + '\0')),
                Config = startup.Configuration.GetInt,
                BaseHandle = (token, code, first, second) => (baseHandle ?? throw new InvalidOperationException("Chat 실제 기본 입력 미설치"))(token, code, first, second),
                Diagnostic = (level, format, value) => ErrorLog.Note("Chat 종류" + level.ToString(CultureInfo.InvariantCulture) + ": " + format.Replace("%d", unchecked((int)value).ToString(CultureInfo.InvariantCulture), StringComparison.Ordinal)),
                Quit = (code, first, second) => _driver.Input.Push(code, unchecked((int)first), unchecked((int)second)),
                Assertion = line => ErrorLog.RecordAssertion("Chat 원문 진단", ".\\ChatWindow.cpp", line),
            });
    }
}
