// 원문 정리 상태와 실제 예약·조회·의식·환불·삭제 경계를 보관한다.
namespace Netstorm.Core.Original;

/// <summary>RawPathCleanup.h의 효과 뒤 현재 값을 다시 읽는 전역이다.</summary>
public sealed class PathCleanupState
{
    /// <summary>현재 helper/너겟 삭제 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>실제 Client의 현재 권한을 정리 효과 사이에서 다시 읽는다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>독립 입력 또는 같은 장기 권한을 원문 소비 위치에서 읽는다.</summary>
    public bool CurrentBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>현재 원문 shouldDestroy 진단 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>의식 장소를 찾는 현재 사제TYPE다.</summary>
    public uint PriestType { get; set; } = 158;
    /// <summary>현재 다이스 TYPE다.</summary>
    public uint DaisType { get; set; } = 165;
    /// <summary>현재 아웃포스트TYPE도 반환 장소로 인정한다.</summary>
    public uint OutpostType { get; set; } = 126;
    /// <summary>독립 입력의 현재 너겟TYPE다.</summary>
    public uint NuggetType { get; set; } = 154;
    /// <summary>원문 보조 건설물의 지연1 예약 사건이다.</summary>
    public uint HelperEvent { get; set; } = 0x1a2c;
    /// <summary>같은 장기005412c0 owner이며 연결 시 내용물 조회마다 읽는다.</summary>
    public UIntBox? SharedNuggetType { get; set; }
    /// <summary>공유 owner 또는 기존 독립 현재 값을 선택한다.</summary>
    public uint CurrentNuggetType() => SharedNuggetType?.Value ?? NuggetType;
}

/// <summary>같은 실제 월드에 연결해야 하는 정리 함수 값이다.</summary>
public sealed record PathCleanupHooks
{
    /// <summary>원문 SharedRegular 예약 메모리 확보의 성공/실패다.</summary>
    public Func<bool>? ReserveShared { get; init; }
    /// <summary>같은 Host.AddSharedRegular의 실제 요청이다.</summary>
    public Action<int, uint, float>? AddShared { get; init; }
    /// <summary>같은 실제 내용물의 mask/kind/index 조회다.</summary>
    public Func<int, uint, uint, uint, int>? Contained { get; init; }
    /// <summary>현재 실제 GraphFor 포인터 식별 값이다.</summary>
    public Func<int, uint>? GraphFor { get; init; }
    /// <summary>선택된 실제 신전 의식 함수다.</summary>
    public Action<int, int, uint>? Ritual { get; init; }
    /// <summary>현재 helperTYPE/owner의 실제 건설 환불이다.</summary>
    public Action<uint, uint>? Refund { get; init; }
    /// <summary>같은 실제 가상 삭제다.</summary>
    public Action<int, uint>? Destroy { get; init; }
}
