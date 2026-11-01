using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// Template 저장소 검사(<see cref="CampaignTemplateStorageTests"/>, <see cref="CampaignTemplateSectionsTests"/>, <see cref="CampaignTemplateFileTests"/>)가 함께 쓰는 도우미다:
/// 원본 .fort 읽기(cpppj의 <c>FortTemplate::Parse</c> 대신 <see cref="FortTemplate.ReadSections"/>), 표준 섹션 위치, 두 범위의 같은 물리 주소 비교,
/// 실제 헤더 180바이트 읽기·쓰기, 같은 저장소에 연결한 Player State getter.
/// </summary>
/// <remarks>2026-11-01 0.132(TS2)에서 <see cref="CampaignTemplateStorageTests"/>의 비공개 도우미를 옮겨 공유했다. 0.133(TS3)에서 노드 관찰(<see cref="Observe"/>)과 할당 배열 조회(<see cref="ArrayOf"/>)를 더했다.</remarks>
internal static class TemplateStorageTestSupport
{
    /// <summary>실제 파일을 읽는 검사가 쓰는 원본 데이터 폴더의 파일 시스템이다(없으면 null).</summary>
    private static readonly Lazy<GameFileSystem?> Files = new(() =>
        GameDataLocator.FindDataDirectory() is { } directory ? GameFileSystem.Open(directory) : null);

    /// <summary>원본 데이터의 .fort 파일을 원시 섹션으로 읽는다.</summary>
    /// <param name="name">확장자를 뺀 파일 이름.</param>
    public static FortTemplate RealFort(string name)
    {
        Assert.True(Files.Value is not null, "클론 데이터 폴더가 없습니다");
        return FortTemplate.ReadSections(Files.Value!.ReadAllBytes("d/" + name + ".fort"));
    }

    /// <summary>원본 데이터의 .fort 파일 전체 바이트를 읽는다(서명·길이 레코드를 포함한 파일 그대로).</summary>
    /// <param name="name">확장자를 뺀 파일 이름.</param>
    /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
    public static byte[] RealFortBytes(string name)
    {
        Assert.True(Files.Value is not null, "클론 데이터 폴더가 없습니다");
        return Files.Value!.ReadAllBytes("d/" + name + ".fort");
    }

    /// <summary>표준 섹션의 위치를 이름표에서 찾는다.</summary>
    /// <param name="name">섹션 이름.</param>
    public static int IndexOf(string name)
    {
        // 이름표를 처음부터 훑어 위치를 찾는다
        for (int index = 0; index < FortTemplate.SectionNames.Count; index++)
        {
            if (FortTemplate.SectionNames[index] == name)
            {
                return index;
            }
        }
        throw new InvalidOperationException(name);
    }

    /// <summary>표준 위치까지 빈 섹션을 채우고 그 위치에 바이트를 넣는다.</summary>
    /// <param name="fort">요새.</param>
    /// <param name="name">섹션 이름.</param>
    /// <param name="bytes">넣을 바이트.</param>
    public static void SetSection(FortTemplate fort, string name, byte[] bytes)
    {
        int at = IndexOf(name);
        // 표준 위치가 나올 때까지 빈 섹션으로 늘린다
        while (fort.Sections.Count <= at)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        fort.Sections[at] = RawSectionBuffer.FromBytes(bytes);
    }

    /// <summary>35개 표준 섹션을 모두 빈 섹션으로 둔 요새를 만든다.</summary>
    public static FortTemplate EmptyFort()
    {
        FortTemplate fort = new();
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            fort.Sections.Add(new RawSectionBuffer());
        }
        return fort;
    }

    /// <summary>두 범위가 같은 물리 시작 주소를 가리키는가(C++의 <c>data()</c> 포인터 비교).</summary>
    /// <param name="first">첫째 범위.</param>
    /// <param name="second">둘째 범위.</param>
    public static bool SameStorage(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second)
    {
        return Unsafe.AreSame(ref MemoryMarshal.GetReference(first), ref MemoryMarshal.GetReference(second));
    }

    /// <summary>실제 헤더 180바이트의 비정렬 DWORD를 읽는다.</summary>
    /// <param name="storage">저장소.</param>
    /// <param name="at">헤더 안 위치.</param>
    public static uint Header(CampaignTemplateStorage storage, int at) => BinaryPrimitives.ReadUInt32LittleEndian(storage.Headers.Span[at..]);

    /// <summary>실제 헤더 180바이트에 DWORD를 쓴다.</summary>
    /// <param name="storage">저장소.</param>
    /// <param name="at">헤더 안 위치.</param>
    /// <param name="value">쓸 값.</param>
    public static void SetHeader(CampaignTemplateStorage storage, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(storage.Headers.Span[at..], value);

    /// <summary>한 노드의 16바이트 MemStream 필드를 인라인 쓰기로 관찰한 값이다.</summary>
    /// <param name="Pointer">data 토큰.</param>
    /// <param name="Position">pos.</param>
    /// <param name="End">end.</param>
    /// <param name="Capacity">alloc.</param>
    public readonly record struct StreamObservation(uint Pointer, uint Position, uint End, uint Capacity);

    /// <summary>이름/핸들의 편의 계산 없이 저장소의 같은 실제 raw 필드를 조회한다.</summary>
    /// <param name="storage">저장소.</param>
    /// <param name="stream">보관한 쓰기 참조.</param>
    public static StreamObservation Observe(CampaignTemplateStorage storage, TemplateWriteStream stream)
    {
        uint pointer = 0, position = 0, end = 0, capacity = 0;
        storage.AccessSection(stream, (fields, _) =>
        {
            pointer = BinaryPrimitives.ReadUInt32LittleEndian(fields);
            position = BinaryPrimitives.ReadUInt32LittleEndian(fields[4..]);
            end = BinaryPrimitives.ReadUInt32LittleEndian(fields[8..]);
            capacity = BinaryPrimitives.ReadUInt32LittleEndian(fields[12..]);
        });
        return new StreamObservation(pointer, position, end, capacity);
    }

    /// <summary>섹션 버퍼가 실제로 쓰는 배열 객체를 돌려준다(C++의 <c>data()</c> 포인터 비교를 객체 동일성으로 대신한다).</summary>
    /// <param name="buffer">섹션 버퍼.</param>
    public static byte[] ArrayOf(RawSectionBuffer buffer)
    {
        Assert.True(MemoryMarshal.TryGetArray<byte>(buffer.Memory, out ArraySegment<byte> segment));
        return segment.Array!;
    }

    /// <summary>State 확장의 명시 입력(28바이트)을 같은 저장소에 연결하는 Player State getter를 만든다. 진단은 모두 실패로 본다.</summary>
    /// <param name="storage">저장소.</param>
    /// <param name="select">참이면 State를 원본 선택(쓰기 허용)으로 얻는다.</param>
    public static RawPlayerTemplateState PlayerState(CampaignTemplateStorage storage, bool select)
    {
        uint depth = 0;
        return new RawPlayerTemplateState(
            OriginalEdition.Patch1078,
            storage.Headers,
            slot => select ? storage.SelectState(slot) : storage.State(slot),
            slot => storage.Name(slot),
            () => depth,
            value => depth = value,
            () => false,
            _ => throw new InvalidOperationException("State 확장의 예상 밖 진단"),
            _ => { },
            (slot, size) => storage.GrowState(slot, size),
            slot => storage.CommitStateGrowth(slot));
    }
}
