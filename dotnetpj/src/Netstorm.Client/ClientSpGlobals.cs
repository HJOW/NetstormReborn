using Netstorm.Core.Original;

// 메뉴와 미션보다 오래 사는 공통 SP·추적 상태를 소유한다.
namespace Netstorm.Client;

/// <summary>원문 공통 시작의 SP와 추적 주소를 소유한다. 생성 뒤 실제 시작 위치에서 Store.Initialize를 호출한다.</summary>
/// <remarks>cpppj client/ClientSpGlobals 대응. C# 호스트가 이미 소유한 난수·CRT·모드 상자를 직접 빌린다.</remarks>
public sealed class ClientSpGlobals
{
    /// <summary>모든 미션과 같은 게임 난수다. 다른 스트림 혼합을 검사할 때 참조를 비교한다.</summary>
    public GameRandom Random { get; }

    /// <summary>공통 시작과 지형이 공유하는 실제 CRT 상태다.</summary>
    public UIntBox CrtState { get; }

    /// <summary>0054d4e0의 최초 BSS0 추적 주소다. 미션 교체에서 다시 초기화하지 않는다.</summary>
    private readonly IntBox _tracking = new();

    /// <summary>같은 실제 분산배열·순열·offset의 유일한 저장소다. 조회만으로 초기화하지 않는다.</summary>
    public RawSpStore Store { get; }

    /// <summary>실제 장기 상태를 연결한다. 시각 읽기·난수 소비·SP 초기화는 수행하지 않는다.</summary>
    public ClientSpGlobals(GameRandom random, UIntBox crtState, UIntBox mode, Func<int> unixSeconds)
    {
        ArgumentNullException.ThrowIfNull(random); ArgumentNullException.ThrowIfNull(crtState);
        ArgumentNullException.ThrowIfNull(mode); ArgumentNullException.ThrowIfNull(unixSeconds);
        Random = random; CrtState = crtState;
        var seed = new RawGameRandomSeed(random, crtState, mode, unixSeconds);
        Store = new RawSpStore(random, seed.Reseed);
        Store.BindTrackingOffset(_tracking);
    }
}
