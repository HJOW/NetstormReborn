using System.Buffers.Binary;
using Netstorm.Core.Original;

// 실제 내용물/감시/삭제 본체가 같은 공장·종속·환불·SP 추적을 사용하도록 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession의2d/2e/65 고정 수신과 삭제 뒤 공통 SP 감사다.</summary>
public sealed partial class HostSession
{
    /// <summary>네 실제 내장 Form 가상30은1을 반환하며 다른 자산은 같은 생성자의 실제 플래그를 읽는다.</summary>
    private uint SquidMutationFirstPop(int sid)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)); var forms = new RawSquidForms(Pool);
        // Source의 실제 네 Form 가상표만 선택하고 TYPE 번호로 가상 클래스를 추측하지 않는다.
        foreach (uint type in new uint[] { 1, 2, 3, 6 }) if (table == forms.Vtable(type)) return 1;
        if (!RawSquidFactory.SupportsConstructedVtable(Pool.Edition, table)) throw new InvalidOperationException("내용물 수신의 현재 가상30 미연결");
        return FixedPacketNeed(_fortFactory).FirstPopFlags(sid);
    }
    /// <summary>실제 ContentForm 정렬/통지 또는 나머지 세 Form/자산의 원문 가상50을 선택한다.</summary>
    private void AttachSquidMutation(int sid, int parent, uint flags)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)); var forms = new RawSquidForms(Pool);
        if (table == forms.Vtable(6)) { FixedPacketNeed(_worldContentForms).AttachCurrent(sid, parent, flags); return; }
        // 세 공통 Form은 같은 Factory의 현재 가상 부착 장치를 빌린다.
        foreach (uint type in new uint[] { 1, 2, 3 }) if (table == forms.Vtable(type)) { FixedPacketNeed(_fortFactory).AttachForm(sid, parent, flags, TransmitSquidMutationForm); return; }
        if (!RawSquidFactory.SupportsConstructedVtable(Pool.Edition, table)) throw new InvalidOperationException("내용물 수신의 현재 가상50 미연결");
        AttachSquidPacketBaseAsset(sid, parent, flags, FixedPacketNeed(_squidPacketStorage));
    }
    /// <summary>감시 가상74의 실제4af160→4ac800 점프만 같은 공장 부착으로 전달한다.</summary>
    private void AttachSquidMutationDependency(int sid, int parent, uint flags)
    {
        if (BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)) != new RawSquidForms(Pool).Vtable(1)) throw new InvalidOperationException("감시 수신의 현재 가상74 미연결");
        FixedPacketNeed(_fortFactory).AttachForm(sid, parent, flags, TransmitSquidMutationForm);
    }
    /// <summary>실제 공통 Form Pop이 선택한 전송을 같은 월드 writer/버퍼에 전달한다.</summary>
    private void TransmitSquidMutationForm(int sid, uint flags)
    {
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
        if (storage.UsesNativeTransmissionDevices()) storage.TransmitPop(sid, flags); else FixedPacketNeed(_worldTransmission).Pop(sid, flags);
    }
    /// <summary>첫 표본을 보관하고 조건부 실제 SP 조회 뒤에만 새 표본을 읽는33170 꼬리다.</summary>
    private void AuditSquidMutationSp()
    {
        PriestRegenPatchState patch = PriestState.Regen.Patch; UIntBox diagnostic = Gameplay.PatchDiagnostic;
        uint expected = unchecked((uint)Sp.TrackingOffsetCurrent.Value), actual = Sp.OffsetCurrent, fallback = patch.ReadSample();
        if (actual != expected) diagnostic.Value = fallback;
        if (patch.Remaining != 0)
        {
            if (MissionPlayer.PlayerKnown != 0)
            {
                float money = BitConverter.UInt32BitsToSingle(Sp.Get(MissionPlayer.BattleOwner)); fallback = patch.ReadSample(); if (money > 7000f) diagnostic.Value = fallback;
            }
            patch.Remaining--;
        }
        if (Sp.FirstPoolCurrent is null) diagnostic.Value = fallback;
        if (Sp.Permutation[0] == -1) diagnostic.Value = fallback;
    }
    /// <summary>고정 entry만 기존 세 본체를 선택하며 호출 전 모든 장치를 미리 요구하지 않는다.</summary>
    private void ReceiveSquidMutationPacket(uint entry, ReadOnlyMemory<byte> packet)
    {
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage); HostSquidPacketEffects effects = FixedPacketNeed(_squidPacketEffects);
        var hooks = new SquidMutationHooks
        {
            ContentType = () => Gameplay.ContentFormTypeBox.Value, DependType = () => GameAux.DependFormTypeBox.Value, Boss = () => PlayerProcessClock.Boss,
            Take = (type, sid) => FixedPacketNeed(_fortFactory).Take(type, sid), Word = (sid, word) => RawSquidWord.Set(Pool, sid, word, () => BridgeState.BridgeType, selected => FixedPacketNeed(_fortBuilding).NotifySurface(selected)),
            FirstPop = SquidMutationFirstPop, Attach = AttachSquidMutation, DependencyAttach = AttachSquidMutationDependency,
            Refund = (sid, owner) => { _ = Reward.Refund(sid, owner, true); }, Destroy = (sid, flags) => FixedPacketNeed(effects.VirtualDestroy)(sid, flags),
            TypeName = SquidPacketTypeName, Log = effects.Log, AuditSp = AuditSquidMutationSp, Assertion = storage.AssertDisplay,
        };
        if (entry == 0x433170) RawSquidMutationReceive.ReceiveDeleted(Pool, packet, hooks);
        else if (entry == 0x432850) RawSquidMutationReceive.ReceiveDependency(Pool, packet, hooks);
        else RawSquidMutationReceive.ReceiveContent(Pool, packet, hooks);
    }
}
