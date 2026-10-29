using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 기본 버튼의 우클릭 도움말 메뉴 요청 영역이다.

/// <summary>원본 본문/사건 전역과 현재 %sHelp 임시 주제.</summary>
public sealed class GumpHelpState
{
    /// <summary>현재 본문 문자열 토큰.</summary>
    public uint Body { get; set; }
    /// <summary>현재 도움말 사건 ID.</summary>
    public uint Event { get; set; }
    /// <summary>원본 바이트를 일대일 문자로 보존한 도움말 주제.</summary>
    public string Topic { get; set; } = "";
}

/// <summary>같은 현재 저장소의 문자열/메뉴/부착/소리 경계다.</summary>
public sealed class GumpHelpHooks
{
    /// <summary>안정된 기본/파생 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>NUL을 포함할 수 있는 원본 단일 바이트 문자열. 호스트는 Latin1 일대일 문자로 공급한다.</summary>
    public Func<uint, string>? Text { get; set; }
    /// <summary>제어 메뉴 구성을0 인자로 시작한다.</summary>
    public Action<uint>? BeginMenu { get; set; }
    /// <summary>현재 제목 문자열 토큰을 설치한다.</summary>
    public Action<uint>? Title { get; set; }
    /// <summary>본문/사건/주제/1/1 및 패치0·CD없음 인자의 메뉴 항목.</summary>
    public Action<uint, uint, string, uint, uint, uint?>? Entry { get; set; }
    /// <summary>signed 좌표와 현재 등록 루트에 메뉴를 붙인다.</summary>
    public Action<int, int, uint>? AttachMenu { get; set; }
    /// <summary>opengump.wav 실제 재생.</summary>
    public Action<string>? Sound { get; set; }
}

/// <summary>조건·주제 조립과 각 콜백 뒤 현재 필드/전역/루트를 읽는 원본 기본 도움말 처리기다.
/// 근거: cpppj/src/client/GumpHelp.h/.cpp, 00464990/CD00493720.</summary>
public sealed class GumpHelp
{
    /// <summary>같은 등록/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 본문/사건/임시 주제.</summary>
    private readonly GumpHelpState _state;
    /// <summary>실제 메뉴/문자열/소리 경계.</summary>
    private readonly GumpHelpHooks _hooks;
    /// <summary>도달한 분기의 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("Gump 도움말 메뉴/문자열/소리 경계 필요");
    /// <summary>기본 노드 조회만 생성 시 필수다.</summary>
    public GumpHelp(GumpCreateState create, GumpHelpState state, GumpHelpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); _create = create; _state = state; _hooks = hooks;
    }
    /// <summary>오른쪽 누름·현재 도움말 토큰이 있을 때만 주제→시작→제목→항목→부착→소리 순으로 메뉴를 요청한다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        GumpCreateNode node = (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 도움말 토큰 없음");
        if ((eventCode & 0x03ffffff) != InputCode.RightButton || (eventCode & InputCode.Release) != 0 || node.AlternateTextPointer == 0) return 0;
        string text = Require(_hooks.Text)(node.AlternateTextPointer); int end = text.IndexOf('\0'); _state.Topic = (end < 0 ? text : text[..end]) + "Help";
        Require(_hooks.BeginMenu)(0); Require(_hooks.Title)(node.TextPointer != 0 ? node.TextPointer : node.AlternateTextPointer);
        uint? extra = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0U : null; Require(_hooks.Entry)(_state.Body, _state.Event, _state.Topic, 1, 1, extra);
        Require(_hooks.AttachMenu)(unchecked((int)first), unchecked((int)second), _create.Entries[0]); Require(_hooks.Sound)("opengump.wav"); return 1;
    }
    /// <summary>기본 버튼에서 도달한 나머지 사건을 실제 원본 도움말로 연결한다.</summary>
    public static void BindGumpHelp(ButtonGumpHooks buttons, GumpHelp help) => buttons.BaseEvent = help.Handle;
}
