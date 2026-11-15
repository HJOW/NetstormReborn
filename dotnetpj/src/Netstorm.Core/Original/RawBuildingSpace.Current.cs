// 현재 TYPE DWORD와 정상 복귀 가능한 원문 진단을 건물 옆면의 소비 위치에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj RawBuildingSpace의 같은 현재 섬/다리 타입과 진단 장치를 빌린다.</summary>
public sealed partial class RawBuildingSpace
{
    /// <summary>실제 Client가 소유하는 현재 지면 TYPE DWORD 조회다.</summary>
    private Func<uint>? _currentIslandType;
    /// <summary>같은 장기 Links의 현재 다리 TYPE DWORD 조회다.</summary>
    private Func<uint>? _currentBridgeType;
    /// <summary>선택된 다리 nibble 진단이 정상 복귀할 수 있는 실제 장치다.</summary>
    private Action<string, string, uint>? _notifyAssertion;

    /// <summary>현재 owner만 빌리고 결합 시 지면/TYPE를 조회하지 않는다.</summary>
    public void BindCurrentIslandType(Func<uint> value) => _currentIslandType = value ?? throw new ArgumentNullException(nameof(value));
    /// <summary>현재 owner만 빌리고 결합 시 다리/TYPE를 조회하지 않는다.</summary>
    public void BindCurrentBridgeType(Func<uint> value) => _currentBridgeType = value ?? throw new ArgumentNullException(nameof(value));
    /// <summary>원문 현재 checking owner를 빌리고 bool 사본으로 축소하지 않는다.</summary>
    public void BindChecking(Func<int> value) => _state.Checking.Bind(value);
    /// <summary>원문 값형 진단 뷰와 함수 객체를 복사한다. 미설치 진단은 도달 위치에서만 실패한다.</summary>
    public void BindNotifyDiagnostics(RawCheckingFlag checking, Action<string, string, uint>? assertion)
    {
        ArgumentNullException.ThrowIfNull(checking); _state.Checking = new(checking); _notifyAssertion = assertion;
    }
}
