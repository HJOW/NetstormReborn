using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 보호된 실제 원본 파일(<c>originals</c>·<c>originalCD</c>)로 타입 표와 <see cref="FortTemplate.Parse"/>를 대조한다. 원본 파일은 읽기만 한다.
/// (1) 두 판본의 .type 전체를 새 파서로 읽어 타입 표를 만든다. (2) 패치판 techBit 188필드를 원본 관측과 대조한다(cpppj <c>TypeTechBit_Protected116…</c>).
/// (3) 기존 경로의 독립 파서(<c>Netstorm.Assets.TypeDefinition</c>)와 이름·수식어·플래그·클러스터·이미지 참조를 비교한다.
/// (4) 모든 .fort의 <c>TypeNames</c> 해시가 해당 판본의 타입 표 해시와 같은지 확인하고 그 파일을 끝까지 해석한다. (F1, 2026-11-02)
/// </summary>
/// <remarks>
/// 이 파일의 검사는 작성 시점(2026-11-02)에 아직 실행하지 않았다(사용자 지시로 테스트 수행을 미뤘다). 해시 일치 개수의 하한은
/// <c>docs/exe/cpp-fort-reconstruction.md</c>의 기록(패치 29개, CD판 낱개 415개 + CD 아카이브 9개)에서 가져왔으며 첫 실행에서 실제 개수로 고쳐 적는다.
/// </remarks>
public sealed class RiftTypeOriginalFilesTests
{
    /// <summary>실행 출력 폴더에서 보호 원본이 있는 저장소 루트를 찾는다.</summary>
    private static string Root()
    {
        // 실제 저장소 경계까지 부모를 따라가며 원본 파일은 읽기만 한다
        for (DirectoryInfo? directory = new(AppContext.BaseDirectory); directory is not null; directory = directory.Parent)
        {
            if (File.Exists(Path.Combine(directory.FullName, "originals", "Netstorm.exe")))
            {
                return directory.FullName;
            }
        }
        throw new DirectoryNotFoundException("보호 원본 루트(originals/Netstorm.exe)를 찾지 못했다");
    }

    /// <summary>판본의 원본 아카이브를 연다.</summary>
    /// <param name="edition">원본 판본.</param>
    private static TaffArchive Archive(OriginalEdition edition) => edition == OriginalEdition.Patch1078
        ? TaffArchive.Open(Path.Combine(Root(), "originals", "netstorm.tarc"))
        : TaffArchive.Open(Path.Combine(Root(), "originalCD", "NETSTORM.TARC"));

    /// <summary>원본 텍스트를 cpppj <c>DecodeOriginalText</c>와 같이 읽는다: UTF-8 BOM이면 그 뒤를 UTF-8로, 아니면 Windows-1252로.</summary>
    /// <param name="bytes">파일 바이트.</param>
    private static string Decode(byte[] bytes) =>
        bytes.AsSpan().StartsWith((ReadOnlySpan<byte>)[0xEF, 0xBB, 0xBF]) ? System.Text.Encoding.UTF8.GetString(bytes, 3, bytes.Length - 3) : OriginalText.DecodeWindows1252(bytes);

    /// <summary>판본의 .type 원문 바이트를 로딩 순서대로 읽는다.</summary>
    /// <param name="edition">원본 판본.</param>
    private static List<byte[]> TypeBytes(OriginalEdition edition)
    {
        TaffArchive archive = Archive(edition);
        List<byte[]> result = [];
        // 로딩 순서의 이름마다 아카이브에서 d/이름.type을 읽는다
        foreach (string name in RiftTypeLoadOrder.Get(edition))
        {
            Assert.True(archive.TryFind("d/" + name + ".type", out TaffEntry entry), $"{edition} 아카이브에 d/{name}.type 이 없다");
            result.Add(archive.Read(entry));
        }
        return result;
    }

