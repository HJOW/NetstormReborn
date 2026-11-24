using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x22 메타 참가자 이동의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaParticipantMove.cpp 의 ExplodeNativeMetaBattleSlot/ReceiveNativeMetaParticipantMove 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x22 등록을 이식한다.
// 0.962에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x22 슬롯 효과의 실제 객체·SHP·지면 효과/음향 장치다.
public sealed class ClientMetaBattleSlotHooks
{
    /// <summary>현재 Pool의 같은 50BYTE 물리다.</summary>
    public Func<uint, ReadOnlyMemory<byte>>? Raw { get; set; }

    /// <summary>실제 객체 TYPE의 첫 프레임을 읽는다.</summary>
    public Func<uint, uint>? FirstFrame { get; set; }

    /// <summary>실제 객체 TYPE의 포구를 읽는다.</summary>
    public Func<uint, int, float[]>? Muzzle { get; set; }

    /// <summary>기존 지면 효과 장치다.</summary>
    public Action<float, float>? Ground { get; set; }

    /// <summary>같은 좌표의 효과음 장치다.</summary>
    public Action<float, float>? Sound { get; set; }

    /// <summary>복귀형 진단 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// 0x22 메타 참가자 이동이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaParticipantMoveHooks
{
    /// <summary>같은 실제 CS 참가자40 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>같은 실제 메타맵 모드 owner다. 거짓이면 수신 전체를 건너뛴다.</summary>
    public Func<bool>? OwnerModeChallenge { get; set; }

    /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }

    /// <summary>같은 실제 known 값을 읽는다.</summary>
    public Func<uint>? Known { get; set; }

    /// <summary>현재 local owner를 읽는다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>원문 Ch.cpp의 현재 battle/좌석 진단 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>진입 옛 전투/현재 슬롯 효과 장치다.</summary>
    public Action<uint, uint>? Kick { get; set; }

    /// <summary>캡처 Battle+4 식별자의 실제 특정 표시 갱신 장치다.</summary>
    public Action<uint>? Battle { get; set; }

    /// <summary>쫓겨남 실제 음향 장치다.</summary>
    public Action? KickedSound { get; set; }

    /// <summary>떨굼 실제 음향 장치다.</summary>
    public Action? DropSound { get; set; }

    /// <summary>실제 음악 요청 장치다.</summary>
    public Action? Music { get; set; }

    /// <summary>같은 참가자의 당시 위치 음향 장치다.</summary>
    public Action<float, float>? Thunder { get; set; }

    /// <summary>캡처 참가자와 현재 전투의 포구/좌표 계산 장치다.</summary>
    public Func<Memory<byte>, uint, uint, float[]>? Seat { get; set; }

    /// <summary>현재 좌표를 쓴 같은 참가자의 실제 Rig 장치다.</summary>
    public Action<Memory<byte>>? Animate { get; set; }

    /// <summary>같은 전투 87BYTE 기본값 장치다.</summary>
    public Action<Memory<byte>>? ResetOptions { get; set; }

    /// <summary>현재 전투를 다시 읽어 같은 87BYTE를 송신하는 장치다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? SendOptions { get; set; }

    /// <summary>실제 Chat 모드 장치다.</summary>
    public Action<uint>? ChatMode { get; set; }

    /// <summary>실제 Chat 일원 갱신 장치다.</summary>
    public Action? ChatMembers { get; set; }

    /// <summary>조건부 NSInfo 장치다.</summary>
    public Action? RefreshInfo { get; set; }

    /// <summary>같은 참가자 표시 장치다.</summary>
    public Action<uint, uint>? Update { get; set; }

    /// <summary>같은 전투 장부 갱신 장치다.</summary>
    public Action? RefreshCore { get; set; }

    /// <summary>마지막 현재 CS 이름표 서식 장치다.</summary>
    public Action<uint>? RebuildName { get; set; }
}

// 22h 참가자 이동의 캡처 CS/현재 packet 쓰기와 실제 좌석 효과·항해·master/Chat 후행 순서다.
public static class ClientMetaParticipantMove
{
    /// <summary>원문 0x22 메타 참가자 이동 종류다. 0.962에서 추가됐다.</summary>
    public const uint Type = 0x22;

