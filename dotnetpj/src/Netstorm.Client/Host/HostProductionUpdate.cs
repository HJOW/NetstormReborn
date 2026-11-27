using Netstorm.Core.Original;

// 실제 적재·모드·기술·변경 관문 뒤 현재 생산 Deck과 내용물을 갱신한다.
namespace Netstorm.Client.Host;

/// <summary>원본 생산 패널의 변경 알림 소비 순서를 실행한다.</summary>
public static class HostProductionUpdate
{
    /// <summary>관문이 열리고 변경된 경우에만 현재 알림을 지운 뒤 계산과 동기화를 실행한다.</summary>
    /// <param name="loaded">전체 세션 설치 완료 여부다.</param>
    /// <param name="mode">현재 소유자 모드다.</param>
    /// <param name="dirty">같은 월드의 생산 변경 알림이다.</param>
    /// <param name="technology">현재 로컬 기술 부모 조회다.</param>
    /// <param name="refresh">현재 템플릿 소유자의 Deck 재계산이다.</param>
    /// <param name="synchronize">같은 세션의 생산 내용물 동기화다.</param>
    /// <returns>재계산과 동기화를 실행했으면 참이다.</returns>
    public static bool Update(bool loaded, SquidOwnerMode mode, UIntBox dirty, Func<uint>? technology, Action? refresh, Action? synchronize)
    {
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(dirty);
        if (!loaded || (!mode.Fort && !mode.Battle && !mode.Challenge)) return false;
        if (mode.Challenge || (technology ?? throw new InvalidOperationException("생산 현재 기술 조회 미연결"))() == 0) return false;
        if (dirty.Value == 0) return false;
        dirty.Value = 0;
        (refresh ?? throw new InvalidOperationException("생산 Deck 재계산 미연결"))();
        (synchronize ?? throw new InvalidOperationException("생산 내용물 동기화 미연결"))();
        return true;
    }
}
