// Root 고정 수신이 읽는 언어 번호·화면 크기·설정 치환의 실제 owner 공급을 모은다.
// cpppj/src/client/LanguageOptions.h·cpp 의 kLanguageNames/OriginalLanguageName/FindOriginalLanguage 와
// cpppj/src/client/CampaignClientServices.cpp 112·114행의 names.language/substitute,
// cpppj/src/client/ClientMain.cpp 854·1154행의 LogicalScreenSize 를 이식한다.
// 0.902에서 RU-2 실제 owner 조립용으로 추가됐다.
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client;

/// <summary>원문 일곱 이름 뒤 클론 한국어를 더한 여덟 언어 번호다.</summary>
public static class RootLanguageNumbers
{
    /// <summary>원문 순서를 그대로 둔 여덟 이름이다. 0/1은 별칭 합치기 없이 그대로 둔다. 0.902에서 추가됐다.</summary>
    public static readonly string[] Names = ["english", "english", "french", "german", "spanish", "japanese", "portuguese", "korean"];

    /// <summary>일본어 번호다. 0.902에서 추가됐다.</summary>
    public const uint Japanese = 5;

    /// <summary>클론 한국어 번호다. 0.902에서 추가됐다.</summary>
    public const uint Korean = 7;

    /// <summary>확장 문자 번역표·외부 글꼴을 쓰는 번호인지 판정한다. 0.902에서 추가됐다.</summary>
    /// <param name="number">판정할 번호다.</param>
    public static bool IsExtended(uint number) => number == Japanese || number == Korean;

    /// <summary>번호의 원문 이름이다. 범위를 벗어나면 예외를 낸다. 0.902에서 추가됐다.</summary>
    /// <param name="number">조회할 번호다.</param>
    public static string Name(uint number) => number < Names.Length ? Names[(int)number]
        : throw new ArgumentOutOfRangeException(nameof(number), "언어 번호의 원문 일곱+클론 하나 범위 오류");

    /// <summary>정확한 C문자열 비교이며 미등록/빈 이름은 0이다. 첫 english는 항상 0이다. 0.902에서 추가됐다.</summary>
    /// <param name="name">설정의 currentLanguage 값 등이다.</param>
    public static uint Find(string? name)
    {
        if (string.IsNullOrEmpty(name))
        {
            return 0;
        }

        int end = name.IndexOf('\0'); if (end >= 0) name = name[..end]; // 원문 strcmp는 첫NUL 뒤 문자열을 비교하지 않는다.

        for (uint index = 0; index < Names.Length; index++)
        {
            if (name == Names[(int)index])
            {
                return index;
            }
        }

        return 0;
    }
}

/// <summary>같은 실제 장기 전역·화면·설정을 빌리는 Root 공급값 묶음이다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>현재 설정의 currentLanguage를 원문 번호로 읽는다. 같은 실제 설정 owner를 빌린다. 0.902에서 추가됐다.</summary>
    /// <param name="currentLanguage">현재 설정의 currentLanguage 값이다.</param>
    public static uint RootLanguageNumber(string? currentLanguage) => RootLanguageNumbers.Find(currentLanguage?.Trim().ToLowerInvariant());

    /// <summary>수신 직전의 같은 논리 Screen을 읽는다. FSR 출력 픽셀이나 도킹 영역을 사용하지 않는다. 0.902에서 추가됐다.</summary>
    /// <param name="screen">같은 실제 논리 화면이다.</param>
    public static (int Width, int Height) RootScreenSize(Screen screen)
    {
        ArgumentNullException.ThrowIfNull(screen);
        return (screen.Width, screen.Height);
    }
}
