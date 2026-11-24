using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x23 메타 전투 생성의 같은 원문 observer·버전·길이와 고정 F를 같은 분배기에 연결한다.
// cpppj/src/client/NativeMetaBattleCreate.cpp 의 ConstructNativeMetaBattle/ReceiveNativeMetaBattleAdd 와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientMetaPacketReceivers 중 0x23 등록을 이식한다.
// 0.958에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x23 메타 전투 생성의 호출 스택 인자다. 식별자 6유효 BYTE와 x/y 비트를 값으로 유지한다.
public readonly record struct ClientMetaBattleCreateData(
    /// <summary>생성 전투 번호이며 현재 기록+4에 저장한다.</summary>
    uint Id,
    /// <summary>호출 스택 두 DWORD에서 온 식별자의 유효 6BYTE다.</summary>
    byte[] Identity,
    /// <summary>원문 x float32 비트다.</summary>
    uint PositionX,
    /// <summary>원문 y float32 비트다.</summary>
    uint PositionY,
    /// <summary>마지막 스택 DWORD다. 0일 때만 새 전투 Rig 애니메이션을 요청한다.</summary>
    uint Existing);

// 0x23 메타 전투 생성이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaBattleCreateHooks
{
    /// <summary>같은 실제 CS 전투10 기록 owner다.</summary>
    public ClientChallengeGlobals? Challenge { get; set; }

    /// <summary>Ch.cpp 1d1의 실제 진단 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>수준9의 이미 존재 전투 진단 장치다.</summary>
    public Action<string>? Log { get; set; }

    /// <summary>같은 현재 전투 TYPE을 객체 생성 직전에 읽는다.</summary>
    public Func<uint>? BattleType { get; set; }

    /// <summary>현재 TYPE/flags2의 실제 공장 장치다.</summary>
    public Func<uint, uint, uint>? Create { get; set; }

    /// <summary>생성한 같은 50BYTE 객체의 현재 물리다.</summary>
    public Func<uint, Memory<byte>>? Raw { get; set; }

    /// <summary>현재 종류 WORD 장치다.</summary>
    public Action<uint, uint>? SetKind { get; set; }

    /// <summary>생성 객체의 가상 부모 장치다.</summary>
    public Action<uint, float, float, uint>? Pop { get; set; }

    /// <summary>같은 객체/지연10 Rig 생성 장치다.</summary>
    public Action<uint, uint>? Animate { get; set; }
}

// Ch.cpp의 전투 기록/그림 객체 생성과 23h 추가 수신의 실제 부모다.
public static class ClientMetaBattleCreate
{
    /// <summary>원문 0x23 메타 전투 생성 종류다. 0.958에서 추가됐다.</summary>
    public const uint Type = 0x23;

    /// <summary>원문 0x23 수신의 실제 고정 F 주소다. 0.958에서 추가됐다.</summary>
    public const uint NativeEntry = 0x004b4640;

    /// <summary>원문 0x23 수신의 실제 고정 길이다. 0.958에서 추가됐다.</summary>
    public const ushort FixedLength = 20;

    /// <summary>기록 식별자 필드의 유효 바이트 수다. 0.958에서 추가됐다.</summary>
    private const int IdentityBytes = 6;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.958에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 전투 생성의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.958에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 전투 생성의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.958에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 전투 생성의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>입력 전체는 값으로 캡처하여 하위 효과가 caller 입력을 변경하더라도 원문 스택 인자를 유지한다. 0.958에서 추가됐다.</summary>
    /// <param name="record">같은 실제 전투 기록이다.</param>
    /// <param name="input">값으로 유지하는 생성 인자다.</param>
    /// <param name="hooks">실제 진단·공장·표시 장치다.</param>
    public static void Construct(Memory<byte> record, ClientMetaBattleCreateData input, ClientMetaBattleCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (input.Identity.Length != IdentityBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(input), "메타 전투 생성의 실제 식별자 6BYTE 부족");
        }

        Span<byte> bytes = record.Span;
        if ((Read(bytes, 0, 1) & 1u) != 0)
        {
            NeedDelegate(hooks.Assertion)("!isValid()", ".\\Ns\\O\\Ch.cpp", 0x1d1);
        }

        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(4, 4), input.Id);
        input.Identity.AsSpan(0, 4).CopyTo(bytes.Slice(8, 4));
        input.Identity.AsSpan(4, 2).CopyTo(bytes.Slice(0xc, 2));
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x10, 4), input.PositionX);
        bytes[0] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x14, 4), input.PositionY);
        // 원문 여덟 참가자 식별자 DWORD를 같은 순서로만 지운다.
        for (uint slot = 0; slot < 8; slot++)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x38 + unchecked((int)slot) * 4, 4), 0);
        }

        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x18, 4), 0xffffffffu);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x1c, 4), 0x100u);
        uint type = NeedDelegate(hooks.BattleType)();
        uint created = NeedDelegate(hooks.Create)(type, 2);
        NeedDelegate(hooks.SetKind)(created, Read(bytes, 4));
        Memory<byte> raw = NeedDelegate(hooks.Raw)(created);
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Span.Slice(8, 2), 0);
        raw.Span[0x1a] = 0;
        uint y = Read(bytes, 0x14);
        uint x = Read(bytes, 0x10);
        NeedDelegate(hooks.Pop)(created, BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y), 0);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(0x20, 4), created);
        if (input.Existing == 0)
        {
            NeedDelegate(hooks.Animate)(created, 10);
        }
    }

    /// <summary>첫 식별자/active 관문 뒤에는 같은 표시/y/x/식별자 DWORD·WORD를 읽는다. 0.958에서 추가됐다.</summary>
    /// <param name="hooks">실제 CS·진단·공장·표시 장치다.</param>
    /// <param name="packet">원문 0x23 패킷이다.</param>
    public static void Receive(ClientMetaBattleCreateHooks hooks, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        ClientChallengeGlobals challenge = Need(hooks.Challenge);
        ReadOnlySpan<byte> bytes = packet.Span;
        uint id = Read(bytes, 4, 1);
        Memory<byte> record = challenge.Battle(id);
        if ((Read(record.Span, 0, 1) & 1u) != 0)
        {
            NeedDelegate(hooks.Log)("ERROR in handleChalBattleAdd: battle " + id.ToString(System.Globalization.CultureInfo.InvariantCulture) + " already exists!\n");
            return;
        }

        byte[] identity = new byte[IdentityBytes];
        BinaryPrimitives.WriteUInt32LittleEndian(identity.AsSpan(0, 4), Read(bytes, 6));
        BinaryPrimitives.WriteUInt16LittleEndian(identity.AsSpan(4, 2), unchecked((ushort)Read(bytes, 10, 2)));
        var input = new ClientMetaBattleCreateData(id, identity, Read(bytes, 0xc), Read(bytes, 0x10), Read(bytes, 5, 1) & 2u);
        Construct(record, input, hooks);
    }

    /// <summary>0x23의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.958에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 CS·진단·공장·표시 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientMetaBattleCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 40b380은 비NULL 빈 observer이며 원문 고정 길이는 20이다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        // 같은 mutable frame을 실제 생성 본문에 전달한다.
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (_, _, packet) => Receive(hooks, packet));
    }
}
