using System.Buffers.Binary;

// 같은 gun raw 이미지의 통지·발사와 선택된 상태 패킷만 원문 순서대로 처리한다.
namespace Netstorm.Core.Original;

/// <summary>RawGunProcess.cpp의 통지/발사/수신 전후/상태 codec 본체다.</summary>
public sealed partial class RawGunProcess
{
    /// <summary>선택된 원문 진단은 복귀를 허용하며 이후 분기는 현재 필드를 다시 읽는다.</summary>
    private void AssertOriginal(string expression, uint line) => Need(_hooks.Assertion)(expression, @"\Ns\O\Gunprocess.cpp", line);

    /// <summary>애니메이션 종료와 감시 표적 삭제 메시지를 현재 단계/권한에 따라 처리한다.</summary>
    public void Notify(GunProcessFields fields, uint ev, uint info, uint value)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (ev == 1)
        {
            if (value == 4 || (value == 7 && fields.Stage() == 6))
            {
                if (patch && _state.Checking.IsEnabled && Need(_hooks.AnimationForm)(fields.Parent()) == 0)
                    AssertOriginal("getAnimFormSid() != 0", value == 4 ? 0x154u : 0x160u);
                SetStage(fields, value); return;
            }
            if (value != 7 && value != 10)
            {
                if (!patch) throw new InvalidOperationException("GunProcess CD animation notify 원본 assert");
                AssertOriginal("false", 0x16a); return;
            }
        }
        else if (_state.IsServer() && ev == 2)
        {
            if (patch && _state.Checking.IsEnabled && value != fields.Target()) AssertOriginal("info2 == getTargetSid()", 0x16e);
            if (value == fields.Target())
            {
                ClearTarget(fields, true);
                if ((Parent(fields)[11] & 2) == 0) Need(_hooks.RepopForm)(fields);
            }
        }
    }

    /// <summary>실제 shot 생성의 수락 여부로 다음 단계를 쓰고 효과 뒤 현재 시각/간격을 읽는다.</summary>
    public void Fire(GunProcessFields fields)
    {
        _ = Parent(fields);
        Func<ushort, bool> blocked = Need(_hooks.ParentBlocked); Func<ushort, uint[]> context = Need(_hooks.ShotContext);
        Func<uint[], ushort, bool> shoot = Need(_hooks.Shoot); Func<ushort, double> interval = Need(_hooks.FiringInterval);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch && _state.Checking.IsEnabled) Need(_hooks.CheckForms)(fields.Parent());
        if (blocked(fields.Parent()) || _state.FixedMode) return;
        uint[] descriptor = context(fields.Parent()); ushort target = fields.Target();
        bool valid = target != 0 && target < _pool.Capacity && (_pool.Slot(target)[11] & 3) == 0;
        if (valid && shoot(descriptor, target)) SetStage(fields, 2); else ClearTarget(fields, true);
        double delay = interval(fields.Parent()); SetTime(fields, _state.Now + delay);
        if (patch && _state.Checking.IsEnabled) Need(_hooks.CheckForms)(fields.Parent());
    }

    /// <summary>상태 수신 전 현재 대기 단계3/6일 때만 stop helper의 현재 애니메이션 두 번 조회를 실행한다.</summary>
    public void ReceiveUpdate(GunProcessFields fields, byte dirty, uint beforeRead)
    {
        if (beforeRead == 0 || (dirty & 8) == 0 || (fields.Stage() != 3 && fields.Stage() != 6)) return;
        Func<ushort, ushort> animation = Need(_hooks.AnimationForm); Action<ushort, uint> kill = Need(_hooks.KillAnimation);
        if (animation(fields.Parent()) != 0) kill(animation(fields.Parent()), 0x40);
    }

    /// <summary>PATCH 수신 생성자의 실제 VT/표적/시각/단계만 쓰고 group과 모든 padding을 보존한다.</summary>
    public void InitializeReceived(GunProcessFields fields)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Gun 수신 생성자의 PATCH 계약");
        Put(fields.Bytes, 0, PatchOuter); Put(fields.Bytes, 40, PatchInner); fields.Bytes[8] = 0xff;
        Put(fields.Bytes, 44, 0); Put(fields.Bytes, 48, 0); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(24), 0);
        PutTime(fields, _state.Now); Put(fields.Bytes, 28, 0);
    }

    /// <summary>실제 dirty를 한 번 잡고 각 필드 소비 시 폭을 검사한다. 앞선 observer/필드 변경은 보존한다.</summary>
    public void ReadData(GunProcessFields fields, ReadOnlyMemory<byte> packet, out uint parent)
    {
        parent = 0;
        if (packet.Length < 9) throw new ArgumentException("Gun 수신9BYTE 머리 부족");
        byte dirty = packet.Span[8]; int cursor = 9; ReceiveUpdate(fields, dirty, 1);
        if ((dirty & 1) != 0) { Require(packet.Length, cursor, 4); parent = Read(packet.Span, cursor); cursor += 4; }
        // 원문 선택된 시각8/표적2/단계4/group4의 비트를 같은 순서와 폭으로 복사한다.
        foreach ((byte bit, int offset, int width) in DataFields)
        {
            if ((dirty & bit) == 0) continue;
            Require(packet.Length, cursor, width); packet.Span.Slice(cursor, width).CopyTo(fields.Bytes.AsSpan(offset)); cursor += width;
        }
        ReceiveUpdate(fields, dirty, 0);
    }

    /// <summary>원문 dirty 선택과 raw 필드의 고정 폭이다.</summary>
    private static readonly (byte Bit, int Offset, int Width)[] DataFields = [(2, 16, 8), (4, 24, 2), (8, 28, 4), (16, 32, 4)];
    /// <summary>범위 실패 직전까지 완료한 효과를 유지하며 현재 필드만 검사한다.</summary>
    private static void Require(int size, int cursor, int width)
    {
        if (cursor > size || width > size - cursor) throw new ArgumentException("Gun 선택된 상태 필드 부족");
    }

    /// <summary>충분한 버퍼를 먼저 확인한 뒤 현재 dirty/raw 선택·머리·길이를 쓰고 dirty를 소비한다.</summary>
    public static ushort WriteData(GunProcessFields fields, Span<byte> packet, uint form, uint parent, bool force, byte protocol = 1, byte message = 0x3a)
    {
        byte dirty = force ? (byte)0xff : fields.Dirty(); int length = 9 + (parent != 0 ? 4 : 0);
        // force를 저장하기 전 실제 필요한 모든 필드의 크기를 합한다.
        foreach ((byte bit, int _, int width) in DataFields) if ((dirty & bit) != 0) length += width;
        if (packet.Length < length) throw new ArgumentException("Gun 패킷 버퍼 크기 오류");
        if (force) fields.Bytes[8] = 0xff;
        packet[0] = protocol; packet[1] = message; Put(packet, 4, form); int cursor = 9;
        if (parent != 0) { fields.Bytes[8] |= 1; Put(packet, cursor, parent); cursor += 4; }
        // 현재 dirty와 고정 raw 폭을 소비하며 미작성 packet tail은 건드리지 않는다.
        foreach ((byte bit, int offset, int width) in DataFields)
        {
            if ((fields.Dirty() & bit) == 0) continue;
            fields.Bytes.AsSpan(offset, width).CopyTo(packet[cursor..]); cursor += width;
        }
        BinaryPrimitives.WriteUInt16LittleEndian(packet[2..], (ushort)cursor); packet[8] = fields.Dirty(); fields.Bytes[8] = 0;
        return (ushort)cursor;
    }

    /// <summary>Form/부모는 inner의 전체 DWORD이며 includeParent/force가 하나라도 참이면 부모를 쓴다.</summary>
    public static ushort WriteProcess(GunProcessFields fields, Span<byte> packet, bool includeParent, bool force, byte protocol = 1, byte message = 0x3a) =>
        WriteData(fields, packet, Read(fields.Bytes, 44), includeParent || force ? Read(fields.Bytes, 48) : 0, force, protocol, message);
}
