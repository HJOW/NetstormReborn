using System.Text;

namespace Netstorm.Client.Host;

/// <summary>
/// Template manager가 +12에 NUL 종료 바이트로 보관하는 파일 이름과, 호스트가 쓰는 실제 문자열(경로) 사이를 변환한다.
/// manager 쪽 "바이트 문자열"은 Core의 <c>RawTemplateLoad</c>처럼 문자 하나가 바이트 하나(Latin1)인 표기이며, 실제 경로는 UTF-16 문자열이다.
/// cpppj는 둘 다 UTF-8 <c>std::string</c>이라 변환이 필요 없지만, C#은 한글·이모지 작업 경로를 manager에 UTF-8 바이트로 보관하고
/// 꺼낼 때 UTF-8로 해석해야 같은 파일을 가리킨다. 순수 ASCII 이름은 두 표기가 같다.
/// </summary>
/// <remarks>
/// 2026-11-02 0.135(TS5)에서 추가했다. 이전 단계의 검사는 모두 ASCII 이름이라 영향이 없었다. 짝이 맞지 않는 UTF-8 바이트(옛 ANSI 이름)는
/// U+FFFD로 바뀌므로 그 이름은 왕복하지 않는다(현재 호출 경로는 모두 UTF-8 경로를 쓴다).
/// </remarks>
internal static class TemplateFileNames
{
    /// <summary>실제 문자열을 manager 바이트 문자열(UTF-8 바이트를 문자 하나씩 담은 문자열)로 바꾼다.</summary>
    /// <param name="name">실제 파일 이름 또는 경로.</param>
    internal static string ToManager(string name) => Encoding.Latin1.GetString(Encoding.UTF8.GetBytes(name));

    /// <summary>manager 바이트 문자열을 실제 문자열로 되돌린다. 문자 값은 모두 0~255여야 한다.</summary>
    /// <param name="stored">manager에서 읽은 바이트 문자열.</param>
    internal static string FromManager(string stored) => Encoding.UTF8.GetString(Encoding.Latin1.GetBytes(stored));

    /// <summary>실제 문자열이 manager에 쓰이는 UTF-8 바이트 수다(NUL 종료 제외).</summary>
    /// <param name="name">실제 파일 이름 또는 경로.</param>
    internal static int ManagerByteCount(string name) => Encoding.UTF8.GetByteCount(name);
}
