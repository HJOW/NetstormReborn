using System.Buffers.Binary;

namespace Netstorm.Assets.Tests;

/// <summary>기존 맵에 없는 버전 0·손상 길이·소유자·타입 변환 경계의 회귀 검사.</summary>
public sealed class FortCompatibilityTests
{
    /// <summary>10.78의 다리 타입 번호. docs/exe/cpp-fort-reconstruction.md에서 확정했다.</summary>
    private const byte BridgeType = 82;

    /// <summary>두 다리 뒤의 상태 바이트를 읽어야 다음 레코드가 밀리지 않는다.</summary>
    [Fact]
    public void VersionZero_PreservesEveryLegacyStateByte()
    {
        var fort = Load([0, 99, 2, 0, 0x23, BridgeType, 35, 0xA5, 0x45, BridgeType, 37, 0x5A]);
        FortObject[] objects = Assert.Single(fort.Chaff).Objects.ToArray();
        Assert.Equal(2, objects.Length);
        Assert.Equal((byte)0xA5, objects[0].LegacyState);
        Assert.Equal((byte)0x5A, objects[1].LegacyState);
        Assert.Equal(4, objects[1].CellHigh);
        Assert.Equal((byte)37, objects[1].BridgeShape);
    }

    /// <summary>원본은 부호 있는 섹션 길이가 음수이면 그 섹션 앞에서 판독을 멈춘다.</summary>
    [Fact]
    public void SectionLength_IsSignedInt16()
    {
        var catalog = new TypeCatalog(GameFileSystem.Open(OriginalData.RequireDirectory()));
        byte[] data = new byte[2 + 0x8000];
        data[0] = 0x46;
        data[1] = 0xFE;
        BinaryPrimitives.WriteUInt16LittleEndian(data.AsSpan(2), 0x8000);
        Assert.Equal(0, new FortFile(data, catalog).RawSectionCount);
    }

    /// <summary>첫 파일 번호만 변환한다. 같은 다리 해시를 반복한 둘째 번호는 해석할 수 없다.</summary>
    [Fact]
    public void DuplicateTypeHash_MapsOnlyFirstFileNumber()
    {
        byte[] hashes = new byte[9];
        hashes[0] = 2;
        uint hash = TypeCatalog.NameHash("bridge");
        BinaryPrimitives.WriteUInt32LittleEndian(hashes.AsSpan(1), hash);
        BinaryPrimitives.WriteUInt32LittleEndian(hashes.AsSpan(5), hash);
        Assert.Throws<InvalidDataException>(() => Load([2, 99, 1, 0, 0x11, 1, 35], hashes));
    }

    /// <summary>타입 0 카드의 원시 항목은 남기지만 실제 초기 덱에는 추가하지 않는다.</summary>
    [Fact]
    public void Deck_SkipsZeroTypeAndPreservesSection()
    {
        byte[] deck = [1, 0, 10, 1, 255];
        FortFile fort = Load([], deck: deck);
        Assert.Empty(fort.Deck);
        Assert.Equal(deck, fort.Section("Deck").ToArray());
    }

    /// <summary>저장 값은 유지하면서 일반 소유자는 1~8, 가이저·매장물·섬은 중립으로 정규화한다.</summary>
    [Theory]
    [InlineData(0, 1)]
    [InlineData(1, 1)]
    [InlineData(8, 8)]
    [InlineData(9, 1)]
    [InlineData(255, 1)]
    public void OwnerNormalization_PreservesRawValue(int stored, int expected)
    {
        var catalog = new TypeCatalog(GameFileSystem.Open(OriginalData.RequireDirectory()));
        var bridge = new FortObject(0, 0, catalog.Find("bridge")!, null, null, null, null, null, stored, []);
        Assert.Equal(stored, bridge.Owner);
        Assert.Equal(expected, bridge.NormalizedOwner);
        Assert.Equal(expected, bridge.OwnerForLoad);
        Assert.Equal(0, (bridge with { Type = catalog.Find("geyser")! }).OwnerForLoad);
        Assert.Null((bridge with { Owner = null }).NormalizedOwner);
    }

    /// <summary>청크와 타입 이름 섹션만 가진 합성 요새를 구성한다. 모든 길이는 접두어 2바이트를 포함한다.</summary>
    private static FortFile Load(byte[] chaff, byte[]? hashes = null, byte[]? deck = null)
    {
        using var stream = new MemoryStream();
        using var writer = new BinaryWriter(stream);
        writer.Write((byte)0x46);
        writer.Write((byte)0xFE);
        // Deck까지의 첫 열두 섹션을 원본 저장 순서대로 쓴다.
        for (int section = 0; section < 12; section++)
        {
            byte[] body = section == 4 ? chaff : section == 7 ? hashes ?? [] : section == 11 ? deck ?? [] : [];
            writer.Write((short)(body.Length + 2));
            writer.Write(body);
        }
        var catalog = new TypeCatalog(GameFileSystem.Open(OriginalData.RequireDirectory()));
        return new FortFile(stream.ToArray(), catalog);
    }
}
