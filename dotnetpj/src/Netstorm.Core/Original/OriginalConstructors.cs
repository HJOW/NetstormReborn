namespace Netstorm.Core.Original;

/// <summary>
/// 생성자가 오브젝트 슬롯에 하는 쓰기 하나. 원본의 짧은 생성자는 같은 슬롯에 mov/or 만 적용한다.
/// </summary>
/// <param name="Offset">슬롯 안의 쓰기 시작 위치(바이트).</param>
/// <param name="Width">쓰는 바이트 수(1~4). 값은 리틀 엔디언으로 쓴다.</param>
/// <param name="OrBits">참이면 기존 바이트와 OR 해서 쓴다. 거짓이면 값을 그대로 덮어쓴다.</param>
/// <param name="Value">쓰는 값(OR 이면 합칠 비트).</param>
public readonly record struct ConstructorWrite(int Offset, int Width, bool OrBits, uint Value);

/// <summary>
/// 파생 생성자 하나의 내용. 쓰기 목록은 원본 실행 순서이며 중간 가상 표 쓰기도 순서대로 남아 있다.
/// </summary>
/// <param name="Address">원본 실행 파일 안의 생성자 시작 주소. 타입 표의 생성자 주소와 대조하는 식별 값이며 호스트 함수로 호출하지 않는다.</param>
/// <param name="Vtable">생성자가 마지막으로 쓰는 가상 표 값. 이미 만든 오브젝트가 이 타입의 것인지 알아보는 데 쓴다.</param>
/// <param name="Writes">순서 있는 필드 쓰기 목록.</param>
public sealed record ConstructorRecipe(uint Address, uint Vtable, ConstructorWrite[] Writes);

/// <summary>
/// 10.78 오브젝트 타입별 생성자 주소와 파생 생성자의 필드 쓰기 표. 표 자료는 OriginalConstructors.Data.cs 에 있고
/// dotnetpj/tools/gen_original_constructors.py 가 cpppj 의 목록에서 만든다.
/// 주소가 0 인 타입은 기본 Squid 생성(가상 표 하나만 쓴다)을 따른다. 표에 없는 주소는 미복원 생성자이므로 호출하지 않고 거부한다.
/// 근거: docs/exe/cpp-creation-reconstruction.md, docs/exe/cpp-derived-reconstruction.md (W02 첫 조각, 2026-10-16).
/// </summary>
public static partial class OriginalConstructors
{
    /// <summary>10.78 의 타입 개수. 생성자 주소 표의 길이와 같다.</summary>
    public const int TypeCount = 188;

    /// <summary>자산 타입이 시작하는 번호. 이보다 작은 번호는 내장 Form/프로세스 타입이라 일반 생성의 대상이 아니다.</summary>
    public const int FirstAssetType = 70;

    /// <summary>기본 Squid 객체가 슬롯 +0 에 쓰는 가상 표 값(10.78). 호스트 포인터가 아니라 클래스 식별 값이다.</summary>
    public const uint BaseVtable = 0x00501DD0;

    /// <summary>기본 Squid 객체가 슬롯 +0 에 쓰는 가상 표 값(CD 10.72). (F2c-11, 2026-11-04)</summary>
    public const uint CdBaseVtable = 0x005058D8;

    /// <summary>생성자 주소로 파생 생성자를 찾는 사전. 같은 주소가 여럿이면 표에서 먼저 나온 항목을 쓴다(원본 조회와 같다).</summary>
    private static readonly Dictionary<uint, ConstructorRecipe> RecipesByAddress = [];

    /// <summary>CD 생성자 주소로 파생 생성자를 찾는 사전. (F2c-11, 2026-11-04)</summary>
    private static readonly Dictionary<uint, ConstructorRecipe> CdRecipesByAddress = [];