    /// <summary>판본의 모든 .type을 읽어 타입 표를 만든다.</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="definitions">읽은 정의(로딩 순서).</param>
    private static RiftTypeTable BuildTable(OriginalEdition edition, out List<RiftTypeDefinition> definitions)
    {
        List<RiftTypeDefinition> parsed = [.. TypeBytes(edition).Select(bytes => RiftTypeDefinition.Parse(Decode(bytes)))];
        definitions = parsed;
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
        return new RiftTypeTable(edition, order.Select((name, index) => new RiftTypeSource(name, parsed[index])).ToArray());
    }

    /// <summary>두 판본의 실제 .type 전체(패치 116개, CD 101개)가 새 파서와 타입 표를 오류 없이 통과하고, 입력이 모두 ASCII라 문자 단위 한도가 바이트 단위와 같다.</summary>
    [Fact]
    public void ProtectedTypesBuildBothEditionTables()
    {
        foreach ((OriginalEdition edition, int assets, int total) in new[] { (OriginalEdition.Patch1078, 116, 188), (OriginalEdition.Cd1072, 101, 171) })
        {
            // 입력이 모두 ASCII인지 확인한다(원본 바이트 = 문자 하나 = 바이트 하나의 전제)
            foreach (byte[] bytes in TypeBytes(edition))
            {
                bool bom = bytes.AsSpan().StartsWith((ReadOnlySpan<byte>)[0xEF, 0xBB, 0xBF]);
                Assert.True(bom || bytes.All(value => value < 0x80), $"{edition}: 비ASCII 바이트가 있는 .type");
            }
            RiftTypeTable table = BuildTable(edition, out List<RiftTypeDefinition> definitions);
            Assert.Equal(assets, definitions.Count);
            Assert.Equal(total, table.Types.Count);
            // 자산 타입은 표시된 번호의 파일 이름을 갖고, 이름 해시는 이름 필드의 C 문자열 읽기와 같다
            for (int index = 0; index < assets; index++)
            {
                OriginalTypeRecord record = table.Types[RiftTypeTable.FirstAssetTypeNumber + index];
                Assert.True(record.FromAsset);
                Assert.Equal(RiftTypeLoadOrder.Get(edition)[index], record.AssetName);
                Assert.StartsWith(record.AssetName.Length > 20 ? record.AssetName[..20] : record.AssetName, record.Name);
                Assert.NotEmpty(definitions[index].Clusters);
            }
            Assert.True(table.BridgeType() == 82 && table.IslandType() == 94);
        }
    }

