using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>
/// 현재 Fort 이름과 대기(pending) Fort 이름, 목록·설정 dirty의 실제 Client 수명을 한곳에 보관한다. 메뉴와 짧은 Session이 같은 이름·키·목록 물리를 빌린다.
/// 원문 두 이름 버퍼(256바이트 할당 둘)와 FileSpec 포인터·목록·선택 모드의 단일 소유자다. cpppj <c>client/ClientFortGlobals</c>를 옮겼다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. 이름 버퍼는 NUL 뒤 미작성 바이트를 임의로 초기화하지 않는 고정 256바이트이며, 문자열로 읽을 때는 첫 NUL 앞을 UTF-8로 해석한다
/// (cpppj의 이름은 UTF-8 <c>std::string</c>이다. 이름 버퍼에 쓰는 바이트가 UTF-8이므로 한글 작업 경로가 그대로 오간다).
/// cpppj의 <c>Catalog()</c>·<c>SelectionMode()</c>는 참조 반환이지만 C#은 같은 객체와 쓰기 가능한 속성이다.
/// </remarks>
public sealed class ClientFortGlobals
{
    /// <summary>이름 버퍼 한 개의 실제 폭이다(원문 할당 크기).</summary>
    public const int NameBytes = 256;

    /// <summary>0055b490이 가리키는 현재 이름의 실제 쓰기 버퍼다.</summary>
    private readonly byte[] _current = new byte[NameBytes];

    /// <summary>0055b494의 별개 256바이트 대기 이름 버퍼다. 현재 이름에 복사하지 않는다.</summary>
    private readonly byte[] _pending = new byte[NameBytes];

    /// <summary>005327ac의 같은 9바이트 키다. SelectSolo/SelectFort를 해도 주소(객체)는 변하지 않는다.</summary>
    private readonly SoloMapFileSpec _fileSpec = new();

    /// <summary>005327e0이 현재 가리키는 FileSpec 키다. 원문 기본 포인터도 처음부터 같은 고정 키를 가리킨다.</summary>
    private SoloMapFileSpec _selectedFileSpec;

    /// <summary>
    /// 이름 두 버퍼를 각 첫 바이트만 빈 문자열로 초기화한다(NUL 뒤는 쓰지 않는다). 목록 상태의 최초 dirty는 보호 PE 00542678의 값 1이다.
    /// </summary>
    public ClientFortGlobals()
    {
        _current[0] = 0;
        _pending[0] = 0;
        _selectedFileSpec = _fileSpec;
        Catalog.Dirty = 1;
    }

    /// <summary>이름과 목록·설정이 같이 쓰는 00542678 dirty를 포함한 고정 50행 캐시다.</summary>
    public SoloMapListState Catalog { get; } = new();

    /// <summary>005cae84의 현재 ReadFort/AskEditFort 모드다. 최초 BSS 값은 0이다.</summary>
    public uint SelectionMode { get; set; }

    /// <summary>005327ac의 같은 9바이트 키다. SelectSolo/SelectFort를 해도 같은 객체다.</summary>
    public SoloMapFileSpec FileSpec => _fileSpec;

    /// <summary>0055b490이 가리키는 현재 이름의 실제 쓰기 버퍼 256바이트다. 호출마다 같은 물리 버퍼를 빌린다.</summary>
    public Span<byte> CurrentBytes() => _current;

    /// <summary>0055b494의 별개 256바이트 대기 이름 버퍼다. 현재 이름에 복사하지 않는다.</summary>
    public Span<byte> PendingBytes() => _pending;

    /// <summary>현재 이름을 첫 NUL 앞까지 읽는다. 미종료 버퍼를 빈 문자열로 만들지 않고 거부한다.</summary>
    /// <exception cref="ArgumentOutOfRangeException">256바이트 안에 NUL이 없다.</exception>
    public string Current() => Name(_current);

    /// <summary>대기 이름을 같은 물리 버퍼에서 읽는다. 별도 문자열 동기화를 하지 않는다.</summary>
    /// <exception cref="ArgumentOutOfRangeException">256바이트 안에 NUL이 없다.</exception>
    public string Pending() => Name(_pending);

    /// <summary>005327e0을 같은 고정 키의 주소로 지정한다. 키 내용은 보존하고 포인터 선택만 구별한다.</summary>
    public void SelectFileSpec() => _selectedFileSpec = _fileSpec;

    /// <summary>현재 선택 포인터의 키를 PathSpec 소비 위치에서 읽는다. 각 경로 작성 때 현재 키를 다시 조회한다.</summary>
    public string SelectedFileSpec() => _selectedFileSpec.Current();

    /// <summary>NUL 뒤의 미작성 바이트를 읽지 않고 현재 C 문자열만 UTF-8 문자열로 돌려준다.</summary>
    /// <param name="bytes">이름 버퍼.</param>
    private static string Name(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(null, "Client Fort 이름의 NUL 물리 범위");
        }
        return Encoding.UTF8.GetString(bytes[..end]);
    }
}
