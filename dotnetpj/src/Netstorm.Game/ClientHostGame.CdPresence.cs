// 실제 State7이 빌릴 현재 CD 확인과 장기 캐시를 같은 Client에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>화면/미션 교체에서 새로 만들지 않는 CD 소유자와 실제 확인 입구다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 Client의 최초 BSS 캐시·시작 결과·재확인 serial이다.</summary>
    private readonly ClientCdGlobals _nativeCdGlobals;
    /// <summary>조회 자체로 OS/파일/캐시를 변경하지 않는 같은 장기 소유자다.</summary>
    public ClientCdGlobals CdGlobals => _nativeCdGlobals;
    /// <summary>현재 원문 파일 이름 고리와 Windows 장치로 확인한다. State7 결과 대입은 실제 부모가 맡는다.</summary>
    public uint CheckNativeCdPresence() => _nativeCdGlobals.Check(WindowsCdPresence.CreateHooks(_staticStrings.FileNames()));
}
