using System.Globalization;
using System.Text.RegularExpressions;

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

    /// <summary>플래그1: 표면 오브젝트 (typeflags "surface"). 표면 번호 지도와 이웃 탐색(원본 004b1e80)의 대상이다.</summary>
    public const uint Surface = 0x800;

    /// <summary>플래그2: 섬 (typeflags "island")</summary>
    public const uint Island = 0x2;

    /// <summary>플래그2: 다리 조각</summary>
    public const uint Bridge = 0x4;

    /// <summary>플래그2: 폭탄 (typeflags "bomb")</summary>
    public const uint Bomb = 0x100;

    /// <summary>플래그2: 포대류 건물 (typeflags "emplacement"). 연결 판정(원본 00441e40)에서 프레임 글자를 'P' 로 만든다.</summary>
    public const uint Emplacement = 0x40000;

    /// <summary>
    /// 플래그2: 놓기 막음(typeflags "dropBlocking"). 원본은 오브젝트 발자국 칸의 스폿 지도(Spot.cpp, 256×256 바이트)에
    /// 플래그2 하위 비트를 OR 해 두며(Squid.cpp FUN_004b02d0), 다리 배치(Rifttype.cpp FUN_0049b510)는 이 비트가 있는 칸을
    /// 이어 붙일 섬 칸으로 보지 않는다. 섬 가장자리 초목 edgeFarm·건물·나무·신전·가이저가 가진다.
    /// </summary>
    public const uint DropBlocking = 0x10;

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
            ["predictable"] = 0,
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
            ["focus"] = 0,
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

    /// <summary>설명 필드로 넘친 긴 이름을 포함한 원본 런타임 이름. 자산 파일 이름은 Name으로 유지한다.</summary>
    public string RuntimeName => TypeCatalog.RuntimeName(Name, Definition);

    /// <summary>원본 zorder 깊이. 실제 그리기 비교에서는 부호 있는 16비트로 읽는다.</summary>
    public int ZOrder => TypeCatalog.ParseZOrder(Definition.GetString("zorder") ?? "0");

    /// <summary>원본 +0xc4의 단정밀도 비용. 플레이어 SP 차감과는 별개다.</summary>
    public float Cost => (float)(Definition.GetDouble("cost") ?? 0);

    /// <summary>원본은 level 속성을 0부터 시작하는 값으로 보관하며 누락 시 0이다.</summary>
    public int Level => Definition.GetDouble("level") is double level ? (int)level - 1 : 0;

    /// <summary>vortex/factory는 0x10, walker/balloon은 0x20 내용물 목록을 가진다.</summary>
    public byte ContainerListFlags => (Flags2 & TypeFlagBits.ContainerSources) == 0 ? (byte)0 : (Flags2 & 0x30000) == 0 ? (byte)0x10 : (byte)0x20;

    /// <summary>bomb 타입 항목이 그릇의 목록 플래그보다 먼저 적용하는 값.</summary>
    public byte ContentListFlags => (Flags2 & 0x100) == 0 ? (byte)0 : (byte)1;
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

    /// <summary>원본 설명 필드의 최대 바이트 수. 원본 타입 이름과 설명은 단일 바이트 문자열이다.</summary>
    private const int StoredDescriptionLength = 40;

    /// <summary>원본 00540d10의 깊이 이름과 값. 대소문자를 구분한 부분 문자열 첫 일치다.</summary>
    private static readonly (string Name, int Value)[] ZOrders =
    [
        ("zoNONE", -127), ("zoFALLING", 30), ("zoSTALAG", 20), ("zoCHALRING", 20), ("zoBATTLE", 10),
        ("zoEDGEFARM", 0), ("zoISLAND", 0), ("zoBRIDGE", 0), ("zoBRIDGE_CONNECTOR", -10), ("zoARTIFACTS", -15),
        ("zoEMPLACEMENTS", -20), ("zoILLEGAL_DITHER", -21), ("zoFLARES", -22), ("zoMISSILES", -25),
        ("zoFLYER_SHADOWS", -26), ("zoFLYERS", -30), ("zoFENCE", -35), ("zoMANAICON", -36),
        ("zoUBERGUMP", -40), ("zoMENUGUMP", -60), ("zoDIALOGGUMP", -80), ("zoLOOKGUMP", -100), ("zoRISING", -31),
    ];

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
            (uint f1, uint f2) = ComputeFlags(def);
            var info = new TypeInfo(i, name, def, f1, f2);
            types.Add(info);
            _byName[name] = info;
            _byHash.TryAdd(NameHash(info.RuntimeName), info);
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
    /// 원본의 타입 이름 해시: NUL 앞까지 각 글자(부호 있는 8비트)를 (i % 4)*8 비트 밀어 더한다.
    /// </summary>
    /// <param name="name">타입 이름</param>
    public static uint NameHash(string name)
    {
        uint sum = 0;
        int nul = name.IndexOf('\0');
        int length = nul < 0 ? name.Length : nul;
        // 글자마다 자리를 바꿔 누적 (오버플로는 32비트에서 버림)
        for (int i = 0; i < length; i++)
        {
            int c = (sbyte)(byte)name[i];
            sum = unchecked(sum + (uint)(c << ((i & 3) * 8)));
        }
        return sum;
    }

    /// <summary>원본 이름 필드가 20바이트 이상일 때 description이 뒤를 덮어쓰는 규칙.</summary>
    public static string RuntimeName(string name, TypeDefinition definition)
    {
        string? description = definition.GetString("description");
        return name.Length < StoredNameLength || description == null ? name
            : name[..StoredNameLength] + description[..Math.Min(description.Length, StoredDescriptionLength)];
    }

    /// <summary>숫자 또는 원본 깊이 이름과 선택적 +/- 오프셋을 해석한다(docs/exe/cpp-fort-reconstruction.md).</summary>
    public static int ParseZOrder(string text)
    {
        if (double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double number)) return (int)number;
        Match identifier = Regex.Match(text, @"([A-Za-z0-9_]+)[ \t]*");
        if (identifier.Success)
        {
            // 표에서 입력 식별자를 포함하는 첫 이름을 찾는다. zoBRIDGE가 CONNECTOR보다 앞선다.
            foreach ((string name, int value) in ZOrders)
            {
                if (!name.Contains(identifier.Groups[1].Value, StringComparison.Ordinal)) continue;
                Match offset = Regex.Match(text, @"[ \t]*([A-Za-z0-9_]+)[ \t]*(\+|-)[ \t]*([0-9]+)");
                if (!offset.Success) return value;
                int delta = int.Parse(offset.Groups[3].Value, CultureInfo.InvariantCulture);
                return unchecked(offset.Groups[2].Value == "+" ? value + delta : value - delta);
            }
        }
        throw new InvalidDataException($"알 수 없는 원본 깊이 이름: {text}");
    }

    /// <summary>typeflags 단어 목록으로 플래그1·2 를 계산하고 파생 규칙을 적용한다</summary>
    /// <param name="words">typeflags 단어</param>
    public static (uint Flags1, uint Flags2) ComputeFlags(IEnumerable<string> words) => ComputeFlags(words, null);

    /// <summary>0049c3b0의 속성 적용과 0049b0d0의 파생 플래그를 함께 계산한다.</summary>
    public static (uint Flags1, uint Flags2) ComputeFlags(TypeDefinition definition) => ComputeFlags(definition.Flags, definition);

    /// <summary>그룹·사용량·발자국을 포함한 원본 플래그 후처리(docs/exe/cpp-fort-reconstruction.md).</summary>
    private static (uint Flags1, uint Flags2) ComputeFlags(IEnumerable<string> words, TypeDefinition? definition)
    {
        uint f1 = 0;
        uint f2 = 0;
        // 단어마다 대응 비트를 켠다 (모르는 단어는 무시)
        foreach (string word in words)
        {
            if (word.Equals("dontdrawdefault", StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("원본이 거부하는 dontdrawdefault 플래그입니다.");
            f1 |= TypeFlagBits.Flag1Words.GetValueOrDefault(word);
            f2 |= TypeFlagBits.Flag2Words.GetValueOrDefault(word);
        }
        if ((f1 & 0x440000) == 0x440000) throw new InvalidDataException("shadow와 flyershadow를 함께 사용할 수 없습니다.");
        string? group = definition?.GetString("group");
        if (string.Equals(group, "battery", StringComparison.OrdinalIgnoreCase)) f2 |= 0x800;
        if (string.Equals(group, "archer", StringComparison.OrdinalIgnoreCase) || string.Equals(group, "cannon", StringComparison.OrdinalIgnoreCase)) f2 |= 0x8000000;
        if (string.Equals(group, "blocker", StringComparison.OrdinalIgnoreCase)) f2 |= 0x4000000;
        if (definition?.GetDouble("maxHitPoints") != null) f1 |= 0x10;
        var usage = new float[4];
        if (definition != null)
        {
            // 사용량 별칭은 파일에 나온 순서대로 적용한다. 중복 속성도 이 목록에 남아 있다.
            foreach ((string key, string value) in definition.PropertySequence)
            {
                int slot = key.ToLowerInvariant() switch
                {
                    "minusage" => 0, "maxusage" or "maxrainbattleusage" => 1,
                    "maxthunderbattleusage" => 2, "maxwindbattleusage" => 3, _ => -1,
                };
                if (slot >= 0 && float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out float power)) usage[slot] = power;
            }
        }
        if (usage.Any(power => power != 0)) f1 |= 0x4000;
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
        // vortex와 battery 계열의 수명/표시, factory, vortex 전용 후처리 비트.
        if ((f2 & 0xA00) != 0) f1 |= 0x10000000;
        if ((f2 & TypeFlagBits.Factory) != 0) f1 |= 0x4000000;
        if ((f2 & 0x200) != 0) f1 |= 0x84000;
        if ((f1 & (TypeFlagBits.SaveQA | TypeFlagBits.SaveQB)) == (TypeFlagBits.SaveQA | TypeFlagBits.SaveQB))
            throw new InvalidDataException("saveQA와 saveQB를 함께 사용할 수 없습니다.");
        // 테두리 전용이 아닌 타입은 섬 위 배치를 허용한다. emplacement는 다리의 부착을 막는다.
        if ((f1 & 4) == 0) f1 |= 2;
        if ((f2 & 0x40000) != 0) f2 |= TypeFlagBits.DropBlocking;
        int footX = (int)(definition?.GetDouble("foot_x") ?? definition?.GetDouble("footx") ?? 1);
        int footY = (int)(definition?.GetDouble("foot_y") ?? definition?.GetDouble("footy") ?? 1);
        if ((f2 & 0x40000) != 0 && (f2 & 0x4200) == 0 && footX == 3 && footY == 3) f1 |= 6;
        if ((f2 & 0x408100) != 0) f1 |= 2;
        if ((f2 & 0x70000) != 0 && (f2 & TypeFlagBits.Factory) == 0) f1 |= 0x8000;
        return (f1, f2);
    }
}
