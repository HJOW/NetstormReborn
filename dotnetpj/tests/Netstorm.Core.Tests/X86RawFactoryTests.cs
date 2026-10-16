using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 오브젝트 생성(<see cref="RawSquidFactory"/>)과 생성자 표(<see cref="OriginalConstructors"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 constructors-x86.tsv(타입별 생성자 주소), creation-x86.tsv(기본 생성·Take 964회 중 10.78 몫), derived-x86.tsv(파생 생성자 6,028회 중 10.78 몫)다.
/// 각 호출 뒤 반환 번호·풀 카운터·목록 머리/꼬리·풀 전체 바이트의 체크섬과 대상 슬롯의 50바이트 전부를 비교한다.
/// 열 구성은 cpppj/tests/CreationTests.cpp 의 Replay 와 같다. CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 첫 조각, 2026-10-16).
/// </summary>
public sealed class X86RawFactoryTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 의 타입 번호 가운데 생성자 주소가 0 이 아닌 개수(기본 생성이 아닌 타입).</summary>
    private const int PatchTypesWithConstructor = 122;

    /// <summary>원본 생성자가 null 을 돌려주거나 assert 로 멈추는 자산 타입: platform(153), lightning(159), daisExtraFrames(169). 표에는 주소가 있지만 지원하지 않는다.</summary>
    private static readonly int[] UnsupportedAssetTypes = [153, 159, 169];

    /// <summary>생성자 주소 표만 채운 10.78 타입 레코드 188개를 만든다(합성 검사 입력의 바탕).</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) };
        }
        return types;
    }

    /// <summary>슬롯의 가상 표(+0, 4바이트)를 리틀 엔디언으로 쓴다. 호스트 함수 주소가 아니라 클래스 식별 값이다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutVtable(Span<byte> bytes, uint value)
    {
        // 값의 네 바이트를 낮은 쪽부터 놓는다
        for (int i = 0; i < 4; i++)
        {
            bytes[i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>
    /// 생성 fixture 를 10.78 몫만 재생한다. Type/Fill/Abstract 는 합성 입력이며 호출 수에 넣지 않는다.
    /// 호출마다 반환 번호·풀 카운터 7개와 풀 전체 바이트 체크섬, 대상 슬롯 50바이트 전부를 대조한다.
    /// </summary>
    /// <param name="fixture">fixture 이름(creation 또는 derived).</param>
    /// <param name="expectedBegins">10.78 세션 수.</param>
    /// <param name="expectedCalls">10.78 호출 수.</param>
    private static void Replay(string fixture, int expectedBegins, int expectedCalls)
    {
        string[][] rows = X86Fixture.Read(fixture);
        RawSidPool? pool = null;
        RawSquidFactory? factory = null;
        OriginalTypeRecord[] types = [];
        bool weakenedMana = false;
        bool patch = false;
        int begins = 0;
        int calls = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                patch = row[1] == PatchEdition;
                if (patch)
                {
                    pool = new RawSidPool(X86Fixture.Int(row[2]), row[3] == "1");
                    types = Types();
                    weakenedMana = row[4] == "1";
                    factory = new RawSquidFactory(pool, types, weakenedMana);
                    begins++;
                }
                return true;
            }
            if (!patch)
            {
                return true;
            }
            switch (row[0])
            {
                case "Type":
                {
                    OriginalTypeRecord type = types[X86Fixture.Int(row[1])];
                    type.Flags1 = X86Fixture.UInt(row[2]);
                    type.Flags2 = X86Fixture.UInt(row[3]);
                    type.MaxHitPoints = X86Fixture.Int(row[4]);
                    type.ZOrder = X86Fixture.Int(row[5]);
                    type.ConstructorAddress = X86Fixture.UInt(row[6]);
                    return true;
                }
                case "Fill":
                {
                    Span<byte> bytes = pool!.AllocatedBytes(X86Fixture.Int(row[1]));
                    int seed = X86Fixture.Int(row[2]);
                    // 슬롯 전체를 합성 입력으로 채운 뒤 상태·타입·가상 표를 쓴다
                    for (int i = 0; i < bytes.Length; i++)
                    {
                        bytes[i] = (byte)(seed + i * 37);
                    }
                    bytes[RawSidPool.StateOffset] = (byte)X86Fixture.Int(row[3]);
                    bytes[RawSidPool.TypeOffset] = (byte)X86Fixture.Int(row[4]);
                    PutVtable(bytes, X86Fixture.UInt(row[5]));
                    return true;
                }
                case "Abstract":
                    pool!.AllocatedBytes(X86Fixture.Int(row[1]))[40] = (byte)X86Fixture.Int(row[2]);
                    return true;
            }
            uint type2 = X86Fixture.UInt(row[2]);
            int arg = X86Fixture.Int(row[3]);
            int result = arg;
            switch (row[1])
            {
                case "Create":
                    result = factory!.Create(type2, (SidAllocation)arg);
                    break;
                case "Construct":
                    result = factory!.Construct(type2, result);
                    break;
                case "Take":
                    result = factory!.Take(type2, result);
                    break;
                case "PostCreate":
                    factory!.PostCreate(result);
                    break;
                case "PostTake":
                    factory!.PostTake(result);
                    break;
                default:
                    return false;
            }
            calls++;
            uint[] actual =
            [
                (uint)result, pool!.FreeCount, pool.PredictableCursor, (uint)pool.FirstFree(true), (uint)pool.Tail(true),
                (uint)pool.FirstFree(false), (uint)pool.Tail(false), X86Fixture.Adler32(pool.Bytes),
            ];
            // 반환 번호와 풀 카운터·목록 머리/꼬리·풀 전체 체크섬을 열마다 대조한다
            for (int i = 0; i < actual.Length; i++)
            {
                if (actual[i] != X86Fixture.UInt(row[i + 4]))
                {
                    return false;
                }
            }
            return Convert.ToHexStringLower(pool.Slot(result)) == row[12];
        });
        Assert.Equal(expectedBegins, begins);
        Assert.Equal(expectedCalls, calls);
    }

    /// <summary>10.78 의 타입별 생성자 주소 표 188개가 원본의 대입 구간 실행 결과와 모두 같다.</summary>
    [Fact]
    public void ConstructorAddressTableMatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("constructors", PatchEdition);
        Assert.Equal(OriginalConstructors.TypeCount, rows.Length);
        X86Fixture.CheckRows(rows, row => OriginalConstructors.Address(X86Fixture.Int(row[1])) == X86Fixture.UInt(row[2]));
        Assert.Equal(PatchTypesWithConstructor, rows.Count(row => X86Fixture.UInt(row[2]) != 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => OriginalConstructors.Address(OriginalConstructors.TypeCount));
        Assert.Throws<ArgumentOutOfRangeException>(() => OriginalConstructors.Address(-1));
    }

    /// <summary>
    /// 파생 생성자 표 82개와 타입 연결을 확인한다. 자산 타입(70 이상) 중 생성자 주소가 있는 타입은 표로 모두 풀리고(98개),
    /// 표에 없는 것은 원본이 null/assert 로 끝나는 153·159·169 뿐이며, 표의 주소는 서로 다르다.
    /// </summary>
    [Fact]
    public void RecipeTableCoversEverySupportedAssetConstructor()
    {
        Assert.Equal(82, OriginalConstructors.Recipes.Count);
        Assert.Equal(82, OriginalConstructors.Recipes.Select(recipe => recipe.Address).Distinct().Count());
        int[] withAddress = Enumerable.Range(OriginalConstructors.FirstAssetType, OriginalConstructors.TypeCount - OriginalConstructors.FirstAssetType)
            .Where(type => OriginalConstructors.Address(type) != 0).ToArray();
        int[] unsupported = withAddress.Where(type => OriginalConstructors.Recipe(OriginalConstructors.Address(type)) is null).ToArray();
        Assert.Equal(UnsupportedAssetTypes, unsupported);
        Assert.Equal(98, withAddress.Length - unsupported.Length);
        // 모든 표 항목의 마지막 가상 표 쓰기가 항목의 최종 가상 표 값과 같다
        Assert.All(OriginalConstructors.Recipes, recipe =>
            Assert.Equal(recipe.Vtable, recipe.Writes.Last(write => write.Offset == 0 && write.Width == 4).Value));
    }

    /// <summary>기본 생성·Take·초기화 검증의 10.78 몫(서버/클라이언트 2세션)이 원본 호출 결과와 슬롯 바이트까지 같다.</summary>
    [Fact]
    public void BaseCreationAndVoidTakeMatchOriginalX86() => Replay("creation", expectedBegins: 2, expectedCalls: 482);

    /// <summary>모든 지원 파생 생성자와 타입 연결의 10.78 몫(서버/클라이언트 × mana 약화 4세션)이 원본 호출 결과와 슬롯 바이트까지 같다.</summary>
    [Fact]
    public void DerivedConstructorsAndTakeMatchOriginalX86() => Replay("derived", expectedBegins: 4, expectedCalls: 3300);
}
