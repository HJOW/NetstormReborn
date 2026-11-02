namespace Netstorm.Core.Original;

/// <summary>
/// .fort 파일의 원시 섹션 35개와 서명 flag, 그리고 TypeNames 해시로 만든 타입 번호 변환표를 담는다(cpppj <c>o/Template.h</c>의 <c>FortTemplate</c>).
/// 0.128 단위에서는 RiftTypeTable 없이 쓸 수 있는 저장 자료와 조회(섹션 이름 순서, 이름별 섹션, 타입 번호 변환)만 옮겼고,
/// F1(2026-11-02)에서 타입 표가 필요한 <see cref="Parse"/>와 객체·내용물·덱 해석(<see cref="FortObject"/>, <see cref="FortChunkSection"/> 등)을 같은 클래스에 더했다(<c>FortTemplate.Parse.cs</c>).
/// </summary>
/// <remarks>
/// 2026-11-01 0.128에서 추가했다. 기존 <c>Netstorm.Assets.FortFile</c>은 같은 파일 형식의 이전 독립 판독기이며 이 클래스와 연결하지 않는다(은퇴는 R4 Fort 적재 단위).
/// 같은 날 0.130(TS0)에서 <see cref="Sections"/>를 <c>byte[]</c> 목록에서 용량을 가진 <see cref="RawSectionBuffer"/> 목록으로 바꾸고 <see cref="Clone"/>을 더했다.
/// </remarks>
public sealed partial class FortTemplate
{
    /// <summary>실행 파일의 섹션 이름 목록(패치 005146b0 15개 + Terr00~Terr19) 길이다.</summary>
    public const int SectionCount = 35;

    /// <summary>영역(Terr) 섹션의 수다.</summary>
    public const int TerritoryCount = 20;

    /// <summary>타입 바이트가 이 값 이상이면 다리 약식 표기로 변환표를 거치지 않는다(소유자 = 0xff - 값).</summary>
    public const byte BridgeShorthandMin = 0xf6;

    /// <summary>저장 순서의 섹션 이름이다. 위치가 곧 파일 안의 섹션 순서다.</summary>
    private static readonly string[] Names =
    [
        "Subscriber", "State", "Mission", "CoreData", "Chaff", "Badges", "Territory", "TypeNames", "CompressedData",
        "Technology", "Money", "Deck", "Reserved2", "Reserved3", "Reserved4",
        "Terr00", "Terr01", "Terr02", "Terr03", "Terr04", "Terr05", "Terr06", "Terr07", "Terr08", "Terr09",
        "Terr10", "Terr11", "Terr12", "Terr13", "Terr14", "Terr15", "Terr16", "Terr17", "Terr18", "Terr19",
    ];

    /// <summary>섹션 이름을 저장 순서대로 돌려준다.</summary>
    public static IReadOnlyList<string> SectionNames => Names;

    /// <summary>파일 둘째 바이트(0 또는 0xfe)다.</summary>
    public byte Flag { get; set; }

    /// <summary>
    /// 읽은 원시 섹션이다(최대 35개, 이름 순서). 각 섹션은 길이 필드 2바이트를 제외한 본문이다.
    /// 원본 MemStream의 alloc에 대응하는 용량을 가지는 <see cref="RawSectionBuffer"/>다(0.130 TS0에서 <c>byte[]</c>에서 바꿈).
    /// </summary>
    public List<RawSectionBuffer> Sections { get; } = [];

    /// <summary>마지막으로 읽은 섹션 뒤에 남은 바이트 수다.</summary>
    public int TrailingBytes { get; set; }

    /// <summary>TypeNames의 해시다(파일의 타입 번호 순서).</summary>
    public List<uint> TypeHashes { get; } = [];

    /// <summary>TypeNames가 비어 있지 않은가.</summary>
    public bool HasTypeConversion { get; set; }

    /// <summary>파일 타입 번호 → 현재 타입 번호 변환표다(없으면 0). 255칸이다.</summary>
    public byte[] TypeConversion { get; } = new byte[255];

