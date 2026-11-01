using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client.Mission;

/// <summary>기술 목록의 매 항목 조회와 현재 허용 표 쓰기 경계다.</summary>
public sealed class MissionTechnologyHooks
{
    public Func<int, string>? Item { get; init; }
    public Func<string, int>? FindType { get; init; }
    public Action<int>? AllowAll { get; init; }
    public Action<int, int>? Allow { get; init; }
}

/// <summary>원본 미션 Header의 기술 허용 목록 처리다. 근거: cpppj MissionTechnology.cpp.</summary>
public static class MissionTechnology
{
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("미션 기술 허용의 실제 조회/쓰기 연결 누락");
    private static string LowerAscii(string value)
    {
        char[] chars = value.ToCharArray();
        for (int i = 0; i < chars.Length; i++) if (chars[i] is >= 'A' and <= 'Z') chars[i] = (char)(chars[i] + 32);
        return new string(chars);
    }

    /// <summary>매 항목을 다시 조회하고 빈 항목에서 종료한다.</summary>
    public static void Apply(MissionTechnologyHooks hooks)
    {
        int allowed = 1; uint index = 0;
        while (true)
        {
            string item = Need(hooks.Item)(unchecked((int)index)); int nul = item.IndexOf('\0'); if (nul >= 0) item = item[..nul];
            if (item.Length == 0) return;
            switch (LowerAscii(item))
            {
                case "deny": allowed = 0; break;
                case "allow": allowed = 1; break;
                case "all": Need(hooks.AllowAll)(allowed); break;
                default: Need(hooks.Allow)(Need(hooks.FindType)(item), allowed); break;
            }
            index = unchecked(index + 1);
        }
    }

    /// <summary>현재 Mission Config와 설치 전·후 공유할 실제 256칸 제한 표를 사용한다.</summary>
    public static void Apply(MissionScript script, Func<string, int> findType, uint[] restrictions)
    {
        ArgumentNullException.ThrowIfNull(script); ArgumentNullException.ThrowIfNull(findType);
        if (restrictions.Length != 256) throw new ArgumentException("미션 기술 제한 표의 물리 크기 오류", nameof(restrictions));
        Apply(new MissionTechnologyHooks
        {
            Item = index =>
            {
                byte[] bytes = OriginalText.EncodeWindows1252(script.Get("techAllowed") ?? "");
                int end = Array.IndexOf(bytes, (byte)0); if (end < 0) end = bytes.Length;
                if (end >= 512) throw new InvalidOperationException("미션 기술 허용의 원본512BYTE 임시 버퍼 범위");
                return ConfigNumbers.ListItem(Encoding.Latin1.GetString(bytes, 0, end), index);
            },
            FindType = findType,
            AllowAll = value => Array.Fill(restrictions, value == 0 ? 1u : 0u, 0, 255),
            Allow = (type, value) => { if ((uint)type >= 256) throw new ArgumentOutOfRangeException(nameof(type)); restrictions[type] = value == 0 ? 1u : 0u; }
        });
    }
}