    /// <summary>원문 0x22 수신의 실제 고정 F 주소다. 0.962에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4820;

    /// <summary>원문 0x22 수신의 실제 고정 길이다. 0.962에서 추가됐다.</summary>
    public const ushort FixedLength = 17;

    /// <summary>00540b68의 읽기 전용 8자리 순서다. 원문 writer는 없다. 0.962에서 추가됐다.</summary>
    private static readonly int[] SeatOrder = [4, 0, 6, 2, 7, 3, 5, 1];

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.962에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 참가자 이동의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.962에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 참가자 이동의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.962에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 이동의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>이웃 플래그와 미작성 필드를 보존한다. 0.962에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 이동의 실제 쓰기 범위");
        }

        // 도달한 폭만 기록하고 선행 효과를 되돌리지 않는다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>원문 MOVSX WORD→DWORD 플래그를 보존한다. 0.962에서 추가됐다.</summary>
    private static uint Flags(ReadOnlySpan<byte> packet) => unchecked((uint)(int)(short)Read(packet, 5, 2));

    /// <summary>원문 8개 좌석 playerId 배열 밖 쓰기를 호스트 경계에서 거부한다. 0.962에서 추가됐다.</summary>
    private static int Slot(uint slot)
    {
        if (slot >= 8)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), "메타 참가자 이동의 실제 좌석8칸 물리 범위");
        }

        return 0x38 + unchecked((int)slot) * 4;
    }

    /// <summary>479cf0/478720 전체: 현재 전투 유효 진단→객체 TYPE/SHP 포구→snapshot 위치 보정→같은 지면/음향이다. 0.962에서 추가됐다.</summary>
    public static void Explode(ClientChallengeGlobals challenge, int battle, int slot, RawCameraViewState snapshot, ClientMetaBattleSlotHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(snapshot);
        ArgumentNullException.ThrowIfNull(hooks);
        Memory<byte> current = challenge.Battle(unchecked((uint)battle));
        if ((Read(current.Span, 0, 1) & 1u) == 0)
        {
            NeedDelegate(hooks.Assertion)("c->isValid()", ".\\Metadisplay.cpp", 0x737);
            if ((Read(current.Span, 0, 1) & 1u) == 0)
            {
                return;
            }
        }

        if (slot < 0 || slot > 7)
        {
            NeedDelegate(hooks.Assertion)("slot >= 0 && slot < MAX_PLAYER_PER_BATTLE", ".\\Metadisplay.cpp", 0x1a5);
        }

        if (slot < 0 || slot > 7)
        {
            throw new ArgumentOutOfRangeException(nameof(slot), "메타 슬롯 효과의 실제8자리 포구 표 범위");
        }

        ReadOnlyMemory<byte> raw = NeedDelegate(hooks.Raw)(Read(current.Span, 0x20));
        uint type = Read(raw.Span, 10, 1);
        int frame = unchecked((int)(NeedDelegate(hooks.FirstFrame)(type) + unchecked((uint)SeatOrder[slot])));
        float[] muzzle = NeedDelegate(hooks.Muzzle)(type, frame);
        if (muzzle.Length < 2)
        {
            throw new ArgumentOutOfRangeException(nameof(hooks), "메타 슬롯 효과의 실제 포구 좌표 부족");
        }

        float x = (float)((double)BitConverter.UInt32BitsToSingle(Read(raw.Span, 0xe)) + (double)muzzle[0] / BitConverter.UInt32BitsToSingle(snapshot.BaseCellFloatBits[0]));
        float y = (float)((double)BitConverter.UInt32BitsToSingle(Read(raw.Span, 0x12)) + (double)muzzle[1] / BitConverter.UInt32BitsToSingle(snapshot.BaseCellFloatBits[1]));
        NeedDelegate(hooks.Ground)(x, y);
        NeedDelegate(hooks.Sound)(x, y);
    }

    /// <summary>42c2e0 전체이며 packet5 bit8을 원문 중간 위치에서 직접 지운다. 0.962에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·모드·소유자·진단·효과·좌표·Chat·표시 장치다.</param>
    /// <param name="packet">원문 mutable 0x22 패킷이다.</param>
    /// <param name="subscriber">현재 자기 구독자 번호다.</param>
    /// <param name="known">현재 known 값이다.</param>
    public static void Receive(ClientMetaParticipantMoveHooks hooks, Memory<byte> packet, uint subscriber, uint known)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        if (!NeedDelegate(hooks.OwnerModeChallenge)())
        {
            return;
        }

        if (challenge.Current == 0)
        {
            NeedDelegate(hooks.Assertion)("CS", ".\\State.cpp", 0xd3c);
        }

        Span<byte> bytes = packet.Span;
        Memory<byte> cs = challenge.Bytes;
        uint id = Read(bytes, 4, 1);
        if (id >= RawChallengeState.ParticipantCount)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 이동의 실제 참가자40 범위");
        }

        Memory<byte> record = cs.Slice(RawChallengeState.ParticipantOffset + unchecked((int)id) * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
        uint owner = NeedDelegate(hooks.Owner)();
        if (owner >= RawChallengeState.ParticipantCount)
        {
            throw new ArgumentOutOfRangeException(nameof(hooks), "메타 참가자 이동의 실제 소유자40 범위");
        }

        uint newBattle = Read(bytes, 0xf, 1);
        Memory<byte> own = cs.Slice(RawChallengeState.ParticipantOffset + unchecked((int)owner) * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
        uint ownBattle = Read(own.Span, 0xa4);
        uint oldBattle = Read(record.Span, 0xa4);
        bool changed = oldBattle != newBattle;
        bool refresh = (Read(bytes, 4, 1) == owner && changed) || (ownBattle != 0 && (ownBattle == oldBattle || ownBattle == newBattle));
        if (changed || Read(record.Span, 0xa8) != Read(bytes, 0x10, 1))
        {
            if ((Read(bytes, 5, 1) & 8u) != 0)
            {
                NeedDelegate(hooks.Kick)(oldBattle, Read(record.Span, 0xa8));
                if (Read(record.Span, 4) == subscriber)
                {
                    NeedDelegate(hooks.KickedSound)();
                }

                if (Read(record.Span, 0xa4) == 0)
                {
                    NeedDelegate(hooks.Assertion)("p->battle != 0", ".\\Ns\\O\\Ch.cpp", 0x5fb);
                }
            }

            uint from = Read(record.Span, 0xa4);
            if (from != 0)
            {
                if (from >= RawChallengeState.BattleCount)
                {
                    throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 이동의 실제 이전 전투10 범위");
                }

                Memory<byte> battle = cs.Slice(RawChallengeState.BattleOffset + unchecked((int)from) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
                Put(battle.Span, Slot(Read(record.Span, 0xa8)), 0);
                NeedDelegate(hooks.Battle)(Read(battle.Span, 4));
                ClientChallengeGlobals.UpdateBattleVersionBounds(cs, unchecked((int)Read(record.Span, 0xa4)), hooks.Assertion);
                Put(record.Span, 0x64, Read(record.Span, 0x64) & 0xfffffffbu);
            }

            uint to = Read(bytes, 0xf, 1);
            if (to != 0)
            {
                if (to >= RawChallengeState.BattleCount)
                {
                    throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 이동의 실제 다음 전투10 범위");
                }

                uint participant = Read(bytes, 4, 1);
                uint slot = Read(bytes, 0x10, 1);
                Memory<byte> battle = cs.Slice(RawChallengeState.BattleOffset + unchecked((int)to) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
                int at = Slot(slot);
                if (Read(battle.Span, at) != 0)
                {
                    NeedDelegate(hooks.Assertion)("!playerIds[slot]", ".\\Ns\\O\\Ch.cpp", 0x1c8);
                }

                Put(battle.Span, at, participant);
                NeedDelegate(hooks.Battle)(Read(battle.Span, 4));
                ClientChallengeGlobals.UpdateBattleVersionBounds(cs, unchecked((int)Read(bytes, 0xf, 1)), hooks.Assertion);
                uint y = Read(record.Span, 0xa0);
                uint x = Read(record.Span, 0x9c);
                NeedDelegate(hooks.Thunder)(BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y));
                if (known != 0 && Read(bytes, 4, 1) == NeedDelegate(hooks.Owner)())
                {
                    NeedDelegate(hooks.Music)();
                }
            }
        }

        Put(bytes, 5, Read(bytes, 5, 1) & 0xf7u, 1);
        uint difference = Flags(bytes) ^ Read(record.Span, 0x64);
        if ((difference & 2u) != 0 && Read(bytes, 0xf, 1) != 0 && ownBattle == Read(bytes, 0xf, 1))
        {
            NeedDelegate(hooks.DropSound)();
        }

        Put(record.Span, 0x64, Flags(bytes));
        uint moved = Read(bytes, 0xf, 1);
        uint[] position = [Read(bytes, 7), Read(bytes, 0xb)];
        Put(record.Span, 0xa4, moved);
        Put(record.Span, 0xa8, Read(bytes, 0x10, 1));
        if (moved != 0)
        {
            float[] point = NeedDelegate(hooks.Seat)(record, moved, Read(record.Span, 0xa8));
            if (point.Length < 2)
            {
                throw new ArgumentOutOfRangeException(nameof(hooks), "메타 참가자 이동의 실제 좌표 부족");
            }

            position = [BitConverter.SingleToUInt32Bits(point[0]), BitConverter.SingleToUInt32Bits(point[1])];
        }

        if (BitConverter.UInt32BitsToSingle(Read(record.Span, 0x9c)) != BitConverter.UInt32BitsToSingle(position[0]) || BitConverter.UInt32BitsToSingle(Read(record.Span, 0xa0)) != BitConverter.UInt32BitsToSingle(position[1]))
        {
            Put(record.Span, 0x9c, position[0]);
            Put(record.Span, 0xa0, position[1]);
            NeedDelegate(hooks.Animate)(record);
        }

        // Rig callback 뒤에도 같은 지역 좌표를 다시 쓴다.
        Put(record.Span, 0x9c, position[0]);
        Put(record.Span, 0xa0, position[1]);
        if ((difference & 4u) != 0 && (Read(bytes, 5, 1) & 4u) != 0 && Read(bytes, 0xf, 1) != 0 && Read(bytes, 4, 1) == owner)
        {
            Memory<byte> options = cs.Slice(RawChallengeState.BattleOffset + unchecked((int)Read(bytes, 0xf, 1)) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes).Slice(0x59, 87);
            NeedDelegate(hooks.ResetOptions)(options);
            uint current = Read(bytes, 0xf, 1);
            NeedDelegate(hooks.SendOptions)(current, cs.Slice(RawChallengeState.BattleOffset + unchecked((int)current) * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes).Slice(0x59, 87));
        }

        if (changed)
        {
            if (Read(bytes, 4, 1) == owner)
            {
                NeedDelegate(hooks.ChatMode)(0);
                if (Read(record.Span, 0xa4) != 0)
                {
                    NeedDelegate(hooks.ChatMode)(1);
                }
            }
            else if (Read(own.Span, 0xa4) != 0)
            {
                NeedDelegate(hooks.ChatMembers)();
            }
        }

        if (refresh)
        {
            NeedDelegate(hooks.RefreshInfo)();
        }

        NeedDelegate(hooks.Update)(Read(bytes, 4, 1), 0xffu);
        NeedDelegate(hooks.RefreshCore)();
        NeedDelegate(hooks.RebuildName)(Read(bytes, 4, 1));
    }

    /// <summary>0x22의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.962에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·모드·소유자·진단·효과·좌표·Chat·표시 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaParticipantMoveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        // 같은 mutable frame을 실제42c2e0에 전달한다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) =>
        {
            byte[] copy = packet.ToArray();
            Receive(hooks, copy, Need(hooks.Subscriber).Value, NeedDelegate(hooks.Known)());
        });
    }
}
