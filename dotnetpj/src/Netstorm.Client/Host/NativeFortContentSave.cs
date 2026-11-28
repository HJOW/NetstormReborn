using Netstorm.Core.Original;

// 같은 장기 Surface와 실제 Template stream을 내용물 저장 본체에 빌려 준다.
namespace Netstorm.Client.Host;

/// <summary>cpppj/src/client/NativeFortSnapshot.cpp의 Contents/NativeContentCursor 연결이다. 전체 Fort/Battle snapshot 부모는 별도다.</summary>
public static class NativeFortContentSave
{
    /// <summary>각 중첩 목록의 독립 finder를 만든다. 진단 뒤 head 재조회·반환 전 next 보관은 기존 원문 몸체가 수행한다.</summary>
    private static FortContentSaveCursor Begin(ClientSurfaceGlobals surface, uint parent)
    {
        RawSurfaceStorage storage = surface.Storage;
        if (parent > ushort.MaxValue || parent >= storage.Pool.Capacity)
            throw new ArgumentOutOfRangeException(nameof(parent), "Fort 내용물 finder의 실제 DWORD 부모 범위 부족");
        var state = new ContainedFinderState
        {
            CheckingDead = new(() => storage.Checking().Read()),
            SharedBoss = () => surface.Gameplay.ProductionFinder.ReadBoss(),
            SharedContainedType = storage.ContentType,
            Assertion = storage.AssertDisplay,
        };
        var finder = new RawContainedFinder(storage.Pool, state);
        int current = finder.Begin((int)parent);
        return new() { Current = () => unchecked((uint)current), Next = () => current = finder.Next() };
    }

    /// <summary>선택된 현재 TYPE에서만 실제 세 DWORD를 읽는다. 미선택 TYPE나 별도 자산 사본을 준비하지 않는다.</summary>
    private static FortContentSaveType Type(RawSurfaceStorage storage, uint kind)
    {
        if (kind >= storage.Types.Count)
            throw new ArgumentOutOfRangeException(nameof(kind), "Fort 내용물 저장의 현재 TYPE 범위 부족");
        OriginalTypeRecord type = storage.Types[(int)kind];
        return new(type.Flags1, type.ContentListFlags, type.ContainerListFlags);
    }

    /// <summary>실제 Create/Select가 반환한 stream 생성 수명을 고정해 전체 재귀 저장을 실행한다. 현재 선택 변경과 실패 전 부분 쓰기를 보존한다.</summary>
    public static void Save(ClientSurfaceGlobals surface, CampaignTemplateStorage templates, uint parent, TemplateWriteStream stream)
    {
        ArgumentNullException.ThrowIfNull(templates);
        var virtuals = new NativeFortSaveVirtual(surface);
        RawSurfaceStorage storage = surface.Storage;
        RawFortContentSave.SaveFortContents(parent, 1, new()
        {
            Begin = selected => Begin(surface, selected),
            PoolLimit = () => unchecked((uint)storage.Pool.Capacity),
            Robust = selected => virtuals.ContentRobust(selected) != 0,
            Form = selected =>
            {
                if (selected > ushort.MaxValue || selected >= storage.Pool.Capacity)
                    throw new ArgumentOutOfRangeException(nameof(selected), "Fort 내용물의 실제 DWORD SID 범위 부족");
                return storage.Pool.Slot((int)selected);
            },
            Type = kind => Type(storage, kind),
            ParentType = selected =>
            {
                if (selected > ushort.MaxValue || selected >= storage.Pool.Capacity)
                    throw new ArgumentOutOfRangeException(nameof(selected), "Fort 내용물의 실제 부모 TYPE 범위 부족");
                return Type(storage, storage.Pool.Slot((int)selected)[10]);
            },
            Assertion = reason => storage.AssertDisplay(
                reason == FortContentSaveAssertion.ListFlags ? "(f.s().getListFlags() & lf) == lf" : "pos+length <= alloc",
                reason == FortContentSaveAssertion.ListFlags ? @"\Ns\O\Template.cpp" : @"c:\ns\o\memstream.h",
                reason == FortContentSaveAssertion.ListFlags ? 0x116u : 0x24u),
            // 단일 실제 stream의 불투명 참조1이며 manager.current를 다시 선택하지 않는다.
            Access = (_, operation) => templates.AccessSection(stream, (fields, memory) => operation(fields, memory)),
        });
    }
}
