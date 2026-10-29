namespace Netstorm.Client.Gump; // Gump 등록과 별개인 원본 다중 줄 버퍼 소유/외부 로그 수명 영역이다.

/// <summary>cpppj/src/client/ScrollingTextGumpWidgets.cpp의 StyleTextBufferStore.실제 상태를 복사하지 않고 저장한다.</summary>
public sealed class StyleTextBufferStore
{
    /// <summary>같은 공통 조각 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>같은 실제 조각 수명/가상 처리기.</summary>
    private readonly StyleText _style;
    /// <summary>원본 값 복사와 같은 생성 경계 사본.</summary>
    private readonly StyleTextBufferHooks _hooks;
    /// <summary>소멸 뒤 호스트 free 통지.</summary>
    private readonly Action<uint> _release;
    /// <summary>상태와 처리기를 같은 레코드 수명에 묶는 별도 토큰 표.</summary>
    private readonly Dictionary<uint, TextGumpBufferBinding> _records = [];
    /// <summary>함수 객체를 값 복사해 각 생성 버퍼가 같은 경계를 갖게 한다.</summary>
    private static StyleTextBufferHooks Copy(StyleTextBufferHooks hooks) => new() { MeasureText = hooks.MeasureText, CloneExternal = hooks.CloneExternal };
    /// <summary>현재 조회 가능한 실제 등록 레코드.소멸 중 free 통지에서도 읽을 수 있다.</summary>
    private TextGumpBufferBinding Entry(uint token) => token != 0 && _records.TryGetValue(token, out var record) ? record : throw new InvalidOperationException("StyleTextBuffer 등록 토큰 없음");
    /// <summary>같은 조각/글꼴/그림과 필수 호스트 토큰 반납을 빌린다.</summary>
    public StyleTextBufferStore(StyleTextState fragments, StyleText style, StyleTextBufferHooks hooks, Action<uint> release)
    { ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(release); _fragments = fragments; _style = style; _hooks = Copy(hooks); _release = release; }
    /// <summary>별도 새 상태에 실제 버퍼를 생성한다.Gump 등록 수/serial은 바꾸지 않는다.</summary>
    public uint Construct(uint token, int maxLines)
    { if (token == 0 || _records.ContainsKey(token)) throw new ArgumentException("StyleTextBuffer 새 토큰 필요"); var state = new StyleTextBufferState(); var handler = new StyleTextBuffer(state, _fragments, _style, maxLines, Copy(_hooks)); _records.Add(token, new(state, handler)); return token; }
    /// <summary>Client 전역 로그 등 외부 상태 자체를 빌려 생성하며 같은 물리 상태의 중복 등록을 거부한다.</summary>
    public uint ConstructBorrowed(uint token, StyleTextBufferState state, int maxLines)
    {
        ArgumentNullException.ThrowIfNull(state); if (token == 0 || _records.ContainsKey(token)) throw new ArgumentException("StyleTextBuffer 새 외부 토큰 필요");
        // 이미 등록한 실제 상태와의 주소 동일성만 검사한다.
        foreach (var record in _records.Values) if (ReferenceEquals(record.State, state)) throw new ArgumentException("StyleTextBuffer 외부 상태 중복 생성");
        var handler = new StyleTextBuffer(state, _fragments, _style, maxLines, Copy(_hooks)); _records.Add(token, new(state, handler)); return token;
    }
    /// <summary>실제 생성에 쓴 상태와 처리기를 그대로 반환한다.</summary>
    public TextGumpBufferBinding Binding(uint token) => Entry(token);
    /// <summary>완료/표시 조각 소멸→호스트 free→레코드 제거.원본 미완료 끝 칸 누락도 유지한다.</summary>
    public void Destroy(uint token) { Entry(token).Handler.Destroy(); _release(token); _records.Remove(token); }
    /// <summary>같은 실제 버퍼를 표시/입력 조회에 연결한다.</summary>
    public static void BindScrollingTextBuffers(ScrollingTextGumpHooks hooks, StyleTextBufferStore buffers) => hooks.Buffer = buffers.Binding;
}
