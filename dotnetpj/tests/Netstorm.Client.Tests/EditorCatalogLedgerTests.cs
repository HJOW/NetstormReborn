using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>편집 장부의 원본 호환 직렬화, 잠금과 실패 처리를 검사한다.</summary>
public sealed class EditorCatalogLedgerTests
{
    /// <summary>격리 디렉터리에서 장부 상태를 다시 열고 원본 파일을 보존한다.</summary>
    [Fact]
    public void StoresCppLedgerAndRetainsCompletedDeletion()
    {
        string parent = Path.Combine(Path.GetTempPath(), "NetstormLedgerTests"); string root = Path.Combine(parent, Guid.NewGuid().ToString("N"));
        string source = Path.Combine(root, "assets"); Directory.CreateDirectory(Path.Combine(source, "d"));
        string original = Path.Combine(source, "d", "Map.fort"); File.WriteAllText(original, "original");
        try
        {
            var paths = new EditorWorkspacePaths(source); string ledgerPath = Path.Combine(paths.WorkRoot + "-state", "catalog-v1.dat");
            using (var ledger = new EditorCatalogLedger(paths))
            {
                Assert.Equal(0u, ledger.State("d/Map.fort")); ledger.SetState("d/Map.fort", 1); Assert.Equal(1u, ledger.State("D/map.fort"));
                ledger.SetState("d/Map.fort", 2); Assert.Equal(2u, ledger.State("d/map.fort"));
                using var rival = new EditorCatalogLedger(paths); Assert.Throws<IOException>(() => rival.State("d/Map.fort"));
            }
            Assert.Equal(new byte[] { 0x4e, 0x53, 0x45, 0x46, 0x4f, 0x52, 0x54, 1 }, File.ReadAllBytes(ledgerPath)[..8]);
            using (var restored = new EditorCatalogLedger(paths)) Assert.Equal(2u, restored.State("d/Map.fort"));
            Assert.Equal("original", File.ReadAllText(original));
            File.WriteAllBytes(ledgerPath, [1, 2, 3]);
            using var broken = new EditorCatalogLedger(paths); Assert.Throws<InvalidDataException>(() => broken.State("d/Map.fort"));
            Assert.Equal(new byte[] { 1, 2, 3 }, File.ReadAllBytes(ledgerPath));
        }
        finally
        {
            string target = Path.GetFullPath(root); string boundary = Path.GetFullPath(parent) + Path.DirectorySeparatorChar;
            if (!target.StartsWith(boundary, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("검사 디렉터리 경계 오류");
            Directory.Delete(target, true);
        }
    }
}
