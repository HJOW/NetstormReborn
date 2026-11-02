using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 현재 Fort의 이름·고정 FileSpec 선택·진단을 상위 소유자에게서 빌리는 효과 묶음이다.
/// 파일 장치는 이 값들을 쓸 때마다 조회하며 생성자에서 미리 거부하지 않는다. 필요한 순간에 연결되지 않은 항목은 명시 오류다.
/// cpppj <c>client/EditorFortFileDevices.h</c>의 <c>EditorFortPathEffects</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.127에서 추가했다.</remarks>
public sealed record EditorFortPathEffects
{
    /// <summary>0055b490의 현재 NUL 제외 Fort 이름이다. pending 이름이나 미션 이름의 사본으로 대체하지 않는다.</summary>
    public Func<string>? CurrentName { get; init; }
    /// <summary>005327e0이 같은 고정 9바이트 FileSpec 버퍼를 가리키게 한다. 버퍼를 fortSpec으로 덮지 않는다.</summary>
    public Action? SelectFileSpec { get; init; }
    /// <summary>선택한 기존 SoloMapFileSpec 현재 키(예: <c>fortSpec</c>)를 읽는다.</summary>
    public Func<string>? FileSpecKey { get; init; }
    /// <summary>빈 현재 이름의 Fortspec.cpp 39번 진단과 stat 실패 진단을 받는다. 진단 뒤 반환하면 원문대로 계속한다.</summary>
    public Action<uint>? Assertion { get; init; }
    /// <summary>선택 사항. 설정 경로를 작업 절대 경로로 해석하는 실제 namespace 매핑으로, 원본·작업본 절대 경로를 같은 이관 정책에 연결한다.</summary>
    public Func<string, string>? Resolve { get; init; }
    /// <summary>선택 사항. 실제 이관·삭제 완료 기록 준비다. 생략하면 독립 호출처럼 없는 작업본을 원본에서 한 번 복사한다.</summary>
    public Action<string>? Prepare { get; init; }
}

/// <summary>
/// 편집(Edit)이 현재 Fort 한 개에 대해 쓰는 실제 작업 파일 장치다. 현재 이름과 Config 경로 지정으로 작업본 경로를 구하고,
/// 없는 작업본만 원본에서 복사해 준비하며, 존재·쓰기 금지·읽기를 작업 namespace에서만 수행한다.
/// 전체 Editor 부모를 복제하지 않고 작업 파일 장치(<see cref="EditorFiles"/>)·원본 파일 시스템·Config를 빌려 쓴다.
/// cpppj <c>client/EditorFortFileDevices</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.127에서 추가했다. 생성만으로는 파일이나 manager를 읽거나 쓰지 않는다. 반환하는 경로 문자열은 다음 호출까지 유효한 scratch가 아니라 값이다.
/// cpppj의 자유 함수 <c>ExistingEditorTemplateValid</c>는 Template 저장소가 옮겨진 뒤 2026-11-02 0.135(TS5)에서 정적 메서드 <see cref="ExistingEditorTemplateValid"/>로 옮겼다.
/// </remarks>
public sealed class EditorFortFileDevices
{
    /// <summary>Template 칸 수(원본 헤더 9개)다. <see cref="ExistingEditorTemplateValid"/>의 슬롯 범위다.</summary>
    private const uint TemplateSlots = 9;

    /// <summary>Template 헤더 한 칸의 폭(바이트)이다. 첫 DWORD가 현재 manager 토큰이다.</summary>
    private const int TemplateHeaderBytes = 20;

    /// <summary>원본·CD 밖의 유일한 작업 namespace와 실제 권한·쓰기 장치다.</summary>
    private readonly EditorFiles _work;
    /// <summary>원본·아카이브를 읽기만 하는 파일 시스템이다. 준비 뒤의 읽기는 이것을 거치지 않는다.</summary>
    private readonly GameFileSystem _source;
    /// <summary>같은 Client의 실제 Config다. <c>PathSpec</c>을 그대로 호출한다.</summary>
    private readonly ConfigInterface _config;
    /// <summary>현재 이름·키·선택·진단을 빌려 오는 상위 저장소 효과다.</summary>
    private readonly EditorFortPathEffects _effects;
    /// <summary>마지막 <c>PathSpec</c> 결과다. 현재 Fort 이름의 소유자가 아니라 scratch다.</summary>
    private string _configuredPath = "";
    /// <summary>기존 게임 경로 규칙으로 해석한 작업 namespace 경로다. 상대 경로만 정규화한다.</summary>
    private string _namespacePath = "";
    /// <summary>다음 <see cref="CurrentPath"/>·<see cref="PrepareCurrent"/>까지 유효한 검사된 작업 절대 경로다.</summary>
    private string _workingPath = "";

