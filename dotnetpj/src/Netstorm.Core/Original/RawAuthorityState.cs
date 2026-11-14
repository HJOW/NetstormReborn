// 실제 서버/권한 두 논리 전역만 소유하며 다른 Squid/Player 상태를 포함하지 않는다.
namespace Netstorm.Core.Original;

// 권한 상태 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.

// 같은 Client의 물리 저장소와 현재 전역이 직접 참조하는 작은 권한 소유자다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.
public sealed class RawAuthorityState
{
    /// <summary>서버 관문에서 현재값을 읽는다. 두 PE의 초기 DWORD 1을 논리 true로 시작한다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.</summary>
    public bool Server { get; set; } = true;

    /// <summary>권한 관문에서 현재값을 읽는다. 두 PE의 초기 DWORD 1을 논리 true로 시작한다. 2026-11-07 F2d 서피스 월드 권한 이식에서 추가됐다.</summary>
    public bool Boss { get; set; } = true;

    /// <summary>같은 권한 소유자의 현재 boss를 읽으며 대리자 동등성으로 실제 소유자를 확인할 때 쓴다.</summary>
    public bool ReadBoss() => Boss;
}
