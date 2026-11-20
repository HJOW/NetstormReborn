using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

// 실제manager/State와Session이헤더를복제하지않는지검증한다.
namespace Netstorm.Client.Tests;

/// <summary>0.741의실제Template공유·수명·초기입력·확장·Notify경계를검증한다.</summary>
public sealed class HostSessionTemplateTests
{
    /// <summary>원문State를가진실제manager1을설치하고원하는mine표식을둔다.</summary>
    private static CampaignTemplateStorage Storage(byte[] state, uint mine = 1)
    {
        var fort = new FortTemplate(); SetSection(fort, "State", state); var storage = new CampaignTemplateStorage(); storage.Install(1, fort, "named");
        BinaryPrimitives.WriteUInt32LittleEndian(storage.Headers.Span.Slice(28, 4), mine); return storage;
    }
    /// <summary>새독립Session이외부의실제같은Template를빌리게한다.</summary>
    private static HostSession Session(CampaignTemplateStorage storage) => new(new(), new(1), new(), sharedTemplates: storage);

    /// <summary>Player의표시비트가managerDWORD대신동일State에쓰이고Notify는mine관문에서파일없이정상복귀한다.</summary>
    [Fact]
    public void StateAndNotifyUseActualSectionWithoutChangingManagerHeaders()
    {
        var storage = Storage(new byte[28]); var session = Session(storage); byte[] headers = storage.Headers.ToArray();
        Assert.Same(storage, session.Templates); Assert.True(SameStorage(session.WorldTemplates, storage.Headers.Span));
        Memory<byte> bytes = session.CurrentPlayerTemplateState(1); bytes.Span[0] |= 4; session.NotifyPlayerTemplateState(1);
        Assert.Equal(4, storage.State(1).Bytes.Span[0]); Assert.True(SameStorage(bytes.Span, storage.State(1).Bytes.Span)); Assert.Equal(headers, storage.Headers.ToArray());
        Assert.Equal(0u, storage.State(1).Position); Assert.Equal(new uint[] { 1 }, session.PlayerTemplateMarks);
        session.ReleasePlayerRecordViews(); storage.ClearAll();
    }

    /// <summary>앞선Session반납뒤다음Session도같은현재헤더/manager/State를초기화하지않고빌린다.</summary>
    [Fact]
    public void SequentialSessionsKeepSameTemplateOwnerAndCurrentState()
    {
        var storage = Storage(new byte[28]); var first = Session(storage); first.CurrentPlayerTemplateState(1).Span[3] = 0x73; byte[] before = storage.Headers.ToArray(); first.ReleasePlayerRecordViews();
        var next = Session(storage); Assert.Same(first.WorldTemplates, next.WorldTemplates); Assert.Equal(before, next.WorldTemplates); Assert.Equal(0x73, next.CurrentPlayerTemplateState(1).Span[3]);
        Assert.Throws<InvalidOperationException>(() => first.CurrentPlayerTemplateState(1)); next.ReleasePlayerRecordViews(); storage.ClearAll();
    }

    /// <summary>빈State는명시미정28BYTE에서정의된필드만초기화하고원래manager의실제섹션을만든다.</summary>
    [Fact]
    public void EmptyStateUsesSuppliedUndefinedBytesAndSameReplacementSection()
    {
        var storage = Storage([]); var session = Session(storage); int calls = 0;
        session.BindTemplateAllocationBytes((slot, bytes) => { Assert.Equal(1u, slot); calls++; bytes.Fill(0xa5); });
        Memory<byte> result = session.CurrentPlayerTemplateState(1); Assert.Equal(1, calls); Assert.Equal(0xa1, result.Span[0]); Assert.Equal(0xa5, result.Span[1]); Assert.Equal(0xa5, result.Span[27]);
        Assert.Equal(0, result.Span[4]); Assert.Equal(28u, storage.State(1).End); Assert.True(SameStorage(result.Span, storage.State(1).Bytes.Span));
        _ = session.CurrentPlayerTemplateState(1); Assert.Equal(1, calls); session.ReleasePlayerRecordViews(); storage.ClearAll();
    }

    /// <summary>미정입력공급실패는빈State의대체를실행하지않고앞선현재cursor0/manager를보존한다.</summary>
    [Fact]
    public void EmptyStateFailureDoesNotPublishFakeZeroSection()
    {
        var storage = Storage([]); var session = Session(storage); byte[] headers = storage.Headers.ToArray();
        session.BindTemplateAllocationBytes((_, _) => throw new IOException("명시입력실패")); Assert.Throws<IOException>(() => session.CurrentPlayerTemplateState(1));
        Assert.Equal(headers, storage.Headers.ToArray()); Assert.Equal(0u, storage.State(1).End); Assert.Empty(storage.State(1).Bytes.ToArray()); Assert.Equal(1, storage.OwnedImages);
        session.ReleasePlayerRecordViews(); storage.ClearAll();
    }

    /// <summary>짧은State의실제섹션확장은prefix를복사하고새끝만0으로쓰며원래manager를유지한다.</summary>
    [Fact]
    public void ShortStateGrowthKeepsExistingBytesAndManager()
    {
        var storage = Storage([0x12, 0x34, 0x56]); var session = Session(storage); byte[] headers = storage.Headers.ToArray();
        Memory<byte> result = session.CurrentPlayerTemplateState(1); Assert.True(result.Length >= 28); Assert.Equal(new byte[] { 0x12, 0x34, 0x56 }, result.Span[..3].ToArray()); Assert.All(result.Span[3..28].ToArray(), value => Assert.Equal(0, value));
        Assert.Equal(headers, storage.Headers.ToArray()); Assert.True(SameStorage(result.Span, storage.State(1).Bytes.Span)); session.ReleasePlayerRecordViews(); storage.ClearAll();
    }

    /// <summary>mine0의실제저장은미연결실패를숨기지않고실제저장부모정상복귀뒤에만변경기록을남긴다.</summary>
    [Fact]
    public void NonMineNotifyRequiresSelectedSaveAndRecordsOnlyNormalReturn()
    {
        var storage = Storage(new byte[28], 0); var session = Session(storage);
        Assert.Throws<InvalidOperationException>(() => session.NotifyPlayerTemplateState(1)); Assert.Empty(session.PlayerTemplateMarks);
        var saves = new List<uint>(); session.BindTemplateSave(saves.Add); session.NotifyPlayerTemplateState(1);
        Assert.Equal(new uint[] { 1 }, saves); Assert.Equal(saves, session.PlayerTemplateMarks); session.ReleasePlayerRecordViews(); storage.ClearAll();
    }
}
