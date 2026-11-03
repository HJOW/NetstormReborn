namespace Netstorm.Client;

/// <summary>
/// 새 제품 표기 11.01의 정수 버전과 표시 문자열이다
/// (cpppj <c>client/ProductVersion.h</c>의 제품 상수).
/// </summary>
/// <remarks>2026-11-03 11.01 요구에서 추가했다. 원본 데이터 판본 번호(10.78/10.72)는 바꾸지 않는다.</remarks>
public static class ProductVersion
{
    /// <summary>새 제품의 정수 주 버전이다.</summary>
    public const int Major = 11;

    /// <summary>표기 11.01의 정수 부 버전이다.</summary>
    public const int Minor = 1;

    /// <summary>선행 0이 필요한 제품 표시 문자열이다.</summary>
    public const string Text = "11.01";

    /// <summary>설정의 버전 표시 값이다.</summary>
    public const string ConfigValue = "v11.01";

    /// <summary>원본 Tell의 등록 객체 조회를 그대로 쓰는 전용 이름이다.</summary>
    public const string CreditsObject = "Patch1101";

    /// <summary>추가 Credits 항목이 실제 입력 큐에 전달할 대상이다.</summary>
    public const string CreditsTarget = "Patch1101.Credits";
}