    /// <summary>
    /// 파생 생성자 표에서 주소 사전을 채운다. 표(RecipeTable)는 부분 클래스의 다른 파일에 있고 정적 필드 초기화 순서는 파일 사이에 정해져 있지 않으므로,
    /// 모든 필드 초기화가 끝난 뒤에 실행되는 정적 생성자에서 만든다.
    /// </summary>
    static OriginalConstructors()
    {
        // 표 순서대로 넣되 이미 있는 주소는 덮지 않는다
        foreach (ConstructorRecipe recipe in RecipeTable)
        {
            RecipesByAddress.TryAdd(recipe.Address, recipe);
        }
        // CD 표도 같은 방식으로 채운다
        foreach (ConstructorRecipe recipe in CdRecipeTable)
        {
            CdRecipesByAddress.TryAdd(recipe.Address, recipe);
        }
    }

    /// <summary>타입 번호의 생성자 원본 주소를 돌려준다. 0 이면 기본 생성이다.</summary>
    /// <param name="typeNumber">타입 번호(0~187).</param>
    public static uint Address(int typeNumber) => typeNumber >= 0 && typeNumber < AddressTable.Length
        ? AddressTable[typeNumber]
        : throw new ArgumentOutOfRangeException(nameof(typeNumber), typeNumber, "생성자 타입 번호 오류");

    /// <summary>
    /// 판본별 타입 번호의 생성자 원본 주소를 돌려준다(cpppj <c>TypeConstructorAddress</c>). 패치판은 0~187, CD판은 0~170이다.
    /// 호스트 함수 주소로 쓰지 않는 식별 값이며 0이면 기본 생성이다. 추가: F1 타입 표가 두 판본의 주소를 채우려고 CD 몫을 더했다(2026-11-02).
    /// </summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="typeNumber">타입 번호.</param>
    /// <exception cref="ArgumentOutOfRangeException">판본의 타입 수 밖이다.</exception>
    public static uint Address(OriginalEdition edition, int typeNumber)
    {
        uint[] table = edition == OriginalEdition.Cd1072 ? CdAddressTable : AddressTable;
        return typeNumber >= 0 && typeNumber < table.Length
            ? table[typeNumber]
            : throw new ArgumentOutOfRangeException(nameof(typeNumber), typeNumber, "생성자 타입 번호 오류");
    }

    /// <summary>생성자 주소에 대응하는 검증된 파생 생성자를 찾는다. 표에 없으면(미복원·null·assert 생성자) null 이다.</summary>
    /// <param name="address">타입의 생성자 주소.</param>
    public static ConstructorRecipe? Recipe(uint address) => RecipesByAddress.GetValueOrDefault(address);

    /// <summary>판본별 생성자 주소에 대응하는 검증된 파생 생성자를 찾는다(cpppj <c>Recipe</c>). 표에 없으면 null 이다.</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="address">타입의 생성자 주소.</param>
    /// <remarks>2026-11-04 F2c-11에서 추가했다.</remarks>
    public static ConstructorRecipe? Recipe(OriginalEdition edition, uint address)
    {
        return edition == OriginalEdition.Cd1072
            ? CdRecipesByAddress.GetValueOrDefault(address)
            : RecipesByAddress.GetValueOrDefault(address);
    }

    /// <summary>판본별 기본 Squid 객체의 슬롯 +0 최종 가상 표 값이다(cpppj <c>SquidFactory::BaseVtable</c>).</summary>
    /// <param name="edition">원본 판본.</param>
    /// <remarks>2026-11-04 F2c-11에서 추가했다.</remarks>
    public static uint BaseVtableFor(OriginalEdition edition)
    {
        return edition == OriginalEdition.Cd1072 ? CdBaseVtable : BaseVtable;
    }

    /// <summary>지원하는 파생 생성자 목록을 읽기 전용으로 돌려준다. 검사와 도구가 개수와 주소를 확인하는 데 쓴다.</summary>
    public static IReadOnlyList<ConstructorRecipe> Recipes => RecipeTable;

    /// <summary>지원하는 CD 파생 생성자 목록을 읽기 전용으로 돌려준다. 판본별 가상 표 판별이 CD 표를 읽는 데 쓴다. 2026-11-07 F2d 공장 현재 생성 이식에서 추가됐다.</summary>
    public static IReadOnlyList<ConstructorRecipe> CdRecipes => CdRecipeTable;
}
