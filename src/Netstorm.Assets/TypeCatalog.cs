namespace Netstorm.Assets;

/// <summary>
/// 타입 플래그 비트. 원본 타입 구조체의 플래그1(+0xE8)·플래그2(+0xEC) 값 (Rifttype.cpp).
/// 이름이 확인된 비트만 정의한다. (docs/formats/fort.md "타입 플래그")
/// </summary>
public static class TypeFlagBits
{
    /// <summary>플래그1: 애니메이션 프레임 저장</summary>
    public const uint SaveFrame = 0x20;

    /// <summary>플래그1: 추가 값 A(1바이트) 저장</summary>
    public const uint SaveQA = 0x1000;

    /// <summary>플래그1: 추가 값 B(2바이트) 저장</summary>
    public const uint SaveQB = 0x2000;

    /// <summary>플래그1: 내용물 목록 저장</summary>
    public const uint Container = 0x20000;

    /// <summary>플래그2: 다리 조각</summary>
    public const uint Bridge = 0x4;

    /// <summary>플래그2: 매장물 (파생 규칙으로 SaveQA 가 켜진다)</summary>
    public const uint Buried = 0x2000;

    /// <summary>플래그2: 작업장</summary>
    public const uint Factory = 0x4000;

    /// <summary>플래그2: 이 중 하나라도 있으면 파생 규칙으로 Container 가 켜진다 (vortex·factory·walker·balloon)</summary>
    public const uint ContainerSources = 0x34200;

    /// <summary>플래그2: 이 중 하나라도 있으면 .fort 에 소유자 바이트를 저장한다 (Netstorm.exe 0x542644 의 값)</summary>
    public const uint OwnerSaved = 0x5D77CF00;

    /// <summary>typeflags 단어 → 플래그1 비트</summary>
    public static IReadOnlyDictionary<string, uint> Flag1Words { get; } =
        new Dictionary<string, uint>(StringComparer.OrdinalIgnoreCase)
        {
            ["default_hotspot"] = 0x1, ["defaulthotspot"] = 0x1, ["mayDropOnIsle"] = 0x2, ["mayDropOnRim"] = 0x4,
            ["saveFrame"] = SaveFrame, ["createsisland"] = 0x400, ["surface"] = 0x800, ["saveQA"] = SaveQA,
            ["saveQB"] = SaveQB, ["carribleInVehicle"] = 0x10000, ["container"] = Container, ["shadow"] = 0x40000,
            ["randframe"] = 0x100000, ["matchframe"] = 0x200000, ["flyershadow"] = 0x400000,
            ["opaqueCollide"] = 0x800000, ["not_selectable"] = 0x8000000, ["notselectable"] = 0x8000000,
            ["dontSave"] = 0x20000000, ["not_real"] = 0x28000000, ["notreal"] = 0x28000000,
        };

    /// <summary>typeflags 단어 → 플래그2 비트</summary>
    public static IReadOnlyDictionary<string, uint> Flag2Words { get; } =
        new Dictionary<string, uint>(StringComparer.OrdinalIgnoreCase)
        {
            ["fencePost"] = 0x1, ["island"] = 0x2, ["bridge"] = Bridge, ["dropblocking"] = 0x10, ["blocking"] = 0x18,
            ["shotblocking"] = 0x20, ["bomb"] = 0x100, ["vortex"] = 0x200, ["guy"] = 0x400, ["fringe"] = 0x1000,
            ["buried"] = Buried, ["factory"] = Factory, ["nugget"] = 0x8000, ["walker"] = 0x10000,
            ["balloon"] = 0x20000, ["emplacement"] = 0x40000, ["edgefarm"] = 0x80000, ["flyer"] = 0x100000,
            ["priest"] = 0x200000, ["dais"] = 0x400000, ["islandThreeByThree"] = 0x1000000,
            ["not_real"] = 0x2000000, ["notreal"] = 0x2000000, ["geyser"] = 0x10000000, ["fence"] = 0x20000000,
            ["residence"] = 0x40000000, ["altar"] = 0x80000000,
        };
}

/// <summary>로딩 목록에 있는 타입 하나 (정의 + 계산된 플래그)</summary>
/// <param name="LoadIndex">로딩 순서 (= 셰이프 블록 번호)</param>
/// <param name="Name">로딩 목록의 타입 이름</param>
/// <param name="Definition">.type 해석 결과</param>
/// <param name="Flags1">플래그1 (파생 규칙 반영)</param>
/// <param name="Flags2">플래그2</param>
public sealed record TypeInfo(int LoadIndex, string Name, TypeDefinition Definition, uint Flags1, uint Flags2)
{
    /// <summary>현재 실행 파일 기준 타입 번호 (70 + 로딩 순서)</summary>
    public int RuntimeIndex => TypeCatalog.RuntimeIndexBase + LoadIndex;
}

