using System.Text.RegularExpressions;

namespace Netstorm.Core.Original;

/// <summary>타입 플래그 1(원본 타입 구조체 +0xe8, "typeFlags")의 비트 가운데 저장 형식과 분류에 쓰이는 것이다. 이름은 .type의 플래그 단어와 원본 assert 문자열을 따른다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다(cpppj <c>TypeFlag1</c>).</remarks>
public static class TypeFlag1
{
    /// <summary><c>default_hotspot</c>.</summary>
    public const uint DefaultHotspot = 0x1;

    /// <summary><c>mayDropOnIsle</c>. 후처리가 rim이 아니면 켠다.</summary>
    public const uint MayDropOnIsle = 0x2;

    /// <summary><c>mayDropOnRim</c>.</summary>
    public const uint MayDropOnRim = 0x4;

    /// <summary><c>maxHitPoints</c> 속성이 있으면 켜진다.</summary>
    public const uint HasHitPoints = 0x10;

    /// <summary><c>saveFrame</c>: .fort에 프레임 1바이트.</summary>
    public const uint SaveFrame = 0x20;

    /// <summary><c>surface</c>: 표면 오브젝트 지도/이웃 탐색 대상.</summary>
    public const uint Surface = 0x800;

    /// <summary><c>saveQA</c>: .fort에 수량 1바이트.</summary>
    public const uint SaveQA = 0x1000;

    /// <summary><c>saveQB</c>: .fort에 수량 2바이트(TYPE_FLAG_SAVE_Q16).</summary>
    public const uint SaveQB = 0x2000;

    /// <summary>minUsage·maxUsage가 0이 아니거나 vortex.</summary>
    public const uint UsesPower = 0x4000;

    /// <summary><c>container</c>: .fort에 내용물 목록.</summary>
    public const uint Container = 0x20000;
}

/// <summary>타입 플래그 2(원본 타입 구조체 +0xec)의 비트 가운데 저장 형식과 분류에 쓰이는 것이다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다(cpppj <c>TypeFlag2</c>).</remarks>
public static class TypeFlag2
{
    /// <summary>섬.</summary>
    public const uint Island = 0x2;

    /// <summary>.fort에 다리 클러스터 번호 1바이트.</summary>
    public const uint Bridge = 0x4;

    /// <summary>낙하 차단.</summary>
    public const uint DropBlocking = 0x10;

    /// <summary>폭탄.</summary>
    public const uint Bomb = 0x100;

    /// <summary>소용돌이(신전).</summary>
    public const uint Vortex = 0x200;

    /// <summary>group = battery(5).</summary>
    public const uint GroupBattery = 0x800;

    /// <summary>묻힌 자원.</summary>
    public const uint Buried = 0x2000;

    /// <summary>.fort 버전 2부터 작업장 상태 1바이트.</summary>
    public const uint Factory = 0x4000;

    /// <summary>걷는 유닛.</summary>
    public const uint Walker = 0x10000;

    /// <summary>풍선.</summary>
    public const uint Balloon = 0x20000;

    /// <summary>포대 자리.</summary>
    public const uint Emplacement = 0x40000;

    /// <summary>group = blocker(2).</summary>
    public const uint GroupBlocker = 0x4000000;

    /// <summary>group = archer(0)·cannon(1).</summary>
    public const uint GroupArcherOrCannon = 0x8000000;

    /// <summary>소유 플레이어 1바이트를 저장하는 타입(패치 DAT_00542644).</summary>
    public const uint OwnerSavedMask = 0x5d77cf00;

    /// <summary>읽은 뒤 소유자를 0으로 만드는 타입: geyser·buried(패치 DAT_00542648).</summary>
    public const uint OwnerClearedMask = 0x10002000;
}

/// <summary>.type 한 개를 타입 표에 넣을 때 쓰는 입력이다.</summary>
/// <param name="AssetName">로딩 순서의 파일 이름. 타입 구조체의 이름이 된다.</param>
/// <param name="Definition">읽은 정의.</param>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed record RiftTypeSource(string AssetName, RiftTypeDefinition? Definition);

