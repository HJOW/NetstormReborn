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

    /// <summary>원문 언어 번호의 한국어다. cpppj <c>client/LanguageOptions.h</c>와 같은 값이다.</summary>
    /// <remarks>2026-11-11 0.358에서 11차·13차 언어 분기에 썼다.</remarks>
    public const uint KoreanLanguage = 7;

    /// <summary>원문 언어 번호의 일본어다. cpppj <c>client/LanguageOptions.h</c>와 같은 값이다.</summary>
    /// <remarks>2026-11-11 0.358에서 11차·13차 언어 분기에 썼다.</remarks>
    public const uint JapaneseLanguage = 5;

    /// <summary>한국어 모드의 11.01 패치 크레딧 본문이다. 영어 본문과 같은 구성이며 새 항목 이름과 제목은 사용자 지정 표기를 따른다.</summary>
    /// <remarks>2026-11-11 0.358에서 cpppj 제품 1291 이후의 K4 번역본을 반영했다. 한국어 UI 글꼴로 표시한다.</remarks>
    public const string PatchCreditsKoreanBody = "<background=\"nsstart\">\n<h2>{H2}NetStorm 11.01 패치 크레딧</h2>\n<p>{Text}\n버전: {version}<br>\n개발 및 NetStorm 11.01 패치: HJOW.<br>\nCopyright 2026 HJOW<br>\n<p>\nOpenAI Codex와 ChatGPT를 활용한 AI 지원으로 개발했습니다.<br>\nAI는 분석, 프로그래밍, 문서화, 검증을 돕는 데 사용했습니다.<br>\n<p>\n원작 제작자는 원본 크레딧 항목에서 확인할 수 있습니다.\n$Button=뒤로,Tell,Credits\n$Button=취소,Tell,Blank\n";

    /// <summary>일본어 모드의 11.01 패치 크레딧 본문이다. 한국어 본문과 같은 구성이며 제목·버튼 이름만 일본어다.</summary>
    /// <remarks>2026-11-11 0.358에서 cpppj 11차 일본어 번역본을 반영했다.</remarks>
    public const string PatchCreditsJapaneseBody = "<background=\"nsstart\">\n<h2>{H2}NetStorm 11.01 パッチ クレジット</h2>\n<p>{Text}\nバージョン: {version}<br>\n開発およびNetStorm 11.01パッチ: HJOW。<br>\nCopyright 2026 HJOW<br>\n<p>\nOpenAI Codexと<br>ChatGPTを活用した<br>AI支援で開発しました。<br>\nAIは分析、プログラミング、<br>ドキュメント作成、<br>検証の補助に使用しました。<br>\n<p>\n原作の制作者は、<br>元のクレジット項目で<br>確認できます。\n$Button=戻る,Tell,Credits\n$Button=キャンセル,Tell,Blank\n";

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
    /// 12차 스커미시 Campaign 항목과 11차·13차 언어별 본문·이름도 같은 규칙으로 더한다.
    /// </summary>
    /// <param name="target">Tell 대상 이름이다.</param>
    /// <param name="original">원본 Credits 본문이다.</param>
    /// <param name="language">현재 원문 언어 번호다. 0·1 영어, 5 일본어, 7 한국어이며 그 밖은 영어와 같다.</param>
    /// <returns>확장된 본문이다.</returns>
    /// <remarks>2026-11-11 0.358에서 cpppj HEAD의 언어·스커미시 분기를 반영했다. 난이도·스커미시 화면 자체는 R8 해당 차수에서 잇는다.</remarks>
    public static string TellBody(string target, string original, uint language = 0)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(original);
        bool korean = language == KoreanLanguage, japanese = language == JapaneseLanguage;
        // 한국어·일본어의 새 항목 본문은 고정 번역본이다
        if (korean && IsCreditsTarget(target))
        {
            return PatchCreditsKoreanBody;
        }
        if (japanese && IsCreditsTarget(target))
        {
            return PatchCreditsJapaneseBody;
        }
        string body = original;
        // 12차 스커미시: Campaign 목록에 Skirmish 항목을 더한다(원문 목록과 Back 버튼 사이)
        if (IsUCampaign(target))
        {
            string entry = "$Menu=" + (korean ? "스커미시" : japanese ? "スカーミッシュ" : "Skirmish") + ",Tell," + ProductVersion.CreditsObject + ".Skirmish\n";
            int back = body.IndexOf("$Button=", StringComparison.Ordinal);
            if (!body.Contains(ProductVersion.CreditsObject + ".Skirmish", StringComparison.Ordinal))
            {
                body = back < 0 ? body + entry : body.Insert(back, entry);
            }
        }
        if (!IsCredits(target) || body.Contains(ProductVersion.CreditsTarget, StringComparison.Ordinal))
        {
            return body;
        }
        return body + (korean ? "\n$Menu=NetStorm 11.01 패치 크레딧,Tell," + ProductVersion.CreditsTarget + "\n" : japanese ? "\n$Menu=NetStorm 11.01 パッチ クレジット,Tell," + ProductVersion.CreditsTarget + "\n" : "\n$Menu=" + PatchCreditsLabel + ",Tell," + ProductVersion.CreditsTarget + "\n");
    }

    /// <summary>
    /// ASCII 문자만 소문자로 바꿔 원본 이력 이름과 비교한다.
    /// </summary>
    /// <param name="text">비교할 대상 이름이다.</param>
    /// <returns>Credits 대상이면 참이다.</returns>
    private static bool IsCredits(string text)
    {
        return Lower(text) == "credits";
    }

    /// <summary>추가 Credits 항목의 Tell 대상과 같은지 비교한다.</summary>
    /// <param name="text">비교할 대상 이름이다.</param>
    /// <returns>새 항목 대상이면 참이다.</returns>
    private static bool IsCreditsTarget(string text)
    {
        return Lower(text) == Lower(ProductVersion.CreditsTarget);
    }

    /// <summary>Campaign 목록 대상과 같은지 비교한다.</summary>
    /// <param name="text">비교할 대상 이름이다.</param>
    /// <returns>Campaign 목록이면 참이다.</returns>
    private static bool IsUCampaign(string text)
    {
        return Lower(text) == "ucampaign";
    }

    /// <summary>ASCII 대문자만 소문자로 바꾼다.</summary>
    /// <param name="text">바꿀 이름이다.</param>
    /// <returns>소문자 이름이다.</returns>
    private static string Lower(string text)
    {
        return string.Concat(text.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
    }
}
