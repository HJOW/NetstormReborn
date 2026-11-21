using Netstorm.Core.Original;

// 같은 Native Host 공간/프레임과 장기 거리·큐 및 현재 진단을 실제 탐색기에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignPathBindings의 실제 검색 저장소/진단 공유 부분이다. 명령/부모 가상 결선은 후속이다.</summary>
public sealed partial class HostSession
{
    /// <summary>이 세션의 실제 같은 공간을 빌린 탐색기다.</summary>
    private RawPathSearch? _pathSearch;
    /// <summary>이 세션의 원문 방향/정책 상태다.</summary>
    private RawPathSearchState? _pathSearchState;
    /// <summary>현재 수명 안의 같은 실제 탐색기이며 미설치/반납 뒤에는 명시 실패한다.</summary>
    public RawPathSearch PathSearch => FixedPacketNeed(_pathSearch);
    /// <summary>현재 방향/Graph 선택을 원문처럼 갱신할 실제 상태다.</summary>
    public RawPathSearchState PathSearchState => FixedPacketNeed(_pathSearchState);
    /// <summary>실제 공간과 프레임을 확인하고 장기 배열/현재005318ec을 복사 없이 빌린다.</summary>
    private void BindPathSearchDevices(RawSurfaceStorage storage)
    {
        if (!ReferenceEquals(storage.Hash, _terrainHash) || !ReferenceEquals(storage.Spots, _terrainSpots) || _terrainPopTypes is null) throw new ArgumentException("경로 검색의 같은 실제 공간 필요");
        if (_pathSearch is not null) return;
        _pathSearchState = new() { SharedPolicySelector = Gameplay.PatchDiagnostic };
        _pathSearch = new(Pool, _terrainHash!, _terrainSpots!, _terrainPopTypes, WorldFrames, _pathSearchState, storage.PathSearch);
    }
    /// <summary>세션 참조만 반납하고 같은 장기 Work/Initialized를 초기화하지 않는다.</summary>
    private void ReleasePathSearchDevices() { _pathSearch = null; _pathSearchState = null; }
}