/// <summary>
/// 원본 전역 타입 배열(패치 DAT_0059ab20, 개수 DAT_00541348 = 188 / CD 171)을 객체로 둔 타입 표다(cpppj <c>o/RiftTypeTable.cpp</c>).
/// 번호 체계는 0 없음, 1~9 내장, 10~ 내장 프로세스 타입, 70~ .type 타입이고 패치판 186·187은 이름 없는 빈 타입이다.
/// 0049ebb0 ↔ CD 004434d0의 초기화, 0049c3b0 ↔ CD 004460f0의 플래그·속성 읽기, 0049b0d0 ↔ CD 00444e10의 후처리를 차례로 적용하고 0049a860 ↔ CD 00443410이 이름 검색이다.
/// </summary>
/// <remarks>
/// 2026-11-02 F1에서 추가했다. 범위는 타입 번호 체계·이름·플래그 1/2·종류 단어·그룹·원소·목록 플래그·발자국·초기 HP/깊이·생성자 주소 기록이다.
/// 비용 +0xc4도 읽는다. 사거리 등 전투 수치·요구 에너지 문자열(+0xa0)·최대 사거리 집계는 cpppj도 후속으로 남겨 둔 범위다.
/// 입력 문자열은 문자 하나 = 바이트 하나로 다룬다(이름 20글자·설명 40글자 한도와 이름 해시가 바이트 기준이다). 실제 자산의 이름·설명은 모두 ASCII다.
/// </remarks>
public sealed class RiftTypeTable
{
    /// <summary>`group` 속성의 기본값(원본 NO_GROUP)이다. 이름 표(패치 00542468): archer cannon blocker aviary flyer battery fence walker balloon misc.</summary>
    public const int NoGroup = 10;

    /// <summary>.type 타입의 번호는 이 값 + 로딩 순서다(원본 0049c3b0의 <c>(읽은 파일 수 + 0x45) * 500</c>). 두 판본이 같다.</summary>
    public const int FirstAssetTypeNumber = 70;

    /// <summary>패치판 타입 수(DAT_00541348의 초기값 188)다.</summary>
    private const int PatchTypeCount = 188;

    /// <summary>CD판 타입 수(DAT_0051cbf0 = 171)다.</summary>
    private const int CdTypeCount = 171;

    /// <summary>내장 프로세스 타입의 첫 번호(원본 루프의 시작값 10)다.</summary>
    private const int FirstProcessTypeNumber = 10;

    /// <summary>타입 구조체의 이름 필드 길이(+4~+0x17)다. 프로세스 타입 이름은 strncpy로 이 길이까지만 들어간다.</summary>
    private const int TypeNameBytes = 20;

    /// <summary>설명 필드(+0x18~+0x3f)에 strncpy로 복사하는 최대 글자 수다.</summary>
    private const int DescriptionBytes = 40;

    /// <summary>종류 단어 필드(+0xf0)에 strncpy로 복사하는 최대 글자 수다.</summary>
    private const int KindBytes = 8;

    /// <summary>두 판본의 원래 사제 TYPE 전역은 158이며 후처리 마지막에 비용을 0으로 되돌린다.</summary>
    private const int CostExemptPriestType = 158;

    /// <summary>CD판 프로세스 타입 이름 수다.</summary>
    private const int CdProcessTypeCount = 52;

    /// <summary>내장 프로세스 타입 이름(패치 00540fa0의 53개)이다. CD판(0051c890)은 마지막 하나가 없는 앞 52개다.</summary>
    private static readonly string[] ProcessNames =
    [
        "evilRaySystemProcessType", "fieryTailSystemProcessType", "streakSystemProcessType", "ropeSystemProcessType",
        "burningSystemProcessType", "kaboomSystemProcessType", "flyingShrapnelSystemProcessType", "buildSystemProcessType",
        "dissolveSystemProcessType", "lightningSystemProcessType", "beamOverSystemProcessType", "smokeSystemProcessType",
        "fountainSystemProcessType", "teleportEffectProcessType", "dudeProcessType", "canonRealizeProcessType",
        "rookAttackProcessType", "targetProcessType", "missileProcessType", "pingPongProcessType",
        "constructionProcessType", "baseFlyerProcessType", "sunAviaryProcessType", "islandDropperProcessType",
        "gunProcessType", "fireAnimProcessType", "rotateAnimProcessType", "scrollTestProcessType",
        "simpleWalkerProcessType", "simpleFlyProcessType", "wormProcessType", "bulfProcessType",
        "balloonProcessType", "ambleProcessType", "driftProcessType", "rainVortexAnimProcessType",
        "regularProcessType", "soundProcessType", "windBatteryProcessType", "buildPlaybackProcessType",
        "manaDisplayProcessType", "rangeDisplayProcessType", "islandAnimatorProcessType", "cameraAnimatorProcessType",
        "priestProcessType", "paletteFlasherProcessType", "thunderArcherSystemProcessType", "bombEffectTimerProcessType",
        "daisProcessType", "windAviaryProcessType", "rainAviaryProcessType", "sharedRegularProcessType",
        "thunderAviaryProcessType",
    ];

    /// <summary>후처리(0049b0d0)가 이름을 채우는 내장 타입의 번호와 이름이다(패치 DAT_00541074~88, CD DAT_0051c964~78). 두 판본이 같다.</summary>
    private static readonly (int Number, string Name)[] BuiltinNames =
    [
        (1, "DependForm"), (2, "ProcessForm"), (3, "GumpForm"), (4, "PlayerSquid"), (6, "ContentForm"),
    ];

