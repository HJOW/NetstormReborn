using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 가상표의 내용물 통지와 공통 Form 해제 연결을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj/tests/CampaignNoticeTests.cpp와 FormTakeTests.cpp의 빈 몸체·파생 통지 계약을 옮긴다.</summary>
public sealed class RawContentParentNoticeTests
{
    /// <summary>원본 빈 몸체는 비정상 TYPE·상태와 범위 밖 내용물 번호도 읽거나 수정하지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x501dd0u, 0x41c100u)]
    [InlineData(OriginalEdition.Cd1072, 0x5058d8u, 0x401c20u)]
    public void EmptyParentBodiesPreserveRawBytes(OriginalEdition edition, uint table, uint entry)
    {
        var pool = new RawSidPool(32768, edition: edition);
        int sid = pool.Allocate();
        pool.RawSlot(sid).Fill(0x9b);
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), table);
        byte[] before = pool.Bytes.ToArray();
        var notice = new RawContentParentNotice(pool, (_, _, _) => throw new InvalidOperationException("빈 몸체가 파생 통지를 선택함"));
        Assert.Equal(entry, notice.NativeEntry(sid));
        notice.Notice(sid, 0, 1); notice.Notice(sid, 65535, 2);
        Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>워크샾은 현재 파생 소비자를 요구하며 알 수 없는 가상표는 앞선 성공 경로를 재사용하지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x509148u, 0x453b50u)]
    [InlineData(OriginalEdition.Cd1072, 0x503298u, 0x47a6c0u)]
    public void DerivedAndUnknownVtablesAreExplicit(OriginalEdition edition, uint table, uint entry)
    {
        var pool = new RawSidPool(32768, edition: edition);
        int sid = pool.Allocate();
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), table);
        var missing = new RawContentParentNotice(pool);
        Assert.Equal(entry, missing.NativeEntry(sid));
        Assert.Throws<InvalidOperationException>(() => missing.Notice(sid, 20000, 2));
        int calls = 0;
        var notice = new RawContentParentNotice(pool, (parent, form, reason) =>
        {
            Assert.Equal(sid, parent); Assert.Equal(20000, form); Assert.Equal(2u, reason); calls++;
        });
        notice.Notice(sid, 20000, 2);
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid), 0xdeadbeef);
        Assert.Throws<InvalidOperationException>(() => notice.Notice(sid, 0, 1));
        Assert.Equal(1, calls);
    }

    /// <summary>네 내장 Form 가상표는 내용물 통지 장치 없이도 원본 빈 몸체로 돌아간다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void EmbeddedFormsUseEmptyNotice(OriginalEdition edition)
    {
        var pool = new RawSidPool(32768, edition: edition);
        var forms = new RawSquidForms(pool);
        // 타입마다 실제 생성자가 기록한 현재 가상표로 분배한다.
        foreach (uint type in new uint[] { 1, 2, 3, 6 })
        {
            int parent = forms.Create(type, SidAllocation.Client);
            new RawContentParentNotice(pool).Notice(parent, 65535, uint.MaxValue);
        }
    }

    /// <summary>공통 Unpop의 내용물 분리가 기본 부모에 도달하면 별도 파생 callback 없이 목록을 해제한다.</summary>
    [Fact]
    public void WorldUnpopUsesActualEmptyParentNotice()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97, storage, frames);
        RawSidPool pool = world.Storage().Pool;
        int parent = pool.Allocate(SidAllocation.Client);
        pool.RawSlot(parent)[11] = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(parent), 0x501dd0);
        var forms = new RawSquidForms(pool);
        int form = forms.Create(6, SidAllocation.Client);
        pool.RawSlot(form)[11] = 8;
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(form)[16..], (ushort)parent);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent)[6..], (ushort)form);
        world.Core().Factory.UnpopForm(form);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(parent)[6..]));
        Assert.Equal(4, pool.Slot(form)[11] & 12);
    }
}
