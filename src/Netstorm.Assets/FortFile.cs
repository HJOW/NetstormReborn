using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Assets;

/// <summary>container 내용물 항목 하나</summary>
/// <param name="Type">타입</param>
/// <param name="QA">추가 값 A (saveQA 타입만)</param>
/// <param name="QB">추가 값 B (saveQB 타입만)</param>
/// <param name="Contents">중첩 내용물</param>
public sealed record FortContent(TypeInfo Type, byte? QA, short? QB, IReadOnlyList<FortContent> Contents);

/// <summary>청크 안의 오브젝트 하나 (docs/formats/fort.md "오브젝트 레코드")</summary>
/// <param name="CellHigh">위치 바이트 상위 4비트 (청크 안 x)</param>
/// <param name="CellLow">위치 바이트 하위 4비트 (청크 안 y)</param>
/// <param name="Type">타입</param>
/// <param name="Frame">저장된 애니메이션 프레임 (saveFrame)</param>
/// <param name="QA">추가 값 A (saveQA)</param>
/// <param name="QB">추가 값 B (saveQB)</param>
/// <param name="BridgeShape">각 칸의 다리 클러스터 번호 (bridge, 기존 속성명 유지)</param>
/// <param name="FactoryState">작업장 상태 (factory)</param>
/// <param name="Owner">소유 플레이어 (저장된 경우)</param>
/// <param name="Contents">내용물 (container)</param>
public sealed record FortObject(
    int CellHigh, int CellLow, TypeInfo Type, byte? Frame, byte? QA, short? QB, byte? BridgeShape,
    byte? FactoryState, int? Owner, IReadOnlyList<FortContent> Contents);

/// <summary>청크 하나의 오브젝트 목록</summary>
/// <param name="Index">섹션 안 청크 순번 (Chaff 는 y*16 + x)</param>
/// <param name="Objects">오브젝트</param>
public sealed record FortChunk(int Index, IReadOnlyList<FortObject> Objects);

/// <summary>Deck 섹션의 항목 하나 (Deck.cpp 0044f160).</summary>
/// <param name="TypeNumber">파일에 저장된 타입 번호</param>
/// <param name="Type">TypeNames 변환표로 찾은 타입. 내장 타입이거나 확인할 수 없으면 null</param>
/// <param name="Chance">카드 선택 가중치</param>
/// <param name="Power">카드 위력 (부호 있는 8비트)</param>
/// <param name="NumRemaining">남은 사용 횟수</param>
public sealed record FortDeckEntry(byte TypeNumber, TypeInfo? Type, byte Chance, sbyte Power, byte NumRemaining);

/// <summary>Technology 섹션에 저장된 타입 상태 하나.</summary>
/// <param name="TypeNumber">파일에 저장된 타입 번호</param>
/// <param name="Type">TypeNames 변환표로 찾은 타입</param>
/// <param name="QA">saveQA 타입의 추가 값</param>
/// <param name="QB">saveQB 타입의 추가 값</param>
/// <param name="ListFlags">원본 오브젝트 목록 플래그 바이트</param>
/// <param name="Contents">container 타입의 중첩 내용물</param>
public sealed record FortTechnologyEntry(byte TypeNumber, TypeInfo Type, byte? QA, short? QB,
    byte ListFlags, IReadOnlyList<FortContent> Contents);

/// <summary>
/// .fort (요새 / 미션 맵) 파일. 포맷: docs/formats/fort.md
/// </summary>
public sealed class FortFile
{
    /// <summary>파일 첫 바이트 ('F')</summary>
    private const byte Magic = 0x46;

    /// <summary>둘째 바이트가 이 값이면 플래그 설정 (의미 미확정)</summary>
    private const byte FlagFE = 0xFE;

    /// <summary>청크 레코드 표지 ('c')</summary>
    private const byte ChunkMark = 99;

