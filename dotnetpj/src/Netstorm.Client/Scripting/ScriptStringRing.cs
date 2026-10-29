namespace Netstorm.Client.Scripting; // HTML/스크립트가 공유하는 원본80칸 문자열 수명과 명령 조회 경계다.

/// <summary>원본004c7910의 한 칸. 문자열 값의 중복을 합치지 않는다.</summary>
internal sealed class ScriptStringSlot
{
    /// <summary>같은 칸에 마지막으로 복사한 첫 NUL 앞 원시 BYTE 문자열.</summary>
    internal string Text { get; set; } = "";
    /// <summary>해제된 이전 수명과 구별하는 비영 토큰.</summary>
    internal uint Token { get; set; }
}
/// <summary>cpppj/src/client/ScriptCommand.h/.cpp의 ScriptStringRing. 양수 주소 관문을 만족하는 opaque 토큰을 쓴다.</summary>
public sealed class ScriptStringRing
{
    /// <summary>원본80개 현재 칸.</summary>
    private readonly ScriptStringSlot[] _slots = [.. Enumerable.Range(0, 80).Select(_ => new ScriptStringSlot())];
    /// <summary>최초0에서 첫 저장 때1이 되는 현재 인덱스.</summary>
    public int Index { get; private set; }
    /// <summary>Begin의 signed 인자&gt;100 관문을 만족하는 호스트 일련번호.</summary>
    private uint _serial = 100;
    /// <summary>다음 칸의 이전 수명을 비운 뒤 첫 NUL 앞을 복사한다. 같은 문자열도 새 칸을 소비한다.</summary>
    public uint Store(string text) { Index = (Index + 1) % _slots.Length; var slot = _slots[Index]; slot.Text = ""; slot.Token = 0; if (_serial == 0x7fffffff) throw new OverflowException("스크립트 문자열의 양수 주소 토큰 수명 소진"); int end = text.IndexOf('\0'); slot.Text = end < 0 ? text : text[..end]; slot.Token = ++_serial; return slot.Token; }
    /// <summary>현재80칸 안에 살아 있는 실제 토큰만 해석한다.</summary>
    public string Resolve(uint token) {
        // 현재 칸만 순회하며 덮어쓴 이전 토큰을 새 문자열로 바꾸지 않는다.
        foreach (var slot in _slots) if (slot.Token != 0 && slot.Token == token) return slot.Text; throw new ArgumentOutOfRangeException(nameof(token), "스크립트 문자열의 해제되었거나 잘못된 토큰");
    }
}
/// <summary>원본004d52c0의 현재 명령 표/진단만 공급한다. 실제 문자열/인자 수명은 호출 모듈이 관리한다.</summary>
public sealed class ScriptCommandHooks
{
    /// <summary>현재 원시 명령 이름 조회. 미등록은 원문0xffffffff다.</summary>
    public Func<string, uint>? Lookup { get; set; }
    /// <summary>선택 분기에서만 실행하는 원본 미등록 명령 진단.</summary>
    public Action<string>? Unknown { get; set; }
}
