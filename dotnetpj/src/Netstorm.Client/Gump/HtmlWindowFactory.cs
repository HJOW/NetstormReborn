using System.Text;

namespace Netstorm.Client.Gump; // 원본 전체 HTML 창의 치환/두 번 파서/메타/본문/모달 순서를 수행한다.

/// <summary>같은 실제 창/파서/Config/공통 UI 장치의 선택 경계.</summary>
public sealed class HtmlWindowFactoryHooks
{
    /// <summary>실제 전체 저장소의 할당/생성.0은 원문 생성 실패다.</summary>
    public Func<uint, uint, uint>? Create { get; set; }
    /// <summary>같은 현재 실제 부모.</summary>
    public Func<uint, HtmlWindowGumpNode?>? Node { get; set; }
    /// <summary>현재 원시 BYTE 문자열의 지정 용량 Config 치환.</summary>
    public Func<string, uint, string>? Substitute { get; set; }
    /// <summary>현재 OkButton 설정 조회.</summary>
    public Func<string>? Fallback { get; set; }
    /// <summary>두 호출이 공유하는 상태와 현재 부모 링크 영역의 실제 파서.</summary>
    public Func<HtmlTextParserState, uint, string, uint, string>? Parse { get; set; }
    /// <summary>실제 mutable8192BYTE 메타 처리.</summary>
    public Func<uint, Memory<byte>, uint>? Metadata { get; set; }
    /// <summary>현재 벽 시각.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>실제 본문 생성.</summary>
    public Action<uint, string>? Body { get; set; }
    /// <summary>현재 전체 Fit 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>같은 공통 모달 설정.</summary>
    public Action<uint, bool>? Modal { get; set; }
    /// <summary>공통 자식 우선 지연 삭제.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>현재 화면/루트 전역 DWORD.</summary>
    public Func<uint, uint>? Read { get; set; }
}
/// <summary>cpppj/src/client/HtmlWindowFactory.h/.cpp의004cf270 전체 생성기.</summary>
public sealed class HtmlWindowFactory
{
    /// <summary>원문 각 지역 출력/부모 링크의 실제 용량.</summary>
    private const int Capacity = 8192;
    /// <summary>Client의 같은 장기 전역.</summary>
    private readonly HtmlWindowGumpState _state;
    /// <summary>같은 실제 장치를 빌린다.</summary>
    private readonly HtmlWindowFactoryHooks _hooks;
    /// <summary>실제 도달한 장치만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 전체 생성 실제 장치 미연결");
    /// <summary>장기 전역만 빌리고 장치는 원문 분기에서 검증한다.</summary>
    public HtmlWindowFactory(HtmlWindowGumpState state, HtmlWindowFactoryHooks hooks) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); _state = state; _hooks = hooks; }
    /// <summary>현재 같은 실제 부모 주소를 읽는다.</summary>
    private HtmlWindowGumpNode Node(uint token) => (token != 0 ? Require(_hooks.Node)(token) : null) ?? throw new InvalidOperationException("HTML 전체 생성 현재 부모 없음");
    /// <summary>원문 첫 NUL 뒤 BYTE는 처리하지 않는다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end >= 0 ? text[..end] : text; }
    /// <summary>현재 실제 배열의 첫 NUL 앞을 원시 BYTE 문자열로 읽는다.</summary>
    private static string CString(byte[] bytes) { int length = Array.IndexOf(bytes, (byte)0); return length >= 0 ? Encoding.Latin1.GetString(bytes, 0, length) : throw new ArgumentException("HTML 전체 생성 실제 NUL 없음"); }
    /// <summary>실제8192BYTE 물리 버퍼에 첫 NUL까지 복사한다.</summary>
    private static void Copy(byte[] target, string text) { text = CString(text); if (text.Length >= target.Length) throw new ArgumentException("HTML 전체 생성 지역 NUL 범위 초과"); Encoding.Latin1.GetBytes(text, target); target[text.Length] = 0; }
    /// <summary>원문 C 로케일의 ASCII 영문/숫자 관문이다.</summary>
    private static bool HasContent(string text) {
        // 원시 BYTE마다 ASCII 영문/숫자만 검사한다.
        foreach (char c in text) if (c is >= 'a' and <= 'z' or >= 'A' and <= 'Z' or >= '0' and <= '9') return true; return false;
    }
    /// <summary>suppress비영은 창 없이 첫 파서 효과만 실행한다.마지막 Modal 효과 뒤 현재 serial을 반환한다.</summary>
    public uint Show(string source, uint suppress, uint width, uint height)
    {
        uint root = suppress != 0 ? 0 : Require(_hooks.Create)(width, height); _state.CreatedSerial = root != 0 ? Node(root).Serial : 0; string substituted = Require(_hooks.Substitute)(CString(source), Capacity); if (CString(substituted).Length >= Capacity) throw new ArgumentException("HTML 치환 지역8192BYTE 범위 초과"); var parser = new HtmlTextParserState(); byte[] first = new byte[Capacity]; Copy(first, Require(_hooks.Parse)(parser, root, CString(substituted), Capacity)); if (root == 0) return 0;
        if (!HasContent(CString(first))) { _state.CreatedSerial = 0; Require(_hooks.Body)(root, "xxx"); Require(_hooks.Attach)(root, 0, 0, Require(_hooks.Read)(0x563e38), 0); Require(_hooks.Kill)(root); return 0; }
        _state.LastCloseTime = Require(_hooks.WallSeconds)(); if (Require(_hooks.Metadata)(root, first) == 0) { byte[] fallback = new byte[Capacity]; Copy(fallback, Require(_hooks.Fallback)()); Require(_hooks.Metadata)(root, fallback); }
        string secondSource = CString(first).Replace("\n", "<br>", StringComparison.Ordinal); string second = CString(Require(_hooks.Parse)(parser, root, secondSource, Capacity)); if (second.Length + 1 >= Capacity) throw new ArgumentException("HTML 마지막 LF/NUL 지역 범위 초과"); second += '\n'; Require(_hooks.Body)(root, second);
        int screenHeight = unchecked((int)Require(_hooks.Read)(0x5c78f4)); uint parent = Require(_hooks.Read)(0x563e38); float y = (float)(((double)screenHeight - Node(root).BaseHeight) * .5); int screenWidth = unchecked((int)Require(_hooks.Read)(0x5c78f0)); float x = (float)(((double)screenWidth - Node(root).BaseWidth) * .5); Require(_hooks.Attach)(root, x, y, parent, 0); Require(_hooks.Modal)(root, true); return Node(root).Serial;
    }
    /// <summary>같은 실제 본문/메타/수명과 파서만 연결한다.할당/현재Config/모달/전역은 상위가 공급한다.</summary>
    public static void BindHtmlWindowFactoryDevices(HtmlWindowFactoryHooks hooks, HtmlWindowGump window, HtmlWindowMetadata metadata, HtmlWindowTextParser parser, GumpLifetime lifetime) { hooks.Body = window.SetBody; hooks.Metadata = metadata.Process; hooks.Parse = parser.Parse; hooks.Kill = lifetime.Kill; }
}
