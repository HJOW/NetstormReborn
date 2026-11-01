using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>Terr 서식 한 줄(NUL 포함 출력)을 만든다(004a89c0, local256, "Terr%02d", index의 같은 출력). 반환 뒤 현재 전역 표를 다시 조회한다.</summary>
/// <param name="index">영역 번호(0..19).</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate ReadOnlySpan<byte> TemplateSectionNameFormat(uint index);

/// <summary>Template 섹션 이름표 최초 생성이 쓰는 외부 장치 연결이다(원본 <c>TemplateSectionNamesHooks</c>). 새 할당의 미정 바이트는 allocator가 소유하며 이 부모는 초기화하지 않는다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class TemplateSectionNamesHooks
{
    /// <summary>실제 512바이트의 불투명 token 또는 0이다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>현재 token의 같은 물리 범위다. 조회 자체는 무효과이며 0을 정상 표로 만들지 않는다.</summary>
    public RawBytesLookup? Buffer { get; set; }

    /// <summary>"Terr%02d" 서식의 같은 NUL 포함 출력이다.</summary>
    public TemplateSectionNameFormat? Format { get; set; }
}

/// <summary>
/// 원문 004bccb0의 Client 전역 005c8aa4 lazy 이름표 생성이다: 최초 할당→현재 전역 공개→277바이트 쓰기이며 비영 현재값이면 하위 장치 없이 그대로 반환한다.
/// 표 끝의 미정 235바이트를 채우지 않으며 SNART 변환은 실제로 선택할 때만 수행한다. cpppj <c>o/RawTemplateSectionNames</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawTemplateSectionNames
{
    /// <summary>실제 operator new의 크기(512)다.</summary>
    public const uint Capacity = 512;

    /// <summary>정상 신규 표에서 실제로 쓰는 접두부와 마지막 NUL까지의 바이트 수(277)다.</summary>
    public const int WrittenBytes = 277;

    /// <summary>원문 기본 135문자의 섹션 이름 접두부다.</summary>
    private const string BaseNames = "/Subscriber/State/Mission/CoreData/Chaff/Badges/Territory/TypeNames/CompressedData/Technology/Money/Deck/Reserved2/Reserved3/Reserved4/";

    /// <summary>Terr 영역 이름의 개수다.</summary>
    private const uint TerritoryNames = 20;

    /// <summary>원문 DAT00507bd8의 동일한 2바이트('/' + NUL)다.</summary>
    private static readonly byte[] Slash = [(byte)'/', 0];

    /// <summary>이미 쓴 앞 바이트는 물리 실패 때도 보존한 채 한 바이트를 쓴다. 범위 밖이면 거부한다.</summary>
    /// <param name="bytes">이름표 물리 바이트.</param>
    /// <param name="at">쓸 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Byte(Span<byte> bytes, int at, byte value)
    {
        if (at >= bytes.Length)
        {
            throw OutOfRange("Template 이름표 실제 쓰기범위");
        }
        bytes[at] = value;
    }

    /// <summary>같은 현재 표의 최초 NUL 위치를 찾는다. 현재 실제 범위만 앞으로 읽는다.</summary>
    /// <param name="bytes">읽을 물리 바이트.</param>
    private static int End(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw OutOfRange("Template 이름표 현재NUL 누락");
        }
        return end;
    }

    /// <summary>처음 읽은 NUL 포함 길이만 현재 끝에 앞으로 복사한다.</summary>
    /// <param name="bytes">이름표 물리 바이트.</param>
    /// <param name="text">덧붙일 NUL 종료 문자열.</param>
    private static void Append(Span<byte> bytes, ReadOnlySpan<byte> text)
    {
        int length = End(text) + 1;
        int end = End(bytes);
        // 복사할 길이만큼 현재 끝 뒤로 한 바이트씩 쓴다
        for (int at = 0; at < length; at++)
        {
            Byte(bytes, end + at, text[at]);
        }
    }

    /// <summary>
    /// 이름표가 아직 없으면(current==0) 512바이트를 할당하고 현재 전역에 공개한 뒤 접두부 135문자·Terr00~19 각 이름과 '/'·마지막 '/'를 쓴다. 이미 있으면 하위 장치 없이 그대로 반환한다.
    /// 표 끝의 미정 바이트는 읽거나 쓰지 않는다.
    /// </summary>
    /// <param name="current">Client 전역 005c8aa4의 현재 token. 최초 할당 직후 공개된다.</param>
    /// <param name="hooks">할당·버퍼·서식 장치.</param>
    /// <returns>현재 token.</returns>
    public static uint Get(ref uint current, TemplateSectionNamesHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (current != 0)
        {
            return current;
        }
        current = Need(hooks.Allocate, "Template 이름표의 선택된 실제 하위 미연결")(Capacity);
        RawBytesLookup buffer = Need(hooks.Buffer, "Template 이름표의 선택된 실제 하위 미연결");
        Span<byte> bytes = buffer(current);
        if (current == 0)
        {
            throw OutOfRange("원문004bcced의NULL 이름표복사");
        }
        // 원문 0x22 DWORD의 136바이트를 같은 순서로 복사한다. 할당 나머지는 읽거나 쓰지 않는다
        for (int at = 0; at < BaseNames.Length; at++)
        {
            Byte(bytes, at, (byte)BaseNames[at]);
        }
        Byte(bytes, BaseNames.Length, 0);
        // 각 Terr 서식은 ASCII 숫자 두 자리이며 실제 표의 현재 NUL 뒤로 붙인다
        for (uint index = 0; index < TerritoryNames; index++)
        {
            ReadOnlySpan<byte> name = Need(hooks.Format, "Template 이름표의 선택된 실제 하위 미연결")(index);
            Append(buffer(current), name);
            Append(buffer(current), Slash);
        }
        Append(buffer(current), Slash);
        return current;
    }
}
