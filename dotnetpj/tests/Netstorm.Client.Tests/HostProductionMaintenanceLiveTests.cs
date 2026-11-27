using System.Buffers.Binary;
using System.Reflection;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 검사 장면에서 같은 실제 세션의 생산 유지 장치를 관측한다.
namespace Netstorm.Client.Tests;

/// <summary>생산 유지가 현재 Player 기록과 Deck·SP를 읽는지 검증한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>부분 적재 관문과 검사 전용 완전 적재 표식에서 실제 현재 값을 구분한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductionMaintenanceReadsCurrentActualSessionOwners()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext();
        HostSession session = scene.Session;
        int parent = session.BuildPlayerWorldHooks().Create!(152, 2);
        session.Pool.AllocatedBytes(parent)[34] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x9c), (uint)parent);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x7c), 19);
        BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0xb0), 1);
        session.MissionPlayer.TemplateOwner = 1;
        session.OwnerMode.Fort = true;
        session.BuildProductionWorldDevices(ProductionDevices());
        session.Sp.Set(1, BitConverter.SingleToUInt32Bits(7000.5f));
        var globals = new ProductionMaintenanceGlobals();
        var panel = new ProductionMaintenancePanel { Slots = 0 };
        var refreshed = new List<uint>();
        var maintenance = session.MakeProductionMaintenance(globals, panel, () => 100, refreshed.Add);
        maintenance.Run();
        Assert.Empty(globals.StockUntil);
        Assert.Equal(0, panel.OldMoney);

        // 실제 제품의 부분 적재는 그대로 두고 이 검사 장면에서만 이후 관문을 연다.
        FieldInfo loaded = typeof(HostSession).GetField("_nativeLoaded", BindingFlags.NonPublic | BindingFlags.Instance)!;
        loaded.SetValue(session, true);
        try
        {
            maintenance.Run();
            Assert.Equal(scene.Types.Count, globals.StockUntil.Count);
            Assert.Equal(19u, panel.Temple);
            Assert.Equal(7000, panel.OldMoney);
            Assert.NotEqual(0u, BinaryPrimitives.ReadUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x68)));
            BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0x7c), 20);
            session.Sp.Set(1, BitConverter.SingleToUInt32Bits(33.5f));
            maintenance.Run();
            Assert.Equal(20u, panel.Temple);
            Assert.Equal(33, panel.OldMoney);
            // 실제 AI 관문을 끄면 같은 난수와 내용물 생성기가 다리 조각을 공급한다.
            BinaryPrimitives.WriteUInt32LittleEndian(session.PlayerRecords.AsSpan(180 + 0xb0), 0);
            panel.Slots = 1;
            uint randomBefore = session.Game.State();
            maintenance.Run();
            Assert.NotEqual(randomBefore, session.Game.State());
            Assert.Equal(1u, globals.Sequence);
            var finder = new RawContainedFinder(session.Pool, session.ProductionFinder);
            int content = finder.Begin(parent);
            Assert.NotEqual(0, content);
            Assert.Equal(82, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(content)[18..]));
            Assert.NotEqual(0, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(content)[20..]) & 0x40);
            Assert.Contains(2u, refreshed);
        }
        finally { loaded.SetValue(session, false); }
    }
}
