using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // HTML 조건의 실제 토큰/현재 설정/가림 효과 영역이다.

/// <summary>미리 복사하지 않고 매 실제 평가 시점의 설정을 치환한다.</summary>
public sealed class HtmlConditionEffectHooks
{
    /// <summary>원본00441740/CD004a9bd0에서 요구하는 현재 치환 결과.</summary>
    public Func<string, string>? Substitute { get; set; }
}
/// <summary>cpppj/src/client/HtmlConditionEffect.h/.cpp의 PATCH 비교/CD boolean과 현재 shown/depth 효과.</summary>
public sealed class HtmlConditionEffect
{
    /// <summary>원본 치환 임시 버퍼의 실제 물리 용량.</summary>
    private const int OutputCapacity = 100;
    /// <summary>현재 Config 치환 장치.</summary>
    private readonly HtmlConditionEffectHooks _hooks;
    /// <summary>PATCH 비교/CD boolean 분기.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>닫힘/숨김에 필요 없는 치환 장치는 선택 평가 시점에만 요구한다.</summary>
    public HtmlConditionEffect(HtmlConditionEffectHooks hooks, OriginalEdition edition = OriginalEdition.Patch1078) { ArgumentNullException.ThrowIfNull(hooks); _hooks = hooks; _edition = edition; }
    /// <summary>NUL/호스트 경계 뒤는 원문 C문자열의 가상0이다.</summary>
    private static char Byte(string source, int offset) => (uint)offset < (uint)source.Length ? source[offset] : '\0';
    /// <summary>각 연산자 BYTE의 ASCII 대/소문자만 허용한다.</summary>
    private static bool Is(char item, char lower) => item == lower || item == lower - 'a' + 'A';
    /// <summary>현재 설정에서 치환하고 첫 NUL/100BYTE 물리 버퍼 경계를 유지한다.</summary>
    private string Substitute(string token) { string value = (_hooks.Substitute ?? throw new InvalidOperationException("HTML 조건 현재 Config 치환 미연결"))(token); int end = value.IndexOf('\0'); if (end >= 0) value = value[..end]; if (value.Length >= OutputCapacity) throw new ArgumentException("HTML 조건 원본100BYTE 치환 버퍼 초과"); return value; }
    /// <summary>오른쪽 치환/atol→왼쪽 치환/atol 순서로 현재 파서 객체를 갱신한다.</summary>
    public void Apply(HtmlTextParserState state, string raw, bool closing)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(raw); int end = raw.IndexOf('\0'); if (end >= 0) raw = raw[..end]; if (Byte(raw, 0) != '?') throw new ArgumentException("HTML 조건 원문은 ?에서 시작해야 함"); if (state.Shown == 0 && state.HiddenDepth == 0) return;
        if (closing) { if (state.HiddenDepth != 0 && unchecked(--state.HiddenDepth) == 0) state.Shown = 1; return; }
        int offset = 1; bool leadingNegation = Byte(raw, offset) == '!'; if (leadingNegation) offset++; uint operation = 0; bool operatorNegation = false;
        if (_edition == OriginalEdition.Patch1078)
        {
            if (Byte(raw, offset) == '?') { offset++; operation = 1; } if (Is(Byte(raw, offset), 'g')) { offset++; operation = 2; if (Is(Byte(raw, offset), 'e')) { offset++; operation = 3; } } if (Is(Byte(raw, offset), 'l')) { offset++; operation = 4; if (Is(Byte(raw, offset), 'e')) { offset++; operation = 5; } } operatorNegation = Byte(raw, offset) == '!'; if (operatorNegation) offset++;
        }
        if (state.Shown == 0) return; bool predicate;
        if (operation != 0)
        {
            var left = RawTextToken.Read(raw, offset, "="); var right = RawTextToken.Read(raw, left.Next, ">"); string rightValue = Substitute(right.Text);
            if (operation == 1) predicate = Substitute(left.Text) == rightValue;
            else { int rightNumber = ConfigNumbers.ParseLong(rightValue), leftNumber = ConfigNumbers.ParseLong(Substitute(left.Text)); predicate = operation switch { 2 => rightNumber < leftNumber, 3 => rightNumber <= leftNumber, 4 => leftNumber < rightNumber, 5 => leftNumber <= rightNumber, _ => throw new InvalidOperationException("HTML 조건 연산자 내부 범위 오류") }; }
        }
        else { var token = RawTextToken.Read(raw, offset, "=>"); predicate = ConfigNumbers.ParseLong(Substitute(token.Text)) != 0; }
        if (predicate != (!leadingNegation && !operatorNegation)) { state.Shown = 0; state.HiddenDepth = unchecked(state.HiddenDepth + 1); }
    }
    /// <summary>같은 현재 ConfigStore를 매번 치환한다.전체 ConfigInterface 임시local 소유는 상위 조립 경계다.</summary>
    public static void BindHtmlConditionConfig(HtmlTextParserHooks hooks, ConfigStore config, OriginalEdition edition = OriginalEdition.Patch1078) { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(config); var condition = new HtmlConditionEffect(new() { Substitute = config.Expand }, edition); hooks.Condition = condition.Apply; }
}