/// <summary>
/// 로딩 목록의 모든 타입을 읽어 이름·플래그를 제공한다.
/// </summary>
public sealed class TypeCatalog
{
    /// <summary>현재 실행 파일에서 .type 타입 번호 = 이 값 + 로딩 순서 (타입 번호 전역 변수 0x541178~ 초기값)</summary>
    public const int RuntimeIndexBase = 70;

    /// <summary>원본이 타입 이름을 저장하는 최대 길이 (strncpy 0x14)</summary>
    private const int StoredNameLength = 20;

    private readonly Dictionary<string, TypeInfo> _byName = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<uint, TypeInfo> _byHash = [];

    /// <summary>로딩 순서대로 나열한 타입</summary>
    public IReadOnlyList<TypeInfo> Types { get; }

    /// <summary>아카이브에서 로딩 목록의 .type 을 모두 읽는다</summary>
    /// <param name="archive">netstorm.tarc</param>
    public TypeCatalog(TaffArchive archive) : this(path => archive.TryFind(path, out TaffEntry entry)
        ? archive.Read(entry) : throw new InvalidDataException($"아카이브에 타입 파일이 없습니다: {path}"))
    {
    }

    /// <summary>디스크 → 아카이브 → 보조 폴더 순서로 타입 정의를 읽는다.</summary>
    public TypeCatalog(GameFileSystem files) : this(files.ReadAllBytes)
    {
    }

    /// <summary>공통 파일 공급자로 로딩 목록을 해석하고 타입 번호·해시를 구성한다.</summary>
    private TypeCatalog(Func<string, byte[]> read)
    {
        var types = new List<TypeInfo>(TypeLoadOrder.Names.Count);
        // 로딩 목록 순서대로 .type 을 읽고 플래그를 계산한다
        for (int i = 0; i < TypeLoadOrder.Names.Count; i++)
        {
            string name = TypeLoadOrder.Names[i];
            TypeDefinition def = TypeDefinition.Parse(OriginalText.Decode(read($"d/{name}.type")));
            (uint f1, uint f2) = ComputeFlags(def.Flags);
            var info = new TypeInfo(i, name, def, f1, f2);
            types.Add(info);
            _byName[name] = info;
            _byHash[NameHash(name)] = info;
        }
        Types = types;
    }

    /// <summary>이름(대소문자 무시)으로 타입을 찾는다</summary>
    /// <param name="name">타입 이름</param>
    public TypeInfo? Find(string name) => _byName.GetValueOrDefault(name);

    /// <summary>.fort TypeNames 해시로 타입을 찾는다</summary>
    /// <param name="hash">이름 해시</param>
    public TypeInfo? FindByHash(uint hash) => _byHash.GetValueOrDefault(hash);

    /// <summary>현재 실행 파일 기준 타입 번호로 찾는다 (70 + 로딩 순서)</summary>
    /// <param name="runtimeIndex">타입 번호</param>
    public TypeInfo? FindByRuntimeIndex(int runtimeIndex)
    {
        int load = runtimeIndex - RuntimeIndexBase;
        return load >= 0 && load < Types.Count ? Types[load] : null;
    }

    /// <summary>
    /// 원본의 타입 이름 해시: 이름을 20바이트로 자르고 각 글자(부호 있는 8비트)를 (i % 4)*8 비트 밀어 더한다.
    /// </summary>
    /// <param name="name">타입 이름</param>
    public static uint NameHash(string name)
    {
        uint sum = 0;
        int length = Math.Min(name.Length, StoredNameLength);
        // 글자마다 자리를 바꿔 누적 (오버플로는 32비트에서 버림)
        for (int i = 0; i < length; i++)
        {
            int c = (sbyte)(byte)name[i];
            sum = unchecked(sum + (uint)(c << ((i & 3) * 8)));
        }
        return sum;
    }

    /// <summary>typeflags 단어 목록으로 플래그1·2 를 계산하고 파생 규칙을 적용한다</summary>
    /// <param name="words">typeflags 단어</param>
    public static (uint Flags1, uint Flags2) ComputeFlags(IEnumerable<string> words)
    {
        uint f1 = 0;
        uint f2 = 0;
        // 단어마다 대응 비트를 켠다 (모르는 단어는 무시)
        foreach (string word in words)
        {
            f1 |= TypeFlagBits.Flag1Words.GetValueOrDefault(word);
            f2 |= TypeFlagBits.Flag2Words.GetValueOrDefault(word);
        }
        // 파생 규칙 (Rifttype.cpp 후처리): 이동체·신전·작업장은 내용물을 가진다
        if ((f2 & TypeFlagBits.ContainerSources) != 0)
        {
            f1 |= TypeFlagBits.Container;
        }
        // 파생 규칙: 매장물은 추가 값 A 를 저장한다
        if ((f2 & TypeFlagBits.Buried) != 0)
        {
            f1 |= TypeFlagBits.SaveQA;
        }
        return (f1, f2);
    }
}
