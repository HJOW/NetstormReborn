using Netstorm.Assets;

namespace Netstorm.Core.Tests;

/// <summary>
/// Core 테스트용 원본 데이터 접근. 원본 폴더를 찾지 못하면 원본이 필요한 테스트를 건너뛴다.
/// 타입 목록은 한 번만 읽어 여러 테스트가 함께 쓴다.
/// </summary>
internal static class OriginalData
{
    /// <summary>찾은 원본 데이터 폴더 (없으면 null)</summary>
    private static readonly Lazy<string?> DirectoryLazy = new(GameDataLocator.FindDataDirectory);

    /// <summary>원본 자원 구성 (폴더가 없으면 null)</summary>
    private static readonly Lazy<GameResources?> ResourcesLazy = new(() =>
        DirectoryLazy.Value == null ? null : new GameResources(GameFileSystem.Open(DirectoryLazy.Value), GameLanguage.English));

    /// <summary>원본 타입 목록 (폴더가 없으면 null)</summary>
    private static readonly Lazy<TypeCatalog?> TypesLazy = new(() => ResourcesLazy.Value?.LoadTypes());

    /// <summary>원본 자원. 없으면 현재 테스트를 건너뛴다</summary>
    public static GameResources RequireResources()
    {
        Assert.SkipWhen(ResourcesLazy.Value == null, $"원본 데이터 폴더가 없어 건너뜀 ({GameDataLocator.EnvironmentVariable} 로 지정 가능)");
        return ResourcesLazy.Value!;
    }

    /// <summary>원본 타입 목록. 없으면 현재 테스트를 건너뛴다</summary>
    public static TypeCatalog RequireTypes()
    {
        RequireResources();
        return TypesLazy.Value!;
    }
}