    /// <summary>타입 바이트가 이 값 이상이면 다리 조각 약식 표기 (소유자 = 0xFF - 값)</summary>
    private const byte BridgeShorthandMin = 0xF6;

    /// <summary>Deck 섹션의 개수 바이트를 제외한 항목 크기.</summary>
    private const int DeckEntryByteSize = 4;

    /// <summary>원본 월드 폭 (청크 수)</summary>
    public const int WorldChunksX = 16;

    /// <summary>영역 섹션 수 (Terr00 ~ Terr19)</summary>
    public const int TerritoryCount = 20;

    /// <summary>섹션 이름 (Netstorm.exe 0x5146B0 + "Terr%02d" × 20). 파일에는 이 순서로 저장된다</summary>
    public static IReadOnlyList<string> SectionNames { get; } =
    [
        "Subscriber", "State", "Mission", "CoreData", "Chaff", "Badges", "Territory", "TypeNames",
        "CompressedData", "Technology", "Money", "Deck", "Reserved2", "Reserved3", "Reserved4",
        .. Enumerable.Range(0, TerritoryCount).Select(i => $"Terr{i:00}"),
    ];

    private readonly Dictionary<string, byte[]> _sections = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>둘째 바이트가 0xFE 인지</summary>
    public bool HasFlagFE { get; }

    /// <summary>파일에 들어 있던 전체 섹션 수 (로더는 앞의 35개만 사용)</summary>
    public int RawSectionCount { get; }

    /// <summary>Subscriber: 소유자 ID</summary>
    public uint SubscriberId { get; }

    /// <summary>Subscriber: 요새 이름</summary>
    public string Name { get; } = "";

    /// <summary>Money: Storm Power (게임 내 재화, 섹션이 없으면 null)</summary>
    public float? Money { get; }

    /// <summary>Deck: 저장된 항목. 타입 번호는 TypeNames로 해석한다.</summary>
    public IReadOnlyList<FortDeckEntry> Deck { get; }

    /// <summary>Technology: 타입별 저장 상태. 원본 섹션의 항목 순서를 유지한다.</summary>
    public IReadOnlyList<FortTechnologyEntry> Technology { get; }

    /// <summary>Chaff: 월드 전체 청크</summary>
    public IReadOnlyList<FortChunk> Chaff { get; } = [];

    /// <summary>TerrNN: 영역별 청크 (없는 영역은 빈 목록)</summary>
    public IReadOnlyList<IReadOnlyList<FortChunk>> Territories { get; }

    /// <summary>이름으로 섹션 원본 바이트를 얻는다 (없으면 빈 배열)</summary>
    /// <param name="name">섹션 이름</param>
    public ReadOnlySpan<byte> Section(string name) => _sections.TryGetValue(name, out byte[]? s) ? s : [];

    /// <summary>파일 내용을 해석한다</summary>
    /// <param name="data">.fort 파일 내용 (아카이브 엔트리는 복호화된 것)</param>
    /// <param name="catalog">타입 목록 (플래그로 레코드 길이를 결정)</param>
    public FortFile(byte[] data, TypeCatalog catalog)
    {
        if (data.Length < 2 || data[0] != Magic)
        {
            throw new InvalidDataException("요새 파일이 아닙니다 (첫 바이트 0x46 아님).");
        }
        HasFlagFE = data[1] == FlagFE;

        int pos = 2;
        int count = 0;
        // 길이 접두 섹션을 파일 끝까지 자르고, 앞의 35개에 이름을 붙인다
        while (pos + 2 <= data.Length)
        {
            int length = BinaryPrimitives.ReadUInt16LittleEndian(data.AsSpan(pos));
            if (length < 2 || pos + length > data.Length)
            {
                break;
            }
            if (count < SectionNames.Count)
            {
                _sections[SectionNames[count]] = data.AsSpan(pos + 2, length - 2).ToArray();
            }
            count++;
            pos += length;
        }
        RawSectionCount = count;

        ReadOnlySpan<byte> sub = Section("Subscriber");
        if (sub.Length >= 6)
        {
            SubscriberId = BinaryPrimitives.ReadUInt32LittleEndian(sub);
            int nameLength = Math.Min(BinaryPrimitives.ReadUInt16LittleEndian(sub[4..]), sub.Length - 6);
            Name = Encoding.Latin1.GetString(sub.Slice(6, nameLength));
        }
        ReadOnlySpan<byte> money = Section("Money");
        if (money.Length == 4)
        {
            Money = BinaryPrimitives.ReadSingleLittleEndian(money);
        }

        Dictionary<int, TypeInfo> conversion = BuildConversion(Section("TypeNames"), catalog);
        Technology = ReadTechnology(Section("Technology"), conversion);
        Deck = ReadDeck(Section("Deck"), conversion);
        Chaff = ReadChunks(Section("Chaff"), conversion);
        var territories = new List<IReadOnlyList<FortChunk>>(TerritoryCount);
        // 영역 섹션 20개를 차례로 해석
        for (int i = 0; i < TerritoryCount; i++)
        {
            territories.Add(ReadChunks(Section($"Terr{i:00}"), conversion));
        }
        Territories = territories;
    }

