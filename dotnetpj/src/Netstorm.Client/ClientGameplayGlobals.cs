using Netstorm.Core.Original;

// 현재 미션이 빌리는 장기 게임 전역의 주소를 유지한다.
namespace Netstorm.Client;

/// <summary>사제 회복·HP·이동이 공유하는 ClientGameplayGlobals의 전역 소유 부분이다.</summary>
/// <remarks>cpppj client/ClientGameplayGlobals의 GameAux/Authority/PatchSample/PatchDiagnostic 대응이다.
/// 커서·선택·Runtime 대여 등 나머지 부분은 해당 규칙 이식 시 추가한다.</remarks>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>같은 현재 사제TYPE·1/4HP·이동 역할을 소유한다.</summary>
    public RawGameAuxState GameAux { get; } = new();

    /// <summary>현재 서버와 boss를 같은 주소에서 읽고 쓰는 소유자다.</summary>
    public RawAuthorityState Authority { get; } = new();

    /// <summary>선택·강조·P/R 입력과 State 목록 정리가 공유하는 장기 실제 전역이다.</summary>
    public ClientUserSelectionGlobals Selection { get; } = new();

    /// <summary>00541250의 최초126이며 공장 고정프레임과 연구/업그레이드가 공유하는 현재 아웃포스트 TYPE다.</summary>
    public UIntBox OutpostTypeBox { get; } = new() { Value = 126 };

    /// <summary>World가 바뀌어도 되감지 않는 Player Deck의 장기 주소/블록 저장소다.</summary>
    public RawProductionDeckStorage ProductionDeckStorage { get; } = new();
    /// <summary>현재 메뉴/튜토리얼/생산이 공유하는256DWORD 금지 표다.0은 허용이다.</summary>
    public uint[] ProductionRestrictions { get; } = new uint[256];
    /// <summary>0054db14의 현재 전체 Deck 모드 DWORD다.</summary>
    public UIntBox FullDeckBox { get; } = new();
    /// <summary>005412bc의 무료 기술 역할이며 최초 Mog153이다.</summary>
    public UIntBox FreeTechnologyTypeBox { get; } = new() { Value = 153 };
    /// <summary>00541088의 현재 내용물 Form 역할이며 최초6이다.</summary>
    public UIntBox ContentFormTypeBox { get; } = new() { Value = 6 };
    /// <summary>0054107c의 현재 GumpForm 역할이며 최초3이다.</summary>
    public UIntBox GumpFormTypeBox { get; } = new() { Value = 3 };
    /// <summary>005427bc의 현재 Player 내용물 통지 사건이다.</summary>
    public UIntBox ContentEventBox { get; } = new() { Value = 0x02000031 };
    /// <summary>종속 해제 UI가 원문 입력 큐에 넣는 현재 사건이다.</summary>
    public UIntBox ContentRemovalEventBox { get; } = new() { Value = 0x02000039 };
    /// <summary>005c89c4의 현재 조건부 SP 검사 횟수다.</summary>
    public UIntBox ContentMoneyChecksBox { get; } = new();
    /// <summary>005c85d4의 현재 저장 게임 관문이다.복원 흐름과 내용물 통지가 공유한다.</summary>
    public BoolBox SavedGameBox { get; } = new();
    /// <summary>005453e8의 현재 폭탄 획득 시각 효과 억제 DWORD다.</summary>
    public UIntBox ContentBombBlockedBox { get; } = new();
    /// <summary>내용물 화면 투영이 읽는 현재59a91c/920의 두 픽셀 원점이다.</summary>
    public int[] ContentScreenOrigin { get; } = new int[2];

    /// <summary>00540cb8의 최초 DWORD6이다. 사제 표본 쓰기 후 마지막 값을 미션 반납 뒤에도 보존한다.</summary>
    public UIntBox PatchSample { get; } = new() { Value = 6 };

    /// <summary>005318ec의 최초 DWORD6이다. 회복 추적 쓰기와 후속 정책이 같은 현재값을 빌린다.</summary>
    public UIntBox PatchDiagnostic { get; } = new() { Value = 6 };

    /// <summary>미션 교체 뒤에도 같은 원문 최근실패비트/TYPE를 보존하는 SID 진단 owner다.</summary>
    public RawSidRobustDiagnostics SidRobustDiagnostics { get; } = new();
}
