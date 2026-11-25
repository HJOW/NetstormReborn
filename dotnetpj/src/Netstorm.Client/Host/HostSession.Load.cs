// 실제 Load·Editor 설치 부모가 세 설치 값을 원문 순서로 공개한다.
namespace Netstorm.Client.Host;

/// <summary>캠페인 Load의 여덟 실제 하위 단계다. 전부 정상 복귀해야 loaded를 공개한다.</summary>
/// <remarks>2026-11-25 0.997에서 RU-6용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 4344~4350행의 Load(Building/Motion→runtime.Load→Devastation/CarrierDrop/PriestPrayer/Builder/Placement/Cursor). 하위 본체는 R5/R6에서 잇고 여기서는 순서·호출·공개만 실제 세션에 둔다.</remarks>
public sealed class HostSessionLoadHooks
{
    /// <summary>Source Load의 실제 건설 연결이다.</summary>
    public Action? Building { get; set; }
    /// <summary>Source Load의 실제 경로 연결이다.</summary>
    public Action? Motion { get; set; }
    /// <summary>실제 runtime.Load다.</summary>
    public Action? RuntimeLoad { get; set; }
    /// <summary>Source Load의 실제 파편 연결이다.</summary>
    public Action? Devastation { get; set; }
    /// <summary>Source Load의 실제 운반체 낙하 연결이다.</summary>
    public Action? CarrierDrop { get; set; }
    /// <summary>Source Load의 실제 사제 기도 연결이다.</summary>
    public Action? PriestPrayer { get; set; }
    /// <summary>Source Load의 실제 건설자 작성이다.</summary>
    public Action? Builder { get; set; }
    /// <summary>Source Load의 실제 배치 연결이다.</summary>
    public Action? Placement { get; set; }
    /// <summary>Source Load의 실제 커서 연결이다.</summary>
    public Action? Cursor { get; set; }
}

/// <summary>세 설치 값의 원문 순서 공개와 Editor 한정 동기 범위다.</summary>
public sealed partial class HostSession
{
    /// <summary>편집기 prefix UI의 한 번 설치 래치다. 캠페인 Load와 중복 진입을 막는다.</summary>
    private bool _editorPrefixUiStarted;
    /// <summary>편집기 후반 UI의 한 번 설치 래치다. 캠페인 Load와 중복 진입을 막는다.</summary>
    private bool _editorUiStarted;
    /// <summary>편집기 후반 UI의 실제 tail 정상 복귀다. 최종 공개(R9)에서만 소비한다.</summary>
    private bool _editorUiTailReady;

    /// <summary>전체 Source Load와 후반 UI가 정상 복귀한 실제 공개 값이다. 부분 적재는 이 값을 쓰지 않는다.</summary>
    public bool NativeLoaded => _nativeLoaded;
    /// <summary>편집기 후반 UI 설치의 실제 한정 동기 범위다.</summary>
    public bool EditorUiInstalling => _editorUiInstalling;
    /// <summary>편집기 prefix UI 설치의 실제 한정 동기 범위다.</summary>
    public bool EditorPrefixUiInstalling => _editorPrefixUiInstalling;
    /// <summary>편집기 후반 UI의 실제 tail 정상 복귀다.</summary>
    public bool EditorUiTailReady => _editorUiTailReady;

    /// <summary>선택된 실제 하위 장치만 요구하며 누락을 빈 성공으로 바꾸지 않는다.</summary>
    private static Action NeedLoadStep(Action? step) => step ?? throw new InvalidOperationException("실제 적재 하위 장치 미연결");

    /// <summary>여덟 실제 하위 단계를 원문 순서로 실행하고 전부 정상 복귀한 뒤에만 loaded를 공개한다.</summary>
    /// <param name="hooks">원문 순서의 실제 여덟 하위 장치다. 하나라도 없으면 시작 전에 거부한다.</param>
    /// <remarks>2026-11-25 0.997에서 RU-6용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 4344~4350행의 Load. 하위 본체는 R5/R6에서 잇는다. 부분 실행의 부수 효과는 되돌리지 않으며(원문과 동일) loaded는 마지막 한 곳에서만 쓴다. 공개 Runtime이나 부분 적재를 loaded로 대신하지 않는다.</remarks>
    public void Load(HostSessionLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (_nativeLoaded) throw new InvalidOperationException("이미 공개된 세션 적재 중복");
        if (_editorPrefixUiStarted || _editorUiStarted) throw new InvalidOperationException("편집기 설치 경로의 캠페인 적재 거부");
        Action[] steps = [NeedLoadStep(hooks.Building), NeedLoadStep(hooks.Motion), NeedLoadStep(hooks.RuntimeLoad), NeedLoadStep(hooks.Devastation), NeedLoadStep(hooks.CarrierDrop), NeedLoadStep(hooks.PriestPrayer), NeedLoadStep(hooks.Builder), NeedLoadStep(hooks.Placement), NeedLoadStep(hooks.Cursor)];
        foreach (Action step in steps) step();
        _nativeLoaded = true;
    }

    /// <summary>실제 State1/2/6 준비 대여의 파일 전 HUD 동기 범위를 한 번 연다. 동기 scope만 닫고 만든 효과는 남긴다.</summary>
    /// <param name="prepareUi">실제 prefix UI 준비다.</param>
    /// <remarks>2026-11-25 0.997에서 RU-6용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 4411~4417행의 WithExistingEditorPrefixUi. 게임 입력/최종 loaded를 열지 않는다.</remarks>
    public void WithEditorPrefixUi(Action prepareUi)
    {
        ArgumentNullException.ThrowIfNull(prepareUi);
        if (_nativeLoaded) throw new InvalidOperationException("공개된 세션의 편집기 prefix 진입");
        if (_editorPrefixUiStarted) throw new InvalidOperationException("편집기 prefix 한 번 설치 중복");
        _editorPrefixUiStarted = true;
        _editorPrefixUiInstalling = true;
        try { prepareUi(); }
        finally { _editorPrefixUiInstalling = false; }
    }

    /// <summary>실제 checkpoint 정상 복귀 뒤 후반 UI 생성의 한정 동기 범위를 한 번 연다. 정상 복귀만 tail 완료를 기록한다.</summary>
    /// <param name="prepareUi">실제 후반 UI 준비다.</param>
    /// <remarks>2026-11-25 0.997에서 RU-6용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 4418~4426행의 WithExistingEditorInstallUi. scope만 반납하고 이미 생긴 효과는 되돌리지 않는다. 최종 공개는 R9에서 잇는다.</remarks>
    public void WithEditorInstallUi(Action prepareUi)
    {
        ArgumentNullException.ThrowIfNull(prepareUi);
        if (_nativeLoaded) throw new InvalidOperationException("공개된 세션의 편집기 후반 UI 진입");
        if (_editorUiStarted) throw new InvalidOperationException("편집기 후반 UI 한 번 설치 중복");
        _editorUiStarted = true;
        _editorUiInstalling = true;
        try
        {
            prepareUi();
            _editorUiTailReady = true;
        }
        finally { _editorUiInstalling = false; }
    }
}
