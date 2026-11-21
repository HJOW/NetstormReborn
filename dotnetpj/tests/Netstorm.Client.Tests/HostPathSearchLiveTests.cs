using Netstorm.Core.Original;

// 실제 Native Host의 공간/현재 진단/장기 작업 배열을 검색기와 공유한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 사제 월드 구성기를 같은 Host 검색 연결 검사에 사용한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>설치한 검색기가 장기 Work와 현재 진단을 직접 읽으며 반납 뒤 API를 거부한다.</summary>
    [Fact]
    public void NativePathSearchSharesActualWorldWorkAndCurrentDiagnostic()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        RawPathSearch search = session.PathSearch; Assert.Same(surface.Storage.PathSearch.Work, search.Work); Assert.Same(session.Gameplay.PatchDiagnostic, session.PathSearchState.SharedPolicySelector);
        Assert.Same(session.Pool, search.Pool); Assert.False(surface.Storage.PathSearch.Initialized);
        // 같은 Host 전역 변경은 별도 복사 없이 현재 검색 상태에 보인다.
        session.Gameplay.PatchDiagnostic.Value = 5; Assert.Equal(5u, session.PathSearchState.SharedPolicySelector!.Value);
        search.SearchCells(new(OriginalEdition.Patch1078), 10, 10, 10, 10, false);
        Assert.True(surface.Storage.PathSearch.Initialized); Assert.Equal(5u, session.PathSearchState.DirectionPrerequisite[3]);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.PathSearch); Assert.True(surface.Storage.PathSearch.Initialized); Assert.Same(search.Work, surface.Storage.PathSearch.Work);
    }
}
