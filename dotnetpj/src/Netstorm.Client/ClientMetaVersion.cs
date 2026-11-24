// Meta/일반/Editor의 버전 차이 확인을 같은 진단/알림 장치에 연결한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 CheckClientMetaVersion(004b4160) 을 이식한다.
// 0.937에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Globalization;

namespace Netstorm.Client;

// 004b4160 버전 차이 확인의 실제 외부 장치다.
public sealed class ClientMetaVersionHooks
{
    /// <summary>현재 설정 정수를 읽는다. suppressVersionMessage 조회에 사용한다.</summary>
    public Func<string, int>? Config { get; set; }

    /// <summary>원문 진단 로그에 쓴다. 0x1d 버전 차이 행이다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>버전 차이 Tell을 실제 UI에 알린다.</summary>
    public Action<string>? Tell { get; set; }
}

// 004b4160 버전 차이 확인 본체다. 메뉴/세션을 요구하지 않는다.
public static class ClientMetaVersion
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.937에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 버전 확인의 실제 장치 미연결");

    /// <summary>10.78과 다르면 진단에 쓰고 주 버전이 다르고 숨김이 꺼졌을 때만 Tell하고 거짓을 반환한다. 0.937에서 추가됐다.</summary>
    /// <param name="major">받은 주 버전이다.</param>
    /// <param name="minor">받은 부 버전이다.</param>
    /// <param name="hooks">실제 설정/진단/알림 장치다.</param>
    public static bool Check(uint major, uint minor, ClientMetaVersionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (major != 10 || minor != 78)
        {
            Need(hooks.Log)(0x1d, string.Format(CultureInfo.InvariantCulture, "VERSION DIFFERENCE: You are 10.78. Meta Server is {0}.{1}.\n", major, minor));
            if (major != 10 && Need(hooks.Config)("suppressVersionMessage") == 0)
            {
                Need(hooks.Tell)(string.Format(CultureInfo.InvariantCulture, "MetaVerDiff({0}.{1})", major, minor));
                return false;
            }
        }

        return true;
    }
}