    /// <summary>원본 004bcea0/004bd130의 타입별 상태와 container 내용을 읽는다.</summary>
    private static IReadOnlyList<FortTechnologyEntry> ReadTechnology(ReadOnlySpan<byte> section,
        Dictionary<int, TypeInfo> conversion)
    {
        if (section.IsEmpty)
        {
            return [];
        }
        var reader = new SpanReader(section);
        int count = reader.U8();
        var entries = new List<FortTechnologyEntry>(count);
        // 타입 플래그에 따라 길이가 달라지는 항목을 원본 순서대로 읽는다.
        for (int i = 0; i < count; i++)
        {
            byte typeNumber = reader.U8();
            if (!conversion.TryGetValue(typeNumber, out TypeInfo? type))
            {
                throw new InvalidDataException($"알 수 없는 기술 타입 번호 {typeNumber} (위치 {reader.Position - 1})");
            }
            byte? qa = (type.Flags1 & TypeFlagBits.SaveQA) != 0 ? reader.U8() : null;
            short? qb = (type.Flags1 & TypeFlagBits.SaveQB) != 0 ? (short)reader.U16() : null;
            byte listFlags = reader.U8();
            IReadOnlyList<FortContent> contents = (type.Flags1 & TypeFlagBits.Container) != 0
                ? ReadContents(ref reader, conversion)
                : [];
            entries.Add(new FortTechnologyEntry(typeNumber, type, qa, qb, listFlags, contents));
        }
        // 원본 두 파일에는 항목 뒤에 0바이트 하나가 더 있으므로 그 경우만 허용한다.
        if (reader.Remaining > 1 || (reader.Remaining == 1 && reader.U8() != 0))
        {
            throw new InvalidDataException($"Technology 섹션 뒤에 예상하지 못한 데이터가 있습니다 (위치 {reader.Position})");
        }
        return entries;
    }

    /// <summary>원본 004befd0/004bf190의 개수와 4바이트 카드 항목을 읽는다.</summary>
    private static IReadOnlyList<FortDeckEntry> ReadDeck(ReadOnlySpan<byte> section,
        Dictionary<int, TypeInfo> conversion)
    {
        if (section.IsEmpty)
        {
            return [];
        }
        int count = section[0];
        if (section.Length != 1 + count * DeckEntryByteSize)
        {
            throw new InvalidDataException($"Deck 섹션 길이 오류: 항목 {count}개, 데이터 {section.Length}바이트");
        }
        var entries = new List<FortDeckEntry>(count);
        // 원본 저장 순서대로 각 항목의 타입·가중치·위력·남은 횟수를 읽는다.
        for (int i = 0; i < count; i++)
        {
            int offset = 1 + i * DeckEntryByteSize;
            byte typeNumber = section[offset];
            entries.Add(new FortDeckEntry(typeNumber, conversion.GetValueOrDefault(typeNumber),
                section[offset + 1], unchecked((sbyte)section[offset + 2]), section[offset + 3]));
        }
        return entries;
    }

