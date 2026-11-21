using Netstorm.Core.Original;
using Netstorm.Client.Chat;
using Netstorm.Client.Gump;

// Chat UI와 공유 로그/완성/수신 상태를 같은 실제 Client 수명에 보관한다.
namespace Netstorm.Client;

/// <summary>ClientChatWindowGlobals.h의 장기 UI/수신/완성 및 호스트 문자열 소유자다.</summary>
public sealed partial class ClientChatWindowGlobals
{
    /// <summary>표시 root의 BSS0이며 현재0에서는 입력줄 장치를 조회하지 않는다.</summary>
    public UIntBox Current => Window.Current;
    /// <summary>현재 root/등록/도움말 후보를 창 교체 뒤에도 유지하는 실제 상태다.</summary>
    public ChatWindowState Window { get; } = new();
    /// <summary>00548758 공유 객체 자체다.128줄의 실제 생성은 같은 조각/글꼴 저장소가 연결될 때 수행한다.</summary>
    public StyleTextBufferState Log { get; } = new();
    /// <summary>0054874c의 BSS0 파일 토큰이며0에서는 파일 장치를 요구하지 않는다.</summary>
    public UIntBox LogFile { get; } = new();
    /// <summary>00548710의 같은 수신/중계 마스크다.</summary>
    public UIntBox ReceivedMask { get; } = new();
    /// <summary>00548748의 같은 뉴스 시각 DWORD다.</summary>
    public UIntBox NewsTime { get; } = new();
    /// <summary>00548790의 실제128BYTE이며 뒤쪽 완성 상태와 분리한다.</summary>
    public byte[] NewsKey { get; } = new byte[128];
    /// <summary>객체 메뉴/수신이 공유하는 같은 장기 구독자 무시 목록이다.</summary>
    public List<uint> Ignored { get; } = [];
    /// <summary>동맹 요청의 현재60초/3회 제한과9행 상태를 같은 Client에 보관한다.</summary>
    public AllianceRequestState AllianceRequests { get; } = new();
    /// <summary>0054db18의 BSS0 전환 요청이며 표시 여부 캐시가 아니다.</summary>
    public IntBox InputLineOff { get; } = new();
    /// <summary>00548810부터의 같은 실제 BSS 후보/접두/순환 물리다.</summary>
    public ChatNameCompletionState Completion { get; } = new();
    /// <summary>공유 조각/글꼴 저장소를 빌린 호스트 객체이며 Native 게임 값이 아니다.</summary>
    public object? UiOwner { get; set; }
    /// <summary>메뉴가 빌린 안정된 BYTE 문자열의 호스트 토큰 표다.</summary>
    public Dictionary<uint, string> Strings { get; } = [];
    /// <summary>같은 문자열의 중복 할당을 피하는 역조회 표다.</summary>
    private readonly Dictionary<string, uint> _stringTokens = new(StringComparer.Ordinal);
    /// <summary>입력 고리/Help와 구별되는 최초 비영 호스트 별칭이다.</summary>
    private uint _nextString = 0x71000000;
    /// <summary>입력 고리를 쓰지 않고 같은 문자열을 같은 안정된 Client 토큰으로 게시한다.</summary>
    public uint Store(string text)
    {
        ArgumentNullException.ThrowIfNull(text); if (_stringTokens.TryGetValue(text, out uint found)) return found;
        if (_nextString == 0x7fffffff) throw new OverflowException("Chat 문자열 주소 별칭 소진");
        uint token = _nextString++; Strings.Add(token, text); _stringTokens.Add(text, token); return token;
    }
    /// <summary>같은 현재 전환 요청을 기본값으로 옵션을 읽으며 누락 키는 현재값을 보존한다.</summary>
    public void ReadOptions(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration); int value = InputLineOff.Value; configuration.ReadInt("inputLineOff", ref value); InputLineOff.Value = value;
    }

}
