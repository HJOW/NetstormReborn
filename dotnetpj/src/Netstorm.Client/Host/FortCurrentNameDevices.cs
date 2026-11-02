using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 기존 현재 Fort 이름 부모(<see cref="RawFortCurrentName.SetCurrentName"/>)를 실제 Client 이름 버퍼·Template 저장소·설정·작업본 파일 장치에 연결한다.
/// 게임 알고리즘은 원본 이름 부모 본체만 실행하고, 물리 버퍼·경로·CRT 비교·설정 장치만 같은 소유자에 연결한다.
/// cpppj <c>client/FortCurrentNameDevices</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. 이름 바이트는 UTF-8이다(한글 작업 경로 지원). 네 외부 소유자(이름 전역, 저장소, 설정, 작업본)는 이 객체보다 오래 살아야 한다.
/// cpppj는 생성자에서 두 dirty DWORD의 주소가 같은지 검사하지만 C#의 <see cref="FortCurrentNameGlobals"/>는 읽기·쓰기 대리자 쌍이라 주소를 비교할 수 없다.
/// 호출자는 <c>Dirty</c>와 <c>SetDirty</c>를 <see cref="ClientFortGlobals.Catalog"/>의 같은 dirty에 연결해야 한다(검사가 효과로 확인한다).
/// </remarks>
public sealed class FortCurrentNameDevices
{
    /// <summary>같은 장기 물리(이름 두 버퍼, 목록 dirty, 파일 명세 키)를 빌린다.</summary>
    private readonly ClientFortGlobals _names;

    /// <summary>manager 이름 바이트를 빌리고 Notify 파일 저장이 소비하는 같은 저장소다.</summary>
    private readonly CampaignTemplateStorage _storage;

    /// <summary>PathSpec 해석과 lastEditFort 기록에 쓰는 현재 설정이다.</summary>
    private readonly ConfigInterface _config;

    /// <summary>현재 getter와 dirty 쓰기를 가진 전역이다.</summary>
    private readonly FortCurrentNameGlobals _globals;

    /// <summary>저장소·이름·경로·비교·삭제·설정을 연결한 실제 동기 경계다.</summary>
    private readonly FortCurrentNameHooks _hooks;

    /// <summary>실제 작업 namespace 선택이다. 없으면 기존 독립 경로 해석을 유지하며 삭제 요청은 명시 오류다.</summary>
    private readonly EditorFiles? _work;

    /// <summary>다음 경로 호출 전까지 유효한 NUL 종료 경로 바이트(UTF-8)다.</summary>
    private byte[] _path = [0];

    /// <summary>
    /// 이름·경로 scratch와 동기 장치 대여를 만든다. Notify·전체 snapshot·제목·State·진단은 <paramref name="hooks"/>에서 받는다.
    /// <paramref name="work"/>를 주면 PathSpec 결과를 그 작업 namespace 경로로 해석하고, <paramref name="resolve"/>가 있으면 절대 source 경로를 같은 작업 namespace로 매핑한다.
    /// </summary>
    /// <param name="names">이름 두 버퍼와 파일 명세 키의 단일 소유자.</param>
    /// <param name="storage">현재 Template manager 저장소.</param>
    /// <param name="config">현재 설정.</param>
    /// <param name="globals">현재 owner·모드·dirty 전역. <c>Dirty</c>는 <see cref="ClientFortGlobals.Catalog"/>의 dirty와 같아야 한다.</param>
    /// <param name="hooks">Notify/전체 snapshot/제목/State/진단. 이 생성자가 헤더·manager 이름을 채운다.</param>
    /// <param name="work">실제 작업 namespace(선택).</param>
    /// <param name="resolve">작업 namespace의 명시 경로 매핑(선택). 설정 경로를 받아 검사된 절대 경로를 돌려준다.</param>
    public FortCurrentNameDevices(ClientFortGlobals names, CampaignTemplateStorage storage, ConfigInterface config, FortCurrentNameGlobals globals,
        FortCurrentNameHooks hooks, EditorFiles? work = null, Func<string, string>? resolve = null)
    {
        ArgumentNullException.ThrowIfNull(names);
        ArgumentNullException.ThrowIfNull(storage);
        ArgumentNullException.ThrowIfNull(config);
        ArgumentNullException.ThrowIfNull(globals);
        _names = names;
        _storage = storage;
        _config = config;
        _globals = globals;
        _work = work;
        _hooks = storage.BindCurrentName(hooks);
        _hooks.CurrentName = () => _names.CurrentBytes();
        _hooks.SelectFileSpec = () => _names.SelectFileSpec();
        _hooks.PathSpec = name => ResolvePath(name, resolve);
        _hooks.SamePath = SamePath;
        _hooks.DeleteRequest = path =>
        {
            if (_work is null)
            {
                throw new InvalidOperationException("현재 Fort 삭제의 실제 작업 namespace 미연결");
            }
            _work.Delete(Text(path));
        };
        _hooks.Config = (key, value) =>
        {
            ReadOnlySpan<byte> text = value[..TextLength(value)];
            _config.Set(key, OriginalText.Decode(text));
        };
    }

