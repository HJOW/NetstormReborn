using System.Text;
using Netstorm.Core.Original;

// 같은60BYTE 지역과 현재 Identity/번역을 사용하는 표시 전용 대상 검사다.
namespace Netstorm.Client.Chat;

/// <summary>선택한 하위만 실제 사용 위치에서 요구하는 원문 장치다.</summary>
public sealed class ChatRecipientHooks
{
    /// <summary>같은60BYTE 지역을 현재 Player/Chat 정보로 수정할 수 있다.</summary>
    public Func<byte[], bool>? Resolve { get; set; }
    /// <summary>빈 키도 포함한 현재 원문 표시 BYTE 문자열을 반환한다.</summary>
    public Func<string, string>? Translate { get; set; }
}
/// <summary>ChatRecipients.cpp의0042dc70/0042d910 고정 문법과 부분 쓰기다.</summary>
public static class ChatRecipients
{
    /// <summary>첫NULL 앞의 여덟 원문 포인터이며 빈 키를 생략하지 않는다.</summary>
    private static readonly string[] Channels = ["", "", "", "BATTLE", "", "EVERYONE", "ALLIES", "OBSERVER"];
    /// <summary>미선택 장치를 앞서 요구하지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("채팅 대상 이름 검사의 선택 장치 미연결");
    /// <summary>C문자열 첫NUL 뒤의 호스트 문자는 해석하지 않는다.</summary>
    private static string Z(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>현재 지역 이름을 인접 주소 첫BYTE의NUL까지 실제 물리에서 읽는다.</summary>
    private static string Name(byte[] record)
    {
        int end = record.AsSpan(8).IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(record), "채팅 대상 지역 이름의 NUL 물리 끝"); return Encoding.Latin1.GetString(record.AsSpan(8, end));
    }
    /// <summary>원문004841b0의 선택 필드만 지워 mode와 미작성 이름 꼬리를 보존한다.</summary>
    private static void Reset(byte[] record) { record.AsSpan(0, 8).Clear(); record[8] = 0; record.AsSpan(48, 4).Clear(); }
    /// <summary>C locale의 ASCII 대문자만 접으며 다른 BYTE는 보존한다.</summary>
    private static char Lower(char value) => value is >= 'A' and <= 'Z' ? (char)(value + 32) : value;
    /// <summary>현재 첫NUL 앞의 바이트 길이와 C locale 동등성만 비교한다.</summary>
    private static bool Equal(string first, string second)
    {
        first = Z(first); second = Z(second); if (first.Length != second.Length) return false;
        // 원문 narrow C문자열의 각BYTE를 순서대로 비교한다.
        for (int index = 0; index < first.Length; index++) if (Lower(first[index]) != Lower(second[index])) return false;
        return true;
    }
    /// <summary>원문 네 구분자만 공백이며 VT/FF/NBSP는 이름의 일부다.</summary>
    private static bool Separator(char value) => value is '\r' or '\n' or '\t' or ' ';
    /// <summary>manager0/NULL은false, 그 외 전체 토큰의 Identity/채널 유효성을 같은 지역에서 검사한다.</summary>
    public static bool CheckChatRecipients(uint manager, string? text, byte[] scratch, ChatRecipientHooks hooks)
    {
        if (manager == 0 || text is null) return false; RawPlayerIdentity.Initialize(scratch, 0); string remaining = Z(text);
        // 공백 나머지는 장치를 요구하지 않고true이며 다음 위치는 절단 전 토큰 끝이다.
        for (;;)
        {
            int begin = 0;
            // 고정 네 구분자만 앞에서 소비한다.
            while (begin < remaining.Length && Separator(remaining[begin])) begin++;
            if (begin == remaining.Length) return true; remaining = remaining[begin..]; int length = 0;
            // 이름40BYTE 상한과 관계없이 원문 토큰 전체 길이를 찾는다.
            while (length < remaining.Length && !Separator(remaining[length])) length++;
            Reset(scratch); int copied = Math.Min(length, 40);
            // 바이트 문자열의 지정 앞부분만 복사하고 인접 주소 첫BYTE에도 종결자를 쓸 수 있다.
            for (int index = 0; index < copied; index++) scratch[8 + index] = unchecked((byte)remaining[index]);
            scratch[8 + copied] = 0;
            if (!Need(hooks.Resolve)(scratch))
            {
                bool found = false;
                // Identity 실패 뒤에만 여덟 키를 순서 조회하고 callback 뒤 현재 이름을 다시 읽는다.
                foreach (string key in Channels)
                {
                    string translated = Need(hooks.Translate)(key); if (Equal(Name(scratch), translated)) { found = true; break; }
                }
                if (!found) return false;
            }
            remaining = remaining[length..];
        }
    }
}
