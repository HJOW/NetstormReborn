namespace Netstorm.Net;

/// <summary>
/// 로컬 미션 전송의 포트·주소 정책이다
/// (cpppj <c>src/zacket/LocalMissionTransport.h</c>의 로컬 경계와 외부 접속 문서의 로컬 예외를 따른다).
/// </summary>
/// <remarks>2026-11-03 멀티플레이 사전 구현에서 추가했다. 테스트 없이 cpppj와 원본 디컴파일만 참고해 골격만 만든다.</remarks>
public static class LocalNetworkPolicy
{
    /// <summary>원본 게임 서버 포트다. 로컬 루프백에만 바인딩한다.</summary>
    public const int GameServerPort = 6799;

    /// <summary>원본 루트 서버 포트다. 로컬 루프백에만 바인딩한다.</summary>
    public const int RootServerPort = 6800;

    /// <summary>원본 탐색 UDP 포트다. 외부 브로드캐스트를 보내지 않는다.</summary>
    public const int DiscoveryPort = 8998;

    /// <summary>원본 내장 HTTP 포트다. 기본값은 꺼짐이며 켜도 로컬에만 바인딩한다.</summary>
    public const int HttpPort = 80;

    /// <summary>리슨 주소다. 원격 목적지가 아니라 로컬 바인딩용이다.</summary>
    public const string ListenAddress = "0.0.0.0";

    /// <summary>허용하는 원격·접속 주소다. 루프백만 허용한다.</summary>
    public const string LoopbackAddress = "127.0.0.1";

    /// <summary>
    /// 외부 도메인·IP 접속을 거부하고 루프백만 허용한다.
    /// </summary>
    /// <param name="host">접속·바인딩할 호스트 문자열이다.</param>
    /// <exception cref="ArgumentException">루프백·리슨 주소가 아니다.</exception>
    public static void RequireLoopback(string host)
    {
        ArgumentNullException.ThrowIfNull(host);
        if (!string.Equals(host, LoopbackAddress, StringComparison.Ordinal) &&
            !string.Equals(host, "localhost", StringComparison.OrdinalIgnoreCase) &&
            !string.Equals(host, ListenAddress, StringComparison.Ordinal))
        {
            throw new ArgumentException("로컬 전송은 루프백 주소만 허용한다", nameof(host));
        }
    }
}