    /// <summary>`group` 속성의 이름 표(패치 00542468)다. 번호 = 배열 위치. 없는 이름은 원본처럼 -1이다.</summary>
    private static readonly string[] GroupNames = ["archer", "cannon", "blocker", "aviary", "flyer", "battery", "fence", "walker", "balloon", "misc"];

    /// <summary>플래그 단어 하나가 켜는 비트다. <paramref name="Kind"/>가 참이면 단어를 종류 필드(+0xf0)에 복사한다.</summary>
    /// <param name="Word">플래그 단어.</param>
    /// <param name="Flags1">켜는 타입 플래그 1 비트.</param>
    /// <param name="Flags2">켜는 타입 플래그 2 비트.</param>
    /// <param name="Kind">종류 필드에 복사하는 단어인가.</param>
    private readonly record struct FlagWord(string Word, uint Flags1, uint Flags2, bool Kind);

    /// <summary>원본 0049c3b0의 단어 비교 순서 그대로다. 앞 20개는 서로 독립이고 뒤는 if-else 사슬이지만 단어가 겹치지 않는다.</summary>
    private static readonly FlagWord[] FlagWords =
    [
        new("carribleInVehicle", 0x10000, 0, false), new("container", 0x20000, 0, false),
        new("default_hotspot", 0x1, 0, false), new("defaulthotspot", 0x1, 0, false),
        new("shadow", 0x40000, 0, false), new("flyershadow", 0x400000, 0, false),
        new("not_selectable", 0x8000000, 0, false), new("notselectable", 0x8000000, 0, false),
        new("not_real", 0x28000000, 0x2000000, false), new("notreal", 0x28000000, 0x2000000, false),
        new("createsisland", 0x400, 0, false), new("mayDropOnRim", 0x4, 0, false), new("mayDropOnIsle", 0x2, 0, false),
        new("surface", 0x800, 0, false), new("randframe", 0x100000, 0, false), new("matchframe", 0x200000, 0, false),
        new("opaqueCollide", 0x800000, 0, false), new("dontSave", 0x20000000, 0, false),
        new("saveFrame", 0x20, 0, false), new("saveQA", 0x1000, 0, false), new("saveQB", 0x2000, 0, false),
        new("dropblocking", 0, 0x10, false), new("blocking", 0, 0x18, false), new("shotblocking", 0, 0x20, false),
        new("fencePost", 0, 0x1, false), new("fence", 0, 0x20000000, false),
        new("buried", 0, 0x2000, true), new("nugget", 0, 0x8000, true), new("bomb", 0, 0x100, true),
        new("emplacement", 0, 0x40000, false), new("bridge", 0, 0x4, true), new("walker", 0, 0x10000, true),
        new("balloon", 0, 0x20000, true), new("island", 0, 0x2, true), new("islandThreeByThree", 0, 0x1000000, false),
        new("factory", 0, 0x4000, true), new("vortex", 0, 0x200, true), new("edgefarm", 0, 0x80000, true),
        new("focus", 0, 0, true), new("guy", 0, 0x400, true), new("geyser", 0, 0x10000000, true),
        new("flyer", 0, 0x100000, false), new("residence", 0, 0x40000000, true), new("fringe", 0, 0x1000, false),
        new("priest", 0, 0x200000, false), new("dais", 0, 0x400000, false), new("altar", 0, 0x80000000, false),
        // `predictable`은 +0x1e8에 99를 OR 한다. 그 필드는 아직 옮기지 않아 비트 없이 둔다(cpppj와 같다)
        new("predictable", 0, 0, false),
    ];

    /// <summary>깊이 이름과 값이다(패치 00540d10 / CD 0051c630의 23개, 두 PE에서 같다).</summary>
    private static readonly (string Name, int Value)[] ZOrders =
    [
        ("zoNONE", -127), ("zoFALLING", 30), ("zoSTALAG", 20), ("zoCHALRING", 20), ("zoBATTLE", 10),
        ("zoEDGEFARM", 0), ("zoISLAND", 0), ("zoBRIDGE", 0), ("zoBRIDGE_CONNECTOR", -10), ("zoARTIFACTS", -15),
        ("zoEMPLACEMENTS", -20), ("zoILLEGAL_DITHER", -21), ("zoFLARES", -22), ("zoMISSILES", -25),
        ("zoFLYER_SHADOWS", -26), ("zoFLYERS", -30), ("zoFENCE", -35), ("zoMANAICON", -36),
        ("zoUBERGUMP", -40), ("zoMENUGUMP", -60), ("zoDIALOGGUMP", -80), ("zoLOOKGUMP", -100), ("zoRISING", -31),
    ];

    /// <summary>원본 0049c3b0의 깊이 문자열 첫 식별자 정규식이다. 깊이 이름은 stricmp와 달리 대소문자를 구분한다.</summary>
    private static readonly Regex ZOrderIdentifier = new(@"([A-Za-z0-9_]+)[ \t]*", RegexOptions.CultureInvariant);