    /// <summary>
    /// TypeNames 섹션([개수 u8][해시 u32]×개수)으로 "파일 타입 번호 → 타입" 표를 만든다.
    /// 섹션이 비어 있으면 현재 번호 체계(70 + 로딩 순서)를 쓴다.
    /// </summary>
    /// <param name="section">TypeNames 섹션</param>
    /// <param name="catalog">타입 목록</param>
    private static Dictionary<int, TypeInfo> BuildConversion(ReadOnlySpan<byte> section, TypeCatalog catalog)
    {
        var map = new Dictionary<int, TypeInfo>();
        if (section.Length == 0)
        {
            // 현재 번호 체계를 그대로 등록
            foreach (TypeInfo t in catalog.Types)
            {
                map[t.RuntimeIndex] = t;
            }
            return map;
        }
        int count = section[0];
        // 해시마다 현재 타입을 찾는다 (내장 타입 등 못 찾은 번호는 비워 둔다)
        for (int i = 0; i < count && 1 + i * 4 + 4 <= section.Length; i++)
        {
            TypeInfo? t = catalog.FindByHash(BinaryPrimitives.ReadUInt32LittleEndian(section[(1 + i * 4)..]));
            if (t != null)
            {
                map[i] = t;
            }
        }
        return map;
    }

    /// <summary>Chaff / TerrNN 섹션: [버전 u8] + 청크 레코드 반복</summary>
    /// <param name="section">섹션 데이터</param>
    /// <param name="conversion">타입 번호 변환표</param>
    private static List<FortChunk> ReadChunks(ReadOnlySpan<byte> section, Dictionary<int, TypeInfo> conversion)
    {
        var chunks = new List<FortChunk>();
        if (section.Length <= 1)
        {
            return chunks;
        }
        var r = new SpanReader(section);
        int version = r.U8();
        // 섹션 끝까지 청크 레코드를 읽는다
        while (r.Remaining > 0)
        {
            int mark = r.U8();
            if (mark != ChunkMark)
            {
                throw new InvalidDataException($"청크 표지 오류 0x{mark:X2} (위치 {r.Position - 1})");
            }
            int objectCount = r.U16();
            var objects = new List<FortObject>(objectCount);
            // 청크 안의 오브젝트를 개수만큼 읽는다 (끝 표지는 null)
            for (int i = 0; i < objectCount; i++)
            {
                FortObject? obj = ReadObject(ref r, version, conversion);
                if (obj != null)
                {
                    objects.Add(obj);
                }
            }
            chunks.Add(new FortChunk(chunks.Count, objects));
        }
        return chunks;
    }

