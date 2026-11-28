using Netstorm.Core.Original;

// Scene·Session·표시 장치 교체와 별개인 Client CD 물리를 보유한다.
namespace Netstorm.Client;

/// <summary>00558dd4 캐시와 서로 다른 최초 결과/재확인 serial을 한 Client 수명에 유지한다.</summary>
public sealed class ClientCdGlobals
{
    /// <summary>최초 BSS0인 실제128BYTE 루트 캐시다.</summary>
    private readonly byte[] _cache = new byte[CdPresence.BufferSize];
    /// <summary>조회로 장치를 실행하지 않는 현재 캐시의 같은 쓰기 뷰다.</summary>
    public Memory<byte> CachedRoot => _cache;
    /// <summary>0054db6c의 최초 결과이며 State7의005c85b8과 별개다.</summary>
    public uint StartupPresent { get; private set; }
    /// <summary>005c85ec의 같은 재확인 serial이며 Session 반납으로 지우지 않는다.</summary>
    public UIntBox RecheckSerial { get; } = new();
    /// <summary>현재 캐시와 호출별 scratch로 확인하며 최초 결과/serial을 쓰지 않는다.</summary>
    public uint Check(CdPresenceHooks hooks) => CdPresence.Check(_cache, new byte[CdPresence.BufferSize], hooks);
    /// <summary>원문 시작 위치의 결과0→실제 확인→정상 반환값 저장 순서다.</summary>
    public void InitializeStartup(CdPresenceHooks hooks) { StartupPresent = 0; StartupPresent = Check(hooks); }
}
