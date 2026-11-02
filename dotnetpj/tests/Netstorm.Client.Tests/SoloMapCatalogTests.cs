using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>실제 파일 열거와 Fort 원문을 편집 목록의 현재 캐시에 연결한다.</summary>
public sealed class SoloMapCatalogTests
{
    [Fact]
    public void EmptyAndShortStateUseOriginalInitialization()
    {
        string root = Path.Combine(Path.GetTempPath(), "netstorm-solo-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(root);
        try
        {
            byte[] Image(byte[] state)
            {
                using var output = new MemoryStream(); output.WriteByte(0x46); output.WriteByte(0xfe);
                byte[] subscriber = [1, 0, 0, 0, 2, 0, (byte)'M', (byte)'e'];
                Span<byte> length = stackalloc byte[2];
                for (int i = 0; i < 35; i++)
                {
                    byte[] content = i == 0 ? subscriber : i == 1 ? state : [];
                    BinaryPrimitives.WriteUInt16LittleEndian(length, (ushort)(content.Length + 2)); output.Write(length); output.Write(content);
                }
                return output.ToArray();
            }
            File.WriteAllBytes(Path.Combine(root, "empty.fort"), Image([]));
            File.WriteAllBytes(Path.Combine(root, "short.fort"), Image([0]));
            var files = new GameFileSystem(root, []); var config = new ConfigInterface(null); config.Startup(new()); config.Set("fortSpec", "{local.1}.fort");
            int allocated = 0;
            var catalog = new SoloMapCatalog(files, config, new SoloMapCatalogEffects
            {
                TypeCount = () => 188, AllocationBytes = bytes => { allocated++; bytes.Fill(0xa5); }, UserName = () => "Me", Translate = x => x
            });
            Assert.Equal(0, catalog.Kind("empty")); Assert.Equal(0, catalog.Kind("short")); Assert.Equal(1, allocated);
            Assert.Equal(2, catalog.List.Refresh(0));
            Assert.Equal(2u, catalog.State.Flags[Array.IndexOf(catalog.State.Names, "empty")]);
            Assert.Equal(0u, catalog.State.Flags[Array.IndexOf(catalog.State.Names, "short")]);
            Assert.Equal(3, allocated); Assert.Throws<InvalidOperationException>(() => catalog.Kind("missing"));
        }
        finally { Directory.Delete(root, true); }
    }

    [Fact]
    public void ReadsRealFortTypesAndBuildsEditableRows()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var files = startup.Files; var config = startup.Configuration;
        config.Set("fortSpec", "d/{local.1}.fort"); config.SetInt("mayEditMissions", 1);
        var catalog = new SoloMapCatalog(files, config, new SoloMapCatalogEffects
        {
            TypeCount = () => 188, AllocationBytes = _ => throw new InvalidOperationException("실제 28바이트 State가 필요"), UserName = () => "", Translate = text => text
        });
        var paths = files.MatchDisk("d/*.fort"); Assert.NotEmpty(paths); int checkedCount = 0, editable = 0;
        foreach (string path in paths)
        {
            byte[] image = files.ReadAllBytes(path); Assert.Equal(0x46, image[0]);
            int firstLength = BinaryPrimitives.ReadInt16LittleEndian(image.AsSpan(2)); int stateOffset = 2 + firstLength;
            int stateLength = BinaryPrimitives.ReadInt16LittleEndian(image.AsSpan(stateOffset)); Assert.True(stateLength >= 30);
            string name = Path.GetFileNameWithoutExtension(path); byte kind = image[stateOffset + 2 + 17];
            Assert.Equal(kind, catalog.Kind(name)); if (kind is 1 or 2) editable++; checkedCount++;
        }
        Assert.True(checkedCount > 100); catalog.Invalidate(); int count = catalog.List.Refresh(2); Assert.Equal(Math.Min(50, editable), count);
        for (int i = 0; i < count; i++) { string name = stateName(catalog, i); var row = catalog.List.Row(i, name, ""); Assert.NotNull(row); Assert.True(row.Checked); Assert.True(row.Enabled); }
        static string stateName(SoloMapCatalog catalog, int index) => catalog.State.Names[index]!;
    }
}