    /// <summary><c>Subscriber</c> 섹션의 소유자 ID와 요새 이름이다. 해당 섹션이 없으면 null이다. 추가: F1(2026-11-02).</summary>
    public FortSubscriber? Subscriber { get; set; }

    /// <summary><c>Money</c> 섹션의 플레이어별 시작 SP(Storm Power)다. 섹션이 4바이트 미만이면 null이다. 추가: F1(2026-11-02).</summary>
    public float? Money { get; set; }

    /// <summary><c>Territory</c> 섹션 120바이트(6바이트 × 20)의 원시 값이다. 섹션이 120바이트 미만이면 빈 배열이다. 해석은 영역 배치 단계에서 한다. 추가: F1(2026-11-02).</summary>
    public byte[] Territory { get; set; } = [];

    /// <summary><c>Technology</c> 섹션이다: 플레이어를 그릇으로 한 내용물 목록(타입별 저장 상태). 추가: F1(2026-11-02).</summary>
    public FortContentList Technology { get; set; } = new();

    /// <summary><c>Technology</c> 목록 뒤에 남은 바이트 수다(원본은 읽지 않는다). 추가: F1(2026-11-02).</summary>
    public int TechnologyTrailingBytes { get; set; }

    /// <summary><c>Deck</c> 섹션의 저장된 다리 조각 덱 항목 목록이다. 추가: F1(2026-11-02).</summary>
    public List<FortDeckEntry> Deck { get; } = [];

    /// <summary><c>Chaff</c> 섹션이다: 특정 영역에 속하지 않는 저장 청크 목록. 추가: F1(2026-11-02).</summary>
    public FortChunkSection Chaff { get; set; } = new();

    /// <summary>영역 번호별(<c>Terr00</c>~<c>Terr19</c>) 저장 청크와 객체 자료다. 추가: F1(2026-11-02).</summary>
    public FortChunkSection[] Territories { get; } = Enumerable.Range(0, TerritoryCount).Select(_ => new FortChunkSection()).ToArray();

    /// <summary>섹션 길이 필드의 크기다. 길이에는 이 2바이트가 포함된다.</summary>
    private const int SectionLengthBytes = 2;

    /// <summary>파일 첫 바이트('F')다. 원본은 다르면 파일을 버린다.</summary>
    private const byte FortMagic = 0x46;