    /// <summary>
    /// 보호 아카이브의 실제 116본문을 새 파서로 읽고 원문 현재 TYPE 188필드(techBit)와 전부 대조한다. 별도 원본 관측 파일(type-tech-bit-assets)의 첫 줄이 188 초기값이고
    /// 그 뒤 116행이 로딩 순서 번호·이름·techBit 원문이다.
    /// </summary>
    [Fact]
    public void ProtectedTypesMatchAll188NativeTechBitFields()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "type-tech-bit-assets-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        // 첫 줄은 '#188필드'로 시작하지만 자료다(주석 표기와 겹치므로 X86Fixture를 쓰지 않고 직접 읽는다)
        string[] first = lines[0].Split('\t');
        Assert.True(first.Length == 2 && first[1].Length == 188 * 8);
        RiftTypeTable table = BuildTable(OriginalEdition.Patch1078, out List<RiftTypeDefinition> definitions);
        Assert.True(table.Types.Count == 188 && definitions.Count == 116);
        // 내장/빈 타입도 원문 초기값과 비교한다
        for (int number = 0; number < table.Types.Count; number++)
        {
            Assert.Equal(RiftTypeX86Tests.TechBitSigned(first[1].Substring(number * 8, 8)), table.Types[number].TechBitIndex);
        }
        int assets = 0;
        int selected = 0;
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(OriginalEdition.Patch1078);
        // 그 뒤 116행은 실제 PE 로딩 순서이며 제품 목록에서 기대를 만들지 않는다
        foreach (string line in lines.Skip(1).Where(line => line.Length > 0 && line[0] != '#'))
        {
            string[] row = line.Split('\t');
            Assert.Equal(6, row.Length);
            Assert.True(int.Parse(row[0]) == assets + 70 && row[1] == order[assets]);
            Assert.Equal(RiftTypeX86Tests.TechBitSigned(row[4]), table.Types[assets + 70].TechBitIndex);
            if (row[3] != "[]")
            {
                selected++;
            }
            assets++;
        }
        Assert.Equal(116, assets);
        Assert.Equal(23, selected);
    }

    /// <summary>
    /// 기존 경로의 독립 .type 파서(<c>Netstorm.Assets.TypeDefinition</c>)와 이름·수식어·플래그 단어·클러스터 이름·이미지 참조를 두 판본 모두 비교한다.
    /// 두 파서는 따로 쓰였으므로 일치하면 문법 해석의 독립 근거가 된다. 기존 경로를 은퇴시킬 때 이 비교도 함께 지운다.
    /// </summary>
    [Fact]
    public void ParserAgreesWithLegacyIndependentParser()
    {
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            List<byte[]> files = TypeBytes(edition);
            for (int index = 0; index < files.Count; index++)
            {
                string name = RiftTypeLoadOrder.Get(edition)[index];
                RiftTypeDefinition mine = RiftTypeDefinition.Parse(Decode(files[index]));
                TypeDefinition legacy = TypeDefinition.Parse(files[index]);
                Assert.True(string.Equals(mine.Name, legacy.Name, StringComparison.OrdinalIgnoreCase), $"{name}: typename");
                Assert.True(string.Equals(mine.Constructor, legacy.Modifier, StringComparison.OrdinalIgnoreCase), $"{name}: 수식어");
                Assert.True(mine.Flags.SequenceEqual(legacy.Flags, StringComparer.OrdinalIgnoreCase), $"{name}: 플래그 단어");
                Assert.True(mine.Properties.Count == legacy.PropertySequence.Count, $"{name}: 속성 수");
                Assert.True(mine.Clusters.Count == legacy.Clusters.Count, $"{name}: 클러스터 수");
                // 클러스터마다 이름과 이미지 참조(파일, 번호)를 비교한다
                for (int cluster = 0; cluster < mine.Clusters.Count; cluster++)
                {
                    Assert.True(string.Equals(mine.Clusters[cluster].Name, legacy.Clusters[cluster].Name, StringComparison.OrdinalIgnoreCase), $"{name}[{cluster}]: 이름");
                    Assert.True(mine.Clusters[cluster].Images.Count == legacy.Clusters[cluster].Layers.Count, $"{name}[{cluster}]: 이미지 수");
                    for (int layer = 0; layer < mine.Clusters[cluster].Images.Count; layer++)
                    {
                        TypeImageReference image = mine.Clusters[cluster].Images[layer];
                        Assert.True(string.Equals(image.File, legacy.Clusters[cluster].Layers[layer].Image, StringComparison.OrdinalIgnoreCase)
                            && (int)image.Number == legacy.Clusters[cluster].Layers[layer].Frame, $"{name}[{cluster}][{layer}]: 이미지 참조");
                    }
                }
            }
        }
    }

    /// <summary>한 .fort 파일의 출처와 바이트다.</summary>
    /// <param name="Source">사람이 읽는 출처(경로 또는 아카이브:이름).</param>
    /// <param name="Bytes">파일 전체 바이트.</param>
    private sealed record FortFile(string Source, byte[] Bytes);

    /// <summary>원본 폴더의 낱개 .fort와 두 아카이브 안의 .fort를 모두 모은다.</summary>
    private static List<FortFile> AllForts()
    {
        List<FortFile> result = [];
        // 패치판 낱개 파일
        foreach (string path in Directory.GetFiles(Path.Combine(Root(), "originals", "d"), "*.fort"))
        {
            result.Add(new FortFile(path, File.ReadAllBytes(path)));
        }
        // 두 아카이브 안의 파일
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            TaffArchive archive = Archive(edition);
            foreach (TaffEntry entry in archive.Entries.Where(entry => entry.Name.EndsWith(".fort", StringComparison.OrdinalIgnoreCase)))
            {
                result.Add(new FortFile($"{edition}:{entry.Name}", archive.Read(entry)));
            }
        }
        return result;
    }

    /// <summary>파일의 <c>TypeNames</c> 해시 목록을 읽는다(섹션이 없거나 비면 빈 목록).</summary>
    /// <param name="bytes">.fort 파일 바이트.</param>
    private static List<uint> StoredHashes(byte[] bytes)
    {
        FortTemplate sections = FortTemplate.ReadSections(bytes);
        ReadOnlySpan<byte> body = sections.Section("TypeNames");
        List<uint> hashes = [];
        // 개수 바이트 뒤로 해시 4바이트가 이어진다
        if (!body.IsEmpty)
        {
            for (int index = 0; index < body[0] && 1 + (index * 4) + 4 <= body.Length; index++)
            {
                hashes.Add(BitConverter.ToUInt32(body.Slice(1 + (index * 4), 4)));
            }
        }
        return hashes;
    }

    /// <summary>
    /// 실제 .fort 전체에서 <c>TypeNames</c>가 패치판 타입 188개(또는 CD판 171개)로 저장된 파일은 해당 판본 타입 표의 해시와 같다(타입 번호 체계의 독립 근거).
    /// 같은 파일은 끝까지 해석되고 모든 저장 번호가 해시가 같은 현재 타입으로 변환된다.
    /// </summary>
    [Fact]
    public void FortTypeNamesMatchEditionTablesAndParseCompletely()
    {
        RiftTypeTable patch = BuildTable(OriginalEdition.Patch1078, out _);
        RiftTypeTable cd = BuildTable(OriginalEdition.Cd1072, out _);
        uint[] patchHashes = patch.Types.Select(type => RiftTypeTable.NameHash(type.Name)).ToArray();
        uint[] cdHashes = cd.Types.Select(type => RiftTypeTable.NameHash(type.Name)).ToArray();
        int patchMatches = 0;
        int cdMatches = 0;
        int objectCount = 0;
        List<string> failures = [];
        // 모든 파일을 훑는다
        foreach (FortFile file in AllForts())
        {
            List<uint> hashes = StoredHashes(file.Bytes);
            RiftTypeTable? table = hashes.SequenceEqual(patchHashes) ? patch : hashes.SequenceEqual(cdHashes) ? cd : null;
            if (table is null)
            {
                // 더 오래된 빌드로 저장되어 타입 구성이 다른 파일(개수가 171개여도 구성이 다른 캠페인 5개 포함)은 이 단위에서 해석하지 않는다
                continue;
            }
            if (table == patch)
            {
                patchMatches++;
            }
            else
            {
                cdMatches++;
            }
            try
            {
                FortTemplate fort = FortTemplate.Parse(file.Bytes, table);
                objectCount += fort.Chaff.Chunks.Sum(chunk => chunk.Objects.Count) + fort.Territories.Sum(section => section.Chunks.Sum(chunk => chunk.Objects.Count));
                // 저장 번호마다 변환한 현재 타입의 이름 해시가 저장된 해시와 같다(이름 없는 타입의 해시 0은 번호가 큰 쪽이 남는다)
                for (int stored = 0; stored < hashes.Count && stored < 0xf6; stored++)
                {
                    if (RiftTypeTable.NameHash(table.Types[fort.ConvertType((byte)stored)].Name) != hashes[stored])
                    {
                        failures.Add($"{file.Source}: 저장 번호 {stored} 변환이 해시와 다르다");
                        break;
                    }
                }
            }
            catch (InvalidDataException error)
            {
                failures.Add($"{file.Source}: {error.Message}");
            }
        }
        Assert.True(failures.Count == 0, $"{failures.Count}개 실패: {string.Join(" | ", failures.Take(40))}");
        // docs/exe/cpp-fort-reconstruction.md 의 기록이 하한이다(패치 29개, CD판 낱개 415개 + CD 아카이브 9개). 첫 실행에서 실제 개수로 고쳐 적는다
        Assert.True(patchMatches >= 29, $"패치판 해시 일치 {patchMatches}개");
        Assert.True(cdMatches >= 415 + 9, $"CD판 해시 일치 {cdMatches}개");
        Assert.True(objectCount > 0);
    }
}