    /// <summary>원본 0049c3b0의 깊이 문자열 선택적 +/- 숫자 정규식이다.</summary>
    private static readonly Regex ZOrderExpression = new(@"[ \t]*([A-Za-z0-9_]+)[ \t]*(\+|-)[ \t]*([0-9]+)", RegexOptions.CultureInvariant);

    /// <summary>번호순 타입 목록이다. 판본의 타입 수만큼 0으로 초기화한 뒤 이름·정의·후처리를 적용한 결과다.</summary>
    private readonly OriginalTypeRecord[] _types;

    /// <summary>다리 약식 표기에 쓰는 타입 번호다(패치 DAT_005411a0 = 82).</summary>
    private readonly int _bridge = -1;

    /// <summary>섬 건너뛰기에 쓰는 타입 번호다(패치 DAT_005411d0 = 94).</summary>
    private readonly int _island = -1;

    /// <summary>
    /// 판본의 타입 수만큼 0으로 초기화한 뒤 이름·정의·후처리를 적용한다. <paramref name="sources"/>는 그 판본의 로딩 순서(<see cref="RiftTypeLoadOrder"/>)와
    /// 개수·이름이 같아야 한다.
    /// </summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="sources">로딩 순서의 파일 이름과 읽은 정의.</param>
    /// <exception cref="ArgumentException">입력이 판본의 로딩 순서와 다르다.</exception>
    public RiftTypeTable(OriginalEdition edition, IReadOnlyList<RiftTypeSource> sources)
    {
        ArgumentNullException.ThrowIfNull(sources);
        bool cd = edition == OriginalEdition.Cd1072;
        Edition = edition;
        _types = new OriginalTypeRecord[cd ? CdTypeCount : PatchTypeCount];
        // 원본 전체 TYPE 초기화는 메모리를 0으로 지운 뒤 +98에 패치 5 / CD 4를 넣는다
        for (int number = 0; number < _types.Length; number++)
        {
            _types[number] = new OriginalTypeRecord { FootX = 0, FootY = 0, Theme = cd ? 4 : 5 };
        }
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
        if (sources.Count != order.Count)
        {
            throw new ArgumentException("Type source count does not match the edition load order", nameof(sources));
        }
        // .type 타입: 번호 = 70 + 로딩 순서. 이름은 정의의 typename이 아니라 파일 이름이다(원본 0049c3b0)
        for (int index = 0; index < sources.Count; index++)
        {
            if (sources[index].Definition is null || !RiftTypeDefinition.AsciiEqual(sources[index].AssetName, order[index]))
            {
                throw new ArgumentException("Type sources must follow the edition load order", nameof(sources));
            }
            OriginalTypeRecord type = _types[FirstAssetTypeNumber + index];
            type.Name = order[index]; // 원본은 실행 파일의 이름 목록에서 복사한다
            type.AssetName = type.Name;
            type.FromAsset = true;
            ApplyDefinition(type, sources[index].Definition!, edition);
        }
        // 내장 프로세스 타입 이름: 20바이트 필드에 strncpy 하므로 긴 이름은 잘린다
        IReadOnlyList<string> processNames = ProcessTypeNames(edition);
        // 번호 10부터 차례로 이름을 넣는다
        for (int index = 0; index < processNames.Count; index++)
        {
            string name = processNames[index];
            _types[FirstProcessTypeNumber + index].Name = name.Length > TypeNameBytes ? name[..TypeNameBytes] : name;
        }
        // 후처리가 이름을 채우는 내장 타입
        foreach ((int number, string name) in BuiltinNames)
        {
            _types[number].Name = name;
        }
        // 후처리는 번호순으로 모든 타입에 적용된다
        for (int number = 0; number < _types.Length; number++)
        {
            PostProcess(_types[number], number);
        }
        // 원본은 로딩/후처리 뒤 생성자 표를 대입한다. 주소는 미복원 파생 dispatch를 감지하는 메타데이터다
        for (int number = 0; number < _types.Length; number++)
        {
            _types[number].ConstructorAddress = OriginalConstructors.Address(edition, number);
        }
        // 원본은 타입 번호를 전역 변수로 갖는다(패치 DAT_005411a0 = 82, DAT_005411d0 = 94). 로딩 순서에서 찾는다
        for (int index = 0; index < order.Count; index++)
        {
            if (order[index] == "bridge")
            {
                _bridge = FirstAssetTypeNumber + index;
            }
            if (order[index] == "island")
            {
                _island = FirstAssetTypeNumber + index;
            }
        }
    }

    /// <summary>이 표가 만들어진 원본 판본이다.</summary>
    public OriginalEdition Edition { get; }

    /// <summary>번호순 타입 목록이다. 패치판 188개, CD판 171개.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types => _types;

