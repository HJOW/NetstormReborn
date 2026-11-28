using Netstorm.Core.Original;

// 같은 Client의 감지/현재/공개 언어 번호를 Scene/Session보다 오래 유지한다.
namespace Netstorm.Client;

/// <summary>0054db58/005435b4/00542384의 서로 다른 원문 물리다.</summary>
public sealed class ClientLanguageGlobals
{
    /// <summary>최초 OS 감지 번호이며 설정/Session 변경으로 다시 감지하지 않는다.</summary>
    public UIntBox Detected { get; } = new();
    /// <summary>첫 적재 전에 쓰는 현재 번호이며 최초-1이다.</summary>
    public IntBox Current { get; } = new() { Value = -1 };
    /// <summary>두 적재 뒤 공개하는 별도 번호이며 최초-1이다.</summary>
    public IntBox Published { get; } = new() { Value = -1 };
    /// <summary>최초 해석 진입에서 켜는 호스트 구독 관문이다.</summary>
    public bool OptionsReady { get; internal set; }
}

/// <summary>실제 Config 변경과 같은 언어 전역/용어표 적재를 연결하고 자신의 구독만 반납한다.</summary>
public sealed class ClientLanguageOptions : IDisposable
{
    /// <summary>전체 Client와 공유하는 같은 원문 번호 저장소다.</summary>
    private readonly ClientLanguageGlobals _globals;
    /// <summary>현재 설정과 실제 LoadLanguage가 공유하는 Registry다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>이미 반납한 구독의 재해석을 거부한다.</summary>
    private bool _disposed;
    /// <summary>최초 해석을 정상 완료한 뒤 자신의 변경 구독을 설치한다.</summary>
    public ClientLanguageOptions(ClientLanguageGlobals globals, ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(configuration);
        _globals = globals; _configuration = configuration;
        Interpret(); configuration.OnChanged += Interpret;
    }
    /// <summary>원문 ready 표식 뒤 현재 설정→현재 번호→실제 정규/raw 적재→공개 번호 순서로 실행한다.</summary>
    public void Interpret()
    {
        ObjectDisposedException.ThrowIf(_disposed, this); _globals.OptionsReady = true;
        LanguageOptions.Apply(_globals.Detected.Value, _globals.Current, _globals.Published, new()
        {
            Read = () => { string name = ""; _configuration.ReadString("currentLanguage", ref name); return name; },
            Load = _configuration.LoadLanguage,
        });
    }
    /// <summary>다른 Config 구독자와 현재 언어/용어표를 보존하며 자기 구독만 해제한다.</summary>
    public void Dispose() { if (_disposed) return; _configuration.OnChanged -= Interpret; _disposed = true; }
}
