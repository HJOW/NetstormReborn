// NSInfo의 같은 현재 zone/서버/버퍼를 실제 제품 호스트 수명에 보유한다.
namespace Netstorm.Client.Host;

/// <summary>ClientMain 생성/순수 getter와 NSInfo 옵션의 실제 장기 소유자를 연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>메뉴와 Session 교체로 다시 만들지 않는 현재 NSInfo 전역 소유자다.</summary>
    private readonly ClientNsInfoGlobals _nsInfoGlobals = new();
    /// <summary>현재 실제 같은 NSInfo 소유자만 조회하며 표 초기화/통신을 실행하지 않는다.</summary>
    public ClientNsInfoGlobals NsInfoGlobals => _nsInfoGlobals;
    /// <summary>같은 NSInfo/설정/파일 장치의 실제 서버 목록 파일 소비자다. 자료 없는 호스트에서는 없다.</summary>
    private RootServerListDisk? _rootServerList;
    /// <summary>실제 물리 장치 준비 뒤 첫 메뉴 전에 한 번 같은 현재 목록을 읽는다.</summary>
    private void LoadRootServerList()
    {
        if (_startup is not { } startup) return;
        _rootServerList = new RootServerListDisk(_nsInfoGlobals, startup.Files, startup.Configuration);
        _rootServerList.Load();
    }
    /// <summary>현재 서버 목록을 사용자 Saves에 저장한다. 실제 Root reply/메타 UI의 선택 호출에서 사용하는 경계다.</summary>
    public void SaveRootServerList() => (_rootServerList ?? throw new InvalidOperationException("현재 서버 목록 파일 장치 미연결")).Save();
}
