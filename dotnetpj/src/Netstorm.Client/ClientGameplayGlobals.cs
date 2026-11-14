using Netstorm.Core.Original;

// 현재 미션이 빌리는 장기 게임 전역의 주소를 유지한다.
namespace Netstorm.Client;

/// <summary>사제 회복·HP·이동이 공유하는 ClientGameplayGlobals의 전역 소유 부분이다.</summary>
/// <remarks>cpppj client/ClientGameplayGlobals의 GameAux/Authority/PatchSample/PatchDiagnostic 대응이다.
/// 커서·선택·Runtime 대여 등 나머지 부분은 해당 규칙 이식 시 추가한다.</remarks>
public sealed class ClientGameplayGlobals
{
    /// <summary>같은 현재 사제TYPE·1/4HP·이동 역할을 소유한다.</summary>
    public RawGameAuxState GameAux { get; } = new();

    /// <summary>현재 서버와 boss를 같은 주소에서 읽고 쓰는 소유자다.</summary>
    public RawAuthorityState Authority { get; } = new();

    /// <summary>00540cb8의 최초 DWORD6이다. 사제 표본 쓰기 후 마지막 값을 미션 반납 뒤에도 보존한다.</summary>
    public UIntBox PatchSample { get; } = new() { Value = 6 };

    /// <summary>005318ec의 최초 DWORD6이다. 회복 추적 쓰기와 후속 정책이 같은 현재값을 빌린다.</summary>
    public UIntBox PatchDiagnostic { get; } = new() { Value = 6 };
}