    /// <summary>다리 타입 번호다(패치 82). 다리 약식 표기에 쓴다.</summary>
    public int BridgeType() => _bridge;

    /// <summary>섬 타입 번호다(패치 94). 섬 건너뛰기에 쓴다.</summary>
    public int IslandType() => _island;

    /// <summary>
    /// 이름으로 타입 번호를 찾는다(원본 FUN_0049a860 ↔ CD 00443410). 원본은 0번부터 순서대로 ASCII 대소문자를 무시하고 비교해 첫 일치를 돌려준다.
    /// 이름 없는 타입은 빈 이름을 찾을 때만 맞는다.
    /// </summary>
    /// <param name="name">찾을 이름.</param>
    /// <returns>타입 번호, 없으면 -1.</returns>
    public int Find(string name)
    {
        // 번호순으로 첫 일치를 찾는다
        for (int number = 0; number < _types.Length; number++)
        {
            if (RiftTypeDefinition.AsciiEqual(_types[number].Name, name))
            {
                return number;
            }
        }
        return -1;
    }

    /// <summary>
    /// .fort <c>TypeNames</c> 섹션의 이름 해시다(원본 004bd6f0·004bd800). 글자를 부호 있는 8비트로 읽어 (위치 % 4) × 8비트 왼쪽으로 밀어 더하며 덧셈은 32비트로 감긴다.
    /// 타입 표의 이름(C 문자열로 읽은 값)을 넣는다. 문자 하나는 바이트 하나로 본다(하위 8비트).
    /// </summary>
    /// <param name="name">타입 이름.</param>
    public static uint NameHash(string name)
    {
        uint hash = 0;
        // 원본은 이름 필드를 NUL까지 읽는다. 20글자에서 자르지 않는다
        for (int index = 0; index < name.Length; index++)
        {
            uint extended = unchecked((uint)(int)(sbyte)(byte)name[index]);
            hash = unchecked(hash + (extended << ((index & 3) * 8)));
        }
        return hash;
    }

    /// <summary>판본의 내장 프로세스 타입 이름이다(번호 10부터). 패치 53개(00540fa0), CD 52개(0051c890). CD판 목록은 패치판 목록의 앞부분이다.</summary>
    /// <param name="edition">원본 판본.</param>
    public static IReadOnlyList<string> ProcessTypeNames(OriginalEdition edition) =>
        edition == OriginalEdition.Cd1072 ? new ArraySegment<string>(ProcessNames, 0, CdProcessTypeCount) : ProcessNames;

