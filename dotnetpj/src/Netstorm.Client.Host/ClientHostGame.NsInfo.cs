// NSInfo의 같은 현재 zone/서버/버퍼를 실제 제품 호스트 수명에 보유한다.
namespace Netstorm.Client.Host;

/// <summary>ClientMain 생성/순수 getter와 NSInfo 옵션의 실제 장기 소유자를 연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>메뉴와 Session 교체로 다시 만들지 않는 현재 NSInfo 전역 소유자다.</summary>
    private readonly ClientNsInfoGlobals _nsInfoGlobals = new();
    /// <summary>현재 실제 같은 NSInfo 소유자만 조회하며 표 초기화/통신을 실행하지 않는다.</summary>
    public ClientNsInfoGlobals NsInfoGlobals => _nsInfoGlobals;
    /// <summary>실제 시작 설정이 있으면 같은 세 DWORD만 해석한다. 자료 없는 화면 호스트는 현재값을 유지한다.</summary>
    private void InterpretNsInfoOptions()
    {
        if (_startup is { } startup) _nsInfoGlobals.ReadOptions(startup.Configuration);
    }
}
