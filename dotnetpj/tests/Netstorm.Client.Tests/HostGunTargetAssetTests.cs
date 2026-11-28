using System.Globalization;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 제품의 TYPE/SHP 적재 경로를 독립 원본 사정선 관측과 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>보호 자산과 제품 어댑터를 거친 썬 캐논의 사정선 48개를 검사한다.</summary>
public sealed class HostGunTargetAssetTests
{
    /// <summary>원본 단정도 비트를 부호 없는 십진수에서 복원한다.</summary>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>계산 결과의 네 경계를 원본 DWORD 순서로 직렬화한다.</summary>
    private static string RectBits(GunTargetRect value) => string.Join(',', new[]
    {
        value.Left, value.Top, value.Right, value.Bottom,
    }.Select(BitConverter.SingleToUInt32Bits));

    /// <summary>게임 실행이나 파일 쓰기 없이 보호 원본의 TYPE/SHP를 제품 적재기로 읽는다.</summary>
    [Fact]
    public void CannonAssetRay48MatchesProtectedOriginalThroughProductShapes()
    {
        DirectoryInfo? directory = new(AppContext.BaseDirectory);
        // 검사 출력 위치에서 저장소 루트까지 올라가 보호 원본을 읽을 위치만 정한다.
        while (directory is not null && !File.Exists(Path.Combine(directory.FullName, "AGENTS.md"))) directory = directory.Parent;
        Assert.NotNull(directory);
        GameFileSystem files = GameFileSystem.Open(Path.Combine(directory!.FullName, "originals"));
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(files);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(files);
        var database = new ShapeDatabase(files.ReadAllBytes("d/_shapes.shp"));
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, database);
        Assert.Equal(0, table.Types[128].HotspotX);
        Assert.Equal(6, table.Types[128].HotspotY);
        var targetTypes = Enumerable.Range(0, table.Types.Count).Select(_ => new GunTargetType()).ToArray();
        targetTypes[128].Range = (float)definitions[128 - TerrainDisplayShapes.FirstAssetType].Number("range")!.Value;
        targetTypes[128].Theme = table.Types[128].Theme;
        var pool = new RawSidPool(24000, edition: OriginalEdition.Patch1078);
        var logic = new RawGunTarget(pool, new RawSquidHash(), table.Types, targetTypes, shapes, new GunTargetState());
        int count = 0;
        // 기대값은 실제 원본에서 동결한 48행이며 제품 헤더나 계산으로 다시 만들지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "cannon-asset-ray-x86.tsv")))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] row = line.Split('\t');
            Assert.Equal(2, row.Length);
            uint[] input = row[0].Split(',').Select(Number).ToArray();
            Assert.Equal(3, input.Length);
            float x = BitConverter.UInt32BitsToSingle(input[0]), y = BitConverter.UInt32BitsToSingle(input[1]);
            var query = new GunTargetQuery { X = x, Y = y, CenterX = x - 1.5f, CenterY = y - 1.5f, Type = 128, Owner = 1 };
            Assert.Equal(row[1], RectBits(logic.CannonRay(query, unchecked((int)input[2]))));
            count++;
        }
        Assert.Equal(48, count);
    }
}