    /// <summary>세 장기 장치와 현재 이름 효과를 빌린다. 생성만으로 파일이나 manager를 읽거나 쓰지 않는다.</summary>
    /// <param name="work">작업 namespace 파일 장치.</param>
    /// <param name="source">원본·아카이브 읽기 전용 파일 시스템.</param>
    /// <param name="config">현재 Client의 Config.</param>
    /// <param name="effects">현재 이름·FileSpec·진단 효과.</param>
    public EditorFortFileDevices(EditorFiles work, GameFileSystem source, ConfigInterface config, EditorFortPathEffects effects)
    {
        _work = work ?? throw new ArgumentNullException(nameof(work)); _source = source ?? throw new ArgumentNullException(nameof(source));
        _config = config ?? throw new ArgumentNullException(nameof(config)); _effects = effects ?? throw new ArgumentNullException(nameof(effects));
    }

    /// <summary>실제로 필요한 현재 효과가 연결됐는지 확인한다. 연결되지 않았으면 정상 완료로 숨기지 않고 멈춘다.</summary>
    private static T Need<T>(T? effect) where T : class => effect ?? throw new InvalidOperationException("Editor 파일의 현재 장치 미연결");

    /// <summary>원본처럼 ASCII 대문자만 소문자로 바꾼다. 나머지 문자는 보존한다.</summary>
    internal static string AsciiLower(string text) => string.Concat(text.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));

    /// <summary>
    /// 기존 파일 시스템이 게임 폴더 기준으로 읽는 경로 표기를 정규화한다. ASCII 소문자, 역슬래시→슬래시, 앞의 <c>./</c> 한 번과 앞쪽 슬래시를 제거한다.
    /// </summary>
    /// <param name="name">게임 경로 표기.</param>
    internal static string NormalizeGamePath(string name)
    {
        string result = AsciiLower(name).Replace('\\', '/');
        if (result.StartsWith("./", StringComparison.Ordinal)) result = result[2..];
        return result.TrimStart('/');
    }

    /// <summary>
    /// 00460680: 현재 이름을 진단하고 같은 FileSpec을 선택한 뒤 Config 경로 지정으로 설정 경로를 얻어 작업 namespace 경로로 해석한다.
    /// 호출마다 현재 이름과 키를 다시 조회하며 파일은 만들지 않는다.
    /// </summary>
    /// <returns>검사된 작업 절대 경로.</returns>
    public string CurrentPath()
    {
        string name = Need(_effects.CurrentName)();
        // 빈 현재 이름은 진단만 낸다. 진단 장치가 돌아오면 원문대로 이어서 같은 키를 선택한다.
        if (name.Length == 0 || name[0] == '\0') Need(_effects.Assertion)(39);
        Need(_effects.SelectFileSpec)(); string key = Need(_effects.FileSpecKey)(); string current = Need(_effects.CurrentName)();
        _configuredPath = _config.PathSpec(key, current);
        // 절대 경로는 그대로 두어 작업 루트 검사가 거부하게 하고, 상대 경로만 기존 게임 경로 규칙으로 정규화한다.
        _namespacePath = Path.IsPathFullyQualified(_configuredPath) ? _configuredPath : NormalizeGamePath(_configuredPath);
        _workingPath = _effects.Resolve is { } resolve ? resolve(_configuredPath) : _work.ResolvePath(_namespacePath);
        return _workingPath;
    }

    /// <summary>
    /// 호스트의 명시 작업본 준비 단계다. 최종 이름과 설정으로 고른 작업 파일의 부모 폴더만 만들고, 이미 열리는 작업 파일은 원본조차 다시 읽지 않고 보존한다.
    /// 없을 때만 원본·아카이브에서 읽어 복사한다. 원본 설치 폴더에는 쓰지 않는다.
    /// </summary>
    /// <returns>준비한 작업 절대 경로. 원본에도 없으면 파일은 만들어지지 않는다.</returns>
    public string PrepareCurrent()
    {
        CurrentPath(); _work.EnsureDirectory(Path.GetDirectoryName(_workingPath)!);
        // 실제 이관 준비 효과가 있으면 그것이 복사·삭제 완료 기록을 맡는다.
        if (_effects.Prepare is { } prepare) { prepare(_configuredPath); return _workingPath; }
        if (_work.Exists(_workingPath) != 0) return _workingPath;
        byte[]? bytes = _source.TryReadAllBytes(_configuredPath);
        if (bytes is not null) _work.PreparePath(_namespacePath, bytes);
        return _workingPath;
    }

    /// <summary>작업 파일을 열 수 있으면 출처 5, 아니면 0이다. 서명 파싱이나 manager 상태는 조회하지 않는다.</summary>
    /// <param name="path">검사된 작업 절대 경로.</param>
    public int Exists(string path) => _work.Exists(path);

    /// <summary>현재 작업 파일의 CRT 쓰기 금지 판정이다. Config의 <c>readOnlyForts</c>는 읽지 않는다.</summary>
    /// <param name="path">검사된 작업 절대 경로.</param>
    public bool ReadOnly(string path) => _work.ReadOnly(path, _effects.Assertion);

    /// <summary>
    /// 검사된 작업 절대 경로를 읽는다. 읽기 실패나 부재는 빈 배열이며 원본·아카이브로 되돌아가지 않는다.
    /// 준비 단계 뒤 삭제된 작업본을 원본에서 다시 읽는 일을 막기 위해 같은 작업 장치만 쓴다.
    /// </summary>
    /// <param name="path">검사된 작업 절대 경로.</param>
    public byte[] Read(string path) => _work.TryRead(path) ?? [];

    /// <summary>C 문자열처럼 첫 NUL까지만 비교 대상으로 삼는다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }

    /// <summary>
    /// 기존 Load 호출의 읽기·쓰기 금지 판정·이름 비교를 이 장치로 채운 복사본을 돌려준다. 이름 비교는 CRT <c>stricmp</c>처럼 ASCII만 대소문자를 접는다.
    /// 현재 owner·TYPE·제목·진단 항목은 호출자가 공급한 값을 그대로 둔다.
    /// </summary>
    /// <param name="hooks">호출자가 채운 나머지 항목을 가진 Load 경계.</param>
    public TemplateFileLoadHooks BindLoad(TemplateFileLoadHooks hooks) => hooks with
    {
        Read = Read,
        ReadOnly = ReadOnly,
        SameName = (first, second) => string.Equals(AsciiLower(CString(first)), AsciiLower(CString(second)), StringComparison.Ordinal)
    };

    /// <summary>
    /// 0041aad0: 지정한 칸의 현재 헤더 첫 DWORD(manager 토큰)가 0이 아니면 1, 0이면 0이다. 토큰 조회나 파일 서명을 추가로 검사하지 않으므로
    /// 호스트가 모르는 비영 토큰도 원문처럼 1이다. 이후 접근이 실패하는지는 별개다. 호스트가 편집 설치 부모의 <c>valid</c> 판정에 공급한다.
    /// </summary>
    /// <param name="storage">헤더 180바이트를 가진 Template 저장소.</param>
    /// <param name="slot">0~8.</param>
    /// <returns>manager가 있으면 1, 없으면 0.</returns>
    /// <exception cref="ArgumentOutOfRangeException">슬롯이 9칸 밖이다.</exception>
    /// <remarks>2026-11-02 0.135(TS5)에서 추가했다. cpppj <c>client/EditorFortFileDevices</c>의 <c>ExistingEditorTemplateValid</c>를 옮겼다.</remarks>
    public static int ExistingEditorTemplateValid(CampaignTemplateStorage storage, uint slot)
    {
        ArgumentNullException.ThrowIfNull(storage);
        if (slot >= TemplateSlots)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), slot, "Editor valid의 실제 9슬롯 범위");
        }
        ReadOnlySpan<byte> header = storage.Headers.Span.Slice((int)slot * TemplateHeaderBytes, 4);
        return header[0] != 0 || header[1] != 0 || header[2] != 0 || header[3] != 0 ? 1 : 0;
    }
}