    /// <summary>
    /// hotFootRatioX/Y의 float32 입력에 원본 고정 칸 배율(가로 16, 세로 11)을 곱해 0쪽으로 버린다(00503358/0050334c ↔ CD 005019b8/005019bc).
    /// AST의 float32 값은 x87에 올린 뒤 중간 float 저장 없이 배율을 곱한다.
    /// </summary>
    /// <param name="ratio">속성의 float32 값.</param>
    /// <param name="vertical">세로 기준점이면 true.</param>
    /// <exception cref="ArgumentOutOfRangeException">결과가 32비트 정수 범위를 넘거나 유한하지 않다.</exception>
    public static int TypeHotFootOffset(float ratio, bool vertical)
    {
        double value = Math.Truncate((double)ratio * (vertical ? 11.0 : 16.0));
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(ratio), ratio, "타입 기준점의 32비트 정수 범위 초과");
        }
        return (int)value;
    }

    /// <summary>원본 FUN_004e49c0(float → int, 0쪽으로 버림)에 해당하는 변환이다. 범위를 벗어나면 0으로 둔다.</summary>
    /// <param name="value">변환할 값.</param>
    private static int ToInt(double value)
    {
        if (!(value > -2147483649.0 && value < 2147483648.0))
        {
            return 0;
        }
        return (int)value;
    }

    /// <summary>techBit 숫자 AST는 float32를 실제 CRT _ftol의 signed64로 절삭한 뒤 낮은 DWORD만 저장한다.</summary>
    /// <param name="value">속성 값(파서의 double).</param>
    private static int TechBitNumber(double value)
    {
        double number = (float)value; // 기존 parser의 double 보관과 원문 float32 AST 경계를 구별한다
        if (!double.IsFinite(number) || number < -9223372036854775808.0 || number >= 9223372036854775808.0)
        {
            return 0;
        }
        return unchecked((int)(uint)(long)number);
    }

    /// <summary>원본 문자열 zorder의 첫 식별자 검색과 이름 표의 부분 문자열 비교, 선택적 +/- 숫자를 처리한다.</summary>
    /// <param name="text">깊이 문자열.</param>
    /// <exception cref="ArgumentException">식별자가 없거나 표에 없는 이름이다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">오프셋 범위 오류.</exception>
    private static int ZOrder(string text)
    {
        Match word = ZOrderIdentifier.Match(text);
        if (!word.Success)
        {
            throw new ArgumentException("zorder 이름이 없습니다");
        }
        // 원본은 표 순서대로 strstr(표 이름, 입력 식별자)의 첫 일치를 선택한다
        foreach ((string name, int baseValue) in ZOrders)
        {
            if (!name.Contains(word.Groups[1].Value, StringComparison.Ordinal))
            {
                continue;
            }
            uint value = unchecked((uint)baseValue);
            Match offset = ZOrderExpression.Match(text);
            if (offset.Success)
            {
                if (!ulong.TryParse(offset.Groups[3].Value, out ulong delta) || delta > 2147483647)
                {
                    throw new ArgumentOutOfRangeException(nameof(text), text, "zorder 오프셋 범위 오류");
                }
                value = offset.Groups[2].Value == "+" ? unchecked(value + (uint)delta) : unchecked(value - (uint)delta);
            }
            return unchecked((int)value);
        }
        throw new ArgumentException("알 수 없는 zorder 이름");
    }

    /// <summary>종류 필드에 strncpy(dst, word, min(strlen, 8))로 덮어쓴다. 짧은 단어는 이전 내용의 뒤가 남는다(원본 그대로).</summary>
    /// <param name="kind">8글자 종류 필드.</param>
    /// <param name="word">복사할 단어.</param>
    private static void CopyKind(char[] kind, string word)
    {
        // 최대 8글자만 덮어쓰고 종료 문자는 쓰지 않는다
        for (int index = 0; index < KindBytes && index < word.Length; index++)
        {
            kind[index] = word[index];
        }
    }

    /// <summary>00472e60/CD 0048a2c0이 만든 TYPE+98 조회 표를 읽는다. 접미사나 대소문자를 정규화하지 않는다.</summary>
    /// <param name="text">theme 문자열.</param>
    /// <param name="edition">원본 판본.</param>
    /// <exception cref="ArgumentOutOfRangeException">첫 글자가 원본 초기화 표 범위(122)를 넘는다.</exception>
    private static int ThemeNumber(string text, OriginalEdition edition)
    {
        // 원본 AST 문자열의 첫 BYTE다. 비ASCII 글자는 UTF-8 선행 바이트(0xC2 이상)처럼 범위를 넘는다
        int first = text.Length == 0 ? 0 : text[0] < 128 ? text[0] : 0xff;
        if (first >= 122)
        {
            throw new ArgumentOutOfRangeException(nameof(text), text, "TYPE theme의 원본 초기화 표 범위 초과");
        }
        bool patch = edition == OriginalEdition.Patch1078;
        // 원본 표의 첫 90 DWORD만 패치 5 / CD 4로 채우며 그 뒤의 미지정 BSS는 0이다
        int value = first < 90 ? (patch ? 5 : 4) : 0;
        const string themeLetters = "wrtsWRTS.?>"; // 두 PE 포인터가 가리키는 원문이며 순서 자체가 조회 의미다
        int count = patch ? 10 : 8; // 패치는 5원소×2, CD는 4원소×2를 기록한다
        int modulus = patch ? 5 : 4; // 패치의 대문자 결과 차이도 원본 그대로 보존한다
        // 초기화 몸체와 같은 순서로 지정 글자의 DWORD를 덮어쓴다
        for (int index = 0; index < count; index++)
        {
            if (first == themeLetters[index])
            {
                value = index % modulus;
            }
        }
        return value;
    }

    /// <summary>0049c3b0 / CD 004460f0의 플래그·속성 부분: 정의에서 타입 구조체의 필드를 채운다.</summary>
    /// <param name="type">채울 타입 구조체.</param>
    /// <param name="definition">읽은 .type 정의.</param>
    /// <param name="edition">원본 판본.</param>
    private static void ApplyDefinition(OriginalTypeRecord type, RiftTypeDefinition definition, OriginalEdition edition)
    {
        type.FootX = 1;
        type.FootY = 1;
        type.Flags1 = 0;
        type.Flags2 = 0;
        char[] kind = new char[KindBytes];
        // 플래그 단어마다 표의 비트를 켠다. 표에 없는 단어는 원본처럼 조용히 지나간다
        foreach (string word in definition.Flags)
        {
            if (RiftTypeDefinition.AsciiEqual(word, "dontdrawdefault"))
            {
                throw new InvalidOperationException("Type flag dontdrawdefault is rejected by the original");
            }
            // 단어 하나는 표의 한 항목에만 맞는다
            foreach (FlagWord entry in FlagWords)
            {
                if (!RiftTypeDefinition.AsciiEqual(word, entry.Word))
                {
                    continue;
                }
                type.Flags1 |= entry.Flags1;
                type.Flags2 |= entry.Flags2;
                if (entry.Kind)
                {
                    CopyKind(kind, word);
                }
            }
        }
        // 원본은 shadow와 flyershadow가 함께 있으면 assert 한다
        if ((type.Flags1 & 0x40000) != 0 && (type.Flags1 & 0x400000) != 0)
        {
            throw new InvalidOperationException("!(ts->typeFlags & TYPE_FLAG_FLYER_SHADOW)");
        }
        bool hasDescription = false;
        // 속성은 파일에 나온 순서대로 적용한다. 같은 이름이 여러 번이면 뒤의 값이 남는다
        foreach (TypeProperty property in definition.Properties)
        {
            double? number = property.Number;
            string? text = property.Text;
            bool Is(string name) => RiftTypeDefinition.AsciiEqual(property.Name, name);
            if (Is("maxHitPoints"))
            {
                if (number is null)
                {
                    throw new ArgumentException("maxHitPoints는 숫자여야 합니다");
                }
                if (type.MaxHitPoints != 0)
                {
                    throw new InvalidOperationException("maxHitPoints == 0");
                }
                type.Flags1 |= TypeFlag1.HasHitPoints;
                type.MaxHitPoints = ToInt(number.Value);
            }
            else if (Is("zorder"))
            {
                if (number is not null)
                {
                    type.ZOrder = ToInt(number.Value);
                }
                else if (text is not null)
                {
                    type.ZOrder = ZOrder(text);
                }
            }
            else if (Is("cost"))
            {
                if (number is null)
                {
                    throw new ArgumentException("cost는 숫자여야 합니다");
                }
                type.Cost = (float)number.Value;
            }
            else if (Is("damageEffect"))
            {
                // 실제 0049c3b0 / CD Parse의 type +0xd4를 숫자 절삭으로 채운다. 누락된 속성은 초기 0이다
                if (number is null)
                {
                    throw new ArgumentException("damageEffect는 숫자여야 합니다");
                }
                type.DamageEffect = ToInt(number.Value);
            }
            else if (Is("techBit"))
            {
                if (number is null)
                {
                    throw new ArgumentException("techBit는 숫자여야 합니다");
                }
                type.TechBitIndex = TechBitNumber(number.Value); // 현재 속성 순서대로 같은 실제 Record 필드를 덮어쓴다
            }
            else if (Is("minUsage") && number is not null)
            {
                type.MinUsage[0] = (float)number.Value;
            }
            else if ((Is("maxUsage") || Is("maxRainBattleUsage")) && number is not null)
            {
                type.MaxUsage[0] = (float)number.Value;
            }
            else if (Is("maxThunderBattleUsage") && number is not null)
            {
                type.MaxUsage[1] = (float)number.Value;
            }
            else if (Is("maxWindBattleUsage") && number is not null)
            {
                type.MaxUsage[2] = (float)number.Value;
            }
            else if ((Is("foot_x") || Is("footx")) && number is not null)
            {
                type.FootX = ToInt(number.Value);
            }
            else if ((Is("foot_y") || Is("footy")) && number is not null)
            {
                type.FootY = ToInt(number.Value);
            }
            else if (Is("hotFootRatioX") || Is("hotFootRatioY"))
            {
                if (number is null)
                {
                    throw new ArgumentException("hotFootRatio는 숫자여야 합니다");
                }
                bool vertical = Is("hotFootRatioY");
                int offset = TypeHotFootOffset((float)number.Value, vertical);
                if (vertical)
                {
                    type.HotspotY = offset;
                }
                else
                {
                    type.HotspotX = offset;
                }
            }
            else if (Is("hotFootX") || Is("hotFootY"))
            {
                throw new ArgumentException("원본이 거부하는 hotFootX/Y 속성입니다");
            }
            else if (Is("level") && number is not null)
            {
                type.Level = ToInt(number.Value) - 1;
            }
            else if (Is("theme"))
            {
                if (text is null)
                {
                    throw new ArgumentException("theme은 문자열이어야 합니다");
                }
                type.Theme = ThemeNumber(text, edition);
            }
            else if (Is("description") && text is not null)
            {
                type.Description = text.Length > DescriptionBytes ? text[..DescriptionBytes] : text;
                hasDescription = true;
            }
            else if (Is("group") && text is not null)
            {
                type.Group = -1;
                // 이름 표에서 첫 일치의 위치가 그룹 번호다
                for (int index = 0; index < GroupNames.Length; index++)
                {
                    if (RiftTypeDefinition.AsciiEqual(text, GroupNames[index]))
                    {
                        type.Group = index;
                        break;
                    }
                }
            }
        }
        // 원본 0049ad00/0049ad60: 사용량을 0이 아닌 값으로 정하면 그 자리에서 0x4000이 켜진다
        for (int element = 0; element < 3; element++)
        {
            if (type.MinUsage[element] != 0.0f || type.MaxUsage[element] != 0.0f)
            {
                type.Flags1 |= TypeFlag1.UsesPower;
            }
        }
        // 이름은 길이 제한 없이 복사되어 20글자 이상이면 설명 필드로 넘친다. 그 뒤에 `description`이 설명 필드를 덮어쓰면
        // C 문자열로 읽은 이름은 "이름 앞 20글자 + 설명"이 된다. 실제 파일의 `TypeNames` 해시로 확인했다
        if (type.Name.Length >= TypeNameBytes && hasDescription)
        {
            type.Name = type.Name[..TypeNameBytes] + type.Description;
        }
        // 종류 필드는 12바이트이고 0으로 초기화되어 있어 8글자를 다 채워도 그 뒤에서 끝난다
        int end = Array.IndexOf(kind, '\0');
        type.Kind = new string(kind, 0, end < 0 ? kind.Length : end);
    }

    /// <summary>0049b0d0 / CD 00444e10의 플래그·생략 비용 후처리다. 모든 타입(내장·빈 타입 포함)에 번호순 적용한다.</summary>
    /// <param name="type">후처리할 타입 구조체.</param>
    /// <param name="number">타입 번호.</param>
    private static void PostProcess(OriginalTypeRecord type, int number)
    {
        if (type.Group == 5)
        {
            type.Flags2 |= TypeFlag2.GroupBattery;
        }
        if (type.Group == 1 || type.Group == 0)
        {
            type.Flags2 |= TypeFlag2.GroupArcherOrCannon;
        }
        if (type.Group == 2)
        {
            type.Flags2 |= TypeFlag2.GroupBlocker;
        }
        uint flags2 = type.Flags2;
        if ((flags2 & 0x34200) != 0)
        {
            // vortex·factory·walker·balloon은 내용물을 가진다
            type.Flags1 |= TypeFlag1.Container;
            type.ContainerListFlags |= (flags2 & 0x30000) == 0 ? 0x10u : 0x20u;
        }
        if ((flags2 & TypeFlag2.Bomb) != 0)
        {
            type.ContentListFlags |= 1;
        }
        if ((flags2 & TypeFlag2.Buried) != 0)
        {
            type.Flags1 |= TypeFlag1.SaveQA;
        }
        if ((flags2 & 0xa00) != 0)
        {
            type.Flags1 |= 0x10000000;
        }
        if ((flags2 & TypeFlag2.Factory) != 0)
        {
            type.Flags1 |= 0x4000000;
        }
        if ((flags2 & TypeFlag2.Vortex) != 0)
        {
            type.Flags1 |= 0x84000;
        }
        // 원본은 여기서 minUsage·maxUsage가 0이 아니면 0x4000을 다시 켠다(ApplyDefinition에서 이미 켰다)
        if ((type.Flags1 & TypeFlag1.SaveQA) != 0 && (type.Flags1 & TypeFlag1.SaveQB) != 0)
        {
            throw new InvalidOperationException("(typeFlags & TYPE_FLAG_SAVE_Q16) == 0"); // 원본 assert
        }
        if ((type.Flags1 & TypeFlag1.MayDropOnRim) == 0)
        {
            type.Flags1 |= TypeFlag1.MayDropOnIsle;
        }
        if ((type.Flags2 & TypeFlag2.Emplacement) != 0)
        {
            type.Flags2 |= TypeFlag2.DropBlocking;
        }
        uint final2 = type.Flags2;
        if ((final2 & TypeFlag2.Emplacement) != 0 && (final2 & 0x4200) == 0 && type.FootX == 3 && type.FootY == 3)
        {
            type.Flags1 |= 6;
        }
        if ((final2 & 0x408100) != 0)
        {
            type.Flags1 |= TypeFlag1.MayDropOnIsle;
        }
        if ((final2 & 0x70000) != 0 && (final2 & TypeFlag2.Factory) == 0)
        {
            type.Flags1 |= 0x8000;
        }
        if (type.Group != NoGroup && type.Cost == 0.0F)
        {
            // 내부 level은 자료 level−1이다. 원본 signed DWORD 증가와 x87 정수 곱 뒤 float32 저장을 보존한다
            int level = unchecked((int)((uint)type.Level + 1U));
            // 패치 00510298/0051029c와 CD 00501998/00501994의 일반 200/운반 400 기본 단가다
            double unit = (final2 & 0x30000U) != 0 ? 400.0 : 200.0;
            type.Cost = (float)((double)level * unit);
        }
        if (type.FootY == 6)
        {
            type.FootY = 8; // 원본: foot_y 6은 8로 바꾼다
        }
        // 원본은 사제와 guy 부류의 명시 비용도 마지막에 지운다. 2026-10-28 기본 가격 누락을 복원했다
        if (number == CostExemptPriestType || (final2 & 0x400U) != 0)
        {
            type.Cost = 0.0F;
        }
    }
}
