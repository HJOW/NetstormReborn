using System.Diagnostics;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Host;

/// <summary>
/// 메인 Edit의 현재 Fort 이름·목록·다음 State를 같은 Client 수명에 보유한다. 목록과 파일 읽기는 편집 작업본의 이관 장부를 거치며
/// 그 작업본 장치(<see cref="Workspace"/>)를 새 이름 확인 등 다른 Edit 소비자가 함께 쓴다. (작업본 연결은 2026-11-01 0.126)
/// </summary>
public sealed class HostEditorSelection : IDisposable
{
    private readonly HostStartupScene _startup;
    private readonly ScriptStringRing _strings;
    private readonly Action<string> _tell;
    /// <summary>같은 Client의 선택 전역과 향후 State 실행 블록이다.</summary>
    public FortSelectionGlobals State { get; }
    /// <summary>다음 파일 선택에 사용하는 실제 pending 이름이다.</summary>
    public string PendingName { get; private set; } = "";
    /// <summary>월드 설치 시 바뀌는 현재 Fort 이름이다.</summary>
    public string CurrentName { get; set; } = "";
    /// <summary>월드 해제 후에도 남는 현재 사용자 이름이다.</summary>
    public string UserName { get; set; } = "";
    /// <summary>실제 맵 파일과 같은 Config를 쓰는 목록이다.</summary>
    public SoloMapCatalog Catalog { get; }
    /// <summary>목록·읽기·새 이름 확인이 공유하는 편집 작업본과 이관 장부다. 이 선택기가 수명을 소유한다.</summary>
    public EditorCatalogWorkspace Workspace { get; }

    /// <summary>
    /// 실제 파일·설정·스크립트 큐와 Tell 표시 장치를 빌린다. <paramref name="workspace"/>를 생략하면 시작 자산 폴더의 형제 작업본을 쓰며,
    /// 주어진 작업본의 소유권은 이 선택기가 넘겨받아 <see cref="Dispose"/>에서 반납한다.
    /// </summary>
    public HostEditorSelection(HostStartupScene startup, ScriptStringRing strings, Action<string> tell, uint[]? next = null, EditorCatalogWorkspace? workspace = null)
    {
        _startup = startup; _strings = strings; _tell = tell; State = new FortSelectionGlobals(next);
        Workspace = workspace ?? new EditorCatalogWorkspace(startup.Files);
        string language = startup.Configuration.Configuration.Get("currentLanguage") ?? "english";
        byte[]? bytes = startup.Files.TryReadAllBytes("d/xlat." + language);
        var translation = bytes == null ? null : Netstorm.Assets.XlatTable.FromFileBytes(bytes);
        Catalog = new SoloMapCatalog(startup.Files, startup.Configuration, new SoloMapCatalogEffects
        {
            TypeCount = () => PatchTypeCount,
            AllocationBytes = _ => throw new InvalidOperationException("맵 목록의 빈 State 미정 할당 바이트 미연결"),
            UserName = () => UserName,
            Translate = text => translation?.Translate(text) ?? text,
            EnumeratePaths = Workspace.Enumerate,
            ReadFile = Workspace.Read
        });
    }

    /// <summary>
    /// 10.78 TYPE 표의 개수다. 맵 목록의 임시 Template이 파일 서명 검사에서 읽는다.
    /// 실제 TYPE 표(<c>RiftTypeTable</c>, F1)가 들어오면 그 현재 개수로 바꾼다. (2026-11-02 TS6)
    /// </summary>
    private const int PatchTypeCount = 188;

    /// <summary>작업본 장부 잠금을 반납한다. 기록된 장부와 작업본은 보존한다.</summary>
    public void Dispose() => Workspace.Dispose();

    /// <summary>실제 입력 큐의 편집 선택을 같은 목록과 Tell에 전달한다.</summary>
    public bool Dispatch(InputEvent input) => FortSelection.Dispatch(input, State, new FortSelectionHooks
    {
        ResolveString = _strings.Resolve,
        LastEditPath = () => _startup.Configuration.PathSpec("lastEditFort"),
        Pending = name => PendingName = FortSelection.PendingName(name, () => Debug.WriteLine("Fort pending 이름의 점 진단")),
        Refresh = mode => Catalog.List.Refresh(mode), Tell = _tell
    });

    /// <summary>State 소비 전에 미이식 월드 경계를 명시한다.</summary>
    public void Poll()
    {
        if (State.Next[0] != 0) throw new InvalidOperationException("편집 Fort State 설치와 월드 소유자 미연결: " + State.Next[0]);
    }
}