    /// <summary>오브젝트 레코드 하나 (Template.cpp FUN_004bdc60). 끝 표지 또는 island 는 null</summary>
    /// <param name="r">읽기 위치</param>
    /// <param name="version">섹션 버전</param>
    /// <param name="conversion">타입 번호 변환표</param>
    private static FortObject? ReadObject(ref SpanReader r, int version, Dictionary<int, TypeInfo> conversion)
    {
        int cell = r.U8();
        int t = r.U8();
        int? owner = null;
        TypeInfo? type;
        if (t >= BridgeShorthandMin)
        {
            owner = 0xFF - t;
            type = conversion.Values.FirstOrDefault(v => v.Name.Equals("bridge", StringComparison.OrdinalIgnoreCase));
        }
        else if (t == 0)
        {
            r.U8(); // 원본은 이 값이 0 인지 확인만 한다
            return null;
        }
        else if (!conversion.TryGetValue(t, out type))
        {
            throw new InvalidDataException($"알 수 없는 타입 번호 {t} (위치 {r.Position - 1})");
        }
        if (type == null)
        {
            throw new InvalidDataException("변환표에 bridge 타입이 없습니다.");
        }
        if (type.Name.Equals("island", StringComparison.OrdinalIgnoreCase))
        {
            r.U8(); // island 는 1바이트만 건너뛴다
            return null;
        }
        byte? frame = (type.Flags1 & TypeFlagBits.SaveFrame) != 0 ? r.U8() : null;
        byte? qa = (type.Flags1 & TypeFlagBits.SaveQA) != 0 ? r.U8() : null;
        short? qb = (type.Flags1 & TypeFlagBits.SaveQB) != 0 ? (short)r.U16() : null;
        byte? bridge = (type.Flags2 & TypeFlagBits.Bridge) != 0 ? r.U8() : null;
        byte? factory = version > 1 && (type.Flags2 & TypeFlagBits.Factory) != 0 ? r.U8() : null;
        if (version > 0 && (type.Flags2 & TypeFlagBits.OwnerSaved) != 0)
        {
            owner = r.U8();
        }
        IReadOnlyList<FortContent> contents = (type.Flags1 & TypeFlagBits.Container) != 0
            ? ReadContents(ref r, conversion)
            : [];
        return new FortObject(cell >> 4, cell & 0xF, type, frame, qa, qb, bridge, factory, owner, contents);
    }

    /// <summary>내용물 목록 (Template.cpp FUN_004bd130): [개수 u8] + 항목 반복</summary>
    /// <param name="r">읽기 위치</param>
    /// <param name="conversion">타입 번호 변환표</param>
    private static List<FortContent> ReadContents(ref SpanReader r, Dictionary<int, TypeInfo> conversion)
    {
        var items = new List<FortContent>();
        int count = r.U8();
        // 개수만큼 항목을 읽는다 (타입 0 이면 중단)
        for (int i = 0; i < count; i++)
        {
            int t = r.U8();
            if (t == 0)
            {
                break;
            }
            if (!conversion.TryGetValue(t, out TypeInfo? type))
            {
                throw new InvalidDataException($"알 수 없는 내용물 타입 번호 {t} (위치 {r.Position - 1})");
            }
            byte? qa = (type.Flags1 & TypeFlagBits.SaveQA) != 0 ? r.U8() : null;
            short? qb = (type.Flags1 & TypeFlagBits.SaveQB) != 0 ? (short)r.U16() : null;
            IReadOnlyList<FortContent> nested = (type.Flags1 & TypeFlagBits.Container) != 0
                ? ReadContents(ref r, conversion)
                : [];
            items.Add(new FortContent(type, qa, qb, nested));
        }
        return items;
    }

    /// <summary>바이트 스팬 순차 읽기 (범위를 넘으면 예외)</summary>
    private ref struct SpanReader
    {
        private readonly ReadOnlySpan<byte> _data;

        /// <summary>현재 위치</summary>
        public int Position { get; private set; }

        /// <summary>남은 바이트 수</summary>
        public readonly int Remaining => _data.Length - Position;

        /// <summary>스팬으로 만든다</summary>
        /// <param name="data">읽을 데이터</param>
        public SpanReader(ReadOnlySpan<byte> data)
        {
            _data = data;
            Position = 0;
        }

        /// <summary>1바이트 읽기</summary>
        public byte U8()
        {
            Require(1);
            return _data[Position++];
        }

        /// <summary>2바이트(u16) 읽기</summary>
        public ushort U16()
        {
            Require(2);
            ushort v = BinaryPrimitives.ReadUInt16LittleEndian(_data[Position..]);
            Position += 2;
            return v;
        }

        /// <summary>남은 길이 확인</summary>
        /// <param name="bytes">필요한 바이트 수</param>
        private readonly void Require(int bytes)
        {
            if (Position + bytes > _data.Length)
            {
                throw new InvalidDataException($"섹션 끝을 넘어 읽으려 함 (위치 {Position})");
            }
        }
    }
}
