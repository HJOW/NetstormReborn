namespace Netstorm.Client.Gump; // 완료 입력 줄과 줄바꿈 표시 복제의 독립 소유 영역이다.

/// <summary>원본24바이트 표시 줄.0 링크는 센티널이다.</summary>
public sealed class StyleTextBufferLine
{
    /// <summary>이 표시 줄이 소유한 실제 조각 head.</summary>
    public uint Head;
    /// <summary>현재 표시 높이.</summary>
    public int Height { get; set; }
    /// <summary>현재 표시 폭.</summary>
    public int Width { get; set; }
    /// <summary>삽입 당시 전임 y+높이.</summary>
    public int Y { get; set; }
    /// <summary>다음 표시 줄.</summary>
    public uint Next { get; set; }
    /// <summary>이전 표시 줄.</summary>
    public uint Previous { get; set; }
}
/// <summary>원본 문자열 추가 직전의 바이트 범위 통지.</summary>
public delegate void StyleTextAppend(ReadOnlySpan<byte> bytes);
/// <summary>현재 문자열/문맥/명시 길이의 실제 Screen 측정.</summary>
public delegate (int Width, int Height) StyleTextBufferMeasure(ReadOnlySpan<byte> bytes, uint[] context, int length);
/// <summary>원본 입력 전체 칸/표시 센티널/상태. HardLines[LineCount]는 미완료 끝 줄이다.</summary>
public sealed class StyleTextBufferState
{
    /// <summary>미완료 한 칸을 포함한 최대 저장 칸.</summary>
    public int MaxLines { get; set; }
    /// <summary>개행이 완료한 입력 줄 수.</summary>
    public int LineCount { get; set; }
    /// <summary>표시 줄 수.</summary>
    public int SoftCount { get; set; }
    /// <summary>실제 소유 head 배열.</summary>
    public uint[] HardLines { get; set; } = [];
    /// <summary>줄0의 실제 저장소.</summary>
    public StyleTextBufferLine Sentinel { get; } = new();
    /// <summary>안정된 표시 줄 표.</summary>
    public SortedDictionary<uint, StyleTextBufferLine> SoftLines { get; } = [];
    /// <summary>미사용 원본 DWORD.</summary>
    public uint Reserved { get; set; }
    /// <summary>호출자가 지우는 변경 상태.</summary>
    public uint Dirty { get; set; }
    /// <summary>비0이면 자동 줄바꿈을 생략한다.</summary>
    public uint NoWrap { get; set; }
    /// <summary>반납 뒤 재사용하지 않는 자동 토큰.0은 소진이다.</summary>
    public uint NextSoftToken { get; set; } = 1;
    /// <summary>살아 있는 원본 입력 저장소 여부.</summary>
    public bool Initialized { get; set; }
    /// <summary>길이/NUL 확인 이전의 문자열 추가 통지. 그림에서는 호출하지 않는다.</summary>
    public StyleTextAppend? OnAppend { get; set; }
}
/// <summary>실제 글자 측정과 비문자 가상 복제 경계.</summary>
public sealed class StyleTextBufferHooks
{
    /// <summary>전체/명시 길이를 구별하는 현재 측정.</summary>
    public StyleTextBufferMeasure? MeasureText { get; set; }
    /// <summary>같은 공통 표의 새로운 파생 조각을 반환한다.</summary>
    public Func<uint, uint>? CloneExternal { get; set; }
}
/// <summary>cpppj/src/client/StyleTextBuffer.cpp의 입력 소유/완료 줄/엄격한 폭 비교/공백 포함 복제와 반납 순서.</summary>
public sealed class StyleTextBuffer
{
    /// <summary>같은 실제 입력/표시 상태.</summary>
    private readonly StyleTextBufferState _state;
    /// <summary>공통 조각 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>같은 실제 조각 수명/측정.</summary>
    private readonly StyleText _style;
    /// <summary>실제 Screen 측정/파생 복제.</summary>
    private readonly StyleTextBufferHooks _hooks;
    /// <summary>상태/처리기 연결의 실제 동일성 검사에 쓴다.</summary>
    public StyleTextBufferState State => _state;
    /// <summary>새 저장소만 초기화하며 미완료 칸을 포함한 양수 최대 칸을 받는다.</summary>
    public StyleTextBuffer(StyleTextBufferState state, StyleTextState fragments, StyleText style, int maxLines, StyleTextBufferHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(hooks);
        if (maxLines < 1 || state.Initialized || state.HardLines.Length != 0 || state.SoftLines.Count != 0 || state.Sentinel.Head != 0) throw new ArgumentException("StyleTextBuffer 새 저장소/양수 최대 칸 필요"); _state = state; _fragments = fragments; _style = style; _hooks = hooks;
        state.HardLines = new uint[maxLines]; state.MaxLines = maxLines; state.LineCount = state.SoftCount = 0; var s = state.Sentinel; s.Head = s.Next = s.Previous = 0; s.Height = s.Width = s.Y = 0; state.Reserved = state.Dirty = state.NoWrap = 0; state.NextSoftToken = 1; state.OnAppend = null; state.Initialized = true;
    }
    /// <summary>소멸 뒤나 외부에서 잘못 바꾼 저장소는 접근 전에 거부한다.</summary>
    private void RequireStorage() { if (!_state.Initialized || _state.MaxLines < 1 || _state.HardLines.Length != _state.MaxLines) throw new ArgumentException("StyleTextBuffer 입력 저장소 없음"); }
    /// <summary>미완료 끝 칸의 실제 소유 주소.</summary>
    private ref uint CurrentLine() { RequireStorage(); if (_state.LineCount < 0 || _state.LineCount >= _state.MaxLines) throw new ArgumentOutOfRangeException("LineCount"); return ref _state.HardLines[_state.LineCount]; }
    /// <summary>같은 표의 살아 있는 공통 조각.</summary>
    private StyleTextFragment Fragment(uint token) => token != 0 && _fragments.Fragments.TryGetValue(token, out var n) ? n : throw new ArgumentOutOfRangeException(nameof(token), "StyleTextBuffer 조각 없음");
    /// <summary>센티널과 일반 표시 줄을 같은 필드 접근으로 읽는다.</summary>
    private StyleTextBufferLine Line(uint token) => token == 0 ? _state.Sentinel : _state.SoftLines.TryGetValue(token, out var n) ? n : throw new ArgumentOutOfRangeException(nameof(token), "StyleTextBuffer 표시 줄 없음");
    /// <summary>손상된 체인의 무한 순회만 거부한다.</summary>
    private static void Visit(HashSet<uint> seen, uint token) { if (!seen.Add(token)) throw new InvalidOperationException("StyleTextBuffer 순환 링크"); }
    /// <summary>현재 소유 head는 유지하며 next를 저장한 뒤 scalar 반납한다.</summary>
    private void DeleteChain(uint head)
    {
        var seen = new HashSet<uint>();
        // 반납 콜백이 바꾼 next 대신 삭제 전 저장한 링크를 따른다.
        while (head != 0) { Visit(seen, head); uint next = Fragment(head).Next; _style.Destroy(head, 1); head = next; }
    }
    /// <summary>음수는NUL 탐색이다. LF만 완료 줄로 세고 마지막 칸이 차면 첫 완료 줄을 버린다.</summary>
    public void AppendText(ReadOnlySpan<byte> bytes, uint[] context, int length = -1, uint metadata = 0)
    {
        RequireStorage(); var notice = _state.OnAppend; notice?.Invoke(bytes); int count = length < 0 ? bytes.IndexOf((byte)0) : length; if (count < 0) throw new ArgumentException("StyleTextBuffer 음수 길이에 NUL 필요"); if (count > bytes.Length) throw new ArgumentOutOfRangeException(nameof(length)); int start = 0;
        // LF 앞의 빈 문자열도 소유 조각으로 남긴다. CR/탭은 변환하지 않는다.
        for (int position = 0; position < count; position++)
        {
            if (bytes[position] != 10) continue; CurrentLine(); if (_state.LineCount == _state.MaxLines - 1) RemoveLine(0); uint token = _style.NewText(bytes[start..position], context, (uint)(position - start), metadata); _style.Append(ref CurrentLine(), token); _state.LineCount = unchecked(_state.LineCount + 1); start = position + 1;
        }
        if (start < count) { uint token = _style.NewText(bytes[start..count], context, (uint)(count - start), metadata); _style.Append(ref CurrentLine(), token); } _state.Dirty = 1;
    }
    /// <summary>그림을 현재 끝 칸에 추가하고 지정한 경우에만 원본 축소 맞춤을 적용한다.</summary>
    public void AppendPicture(Markup pictures, uint type, uint frame, uint metadata = 0, int fitWidth = 0, int fitHeight = 0)
    { RequireStorage(); uint token = pictures.NewPicture(type, frame, metadata); if (fitWidth != 0 || fitHeight != 0) pictures.Fit(token, fitWidth, fitHeight); _style.Append(ref CurrentLine(), token); _state.Dirty = 1; }
    /// <summary>완료 줄만 삭제하되 전체 최대 칸을 옮겨 미완료 체인도 앞으로 이동한다.</summary>
    public void RemoveLine(int index)
    {
        RequireStorage(); if (index < 0 || index >= _state.MaxLines - 1 || index >= _state.LineCount) throw new ArgumentOutOfRangeException(nameof(index)); DeleteChain(_state.HardLines[index]);
        // 각 칸을 앞으로 복사한 직후 원래 칸을 비운다.
        for (int p = index + 1; p < _state.MaxLines; p++) { _state.HardLines[p - 1] = _state.HardLines[p]; _state.HardLines[p] = 0; } _state.LineCount = unchecked(_state.LineCount - 1); _state.Dirty = 1;
    }
    /// <summary>입력 원본과 별개인 이전 표시 복제만 반납한다.</summary>
    private void ClearSoft(bool resetCount)
    {
        uint token = _state.Sentinel.Next; var seen = new HashSet<uint>();
        // 삭제 전 다음 줄을 저장한다. 정상 마지막 링크는0센티널이다.
        while (token != 0) { Visit(seen, token); uint next = Line(token).Next; DeleteChain(Line(token).Head); var row = Line(token); row.Head = row.Next = row.Previous = 0; _state.SoftLines.Remove(token); token = next; } _state.Sentinel.Next = _state.Sentinel.Previous = 0; if (resetCount) _state.SoftCount = 0;
    }
    /// <summary>전임 바로 뒤에 빈 줄을 삽입한다. y는 이 시점의 전임 높이로만 정한다.</summary>
    private uint NewSoftLine(uint previous)
    {
        uint next = Line(previous).Next, token = _state.NextSoftToken;
        // 외부 선점 토큰은 넘기며 반납 번호를 자동 재사용하지 않는다.
        while (token != 0 && _state.SoftLines.ContainsKey(token)) token = unchecked(token + 1);
        if (token == 0) throw new InvalidOperationException("StyleTextBuffer 표시 토큰 소진"); _state.SoftLines.Add(token, new() { Next = next, Previous = previous, Y = unchecked(Line(previous).Y + Line(previous).Height) }); _state.NextSoftToken = unchecked(token + 1); Line(next).Previous = token; Line(previous).Next = token; _state.SoftCount = unchecked(_state.SoftCount + 1); return token;
    }
    /// <summary>명시 복사에 마지막NUL도 사용할 수 있도록 포함해 반환한다.</summary>
    private ReadOnlySpan<byte> TextBytes(uint token)
    { var n = Fragment(token); if (n.Kind != StyleTextFragmentKind.Text) throw new ArgumentException("StyleTextBuffer 문자열 필요"); int end = n.Text.AsSpan().IndexOf((byte)0); if (end < 0 || end == int.MaxValue) throw new ArgumentException("StyleTextBuffer 저장 NUL 필요"); return n.Text.AsSpan(0, end + 1); }
    /// <summary>space/NUL만 단어 경계로 사용한다.</summary>
    private static int WordEnd(ReadOnlySpan<byte> bytes, int position)
    {
        // 연속 space도 하나씩 처리하며 다른 공백 문자는 단어에 포함한다.
        while (position < bytes.Length && bytes[position] != 0 && bytes[position] != 32) position++; if (position == bytes.Length) throw new ArgumentOutOfRangeException(nameof(bytes), "단어 끝 NUL 필요"); return position;
    }
    /// <summary>접두 폭을 엄격히 비교하고 첫 단어 강제 배치/공백·마지막NUL 포함 복사를 보존한다.</summary>
    private void SplitText(uint token, ref uint line, int width)
    {
        int length = TextBytes(token).Length - 1, consumed = 0;
        // 남은 문자열에서 다음 표시 조각을 반복 생성한다.
        while (consumed < length)
        {
            int accepted = 0; bool newLine = false, copy = true;
            // 이미 허용한 공백 뒤의 다음 단어까지 전체 접두 폭을 측정한다.
            while (consumed + accepted < length)
            {
                var suffix = TextBytes(token)[consumed..]; int end = WordEnd(suffix, accepted); var size = (_hooks.MeasureText ?? throw new ArgumentException("StyleTextBuffer 측정 경계 필요"))(suffix, Fragment(token).Context, end);
                if (unchecked(Line(line).Width + size.Width) >= width) { newLine = true; if (accepted == 0) { if (Line(line).Width != 0) copy = false; else accepted = end + 1; } break; } accepted = end + 1;
            }
            if (copy) { var source = Fragment(token); uint fragment = _style.NewText(TextBytes(token)[consumed..], source.Context, (uint)accepted, source.Metadata); var size = _style.MeasureFragment(fragment); Line(line).Height = Math.Max(Line(line).Height, size.Height); Line(line).Width = unchecked(Line(line).Width + size.Width); _style.Append(ref Line(line).Head, fragment); consumed += accepted; } if (newLine) line = NewSoftLine(line);
        }
    }
    /// <summary>양수 폭에서만 이전 표시를 반납한다. 마지막 미완료 입력은 표시하지 않으며 Dirty도 지우지 않는다.</summary>
    public void Layout(int width)
    {
        RequireStorage(); if (width <= 0) return; ClearSoft(true); uint line = 0;
        // 개행이 완료한 입력만 새 표시 줄과 복제 조각으로 만든다.
        for (int index = 0; index < _state.LineCount; index++)
        {
            if (index >= _state.MaxLines) throw new ArgumentOutOfRangeException("LineCount"); uint token = _state.HardLines[index]; line = NewSoftLine(line); var seen = new HashSet<uint>();
            // 가상 호출 뒤 현재 원본 next를 다시 읽는다.
            while (token != 0)
            {
                Visit(seen, token);
                if (Fragment(token).Kind != StyleTextFragmentKind.Text)
                {
                    var size = _style.MeasureFragment(token); Line(line).Height = Math.Max(Line(line).Height, size.Height);
                    if (unchecked(Line(line).Width + size.Width) >= width && _state.NoWrap == 0 && (Line(line).Head != 0 || size.Width < width)) line = NewSoftLine(line);
                    Line(line).Width = unchecked(Line(line).Width + size.Width); uint clone = (_hooks.CloneExternal ?? throw new ArgumentException("StyleTextBuffer 파생 복제 경계 필요"))(token); _style.Append(ref Line(line).Head, clone);
                }
                else
                {
                    var size = (_hooks.MeasureText ?? throw new ArgumentException("StyleTextBuffer 측정 경계 필요"))(TextBytes(token), Fragment(token).Context, -1);
                    if (unchecked(Line(line).Width + size.Width) < width || _state.NoWrap != 0) { Line(line).Height = Math.Max(Line(line).Height, size.Height); Line(line).Width = unchecked(Line(line).Width + size.Width); var source = Fragment(token); uint clone = _style.NewText(TextBytes(token), source.Context, uint.MaxValue, source.Metadata); _style.Append(ref Line(line).Head, clone); }
                    else SplitText(token, ref line, width);
                }
                token = Fragment(token).Next;
            }
        }
    }
    /// <summary>현재 표시 개수만 순회한 signed 최대 폭. 초기값은0이다.</summary>
    public int MaximumWidth()
    {
        RequireStorage(); int result = 0; uint token = _state.Sentinel.Next;
        // 카운터보다 링크가 먼저 끝나면 잘못된 호스트 상태다.
        for (int i = 0; i < _state.SoftCount; i++) { if (token == 0) throw new ArgumentOutOfRangeException("SoftCount"); result = Math.Max(result, Line(token).Width); token = Line(token).Next; } return result;
    }
    /// <summary>마지막 줄의 저장 y+높이를 원본 DWORD 감김으로 반환한다.</summary>
    public int TotalHeight() { RequireStorage(); if (_state.SoftCount < 1) return 0; var last = Line(_state.Sentinel.Previous); return unchecked(last.Y + last.Height); }
    /// <summary>완료 입력→표시→센티널 소유 순서로 반납한다. 원본처럼 미완료 끝 조각과 카운터는 남긴다.</summary>
    public void Destroy()
    {
        RequireStorage();
        // 미완료 칸은 소멸자가 반납하지 않으므로 완료 입력만 지운다.
        for (int i = 0; i < _state.LineCount; i++) { if (i >= _state.MaxLines) throw new ArgumentOutOfRangeException("LineCount"); DeleteChain(_state.HardLines[i]); _state.HardLines[i] = 0; } _state.HardLines = []; ClearSoft(false); DeleteChain(_state.Sentinel.Head); _state.Sentinel.Head = _state.Sentinel.Next = _state.Sentinel.Previous = 0; _state.Initialized = false;
    }
    /// <summary>실제 첫 글자 폭/논리 슬롯 높이를 명시 길이와NUL 측정 모두에 공급한다.</summary>
    public static void BindStyleTextBufferFonts(StyleTextBufferHooks hooks, GumpText text)
    { hooks.MeasureText = (ReadOnlySpan<byte> bytes, uint[] context, int length) => { int width = 0, height = 0; text.Measure(bytes, GumpText.DecodeGumpTextContext(context), ref width, ref height, length); return (width, height); }; }
    /// <summary>같은 그림의 원본 복제 규칙을 연결하고 다른 파생 분배를 보존한다.</summary>
    public static void BindStyleTextBufferMarkup(StyleTextBufferHooks hooks, Markup pictures)
    { var previous = hooks.CloneExternal; hooks.CloneExternal = t => pictures.Contains(t) ? pictures.ClonePicture(t) : (previous ?? throw new ArgumentException("StyleTextBuffer 그림 외 복제 경계 필요"))(t); }
}
