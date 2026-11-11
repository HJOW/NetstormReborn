using Netstorm.Core.Original;

namespace Netstorm.Client;

/// <summary>
/// 사용자 요청의 제품 표기는 원본 데이터 판본과 분리하고 같은 HTML·Config·Tell 경로를 사용한다
/// (cpppj <c>client/ProductPresentation.cpp</c>의 제품 전용 표시 경계).
/// </summary>
/// <remarks>2026-11-03 11.01 요구에서 추가했다. 원본 파일에 쓰지 않는 제품 전용 표시 경계다.</remarks>
public static class ProductPresentation
{
    /// <summary>추가 Credits의 고정 본문이며 기존 HTML 창 공장을 그대로 사용한다.</summary>
    /// <remarks>2026-11-03 11.01 요구에서 추가했고, 2026-11-11 0.342에서 cpppj 제품 1291의 Copyright 2026 HJOW 줄을 반영했다.</remarks>
    public const string PatchCreditsBody = "[Credits]\n<background=\"nsstart\">\n<h2>{H2}NetStorm 11.01 Patch Credits</h2>\n<p>{Text}\nVersion: {version}<br>\nDevelopment and Netstorm 11.01 patch: HJOW.<br>\nCopyright 2026 HJOW<br>\n<p>\nDeveloped with AI assistance using OpenAI Codex and ChatGPT.<br>\nAI was used to assist analysis, programming, documentation, and verification.<br>\n<p>\nThe original game creators are acknowledged in the original Credits entries.\n$Button=Back,Tell,Credits\n$Button=Cancel,Tell,Blank\n";

    /// <summary>새 Credits 항목의 표시 이름이다.</summary>
    public const string PatchCreditsLabel = "Netstorm 11.01 Patch Credits";

    /// <summary>
    /// Startup의 원문 v주.부 정수 표기 뒤 제품 표기 11.01의 선행 0을 명시적으로 보존한다.
    /// </summary>
    /// <param name="config">제품 버전을 적용할 설정 경계다.</param>
    /// <exception cref="ArgumentNullException">설정 경계가 null이다.</exception>
    public static void ApplyProductVersion(ConfigInterface config)
    {
        ArgumentNullException.ThrowIfNull(config);
        config.SetInt("MAJOR_VERSION", ProductVersion.Major);
        config.SetInt("MINOR_VERSION", ProductVersion.Minor);
        config.Set("version", ProductVersion.ConfigValue);
    }

    /// <summary>
    /// 원본 두 항목·Cancel을 유지하고 중복 확장도 피한다. 새 항목의 대소문자는 사용자 지정 표기다.
    /// </summary>
    /// <param name="target">Tell 대상 이름이다.</param>
    /// <param name="original">원본 Credits 본문이다.</param>
    /// <returns>세 번째 항목이 추가된 본문이다.</returns>
    public static string TellBody(string target, string original)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(original);
        if (!IsCredits(target))
        {
            return original;
        }
        if (original.Contains(ProductVersion.CreditsTarget, StringComparison.Ordinal))
        {
            return original;
        }
        return original + "\n$Menu=" + PatchCreditsLabel + ",Tell," + ProductVersion.CreditsTarget + "\n";
    }

    /// <summary>
    /// ASCII 문자만 소문자로 바꿔 원본 이력 이름과 비교한다.
    /// </summary>
    /// <param name="text">비교할 대상 이름이다.</param>
    /// <returns>Credits 대상이면 참이다.</returns>
    private static bool IsCredits(string text)
    {
        return string.Concat(text.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c)) == "credits";
    }
}