    /// <summary>
    /// 파일 전체를 원시 섹션으로 나눈다(cpppj <c>FortTemplate::Parse</c>의 타입 표가 필요 없는 앞부분). 서명과 flag를 읽고,
    /// 길이(부호 있는 16비트, 길이 필드 포함)와 내용이 이어지는 섹션을 최대 35개까지 모으며 파일이 먼저 끝나면 거기서 멈춘다(새 구현의 보호 처리).
    /// Subscriber·Money·Territory·TypeNames 변환·Technology·Deck·객체 해석은 타입 표가 필요해 RiftTypeTable 이식 뒤에 같은 클래스에 더한다.
    /// </summary>
    /// <param name="bytes">.fort 파일 전체 바이트.</param>
    /// <exception cref="InvalidDataException">첫 바이트가 0x46이 아니다.</exception>
    public static FortTemplate ReadSections(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < 2 || bytes[0] != FortMagic)
        {
            throw new InvalidDataException("fort: first byte is not 0x46");
        }
        FortTemplate fort = new() { Flag = bytes[1] };
        int position = 2;
        // 길이(부호 있는 16비트, 길이 필드 포함)와 내용이 이어진다
        while (fort.Sections.Count < SectionCount && bytes.Length - position >= SectionLengthBytes)
        {
            int length = bytes[position] | (bytes[position + 1] << 8);
            if (length < SectionLengthBytes || length > 0x7fff || length > bytes.Length - position)
            {
                break;
            }
            fort.Sections.Add(RawSectionBuffer.FromBytes(bytes.Slice(position + SectionLengthBytes, length - SectionLengthBytes)));
            position += length;
        }
        fort.TrailingBytes = bytes.Length - position;
        return fort;
    }

    /// <summary>
    /// 이름으로 원시 섹션을 찾는다. 이름은 대소문자를 구분하지 않으며(원본 stricmp, ASCII만 접는다) 파일에 그 섹션이 없으면 빈 범위다.
    /// </summary>
    /// <param name="name">섹션 이름.</param>
    public ReadOnlySpan<byte> Section(string name)
    {
        // 이름표의 위치가 곧 파일 안의 섹션 순서다
        for (int index = 0; index < Names.Length && index < Sections.Count; index++)
        {
            if (AsciiEquals(Names[index], name))
            {
                return Sections[index].Span;
            }
        }
        return default;
    }

    /// <summary>
    /// 값 복사(C++ 복사 생성)와 같이 깊은 복사본을 만든다. 섹션 버퍼는 크기만큼만 복사해 용량이 크기와 같아지며 원본의 예약 용량은 따라가지 않는다.
    /// C#의 대입은 참조를 공유하므로 같은 Template을 두 번 설치하는 호출자만 이 복사를 거친다.
    /// 해석 필드(Subscriber·Money·Technology·Deck·객체 등)는 F1에서 더했고 읽은 뒤 바꾸지 않는 자료는 참조를 공유하며 배열·목록은 새로 만든다.
    /// </summary>
    public FortTemplate Clone()
    {
        FortTemplate copy = new()
        {
            Flag = Flag,
            TrailingBytes = TrailingBytes,
            HasTypeConversion = HasTypeConversion,
        };
        // 섹션마다 용량이 크기와 같은 새 버퍼로 복사한다
        foreach (RawSectionBuffer section in Sections)
        {
            copy.Sections.Add(section.Clone());
        }
        copy.TypeHashes.AddRange(TypeHashes);
        TypeConversion.CopyTo(copy.TypeConversion, 0);
        // 해석 필드: 읽은 뒤 바꾸지 않는 레코드는 공유하고, 바뀔 수 있는 배열·목록은 새로 만든다
        copy.Subscriber = Subscriber;
        copy.Money = Money;
        copy.Territory = (byte[])Territory.Clone();
        copy.Technology = Technology;
        copy.TechnologyTrailingBytes = TechnologyTrailingBytes;
        copy.Deck.AddRange(Deck);
        copy.Chaff = Chaff;
        Territories.CopyTo(copy.Territories, 0);
        return copy;
    }

    /// <summary>
    /// <c>TypeNames</c>로 파일의 타입 번호를 현재 번호로 바꾼다. 원본은 0xf6 미만의 번호만 변환표로 바꾸며 변환표가 없으면(섹션이 비었으면) 그대로 쓴다.
    /// </summary>
    /// <param name="storedType">파일에 적힌 타입 번호.</param>
    public int ConvertType(byte storedType)
    {
        if (storedType >= BridgeShorthandMin || !HasTypeConversion)
        {
            return storedType;
        }
        return TypeConversion[storedType];
    }

    /// <summary>ASCII 대문자만 소문자로 접어 비교한다. 비ASCII 문자는 그대로 비교한다.</summary>
    /// <param name="first">첫째 문자열.</param>
    /// <param name="second">둘째 문자열.</param>
    private static bool AsciiEquals(string first, string second)
    {
        if (first.Length != second.Length)
        {
            return false;
        }
        // 글자마다 ASCII 소문자로 접은 값을 비교한다
        for (int index = 0; index < first.Length; index++)
        {
            if (AsciiLower(first[index]) != AsciiLower(second[index]))
            {
                return false;
            }
        }
        return true;
    }

    /// <summary>ASCII 대문자 한 글자를 소문자로 바꾼다.</summary>
    /// <param name="value">바꿀 글자.</param>
    private static char AsciiLower(char value) => value is >= 'A' and <= 'Z' ? (char)(value + 32) : value;
}
