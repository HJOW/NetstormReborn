using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// 서버별 뉴스 시각의 원문 키/조회/설정문 순서를 같은 저장소에서 실행한다.
namespace Netstorm.Client.Chat;

/// <summary>원문00548748 DWORD와00548790 키를 값 사본 없이 빌린다.</summary>
public sealed class SystemNewsTimeGlobals
{
    /// <summary>같은 현재 뉴스 시각의 실제 주소다.</summary>
    public UIntBox Value { get; }
    /// <summary>실제 장기 키 저장소이며 Root는128BYTE를 제공한다.</summary>
    public byte[] Key { get; }
    /// <summary>현재값을 초기화하지 않고 두 저장소만 빌린다.</summary>
    public SystemNewsTimeGlobals(UIntBox value, byte[] key) { Value = value ?? throw new ArgumentNullException(nameof(value)); Key = key ?? throw new ArgumentNullException(nameof(key)); }
}
/// <summary>현재 서버 이름과 같은 Config의 선택 장치다.</summary>
public sealed class SystemNewsTimeHooks
{
    /// <summary>매호출 현재 좁은 BYTE 서버 이름이다.</summary>
    public Func<string>? ServerName { get; init; }
    /// <summary>누락0을 포함한 현재 signed 설정값이다.</summary>
    public Func<string, int>? Read { get; init; }
    /// <summary>인용 설정문을 실제 해석기로 보내며 디스크 저장은 강제하지 않는다.</summary>
    public Action<string>? Configure { get; init; }
}
/// <summary>0042e470/0042e4d0/0042e4f0의 물리 경계와 부분 쓰기다.</summary>
public static class SystemNewsTime
{
    /// <summary>실제 선택된 하위만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("시스템 뉴스 시각의 실제 장치 미연결");
    /// <summary>이름+NewsTime을 NUL까지 복사한 뒤 작성 범위의 점만 밑줄로 바꾼다.</summary>
    public static string BuildKey(SystemNewsTimeGlobals globals, SystemNewsTimeHooks hooks)
    {
        string name = Need(hooks.ServerName)(); int end = name.IndexOf('\0'); if (end >= 0) name = name[..end]; string formatted = name + "NewsTime";
        if (formatted.Length >= 256 || formatted.Length >= globals.Key.Length) throw new ArgumentOutOfRangeException(nameof(globals), "시스템 뉴스 키의 실제 로컬/공유 물리 부족");
        Encoding.Latin1.GetBytes(formatted).CopyTo(globals.Key, 0); globals.Key[formatted.Length] = 0;
        // 작성한 C문자열의 점만 바꾸며 미작성 꼬리는 유지한다.
        for (int index = 0; index < formatted.Length; index++) if (globals.Key[index] == '.') globals.Key[index] = (byte)'_';
        return Encoding.Latin1.GetString(globals.Key, 0, formatted.Length);
    }
    /// <summary>키 작성/설정 조회 뒤 signed 반환의 모든 비트를 같은 DWORD에 쓴다.</summary>
    public static void Read(SystemNewsTimeGlobals globals, SystemNewsTimeHooks hooks) { string key = BuildKey(globals, hooks); globals.Value.Value = unchecked((uint)Need(hooks.Read)(key)); }
    /// <summary>서버 getter보다 먼저 현재값을 캡처하고 원문 signed 설정문을 해석기에 전달한다.</summary>
    public static void Write(SystemNewsTimeGlobals globals, SystemNewsTimeHooks hooks)
    {
        uint value = globals.Value.Value; string key = BuildKey(globals, hooks); string expression = key + "=\"" + unchecked((int)value).ToString(CultureInfo.InvariantCulture) + "\"";
        if (expression.Length >= 256) throw new ArgumentOutOfRangeException(nameof(globals), "시스템 뉴스 설정문256BYTE 물리 부족"); Need(hooks.Configure)(expression);
    }
}
