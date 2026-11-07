// 실제 미션 새 login의 저장 월드 부모 전역을 generic Fort 메뉴 부모와 별도 몸체로 연결한다.
namespace Netstorm.Core.Original;

// 캠페인 월드 전역 모듈을 담는 네임스페이스다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.

// 외부 DWORD 저장소를 빌리는 상자다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.
public sealed class UIntBox
{
    /// <summary>빌린 DWORD 값이다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    public uint Value { get; set; }
}

// 외부 문자열 상세 owner를 빌리는 상자다. 원문 `std::optional&lt;string&gt;&amp;` 빌림을 옮긴다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class StringBox
{
    /// <summary>빌린 문자열 값이다. null이면 비어 있는 원문 optional과 같다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    public string? Value { get; set; }
}

// 외부 배정밀도 시각 owner를 빌리는 상자다. 원문 `double*` 빌림을 옮긴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.
public sealed class DoubleBox
{
    /// <summary>빌린 배정밀도 값이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public double Value { get; set; }
}

// 실제 부모 prologue와 선택한 mission 후처리가 읽는 현재 전역이다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.
public sealed class CampaignWorldGlobals
{
    /// <summary>빌리지 않을 때 쓰는 자체 my 저장소다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    private uint _localMy;

    /// <summary>빌리지 않을 때 쓰는 자체 login 저장소다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    private uint _localLogin;

    /// <summary>빌린 외부 my 저장소다. null이면 자체 저장소를 쓴다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    private UIntBox? _myBox;

    /// <summary>빌린 외부 login 저장소다. null이면 자체 저장소를 쓴다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    private UIntBox? _loginBox;

    /// <summary>명시 외부 owner 또는 자체 fallback의 같은 DWORD다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    public uint MySubscriber
    {
        get => _myBox is not null ? _myBox.Value : _localMy;
        set
        {
            if (_myBox is not null)
            {
                _myBox.Value = value;
            }
            else
            {
                _localMy = value;
            }
        }
    }

    /// <summary>명시 외부 owner 또는 자체 fallback의 같은 DWORD다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    public uint LoginSubscriber
    {
        get => _loginBox is not null ? _loginBox.Value : _localLogin;
        set
        {
            if (_loginBox is not null)
            {
                _loginBox.Value = value;
            }
            else
            {
                _localLogin = value;
            }
        }
    }

    /// <summary>현재 Player의 residence·island DWORD에 0으로 쓰는 값이다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    public uint Editor { get; set; }

    /// <summary>후처리 두 큰 분기를 모두 생략하는 실제 미션 전역이다. 2026-11-07 F2d 캠페인 월드 적재 이식에서 추가됐다.</summary>
    public bool MissionPost { get; set; } = true;

    /// <summary>
    /// 기존 독립 기본값 0·0·0·true를 보존한다.
    /// </summary>
    public CampaignWorldGlobals()
    {
    }

    /// <summary>
    /// 기존 값과 부분 값 초기화는 자체 fallback을 소유하는 값 생성이다.
    /// </summary>
    /// <param name="my">쓸 my 값이다.</param>
    /// <param name="login">쓸 login 값이다.</param>
    /// <param name="selectedEditor">쓸 editor 값이다.</param>
    /// <param name="post">쓸 missionPost 값이다.</param>
    public CampaignWorldGlobals(uint my, uint login = 0u, uint selectedEditor = 0u, bool post = true)
    {
        _localMy = my;
        _localLogin = login;
        Editor = selectedEditor;
        MissionPost = post;
    }

    /// <summary>
    /// 같은 두 주소를 직접 빌리며 외부 값에 쓰지 않는다. null인 슬롯은 해당 view의 독립 저장소를 쓴다.
    /// </summary>
    /// <param name="my">빌릴 외부 my 저장소다. null이면 자체 저장소를 쓴다.</param>
    /// <param name="login">빌릴 외부 login 저장소다. null이면 자체 저장소를 쓴다.</param>
    public CampaignWorldGlobals(UIntBox? my, UIntBox? login)
    {
        _myBox = my;
        _loginBox = login;
    }

    /// <summary>
    /// 복사·이동은 현재 네 값을 자체 fallback으로 복제하며 외부 참조를 암묵 전파하지 않는다.
    /// </summary>
    /// <param name="source">복제할 원본이다.</param>
    public CampaignWorldGlobals(CampaignWorldGlobals source)
    {
        ArgumentNullException.ThrowIfNull(source);
        _localMy = source.MySubscriber;
        _localLogin = source.LoginSubscriber;
        Editor = source.Editor;
        MissionPost = source.MissionPost;
    }

    /// <summary>
    /// 외부 저장소는 이 view보다 오래 살아야 한다. 같은 두 주소를 직접 빌린다.
    /// </summary>
    /// <param name="my">빌릴 외부 my 저장소다.</param>
    /// <param name="login">빌릴 외부 login 저장소다.</param>
    /// <returns>두 주소를 빌린 전역이다.</returns>
    public static CampaignWorldGlobals BorrowSubscribers(UIntBox my, UIntBox login)
    {
        ArgumentNullException.ThrowIfNull(my);
        ArgumentNullException.ThrowIfNull(login);
        return new CampaignWorldGlobals(my, login);
    }

    /// <summary>
    /// 대입은 목적지의 기존 바인딩을 유지하고 원본 네 값을 먼저 캡처한 뒤 쓴다.
    /// </summary>
    /// <param name="source">읽을 원본이다.</param>
    public void AssignFrom(CampaignWorldGlobals source)
    {
        ArgumentNullException.ThrowIfNull(source);
        uint my = source.MySubscriber;
        uint login = source.LoginSubscriber;
        uint editor = source.Editor;
        bool post = source.MissionPost;
        MySubscriber = my;
        LoginSubscriber = login;
        Editor = editor;
        MissionPost = post;
    }
}