    /// <summary>
    /// 원문 4410f0의 인자 없는 현재 조회가 끝난 뒤 기존 460980을 호출한다(PATCH 00460c20 / CD 004ca8e0). 현재 newFort의 PathSpec 결과를 기존 전체 이름 부모에 전달하며
    /// 이름이 다시 비어도 여기서 반복하지 않고 재검사하거나 임의 이름을 추가하지 않는다.
    /// </summary>
    /// <param name="config">현재 설정.</param>
    /// <param name="select">기존 전체 이름 선택 장치.</param>
    public static void SelectConfiguredNewFort(ConfigInterface config, Action<string>? select)
    {
        ArgumentNullException.ThrowIfNull(config);
        string name = config.PathSpec("newFort");
        if (select is null)
        {
            throw new InvalidOperationException("기본 Fort의 실제 전체 이름 선택 장치 미연결");
        }
        select(name);
    }

    /// <summary>NUL 포함 같은 source와 실제 원문 256바이트 지역 scratch로 전체 이름 부모를 실행한다.</summary>
    /// <param name="name">새 이름(NUL 포함 UTF-8 바이트). 동기 부모가 끝날 때까지 안정되어야 한다.</param>
    public void Set(ReadOnlySpan<byte> name)
    {
        // 원문 지역 미작성 scratch이며 선택된 oldPath 복사가 먼저 채운다
        Set(name, new byte[RawFortCurrentName.PathBytes]);
    }

    /// <summary>Native 입력·진단 검사가 명시한 지역 물리를 그대로 제공할 수 있는 판이다. 두 snapshot·Notify·삭제와 부분 효과는 같은 기존 본체가 소유한다.</summary>
    /// <param name="name">새 이름(NUL 포함 UTF-8 바이트).</param>
    /// <param name="oldPathScratch">이전 경로를 담는 256바이트 scratch.</param>
    public void Set(ReadOnlySpan<byte> name, Span<byte> oldPathScratch)
    {
        RawFortCurrentName.SetCurrentName(name, oldPathScratch, _globals, _hooks);
    }

    /// <summary>
    /// 선택한 FileSpec 키로 이름을 설정 경로로 바꾸고, 작업 namespace가 있으면 그 경로로 해석한다. 절대 source 매핑은 명시 장치만 허용한다.
    /// Windows 10/11 한글 경로는 원본 ANSI 대신 UTF-8 바이트이며 writer 보호는 유지한다.
    /// </summary>
    /// <param name="name">NUL 포함 이름 바이트.</param>
    /// <param name="resolve">명시 경로 매핑(선택).</param>
    private ReadOnlySpan<byte> ResolvePath(ReadOnlySpan<byte> name, Func<string, string>? resolve)
    {
        string path = _config.PathSpec(_names.SelectedFileSpec(), Text(name));
        if (_work is not null)
        {
            path = resolve is not null
                ? resolve(path)
                : _work.ResolvePath(Path.IsPathFullyQualified(path) ? path : EditorFortFileDevices.NormalizeGamePath(path));
        }
        byte[] bytes = Encoding.UTF8.GetBytes(path);
        _path = new byte[bytes.Length + 1];
        bytes.CopyTo(_path, 0);
        return _path;
    }

    /// <summary>원문 CRT 비교(004e65ed)다: 두 NUL 종료 경로를 ASCII 대소문자를 무시하고 같은지 본다. ASCII 밖 바이트는 그대로 비교한다.</summary>
    /// <param name="first">첫째 NUL 종료 경로.</param>
    /// <param name="second">둘째 NUL 종료 경로.</param>
    private static bool SamePath(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second)
    {
        ReadOnlySpan<byte> left = first[..TextLength(first)], right = second[..TextLength(second)];
        if (left.Length != right.Length)
        {
            return false;
        }
        // 같은 위치의 바이트를 ASCII 소문자로 맞춰 비교한다
        for (int index = 0; index < left.Length; index++)
        {
            if (Lower(left[index]) != Lower(right[index]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>ASCII 대문자만 소문자로 바꾼다(C 로캘의 CRT 비교와 같다).</summary>
    /// <param name="value">바이트.</param>
    private static byte Lower(byte value) => value is >= (byte)'A' and <= (byte)'Z' ? (byte)(value + 32) : value;

    /// <summary>첫 NUL 앞의 길이를 돌려준다. NUL이 없는 임의 입력은 거부한다.</summary>
    /// <param name="bytes">NUL 종료 바이트.</param>
    private static int TextLength(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(null, "Fort 이름 장치의 NUL 범위");
        }
        return end;
    }

    /// <summary>NUL 종료 UTF-8 바이트를 실제 문자열로 읽는다.</summary>
    /// <param name="bytes">NUL 종료 바이트.</param>
    private static string Text(ReadOnlySpan<byte> bytes) => Encoding.UTF8.GetString(bytes[..TextLength(bytes)]);
}
