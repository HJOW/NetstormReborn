using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Client.Gump;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

// 같은 실제 창 전역/100행을 자동완성·공유 로그·뉴스 설정 소비자에 연결한다.
namespace Netstorm.Client;

/// <summary>ClientChatRuntime/RawMenuChat의 같은 현재 저장소와 선택 장치 부모다.</summary>
public sealed class ClientChatWindowRuntime
{
    /// <summary>창 교체 뒤에도 유지하는 실제 장기 전역이다.</summary>
    private readonly ClientChatWindowGlobals _globals;
    /// <summary>같은 실제100행 채팅 기록이다.</summary>
    private readonly ClientChatGlobals _chat;
    /// <summary>binding 위치의 실제 현재 표시 BYTE 번역이다.</summary>
    private readonly Func<string, string>? _translate;
    /// <summary>127 이상 접두 길이의 실제 원문 진단이다.</summary>
    private readonly Action<uint>? _assertion;
    /// <summary>상태만 연결하며 기록/번역/파일/설정은 선행 조회하지 않는다.</summary>
    public ClientChatWindowRuntime(ClientChatWindowGlobals globals, ClientChatGlobals chat, Func<string, string>? translate = null, Action<uint>? assertion = null)
    {
        _globals = globals ?? throw new ArgumentNullException(nameof(globals)); _chat = chat ?? throw new ArgumentNullException(nameof(chat)); _translate = translate; _assertion = assertion;
    }
    /// <summary>같은 Console callback과 장기 BSS를 사용하고 새 단어에서만 현재100행을 읽는다.</summary>
    public string? Complete(uint callback, string? word, int length, bool backward)
    {
        if (callback != 0x42e540) throw new InvalidOperationException("실제 Console의 미지원 자동완성 callback");
        return ChatNameCompletionBody.CompleteChatName(_globals.Completion, word, length, backward, new() { Translate = _translate, Records = () => _chat.Records, Assertion = _assertion });
    }
    /// <summary>실제 TextEntry의 NUL 포함 또는 명시 BYTE 단어를 같은 원문 완성 부모에 전달한다.</summary>
    public byte[]? CompleteEntry(uint callback, ReadOnlyMemory<byte>? word, int length, bool backward)
    {
        string? value = Complete(callback, word.HasValue ? Encoding.Latin1.GetString(word.Value.Span) : null, length, backward); return value is null ? null : Encoding.Latin1.GetBytes(value);
    }
    /// <summary>실제 공유128줄 객체를 같은 조각 저장소가 한 번만 빌리도록 설치한다.</summary>
    public void BindSharedLog(object owner, StyleTextBufferStore buffers)
    {
        ArgumentNullException.ThrowIfNull(owner); ArgumentNullException.ThrowIfNull(buffers);
        if (_globals.UiOwner is { } prior) { if (!ReferenceEquals(prior, owner)) throw new InvalidOperationException("ChatWindow 공유 조각 저장소 이중 대여"); return; }
        buffers.ConstructBorrowed(0x548758, _globals.Log, 128); _globals.UiOwner = owner;
    }
    /// <summary>표시 자식 정리 뒤 같은 owner만 공유 조각을 반납하고 성공 뒤 대여를 해제한다.</summary>
    public void ReleaseSharedLog(object owner, StyleTextBufferStore buffers)
    {
        if (!ReferenceEquals(_globals.UiOwner, owner)) return; buffers.Destroy(0x548758); _globals.UiOwner = null;
    }
    /// <summary>원문 공유 로그 callback을 설치하며 실제 파일0에서는 장치를 요구하지 않는다.</summary>
    public void InstallLogCallback(Func<uint, bool>? valid, ClientChatLogWrite? write)
    {
        _globals.Log.OnAppend = text =>
        {
            uint file = _globals.LogFile.Value; if (file == 0) return; if (valid is null || write is null) throw new InvalidOperationException("Chat 로그 실제 파일 장치 미연결"); if (!valid(file)) return;
            int end = text.IndexOf((byte)0); if (end < 0) throw new InvalidOperationException("Chat 로그 원문 NUL 범위 없음"); write(_globals.LogFile.Value, text[..end]);
        };
    }
    /// <summary>같은 현재 서버 이름과 실제 UTF8 Config의 조회/해석 경계를 구성한다.</summary>
    private SystemNewsTimeHooks NewsHooks(ClientNsInfoGlobals servers, ConfigInterface configuration) => new()
    {
        ServerName = () =>
        {
            ReadOnlyMemory<byte> name = servers.ResolveServerName(key => Encoding.Latin1.GetBytes((_translate ?? throw new InvalidOperationException("뉴스 서버 이름의 실제 번역 미연결"))(key)));
            return Encoding.Latin1.GetString(name.Span);
        },
        Read = key => configuration.GetInt(ExtendedText.DecodeDisplayText(Encoding.Latin1.GetBytes(key))),
        Configure = expression => ScriptCommand.ApplyScriptConfig(ExtendedText.DecodeDisplayText(Encoding.Latin1.GetBytes(expression)), configuration),
    };
    /// <summary>현재 서버별 키와 signed 설정을 같은 뉴스 DWORD에 적재한다.</summary>
    public void LoadNewsTime(ClientNsInfoGlobals servers, ConfigInterface configuration) => SystemNewsTime.Read(new(_globals.NewsTime, _globals.NewsKey), NewsHooks(servers, configuration));
    /// <summary>이미 수신한 시각을 실제 설정 해석기에 전달하며 일반 옵션 저장 시점을 유지한다.</summary>
    public void SaveNewsTime(ClientNsInfoGlobals servers, ConfigInterface configuration) => SystemNewsTime.Write(new(_globals.NewsTime, _globals.NewsKey), NewsHooks(servers, configuration));
}
/// <summary>선택된 실제 파일 토큰에 NUL 전 로그 BYTE만 작성하는 장치다.</summary>
public delegate void ClientChatLogWrite(uint file, ReadOnlySpan<byte> bytes);
